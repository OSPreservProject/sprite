/* 
 * signals.c --
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * This contains routines that deal with Sprite signals.  See the man pages
 * on signals for an explanation of the Sprite signaling facilities.  The
 * only thing that is explained in these comments is the implementation of
 * these facilities.
 *
 * SYNCHRONIZATION
 *
 * Whenever the signal state of a process is modified a master lock on the
 * schedule mutex is grabed.  A master lock is used instead of a monitor lock
 * so that signals can be sent at interrupt time.  The schedule mutex is 
 * used because sending a signal requires looking at and modifying the 
 * proc table.
 *
 * When the signal state is looked at no locking is done.  It is assumed 
 * that there are two ways that the signal state will be looked at:
 *
 *      1) A process in the middle of executing a system call
 *         will check to see if any signals are pending before waiting for
 *         an extended period (i.e. waiting for a read to complete).  If
 *         not then it will go to sleep until either a signal comes in or
 *         the thing that it is waiting for completes.  This does not
 *         require any synchronization on reading because the routine
 *         which is used to put a process to sleep (see Sync_WaitEventInt)
 *         will check for signals with the master lock down before the
 *         process is put to sleep.  If there are signals pending, then
 *         the sleep call will return immediately.  Otherwise if a signal
 *         comes in after the process goes to sleep then it will be
 *         awakened by the Sig_Send.  Thus there is no way to miss a
 *         signal in this case.
 *
 *	2) A process is returning to user mode after trapping into the kernel
 *	   for some reason and it wants to see if signals are pending before
 *	   it returns.  In this case the trap handler (see Exc_Trap) will 
 *	   disable interrupts before checking to see if signals are pending.  
 *	   If they are then it will enable interrupts and process the signal.
 *	   Otherwise it will return to user mode with interrupts being enabled
 *	   on the return to user mode.  If a signal came in when 
 *	   interrupts were disabled then once interrupts are enabled the 
 *	   process will be interrupted and return back into the kernel.  
 *	   Likewise once the user process returns to user mode if a signal is 
 *	   delivered then the user process will be interrupted.  Interruption
 *	   is possible of course only on a multi-processor. Once interrupted it
 *	   will be forced back into the kernel where it will discover a
 *	   signal.  Thus a signal cannot be missed in this case either.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"
#include "sync.h"
#include "dbg.h"
#include "exc.h"
#include "list.h"
#include "proc.h"
#include "status.h"
#include "byte.h"
#include "machine.h"
#include "sync.h"
#include "sched.h"
#include "sigInt.h"
#include "rpc.h"

#define	SigGetBitMask(sig) (1 << (sig - 1))
#define	KILL_BIT_MASK	SIG_BIT_MASK(SIG_KILL)

unsigned int 	sigBitMasks[SIG_NUM_SIGNALS];
int		sigDefActions[SIG_NUM_SIGNALS];
int		sigCanHoldMask;

Sync_Lock	sigLock = {0, 0};
Sync_Condition	signalCondition;


/*
 *----------------------------------------------------------------------
 *
 * Sig_Init --
 *
 *	Initialize the signal data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The set of bit masks and the set of default actions are set up.
 *
 *----------------------------------------------------------------------
 */

