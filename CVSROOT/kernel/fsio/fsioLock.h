/*
 * fsioLock.h --
 *
 *	Declarations for user-level file locking routines.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSLOCK
#define _FSLOCK
/*
 * There is a system call to lock a file.  The lock state is kept in
 * the I/O handle on the I/O server so that the lock has network-wide effect.
 */

typedef struct Fsio_LockState {
    int		flags;		/* Bits defined below */
    List_Links	waitList;	/* List of processes to wakeup when the
				 * file gets unlocked */
    int		numShared;	/* Number of shared lock holders */
    List_Links	ownerList;	/* List of processes responsible for locks */
} Fsio_LockState;

/*
 * (The following lock bits are defined in user/fs.h)
 * IOC_LOCK_EXCLUSIVE - only one process may hold an exclusive lock.
 * IOC_LOCK_SHARED    - many processes may hold shared locks as long as
 *	there are no exclusive locks held.  Exclusive locks have to
 *	wait until all shared locks go away.
 */

/*
 * Cache consistency routines.
 */
#endif _FSLOCK
