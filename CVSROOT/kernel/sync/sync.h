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
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SYNC
#define _SYNC

#include <sprite.h>
#include <list.h>

#ifdef KERNEL
#include <syncTypes.h>
#include <mach.h>
#include <proc.h>
#else
#include <kernel/syncTypes.h>
#include <kernel/mach.h>
#include <kernel/proc.h>
#endif /* KERNEL */

/*
 * These include files are needed by the SysV sema support.
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 * Exported procedures and variables of the sync module.
 */
    
extern Sync_Instrument 	sync_Instrument[MACH_MAX_NUM_PROCESSORS];
extern Sync_Instrument	*sync_InstrumentPtr[MACH_MAX_NUM_PROCESSORS];
extern int sync_BusyWaits;

extern void Sync_Init _ARGS_((void));
extern ReturnStatus Sync_GetLock _ARGS_((Sync_Lock *lockPtr));
extern ReturnStatus Sync_Unlock _ARGS_((Sync_Lock *lockPtr));
extern ReturnStatus Sync_SlowLock _ARGS_((register Sync_Lock *lockPtr));
extern Boolean Sync_SlowWait _ARGS_((Sync_Condition *conditionPtr, 
			Sync_Lock *lockPtr, Boolean wakeIfSignal));
extern ReturnStatus Sync_SlowBroadcast _ARGS_((unsigned int event, 
			int *waitFlagPtr));

extern Boolean Sync_SlowMasterWait _ARGS_((unsigned int event,
			Sync_Semaphore *mutexPtr, Boolean wakeIfSignal));
extern void Sync_UnlockAndSwitch _ARGS_((Sync_Lock *lockPtr, Proc_State state));
extern void Sync_WakeWaitingProcess _ARGS_((register Proc_ControlBlock *procPtr));
extern void Sync_WakeupProcess _ARGS_((Timer_Ticks time, ClientData procAddress));

extern void Sync_GetWaitToken _ARGS_((Proc_PID *pidPtr, int *tokenPtr));
extern void Sync_SetWaitToken _ARGS_((Proc_ControlBlock *procPtr, int waitToken));

extern Boolean Sync_WaitTime _ARGS_((Time time));
extern Boolean Sync_WaitTimeInTicks _ARGS_((Timer_Ticks time));
extern Boolean Sync_WaitTimeInterval _ARGS_((unsigned int interval));

extern Boolean Sync_ProcWait _ARGS_((Sync_Lock *lockPtr, Boolean wakeIfSignal));
extern void Sync_ProcWakeup _ARGS_((Proc_PID pid, int token));

extern ReturnStatus Sync_RemoteNotify _ARGS_((Sync_RemoteWaiter *waitPtr));
extern ReturnStatus Sync_RemoteNotifyStub _ARGS_((ClientData srvToken, 
			int clientID, int command, Rpc_Storage *storagePtr));

extern ReturnStatus Sync_SlowLockStub _ARGS_((Sync_UserLock *lockPtr));
extern ReturnStatus Sync_SlowWaitStub _ARGS_((unsigned int event, 
			Sync_UserLock *lockPtr, Boolean wakeIfSignal));
extern ReturnStatus Sync_SlowBroadcastStub _ARGS_((unsigned int event,
				int *waitFlagPtr));

extern void Sync_PrintStat _ARGS_((void));

extern void Sync_LockStatInit _ARGS_((void));
extern void SyncAddPriorInt _ARGS_((int type, int *priorCountPtr, 
			int *priorTypes, Address lockPtr, 
			Proc_ControlBlock *pcbPtr));

extern void SyncDeleteCurrentInt _ARGS_((Address lockPtr, 
				Proc_ControlBlock *pcbPtr));
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

extern void Sync_RemoveWaiter _ARGS_((Proc_ControlBlock *procPtr));

extern Sync_RegElement  *regQueuePtr;



