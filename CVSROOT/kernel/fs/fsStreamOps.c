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
#include "fsStat.h"
#include "fsDisk.h"
#include "mem.h"
#include "rpc.h"
#include "vm.h"

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
    Sync_RemoteWaiter	remoteWaiter;
    int			savedLength;
    int			streamType;
    register	int	size;

    size = *lenPtr;
    if (sys_ShuttingDown) {
	return(FAILURE);
    } else if ((streamPtr->flags & FS_READ) == 0) {
	return(FS_NO_ACCESS);
    } else if (size == 0) {
	return(SUCCESS);
    } else if (size < 0) {
	return(GEN_INVALID_ARG);
    } else if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }
    streamType = streamPtr->ioHandlePtr->fileID.type;

    /*
     * Outer loop to attempt the read and then block if no data is ready.
     */
    remoteWaiter.hostID = rpc_SpriteID;
    while (TRUE) {
	Sync_GetWaitToken(&remoteWaiter.pid, &remoteWaiter.waitToken);

	savedLength = size;
	status = (fsStreamOpTable[streamType].read) (streamPtr,
		    streamPtr->flags, buffer, &offset, lenPtr, &remoteWaiter);

	if (status == SUCCESS) {
	    break;
	} else if (status == FS_WOULD_BLOCK && 
	    (streamPtr->flags & FS_NON_BLOCKING) == 0) {
	    if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
		status = GEN_ABORTED_BY_SIGNAL;
		break;
	    } else {
		/*
		 * Restore the length parameter because it was set to
		 * zero when the read blocked.
		 */
		*lenPtr = size = savedLength;
	    }
	} else if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	           status == RPC_SERVICE_DISABLED)  {
	    status = FsWaitForRecovery(streamPtr->ioHandlePtr, status);
	    if (status != SUCCESS) {
		break;
	    }
	} else {
	    break;
	}
    }
    /*
     * Cache the file offset for sequential access.
     */
    streamPtr->offset = offset;
    if (status == FS_BROKEN_PIPE) {
	Sig_Send(SIG_PIPE, 0, PROC_MY_PID, FALSE);
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
    register Address	bufPtr;			/* Pointer into buffer */
    int			toWrite;		/* Amount to write.  This is
						 * needed because of writes
						 * that block after partial
						 * completion */
    register int	size;			/* Original value of *lenPtr */
    int			streamType;		/* Type from I/O handle */

    size = *lenPtr;
    if (sys_ShuttingDown) {
	return(FAILURE);
    } else if ((streamPtr->flags & FS_WRITE) == 0) {
	return(FS_NO_ACCESS);
    } else if (size == 0) {
	return(SUCCESS);
    } else if (size < 0) {
	return(GEN_INVALID_ARG);
    } else if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }
    streamType = streamPtr->ioHandlePtr->fileID.type;

    remoteWaiter.hostID = rpc_SpriteID;

    /*
     * Outer loop to attempt the write and block if not ready.  The
     * extra size and toWrite variables are needed because of devices
     * and pipes that may only write some of the data before blocking.
     */
    bufPtr = buffer;
    while (TRUE) {
	Sync_GetWaitToken(&remoteWaiter.pid, &remoteWaiter.waitToken);

	toWrite = size;
	status = (fsStreamOpTable[streamType].write) (streamPtr,
		    streamPtr->flags, bufPtr, &offset, &toWrite, &remoteWaiter);

	bufPtr += toWrite;
	size -= toWrite;

	if (status == SUCCESS) {
	    break;
	} else if (status == FS_WOULD_BLOCK &&
	    (streamPtr->flags & FS_NON_BLOCKING) == 0) {
	    if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
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
	    break;
	}
    }
    *lenPtr -= size;
    /*
     * Cache the file offset for sequential access.
     */
    streamPtr->offset = offset;
    if (status == FS_BROKEN_PIPE) {
	Sig_Send(SIG_PIPE, 0, PROC_MY_PID, FALSE);
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
	for (i = (unsigned int) offset / FS_BLOCK_SIZE; i <= lastBlock; i++) {
	    do {
		if ((streamPtr->flags & FS_SWAP) == 0) {

		    FsCacheFetchBlock(&handlePtr->cacheInfo, i,
			    FS_DATA_CACHE_BLOCK, &blockPtr, &found);
		    if (found) {
			Byte_Copy(FS_BLOCK_SIZE, blockPtr->blockAddr, pageAddr);
			if (blockPtr->flags & FS_READ_AHEAD_BLOCK) {
			    fsStats.blockCache.readAheadHits++;
			}
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, cacheFlags);
			offset += FS_BLOCK_SIZE;
			break;	/* do-while, go to next for loop iteration */
		    } else if (pageType == FS_CODE_PAGE) {
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
		    }
		}

		retry = FALSE;
		bytesRead = FS_BLOCK_SIZE;
		status = (*fsStreamOpTable[streamType].blockRead)
			(streamPtr->ioHandlePtr, 0, pageAddr, &offset,
			 &bytesRead, (Sync_RemoteWaiter *)NIL);
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
			    Sys_Panic(SYS_WARNING,
				"Fs_PageRead recovery failed <%x>\n", status);
			    return(status);
			}
		    } else if (status != SUCCESS) {
			Sys_Panic(SYS_WARNING,
				"Fs_PageRead: Read failed <%x>\n", status);
			    return(status);
		    }
		} else if (bytesRead != FS_BLOCK_SIZE) {
		    Sys_Panic(SYS_WARNING,
			    "FsPageRead: Short read of length %d\n", bytesRead);
		    return(VM_SHORT_READ);
		}
		if (!retry && pageType == FS_HEAP_PAGE) {
		    /*
		     * We read the data into the page, now copy it into the
		     * cache since initialized heap pages live in the cache.
		     */
		    Byte_Copy(FS_BLOCK_SIZE, pageAddr, blockPtr->blockAddr);
		    FsCacheUnlockBlock(blockPtr, 0, -1, 0, 0);
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
		Sys_Panic(SYS_WARNING, "Fs_PageWrite: Block Alloc failed\n");
		status = FS_NO_DISK_SPACE;
		break;
	    }
	    status = (*fsStreamOpTable[streamType].blockWrite)
		    (streamPtr->ioHandlePtr, blockAddr, FS_BLOCK_SIZE,
			    pageAddr, FALSE);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING, "Fs_PageWrite: Write failed\n");
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
			Sys_Panic(SYS_WARNING,
			    "Fs_PageCopy, recovery failed <%x>\n", status);
			return(status);
		    }
		} else {
		    Sys_Panic(SYS_WARNING,
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
Fs_IOControl(streamPtr, command, inBufSize, inBuffer,  outBufSize, outBuffer)
    register Fs_Stream *streamPtr;
    int command;			/* IOControl command */
    int inBufSize;			/* Size of inBuffer */
    char *inBuffer;			/* Command specific input parameters */
    int outBufSize;			/* Size of outBuffer */
    char *outBuffer;			/* Command specific return parameters */
{
    register ReturnStatus	status;
    register Boolean		retry;
    int				offset;

    /*
     * Special case for IOC_NUM_READABLE.  We pass the stream offset
     * down using the inBuffer so that the stream-type-specific routines
     * can correctly compute how much data is available.
     */
    if (command == IOC_NUM_READABLE) {
	offset = streamPtr->offset;
	inBuffer = (Address) &offset;
	inBufSize = sizeof(int);
    }
    /*
     * First call down stream to see if the IOControl is applicable.
     * If so, we may also have to adjust some state of our own.
     */
    do {
	retry = FALSE;
	status = (fsStreamOpTable[streamPtr->ioHandlePtr->fileID.type].ioControl)
	    (streamPtr->ioHandlePtr, command, inBufSize, inBuffer, outBufSize,
		outBuffer);
	if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    status = FsWaitForRecovery(streamPtr->ioHandlePtr, status);
	    if (status == SUCCESS) {
		retry = TRUE;
	    }
	}
	if (status != SUCCESS) {
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

	    if (inBuffer == (Address)NIL) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    iocArgsPtr = (Ioc_RepositionArgs *)inBuffer;
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
	    if (outBuffer != (Address)NIL) {
		*(int *)outBuffer = newOffset;
	    }
	    streamPtr->offset = newOffset;
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
	    if (streamPtr->flags & FS_CLOSE_ON_EXEC) {
		flags |= IOC_CLOSE_ON_EXEC;
	    }
	    if (outBuffer == (Address)NIL) {
		status = GEN_INVALID_ARG;
	    } else {
		*(int *)outBuffer |= flags;
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
	    if (inBuffer == (Address)NIL) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    flags = *(int *)inBuffer;
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
	    if (flags & IOC_CLOSE_ON_EXEC) {
		streamPtr->flags |= FS_CLOSE_ON_EXEC;
	    } else if (command == IOC_SET_FLAGS) {
		streamPtr->flags &= ~FS_CLOSE_ON_EXEC;
	    }
	    break;
	}
	case IOC_CLEAR_BITS:{
	    register int flags;
	    if (inBuffer == (Address)NIL) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    flags = *(int *)inBuffer;
	    if (flags & IOC_APPEND) {
		streamPtr->flags &= ~FS_APPEND;
	    }
	    if (flags & IOC_NON_BLOCKING) {
		streamPtr->flags &= ~FS_NON_BLOCKING;
	    }
	    if (flags & IOC_CLOSE_ON_EXEC) {
		streamPtr->flags &= ~FS_CLOSE_ON_EXEC;
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
	     break;
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
 *      the file is released and the input parameter is Mem_Free'd.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Close(streamPtr)
    register Fs_Stream 	*streamPtr;
{
    register ReturnStatus 	status;

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
	FsHandleLock(streamPtr->ioHandlePtr);
	status = (fsStreamOpTable[streamPtr->ioHandlePtr->fileID.type].close)
		(streamPtr->ioHandlePtr, 
		rpc_SpriteID,
		 streamPtr->flags, 0, (ClientData)NIL);
	FsStreamDispose(streamPtr);
    }
    return(status);
}

