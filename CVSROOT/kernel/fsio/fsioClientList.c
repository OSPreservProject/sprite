/* 
 * fsClient.c --
 *
 *	Routines to handle the client lists.  There are two kinds of client
 *	lists maintained, one at the stream level, and one at the I/O handle
 *	level.  The stream-level client list is needed for migration, and
 *	the I/O handle client list is needed for cache consistency and to
 *	verify that client's are valid.  The routines here add and remove
 *	clients from the two kinds of lists.
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
#include "fsInt.h"
#include "fsClient.h"
#include "fsStream.h"
#include "fsRecovery.h"
#include "rpc.h"

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
void ClientOpenInt();


/*
 * ----------------------------------------------------------------------------
 *
 * FsClientInit --
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
FsClientInit()
{
    List_Init(&masterClientListHdr);
    masterClientList = &masterClientListHdr;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsIOClientOpen --
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

ENTRY FsClientInfo *
FsIOClientOpen(clientList, clientID, useFlags, cached)
    List_Links	*clientList;	/* List of clients for the I/O handle. */
    int		clientID;	/* The client who is opening the file. */
    int		useFlags;	/* FS_READ | FS_WRITE | FS_EXECUTE */
    Boolean	cached;		/* Boolean property recorded for client */
{
    register FsClientInfo *clientPtr;

    LIST_FORALL(clientList, (List_Links *)clientPtr) {
	if (clientPtr->clientID == clientID) {
	    goto found;
	}
    }
    clientPtr = mnew(FsClientInfo);
    clientPtr->clientID = clientID;
    clientPtr->use.ref = 0;
    clientPtr->use.write = 0;
    clientPtr->use.exec = 0;
    clientPtr->openTimeStamp = 0;
    List_InitElement((List_Links *)clientPtr);
    List_Insert((List_Links *) clientPtr, LIST_ATFRONT(clientList));
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
    ClientOpenInt(clientID);
    return(clientPtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsIOClientClose --
 *
 *	Decrement the reference, executor and/or writer counts for the client 
 *	for the given handle.  Note, we don't mess with the master list of
 *	clients.  That gets cleaned up by FsClientScavenge.
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
FsIOClientClose(clientList, clientID, flags, cachePtr)
    List_Links		*clientList;	/* List of clients for I/O handle */
    int			clientID;	/* Host ID of client that had it open */
    register int	flags;		/* Flags from the stream. */
    Boolean		*cachePtr;	/* In/Out.  If TRUE on entry, this won't
					 * delete the client list entry if
					 * the entry's cached field is also
					 * TRUE.  On return, this is the value
					 * of the client's cached field. */
{
    register	FsClientInfo	*clientPtr;
    register	Boolean		found = FALSE;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	clientPtr->use.ref--;
	if (flags & FS_WRITE) {
	    clientPtr->use.write--;
	}
	if (flags & FS_EXECUTE) {
	    clientPtr->use.exec--;
	}
	if ((clientPtr->use.ref < 0) || (clientPtr->use.write < 0) ||
	    (clientPtr->use.exec < 0)) {
	    panic("FsClientClose: client %d ref %d write %d exec %d\n",
		clientPtr->clientID,
		clientPtr->use.ref, clientPtr->use.write, clientPtr->use.exec);
	}
	if ((!(*cachePtr) || !clientPtr->cached) &&
	    (clientPtr->use.ref == 0)) {
	    List_Remove((List_Links *) clientPtr);
	    free((Address) clientPtr);
	    *cachePtr = FALSE;
	} else {
	    *cachePtr = clientPtr->cached;
	}
    }
    return(found);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsIOClientRemoveWriter --
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
FsIOClientRemoveWriter(clientList, clientID)
    List_Links		*clientList;	/* List of clients for I/O handle */
    int			clientID;	/* Host ID of client that had it open */
{
    register	FsClientInfo	*clientPtr;
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
	    panic("FsClientRemoveWriter: client %d ref %d write %d exec %d\n",
		clientPtr->clientID,
		clientPtr->use.ref, clientPtr->use.write, clientPtr->use.exec);
	}
    }
    return(found);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsStreamClientOpen --
 *
 *	Add a client to the set of clients for a stream.  When a stream is
 *	created the client list is started, and when migration causes
 *	the stream to be shared by processes on different clients, new client
 *	list elements are added.
 *
 * Results:
 *	TRUE if the stream is shared by different clients after the open.
 *	FALSE if this client is the only client of the stream.
 *
 * Side effects:
 *	As well as putting the client in it's list,
 *	the client is recorded in the master list of clients (if it isn't
 *	already there) so client hosts can be easily scavenged.
 *
 * ----------------------------------------------------------------------------
 */

Boolean
FsStreamClientOpen(clientList, clientID, useFlags, foundPtr)
    List_Links		*clientList;	/* List of clients */
    int			clientID;	/* The client who is opening the file */
    int			useFlags;	/* FS_READ | FS_WRITE | FS_EXECUTE */
    Boolean		*foundPtr;	/* Return - TRUE if client existed */
{
    register FsStreamClientInfo *clientPtr;
    register Boolean found = FALSE;
    register Boolean shared = FALSE;

    LIST_FORALL(clientList, (List_Links *)clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	} else {
	    shared = TRUE;
	    if (found) {
		break;
	    }
	}
    }
    if (!found) {
	clientPtr = mnew(FsStreamClientInfo);
	clientPtr->clientID = clientID;
	clientPtr->useFlags = useFlags;
	List_InitElement((List_Links *)clientPtr);
	List_Insert((List_Links *) clientPtr, LIST_ATFRONT(clientList));
    }

    /*
     * Make sure the client is in the master list of all clients for this host.
     */
    ClientOpenInt(clientID);
    if (foundPtr != (Boolean *)NIL) {
	*foundPtr = found;
    }
    return(shared);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsStreamClientClose --
 *
 *	Remove a client from the client list of a stream.
 *
 * Results:
 *	TRUE if the client list is empty after the close.
 *
 * Side effects:
 *	The client list entry from the stream is removed.
 *
 * ----------------------------------------------------------------------------
 *
 */
Boolean
FsStreamClientClose(clientList, clientID)
    List_Links		*clientList;	/* List of clients who have it open */
    int			clientID;	/* Host ID of client that had it open */
{
    register	FsStreamClientInfo	*clientPtr;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    List_Remove((List_Links *) clientPtr);
	    free((Address) clientPtr);
	    break;
	}
    }
    return(List_IsEmpty(clientList));
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsStreamClientFind --
 *
 *      See if a client appears in a client list.
 *
 * Results:
 *      TRUE if the client is in the list.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 *
 */
