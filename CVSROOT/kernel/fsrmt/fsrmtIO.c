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

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsFile.h"
#include "fsSpriteDomain.h"
#include "fsNameOps.h"
#include "fsPrefix.h"
#include "fsBlockCache.h"
#include "fsConsist.h"
#include "fsOpTable.h"
#include "fsTrace.h"
#include "fsDebug.h"
#include "fsStream.h"
#include "fsStat.h"
#include "proc.h"
#include "rpc.h"
#include "vm.h"
#include "dbg.h"

void FsRpcCacheUnlockBlock();

Boolean fsRpcDebug = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * FsRemoteRead --
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
FsRemoteRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream	*streamPtr;		/* Stream to Remote I/O handle. */
    int		flags;			/* FS_CONSUME | FS_CLIENT_CACHE_READ */
    register Address buffer;		/* Where to read into. */
    int 	*offsetPtr;		/* In/Out Byte offset */
    int		*lenPtr;		/* Return,  bytes read. */
    Sync_RemoteWaiter *waitPtr;
{
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsRemoteReadParams	readParams;
    Proc_ControlBlock	*procPtr;
    int			length;
    int			offset;
    register Boolean	userSpace;
    int			amountRead;
    int			savedLen;
    register int	readLen;
    register Address	readBufferPtr;

    /*
     * Set up parameters that won't change in each loop iteration.
     */
    readParams.fileID = rmtHandlePtr->hdr.fileID;
    readParams.streamID = streamPtr->hdr.fileID;
    userSpace = flags & FS_USER;
    readParams.flags = flags & ~(FS_USER);
    if (waitPtr == (Sync_RemoteWaiter *)NIL) {
	readParams.waiter.hostID = -1;
	readParams.waiter.pid = -1;
    } else {
	readParams.waiter = *waitPtr;
    }
    procPtr = Proc_GetEffectiveProc();
    readParams.pid = procPtr->processID;
    readParams.familyID = procPtr->familyID;

    storage.requestParamPtr = (Address)&readParams;
    storage.requestParamSize = sizeof(readParams);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;

    length = *lenPtr;
    if (userSpace) {
	/*
	 * We suffer a malloc and copy cost here because we don't
	 * map the users buffer into the kernel and we can't rely
	 * on being able to address the user's context from the
	 * interrupt handler that receives the packet.
	 */
	readBufferPtr =  (Address)malloc((length > fsMaxRpcDataSize) ?
				    fsMaxRpcDataSize : length);
    }
    /*
     * Outer loop to chop reads into the largest pieces
     * supported by the RPC system.
     */
    offset = *offsetPtr;
    amountRead = 0;
    while (length > 0) {
	readLen = (length > fsMaxRpcDataSize) ? fsMaxRpcDataSize : length;
	savedLen = readLen;
	if (!userSpace) {
	    readBufferPtr = buffer;
	}

	readParams.offset = offset;
	readParams.length = readLen;
	storage.replyDataSize = readLen;
	storage.replyDataPtr = readBufferPtr;

	status = Rpc_Call(rmtHandlePtr->hdr.fileID.serverID, RPC_FS_READ,
			    &storage);
    
	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    readLen = storage.replyDataSize;
	    if (userSpace) {
		if (Vm_CopyOut(readLen, readBufferPtr, buffer) != SUCCESS) {
		    status = FS_INVALID_ARG;
		    break;
		}
	    }
	    length -= readLen;
	    buffer += readLen;
	    offset += readLen;
	    amountRead += readLen;
	    if (readLen < savedLen || status == FS_WOULD_BLOCK) {
		/*
		 * Quit on short read because may have hit eof or
		 * used up the data in a pipe or device.
		 */
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    FsWantRecovery((FsHandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    *lenPtr = amountRead;
    *offsetPtr = offset;
    FsStat_Add(amountRead, fsStats.gen.remoteBytesRead,
	       fsStats.gen.remoteReadOverflow);
    if (userSpace) {
	free(readBufferPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcRead --
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
 *	a reply message is returned to the caller.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcRead(srvToken, clientID, command, storagePtr)
    ClientData		srvToken;	/* Handle for the server process */
    int			clientID;	/* Sprite ID of client host */
    int			command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* Specifies the size and location of
					 * the two parts of the read request.
					 * This routine sets up parts that
					 * indicate the size and location of
					 * the read reply. */
{
    register FsRemoteReadParams	*paramsPtr;
    register FsHandleHeader	*hdrPtr;
    register Fs_Stream		*streamPtr;
    int			lengthRead;
    int			*offsetPtr;
    ReturnStatus	status;
    Rpc_ReplyMem	*replyMemPtr;	/* For call-back to free buffer */
    int			(*callBack)();	/* Call back to clean up after RPC */
    ClientData		clientData;	/* Client data for callBack */
    Proc_ControlBlock	*procPtr;
    Proc_PID		origFamilyID, origPID;

    paramsPtr = (FsRemoteReadParams *)storagePtr->requestParamPtr;

    /*
     * Fetch the handle for the file and verify the client.
     */
    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	printf( "Fs_RpcRead, stale handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);

    if (hdrPtr->fileID.type == FS_LCL_FILE_STREAM &&
	paramsPtr->length == FS_BLOCK_SIZE &&
	(paramsPtr->offset & FS_BLOCK_OFFSET_MASK) == 0) {
	/*
	 * This is a quick check to see if we can go to the cache
	 * directly.  This doesn't fit easily into the
	 * Stream read interface so it is left as a wart here instead of
	 * permeating the whole interface.
	 */
	FsCacheBlock	*cacheBlockPtr;	/* Direct reference to cache block */
	FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;

	status = FsCacheBlockRead(&handlePtr->cacheInfo,
				  paramsPtr->offset / FS_BLOCK_SIZE,
				  &cacheBlockPtr, &lengthRead, 
				  FS_DATA_CACHE_BLOCK, FALSE);
	if (cacheBlockPtr != (FsCacheBlock *)NIL) {
	    storagePtr->replyDataPtr = cacheBlockPtr->blockAddr;
	    callBack = (int(*)())FsRpcCacheUnlockBlock;
	    clientData = (ClientData)cacheBlockPtr;
	} else {
	    /*
	     * Either we are past eof or there was an I/O error.
	     * In the former case lengthRead is all set up,
	     * in the latter status is set up.
	     */
	    callBack = (int(*)())NIL;
	    clientData = (ClientData)NIL;
	}
    } else {
	/*
	 * Regular non-block aligned read to a file or device.
	 */
	register Address buffer = (Address) malloc(paramsPtr->length);
	lengthRead = paramsPtr->length;

	/*
	 * Set our familyID and pid to the ids in the parameters in case we are
	 * reading from a device that requires us to have the proper family ID.
	 */
	procPtr = Proc_GetCurrentProc();
	origFamilyID = procPtr->familyID;
	origPID = procPtr->processID;
	procPtr->familyID = paramsPtr->familyID;
	procPtr->processID = paramsPtr->pid;

	/*
	 * Latch ahold of the local stream in case it is shared and
	 * we need to use the offset in it, and then call the stream
	 * type read routine.
	 */
	streamPtr = FsStreamClientVerify(&paramsPtr->streamID, clientID);
	if (streamPtr == (Fs_Stream *)NIL) {
	    status = FS_STALE_HANDLE;
	} else {
	    FsHandleUnlock(streamPtr);
	    if (paramsPtr->flags & FS_RMT_SHARED) {
		offsetPtr = &streamPtr->offset;
	    } else {
		offsetPtr = &paramsPtr->offset;
	    }
	    status = (fsStreamOpTable[hdrPtr->fileID.type].read)(streamPtr,
			    paramsPtr->flags, buffer, offsetPtr,
			    &lengthRead, &paramsPtr->waiter);
	    FsHandleLock(streamPtr);
	    FsHandleRelease(streamPtr, TRUE);
	}

	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    storagePtr->replyDataPtr = buffer;
	    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = (Address) NIL;
	    replyMemPtr->dataPtr = buffer;
	    callBack = (int(*)())Rpc_FreeMem;
	    clientData = (ClientData)replyMemPtr;
	} else {
	    free(buffer);
	}
	procPtr->familyID = origFamilyID;
	procPtr->processID = origPID;
    }
    FsHandleRelease(hdrPtr, FALSE);
    FS_RPC_DEBUG_PRINT1("Fs_RpcRead: Returning %x\n", status);
    if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	storagePtr->replyDataSize = lengthRead;
	Rpc_Reply(srvToken, status, storagePtr, callBack, clientData);
	return(SUCCESS);
    } else {
	return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsRpcCacheUnlockBlock --
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
void
FsRpcCacheUnlockBlock(cacheBlockPtr)
    FsCacheBlock *cacheBlockPtr;
{
    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
}



/*----------------------------------------------------------------------
 *
 * FsRemoteWrite --
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
FsRemoteWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream	*streamPtr;		/* Open stream to a remote thing */
    int flags;				/* FS_LAST_DIRTY_BLOCK | FS_APPEND |
					 * FS_CLIENT_CACHE_WRITE |
					 * FS_WB_ON_LDB */
    Address  buffer;			/* Buffer to write bytes from */
    int      *offsetPtr;		/* Offset at which to write */
    int      *lenPtr;			/* In/Out byte count */
    Sync_RemoteWaiter *waitPtr;		/* process info for remote waiting */
{
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsRemoteWriteParams	writeParams;
    int			length;
    int			offset;
    int			amountWritten;	/* Total amount written */
    int			lenWritten;	/* Amount written during 1 RPC */
    register int	writeLen;	/* Amount to write each RPC */
    register Boolean	userSpace = flags & FS_USER;
    register Address	writeBufferPtr;

    /*
     * If we are in write-back-on-last-dirty-block mode then mark this
     * block specially.
     */
    if (fsWBOnLastDirtyBlock && (flags & FS_LAST_DIRTY_BLOCK)) {
	flags |= FS_WB_ON_LDB;
    }
    /*
     * Initialize things that won't change on each RPC.
     */
    writeParams.fileID = rmtHandlePtr->hdr.fileID;
    writeParams.streamID = streamPtr->hdr.fileID;
    writeParams.flags = flags & ~FS_USER;
    if (waitPtr == (Sync_RemoteWaiter *)NIL) {
	writeParams.waiter.hostID = -1;
	writeParams.waiter.pid = -1;
    } else {
	writeParams.waiter = *waitPtr;
    }

    storage.requestParamPtr = (Address)&writeParams;
    storage.requestParamSize = sizeof(writeParams);
    storage.replyParamPtr = (Address) &lenWritten;
    storage.replyParamSize = sizeof(lenWritten);
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    length = *lenPtr;
    if (userSpace) {
	/*
	 * We suffer a malloc and copy cost here because we don't
	 * map the users buffer into the kernel and we can't rely
	 * on being able to address the users context from the
	 * interrupt handler that receives the packet.
	 */
	writeBufferPtr =  (Address)malloc((length > fsMaxRpcDataSize) ?
			    fsMaxRpcDataSize : length);
    }
    /*
     * Outer loop to chop reads into the largest pieces
     * supported by the RPC system.
     */
    offset = *offsetPtr;
    amountWritten = 0;
    while (length > 0) {
	writeLen = (length > fsMaxRpcDataSize) ? fsMaxRpcDataSize : length;
	if (userSpace) {
	    if (Vm_CopyIn(writeLen, buffer, writeBufferPtr) != SUCCESS) {
		status = FS_INVALID_ARG;
		break;
	    }
	} else {
	    writeBufferPtr = buffer;
	}
	storage.requestDataPtr = writeBufferPtr;
	storage.requestDataSize = writeLen;
	writeParams.offset = offset;
	writeParams.length = writeLen;
    
	status = Rpc_Call(rmtHandlePtr->hdr.fileID.serverID, RPC_FS_WRITE,
		    &storage);
	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    register int shortWrite = (lenWritten < writeLen);
	    writeLen = lenWritten;
	    length -= writeLen;
	    buffer += writeLen;
	    offset += writeLen;
	    amountWritten += writeLen;
	    if (status == FS_WOULD_BLOCK || shortWrite) {
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    FsWantRecovery((FsHandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    *lenPtr = amountWritten;
    *offsetPtr += amountWritten;
    FsStat_Add(amountWritten, fsStats.gen.remoteBytesWritten,
	       fsStats.gen.remoteWriteOverflow);
    if (userSpace) {
	free(writeBufferPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcWrite --
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
Fs_RpcWrite(srvToken, clientID, command, storagePtr)
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
    register FsHandleHeader	 *hdrPtr;
    register FsRemoteWriteParams *paramsPtr;
    register Fs_Stream		*streamPtr;
    register int		*offsetPtr;
    Fs_Stream			dummyStream;
    int				 lengthWritten;
    ReturnStatus		 status;
    int				*lengthWrittenPtr;
    Rpc_ReplyMem		*replyMemPtr;

    paramsPtr = (FsRemoteWriteParams *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	printf( "Fs_RpcWrite, stale handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);

    paramsPtr->flags &= ~FS_USER;

    if (paramsPtr->flags & FS_CLIENT_CACHE_WRITE) {
	dummyStream.ioHandlePtr = hdrPtr;
	streamPtr = &dummyStream;
    } else {
	streamPtr = FsStreamClientVerify(&paramsPtr->streamID, clientID);
    }
    if (streamPtr == (Fs_Stream *)NIL) {
	status = FS_STALE_HANDLE;
    } else {
	if (streamPtr != &dummyStream) {
	    FsHandleUnlock(streamPtr);
	}
	if (paramsPtr->flags & FS_RMT_SHARED) {
	    offsetPtr = &streamPtr->offset;
	} else {
	    offsetPtr = &paramsPtr->offset;
	}
	lengthWritten = paramsPtr->length;
	FS_TRACE_IO(FS_TRACE_SRV_WRITE_2, hdrPtr->fileID, *offsetPtr,
		    lengthWritten );
	status = (fsStreamOpTable[hdrPtr->fileID.type].write)(streamPtr,
		    paramsPtr->flags, storagePtr->requestDataPtr,
		    offsetPtr, &lengthWritten, &paramsPtr->waiter);
	if (streamPtr != &dummyStream) {
	    FsHandleLock(streamPtr);
	    FsHandleRelease(streamPtr, TRUE);
	}
    }
    if (status == SUCCESS && (paramsPtr->flags & FS_LAST_DIRTY_BLOCK)) {
	/*
	 * This is done here because the regular file write routine doesn't
	 * know what client is doing the write.
	 */
	if (hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	    printf(
		    "Fs_RpcWrite, lastDirtyBlock flag on bad, #%d, stream\n",
		    hdrPtr->fileID.type);
	} else {
	    FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	    FS_TRACE_HANDLE(FS_TRACE_DEL_LAST_WR, hdrPtr);
	    FsDeleteLastWriter(&handlePtr->consist, clientID);
	    if (paramsPtr->flags & FS_WB_ON_LDB) {
		Boolean	blocksSkipped;
		/*
		 * Force out all data, indirect and descriptor blocks
		 * for this file.
		 */
		status = FsCacheFileWriteBack(&handlePtr->cacheInfo, 0, 
					FS_LAST_BLOCK,
					FS_FILE_WB_WAIT | FS_FILE_WB_INDIRECT,
				        &blocksSkipped);
		if (status != SUCCESS) {
		    printf("Fs_RpcWrite: write back <%d,%d> \"%s\" err <%x>\n",
			handlePtr->hdr.fileID.major,
			handlePtr->hdr.fileID.minor,
			FsHandleName(handlePtr), status);
		}
		status = FsWriteBackDesc(handlePtr, TRUE);
		if (status != SUCCESS) {
		    printf("Fs_RpcWrite: desc write <%d,%d> \"%s\" err <%x>\n",
			handlePtr->hdr.fileID.major,
			handlePtr->hdr.fileID.minor,
			FsHandleName(handlePtr), status);
		}
	    }
	}
    }
    FsHandleRelease(hdrPtr, FALSE);

    lengthWrittenPtr = (int *) malloc(sizeof(int));
    *(int *) lengthWrittenPtr = lengthWritten;
    storagePtr->replyParamPtr = (Address) lengthWrittenPtr;
    storagePtr->replyParamSize = sizeof(int);
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
 * FsRemoteSelect --
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
FsRemoteSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle from stream to select */
    Sync_RemoteWaiter	*waitPtr;	/* Information for remote waiting. */
    int      		*readPtr;	/* In/Out read ability */
    int      		*writePtr;	/* In/Out write ability */
    int      		*exceptPtr;	/* In/Out exception ability */
{
    ReturnStatus 		status;
    Rpc_Storage 		storage;
    FsRemoteSelectParams	selectParams;
    FsRemoteSelectResults	selectResults;

    FS_RPC_DEBUG_PRINT("FsRemoteSelect: Selecting file\n");

    selectParams.fileID = hdrPtr->fileID;
    selectParams.read = *readPtr;
    selectParams.write = *writePtr;
    selectParams.except = *exceptPtr;
    selectParams.waiter = *waitPtr;

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
	 * If the server is down the handle gets marked so that
	 * recovery will be done on it, and part of that will be
	 * to notify the handle's wait lists, which is were we may be.
	 */
	FsWantRecovery(hdrPtr);
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
 * Fs_RpcSelectStub --
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
Fs_RpcSelectStub(srvToken, clientID, command, storagePtr)
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
    register	FsHandleHeader		*hdrPtr;
    register	Rpc_ReplyMem		*replyMemPtr;
    ReturnStatus			status;

    FS_RPC_DEBUG_PRINT("RPC select request\n");

    paramsPtr = (FsRemoteSelectParams *)storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
	(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);
    status = (*fsStreamOpTable[paramsPtr->fileID.type].select)
	(hdrPtr, &paramsPtr->waiter, &paramsPtr->read,
	 &paramsPtr->write, &paramsPtr->except);
    FsHandleRelease(hdrPtr, FALSE);
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
 * FsRemoteIOControl --
 *
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
FsRemoteIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream	*streamPtr;
    int         command;
    int		byteOrder;
    Fs_Buffer   *inBufPtr;		/* Command inputs */
    Fs_Buffer   *outBufPtr;		/* Buffer for return parameters */
{
    register FsHandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    FsSpriteIOCParams		params;
    ReturnStatus		status;
    Rpc_Storage			storage;

    FS_RPC_DEBUG_PRINT("FsRemoteIOControl\n");
	
    params.fileID = hdrPtr->fileID;
    params.streamID = streamPtr->hdr.fileID;
    params.procID = (Proc_PID) NIL;
    params.familyID = (Proc_PID) NIL;
    params.command = command;
    params.inBufSize = inBufPtr->size;
    params.outBufSize = outBufPtr->size;
    params.byteOrder = byteOrder;

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsSpriteIOCParams);
    storage.requestDataPtr = (Address) inBufPtr->addr;
    storage.requestDataSize = inBufPtr->size;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)outBufPtr->addr;
    storage.replyDataSize = outBufPtr->size;

    status = Rpc_Call(hdrPtr->fileID.serverID, RPC_FS_IO_CONTROL, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	FsWantRecovery(hdrPtr);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcIOControl --
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
Fs_RpcIOControl(srvToken, clientID, command, storagePtr)
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
    register	FsSpriteIOCParams	*paramsPtr;
    register	FsHandleHeader		*hdrPtr;
    register	Fs_Stream		*streamPtr;
    register	Rpc_ReplyMem		*replyMemPtr;
    ReturnStatus			status;
    Address				outBufPtr;
    Fs_Buffer				outBuf;
    Fs_Buffer				inBuf;

    paramsPtr = (FsSpriteIOCParams *)storagePtr->requestParamPtr;

    streamPtr = FsStreamClientVerify(&paramsPtr->streamID, clientID);
    if (streamPtr == (Fs_Stream *)NIL) {
	printf( "Fs_RpcIOControl, no stream to %s <%d, %d>\n",
		FsFileTypeToString(paramsPtr->fileID.type),
		paramsPtr->fileID.major, paramsPtr->fileID.minor);
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(streamPtr, TRUE);

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    } else if (streamPtr->ioHandlePtr != hdrPtr) {
	    printf( "Fs_RpcIOControl: Stream/handle mis-match\n");
	    printf("Stream <%d, %d, %d> => %s I/O <%d, %d, %d>\n",
		paramsPtr->streamID.serverID, paramsPtr->streamID.major,
		paramsPtr->streamID.minor,
		FsFileTypeToString(paramsPtr->fileID.type),
		paramsPtr->fileID.serverID, paramsPtr->fileID.major,
		paramsPtr->fileID.minor);
	    FsHandleRelease(hdrPtr, TRUE);
	    return(FS_STALE_HANDLE);
    }
    
    if (paramsPtr->outBufSize != 0) {
	outBufPtr = (Address)malloc(paramsPtr->outBufSize);
    } else {
	outBufPtr = (Address)NIL;
    }
    FsHandleUnlock(hdrPtr);
    inBuf.addr = storagePtr->requestDataPtr;
    inBuf.size = paramsPtr->inBufSize;
    inBuf.flags = 0;
    outBuf.addr = outBufPtr;
    outBuf.size = paramsPtr->outBufSize;
    outBuf.flags = 0;
    status = (*fsStreamOpTable[hdrPtr->fileID.type].ioControl)(streamPtr,
	    paramsPtr->command, paramsPtr->byteOrder, &inBuf, &outBuf);
#ifdef lint
    status = FsFileIOControl(streamPtr,
	    paramsPtr->command, paramsPtr->byteOrder, &inBuf, &outBuf);
    status = FsPipeIOControl(streamPtr,
	    paramsPtr->command, paramsPtr->byteOrder, &inBuf, &outBuf);
    status = FsDeviceIOControl(streamPtr,
	    paramsPtr->command, paramsPtr->byteOrder, &inBuf, &outBuf);
#endif /* lint */
    FsHandleRelease(hdrPtr, FALSE);

    FS_RPC_DEBUG_PRINT1("Fs_RpcIOControl returns <%x>\n", status);

    if (status != SUCCESS || paramsPtr->outBufSize == 0) {
	if (paramsPtr->outBufSize != 0) {
	    free((Address) outBufPtr);
	}
	Rpc_Reply(srvToken, status, storagePtr,(int (*)())NIL, (ClientData)NIL);
    } else {
	storagePtr->replyDataPtr = outBufPtr;
	storagePtr->replyDataSize = paramsPtr->outBufSize;
	replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) NIL;
	replyMemPtr->dataPtr = outBufPtr;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, 
		  (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRemoteBlockCopy --
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
FsRemoteBlockCopy(srcHdrPtr, dstHdrPtr, blockNum)
    FsHandleHeader	*srcHdrPtr;	/* Source file handle. */
    FsHandleHeader	*dstHdrPtr;	/* Dest file handle. */
    int			blockNum;	/* Block to copy. */
{
    ReturnStatus 		status;
    FsRemoteBlockCopyParams	params;
    Rpc_Storage 		storage;
    FsRmtFileIOHandle		*srcHandlePtr;
    FsRmtFileIOHandle		*dstHandlePtr;

    if (srcHdrPtr->fileID.type != FS_RMT_FILE_STREAM) {
	panic( "FsRemoteBlockCopy, bad stream type <%d>\n",
	    srcHdrPtr->fileID.type);
    } else {
	srcHandlePtr = (FsRmtFileIOHandle *)srcHdrPtr;
	dstHandlePtr = (FsRmtFileIOHandle *)dstHdrPtr;
    }

    params.srcFileID = srcHdrPtr->fileID;
    params.destFileID = dstHdrPtr->fileID;
    params.blockNum = blockNum;
    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsRemoteBlockCopyParams);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(srcHdrPtr->fileID.serverID, RPC_FS_COPY_BLOCK, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	FsWantRecovery((FsHandleHeader *)srcHandlePtr);
	FsWantRecovery((FsHandleHeader *)dstHandlePtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcBlockCopy --
 *
 *	Service stub for FsRemoteBlockCopy.
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
Fs_RpcBlockCopy(srvToken, clientID, command, storagePtr)
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
    register	FsRemoteBlockCopyParams	*paramsPtr;
    register	FsHandleHeader		*srcHdrPtr;
    register	FsHandleHeader		*dstHdrPtr;
    ReturnStatus			status;

    FS_RPC_DEBUG_PRINT("RPC block copy request\n");

    paramsPtr = (FsRemoteBlockCopyParams *)storagePtr->requestParamPtr;

    /*
     * Fetch the source and dest handles.  We know that they won't go away
     * while we are using them because of the way swap files are handled.
     */
    dstHdrPtr = (*fsStreamOpTable[paramsPtr->destFileID.type].clientVerify)
		(&paramsPtr->destFileID, clientID, (int *)NIL);
    if (dstHdrPtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(dstHdrPtr, TRUE);

    srcHdrPtr = (*fsStreamOpTable[paramsPtr->srcFileID.type].clientVerify)
		(&paramsPtr->srcFileID, clientID, (int *)NIL);
    if (srcHdrPtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }

    status = (*fsStreamOpTable[dstHdrPtr->fileID.type].blockCopy)
		(srcHdrPtr, dstHdrPtr, paramsPtr->blockNum);
    FsHandleRelease(srcHdrPtr, TRUE);

    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRemoteDomainInfo --
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
FsRemoteDomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID		*fileIDPtr;
    Fs_DomainInfo	*domainInfoPtr;	
{
    register ReturnStatus	status;
    Rpc_Storage			storage;

    storage.requestParamPtr = (Address)&fileIDPtr->major;
    storage.requestParamSize = sizeof(int);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)domainInfoPtr;
    storage.replyParamSize = sizeof(Fs_DomainInfo);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_DOMAIN_INFO, &storage);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcDomainInfo --
 *
 *	Service stub for RPC_FS_DOMAIN_INFO.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_DOMAIN_UNAVAILABLE is
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the local io control routine.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcDomainInfo(srvToken, clientID, command, storagePtr)
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
    int			domainNumber;
    Fs_DomainInfo	*domainInfoPtr;

    domainNumber = *(int *)storagePtr->requestParamPtr;
    domainInfoPtr = (Fs_DomainInfo *)malloc(sizeof(Fs_DomainInfo));
    status = FsLocalDomainInfo(domainNumber, domainInfoPtr);
    if (status != SUCCESS) {
	free((Address)domainInfoPtr);
    } else {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyParamPtr = (Address) domainInfoPtr;
	storagePtr->replyParamSize = sizeof(Fs_DomainInfo);
	replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) domainInfoPtr;
	replyMemPtr->dataPtr = (Address) NIL;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, 
		  (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);
    }

    return(SUCCESS);
}
