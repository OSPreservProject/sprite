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
 *	Calls Mem_Alloc and adds to the list.
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

    myWaitPtr = (Sync_RemoteWaiter *) Mem_Alloc(sizeof(Sync_RemoteWaiter));
    *myWaitPtr = *waitPtr;
    List_Insert((List_Links *)myWaitPtr, LIST_ATREAR(list));

    UNLOCK_MONITOR;
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
 *      with Mem_Free.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListNotify(list)
    List_Links *list;		/* List of waiting processes to notify */
{
    Sync_RemoteWaiter *waitPtr;

    LOCK_MONITOR;
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
	Mem_Free((Address)waitPtr);
    }
    UNLOCK_MONITOR;
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
 *	Calls Mem_Free and deletes from the list.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsWaitListRemove(list, waitPtr)
    List_Links *list;		/* List to remove waiter from. */
    Sync_RemoteWaiter *waitPtr;	/* Info about process for remote waiting */
{
    register Sync_RemoteWaiter *myWaitPtr;

    LOCK_MONITOR;
    LIST_FORALL(list, (List_Links *) myWaitPtr) {
	if (myWaitPtr->pid == waitPtr->pid &&
	    myWaitPtr->hostID == waitPtr->hostID) {
	    List_Remove((List_Links *) myWaitPtr);
	    Mem_Free((Address) myWaitPtr);
	}
    }
    UNLOCK_MONITOR;
}