Boolean
FsStreamClientFind(clientList, clientID)
    List_Links          *clientList;    /* List of clients who have it open */
    int                 clientID;       /* Host ID of client to find */
{
    register    FsStreamClientInfo      *clientPtr;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    return(TRUE);
	}
    }
    return(FALSE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * ClientOpenInt --
 *
 *      Add a client to the master list of clients that is checked by
 *	FsClientScavenge.
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
ENTRY void
ClientOpenInt(clientID)
    int clientID;
{
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
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsClientScavenge --
 *
 *      Check the master list of clients for ones that have crashed.  If any
 *	one has then we call FsRemoveClient to clean up the file state
 *	associated with it.
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
ENTRY void
FsClientScavenge()
{
    register	ClientItem	*listPtr;

    LOCK_MONITOR;

    LIST_FORALL(masterClientList, (List_Links *)listPtr) {
	if (listPtr->clientID != rpc_SpriteID && 
	    Recov_IsHostDown(listPtr->clientID) == FAILURE) {
	    FsRemoveClient(listPtr->clientID);
	}
    }

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsIOClientKill --
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
FsIOClientKill(clientList, clientID, refPtr, writePtr, execPtr)
    List_Links *clientList;	/* List of clients to a file. */
    int		clientID;	/* Client to delete. */
    int		*refPtr;	/* Number of times client has file open. */
    int		*writePtr;	/* Number of times client is writing file. */
    int		*execPtr;	/* Number of times clients is executing file.*/
{
    register FsClientInfo 	*clientPtr;

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
	    List_Remove((List_Links *) clientPtr);
	    free((Address) clientPtr);
	    break;
	}
    }

}

/*
 * ----------------------------------------------------------------------------
 *
 * FsIOClientStatus --
 *
 *	This returns  information similar to that of FsIOClientKill, but it
 *	does not remove the client from the list.  It is used for determining
 *	reference counts for process migration.
 *
 * Results:
 *	If non-NIL, *inUsePtr set to the number of times the client
 *	has the file open, *writingPtr set to the number of times the
 *	client has the file open for writing, and *executingPtr set to
 *	the number of times the client has the file open for
 *	execution.
 *	
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
FsIOClientStatus(clientList, clientID, refPtr, writePtr, execPtr)
    List_Links *clientList;	/* List of clients to a file. */
    int		clientID;	/* Client to check. */
    int		*refPtr;	/* Number of times client has file open. */
    int		*writePtr;	/* Number of times client is writing file. */
    int		*execPtr;	/* Number of times clients is executing file.*/
{
    register FsClientInfo 	*clientPtr;

    /*
     * Count references for the client using the file.
     */
    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    if (refPtr != (int *) NIL) {
		*refPtr = clientPtr->use.ref;
	    }
	    if (writePtr != (int *) NIL) {
		*writePtr = clientPtr->use.write;
	    }
	    if (execPtr != (int *) NIL) {
		*execPtr = clientPtr->use.exec;
	    }
	    return;
	}
    }
    if (refPtr != (int *) NIL) {
	*refPtr = 0;
    }
    if (writePtr != (int *) NIL) {
	*writePtr = 0;
    }
    if (execPtr != (int *) NIL) {
	*execPtr = 0;
    }
}


