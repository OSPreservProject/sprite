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

#include <stdio.h>
#include <bstring.h>

/*
 * The information that is transferred between two machines.
 */
typedef struct MigratedState {
    Mach_RegState	trapRegs;	/* The pointer to trapRegs in state
					 * should be set to point to this in the
					 * deencap routine.  It usually
					 * points to the kernel stack, but
					 * this won't exist yet on a
					 * migrated process. */
    int			fpuStatus;	/* See Mach_State. */
    int			lastSysCall;	/* Used in sys module. */
    int			savedArgI0;	/* Arg 0 saved for sys call. */
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
	    printf("Mach_EncapState: FPU was active (fsr %x), dumping state.\n",
		   procPtr->machStatePtr->fpuStatus);
	}
	/*
	 * We may or may not have saved the floating point state from the
	 * FPU.  If an exception is pending then we must of done a 
	 * save and doing another will overwrite the exception. 
	 */
	if ((machStatePtr->fpuStatus & MACH_FPU_EXCEPTION_PENDING) == 0) {
	    MachFPUDumpState(machStatePtr->trapRegs);
	    if (machStatePtr->fpuStatus & MACH_FPU_EXCEPTION_PENDING) {
		machStatePtr->fpuStatus |= 
		    (machStatePtr->trapRegs->fsr & MACH_FSR_TRAP_TYPE_MASK);
	    }
	}
	if (proc_MigDebugLevel > 2) {
	    printf("Mach_EncapState: FPU fsr now %x.\n",
		   procPtr->machStatePtr->fpuStatus);
	}
    }
    /*
     * There must not be any saved window state for migration.  This shouldn't
     * ever happen the way things are set up.
     */
    if (machStatePtr->savedMask != 0) {
	panic("Mach_EncapState: saved window state of proc isn't empty.\n");
    }
    bcopy((Address) machStatePtr->trapRegs, (Address) &migPtr->trapRegs,
	    sizeof (Mach_RegState));
    /* Copy necessary fields from state structure. */
    migPtr->fpuStatus = machStatePtr->fpuStatus;
    migPtr->lastSysCall = machStatePtr->lastSysCall;
    migPtr->savedArgI0 = machStatePtr->savedArgI0;

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
    Mach_State	state;
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

    bzero((char *) &state, sizeof (Mach_State));
    state.switchRegs = (Mach_RegState *) NIL;
    /*
     * Set trapRegs to the trapRegs area of the migrated state.
     */
    state.trapRegs = &migPtr->trapRegs;
    /* Get other necessary fields for Mach_State structure. */
    state.fpuStatus = migPtr->fpuStatus;
    state.lastSysCall = migPtr->lastSysCall;
    state.savedArgI0 = migPtr->savedArgI0;

    /* Don't pass the pc explicitly -- it will be gotten from trapRegs. */
    status = Mach_SetupNewState(procPtr, &state,
	    Proc_ResumeMigProc, (Address)NIL, TRUE);

    /*
     * Mach_SetupNewState thinks that all new processes have a clean FPU
     * slate, and it zeroes the mach state.  Make sure to keep any
     * pending exceptions.
     */
    if (proc_MigDebugLevel > 2) {
	printf("Mach_DeencapState: FPU status register was %x.\n",
	       migPtr->fpuStatus);
    }
    if (migPtr->fpuStatus) {
	procPtr->machStatePtr->fpuStatus = migPtr->fpuStatus;
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
