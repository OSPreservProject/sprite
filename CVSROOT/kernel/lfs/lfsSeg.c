/* 
 * lfsSeg.c --
 *
 *	Handles the manipulation of LFS segments.
 *
 * Copyright 1989 Regents of the University of California
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
#endif /* not lint */

#include <lfsInt.h>
#include <lfsSeg.h>
#include <stdlib.h>
#include <sync.h>

#define	LOCKPTR	&lfsPtr->lock

Boolean	lfsSegWriteDebug = FALSE;

#define	MIN_SUMMARY_REGION_SIZE	16


enum CallBackType { SEG_LAYOUT, SEG_CLEAN_IN, SEG_CLEAN_OUT, SEG_CHECKPOINT, 
		   SEG_WRITEDONE};

LfsSegIoInterface *lfsSegIoInterfacePtrs[LFS_MAX_NUM_MODS];


static void SegmentCleanProc _ARGS_((ClientData clientData, 
				     Proc_CallInfo *callInfoPtr));
static LfsSeg *CreateSegmentToClean _ARGS_((Lfs *lfsPtr, int segNumber, 
			char *cleaningMemPtr));
static LfsSeg *CreateSegmentToWrite _ARGS_((Lfs *lfsPtr, Boolean dontBlock));
static LfsSeg *GetSegStruct _ARGS_((Lfs *lfsPtr, LfsSegLogRange 
			*segLogRangePtr, int startBlockOffset, char *memPtr));
static void AddNewSummaryBlock _ARGS_((LfsSeg *segPtr));
static Boolean DoOutCallBacks _ARGS_((enum CallBackType type, LfsSeg *segPtr, int flags, char *checkPointPtr, int *sizePtr, ClientData *clientDataPtr));
static ReturnStatus WriteSegmentStart _ARGS_((LfsSeg *segPtr));
static ReturnStatus WriteSegmentFinish _ARGS_((LfsSeg *segPtr));
static void WriteDoneNotify _ARGS_((Lfs *lfsPtr));
static void RewindCurPtrs _ARGS_((LfsSeg *segPtr));
static Boolean DoInCallBacks _ARGS_((enum CallBackType type, LfsSeg *segPtr, int flags, int *sizePtr, int *numCacheBlocksPtr, ClientData *clientDataPtr));
static void DestorySegStruct _ARGS_((LfsSeg *segPtr));

/*
 * Macro returning TRUE if segment is completely empty.
 */
#define SegIsEmpty(segPtr) (((segPtr)->numBlocks == 1) && 		\
	    ((segPtr)->curSegSummaryPtr->size == sizeof(LfsSegSummary)))

/*
 *----------------------------------------------------------------------
 *
 * -- LfsSegIoRegister
 *
 *	Register with the segment module an interface to objects in the log.   
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegIoRegister(moduleType, ioInterfacePtr)
    int			moduleType; /* Module type registering. Type defined in
				     * lfsSegWrite.h */
    LfsSegIoInterface	*ioInterfacePtr;  /* Interface to use. */

