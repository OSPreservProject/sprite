/* 
 * machMigrate.c --
 *
 *     	Machine dependent code to support process migration.  These routines
 *     	encapsulate and deencapsulate the machine-dependent state of a
 *	process and set up the state of the process on its new machine.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "machConst.h"
#include "machInt.h"
#include "mach.h"
#include "machMon.h"
#include "sched.h"
#include "procMigrate.h"

/*
 * The information that is transferred between two machines.
 */
typedef struct MigratedState {
    Mach_State 		state;		/* The contiguous machine-dependent
					 * state. */
    Mach_RegState	trapRegs;	/* The pointer to trapRegs in state
					 * should be set to point to this in the
					 * deencap routine.  It usually
					 * points to the kernel stack, but
					 * this won't exist yet on a
					 * migrated process. */
} MigratedState;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_EncapState --
 *
 *	Copy the machine-dependent information for a process into
 *	a buffer.  The buffer passed to the routine must contain space for
 *	a MigratedState structure, the size of which is accessible via 
 *	another procedure.  
 *
 * Results:
 *  	SUCCESS.
 *	The buffer is filled with the user state and PC of the process.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Mach_EncapState(procPtr, hostID, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address buffer;			   /* Pointer to allocated buffer */
{
    Mach_State *machStatePtr = procPtr->machStatePtr;
    MigratedState *migPtr = (MigratedState *) buffer;

    if (machStatePtr->fpuStatus & MACH_FPU_ACTIVE) {
	if (proc_MigDebugLevel > 2) {
	    printf("Mach_EncapState: FPU was active, dumping state.\n");
	}
	MachFPUDumpState(machStatePtr->trapRegs);
    }
    bcopy((Address) machStatePtr, (Address) &migPtr->state,
	    sizeof (Mach_State));
    bcopy((Address) machStatePtr->trapRegs, (Address) &migPtr->trapRegs,
	    sizeof (Mach_RegState));
    /*
     * The trapRegs ptr will be set to point to the trapRegs area of the
     * migrated state on the other machine.  For now, make sure any reference
     * to it will panic.
     */
    migPtr->state.trapRegs = (Mach_RegState *) NIL;

    return(SUCCESS);
}    
    

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_DeencapState --
 *
 *	Copy the machine-dependent information for a process from
 *	a buffer.  The buffer passed to the routine must contain
 *	a MigratedState structure created by Mach_EncapState on the
 *	machine starting a migration.  
 *
 * Results:
 *	The user state and PC of the process are initialized from the
 *	encapsulated information, and the other standard process
 *	initialization operations are performed (by the general initialization
 *	procedure).  The status from that procedure is returned.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Mach_DeencapState(procPtr, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address buffer;			  /* buffer containing data */
{
    MigratedState *migPtr = (MigratedState *) buffer;
    ReturnStatus status;

    if (infoPtr->size != sizeof(MigratedState)) {
	if (proc_MigDebugLevel > 0) {
	    printf("Mach_DeencapState: warning: host %d tried to migrate",
		procPtr->peerHostID);
	    printf(" onto this host with wrong structure size.");
	    printf("  Ours is %d, theirs is %d.\n",
		sizeof(MigratedState), infoPtr->size);
	}
	return(PROC_MIGRATION_REFUSED);
    }


    /*
     * Get rid of the process's old machine-dependent state if it exists.
     */
    if (procPtr->machStatePtr != (Mach_State *) NIL) {
	Mach_FreeState(procPtr);
    }

#ifdef WRONG
    /*
     * This procedure relies on the fact that Mach_SetupNewState
     * only looks at the Mach_UserState part of the Mach_State structure
     * it is given.  Therefore, we can coerce the pointer to a Mach_State
     * pointer and give it to Mach_UserState to get registers & such.
     */
#endif

    if (migPtr->state.savedMask != 0) {
	panic("Mach_DeencapState: saved window state of proc isn't empty.\n");
    }
    /*
     * Set trapRegs to the trapRegs area of the migrated state.
     */
    migPtr->state.trapRegs = &migPtr->trapRegs;

/*
 * Note - I could pass the pc explicitly, but I don't think I have to since
 * Mach_SetupNewState will get it from trap regs.
 */
    status = Mach_SetupNewState(procPtr, (Mach_State *) &migPtr->state,
	    Proc_ResumeMigProc, (Address)NIL, TRUE);

    /*
     * Mach_SetupNewState thinks that all new processes have a clean FPU
     * slate, and it zeroes the mach state.  Make sure to keep any
     * pending exceptions.
     */
    if (proc_MigDebugLevel > 2) {
	printf("Mach_DeencapState: FPU status register was %x.\n",
	       migPtr->state.fpuStatus);
    }
    if (migPtr->state.fpuStatus) {
	procPtr->machStatePtr->fpuStatus = migPtr->state.fpuStatus;
	procPtr->specialHandling = 1;
    }
	
	
    return(status);
}    
    


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetEncapSize --
 *
 *	Return the size of the encapsulated machine-dependent data.
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

/* ARGSUSED */
ReturnStatus
Mach_GetEncapSize(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    infoPtr->size = sizeof(MigratedState);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_CanMigrate --
 *
 *	Indicate whether a process's trapstack is in a form suitable for
 *	starting a migration.  We require that nextPc follow pc -- if
 * 	we just did a jump, then we defer migration momentarily.
 *
 * Results:
 *	TRUE if we can migrate using this trapstack, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
Mach_CanMigrate(procPtr)
    Proc_ControlBlock *procPtr;		/* pointer to process to check */
{
    Boolean okay;
    Mach_RegState *regsPtr;
    
    okay = TRUE;
    regsPtr = procPtr->machStatePtr->trapRegs;


    if (regsPtr->nextPc != regsPtr->pc + 4) {
	okay = FALSE;
    }
    if (proc_MigDebugLevel > 4) {
	printf("Mach_CanMigrate called. PC %x, returning %d.\n",
	       regsPtr->pc, okay);
    }
    return okay;
}    


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetLastSyscall --
 *
 *	Return the number of the last system call performed for the current
 *	process.
 *
 * Results:
 *	The system call number is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Mach_GetLastSyscall()
{
    Proc_ControlBlock *procPtr;		/* pointer to process to check */

    procPtr = Proc_GetCurrentProc();

    return(procPtr->machStatePtr->lastSysCall);
}
