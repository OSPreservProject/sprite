/*
 * fsMigrate.c --
 *
 * Procedures to handle migrating open files between machines.  The basic
 * strategy is to first do some local book-keeping on the client we are
 * leaving, then ship state to the new client, then finally tell the
 * I/O server about it, and finish up with local book-keeping on the
 * new client.  There are three stream-type procedures used: 'migStart'
 * does the initial book-keeping on the original client, 'migEnd' does
 * the final book-keeping on the new client, and 'srvMigrate' is called
 * on the I/O server to shift around state associated with the client.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsMigrate.h"
#include "fsStream.h"
#include "fsPdev.h"
#include "fsFile.h"
#include "fsDevice.h"
#include "fsSpriteDomain.h"
#include "fsPrefix.h"
#include "fsOpTable.h"
#include "fsDebug.h"
#include "mem.h"
#include "rpc.h"

Boolean fsMigDebug = TRUE;
#define DEBUG( format ) \
	if (fsMigDebug) { Sys_Printf format ; }

/*
 * The following record defines what parameters the I/O server returns
 * after being told about a migration.  (Note, it will also return
 * data that is specific to the type of the I/O handle.)
 */
typedef struct MigrateReply {
    int flags;		/* New stream flags, the FS_RMT_SHARED bit is modified*/
    int offset;		/* New stream offset */
} MigrateReply;


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_EncapStream --
 *
 *	Package up a stream's state for migration to another host.  This
 *	copies the stream's offset, streamID, ioFileID, nameFileID, and flags.
 *	The server of the file does not perceive the migration at this
 *	time; only local book-keeping is done.  A future call to Fs_Deencap...
 *	will result in an RPC to the server to 'move' the client.
 *	It is reasonable to call Fs_DeencapStream again on this host,
 *	for example, to back out an aborted migration.
 *
 * Results:
 *	FS_REMOTE_OP_INVALID if the domain of the file does not support 
 *	migration.
 *	
 *
 * Side effects:
 *	One reference to the stream is removed, and when the last one
 *	goes away the stream itself is removed.  The migStart procedure
 *	for the I/O handle will do some local book-keeping, although
 *	this is side-effect free with respect to the book-keeping on
 *	the server.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fs_EncapStream(streamPtr, bufPtr)
    Fs_Stream	*streamPtr;	/* Stream to be migrated */
    Address	bufPtr;		/* Buffer to hold encapsulated stream */
{
    register	FsMigInfo	*migInfoPtr;
    ReturnStatus		status = SUCCESS;
    register FsHandleHeader	*ioHandlePtr;

    /*
     * Synchronize with stream duplication.  Then set the shared flag
     * so the I/O handle routines know whether or not to release references;
     * if the stream is still shared they shouldn't clean up refs.
     */
    FsHandleLock(streamPtr);
    if (streamPtr->hdr.refCount <= 1) {
	DEBUG( ("Encap stream %d, last ref", streamPtr->hdr.fileID.minor) );
	streamPtr->flags &= ~FS_RMT_SHARED;
    } else {
	DEBUG( ("Encap stream %d, shared", streamPtr->hdr.fileID.minor) );
	streamPtr->flags |= FS_RMT_SHARED;
    }

    /*
     * The encapsulated stream state includes the read/write offset,
     * the I/O server, the useFlags of the stream, and our SpriteID so
     * the target of migration and the server can do the right thing later. 
     */
    migInfoPtr = (FsMigInfo *) bufPtr;
    ioHandlePtr = streamPtr->ioHandlePtr;
    migInfoPtr->streamID = streamPtr->hdr.fileID;
    migInfoPtr->ioFileID = ioHandlePtr->fileID;
    migInfoPtr->nameID = streamPtr->nameInfoPtr->fileID;
    migInfoPtr->rootID = streamPtr->nameInfoPtr->rootID;
    migInfoPtr->offset = streamPtr->offset;
    migInfoPtr->srcClientID = rpc_SpriteID;
    migInfoPtr->flags = streamPtr->flags;

    /*
     * Branch to a stream specific routine to encapsultate any extra state
     * associated with the I/O handle, and to do local book-keeping.
     */
    status = (*fsStreamOpTable[ioHandlePtr->fileID.type].migStart) (ioHandlePtr,
		    streamPtr->flags, rpc_SpriteID, migInfoPtr->data);

    /*
     * Clean up our reference to the stream.  We'll remove the stream from
     * the handle table entirely if this is the last reference.  Otherwise
     * we mark the stream as potentially being shared across the network.
     */
    if (streamPtr->hdr.refCount <= 1) {
	FsStreamDispose(streamPtr);
    } else {
	FsHandleRelease(streamPtr, TRUE);
    }
    DEBUG( (" status %x\n", status) );
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fs_DeencapStream --
 *
 *	Deencapsulate the stream that was packaged up on another machine
 *	and recreate the stream on this machine.  This uses two stream-type
 *	routines to complete the setup of the stream.  First, the
 *	srvMigrate routine is called to shift client references on the
 *	server.  Then the migEnd routine is called to do local book-keeping.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Calls the srvMigrate and migEnd stream-type procedures.  See these
 *	routines for further details.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fs_DeencapStream(bufPtr, streamPtrPtr)
    Address	bufPtr;		/* Encapsulated stream information. */
    Fs_Stream	**streamPtrPtr;	/* Where to return pointer to the new stream */
{
    register	Fs_Stream	*streamPtr;
    register	FsMigInfo	*migInfoPtr;
    register	FsNameInfo	*nameInfoPtr;
    ReturnStatus		status = SUCCESS;
    Boolean			found;
    int				size;
    ClientData			data;

    migInfoPtr = (FsMigInfo *) bufPtr;

    /*
     * Allocate and set up the stream.  We note if this is the first
     * occurence of the stream on this host so the server can
     * do the right thing.
     */

    streamPtr = FsStreamFind(&migInfoPtr->streamID, (FsHandleHeader *)NIL,
			     migInfoPtr->flags, &found);

    if (!found) {
	migInfoPtr->flags |= FS_NEW_STREAM;
	streamPtr->offset = migInfoPtr->offset;
	DEBUG( ("Deencap NEW stream %d, migOff %d, ",
		streamPtr->hdr.fileID.minor, migInfoPtr->offset) );
    } else {
	migInfoPtr->flags &= ~FS_NEW_STREAM;
	DEBUG( ("Deencap OLD stream %d, migOff %d, ",
		streamPtr->hdr.fileID.minor,
		migInfoPtr->offset, streamPtr->offset) );
    }
    if (streamPtr->nameInfoPtr == (FsNameInfo *)NIL) {
	/*
	 * Set up the nameInfo.  We sacrifice the name as it is only
	 * used in error messages.  The fileID is used with get/set attr.
	 * If this file is the current directory then rootID is passed
	 * to the server to trap "..", domainType is used to index the
	 * name lookup operation switch, and prefixPtr is used for
	 * efficient handling of lookup redirections.
	 */
	streamPtr->nameInfoPtr = nameInfoPtr = Mem_New(FsNameInfo);
	nameInfoPtr->fileID = migInfoPtr->nameID;
	nameInfoPtr->rootID = migInfoPtr->rootID;
	if (nameInfoPtr->fileID.serverID != rpc_SpriteID) {
	    nameInfoPtr->domainType = FS_REMOTE_SPRITE_DOMAIN;
	} else {
	    nameInfoPtr->domainType = FS_LOCAL_DOMAIN;
	}
	nameInfoPtr->prefixPtr = FsPrefixFromFileID(&migInfoPtr->rootID);
	if (nameInfoPtr->prefixPtr == (struct FsPrefix *)NIL) {
	    Sys_Panic(SYS_WARNING, "Didn't find prefix entry for <%d,%d,%d>\n",
		migInfoPtr->rootID.serverID,
		migInfoPtr->rootID.major, migInfoPtr->rootID.minor);
	}
	nameInfoPtr->name = (char *)NIL;
    }
    /*
     * Contact the I/O server to tell it that the client moved.  The I/O
     * server checks for cross-network stream sharing and sets the
     * FS_RMT_SHARED flag if it is shared.  It also looks at the
     * FS_NEW_STREAM flag which we've set/unset above.
     */
    status = (*fsStreamOpTable[migInfoPtr->ioFileID.type].migrate)
		(migInfoPtr, rpc_SpriteID, &streamPtr->flags,
		 &streamPtr->offset, &size, &data);
    DEBUG( (" Type %d <%d,%d> offset %d, ", migInfoPtr->ioFileID.type,
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor,
		streamPtr->offset) );

    FsHandleUnlock(streamPtr);
    if (status == SUCCESS && !found) {
	/*
	 * Complete the setup of the stream with local book-keeping.  The
	 * local book-keeping only needs to be done the first time the
	 * stream is created here because stream sharing is not reflected
	 * in the reference/use counts on the I/O handle.
	 */
	migInfoPtr->flags = streamPtr->flags;
	status = (*fsStreamOpTable[migInfoPtr->ioFileID.type].migEnd)
		(migInfoPtr, size, data, &streamPtr->ioHandlePtr);
	DEBUG( ("migEnd status %x\n", status) );
    } else {
	DEBUG( ("srvMigrate status %x\n", status) );
    }

    if (status == SUCCESS) {
	*streamPtrPtr = streamPtr;
    } else if (!found) {
	FsHandleLock(streamPtr);
	FsStreamDispose(streamPtr);
    } else {
	FsHandleRelease(streamPtr, FALSE);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsNotifyOfMigration --
 *
 *	Send an rpc to the server to inform it about a migration.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *      None.
 *	
 *----------------------------------------------------------------------
 */
ReturnStatus
FsNotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr, outSize, outData)
    FsMigInfo	*migInfoPtr;	/* Encapsulated information */
    int		*flagsPtr;	/* New flags, may have FS_RMT_SHARED bit set */
    int		*offsetPtr;	/* New stream offset */
    int		outSize;	/* Size of returned data, outData */
    Address	outData;	/* Returned data from server */
{
    register ReturnStatus	status;
    Rpc_Storage 	storage;
    MigrateReply	migReply;

    storage.requestParamPtr = (Address) migInfoPtr;
    storage.requestParamSize = sizeof(FsMigInfo);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address)&migReply;
    storage.replyParamSize = sizeof(MigrateReply);
    storage.replyDataPtr = outData;
    storage.replyDataSize = outSize;

    status = Rpc_Call(migInfoPtr->ioFileID.serverID, RPC_FS_START_MIGRATION,
		&storage);
    if (status == SUCCESS) {
	*flagsPtr = migReply.flags;
	*offsetPtr = migReply.offset;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcNofityOfMigration --
 * Fs_RpcStartMigration --
 *
 *	The service stub for FsNotifyOfMigration.
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
Fs_RpcStartMigration(srvToken, clientID, command, storagePtr)
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
    register FsMigInfo		*migInfoPtr;
    register FsHandleHeader	*hdrPtr;
    register ReturnStatus	status;
    register MigrateReply	*migReplyPtr;
    register Rpc_ReplyMem	*replyMemPtr;

    migInfoPtr = (FsMigInfo *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[migInfoPtr->ioFileID.type].clientVerify)
		(&migInfoPtr->ioFileID, migInfoPtr->srcClientID);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	Sys_Panic(SYS_WARNING, "Fs_RpcMigrate, unknown I/O handle <%d,%d,%d>\n",
	    migInfoPtr->ioFileID.type,
	    migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);
    migReplyPtr = Mem_New(MigrateReply);
    migReplyPtr->flags = migInfoPtr->flags;
    storagePtr->replyParamPtr = (Address)migReplyPtr;
    storagePtr->replyParamSize = sizeof(int);
    status = (*fsStreamOpTable[hdrPtr->fileID.type].migrate) (migInfoPtr,
		clientID, &migReplyPtr->flags, &migReplyPtr->offset,
		&storagePtr->replyDataSize, &storagePtr->replyDataPtr);
    FsHandleRelease(hdrPtr, FALSE);

    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcFinishMigration --
 *
 *	Server stub for RemoteFinishMigration.  NOT USED.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Call the client open routine to do cache consistency.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcFinishMigration(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* IGNORED */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    return(FAILURE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_GetEncapSize --
 *
 *	Return the size of the encapsulated stream.
 *
 * Results:
 *	The size of the migration information structure.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

int
Fs_GetEncapSize()
{
    return(sizeof(FsMigInfo));
}
