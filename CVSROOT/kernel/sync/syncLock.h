/*
 * syncLock.h --
 *
 *	Definitions of locks and semaphores. This has to be seperate from
 *	sync.h to prevent circular dependencies with proc.h. See sync.h
 *	for more information.
 *	
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SYNCLOCK
#define _SYNCLOCK

#ifdef KERNEL
#include <user/list.h>
#include <user/sync.h>
#else
#include <list.h>
#include <user/sync.h>
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
 * Classes of locks. The "class" field of both locks and semaphores is 
 * at the same offset within the structures. This allows routines to determine
 * the class of a parameter.
 */
typedef enum Sync_LockClass {
    SYNC_SEMAPHORE,			
    SYNC_LOCK
} Sync_LockClass;

/*
 * Structure that defines a semaphore or spin lock.
 */
typedef struct Sync_Semaphore {
    /*
     * The value field must be first.
     */
    int value;				/* value of semaphore */

#ifdef LOCKREG
    int miss;				/* count of misses on lock */
    int	hit;				/* count of lock hits */
    /*
     * The class field must be at the same offset in both locks and semaphores.
     */
    Sync_LockClass class;		/* class of lock (semaphore) */
    int type;				/* id of lock name */
    Sync_ListInfo listInfo;		/* used to link these into lists */
#endif /* LOCKREG */

#ifndef CLEAN_LOCK
    char *name;				/* name of semaphore */
    Address holderPC;			/* pc of lock holder */
    Address holderPCBPtr;		/* process holding lock */
#endif /*CLEAN_LOCK */

#ifdef LOCKDEP
    int priorCount;			/* count of locks that were grabbed
					 * immediately before this one */
    int priorTypes[SYNC_MAX_PRIOR];     /* types of prior locks */
#endif /* LOCKDEP */

} Sync_Semaphore;

/*
 * Structure that defines a kernel monitor lock.
 */

typedef struct Sync_KernelLock{
    /*
     * The inUse and waiting fields must be first and in this order.
     */
    Boolean inUse;			/* 1 while the lock is busy */
    Boolean waiting;	        	/* 1 if someone wants the lock */
#ifdef LOCKREG
    int hit;				/* number of times lock is grabbed */
    /*
     * The class field must be at the same offset in both locks and semaphores.
     */
    Sync_LockClass class;		/* class of lock (lock) */
    int type;				/* type of lock */
    Sync_ListInfo listInfo;		/* used to put locks into lists */
    int miss;				/* number of times lock is missed */
#endif /* LOCKREG */

#ifndef CLEAN_LOCK
    char *name;				/* name of lock type */
    Address holderPC;			/* pc of lock holder */
    Address holderPCBPtr;		/* process holding lock */
#endif /*CLEAN_LOCK */

#ifdef LOCKDEP
    int priorCount;			/* count of locks that were grabbed
					 * immediately before this one */
    int priorTypes[SYNC_MAX_PRIOR];     /* types of prior locks */

#endif /* LOCKDEP */
} Sync_KernelLock;


#ifdef KERNEL
typedef Sync_KernelLock Sync_Lock;	/* define locks for kernel */
#endif


#endif /* _SYNCLOCK */

