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

static Sync_Lock notifyLock;
#define LOCKPTR (&notifyLock)


/*
 *----------------------------------------------------------------------
 *
 * FsWaitListInsert --
 *
 *	Add a process to a list of waiters.  This handles the case where
 *	the process is already on the list.
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

    LOCK_MONITOR;

    LIST_FORALL(list, (List_Links *) myWaitPtr) {
	/*
	 * If already on list then update wait token.
	 */
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    myWaitPtr->waitToken = waitPtr->waitToken;
	    UNLOCK_MONITOR;
	    return;
	}
    }

    /*
     * Not on the list so put it there.
     */

    myWaitPtr = (Sync_RemoteWaiter *) malloc(sizeof(Sync_RemoteWaiter));
    *myWaitPtr = *waitPtr;
    List_Insert((List_Links *)myWaitPtr, LIST_ATREAR(list));

    UNLOCK_MONITOR;
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

    LIST_FORALL(list, (List_Links *) myWaitPtr) {
	/*
	 * If already on list then update wait token.
	 */
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    myWaitPtr->waitToken = waitPtr->waitToken;
	    return;
	}
    }

    /*
     * Not on the list so put it there.
     */

    myWaitPtr = (Sync_RemoteWaiter *) malloc(sizeof(Sync_RemoteWaiter));
    *myWaitPtr = *waitPtr;
    List_Insert((List_Links *)myWaitPtr, LIST_ATREAR(list));
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

    LOCK_MONITOR;
    while ( ! List_IsEmpty(list)) {
	waitPtr = (Sync_RemoteWaiter *)List_First(list);
	if (waitPtr->hostID != rpc_SpriteID) {
	    /*
	     * Contact the remote host and get it to notify the waiter.
	     */
	    if (waitPtr->hostID > NET_NUM_SPRITE_HOSTS) {
		printf( "FsWaitListNotify bad hostID %d.\n",
			  waitPtr->hostID);
	    } else {
		(void)Sync_RemoteNotify(waitPtr);
	    }
	} else {
	    /*
	     * Mark the local process as runable.
	     */
	    Sync_ProcWakeup(waitPtr->pid, waitPtr->waitToken);
	}
	List_Remove((List_Links *)waitPtr);
	free((Address)waitPtr);
    }
    UNLOCK_MONITOR;
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

    while ( ! List_IsEmpty(list)) {
	waitPtr = (Sync_RemoteWaiter *)List_First(list);
	if (waitPtr->hostID != rpc_SpriteID) {
	    /*
	     * Contact the remote host and get it to notify the waiter.
	     */
	    (void)Sync_RemoteNotify(waitPtr);
	} else {
	    /*
	     * Mark the local process as runable.
	     */
	    Sync_ProcWakeup(waitPtr->pid, waitPtr->waitToken);
	}
	List_Remove((List_Links *)waitPtr);
	free((Address)waitPtr);
    }
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

    LOCK_MONITOR;

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
    UNLOCK_MONITOR;
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

    while (!List_IsEmpty(list)) {
	myWaitPtr = (Sync_RemoteWaiter *)List_First(list);
	List_Remove((List_Links *) myWaitPtr);
	free((Address) myWaitPtr);
    }
}

