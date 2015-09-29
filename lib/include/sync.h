/*
 * sync.h --
 *
 * 	Typedefs and macros to support the use of monitors.
 *      Clients use the macros to acquire and release monitor locks, and
 *      to wait on and notify monitored condition variables.  Only Broadcast
 *      semantics are supported for lock release; that is, all processes
 *      waiting on a lock are made runnable when the locked is released.
 *
 *      These macros do a fast check on the state of the lock or condition
 *      variable to reduce overhead.  In the best case the lock and unlock
 *      routines just set or clear flags.  In the worst case these routines
 *      have to call slower routines that must acquire a master lock.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/sync.h,v 1.7 90/01/16 17:59:49 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _SYNCUSER
#define _SYNCUSER

#ifndef _SPRITE
#include "sprite.h"
#endif

/*
 * The state of a lock consists of two bits of information, a lock bit
 * and a waiting bit.  The lock bit indicates that someone has the
 * lock, and no other process can acquire the lock until this bit is
 * cleared.  The waiting bit indicates that someone wants the lock.
 *
 * The user locks are different from the kernel locks. If we are compiling
 * user code then the Sync_UserLock type is typedef'ed to be Sync_Lock.
 * The Sync_UserLock type is used by the kernel to differentiate between
 * the two types of locks.
 *
 * Note: the inUse field is a whole word so it can be
 *       an argument to the test-and-set instruction.
 */

typedef struct Sync_UserLock {
    Boolean inUse;		/* 1 while the lock is busy */
    Boolean waiting;		/* 1 if someone wants the lock */
} Sync_UserLock;

#ifndef KERNEL 
typedef Sync_UserLock Sync_Lock;	 /* define user locks */
#endif

/*
 * Condition variables represent events that are associated with
 * locks.  The operations on a condition variable are Sync_Wait and
 * Sync_Broadcast. The lock must be acquired before a call to
 * Sync_Wait is made.  The lock is released while a process waits on a
 * condition, and is then re-acquired when the condition is notified
 * via Sync_Broadcast.
 */

typedef struct Sync_Condition {
    Boolean waiting;		/* 1 if someone is waiting on the condition */
} Sync_Condition;

/*
 * Maximum number of prior locks that can exist for any type of lock. A prior
 * lock is the last lock to be grabbed by the process grabbing the current 
 * lock. Keeping track of the locks held by a process is done in the proc
 * module. By knowing all the prior locks of a lock it is possible to 
 * construct a tree representing the locking behavior of the system.
 * SYNC_MAX_PRIOR places a limit on how many prior locks will be remembered
 * for an individual lock.
 */

#ifndef LOCKDEP
#define SYNC_MAX_PRIOR 1
#else
#define SYNC_MAX_PRIOR 30
#endif

/*
 * This structure is used to pass locking statistics to user programs.
 * One of these structures is needed per lock type.
 */
typedef struct {
    int		inUse;				/* 1 if structure is valid  */
    int		class;				/* 0 = semaphore, 1 = lock */
    int		hit;				/* # times lock was grabbed */
    int		miss;				/* # times lock was missed */
    char	name[30];			/* name of lock */
    int		priorCount;			/* # of prior locks  */
    int		priorTypes[SYNC_MAX_PRIOR];	/* types of prior locks */
    int		activeCount;			/* # active locks of type */
    int		deadCount;			/* # dead locks of type */
    int		spinCount;			/* # spins waiting for lock */
} Sync_LockStat;


/*
 * ----------------------------------------------------------------------------
 *
 * Monitor Locks --
 *
 *	The following set of macros are used to emulate monitored
 *	procedures of Mesa.  The Sprite style document has a complete
 *	description of the conventions required to emulate a set
 *	of monitored procedures; a summary is presented here.
 *
 *      The LOCK_MONITOR and UNLOCK_MONITOR macros depend on a constant
 *      LOCKPTR.  LOCKPTR should be defined as the address of the lock
 *      variable used to lock the monitor.  Something like the following
 *      two lines of code should appear at the beginning of a file of
 *      monitored procedures.
 *
 *	Sync_Lock modMonitorLock;
 *	#define LOCKPTR (&modMonitorLock)
 *
 *	The pseudo-keywords INTERNAL and ENTRY denote internal and entry
 *	procedures of a monitor.  INTERNAL procedures can only be called
 *	when the monitor lock is held.  ENTRY procedures are procedures
 *	that acquire the lock.  There may also be External procedures.
 *	They are the default and there is no special keyword.  An External
 *	procedure doesn't explicitly acquire the monitor lock, but may
 *	call an ENTRY procedure.
 *
 * ----------------------------------------------------------------------------
 */

#define LOCK_MONITOR   		    (void) Sync_GetLock(LOCKPTR)
#define UNLOCK_MONITOR 		    (void) Sync_Unlock(LOCKPTR)

#define ENTRY
#define INTERNAL


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_Wait --
 *
 * Wait on a condition variable.  This can only be called while a lock
 * is aquired because it is only safe to check global state while in a
 * monitor.  This releases the monitor lock and makes the process sleep
 * on the condition. The monitor lock is reacquired after the process 
 * has been woken up.
 *
 * The address of the condition variable is used as the generic 'event'
 * that the process blocks on.  The value of the condition variable is
 * used to indicate if there is a process waiting on the condition.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Put the process to sleep and release the monitor lock.
 *
 * ----------------------------------------------------------------------------
 */

#define Sync_Wait(conditionPtr, wakeIfSignal) \
    Sync_SlowWait(conditionPtr, LOCKPTR, wakeIfSignal)


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_Broadcast --
 *
 *      Notify other processes that a condition has been met.  If the
 *      condition variable indicates that there are processes waiting
 *      on the condition, then Sync_SlowBroadcast is used to wakeup
 *      waiting processes.
 *
 * Monitor Lock:
 *	This routine needs to be called with the monitor lock held.
 *	This ensures synchronous access to the conditionPtr->waiting flag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Make the processes waiting on the condition variable runnable.
 *
 * ----------------------------------------------------------------------------
 */

#define Sync_Broadcast(conditionPtr) \
    if (((Sync_Condition *)conditionPtr)->waiting == TRUE)  { \
	(void)Sync_SlowBroadcast((unsigned int) conditionPtr, \
	  &((Sync_Condition *)conditionPtr)->waiting); \
    }

/*
 * Exported procedures of the sync module for monitors.
 */

extern ReturnStatus	Sync_GetLock();
extern ReturnStatus	Sync_Unlock();
extern ReturnStatus	Sync_SlowLock();
extern ReturnStatus	Sync_SlowWait();
extern ReturnStatus	Sync_SlowBroadcast();

#endif /* _SYNCUSER */
