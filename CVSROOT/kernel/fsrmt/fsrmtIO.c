/* 
 * fsSpriteIO.c --
 *
 *	This has the stubs for remote I/O operations handled by Sprite servers.
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

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsio.h>
#include <fsrmtInt.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fscache.h>
#include <fsconsist.h>
#include <fsioFile.h>
#include <fsStat.h>
#include <proc.h>
#include <rpc.h>
#include <vm.h>
#include <dbg.h>

int FsrmtRpcCacheUnlockBlock _ARGS_((ClientData clientData));

Boolean fsrmt_RpcDebug = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_Read --
 *
 *	Read data from a remote file/device/pipe/etc.
 *	This routine is in charge of breaking the request up into pieces
 *	that can be handled by the RPC system.
 *	Also, if the FS_USER flag is present then this will allocate
 *	a temporary buffer in the kernel to avoid addressing problems
 *	in the RPC interrupt handler.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the bufSize parameter.  *readCountPtr is filled with the number
 *	of bytes actually read.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_Read(streamPtr, readPtr, waitPtr, replyPtr)
    Fs_Stream	*streamPtr;		/* Stream to Remote I/O handle. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register Fsrmt_IOHandle *rmtHandlePtr =
	    (Fsrmt_IOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsrmtIOParam	readParams;
    register Boolean	userSpace;
    int			amountRead;
    register Address	readBufferPtr = (Address)NIL;

    status = SUCCESS;
    /*
     * Set up parameters that won't change in each loop iteration.
     */
    readParams.fileID = rmtHandlePtr->hdr.fileID;
    readParams.streamID = streamPtr->hdr.fileID;
    if (waitPtr == (Sync_RemoteWaiter *)NIL) {
	readParams.waiter.hostID = -1;
	readParams.waiter.pid = -1;
    } else {
	readParams.waiter = *waitPtr;
    }
    readParams.io.buffer = 0;	/* not used */
    userSpace = readPtr->flags & FS_USER;
    readParams.io.flags = readPtr->flags & ~FS_USER;
    readParams.io.procID = readPtr->procID;
    readParams.io.familyID = readPtr->familyID;
    readParams.io.uid = readPtr->uid;
    readParams.io.reserved = 0;

    storage.requestParamPtr = (Address)&readParams;
    storage.requestParamSize = sizeof(readParams);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    /*
     * Allow remote site to return a signal.
     */
    storage.replyParamPtr = (Address)replyPtr;
    storage.replyParamSize = sizeof(Fs_IOReply);

    if (userSpace) {
	/*
	 * We suffer a malloc and copy cost here because we don't
	 * map the users buffer into the kernel and we can't rely
	 * on being able to address the user's context from the
	 * interrupt handler that receives the packet.
	 */
	readBufferPtr =  (Address)malloc((readPtr->length > fsMaxRpcDataSize) ?
				    fsMaxRpcDataSize : readPtr->length);
    }
    /*
     * Outer loop to chop reads into the largest pieces
     * supported by the RPC system.
     */
    amountRead = 0;
    while (readPtr->length > 0) {
	readParams.io.length = (readPtr->length > fsMaxRpcDataSize) ?
				fsMaxRpcDataSize : readPtr->length;
	readParams.io.offset = readPtr->offset + amountRead;
	if (!userSpace) {
	    readBufferPtr = readPtr->buffer + amountRead;
	}
	storage.replyDataSize = readParams.io.length;
	storage.replyDataPtr = readBufferPtr;

	status = Rpc_Call(rmtHandlePtr->hdr.fileID.serverID, RPC_FS_READ,
			    &storage);

	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    if (userSpace) {
		if (Vm_CopyOut(storage.replyDataSize, readBufferPtr,
				readPtr->buffer + amountRead)
				!= SUCCESS) {
		    status = FS_INVALID_ARG;
		    break;
		}
	    }
	    readPtr->length -= storage.replyDataSize;
	    amountRead += storage.replyDataSize;
	    if (storage.replyDataSize < readParams.io.length ||
		status == FS_WOULD_BLOCK) {
		/*
		 * Quit on short read because may have hit eof or
		 * used up the data in a pipe or device.
		 */
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    Fsutil_WantRecovery((Fs_HandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    replyPtr->length = amountRead;
    Fs_StatAdd(amountRead, fs_Stats.gen.remoteBytesRead,
	       fs_Stats.gen.remoteReadOverflow);
    if (userSpace) {
	free(readBufferPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcRead --
 *
 *	Service stub for the RPC_FS_READ call.  This verifies the client
 *	and uses the stream read routine to get data.
 *	There is also an optimization here to read directly
 *	out of the cache for block-aligned cacheable reads.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *      If the read would block (ie, no data ready), the remote caller is
 *      put into the file handle's readWaitList.  If the read went ok
 *	a reply message is returned to the caller.  Note that we may
 *	or may not return an Fs_IOReply.  This is not done with a cache
 *	read because signals are not generated in that case, which is
 *	why the Fs_IOReply struct is used.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcRead(srvToken, clientID, command, storagePtr)
    ClientData		srvToken;	/* Handle for the server process */
    int			clientID;	/* Sprite ID of client host */
    int			command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* Specifies the size and location of
					 * the two parts of the read request.
					 * This routine sets up parts that
					 * indicate the size and location of
					 * the read reply. */
{
    register FsrmtIOParam	*paramsPtr;
    register Fs_HandleHeader	*hdrPtr;
    register Fs_Stream		*streamPtr;
    ReturnStatus	status;
    Rpc_ReplyMem	*replyMemPtr;	/* For call-back to free buffer */
    int			(*callBack) _ARGS_((ClientData));
				/* Call back to clean up after RPC */
    ClientData		clientData;	/* Client data for callBack */

    callBack = (int(*)()) NIL;
    clientData = (ClientData) NIL;

    paramsPtr = (FsrmtIOParam *)storagePtr->requestParamPtr;

    /*
     * Fetch the handle for the file and verify the client.
     */
    hdrPtr = (*fsio_StreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	printf("Fsrmt_RpcRead, no handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleUnlock(hdrPtr);

    /*
     * Fetch the shadow stream in case we need to use our offset.
     */
    if (paramsPtr->streamID.type == FSIO_STREAM &&
	paramsPtr->streamID.serverID == rpc_SpriteID) {
	streamPtr = Fsio_StreamClientVerify(&paramsPtr->streamID, hdrPtr,
		    clientID);
	if (streamPtr == (Fs_Stream *)NIL) {
	    printf("Fsrmt_RpcRead no stream <%d> to handle <%d,%d> client %d\n",
		    paramsPtr->streamID.minor,
		    paramsPtr->fileID.major, paramsPtr->fileID.minor,
		    clientID);
	    Fsutil_HandleRelease(hdrPtr, FALSE);
	    return( (paramsPtr->streamID.minor < 0) ? GEN_INVALID_ARG
						    : FS_STALE_HANDLE );
	} else {
	    if (paramsPtr->io.flags & FS_RMT_SHARED) {
		paramsPtr->io.offset = streamPtr->offset;
	    }
	    Fsutil_HandleUnlock(streamPtr);
	}
    } else {
	/*
	 * Read from the cache, no stream available.
	 */
	streamPtr = (Fs_Stream *)NIL;
    }

/* This code can be removed after SOSP.  -Mary 10/1/91 */
    paramsPtr->io.reserved = clientID;
/* End of code to remove. */
    if (hdrPtr->fileID.type == FSIO_LCL_FILE_STREAM &&
	paramsPtr->io.length == FS_BLOCK_SIZE &&
	(paramsPtr->io.offset & FS_BLOCK_OFFSET_MASK) == 0) {
	/*
	 * This is a quick check to see if we can go to the cache
	 * directly.  This doesn't fit easily into the
	 * Stream read interface so it is left as a wart here instead of
	 * permeating the whole interface.
	 */
	Fscache_Block	*cacheBlockPtr;	/* Direct reference to cache block */
	Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
	int lengthRead = 0;
	status = Fscache_BlockRead(&handlePtr->cacheInfo,
				  paramsPtr->io.offset / FS_BLOCK_SIZE,
				  &cacheBlockPtr, &lengthRead, 
				  FSCACHE_DATA_BLOCK, FALSE);
	if (cacheBlockPtr != (Fscache_Block *)NIL) {
	    storagePtr->replyDataPtr = cacheBlockPtr->blockAddr;
	    storagePtr->replyDataSize = lengthRead;
	    callBack = FsrmtRpcCacheUnlockBlock;
	    clientData = (ClientData)cacheBlockPtr;
	} else {
	    /*
	     * Either we are past eof or there was an I/O error.
	     * No data to return.
	     */
	    callBack = (int(*)())NIL;
	    clientData = (ClientData)NIL;
	}
	if (streamPtr != (Fs_Stream *)NIL) {
	    streamPtr->offset = paramsPtr->io.offset + lengthRead;
	    Fsutil_HandleLock(streamPtr);
	    Fsutil_HandleRelease(streamPtr, TRUE);
	}
    } else {
	/*
	 * Regular read to a file, device, pipe, pseudo-device.
	 */
	Fs_IOReply *replyPtr = mnew(Fs_IOReply);

	replyPtr->length = 0;
	replyPtr->signal = 0;
	replyPtr->flags = 0;
	paramsPtr->io.buffer = (Address) malloc(paramsPtr->io.length);

	if (streamPtr == (Fs_Stream *)NIL) {
	    printf("Fsrmt_RpcRead, non block-aligned cache read from client %d\n",
		clientID);
	    status = GEN_INVALID_ARG;
	} else {
	    status = (fsio_StreamOpTable[hdrPtr->fileID.type].read)(streamPtr,
			    &paramsPtr->io, &paramsPtr->waiter, replyPtr);
	    streamPtr->offset = paramsPtr->io.offset + replyPtr->length;
	    Fsutil_HandleLock(streamPtr);
	    Fsutil_HandleRelease(streamPtr, TRUE);
	}

	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    storagePtr->replyDataPtr = paramsPtr->io.buffer;
	    storagePtr->replyDataSize = replyPtr->length;
	    storagePtr->replyParamPtr = (Address)replyPtr;
	    storagePtr->replyParamSize = sizeof(Fs_IOReply);
	    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
	    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
	    callBack = Rpc_FreeMem;
	    clientData = (ClientData)replyMemPtr;
	} else {
	    free(paramsPtr->io.buffer);
	    free((Address)replyPtr);
	}
    }
    Fsutil_HandleRelease(hdrPtr, FALSE);
    FSRMT_RPC_DEBUG_PRINT1("Fsrmt_RpcRead: Returning %x\n", status);
    if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	Rpc_Reply(srvToken, status, storagePtr, callBack, clientData);
	return(SUCCESS);
    } else {
	return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtRpcCacheUnlockBlock --
 *
 *	A call-back to release a cache block after a successful read RPC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unlock the cache block locked down during a read for a remote client.
 *
 *----------------------------------------------------------------------
 */
int
FsrmtRpcCacheUnlockBlock(clientData)
    ClientData clientData;
{
    Fscache_Block *cacheBlockPtr = (Fscache_Block *) clientData;
    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
    return 0;
}



/*----------------------------------------------------------------------
 *
 * Fsrmt_Write --
 *
 *      Write to a remote Sprite file, device, or pipe.  This is in charge
 *	of breaking the write up into pieces that the RPC system can handle.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The write to the remote file.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsrmt_Write(streamPtr, writePtr, waitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to a remote thing */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register Fsrmt_IOHandle *rmtHandlePtr =
	    (Fsrmt_IOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status = SUCCESS;
    Rpc_Storage 	storage;
    FsrmtIOParam	writeParams;
    int			amountWritten;	/* Total amount written */
    register int	writeLen;	/* Amount to write each RPC */
    register Boolean	userSpace = writePtr->flags & FS_USER;
    register Address	writeBufferPtr = (Address) NIL;

    /*
     * Initialize things that won't change on each RPC.
     */
    writeParams.fileID = rmtHandlePtr->hdr.fileID;
    writeParams.streamID = streamPtr->hdr.fileID;
    if (waitPtr == (Sync_RemoteWaiter *)NIL) {
	writeParams.waiter.hostID = -1;
	writeParams.waiter.pid = -1;
    } else {
	writeParams.waiter = *waitPtr;
    }

    writeParams.io.buffer = 0;	/* not used */
    writeParams.io.flags = writePtr->flags & ~FS_USER;
    writeParams.io.procID = writePtr->procID;
    writeParams.io.familyID = writePtr->familyID;
    writeParams.io.uid = writePtr->uid;
    writeParams.io.reserved = 0;

    storage.requestParamPtr = (Address)&writeParams;
    storage.requestParamSize = sizeof(writeParams);
    storage.replyParamPtr = (Address) replyPtr;
    storage.replyParamSize = sizeof(Fs_IOReply);
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    if (userSpace) {
	/*
	 * We suffer a malloc and copy cost here because we don't
	 * map the users buffer into the kernel and we can't rely
	 * on being able to address the users context from the
	 * interrupt handler that receives the packet.
	 */
	writeBufferPtr = (Address)malloc((writePtr->length > fsMaxRpcDataSize) ?
					 fsMaxRpcDataSize : writePtr->length);
    }
    /*
     * Outer loop to chop reads into the largest pieces
     * supported by the RPC system.
     */
    amountWritten = 0;
    while (writePtr->length > 0) {
	writeLen = (writePtr->length > fsMaxRpcDataSize) ?
		    fsMaxRpcDataSize : writePtr->length;
	if (userSpace) {
	    if (Vm_CopyIn(writeLen, writePtr->buffer + amountWritten,
		    writeBufferPtr) != SUCCESS) {
		status = FS_INVALID_ARG;
		break;
	    }
	} else {
	    writeBufferPtr = writePtr->buffer + amountWritten;
	}
	storage.requestDataPtr = writeBufferPtr;
	storage.requestDataSize = writeLen;
	writeParams.io.offset = writePtr->offset + amountWritten;
	writeParams.io.length = writeLen;
    
	status = Rpc_Call(rmtHandlePtr->hdr.fileID.serverID, RPC_FS_WRITE,
		    &storage);
	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    writePtr->length -= replyPtr->length;
	    amountWritten += replyPtr->length;
	    if (status == FS_WOULD_BLOCK || replyPtr->length < writeLen) {
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    Fsutil_WantRecovery((Fs_HandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    replyPtr->length = amountWritten;
    Fs_StatAdd(amountWritten, fs_Stats.gen.remoteBytesWritten,
	       fs_Stats.gen.remoteWriteOverflow);
    if (userSpace) {
	free(writeBufferPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcWrite --
 *
 *	Server stub for the FS_RPC_WRITE call.  This verifies the client
 *	and then calls the stream-type write routine.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to the
 *	client.  If the arguments are bad then FS_STALE_HANDLE is returned
 *	and the main level sends back an error reply.
 *
 * Side effects:
 *	The write on the stream.  See also the stream write routines.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcWrite(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
					 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    register Fs_HandleHeader	 *hdrPtr;
    register FsrmtIOParam	*paramsPtr;
    register Fs_Stream		*streamPtr;
    Fs_Stream			dummyStream;
    Fs_IOReply			*replyPtr;
    ReturnStatus		 status;
    Rpc_ReplyMem		*replyMemPtr;

    paramsPtr = (FsrmtIOParam *) storagePtr->requestParamPtr;

    hdrPtr = (*fsio_StreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	printf( "Fsrmt_RpcWrite, stale handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleUnlock(hdrPtr);

    replyPtr = mnew(Fs_IOReply);
    replyPtr->length = 0;
    replyPtr->flags = 0;
    replyPtr->signal = 0;

    paramsPtr->io.flags &= ~FS_USER;
    if (paramsPtr->io.flags & FS_CLIENT_CACHE_WRITE) {
	dummyStream.ioHandlePtr = hdrPtr;
	streamPtr = &dummyStream;
    } else {
	streamPtr = Fsio_StreamClientVerify(&paramsPtr->streamID, hdrPtr,
		    clientID);
	if (streamPtr == (Fs_Stream *)NIL) {
	    printf("Fsrmt_RpcWrite no stream <%d> to handle <%d,%d> client %d\n",
		    paramsPtr->streamID.minor,
		    paramsPtr->fileID.major, paramsPtr->fileID.minor,
		    clientID);
	    status = (paramsPtr->streamID.minor < 0) ? GEN_INVALID_ARG
						     : FS_STALE_HANDLE;
	    goto exit;
	} else {
	    if (paramsPtr->io.flags & FS_RMT_SHARED) {
		paramsPtr->io.offset = streamPtr->offset;
	    }
	    Fsutil_HandleUnlock(streamPtr);
	}
    }
/* This code can be removed after SOSP.  -Mary 10/1/91 */
    paramsPtr->io.reserved = clientID;
/* End of code to remove. */
    paramsPtr->io.buffer = storagePtr->requestDataPtr;
    status = (fsio_StreamOpTable[hdrPtr->fileID.type].write)(streamPtr,
		&paramsPtr->io, &paramsPtr->waiter, replyPtr);
    if (streamPtr != &dummyStream) {
	streamPtr->offset = paramsPtr->io.offset + replyPtr->length;
	Fsutil_HandleLock(streamPtr);
	Fsutil_HandleRelease(streamPtr, TRUE);
    }
    if (status == SUCCESS && (paramsPtr->io.flags & FS_LAST_DIRTY_BLOCK)) {
        /*
         * This is done here because the regular file write routine doesn't
         * know what client is doing the write.
         */
        if (hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
            printf("Fsrmt_RpcWrite, lastDirtyBlock flag on bad stream type (%d)\
n",
                    hdrPtr->fileID.type);
        } else {
            Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
            Fsconsist_DeleteLastWriter(&handlePtr->consist, clientID);
	}
    }
exit:
    Fsutil_HandleRelease(hdrPtr, FALSE);

    storagePtr->replyParamPtr = (Address)replyPtr;
    storagePtr->replyParamSize = sizeof(Fs_IOReply);
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);

    return(SUCCESS);
}

/*
 * Parameters for the file select RPC.
 */

typedef struct FsRemoteSelectParams {
    Fs_FileID	fileID;		/* File to be re-opened */
    int		read;		/* 1 or zero */
    int		write;		/* 1 or zero */
    int		except;		/* 1 or zero */
    Sync_RemoteWaiter waiter;	/* Process info for remote waiting */
} FsRemoteSelectParams;

typedef struct FsRemoteSelectResults {
    int		read;		/* 1 or zero */
    int		write;		/* 1 or zero */
    int		except;		/* 1 or zero */
} FsRemoteSelectResults;

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_Select --
 *
 *	Select on a remote file/device/pipe.  This does an RPC to the
 *	I/O server which invokes a stream-specific select routine.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_Select(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    Fs_HandleHeader	*hdrPtr;	/* Handle from stream to select */
    Sync_RemoteWaiter	*waitPtr;	/* Information for remote waiting. */
    int      		*readPtr;	/* In/Out read ability */
    int      		*writePtr;	/* In/Out write ability */
    int      		*exceptPtr;	/* In/Out exception ability */
{
    ReturnStatus 		status;
    Rpc_Storage 		storage;
    FsRemoteSelectParams	selectParams;
    FsRemoteSelectResults	selectResults;

    FSRMT_RPC_DEBUG_PRINT("Fsrmt_Select: Selecting file\n");

    selectParams.fileID = hdrPtr->fileID;
    selectParams.read = *readPtr;
    selectParams.write = *writePtr;
    selectParams.except = *exceptPtr;
    if (waitPtr == (Sync_RemoteWaiter *)NIL) {
	/*
	 * Indicate a polling select with a NIL hostID.
	 */
	selectParams.waiter.hostID = NIL;
    } else {
	selectParams.waiter = *waitPtr;
    }

    storage.requestParamPtr = (Address)&selectParams;
    storage.requestParamSize = sizeof(selectParams);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) &selectResults;
    storage.replyParamSize = sizeof(FsRemoteSelectResults);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(hdrPtr->fileID.serverID, RPC_FS_SELECT, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	/*
	 * Mask the error and leave the bits set in the request masks.
	 * This will cause the application to try a read or write
	 * of this stream and then it will learn something is amiss.
	 */
	Fsutil_WantRecovery(hdrPtr);
	status = SUCCESS;
    } else {
	*readPtr = selectResults.read;
	*writePtr = selectResults.write;
	*exceptPtr = selectResults.except;
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcSelectStub --
 *
 *	The service stub for RPC_FS_SELECT.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the domain-level select routine to attempt the lock operation.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcSelectStub(srvToken, clientID, command, storagePtr)
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
    register	FsRemoteSelectParams	*paramsPtr;
    register	FsRemoteSelectResults	*resultsPtr;
    register	Fs_HandleHeader		*hdrPtr;
    register	Rpc_ReplyMem		*replyMemPtr;
    register	Sync_RemoteWaiter	*waitPtr;
    ReturnStatus			status;

    FSRMT_RPC_DEBUG_PRINT("RPC select request\n");

    paramsPtr = (FsRemoteSelectParams *)storagePtr->requestParamPtr;

    hdrPtr = (*fsio_StreamOpTable[paramsPtr->fileID.type].clientVerify)
	(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleUnlock(hdrPtr);
    if (paramsPtr->waiter.hostID == NIL) {
	/*
	 * Indicate a polling select.
	 */
	waitPtr = (Sync_RemoteWaiter *)NIL;
    } else {
	waitPtr = &paramsPtr->waiter;
    }
    status = (*fsio_StreamOpTable[paramsPtr->fileID.type].select)
	(hdrPtr, waitPtr, &paramsPtr->read,
	 &paramsPtr->write, &paramsPtr->except);
    Fsutil_HandleRelease(hdrPtr, FALSE);
    if (status == SUCCESS) {
	resultsPtr = mnew(FsRemoteSelectResults);
	resultsPtr->read = paramsPtr->read;
	resultsPtr->write = paramsPtr->write;
	resultsPtr->except = paramsPtr->except;
	storagePtr->replyParamPtr = (Address) resultsPtr;
	storagePtr->replyParamSize = sizeof(FsRemoteSelectResults);
	replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) resultsPtr;
	replyMemPtr->dataPtr = (Address) NIL;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, 
		  (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);
    } else {
	Rpc_Reply(srvToken, status, storagePtr, 
		  (int (*)())NIL, (ClientData)NIL);
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_IOControl --
 *
 *	Client stub for RPC_FS_IOCONTROL.
 *	Do a special operation on a remote Sprite file/device/pipe/etc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream	*streamPtr;
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fs_HandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    FsrmtIOCParam		params;
    ReturnStatus		status;
    Rpc_Storage			storage;

    FSRMT_RPC_DEBUG_PRINT("Fsrmt_IOControl\n");

    params.fileID = hdrPtr->fileID;
    params.streamID = streamPtr->hdr.fileID;
    params.procID = ioctlPtr->procID;
    params.familyID = ioctlPtr->familyID;
    params.command = ioctlPtr->command;
    params.inBufSize = ioctlPtr->inBufSize;
    params.outBufSize = ioctlPtr->outBufSize;
    params.format = ioctlPtr->format;
    params.uid = ioctlPtr->uid;

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsrmtIOCParam);
    storage.requestDataPtr = (Address) ioctlPtr->inBuffer;
    storage.requestDataSize = ioctlPtr->inBufSize;
    storage.replyParamPtr = (Address)replyPtr;
    storage.replyParamSize = sizeof(Fs_IOReply);
    storage.replyDataPtr = (Address)ioctlPtr->outBuffer;
    storage.replyDataSize = ioctlPtr->outBufSize;

    status = Rpc_Call(hdrPtr->fileID.serverID, RPC_FS_IO_CONTROL, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	Fsutil_WantRecovery(hdrPtr);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcIOControl --
 *
 *	Service stub for RPC_FS_IOCONTROL.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the local io control routine.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcIOControl(srvToken, clientID, command, storagePtr)
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
    register	FsrmtIOCParam	*paramsPtr;
    register	Fs_HandleHeader		*hdrPtr;
    register	Fs_Stream		*streamPtr;
    register	Rpc_ReplyMem		*replyMemPtr;
    ReturnStatus			status = SUCCESS;
    Address				outBufPtr;
    Fs_IOCParam				ioctl;
    Fs_IOReply				*replyPtr;

    paramsPtr = (FsrmtIOCParam *)storagePtr->requestParamPtr;

    hdrPtr = (*fsio_StreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *)NIL) {
	printf("Fsrmt_RpcIOControl, no handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    streamPtr = Fsio_StreamClientVerify(&paramsPtr->streamID, hdrPtr, clientID);
    if (streamPtr == (Fs_Stream *)NIL) {
	printf("Fsrmt_RpcIOControl no stream <%d> to handle <%d,%d> client %d\n",
		paramsPtr->streamID.minor,
		paramsPtr->fileID.major, paramsPtr->fileID.minor,
		clientID);
	Fsutil_HandleRelease(hdrPtr, TRUE);
	return( (paramsPtr->streamID.minor < 0) ? GEN_INVALID_ARG
						: FS_STALE_HANDLE );
    }
    Fsutil_HandleUnlock(hdrPtr);

    if (paramsPtr->outBufSize != 0) {
	outBufPtr = (Address)malloc(paramsPtr->outBufSize);
    } else {
	outBufPtr = (Address)NIL;
    }
    ioctl.command = paramsPtr->command;
    ioctl.inBuffer = storagePtr->requestDataPtr;
    ioctl.inBufSize = paramsPtr->inBufSize;
    ioctl.outBuffer = outBufPtr;
    ioctl.outBufSize = paramsPtr->outBufSize;
    ioctl.flags = 0;	/* All buffers in kernel space */
    ioctl.format = paramsPtr->format;
    ioctl.procID = paramsPtr->procID;
    ioctl.familyID = paramsPtr->familyID;
    ioctl.uid = paramsPtr->uid;

    replyPtr = mnew(Fs_IOReply);
    replyPtr->length = paramsPtr->outBufSize;
    replyPtr->flags = 0;
    replyPtr->signal = 0;
    replyPtr->code = 0;

    /*
     * Update server's shadow stream offset for IOC_REPOSITION
     */
    if (ioctl.command == IOC_REPOSITION) {
	int newOffset = -1;
	register Ioc_RepositionArgs *iocArgsPtr = (Ioc_RepositionArgs *) NIL;
	Ioc_RepositionArgs	iocArgs;
	int size;
	int inSize;

	if ((ioctl.inBuffer == (Address)NIL) || 
	    (ioctl.inBufSize < sizeof(Ioc_RepositionArgs))) {
	    status = GEN_INVALID_ARG;
	} else if (ioctl.format != mach_Format) {
	    int fmtStatus;
	    size = sizeof(Ioc_RepositionArgs);
	    inSize = ioctl.inBufSize;
	    fmtStatus = Fmt_Convert("ww", ioctl.format, &inSize,
			    ioctl.inBuffer, mach_Format, &size,
			    (Address) &iocArgs);
	    if (fmtStatus != 0) {
		printf("Format of ioctl failed <0x%x>\n", fmtStatus);
		status = GEN_INVALID_ARG;
	    }
	    if (size != sizeof(Ioc_RepositionArgs)) {
		status = GEN_INVALID_ARG;
	    }
	    iocArgsPtr = &iocArgs;
	} else {
	    iocArgsPtr = (Ioc_RepositionArgs *)ioctl.inBuffer;
	}
	if (status == SUCCESS) {
	    switch(iocArgsPtr->base) {
		case IOC_BASE_ZERO:
		    newOffset = iocArgsPtr->offset;
		    break;
		case IOC_BASE_CURRENT:
		    newOffset = streamPtr->offset + iocArgsPtr->offset;
		    break;
		case IOC_BASE_EOF: {
		    Fs_Attributes attrs;
    
		    status = Fs_GetAttrStream(streamPtr, &attrs);
		    if (status != SUCCESS) {
			break;
		    }
		    if (streamPtr->nameInfoPtr != (Fs_NameInfo *) NIL) {
			size = attrs.size;
		    }
		    newOffset = size + iocArgsPtr->offset;
		    break;
		}
	    }
	    if (newOffset < 0) {
		status = GEN_INVALID_ARG;
	    } else {
		if (ioctl.outBufSize >= sizeof(int) &&
		    ioctl.outBuffer == (Address) NIL) {
		    if (ioctl.format != mach_Format) {
			int size = sizeof(int);
			int inSize = sizeof(int);
			int fmtStatus;
			fmtStatus = Fmt_Convert("w", mach_Format, &inSize,
						(Address) &newOffset,
						ioctl.format, 
						&size,
						(Address) ioctl.outBuffer);
			if (fmtStatus != 0) {
			    printf("Format of ioctl failed <0x%x>\n",
				   fmtStatus);
			    status = GEN_INVALID_ARG;
			}
			if (size != sizeof(int)) {
			    status = GEN_INVALID_ARG;
			}
		    } else {
			*(int *)ioctl.outBuffer = newOffset;
		    }
		}
		if (status == SUCCESS) {
		    streamPtr->offset = newOffset;
		}
	    }
	}
    }
    Fsutil_HandleRelease(streamPtr, TRUE);
    if (status == SUCCESS) {
	status = (*fsio_StreamOpTable[hdrPtr->fileID.type].ioControl)(streamPtr,
		    &ioctl, replyPtr);
#ifdef lint
	status = Fsio_FileIOControl(streamPtr, &ioctl, replyPtr);
	status = Fsio_PipeIOControl(streamPtr, &ioctl, replyPtr);
	status = Fsio_DeviceIOControl(streamPtr, &ioctl, replyPtr);
	status = FspdevPseudoStreamIOControl(streamPtr, &ioctl, replyPtr);
#endif /* lint */
    }
    Fsutil_HandleRelease(hdrPtr, FALSE);

    FSRMT_RPC_DEBUG_PRINT1("Fsrmt_RpcIOControl returns <%x>\n", status);

    if ((replyPtr->length == 0) && (outBufPtr != (Address)NIL)) {
	free((Address) outBufPtr);
	outBufPtr = (Address)NIL;
    }
    storagePtr->replyDataPtr = outBufPtr;
    storagePtr->replyDataSize = replyPtr->length;
    storagePtr->replyParamPtr = (Address)replyPtr;
    storagePtr->replyParamSize = sizeof(Fs_IOReply);

    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address)replyPtr;
    replyMemPtr->dataPtr = outBufPtr;
    Rpc_Reply(srvToken, status, storagePtr, 
	      (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_BlockCopy --
 *
 *	Copy the file system block from the source to the destination file.
 *	This only works for remote file handles as this is only used
 *	on remote swap files.
 *
 * Results:
 *	Return status from the rpc to the server.
 *
 * Side effects:
 *	The RPC does the block copy on the server.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsrmt_BlockCopy(srcHdrPtr, dstHdrPtr, blockNum)
    Fs_HandleHeader	*srcHdrPtr;	/* Source file handle. */
    Fs_HandleHeader	*dstHdrPtr;	/* Dest file handle. */
    int			blockNum;	/* Block to copy. */
{
    ReturnStatus 		status;
    FsrmtBlockCopyParam	params;
    Rpc_Storage 		storage;
    Fsrmt_FileIOHandle		*srcHandlePtr;
    Fsrmt_FileIOHandle		*dstHandlePtr;

    if (srcHdrPtr->fileID.type != FSIO_RMT_FILE_STREAM) {
	panic( "Fsrmt_BlockCopy, bad stream type <%d>\n",
	    srcHdrPtr->fileID.type);
	srcHandlePtr = (Fsrmt_FileIOHandle *) NIL;
	dstHandlePtr = (Fsrmt_FileIOHandle *) NIL;
	return(FAILURE);
    } else {
	srcHandlePtr = (Fsrmt_FileIOHandle *)srcHdrPtr;
	dstHandlePtr = (Fsrmt_FileIOHandle *)dstHdrPtr;
    }

    params.srcFileID = srcHdrPtr->fileID;
    params.destFileID = dstHdrPtr->fileID;
    params.blockNum = blockNum;
    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsrmtBlockCopyParam);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(srcHdrPtr->fileID.serverID, RPC_FS_COPY_BLOCK, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	Fsutil_WantRecovery((Fs_HandleHeader *)srcHandlePtr);
	Fsutil_WantRecovery((Fs_HandleHeader *)dstHandlePtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcBlockCopy --
 *
 *	Service stub for Fsrmt_BlockCopy.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the local io control routine.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcBlockCopy(srvToken, clientID, command, storagePtr)
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
    register	FsrmtBlockCopyParam	*paramsPtr;
    register	Fs_HandleHeader		*srcHdrPtr;
    register	Fs_HandleHeader		*dstHdrPtr;
    ReturnStatus			status;

    FSRMT_RPC_DEBUG_PRINT("RPC block copy request\n");

    paramsPtr = (FsrmtBlockCopyParam *)storagePtr->requestParamPtr;

    /*
     * Fetch the source and dest handles.  We know that they won't go away
     * while we are using them because of the way swap files are handled.
     */
    dstHdrPtr = (*fsio_StreamOpTable[paramsPtr->destFileID.type].clientVerify)
		(&paramsPtr->destFileID, clientID, (int *)NIL);
    if (dstHdrPtr == (Fs_HandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleRelease(dstHdrPtr, TRUE);

    srcHdrPtr = (*fsio_StreamOpTable[paramsPtr->srcFileID.type].clientVerify)
		(&paramsPtr->srcFileID, clientID, (int *)NIL);
    if (srcHdrPtr == (Fs_HandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }

    status = (*fsio_StreamOpTable[paramsPtr->srcFileID.type].blockCopy)
				(srcHdrPtr, dstHdrPtr, paramsPtr->blockNum);
    Fsutil_HandleRelease(srcHdrPtr, TRUE);

    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}

/*
 * The return values from the RPC_FS_DOMAIN_INFO call.
 * (The inputs are a fileID.)
 */
typedef struct FsDomainInfoResults {
    Fs_DomainInfo	domain;
    Fs_FileID		fileID;
} FsDomainInfoResults;
/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_DomainInfo --
 *
 *	Return information about the given domain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_DomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID		*fileIDPtr;
    Fs_DomainInfo	*domainInfoPtr;	
{
    register ReturnStatus	status;
    FsDomainInfoResults		results;
    Rpc_Storage			storage;

#ifdef notdef
retry:
#endif
    storage.requestParamPtr = (Address)fileIDPtr;
    storage.requestParamSize = sizeof(Fs_FileID);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)&results;
    storage.replyParamSize = sizeof(FsDomainInfoResults);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_DOMAIN_INFO, &storage);

    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	/*
	 * Wait for recovery here, instead of higher up in Fsutil_DomainInfo.
	 * This is because the server-side stub can't check against
	 * stale handle conditions, so it always calls Fsutil_DomainInfo.
	 * We don't want to ever wait for recovery on the server,
	 * so we do it here where we know we are a client.
	 */
	Fs_HandleHeader *hdrPtr;
	hdrPtr = Fsutil_HandleFetch(fileIDPtr);
	if (hdrPtr == (Fs_HandleHeader *)NIL) {
	    printf("Fsrmt_DomainInfo: Can't fetch <%d,%d,%d,%d>\n",
		    fileIDPtr->type, fileIDPtr->serverID,
		    fileIDPtr->major, fileIDPtr->minor);
	} else {
	    Fsutil_HandleUnlock(hdrPtr);
	    Fsutil_WantRecovery(hdrPtr);
#ifdef notdef
	    /*
	     * We don't wait for recovery because that hangs getwd(),
	     * which in turn hangs shell scripts.  We have marked the
	     * handle as needing recovery, however, so recovery will
	     * happen eventually.
	     */
	    printf("Fsrmt_DomainInfo: waiting for recovery <%d,%d> server %d\n",
		    fileIDPtr->major, fileIDPtr->minor, fileIDPtr->serverID);
	    status = Fsutil_WaitForRecovery(hdrPtr, status);
	    if (status == SUCCESS) {
		goto retry;
	    }
#endif
	}
    }
    if (status == SUCCESS) {
	*domainInfoPtr = results.domain;
	*fileIDPtr = results.fileID;
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcDomainInfo --
 *
 *	Service stub for RPC_FS_DOMAIN_INFO.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_DOMAIN_UNAVAILABLE is
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the top-level Fsutil_DomainInfo routine to get information
 *	about the domain.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcDomainInfo(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* IGNORED */
    int command;		/* IGNORED */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    ReturnStatus	status;
    Fs_FileID		*fileIDPtr;
    FsDomainInfoResults	*resultsPtr;
    Rpc_ReplyMem	*replyMemPtr;

    fileIDPtr = (Fs_FileID *)storagePtr->requestParamPtr;
    resultsPtr = mnew(FsDomainInfoResults);
    fileIDPtr->type = Fsio_MapRmtToLclType(fileIDPtr->type);
    resultsPtr->fileID = *fileIDPtr;

    status = Fsutil_DomainInfo(&resultsPtr->fileID, &resultsPtr->domain);

    storagePtr->replyParamPtr = (Address) resultsPtr;
    storagePtr->replyParamSize = sizeof(FsDomainInfoResults);
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address) resultsPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, 
	      (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);

    return(SUCCESS);	/* Because we've already replied */
}
