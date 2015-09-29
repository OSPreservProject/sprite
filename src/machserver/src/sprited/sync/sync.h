/*
 * sync.h --
 *
 * 	Definitions of routines for the synchronization module.
 * 	The synchronization module provides locks and condition
 * 	variables to other modules, plus a low level binary semaphore 
 *	needed to synchronize with interrupt handlers.
 *
 *	The behavior of the sync module can be modified using compiler 
 *	variables. These variables will change the structure of locks and
 *	how the locks are used. In general it is not a good idea to link
 *	modules that have been compiled with different versions of locks.
 *	
 *	    <default> -	 semaphores and locks have fields that contain a
 *		 	 character string name, the pc where the lock was
 *			 last locked, and a pointer to the pcb of the last
 *			 lock holder. The locking operation is slower because
 *			 these fields must be updated.
 *
 *	    CLEAN_LOCK - locks do not contain any extra fields. This version
 *		         of locks is intended for benchmarking the kernel.
 *	
 *	    LOCKREG    - locks are registered so that the information stored
 *			 in them can be retrieved. In addition to the fields
 *			 in the default version of locks, a count of hits
 *			 and misses on each lock is kept. Lock registration
 *			 must be done when the lock is created and destroyed.
 *			 The locking operation is slower due to the hit/miss
 *			 counters.  A count is kept for each spin lock that
 *			 records the number of times a processor spun waiting
 *			 for the lock.
 *
 *	    LOCKDEP    - Each lock keeps a list of locks that were held when
 *			 it was locked in addition to the information kept
 *			 in the LOCKREG version. Locks compiled with LOCKDEP
 *		         will get very large. This information can be used to 
 *			 construct a graph of the locking behavior of the
 *			 kernel. Locking and unlocking is slowed down due
 *			 to the necessity of recording previously held lock.
 *
 *
 * Copyright 1986, 1991 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/sync/RCS/sync.h,v 1.10 92/04/02 20:53:41 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SYNC
#define _SYNC

#include <sprite.h>
#include <spriteTime.h>
/* 
 * The kernel proc.h (or procTypes.h, if one is created) isn't 
 * included, to avoid possible circular dependencies between the sync 
 * and proc type definitions.  So we use "struct foo *" instead of 
 * "foo *" for things that point to proc objects.
 */

#ifdef SPRITED

#include <user/proc.h>
#include <user/sync.h>

#include <rpcTypes.h>
#include <syncTypes.h>
#include <timerTick.h>
#include <utilsMach.h>

#else /* SPRITED */

#include <proc.h>
#include <sync.h>

#include <kernel/rpcTypes.h>
#include <kernel/syncTypes.h>
#include <kernel/timerTick.h>
#include <kernel/utilsMach.h>

#endif /* SPRITED */

/*
 * These include files are needed by the SysV sema support.
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


/* 
 * Aliases for compatibility with native Sprite code.
 */

#define MASTER_LOCK		Sync_GetLock
#define MASTER_UNLOCK		Sync_Unlock
#define Sync_MasterBroadcast	Sync_Broadcast
#define Sync_MasterWait(condPtr, mutexPtr, wakeIfSignal) \
    (void)Sync_SlowWait((condPtr), (mutexPtr), (wakeIfSignal))
#define Sync_SlowLock		Sync_GetLock
#define Sync_SemInitStatic	Sync_LockInitStatic
#define Sync_SemInitDynamic	Sync_LockInitDynamic


/*
 * Exported procedures and variables of the sync module.
 */
    
extern void Sync_Init _ARGS_((void));
extern void Sync_LockInitDynamic _ARGS_((Sync_Lock *lockPtr, char *lockName));
extern void Sync_ConditionFree _ARGS_((Sync_Condition *condPtr));
extern void Sync_ConditionInit _ARGS_((Sync_Condition *condPtr,
				       char *name, Boolean force));

#if defined(SPRITED) || defined(KERNEL)
extern void Sync_GetLock _ARGS_((Sync_Lock *lockPtr));
extern void Sync_Unlock _ARGS_((Sync_Lock *lockPtr));
extern void Sync_Broadcast _ARGS_((Sync_Condition *conditionPtr));
#endif

extern Boolean Sync_SlowWait _ARGS_((Sync_Condition *conditionPtr, 
			Sync_Lock *lockPtr, Boolean wakeIfSignal));

extern void Sync_UnlockAndSwitch _ARGS_((Sync_Lock *lockPtr,
					 Proc_State state));
