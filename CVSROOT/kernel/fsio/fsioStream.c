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
#include "fsutil.h"
#include "fsio.h"
#include "fsNameOps.h"
#include "fsconsist.h"
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
 * Fsio_StreamCreate --
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
Fsio_StreamCreate(serverID, clientID, ioHandlePtr, useFlags, name)
    int			serverID;	/* I/O server for stream */
    int			clientID;	/* Client of the stream */
    Fs_HandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
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
    fileID.type = FSIO_STREAM;
    fileID.serverID = serverID;
    fileID.major = rpc_SpriteID;

    do {
	fileID.minor = ++streamCount;
	found = Fsutil_HandleInstall(&fileID, sizeof(Fs_Stream), name,
				(Fs_HandleHeader **)&newStreamPtr);
	if (found) {
	    /*
	     * Don't want to conflict with existing streams.
	     */
	    Fsutil_HandleRelease(newStreamPtr, TRUE);
	}
    } while (found);

    streamPtr = newStreamPtr;
    streamPtr->offset = 0;
    streamPtr->flags = useFlags;
    streamPtr->ioHandlePtr = ioHandlePtr;
    streamPtr->nameInfoPtr = (Fs_NameInfo *)NIL;
    List_Init(&streamPtr->clientList);
    fs_Stats.object.streams++;

    (void)Fsio_StreamClientOpen(&streamPtr->clientList, clientID, useFlags,
	    (Boolean *)NIL);

    UNLOCK_MONITOR;
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_StreamAddClient --
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
Fsio_StreamAddClient(streamIDPtr, clientID, ioHandlePtr, useFlags, name,
	    foundClientPtr, foundStreamPtr)
    Fs_FileID		*streamIDPtr;	/* File ID for stream */
    int			clientID;	/* Client of the stream */
    Fs_HandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
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

    found = Fsutil_HandleInstall(streamIDPtr, sizeof(Fs_Stream), name,
			    (Fs_HandleHeader **)&newStreamPtr);
    streamPtr = newStreamPtr;
    if (!found) {
	streamPtr->offset = 0;
	streamPtr->flags = useFlags;
	streamPtr->ioHandlePtr = ioHandlePtr;
	streamPtr->nameInfoPtr = (Fs_NameInfo *)NIL;
	List_Init(&streamPtr->clientList);
	fs_Stats.object.streams++;
    } else if (streamPtr->ioHandlePtr == (Fs_HandleHeader *)NIL) {
	streamPtr->ioHandlePtr = ioHandlePtr;
    }
    (void)Fsio_StreamClientOpen(&streamPtr->clientList, clientID, useFlags,
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
 * Fsio_StreamMigClient --
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
Fsio_StreamMigClient(migInfoPtr, dstClientID, ioHandlePtr, closeSrcClientPtr)
    FsMigInfo		*migInfoPtr;	/* Encapsulated stream */
    int			dstClientID;	/* New client of the stream */
    Fs_HandleHeader	*ioHandlePtr;	/* I/O handle to attach to stream */
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
    Fsutil_HandleUnlock(ioHandlePtr);
    found = Fsutil_HandleInstall(&migInfoPtr->streamID, sizeof(Fs_Stream),
			    (char *)NIL, (Fs_HandleHeader **)&newStreamPtr);
    streamPtr = newStreamPtr;
    if (!found) {
	streamPtr->offset = migInfoPtr->offset;
	streamPtr->flags = migInfoPtr->flags & ~FS_NEW_STREAM;
	streamPtr->ioHandlePtr = ioHandlePtr;
	streamPtr->nameInfoPtr = (Fs_NameInfo *)NIL;
	List_Init(&streamPtr->clientList);
	fs_Stats.object.streams++;
    } else if (streamPtr->ioHandlePtr == (Fs_HandleHeader *)NIL) {
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
	Fsutil_HandleDecRefCount((Fs_HandleHeader *)streamPtr);
	shared = (streamPtr->hdr.refCount > 1);
	status = SUCCESS;
    }

    if (status != SUCCESS || !shared) {
	/*
	 * The client doesn't perceive sharing of the stream so
	 * it must be its last reference so we indicate an I/O close is needed.
	 */
	*closeSrcClientPtr = TRUE;
	(void)Fsio_StreamClientClose(&streamPtr->clientList,
				  migInfoPtr->srcClientID);
    } else {
	*closeSrcClientPtr = FALSE;
    }
    /*
     * Mark (unmark) the stream if it is being shared.  This is checked
     * in the read and write RPC stubs in order to know what offset to use,
     * the one here in the shadow stream, or the one from the client.
     */
    if (Fsio_StreamClientOpen(&streamPtr->clientList, dstClientID,
			    migInfoPtr->flags, (Boolean *)NIL)) {
	streamPtr->flags |= FS_RMT_SHARED;
    } else {
	streamPtr->flags &= ~FS_RMT_SHARED;
    }
    migInfoPtr->flags = streamPtr->flags | newClientStream;
    migInfoPtr->offset = streamPtr->offset;
    Fsutil_HandleRelease(streamPtr, TRUE);
    Fsutil_HandleLock(ioHandlePtr);
}

