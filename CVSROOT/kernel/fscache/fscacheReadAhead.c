/* 
 * fsReadAhead.c 
 *
 *	Routines to implement read ahead.  Read ahead is synchronized
 *	with regular writes to a file to avoid inconsistencies.
 *
 * Copyright 1986 Regents of the University of California.
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsFile.h"
#include "fsReadAhead.h"
#include "fsStat.h"
#include "fsBlockCache.h"
#include "fsOpTable.h"

/* 
 * Number of blocks to read ahead.  Zero turns off read ahead.
 */
Boolean	fsReadAheadBlocks = 0;
Boolean	fsRATracing = TRUE;


#define	LOCKPTR	(&readAheadPtr->lock)
typedef struct {
    FsCacheFileInfo	*cacheInfoPtr;
    FsReadAheadInfo	*readAheadPtr;
    FsCacheBlock	*blockPtr;
    int			blockNum;
} ReadAheadCallBackData;

void	FsIncReadAheadCount();
void	FsDecReadAheadCount();
void	DoReadAhead();


/*
 *----------------------------------------------------------------------
 *
 * FsReadAheadInit --
 *
 *	Read ahead the next block in the cache
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read ahead process may be started.
 *
 *----------------------------------------------------------------------
 */
void
FsReadAheadInit(readAheadPtr)
    register	FsReadAheadInfo *readAheadPtr;
{
    Byte_Zero(sizeof(FsReadAheadInfo), (Address) readAheadPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsReadAhead --
 *
 *	Read ahead the next block in the cache
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read ahead process may be started.
 *
 *----------------------------------------------------------------------
 */
void
FsReadAhead(cacheInfoPtr, blockNum)
    register	FsCacheFileInfo *cacheInfoPtr;
    int				blockNum;
{
    int				i;
    ReadAheadCallBackData	*callBackData;
    FsReadAheadInfo		*readAheadPtr;
    Boolean			openForWriting;
    FsCacheBlock		*blockPtr;
    Boolean			found;

    switch (cacheInfoPtr->hdrPtr->fileID.type) {
	case FS_LCL_FILE_STREAM: {
	    register FsLocalFileIOHandle *handlePtr =
		    (FsLocalFileIOHandle *)cacheInfoPtr->hdrPtr;
	    openForWriting = (handlePtr->use.write > 0);
	    readAheadPtr = &handlePtr->readAhead;
	    break;
	}
	case FS_RMT_FILE_STREAM: {
	    register FsRmtFileIOHandle *rmtHandlePtr =
		    (FsRmtFileIOHandle *)cacheInfoPtr->hdrPtr;
	    openForWriting = (rmtHandlePtr->rmt.recovery.use.write > 0);
	    readAheadPtr = &rmtHandlePtr->readAhead;
	    break;
	}
	default:
	    Sys_Panic(SYS_FATAL, "FsReadAhead, bad stream type <%d>\n",
		cacheInfoPtr->hdrPtr->fileID.type);
	    return;
    }

    if (fsReadAheadBlocks == 0 || openForWriting > 0 ||
        FsAllInCache(cacheInfoPtr)) {
	/*
	 * Don't do read ahead if there is no read ahead, the file is
	 * open for writing, or all the blocks are already in the cache.
	 * Read ahead is disallowed if the file is open for writing because
	 * read ahead is done without the handle locked and it is unsafe to
	 * be reading and writing a file at the same time.
	 */
	return;
    }
    for (i = blockNum; i < blockNum + fsReadAheadBlocks; i++) {
	if (i * FS_BLOCK_SIZE > cacheInfoPtr->attr.lastByte) {
	    return;
	}
	FsCacheFetchBlock(cacheInfoPtr, i,
	      FS_DATA_CACHE_BLOCK | FS_CACHE_DONT_BLOCK | FS_READ_AHEAD_BLOCK,
	      &blockPtr, &found);
	if (found) {
	    if (blockPtr != (FsCacheBlock *) NIL) {
		FsCacheUnlockBlock(blockPtr, 0, -1, 0, 0);
	    }
	    continue;
	}

	fsStats.blockCache.readAheads++;
	FsIncReadAheadCount(readAheadPtr);
	callBackData = Mem_New(ReadAheadCallBackData);
	callBackData->cacheInfoPtr = cacheInfoPtr;
	callBackData->readAheadPtr = readAheadPtr;
	callBackData->blockNum = i;
	callBackData->blockPtr = blockPtr;
	Proc_CallFunc(DoReadAhead, (ClientData) callBackData, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DoReadAhead --
 *
 *	Actually read ahead the given block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given block is read in.
 *
 *----------------------------------------------------------------------
 */
void
DoReadAhead(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register	FsCacheFileInfo *cacheInfoPtr;
    register	ReadAheadCallBackData *callBackData;
    register	FsCacheBlock	*blockPtr;
    int				amountRead;
    int				blockOffset;
    ReturnStatus		status;
    int				blockNum;

    callBackData = (ReadAheadCallBackData *) data;

    blockNum = callBackData->blockNum;
    cacheInfoPtr = callBackData->cacheInfoPtr;
    blockPtr = callBackData->blockPtr;
    blockOffset = blockNum * FS_BLOCK_SIZE;
    amountRead = FS_BLOCK_SIZE;

    status = (*fsStreamOpTable[cacheInfoPtr->hdrPtr->fileID.type].blockRead)
		(cacheInfoPtr->hdrPtr, 0, blockPtr->blockAddr, &blockOffset,
		&amountRead, (Sync_RemoteWaiter *)NIL);
    if (status != SUCCESS) {
	fsStats.blockCache.domainReadFails++;
	FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
    } else {
	if (amountRead < FS_BLOCK_SIZE) {
	    /*
	     * We always must make sure that every cache block is filled
	     * with zeroes.  Since we didn't read a full block zero fill
	     * the rest.
	     */
	    fsStats.blockCache.readZeroFills++;
	    Byte_Zero(FS_BLOCK_SIZE - amountRead,
		      blockPtr->blockAddr + amountRead);
	}
	FsCacheUnlockBlock(blockPtr, 0, -1, 0, 0);
    }
    FsDecReadAheadCount(callBackData->readAheadPtr);
    Mem_Free((Address) callBackData);
    callInfoPtr->interval = 0;	/* don't call us again */
}

/*
 *----------------------------------------------------------------------------
 *
 * FsWaitForReadAhead --
 *
 *	Block the caller until the read ahead count on this handle goes to
 *	zero.  Called before a write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blocks read-ahead until FsAllowReadAhead is called.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsWaitForReadAhead(readAheadPtr)
    FsReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    while (readAheadPtr->count > 0) {
	(void) Sync_Wait(&readAheadPtr->done, FALSE);
    }
    readAheadPtr->blocked = TRUE;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsAllowReadAhead --
 *
 *	Indicate that it is ok to initiate read ahead.  Called when a
 *	write completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notifies the okToRead condition.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsAllowReadAhead(readAheadPtr)
    FsReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    readAheadPtr->blocked = FALSE;
    Sync_Broadcast(&readAheadPtr->okToRead);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsIncReadAheadCount --
 *
 *	Increment the number of read aheads on this file.  This will block
 *	if read aheads are blocked because of a write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment the number of read aheads on the file.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsIncReadAheadCount(readAheadPtr)
    FsReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    while (readAheadPtr->blocked) {
	(void) Sync_Wait(&readAheadPtr->okToRead, FALSE);
    }
    readAheadPtr->count++;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsDecReadAheadCount --
 *
 *	Decrement the number of read aheads on this file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read ahead count is decremented and if it goes to zero a broadcast 
 *	is done on the handles condition.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsDecReadAheadCount(readAheadPtr)
    FsReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    readAheadPtr->count--;
    if (readAheadPtr->count == 0) {
	Sync_Broadcast(&readAheadPtr->done);
    }

    UNLOCK_MONITOR;
}