extern void Sync_WakeWaitingProcess _ARGS_((struct Proc_LockedPCB *procPtr));
extern void Sync_WakeupProcess _ARGS_((Timer_Ticks time, ClientData procAddress));

extern void Sync_GetWaitToken _ARGS_((Proc_PID *pidPtr, int *tokenPtr));
extern void Sync_SetWaitToken _ARGS_((struct Proc_ControlBlock *procPtr,
				      int waitToken));

extern Boolean Sync_WaitTime _ARGS_((Time time));
extern Boolean Sync_WaitTimeInTicks _ARGS_((Timer_Ticks time));
extern Boolean Sync_WaitTimeInterval _ARGS_((Time interval));

extern Boolean Sync_ProcWait _ARGS_((Sync_Lock *lockPtr,
				     Boolean wakeIfSignal));
extern void Sync_ProcWakeup _ARGS_((Proc_PID pid, int token));

extern ReturnStatus Sync_RemoteNotify _ARGS_((Sync_RemoteWaiter *waitPtr));
extern ReturnStatus Sync_RemoteNotifyStub _ARGS_((ClientData srvToken, 
			int clientID, int command, Rpc_Storage *storagePtr));

extern void Sync_PrintStat _ARGS_((void));

extern void Sync_LockStatInit _ARGS_((void));
extern void SyncAddPriorInt _ARGS_((int type, int *priorCountPtr, 
			int *priorTypes, Address lockPtr, 
			struct Proc_ControlBlock *pcbPtr));

extern void SyncDeleteCurrentInt _ARGS_((Address lockPtr, 
				struct Proc_ControlBlock *pcbPtr));
extern void SyncMergePriorInt _ARGS_((int priorCount, int *priorTypes, 
				Sync_RegElement *regPtr));
extern void Sync_RegisterInt _ARGS_((Address lock));
extern void Sync_CheckoutInt _ARGS_((Address lock));

extern ReturnStatus Sync_SemgetStub _ARGS_((long key, int nsems, int semflg, 
					int *retValOut));
extern ReturnStatus Sync_SemopStub _ARGS_((int semid, struct sembuf *sopsIn, 
					int nsops, int retValOut));

extern ReturnStatus Sync_SemctlStub _ARGS_((int semid, int semnum, int cmd, 
					union semun arg, int *retValOut));
extern ReturnStatus Sync_SemStruct _ARGS_((int id, int *perm, 
					   Sync_SysVSem **retPtr));

extern ReturnStatus Sync_Sleep _ARGS_((Time time));
extern void Sync_SemInit _ARGS_((void));

extern ReturnStatus Sync_GetLockStats _ARGS_((int size, Address argPtr));
extern ReturnStatus Sync_ResetLockStats _ARGS_((void));

extern void Sync_RemoveWaiter _ARGS_((struct Proc_ControlBlock *procPtr));

extern Sync_RegElement  *regQueuePtr;


#if 0
extern ReturnStatus Sync_SlowLockStub _ARGS_((Sync_UserLock *lockPtr));
extern ReturnStatus Sync_SlowWaitStub _ARGS_((unsigned int event, 
			Sync_UserLock *lockPtr, Boolean wakeIfSignal));
extern ReturnStatus Sync_SlowBroadcastStub _ARGS_((unsigned int event,
				int *waitFlagPtr));
#endif /* 0 */


/*
 *----------------------------------------------------------------------
 *
 * UNLOCK_AND_SWITCH --
 *
 *	Macro to call the internal routine to release the monitor lock and
 *	then context switch.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lock released and process context switched.
 *
 *----------------------------------------------------------------------
 */

#define UNLOCK_MONITOR_AND_SWITCH(state) Sync_UnlockAndSwitch(LOCKPTR, state)


/*
 *	The initialization routines are used to initialize a semaphore.
 *	The name parameter is the name of the semaphore and is used to
 *	distinguish between types of locks. If you want the statistics
 *	(hit, miss, etc) for two semaphores to be shared then give them the
 *	same name. An example might be locks around file handles, where it
 *	makes more sense to have the statistics for all the locks as a whole,
 *	rather than just one lock. Also beware that no distinction is made
 *	between locks and semaphores when determining types -- if they have
 *	the same name they have the same type.
 *	   Sync_SemClear should be called when a semaphore is being deallocated
 *	and will be no longer used. Currently all this routine does is to
 *	take the statistics associated with the semaphore and merge them in
 *	with those for the type as a whole.
 */

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemRegister --
 *
 * 	Register a semaphore.
 *
 *----------------------------------------------------------------------
 */
