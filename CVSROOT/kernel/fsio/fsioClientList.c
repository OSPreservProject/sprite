/* 
 * fsClient.c --
 *
 *	Routines to handle the client lists maintained at the stream level.
 *	The stream-level client list is needed for migration.
 *
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
#include "rpc.h"



/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_StreamClientOpen --
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
Fsio_StreamClientOpen(clientList, clientID, useFlags, foundPtr)
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
	fs_Stats.object.streamClients++;
    }

    /*
     * Make sure the client is in the master list of all clients for this host.
     */
    Fsconsist_AddClient(clientID);
    if (foundPtr != (Boolean *)NIL) {
	*foundPtr = found;
    }
    return(shared);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_StreamClientClose --
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
Fsio_StreamClientClose(clientList, clientID)
    List_Links		*clientList;	/* List of clients who have it open */
    int			clientID;	/* Host ID of client that had it open */
{
    register	FsStreamClientInfo	*clientPtr;

    LIST_FORALL(clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    List_Remove((List_Links *) clientPtr);
	    free((Address) clientPtr);
	    fs_Stats.object.streamClients--;
	    break;
	}
    }
    return(List_IsEmpty(clientList));
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_StreamClientFind --
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
Fsio_StreamClientFind(clientList, clientID)
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