{
    lfsSegIoInterfacePtrs[moduleType] = ioInterfacePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegmentWriteProc --
 *
 *	Proc_CallFunc procedure for performing a segment writes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
 void
LfsSegmentWriteProc(clientData, callInfoPtr)
    ClientData	   clientData;
    Proc_CallInfo *callInfoPtr;         /* Not used. */
{
    register Lfs *lfsPtr;            /* File system with dirty blocks. */
    Boolean	full;
    LfsSeg	*segPtr;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			i;
    ReturnStatus	status;
    Boolean		moreWork;

    lfsPtr = (Lfs *) clientData;
    moreWork = TRUE;
    while (moreWork) { 
	full = TRUE;
	for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	    clientDataArray[i] = (ClientData) NIL;
	}
	while (full) {
	    segPtr = CreateSegmentToWrite(lfsPtr, FALSE);
	    full = DoOutCallBacks(SEG_LAYOUT, segPtr, 0, (char *) NIL,
				    (int *) NIL, clientDataArray);
	    status = WriteSegmentStart(segPtr);
	    if (status == SUCCESS) {
		status = WriteSegmentFinish(segPtr);
	    }
	    if (status != SUCCESS) {
		LfsError(lfsPtr, status, "Can't write segment to log\n");
	    }
	    RewindCurPtrs(segPtr);
	    (void) DoInCallBacks(SEG_WRITEDONE, segPtr, 0,
				    (int *) NIL, (int *) NIL, clientDataArray);
	    WriteDoneNotify(lfsPtr);
	    DestorySegStruct(segPtr);
	}
	moreWork = LfsMoreToWriteBack(lfsPtr);
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegCleanStart --
 *
 *	Request that the segment manager start cleaning segments
 *	for the specified file system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A segment cleaning process may be started.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegCleanStart(lfsPtr)
    Lfs	 *lfsPtr;	/* File system needing segments cleaned. */
{
    LFS_STATS_INC(lfsPtr->stats.cleaning.startRequests);
    if (lfsPtr->activeFlags & LFS_CLEANER_ACTIVE) {
	LFS_STATS_INC(lfsPtr->stats.cleaning.alreadyActive);
	return;
    }
    lfsPtr->activeFlags |= LFS_CLEANER_ACTIVE;
    Proc_CallFunc(SegmentCleanProc, (ClientData) lfsPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegSlowGrowSummary --
 *
 *	Insure that there is enought room for an object with the 
 *	specified number of blocks and summary bytes. Possibly add a
 *	new summary block if needed.
 *
 * Results:
 *	A pointer to the summary bytes for this region or NIL if
 *	there is not enought room.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

char *
LfsSegSlowGrowSummary(segPtr, dataBlocksNeeded, sumBytesNeeded, addNewBlock)
    register LfsSeg	*segPtr;	/* Segment of interest. */
    int	 dataBlocksNeeded;		/* Number of data blocks needed. */
    int	sumBytesNeeded;			/* Number of summary bytes needed. */
    Boolean addNewBlock;		/* Added a new block if necessary. */
{
    Lfs	*lfsPtr;
    int	 sumBytesLeft, blocksLeft, sumBlocks;

    /*
     * Test the most common case first. Do the data and summary fit
     * in the current configuration. 
     */
    sumBytesLeft = LfsSegSummaryBytesLeft(segPtr);
    blocksLeft = LfsSegBlocksLeft(segPtr);
    if ((blocksLeft >= dataBlocksNeeded) && (sumBytesLeft >= sumBytesNeeded)) {
       return segPtr->curSummaryPtr;
    }
    /*
     * Need to add a summary block. Bail if the user doesn't what it 
     * or there is not enought room.
     */
    sumBlocks = 1;
    if (!addNewBlock || (dataBlocksNeeded + sumBlocks > blocksLeft)) { 
	return (char *) NIL;
    }
    lfsPtr = segPtr->lfsPtr;
    /*
     * Malloc a new summary buffer and add it to the segment.
     */
    AddNewSummaryBlock(segPtr);
    return segPtr->curSummaryPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegSlowDiskAddress --
 *
 *	Compute the disk address of a LfsSegElement.
 *
 * Results:
 *	The disk address of the element.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

LfsDiskAddr
LfsSegSlowDiskAddress(segPtr, segElementPtr)
    register LfsSeg	*segPtr; 	/* Segment of interest. */
    LfsSegElement *segElementPtr; /* Segment element of interest. */
{
    int	elementNumber, blockOffset;
    LfsDiskAddr diskAddress, newDiskAddr;
    /*
     * Check the common case that we are asking about the "current"
     * element. 
     */
    elementNumber = segElementPtr - segPtr->segElementPtr;

    if (elementNumber == segPtr->curElement) {
	blockOffset = segPtr->curBlockOffset;
    } else {
	int	i;
	blockOffset = segPtr->startBlockOffset;
	for (i = 0; i <= elementNumber; i++) {
	    blockOffset += segPtr->segElementPtr[i].lengthInBlocks;
	}
    }
    LfsSegNumToDiskAddress(segPtr->lfsPtr, segPtr->logRange.current, 
		&diskAddress);
    blockOffset = LfsSegSizeInBlocks(segPtr->lfsPtr) - blockOffset;
    LfsDiskAddrPlusOffset(diskAddress, blockOffset, &newDiskAddr);
    return newDiskAddr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegSlowAddDataBuffer --
 *
 *	Add a LfsSegElement to a segment.
 *
 * Results:
 *	A pointer to the LfsSegElement added. NIL if the object would not
 *	fit.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

LfsSegElement *
LfsSegSlowAddDataBuffer(segPtr, blocks, bufferPtr, clientData)
    register LfsSeg	*segPtr;  /* Segment to add to. */ 
    int	        blocks;	  /* Size of buffer to add in blocks */
    char   *bufferPtr;	  /* Buffer to add. */
    ClientData clientData; /* ClientData associated with this field. */
{
    LfsSegElement *elementPtr;

    if (segPtr->curBlockOffset + blocks > segPtr->curDataBlockLimit) {
	return (LfsSegElement *) NIL;
    }
    segPtr->curElement++;
    segPtr->curBlockOffset += blocks;

    elementPtr = segPtr->segElementPtr + segPtr->curElement;
    elementPtr->lengthInBlocks = blocks;
    elementPtr->clientData = clientData;
    elementPtr->address	= bufferPtr;
    segPtr->numElements = segPtr->curElement+1;
    segPtr->numBlocks += blocks;
    return elementPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * InitSegmentMem --
 *
 *	Initialize the memory used by a file systems segment code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	LfsSeg memories allocated.
 *
 *----------------------------------------------------------------------
 */
void
InitSegmentMem(lfsPtr)
    Lfs	*lfsPtr;
{
    LfsSeg	*segPtr;
    int		maxSegElementSize;
    int		i;
    DevBlockDeviceHandle *handlePtr;

    /*
     * Compute the maximum size of the seg element array. It can't be
     * bigger than one element per block in segment.
     */

    maxSegElementSize = LfsBytesToBlocks(lfsPtr,LfsSegSize(lfsPtr)) * 
				    sizeof(LfsSegElement);
    /*
     * Fill in the fixed fields of the preallocated segments.
     */
    lfsPtr->segsInUse = 0;
    lfsPtr->segs = (LfsSeg *) malloc(LFS_NUM_PREALLOC_SEGS *sizeof(LfsSeg));
    for (i = 0; i < LFS_NUM_PREALLOC_SEGS; i++) { 
	segPtr = lfsPtr->segs + i;
	segPtr->lfsPtr = lfsPtr;
	segPtr->segElementPtr = (LfsSegElement *) malloc(maxSegElementSize);
    }
    handlePtr = (DevBlockDeviceHandle *) lfsPtr->devicePtr->data;
    if (handlePtr->maxTransferSize < LfsSegSize(lfsPtr)) { 
	lfsPtr->writeBuffers[0] = malloc(handlePtr->maxTransferSize*2);
	lfsPtr->writeBuffers[1] = lfsPtr->writeBuffers[0] + 
				    handlePtr->maxTransferSize;
    } else {
	lfsPtr->writeBuffers[0] = malloc(LfsSegSize(lfsPtr));
	lfsPtr->writeBuffers[1] = (char *) NIL;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * FreeSegmentMem --
 *
 *	Free the memory used by a file systems segment code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	LfsSeg structures freed.
 *
 *----------------------------------------------------------------------
 */
void
FreeSegmentMem(lfsPtr)
    Lfs	*lfsPtr;
{
    int i;
    free(lfsPtr->writeBuffers[0]);
    lfsPtr->segsInUse = 0;
    for (i = 0; i < LFS_NUM_PREALLOC_SEGS; i++) { 
	free((char *)(lfsPtr->segs[i].segElementPtr));
    }
    free((char *) (lfsPtr->segs));

}

/*
 *----------------------------------------------------------------------
 *
 * AddNewSummaryBlock --
 *
 *  Add a summary block to a segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new summary block is malloc() and initialized.
 *
 *----------------------------------------------------------------------
 */
static void
AddNewSummaryBlock(segPtr)
    LfsSeg	*segPtr;	/* Seg to add block to. */
{
    LfsSegElement *sumBufferPtr;
    LfsSegSummary *newSummaryPtr;
    int		  sumBytes;

    sumBytes = LfsBlockSize(segPtr->lfsPtr);
    sumBufferPtr = LfsSegAddDataBuffer(segPtr, 1, malloc(sumBytes),
					(ClientData) NIL);
    newSummaryPtr =  (LfsSegSummary *) sumBufferPtr->address;
    newSummaryPtr->magic = LFS_SEG_SUMMARY_MAGIC;
    newSummaryPtr->timestamp = LfsGetCurrentTimestamp(segPtr->lfsPtr);
    newSummaryPtr->prevSeg = segPtr->logRange.prevSeg;
    newSummaryPtr->nextSeg = segPtr->logRange.nextSeg;
    newSummaryPtr->size = sizeof(LfsSegSummary);
    newSummaryPtr->nextSummaryBlock = -1;

    if (segPtr->curSegSummaryPtr != (LfsSegSummary *) NIL) { 
	/*
	 * This is not the first summary block in the segment.  Fixup the
	 * size of the last summary block and point it at the new one. 
	 */
	segPtr->curSegSummaryPtr->size = segPtr->curSummaryPtr - 
				    (char *) (segPtr->curSegSummaryPtr);
	segPtr->curSegSummaryPtr->nextSummaryBlock = segPtr->curBlockOffset;
    } 
    segPtr->curSegSummaryPtr = newSummaryPtr;

    segPtr->curSummaryPtr = sumBufferPtr->address + sizeof(LfsSegSummary);
    segPtr->curSummaryLimitPtr = sumBufferPtr->address + sumBytes;
}

void
CopySegToBuffer( segPtr, maxSize, bufferPtr, lenPtr)
    LfsSeg	*segPtr;
    int		maxSize;
    char	*bufferPtr;
    int		*lenPtr;
{
    int bytes, offset;
    LfsSegElement *elementPtr;
    Boolean full;

    *lenPtr = 0;
    offset = 0;
    full = FALSE;
    while ((segPtr->curElement >= 0) && !full) {
	elementPtr = segPtr->segElementPtr + segPtr->curElement;
	bytes = LfsBlocksToBytes(segPtr->lfsPtr, 
		elementPtr->lengthInBlocks - segPtr->curBlockOffset);
	if (*lenPtr + bytes > maxSize) {
	    /*
	     * Element doesn't fit in this buffer. 
	     */
	    if (*lenPtr == maxSize) {
		offset = 0;
	    } else { 
		offset = LfsBytesToBlocks(segPtr->lfsPtr,(maxSize - *lenPtr));
	    }
	    bytes = LfsBlocksToBytes(segPtr->lfsPtr, offset);
	    full = TRUE;
	} 
	if (segPtr->curBlockOffset == 0) { 
	    bcopy(elementPtr->address, bufferPtr + *lenPtr, bytes);
	} else {
	    bcopy(elementPtr->address + 
		   LfsBlocksToBytes(segPtr->lfsPtr,segPtr->curBlockOffset),
		  bufferPtr + *lenPtr, bytes);
	}
	*lenPtr += bytes;
	if (full) {
	    segPtr->curBlockOffset += offset;
	} else {
	    segPtr->curElement--;
	    segPtr->curBlockOffset = 0;
	}
    }
}

/*
 * By defining SUN4DMAHACK, LFS copies data to block already allocated
 * in DMA space.  This saves DMA allocation and free in the device
 * driver.
 */
#ifdef notdef
#define	SUN4DMAHACK
#endif

/*
 *----------------------------------------------------------------------
 *
 * SegIoDoneProc --
 *
 *	This procedure is called when a sync block command started by 
 *	WriteSegmentStart finished. It's calling sequence is 
 *	defined by the call back caused by the Dev_BlockDeviceIO routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
SegIoDoneProc(requestPtr, status, amountTransferred)
    DevBlockDeviceRequest	*requestPtr;
    ReturnStatus status;
    int	amountTransferred;
{
    LfsSeg	*segPtr = (LfsSeg *) (requestPtr->clientData);
    DevBlockDeviceHandle *handlePtr;

    handlePtr = (DevBlockDeviceHandle *) segPtr->lfsPtr->devicePtr->data;
    /*
     * A pointer to the LfsSeg is passed as the clientData to this call.
     * Start the new request if one is available. If this is the last
     * request note the I/O as done.
     */
    MASTER_LOCK(&segPtr->ioMutex);
    if (amountTransferred != requestPtr->bufferLen) {
	status = VM_SHORT_WRITE;
    }
    if (status != SUCCESS) {
	segPtr->ioReturnStatus = status;
    }
    requestPtr->startAddress = DEV_BYTES_PER_SECTOR * 
				LfsDiskAddrToOffset(segPtr->nextDiskAddress);
    requestPtr->bufferLen = 0;
    CopySegToBuffer(segPtr, handlePtr->maxTransferSize, requestPtr->buffer, 
			&requestPtr->bufferLen);
    segPtr->nextDiskAddress += requestPtr->bufferLen/DEV_BYTES_PER_SECTOR;
    if (requestPtr->bufferLen == 0) { 
	segPtr->requestActive--;
#ifdef SUN4DMAHACK
	    if (segPtr->numElements >= 8) {
		VmMach_DMAFree(handlePtr->maxTransferSize, requestPtr->buffer);
	    }
#endif
	if (segPtr->requestActive == 0) {
	    segPtr->ioDone = TRUE;
	    Sync_MasterBroadcast(&segPtr->ioDoneWait);
	}
    } 
    MASTER_UNLOCK(&segPtr->ioMutex);
    if (requestPtr->bufferLen > 0) {
	status = Dev_BlockDeviceIO(handlePtr, requestPtr);
	if (status != SUCCESS) {
	    LfsError(segPtr->lfsPtr, status, "Can't start log write.\n");
	}
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WriteSegmentStart --
 *
 *	Start a segment write to the log.
 *
 * Results:
 *	SUCCESS if write complete. The ReturnStatus otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteSegmentStart(segPtr) 
    LfsSeg	*segPtr;	/* Segment to write. */
{
    Lfs	*lfsPtr = segPtr->lfsPtr;
    int offset;
    LfsDiskAddr  diskAddress;
    ReturnStatus status = SUCCESS;
    DevBlockDeviceHandle *handlePtr;
    DevBlockDeviceRequest *requestPtr;

    Sync_SemInitDynamic(&(segPtr->ioMutex),"LfsSegIoMutex");
    segPtr->ioDone = FALSE;
    segPtr->ioReturnStatus = SUCCESS;

    if (SegIsEmpty(segPtr)) { 
	/*
	 * The segment being written is empty so we don't have
	 * to write anytime. Just mark the I/O as done.
	 */
	segPtr->ioDone = TRUE;
	return SUCCESS;
    }

    handlePtr = (DevBlockDeviceHandle *) lfsPtr->devicePtr->data;

    /*
     * Writing to a segment nukes the segment cache.
     */
    if (lfsPtr->segCache.valid && 
	 (lfsPtr->segCache.segNum == segPtr->logRange.current)) {
	lfsPtr->segCache.valid = FALSE;
    }
    LFS_STATS_INC(lfsPtr->stats.log.segWrites);

    LFS_STATS_ADD(lfsPtr->stats.log.wasteBlocks,
			    segPtr->curDataBlockLimit - segPtr->curBlockOffset);
    /*
     * Compute the starting disk address of this I/O.
     */
    LfsSegNumToDiskAddress(lfsPtr, segPtr->logRange.current, &diskAddress);
    offset = LfsSegSizeInBlocks(lfsPtr)	- segPtr->curBlockOffset;
    LfsDiskAddrPlusOffset(diskAddress,offset, &segPtr->nextDiskAddress);


    /*
     * Fill in the request block fields that don't change between 
     * requests.
     */

    segPtr->bioreq[0].operation = segPtr->bioreq[1].operation = FS_WRITE;
    segPtr->bioreq[0].startAddrHigh = segPtr->bioreq[1].startAddrHigh = 0;
    segPtr->bioreq[0].doneProc = segPtr->bioreq[1].doneProc = SegIoDoneProc;
    segPtr->bioreq[0].clientData = segPtr->bioreq[1].clientData = 
				(ClientData) segPtr;

    segPtr->curElement = segPtr->numElements-1;
    segPtr->curBlockOffset = 0;
    /*
     * Start up the first two disk I/Os. 
     */
    requestPtr = segPtr->bioreq+0;
    requestPtr->startAddress = DEV_BYTES_PER_SECTOR * 
				LfsDiskAddrToOffset(segPtr->nextDiskAddress);
#ifdef SUN4DMAHACK
    if (segPtr->numElements >= 8) {
	requestPtr->buffer = VmMach_DMAAlloc(handlePtr->maxTransferSize,
					     lfsPtr->writeBuffers[0]);
    } else {
	requestPtr->buffer = lfsPtr->writeBuffers[0];
    }
#else
    requestPtr->buffer = lfsPtr->writeBuffers[0];
#endif
    requestPtr->bufferLen = 0;
    CopySegToBuffer(segPtr, handlePtr->maxTransferSize, requestPtr->buffer,
		&requestPtr->bufferLen);
    segPtr->nextDiskAddress += requestPtr->bufferLen/DEV_BYTES_PER_SECTOR;
    segPtr->requestActive = 1;

    /*
     * Disk request number 2.
     */
    requestPtr = segPtr->bioreq+1;
    requestPtr->startAddress = DEV_BYTES_PER_SECTOR * 
				LfsDiskAddrToOffset(segPtr->nextDiskAddress);
#ifdef SUN4DMAHACK
    if (segPtr->numElements >= 8) {
	requestPtr->buffer = VmMach_DMAAlloc(handlePtr->maxTransferSize,
					     lfsPtr->writeBuffers[1]);
    } else {
	requestPtr->buffer = lfsPtr->writeBuffers[1];
    }
#else
    requestPtr->buffer = lfsPtr->writeBuffers[1];
#endif 
    requestPtr->bufferLen = 0;
    CopySegToBuffer(segPtr, handlePtr->maxTransferSize, requestPtr->buffer,
			&requestPtr->bufferLen);
    segPtr->nextDiskAddress += requestPtr->bufferLen/DEV_BYTES_PER_SECTOR;
    if (requestPtr->bufferLen > 0) { 
	segPtr->requestActive = 2;
    } else {
	segPtr->requestActive = 1;
    }
    status = Dev_BlockDeviceIO(handlePtr, segPtr->bioreq);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't start disk log write.\n");
    }
    if (requestPtr->bufferLen > 0) {
	status = Dev_BlockDeviceIO(handlePtr, segPtr->bioreq+1);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't start disk log write.\n");
	}
    }


    LFS_STATS_ADD(lfsPtr->stats.log.blocksWritten, segPtr->numBlocks);
    LFS_STATS_ADD(lfsPtr->stats.log.bytesWritten, segPtr->activeBytes);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteSegmentFinish --
 *
 *	Wait for a segment write to finish.
 *
 * Results:
 *	SUCCESS if write complete. The ReturnStatus otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteSegmentFinish(segPtr) 
    LfsSeg	*segPtr;	/* Segment to wait for. */
{
    ReturnStatus status;
    MASTER_LOCK((&segPtr->ioMutex));
    while (segPtr->ioDone == FALSE) { 
	Sync_MasterWait((&segPtr->ioDoneWait),(&segPtr->ioMutex),FALSE);
    }
    status = segPtr->ioReturnStatus;
    MASTER_UNLOCK((&segPtr->ioMutex));

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * WriteDoneNotify --
 *
 *	Notify others that a log write has finished and another may be
 *	started.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
WriteDoneNotify(lfsPtr)
	Lfs	*lfsPtr;
{
    LOCK_MONITOR;
    lfsPtr->activeFlags &= ~LFS_WRITE_ACTIVE;
    Sync_Broadcast(&lfsPtr->writeWait);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateSegmentToWrite --
 *
 *	Create an LfsSeg structure describing an empty segment to be 
 *	filled in by the callback routines.
 *
 * Results:
 *	A pointer to a lfsSeg.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static LfsSeg *
CreateSegmentToWrite(lfsPtr, dontBlock) 
    Lfs	*lfsPtr;		/* For which file system. */
    Boolean	    dontBlock;	/* Don't wait for segment. */
{
    LfsSeg	*segPtr;
    LfsSegLogRange	segLogRange;
    int		startBlock;
    ReturnStatus status;

    LOCK_MONITOR;

    /*
     * Wait for previous writes to finish.
     */
    while (lfsPtr->activeFlags & LFS_WRITE_ACTIVE) {
	Sync_Wait(&lfsPtr->writeWait, FALSE);
    }

    do { 
	status = LfsGetLogTail(lfsPtr, dontBlock, &segLogRange, &startBlock);
	if ((status == FS_WOULD_BLOCK) && !dontBlock) {
	    LFS_STATS_INC(lfsPtr->stats.log.cleanSegWait);
	    Sync_Wait(&lfsPtr->cleanSegmentsWait, FALSE);
	} 
    } while ((status == FS_WOULD_BLOCK) && !dontBlock);

    if (status == SUCCESS) { 
	lfsPtr->activeFlags |= LFS_WRITE_ACTIVE;
	segPtr = GetSegStruct(lfsPtr, &segLogRange, startBlock, (char *) NIL);
    } else {
	segPtr = (LfsSeg *) NIL;
    }
    UNLOCK_MONITOR;
    return segPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * RewindCurPtrs --
 *
 *	Rewind the current pointers of a segment to the start of the first
 *	segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
RewindCurPtrs(segPtr)
    LfsSeg	*segPtr;
{
    segPtr->curSegSummaryPtr = (LfsSegSummary *) 
				(segPtr->segElementPtr[0].address);
    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *) 
				(segPtr->curSegSummaryPtr + 1);
    segPtr->curElement = 1;

    segPtr->curBlockOffset = segPtr->startBlockOffset+1;
    segPtr->curDataBlockLimit = segPtr->curSummaryHdrPtr->numDataBlocks;
    segPtr->curSummaryPtr = (char *) (segPtr->curSummaryHdrPtr + 1);
    segPtr->curSummaryLimitPtr = (char *) (segPtr->curSummaryHdrPtr) +
			segPtr->curSummaryHdrPtr->lengthInBytes;

}

/*
 *----------------------------------------------------------------------
 *
 * DestorySegStruct --
 *
 *	Destory an LfsSeg structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DestorySegStruct(segPtr)
    LfsSeg	*segPtr;	/* Segment to Destory. */
{

    int num;

    num = segPtr - segPtr->lfsPtr->segs;
    segPtr->lfsPtr->segsInUse  &= ~(1 << num);
}


/*
 *----------------------------------------------------------------------
 *
 * GetSegStruct --
 *
 *	Allocate an LfsSeg structure.
 *
 * Results:
 *	A lfsSeg structure
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static LfsSeg *
GetSegStruct(lfsPtr, segLogRangePtr, startBlockOffset, memPtr)
    Lfs	*lfsPtr;	/* File system. */
    LfsSegLogRange *segLogRangePtr; /* Log range of segment. */
    int		   startBlockOffset; /* Starting block offset into segment */
    char	   *memPtr;	     /* Memory allocated for segment. */
{
    int		i;
    LfsSeg	*segPtr;

    segPtr = (LfsSeg *) NIL;
    for (i = 0; i < LFS_NUM_PREALLOC_SEGS; i++) { 
	if (!(lfsPtr->segsInUse & (1 << i))) {
	    lfsPtr->segsInUse |= (1 << i);
	    segPtr = lfsPtr->segs + i;
	    break;
	}
    }
    if (segPtr == (LfsSeg *) NIL) {
	panic("GetSegStruct out of segment structures.\n");
    }
    segPtr->memPtr = memPtr;
    segPtr->logRange = *segLogRangePtr;
    segPtr->numElements = 0;
    segPtr->numBlocks = 0;
    segPtr->startBlockOffset = startBlockOffset;
    segPtr->activeBytes = 0;
    segPtr->curSegSummaryPtr = (LfsSegSummary *) NIL;
    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *) NIL;
    segPtr->curElement = -1;
    segPtr->curBlockOffset = segPtr->startBlockOffset;
    segPtr->curDataBlockLimit = LfsSegSizeInBlocks(lfsPtr);
    segPtr->curSummaryPtr = (char *) NIL;
    segPtr->curSummaryLimitPtr = (char *) NIL;

    return segPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * DoInCallBacks --
 *
 *	Perform the call backs that take segments as input.
 *
 * Results:
 *	TRUE if the segment is full. False otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
DoInCallBacks(type, segPtr, flags, sizePtr, numCacheBlocksPtr, clientDataPtr) 
    enum CallBackType type;	/* Type of segment operation. */
    LfsSeg	*segPtr;	/* Segment to fill in or out. */
    int		flags;		/* Flags used during checkpoint. */
    int	        *sizePtr; /* Size of checkpoint buffer or segment cleaned. */
    int *numCacheBlocksPtr;
    ClientData *clientDataPtr;

{
    int	moduleType, size, numCacheBlocks, next;
    Boolean error = FALSE;
    char	 *endSummaryBlockPtr;
    LfsSegElement *bufferPtr;

    /*
     * We're doing a pass over an existing segment such as during a
     * cleaning IN phase or a WRITE_DONE callback. Initialize the 
     * moduleType from the summary region. 
     */
    endSummaryBlockPtr = (char *)segPtr->curSegSummaryPtr + 
			segPtr->curSegSummaryPtr->size;
    while(!error) { 
	while (((char *)segPtr->curSummaryHdrPtr < endSummaryBlockPtr) &&
	        (segPtr->curSummaryHdrPtr->lengthInBytes > 0))  {
	    LfsSegIoInterface *intPtr;
    
	    moduleType = segPtr->curSummaryHdrPtr->moduleType;
	    segPtr->curSummaryLimitPtr = ((char *)(segPtr->curSummaryHdrPtr) + 
				segPtr->curSummaryHdrPtr->lengthInBytes);
	    segPtr->curSummaryPtr = (char *) (segPtr->curSummaryHdrPtr + 1);
	    intPtr = lfsSegIoInterfacePtrs[moduleType];
	    switch (type) {
	    case SEG_CLEAN_IN:
		size = 0;
		numCacheBlocks = 0;
		error = intPtr->clean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
#ifdef lint
		error = LfsDescMapClean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
		error = LfsSegUsageClean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
		error = LfsFileLayoutClean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
#endif /* lint */
		*sizePtr += size;
		*numCacheBlocksPtr += numCacheBlocks;
		break;
	    case SEG_WRITEDONE: {
		intPtr->writeDone(segPtr, flags, clientDataPtr + moduleType);
#ifdef lint
		LfsDescMapWriteDone(segPtr, flags,  clientDataPtr + moduleType);
		LfsSegUsageWriteDone(segPtr, flags, clientDataPtr + moduleType);
		LfsFileLayoutWriteDone(segPtr, flags, clientDataPtr + moduleType);
#endif /* lint */
		break;
	     }
	     default:
		 panic("lfsSeg.c: Bad case statement\n");
	     }
	     segPtr->curBlockOffset += segPtr->curSummaryHdrPtr->numDataBlocks;
	     /*
	      * Skip to the next summary header. 
	      */
	     segPtr->curSummaryHdrPtr = 
			(LfsSegSummaryHdr *) segPtr->curSummaryLimitPtr;
	 }
	 if (error) {
		break;
	 }
	 /*
	  * If we ran over the end of current summary block move on to 
	  * the next.
	  */
	  next = segPtr->curSegSummaryPtr->nextSummaryBlock;
	  if (type == SEG_WRITEDONE) {
		/*
		 * Free up any memory we allocated during the layout
		 */
		LFS_STATS_ADD(segPtr->lfsPtr->stats.log.summaryBytesWritten,
				segPtr->curSegSummaryPtr->size);
		LFS_STATS_INC(segPtr->lfsPtr->stats.log.summaryBlocksWritten);
		free((char *) segPtr->curSegSummaryPtr);
	  }
          if (next == -1) {
		/*
		 * No more summary bytes for this segment.
		 */
		break;
	   }
	   bufferPtr = LfsSegGetBufferPtr(segPtr);
	   segPtr->curSegSummaryPtr = (LfsSegSummary *) bufferPtr->address;
	   segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)
					    (segPtr->curSegSummaryPtr + 1);
	   endSummaryBlockPtr = (char *)segPtr->curSegSummaryPtr + 
			segPtr->curSegSummaryPtr->size;
	   bufferPtr++;
	   LfsSegSetBufferPtr(segPtr, bufferPtr);
	   segPtr->curBlockOffset = next;
    }
   return error;
}


/*
 *----------------------------------------------------------------------
 *
 * DoOutCallBacks --
 *
 *	Perform the call backs for output to segments.
 *
 * Results:
 *	TRUE if the segment is full. False otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
DoOutCallBacks(type, segPtr, flags, checkPointPtr, sizePtr, clientDataPtr) 
    enum CallBackType type;	/* Type of segment operation. */
    LfsSeg	*segPtr;	/* Segment to fill in or out. */
    int		flags;		/* Flags used during checkpoint. */
    char	*checkPointPtr; /* Checkpoint buffer. */
    int	        *sizePtr; /* Size of checkpoint buffer or segment cleaned. */
    ClientData *clientDataPtr;

{
    int	moduleType, startOffset;
    Boolean full;
    char	*summaryPtr, *endSummaryPtr;
    int		newStartBlockOffset;
    LfsCheckPointRegion	*segUsageCheckpointRegionPtr;

    full = FALSE;
    segUsageCheckpointRegionPtr = (LfsCheckPointRegion *) NIL;
    for(moduleType = 0; moduleType < LFS_MAX_NUM_MODS; ) {
	LfsSegIoInterface *intPtr = lfsSegIoInterfacePtrs[moduleType];
	/*
	 * Filling in a segment, be sure that there is enought
	 * room for the LfsSegSummaryHdr.
	 */
	summaryPtr = LfsSegSlowGrowSummary(segPtr, 1,
		sizeof(LfsSegSummaryHdr) + MIN_SUMMARY_REGION_SIZE, TRUE);
	if (summaryPtr == (char *) NIL) {
	    full = TRUE;
	    break;
	}
	segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)  summaryPtr;
	LfsSegSetSummaryPtr(segPtr, summaryPtr + sizeof(LfsSegSummaryHdr));
	startOffset = segPtr->curBlockOffset;
	switch (type) {
	case SEG_CLEAN_OUT:
	case SEG_LAYOUT: 
	    full = intPtr->layout(segPtr, flags, clientDataPtr + moduleType);
#ifdef lint
	    full = LfsSegUsageLayout(segPtr, flags, clientDataPtr + moduleType);
	    full = LfsDescMapLayout(segPtr, flags, clientDataPtr + moduleType);
	    full = LfsFileLayoutProc(segPtr, flags, clientDataPtr + moduleType);
#endif /* lint */
	    break;
	case SEG_CHECKPOINT: {
	    int	size;
	    LfsCheckPointRegion	*regionPtr;
	    regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	    size = 0;
	    full = intPtr->checkpoint(segPtr, flags, (char *)(regionPtr + 1),
				     &size, clientDataPtr + moduleType);
#ifdef lint
	    full = LfsDescMapCheckpoint(segPtr, flags, (char *)(regionPtr + 1),
				     &size, clientDataPtr + moduleType);
	    full = LfsSegUsageCheckpoint(segPtr, flags, (char *)(regionPtr + 1),
				     &size, clientDataPtr + moduleType);
	    full = LfsFileLayoutCheckpoint(segPtr, flags,
				(char *)(regionPtr + 1), &size,
				clientDataPtr + moduleType);
#endif /* lint */
	    if (size > 0) {
		if (moduleType == LFS_SEG_USAGE_MOD) {
			segUsageCheckpointRegionPtr = regionPtr;
		}
		regionPtr->type = moduleType;
		regionPtr->size = size + sizeof(LfsCheckPointRegion);
		*sizePtr += regionPtr->size;
		checkPointPtr += regionPtr->size;
	    }
	    break;
	 }
	 default:
	     panic("lfsSeg.c: Bad case statement\n");
	 }
	 /*
	  * If the callback added data to the segment, fill in the summary 
	  * header. 
	  */
	 endSummaryPtr = LfsSegGetSummaryPtr(segPtr); 
	 if ((startOffset != segPtr->curBlockOffset) ||
	     ((summaryPtr + sizeof(LfsSegSummaryHdr)) != endSummaryPtr)) {
	    segPtr->curSummaryHdrPtr->moduleType = moduleType;
	    segPtr->curSummaryHdrPtr->lengthInBytes = endSummaryPtr - 
			    (char *) summaryPtr;
	    segPtr->curSummaryHdrPtr->numDataBlocks =  
			    segPtr->curBlockOffset - startOffset;
	 } else {
	     LfsSegSetSummaryPtr(segPtr, summaryPtr);
	 }
	 /*
	  * If we didn't fill the segment in skip to the next module.
	  */
	 if (full) { 
	    if (LfsSegSummaryBytesLeft(segPtr) > MIN_SUMMARY_REGION_SIZE) {
		break;
	    }
	 } else {
	    moduleType++;
	 }
   }
   /*
    * Update the size of the last summary block and cap off this segment. 
    */
   segPtr->curSegSummaryPtr->size = segPtr->curSummaryPtr - 
					(char *) segPtr->curSegSummaryPtr;

   newStartBlockOffset = -1;
   if (!full) {
	if (SegIsEmpty(segPtr)) { 
	    /*
	     * The segment is totally empty.  We don't need to write
	     * this one yet.
	     */
	    LFS_STATS_INC(segPtr->lfsPtr->stats.log.emptyWrites);
	    newStartBlockOffset = segPtr->startBlockOffset;
	} else if ((segPtr->curDataBlockLimit -  segPtr->curBlockOffset) > 
		       segPtr->lfsPtr->superBlock.usageArray.wasteBlocks) { 
	    /*
	     * If this is considered to be a partial segment write add the
	     * summary block we needed.
	     */
	    AddNewSummaryBlock(segPtr);
	    newStartBlockOffset = segPtr->curBlockOffset-1;
	    LFS_STATS_INC(segPtr->lfsPtr->stats.log.partialWrites);
	}
   }
   LfsSetLogTail(segPtr->lfsPtr, &segPtr->logRange, newStartBlockOffset, 
					segPtr->activeBytes);  
   if (segUsageCheckpointRegionPtr != (LfsCheckPointRegion *) NIL) {
       LfsSegUsageCheckpointUpdate(segPtr->lfsPtr, 
			(char *) (segUsageCheckpointRegionPtr + 1),
			segUsageCheckpointRegionPtr->size - 
					sizeof(LfsCheckPointRegion));
    }

    return full;
}




/*
 *----------------------------------------------------------------------
 *
 * SegmentCleanProc --
 *
 *	Proc_CallFunc procedure for cleaning segments write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.	
 *
 *----------------------------------------------------------------------
 */


static void
SegmentCleanProc(clientData, callInfoPtr)
    ClientData	  clientData;	/* File system to clean blocks in */
    Proc_CallInfo *callInfoPtr;         /* Not used. */

{
    register Lfs *lfsPtr = (Lfs *) clientData;       
#define	MAX_NUM_TO_CLEAN 100
    int		numSegsToClean;
    LfsSegList	segs[MAX_NUM_TO_CLEAN];
    int		i, numCacheBlocksUsed, totalCacheBlocksUsed;
    LfsSeg	 *segPtr;
    Boolean	full;
    ReturnStatus	status;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			numWritten, numCleaned, totalSize, cacheBlocksReserved;
    Boolean		error;
    char		*memPtr;

    LfsMemReserve(lfsPtr, &cacheBlocksReserved, &memPtr);
    do { 
	for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	    clientDataArray[i] = (ClientData) NIL;
	}
	numSegsToClean = lfsPtr->superBlock.usageArray.numSegsToClean;
	if (numSegsToClean > MAX_NUM_TO_CLEAN) {
	    numSegsToClean = MAX_NUM_TO_CLEAN;
	}
	numSegsToClean = LfsGetSegsToClean(lfsPtr, 
			 cacheBlocksReserved, numSegsToClean, segs);

	if (numSegsToClean < 2) {
	    break;
	}
	LFS_STATS_INC(lfsPtr->stats.cleaning.getSegsRequests);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.segsToClean, numSegsToClean);
	if (lfsSegWriteDebug || TRUE) { 
	    printf("Cleaning started - %d segs\n", numSegsToClean);
	}
	/*
	 * Reading in segments to clean.
	 */
	totalSize = 0;
	totalCacheBlocksUsed = 0;
	numCleaned = 0;
	for (i = 0; (i < numSegsToClean) && 
		    (totalCacheBlocksUsed < cacheBlocksReserved); i++) { 
	    int size;
	    segPtr = CreateSegmentToClean(lfsPtr, segs[i].segNumber, memPtr);
	    size = 0;
	    numCacheBlocksUsed = 0;
	    error = DoInCallBacks(SEG_CLEAN_IN, segPtr, 0, &size,
			    &numCacheBlocksUsed, clientDataArray);
	    if (!error && (segs[i].activeBytes == 0) && (size != 0)) {
		printf("Warning: Segment %d cleaned found wrong active bytes %d != %d\n", segs[i].segNumber, size, segs[i].activeBytes);
	    }

	    DestorySegStruct(segPtr);
	    if (error) {
		LFS_STATS_ADD(lfsPtr->stats.cleaning.readErrors, 1);
		segs[i].segNumber = -1;
	    } else { 
		if (size == 0) {
		    LFS_STATS_INC(lfsPtr->stats.cleaning.readEmpty);
		}
		numCleaned++;
	    }
	    totalSize += size;
	    totalCacheBlocksUsed += numCacheBlocksUsed;
	}
	LFS_STATS_ADD(lfsPtr->stats.cleaning.segReads,numCleaned);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.bytesCleaned,totalSize);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.cacheBlocksUsed,
			totalCacheBlocksUsed);
	/*
	 * Write out segments cleaned.
	 */

	numWritten = 0;
	if (totalSize > 0) { 
	    full = TRUE;
	    while (full) {
		segPtr = CreateSegmentToWrite(lfsPtr, TRUE);
		if (segPtr == (LfsSeg *) NIL) {
		    LfsError(lfsPtr, FAILURE, "Ran out of clean segments during cleaning.\n");
		}
		full = DoOutCallBacks(SEG_CLEAN_OUT, segPtr, 
				LFS_CLEANING_LAYOUT, (char *) NIL,
				(int *) NIL, clientDataArray);

		status = WriteSegmentStart(segPtr);
		if (status == SUCCESS) {
		    status = WriteSegmentFinish(segPtr);
		}
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, "Can't write segment to log\n");
		}
		RewindCurPtrs(segPtr);
		(void) DoInCallBacks(SEG_WRITEDONE, segPtr, LFS_CLEANING_LAYOUT,
				(int *) NIL, (int *) NIL,  clientDataArray);
		WriteDoneNotify(lfsPtr);
		numWritten++;
		LFS_STATS_ADD(lfsPtr->stats.cleaning.blocksWritten, 
				segPtr->numBlocks);
		LFS_STATS_ADD(lfsPtr->stats.cleaning.bytesWritten, 
				segPtr->activeBytes);
		LFS_STATS_ADD(lfsPtr->stats.cleaning.segWrites, numWritten);
		DestorySegStruct(segPtr);
	    }
	}
	status = LfsCheckPointFileSystem(lfsPtr, 
			LFS_CHECKPOINT_NOSEG_WAIT|LFS_CHECKPOINT_CLEANER);
	if (status != SUCCESS) {
		LfsError(lfsPtr, status, "Can't checkpoint after cleaning.\n");
	}
	for (i = 0; i < numSegsToClean; i++) { 
	    if (segs[i].segNumber != -1) { 
		LfsMarkSegClean(lfsPtr, segs[i].segNumber);
	    }
	}
	if (numCleaned > 0) { 
	    LOCK_MONITOR;
	    Sync_Broadcast(&lfsPtr->cleanSegmentsWait);
	    UNLOCK_MONITOR;
	}
	if (lfsSegWriteDebug || TRUE) { 
	    printf("Cleaned %d segments in %d segments\n", numCleaned, 
			numWritten);
	}
    } while (numCleaned > 2);
    lfsPtr->segCache.valid = FALSE;
    LfsMemRelease(lfsPtr, cacheBlocksReserved, memPtr);
    LOCK_MONITOR;
    lfsPtr->activeFlags &= ~LFS_CLEANER_ACTIVE;
    if (lfsPtr->activeFlags & LFS_CHECKPOINTWAIT_ACTIVE) {
	UNLOCK_MONITOR;
	status = LfsCheckPointFileSystem(lfsPtr, 0);
    } else {
	UNLOCK_MONITOR;
    }


}


static LfsSeg *
CreateSegmentToClean(lfsPtr, segNumber, cleaningMemPtr)
    Lfs	*lfsPtr;	/* File system of segment. */
    int	segNumber;	/* Segment number to clean. */
    char *cleaningMemPtr; /* Memory to use for cleaning. */
{
    LfsSeg		*segPtr;
    int			segSize;
    ReturnStatus	status;
    LfsSegLogRange	logRange;
    LfsSegSummary	*segSumPtr;
    LfsDiskAddr		diskAddress;

    logRange.prevSeg = -1;
    logRange.current = segNumber;
    logRange.nextSeg = -1;

    /*
     * Get a LfsSeg structure.
     */
    lfsPtr->segCache.valid = FALSE;
    segPtr = GetSegStruct(lfsPtr, &logRange, 0, cleaningMemPtr);

    /*
     * Read in the segment in memory.
     */
    segSize = LfsSegSize(lfsPtr);

    LfsSegNumToDiskAddress(lfsPtr, segNumber, &diskAddress);
    status = LfsReadBytes(lfsPtr, diskAddress, segSize, cleaningMemPtr);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't read segment to clean.\n");
	return (LfsSeg *) NIL;
    }
    lfsPtr->segCache.segNum = segNumber;
    LfsSegNumToDiskAddress(lfsPtr, segNumber, 
		&lfsPtr->segCache.startDiskAddress);
    LfsSegNumToDiskAddress(lfsPtr, segNumber+1, 
		&lfsPtr->segCache.endDiskAddress);
    lfsPtr->segCache.memPtr = cleaningMemPtr;
    lfsPtr->segCache.valid = TRUE;

    segSumPtr = (LfsSegSummary *)
		(cleaningMemPtr + segSize - LfsBlockSize(lfsPtr));
    while (1) { 
	segPtr->segElementPtr[segPtr->numElements].lengthInBlocks = 0;
	segPtr->segElementPtr[segPtr->numElements].address = (char *) segSumPtr;
	segPtr->segElementPtr[segPtr->numElements].clientData = 
				(ClientData) NIL;
	segPtr->numElements++;

	LFS_STATS_INC(lfsPtr->stats.cleaning.summaryBlocksRead);
	if (segSumPtr->nextSummaryBlock == -1) {
		break;
	}
	segSumPtr = (LfsSegSummary *) 
			LfsSegFetchBytes(segPtr, segSumPtr->nextSummaryBlock,
				LfsBlockSize(lfsPtr));
    }
    RewindCurPtrs(segPtr);
    return segPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegAttach --
 *
 *	Call the attach routines all the segment I/O modules.
 *
 * Results:
 *	SUCCESS if everything when well.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsSegAttach(lfsPtr, checkPointPtr, checkPointSize)
    Lfs		*lfsPtr;	/* File system being attached. */
    char        *checkPointPtr; /* Latest checkpoint header. */
    int		checkPointSize; /* Size of the checkpoint buffer. */
{
    int		moduleType;
    char	*limitPtr;
    LfsCheckPointRegion *regionPtr;
    ReturnStatus	status;
    LfsSegIoInterface	*segIoPtr;

    lfsPtr->segCache.valid = FALSE;

    limitPtr = checkPointPtr + checkPointSize;
    for (moduleType = 0; moduleType < LFS_MAX_NUM_MODS; moduleType++) {
	regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	segIoPtr = lfsSegIoInterfacePtrs[moduleType];
	if ((checkPointPtr < limitPtr) && (regionPtr->type == moduleType)) { 
	    checkPointPtr += regionPtr->size;
	    status = segIoPtr->attach(lfsPtr, 
				(int)(regionPtr->size - sizeof(*regionPtr)),
				(char *) (regionPtr+1));
#ifdef lint
	    status = LfsDescMapAttach(lfsPtr, 
				(int)(regionPtr->size - sizeof(*regionPtr)),
				(char *) (regionPtr+1));
	    status = LfsFileLayoutAttach(lfsPtr, 
				(int)(regionPtr->size - sizeof(*regionPtr)),
				(char *) (regionPtr+1));
	    status = LfsSegUsageAttach(lfsPtr, 
				(int)(regionPtr->size - sizeof(*regionPtr)),
				(char *) (regionPtr+1));

#endif /* lint */
	} else {
	    status = segIoPtr->attach(lfsPtr, 0, (char *)NIL);
#ifdef lint
	    status = LfsDescMapAttach(lfsPtr, 0, (char *)NIL);
	    status = LfsFileLayoutAttach(lfsPtr, 0, (char *)NIL);
	    status = LfsSegUsageAttach(lfsPtr, 0, (char *)NIL);
#endif /* lint */
	}
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't attach module");
	    break;
	}
    }
    if (status == SUCCESS) {
	InitSegmentMem(lfsPtr);
	return status;
    }
    /*
     * XXX - Back out of error here by shutting down all module.
     */
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegCheckPoint --
 *
 *	Call the checkpoint routines all the segment I/O modules.
 *
 * Results:
 *	SUCCESS if everything when well.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsSegCheckPoint(lfsPtr, flags, checkPointPtr, checkPointSizePtr)
    Lfs	  	    *lfsPtr;		/* File system to checkpoint. */
    int		    flags;		/* Flags to checkpoint operation. */
    char	    *checkPointPtr;	/* Checkpoint area to be filled in.*/
    int		    *checkPointSizePtr; /* Size of checkpoint buffer. */
{
    Boolean	full = TRUE;
    LfsSeg	*segPtr;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			i;
    ReturnStatus	status= SUCCESS;

    LOCK_MONITOR;

    /*
     * Wait for the cleaner not to be active unless we are called from the 
     * cleaner, the detach code, or the timer callback.
     */
    if (lfsPtr->activeFlags & LFS_CLEANER_ACTIVE) {
	if (flags & LFS_CHECKPOINT_TIMER) {
	    /*
	     * Since the cleaner does checkpoints frequently, we can safely
	     * ignore TIMER checkpoints.
	     */
	    UNLOCK_MONITOR;
	    return GEN_EINTR;
        }
	if (!(flags & (LFS_CHECKPOINT_DETACH|LFS_CHECKPOINT_CLEANER))) {
	    /*
	     * Unless the checkpoint is from the cleaner or the detach 
	     * code we simply wait for a checkpoint to complete and 
	     * return.
	     */
	    if (!(lfsPtr->activeFlags & LFS_CHECKPOINTWAIT_ACTIVE)) {
		lfsPtr->activeFlags |= LFS_CHECKPOINTWAIT_ACTIVE;
	    }
	    while (lfsPtr->activeFlags & LFS_CHECKPOINTWAIT_ACTIVE) { 
		Sync_Wait(&lfsPtr->checkPointWait, FALSE);
	    }
	    UNLOCK_MONITOR;
	    return SUCCESS;
	} 
    }
    /*
     * Wait for any currently running checkpoint to finish.
     * TIMER checkpoints again can be ignored and cleaner checkpoint can
     * run in parallel.
     */
    if (!(flags & LFS_CHECKPOINT_CLEANER)) { 
	while (lfsPtr->activeFlags & LFS_CHECKPOINT_ACTIVE) {
	    if (flags & LFS_CHECKPOINT_TIMER) {
		UNLOCK_MONITOR;
		return GEN_EINTR;
	    }
	    Sync_Wait(&lfsPtr->checkPointWait, FALSE);
	}
	lfsPtr->activeFlags |= LFS_SYNC_CHECKPOINT_ACTIVE;
    } else {
	lfsPtr->activeFlags |= LFS_CLEANER_CHECKPOINT_ACTIVE;
    }

    /*
     * Wait for any directory log operations to finish.
     */
    while (lfsPtr->dirModsActive > 0) {
	Sync_Wait(&lfsPtr->checkPointWait, FALSE);
    }
    /*
     * If we a detatching the file system wait for any cleaners to exit.
     */
    if (flags & LFS_CHECKPOINT_DETACH) {
	lfsPtr->activeFlags |= LFS_SHUTDOWN_ACTIVE;
	while (lfsPtr->activeFlags & LFS_CLEANER_ACTIVE) {
	    Time time;
	    time.seconds = 1;
	    time.microseconds = 0;
	    Sync_WaitTime(time);
	}
    }
    LFS_STATS_INC(lfsPtr->stats.checkpoint.count);
    UNLOCK_MONITOR;

    if (flags & LFS_CHECKPOINT_DETACH) {
	LfsStopWriteBack(lfsPtr);
    }

    for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	clientDataArray[i] = (ClientData) NIL;
    }
    while (full) {
	segPtr = CreateSegmentToWrite(lfsPtr, 
			((flags & LFS_CHECKPOINT_NOSEG_WAIT) != 0));
	if (segPtr == (LfsSeg *) NIL) {
	    LfsError(lfsPtr, FAILURE, "Ran out of clean segments during cleaner checkpoint.\n");
	}
	*checkPointSizePtr = 0;
	full = DoOutCallBacks(SEG_CHECKPOINT, segPtr, flags,
			    checkPointPtr, checkPointSizePtr, clientDataArray);
	LFS_STATS_INC(lfsPtr->stats.checkpoint.segWrites);
	LFS_STATS_ADD(lfsPtr->stats.checkpoint.blocksWritten,
					segPtr->numBlocks);
	LFS_STATS_ADD(lfsPtr->stats.checkpoint.bytesWritten,
				segPtr->activeBytes);
	status = WriteSegmentStart(segPtr);
	if (status == SUCCESS) {
	    status = WriteSegmentFinish(segPtr);
	}
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't write segment to log\n");
	}
	RewindCurPtrs(segPtr);
	(void) DoInCallBacks(SEG_WRITEDONE, segPtr, flags, (int *) NIL, (int *) NIL,
				clientDataArray);
	WriteDoneNotify(lfsPtr);
        DestorySegStruct(segPtr);
    }
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegCheckPointDone --
 *
 *	Mark a checkpoint as done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegCheckPointDone(lfsPtr, flags)
    Lfs	*lfsPtr;
    int	flags;
{
    LOCK_MONITOR;
#ifdef ERROR_CHECK
    { 
	int numBlocks, numDirty;
	Fscache_CountBlocks(rpc_SpriteID, lfsPtr->domainPtr->domainNumber,
				&numBlocks, &numDirty);
	if (((flags & LFS_CHECKPOINT_DETACH) && numDirty) || (numDirty > 1)) {
		printf("DirtyBlocks (%d) after a checkpoint\n", numDirty);
	}
    }
#endif
    if (flags & LFS_CHECKPOINT_CLEANER) {
	lfsPtr->activeFlags &= 
		~(LFS_CLEANER_CHECKPOINT_ACTIVE|LFS_CHECKPOINTWAIT_ACTIVE);
    } else { 
	lfsPtr->activeFlags &= 
		~(LFS_SYNC_CHECKPOINT_ACTIVE|LFS_CHECKPOINTWAIT_ACTIVE);
    }
    Sync_Broadcast(&lfsPtr->checkPointWait);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSegDetach --
 *
 *	Call a shutdown time on an LFS file system.
 *
 * Results:
 *	SUCCESS if everything when well.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsSegDetach(lfsPtr)
    Lfs	  	    *lfsPtr;		/* File system to checkpoint. */
{

    FreeSegmentMem(lfsPtr);
    return SUCCESS;

}

void
LfsWaitForCheckPoint(lfsPtr) 
    Lfs	*lfsPtr;
{
    LOCK_MONITOR;
    while (lfsPtr->activeFlags & LFS_CHECKPOINT_ACTIVE) {
	Sync_Wait(&lfsPtr->checkPointWait, FALSE);
    }
    UNLOCK_MONITOR;
}