#define Sync_SemRegister Sync_LockRegister

/* 
 *----------------------------------------------------------------------
 *
 * Sync_SemClear
 * 
 * 	Clear a semaphore.
 *
 *----------------------------------------------------------------------
 */
#define Sync_SemClear Sync_LockClear


/*
 *----------------------------------------------------------------------
 *
 * Sync_LockInitStatic --
 *
 *	Initializes the fields of a lock in an initialization statement.
 *	Ex:
 *		static Sync_Lock foo = Sync_LockInitStatic("foo");
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef CLEAN_LOCK

#define Sync_LockInitStatic(name) \
    {{SPIN_LOCK_INITIALIZER, name, QUEUE_INITIALIZER, SPIN_LOCK_INITIALIZER}}

#else

#ifdef LOCKDEP

#define Sync_LockInitStatic(name) \
    {{SPIN_LOCK_INITIALIZER, name, QUEUE_INITIALIZER, SPIN_LOCK_INITIALIZER},\
     0, 0, SYNC_LISTINFO_INIT, 0, (Address) NIL, \
     (struct Proc_ControlBlock *) NIL, 0}

#else

#ifdef LOCKREG

#define Sync_LockInitStatic(name) \
    {{SPIN_LOCK_INITIALIZER, name, QUEUE_INITIALIZER, SPIN_LOCK_INITIALIZER},\
     0, 0, SYNC_LISTINFO_INIT, 0, (Address) NIL, \
     (struct Proc_ControlBlock *) NIL}

#else

#define Sync_LockInitStatic(name) \
    {{SPIN_LOCK_INITIALIZER, name, QUEUE_INITIALIZER, SPIN_LOCK_INITIALIZER},\
     (Address) NIL, (struct Proc_ControlBlock *) NIL}

#endif /* LOCKDEP */
#endif /* LOCKREG */
#endif /* CLEAN_LOCK */


/*
 *----------------------------------------------------------------------
 *
 * Sync_IsRegistered --
 *
 *	Returns true if a lock or semaphore has already been registered. If
 *	LOCKREG is not defined then this macro always returns FALSE.
 *
 * Results:
 *	TRUE if lock or semaphore registered.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef LOCKREG

#define Sync_IsRegistered(lock) \
    (((lock)->type > 0) ? TRUE : FALSE)

#else /* LOCKREG */

#define Sync_IsRegistered(lock) FALSE

#endif /* LOCKREG */

/*
 *----------------------------------------------------------------------
 *
 * Sync_LockRegister --
 *
 *	Used to add a lock to the registration database.
 *	If LOCKREG is not defined then this macro does nothing.
 *	XXX Sync_RegisterInt needs some mutex protection in case of 
 *	concurrent calls.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is registered.
 *
 *----------------------------------------------------------------------
 */
#ifdef LOCKREG

#define Sync_LockRegister(lock) \
    { \
	if (!Sync_IsRegistered((Sync_Lock *) lock)) { \
	    Sync_RegisterInt((Address) (lock)); \
	} \
    }

#else /* LOCKREG */

#define Sync_LockRegister(lock) {}

#endif /* LOCKREG */



/*
 *----------------------------------------------------------------------
 *
 * Sync_LockClear --
 *
 *	Used to clear and deregister a lock before it is freed.
 *	If LOCKREG is not defined then this macro does nothing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is deregistered.
 *
 *----------------------------------------------------------------------
 */
#ifdef LOCKREG

#define Sync_LockClear(lock) \
    { \
	if (Sync_IsRegistered(lock)) { \
	    Sync_CheckoutInt((Address) (lock)); \
	} \
    }

#else /* LOCKREG */

#define Sync_LockClear(sem) 

#endif /* LOCKREG */

/*
 *----------------------------------------------------------------------
 *
 * Sync_AddPrior --
 *
 *	When a lock is grabbed the prior lock must be added to the prior
 *	types for the lock.
 *	This macro does nothing if LOCKDEP is not defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef LOCKDEP

#define Sync_AddPrior(lockPtr) { \
    Sync_AddPriorInt((lockPtr)->type, &(lockPtr)->priorCount, \
    (lockPtr)->priorTypes, (lockPtr), (lockPtr)->holderPCBPtr);  \
}

#else /* LOCKDEP */

