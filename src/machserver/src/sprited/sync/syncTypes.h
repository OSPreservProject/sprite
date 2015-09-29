/*
 * syncTypes.h --
 *
 *	Definitions of locks and semaphores and other goodies for the
 *	sync module.  In native Sprite this was separate from sync.h
 *	to prevent circular dependencies with proc.h.  (See sync.h for
 *	more information.)  Dunno if this is still a problem.
 *	
 * Copyright 1989, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/sync/RCS/syncTypes.h,v 1.6 92/07/12 23:25:18 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SYNCTYPES
#define _SYNCTYPES

/* 
 * Don't include procTypes.h.  The actual definition of "struct 
 * Proc_ControlBlock" isn't needed for holderPCBPtr, and including it 
 * introduces a dependency loop (because it needs the definition of 
 * Sync_Semaphore, which is defined below). 
 */

#ifdef SPRITED
#include <cthreads.h>
#include <user/list.h>
#include <user/proc.h>
#include <user/sync.h>
#else
#include <list.h>
#include <proc.h>
#include <sync.h>
#endif

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

typedef int Sync_PCBFlags;

#define	SYNC_WAIT_COMPLETE	0x1
#define	SYNC_WAIT_REMOTE	0x2

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

#define SYNC_MAX_LOCK_TYPES 60

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

#define SYNC_LISTINFO_INIT  \
    {{(struct List_Links *) NIL,(struct List_Links *) NIL}, (Address) NIL}


/*
 * Structure that defines a kernel monitor lock.
 */

typedef struct Sync_KernelLock{
    struct mutex mutex;		/* the thing that is actually locked */
#ifdef LOCKREG
    int hit;				/* number of times lock is grabbed */
    int type;				/* type of lock */
    Sync_ListInfo listInfo;		/* used to put locks into lists */
    int miss;				/* number of times lock is missed */
#endif /* LOCKREG */

#ifndef CLEAN_LOCK
    Address holderPC;			/* pc of lock holder */
    struct Proc_ControlBlock *holderPCBPtr; /* process holding lock */
#endif /*CLEAN_LOCK */

#ifdef LOCKDEP
    int priorCount;			/* count of locks that were grabbed
					 * immediately before this one */
    int priorTypes[SYNC_MAX_PRIOR];     /* types of prior locks */

#endif /* LOCKDEP */
} Sync_KernelLock;

#ifdef SPRITED

typedef Sync_KernelLock Sync_Lock;	/* define locks for kernel */

#endif /* SPRITED */

typedef Sync_Lock Sync_Semaphore;	/* compatibility with old code */


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
