/* 
 * fsLock.c --
 *
 *	File locking routines.  The FsLockState data structure keeps info
 *	about shared and exlusive locks.  This includes a list of waiting
 *	processes, and a list of owning processes.  The ownership list
 *	is used to recover from processes that exit before unlocking their
 *	file, and to recover from hosts that crash running processes that
 *	held file locks.  Synchronization over these routines is assumed
 *	to be done by the caller via FsHandleLock.
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
#include "fsLock.h"
#include "fsOpTable.h"
#include "proc.h"
#include "rpc.h"
#include "net.h"
#include "swapBuffer.h"

/*
 * A  counter is incremented each time a process waits for a lock.
 * This is used to track locking activity.
 */
int fsLockWaits = 0;

/*
 * A list of lock owners is kept for files for error recovery.
 * If a process exits without unlocking a file, or a host crashes
 * that had processes with locks, then the locks are broken.
 */
typedef struct FsLockOwner {
    List_Links links;		/* A list of these hangs from FsLockState */
    int hostID;			/* SpriteID of process that got the lock */
    int procID;			/* ProcessID of owning process */
    Fs_FileID streamID;		/* Stream on which lock call was made */
    int flags;			/* IOC_LOCK_EXCLUSIVE, IOC_LOCK_SHARED */
} FsLockOwner;

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
    List_Init(&lockPtr->ownerList);
    lockPtr->flags = 0;
    lockPtr->numShared = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FsIocLock --
 *
 *	Top-level locking/unlocking routine that handles I/O control
 *	related byte swapping.  If the lock I/O control has been issued
 *	from a client with a different archetecture the data block containing
 *	the lock arguments has to be byte swapped.
 *
 * Results:
 *	SUCCESS or FS_WOULD_BLOCK
 *
 * Side effects:
 *	Lock or unlock the file, see FsLock and FsUnlock.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsIocLock(lockPtr, command, byteOrder, inBufPtr, streamIDPtr)
    register FsLockState *lockPtr;	/* Locking state for a file. */
    int		command;		/* IOC_LOCK, IOC_UNLOCK */
    int		byteOrder;		/* Client's byte ordering */
    Fs_Buffer	*inBufPtr;		/* Buffer containing Ioc_LockArgs */
    Fs_FileID	*streamIDPtr;		/* ID of stream associated with lock */
{
    register Ioc_LockArgs *lockArgsPtr;
    register ReturnStatus status = SUCCESS;
    Ioc_LockArgs lockArgs;

    if (byteOrder != mach_ByteOrder) {
	int size = sizeof(Ioc_LockArgs);
	Swap_Buffer(inBufPtr->addr, inBufPtr->size, byteOrder, mach_ByteOrder,
		    "wwww", (Address)&lockArgs, &size);
	if (size != sizeof(Ioc_LockArgs)) {
	    status = GEN_INVALID_ARG;
	} else {
	    lockArgsPtr = &lockArgs;
	}
    } else if (inBufPtr->size < sizeof(Ioc_LockArgs)) {
	status = GEN_INVALID_ARG;
    } else {
	lockArgsPtr = (Ioc_LockArgs *)inBufPtr->addr;
    }
    if (status == SUCCESS) {
	if (command == IOC_LOCK) {
	    status = FsLock(lockPtr, lockArgsPtr, streamIDPtr);
	} else {
	    status = FsUnlock(lockPtr, lockArgsPtr, streamIDPtr);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLock --
 *
 *	Try to get a lock a stream.  If the lock is already held then
 *	the caller is added to the waitlist for the lock and FS_WOULD_BLOCK
 *	is returned.  Otherwise, the lock is marked as held and our
 *	caller is put on the ownership list for the lock.
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
FsLock(lockPtr, argPtr, streamIDPtr)
    register FsLockState *lockPtr;	/* Locking state for a file. */
    Ioc_LockArgs *argPtr;		/* IOC_LOCK_EXCLUSIVE|IOC_LOCK_SHARED */
    Fs_FileID	*streamIDPtr;		/* Stream that owns the lock */
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
    if (status == SUCCESS) {
	register FsLockOwner *lockOwnerPtr;
	/*
	 * Put the calling process on the lock ownership list.
	 */
	lockOwnerPtr = Mem_New(FsLockOwner);
	List_InitElement((List_Links *)lockOwnerPtr);
	lockOwnerPtr->hostID = argPtr->hostID;
	lockOwnerPtr->procID = argPtr->pid;
	if (streamIDPtr != (Fs_FileID *)NIL) {
	    lockOwnerPtr->streamID = *streamIDPtr;
	} else {
	    lockOwnerPtr->streamID.type = -1;
	}
	lockOwnerPtr->flags = operation & (IOC_LOCK_EXCLUSIVE|IOC_LOCK_SHARED);
	List_Insert((List_Links *)lockOwnerPtr,
		    LIST_ATREAR(&lockPtr->ownerList));
    } else if (status == FS_WOULD_BLOCK) {
	Sync_RemoteWaiter wait;
	/*
	 * Put the potential waiter on the file's lockWaitList.
	 */
	if (argPtr->hostID > NET_NUM_SPRITE_HOSTS) {
	    Sys_Panic(SYS_WARNING, "FsLock: bad hostID %d.\n",
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
 * FsUnlock --
 *
 *	Release a lock a stream.  The ownership list is checked here, but
 *	the lock is released anyway (so far).
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
FsUnlock(lockPtr, argPtr, streamIDPtr)
    register FsLockState *lockPtr;	/* Locking state for the file. */
    Ioc_LockArgs *argPtr;	/* Lock flags and process info for waiting */
    Fs_FileID	*streamIDPtr;	/* Verified against the lock ownership list */ 
{
    ReturnStatus status = SUCCESS;
    register int operation = argPtr->flags;
    register FsLockOwner *lockOwnerPtr;

    if (operation & IOC_LOCK_EXCLUSIVE) {
	if (lockPtr->flags & IOC_LOCK_EXCLUSIVE) {
	    LIST_FORALL(&lockPtr->ownerList, (List_Links *)lockOwnerPtr) {
		if ((lockOwnerPtr->procID == argPtr->pid) ||
		    (streamIDPtr != (Fs_FileID *)NIL &&
		     lockOwnerPtr->streamID.major == streamIDPtr->major &&
		     lockOwnerPtr->streamID.minor == streamIDPtr->minor &&
		     lockOwnerPtr->streamID.serverID == streamIDPtr->serverID)){
		    lockPtr->flags &= ~IOC_LOCK_EXCLUSIVE;
		    List_Remove((List_Links *)lockOwnerPtr);
		    Mem_Free((Address)lockOwnerPtr);
		    break;
		}
	    }
	    if (lockPtr->flags & IOC_LOCK_EXCLUSIVE) {
		/*
		 * Oops, unlocking process didn't match lock owner.
		 */
		if (!List_IsEmpty(&lockPtr->ownerList)) {
		    lockOwnerPtr =
			(FsLockOwner *)List_First(&lockPtr->ownerList);
		    Sys_Panic(SYS_WARNING,
			"FsUnlock, non-owner <%x> unlocked, owner <%x>\n",
			argPtr->pid, lockOwnerPtr->procID);
		    List_Remove((List_Links *)lockOwnerPtr);
		    Mem_Free((Address)lockOwnerPtr);
		} else {
		    Sys_Panic(SYS_WARNING, "FsUnlock, no lock owner\n");
		}
		lockPtr->flags &= ~IOC_LOCK_EXCLUSIVE;
	    }
	} else {
	    status = FS_NO_EXCLUSIVE_LOCK;
	}
    } else if (operation & IOC_LOCK_SHARED) {
	if (lockPtr->flags & IOC_LOCK_SHARED) {
	    status = FAILURE;
	    lockPtr->numShared--;
	    LIST_FORALL(&lockPtr->ownerList, (List_Links *)lockOwnerPtr) {
		if ((lockOwnerPtr->procID == argPtr->pid) ||
		    (streamIDPtr != (Fs_FileID *)NIL &&
		     lockOwnerPtr->streamID.major == streamIDPtr->major &&
		     lockOwnerPtr->streamID.minor == streamIDPtr->minor &&
		     lockOwnerPtr->streamID.serverID == streamIDPtr->serverID)){
		    status = SUCCESS;
		    List_Remove((List_Links *)lockOwnerPtr);
		    Mem_Free((Address)lockOwnerPtr);
		    break;
		}
	    }
	    if (status != SUCCESS) {
		/*
		 * Oops, unlocking process didn't match lock owner.
		 */
		Sys_Panic(SYS_WARNING,
		    "FsUnlock, non-owner <%x> did shared unlock\n",
		    argPtr->pid);
		status = SUCCESS;
	    }
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

/*
 *----------------------------------------------------------------------
 *
 * FsLockClose --
 *
 *	Check that that calling process owns a lock on this file,
 *	and if it does then break that lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up the lock and frees owner information.
 *
 *----------------------------------------------------------------------
 */

void
FsLockClose(lockPtr, procID, streamIDPtr)
    register FsLockState *lockPtr;	/* Locking state for the file. */
    Proc_PID procID;			/* ProcessID of closing process. */
    Fs_FileID *streamIDPtr;		/* Stream being closed */
{
    register FsLockOwner *lockOwnerPtr;

    LIST_FORALL(&lockPtr->ownerList, (List_Links *)lockOwnerPtr) {
	if ((lockOwnerPtr->procID == procID) ||
	    (streamIDPtr != (Fs_FileID *)NIL &&
	     lockOwnerPtr->streamID.major == streamIDPtr->major &&
	     lockOwnerPtr->streamID.minor == streamIDPtr->minor &&
	     lockOwnerPtr->streamID.serverID == streamIDPtr->serverID)) {
	    lockPtr->flags &= ~lockOwnerPtr->flags;
	    List_Remove((List_Links *)lockOwnerPtr);
	    Mem_Free((Address)lockOwnerPtr);
	    FsWaitListNotify(&lockPtr->waitList);
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsLockClientKill --
 *
 *	Go through the list of lock owners and release any locks
 *	held by processes on the given client.  This is called after
 *	the client is assumed to be down.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases locks held by processes on the client.
 *
 *----------------------------------------------------------------------
 */

void
FsLockClientKill(lockPtr, clientID)
    register FsLockState *lockPtr;	/* Locking state for the file. */
    int clientID;			/* SpriteID of crashed client. */
{
    register FsLockOwner *lockOwnerPtr;
    register FsLockOwner *nextOwnerPtr;
    register Boolean breakLock = FALSE;

    nextOwnerPtr = (FsLockOwner *)List_First(&lockPtr->ownerList);
    while (!List_IsAtEnd(&lockPtr->ownerList, (List_Links *)nextOwnerPtr)) {
	lockOwnerPtr = nextOwnerPtr;
	nextOwnerPtr = (FsLockOwner *)List_Next((List_Links *)lockOwnerPtr);

	if (lockOwnerPtr->hostID == clientID) {
	    breakLock = TRUE;
	    lockPtr->flags &= ~lockOwnerPtr->flags;
	    List_Remove((List_Links *)lockOwnerPtr);
	    Mem_Free((Address)lockOwnerPtr);
	}
    }
    if (breakLock) {
	FsWaitListNotify(&lockPtr->waitList);
    }
}

