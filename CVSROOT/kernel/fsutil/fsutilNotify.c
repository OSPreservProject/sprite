/* 
 * fsNotify.c --
 *
 * Routines to handle notification of processes waiting on handles.
 * Each handle has a few "wait lists" associated with it that contain
 * state for each process waiting on a file, each list is for a different
 * kind of waiter, ie. readers, writers, lockers.  Processes get stuck into
 * these wait lists when an operation on the file would block for some reason.
 * When the file unblocks, there are other routines to call to notify all
 * the processes that have been added to the waiting lists.
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
#include "sync.h"
#include "rpc.h"
#include "net.h"

/*
 * Access to wait lists are either synchronized by higher levels
 * (with HandleLocks, for example) or they are synchronized with
 * the following MASTER_LOCK.  This is used for device notifications.
 */
#ifdef notdef
static Sync_Lock notifyLock = Sync_LockInitStatic("Fs:notifyLock");
#define LOCKPTR (&notifyLock)
#endif notdef

static Sync_Semaphore notifyMutex = Sync_SemInitStatic("Fs:notifyMutex");


/*
 *----------------------------------------------------------------------
 *
 * FsWaitListInsert --
 *
 *	Add a process to a list of waiters.  This handles the case where
 *	the process is already on the list.  This is only called from
 *	process level code so it is ok for it to hold a MASTER lock
 *	and call malloc, which has a monitor lock.  What would cause deadlock
 *	is a call to free from an interrupt handler if the process holding
 *	the free monitor lock was the one interrupted:
 *		Hold malloc/free monitor
 *		* INTERRUPT *
 *		grab notify master lock
 *		block on free monitor lock
 *		DEADLOCK
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Calls malloc and adds to the list.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListInsert(list, waitPtr)
    List_Links *list;		/* List to add waiter to */
    Sync_RemoteWaiter *waitPtr;	/* Info about process for remote waiting */
{
    register Sync_RemoteWaiter *myWaitPtr;

    MASTER_LOCK(&notifyMutex);
    Sync_LockRegister(&notifyLock);

    LIST_FORALL(list, (List_Links *) myWaitPtr) {
	/*
	 * If already on list then update wait token.
	 */
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    myWaitPtr->waitToken = waitPtr->waitToken;
	    MASTER_UNLOCK(&notifyMutex);
	    return;
	}
    }

    /*
     * Not on the list so put it there.
     */

    myWaitPtr = (Sync_RemoteWaiter *) malloc(sizeof(Sync_RemoteWaiter));
    *myWaitPtr = *waitPtr;
    List_Insert((List_Links *)myWaitPtr, LIST_ATREAR(list));

    MASTER_UNLOCK(&notifyMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFastWaitListInsert --
 *
 *	An un-monitored version of FsWaitListInsert that depends
 *	on handle locking, or something, by higher levels for
 *	synchronization.  Note: the malloc is needed because
 *	of select.  Regular read and write use a Sync_RemoteWaiter
 *	struct declared in Fs_Read or Fs_Write, and it won't go
 *	away while the pipe reader or writer waits.  However, with
 *	select the waiter might go away before we notify it, so
 *	we have to malloc and copy the wait structure.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Calls malloc and adds to the list.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsFastWaitListInsert(list, waitPtr)
    List_Links *list;		/* List to add waiter to */
    Sync_RemoteWaiter *waitPtr;	/* Info about process for remote waiting */
{
    register Sync_RemoteWaiter *myWaitPtr;

    MASTER_LOCK(&notifyMutex);

    LIST_FORALL(list, (List_Links *) myWaitPtr) {
	/*
	 * If already on list then update wait token.
	 */
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    myWaitPtr->waitToken = waitPtr->waitToken;
	    MASTER_UNLOCK(&notifyMutex);
	    return;
	}
    }

    /*
     * Not on the list so put it there.
     */

    myWaitPtr = (Sync_RemoteWaiter *) malloc(sizeof(Sync_RemoteWaiter));
    *myWaitPtr = *waitPtr;
    List_Insert((List_Links *)myWaitPtr, LIST_ATREAR(list));

    MASTER_UNLOCK(&notifyMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FsWaitListNotify --
 *
 *      Notify all the processes in a wait-list.  If the process is on a
 *      remote host then an RPC is done to that host.
 *
 * Results:
 *	None
 *
 * Side effects:
 *      This results in a call to Sync_ProcWakeup on the host of the
 *      waiting process.  The list is emptied with each item being freed
 *      with free.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListNotify(list)
    register List_Links *list;	/* List of waiting processes to notify */
{
    register Sync_RemoteWaiter *waitPtr;

    MASTER_LOCK(&notifyMutex);

    LIST_FORALL(list, (List_Links *)waitPtr) {
	if (waitPtr->hostID != rpc_SpriteID) {
	    /*
	     * Contact the remote host and get it to notify the waiter.
	     * UGLY, unlock monitor during RPC.  The monitor really
	     * protects free/malloc calls.
	     */

	    MASTER_UNLOCK(&notifyMutex);
	    if (waitPtr->hostID > NET_NUM_SPRITE_HOSTS) {
		printf( "FsWaitListNotify bad hostID %d.\n",
			  waitPtr->hostID);
	    } else {
		(void)Sync_RemoteNotify(waitPtr);
	    }
	    MASTER_LOCK(&notifyMutex);
	} else {
	    /*
	     * Mark the local process as runable.
	     */
	    Sync_ProcWakeup(waitPtr->pid, waitPtr->waitToken);
	}
#ifdef notdef
	/*
	 * The free() call can cause deadlock.
	 */
	List_Remove((List_Links *)waitPtr);
	free((Address)waitPtr);
#endif notdef
    }
    MASTER_UNLOCK(&notifyMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFastWaitListNotify --
 *
 *      A faster version of FsWaitListNotify that depends on higher
 *	level synchronization like handle locking.
 *
 * Results:
 *	None
 *
 * Side effects:
 *      This results in a call to Sync_ProcWakeup on the host of the
 *      waiting process.  The list is emptied with each item being freed
 *      with free.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsFastWaitListNotify(list)
    register List_Links *list;	/* List of waiting processes to notify */
{
    register Sync_RemoteWaiter *waitPtr;

    MASTER_LOCK(&notifyMutex);

    LIST_FORALL(list, (List_Links *)waitPtr) {
	if (waitPtr->hostID != rpc_SpriteID) {
	    /*
	     * Contact the remote host and get it to notify the waiter.
	     */
	    MASTER_UNLOCK(&notifyMutex);
	    (void)Sync_RemoteNotify(waitPtr);
	    MASTER_LOCK(&notifyMutex);
	} else {
	    /*
	     * Mark the local process as runable.
	     */
	    Sync_ProcWakeup(waitPtr->pid, waitPtr->waitToken);
	}
#ifdef notdef
	List_Remove((List_Links *)waitPtr);
	free((Address)waitPtr);
#endif notdef
    }
    MASTER_UNLOCK(&notifyMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * FsWaitListRemove --
 *
 *	Remove a process from the list of waiters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Calls free and deletes from the list.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListRemove(list, waitPtr)
    List_Links *list;		/* List to remove waiter from. */
    Sync_RemoteWaiter *waitPtr;	/* Info about process for remote waiting */
{
    register Sync_RemoteWaiter *myWaitPtr;
    register Sync_RemoteWaiter *nextPtr;

    MASTER_LOCK(&notifyMutex);

    nextPtr = (Sync_RemoteWaiter *) List_First(list);
    while (! List_IsAtEnd(list, (List_Links *)nextPtr) ) {
	myWaitPtr = nextPtr;
	nextPtr = (Sync_RemoteWaiter *)List_Next((List_Links *)myWaitPtr);
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    List_Remove((List_Links *) myWaitPtr);
	    free((Address) myWaitPtr);
	}
    }
    MASTER_UNLOCK(&notifyMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FsWaitListDelete --
 *
 *	Delete and Free all entries from a wait list.  This is used
 *	when removing handles.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Calls free and deletes from the list.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListDelete(list)
    List_Links *list;		/* List to clean up. */
{
    register Sync_RemoteWaiter *myWaitPtr;

    MASTER_LOCK(&notifyMutex);
    while (!List_IsEmpty(list)) {
	myWaitPtr = (Sync_RemoteWaiter *)List_First(list);
	List_Remove((List_Links *) myWaitPtr);
	free((Address) myWaitPtr);
    }
    MASTER_UNLOCK(&notifyMutex);
}

