/* 
 * syncLock.c --
 *
 *	Sprite locks.  The mutex variables provided by C Threads are 
 *	the basic locking mechanism.  We provide additional 
 *	instrumentation and debugging support.
 *	
 *	The lock and condition variable code changes the state of the 
 *	current process without locking the process.  This is okay because 
 *	the only code that might cause a race is the process exit/kill 
 *	code.  This code only sets the process's state if it doesn't have a 
 *	Sprite thread running (in which case there's no race after all).
 *
 * Copyright 1985, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/sync/RCS/syncLock.c,v 1.11 92/05/27 21:23:18 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <ckalloc.h>
#include <cthreads.h>
#include <stdio.h>
#include <string.h>

#include <proc.h>
#include <rpc.h>
#include <sig.h>
#include <sync.h>
#include <utils.h>

/* 
 * Define a magic number for the "initialized" flag for condition 
 * variables.  This helps us detect garbage condition variables, especially 
 * those allocated on the stack.
 */
#define INITIALIZED_COND	996

/* 
 * Monitor lock to serialize access to the process's state when 
 * waiting on a condition variable.  This is to avoid race conditions 
 * between signals and a process's sleeping.
 */
static struct mutex signalsMutex;

/* 
 * This mutex variable avoids races between processes that 
 * want to initialize a condition variable.
 */
static struct mutex condInitMutex;

/* 
 * This lock is used to serialize access to a process's syncFlags and 
 * waitToken, which are used when waiting on a remote host.
 */
static Sync_Lock remoteLock = Sync_LockInitStatic("sync:remoteLock");

/*
 * Statistics related to remote waiting.
 */
int syncProcWakeupRaces = 0;

/* 
 * Locks held during initialization, when there's no "current process".
 */
static nullProcLocks = 0;

/* Forward references: */

static void CheckUnlock _ARGS_ ((Sync_Lock *lockPtr));
static void SyncBroadcastInt _ARGS_((Sync_Condition *condPtr));


/*
 *----------------------------------------------------------------------
 *
 * Sync_Init --
 *
 *	Initialize primitives used by the sync module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the C Threads packages and some private mutex 
 *	variables. 
 *
 *----------------------------------------------------------------------
 */
    
