/*
 * syncTypes.h --
 *
 * 	Definitions of types for the synchronization module.
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

#ifndef _SYNCTYPES
#define _SYNCTYPES

#ifdef KERNEL
#include "user/sync.h"
#include "procTypes.h"
#include "syncLock.h"
#include "machTypes.h"
#else
#include <sync.h>
#include <kernel/procTypes.h>
#include <kernel/syncLock.h>
#include <kernel/machTypes.h>
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
#if VMMACH_CACHE_LINE_SIZE != 0
    char pad[VMMACH_CACHE_LINE_SIZE];
#endif
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
 * Structure for System V semaphores.
 */
typedef struct semid_ds Sync_SysVSem;


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

#endif /* _SYNCTYPES */

