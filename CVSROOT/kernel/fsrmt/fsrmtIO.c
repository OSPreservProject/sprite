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
FsRemoteRead(streamPtr, readPtr, waitPtr, replyPtr)
    Fs_Stream	*streamPtr;		/* Stream to Remote I/O handle. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsRemoteIOParam	readParams;
    register Boolean	userSpace;
    int			amountRead;
    register Address	readBufferPtr;

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
	    FsWantRecovery((FsHandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    replyPtr->length = amountRead;
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
 *	a reply message is returned to the caller.  Note that we may
 *	or may not return an Fs_IOReply.  This is not done with a cache
 *	read because signals are not generated in that case, which is
 *	why the Fs_IOReply struct is used.
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
    register FsRemoteIOParam	*paramsPtr;
    register FsHandleHeader	*hdrPtr;
    register Fs_Stream		*streamPtr;
    ReturnStatus	status;
    Rpc_ReplyMem	*replyMemPtr;	/* For call-back to free buffer */
    int			(*callBack)();	/* Call back to clean up after RPC */
    ClientData		clientData;	/* Client data for callBack */
#ifdef notdef
    Proc_ControlBlock	*procPtr;
    Proc_PID		origFamilyID, origPID;
#endif

    paramsPtr = (FsRemoteIOParam *)storagePtr->requestParamPtr;

    /*
     * Fetch the handle for the file and verify the client.
     */
    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	printf("Fs_RpcRead, no handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);

    if (hdrPtr->fileID.type == FS_LCL_FILE_STREAM &&
	paramsPtr->io.length == FS_BLOCK_SIZE &&
	(paramsPtr->io.offset & FS_BLOCK_OFFSET_MASK) == 0) {
	/*
	 * This is a quick check to see if we can go to the cache
	 * directly.  This doesn't fit easily into the
	 * Stream read interface so it is left as a wart here instead of
	 * permeating the whole interface.
	 */
	FsCacheBlock	*cacheBlockPtr;	/* Direct reference to cache block */
	FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	int lengthRead = 0;

	status = FsCacheBlockRead(&handlePtr->cacheInfo,
				  paramsPtr->io.offset / FS_BLOCK_SIZE,
				  &cacheBlockPtr, &lengthRead, 
				  FS_DATA_CACHE_BLOCK, FALSE);
	if (cacheBlockPtr != (FsCacheBlock *)NIL) {
	    storagePtr->replyDataPtr = cacheBlockPtr->blockAddr;
	    storagePtr->replyDataSize = lengthRead;
	    callBack = (int(*)())FsRpcCacheUnlockBlock;
	    clientData = (ClientData)cacheBlockPtr;
	} else {
	    /*
	     * Either we are past eof or there was an I/O error.
	     * No data to return.
	     */
	    callBack = (int(*)())NIL;
	    clientData = (ClientData)NIL;
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

#ifdef notdef
	/*
	 * Set our familyID and pid to the ids in the parameters in case we are
	 * reading from a device that requires us to have the proper family ID.
	 * (Shouldn't have to do this anymore cause of Fs_IOParam)
	 */
	procPtr = Proc_GetCurrentProc();
	origFamilyID = procPtr->familyID;
	origPID = procPtr->processID;
	procPtr->familyID = paramsPtr->io.familyID;
	procPtr->processID = paramsPtr->io.procID;
#endif

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
	    if (paramsPtr->io.flags & FS_RMT_SHARED) {
		paramsPtr->io.offset = streamPtr->offset;
	    }
	    status = (fsStreamOpTable[hdrPtr->fileID.type].read)(streamPtr,
			    &paramsPtr->io, &paramsPtr->waiter, replyPtr);
	    streamPtr->offset = paramsPtr->io.offset + replyPtr->length;
	    FsHandleLock(streamPtr);
	    FsHandleRelease(streamPtr, TRUE);
	}

	if (status == SUCCESS || status == FS_WOULD_BLOCK) {
	    storagePtr->replyDataPtr = paramsPtr->io.buffer;
	    storagePtr->replyDataSize = replyPtr->length;
	    storagePtr->replyParamPtr = (Address)replyPtr;
	    storagePtr->replyParamSize = sizeof(Fs_IOReply);
	    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
	    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
	    callBack = (int(*)())Rpc_FreeMem;
	    clientData = (ClientData)replyMemPtr;
	} else {
	    free(paramsPtr->io.buffer);
	    free((Address)replyPtr);
	}
#ifdef notdef
	procPtr->familyID = origFamilyID;
	procPtr->processID = origPID;
#endif
    }
    FsHandleRelease(hdrPtr, FALSE);
    FS_RPC_DEBUG_PRINT1("Fs_RpcRead: Returning %x\n", status);
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
FsRemoteWrite(streamPtr, writePtr, waitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to a remote thing */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsRemoteIOParam	writeParams;
    int			amountWritten;	/* Total amount written */
    register int	writeLen;	/* Amount to write each RPC */
    register Boolean	userSpace = writePtr->flags & FS_USER;
    register Address	writeBufferPtr;

    /*
     * If we are in write-back-on-last-dirty-block mode then mark this
     * block specially.
     */
    if (fsWBOnLastDirtyBlock && (writePtr->flags & FS_LAST_DIRTY_BLOCK)) {
	writePtr->flags |= FS_WB_ON_LDB;
    }
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
	    FsWantRecovery((FsHandleHeader *)rmtHandlePtr);
	    break;
	} else {
	    break;
	}
    }
    replyPtr->length = amountWritten;
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
    register FsRemoteIOParam	*paramsPtr;
    register Fs_Stream		*streamPtr;
    Fs_Stream			dummyStream;
    Fs_IOReply			*replyPtr;
    ReturnStatus		 status;
    Rpc_ReplyMem		*replyMemPtr;

    paramsPtr = (FsRemoteIOParam *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	printf( "Fs_RpcWrite, stale handle <%d,%d> client %d\n",
		paramsPtr->fileID.major, paramsPtr->fileID.minor, clientID);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);

    replyPtr = mnew(Fs_IOReply);
    replyPtr->length = 0;
    replyPtr->flags = 0;
    replyPtr->signal = 0;

    paramsPtr->io.flags &= ~FS_USER;
    if (paramsPtr->io.flags & FS_CLIENT_CACHE_WRITE) {
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
	if (paramsPtr->io.flags & FS_RMT_SHARED) {
	    paramsPtr->io.offset = streamPtr->offset;
	}
	FS_TRACE_IO(FS_TRACE_SRV_WRITE_2, hdrPtr->fileID, paramsPtr->io.offset,
		    paramsPtr->io.length );
	paramsPtr->io.buffer = storagePtr->requestDataPtr;
	status = (fsStreamOpTable[hdrPtr->fileID.type].write)(streamPtr,
		    &paramsPtr->io, &paramsPtr->waiter, replyPtr);
	if (streamPtr != &dummyStream) {
	    FsHandleLock(streamPtr);
	    FsHandleRelease(streamPtr, TRUE);
	}
    }
    if (status == SUCCESS && (paramsPtr->io.flags & FS_LAST_DIRTY_BLOCK)) {
	/*
	 * This is done here because the regular file write routine doesn't
	 * know what client is doing the write.
	 */
	if (hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	    printf("Fs_RpcWrite, lastDirtyBlock flag on bad stream type (%d)\n",
		    hdrPtr->fileID.type);
	} else {
	    FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	    FS_TRACE_HANDLE(FS_TRACE_DEL_LAST_WR, hdrPtr);
	    FsDeleteLastWriter(&handlePtr->consist, clientID);
	    if (paramsPtr->io.flags & FS_WB_ON_LDB) {
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
    register	Sync_RemoteWaiter	*waitPtr;
    ReturnStatus			status;

    FS_RPC_DEBUG_PRINT("RPC select request\n");

    paramsPtr = (FsRemoteSelectParams *)storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
	(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);
    if (paramsPtr->waiter.hostID == NIL) {
	/*
	 * Indicate a polling select.
	 */
	waitPtr = (Sync_RemoteWaiter *)NIL;
    } else {
	waitPtr = &paramsPtr->waiter;
    }
    status = (*fsStreamOpTable[paramsPtr->fileID.type].select)
	(hdrPtr, waitPtr, &paramsPtr->read,
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
FsRemoteIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream	*streamPtr;
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register FsHandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    FsSpriteIOCParams		params;
    ReturnStatus		status;
    Rpc_Storage			storage;

    FS_RPC_DEBUG_PRINT("FsRemoteIOControl\n");
	
    params.fileID = hdrPtr->fileID;
    params.streamID = streamPtr->hdr.fileID;
    params.procID = ioctlPtr->procID;
    params.familyID = ioctlPtr->familyID;
    params.command = ioctlPtr->command;
    params.inBufSize = ioctlPtr->inBufSize;
    params.outBufSize = ioctlPtr->outBufSize;
    params.byteOrder = ioctlPtr->byteOrder;
    params.uid = ioctlPtr->uid;

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsSpriteIOCParams);
    storage.requestDataPtr = (Address) ioctlPtr->inBuffer;
    storage.requestDataSize = ioctlPtr->inBufSize;
    storage.replyParamPtr = (Address)replyPtr;
    storage.replyParamSize = sizeof(Fs_IOReply);
    storage.replyDataPtr = (Address)ioctlPtr->outBuffer;
    storage.replyDataSize = ioctlPtr->outBufSize;

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
    Fs_IOCParam				ioctl;
    Fs_IOReply				*replyPtr;

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
    FsHandleUnlock(hdrPtr);

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
    ioctl.byteOrder = paramsPtr->byteOrder;
    ioctl.procID = paramsPtr->procID;
    ioctl.familyID = paramsPtr->familyID;
    ioctl.uid = paramsPtr->uid;

    replyPtr = mnew(Fs_IOReply);
    replyPtr->length = paramsPtr->outBufSize;
    replyPtr->flags = 0;
    replyPtr->signal = 0;
    replyPtr->code = 0;

    status = (*fsStreamOpTable[hdrPtr->fileID.type].ioControl)(streamPtr,
		&ioctl, replyPtr);
#ifdef lint
    status = FsFileIOControl(streamPtr, &ioctl, replyPtr);
    status = FsPipeIOControl(streamPtr, &ioctl, replyPtr);
    status = FsDeviceIOControl(streamPtr, &ioctl, replyPtr);
    status = FsPseudoStreamIOControl(streamPtr, &ioctl, replyPtr);
#endif /* lint */
    FsHandleRelease(hdrPtr, FALSE);

    FS_RPC_DEBUG_PRINT1("Fs_RpcIOControl returns <%x>\n", status);

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
	 * Wait for recovery here, instead of higher up in FsDomainInfo.
	 * This is because the server-side stub can't check against
	 * stale handle conditions, so it always calls FsDomainInfo.
	 * We don't want to ever wait for recovery on the server,
	 * so we do it here where we know we are a client.
	 */
	FsHandleHeader *hdrPtr;
	hdrPtr = FsHandleFetch(fileIDPtr);
	if (hdrPtr == (FsHandleHeader *)NIL) {
	    printf("FsRemoteDomainInfo: Can't fetch <%d,%d,%d,%d>\n",
		    fileIDPtr->type, fileIDPtr->serverID,
		    fileIDPtr->major, fileIDPtr->minor);
	} else {
	    FsHandleUnlock(hdrPtr);
	    FsWantRecovery(hdrPtr);
#ifdef notdef
	    /*
	     * We don't wait for recovery because that hangs getwd(),
	     * which in turn hangs shell scripts.  We have marked the
	     * handle as needing recovery, however, so recovery will
	     * happen eventually.
	     */
	    printf("FsRemoteDomainInfo: waiting for recovery <%d,%d> server %d\n",
		    fileIDPtr->major, fileIDPtr->minor, fileIDPtr->serverID);
	    status = FsWaitForRecovery(hdrPtr, status);
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
 *	Calls the top-level FsDomainInfo routine to get information
 *	about the domain.
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
    Fs_FileID		*fileIDPtr;
    FsDomainInfoResults	*resultsPtr;
    Rpc_ReplyMem	*replyMemPtr;

    fileIDPtr = (Fs_FileID *)storagePtr->requestParamPtr;
    resultsPtr = mnew(FsDomainInfoResults);
    fileIDPtr->type = FsMapRmtToLclType(fileIDPtr->type);
    resultsPtr->fileID = *fileIDPtr;

    status = FsDomainInfo(&resultsPtr->fileID, &resultsPtr->domain);

    storagePtr->replyParamPtr = (Address) resultsPtr;
    storagePtr->replyParamSize = sizeof(FsDomainInfoResults);
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address) resultsPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, 
	      (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);

    return(SUCCESS);	/* Because we've already replied */
}
