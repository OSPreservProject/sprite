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
 * Copyright 1986, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procDebug.c,v 1.10 92/03/12 17:36:15 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <mach_error.h>
#include <mach/exception.h>
#include <mach/message.h>
#include <status.h>

#include <proc.h>
#include <procInt.h>
#include <procMachInt.h>
#include <sig.h>
#include <sigMach.h>
#include <sync.h>
#include <sys.h>
#include <vm.h>

Sync_Condition	debugListCondition;	/* Condition to sleep on when
					 * waiting for a process to go
					 * onto the debug list. */

static Sync_Lock debugLock; 			/* Monitor lock. */
#define LOCKPTR &debugLock

List_Links	debugListHdr;
List_Links	*debugList = &debugListHdr;

/* Forward declarations: */

static	ENTRY	void		AddToDebugList _ARGS_((
				    Proc_LockedPCB *procPtr));
static	ENTRY	void		RemoveFromDebugList _ARGS_((
				    Proc_LockedPCB *procPtr));
static	ENTRY	ReturnStatus	ProcGetThisDebug _ARGS_((Proc_PID pid,
				    Proc_ControlBlock **procPtrPtr));
static	ENTRY	ReturnStatus	ProcGetNextDebug _ARGS_((Address destAddr,
				    Proc_ControlBlock **procPtrPtr));

static		Boolean		HandleStackFault _ARGS_((
				    Proc_LockedPCB *procPtr,
				    kern_return_t kernFaultStatus, 
				    Address faultAddress));


/*
 *----------------------------------------------------------------------
 *
 * ProcDebugInit --
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
    Sync_LockInitDynamic(&debugLock, "Proc:debugLock");
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Debug --
 *
 *	This routine is used to debug a process. This routine is not
 *	inside the monitor. 
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

ReturnStatus
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
    register Proc_ControlBlock 	*procPtr = (Proc_ControlBlock *) NIL;
#ifdef SPRITED_USERDEBUG
    Proc_DebugState		debugState;
    int				i;
#endif
    ReturnStatus		status = SUCCESS;
    Proc_ControlBlock		*tProcPtr;

#ifndef SPRITED_USERDEBUG
    srcAddr = srcAddr;		/* lint */
    numBytes = numBytes;	/* lint */