/*
 *----------------------------------------------------------------------------
 *
 * MASTER_LOCK --
 *
 *	Enter a critical section guarded by a binary semaphore.
 *	This is for use in a multiprocessor environment
 *	within the synchronization module, and in other
 *	modules that interact with interrupt-time routines.
 *	(All other synchronization should be done with Monitors.)
 *	
 *	Interrupts are disabled on the local processor to prevent
 *	a preemptive context switch.  The semaphore is checked
 *	with a Mach_TestAndSet atomic operation in a busy wait
 *	to prevent races with other processors.
 *
 * 	For uniprocessor debugging, panic when the lock is held (otherwise
 * 	we get an infinite loop).
 *
 *	There are three versions of this macro. This is due to the different
 *	sizes of locks. There is only one uniprocessor version. It uses
 *	other macros that are modified by compiler variables. There are
 *	two versions of the multiprocessor implementation. The first is
 *	used when we are keeping hit/miss ratios and the second is for
 *	when we are not.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	The semaphore has its value set from 0 to 1.
 *	Interrupts are disabled.
 *
 *----------------------------------------------------------------------------
 */

#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */

#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument[Mach_GetProcessorNumber()].numLocks++; \
	DISABLE_INTR(); \
	if ((semaphore)->value == 1) { \
	    SyncDeadlockPanic((semaphore)); \
	} else { \
	    (semaphore)->value++;\
	    Sync_SemRegister(semaphore); \
	    Sync_RecordHit(semaphore); \
	    Sync_StoreDbgInfo(semaphore, TRUE); \
	    Sync_AddPrior(semaphore); \
	}\
    }

#else  			/* multiprocessor implementation */

#ifdef LOCKREG

#define MASTER_LOCK(semaphore) \
    { \
	int missFlag = 0;\
	int pnum = Mach_GetProcessorNumber();\
	int type = ((semaphore)->type > 0) ? (semaphore)->type : 0;\
        sync_InstrumentPtr[pnum]->numLocks++; \
	DISABLE_INTR(); \
	for(;;) { \
	    /* \
	     * wait until semaphore looks free -- avoid bouncing between \
	     * processor caches. \
	     */ \
	    while((semaphore)->value != 0) { \
		if (missFlag == 0) { \
		    missFlag = 1; \
		} \
		sync_InstrumentPtr[pnum]->spinCount[type]++;\
	    } \
	    if(Mach_TestAndSet(&((semaphore)->value)) == 0) { \
		break; \
	    } else if (missFlag == 0) { \
		missFlag = 1; \
	    } \
	    sync_InstrumentPtr[pnum]->spinCount[type]++;\
	} \
	if(missFlag == 1) { \
	    Sync_RecordMiss(semaphore); \
	} \
	Sync_SemRegister(semaphore); \
	Sync_RecordHit(semaphore) ; \
	Sync_StoreDbgInfo(semaphore, TRUE); \
	Sync_AddPrior(semaphore);	\
    }

#else   /* LOCKREG -- These are the clean versions of the macros */

#define MASTER_LOCK(semaphore) \
    { \
        sync_InstrumentPtr[Mach_GetProcessorNumber()]->numLocks++; \
	DISABLE_INTR(); \
	for(;;) { \
	    /* \
	     * wait until semaphore looks free -- avoid bouncing between \
	     * processor caches. \
	     */ \
	    while((semaphore)->value != 0) { \
	    } \
	    if(Mach_TestAndSet(&((semaphore)->value)) == 0) { \
		break; \
	    } \
	} \
    }

#endif /* LOCKREG */
#endif /*multiprocessor implementation */


/*
 *----------------------------------------------------------------------------
 *
 * MASTER_UNLOCK --
 *
 *	Leave a critical section guarded by a binary semaphore.  This is for
 *	use in a multiprocessor environment.  Interrupts are enabled and the
 *	semaphore value is reset to 0 to allow other processors entry into
 *	the critical section.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	The semaphore has its value reset to 0.
 *	Interrupts are enabled.
 *
 *----------------------------------------------------------------------------
 */

