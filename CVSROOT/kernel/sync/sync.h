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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SYNC
#define _SYNC

#include "sprite.h"
#include "list.h"

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
    int miss;				/* count of misses on lock */
    int	hit;				/* count of lock hits */
    /*
     * The class field must be at the same offset in both locks and semaphores.
     */
    Sync_LockClass class;		/* class of lock (semaphore) */
    char *name;				/* name of semaphore */
    Address holderPC;			/* pc of lock holder */
    Proc_ControlBlock *holderPCBPtr;	/* process id of lock holder */
    int priorCount;			/* count of locks that were grabbed
					 * immediately before this one */
    int type;				/* id of lock name */
    Sync_ListInfo listInfo;		/* used to link these into lists */
    int priorTypes[SYNC_MAX_PRIOR];     /* types of prior locks */
} Sync_Semaphore;

typedef struct Sync_KernelLock{
    /*
     * The inUse and waiting fields must be first and in this order.
     */
    Boolean inUse;			/* 1 while the lock is busy */
    Boolean waiting;	        	/* 1 if someone wants the lock */
    int hit;				/* number of times lock is grabbed */
    /*
     * The class field must be at the same offset in both locks and semaphores.
     */
    Sync_LockClass class;		/* class of lock (lock) */
    int miss;				/* number of times lock is missed */
    char *name;				/* name of lock type */
    Address holderPC;			/* pc of lock holder */
    Proc_ControlBlock *holderPCBPtr;	/* process id of lock holder */
    int priorCount;			/* count of locks that were grabbed
					 * immediately before this one */
    int type;				/* type of lock */
    Sync_ListInfo listInfo;		/* used to put locks into lists */
    int priorTypes[SYNC_MAX_PRIOR];     /* types of prior locks */
} Sync_KernelLock;

#ifdef KERNEL
typedef Sync_KernelLock Sync_Lock;	/* define locks for kernel */
#endif

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
    
extern Sync_Instrument 	sync_Instrument;
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
extern	void		SyncAddPriorLock();
extern	void		SyncDeleteCurrentLock();
extern 	void		SyncMergePriorLocks();
extern	void		Sync_RegisterAnyLock();
extern	void		Sync_CheckoutAnyLock();
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
#ifndef CLEAN_LOCK  /* locking statistics version */
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]++; \
	} \
	if ((semaphore)->value == 1) { \
	    panic("Deadlock!!!(%s @ 0x%x)\nHolder PC: 0x%x Current PC: 0x%x\nHolder PCB @ 0x%x Current PCB @ 0x%x\n", \
		(semaphore)->name,(int)(semaphore),(int)(semaphore)->holderPC,\
		(int) Mach_GetPC(),(int) (semaphore)->holderPCBPtr, \
		(int) Proc_GetCurrentProc()); \
	} else { \
	    (semaphore)->value++;\
	    (semaphore)->hit++; \
	    (semaphore)->holderPC = Mach_GetPC(); \
	    (semaphore)->holderPCBPtr = Proc_GetCurrentProc(); \
	    SyncAddPrior((semaphore)->type, &((semaphore)->priorCount), \
			     (semaphore)->priorTypes, (Address) (semaphore), \
			     (semaphore)->holderPCBPtr); \
	}\
    }
#else  			/* multiprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
	int missFlag = 0;\
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]++; \
	} \
	for(;;) { \
	    /* \
	     * wait until semaphore looks free -- avoid bouncing between \
	     * processor caches. \
	     */ \
	    while((semaphore)->value != 0) { \
	    } \
	    if(Mach_TestAndSet(&((semaphore)->value)) == 0) { \
		break; \
	    } else if (missFlag == 0) { \
		missFlag = 1; \
	    } \
	} \
	if(missFlag == 1) { \
	    (semaphore)->miss++; \
	} \
	(semaphore)->hit++; \
	(semaphore)->holderPC = Mach_GetPC(); \
	(semaphore)->holderPCBPtr = Proc_GetCurrentProc(); \
	SyncAddPrior((semaphore)->type, &((semaphore)->priorCount), \
			     (semaphore)->priorTypes, (Address) (semaphore), \
			     (semaphore)->holderPCBPtr); \
    }
#endif  /* multiprocessor implementation */
#else   /* CLEAN */
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]++; \
	} \
	if ((semaphore)->value == 1) { \
	    panic("Deadlock!!! (semaphore @ 0x%x)\n", (int)(semaphore)); \
	} \
	(semaphore)->value = 1; \
    }

#else  			/* multiprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]++; \
	} \
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

#endif /*multiprocessor implementation */
#endif /* CLEAN */


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

#ifdef NOLOCKDEP

#define MASTER_UNLOCK(semaphore) \
    { \
        sync_Instrument.numUnlocks++; \
	(semaphore)->value = 0; \
	if (!Mach_AtInterruptLevel()) { \
	    --mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]; \
	    if (mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()] == 0) { \
		Mach_EnableIntr(); \
	    } \
	} \
    }

#else  /* NOLOCKDEP */

#define MASTER_UNLOCK(semaphore) \
    { \
        sync_Instrument.numUnlocks++; \
	(semaphore)->value = 0; \
	SyncDeleteCurrentLock((Address)(semaphore),(semaphore)->holderPCBPtr);\
	if (!Mach_AtInterruptLevel()) { \
	    --mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]; \
	    if (mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()] == 0) { \
		Mach_EnableIntr(); \
	    } \
	} \
    }