#endif

    /*
     * If the caller is trying to manipulate a debugged process make sure that
     * the process is actually in the debug state.
     */
    if (request != PROC_GET_NEXT_DEBUG && request != PROC_GET_THIS_DEBUG) {
	procPtr = (Proc_ControlBlock *)Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL || 
	    !(procPtr->genFlags & PROC_DEBUGGED) ||
	    (procPtr->genFlags & PROC_ON_DEBUG_LIST) ||
	    procPtr->state != PROC_SUSPENDED ||
	    ((procPtr->genFlags & PROC_KILLING) && request==PROC_CONTINUE)) {
	    if (procPtr != (Proc_ControlBlock *) NIL) {
		Proc_Unlock(Proc_AssertLocked(procPtr));
	    }
	    return (PROC_INVALID_PID);
	}
    }

    switch (request) {
	case PROC_GET_THIS_DEBUG:
	    status = ProcGetThisDebug(pid, &tProcPtr);
	    procPtr = tProcPtr;
	    break;
	    
	case PROC_GET_NEXT_DEBUG: {
	    status = ProcGetNextDebug(destAddr, &tProcPtr);
	    procPtr = tProcPtr;
	    break;
	}

#ifdef SPRITED_USERDEBUG
	case PROC_SINGLE_STEP:
            procPtr->genFlags |= PROC_SINGLE_STEP_FLAG;
	    procPtr->specialHandling = 1;
	    /* Fall through to ... */
	    
	case PROC_CONTINUE:
	    /* 
	     * XXX if the process went into the debugger because of an 
	     * exception, you need some way to get the 
	     * catch_exception_raise thread to unwind and return, so that 
	     * the process can resume execution.
	     */
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

	    /* XXX Need to do thread_abort() here? */
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
	    Vm_FlushCode(procPtr, destAddr, numBytes);

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
	    Proc_InformParent(procPtr, PROC_RESUME_STATUS);
	    break;
#endif /* SPRITED_USERDEBUG */

	default:
	    status = SYS_INVALID_ARG;
	    break;
    }

    if (status != GEN_ABORTED_BY_SIGNAL && status != PROC_INVALID_PID) {
	Proc_Unlock(Proc_AssertLocked(procPtr));
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SuspendProcess --
 *
 *	Put the process into the suspended state.  If the process is
 *	entering the suspended state because of a bug and no process is
 *	debugging it then put it onto the debug list.  If the process is
 *	already suspended, just do the bookkeeping and parent notification.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process state is changed and the process may be put onto
 *	the debug list.  If the process is suspending itself, then the
 *	process is unlocked and a context switch is performed to the
 *	suspend state.
 *
 *----------------------------------------------------------------------
 */
void
Proc_SuspendProcess(procPtr, debug, termReason, termStatus, termCode)
    register	Proc_LockedPCB	*procPtr;	/* Process to put on the
						 * debug list. */
    Boolean			debug;		/* TRUE => this process
						 * is being suspended
						 * because of an 
						 * error. */
    int				termReason;	/* Reason why process
						 * went to this state.*/
    int				termStatus;	/* Termination status.*/
    int				termCode;	/* Termination code. */
{
#ifdef SPRITED_MIGRATION
    Boolean foreign = (procPtr->pcb.genFlags & PROC_FOREIGN);
#endif
    kern_return_t kernStatus;

    procPtr->pcb.termReason	= termReason;
    procPtr->pcb.termStatus	= termStatus;
    procPtr->pcb.termCode	= termCode;

#ifdef SPRITED_MIGRATION
    if (debug &&  foreign &&
	proc_KillMigratedDebugs) {
	if (proc_MigDebugLevel > 0) {
	    panic("Migrated process being placed on debug list.\n");
	}
    }
#endif /* SPRITED_MIGRATION */

    if (debug) {
	if (!(procPtr->pcb.genFlags & PROC_DEBUGGED)) {
	    /*
	     * If the process isn't currently being debugged then it goes on 
	     * the debug list and its parent is notified of a state change.
	     */
	    if (!(procPtr->pcb.genFlags & PROC_ON_DEBUG_LIST)) {
		AddToDebugList(procPtr);
	    }
	    Proc_InformParent(procPtr, PROC_SUSPEND_STATUS);
	    ProcDebugWakeup();
	} else if (procPtr->pcb.genFlags & PROC_DEBUG_WAIT) {
	    /*
	     * A process is waiting for this process so wake it up.
	     */
	    ProcDebugWakeup();
	}
    } else {
	/*
	 * The process is being suspended.  Notify the parent and then wakeup
	 * anyone waiting for this process to enter the debug state.
	 */
	Proc_InformParent(Proc_AssertLocked(procPtr), PROC_SUSPEND_STATUS);
	if (procPtr->pcb.genFlags & PROC_DEBUG_WAIT) {
	    ProcDebugWakeup();
	}
    }
#ifdef SPRITED_MIGRATION
    if (foreign) {
	ProcRemoteSuspend(procPtr, PROC_SUSPEND_STATUS);
    }
#endif

    if (procPtr->pcb.state == PROC_SUSPENDED) {
	if ((Proc_ControlBlock *)procPtr == Proc_GetCurrentProc()) {
	    panic("%s: current is suspended, yet running.\n",
		  "Proc_SuspendProcess");
	}
    } else {
	kernStatus = thread_suspend(procPtr->pcb.thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("%s: couldn't suspend thread for process %x: %s.\n",
		   "Proc_SuspendProcess", procPtr->pcb.processID,
		   mach_error_string(kernStatus));
	}
	if ((Proc_ControlBlock *)procPtr == Proc_GetCurrentProc()) {
	    Proc_UnlockAndSwitch(procPtr, PROC_SUSPENDED);
	} else {
	    Proc_SetState((Proc_ControlBlock *)procPtr, PROC_SUSPENDED);
	}
    }
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
void
Proc_ResumeProcess(procPtr, killingProc)
    register	Proc_LockedPCB		*procPtr;	/* Process to remove
							 * from list. */
    Boolean				killingProc;	/* This process is
							 * being resumed for
							 * the purpose of 
							 * killing it. */
{
    if (procPtr->pcb.state == PROC_SUSPENDED &&
        (killingProc || !(procPtr->pcb.genFlags & PROC_DEBUGGED))) {
	/*
	 * Only processes that are currently suspended and are either being
	 * killed or aren't being actively debugged can be resumed.
	 */
	RemoveFromDebugList(procPtr);
	if (procPtr->pcb.genFlags & PROC_DEBUGGED) {
	    procPtr->pcb.genFlags |= PROC_KILLING;
	}
	if (procPtr->pcb.genFlags & PROC_DEBUG_WAIT) {
	    ProcDebugWakeup();
	}
	procPtr->pcb.genFlags &= ~PROC_DEBUG_WAIT;
	if (!killingProc) {
	    procPtr->pcb.termReason = PROC_TERM_RESUMED;
	    procPtr->pcb.termStatus = SIG_RESUME;
	    procPtr->pcb.termCode = SIG_NO_CODE;
	    /*
	     * The parent is notified in background because we are called
	     * by the signal code as part of the act of sending a signal
	     * and if a SIG_CHILD happens now we will have deadlock.
	     * If the process is remote, send the term flags over and
	     * let the home node handle signalling the parent.
	     */
#ifdef SPRITED_MIGRATION
	    if (procPtr->pcb.genFlags & PROC_FOREIGN) {
		ProcRemoteSuspend(procPtr, PROC_RESUME_STATUS);
	    } else {
		Proc_InformParent(procPtr, PROC_RESUME_STATUS);
	    }
#else
	    Proc_InformParent(procPtr, PROC_RESUME_STATUS);
#endif /* SPRITED_MIGRATION */
	}
	/* 
	 * Now actually restart the process.  This goes after setting the 
	 * process's status codes so that if there's a failure (e.g., the 
	 * thread has gone away), the process gets the right termination 
	 * code. 
	 */
	Proc_MakeReady(procPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ProcDebugWakeup --
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


/*
 *----------------------------------------------------------------------
 *
 * ProcGetThisDebug --
 *
 *	Get the specified process on the debug list. If it isn't there
 *	then wait for it to show up.
 *
 * Results:
 *	PROC_INVALID_PID if process doesn't exist or is dieing.
 *	GEN_ABORTED_BY_SIGNAL if wait for process was interrupted by a 
 *	    signal
 *
 *
 * Side effects:
 *	Process may be locked.
 *
 *----------------------------------------------------------------------
 */

static ENTRY ReturnStatus
ProcGetThisDebug(pid, procPtrPtr)
    Proc_PID		pid;
    Proc_ControlBlock	**procPtrPtr;
{
    register Proc_ControlBlock 	*procPtr;
    ReturnStatus		status;


    LOCK_MONITOR;
    status = SUCCESS;
    while (TRUE) {
	procPtr = (Proc_ControlBlock *)Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL ||
	    (procPtr->genFlags & PROC_DYING)) {
	    /*
	     * The pid they gave us either doesn't exist or the
	     * corresponding process is exiting.
	     */
	    if (procPtr != (Proc_ControlBlock *) NIL) {
		Proc_Unlock(Proc_AssertLocked(procPtr));
	    }
	    status = PROC_INVALID_PID;
	    goto exit;
	}
	procPtr->genFlags |= PROC_DEBUG_WAIT;

	if (procPtr->state == PROC_SUSPENDED) {
	    procPtr->genFlags &= ~PROC_DEBUG_WAIT;
	    procPtr->genFlags |= PROC_DEBUGGED;
	    if (procPtr->genFlags & PROC_ON_DEBUG_LIST) {
		List_Remove((List_Links *) procPtr);
		procPtr->genFlags &= ~PROC_ON_DEBUG_LIST;
	    }
	    goto exit;
	}

	Proc_Unlock(Proc_AssertLocked(procPtr));
	if (Sync_Wait(&debugListCondition, TRUE)) {
	    Proc_Lock(procPtr);
	    procPtr->genFlags &= ~PROC_DEBUG_WAIT;
	    Proc_Unlock(Proc_AssertLocked(procPtr));
	    status = GEN_ABORTED_BY_SIGNAL;
	    goto exit;
	}
    }
exit:
    *procPtrPtr = procPtr;
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * ProcGetNextDebug --
 *
 *	Look through the list of debuggable processes and get the
 *	first one that hasn't been debugged yet. Wait for one if
 *	there aren't any.
 *
 * Results:
 *	SYS_ARG_NOACCESS if data couldn't be copied to user address space.
 *	GEN_ABORTED_BY_SIGNAL if wait for process was interrupted by a 
 *	    signal
 *
 * Side effects:
 *	Process is removed from list, process id is copied to user variable
 *
 *----------------------------------------------------------------------
 */

static ENTRY ReturnStatus
ProcGetNextDebug(destAddr, procPtrPtr)
    Address		destAddr;
    Proc_ControlBlock	**procPtrPtr;
{
    Boolean			sigPending = FALSE;
    register Proc_ControlBlock 	*procPtr = (Proc_ControlBlock *) NIL;
    ReturnStatus		status;


    LOCK_MONITOR;

    status = SUCCESS;
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
    *procPtrPtr = procPtr;
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * AddToDebugList --
 *
 *	Adds the given process to the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process is added to list, process genFlags is modified.
 *
 *----------------------------------------------------------------------
 */

static ENTRY void
AddToDebugList(procPtr)
    register Proc_LockedPCB 	*procPtr;
{
    LOCK_MONITOR;

    List_Insert((List_Links *) procPtr, LIST_ATREAR(debugList));
    procPtr->pcb.genFlags |= PROC_ON_DEBUG_LIST;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveFromDebugList --
 *
 *	Removes the given process from the debug list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is removed from the list, process's genFlags field
 *	is modified.
 *
 *----------------------------------------------------------------------
 */

static ENTRY void
RemoveFromDebugList(procPtr)
    register Proc_LockedPCB 	*procPtr;
{
    LOCK_MONITOR;

    if (procPtr->pcb.genFlags & PROC_ON_DEBUG_LIST) {
	List_Remove((List_Links *) procPtr);
	procPtr->pcb.genFlags &= ~PROC_ON_DEBUG_LIST;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * catch_exception_raise --
 *
 *	Handle a Mach exception for a process.
 *
 * Results:
 *	Returns KERN_SUCCESS if the process should be restarted, 
 *	KERN_FAILURE otherwise.
 *
 * Side effects:
 *	Kills the process.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
catch_exception_raise(exceptionPort, thread, task, exceptionType,
		      exceptionCode, exceptionSubcode)
    mach_port_t exceptionPort;	/* the exception port for the process */
    mach_port_t thread;		/* the thread that generated the exception */
    mach_port_t task;		/* the faulting task */
    int exceptionType;		/* EXC_BAD_ACCESS, etc. */
    int exceptionCode;		/* more information */
    int exceptionSubcode;	/* even more information */
{
    Proc_ControlBlock *procPtr;
    kern_return_t returnStatus;
    int sigNum;			/* signal number */
    int code;			/* signal subcode */
    Address sigAddr;		/* responsible address */
    ReturnStatus status;
    Address userPC;
    Address userStack;		/* dummy */

    procPtr = Proc_ExceptionToPCB(exceptionPort);
    if (procPtr == NULL) {
	return KERN_FAILURE;
    }

    Proc_Lock(procPtr);
    if (procPtr->thread != thread) {
	panic("catch_exception_raise: threads don't match.\n");
    }
    if (procPtr->taskInfoPtr->task != task) {
	panic("catch_exception_raise: tasks don't match.\n");
    }
    if (procPtr->genFlags & PROC_NO_MORE_REQUESTS) {
	returnStatus = KERN_FAILURE;
	goto done;
    }

    /* 
     * If it is an address fault, see whether we just need to map in 
     * an additional stack page.  If not, convert the exception to a signal 
     * and send it.  The signals code is responsible for making any 
     * necessary changes to the thread's state.
     * XXX May want to use special-case code (instead of signal) for 
     * handling breakpoint exceptions.
     */
    if (exceptionType == EXC_BAD_ACCESS &&
		HandleStackFault(Proc_AssertLocked(procPtr), exceptionCode,
				 (Address)exceptionSubcode)) {
	returnStatus = KERN_SUCCESS;
    } else {
	status = ProcMachGetUserRegs(Proc_AssertLocked(procPtr), &userPC,
				     &userStack);
#if 0
	/* 
	 * This code is waiting for mach_mips_exception_string to get 
	 * installed in a Mach library (and for us to install the Mach 
	 * library). 
	 */
	/* XXX possible memory leak; see mach_foo_exception_string. */
	printf("catch_exception_raise: %s in pid %x, ",
	       SigMach_ExceptionString(exceptionType, exceptionCode,
				       exceptionSubcode),
	       procPtr->processID);
#else
	printf("%s: pid %x, exception %d, code %d, subcode 0x%x, ",
	       "catch_exception_raise", procPtr->processID, exceptionType,
	       exceptionCode, exceptionSubcode);
#endif
	if (status == SUCCESS) {
	    printf("PC = %x\n", userPC); 
	} else {
	    printf("PC = ???\n");
	}
	SigMach_ExcToSig(exceptionType, exceptionCode, exceptionSubcode,
			 &sigNum, &code, &sigAddr);
	(void)Sig_SendProc(Proc_AssertLocked(procPtr), sigNum, TRUE,
			   code, sigAddr);
	/* 
	 * We will take care of resuming the process, so don't return 
	 * KERN_SUCCESS to the kernel.
	 * Note: I'm told that the server need not suspend/abort/resume the
	 * thread when there's an exception because the kernel has already
	 * set things up so that the server need only change the thread's
	 * state and return (e.g., to invoke a handler).  On the other
	 * hand, the signals code needs to do the suspend/abort/resume in
	 * the general case anyway, so we might as well just use that model
	 * everywhere.
	 */
	returnStatus = MIG_NO_REPLY;
    }

 done:
    Proc_Unlock(Proc_AssertLocked(procPtr));
    return returnStatus;
}


/*
 *----------------------------------------------------------------------
 *
 * HandleStackFault --
 *
 *	Check if the exception is for a page fault in the stack.  If 
 *	it is, grow the stack.
 *
 * Results:
 *	Returns TRUE if we were able to handle the fault.
 *
 * Side effects:
 *	Might extend the process's stack segment.
 *
 *----------------------------------------------------------------------
 */

static Boolean
HandleStackFault(procPtr, kernFaultStatus, faultAddress)
    Proc_LockedPCB *procPtr;	/* the process that faulted, already locked */
    kern_return_t kernFaultStatus; /* code to describe the error */
    Address faultAddress;	/* the address that caused the fault */
{
    /* 
     * If the fault was caused by something other than an unmapped
     * memory location, we can't deal with it.
     */
    if (kernFaultStatus != KERN_INVALID_ADDRESS) {
	return FALSE;
    }

    return Vm_ExtendStack(procPtr, faultAddress) == SUCCESS;
}