void
Sync_Init()
{
    cthread_init();

    mutex_init(&signalsMutex);
    mutex_set_name(&signalsMutex, "sync:signalsMutex");
    mutex_init(&condInitMutex);
    mutex_set_name(&condInitMutex, "sync:condInitMutex");
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_LockInitDynamic --
 *
 *	Initialize a lock at run time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the fields of the lock.
 *
 *----------------------------------------------------------------------
 */

void
Sync_LockInitDynamic(lockPtr, lockName)
    Sync_Lock *lockPtr;		/* lock to initialize */
    char *lockName;		/* name for the lock; must not change later */
{
    mutex_init(&lockPtr->mutex);
    mutex_set_name(&lockPtr->mutex, lockName);
#ifndef CLEAN_LOCK
    lockPtr->holderPC = (Address)NIL;
    lockPtr->holderPCBPtr = (Proc_ControlBlock *)NIL;
#endif /* CLEAN_LOCK */
#ifdef LOCKREG
    lockPtr->hit = 0;
    lockPtr->type = 0;
    lockPtr->miss = 0;
    List_InitElement((List_Links *)&lockPtr->listInfo);
    lockPtr->listInfo.lock = lockPtr;
#ifdef LOCKDEP
    lockPtr->priorCount = 0;
#endif /* LOCKDEP */
#endif /* LOCKREG */
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_GetLock --
 *
 *	Obtain a monitor lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The type of the previous lock is added to the array of prior types.
 *	The lock is added to the lock stack in the pcb.
 *
 *----------------------------------------------------------------------
 */

void
Sync_GetLock(lockPtr)
   Sync_Lock *lockPtr;
{
    Proc_ControlBlock	*procPtr = NULL;
    Proc_State		oldState; /* state of process, should be PROC_READY */

    procPtr = Proc_GetCurrentProc();
    Sync_LockRegister(lockPtr);

    if (!mutex_try_lock(&lockPtr->mutex)) {
	oldState = procPtr->state;
#ifndef CLEAN_LOCK
	if (oldState != PROC_READY) {
	    printf("Sync_GetLock called with state = %s\n",
		   Proc_StateName(oldState));
	}
#endif
	Proc_SetState(procPtr, PROC_WAITING);
	mutex_lock(&lockPtr->mutex);
	Proc_SetState(procPtr, oldState);
	Sync_RecordMiss(lockPtr);
    } 

    Sync_RecordHit(lockPtr);
    Sync_StoreDbgInfo(lockPtr, FALSE);
    Sync_AddPrior(lockPtr);

#ifndef CLEAN_LOCK
    if (procPtr == NULL) {
	nullProcLocks++;
    } else {
	procPtr->locksHeld++;
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
 *  Sync_Unlock--
 *
 *	Release a monitor lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is removed from the lock stack in the pcb.
 *
 *----------------------------------------------------------------------
 */

void
Sync_Unlock(lockPtr)
    Sync_Lock *lockPtr;
{

#ifndef CLEAN_LOCK
    CheckUnlock(lockPtr);
#endif

    SyncDeleteCurrent(lockPtr);
    mutex_unlock(&lockPtr->mutex);
}


/*
 *----------------------------------------------------------------------------
 *
 * Sync_SlowWait --
 *
 *      Wait on a condition.  The lock is released and the process is blocked
 *      until the condition variable is signaled.  Before returning
 *	the lock is reaquired.
 *
 *      This can only be called while a lock is held.  This forces our
 *      client to safely check global state while in a monitor.
 *
 * Results:
 *	TRUE if interrupted because of signal, FALSE otherwise.
 *
 * Side effects:
 *      Put the process to sleep and release the monitor lock.  Other
 *      processes waiting on the monitor lock become runnable.
 *
 *----------------------------------------------------------------------------
 */

ENTRY Boolean
Sync_SlowWait(conditionPtr, lockPtr, wakeIfSignal)
    Sync_Condition	*conditionPtr;	/* Condition to wait on. */
    register Sync_Lock 	*lockPtr;	/* Lock to release. */
    Boolean		wakeIfSignal;	/* TRUE => wake if signal pending. */
{
    Boolean	sigPending = FALSE;
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();

    if (conditionPtr->initialized &&
	    conditionPtr->initialized != INITIALIZED_COND) {
	panic("Sync_SlowWait: garbage condition variable at 0x%x.\n",
	       conditionPtr);
    }
    if (!conditionPtr->initialized) {
	Sync_ConditionInit(conditionPtr, (char *)NULL, FALSE);
    }

    mutex_lock(&signalsMutex);
    if (wakeIfSignal && Sig_Pending(procPtr)) {
	mutex_unlock(&signalsMutex);
	return TRUE;
    }

    /* 
     * No pending signals, so we actually sleep now.
     */

    if (procPtr->state != PROC_SUSPENDED) {
	Proc_SetState(procPtr, PROC_WAITING);
    }
    procPtr->currCondPtr = conditionPtr;

    /*
     * Release the monitor lock and wait on the condition.  Notice 
     * that we free the given lock pointer, then we use the "monitor 
     * lock" when we actually sleep on the condition variable.  This 
     * is so that no other process will think the current process has 
     * gone to sleep before it actually has.
     */
    Sync_Unlock(lockPtr);
    condition_wait(&conditionPtr->condVar, &signalsMutex);

    if (procPtr->state == PROC_WAITING) {
	Proc_SetState(procPtr, PROC_READY);
    }
    procPtr->currCondPtr = NULL;
    sigPending = Sig_Pending(procPtr);
    mutex_unlock(&signalsMutex);

    (void) Sync_GetLock(lockPtr);

    return(sigPending);
}


/*
 *----------------------------------------------------------------------------
 *
 * Sync_UnlockAndSwitch --
 *
 *      Release the monitor lock and then "context switch" to the
 *	given state, which should be PROC_DEAD or PROC_EXITING.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The current thread exits.
 *	
 *----------------------------------------------------------------------------
 */

void
Sync_UnlockAndSwitch(lockPtr, newState)
    register	Sync_Lock 	*lockPtr;
    Proc_State			newState;
{
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();

    if (newState != PROC_DEAD && newState != PROC_EXITING) {
	panic("Sync_UnlockAndSwitch: unexpected new state.\n");
    }
    CheckUnlock(lockPtr);

    if (procPtr->state != newState) {
	Proc_SetState(procPtr, newState);
    }

    /*
     * Release the monitor lock and context switch.  Don't call 
     * Proc_ContextSwitch, because that would cause the process state to 
     * get (re)set after the monitor lock had been released.
     */
    SyncDeleteCurrent(lockPtr);
    mutex_unlock(&lockPtr->mutex);

    cthread_set_name(cthread_self(), (char *)0);
    cthread_exit(0);
}


/*
 *----------------------------------------------------------------------------
 *
 * Sync_WakeWaitingProcess --
 *
 *	Wake up a particular process as though the local or remote
 *	event it is awaiting has occurred.  If the process is not currently
 *	waiting for anything, or if the process is SUSPENDED, nothing is 
 *	done.
 *	
 *	This is primarily used to wake up a process that has gotten a
 *	signal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      If the named process is waiting on a condition variable, a 
 *      broadcast is done on the condition variable.
 *
 *----------------------------------------------------------------------------
 */

void
Sync_WakeWaitingProcess(procPtr)
    register	Proc_LockedPCB 	*procPtr;
{
    /* 
     * Unlike in native Sprite, there is no special code for the case where
     * the process is waiting on a remote event.  This is because
     * (1) notification for a remote event is done by calling
     *     Sync_ProcWakeup (so Sync_WakeWaitingProcess doesn't have to address
     *     the race condition between sleeping and notification);
     * (2) native Sprite uses different mechanisms for waiting, whereas in
     *     the Sprite server it's all done by waiting on a condition variable.
     */
    mutex_lock(&signalsMutex);
    if (procPtr->pcb.state != PROC_SUSPENDED
	    && procPtr->pcb.currCondPtr != NULL) {
	SyncBroadcastInt(procPtr->pcb.currCondPtr);
    }
    mutex_unlock(&signalsMutex);
}


/*
 *----------------------------------------------------------------------------
 *
 * Sync_GetWaitToken --
 *
 *	Return the process id and increment and return wait token for the 
 *	current process.
 *
 * Results:
 *	process ID and wait token for current process.
 *
 * Side effects:
 *	Wait token incremented.
 *
 *----------------------------------------------------------------------------
 */

void
Sync_GetWaitToken(pidPtr, tokenPtr)
    Proc_PID	*pidPtr;	/* If non-nil pid of current process. */
    int		*tokenPtr;	/* Wait token of current process. */
{
    register	Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();

    Sync_GetLock(&remoteLock);

    procPtr->waitToken++;
    if (pidPtr != (Proc_PID *) NIL) {
	*pidPtr = procPtr->processID;
    }
    *tokenPtr = procPtr->waitToken;

    Sync_Unlock(&remoteLock);
}


#ifdef SPRITED_MIGRATION
/*
 *----------------------------------------------------------------------------
 *
 * Sync_SetWaitToken --
 *
 *	Set the wait token for the given process.  Only exists for process
 *	migration should not be used in general.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Wait token value set in the PCB for the given process.
 *
 *----------------------------------------------------------------------------
 */

ENTRY void
Sync_SetWaitToken(procPtr, waitToken)
    Proc_ControlBlock	*procPtr;	/* Process to set token for. */
    int			waitToken;	/* Token value. */
{
    Sync_GetLock(&remoteLock);

    procPtr->waitToken = waitToken;

    Sync_Unlock(&remoteLock);
}
#endif /* SPRITED_MIGRATION */


/*
 *----------------------------------------------------------------------------
 *
 * Sync_ProcWait --
 *
 *	This is called to block a process after it has been told by a
 *	remote host that it has to wait.  The wait completed flag is
 *	checked here to see if the remote host's wakeup message has raced 
 *	(and won) with this process's decision to call this procedure.
 *
 *	For safety, this routine should be expanded to automatically
 *	set up a timeout event which will wake up the process anyway.
 *
 * Results:
 *	TRUE if woke up because of a signal, FALSE otherwise.
 *
 * Side effects:
 *	The wait complete flag of the process is checked and the process
 *	is blocked if a notify message has not already arrived.
 *
 *----------------------------------------------------------------------------
 */

Boolean
Sync_ProcWait(lockPtr, wakeIfSignal)
    Sync_Lock	*lockPtr;	/* If non-nil release this lock before going to
				 * sleep and reaquire it after waking up. */
    Boolean	wakeIfSignal;	/* TRUE => Don't go to sleep if a signal is
				 *         pending. */
{
    register	Proc_ControlBlock 	*procPtr;
    Boolean				releasedLock = FALSE;
    Boolean				sigPending = FALSE;

    Sync_GetLock(&remoteLock);
    procPtr = Proc_GetCurrentProc();
    if (!(procPtr->syncFlags & SYNC_WAIT_COMPLETE)) {
	if (wakeIfSignal && Sig_Pending(procPtr)) {
	    /*
	     * Check for signals.   If a signal is pending, then bail out.
	     */
	    sigPending = TRUE;
	} else {
	    /*
	     * Block the process.  The wakeup message from the remote host
	     * has not arrived.
	     */
	    procPtr->syncFlags |= SYNC_WAIT_REMOTE;
	    if (lockPtr != (Sync_Lock *) NIL) {
		/*
		 * We were given a monitor lock to release, so release it.
		 */
		Sync_Unlock(lockPtr);
		releasedLock = TRUE;
	    }
	    Sync_SlowWait(&procPtr->remoteCondition, &remoteLock,
			  wakeIfSignal);
	    if (wakeIfSignal && Sig_Pending(procPtr)) {
		sigPending = TRUE;
	    }
	}
    }
    /*
     * After being notified (and context switching back to existence),
     * or if we have already been notified, clear state about the
     * remote wait.  This means our caller should get a new token
     * (ie. retry whatever remote operation it was) before waiting
     * again.
     */
    procPtr->waitToken++;
    procPtr->syncFlags &= ~(SYNC_WAIT_COMPLETE | SYNC_WAIT_REMOTE);
    Sync_Unlock(&remoteLock);
    if (releasedLock) {
	(void) Sync_GetLock(lockPtr);
    }
    return(sigPending);
}


/*
 *----------------------------------------------------------------------------
 *
 * Sync_ProcWakeup --
 *
 *	Wakeup a blocked process in response to a message from a remote
 *	host.  It is possible that the wakeup message has raced and
 *	won against the local process's call to Sync_ProcWait.  This is
 *	protected against with a token and a wakeup complete flag.
 *	(The token provides extra protection against spurious wakeups.
 *	As we don't make any guarantees about the correctness of a
 *	wakeup anyway, we ignore the token here.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A flag is set in the PCB to indicate that the wakeup message has 
 *	arrived.
 *
 *----------------------------------------------------------------------------
 */

/* ARGSUSED */
void
Sync_ProcWakeup(pid, token)
    Proc_PID 	pid;	/* PID of process to wake up. */
    int		token;	/* Token to use to wake up process (unused). */
{
    Proc_ControlBlock 	*procPtr;

    procPtr = Proc_GetPCB(pid);
    if (procPtr != (Proc_ControlBlock *)NIL) {
	Sync_GetLock(&remoteLock);
	procPtr->syncFlags |= SYNC_WAIT_COMPLETE;
	if (procPtr->syncFlags & SYNC_WAIT_REMOTE) {
	    Sync_Broadcast(&procPtr->remoteCondition);
	} else {
	    syncProcWakeupRaces++;
	}
	Sync_Unlock(&remoteLock);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_RemoteNotify --
 *
 *	Perform an RPC to notify a remote process.
 *
 * Results:
 *	The return code from the RPC.  This enables the caller to decide
 *	if it should wait and retry the notify later if the remote
 *	host is unavailable.
 *
 * Side effects:
 *      This results in a call to Sync_ProcWakeup on the host of the
 *      waiting process.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sync_RemoteNotify(waitPtr)
    Sync_RemoteWaiter *waitPtr;		/* Arguments to remote notify. */
{
    Rpc_Storage storage;
    ReturnStatus status;

    storage.requestParamPtr = (Address)waitPtr;
    storage.requestParamSize = sizeof(Sync_RemoteWaiter);
    storage.requestDataSize = 0;
    storage.replyParamSize = 0;
    storage.replyDataSize = 0;
    status = Rpc_Call(waitPtr->hostID, RPC_REMOTE_WAKEUP, &storage);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_RemoteNotifyStub --
 *
 *	The service stub for the remote wakeup RPC.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *      A call to Sync_ProcWakeup on the process.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Sync_RemoteNotifyStub(srvToken, clientID, command, storagePtr)
    ClientData 	srvToken;	/* Handle on server process passed to
				 * Rpc_Reply. */
    int 	clientID;	/* Sprite ID of client host (ignored). */
    int 	command;	/* Command identifier (ignored). */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply. */
{
    register Sync_RemoteWaiter *waitPtr;

    waitPtr = (Sync_RemoteWaiter *)storagePtr->requestParamPtr;
    Sync_ProcWakeup(waitPtr->pid, waitPtr->waitToken);
    Rpc_Reply(srvToken, SUCCESS, storagePtr, (int (*) ()) NIL,
	      (ClientData) NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * CheckUnlock --
 *
 *	Paranoia checks when releasing a monitor lock.  Make 
 *	sure that 
 *	(1) the count of obtained locks is high enough
 *	(2) the lock is actually in use
 *	(3) the process that owns the lock is the process that 
 *	    releases it .
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Decrements the counter in the current process's pcb.  If there 
 *	isn't a current process, decrements the global "no process" 
 *	lock counter.
 *
 *----------------------------------------------------------------------
 */

#ifndef CLEAN_LOCK
static void
CheckUnlock(lockPtr)
    Sync_Lock	*lockPtr;
{
    int			locksHeld; /* locks held by current process */
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    if (procPtr == (Proc_ControlBlock *)NIL) {
	nullProcLocks--;
	locksHeld = nullProcLocks;
    } else {
	procPtr->locksHeld--;
	locksHeld = procPtr->locksHeld;
    }
    if (locksHeld < 0) {
	panic("more unlocks than locks.\n");
    }
    if (mutex_try_lock(&lockPtr->mutex)) {
	mutex_unlock(&lockPtr->mutex);
	panic("unlocking an unlocked lock.\n");
    }

    if (lockPtr->holderPCBPtr != (Proc_ControlBlock *)NIL &&
	    lockPtr->holderPCBPtr != procPtr) {
	panic("unlocking somebody else's lock.\n");
    }
}
#endif /* CLEAN_LOCK */


/*
 *----------------------------------------------------------------------
 *
 * Sync_ConditionInit --
 *
 *	Initialize a Sprite condition variable.  This routine is needed
 *	because in native Sprite condition variables need no
 *	initialization, but that's not true for C Threads conditions.
 *	
 *	The caller is responsible for freeing the name after calling 
 *	Sync_ConditionFree.  (Untangling the FS code so that it properly 
 *	deallocates condition variables owned by various handles is more 
 *	than I want to deal with.  Fortunately, all those condition 
 *	variables use static names.)
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
Sync_ConditionInit(condPtr, name, force)
    Sync_Condition *condPtr;	/* c.v. to initialize */
    char *name;			/* name for the c.v.; maybe NULL */
    Boolean force;		/* do it even if it looks initialized */
{
    char backupName[1024];	/* name for the c.v. if none was given */

    mutex_lock(&condInitMutex);

    if (!condPtr->initialized || force) {
	condition_init(&condPtr->condVar);
	if (name != NULL) {
	    condPtr->dynamicName = FALSE;
	} else {
	    sprintf(backupName, "(Sync_Condition *)0x%x", condPtr);
	    name = ckstrdup(backupName);
	    condPtr->dynamicName = TRUE;
	}
	condition_set_name(&condPtr->condVar, name);
	condPtr->initialized = INITIALIZED_COND;
    }

    mutex_unlock(&condInitMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_Broadcast --
 *
 *      Mark all processes waiting on an event as runnable.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All processes waiting on the given condition variable are 
 *	woken up.
 *
 *----------------------------------------------------------------------
 */

void
Sync_Broadcast(condPtr)
    Sync_Condition *condPtr;
{
    if (condPtr->initialized && condPtr->initialized != INITIALIZED_COND) {
	panic("Sync_Broadcast: garbage condition variable at 0x%x.\n",
	       condPtr);
    }
    if (!condPtr->initialized) {
	Sync_ConditionInit(condPtr, (char *)NULL, FALSE);
    }

    /* 
     * We must hold signalsMutex in order to avoid a race with 
     * Sync_SlowWait. 
     */
    mutex_lock(&signalsMutex);
    condition_broadcast(&condPtr->condVar);
    mutex_unlock(&signalsMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * SyncBroadcastInt --
 *
 *	Like Sync_Broadcast, but the caller is already holding the signals 
 *	mutex. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All processes waiting on the given condition variable are 
 *	woken up.
 *
 *----------------------------------------------------------------------
 */

static void
SyncBroadcastInt(condPtr)
    Sync_Condition *condPtr;
{
    if (condPtr->initialized && condPtr->initialized != INITIALIZED_COND) {
	panic("SyncBroadcastInt: garbage condition variable at 0x%x.\n",
	       condPtr);
    }
    if (!condPtr->initialized) {
	Sync_ConditionInit(condPtr, (char *)NULL, FALSE);
    }
    condition_broadcast(&condPtr->condVar);
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_ConditionFree --
 *
 *	Free any storage held by a condition variable.
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
Sync_ConditionFree(condPtr)
    Sync_Condition *condPtr;
{
    if (condPtr->initialized && condPtr->initialized != INITIALIZED_COND) {
	panic("Sync_ConditionFree: garbage condition variable at 0x%x.\n",
	       condPtr);
    }
    if (condPtr->initialized && condPtr->dynamicName) {
	ckfree(condPtr->condVar.name);
	condition_set_name(&condPtr->condVar, NULL);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_GetConditionName --
 *
 *	Get the name of a condition variable.
 *
 * Results:
 *	Returns the string and a flag saying whether Sync_ConditionFree 
 *	will deallocate the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Sync_GetConditionName(condPtr, namePtr, syncFreesPtr)
    Sync_Condition *condPtr;	/* the condition variable to look at */
    char **namePtr;		/* OUT: the c.v.'s name */
    Boolean *syncFreesPtr;	/* OUT: Sync_ConditionFree frees the name */
{
    if (condPtr->initialized && condPtr->initialized != INITIALIZED_COND) {
	panic("Sync_GetConditionName: garbage condition variable at 0x%x.\n",
	       condPtr);
    }
    if (!condPtr->initialized) {
	*namePtr = NULL;
	*syncFreesPtr = TRUE;
    } else {
	*namePtr = condPtr->condVar.name;
	*syncFreesPtr = condPtr->dynamicName;
    }
}
