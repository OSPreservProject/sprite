/* 
 * fsIOClient.c --
 *
 *	Routines to handle the client lists at the I/O handle level.  
 *	The I/O handle client list is needed for cache consistency and to
 *	verify that client's are valid.  The routines here add and remove
 *	clients.
 *	(Note that fsCacheConsist.c has routines which also use the client
 *	list on the I/O handle, but those routines are specific to files.)
 *
 *	A master list of clients is also kept here to simplify cleaning
 *	up after crashed clients.  Periodically this master list is checked
 *	and the file state for dead clients is cleaned up.
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
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsutil.h"
#include "fsconsist.h"
#include "fsio.h"
#include "fsStat.h"
#include "stdlib.h"
#include "rpc.h"
#include "sync.h"

/*
 * A master list of clients of this host.  This is maintained here and
 * consulted periodically in order to be able to clean up after dead clients.
 */
static List_Links masterClientListHdr;
static List_Links *masterClientList;

typedef struct {
    List_Links	links;
    int		clientID;
} ClientItem;

static Sync_Lock clientLock;
#define LOCKPTR (&clientLock)


/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_ClientInit --
 *
 *	Initialize the master list of clients for this server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List_Init.
 *
 * ----------------------------------------------------------------------------
 */

void
Fsconsist_ClientInit()
{
    Sync_LockInitDynamic(&clientLock, "Fs:clientLock");
    List_Init(&masterClientListHdr);
    masterClientList = &masterClientListHdr;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_IOClientOpen --
 *
 *	Add the client to the set of clients doing I/O on a file.  This
 *	increments reference counts due to the client.
 *
 * Results:
 *	Returns a pointer to the client state just added.  This is used
 *	by the file consistency routines, which set the cached boolean
 *	and the openTimeStamp, but ignored by others.
 *
 * Side effects:
 *	As well as adding the client to the set of clients for the I/O handle,
 *	the client is recorded in the master list of clients (if it isn't
 *	already there) so clients can be easily scavenged.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY Fsconsist_ClientInfo *
Fsconsist_IOClientOpen(clientList, clientID, useFlags, cached)
    List_Links	*clientList;	/* List of clients for the I/O handle. */
    int		clientID;	/* The client who is opening the file. */
    int		useFlags;	/* FS_READ | FS_WRITE | FS_EXECUTE */
    Boolean	cached;		/* Boolean property recorded for client */
{
    register Fsconsist_ClientInfo *clientPtr;

    LIST_FORALL(clientList, (List_Links *)clientPtr) {
	if (clientPtr->clientID == clientID) {
	    goto found;
	}
    }
    INSERT_CLIENT(clientList, clientPtr, clientID);
found:
    clientPtr->cached = cached;
    clientPtr->use.ref++;
    if (useFlags & FS_WRITE) {
	clientPtr->use.write++;
    }
    if (useFlags & FS_EXECUTE) {
	clientPtr->use.exec++;
    }

    /*
     * Make sure the client is in the master list of all clients for this host.
     */
    Fsconsist_AddClient(clientID);
    return(clientPtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * 
 Fsconsist_IOClientReopen --
 *
 *	Add the client to the set of clients doing I/O on a file.  This
 *	updates reference counts due to the client's reopen attempt.
 *
 * Results:
 *	TRUE if the client was already listed.
 *
 * Side effects:
 *	As well as adding the client to the set of clients for the I/O handle,
 *	the client is recorded in the master list of clients (if it isn't
 *	already there) so clients can be easily scavenged.
 *
 * ----------------------------------------------------------------------------
 */

Boolean
Fsconsist_IOClientReopen(clientList, clientID, usePtr)
    List_Links	*clientList;	/* List of clients for the I/O handle. */
    int		clientID;	/* The client who is opening the file. */
    Fsutil_UseCounts	*usePtr;	/* In - Client's usage of the object.
				 * Out - difference between old client useage.
				 *  This means that the summary use counts
				 *  can be updated by adding the use counts
				 *  left over in this structure after the
				 *  reconciliation with the old state. */
{
    register Fsconsist_ClientInfo *clientPtr;
    register Boolean found = FALSE;

    LIST_FORALL(clientList, (List_Links *)clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	    goto doit;
	}
    }
    INSERT_CLIENT(clientList, clientPtr, clientID);
doit:
    clientPtr->use.ref += usePtr->ref;
    clientPtr->use.write += usePtr->write;
    clientPtr->use.exec += usePtr->exec;
    /*
     * Make sure the client is in the master list of all clients for this host.
     */
    Fsconsist_AddClient(clientID);
    return(found);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_IOClientClose --
 *
 *	Decrement the reference, executor and/or writer counts for the client 
 *	for the given handle.  Note, we don't mess with the master list of
 *	clients.  That gets cleaned up by Fsconsist_ClientScavenge.
 *
 * Results:
 *	TRUE if there was a record that the client was using the file.
 *	This is used to trap out invalid closes.
 *
 * Side effects:
 *	The client list entry from the stream is removed.
 *
 * ----------------------------------------------------------------------------
 *
 */
Boolean
Fsconsist_IOClientClose(clientList, clientID, flags, cachePtr)
    List_Links		*clientList;	/* List of clients for I/O handle */
    int			clientID;	/* Host ID of client that had it open */
    register int	flags;		/* Flags from the stream. */
    Boolean		*cachePtr;	/* In/Out.  If TRUE on entry, this won't
					 * delete the client list entry if
					 * the entry's cached field is also
					 * TRUE.  On return, this is the value
					 * of the client's cached field. */
{
    register	Fsconsist_ClientInfo	*clientPtr;
    register	Boolean		found = FALSE;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	if ((clientPtr->use.ref < 0) || (clientPtr->use.write < 0) ||
	    (clientPtr->use.exec < 0)) {
	    printf("Fsconsist_IOClientClose: Values are bad at start:\n");
	    panic("Fsconsist_IOClientClose: client %d ref %d write %d exec %d\n",
		clientPtr->clientID,
		clientPtr->use.ref, clientPtr->use.write, clientPtr->use.exec);
	}
	clientPtr->use.ref--;
	if (flags & FS_WRITE) {
	    clientPtr->use.write--;
	}
	if (flags & FS_EXECUTE) {
	    clientPtr->use.exec--;
	}
	if ((clientPtr->use.ref < 0) || (clientPtr->use.write < 0) ||
	    (clientPtr->use.exec < 0)) {
	    printf("This is (probably) a continuable panic.\n");
	    panic("Fsconsist_IOClientClose: client %d ref %d write %d exec %d\n",
		clientPtr->clientID,
		clientPtr->use.ref, clientPtr->use.write, clientPtr->use.exec);
	    clientPtr->use.exec = 0;
	}
	if ((!(*cachePtr) || !clientPtr->cached) &&
	    (clientPtr->use.ref == 0)) {
	    *cachePtr = clientPtr->cached;
	    if (!clientPtr->locked) {
		/*
		 * Free up the client list entry if it is not locked
		 * due to an iteration through the client list.
		 */
		REMOVE_CLIENT(clientPtr);
	    }
	} else {
	    *cachePtr = clientPtr->cached;
	}
    }
    return(found);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_IOClientRemoveWriter --
 *
 *	Decrement the writer count for the client for the given handle.
 *	This is done when a writer of a stream (for instance, a pipe)
 *	becomes only a reader.
 *
 * Results:
 *	TRUE if there was a record that the client was using the file.
 *	This is used to trap out invalid closes.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
Boolean
Fsconsist_IOClientRemoveWriter(clientList, clientID)
    List_Links		*clientList;	/* List of clients for I/O handle */
    int			clientID;	/* Host ID of client that had it open */
{
    register	Fsconsist_ClientInfo	*clientPtr;
    register	Boolean		found = FALSE;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	clientPtr->use.write--;
	if (clientPtr->use.write < 0) {
	    panic("Fsconsist_IOClientRemoveWriter: client %d ref %d write %d exec %d\n",
		clientPtr->clientID,
		clientPtr->use.ref, clientPtr->use.write, clientPtr->use.exec);
	}
    }
    return(found);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_ClientScavenge --
 *
 *      Check the master list of clients for ones that have crashed.  If any
 *	one has then we call Fsutil_RemoveClient to clean up the file state
 *	associated with it.
 *
 *	NOT USED.  File servers let regular traffic, like consistency
 *	call backs, detect failures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls FsClientRemove on dead clients, and nukes them from the
 *	master client list.
 *
 * ----------------------------------------------------------------------------
 *
 */
#ifdef notdef
ENTRY void
Fsconsist_ClientScavenge()
{
    register	ClientItem	*listPtr;

    LOCK_MONITOR;

    LIST_FORALL(masterClientList, (List_Links *)listPtr) {
	if (listPtr->clientID != rpc_SpriteID && 
	    Recov_IsHostDown(listPtr->clientID)) {
	    Fsutil_RemoveClient(listPtr->clientID);
	}
    }

    UNLOCK_MONITOR;
}
#endif notdef

/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_IOClientKill --
 *
 *	Find and remove the given client in the list for the handle.  The
 *	number of client references, writers, and executers is returned
 *	so our caller can clean up the reference counts in the handle.
 *
 * Results:
 *	*inUsePtr set to TRUE if the client has the file open, *writingPtr
 *	set to TRUE if the client has the file open for writing, and 
 *	*executingPtr set to TRUE if the client has the file open for
 *	execution.
 *	
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
Fsconsist_IOClientKill(clientList, clientID, refPtr, writePtr, execPtr)
    List_Links *clientList;	/* List of clients to a file. */
    int		clientID;	/* Client to delete. */
    int		*refPtr;	/* Number of times client has file open. */
    int		*writePtr;	/* Number of times client is writing file. */
    int		*execPtr;	/* Number of times clients is executing file.*/
{
    register Fsconsist_ClientInfo 	*clientPtr;

    *refPtr = 0;
    *writePtr = 0;
    *execPtr = 0;

    /*
     * Remove the client from the list of clients using the file.
     */
    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    *refPtr += clientPtr->use.ref;
	    *writePtr += clientPtr->use.write;
	    *execPtr += clientPtr->use.exec;
	    if (clientPtr->locked) {
		clientPtr->use.ref = 0;
		clientPtr->use.write = 0;
		clientPtr->use.exec = 0;
	    } else {
		REMOVE_CLIENT(clientPtr);
	    }
	    break;
	}
    }

}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_IOClientStatus --
 *
 *	This computes the difference between a client's version of its
 *	state and our version of the client's usage state.  This is called
 *	during a device reopen to see how the client's state is different.
 *
 * Results:
 *	The client's version of the use state is modified to reflect
 *	the difference between our previous notion of the state and
 *	the client's notion of the state.  This means that the ref, write,
 *	and exec fields can be positive or negative after this call.
 *	If they are zero then the client's state matches ours.
 *	
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
Fsconsist_IOClientStatus(clientList, clientID, clientUsePtr)
    List_Links *clientList;	/* List of clients to a file. */
    int		clientID;	/* Client to check. */
    Fsutil_UseCounts	*clientUsePtr;	/* Client's version of the usage */
{
    register Fsconsist_ClientInfo 	*clientPtr;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    clientUsePtr->ref -= clientPtr->use.ref;
	    clientUsePtr->write -= clientPtr->use.write;
	    clientUsePtr->exec -= clientPtr->use.exec;
	    return;
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsconsist_AddClient --
 *
 *      Add a client to the master list of clients that is checked by
 *	Fsconsist_ClientScavenge.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May add the client to the list.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ENTRY void
Fsconsist_AddClient(clientID)
    int clientID;
{
#ifdef notdef
    register	ClientItem	*listPtr;

    LOCK_MONITOR;

    LIST_FORALL(masterClientList, (List_Links *)listPtr) {
	if (listPtr->clientID == clientID) {
	    goto exit;
	}
    }
    listPtr = mnew(ClientItem);
    listPtr->clientID = clientID;
    List_InitElement((List_Links *)listPtr);
    List_Insert((List_Links *)listPtr, LIST_ATFRONT(masterClientList));
exit:
    UNLOCK_MONITOR;
#endif notdef
}

