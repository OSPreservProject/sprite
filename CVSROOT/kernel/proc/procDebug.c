/*
 *  procDebug.c --
 *
 *	Routines to debug a process.  This file maintains a monitor that 
 *	synchronizes access to the debug list.  Routines in this monitor
 *	are responsible for the following fields in the proc table:
 *
 *	    PROC_DEBUGGED, PROC_ON_DEBUG_LIST, PROC_SINGLE_STEP_FLAG,
 *	       and PROC_DEBUG_WAIT can be set in the genFlags field.
 *
 *	The PROC_DEBUGGED flag is set when a process is being actively debugged
 *	by a debugger.  It is not cleared until a debugger issues the
 *	PROC_DETACH_DEBUGGER debug command.  The PROC_ON_DEBUG_LIST flag is
 *	set when a process is put onto the debug queue and cleared when
 *	it is taken off.
 *
 * Copyright 1986, 1988 Regents of the University of California
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
#include "proc.h"
#include "procMigrate.h"
#include "status.h"
#include "sync.h"
#include "sched.h"
#include "sys.h"
#include "list.h"
#include "mem.h"
#include "vm.h"

static Sync_Condition	debugListCondition;	/* Condition to sleep on when
						 * waiting for a process to go
						 * onto the debug list. */
static Sync_Lock	debugLock = {0, 0};	/* Monitor lock. */
#define LOCKPTR &debugLock

List_Links	debugListHdr;
List_Links	*debugList = &debugListHdr;


/*
 *----------------------------------------------------------------------
 *
 * Proc_DebugInit --
 *
 *	Initialize the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The debug list is initialized.
 *
 *----------------------------------------------------------------------
 */

