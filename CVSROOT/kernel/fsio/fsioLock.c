/* 
 * fsLock.c --
 *
 *	File locking routines.  They are monitored to synchronize
 *	access to the locking state.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "proc.h"
#include "rpc.h"
#include "net.h"

/*
 * A  counter is incremented each time a process waits for a lock.
 * This is used to track locking activity.
 */
int fsLockWaits = 0;

/*
 *----------------------------------------------------------------------
 *
 * FsLockInit --
 *
 *	Initialize lock state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the wait list, zeros out counters, etc.
 *
 *----------------------------------------------------------------------
 */

void
FsLockInit(lockPtr)
    register FsLockState *lockPtr;	/* Locking state for a file. */
{
    List_Init(&lockPtr->waitList);
    lockPtr->flags = 0;
    lockPtr->numShared = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileLock --
 *
 *	Try to get a lock a file in the local domain.  If the lock cannot
 *	be taken then return FS_WOULD_BLOCK so our caller can wait.
 *
 * Results:
 *	SUCCESS or FS_WOULD_BLOCK
 *
 * Side effects:
 *	If the lock is available then update the lock state of the file.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsFileLock(lockPtr, argPtr)
    register FsLockState *lockPtr;	/* Locking state for a file. */
    Ioc_LockArgs *argPtr;		/* IOC_LOCK_EXCLUSIVE|IOC_LOCK_SHARED */
{
    ReturnStatus status = SUCCESS;
    register int operation = argPtr->flags;

    /*
     * Attempt to lock the file.  Exclusive locks can't co-exist with
     * any locks, while shared locks can exist with other shared locks.
     */
    if (operation & IOC_LOCK_EXCLUSIVE) {
	if (lockPtr->flags & (IOC_LOCK_SHARED|IOC_LOCK_EXCLUSIVE)) {
	    status = FS_WOULD_BLOCK;
	} else {
	    lockPtr->flags |= IOC_LOCK_EXCLUSIVE;
	}
    } else if (operation & IOC_LOCK_SHARED) {
	if (lockPtr->flags & IOC_LOCK_EXCLUSIVE) {
	    status = FS_WOULD_BLOCK;
	} else {
	    lockPtr->flags |= IOC_LOCK_SHARED;
	    lockPtr->numShared++;
	}
    } else {
	status = GEN_INVALID_ARG;
    }
    if (status == FS_WOULD_BLOCK) {
	Sync_RemoteWaiter wait;
	/*
	 * Put the potential waiter on the file's lockWaitList.
	 */
	if (argPtr->hostID > NET_NUM_SPRITE_HOSTS) {
	    Sys_Panic(SYS_WARNING, "FsFileLock: bad hostID %d.\n",
		      argPtr->hostID);
	} else {
	    wait.hostID = argPtr->hostID;
	    wait.pid = argPtr->pid;
	    wait.waitToken = argPtr->token;
	    FsWaitListInsert(&lockPtr->waitList, &wait);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileUnlock --
 *
 *	Release a lock a file in the local domain.
 *
 * Results:
 *	SUCCESS or FS_WOULD_BLOCK
 *
 * Side effects:
 *	If the lock is available then update the lock state of the file.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsFileUnlock(lockPtr, argPtr)
    register FsLockState *lockPtr;	/* Locking state for the file. */
    Ioc_LockArgs *argPtr;	/* Lock flags and process info for waiting */
{
    ReturnStatus status = SUCCESS;
    register int operation = argPtr->flags;

    /*
     * Release the lock on the file.
     */
    if (operation & IOC_LOCK_EXCLUSIVE) {
	if (lockPtr->flags & IOC_LOCK_EXCLUSIVE) {
	    status = SUCCESS;
	    lockPtr->flags &= ~IOC_LOCK_EXCLUSIVE;
	} else {
	    status = FS_NO_EXCLUSIVE_LOCK;
	}
    } else if (operation & IOC_LOCK_SHARED) {
	if (lockPtr->flags & IOC_LOCK_SHARED) {
	    status = SUCCESS;
	    lockPtr->numShared--;
	    if (lockPtr->numShared == 0) {
		lockPtr->flags &= ~IOC_LOCK_SHARED;
	    }
	} else {
	    status = FS_NO_SHARED_LOCK;
	}
    } else {
	status = GEN_INVALID_ARG;
    }
    if (status == SUCCESS) {
	/*
	 * Go through the list of waiters and notify them.  There is only
	 * a single waiting list for both exclusive and shared locks.  This
	 * means that exclusive lock attempts will be retried even if the
	 * shared lock count has not gone to zero.
	 */
	FsWaitListNotify(&lockPtr->waitList);
    }
    return(status);
}

