/*
 * fsStreamOps.c --
 *
 *	The has procedures for the following stream operations:
 *	Read, Write, IOControl, Close.  Select and attributes functions
 *	are in their own files.
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
#include "fsFile.h"
#include "fsStream.h"
#include "fsOpTable.h"
#include "fsBlockCache.h"
#include "fsTrace.h"
#include "fsStat.h"
#include "fsDisk.h"
#include "rpc.h"
#include "vm.h"

extern Boolean fsClientCaching;


/*
 *----------------------------------------------------------------------
 *
 * Fs_Read --
 *
 *	Read from a stream.  The main jobs of this routine are to
 *	set things up for (remote) waiting, and to branch out to
 *	the procedure that implements Read for the type of the stream.
 *	If the server is down, or the stream's handle has gone stale,
 *	this blocks the process while waiting for handle recovery.
 *	Also, the stream access position is maintained by this procedure,
 *	even though the read offset is an explicit argument.
 *
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the length parameter.  The in/out length parameter specifies
 *	the buffer size on input and is updated to reflect the number
 *	of bytes actually read.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_Read(streamPtr, buffer, offset, lenPtr)
    Fs_Stream 	*streamPtr;	/* Stream to read from. */
    Address 	buffer;		/* Where to read into. */
    int 	offset;		/* Where to start reading from. */
    int 	*lenPtr;	/* Contains number of bytes to read on input,
				   and is filled with number of bytes read. */
{
    register ReturnStatus 	status = SUCCESS;
    Sync_RemoteWaiter		remoteWaiter;
    Fs_IOParam			io;
    register Fs_IOParam 	*ioPtr = &io;
    Fs_IOReply			reply;
    int				streamType;
    register int		toRead;

    toRead = *lenPtr;
    *lenPtr = 0;
    if (sys_ShuttingDown) {
	return(FAILURE);
    } else if ((streamPtr->flags & FS_READ) == 0) {
	return(FS_NO_ACCESS);
    } else if (toRead == 0) {
	return(SUCCESS);
    } else if ((toRead < 0) || (offset < 0)) {
	return(GEN_INVALID_ARG);
    } else if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }
    streamType = streamPtr->ioHandlePtr->fileID.type;

    FsSetIOParam(ioPtr, buffer, toRead, offset, streamPtr->flags);
    reply.length = 0;
    reply.flags = 0;
    reply.signal = 0;
    reply.code = 0;

    /*
     * Outer loop to attempt the read and then block if no data is ready.
     * The loop terminates upon error or if any data is transferred.
     */
    remoteWaiter.hostID = rpc_SpriteID;
    while (TRUE) {
	Sync_GetWaitToken(&remoteWaiter.pid, &remoteWaiter.waitToken);

	status = (fsStreamOpTable[streamType].read) (streamPtr,
		    ioPtr, &remoteWaiter, &reply);
#ifdef lint
	status = FsFileRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsRmtFileRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsDeviceRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsPipeRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsControlRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsServerStreamRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsPseudoStreamRead(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsRemoteRead(streamPtr, ioPtr, &remoteWaiter, &reply);
#endif

	if (status == SUCCESS) {
	    break;
	} else if (status == FS_WOULD_BLOCK && 
	    (streamPtr->flags & FS_NON_BLOCKING) == 0) {
	    if (reply.length > 0) {
		/*
		 * Stream routine ought not do return FS_WOULD_BLOCK
		 * in this case, but we cover for it here.
		 */
		status = SUCCESS;
		break;
	    } else if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
		status = GEN_ABORTED_BY_SIGNAL;
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	           status == RPC_SERVICE_DISABLED)  {
	    status = FsWaitForRecovery(streamPtr->ioHandlePtr, status);
	    if (status != SUCCESS) {
		break;
	    }
	} else {
	    if (status == FS_WOULD_BLOCK && reply.length > 0) {
		/*
		 * Cannot return FS_WOULD_BLOCK if some data was read.
		 */
		status = SUCCESS;
	    }
	    break;
	}
	/*
	 * Restore the length parameter because it may have been set to
	 * zero when the read blocked.
	 */
	ioPtr->length = toRead;
    }
    /*
     * Cache the file offset for sequential access.
     */
    streamPtr->offset += reply.length;
    *lenPtr = reply.length;

    if (status == FS_BROKEN_PIPE) {
	Sig_Send(SIG_PIPE, 0, PROC_MY_PID, FALSE);
    } else if (reply.signal != 0) {
	Sig_Send(reply.signal, reply.code, PROC_MY_PID, FALSE);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_Write --
 *
 *	Write to a stream.  This sets up for (remote) waiting and then
 *	branches to the routine that implements writing for the stream.
 *	If the server is down, or the streams handle has gone stale,
 *	this will block the process while waiting for handle recovery.
 *	Finally, the stream access position of the stream is maintained
 *	even though the write offset is an explicit parameter.
 *
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	The data in the buffer is written to the file at the indicated offset.
 *	The in/out length parameter specifies the amount of data to write
 *	and is updated to reflect the number of bytes actually written.
 *	The stream offset field is updated to after the bytes written.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_Write(streamPtr, buffer, offset, lenPtr)
    Fs_Stream *streamPtr;		/* The stream to write to */
    Address buffer;			/* The buffer to fill in */
    int offset;				/* Where in the stream to write to */
    int *lenPtr;			/* In/Out byte count */
{
    register ReturnStatus 	status = SUCCESS;	/* I/O return status */
    Sync_RemoteWaiter	remoteWaiter;		/* Process info for waiting */
    Fs_IOParam		io;			/* Write parameter block */
    register Fs_IOParam *ioPtr = &io;
    Fs_IOReply		reply;			/* Return length, signal */
    int			toWrite;		/* Amount remaining to write.
						 * Keep our own copy because
						 * lower-levels may modify
						 * ioPtr->length */
    register int	amountWritten;		/* Amount transferred */
    int			streamType;		/* Type from I/O handle */

    toWrite = *lenPtr;
    amountWritten = 0;
    *lenPtr = 0;
    if (sys_ShuttingDown) {
	return(FAILURE);
    } else if ((streamPtr->flags & FS_WRITE) == 0) {
	return(FS_NO_ACCESS);
    } else if (toWrite == 0) {
	return(SUCCESS);
    } else if ((toWrite < 0) || (offset < 0)) {
	return(GEN_INVALID_ARG);
    } else if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }
    streamType = streamPtr->ioHandlePtr->fileID.type;

    FsSetIOParam(ioPtr, buffer, toWrite, offset, streamPtr->flags);
    reply.length = 0;
    reply.flags = 0;
    reply.signal = 0;
    reply.code = 0;

    remoteWaiter.hostID = rpc_SpriteID;

    FS_TRACE_IO(FS_TRACE_WRITE, streamPtr->ioHandlePtr->fileID, offset,toWrite);
    /*
     * Main write loop.  This handles partial writes, non-blocking streams,
     * and crash recovery.  This loop expects the stream write procedure to
     * return FS_WOULD_BLOCK if it transfers no data, and lets it return
     * either SUCCESS or FS_WOULD_BLOCK on partial writes.  SUCCESS with a
     * partial write makes this loop attempt another write immediately.
     * If a stream write procedure returns FS_WOULD_BLOCK is is required to
     * have put the remoteWaiter information on an appropriate wait list.
     * This loop ensures that a non-blocking stream returns SUCCESS if some
     * data is transferred, and FS_WOULD_BLOCK if none can be transferred now.
     */
    while (TRUE) {
	Sync_GetWaitToken(&remoteWaiter.pid, &remoteWaiter.waitToken);

	status = (fsStreamOpTable[streamType].write) (streamPtr, ioPtr,
						      &remoteWaiter, &reply);
#ifdef lint
	status = FsFileWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsRmtFileWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsDeviceWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsPipeWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsPseudoStreamWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
	status = FsRemoteWrite(streamPtr, ioPtr, &remoteWaiter, &reply);
#endif
	toWrite -= reply.length;
	amountWritten += reply.length;
	/*
	 * Reset pointers because stream-specific routine may have
	 * modified them arbitrarily.
	 */
	ioPtr->buffer = buffer + amountWritten;
	ioPtr->offset = offset + amountWritten;
	ioPtr->length = toWrite;
	if (status == SUCCESS) {
	    if ((toWrite == 0) || (streamPtr->flags & FS_NON_BLOCKING)) {
		break;
	    }
	} else if (status == FS_WOULD_BLOCK) {
	    if ((streamPtr->flags & FS_NON_BLOCKING) == 0) {
		if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
		    status = GEN_ABORTED_BY_SIGNAL;
		    break;
		}
	    } else {
		if (amountWritten > 0) {
		    status = SUCCESS;	/* non-blocking partial write */
		}
		break;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	           status == RPC_SERVICE_DISABLED)  {
	    status = FsWaitForRecovery(streamPtr->ioHandlePtr, status);
	    if (status != SUCCESS) {
		break;
	    }
	} else {
	    break;			/* stream error */
	}
    }
    /*
     * Return info about the transfer.
     */
    *lenPtr = amountWritten;
    streamPtr->offset += amountWritten;
    if (status == FS_BROKEN_PIPE) {
	Sig_Send(SIG_PIPE, 0, PROC_MY_PID, FALSE);
    } else if (reply.signal != 0) {
	Sig_Send(reply.signal, reply.code, PROC_MY_PID, FALSE);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PageRead --
 *
 *	Read in a virtual memory page.  This routine bypasses the cache.
 *
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	The page is filled with data read from the indicate offset.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_PageRead(streamPtr, pageAddr, offset, numBytes, pageType)
    Fs_Stream	*streamPtr;	/* Swap file stream. */
    Address	pageAddr;	/* Pointer to page. */
    int		offset;		/* Offset in file. */
    int		numBytes;	/* Number of bytes in page. */
    Fs_PageType	pageType;	/* CODE HEAP or SWAP */
{
    ReturnStatus		status = SUCCESS;

    if (streamPtr->ioHandlePtr->fileID.type == FS_LCL_FILE_STREAM) {
	/*
	 * Swap file pages live in the cache on the server so that swap
	 * file copies and writes won't have to access the disk.
	 * However, by calling Fs_CacheBlockUnneeded the swap file blocks are 
	 * assured of getting written to disk the next time that a file system 
	 * block is needed.
	 */
	register FsLocalFileIOHandle *handlePtr = 
		(FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
	int tmpNumBytes = numBytes;

	status = FsCacheRead(&handlePtr->cacheInfo, 0, pageAddr, offset, 
			     &tmpNumBytes, (Sync_RemoteWaiter *) NIL);
	if (status == SUCCESS) {
	    Fs_CacheBlocksUnneeded(streamPtr, offset, numBytes, FALSE);
	}
    } else {
	int	lastBlock;
	int	bytesRead;
	int	i, cacheFlags;
	int	streamType = streamPtr->ioHandlePtr->fileID.type;
	register FsRmtFileIOHandle *handlePtr = 
		(FsRmtFileIOHandle *)streamPtr->ioHandlePtr;
	Boolean retry;
	FsCacheBlock *blockPtr;
	Boolean found;

	lastBlock = (unsigned int) (offset + numBytes - 1) / FS_BLOCK_SIZE;

	if (pageType == FS_CODE_PAGE) {
	    cacheFlags = FS_CLEAR_READ_AHEAD | FS_BLOCK_UNNEEDED;
	} else {
	    cacheFlags = FS_CLEAR_READ_AHEAD;
	}
	blockPtr = (FsCacheBlock *)NIL;
	for (i = (unsigned int) offset / FS_BLOCK_SIZE; i <= lastBlock; i++) {
	    do {
		if (!(streamPtr->flags & FS_SWAP) && fsClientCaching) {
		    FsCacheFetchBlock(&handlePtr->cacheInfo, i,
			    FS_DATA_CACHE_BLOCK, &blockPtr, &found);
		    if (found) {
			bcopy(blockPtr->blockAddr, pageAddr, FS_BLOCK_SIZE);
			if (blockPtr->flags & FS_READ_AHEAD_BLOCK) {
			    fsStats.blockCache.readAheadHits++;
			}
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, cacheFlags);
			blockPtr = (FsCacheBlock *)NIL;
			offset += FS_BLOCK_SIZE;
			break;	/* do-while, go to next for loop iteration */
		    } else if (pageType == FS_CODE_PAGE) {
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
			blockPtr = (FsCacheBlock *)NIL;
		    }
		}

		retry = FALSE;
		bytesRead = FS_BLOCK_SIZE;
		status = (*fsStreamOpTable[streamType].blockRead)
			(streamPtr->ioHandlePtr, 0, pageAddr, &offset,
			 &bytesRead, (Sync_RemoteWaiter *)NIL);
		FS_TRACE_IO(FS_TRACE_READ, streamPtr->ioHandlePtr->fileID,
			    offset, bytesRead);
		if (status != SUCCESS) {
		    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
			status == RPC_SERVICE_DISABLED) {
			/*
			 * The server is down so we wait for it.  This blocks
			 * the user process doing the page fault.
			 */
			Net_HostPrint(streamPtr->ioHandlePtr->fileID.serverID,
				"Fs_PageRead waiting\n");
			status = FsWaitForRecovery(streamPtr->ioHandlePtr, 
				    status);
			if (status == SUCCESS) {
			    retry = TRUE;
			} else {
			    printf(
				"Fs_PageRead recovery failed <%x>\n", status);
			    if (blockPtr != (FsCacheBlock *)NIL) {
				FsCacheUnlockBlock(blockPtr, 0, -1, 0,
						   FS_DELETE_BLOCK);
			    }
			    return(status);
			}
		    } else if (status != SUCCESS) {
			    printf(
				"Fs_PageRead: Read failed <%x>\n", status);
			    if (blockPtr != (FsCacheBlock *)NIL) {
				FsCacheUnlockBlock(blockPtr, 0, -1, 0,
						   FS_DELETE_BLOCK);
			    }
			    return(status);
		    }
		} else if (bytesRead != FS_BLOCK_SIZE) {
		    printf(
			    "FsPageRead: Short read of length %d\n", bytesRead);
		    if (blockPtr != (FsCacheBlock *)NIL) {
			FsCacheUnlockBlock(blockPtr, 0, -1, 0,
					   FS_DELETE_BLOCK);
		    }
		    return(VM_SHORT_READ);
		}
		if (blockPtr != (FsCacheBlock *)NIL) {
		    if (retry) {
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
		    } else {
			/*
			 * We read the data into the page, now copy it into the
			 * cache since initialized heap pages live in the cache.
			 */
			bcopy(pageAddr, blockPtr->blockAddr, FS_BLOCK_SIZE);
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, 0);
		    }
		    blockPtr = (FsCacheBlock *)NIL;
		}
	    } while (retry);
	    pageAddr += FS_BLOCK_SIZE;
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_PageWrite --
 *
 *	Write out a virtual memory page.  The cache is by-passed if the
 *	swap file is remote.  Swap files are cached on their file servers.
 *
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	The data in the buffer is written to the file at the indicated offset.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_PageWrite(streamPtr, pageAddr, offset, numBytes)
    Fs_Stream	*streamPtr;	/* Swap file stream. */
    Address	pageAddr;	/* Pointer to page. */
    int		offset;		/* Offset in file. */
    int		numBytes;	/* Number of bytes in page. */
{
    ReturnStatus		status = SUCCESS;

    if (streamPtr->ioHandlePtr->fileID.type == FS_LCL_FILE_STREAM) {
	/*
	 * Swap file pages live in the cache on the server so that swap file
	 * copies and writes won't have to always access the disk directly.
	 * However, by calling Fs_CacheBlockUnneeded the swap file blocks are 
	 * assured of getting written to disk the next time that a file system 
	 * block is needed.
	 */
	register FsLocalFileIOHandle *handlePtr = 
		(FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
	int tmpNumBytes = numBytes;

	status = FsCacheWrite(&handlePtr->cacheInfo, 0, pageAddr, offset, 
			     &tmpNumBytes, (Sync_RemoteWaiter *) NIL);
	if (status == SUCCESS) {
	    Fs_CacheBlocksUnneeded(streamPtr, offset, numBytes, FALSE);
	}
    } else {
	/*
	 * Write past the cache.
	 */
	int		lastBlock;
	int		blockAddr;
	Boolean		newBlock;
	int		i;
	int		streamType = streamPtr->ioHandlePtr->fileID.type;

	lastBlock = (unsigned int) (offset + numBytes - 1) / FS_BLOCK_SIZE;
	for (i = (unsigned int) offset / FS_BLOCK_SIZE; i <= lastBlock; i++) {
	    (*fsStreamOpTable[streamType].allocate)(streamPtr->ioHandlePtr,
		    offset, FS_BLOCK_SIZE, &blockAddr, &newBlock);
	    if (blockAddr == FS_NIL_INDEX) {
		printf( "Fs_PageWrite: Block Alloc failed\n");
		status = FS_NO_DISK_SPACE;
		break;
	    }
	    status = (*fsStreamOpTable[streamType].blockWrite)
		    (streamPtr->ioHandlePtr, blockAddr, FS_BLOCK_SIZE,
			    pageAddr, 0);
	    if (status != SUCCESS) {
		printf( "Fs_PageWrite: Write failed\n");
		break;
	    }
	    pageAddr += FS_BLOCK_SIZE;
	    offset += FS_BLOCK_SIZE;
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_PageCopy --
 *
 *	Copy the file system blocks in the source swap file to the destination
 *	swap file.
 *
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	Appropriate blocks in the source file are copied to the dest file.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_PageCopy(srcStreamPtr, destStreamPtr, offset, numBytes)
    Fs_Stream	*srcStreamPtr;	/* File to copy blocks from. */
    Fs_Stream	*destStreamPtr;	/* File to copy blocks to. */
    int		offset;		/* Offset in file. */
    int		numBytes;	/* Number of bytes in page. */
{
    int				lastBlock;
    register	FsHandleHeader	*srcHdrPtr;
    register	FsHandleHeader	*destHdrPtr;
    ReturnStatus		status;
    int				i;
    Boolean			retry;

    srcHdrPtr = srcStreamPtr->ioHandlePtr;
    destHdrPtr = destStreamPtr->ioHandlePtr;
    lastBlock = (unsigned int) (offset + numBytes - 1) / FS_BLOCK_SIZE;

    /*
     * Copy all blocks in the page.
     */
    for (i = (unsigned int) offset / FS_BLOCK_SIZE; i <= lastBlock; i++) {
	do {
	    retry = FALSE;
	    status = (*fsStreamOpTable[srcHdrPtr->fileID.type].blockCopy)
				    (srcHdrPtr, destHdrPtr, i);
	    if (status != SUCCESS) {
		if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
		    status == RPC_SERVICE_DISABLED) {
		    /*
		     * The server is down so we wait for it.  This just blocks
		     * the user process doing the page fault.
		     */
		    Net_HostPrint(srcHdrPtr->fileID.serverID,
			    "Fs_PageCopy, waiting for server %d\n");
		    status = FsWaitForRecovery(srcStreamPtr->ioHandlePtr,
				status);
		    if (status == SUCCESS) {
			retry = TRUE;
		    } else {
			printf(
			    "Fs_PageCopy, recovery failed <%x>\n", status);
			return(status);
		    }
		} else {
		    printf(
			    "Fs_PageCopy: Copy failed <%x>\n", status);
		    return(status);
		}
	    }
	} while (retry);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_IOControl --
 *
 *	Generic IOControl handler.  This will also propogate ALL
 *	IOControls down to lower levels, mainly so that pseudo-device
 *	drivers (user-level server programs) will see all of them.
 *
 * Results:
 *	An error code that depends in the command
 *
 * Side effects:
 *	Command specific
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_IOControl(streamPtr, ioctlPtr, replyPtr)
    register Fs_Stream *streamPtr;
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register ReturnStatus	status;
    register Boolean		retry;
    register int		command = ioctlPtr->command;
    int				offset;
    Ioc_LockArgs		*lockArgsPtr;
    register int		streamType;

    /*
     * Retry loop to handle server error recovery and blocking locks.
     */
    streamType = streamPtr->ioHandlePtr->fileID.type;
    do {
	retry = FALSE;
	/*
	 * Pre-processing for some of the IOControls.
	 *
	 * IOC_NUM_READABLE.  We pass the stream offset
	 * down using the inBuffer so that the stream-type-specific routines
	 * can correctly compute how much data is available.  (Still have to
	 * do this even though we pass the streamPtr down because the offset
	 * on the server may not be up-to-date.  Probably fixable.)
	 *
	 * IOC_LOCK and IOC_UNLOCK.  We have to fill in the process and hostID
	 * entries in the buffer passed in from the user.
	 */
	if (command == IOC_NUM_READABLE) {
	    offset = streamPtr->offset;
	    ioctlPtr->inBuffer = (Address) &offset;
	    ioctlPtr->inBufSize = sizeof(int);
	    ioctlPtr->flags &= ~FS_USER_IN;
	} else if (command == IOC_LOCK || command == IOC_UNLOCK) {
	    lockArgsPtr = (Ioc_LockArgs *)ioctlPtr->inBuffer;
	    lockArgsPtr->hostID = rpc_SpriteID;
	    Sync_GetWaitToken(&lockArgsPtr->pid, &lockArgsPtr->token);
	}

	replyPtr->length = ioctlPtr->outBufSize;
	replyPtr->flags = 0;
	replyPtr->signal = 0;
	replyPtr->code = 0;
	status = (*fsStreamOpTable[streamType].ioControl)
			(streamPtr, ioctlPtr, replyPtr);
#ifdef lint
	status = FsFileIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsRmtFileIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsDeviceIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsRemoteIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsPipeIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsControlIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsServerStreamIOControl(streamPtr, ioctlPtr, replyPtr);
	status = FsPseudoStreamIOControl(streamPtr, ioctlPtr, replyPtr);
#endif /* lint */

	switch(status) {
	    case SUCCESS:
		break;
	    case RPC_TIMEOUT:
	    case RPC_SERVICE_DISABLED:
	    case FS_STALE_HANDLE:
		status = FsWaitForRecovery(streamPtr->ioHandlePtr, status);
		if (status == SUCCESS) {
		    retry = TRUE;
		    break;
		} else {
		    return(status);
		}
	    case FS_WOULD_BLOCK:
		/*
		 * Blocking I/O control.  Should be a lock, although some
		 * pseudo-device servers (like ipServer) will return
		 * this status code for their own reasons.
		 */
		if ((command == IOC_LOCK) &&
		    ((lockArgsPtr->flags & IOC_LOCK_NO_BLOCK) == 0)) {
		    if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
			return(GEN_ABORTED_BY_SIGNAL);
		    } else {
			retry = TRUE;
			break;
		    }
		} else {
		    return(status);
		}
	    default:
		return(status);
	}
    } while (retry);

    /*
     * Do generic I/O controls that affect streams -
     * flag manipulation, and offset seeking.
     */
    switch (command) {
	case IOC_REPOSITION: {
	    /*
	     * Set the read/write offset of the stream.
	     */
	    register int newOffset;
	    register Ioc_RepositionArgs	*iocArgsPtr;

	    if (ioctlPtr->inBuffer == (Address)NIL) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    iocArgsPtr = (Ioc_RepositionArgs *)ioctlPtr->inBuffer;
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
		    newOffset = attrs.size + iocArgsPtr->offset;
		    break;
		}
	    }
	    if (newOffset < 0) {
		status = GEN_INVALID_ARG;
	    } else {
		if (ioctlPtr->outBuffer != (Address)NIL) {
		    *(int *)ioctlPtr->outBuffer = newOffset;
		    replyPtr->length = sizeof(int);
		}
		streamPtr->offset = newOffset;
	    }
	    break;
	}
	/*
	 * Stream flags manipulation.  There are the FS_ constants which
	 * have a place in the streams flag field, and the IOC_ constants
	 * which user programs know about.  These allow flexibility, but
	 * requires specific checking for each kernel flag...
	 */
	case IOC_GET_FLAGS: {
	    /*
	     * OR the kernel flags from the stream into the output flags word.
	     */
	    register int flags = 0;
	    if (streamPtr->flags & FS_APPEND) {
		flags |= IOC_APPEND;
	    }
	    if (streamPtr->flags & FS_NON_BLOCKING) {
		flags |= IOC_NON_BLOCKING;
	    }
	    if (ioctlPtr->outBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		*(int *)ioctlPtr->outBuffer |= flags;
		replyPtr->length = sizeof(int);
	    }
	    break;
	}
	case IOC_SET_BITS:
	case IOC_SET_FLAGS: {
	    /*
	     * Set any kernel stream flags specifid by the IOControl flags.
	     * We rely on the file-type IOControl routine to verify
	     * the validity of the flag choices.  See that IOC_SET_FLAGS
	     * differs from IOC_SET_BITS by turning off any bits that
	     * are not in the input word.
	     */
	    register int flags;
	    if (ioctlPtr->inBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    flags = *(int *)ioctlPtr->inBuffer;
	    if ((flags & IOC_APPEND) && (streamPtr->flags & FS_WRITE) == 0) {
		status = FS_NO_ACCESS;
		break;
	    }
	    if (flags & IOC_APPEND) {
		streamPtr->flags |= FS_APPEND;
	    } else if (command == IOC_SET_FLAGS) {
		streamPtr->flags &= ~FS_APPEND;
	    }
	    if (flags & IOC_NON_BLOCKING) {
		streamPtr->flags |= FS_NON_BLOCKING;
	    } else if (command == IOC_SET_FLAGS) {
		streamPtr->flags &= ~FS_NON_BLOCKING;
	    }
	    break;
	}
	case IOC_CLEAR_BITS:{
	    register int flags;
	    if (ioctlPtr->inBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    flags = *(int *)ioctlPtr->inBuffer;
	    if (flags & IOC_APPEND) {
		streamPtr->flags &= ~FS_APPEND;
	    }
	    if (flags & IOC_NON_BLOCKING) {
		streamPtr->flags &= ~FS_NON_BLOCKING;
	    }
	    break;
	}
	/*
	 * Everything for the following has already been handled by
	 * the stream specific IOControl routines.
	 */
	case IOC_LOCK:
	case IOC_UNLOCK:
	case IOC_NUM_READABLE:
	case IOC_TRUNCATE:
	case IOC_GET_OWNER:
	case IOC_SET_OWNER:
	case IOC_MAP:
	case IOC_PREFIX:
	     break;
    }
    /*
     * Generate signal returned from stream-specific routine.
     */
    if (replyPtr->signal != 0) {
	Sig_Send(replyPtr->signal, replyPtr->code, PROC_MY_PID, FALSE);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_Close --
 *
 *	Free this reference to the Stream.  The reference count is decreased
 *	and when it goes to zero the stream close routine is called
 *	to release the reference to the I/O handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      If the ref count is zero after the decrement,  the reference to
 *      the file is released and the input parameter is free'd.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Close(streamPtr)
    register Fs_Stream 	*streamPtr;
{
    register ReturnStatus 	status;
    Proc_ControlBlock		*procPtr;

    if (streamPtr == (Fs_Stream *)NIL) {
	/*
	 * Bomb-proofing.  The current directory sometimes doesn't get
	 * found at boot time and so there is a NIL streamPtr around for it.
	 */
	return(FS_INVALID_ARG);
    }
    FsHandleLock(streamPtr);
    if (streamPtr->hdr.refCount > 1) {
	/*
	 * There are other copies of the stream (due to fork/dup) so
	 * we don't close the I/O handle yet.
	 */
	FsHandleRelease(streamPtr, TRUE);
	status = SUCCESS;
    } else {
	/*
	 * Call the stream type close routine to clean up this reference
	 * to the I/O handle.
	 */
	procPtr = Proc_GetEffectiveProc();
	FsHandleLock(streamPtr->ioHandlePtr);
	status = (fsStreamOpTable[streamPtr->ioHandlePtr->fileID.type].close)
		(streamPtr, rpc_SpriteID, procPtr->processID, streamPtr->flags,
		0, (ClientData)NIL);
#ifdef lint
	status = FsFileClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsRmtFileClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsPipeClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsDeviceClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsRemoteIOClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsControlClose(streamPtr, rpc_SpriteID, procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsPseudoStreamClose(streamPtr, rpc_SpriteID,procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
	status = FsServerStreamClose(streamPtr, rpc_SpriteID,procPtr->processID,
		streamPtr->flags, 0, (ClientData)NIL);
#endif /* lint */
	if (FsStreamClientClose(&streamPtr->clientList, rpc_SpriteID)) {
	    FsStreamDispose(streamPtr);
	} else {
	    FsHandleRelease(streamPtr, TRUE);
	}
    }
    return(status);
}