#define MASTER_UNLOCK(semaphore) \
    { \
	int pnum = Mach_GetProcessorNumber();\
        sync_InstrumentPtr[pnum]->numUnlocks++; \
	(semaphore)->value = 0; \
	SyncDeleteCurrent(semaphore); \
	if (!Mach_AtInterruptLevel()) { \
	    --mach_NumDisableIntrsPtr[pnum]; \
	    if (mach_NumDisableIntrsPtr[pnum] == 0) { \
		Mach_EnableIntr(); \
	    } \
	} \
    }


/* 
 * Condition variables can be used in critical sections guarded by
 * MASTER_LOCK and MASTER_UNLOCK.  Sync_MasterWait and
 * Sync_MasterBroadcast are the analogues of Sync_Wait and
 * Sync_Broadcast.
 */

/*
 *----------------------------------------------------------------------
 *
 * Sync_MasterWait --
 *
 *	Wait on an event with a master lock held.
 *	This has the same semantics as Sync_Wait except
 *	that the lock release when the process sleeps is
 *	a master lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process gets descheduled, the master lock gets released
 *	and then reacquired after the condition is notified.
 *
 *----------------------------------------------------------------------
 */

#define Sync_MasterWait(conditionPtr, mutexPtr, wakeIfSignal) \
    { \
	(conditionPtr)->waiting = TRUE; \
	(void) Sync_SlowMasterWait((unsigned int) conditionPtr, mutexPtr, \
		wakeIfSignal); \
    }

/*
 *----------------------------------------------------------------------
 *
 * Sync_MasterBroadcast --
 *
 *	Notify an event, like Sync_Broadcast except it
 *	should be used with a master lock held because of the
 *	check on conditionPtr->waiting.
 *
 *	(This could verify that a master lock is held.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notify all processes waiting on the event.
 *
 *----------------------------------------------------------------------
 */

#define Sync_MasterBroadcast(conditionPtr) \
    { \
	if ((conditionPtr)->waiting == TRUE) { \
	    (void) Sync_SlowBroadcast((unsigned int)conditionPtr, \
				&(conditionPtr)->waiting); \
	} \
    }


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
 * Sync_SemInitStatic --
 *
 *	Initializes the fields of a semaphore in an initialization statement.
 *	Ex:
 *		static Sync_Semaphore foo = Sync_SemInitStatic("foo");
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

#define Sync_SemInitStatic(name) \
    {0}

#else
#ifdef LOCKREG

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE, 0, SYNC_LISTINFO_INIT, name,(Address) NIL, \
     (Address) NIL}
#else
#ifdef LOCKDEP

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE, 0, SYNC_LISTINFO_INIT, name,(Address) NIL, \
     (Address) NIL, 0}

#else

#define Sync_SemInitStatic(name) \
    {0,name, (Address) NIL, (Address) NIL}

#endif /* LOCKDEP */
#endif /* LOCKREG */
#endif /* CLEAN_LOCK */


/*
 *----------------------------------------------------------------------
 *
 * Sync_SemInitDynamic --
 *
 *	Initializes the fields of a semaphore during program execution.
 *	Ex:
 *		Sync_SemInitDynamic(foo,"foo");
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

#define Sync_SemInitDynamic(sem,semName) \
    { (sem)->value = 0; }

#else
#ifdef LOCKREG

#define Sync_SemInitDynamic(sem, semName) { \
	(sem)->value = (sem)->miss = 0; (sem)->name = semName; \
	(sem)->hit = 0; (sem)->type = 0;\
	(sem)->holderPC = (Address)NIL; (sem)->class = SYNC_SEMAPHORE;\
	(sem)->holderPCBPtr = (Address) NIL; \
}

#else
#ifdef LOCKDEP

#define Sync_SemInitDynamic(sem, semName) { \
	(sem)->value = (sem)->miss = 0; (sem)->name = semName; \
	(sem)->hit = 0; (sem)->type = 0; \
	(sem)->holderPC = (Address)NIL; (sem)->class = SYNC_SEMAPHORE;\
	(sem)->holderPCBPtr = (Address) NIL; \
	(sem)->priorCount = 0;\
}


#else

#define Sync_SemInitDynamic(sem, semName) { \
	(sem)->value = 0; (sem)->name = semName; \
	(sem)->holderPC = (Address)NIL;\
	(sem)->holderPCBPtr = (Address) NIL; \
}
#endif /* LOCKDEP */
#endif /* LOCKREG */
#endif /* CLEAN_LOCK */


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

