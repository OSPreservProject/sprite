/* 
 * fsStream.c --
 *
 *	There are two sets of procedures here.  The first manage the stream
 *	as it relates to the handle table; streams are installed in this
 *	table so that handle synchronization primitives can be used, and
 *	so that streams can be found after migration.  The golden rule is
 *	that the stream's refCount reflects local use, and its client list
 *	is used to reflect remote use of a stream.  Thus I/O server's
 *	don't keep references, only client list entries, unless there is
 *	a local user.
 *
 *	The second set of procedures handle the mapping from streams to
 *	user-level stream IDs,	which are indexes into a per-process array
 *	of stream pointers.
 *
 * Copyright (C) 1987 Regents of the University of California
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
#include "fsStream.h"
#include "fsOpTable.h"
#include "fsClient.h"
#include "fsMigrate.h"
#include "fsStat.h"
#include "proc.h"
#include "sync.h"
#include "rpc.h"

/*
 * Monitor to synchronize access to the streamCount variable.
 */
static	Sync_Lock	streamLock = Sync_LockInitStatic("Fs:streamLock");
#define LOCKPTR (&streamLock)

static int	streamCount;	/* Used to generate fileIDs for streams*/

/*
 * Forward declarations. 
 */
static ReturnStatus StreamMigCallback();
static ReturnStatus GrowStreamList();