#define Sync_AddPrior(lockPtr)

#endif /* LOCKDEP */

/*
 *----------------------------------------------------------------------
 *
 * SyncMergePrior --
 *
 *	When a lock is cleared its statistics must be added to those for
 *	the type as a whole.
 *	This macro does nothing if LOCKDEP is not defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef LOCKDEP

#define SyncMergePrior(lockPtr, regPtr) \
    { SyncMergePriorInt((lockPtr)->priorCount, (lockPtr)->priorTypes, \
               (regPtr)); } 

#else /* LOCKDEP */

#define SyncMergePrior(lockPtr, regPtr)

#endif /* LOCKDEP */


/*
 *----------------------------------------------------------------------
 *
 * SyncDeleteCurrent --
 *
 *	When we unlock a lock we have to delete it from the stack of 
 *	current locks.
 *	If LOCKDEP is not defined then don't do anything.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is removed from the lock stack in the current pcb.
 *
 *----------------------------------------------------------------------
 */

#ifdef LOCKDEP

#define SyncDeleteCurrent(lockPtr) \
    { SyncDeleteCurrentInt((lockPtr), (lockPtr)->holderPCBPtr); }

#else /* LOCKDEP */

#define SyncDeleteCurrent(lockPtr) 

#endif /* LOCKDEP */


/*
 *----------------------------------------------------------------------
 *
 * SyncDeadlockPanic --
 *
 *	Prints out a warning message and calls panic. There is one
 *	version for clean locks, and another version for printing
 *	debugging information found in the locks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	panic is called.
 *
 *----------------------------------------------------------------------
 */

#ifdef CLEAN_LOCK

#define SyncDeadlockPanic(semaphore) { \
	    panic("Deadlock!!! (semaphore @ 0x%x)\n", (int)(semaphore)); \
}

#else /* CLEAN_LOCK */

#define SyncDeadlockPanic(semaphore) { \
	    panic("Deadlock!!!(%s @ 0x%x)\nHolder PC: 0x%x Current PC: 0x%x\nHolder PCB @ 0x%x Current PCB @ 0x%x\n", \
		(semaphore)->name,(int)(semaphore),(int)(semaphore)->holderPC,\
		(int) UtilsMach_GetPC(),(int) (semaphore)->holderPCBPtr, \
		(int) Proc_GetCurrentProc()); \
}

#endif /* CLEAN_LOCK */

/*
 *----------------------------------------------------------------------
 *
 * Sync_RecordHit --
 *
 *	If LOCKREG is defined then the hit field of the lock
 *	is incremented.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef LOCKREG 

#define Sync_RecordHit(lock) {}

#else /* LOCKREG */

#define Sync_RecordHit(lock) {	(lock)->hit++; }

#endif /* LOCKREG */

/*
 *----------------------------------------------------------------------
 *
 * Sync_RecordMiss --
 *
 *	If LOCKREG is defined then the miss field of the lock
 *	is incremented.
 *	XXX Needs mutex protection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifndef LOCKREG

#define Sync_RecordMiss(lock) {}

#else /* LOCKREG */

#define Sync_RecordMiss(lock) { (lock)->miss++; }

#endif /* LOCKREG */

/*
 *----------------------------------------------------------------------
 *
 * Sync_StoreDbgInfo --
 *
 *	If CLEAN_LOCK isn't defined then store debugging information
 *	in the lock.  If "lockedByMacro" is TRUE, the lock was obtained
 *	via a macro, so we record the current PC, which is in the
 *	function that invoked the macro.  If it's FALSE, the lock was
 *	obtained by a function whose sole job is to obtain the lock,
 *	so we want to record the PC of that function's caller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef CLEAN_LOCK

#define Sync_StoreDbgInfo(semaphore, lockedByMacro) {}

#else /* CLEAN_LOCK */

#define Sync_StoreDbgInfo(semaphore, lockedByMacro) { 			\
    if ((lockedByMacro) == TRUE) {					\
	(semaphore)->holderPC = (Address) UtilsMach_GetPC(); 		\
    } else {								\
	(semaphore)->holderPC = (Address) UtilsMach_GetCallerPC(); 	\
    }									\
    (semaphore)->holderPCBPtr = Proc_GetCurrentProc(); 	\
}

#endif /* CLEAN_LOCK */
#endif /* _SYNC */