void
Sig_Init()
{
    int	i;

    for (i = SIG_MIN_SIGNAL; i < SIG_NUM_SIGNALS; i++) {
	sigBitMasks[i] = SigGetBitMask(i);
	sigDefActions[i] = SIG_IGNORE_ACTION;
    }

    sigDefActions[SIG_INTERRUPT]	= SIG_KILL_ACTION;
    sigDefActions[SIG_KILL]		= SIG_KILL_ACTION;
    sigDefActions[SIG_DEBUG]		= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ARITH_FAULT]	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ILL_INST] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ADDR_FAULT] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_BREAKPOINT] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_TRACE_TRAP] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_MIGRATE_TRAP] 	= SIG_MIGRATE_ACTION;
    sigDefActions[SIG_MIGRATE_HOME] 	= SIG_MIGRATE_ACTION;
    sigDefActions[SIG_SUSPEND]		= SIG_SUSPEND_ACTION;
    sigDefActions[SIG_RESUME]		= SIG_KILL_ACTION;
    sigDefActions[SIG_TTY_INPUT]	= SIG_SUSPEND_ACTION;
    sigDefActions[SIG_PIPE]		= SIG_KILL_ACTION;
    sigDefActions[SIG_TIMER]		= SIG_KILL_ACTION;
    sigDefActions[SIG_TERM]		= SIG_KILL_ACTION;

    sigCanHoldMask = 
	      ~(sigBitMasks[SIG_ARITH_FAULT] | sigBitMasks[SIG_ILL_INST] |
		sigBitMasks[SIG_ADDR_FAULT]  | sigBitMasks[SIG_KILL] |
		sigBitMasks[SIG_BREAKPOINT]  | sigBitMasks[SIG_TRACE_TRAP] |
		sigBitMasks[SIG_MIGRATE_HOME]);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_ProcInit --
 *
 *	Initialize the signal data structures for a process.  It is assumed
 *	that this routine is able to muck with the signaling stuff in
 *	the proc table without having to grab any lock or anything.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The set of bit masks and the set of default actions are set up.
 *
 *----------------------------------------------------------------------
 */

