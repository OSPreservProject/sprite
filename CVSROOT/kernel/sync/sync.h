/*
 * sync.h --
 *
 * 	Definitions of the synchronization module.
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

#include "sprite.h"
#include "list.h"

#ifdef KERNEL
#include "user/sync.h"
#include "proc.h"
#include "syncLock.h"
#include "sys.h"
#include "mach.h"
#else
#include <sync.h>
#include <kernel/proc.h>
#include <kernel/syncLock.h>
#include <kernel/sys.h>
#include <kernel/mach.h>
#endif /* */

/*
 * If CLEAN_LOCK is defined then don't register locks and don't keep track
 * of lock dependency pairs.
 */
#ifdef CLEAN_LOCK
#undef LOCKREG
#undef LOCKDEP
#endif

/*
 * If LOCKDEP is  defined then we need to register locks.
 */
#ifdef LOCKDEP
#define LOCKREG
#endif

/*
 * Flags for syncFlags field in the proc table:
 *
 *  SYNC_WAIT_REMOTE            - The process is on a wait list somewhere on
 *                                some host and will block until it gets a
 *                                wakeup message.
 *  SYNC_WAIT_COMPLETE          - The process was doing a remote wait and
 *                                it has received a wakeup message.
 */

#define	SYNC_WAIT_COMPLETE	0x1
#define	SYNC_WAIT_REMOTE	0x2

/*
 * Definitions of variables for instrumentation.
 */

typedef struct Sync_Instrument {
    int numWakeups;		/* number of wakeups performed */
    int numWakeupCalls;		/* number of calls to wakeup */
    int numSpuriousWakeups;	/* number of incorrectly awakened sleeps */
    int numLocks;		/* number of calls to MASTER_LOCK */
    int numUnlocks;		/* number of calls to MASTER_UNLOCK */
    int spinCount[SYNC_MAX_LOCK_TYPES+1]; /* spin count per lock type */
    int sched_MutexMiss;	/* number of times we missed sched_Mutex
				 * in the idle loop. */
    char pad[VMMACH_CACHE_LINE_SIZE];
} Sync_Instrument;

/*
 * Structure used to keep track of lock statistics and registration. 
 * One of these exists for each type of lock, and the active locks of the type
 * is linked to the activeLocks field. 
 */
typedef struct Sync_RegElement {
    List_Links 		links;			/* used to link into lists */
    int			hit;			/* number of hits on type */
    int			miss;			/* number of misses on type */
    int			type;			/* type of lock */
    char		*name;			/* name of type */
    Sync_LockClass	class;			/* either semaphore or lock */
    int			priorCount;		/* count of prior types */
    int			priorTypes[SYNC_MAX_PRIOR]; /* prior types */
    int			activeLockCount;	/* # active locks of type */
    List_Links		activeLocks;		/* list of active locks */
    int			deadLockCount;		/* # deactivated locks */
} Sync_RegElement;

/*
 * Define a structure to keep track of waiting processes on remote machines.
 */

typedef struct {
    List_Links	links;		/* Link info about related waiting processes */
    int		hostID;		/* Host ID of waiting process */
    Proc_PID	pid;		/* ID of waiting process */
    int		waitToken;	/* Local time stamp used to catch races */
} Sync_RemoteWaiter;

/*
 * Wait token value used to wakeup a process regardless of its value of
 * the wait token.
 */
#define	SYNC_BROADCAST_TOKEN	-1

/*
 * Exported procedures and variables of the sync module.
 */
    
extern Sync_Instrument 	sync_Instrument[MACH_MAX_NUM_PROCESSORS];
extern Sync_Instrument	*sync_InstrumentPtr[MACH_MAX_NUM_PROCESSORS];
extern int sync_BusyWaits;

extern 	void 		Sync_Init();

extern 	void 		Sync_WakeupProcess();
extern 	void 		Sync_EventWakeup();
extern 	void 		Sync_WakeWaitingProcess();
extern 	void 		Sync_UnlockAndSwitch();

extern 	Boolean 	Sync_SlowMasterWait();
extern 	Boolean 	Sync_SlowWait();
extern 	Boolean 	Sync_EventWait();
extern 	Boolean 	Sync_WaitTime();
extern 	Boolean 	Sync_WaitTimeInTicks();
extern 	Boolean 	Sync_WaitTimeInterval();

