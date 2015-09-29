/* 
 * signals.c --
 *
 * Copyright 1988, 1992 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * This contains routines that deal with Sprite signals.  See the man pages
 * on signals for an explanation of the Sprite signaling facilities.  The
 * only thing that is explained in these comments is the implementation of
 * these facilities.
 *
 * SYNCHRONIZATION
 * 
 * Synchronization is mostly done by locking the PCB in question.  There
 * may be occasional unlocked calls to Sig_Pending.  If such a call returns
 * a false negative, the signal will simply be delayed.  If the call
 * returns a false positive, Sig_Handle will check again with the PCB
 * locked and bail out if there's no pending signal.  If a process waits on 
 * a condition variable, it can say that it wants to be woken up if it 
 * receives a signal.  The synchronization to avoid a race between sleeping 
 * and receiving a signal is handled by the sync module.
 * 
 * There is also a monitor lock to make Sig_Pause work, but only because 
 * pcb's are locked via a flag, not by real locks.
 *
 * SIGNAL HANDLERS
 *
 * When a signal handler is called, the process's state is saved, the 
 * handler is called, and then the state is restored.  XXX Once I figure
 * out how this all really works I imagine I'll have more to say here...
 */


#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/sig/RCS/signals.c,v 1.7 92/05/08 15:12:30 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <bstring.h>
#include <ckalloc.h>
#include <mach_error.h>
#include <mach/exception.h>
#include <list.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>

#include <sig.h>
#include <sigMach.h>
#include <sync.h>
#include <dbg.h>
#include <proc.h>
#include <procMigrate.h>
#include <sigInt.h>
#include <rpc.h>
#include <net.h>
#include <vm.h>

#define	SigGetBitMask(sig) (1 << (sig - 1))

unsigned int 	sigBitMasks[SIG_NUM_SIGNALS];
int		sigDefActions[SIG_NUM_SIGNALS];
int		sigCanHoldMask;
Boolean		sigDebug = FALSE;

/* 
 * Condition variable to wait on when waiting for a signal, plus a monitor 
 * lock to keep the sync module happy.
 */
static Sync_Lock	sigLock;
#define LOCKPTR	(&sigLock)
static Sync_Condition	signalCondition;

/* Forward declarations */

static Boolean AsynchHandlerOkay _ARGS_((Proc_LockedPCB *procPtr,
			Boolean *suspendedPtr));
static void LocalSend _ARGS_((Proc_LockedPCB *procPtr, int sigNum, int code,
			Address addr));


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

    Sync_LockInitDynamic(&sigLock, "Sig:sigLock");

    /* 
     * Verify that the MIG type definition matches the C definition.
     */
    if (SIG_ACTION_SIZE * sizeof(int) != sizeof(Sig_Action)) {
	panic("Sig_Init: size mismatch for Sig_Action.\n");
    }

    for (i = SIG_MIN_SIGNAL; i < SIG_NUM_SIGNALS; i++) {
	sigBitMasks[i] = SigGetBitMask(i);
	sigDefActions[i] = SIG_KILL_ACTION;
    }

    /* 
     * Note that SIG_RESUME uses the "kill" action, even though it's not 
     * actually used to kill the process.
     */
    sigDefActions[SIG_DEBUG]		= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ARITH_FAULT]	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ILL_INST] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_ADDR_FAULT] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_BREAKPOINT] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_TRACE_TRAP] 	= SIG_DEBUG_ACTION;
    sigDefActions[SIG_MIGRATE_TRAP] 	= SIG_MIGRATE_ACTION;
    sigDefActions[SIG_MIGRATE_HOME] 	= SIG_MIGRATE_ACTION;
    sigDefActions[SIG_SUSPEND]		= SIG_SUSPEND_ACTION;
    sigDefActions[SIG_TTY_INPUT]	= SIG_SUSPEND_ACTION;
    sigDefActions[SIG_URGENT]		= SIG_IGNORE_ACTION;
    sigDefActions[SIG_CHILD]		= SIG_IGNORE_ACTION;
    sigDefActions[SIG_TTY_SUSPEND]	= SIG_SUSPEND_ACTION;
    sigDefActions[SIG_TTY_OUTPUT]	= SIG_SUSPEND_ACTION;

    sigCanHoldMask = 
	      ~(sigBitMasks[SIG_ARITH_FAULT] | sigBitMasks[SIG_ILL_INST] |
		sigBitMasks[SIG_ADDR_FAULT]  | sigBitMasks[SIG_KILL] |
		sigBitMasks[SIG_BREAKPOINT]  | sigBitMasks[SIG_TRACE_TRAP] |
		sigBitMasks[SIG_MIGRATE_HOME] | sigBitMasks[SIG_SUSPEND]);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_ProcInit --
 *
 *	Initialize the signal data structures for the first process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signal state initialized.
 *
 *----------------------------------------------------------------------
 */
void
Sig_ProcInit(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    procPtr->sigHoldMask = 0;
    procPtr->sigPendingMask = 0;
    bcopy((Address)sigDefActions,(Address)procPtr->sigActions,
              sizeof(sigDefActions));
    bzero((Address)procPtr->sigMasks,sizeof(procPtr->sigMasks)); 
    bzero((Address)procPtr->sigCodes,sizeof(procPtr->sigCodes));
    procPtr->sigFlags = 0;
    procPtr->sigTrampProc = USER_NIL;
}



/*
 *----------------------------------------------------------------------
 *
 * Sig_Fork --
 *
 *	Copy over the parents signal state into the child.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signal state copied from parent to child and pending mask cleared in
 *	child.  Migration is held until the first return into user mode.
 *
 *----------------------------------------------------------------------
 */