#define Sync_LockInitStatic(name) {0,0}

#else
#ifdef LOCKREG

#define Sync_LockInitStatic(name) \
    {0,0,0,SYNC_LOCK, 0, SYNC_LISTINFO_INIT, 0, name, (Address) NIL, \
     (Address) NIL}

#else
#ifdef LOCKDEP

#define Sync_LockInitStatic(name) \
    {0,0,0,SYNC_LOCK, 0, SYNC_LISTINFO_INIT, 0, name, (Address) NIL, \
     (Address) NIL,0}

#else

#define Sync_LockInitStatic(name) \
    {0,0,name, (Address) NIL, (Address) NIL}


#endif /* LOCKDEP */
#endif /* LOCKREG */
#endif /* CLEAN_LOCK */


/*
 *----------------------------------------------------------------------
 *
 * Sync_LockInitDynamic --
 *
 *	Initializes the fields of a lock during program execution.
 *	Ex:
 *		Sync_LockInitDynamic(foo,"foo");
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

#define Sync_LockInitDynamic(lock, lockName) \
    {(lock)->inUse = (lock)->waiting = 0;}

#else
#ifdef LOCKREG

#define Sync_LockInitDynamic(lock, lockName) { \
    (lock)->inUse = (lock)->waiting = 0; (lock)->class = SYNC_LOCK;\
    (lock)->hit = (lock)->miss = 0; (lock)->name = lockName; \
    (lock)->holderPC = (Address) NIL; (lock)->type = 0; \
    (lock)->holderPCBPtr  = (Address) NIL; \
}

#else
#ifdef LOCKDEP

#define Sync_LockInitDynamic(lock, lockName) { \
    (lock)->inUse = (lock)->waiting = 0; (lock)->class = SYNC_LOCK;\
    (lock)->hit = (lock)->miss = 0; (lock)->name = lockName; \
    (lock)->holderPC = (Address) NIL; (lock)->type = 0; \
    (lock)->holderPCBPtr  = (Address) NIL; \
    (lock)->priorCount = 0;\

#else

#define Sync_LockInitDynamic(lock, lockName) { \
    (lock)->inUse = (lock)->waiting = 0;  (lock)->name = lockName; \
    (lock)->holderPC = (Address) NIL; \
    (lock)->holderPCBPtr  = (Address) NIL; \
}

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
		(int) Mach_GetPC(),(int) (semaphore)->holderPCBPtr, \
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
 * SyncStorDbgInfo --
 *
 *	If CLEAN_LOCK isn't defined then store debugging information
 *	in the lock.
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

#define Sync_StoreDbgInfo(semaphore, macro) {}

#else /* CLEAN_LOCK */

#define Sync_StoreDbgInfo(semaphore, macro) { 				\
    if ((macro) == TRUE) {						\
	(semaphore)->holderPC = (Address) Mach_GetPC(); 	\
    } else {								\
	(semaphore)->holderPC = (Address) Mach_GetCallerPC(); \
    }									\
	(semaphore)->holderPCBPtr = (Address) Proc_GetCurrentProc(); 	\
}

#endif /* CLEAN_LOCK */
#endif /* _SYNC */

