/*
 * sync.h --
 *
 * 	Definitions of the synchronization module.
 * 	The synchronization module provides locks and condition
 * 	variables to other modules, plus a low level binary semaphore 
 *	needed to synchronize with interrupt handlers.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /sprite/src/kernel/sync/RCS/sync.h,v 8.11 89/04/06 12:09:40 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _SYNC
#define _SYNC

#include "sprite.h"
#include "list.h"
#include "boot.h"
#ifdef KERNEL
#include "proc.h"
#include "user/sync.h"
#include "sys.h"
#include "mach.h"
#else
#include <sync.h>
#include <kernel/sys.h>
#include <kernel/proc.h>
#include <kernel/mach.h>
#endif /* */

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
} Sync_Instrument;

/*
 * This is used inside the Sync_Semaphore and Sync_Lock structures to allow
 * them to be linked into lists. Usually the links field is first in a
 * structure so that the list routines work correctly. The CLEAN_LOCK
 * version of locks do not use the links field and expect the value of
 * the lock to be the first field. The easiest solution is to put the
 * links inside a structure which in turn is inside the locks. The linked
 * list elements are these inner structures, which in turn have a pointer
 * to the lock that contains them.
 */
typedef struct Sync_ListInfo {
    List_Links	links;		/* used to link into lists */
    Address	lock;		/* ptr at outer structure that contains this
				 * structure */
} Sync_ListInfo;

/*
 * Classes of locks. The "class" field of both locks and semaphores is 
 * at the same offset within the structures. This allows routines to determine
 * the class of a parameter.
 */
typedef enum Sync_LockClass {
    SYNC_SEMAPHORE,			
    SYNC_LOCK
} Sync_LockClass;

/*
 *  Maximum types of locks. Types are assigned as locks are registered, 
 *  starting at 1. No distiction is made between locks and semaphores when
 *  assigning a type. The type is used as an index into the array of
 *  statistics for that lock type. Unregistered locks have a type of 0,
 *  and the type of the lock that protects the lock registration itself is
 *  -1. We have to treat this lock specially because a lock is registered
 *  after it is locked, and we need to lock the registration lock in order
 *  to register a lock. Hence we can't register the registration lock.
 */

#define SYNC_MAX_LOCK_TYPES 50

/*
 * Semaphore structure
 */
typedef struct Sync_Semaphore {
    /*
     * The value field must be first.
     */
    int value;				/* value of semaphore */
} Sync_Semaphore;

typedef struct Sync_KernelLock{
    /*
     * The inUse and waiting fields must be first and in this order.
     */
    Boolean inUse;			/* 1 while the lock is busy */
    Boolean waiting;	        	/* 1 if someone wants the lock */
} Sync_KernelLock;

#ifdef KERNEL
typedef Sync_KernelLock Sync_Lock;	/* define locks for kernel */
#endif


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
extern	void		SyncAddPriorLock();
extern	void		SyncDeleteCurrentLock();
extern 	void		SyncMergePriorLocks();
extern	void		Sync_RegisterAnyLock();
extern	void		Sync_CheckoutAnyLock();
extern	void		Sync_PrintLockStats();


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
 *
 * For uniprocessor debugging, panic when the lock is held (otherwise
 * we get an infinite loop).
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

#define MASTER_LOCK(semaphore) 
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

#define MASTER_UNLOCK(semaphore) 


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

#define Sync_MasterWait(conditionPtr, mutexPtr, wakeIfSignal) (boot_Poll)()
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

#define Sync_MasterBroadcast(conditionPtr) 

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
 * Obsolete.
 *
 *----------------------------------------------------------------------
 */
#define Sync_SemRegister Sync_LockRegister

/* 
 *----------------------------------------------------------------------
 *
 * Sync_SemClear
 * 
 * Obsolete.
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


#define Sync_SemInitStatic(name) {0}



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


#define Sync_SemInitDynamic(sem,semName) \
    { \
	(sem)->value =  0;    }


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

#define Sync_LockInitStatic(name) {0}


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


#define Sync_LockInitDynamic(lock, name) {(lock)->inUse = (lock)->waiting = 0;}

#endif /* _SYNC */

