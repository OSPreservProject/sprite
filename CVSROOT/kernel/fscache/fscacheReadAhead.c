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
#include "fsutil.h"
#include "fsio.h"
#include "fsStat.h"
#include "fscache.h"
#include "fsNameOps.h"
#include "fsdm.h"
#include "fsrmt.h"

/* 
 * Number of blocks to read ahead.  Zero turns off read ahead.
 */
Boolean	fscache_NumReadAheadBlocks = 0;
Boolean	fscache_RATracing = TRUE;


#define	LOCKPTR	(&readAheadPtr->lock)
typedef struct {
    Fscache_FileInfo	*cacheInfoPtr;
    Fscache_ReadAheadInfo	*readAheadPtr;
    Fscache_Block	*blockPtr;
    int			blockNum;
} ReadAheadCallBackData;

static void	IncReadAheadCount();
static void	DecReadAheadCount();
static void	DoReadAhead();


/*
 *----------------------------------------------------------------------
 *
 * Fscache_ReadAheadInit --
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
Fscache_ReadAheadInit(readAheadPtr)
    register	Fscache_ReadAheadInfo *readAheadPtr;
{
    bzero((Address) readAheadPtr, sizeof(Fscache_ReadAheadInfo));
    Sync_LockInitDynamic(&readAheadPtr->lock, "Fs:readAheadLock");
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_ReadAheadSyncLockCleanup --
 *
 *	Clean up the Sync_Lock tracing info for the read ahead lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	As above.
 *
 *----------------------------------------------------------------------
 */
void
Fscache_ReadAheadSyncLockCleanup(readAheadPtr)
    register	Fscache_ReadAheadInfo *readAheadPtr;
{
    Sync_LockClear(&readAheadPtr->lock);
}

/*
 *----------------------------------------------------------------------
 *
 * FscacheReadAhead --
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
FscacheReadAhead(cacheInfoPtr, blockNum)
    register	Fscache_FileInfo *cacheInfoPtr;
    int				blockNum;
{
    int				i;
    ReadAheadCallBackData	*callBackData;
    Fscache_ReadAheadInfo		*readAheadPtr;
    Boolean			openForWriting;
    Fscache_Block		*blockPtr;
    Boolean			found;

    switch (cacheInfoPtr->hdrPtr->fileID.type) {
	case FSIO_LCL_FILE_STREAM: {
	    register Fsio_FileIOHandle *handlePtr =
		    (Fsio_FileIOHandle *)cacheInfoPtr->hdrPtr;
	    openForWriting = (handlePtr->use.write > 0);
	    readAheadPtr = &handlePtr->readAhead;
	    break;
	}
	case FSIO_RMT_FILE_STREAM: {
	    register Fsrmt_FileIOHandle *rmtHandlePtr =
		    (Fsrmt_FileIOHandle *)cacheInfoPtr->hdrPtr;
	    openForWriting = (rmtHandlePtr->rmt.recovery.use.write > 0);
	    readAheadPtr = &rmtHandlePtr->readAhead;
	    break;
	}
	default:
	    panic("FscacheReadAhead, bad stream type <%d>\n",
		cacheInfoPtr->hdrPtr->fileID.type);
	    return;
    }

    if (fscache_NumReadAheadBlocks == 0 || openForWriting > 0 ||
        FscacheAllBlocksInCache(cacheInfoPtr)) {
	/*
	 * Don't do read ahead if there is no read ahead, the file is
	 * open for writing, or all the blocks are already in the cache.
	 * Read ahead is disallowed if the file is open for writing because
	 * read ahead is done without the handle locked and it is unsafe to
	 * be reading and writing a file at the same time.
	 */
	return;
    }
    for (i = blockNum; i < blockNum + fscache_NumReadAheadBlocks; i++) {
	if (i * FS_BLOCK_SIZE > cacheInfoPtr->attr.lastByte) {
	    return;
	}
	Fscache_FetchBlock(cacheInfoPtr, i,
	      FSCACHE_DATA_BLOCK | FSCACHE_DONT_BLOCK | FSCACHE_READ_AHEAD_BLOCK,
	      &blockPtr, &found);
	if (found) {
	    if (blockPtr != (Fscache_Block *) NIL) {
		Fscache_UnlockBlock(blockPtr, 0, -1, 0, 0);
	    }
	    continue;
	}

	fs_Stats.blockCache.readAheads++;
	IncReadAheadCount(readAheadPtr);
	callBackData = mnew(ReadAheadCallBackData);
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
static void
DoReadAhead(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register	Fscache_FileInfo *cacheInfoPtr;
    register	ReadAheadCallBackData *callBackData;
    register	Fscache_Block	*blockPtr;
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

    status = (cacheInfoPtr->ioProcsPtr->blockRead)
		(cacheInfoPtr->hdrPtr, 0, blockPtr->blockAddr, &blockOffset,
		&amountRead, (Sync_RemoteWaiter *)NIL);
    if (status != SUCCESS) {
	fs_Stats.blockCache.domainReadFails++;
	Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
    } else {
	if (amountRead < FS_BLOCK_SIZE) {
	    /*
	     * We always must make sure that every cache block is filled
	     * with zeroes.  Since we didn't read a full block zero fill
	     * the rest.
	     */
	    fs_Stats.blockCache.readZeroFills++;
	    bzero(blockPtr->blockAddr + amountRead, FS_BLOCK_SIZE - amountRead);
	}
	Fscache_UnlockBlock(blockPtr, 0, -1, 0, 0);
    }
    DecReadAheadCount(callBackData->readAheadPtr);
    free((Address) callBackData);
    callInfoPtr->interval = 0;	/* don't call us again */
}

/*
 *----------------------------------------------------------------------------
 *
 * Fscache_WaitForReadAhead --
 *
 *	Block the caller until the read ahead count on this handle goes to
 *	zero.  Called before a write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blocks read-ahead until FscacheAllowReadAhead is called.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_WaitForReadAhead(readAheadPtr)
    Fscache_ReadAheadInfo *readAheadPtr;
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
 * Fscache_AllowReadAhead --
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
Fscache_AllowReadAhead(readAheadPtr)
    Fscache_ReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    readAheadPtr->blocked = FALSE;
    Sync_Broadcast(&readAheadPtr->okToRead);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * IncReadAheadCount --
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
static void
IncReadAheadCount(readAheadPtr)
    Fscache_ReadAheadInfo *readAheadPtr;
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
 * DecReadAheadCount --
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
static void
DecReadAheadCount(readAheadPtr)
    Fscache_ReadAheadInfo *readAheadPtr;
{
    LOCK_MONITOR;

    readAheadPtr->count--;
    if (readAheadPtr->count == 0) {
	Sync_Broadcast(&readAheadPtr->done);
    }

    UNLOCK_MONITOR;
}
