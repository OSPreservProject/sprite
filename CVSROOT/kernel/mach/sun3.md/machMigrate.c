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
typedef struct {
    Mach_UserState userState;		/* The contiguous machine-dependent
					 * user state. */
    int	pc;				/* PC at which to resume execution. */
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

    bcopy((Address) &machStatePtr->userState, (Address) &migPtr->userState,
	    sizeof(Mach_UserState));
    migPtr->pc = machStatePtr->userState.excStackPtr->pc;
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

#ifdef sun3
    if (migPtr->userState.trapFpuState.version != 0) {
	if (mach68881Present == FALSE) {
	    /*
	     *  This machine has no fpu, and this process needs one.
	     *  So we can't accept it.
	     */
	    printf("Mach_DeencapState: warning: host %d tried to migrate",
		procPtr->peerHostID);
	    printf(" a process that uses the fpu.  This host has no fpu.\n");
	    return PROC_MIGRATION_REFUSED;
	}
	if (migPtr->userState.trapFpuState.state != MACH_68881_IDLE_STATE) {
	    printf("Mach_DeencapState: warning: host %d tried to migrate",
		procPtr->peerHostID);
	    printf(" a process with a non-idle fpu onto this host.  ");
	    printf("The fpu was in state 0x%02x\n",
		migPtr->userState.trapFpuState.state);
	    return PROC_MIGRATION_REFUSED;
	}
	if (migPtr->userState.trapFpuState.version != mach68881Version) {
	    /*
	     *  The sending host has a different version of the
	     *  mc68881 fpu.  The state frames are incompatible
	     *  between versions.  But since it is in idle state
	     *  we can just use a generic idle state frame, rather
	     *  than the one we were sent.
	     */
	    migPtr->userState.trapFpuState = mach68881IdleState;
	}
    }
#endif

    /*
     * Get rid of the process's old machine-dependent state if it exists.
     */
    if (procPtr->machStatePtr != (Mach_State *) NIL) {
	Mach_FreeState(procPtr);
    }

    /*
     * This procedure relies on the fact that Mach_SetupNewState
     * only looks at the Mach_UserState part of the Mach_State structure
     * it is given.  Therefore, we can coerce the pointer to a Mach_State
     * pointer and give it to Mach_UserState to get registers & such.
     */

    status = Mach_SetupNewState(procPtr, (Mach_State *) &migPtr->userState,
				Proc_ResumeMigProc, (Address)migPtr->pc, TRUE);
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
 *	starting a migration.
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
    int stackFormat;
    Boolean okay;

    okay = TRUE;

#ifdef sun2
    /*
     * We have trouble getting the pc from the 68010 bus fault stack,
     * but it seems okay for others.
     */
    stackFormat = procPtr->machStatePtr->userState.excStackPtr->vor.stackFormat;
    if (stackFormat == MACH_MC68010_BUS_FAULT) {
	okay = FALSE;
    }
#endif

#ifdef sun3
    /*
     *  If the floating point state is busy, that means that a
     *  floating point instruction is suspended, waiting to be
     *  restarted.  It cannot be restarted on a machine with another
     *  version of the chip, since the microcode is incompatible between
     *  revisions.  So we will delay the migration until the instruction
     *  completes.
     */
    if (procPtr->machStatePtr->userState.trapFpuState.state >=
        MACH_68881_BUSY_STATE) {
	okay = FALSE;
    }
#endif

    if (proc_MigDebugLevel > 4) {
	printf("Mach_CanMigrate called. PC %x, stackFormat %x, returning %d.\n",
		   procPtr->machStatePtr->userState.excStackPtr->pc,
		   stackFormat, okay);
    }
    return(okay);
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
    return(procPtr->machStatePtr->userState.lastSysCall);
}