/*
 *----------------------------------------------------------------------
 *
 * FsStreamNewClient --
 *
 *	Create a new stream for a client.  This chooses a unique minor number
 *	for the	fileID of the stream, installs it in the handle table,
 *	and initializes the client list to contain the client.  This is
 *	used on the file server to remember clients of regular files, and
 *	when creating pipe streams which need client info for migration.
 *
 * Results:
 *	A pointer to a locked stream with 1 reference and one client entry.
 *
 * Side effects:
 *	Install the new stream into the handle table and increment the global
 *	streamCount used to generate IDs.  The stream is returned locked and
 *	with one reference.  Our caller should release this reference if
 *	this is just a shadow stream.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fs_Stream *
FsStreamNewClient(serverID, clientID, ioHandlePtr, useFlags, name)
    int			serverID;	/* I/O server for stream */
    int			clientID;	/* Client of the stream */
    FsHandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
    int			useFlags;	/* Usage flags from Fs_Open call */
    char		*name;		/* Name for error messages */
{
    register Boolean found;
    register Fs_Stream *streamPtr;
    Fs_Stream *newStreamPtr;
    Fs_FileID fileID;

    LOCK_MONITOR;

    /*
     * The streamID is uniquified by using our own host ID for the major
     * field (for network uniqueness), and then choosing minor
     * numbers until we don't have a local conflict.
     */
    fileID.type = FS_STREAM;
    fileID.serverID = serverID;
    fileID.major = rpc_SpriteID;

    do {
	fileID.minor = ++streamCount;
	found = FsHandleInstall(&fileID, sizeof(Fs_Stream), name,
				(FsHandleHeader **)&newStreamPtr);
	if (found) {
	    /*
	     * Don't want to conflict with existing streams.
	     */
	    FsHandleRelease(newStreamPtr, TRUE);
	}
    } while (found);

    streamPtr = newStreamPtr;
    streamPtr->offset = 0;
    streamPtr->flags = useFlags;
    streamPtr->ioHandlePtr = ioHandlePtr;
    streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
    List_Init(&streamPtr->clientList);
    fsStats.object.streams++;

    (void)FsStreamClientOpen(&streamPtr->clientList, clientID, useFlags,
	    (Boolean *)NIL);

    UNLOCK_MONITOR;
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamAddClient --
 *
 *	Find a stream and add another client to its client list.
 *
 * Results:
 *	A pointer to a locked stream with 1 reference and one client entry.
 *	Our call should release this reference if this is just a shadow stream.
 *
 * Side effects:
 *	Install the new stream into the handle table and increment the global
 *	streamCount used to generate IDs.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fs_Stream *
FsStreamAddClient(streamIDPtr, clientID, ioHandlePtr, useFlags, name,
	    foundClientPtr, foundStreamPtr)
    Fs_FileID		*streamIDPtr;	/* File ID for stream */
    int			clientID;	/* Client of the stream */
    FsHandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
    int			useFlags;	/* Usage flags from Fs_Open call */
    char		*name;		/* Name for error messages */
    /*
     * These two boolean pointers may be NIL if their info is not needed.
     */
    Boolean		*foundClientPtr;/* True if Client already existed */
    Boolean		*foundStreamPtr;/* True if stream already existed */
{
    register Boolean found;
    register Fs_Stream *streamPtr;
    Fs_Stream *newStreamPtr;

    found = FsHandleInstall(streamIDPtr, sizeof(Fs_Stream), name,
			    (FsHandleHeader **)&newStreamPtr);
    streamPtr = newStreamPtr;
    if (!found) {
	streamPtr->offset = 0;
	streamPtr->flags = useFlags;
	streamPtr->ioHandlePtr = ioHandlePtr;
	streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
	List_Init(&streamPtr->clientList);
	fsStats.object.streams++;
    } else if (streamPtr->ioHandlePtr == (FsHandleHeader *)NIL) {
	streamPtr->ioHandlePtr = ioHandlePtr;
    }
    (void)FsStreamClientOpen(&streamPtr->clientList, clientID, useFlags,
	    foundClientPtr);
    if (foundStreamPtr != (Boolean *)NIL) {
	*foundStreamPtr = found;
    }
    UNLOCK_MONITOR;
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamMigClient --
 *
 *	This is called on the I/O server for to move client streams refs.
 *	This makes a callback to the source client to release the reference
 *	to the stream which has (now) moved away.
 *	Note:  this operation locks the stream in order to serialize
 *	with a close comming in from a remote client who has dup'ed the
 *	stream, migrated one refernece, and closed the other reference.
 *	Also, on the client the callback and the regular close will both
 *	try to lock the stream in order to release a reference.  Deadlock
 *	cannot occur because if the close happens first there will be two
 *	references at the client.  The close at the client will release
 *	one reference and not try to contact us.  If the callback occurs
 *	first then the close will come through to us, but it will have
 *	to wait until we are done with this migration.
 *
 * Results:
 *	TRUE if the stream is shared across the network after migration.
 *
 * Side effects:
 *	Shifts the client list entry from one host to another.  This does
 *	not add/subtract any references to the stream here on this host.
 *	However, the call-back releases a reference at the source client.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsStreamMigClient(migInfoPtr, dstClientID, ioHandlePtr, closeSrcClientPtr)
    FsMigInfo		*migInfoPtr;	/* Encapsulated stream */
    int			dstClientID;	/* New client of the stream */
    FsHandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
    Boolean		*closeSrcClientPtr;	/* Return - TRUE if the src
					 * client stopped using stream */
{
    register Boolean found;
    register Fs_Stream *streamPtr;
    Fs_Stream *newStreamPtr;
    int newClientStream = migInfoPtr->flags & FS_NEW_STREAM;
    ReturnStatus status;
    Boolean shared;

    /*
     * Get the stream and synchronize with closes from the client.
     * The I/O handle has to be unlocked while the stream is locked
     * in order to prevent deadlock with un-related open/close activity.
     */
    FsHandleUnlock(ioHandlePtr);
    found = FsHandleInstall(&migInfoPtr->streamID, sizeof(Fs_Stream),
			    (char *)NIL, (FsHandleHeader **)&newStreamPtr);
    streamPtr = newStreamPtr;
    if (!found) {
	streamPtr->offset = migInfoPtr->offset;
	streamPtr->flags = migInfoPtr->flags;
	streamPtr->ioHandlePtr = ioHandlePtr;
	streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
	List_Init(&streamPtr->clientList);
	fsStats.object.streams++;
    } else if (streamPtr->ioHandlePtr == (FsHandleHeader *)NIL) {
	streamPtr->ioHandlePtr = ioHandlePtr;
    }
    if ((streamPtr->flags & FS_RMT_SHARED) == 0) {
	/*
	 * We don't think the stream is being shared so we
	 * grab the offset from the client.
	 */
	streamPtr->offset = migInfoPtr->offset;
    }
    if (migInfoPtr->srcClientID != rpc_SpriteID) {
	/*
	 * Call back to the client to tell it to release its reference
	 * on the stream.  We can't hold the I/O handle locked because
	 * an unrelated close from the source client might have the
	 * I/O handle locked over there.  By unlocking this I/O handle
	 * we allow unrelated closes to complete, while the stream
	 * lock prevents closes of other references to this stream
	 * from comming in and changing the state.
	 */
	status = StreamMigCallback(migInfoPtr, &shared);
    } else {
	/*
	 * The stream has been migrated away from us, the I/O server.
	 * Decrement the stream ref count.  The I/O handle references
	 * are left alone here on the I/O server.
	 */
	FsHandleDecRefCount((FsHandleHeader *)streamPtr);
	shared = (streamPtr->hdr.refCount > 1);
	status = SUCCESS;
    }

    if (status != SUCCESS || !shared) {
	/*
	 * The client doesn't perceive sharing of the stream so
	 * it must be its last reference so we indicate an I/O close is needed.
	 */
	*closeSrcClientPtr = TRUE;
	(void)FsStreamClientClose(&streamPtr->clientList,
				  migInfoPtr->srcClientID);
    } else {
	*closeSrcClientPtr = FALSE;
    }
    /*
     * Mark (unmark) the stream if it is being shared.  This is checked
     * in the read and write RPC stubs in order to know what offset to use,
     * the one here in the shadow stream, or the one from the client.
     */
    if (FsStreamClientOpen(&streamPtr->clientList, dstClientID,
			    migInfoPtr->flags, (Boolean *)NIL)) {
	streamPtr->flags |= FS_RMT_SHARED;
    } else {
	streamPtr->flags &= ~FS_RMT_SHARED;
    }
    migInfoPtr->flags = streamPtr->flags | newClientStream;
    migInfoPtr->offset = streamPtr->offset;
    FsHandleRelease(streamPtr, TRUE);
    FsHandleLock(ioHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * StreamMigCallback --
 *
 *	Call back to the source client of a migration and tell it to
 *	release its stream.  This invokes FsStreamMigrate on the
 *	remote client
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *      None.
 *	
 *----------------------------------------------------------------------
 */
static ReturnStatus
StreamMigCallback(migInfoPtr, sharedPtr)
    FsMigInfo	*migInfoPtr;	/* Encapsulated information */
    Boolean	*sharedPtr;	/* TRUE if stream still used on client */
{
    register ReturnStatus	status;
    Rpc_Storage 	storage;
    FsMigInfo		migInfo;
    FsMigParam		migParam;

    /*
     * We want to call FsStreamMigrate, so we change the ioFileID to
     * the streamID before passing it.
     */
    migInfo = *migInfoPtr;
    migInfo.ioFileID = migInfoPtr->streamID;
    storage.requestParamPtr = (Address) &migInfo;
    storage.requestParamSize = sizeof(FsMigInfo);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address)&migParam;
    storage.replyParamSize = sizeof(FsMigParam);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(migInfoPtr->srcClientID, RPC_FS_MIGRATE, &storage);

    if (status == SUCCESS) {
	FsMigrateReply	*migReplyPtr;

	migReplyPtr = &(migParam.migReply);
	*sharedPtr = migReplyPtr->flags & FS_RMT_SHARED;
    } else if (fsMigDebug) {
	printf(
	  "StreamMigCallback: status %x from remote migrate routine.\n",
		  status);
    }
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsStreamMigrate --
 *
 *	This is called to release a reference to a stream at the source
 *	of a migration.  We are told to release the reference by the
 *	I/O server during its FsStreamMigClient call.  The timing of our
 *	call ensures that a simultaneous Fs_Close on the stream will be
 *	properly synchronized - the I/O server has to know how many
 *	stream references we, the source of a migration, really have.
 *
 *	FIXME: this uses FS_RPC_MIGRATE instead of a new RPC.  This requires
 *	a patch in the RPC stub for FS_RPC_MIGRATE, and a patch here because
 *	there are more references to the stream that originally thought.
 *
 * Results:
 *	SUCCESS unless the stream isn't even found.  This sets the FS_RMT_SHARED
 *	flag in the *flagsPtr field if the stream is still in use here,
 *	otherwise it clears this flag.
 *
 * Side effects:
 *	This releases one reference to the stream.  If it is the last
 *	reference then this propogates the close down to the I/O handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsStreamMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset (not needed) */
    int		*sizePtr;	/* Return - 0 */
    Address	*dataPtr;	/* Return - NIL */
{
    register Fs_Stream *streamPtr;

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    /*
     * Fetch the stream but don't increment the refernece count while
     * keeping it locked.
     */
    streamPtr = FsHandleFetchType(Fs_Stream, &migInfoPtr->streamID);
    if (streamPtr == (Fs_Stream *)NIL) {
	printf(
		"FsStreamMigrate, no stream <%d> for %s handle <%d,%d>\n",
		migInfoPtr->streamID.minor,
		FsFileTypeToString(migInfoPtr->ioFileID.type),
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_FILE_NOT_FOUND);
    }
    FsHandleDecRefCount((FsHandleHeader *)streamPtr);

    *flagsPtr = streamPtr->flags;
    *offsetPtr = streamPtr->offset;
    /*
     * If this is the last refernece then call down to the I/O handle
     * so it can decrement use counts that come from the stream.
     * (The funky check against 2 is because there have been two fetches
     * of the stream.  One by Fs_RpcStartMigration, and one by use.  We
     * decrement our reference above, but there is still an extra from
     * Fs_RpcStartMigration.  Plus there is the original ref from the
     * stream that has migrated.  Thus 2 is the minimum.)
     */
    if (streamPtr->hdr.refCount <= 2) {
	(*fsStreamOpTable[streamPtr->ioHandlePtr->fileID.type].release)
		(streamPtr->ioHandlePtr, streamPtr->flags);
	if (FsStreamClientClose(&streamPtr->clientList, rpc_SpriteID)) {
	    /*
	     * No references, no other clients, nuke it.
	     */
	    *flagsPtr &= ~FS_RMT_SHARED;
	    FsStreamDispose(streamPtr);
	    return(SUCCESS);
	}
    }
    *flagsPtr |= FS_RMT_SHARED;
    FsHandleRelease(streamPtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamNewID --
 *
 *	Generate a new streamID for a client.  This chooses a unique minor
 *	number for the	fileID of the stream and returns the fileID.  This
 *	is used on the file server to generate IDs for remote device streams.
 *	This ID will be used to create matching streams on the device I/O server
 *	and on the client's machine.
 *
 * Results:
 *	A unique fileID for a stream to the given I/O server.
 *
 * Side effects:
 *	Increment the global streamCount used to generate IDs.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsStreamNewID(serverID, streamIDPtr)
    int			serverID;	/* I/O server for stream */
    Fs_FileID		*streamIDPtr;	/* Return - FileID for the stream */
{
    register Boolean found;
    Fs_Stream *newStreamPtr;
    Fs_FileID fileID;

    LOCK_MONITOR;

    /*
     * The streamID is uniquified by using our own host ID for the major
     * field (for network uniqueness), and then choosing minor
     * numbers until we don't have a local conflict.
     */
    fileID.type = FS_STREAM;
    fileID.serverID = serverID;
    fileID.major = rpc_SpriteID;

    do {
	fileID.minor = ++streamCount;
	found = FsHandleInstall(&fileID, sizeof(Fs_Stream), (char *)NIL,
				(FsHandleHeader **)&newStreamPtr);
	if (found) {
	    /*
	     * Don't want to conflict with existing streams.
	     */
	    FsHandleRelease(newStreamPtr, TRUE);
	}
    } while (found);
    *streamIDPtr = newStreamPtr->hdr.fileID;
    FsHandleRelease(newStreamPtr, TRUE);
    FsHandleRemove(newStreamPtr);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_StreamCopy --
 *
 *	Duplicate a stream.  This ups the reference count on the stream
 *	so that it won't go away until its last user closes it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count on the stream is incremented.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fs_StreamCopy(oldStreamPtr, newStreamPtrPtr)
    Fs_Stream *oldStreamPtr;
    Fs_Stream **newStreamPtrPtr;
{
    *newStreamPtrPtr = FsHandleDupType(Fs_Stream, oldStreamPtr);
    FsHandleUnlock(oldStreamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamClientVerify --
 *
 *	Verify that the remote client is known for the stream, and return
 *	a locked pointer to the stream's handle.
 *
 * Results:
 *	A pointer to the handle for the stream, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease(..., TRUE)
 *
 *----------------------------------------------------------------------
 */

Fs_Stream *
FsStreamClientVerify(streamIDPtr, clientID)
    Fs_FileID	*streamIDPtr;	/* Client's stream ID */
    int		clientID;	/* Host ID of the client */
{
    register FsStreamClientInfo *clientPtr;
    register Fs_Stream *streamPtr;
    Boolean found = FALSE;

    streamPtr = FsHandleFetchType(Fs_Stream, streamIDPtr);
    if (streamPtr != (Fs_Stream *)NIL) {
	LIST_FORALL(&streamPtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    register FsHandleHeader *tHdrPtr = streamPtr->ioHandlePtr;
	    printf("FsStreamClientVerify, unknown client %d for stream <%d>\n",
		clientID, tHdrPtr->fileID.minor);
	    FsHandleRelease(streamPtr, TRUE);
	    streamPtr = (Fs_Stream *)NIL;
	}
    } else {
	printf("No stream <%d> for client %d\n", streamIDPtr->minor, clientID);
    }
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamDispose --
 *
 *	Discard a stream.  This call removes the stream from the handle
 *	table and frees associated storage.  The I/O handle pointer part
 *	should have already been cleaned up by its handler.
 *
 *	If the stream still has associated clients, release the reference
 *	to the stream but don't get rid of the stream, since it is a shadow
 *	stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Remove the stream handle from the handle table.
 *
 *----------------------------------------------------------------------
 */

Boolean fsStreamDisposeDebug = TRUE;

ENTRY void
FsStreamDispose(streamPtr)
    register Fs_Stream *streamPtr;
{
    Boolean noClients = TRUE;
    
    if (!List_IsEmpty(&streamPtr->clientList)) {
	noClients = FALSE;
	if (fsStreamDisposeDebug) {
	    register FsStreamClientInfo *clientPtr;

	    LIST_FORALL(&streamPtr->clientList, (List_Links *) clientPtr) {

		printf("FsStreamDispose, client %d still in list for stream <%d,%d>, refCount %d\n",
			  clientPtr->clientID, streamPtr->hdr.fileID.major,
			  streamPtr->hdr.fileID.minor, streamPtr->hdr.refCount);
		if (streamPtr->ioHandlePtr != (FsHandleHeader *)NIL) {
		    printf("\tI/O handle: %s <%d,%d>, refCount %d\n",
			       FsFileTypeToString(streamPtr->ioHandlePtr->fileID.type),
			       streamPtr->ioHandlePtr->fileID.major,
			       streamPtr->ioHandlePtr->fileID.minor,
			       streamPtr->ioHandlePtr->refCount);
		}
	    }
	}
    } 

    FsHandleRelease(streamPtr, TRUE);
    if (noClients) {
	if (streamPtr->nameInfoPtr != (FsNameInfo *)NIL) {
	    free((Address)streamPtr->nameInfoPtr);
	}
	FsHandleRemove(streamPtr);
	fsStats.object.streams--;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamScavenge --
 *
 *	Scavenge a stream.  Servers may have no references to a stream handle,
 *	but still have some things on its client list.  Clients have
 *	references, but no client list.  A stream with neither references
 *	or a client list is scavengable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the stream handle if it has no references and no clients.
 *
 *----------------------------------------------------------------------
 */
#ifdef notdef
Boolean
FsStreamScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;
{
    register Fs_Stream *streamPtr = (Fs_Stream *)hdrPtr;

    if (streamPtr->hdr.refCount == 0 &&
	List_IsEmpty(&streamPtr->clientList)) {
	printf( "FsStreamScavenge, removing stream <%d,%d>\n",
		streamPtr->hdr.fileID.serverID,
		streamPtr->hdr.fileID.minor);
	FsHandleRemove((FsHandleHeader *)streamPtr);
	fsStats.object.streams--;
	return(TRUE);
    } else {
	FsHandleUnlock((FsHandleHeader *)streamPtr);
	return(FALSE);
    }
}
#endif notdef


typedef struct StreamReopenParams {
    Fs_FileID	streamID;
    Fs_FileID	ioFileID;
    int		useFlags;
    int		offset;
} StreamReopenParams;

/*
 *----------------------------------------------------------------------
 *
 * FsStreamReopen --
 *
 *	This is called initially on the client side from FsHandleReopen.
 *	That instance then does an RPC to the server, which again invokes
 *	this routine.  On the client side we don't do much except pass
 *	over the streamID and the ioHandle fileID so the server can
 *	re-create state.  On the server we have to re-setup the stream,
 *	which is sort of a pain because it must reference the correct
 *	I/O handle.
 *
 * Results:
 *	SUCCESS if the stream was reopened.
 *
 * Side effects:
 *	On the client, do an RPC to the server.
 *	On the server, re-create the stream.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY ReturnStatus
FsStreamReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;	/* Stream's handle header */
    int			clientID;
    ClientData		inData;		/* Non-NIL on the server */
    int			*outSizePtr;	/* Non-NIL on the server */
    ClientData		*outDataPtr;	/* Non-NIL on the server */
{
    register Fs_Stream	*streamPtr = (Fs_Stream *)hdrPtr;
    ReturnStatus status;

    if (inData == (ClientData)NIL) {
	/*
	 * Called on the client side.  We contact the server to invoke
	 * this procedure there with some input parameters.
	 */
	StreamReopenParams reopenParams;
	int outSize = 0;

	reopenParams.streamID = hdrPtr->fileID;
	reopenParams.ioFileID = streamPtr->ioHandlePtr->fileID;
	reopenParams.useFlags = streamPtr->flags;
	reopenParams.offset   = streamPtr->offset;
	status = FsSpriteReopen(hdrPtr, sizeof(reopenParams),
		    (Address)&reopenParams, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the server side.  We need to first make sure there
	 * is a corresponding I/O handle for the stream, and then we
	 * can set up the stream.
	 */
	StreamReopenParams	*reopenParamsPtr;
	register Fs_FileID	*fileIDPtr;
	FsHandleHeader		*ioHandlePtr;

	reopenParamsPtr = (StreamReopenParams *)inData;
	fileIDPtr = &reopenParamsPtr->ioFileID;
	ioHandlePtr = (*fsStreamOpTable[fileIDPtr->type].clientVerify)
			(fileIDPtr, clientID, (int *)NIL);
	if (ioHandlePtr != (FsHandleHeader *)NIL) {
	    streamPtr = FsStreamAddClient(&reopenParamsPtr->streamID, clientID,
		    ioHandlePtr, reopenParamsPtr->useFlags, ioHandlePtr->name,
		    (Boolean *)NIL, (Boolean *)NIL);
	    /*
	     * BRENT Have to worry about the shared offset here.
	     */
	    streamPtr->offset = reopenParamsPtr->offset;

	    FsHandleRelease(ioHandlePtr, TRUE);
	    FsHandleRelease(streamPtr, TRUE);
	    status = SUCCESS;
	} else {
	    printf(
		"FsStreamReopen, %s I/O handle <%d,%d> not found\n",
		FsFileTypeToString(fileIDPtr->type),
		fileIDPtr->major, fileIDPtr->minor);
	    status = FAILURE;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsGetStreamID --
 *
 *	Save the stream pointer in the process's list of stream pointers
 *	and return its index in that list.  The index is used as
 *	a handle for the stream.  E.g. the user supplies the index
 *	in read and write calls and the kernel gets the file pointer
 *	from the list.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	others		- value returned by GrowStreamList.
 *
 * Side effects:
 *	It adds the input streamPtr to the end of the process's list,
 *	if the list is too short (or empty) it is expanded (or created).
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsGetStreamID(streamPtr, streamIDPtr)
    Fs_Stream	*streamPtr;	/* A reference to an open file */
    int		*streamIDPtr;	/* Return value, the index of the file pointer
				 * in the process's list of open files */
{
    register Fs_ProcessState	*fsPtr;		/* From process's proc table 
						 * entry */
    register Fs_Stream		**streamPtrPtr;	/* Process's list of open 
						 * streams. */
    register int		index;		/* Index into list of open 
						 * streams */
    ReturnStatus		status;		/* Error from growing file 
						 * list. */

    fsPtr = (Proc_GetEffectiveProc())->fsPtr;

    if (streamPtr == (Fs_Stream *)0) {
	panic( "Zero valued streamPtr");
    }
    if (fsPtr->streamList == (Fs_Stream **)NIL) {
	/*
	 * Allocate the initial array of file pointers.
	 */
	(void)GrowStreamList(fsPtr, 8);
    }

    /*
     * Take the first free streamID, or add a new one to the end.
     */
    for (index = 0, streamPtrPtr = fsPtr->streamList; 
	 index < fsPtr->numStreams; 
	 index++, streamPtrPtr++) {
	if (*streamPtrPtr == (Fs_Stream *)NIL) {
	    *streamPtrPtr = streamPtr;
	    *streamIDPtr = index;
	    fsPtr->streamFlags[index] = 0;
	    return(SUCCESS);
	}
    }
    /*
     * Ran out of room in the original array, allocate a larger
     * array, copy the contents of the original into the beginning,
     * then pick the first empty slot.
     */
    index = fsPtr->numStreams;
    status = GrowStreamList(fsPtr, fsPtr->numStreams * 2);
    if (status == SUCCESS) {
	*streamIDPtr = index;
	fsPtr->streamList[index] = streamPtr;
	fsPtr->streamFlags[index] = 0;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsClearStreamID --
 *
 *	This invalidates a stream ID.  This is called in conjuction
 *	with Fs_Close to close a stream.  The open stream is identified
 *	by the stream ID which this routine invalidates.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stream pointer entry in the open stream list is cleared.
 *
 *----------------------------------------------------------------------
 */
void
FsClearStreamID(streamID, procPtr)
    int streamID;		/* Stream ID to invalidate */
    Proc_ControlBlock *procPtr;	/* (Optional) process pointer */
{
    if (procPtr == (Proc_ControlBlock *)NIL) {
	procPtr = Proc_GetEffectiveProc();
    }
    procPtr->fsPtr->streamList[streamID] = (Fs_Stream *)NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * GrowStreamList --
 *
 *	Grow a stream ID list.  This routine
 *	allocates another array of file pointers and copies the
 *	values from the original array into the new one.  It also
 *	initializes the new array elements to NIL.  The original
 *	array of pointers is free'd and the pointer to the
 *	array is reset to point to the new array.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	(One could limit the number of streams here, but we don't.)
 *
 * Side effects:
 *	Grows the stream list and the associated array of flag bytes.
 *	The number of streams in the file system state is updated.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GrowStreamList(fsPtr, newLength)
    Fs_ProcessState *fsPtr;	/* The file system state */
    int newLength;		/* The length of the new array */
{
    register int index;
    register Fs_Stream **streamList;
    register char *streamFlags;

    streamList = (Fs_Stream **)malloc(newLength * sizeof(Fs_Stream *));
    streamFlags = (char *)malloc(newLength * sizeof(char));

    if (fsPtr->numStreams > 0) {
	bcopy((Address)fsPtr->streamList, (Address)streamList, sizeof(Fs_Stream *) * fsPtr->numStreams);
	bcopy((Address)fsPtr->streamFlags, (Address)streamFlags, sizeof(char) * fsPtr->numStreams);

	free((Address)fsPtr->streamList);
	free((Address)fsPtr->streamFlags);
    
	for (index=0 ; index < fsPtr->numStreams ; index++) {
	    if ((int)streamList[index] != NIL &&
		(unsigned int)streamList[index] < 1024) {
		panic( "GrowStreamList copied bad streamPtr, %x\n",
				       streamList[index]);
	    }
	}
    }

    fsPtr->streamList = streamList;
    fsPtr->streamFlags =  streamFlags;

    for (index=fsPtr->numStreams ; index < newLength ; index++) {
	fsPtr->streamList[index] = (Fs_Stream *)NIL;
	fsPtr->streamFlags[index] = 0;
    }
    fsPtr->numStreams = newLength;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsGetStreamPtr --
 *
 *	This converts a users stream id into a pointer to the
 *	stream structure for the open stream.  The stream id is
 *	an index into a per-process open stream list.  This does
 *	bounds checking the open stream list and returns the
 *	indexed stream pointer.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_INVALID_ARG	- the stream ID was out of range or the streamPtr
 *			  for streamID was NIL.
 *
 * Side effects:
 *	*streamPtrPtr is set to reference the stream structure indexed
 *	by the streamID.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsGetStreamPtr(procPtr, streamID, streamPtrPtr)
    Proc_ControlBlock	*procPtr;	/* The owner of an open file list */
    int			streamID;	/* A possible index into the list */
    Fs_Stream		**streamPtrPtr;	/* The pointer from the list*/
{
    if (streamID < 0 || streamID >= procPtr->fsPtr->numStreams) {
	return(FS_INVALID_ARG);
    } else {
	register Fs_Stream *streamPtr;

	streamPtr = procPtr->fsPtr->streamList[streamID];
	if (streamPtr == (Fs_Stream *)NIL) {
	    return(FS_INVALID_ARG);
	} else if ((unsigned int)streamPtr < 1024) {
	    /*
	     * There was a time when control stream pointers were not
	     * being passed right, or being passed a second time after
	     * already being converted to a streamID, which resulted in
	     * small integers being kept in the stream list instead of
	     * valid stream pointers.  Not sure if that still happens.
	     */
	    panic( "Stream Ptr # %d was an int %d!\n",
				streamID, streamPtr);
	    return(FS_INVALID_ARG);
	} else {
	    *streamPtrPtr = streamPtr;
	    return(SUCCESS);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetNewID --
 *
 *	This gets a new stream ID that refers to the same open file
 *	as the first argument.  After the call the new stream ID will
 *	be equivalent to the old one - system calls that take a stream
 *	ID could be passed either stream ID.  There are two uses of this
 *	routine.  If it doesn't matter what the new stream ID is then
 *	the second argument should point to FS_ANYID.  If the new stream
 *	ID should have a value then the second argument should point
 *	to that value.  If that value was a valid stream ID then the
 *	stream is first closed.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_INVALID_ARG	- newStreamIDPtr was bad or had a bad value.
 *	other		- value returned by FsGrowList.
 *
 * Side effects:
 *	The second argument gets instantiated to a new stream ID.
 *	If the second argument refered to a valid stream ID on entry
 *	(as opposed to pointing to FS_ANYID) then that stream is first closed.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_GetNewID(streamID, newStreamIDPtr)
    int streamID;
    int *newStreamIDPtr;
{
    register ReturnStatus 	status;
    Fs_Stream 			*streamPtr;
    Proc_ControlBlock		*procPtr;
    register Fs_ProcessState	*fsPtr;

    if (newStreamIDPtr == (int *)NIL) {
	return(FS_INVALID_ARG);
    }
    procPtr = Proc_GetEffectiveProc();
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    fsPtr = procPtr->fsPtr;
    if (*newStreamIDPtr == FS_ANYID) {
	Fs_Stream		*newStreamPtr;

	Fs_StreamCopy(streamPtr, &newStreamPtr);
	status = FsGetStreamID(newStreamPtr, newStreamIDPtr);
	if (status != SUCCESS) {
	    (void)Fs_Close(newStreamPtr);
	}
	return(status);
    } else {
	if (*newStreamIDPtr == streamID) {
	    /*
	     * Probably a user error.  We just return without fiddling
	     * with reference counts.
	     */
	    return(SUCCESS);
	} else {
	    /*
	     * Trying to get a specific stream ID.
	     */
	    register int newStreamID;

	    newStreamID = *newStreamIDPtr;
	    if (newStreamID < 0) {
		return(FS_INVALID_ARG);
	    }
	    if (newStreamID >= fsPtr->numStreams) {
		register int maxID;
		/*
		 * Need to grow the file list to accomodate this stream ID.
		 * We do a sanity check on the value of stream ID so
		 * we don't nuke ourselves with a huge array.
		 */
		maxID = 2 * fsPtr->numStreams;
		maxID = (maxID<128 ? 128 : maxID);
		if (newStreamID > maxID) {
		    return(FS_NEW_ID_TOO_BIG);
		}
		status = GrowStreamList(fsPtr, newStreamID + 1 );
		if (status != SUCCESS) {
		    return(status);
		}
	    } else {
		/*
		 * Check to see if *newStreamIDPtr is a valid (Fs_Stream *)
		 * and close it if it is.
		 */
		register Fs_Stream *oldFilePtr;

		oldFilePtr = fsPtr->streamList[newStreamID];
		if (oldFilePtr != (Fs_Stream *)NIL) {
		    (void)Fs_Close(oldFilePtr);
		}
	    }
	    Fs_StreamCopy(streamPtr, &fsPtr->streamList[newStreamID]);
	    return(SUCCESS);
	}
    }
}