extern 	Boolean 	Sync_ProcWait();
extern 	void 		Sync_ProcWakeup();
extern 	void 		Sync_GetWaitToken();
extern 	void 		Sync_SetWaitToken();
extern 	ReturnStatus 	Sync_RemoteNotify();
extern 	ReturnStatus 	Sync_RemoteNotifyStub();
	
extern 	ReturnStatus 	Sync_SlowLockStub();
extern 	ReturnStatus 	Sync_SlowWaitStub();
extern 	ReturnStatus 	Sync_SlowBroadcastStub();

extern 	void 		Sync_PrintStat();

extern	void		Sync_LockStatInit();
extern	void		SyncAddPriorInt();
extern	void		SyncDeleteCurrentInt();
extern 	void		SyncMergePriorInt();
extern	void		Sync_RegisterInt();
extern	void		Sync_CheckoutInt();
extern	void		Sync_PrintLockStats();

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
	    SyncRecordHit(semaphore); \
	    SyncStoreDbgInfo(semaphore); \
	    SyncAddPrior(semaphore); \
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
	    SyncRecordMiss(semaphore); \
	} \
	SyncRecordHit(semaphore) ; \
	SyncStoreDbgInfo(semaphore); \
	SyncAddPrior(semaphore);	\
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

#elif (defined(LOCKREG)) 

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE, 0, SYNC_LISTINFO_INIT, name,(Address) NIL, \
     (Address) NIL}

#elif (defined(LOCKDEP))

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE, 0, SYNC_LISTINFO_INIT, name,(Address) NIL, \
     (Address) NIL, 0}

#else

#define Sync_SemInitStatic(name) \
    {0,name, (Address) NIL, (Address) NIL}

#endif 

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

#elif (defined(LOCKREG)) 

#define Sync_SemInitDynamic(sem, semName) { \
	(sem)->value = (sem)->miss = 0; (sem)->name = semName; \
	(sem)->hit = 0; (sem)->type = 0;\
	(sem)->holderPC = (Address)NIL; (sem)->class = SYNC_SEMAPHORE;\
	(sem)->holderPCBPtr = (Address) NIL; \
}

#elif (defined(LOCKDEP))

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

#endif

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

#elif (defined(LOCKREG)) 

#define Sync_LockInitStatic(name) \
    {0,0,0,SYNC_LOCK, 0, SYNC_LISTINFO_INIT, 0, name, (Address) NIL, \
     (Address) NIL}

#elif (defined(LOCKDEP))

#define Sync_LockInitStatic(name) \
    {0,0,0,SYNC_LOCK, 0, SYNC_LISTINFO_INIT, 0, name, (Address) NIL, \
     (Address) NIL,0}

#else

#define Sync_LockInitStatic(name) \
    {0,0,name, (Address) NIL, (Address) NIL}


#endif 


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

#elif (defined(LOCKREG)) 

#define Sync_LockInitDynamic(lock, lockName) { \
    (lock)->inUse = (lock)->waiting = 0; (lock)->class = SYNC_LOCK;\
    (lock)->hit = (lock)->miss = 0; (lock)->name = lockName; \
    (lock)->holderPC = (Address) NIL; (lock)->type = 0; \
    (lock)->holderPCBPtr  = (Address) NIL; \
}

#elif (defined(LOCKDEP))

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
 * SyncAddPrior --
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

#define SyncAddPrior(lockPtr) { \
    SyncAddPriorInt((lockPtr)->type, &(lockPtr)->priorCount, \
    (lockPtr)->priorTypes, (lockPtr), (lockPtr)->holderPCBPtr);  \
}

#else /* LOCKDEP */

#define SyncAddPrior(lockPtr)

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

#endif /* _SYNC */

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
 * SyncRecordHit --
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

#define SyncRecordHit(lock) {}

#else /* LOCKREG */

#define SyncRecordHit(lock) {	(lock)->hit++; }

#endif /* LOCKREG */

/*
 *----------------------------------------------------------------------
 *
 * SyncRecordMiss --
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

#define SyncRecordMiss(lock) {}

#else /* LOCKREG */

#define SyncRecordMiss(lock) { (lock)->miss++; }

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

#define SyncStoreDbgInfo(semaphore) {}

#else /* CLEAN_LOCK */

#define SyncStoreDbgInfo(semaphore) { \
	    (semaphore)->holderPC = Mach_GetPC(); \
	    (semaphore)->holderPCBPtr = (Address) Proc_GetCurrentProc(); \
}

#endif /* CLEAN_LOCK */

