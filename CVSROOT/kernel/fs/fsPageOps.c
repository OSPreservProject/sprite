/*
 * fsPageOps.c --
 *
 *	The has the page-in, page-out, and page-copy routines used
 *	by the VM system.
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


#include <sprite.h>
#include <fs.h>
#include <fsio.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fscache.h>
#include <fsutilTrace.h>
#include <fsStat.h>
#include <fsdm.h>
#include <fsprefix.h>
#include <rpc.h>
#include <vm.h>
#include <fsrmt.h>
#include <fslcl.h>

#include <stdio.h>


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
    Fs_IOParam			io;	/* Write parameter block */
    register Fs_IOParam		*ioPtr = &io;
    Fs_IOReply			reply;	/* Return length, signal */
    Boolean			retry;	/* Error retry flag */

    FsSetIOParam(ioPtr, pageAddr, numBytes, offset, 0);
    reply.length = 0;
    reply.flags = 0;
    reply.signal = 0;
    reply.code = 0;
    /*
     * Tell the page routine if the file is not cacheable on clients.
     * We currently don't tell it if there is a HEAP or CODE page,
     * although we used to use this information to copy initialized
     * HEAP pages into the cache, even on clients.  However, for non-SWAP
     * pages a client should still check its cache to make sure it
     * didn't just generate the data.
     */
    if ((pageType == FS_SWAP_PAGE) || (pageType == FS_SHARED_PAGE)) {
	ioPtr->flags |= FS_SWAP;
    } else if (pageType == FS_HEAP_PAGE) {
	ioPtr->flags |= FS_HEAP;
    }
    do {
	retry = FALSE;
	status = (fsio_StreamOpTable[streamPtr->ioHandlePtr->fileID.type].pageRead) (streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
#ifdef lint
	status = Fsio_FileRead(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
	status = FsrmtFilePageRead(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
	status = FspdevPseudoStreamRead(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
	status = Fsrmt_Read(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
#endif /* lint */

	if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	    status == RPC_SERVICE_DISABLED) {
	    /*
	     * The server is down so we wait for it.  This blocks
	     * the user process doing the page fault.
	     */
	    Net_HostPrint(streamPtr->ioHandlePtr->fileID.serverID,
		    "Fs_PageRead waiting\n");
	    status = Fsutil_WaitForRecovery(streamPtr->ioHandlePtr, status);
	    if (status == SUCCESS) {
		retry = TRUE;
	    } else {
		printf("Fs_PageRead recovery failed <%x>\n", status);
	    }
	} else if (status == FS_WOULD_BLOCK) {
	    /*
	     * The remote server is so hosed that it can't
	     * deliver us a block.  There is no good way
	     * to wait.  Retry immediately?  Pound pound pound?
	     */
	    retry = TRUE;
	    printf("Fs_PageRead: blocked, waiting 1 min\n");
	    (void)Sync_WaitTime(time_OneMinute);
	} else if (status != SUCCESS) {
	    printf("Fs_PageRead: Read failed <%x>\n", status);
	}
    } while (retry);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PageWrite --
 *
 *	Write out a virtual memory page.  This is a simplified form of
 *	Fs_Write that invokes the stream's pageWrite operation.  The
 *	stream-specific procedure that is invoked can be the regular
 *	write routine (as it is for local files)  or something more fancy
 *	(as is true for remote files).  This passes the FS_SWAP flag down
 *	to the pageWrite routine so a regular write routine can detect
 *	the difference.
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
Fs_PageWrite(streamPtr, pageAddr, offset, numBytes, toDisk)
    Fs_Stream	*streamPtr;	/* Swap file stream. */
    Address	pageAddr;	/* Pointer to page. */
    int		offset;		/* Offset in file. */
    int		numBytes;	/* Number of bytes in page. */
    Boolean	toDisk;		/* TRUE to write through to disk. */
{
    ReturnStatus		status = SUCCESS;
    Fs_IOParam			io;	/* Write parameter block */
    register Fs_IOParam		*ioPtr = &io;
    Fs_IOReply			reply;	/* Return length, signal */

    FsSetIOParam(ioPtr, pageAddr, numBytes, offset, FS_SWAP |
	    (toDisk? FS_WRITE_TO_DISK : 0));
    reply.length = 0;
    reply.flags = 0;
    reply.signal = 0;
    reply.code = 0;

    status = (fsio_StreamOpTable[streamPtr->ioHandlePtr->fileID.type].pageWrite) (streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
#ifdef lint
    status = Fsio_FileWrite(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
    status = FsrmtFilePageWrite(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
    status = FspdevPseudoStreamWrite(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
    status = Fsrmt_Write(streamPtr, ioPtr, (Sync_RemoteWaiter *)NIL, &reply);
#endif /* lint */

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_PageCopy --
 *
 *	Copy the file system blocks in the source swap file to the destination
 *	swap file.  NOTE:  This is still specific to local and remote files.
 *	This means that only Sprite files can be used for swap space.
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
    register	Fs_HandleHeader	*srcHdrPtr;
    register	Fs_HandleHeader	*destHdrPtr;
    ReturnStatus		status = SUCCESS;
    int				i;
    Boolean			retry;
    Address			swapPageCopy = (Address) NIL;
    Boolean			swapPageAllocated = FALSE;

    srcHdrPtr = srcStreamPtr->ioHandlePtr;
    destHdrPtr = destStreamPtr->ioHandlePtr;
    lastBlock = (unsigned int) (offset + numBytes - 1) / FS_BLOCK_SIZE;

    /*
     * Copy all blocks in the page.
     */
    for (i = (unsigned int) offset / FS_BLOCK_SIZE; i <= lastBlock; i++) {
	do {
	    retry = FALSE;
	    if (srcHdrPtr->fileID.serverID != destHdrPtr->fileID.serverID) {
		/*
		 * The swap files are on different machines, so we need to read
		 * from one and write to the other.
		 */
		if (!swapPageAllocated) {
		    swapPageCopy = (Address) malloc(Vm_GetPageSize());
		}
		swapPageAllocated = TRUE;
		status =
			Fs_PageRead(srcStreamPtr, swapPageCopy, offset,
			numBytes, FS_SWAP_PAGE);
		if (status != SUCCESS) {
		    /* Fs_PageRead handles recovery itself, so we don't here. */
		    break;
		}
		status =
			Fs_PageWrite(destStreamPtr, swapPageCopy, offset,
			numBytes, TRUE);
	    } else {
		status = (fsio_StreamOpTable[srcHdrPtr->fileID.type].blockCopy)
			(srcHdrPtr, destHdrPtr, i);
#ifdef lint
		status = Fsio_FileBlockCopy(srcHdrPtr, destHdrPtr, i);
		status = Fsrmt_BlockCopy(srcHdrPtr, destHdrPtr, i);
#endif /* lint */
	    }
	    if (status != SUCCESS) {
		if (status == RPC_TIMEOUT || status == RPC_SERVICE_DISABLED ||
			status == FS_STALE_HANDLE) {
		    /*
		     * The server is down so we wait for it.  This just blocks
		     * the user process doing the page fault.
		     */
		    Net_HostPrint(srcHdrPtr->fileID.serverID,
			    "Fs_PageCopy, waiting for server %d\n");
		    status = Fsutil_WaitForRecovery(srcStreamPtr->ioHandlePtr,
				status);
		    if (status == SUCCESS) {
			retry = TRUE;
		    } else {
			printf("Fs_PageCopy, recovery failed <%x>\n", status);
			if (swapPageAllocated) {
			    free(swapPageCopy);
			}
			return(status);
		    }
		} else {
		    printf("Fs_PageCopy: Copy failed <%x>\n", status);
		    if (swapPageAllocated) {
			free(swapPageCopy);
		    }
		    return(status);
		}
	    }
	} while (retry);

    }
    if (swapPageAllocated) {
	free(swapPageCopy);
    }
    return(status);
}