/*
 * Parameters and results for RPC_FS_RELEASE that is called
 * to release a reference to a stream on the source of a migration.
 */
typedef struct {
    Fs_FileID streamID;		/* Stream from which to release a reference */
} FsStreamReleaseParam;

typedef struct {
    Boolean	inUse;		/* TRUE if stream still in use after release */
} FsStreamReleaseReply;

/*
 *----------------------------------------------------------------------
 *
 * StreamMigCallback --
 *
 *	Call back to the source client of a migration and tell it to
 *	release its stream.  This invokes Fsio_StreamMigrate on the
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
    Rpc_Storage 		storage;
    FsStreamReleaseParam	param;
    FsStreamReleaseReply	reply;

    param.streamID = migInfoPtr->streamID;
    storage.requestParamPtr = (Address) &param;
    storage.requestParamSize = sizeof(param);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    reply.inUse = FALSE;
    storage.replyParamPtr = (Address)&reply;
    storage.replyParamSize = sizeof(reply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(migInfoPtr->srcClientID, RPC_FS_RELEASE, &storage);

    *sharedPtr = reply.inUse;
#ifdef NOTDEF
    if (status != SUCCESS && fsio_MigDebug) {
	printf("StreamMigCallback: status %x from RPC.\n", status);
    }
#endif
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_RpcStreamMigClose --
 *
 *	The service stub for FsStreamMigCallback.
 *	This invokes the StreamMigrate routine that releases a reference
 *	to a stream on this host.  Our reply message indicates if
 *	the stream is still in use on this host.
 *
 * Results:
 *	FS_STALE_HANDLE if handle that if client that is migrating the file
 *	doesn't have the file opened on this machine.  Otherwise return
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_RpcStreamMigClose(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    register FsStreamReleaseParam	*paramPtr;
    register Fs_Stream			*streamPtr;
    register ReturnStatus		status;
    register FsStreamReleaseReply	*replyPtr;
    register Rpc_ReplyMem		*replyMemPtr;

    paramPtr = (FsStreamReleaseParam *) storagePtr->requestParamPtr;

    streamPtr = Fsio_StreamClientVerify(&paramPtr->streamID,
				(Fs_HandleHeader *)NIL, rpc_SpriteID);
    if (streamPtr == (Fs_Stream *) NIL) {
	printf("Fsio_RpcStreamMigClose, unknown stream <%d>, client %d\n",
	    paramPtr->streamID.minor, clientID);
	return( (paramPtr->streamID.minor < 0) ? GEN_INVALID_ARG
					       : FS_STALE_HANDLE);
    }
    replyPtr = mnew(FsStreamReleaseReply);
    storagePtr->replyParamPtr = (Address)replyPtr;
    storagePtr->replyParamSize = sizeof(FsStreamReleaseReply);
    storagePtr->replyDataPtr = (Address)NIL;
    storagePtr->replyDataSize = 0;

    status = Fsio_StreamMigClose(streamPtr, &replyPtr->inUse);

    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_StreamMigClose --
 *
 *	This is called to release a reference to a stream at the source
 *	of a migration.  We are told to release the reference by the
 *	I/O server during its Fsio_StreamMigClient call.  The timing of our
 *	call ensures that a simultaneous Fs_Close on the stream will be
 *	properly synchronized - the I/O server has to know how many
 *	stream references we, the source of a migration, really have.
 *
 * Results:
 *	SUCCESS unless the stream isn't even found.  This sets the FS_RMT_SHARED
 *	flag in the *flagsPtr field if the stream is still in use here,
 *	otherwise it clears this flag.
 *
 * Side effects:
 *	This releases one reference to the stream.  If it is the last
 *	reference then this propogates the close down to the I/O handle
 *	by calling the stream-specific release procedure.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_StreamMigClose(streamPtr, inUsePtr)
    Fs_Stream *streamPtr;	/* Stream to release, should be locked */
    Boolean *inUsePtr;		/* TRUE if still in use after release */
{
    /*
     * Release the refernece that has now migrated away.
     */
    Fsutil_HandleDecRefCount((Fs_HandleHeader *)streamPtr);
    /*
     * If this is the last refernece then call down to the I/O handle
     * so it can decrement use counts that come from the stream.
     * (Remember there is still one reference from Fsio_RpcStreamMigClose)
     */
    if (streamPtr->hdr.refCount <= 1) {
	(*fsio_StreamOpTable[streamPtr->ioHandlePtr->fileID.type].release)
		(streamPtr->ioHandlePtr, streamPtr->flags);
	if (Fsio_StreamClientClose(&streamPtr->clientList, rpc_SpriteID)) {
	    /*
	     * No references, no other clients, nuke it.
	     */
	    *inUsePtr = FALSE;
	    Fsio_StreamDestroy(streamPtr);
	    return(SUCCESS);
	}
    }
    *inUsePtr = TRUE;
    Fsutil_HandleRelease(streamPtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_StreamCreateID --
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
Fsio_StreamCreateID(serverID, streamIDPtr)
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
    fileID.type = FSIO_STREAM;
    fileID.serverID = serverID;
    fileID.major = rpc_SpriteID;

    do {
	fileID.minor = ++streamCount;
	found = Fsutil_HandleInstall(&fileID, sizeof(Fs_Stream), (char *)NIL,
				(Fs_HandleHeader **)&newStreamPtr);
	if (found) {
	    /*
	     * Don't want to conflict with existing streams.
	     */
	    Fsutil_HandleRelease(newStreamPtr, TRUE);
	}
    } while (found);
    *streamIDPtr = newStreamPtr->hdr.fileID;
    Fsutil_HandleRelease(newStreamPtr, TRUE);
    Fsutil_HandleRemove(newStreamPtr);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_StreamCopy --
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
Fsio_StreamCopy(oldStreamPtr, newStreamPtrPtr)
    Fs_Stream *oldStreamPtr;
    Fs_Stream **newStreamPtrPtr;
{
    *newStreamPtrPtr = Fsutil_HandleDupType(Fs_Stream, oldStreamPtr);
    Fsutil_HandleUnlock(oldStreamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_StreamClientVerify --
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
 *	It should be released with Fsutil_HandleRelease(..., TRUE)
 *
 *----------------------------------------------------------------------
 */

Fs_Stream *
Fsio_StreamClientVerify(streamIDPtr, ioHandlePtr, clientID)
    Fs_FileID	*streamIDPtr;		/* Client's stream ID */
    Fs_HandleHeader *ioHandlePtr;	/* I/O handle the client thinks
					 * is attached to the stream */
    int		clientID;		/* Host ID of the client */
{
    register FsioStreamClient *clientPtr;
    register Fs_Stream *streamPtr;
    Boolean found = FALSE;

    streamPtr = Fsutil_HandleFetchType(Fs_Stream, streamIDPtr);
    if (streamPtr != (Fs_Stream *)NIL) {
	LIST_FORALL(&streamPtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    printf("Fsio_StreamClientVerify: no client %d for stream <%d> \"%s\"\n",
		clientID, streamPtr->hdr.fileID.minor,
		Fsutil_HandleName((Fs_HandleHeader *)streamPtr));
	    Fsutil_HandleRelease(streamPtr, TRUE);
	    streamPtr = (Fs_Stream *)NIL;

	} else if (ioHandlePtr != (Fs_HandleHeader *)NIL &&
		   streamPtr->ioHandlePtr != ioHandlePtr) {
	    /*
	     * The client's stream doesn't reference the same handle as we do.
	     * Note that ioHandlePtr is NIL when we are called from
	     * Fsio_RpcStreamMigClose, so we can't make this check in that case.
	     */
	    printf("Fsio_StreamClientVerify ioHandle mismatch client ID %d:\n",
			clientID);
	    if (streamPtr->ioHandlePtr == (Fs_HandleHeader *)NIL) {
		printf("\tStream <%d> \"%s\" my I/O handle NIL\n",
		    Fsutil_HandleName(streamPtr), streamIDPtr->minor);
	    } else {
		printf("\tStream <%d> my handle %s \"%s\" <%d,%d>\n",
		    streamIDPtr->minor,
		    Fsutil_FileTypeToString(streamPtr->ioHandlePtr->fileID.type),
		    Fsutil_HandleName(streamPtr->ioHandlePtr),
		    streamPtr->ioHandlePtr->fileID.major,
		    streamPtr->ioHandlePtr->fileID.minor);
	    }
	    printf("\tClient %d handle %s \"%s\" <%d,%d>\n",
		    clientID, Fsutil_HandleName(ioHandlePtr),
		    Fsutil_FileTypeToString(ioHandlePtr->fileID.type),
		    ioHandlePtr->fileID.major, ioHandlePtr->fileID.minor);
	   Fsutil_HandleRelease(streamPtr, TRUE);
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
 * Fsio_StreamDestroy --
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

Boolean fsio_StreamDisposeDebug = TRUE;

ENTRY void
Fsio_StreamDestroy(streamPtr)
    register Fs_Stream *streamPtr;
{
    Boolean noClients = TRUE;
    
    if (!List_IsEmpty(&streamPtr->clientList)) {
	noClients = FALSE;
	if (fsio_StreamDisposeDebug) {
	    register FsioStreamClient *clientPtr;

	    LIST_FORALL(&streamPtr->clientList, (List_Links *) clientPtr) {

		printf("Fsio_StreamDestroy, client %d still in list for stream <%d,%d>, refCount %d\n",
			  clientPtr->clientID, streamPtr->hdr.fileID.major,
			  streamPtr->hdr.fileID.minor, streamPtr->hdr.refCount);
		if (streamPtr->ioHandlePtr != (Fs_HandleHeader *)NIL) {
		    printf("\tI/O handle: %s <%d,%d>, refCount %d\n",
			       Fsutil_FileTypeToString(streamPtr->ioHandlePtr->fileID.type),
			       streamPtr->ioHandlePtr->fileID.major,
			       streamPtr->ioHandlePtr->fileID.minor,
			       streamPtr->ioHandlePtr->refCount);
		}
	    }
	}
    } 

    Fsutil_HandleRelease(streamPtr, TRUE);
    if (noClients) {
	if (streamPtr->nameInfoPtr != (Fs_NameInfo *)NIL) {
	    free((Address)streamPtr->nameInfoPtr);
	}
	Fsutil_HandleRemove(streamPtr);
	fs_Stats.object.streams--;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_StreamScavenge --
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
Fsio_StreamScavenge(hdrPtr)
    Fs_HandleHeader *hdrPtr;
{
    register Fs_Stream *streamPtr = (Fs_Stream *)hdrPtr;

    if (streamPtr->hdr.refCount == 0 &&
	List_IsEmpty(&streamPtr->clientList)) {
	printf( "Fsio_StreamScavenge, removing stream <%d,%d>\n",
		streamPtr->hdr.fileID.serverID,
		streamPtr->hdr.fileID.minor);
	Fsutil_HandleRemove((Fs_HandleHeader *)streamPtr);
	fs_Stats.object.streams--;
	return(TRUE);
    } else {
	Fsutil_HandleUnlock((Fs_HandleHeader *)streamPtr);
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
 * Fsio_StreamReopen --
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
Fsio_StreamReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;	/* Stream's handle header */
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
	status = FsrmtReopen(hdrPtr, sizeof(reopenParams),
		    (Address)&reopenParams, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the server side.  We need to first make sure there
	 * is a corresponding I/O handle for the stream, and then we
	 * can set up the stream.
	 */
	StreamReopenParams	*reopenParamsPtr;
	register Fs_FileID	*fileIDPtr;
	Fs_HandleHeader		*ioHandlePtr;
	/* 
	 * Note about shared stream recovery.  We loose the offset of
	 * a shared stream during a crash.  We just print a message and
	 * try to go forward.  Often the shared stream is for a process's
	 * current working directory, so the offset doesn't matter.
	 */
	Boolean			patchOffset = FALSE;

	reopenParamsPtr = (StreamReopenParams *)inData;
	fileIDPtr = &reopenParamsPtr->ioFileID;
	ioHandlePtr = (*fsio_StreamOpTable[fileIDPtr->type].clientVerify)
			(fileIDPtr, clientID, (int *)NIL);
	if (ioHandlePtr != (Fs_HandleHeader *)NIL) {
	    status = SUCCESS;
	    streamPtr = Fsutil_HandleFetchType(Fs_Stream,
			&reopenParamsPtr->streamID);
	    if (streamPtr != (Fs_Stream *)NIL) {
		/*
		 * Verify that we have the stream hooked to the same
		 * I/O handle as the client.  It is possible that we
		 * have reused the client's stream ID with a different
		 * I/O handle, in which case the client loses this stream.
		 */
		if (streamPtr->ioHandlePtr != ioHandlePtr) {
		    printf("Fsio_StreamReopen, I/O handle mismatch, client %d its I/O <%d,%d> my I/O <%d,%d>\n",
			clientID,
			ioHandlePtr->fileID.major,
			ioHandlePtr->fileID.minor,
			streamPtr->ioHandlePtr->fileID.major,
			streamPtr->ioHandlePtr->fileID.minor);
		    status = FAILURE;
		} else if ((reopenParamsPtr->useFlags & FS_RMT_SHARED) &&
			   (streamPtr->flags & FS_RMT_SHARED) == 0) {
		    /*
		     * The client thinks the stream is shared by processes
		     * on a different client, but we lost the shadow stream.
		     * (If we do think the stream is shared things are ok.)
		     */
		    printf("Fsio_StreamReopen, not a shadow stream, client %d stream <%d> client I/O <%d,%d>\n",
			    clientID, streamPtr->hdr.fileID.minor,
			    ioHandlePtr->fileID.major,
			    ioHandlePtr->fileID.minor);
		    status = FAILURE;
		}
		Fsutil_HandleRelease(streamPtr, TRUE);
	    } else if (reopenParamsPtr->useFlags & FS_RMT_SHARED) {
		/*
		 * The client thinks the stream is shared by processes
		 * on a different client, but we don't have a shadow stream.
		 */
		printf("Fsio_StreamReopen, lost shared stream offset? using offset (%d), client %d, I/O <%d,%d> \"%s\"\n",
			reopenParamsPtr->offset, clientID,
			ioHandlePtr->fileID.major,
			ioHandlePtr->fileID.minor,
			Fsutil_HandleName(ioHandlePtr));
		patchOffset = TRUE;
	    }
	    if (status == SUCCESS) {
		streamPtr = Fsio_StreamAddClient(&reopenParamsPtr->streamID,
		    clientID, ioHandlePtr, reopenParamsPtr->useFlags,
		    ioHandlePtr->name, (Boolean *)NIL, (Boolean *)NIL);
		/*
		 * There isn't proper recovery of the offset if the stream
		 * was shared when we crashed, but here we fake it.
		 */
		if (patchOffset) {
		    streamPtr->offset = reopenParamsPtr->offset;
		}
		Fsutil_HandleRelease(streamPtr, TRUE);
	    }
	    Fsutil_HandleRelease(ioHandlePtr, TRUE);
	} else {
	    printf("Fsio_StreamReopen, %s I/O handle <%d,%d> not found\n",
		Fsutil_FileTypeToString(fileIDPtr->type),
		fileIDPtr->major, fileIDPtr->minor);
	    status = FAILURE;
	}
    }
    return(status);
}
