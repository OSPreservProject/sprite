/*
 *  procDebug.c --
 *
 *	Routines to debug a process.  This file maintains a monitor that 
 *	synchronizes access to the debug list.  Routines in this monitor
 *	are responsible for the following fields in the proc table:
 *
 *	    1) Process state can go to PROC_DEBUGABLE.
 *	    2) PROC_DEBUGGED, PROC_SINGLE_STEP and PROC_DEBUG_WAIT can be set 
 *	       in the genFlags field.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"
#include "status.h"
#include "sync.h"
#include "sched.h"
#include "sys.h"
#include "list.h"
#include "mem.h"
#include "vm.h"
#include "byte.h"

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
    Address			bufferPtr;
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
	    (procPtr->state != PROC_SUSPENDED && 
	     procPtr->state != PROC_DEBUGABLE)) {
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
		     * The pid they gave us either doesn't exist or is 
		     * exiting.
		     */
		    if (procPtr != (Proc_ControlBlock *) NIL) {
			Proc_Unlock(procPtr);
		    }
		    UNLOCK_MONITOR;
		    return(PROC_INVALID_PID);
		}

		procPtr->genFlags |= PROC_DEBUG_WAIT;

		if (procPtr->state == PROC_SUSPENDED) {
		    /*
		     * The process that we want to debug is suspended.
		     * Send it another stop signal so that we can debug it.
		     * The act of signaling it will wake it up so that it
		     * can be suspended again.  This time when it gets 
		     * suspended it will realize that we are waiting and
		     * enter the debug state instead of the suspended state.j
		     */
		    Sig_SendProc(procPtr, procPtr->termStatus, SIG_NO_CODE);
		} else if (procPtr->state == PROC_DEBUGABLE) {
		    procPtr->genFlags &= ~PROC_DEBUG_WAIT;
		    procPtr->genFlags |= PROC_DEBUGGED;
		    List_Remove((List_Links *) procPtr);
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
            procPtr->genFlags |= PROC_SINGLE_STEP;
	    /* Fall through to ... */
	    
	case PROC_CONTINUE:
	    procPtr->genFlags 	&= ~PROC_DEBUGGED;
	    Sched_MakeReady(procPtr);
	    break;

	case PROC_GET_DBG_STATE:

	    debugState.processID	= procPtr->processID;
	    debugState.termReason	= procPtr->termReason;
	    debugState.termStatus	= procPtr->termStatus;
	    debugState.termCode		= procPtr->termCode;
	    for (i = 0; i < PROC_NUM_GENERAL_REGS; i++) {
		debugState.genRegs[i]	= procPtr->genRegs[i];
	    }
	    debugState.progCounter	= procPtr->progCounter;
	    debugState.statusReg	= procPtr->statusReg;
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
		procPtr->progCounter = debugState.progCounter;
		for (i = 0; i < PROC_NUM_GENERAL_REGS; i++) {
		    procPtr->genRegs[i]	= debugState.genRegs[i];
		}
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
		bufferPtr = (Address) Mem_Alloc(numBytes);
		/*
		 * Read from the debuggee to a kernel buffer.
		 */
		if (Vm_CopyInProc(procPtr, numBytes, srcAddr, 
				  (Address) bufferPtr) != SUCCESS) {
		    status = SYS_ARG_NOACCESS;
		/*
		 * Write the buffer to the requestor.
		 */
		} else if (Vm_CopyOut(numBytes, (Address) bufferPtr, 
				      destAddr) != SUCCESS) {
		    status = SYS_ARG_NOACCESS;
		}
		Mem_Free(bufferPtr);
	    }

	    break;

	case PROC_WRITE:
	    if (numBytes > MAX_REQUEST_SIZE) {
		status = SYS_INVALID_ARG;
		break;
	    }

	    bufferPtr = (Address) Mem_Alloc(numBytes);
	    /*
	     * Read from the requestor to a kernel buffer.
	     */
	    if (Vm_CopyIn(numBytes, srcAddr, (Address) bufferPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    } else {
		/*
		 * Make sure that the range of bytes is writable.
		 */
		Vm_ChangeCodeProt(procPtr, destAddr, numBytes, VM_URW_PROT);
		/*
		 * Write the buffer to the debuggee.
		 */
		if (Vm_CopyOutProc(procPtr, numBytes, 
				    (Address) bufferPtr, destAddr) != SUCCESS) {
		    status = SYS_ARG_NOACCESS;
		}
		/*
		 * Change the protection back.
		 */
		Vm_ChangeCodeProt(procPtr, destAddr, numBytes, VM_UR_PROT);
	    }
	    Mem_Free(bufferPtr);

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
 * Proc_PutOnDebugList --
 *
 *	This routine puts a process to the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process state is changed and the process is put on
 *	the debug list. A context switch is performed to the debugable state.
 *
 *----------------------------------------------------------------------
 */
 
void
Proc_PutOnDebugList(procPtr, sigNum, statusReg)
    register	Proc_ControlBlock	*procPtr;	/* Process to put on the
							 * debug list. */
    int					sigNum;		/* Signal that caused 
							 * process to go on 
							 * list. */
    short				statusReg;	/* Status register of
							 * the process. */
{
    LOCK_MONITOR;

    procPtr->statusReg	= statusReg;
    procPtr->termReason	= PROC_TERM_SIGNALED;
    procPtr->termStatus	= sigNum;
    procPtr->termCode	= procPtr->sigCodes[sigNum];

    if (procPtr->genFlags & PROC_FOREIGN) {
	Sys_Panic(SYS_FATAL, "Migrated process being placed on debug list.\n");
    } else {
	/*
	 * Detach the debugable process from its parent.
	 */
	Proc_DetachInt(procPtr);
    }

    List_Insert((List_Links *) procPtr, LIST_ATREAR(debugList));
    Sync_Broadcast(&debugListCondition);

    UNLOCK_MONITOR_AND_SWITCH(PROC_DEBUGABLE);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_TakeOffDebugList --
 *
 *	This routine removes the given process from the debug list and makes
 *	the process runnable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is removed from the debug list and made runnable.
 *
 *----------------------------------------------------------------------
 */
 
ENTRY void
Proc_TakeOffDebugList(procPtr)
    register	Proc_ControlBlock	*procPtr;	/* Process to remove
							 * from list. */
{
    LOCK_MONITOR;

    if (procPtr->state == PROC_DEBUGABLE) {
	List_Remove((List_Links *) procPtr);
	Sched_MakeReady(procPtr);
	Sync_Broadcast(&debugListCondition);
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
