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
#include "user/sync.h"
#include "sys.h"
#include "proc.h"
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

typedef struct Sync_Semaphore {
    int value;			/* value of semaphore */
    int miss;			/* count of misses on lock */
    char *name;			/* name of semaphore */
    Address pc;			/* pc of lock holder */
    char *lineInfo;		/* line/file of lock holder */
} Sync_Semaphore;

#ifdef CLEAN
#define SYNC_SEM_INIT_STATIC(name) {0,0,(char *)NIL,(Address)NIL,(char *)NIL}
#define SYNC_SEM_INIT_DYNAMIC(sem,semName) { \
    (sem)->value = (sem)->miss = 0; (sem)->name = (char *)NIL; \
    (sem)->pc = (Address)NIL; (sem)->lineInfo = (char *)NIL; }

#else
#define SYNC_SEM_INIT_STATIC(name) {0,0,name,(Address) NIL,""}
#define SYNC_SEM_INIT_DYNAMIC(sem,semName) { \
    (sem)->value = (sem)->miss = 0; (sem)->name = semName; \
    (sem)->pc = (Address)NIL; (sem)->lineInfo = ""; }

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
 *	The semaphore is just an integer.
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

/* The following 2 macros are used to turn __LINE__ into a string. The first
 * turns its argument into a string, but it doesn't expand the argument so you
 * end up with the string "__LINE__". The second expands __LINE__ before passing
 * it to the first. These are only defined and used within the MASTER_LOCK
 * and MASTER_UNLOCK definitions
 */

#define _hack(x) #x
#define _hack2(x) _hack(x)

#ifdef lint
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[0]++; \
	} \
	if ((semaphore)->value == 1) { \
	    panic("Deadlock!!! (semaphore @ 0x%x)\n", (int)(semaphore)); \
	} \
	(semaphore)->value == 1; \
    }

#else /* lint */

#ifndef CLEAN
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[0]++; \
	} \
	if ((semaphore)->value == 1) { \
	    panic("Deadlock!!!(%s @ 0x%x)\nHolder PC: 0x%x Current PC: 0x%x\n" \
		"Holder: %s Current: line " _hack2(__LINE__) \
		", file " __FILE__ "\n", \
		(semaphore)->name,(int)(semaphore),(int)(semaphore)->pc,\
		Mach_GetPC(),(semaphore)->lineInfo); \
	} else { \
	    (semaphore)->value++;\
	    (semaphore)->pc = Mach_GetPC(); \
	    (semaphore)->lineInfo = "line " _hack2(__LINE__) \
		", file " __FILE__ ; \
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
		(semaphore)->miss++; \
		missFlag = 1; \
	    } \
	} \
	(semaphore)->pc = Mach_GetPC(); \
	(semaphore)->lineInfo = "line " _hack2(__LINE__) \
		", file " __FILE__ ; \
    }
#endif  /* multiprocessor implementation */
#else   /* CLEAN */
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#define MASTER_LOCK(semaphore) \
    { \
        sync_Instrument.numLocks++; \
	if (!Mach_AtInterruptLevel()) { \
	    Mach_DisableIntr(); \
	    mach_NumDisableIntrsPtr[0]++; \
	} \
	if ((semaphore)->value == 1) { \
	    panic("Deadlock!!! (semaphore @ 0x%x)\n", (int)(semaphore)); \
	} \
	(semaphore)->value == 1; \
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
	    while((semaphore)->value) != 0) { \
	    } \
	    if(Mach_TestAndSet(&((semaphore)->value)) == 0) { \
		break; \
	    } \
	} \
    }

#endif /*multiprocessor implementation */
#endif /* CLEAN */
#endif /* lint */



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
        sync_Instrument.numUnlocks++; \
	(semaphore)->value = 0; \
	if (!Mach_AtInterruptLevel()) { \
	    --mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()]; \
	    if (mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()] == 0) { \
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

#endif /* _SYNC */