void
ProcDebugInit()
{
    List_Init(debugList);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Debug --
 *
 *	This routine is used to debug a process.
 *
 * Results:
 *	SYS_INVALID_ARG - 	buffer address was invalid.
 *	PROC_INVALID_PID - 	The pid was out-of-range or specified a
 *				non-existent process.
 *	SYS_ARG_NOACCESS - 	The buffers were not accessible.
 *
 * Side effects:
 *	The process state and address space may be updated.
 *	A process may be removed from the debug list.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_Debug(pid, request, numBytes, srcAddr, destAddr)
    Proc_PID 		pid;		/* Process id of the process to be 
					 * debugged. */
    Proc_DebugReq 	request; 	/* Type of action on the debugged 
					 * process. */
    int 		numBytes;	/* # of bytes of info to be read or 
					 * written. */
    Address 		srcAddr;	/* Location (either in caller or pid) 
					 * where info is to be read. */
    Address 		destAddr;	/* Location (either in caller or pid) 
					 * where info info is to be written. */
{
    register Proc_ControlBlock 	*procPtr;
    Proc_DebugState		debugState;
    int				i;
    ReturnStatus		status = SUCCESS;

    LOCK_MONITOR;

    /*
     * If the caller is trying to manipulate a debugged process make sure that
     * the process is actually in the debug state.
     */
    if (request != PROC_GET_NEXT_DEBUG && request != PROC_GET_THIS_DEBUG) {
	procPtr = Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL || 
	    !(procPtr->genFlags & PROC_DEBUGGED) ||
	    (procPtr->genFlags & PROC_ON_DEBUG_LIST) ||
	    procPtr->state != PROC_SUSPENDED) {
	    if (procPtr != (Proc_ControlBlock *) NIL) {
		Proc_Unlock(procPtr);
	    }
	    UNLOCK_MONITOR;
	    return (PROC_INVALID_PID);
	}
    }

    switch (request) {
	case PROC_GET_THIS_DEBUG:
	    /*
	     * Look for a specific process on the debug list. If it isn't
	     * there, wait for it to get on the list.
	     */
	    while (TRUE) {
		procPtr = Proc_LockPID(pid);
		if (procPtr == (Proc_ControlBlock *) NIL ||
		    (procPtr->genFlags & PROC_DIEING)) {
		    /*
		     * The pid they gave us either doesn't exist or the
		     * corresponding process is exiting.
		     */
		    if (procPtr != (Proc_ControlBlock *) NIL) {
			Proc_Unlock(procPtr);
		    }
		    UNLOCK_MONITOR;
		    return(PROC_INVALID_PID);
		}

		procPtr->genFlags |= PROC_DEBUG_WAIT;

		if (procPtr->state == PROC_SUSPENDED) {
		    procPtr->genFlags &= ~PROC_DEBUG_WAIT;
		    procPtr->genFlags |= PROC_DEBUGGED;
		    if (procPtr->genFlags & PROC_ON_DEBUG_LIST) {
			List_Remove((List_Links *) procPtr);
			procPtr->genFlags &= ~PROC_ON_DEBUG_LIST;
		    }
		    break;
		}

		Proc_Unlock(procPtr);
		if (Sync_Wait(&debugListCondition, TRUE)) {
		    Proc_Lock(procPtr);
		    procPtr->genFlags &= ~PROC_DEBUG_WAIT;
		    Proc_Unlock(procPtr);
		    status = GEN_ABORTED_BY_SIGNAL;
		    break;
		}
	    }
	    break;
	    
	case PROC_GET_NEXT_DEBUG: {
	    Boolean	sigPending = FALSE;

	    /*
	     * Loop through the list of debuggable processes, looking for the 
	     * first one that hasn't been debugged yet. Wait until one is found.
	     */
	    while (!sigPending) {
		if (!List_IsEmpty(debugList)) {
		    procPtr = (Proc_ControlBlock *) List_First(debugList);
		    Proc_Lock(procPtr);
		    procPtr->genFlags |= PROC_DEBUGGED;
		    List_Remove((List_Links *) procPtr);
		    procPtr->genFlags &= ~PROC_ON_DEBUG_LIST;
		    break;
		}
		sigPending = Sync_Wait(&debugListCondition, TRUE);
	    }
	    if (!sigPending) {
		if ((Vm_CopyOut(sizeof(Proc_PID), 
			  (Address) &procPtr->processID, destAddr)) != SUCCESS){
		    status = SYS_ARG_NOACCESS;
		}
	    } else {
		status = GEN_ABORTED_BY_SIGNAL;
	    }
	    break;
	}

	case PROC_SINGLE_STEP:
            procPtr->genFlags |= PROC_SINGLE_STEP_FLAG;
	    procPtr->specialHandling = 1;
	    /* Fall through to ... */
	    
	case PROC_CONTINUE:
	    Sched_MakeReady(procPtr);
	    break;

	case PROC_GET_DBG_STATE:

	    debugState.processID	= procPtr->processID;
	    debugState.termReason	= procPtr->termReason;
	    debugState.termStatus	= procPtr->termStatus;
	    debugState.termCode		= procPtr->termCode;
	    Mach_GetDebugState(procPtr, &debugState);
	    debugState.sigHoldMask	= procPtr->sigHoldMask;
	    debugState.sigPendingMask	= procPtr->sigPendingMask;
	    for (i = 0; i < SIG_NUM_SIGNALS; i++) {
		debugState.sigActions[i]	= procPtr->sigActions[i];
		debugState.sigMasks[i]		= procPtr->sigMasks[i];
		debugState.sigCodes[i]		= procPtr->sigCodes[i];
	    }

	    if (Vm_CopyOut(sizeof(Proc_DebugState), (Address) &debugState, 
			   destAddr)) {
		status = SYS_ARG_NOACCESS;
	    }
	    break;

	case PROC_SET_DBG_STATE:

	    if (Vm_CopyIn(sizeof(Proc_DebugState), srcAddr, 
				(Address) &debugState)) {
		status = SYS_ARG_NOACCESS;
	    } else {
		Mach_SetDebugState(procPtr, &debugState);
		Sig_ChangeState(procPtr, debugState.sigActions,
				debugState.sigMasks, debugState.sigPendingMask,
				debugState.sigCodes, debugState.sigHoldMask);
	    }
	    break;

#define MAX_REQUEST_SIZE 16384

	case PROC_READ:
	    if (numBytes > MAX_REQUEST_SIZE) {
		status = SYS_INVALID_ARG;
	    } else {
		/*
		 * Read from the debuggee to the debugger.
		 */
		status = Vm_CopyInProc(numBytes, procPtr, srcAddr, 
				       destAddr, FALSE);
	    }

	    break;

	case PROC_WRITE:
	    if (numBytes > MAX_REQUEST_SIZE) {
		status = SYS_INVALID_ARG;
		break;
	    }

	    /*
	     * Make sure that the range of bytes is writable.
	     */
	    Vm_ChangeCodeProt(procPtr, destAddr, numBytes, TRUE);
	    /*
	     * Write from the debugger to the debuggee.
	     */
	    status = Vm_CopyOutProc(numBytes, srcAddr, FALSE,
				    procPtr, destAddr);
	    /*
	     * Change the protection back.
	     */
	    Vm_ChangeCodeProt(procPtr, destAddr, numBytes, FALSE);

	    break;

	case PROC_DETACH_DEBUGGER:
	    /*
	     * Detach from this process.  This has the side effect of 
	     * continuing the process as if a resume signal had been sent.
	     */
	    procPtr->genFlags &= ~(PROC_DEBUGGED | PROC_DEBUG_WAIT);
	    Sched_MakeReady(procPtr);
	    procPtr->termReason = PROC_TERM_RESUMED;
	    procPtr->termStatus = SIG_RESUME;
	    procPtr->termCode = SIG_NO_CODE;
	    Proc_InformParent(procPtr, PROC_RESUME_STATUS, FALSE);
	    break;

	default:
	    status = SYS_INVALID_ARG;
	    break;
    }

    if (status != GEN_ABORTED_BY_SIGNAL) {
	Proc_Unlock(procPtr);
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SuspendProcess --
 *
 *	Put the process into the suspended state.  If the process is
 *	entering the suspended state because of a bug and no process is 
 *	debugging it then put it onto the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process state is changed and the process may be put onto
 *	the debug list. A context switch is performed to the suspend state.
 *
 *----------------------------------------------------------------------
 */
void
Proc_SuspendProcess(procPtr, debug, termReason, termStatus, termCode)
    register	Proc_ControlBlock	*procPtr;	/* Process to put on the
							 * debug list. */
    Boolean				debug;		/* TRUE => this process
							 * is being suspended
							 * because of an 
							 * error. */
    int					termReason;	/* Reason why process
							 * went to this state.*/
    int					termStatus;	/* Termination status.*/
    int					termCode;	/* Termination code. */
{
    LOCK_MONITOR;

    procPtr->termReason	= termReason;
    procPtr->termStatus	= termStatus;
    procPtr->termCode	= termCode;

    if (debug && (procPtr->genFlags & PROC_FOREIGN) &&
	proc_KillMigratedDebugs) {
	if (proc_MigDebugLevel > 0) {
	    panic("Migrated process being placed on debug list.\n");
	}
    }

    if (debug) {
	if (!(procPtr->genFlags & PROC_DEBUGGED)) {
	    /*
	     * If the process isn't currently being debugged then it goes on 
	     * the debug list and its parent is notified of a state change.
	     */
	    List_Insert((List_Links *) procPtr, LIST_ATREAR(debugList));
	    Proc_Lock(procPtr);
	    procPtr->genFlags |= PROC_ON_DEBUG_LIST;
	    Proc_Unlock(procPtr);
	    Proc_InformParent(procPtr, PROC_SUSPEND_STATUS, FALSE);
	    Sync_Broadcast(&debugListCondition);
	} else if (procPtr->genFlags & PROC_DEBUG_WAIT) {
	    /*
	     * A process is waiting for this process so wake it up.
	     */
	    Sync_Broadcast(&debugListCondition);
	}
    } else {
	/*
	 * The process is being suspended.  Notify the parent and then wakeup
	 * anyone waiting for this process to enter the debug state.
	 */
	Proc_InformParent(procPtr, PROC_SUSPEND_STATUS, FALSE);
	if (procPtr->genFlags & PROC_DEBUG_WAIT) {
	    Sync_Broadcast(&debugListCondition);
	}
    }

    UNLOCK_MONITOR_AND_SWITCH(PROC_SUSPENDED);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ResumeProcess --
 *
 *	Resume execution of the given process.  It is assumed that this 
 *	procedure is called with the process table entry locked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process may be made runnable and may be removed from the debug list.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Proc_ResumeProcess(procPtr, killingProc)
    register	Proc_ControlBlock	*procPtr;	/* Process to remove
							 * from list. */
    Boolean				killingProc;	/* This process is
							 * being resumed for
							 * the purpose of 
							 * killing it. */
{
    LOCK_MONITOR;

    if (procPtr->state == PROC_SUSPENDED &&
        (killingProc || !(procPtr->genFlags & PROC_DEBUGGED))) {
	/*
	 * Only processes that are currently suspended and are either being
	 * killed or aren't being actively debugged can be resumed.
	 */
	if (procPtr->genFlags & PROC_ON_DEBUG_LIST) {
	    List_Remove((List_Links *) procPtr);
	}
	if (procPtr->genFlags & PROC_DEBUG_WAIT) {
	    Sync_Broadcast(&debugListCondition);
	}
	procPtr->genFlags &= ~(PROC_ON_DEBUG_LIST | PROC_DEBUG_WAIT);
	Sched_MakeReady(procPtr);
	if (!killingProc) {
	    procPtr->termReason = PROC_TERM_RESUMED;
	    procPtr->termStatus = SIG_RESUME;
	    procPtr->termCode = SIG_NO_CODE;
	    /*
	     * The parent is notified in background because we are called
	     * by the signal code as part of the act of sending a signal
	     * and if a SIG_CHILD happens now we will have deadlock.
	     */
	    Proc_InformParent(procPtr, PROC_RESUME_STATUS, TRUE);
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_DebugWakeup --
 *
 *	Wakeup any processes waiting on the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ProcDebugWakeup()
{
    LOCK_MONITOR;
    Sync_Broadcast(&debugListCondition);
    UNLOCK_MONITOR;
}
