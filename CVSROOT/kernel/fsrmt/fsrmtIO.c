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
 * FsSpriteRead --
 *
 *	Read data from a remote file.  This routine is in charge of breaking
 *	the request up into pieces that can be handled by the RPC system.
 *	Also, if the FS_USER flag is present then this will allocate
 *	a temporary buffer in the kernel to avoid addressing problems
 *	by the RPC interrupt handler.
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
FsSpriteRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
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
    FsSpriteReadParams	readParams;
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
	readBufferPtr =  Mem_Alloc((length > fsMaxRpcDataSize) ?
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
    fsStats.gen.remoteBytesRead += amountRead;
    if (userSpace) {
	Mem_Free(readBufferPtr);
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
    register FsSpriteReadParams	*paramsPtr;
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

    paramsPtr = (FsSpriteReadParams *)storagePtr->requestParamPtr;

    /*
     * Fetch the handle for the file and verify the client.
     */
    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	Sys_Panic(SYS_WARNING, "Fs_RpcRead, stale handle <%d,%d> client %d\n",
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
	register Address buffer = (Address) Mem_Alloc(paramsPtr->length);
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
	    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = (Address) NIL;
	    replyMemPtr->dataPtr = buffer;
	    callBack = (int(*)())Rpc_FreeMem;
	    clientData = (ClientData)replyMemPtr;
	} else {
	    Mem_Free(buffer);
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
 * FsSpriteWrite --
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
FsSpriteWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream	*streamPtr;		/* Open stream to a remote thing */
    int flags;				/* FS_LAST_DIRTY_BLOCK | FS_APPEND |
					 * FS_CLIENT_CACHE_WRITE */
    Address  buffer;			/* Buffer to write bytes from */
    int      *offsetPtr;		/* Offset at which to write */
    int      *lenPtr;			/* In/Out byte count */
    Sync_RemoteWaiter *waitPtr;		/* process info for remote waiting */
{
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsSpriteWriteParams	writeParams;
    int			length;
    int			offset;
    int			amountWritten;	/* Total amount written */
    int			lenWritten;	/* Amount written during 1 RPC */
    register int	writeLen;	/* Amount to write each RPC */
    register Boolean	userSpace = flags & FS_USER;
    register Address	writeBufferPtr;

    /*
     * Initialize things that won't change on each RPC.
     */
    writeParams.fileID = rmtHandlePtr->hdr.fileID;
    writeParams.streamID = streamPtr->hdr.fileID;
    writeParams.flags = flags;
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
	writeBufferPtr =  Mem_Alloc((length > fsMaxRpcDataSize) ?
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
	    writeLen = lenWritten;
	    length -= writeLen;
	    buffer += writeLen;
	    offset += writeLen;
	    amountWritten += writeLen;
	    if (status == FS_WOULD_BLOCK) {
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
    fsStats.gen.remoteBytesWritten += amountWritten;
    if (userSpace) {
	Mem_Free(writeBufferPtr);
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
    register FsSpriteWriteParams *paramsPtr;
    register Fs_Stream		*streamPtr;
    register int		*offsetPtr;
    Fs_Stream			dummyStream;
    int				 lengthWritten;
    ReturnStatus		 status;
    int				*lengthWrittenPtr;
    Rpc_ReplyMem		*replyMemPtr;

    paramsPtr = (FsSpriteWriteParams *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	Sys_Panic(SYS_WARNING, "Fs_RpcWrite, stale handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);

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
	FS_TRACE_HANDLE(FS_TRACE_WRITE, hdrPtr);
	if (paramsPtr->flags & FS_RMT_SHARED) {
	    offsetPtr = &streamPtr->offset;
	} else {
	    offsetPtr = &paramsPtr->offset;
	}
	lengthWritten = paramsPtr->length;
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
	    Sys_Panic(SYS_WARNING,
		    "Fs_RpcWrite, lastDirtyBlock flag on bad, #%d, stream\n",
		    hdrPtr->fileID.type);
	} else {
	    FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	    FS_TRACE_HANDLE(FS_TRACE_4, hdrPtr);
	    FsDeleteLastWriter(&handlePtr->consist, clientID);
	}
    }
    FsHandleRelease(hdrPtr, FALSE);

    lengthWrittenPtr = (int *) Mem_Alloc(sizeof(int));
    *(int *) lengthWrittenPtr = lengthWritten;
    storagePtr->replyParamPtr = (Address) lengthWrittenPtr;
    storagePtr->replyParamSize = sizeof(int);
    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);

    return(SUCCESS);
}

/*
 * Parameters for the file select RPC.
 */

typedef struct FsSpriteSelectParams {
    FsFileID	fileID;		/* File to be re-opened */
    int		read;		/* 1 or zero */
    int		write;		/* 1 or zero */
    int		except;		/* 1 or zero */
    Sync_RemoteWaiter waiter;	/* Process info for remote waiting */
} FsSpriteSelectParams;

typedef struct FsSpriteSelectResults {
    int		read;		/* 1 or zero */
    int		write;		/* 1 or zero */
    int		except;		/* 1 or zero */
} FsSpriteSelectResults;

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteSelect --
 *
 *	Sprite Domain Select.  This routine just calls the file type 
 *	specific routine.
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
FsSpriteSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle from stream to select */
    Sync_RemoteWaiter	*waitPtr;	/* Information for remote waiting. */
    int      		*readPtr;	/* In/Out read ability */
    int      		*writePtr;	/* In/Out write ability */
    int      		*exceptPtr;	/* In/Out exception ability */
{
    ReturnStatus 		status;
    Rpc_Storage 		storage;
    FsSpriteSelectParams	selectParams;
    FsSpriteSelectResults	selectResults;

    FS_RPC_DEBUG_PRINT("FsSpriteSelect: Selecting file\n");

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
    storage.replyParamSize = sizeof(FsSpriteSelectResults);
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
 *	The service stub for the FsSpriteSelect.
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
    register	FsSpriteSelectParams	*paramsPtr;
    register	FsSpriteSelectResults	*resultsPtr;
    register	FsHandleHeader		*hdrPtr;
    register	Rpc_ReplyMem		*replyMemPtr;
    ReturnStatus			status;

    FS_RPC_DEBUG_PRINT("RPC select request\n");

    paramsPtr = (FsSpriteSelectParams *)storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
	(&paramsPtr->fileID, clientID);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    } 
    status = (*fsStreamOpTable[paramsPtr->fileID.type].select)
	(hdrPtr, &paramsPtr->waiter, &paramsPtr->read,
	 &paramsPtr->write, &paramsPtr->except);
    FsHandleRelease(hdrPtr, TRUE);
    if (status == SUCCESS) {
	resultsPtr = Mem_New(FsSpriteSelectResults);
	resultsPtr->read = paramsPtr->read;
	resultsPtr->write = paramsPtr->write;
	resultsPtr->except = paramsPtr->except;
	storagePtr->replyParamPtr = (Address) resultsPtr;
	storagePtr->replyParamSize = sizeof(FsSpriteSelectResults);
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
 *	Do a special operation on a remote Sprite file.
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
FsRemoteIOControl(hdrPtr, command, inBufSize, inBuffer, outBufSize, outBuffer)
    FsHandleHeader *hdrPtr;
    int         command;
    int         inBufSize;
    Address     inBuffer;
    int         outBufSize;
    Address     outBuffer;
{
    FsSpriteIOCParams	params;
    ReturnStatus	status;
    Rpc_Storage		storage;

    FS_RPC_DEBUG_PRINT("FsSpriteIOControl\n");
	
    params.fileID = hdrPtr->fileID;
    params.command = command;
    params.inBufSize = inBufSize;
    params.outBufSize = outBufSize;

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsSpriteIOCParams);
    storage.requestDataPtr = (Address) inBuffer;
    storage.requestDataSize = inBufSize;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)outBuffer;
    storage.replyDataSize = outBufSize;

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
 *	Service stub for FsSpriteIOControl.
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
    register	Rpc_ReplyMem		*replyMemPtr;
    ReturnStatus			status;
    Address				outBufPtr;

    paramsPtr = (FsSpriteIOCParams *)storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID);
    if (hdrPtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }

    if (paramsPtr->outBufSize != 0) {
	outBufPtr = Mem_Alloc(paramsPtr->outBufSize);
    }
    FsHandleUnlock(hdrPtr);
    status = (*fsStreamOpTable[hdrPtr->fileID.type].ioControl)(hdrPtr,
	    paramsPtr->command, paramsPtr->inBufSize,
	    storagePtr->requestDataPtr, paramsPtr->outBufSize, outBufPtr);
    FsHandleRelease(hdrPtr, FALSE);

    FS_RPC_DEBUG_PRINT1("Fs_RpcIOControl returns <%x>\n", status);

    if (status != SUCCESS || paramsPtr->outBufSize == 0) {
	if (paramsPtr->outBufSize != 0) {
	    Mem_Free((Address) outBufPtr);
	}
	Rpc_Reply(srvToken, status, storagePtr,(int (*)())NIL, (ClientData)NIL);
    } else {
	storagePtr->replyDataPtr = outBufPtr;
	storagePtr->replyDataSize = paramsPtr->outBufSize;
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
 * FsSpriteBlockCopy --
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
FsSpriteBlockCopy(srcHdrPtr, dstHdrPtr, blockNum)
    FsHandleHeader	*srcHdrPtr;	/* Source file handle. */
    FsHandleHeader	*dstHdrPtr;	/* Dest file handle. */
    int			blockNum;	/* Block to copy. */
{
    ReturnStatus 		status;
    FsSpriteBlockCopyParams	params;
    Rpc_Storage 		storage;
    FsRmtFileIOHandle		*srcHandlePtr;
    FsRmtFileIOHandle		*dstHandlePtr;

    if (srcHdrPtr->fileID.type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "FsSpriteBlockCopy, bad stream type <%d>\n",
	    srcHdrPtr->fileID.type);
    } else {
	srcHandlePtr = (FsRmtFileIOHandle *)srcHdrPtr;
	dstHandlePtr = (FsRmtFileIOHandle *)dstHdrPtr;
    }

    params.srcFileID = srcHdrPtr->fileID;
    params.destFileID = dstHdrPtr->fileID;
    params.blockNum = blockNum;
    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsSpriteBlockCopyParams);
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
 *	Service stub for FsSpriteBlockCopy.
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
    register	FsSpriteBlockCopyParams	*paramsPtr;
    register	FsHandleHeader		*srcHdrPtr;
    register	FsHandleHeader		*dstHdrPtr;
    ReturnStatus			status;

    FS_RPC_DEBUG_PRINT("RPC block copy request\n");

    paramsPtr = (FsSpriteBlockCopyParams *)storagePtr->requestParamPtr;

    /*
     * Fetch the source and dest handles.  We know that they won't go away
     * while we are using them because of the way swap files are handled.
     */
    dstHdrPtr = (*fsStreamOpTable[paramsPtr->destFileID.type].clientVerify)
		(&paramsPtr->destFileID, clientID);
    if (dstHdrPtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(dstHdrPtr, TRUE);

    srcHdrPtr = (*fsStreamOpTable[paramsPtr->srcFileID.type].clientVerify)
		(&paramsPtr->srcFileID, clientID);
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
 * FsSpriteDomainInfo --
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
FsSpriteDomainInfo(fileIDPtr, domainInfoPtr)
    FsFileID		*fileIDPtr;
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
    domainInfoPtr = (Fs_DomainInfo *)Mem_Alloc(sizeof(Fs_DomainInfo));
    status = FsLocalDomainInfo(domainNumber, domainInfoPtr);
    if (status != SUCCESS) {
	Mem_Free((Address)domainInfoPtr);
    } else {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyParamPtr = (Address) domainInfoPtr;
	storagePtr->replyParamSize = sizeof(Fs_DomainInfo);
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) domainInfoPtr;
	replyMemPtr->dataPtr = (Address) NIL;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, 
		  (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteBlockAllocate --
 *
 *	Sprite domain block allocator.
 *
 * Results:
 *	Just returns the file-relative block number that corresponds
 *	to the given byte offset.  What this could also do is manage
 *	blocks from an allotment handed out by the file server.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
FsSpriteBlockAllocate(streamPtr, offset, numBytes, blockAddrPtr, newBlockPtr)
    Fs_Stream	*streamPtr;	/* Stream of file to allocate for */
    int		offset;		/* Offset at which to allocate */
    int		numBytes;	/* Amount to allocate */
    int		*blockAddrPtr;	/* Last block allocated. */
    int		*newBlockPtr;	/* Always FALSE. */
{
    *newBlockPtr = FALSE;
    *blockAddrPtr = offset / FS_BLOCK_SIZE;
}