#endif /* NOLOCKDEP */

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

#ifdef CLEAN_LOCK
#define NOLOCKDEP

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE,(char *)NIL,(Address)NIL,(Proc_ControlBlock *) NIL,0}

#else /* CLEAN_LOCK */

#define Sync_SemInitStatic(name) \
    {0,0,0, SYNC_SEMAPHORE, (name),(Address) NIL,(Proc_ControlBlock *) NIL,0,0}

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
    { \
	(sem)->value = (sem)->miss = 0; (sem)->name = (char *)NIL; \
	(sem)->hit = 0; \
	(sem)->holderPC = (Address)NIL; (sem)->class = SYNC_SEMAPHORE;\
	(sem)->holderPCBPtr = (Proc_ControlBlock *) NIL; \
	(sem)->priorCount = 0; \
    }

#else /* CLEAN_LOCK */

#define Sync_SemInitDynamic(sem,semName) \
    { \
	(sem)->value = (sem)->miss = 0; (sem)->name = (semName); \
	(sem)->hit = 0; \
	(sem)->holderPC = (Address)NIL; (sem)->class = SYNC_SEMAPHORE;\
	(sem)->holderPCBPtr = (Proc_ControlBlock *) NIL; \
	(sem)->priorCount = 0; (sem)->type = 0; \
    }

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

#else /* CLEAN_LOCK */

#define Sync_LockInitStatic(name) \
    {0,0,0,SYNC_LOCK, 0, (name), (Address) NIL, (Proc_ControlBlock *) NIL,0}

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

#define Sync_LockInitDynamic(lock, name) {(lock)->inUse = (lock)->waiting = 0;}

#else /* CLEAN_LOCK */

#define Sync_LockInitDynamic(lock, lockName) { \
    (lock)->inUse = (lock)->waiting = 0; (lock)->class = SYNC_LOCK;\
    (lock)->hit = (lock)->miss = 0; (lock)->name = (lockName); \
    (lock)->holderPC = (Address) NIL; \
    (lock)->holderPCBPtr  = (Proc_ControlBlock *) NIL; \
    (lock)->priorCount = 0; (lock)->type = 0; \
}

#endif /* CLEAN_LOCK */


/*
 *----------------------------------------------------------------------
 *
 * Sync_IsRegistered --
 *
 *	Returns true if a lock or semaphore has already been registered. If
 *	CLEAN_LOCK is defined then this macro always returns FALSE.
 *
 * Results:
 *	TRUE if lock or semaphore registered.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef CLEAN_LOCK

#define Sync_IsRegistered(lock) FALSE

#else /* CLEAN_LOCK */

#define Sync_IsRegistered(lock) \
    (((lock)->type > 0) ? TRUE : FALSE)

#endif /* CLEAN_LOCK */

/*
 *----------------------------------------------------------------------
 *
 * Sync_LockRegister --
 *
 *	Used to add a lock to the registration database.
 *	If CLEAN_LOCK is defined then this macro does nothing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is registered.
 *
 *----------------------------------------------------------------------
 */
#ifdef CLEAN_LOCK

#define Sync_LockRegister(sem) {}

#else /* CLEAN_LOCK */

#define Sync_LockRegister(lock) \
    { \
	if (!Sync_IsRegistered(lock)) { \
	    Sync_RegisterAnyLock((Address) (lock)); \
	} \
    }

#endif /* CLEAN_LOCK */



/*
 *----------------------------------------------------------------------
 *
 * Sync_LockClear --
 *
 *	Used to clear and deregister a lock before it is freed.
 *	If CLEAN_LOCK is defined then this macro does nothing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is deregistered.
 *
 *----------------------------------------------------------------------
 */
#ifdef CLEAN_LOCK

#define Sync_LockClear(sem) 

#else /* CLEAN_LOCK */

#define Sync_LockClear(lock) \
    { \
	if (Sync_IsRegistered(lock)) { \
	    Sync_CheckoutAnyLock((Address) (lock)); \
	} \
    }

#endif /* CLEAN_LOCK */

/*
 *----------------------------------------------------------------------
 *
 * SyncAddPrior --
 *
 *	When a lock is grabbed the prior lock must be added to the prior
 *	types for the lock.
 *	This macro does nothing if NOLOCKDEP is defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef NOLOCKDEP

#define SyncAddPrior(type, priorCount, priorTypes, lockPtr, pcbPtr)

#else /* NOLOCKDEP */

#define SyncAddPrior(type, priorCount, priorTypes, lockPtr, pcbPtr) \
    { SyncAddPriorLock((type), (priorCount), (priorTypes), (lockPtr), \
    (pcbPtr)); }

#endif /* NOLOCKDEP */

/*
 *----------------------------------------------------------------------
 *
 * SyncMergePrior --
 *
 *	When a lock is cleared its statistics must be added to those for
 *	the type as a whole.
 *	This macro does nothing if NOLOCKDEP is defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef NOLOCKDEP

#define SyncMergePrior(priorCount, priorTypes, regPtr)

#else /* NOLOCKDEP */

#define SyncMergePrior(priorCount, priorTypes, regPtr) \
    { SyncMergePriorLocks((priorCount), (priorTypes), (regPtr)); } 

#endif /* NOLOCKDEP */

#endif /* _SYNC */