void
Sig_Fork(parProcPtr, childProcPtr)
    register	Proc_ControlBlock *parProcPtr;
    register	Proc_LockedPCB	*childProcPtr;
{
    Proc_Lock(parProcPtr);

    /*
     * Copy the parent's signal state to the child.  Set up migration
     * to be held initially.  On the first return to user mode, after
     * signals are processed, migration will be reenabled.
     */
    childProcPtr->pcb.sigHoldMask = parProcPtr->sigHoldMask |
	    SigGetBitMask(SIG_MIGRATE_TRAP);
    childProcPtr->pcb.sigPendingMask = 0;
    bcopy((Address)parProcPtr->sigActions, 
	  (Address)childProcPtr->pcb.sigActions,
	  sizeof(childProcPtr->pcb.sigActions)); 
    bcopy((Address)parProcPtr->sigMasks, 
	  (Address)childProcPtr->pcb.sigMasks,
    	  sizeof(childProcPtr->pcb.sigMasks)); 
    bzero((Address)childProcPtr->pcb.sigCodes,
	  sizeof(childProcPtr->pcb.sigCodes));
    childProcPtr->pcb.sigFlags = 0;

    Proc_Unlock(Proc_AssertLocked(parProcPtr));
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_Exec --
 *
 *	Clear all signal handlers on exec.  Assumed called with the proc
 * 	table entry locked such that signals against this process are
 *	prevented.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All signal handlers are cleared and the pending mask is cleared.
 *
 *----------------------------------------------------------------------
 */
void
Sig_Exec(procPtr)
    Proc_LockedPCB	*procPtr;
{
    register	int	*actionPtr;
    register	int	i;

    for (i = SIG_MIN_SIGNAL,
	     actionPtr = &procPtr->pcb.sigActions[SIG_MIN_SIGNAL]; 
	 i < SIG_NUM_SIGNALS;
	 i++, actionPtr++) {
	if (*actionPtr > SIG_SUSPEND_ACTION) {
	    /*
	     * The action contains a signal handler to call.  Reset back to
	     * the default action.
	     */
	    *actionPtr = sigDefActions[i];
	    procPtr->pcb.sigMasks[i] = 0;
	}
    }
    procPtr->pcb.sigPendingMask = 0;
    procPtr->pcb.sigTrampProc = USER_NIL;
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

void
Sig_ChangeState(procPtr, actions, sigMasks, pendingMask, sigCodes, holdMask)
    register	Proc_LockedPCB		*procPtr;
    int					actions[];
    register	int			sigMasks[];
    int					pendingMask;
    int					sigCodes[];
    int					holdMask;
{
    register	int	i;
    register	int	*actionPtr;

    for (i = SIG_MIN_SIGNAL, actionPtr = &actions[SIG_MIN_SIGNAL]; 
	 i < SIG_NUM_SIGNALS; 
	 i++, actionPtr++) {
	if (i == SIG_KILL) {
	    continue;
	}
	procPtr->pcb.sigActions[i] = *actionPtr;
	if (*actionPtr == SIG_IGNORE_ACTION) {
	    /*
	     * If is ignore action then make sure that is not one of the
	     * signals that cannot be ignored.  If not then remove the signal
	     * from the pending mask.
	     */
	    if (sigBitMasks[i] & sigCanHoldMask) {
		pendingMask &= ~sigBitMasks[i];
	    } else {
		procPtr->pcb.sigActions[i] = sigDefActions[i];
	    }
	} else if (*actionPtr > SIG_NUM_ACTIONS) {
	    /*
	     * If greater than one of the actions then must be the address
	     * of a signal handler so store the signal mask.
	     */
	    procPtr->pcb.sigMasks[i] = sigMasks[i] & sigCanHoldMask;
	}
    }

    procPtr->pcb.sigPendingMask = pendingMask;

    procPtr->pcb.sigHoldMask = holdMask & sigCanHoldMask;
    bcopy((Address) sigCodes, (Address) procPtr->pcb.sigCodes,
          sizeof(procPtr->pcb.sigCodes));
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

static void
LocalSend(procPtr, sigNum, code, addr)
    register	Proc_LockedPCB		*procPtr;
    int					sigNum;
    int					code;
    Address				addr;
{
    int	sigBitMask;

    /*
     * Signals can't be sent to kernel processes unless the system is being
     * shutdown since kernel processes never get the opportunity to handle
     * signals.
     */
    if ((procPtr->pcb.genFlags & PROC_KERNEL) && !sys_ShuttingDown) {
	return;
    }

#ifdef SPRITED_MIGRATION
    if ((procPtr->pcb.sigActions[sigNum] == SIG_DEBUG_ACTION) &&
	proc_KillMigratedDebugs && (procPtr->pcb.genFlags & PROC_FOREIGN)) {
	/*
	 * Kill the process rather than letting it go silently into that
	 * good night (on the wrong machine).   Debugging migrated
	 * processes is nasty.  It would be nice if we could redirect
	 * the printf to the process's home node, too.
	 */
	sigNum = SIG_KILL;
	if (proc_MigDebugLevel > 1) {
	    printf("Warning: killing a migrated process that would have gone into the debugger, pid %x rpid %x uid %d.\n",
	        procPtr->pcb.processID, (int) procPtr->pcb.peerProcessID, 
		procPtr->pcb.userID);

	}
    }
#endif /* SPRITED_MIGRATION */

    /*
     * Only send the signal if it shouldn't be ignored and if it isn't
     * a signal to migrate an unmigrated process.  (The latter can easily
     * happen when signalling a process family to migrate home.)
     */
    if ((procPtr->pcb.sigActions[sigNum] != SIG_IGNORE_ACTION) &&
	!((sigNum == SIG_MIGRATE_HOME) && (procPtr->pcb.peerHostID == NIL))) {
	if (sigNum == SIG_RESUME) {
	    /* 
	     * Resume the suspended process.
	     */
	    Proc_ResumeProcess(procPtr, FALSE);
	}
	if (procPtr->pcb.sigActions[sigNum] == SIG_SUSPEND_ACTION &&
		   procPtr->pcb.state == PROC_SUSPENDED) {
	    /*
	     * Are sending a suspend signal to a process that is already
	     * suspended.  In this case just notify the parent that the 
	     * process has been suspended.  This is necessary because resume
	     * signals are sent by processes to debugged processes which do not
	     * really get resumed.  However, the signaling process will not
	     * be informed that the process it sent the signal to did not get
	     * resumed (SIG_RESUME works regardless whether it actually 
	     * resumes anything or not).  Thus a process may believe that
	     * a process is running even though it really isn't and it may
	     * send a suspend signal to an already suspended process.
	     *
	     * There is a potential race here between a process getting
	     * suspended and us checking here but it doesn't matter.  If
	     * it gets suspended after we check then the parent will get 
	     * notified anyway.
	     */
	    Proc_InformParent(procPtr, PROC_SUSPEND_STATUS);
	} else if (sigNum != SIG_RESUME ||
		procPtr->pcb.sigActions[sigNum] != SIG_KILL_ACTION) {
	    sigBitMask = sigBitMasks[sigNum];
	    procPtr->pcb.sigPendingMask |= sigBitMask;
	    procPtr->pcb.sigCodes[sigNum] = code;
	    procPtr->pcb.sigAddr = (int)addr;
	    if (procPtr->pcb.sigHoldMask & sigBitMask & ~sigCanHoldMask) {
		/*
		 * We received a signal that was blocked but can't be blocked
		 * by users.  It only can be blocked if we are in the middle of
		 * executing a signal handler for the signal.  So we set things
		 * up to take the default action and make the signal unblocked
		 * so that we don't get an infinite loop of errors.
		 */
		procPtr->pcb.sigHoldMask &= ~sigBitMask;
		procPtr->pcb.sigActions[sigNum] = sigDefActions[sigNum];
	    }
	    /*
	     * If the process is waiting then wake it up.
	     */
	    Sync_WakeWaitingProcess(procPtr);
	    if (sigNum == SIG_KILL || sigNum == SIG_MIGRATE_TRAP ||
		sigNum == SIG_MIGRATE_HOME) {
		if (sigNum == SIG_KILL && procPtr->pcb.state == PROC_NEW &&
		    (procPtr->pcb.genFlags & PROC_FOREIGN)) {
		    /*
		     * The process was only partially created.  We can't make
		     * it runnable so we have to reclaim it directly.
		     * Do this in the background so that
		     * Proc_DestroyMigratedProc has to wait for Sig_Send
		     * to unlock the process and we avoid a race condition.
		     */
#ifdef SPRITED_MIGRATION
		    Proc_CallFunc(Proc_DestroyMigratedProc,
				  (ClientData) procPtr->pcb.processID, 0);
#else
		    panic("LocalSend: foreign process.\n"); 
				/* shouldn't happen */
#endif
		} else {
		    /*
		     * Resume the process so that we can perform the signal.
		     * If we're killing it, we tell Proc_ResumeProcess so it
		     * will even wake up a debugged process.
		     */
		    Proc_ResumeProcess(procPtr,
				       (sigNum == SIG_KILL) ? TRUE : FALSE);
		}
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SendProc --
 *
 *	Store the signal in the pending mask and store the code for the
 *	given process.  Causes the signal to be handled unless the process
 *	is in an awkward state.
 *
 * Results:
 *	In the case of a local process, SUCCESS is usually returned.  If
 *	the process is migrated, error conditions such as RPC_TIMEOUT may
 *	be returned.
 *
 * Side effects:
 *	Signal pending mask and code modified.  If the process being signalled
 *	is migrated, an RPC is sent.  Other side effects are possible, 
 *	depending on the signal and whether the user process has a handler 
 *	for it.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sig_SendProc(procPtr, sigNum, isException, code, addr)
    register	Proc_LockedPCB	*procPtr;
    int				sigNum;
    Boolean			isException; /* signal must be handled 
					      * synchronously */
    int				code; /* signal cause, passed to user 
				       * signal handler */
    Address			addr; /* fault address, passed to user 
				       * signal handler */
{
    ReturnStatus status = SUCCESS;
    Sig_Stack sigStack;		/* stuff passed to user signal handler */
    Sig_Context sigContext;
    Boolean suspended;		/* was the thread suspended by Sig_Handle */
    Address sigHandler;		/* address of user signal handler */

    sigStack.contextPtr = &sigContext;

    /*
     * Make sure that the signal is in range.
     * XXX Dropping signal 0 on the floor is incompatible with UNIX.  (Of 
     * course, it would help if we mapped certain UNIX signals to a Sprite 
     * signal number other than 0...)
     */
    if (sigNum < SIG_MIN_SIGNAL || sigNum >= SIG_NUM_SIGNALS) {
	if (sigNum == 0) {
	    return(SUCCESS);
	} else {
	    return(SIG_INVALID_SIGNAL);
	}
    }

#ifdef SPRITED_MIGRATION
    /*
     * Handle migrated processes specially. There's a race condition
     * when sending a signal to a migrated process, since it can
     * migrate back to this host while we're doing it.  Therefore,
     * if the problem was that the process didn't exist, check
     * to see if it has migrated back to this host (it's no longer MIGRATED).
     * We don't have to check for MIGRATING, since SigMigSend waits for
     * a migration in progress to complete.   Also make sure that while the
     * signal is sent and the process is unlocked, it processID doesn't change.
     */
    if (procPtr->pcb.state == PROC_MIGRATED ||
        (procPtr->pcb.genFlags & PROC_MIGRATING)) {
	Proc_PID processID;
	processID = procPtr->pcb.processID;
	status = SigMigSend(procPtr, sigNum, code, addr);
	if (processID != procPtr->pcb.processID) {
	    return(status);
	}
	if ((status != PROC_INVALID_PID) ||
	    (procPtr->pcb.state == PROC_MIGRATED)) {
	    return(status);
	}
    }
#endif /* SPRITED_MIGRATION */

    if (procPtr->pcb.state == PROC_EXITING ||
	    procPtr->pcb.state == PROC_DEAD) {
	return(PROC_INVALID_PID);
    } else if (procPtr->pcb.state == PROC_NEW) {
#ifdef SPRITED_MIGRATION
	if (procPtr->pcb.genFlags & PROC_FOREIGN && proc_MigDebugLevel > 0) {
	    printf("Warning: got signal for process %x before migration complete.\n",
		   procPtr->pcb.processID);
	}
#endif
	return(PROC_INVALID_PID);
    } else {
	LocalSend(procPtr, sigNum, code, addr);
	if (Sig_Handle(procPtr, isException, &suspended, &sigStack,
		       &sigHandler)) {
	    Sig_SetUpHandler(procPtr, suspended, &sigStack, sigHandler);
	}
	return(status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_Send --
 *
 *	Send a signal to a process.  This entails marking the signal into
 *	the signal pending mask for the process and waking up the process
 *	if it is asleep.   
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
Sig_Send(sigNum, code, id, familyID, addr)
    int		sigNum;		/* The signal to send. */
    int		code;		/* The code that goes with the signal. */
    Proc_PID	id;		/* The id number of the process or process
				   family. */
    Boolean	familyID;	/* Whether the id is a process id or a process
				   group id. */
    Address	addr;		/* The address of the fault */
{
    register	Proc_LockedPCB		*procPtr;
    Proc_PCBLink			*procLinkPtr;
    ReturnStatus			status;
    List_Links				*familyList;
    int					userID;
    int					hostID;

    if (!Proc_ComparePIDs(id, PROC_MY_PID)) {
	hostID = Proc_GetHostID(id);
	if (hostID != rpc_SpriteID) {
	    /*
	     * Send a remote signal.
	     */
	    if (hostID == NET_BROADCAST_HOSTID ||
		hostID >  NET_NUM_SPRITE_HOSTS || hostID < 0) {
		return(PROC_INVALID_PID);
	    } else {
		return(SigSendRemoteSignal(hostID, sigNum, code, id,
					   familyID, addr));
	    }
	}
    }
    /*
     * Get the pointer to the control block if this is a valid process id.
     */
    if (!familyID) {
	if (Proc_ComparePIDs(id, PROC_MY_PID)) {
	    procPtr = (Proc_LockedPCB *)Proc_GetEffectiveProc();
	    if (procPtr == (Proc_LockedPCB *) NIL) {
		panic("Sig_Send: procPtr == NIL\n");
	    }
	    Proc_Lock((Proc_ControlBlock *)procPtr);
	} else {
	    procPtr = Proc_LockPID(id);
	    if (procPtr == (Proc_LockedPCB *) NIL) {
		return(PROC_INVALID_PID);
	    }
	    if (!Proc_HasPermission(procPtr->pcb.effectiveUserID)) {
		Proc_Unlock(procPtr);
		return(PROC_UID_MISMATCH);
	    }
	}
	status = Sig_SendProc(procPtr, sigNum, FALSE, code, addr);
	Proc_Unlock(procPtr);
    } else {
	Proc_PID *pidArray;
	int i;
	int numProcs;
	
	status = Proc_LockFamily((int)id, &familyList, &userID);
	if (status != SUCCESS) {
	    return(status);
	}
	if (!Proc_HasPermission(userID)) {
            Proc_UnlockFamily((int)id);
            return(PROC_UID_MISMATCH);
        }

	/*
	 * Send a signal to everyone in the given family.  We do this
	 * by grabbing a list of process IDs and then sending the signals
	 * with the family not locked, to avoid deadlocks resulting from
	 * signals being sent with the family locked.
	 */

	numProcs = 0;
	LIST_FORALL(familyList, (List_Links *) procLinkPtr) {
	    numProcs++;
	}
	pidArray = (Proc_PID *) ckalloc(numProcs * sizeof(Proc_PID));
	i = 0;
	LIST_FORALL(familyList, (List_Links *) procLinkPtr) {
	    procPtr = (Proc_LockedPCB *)procLinkPtr->procPtr;
	    pidArray[i] = procPtr->pcb.processID;
	    i++;
	    if (i > numProcs) {
		panic("Sig_Send: process family changed size while locked.\n");
		ckfree((Address) pidArray);
		return(FAILURE);
	    }
	}
	Proc_UnlockFamily((int)id);
	for (i = 0; i < numProcs; i++) {
	    procPtr = Proc_LockPID(pidArray[i]);
	    if (procPtr == (Proc_LockedPCB *) NIL ||
		procPtr->pcb.familyID != id) {
		/*
		 * Race condition: process got removed.
		 */
		continue;
	    }
	    status = Sig_SendProc(procPtr, sigNum, FALSE, code, addr);
	    Proc_Unlock(procPtr); 
	    if (status != SUCCESS) {
		break;
	    }
	}
	ckfree((Address) pidArray);
    }

    return(status);
}

typedef struct {
	int		sigNum;
	int		code;
	Proc_PID	id;
	Boolean		familyID;
	int		effUid;
	Address		addr;
} SigParms;


/*
 *----------------------------------------------------------------------
 *
 * SigSendRemoteSignal --
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
SigSendRemoteSignal(hostID, sigNum, code, id, familyID, addr)
    int		hostID;		/* Host to send message to. */
    int		sigNum;		/* Signal to send. */
    int		code;		/* Code to send. */
    Proc_PID	id;		/* ID to send it to. */
    Boolean	familyID;	/* TRUE if are sending to a process family. */
    Address	addr;		/* Address of signal. */
{
    SigParms		sigParms;
    Rpc_Storage		storage;
    Proc_ControlBlock	*procPtr;

    sigParms.sigNum = sigNum;
    sigParms.code = code;
    sigParms.id = id;
    sigParms.familyID = familyID;
    procPtr = Proc_GetEffectiveProc();
    sigParms.effUid = procPtr->effectiveUserID;
    sigParms.addr = addr;

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
/*ARGSUSED*/
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
    procPtr = Proc_GetCurrentProc();
    effUid = procPtr->effectiveUserID;
    procPtr->effectiveUserID = sigParmsPtr->effUid;
    status = Sig_Send(sigParmsPtr->sigNum, sigParmsPtr->code, sigParmsPtr->id,
		      sigParmsPtr->familyID, sigParmsPtr->addr);
    procPtr->effectiveUserID = effUid;
    Rpc_Reply(srvToken, status, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SetHoldMask --
 *
 *	Set the signal hold mask for the current process.
 *
 * Results:
 *	Always returns SUCCESS.  Fills in the old signal mask.
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

    procPtr = Proc_GetActualProc();
    Proc_Lock(procPtr);

    *oldMaskPtr = procPtr->sigHoldMask;
    SigUpdateHoldMask(Proc_AssertLocked(procPtr), newMask);

    Proc_Unlock(Proc_AssertLocked(procPtr));
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * SigUpdateHoldMask --
 *
 *	Set a process's hold mask, considering what is legally held and 
 *	what was already held.  This routine should be used to update the 
 *	process's hold mask after a signal handler returns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
SigUpdateHoldMask(procPtr, newMask)
    Proc_LockedPCB *procPtr;	/* the process to update */
    int newMask;		/* the proposed new signal mask */
{
    int oldMask = procPtr->pcb.sigHoldMask;

    /* 
     * The first term of the OR names the signals that the user can try to 
     * hold.  The second term contains signals that the user cannot hold,
     * but which might be held because a handler was invoked for the
     * signal.
     */
    procPtr->pcb.sigHoldMask = (newMask & sigCanHoldMask) |
	(newMask & ~sigCanHoldMask & oldMask);
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
    ReturnStatus	status = SUCCESS;

    /*
     * Make sure that the signal is in range.
     */
    if (sigNum < SIG_MIN_SIGNAL || sigNum >= SIG_NUM_SIGNALS || 
	sigNum == SIG_KILL || sigNum == SIG_SUSPEND) {
	return(SIG_INVALID_SIGNAL);
    }

    procPtr = Proc_GetActualProc();
    Proc_Lock(procPtr);

    /* 
     * Copy out the current action.  There are two cases:
     *
     *    1) The current action really contains a handler to call.  Thus
     *	     the current action is SIG_HANDLE_ACTION.
     *	  2) The current action is one of the other four actions.
     */

    if (procPtr->sigActions[sigNum] > SIG_NUM_ACTIONS) {
	action.action = SIG_HANDLE_ACTION;
	action.handler = (int (*)())procPtr->sigActions[sigNum];
	action.sigHoldMask = procPtr->sigMasks[sigNum];
    } else {
	if (procPtr->sigActions[sigNum] == sigDefActions[sigNum]) {
	    action.action = SIG_DEFAULT_ACTION;
	} else {
	    action.action = procPtr->sigActions[sigNum];
	}
    }
    *oldActionPtr = action;

    /*
     * Get the new action and make sure it's valid.
     */

    action = *newActionPtr;
    if (action.action < 0 || action.action > SIG_NUM_ACTIONS) {
	status = SIG_INVALID_ACTION;
	goto done;
    }

    if (action.action == SIG_DEFAULT_ACTION) {
	action.action = sigDefActions[sigNum];
    }

    /*
     * Store the action.  If it is SIG_HANDLE_ACTION then the handler is stored
     * in place of the action.
     */

    if (action.action == SIG_HANDLE_ACTION) {
	if (Vm_CopyIn(4, (Address) ((unsigned int) (action.handler)), 
			(Address) &dummy) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	    goto done;
	}
	procPtr->sigMasks[sigNum] = 
		(sigBitMasks[sigNum] | action.sigHoldMask) & sigCanHoldMask;
	procPtr->sigActions[sigNum] = (unsigned int) action.handler;
    } else if (action.action == SIG_IGNORE_ACTION) {

	/*
	 * Only actions that can be blocked can be ignored.  This prevents a
	 * user from ignoring a signal such as a bus error which would cause
	 * the process to take a bus error repeatedly.
	 */

	if (sigBitMasks[sigNum] & sigCanHoldMask) {
	    procPtr->sigActions[sigNum] = SIG_IGNORE_ACTION;
	    SigClearPendingMask(Proc_AssertLocked(procPtr), sigNum);
	} else {
	    status = SIG_INVALID_SIGNAL;
	    goto done;
	}
	procPtr->sigMasks[sigNum] = 0;
    } else {
	procPtr->sigActions[sigNum] = action.action;
	procPtr->sigMasks[sigNum] = 0;
    }

 done:
    Proc_Unlock(Proc_AssertLocked(procPtr));
    return status;
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
    int migMask;

    LOCK_MONITOR;

    procPtr = Proc_GetActualProc();
    Proc_Lock(procPtr);

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
    Proc_Unlock(Proc_AssertLocked(procPtr));

    /*
     * Wait on the signal condition.  As it turns out since a signal
     * wakes up the process regardless what it is sleeping on, this condition
     * variable is never broadcasted on, but we have to wait on something in 
     * order to release the monitor lock.
     *
     * Don't let a Sig_Pause be interrupted by a migrate trap signal.
     * So, if none of the signal bits are set besides migration-related
     * signals, and a migration-related signal bit is set, let the user-level
     * code retry  the signal.
     */
    (void) Sync_Wait(&signalCondition, TRUE);

    migMask = (SigGetBitMask(SIG_MIGRATE_TRAP)) |
	(SigGetBitMask(SIG_MIGRATE_HOME));
    if ((! (procPtr->sigPendingMask & ~migMask)) &&
	(procPtr->sigPendingMask & migMask)) {
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

void
SigClearPendingMask(procPtr, sigNum)
    register	Proc_LockedPCB		*procPtr;
    int					sigNum;
{
    procPtr->pcb.sigPendingMask &= ~sigBitMasks[sigNum];
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_Handle --
 *
 * 	Process a signal or prepare to call a user handler.
 *
 * Results:
 *	Return TRUE if a signal is setup to be handled by the user and the
 *	process is in a state that it can take the signal.  In that case,
 *	the signal stack and PC for calling the handler are filled in.
 *	Always fills in a flag telling whether the Mach thread for the
 *	process was suspended.
 *
 * Side effects:
 *	If the signal can be dealt with now (either by calling a signal 
 *	handler or by, e.g., suspending the process), it is removed from
 *	the pending signals bitmask.
 *
 *----------------------------------------------------------------------
 */
Boolean		
Sig_Handle(procPtr, doNow, suspendedPtr, sigStackPtr, pcPtr)
    Proc_LockedPCB	*procPtr;
    Boolean		doNow;	/* the caller knows that the signal can be 
				 * handled right now */
    Boolean		*suspendedPtr; /* OUT: was the thread suspended */
    Sig_Stack		*sigStackPtr; /* OUT: info to put on stack for 
				       * handler */ 
    Address		*pcPtr;	/* OUT: start address of handler */
{
    int			sigs;
    int			sigNum = 0;
    unsigned	int	*bitMaskPtr;
    int			sigBitMask;
    int			reason;	/* destroyed, signaled, etc. */
    int			status; /* which signal, etc. if proc dies */
    Boolean		clearSignal; /* remove the signal from the 
				      * pending signals mask? */
    Boolean		callUserHandler; /* invoke user handler now? */

    *suspendedPtr = FALSE;
    clearSignal = FALSE;
    callUserHandler = FALSE;

    /*
     * Find out which signals are pending.
     */
    sigs = procPtr->pcb.sigPendingMask & ~procPtr->pcb.sigHoldMask;
    if (sigs == 0) {
	goto done;
    }

    /*
     * Check for the signal SIG_KILL.  This is processed specially because
     * it is how processes that have some problem such as being unable
     * to write to swap space on the file server are destroyed.
     */

    if (sigs & sigBitMasks[SIG_KILL]) {
	if (procPtr->pcb.sigCodes[SIG_KILL] != SIG_NO_CODE) {
	    reason = PROC_TERM_DESTROYED;
	    status = procPtr->pcb.sigCodes[SIG_KILL];
	} else {
	    reason = PROC_TERM_SIGNALED;
	    status = SIG_KILL;
	}
	sigNum = SIG_KILL;
	clearSignal = TRUE;
	Proc_Kill(procPtr, reason, status);
	goto done;
    }

    /* 
     * Find the lowest-numbered pending signal.  
     * XXX If a process is in an infinite loop and has a handler registered 
     * for a signal, that signal will prevent any higher-numbered signal 
     * (other than SIG_KILL) from taking effect.  Fortunately, SIG_DEBUG is
     * signal number 1.
     */
    
    for (sigNum = SIG_MIN_SIGNAL, bitMaskPtr = &sigBitMasks[SIG_MIN_SIGNAL];
	 !(sigs & *bitMaskPtr);
	 sigNum++, bitMaskPtr++) {
    }

    clearSignal = TRUE;

    /*
     * Process the signal.  First check for actions other than calling a 
     * user signal handler.
     */
    
    switch (procPtr->pcb.sigActions[sigNum]) {
	case SIG_IGNORE_ACTION:
	    printf("Warning: %s\n",
	    "Sig_Handle:  An ignored signal was in a signal pending mask.");
	    goto done;

	case SIG_KILL_ACTION:
	    if (sigNum == SIG_KILL
		    || !(procPtr->pcb.genFlags & PROC_DEBUGGED)) {
		/* 
		 * Resume the process before killing it, in case it's in 
		 * the debug list.
		 */
		Proc_ResumeProcess(procPtr, (sigNum == SIG_KILL));
		Proc_Kill(procPtr, PROC_TERM_SIGNALED, sigNum);
		goto done;
	    } 
	    /* Fall through */
	    
	case SIG_SUSPEND_ACTION:
	case SIG_DEBUG_ACTION:
	    /* 
	     * A suspended process and a debugged process are basically
	     * the same.  A suspended process can be debugged just like
	     * a process in the debug state.   The only difference is that
	     * a suspended process does not go onto the debug list; it can
	     * only be debugged by a debugger that specifically asks for
	     * it.
	     *
	     * If the target is not the current process and the target is
	     * currently being serviced, postpone the signal.  Otherwise,
	     * clear the pending signal now (e.g., in case the target is
	     * the current process) and suspend the target.
	     */
	    clearSignal = FALSE;
	    if (!(procPtr->pcb.genFlags & PROC_BEING_SERVED) || doNow) {
		SigClearPendingMask(procPtr, sigNum);
		Proc_SuspendProcess(procPtr,
			procPtr->pcb.sigActions[sigNum] == SIG_DEBUG_ACTION,
			PROC_TERM_SIGNALED, sigNum, 
			procPtr->pcb.sigCodes[sigNum]);
		if ((Proc_ControlBlock *)procPtr == Proc_GetCurrentProc()) {
		    Proc_Lock((Proc_ControlBlock *)procPtr);
		}
	    }
	    goto done;

	case SIG_MIGRATE_ACTION:
#ifndef SPRITED_MIGRATION
	    panic("Sig_Handle: migrate trap.\n");
#else
	    /* 
	     * XXX Think about whether the pending signal needs to be 
	     * cleared here or down at the bottom of the function.
	     */

	    /*
	     * If the process was in the middle of a page fault,
	     * its PC in the trap stack is not useable.
	     * Reset the pending condition but hold it until we get out of
	     * the kernel.
	     */
	    if (!Mach_CanMigrate(procPtr)) {
		LocalSend(procPtr, sigNum, procPtr->pcb.sigCodes[sigNum],
		    (Address)procPtr->pcb.sigAddr);
		procPtr->pcb.sigHoldMask |= SigGetBitMask(SIG_MIGRATE_TRAP);
		goto done;
	    }

	    /*
	     * Double-check against process not allowed to migrate.  This
	     * can happen if a process migrates, opens a pdev as master,
	     * and gets signalled to migrate home.
	     */
	    if (procPtr->pcb.genFlags & PROC_DONT_MIGRATE) {
		if (proc_MigDebugLevel > 0) {
		    printf("Proc_Migrate: process %x is not allowed to migrate.\n",
			       procPtr->pcb.processID);
		}
		goto done;
	    }
	    if (procPtr->pcb.peerHostID != NIL) {
		if (proc_MigDebugLevel > 6) {
		    printf("Sig_Handle calling Proc_MigrateTrap for process %x.\n",
			       procPtr->pcb.processID);
		}
		Proc_MigrateTrap(procPtr);
	    }
	    goto done;
#endif /* SPRITED_MIGRATION */

	case SIG_DEFAULT_ACTION:
	    panic("Sig_Handle: SIG_DEFAULT_ACTION found in array of actions?\n");
    }

    /*
     * There is a user signal handler for the signal.  Verify that the 
     * process is currently able to take a signal.
     */

    if (!doNow && !AsynchHandlerOkay(procPtr, suspendedPtr)) {
	/* 
	 * The signal will have to be handled when the user process asks 
	 * for it.
	 */
	clearSignal = FALSE;
	goto done;
    }

    /*
     * Set up our part of the signal stack for the signal handler.
     */

    callUserHandler = TRUE;
    sigStackPtr->sigNum = sigNum;
    sigStackPtr->sigCode = procPtr->pcb.sigCodes[sigNum];
    sigStackPtr->sigAddr = procPtr->pcb.sigAddr;
    /*
     * If this signal handler is being called after a call to Sig_Pause then
     * the real signal hold mask has to be restored after the handler returns.
     * This is assured by pushing the real hold mask which is stored in 
     * the proc table onto the stack.
     */
    if (procPtr->pcb.sigFlags & SIG_PAUSE_IN_PROGRESS) {
	procPtr->pcb.sigFlags &= ~SIG_PAUSE_IN_PROGRESS;
	sigStackPtr->contextPtr->oldHoldMask = procPtr->pcb.oldSigHoldMask;
    } else {
	sigStackPtr->contextPtr->oldHoldMask = procPtr->pcb.sigHoldMask;
    }

    procPtr->pcb.sigHoldMask |= procPtr->pcb.sigMasks[sigNum];
    sigBitMask = sigBitMasks[sigNum];
    if (sigBitMask & ~sigCanHoldMask) {
	/*
	 * If this is a non-blockable signal then add it to the hold mask
	 * so that if we get it again we know that it can't be handled.
	 */
	procPtr->pcb.sigHoldMask |= sigBitMask;
    }
    *pcPtr = (Address)procPtr->pcb.sigActions[sigNum];

 done:
    if (clearSignal) {
	SigClearPendingMask(procPtr, sigNum);
    }
    return callUserHandler;
}


#ifdef SPRITED_MIGRATION
/*
 *----------------------------------------------------------------------
 *
 * Sig_AllowMigration --
 *
 *	Set up a process to allow migration.  This is a special call
 *	because normally the SIG_MIGRATE_TRAP signal is not holdable in
 *	the first place.
 *
 *	This could be a macro and be called directly from
 *	Mach_StartUserProc, once things are stable....
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's hold mask is modified.
 *
 *----------------------------------------------------------------------
 */
void		
Sig_AllowMigration(procPtr)
    register Proc_LockedPCB	*procPtr;	/* process to modify */
{
    if (procPtr->sigHoldMask &&
	(procPtr->sigHoldMask & sigBitMasks[SIG_MIGRATE_TRAP])) {
    	procPtr->sigHoldMask &= ~sigBitMasks[SIG_MIGRATE_TRAP];
    }
}
#endif /* SPRITED_MIGRATION */


/*
 *----------------------------------------------------------------------
 *
 * AsynchHandlerOkay --
 *
 *	Determine if we can force the given process to invoke a signal 
 *	handler.
 *
 * Results:
 *	TRUE if the process is in a state that permits it to call a 
 *	handler for an asynchronous signal, FALSE otherwise.  Sets
 *	*suspendedPtr to TRUE if the process's thread was suspended.
 *
 * Side effects:
 *	May suspend the process's thread.
 *
 *----------------------------------------------------------------------
 */

static Boolean
AsynchHandlerOkay(procPtr, suspendedPtr)
    Proc_LockedPCB *procPtr;	/* the process to check */
    Boolean *suspendedPtr;	/* OUT: did we suspend the process's thread */
{
    /* 
     * XXX Eventually we might want to suspend the thread and then either
     * check its PC, or (maybe) figure out if it's doing a mach_msg.  For
     * the time being, though, we're punting on asynchronous signal
     * delivery.  Be careful not to allow signal delivery if the process is
     * suspended.  Also need to think some more about race conditions
     * between exceptions and asynchronously delivered signals.
     */
    
#ifdef lint
    suspendedPtr = suspendedPtr;
    procPtr = procPtr;
#endif

    return FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_ExcToSig --
 *
 *	Machine-independent mappings from Mach exceptions to Sprite 
 *	signals.
 *
 * Results:
 *	Fills in the Sprite signal number, subcode, and address.  Note: 
 *	callers should normally use SigMach_ExcToSig.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Sig_ExcToSig(exceptionType, exceptionCode, exceptionSubcode, sigNumPtr,
		 codePtr, sigAddrPtr)
    int exceptionType;
    int exceptionCode;
    int exceptionSubcode;
    int *sigNumPtr;		/* OUT: signal number */
    int *codePtr;		/* OUT: signal subcode */
    Address *sigAddrPtr;	/* OUT: the guilty address, if any */
{
    int sigNum = 0;
    int code = SIG_NO_CODE;

    switch (exceptionType) {
    case EXC_BAD_ACCESS:
	sigNum = SIG_ADDR_FAULT;
	switch (exceptionCode) {
	case KERN_INVALID_ADDRESS:
	    code = SIG_ADDR_ERROR;
	    break;
	case KERN_PROTECTION_FAILURE:
	    code = SIG_ACCESS_VIOL;
	    break;
	}
	break;
    case EXC_BAD_INSTRUCTION:
	sigNum = SIG_ILL_INST;
	break;
    case EXC_ARITHMETIC:
	sigNum = SIG_ARITH_FAULT;
	break;
    case EXC_EMULATION:
	sigNum = 29;		/* SIGEMT; see compatSig.h XXX */
	break;
    case EXC_SOFTWARE:
	sigNum = SIG_ARITH_FAULT;
	break;
    case EXC_BREAKPOINT:
	sigNum = SIG_BREAKPOINT;
	break;
    default:
	panic("Sig_ExcToSig: unexpected exception type: %d\n",
	      exceptionType);
	break;
    }

    *sigNumPtr = sigNum;
    *codePtr = code;
    *sigAddrPtr = (Address)exceptionSubcode;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SetUpHandler --
 *
 *	Set up a user process to call a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Puts information on the stack for the handler and changes the
 *	thread's state so that when it resumes it calls the signal handler.
 *
 *----------------------------------------------------------------------
 */

void
Sig_SetUpHandler(procPtr, suspended, sigStackPtr, pc)
    Proc_LockedPCB *procPtr;	/* the process that's taking the signal */
    Boolean suspended;		/* was the thread already suspended */
    Sig_Stack *sigStackPtr;	/* information to put on the thread's stack */
    Address pc;			/* start address of the signal handler */
{
    kern_return_t kernStatus;
    ReturnStatus status = SUCCESS;

    if (!suspended) {
	kernStatus = thread_suspend(procPtr->pcb.thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("%s: can't suspend thread for pid %x: %s\n",
		   "Sig_SetUpHandler", procPtr->pcb.processID,
		   mach_error_string(kernStatus));
	    status = FAILURE;
	    goto done;
	}
    }

    /* 
     * Abort any pending system call (e.g., if there was an exception).
     */
    kernStatus = thread_abort(procPtr->pcb.thread);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't abort pending operation for pid %x: %s\n",
	       "Sig_SetUpHandler", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	status = FAILURE;
	goto done;
    }

    /* 
     * Save the thread's current state, and mung the stack, stack pointer, 
     * and PC so that the trampoline routine will get called next.
     */
    status = SigMach_SetSignalState(procPtr, sigStackPtr, pc);
    if (status != SUCCESS) {
	goto done;
    }

    /* 
     * If we are asynchronously invoking a handler, the target process can 
     * be resumed now.  If the handler is being invoked because of an 
     * exception, the process shouldn't be resumed until we're done 
     * processing the exception call.
     */
    if (procPtr->pcb.genFlags & PROC_BEING_SERVED) {
	procPtr->pcb.genFlags |= PROC_NEEDS_WAKEUP;
    } else {
	/* 
	 * Currently (2-Mar-92), asynchronous signal handling is done by 
	 * having the process request information about the signal, so 
	 * this code shouldn't get executed.
	 */
	printf("Sig_SetUpHandler: warning: resuming pid %x\n",
	       procPtr->pcb.processID);
	kernStatus = thread_resume(procPtr->pcb.thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("Sig_SetUpHandler: can't resume pid %x: %s\n",
		   procPtr->pcb.processID, mach_error_string(kernStatus));
	    status = FAILURE;
	    goto done;
	}
    }

done:
    if (status != SUCCESS) {
	printf("Sig_SetupHandler killing pid %x.\n",
	       procPtr->pcb.processID);
	(void)Sig_SendProc(procPtr, SIG_KILL, FALSE, PROC_MACH_FAILURE,
			   (Address)0);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_RestoreAfterSignal --
 *
 *	Restore a process's state after a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	Aborts the thread's current call, resets the process's registers
 *	from the values that were saved before calling the handler, and
 *	marks the thread to be resumed.
 *
 *----------------------------------------------------------------------
 */

void
Sig_RestoreAfterSignal(procPtr, sigContextPtr)
    Proc_LockedPCB *procPtr;	/* the process to restore */
    Sig_Context *sigContextPtr;	/* state information to restore from */
{
    kern_return_t kernStatus;

    kernStatus = thread_suspend(procPtr->pcb.thread);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't suspend thread for pid %x: %s\n",
	       "Sig_RestoreAfterSignal", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	goto done;
    }
    kernStatus = thread_abort(procPtr->pcb.thread);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't abort pending operation for pid %x: %s\n",
	       "Sig_RestoreAfterSignal", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	goto done;
    }

    if (SigMach_RestoreState(procPtr, sigContextPtr) != SUCCESS) {
	kernStatus = KERN_FAILURE;
	goto done;
    }

    procPtr->pcb.genFlags |= PROC_NEEDS_WAKEUP;

 done:
    if (kernStatus != KERN_SUCCESS) {
	printf("Sig_RestoreAfterSignal killing pid %x.\n",
	       procPtr->pcb.processID);
	(void)Sig_SendProc(procPtr, SIG_KILL, FALSE, PROC_MACH_FAILURE,
			   (Address)0);
    }
}