void
Sig_ProcInit(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    procPtr->sigFlags = 0;
    procPtr->sigHoldMask = 0;
    procPtr->sigPendingMask = 0;

    Byte_Copy(sizeof(sigDefActions), (Address) sigDefActions, 
	      (Address) procPtr->sigActions);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_ChangeState --
 *
 *	Set the entire signal state of the process to that given.  When
 *	setting the state verify that improper signals are not blocked or
 *	ignored.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The signal actions and hold mask will be set for the process.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Sig_ChangeState(procPtr, actions, sigMasks, pendingMask, sigCodes, holdMask)
    register	Proc_ControlBlock	*procPtr;
    int					actions[];
    register	int			sigMasks[];
    int					pendingMask;
    int					sigCodes[];
    int					holdMask;
{
    register	int	i;
    register	int	*actionPtr;

    LOCK_MONITOR;

    for (i = SIG_MIN_SIGNAL, actionPtr = &actions[SIG_MIN_SIGNAL]; 
	 i < SIG_NUM_SIGNALS; 
	 i++, actionPtr++) {
	if (i == SIG_KILL) {
	    continue;
	}
	procPtr->sigActions[i] = *actionPtr;
	if (*actionPtr == SIG_IGNORE_ACTION) {
	    /*
	     * If is ignore action then make sure that is not one of the
	     * signals that cannot be ignored.  If not then remove the signal
	     * from the pending mask.
	     */

	    if (sigBitMasks[i] & sigCanHoldMask) {
		pendingMask &= ~sigBitMasks[i];
	    } else {
		procPtr->sigActions[i] = sigDefActions[i];
	    }
	} else if (*actionPtr > SIG_NUM_ACTIONS) {
	    /*
	     * If greater than one of the actions then must be the address
	     * of a signal handler so store the signal mask.
	     */

	    procPtr->sigMasks[i] = sigMasks[i] & sigCanHoldMask;
	}
    }

    procPtr->sigPendingMask = pendingMask;

    procPtr->sigHoldMask = holdMask & sigCanHoldMask;
    Byte_Copy(sizeof(procPtr->sigCodes), (Address) sigCodes, 
		(Address) procPtr->sigCodes);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_UserSend --
 *	Send a signal to a process.  Call the internal routine to do the
 *	work.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus	
Sig_UserSend(sigNum, pid, familyID)
    int		sigNum;		/* The signal to send. */
    Proc_PID	pid;		/* The id number of the process or process
				   family. */
    Boolean	familyID;	/* Whether the id is a process id or a process
				   group id. */
{
    return(Sig_Send(sigNum, SIG_NO_CODE, pid, familyID));
}


/*
 *----------------------------------------------------------------------
 *
 * LocalSend --
 *
 *	Send a signal to a process on the local machine.  It assumed that the
 *	process is locked down when we are called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signal pending mask and code modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
LocalSend(procPtr, sigNum, code)
    register	Proc_ControlBlock	*procPtr;
    int					sigNum;
    int					code;
{
    int	sigBitMask;

    LOCK_MONITOR;

    /*
     * Signals can't be sent to kernel processes unless the system is being
     * shutdown since kernel processes never get the oppurtunity to handle
     * signals.
     */
    if ((procPtr->genFlags & PROC_KERNEL) && !sys_ShuttingDown) {
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Only send the signal if it shouldn't be ignored and if it isn't
     * a signal to migrate an unmigrated process.  (The latter can easily
     * happen when signalling a process family to migrate home.)
     */
    if ((procPtr->sigActions[sigNum] != SIG_IGNORE_ACTION) &&
	!((sigNum == SIG_MIGRATE_HOME) && (procPtr->peerHostID == NIL))) {
	sigBitMask = sigBitMasks[sigNum];
	if (sigNum != SIG_RESUME) {
	    /*
	     * A resume signal just resumes a process - it does not leave any
	     * state around.  All other types of signals leave state.
	     */
	    procPtr->sigPendingMask |= sigBitMask;
	    procPtr->sigCodes[sigNum] = code;
	}
	if (procPtr->sigHoldMask & sigBitMask & ~sigCanHoldMask) {
	    /*
	     * We received a signal that was blocked but can't be blocked
	     * by users.  It only can be blocked if we are in the middle of
	     * executing a signal handler for the signal.  So we set things
	     * up to take the default action and make the signal unblocked
	     * so that we don't get an infinite loop of errors.
	     */
	    procPtr->sigHoldMask &= ~sigBitMask;
	    procPtr->sigActions[sigNum] = sigDefActions[sigNum];
	}

	/*
	 * If the process is waiting or suspended then wake it up.
	 */
	Sync_WakeWaitingProcess(procPtr);
	Proc_Resume(procPtr);

	if (sigNum == SIG_KILL) {
	    /*
	     * If the process is in the debug state or suspended then make it 
	     * runnable so that it can die.
	     *
	     * SYNCHRONIZATION NOTE:
	     *
	     *     We are the only ones that can put a process into and out of
	     *     the debuggable and suspended state since we have the
	     *	   monitor lock down.  Therefore there are no race conditions
	     *     on accessing the state of the destination process.
	     */
	    if (procPtr->state == PROC_DEBUGABLE) {
		Proc_TakeOffDebugList(procPtr);
	    } else if (procPtr->state == PROC_SUSPENDED) {
		Proc_Resume(procPtr);
	    }
	}
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SendProc --
 *
 *	Store the signal in the pending mask and store the code for the given
 *	process.
 *
 *	NOTE: Assumes that we are called without the master lock down and
 *	with the process locked.
 *
 * Results:
 *	In the case of a local process, SUCCESS is returned.  If the process
 *	is migrated, error conditions such as RPC_TIMEOUT may be returned.
 *
 * Side effects:
 *	Signal pending mask and code modified.  If the process being signalled
 *	is migrated, an RPC is sent.  If the process is local, the sched_Mutex
 *	master lock is grabbed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sig_SendProc(procPtr, sigNum, code)
    register	Proc_ControlBlock *procPtr;
    int				  sigNum;
    int				  code;
{
    if (procPtr->state == PROC_MIGRATED ||
        (procPtr->genFlags & PROC_MIGRATING)) {
	return(SigMigSend(procPtr, sigNum, code));
    } else {
	LocalSend(procPtr, sigNum, code);
	return(SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_Send --
 *
 *	Send a signal to a process.  This entails marking the signal into
 *	the signal pending mask for the process and waking up the process
 *	if it is asleep.  When we go to a multi-processor this routine must
 *	be rewritten to possibly interrupt a running process.
 *
 * Results:
 *	An error is the signal or the process id are invalid.  SUCCESS 
 *	otherwise.
 *
 * Side effects:
 *	The signal information in the proc table for the process that
 *	is being sent the signal may be modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus	
Sig_Send(sigNum, code, id, familyID)
    int		sigNum;		/* The signal to send. */
    int		code;		/* The code that goes with the signal. */
    Proc_PID	id;		/* The id number of the process or process
				   family. */
    Boolean	familyID;	/* Whether the id is a process id or a process
				   group id. */
{
    register	Proc_ControlBlock	*procPtr;
    register	Proc_ControlBlock	*curProcPtr;
    register	Proc_ControlBlock	*famProcPtr;
    Proc_PCBLink			*procLinkPtr;
    ReturnStatus			status;
    List_Links				*familyList;
    int					userID;
    int					hostID;

    /*
     * Make sure that the signal is in range.
     */
    if (sigNum < SIG_MIN_SIGNAL || sigNum >= SIG_NUM_SIGNALS) {
	return(SIG_INVALID_SIGNAL);
    }

    if (!Proc_ComparePIDs(id, PROC_MY_PID)) {
	hostID = Proc_GetHostID(id);
	if (hostID != rpc_SpriteID) {
	    /*
	     * Send a remote signal.
	     */
	    if (hostID == NET_BROADCAST_HOSTID) {
		return(PROC_INVALID_PID);
	    } else {
		return(SendRemoteSignal(hostID, sigNum, code, id, familyID));
	    }
	}
    }
    /*
     * Get the pointer to the control block if this is a valid process id.
     */
    if (!familyID) {
	if (Proc_ComparePIDs(id, PROC_MY_PID)) {
	    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
	    if (procPtr == (Proc_ControlBlock *) NIL) {
		Sys_Panic(SYS_FATAL, "Sig_Send: procPtr == NIL\n");
	    }
	    Proc_Lock(procPtr);
	} else {
	    procPtr = Proc_LockPID(id);
	    if (procPtr == (Proc_ControlBlock *) NIL) {
		return(PROC_INVALID_PID);
	    }
	    if (!Proc_HasPermission(procPtr->effectiveUserID)) {
		Proc_Unlock(procPtr);
		return(PROC_UID_MISMATCH);
	    }
	}
	status = Sig_SendProc(procPtr, sigNum, code);
	Proc_Unlock(procPtr);
    } else {
	status = Proc_LockFamily(id, &familyList, &userID);
	if (status != SUCCESS) {
	    return(status);
	}
	if (!Proc_HasPermission(userID)) {
            Proc_UnlockFamily(id);
            return(PROC_UID_MISMATCH);
        }

	/*
	 * Send a signal to everyone in the given family.
	 */

	LIST_FORALL(familyList, (List_Links *) procLinkPtr) {
	    procPtr = procLinkPtr->procPtr;
	    Proc_Lock(procPtr); 
	    status = Sig_SendProc(procPtr, sigNum, code);
	    Proc_Unlock(procPtr); 
	    if (status != SUCCESS) {
		break;
	    }
	}
	Proc_UnlockFamily(id);
    }

    return(status);
}

typedef struct {
	int		sigNum;
	int		code;
	Proc_PID	id;
	Boolean		familyID;
	int		effUid;
} SigParms;


/*
 *----------------------------------------------------------------------
 *
 * SendRemoteSignal --
 *
 *	Send a signal to a process on a remote machine.
 *
 * Results:
 *	Return the status from the remote machine.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus	
SendRemoteSignal(hostID, sigNum, code, id, familyID)
    int		hostID;		/* Host to send message to. */
    int		sigNum;		/* Signal to send. */
    int		code;		/* Code to send. */
    Proc_PID	id;		/* ID to send it to. */
    Boolean	familyID;	/* TRUE if are sending to a process family. */
{
    SigParms		sigParms;
    Rpc_Storage		storage;
    Proc_ControlBlock	*procPtr;

    sigParms.sigNum = sigNum;
    sigParms.code = code;
    sigParms.id = id;
    sigParms.familyID = familyID;
    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    sigParms.effUid = procPtr->effectiveUserID;

    storage.requestParamPtr = (Address)&sigParms;
    storage.requestParamSize = sizeof(sigParms);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    return(Rpc_Call(hostID, RPC_SIG_SEND, &storage));

}


/*
 *----------------------------------------------------------------------
 *
 * Sig_RpcSend --
 *
 *	Stub to handle a remote signal RPC.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Reply is sent.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus	
Sig_RpcSend(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* Command identifier */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    SigParms		*sigParmsPtr;
    ReturnStatus	status;
    Proc_ControlBlock	*procPtr;
    int			effUid;

    sigParmsPtr = (SigParms *) storagePtr->requestParamPtr;
    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    effUid = procPtr->effectiveUserID;
    procPtr->effectiveUserID = sigParmsPtr->effUid;
    status = Sig_Send(sigParmsPtr->sigNum, sigParmsPtr->code, sigParmsPtr->id,
		      sigParmsPtr->familyID);
    procPtr->effectiveUserID = effUid;
    Rpc_Reply(srvToken, status, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SetHoldMask --
 *
 *	Set the signal hold mask for the current process.  Return the
 *	old mask.  No synchronization required since the only process
 *	that can modify the hold mask is this process.
 *
 * Results:
 *	Error if the place to store the old mask is invalid,  SUCCESS
 *	otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus	
Sig_SetHoldMask(newMask, oldMaskPtr)
    int	newMask;	/* Mask to set the hold mask to. */
    int	*oldMaskPtr;	/* Where to store the old mask. */
{
    register	Proc_ControlBlock	*procPtr;

    /*
     * Get out the old mask value and store the new one.
     */

    procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());

    if (oldMaskPtr != USER_NIL) {
	if (Vm_CopyOut(sizeof(procPtr->sigHoldMask), 
		       (Address) &(procPtr->sigHoldMask),
		       (Address) oldMaskPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    procPtr->sigHoldMask = newMask & sigCanHoldMask;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SetAction --
 *
 *	Set the action for a particular signal.
 *
 * Results:
 *	Error if the action, signal, or handler is invalid.
 *
 * Side effects:
 *	The sigAction and sigMasks fields may be modified for the
 *	particular signal.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus	
Sig_SetAction(sigNum, newActionPtr, oldActionPtr)
    int		sigNum;	       /* The signal for which the action is to be 
				  set. */
    Sig_Action	*newActionPtr; /* The actions to take for the signal. */
    Sig_Action	*oldActionPtr; /* The action that was taken for the signal. */
{
    Proc_ControlBlock	*procPtr;
    Address		dummy;
    Sig_Action		action;

    /*
     * Make sure that the signal is in range.
     */

    if (sigNum < SIG_MIN_SIGNAL || sigNum >= SIG_NUM_SIGNALS || 
	sigNum == SIG_KILL) {
	return(SIG_INVALID_SIGNAL);
    }

    procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());

    /* 
     * Copy out the current action.  There are two cases:
     *
     *    1) The current action really contains a handler to call.  Thus
     *	     the current action is SIG_HANDLE_ACTION.
     *	  2) The current action is one of the other four actions.
     */

    if (oldActionPtr != (Sig_Action *) USER_NIL) {
	if (procPtr->sigActions[sigNum] > SIG_NUM_ACTIONS) {
	    action.action = SIG_HANDLE_ACTION;
	    (int) action.handler = procPtr->sigActions[sigNum];
	    action.sigHoldMask = procPtr->sigMasks[sigNum];
	} else {
	    if (procPtr->sigActions[sigNum] == sigDefActions[sigNum]) {
		action.action = SIG_DEFAULT_ACTION;
	    } else {
		action.action = procPtr->sigActions[sigNum];
	    }
	}
	if (Vm_CopyOut(sizeof(action), (Address) &action, 
		(Address) oldActionPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    /*
     * Copy in the action to take.
     */

    if (Vm_CopyIn(sizeof(action), (Address) newActionPtr, 
		(Address) &action) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Make sure that the action is valid.
     */

    if (action.action < 0 || action.action > SIG_NUM_ACTIONS) {
	return(SIG_INVALID_ACTION);
    }

    if (action.action == SIG_DEFAULT_ACTION) {
	action.action = sigDefActions[sigNum];
    }

    /*
     * Store the action.  If it is SIG_HANDLE_ACTION then the handler is stored
     * in place of the action.
     */

    if (action.action == SIG_HANDLE_ACTION) {
	if (Vm_CopyIn(4, (Address) action.handler, 
			(Address) &dummy) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
	procPtr->sigMasks[sigNum] = 
		(sigBitMasks[sigNum] | action.sigHoldMask) & sigCanHoldMask;
	procPtr->sigActions[sigNum] = (int) action.handler;
    } else if (action.action == SIG_IGNORE_ACTION) {

	/*
	 * Only actions that can be blocked can be ignored.  This prevents a
	 * user from ignoring a signal such as a bus error which would cause
	 * the process to take a bus error repeatedly.
	 */

	if (sigBitMasks[sigNum] & sigCanHoldMask) {
	    procPtr->sigActions[sigNum] = SIG_IGNORE_ACTION;
	    SigClearPendingMask(procPtr, sigNum);
	} else {
	    return(SIG_INVALID_SIGNAL);
	}
    } else {
	procPtr->sigActions[sigNum] = action.action;
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_Pause --
 *
 *	Atomically change signal hold mask and wait for a signal to arrive.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus	
Sig_Pause(sigHoldMask)
    int	sigHoldMask;	/* The value that the mask of held signals is to be set
			   to while waiting for a signal to arrive. */
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus status;

    LOCK_MONITOR;

    procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());

    /*
     * The signal mask cannot be restored until the signal handler has
     * had a chance to be called for the signal that caused Sig_Pause
     * to return.  To allow this the current hold mask is stored in the
     * proc table and the flag sigPause is set to be true to indicate that
     * the hold mask has to be restored after the signal handler has had a
     * chance to be called.
     */

    procPtr->oldSigHoldMask = procPtr->sigHoldMask;
    procPtr->sigFlags |= SIG_PAUSE_IN_PROGRESS;
    procPtr->sigHoldMask = sigHoldMask & sigCanHoldMask;

    /*
     * Wait on the signal condition.  As it turns out since a signal
     * wakes up the process regardless what it is sleeping on, this condition
     * variable is never broadcasted on, but we have to wait on something in 
     * order to release the monitor lock.
     *
     * Don't let a Sig_Pause be interrupted by a migrate trap signal.
     */
    (void) Sync_Wait(&signalCondition, TRUE);

    if (procPtr->sigPendingMask & ((SigGetBitMask(SIG_MIGRATE_TRAP)) ||
				   (SigGetBitMask(SIG_MIGRATE_HOME)))) {
	status = GEN_ABORTED_BY_SIGNAL;
    } else {
	status = SUCCESS;
    }
    
    UNLOCK_MONITOR;

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SigClearPendingMask --
 *
 *	Remove the given signal from the pending mask.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Pending mask for process modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
SigClearPendingMask(procPtr, sigNum)
    register	Proc_ControlBlock	*procPtr;
    int					sigNum;
{
    LOCK_MONITOR;

    procPtr->sigPendingMask &= ~sigBitMasks[sigNum];

    UNLOCK_MONITOR;
}

