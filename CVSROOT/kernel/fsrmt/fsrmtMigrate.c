/*
 * fsRmtMigrate.c --
 *
 * Procedures to handle migrating open files between machines.  The basic
 * strategy is to first do some local book-keeping on the client we are
 * leaving, then ship state to the new client, then finally tell the
 * I/O server about it, and finish up with local book-keeping on the
 * new client.  There are three stream-type procedures used: 'migStart'
 * does the initial book-keeping on the original client, 'migEnd' does
 * the final book-keeping on the new client, and 'migrate' is called
 * on the I/O server to shift around state associated with the client.
 *
 * Copyright (C) 1985, 1988, 1989 Regents of the University of California
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


#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsconsist.h>
#include <fsrmtMigrate.h>
#include <fsio.h>
#include <fspdev.h>
#include <fsprefix.h>
#include <fsNameOps.h>
#include <byte.h>
#include <rpc.h>
#include <procMigrate.h>

#include <stdio.h>

#ifdef SOSP91
#include <sospRecord.h>
#endif

extern Boolean fsio_MigDebug;
#define DEBUG( format ) \
	if (fsio_MigDebug) { printf format ; }



/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_NotifyOfMigration --
 *
 *	This invokes the stream-specific migration routine on the I/O server.
 *	This is used by various RMT (remote) stream types.
 *
 * Results:
 *	A return status, plus new flags containing FS_RMT_SHARED bit,
 *	a new stream offset, plus some stream-type-specific data used
 *	when creating the I/O handle in the migEnd procedure.
 *
 * Side effects:
 *      None here, but bookkeeping is done at the I/O server.
 *	
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_NotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr, outSize, outData)
    Fsio_MigInfo	*migInfoPtr;	/* Encapsulated information */
    int		*flagsPtr;	/* New flags, may have FS_RMT_SHARED bit set */
    int		*offsetPtr;	/* New stream offset */
    int		outSize;	/* Size of returned data, outData */
    Address	outData;	/* Returned data from server */
{
    register ReturnStatus	status;
    Rpc_Storage 	storage;
    FsrmtMigParam		migParam;

    storage.requestParamPtr = (Address) migInfoPtr;
    storage.requestParamSize = sizeof(Fsio_MigInfo);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address)&migParam;
    storage.replyParamSize = sizeof(FsrmtMigParam);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(migInfoPtr->ioFileID.serverID, RPC_FS_MIGRATE, &storage);

    if (status == SUCCESS) {
	FsrmtMigrateReply	*migReplyPtr;

	migReplyPtr = &(migParam.migReply);
	*flagsPtr = migReplyPtr->flags;
	*offsetPtr = migReplyPtr->offset;
	if (migParam.dataSize > 0) {
	    if (outSize < migParam.dataSize) {
		panic("Fsrmt_NotifyOfMigration: too much data returned %d not %d\n",
			  migParam.dataSize, outSize);
		status = FAILURE;
	    } else {
		bcopy((Address)&migParam.data, outData, migParam.dataSize);
	    }
	}
    } else if (fsio_MigDebug) {
	printf("Fsrmt_NotifyOfMigration: status %x from remote migrate routine.\n",
		  status);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcMigrateStream --
 *
 *	The RPC service stub for Fsrmt_NotifyOfMigration.
 *	This invokes the Migrate routine for the I/O handle given in
 *	the encapsulated stream state.
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
Fsrmt_RpcMigrateStream(srvToken, clientID, command, storagePtr)
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
    register Fsio_MigInfo		*migInfoPtr;
    register Fs_HandleHeader	*hdrPtr;
    register ReturnStatus	status;
    register FsrmtMigrateReply	*migReplyPtr;
    register FsrmtMigParam		*migParamPtr;
    register Rpc_ReplyMem	*replyMemPtr;
    Address    			dataPtr;
    int				dataSize;

    migInfoPtr = (Fsio_MigInfo *) storagePtr->requestParamPtr;

    hdrPtr = (*fsio_StreamOpTable[migInfoPtr->ioFileID.type].clientVerify)
	    (&migInfoPtr->ioFileID, migInfoPtr->srcClientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	printf("Fsrmt_RpcMigrateStream, unknown %s handle <%d,%d>\n",
	    Fsutil_FileTypeToString(migInfoPtr->ioFileID.type),
	    migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleUnlock(hdrPtr);
    migParamPtr = mnew(FsrmtMigParam);
    migReplyPtr = &(migParamPtr->migReply);
    migReplyPtr->flags = migInfoPtr->flags;
    storagePtr->replyParamPtr = (Address)migParamPtr;
    storagePtr->replyParamSize = sizeof(FsrmtMigParam);
    storagePtr->replyDataPtr = (Address)NIL;
    storagePtr->replyDataSize = 0;
    status = (*fsio_StreamOpTable[hdrPtr->fileID.type].migrate) (migInfoPtr,
		clientID, &migReplyPtr->flags, &migReplyPtr->offset,
		&dataSize, &dataPtr);
    migParamPtr->dataSize = dataSize;
    if ((status == SUCCESS) && (dataSize > 0)) {
	if (dataSize <= sizeof(migParamPtr->data)) {
	    bcopy(dataPtr, (Address) &migParamPtr->data, dataSize);
	    free(dataPtr);
	} else {
	    panic("Fsrmt_RpcMigrateStream: migrate returned oversized buffer.\n");
	    return(FAILURE);
	}
    } 
#ifdef SOSP91
    {
	SOSP_ADD_MIGRATE_TRACE(migInfoPtr->srcClientID, clientID,
	    migInfoPtr->streamID, migInfoPtr->offset);
    }
#endif
    Fsutil_HandleRelease(hdrPtr, FALSE);

    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    return(SUCCESS);
}
