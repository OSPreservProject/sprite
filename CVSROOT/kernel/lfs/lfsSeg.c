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

#include "lfs.h"
#include "lfsSeg.h"
#include "stdlib.h"
#include "sync.h"

/*
 * For lint.
 */

#include "lfsFileLayout.h"
#include "lfsSegUsageInt.h"
#include "lfsDescMap.h"

Boolean	lfsSegWriteDebug = FALSE;

#define	MIN_SUMMARY_REGION_SIZE	16

#define	LOCKPTR	&lfsPtr->lfsLock

enum CallBackType { SEG_LAYOUT, SEG_CLEAN_IN, SEG_CLEAN_OUT, SEG_CHECKPOINT, 
		   SEG_WRITEDONE};

LfsSegIoInterface *lfsSegIoInterfacePtrs[LFS_MAX_NUM_MODS];

static void SegmentWriteProc();
static void SegmentCleanProc();
static LfsSeg	*CreateSegmentToClean();
static LfsSeg   *GetSegStruct();
static void  AddSummaryBlock();

/*
 *----------------------------------------------------------------------
 *
 * -- LfsSegIoRegister
 *
 *	Register with the segment module an interface to objects to the log.   
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
 * Lfs_StartWriteBack --
 *
 *	Request that the segment manager start the segment write sequence
 *	for the specified file system.
 *
 * Results:
 *	True is a backend backend was started.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Lfs_StartWriteBack(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    Lfs	 *lfsPtr;	/* File system with data to write. */
    Boolean	retVal;

    lfsPtr = (Lfs *) (cacheInfoPtr->backendPtr->clientData);

    LFS_STATS_INC(lfsPtr->stats.backend.startRequests);
    LOCK_MONITOR;
    if (lfsPtr->writeBackActive) {
	LFS_STATS_INC(lfsPtr->stats.backend.alreadyActive);
	lfsPtr->checkForMoreWork = TRUE;
	UNLOCK_MONITOR;
	return FALSE;
    }
    lfsPtr->writeBackActive = TRUE;
    Proc_CallFunc(SegmentWriteProc, (ClientData) lfsPtr, 0);
    UNLOCK_MONITOR;
    return TRUE;

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
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegCleanStart(lfsPtr)
    Lfs	 *lfsPtr;	/* File system needing segments cleaned. */
{
    LOCK_MONITOR;
    LFS_STATS_INC(lfsPtr->stats.cleaning.startRequests);
    if (lfsPtr->cleanActive) {
	LFS_STATS_INC(lfsPtr->stats.cleaning.alreadyActive);
	UNLOCK_MONITOR;
	return;
    }
    lfsPtr->cleanActive = TRUE;
    Proc_CallFunc(SegmentCleanProc, (ClientData) lfsPtr, 0);
    UNLOCK_MONITOR;
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
    LfsSegElement *sumBufferPtr;
    int	 sumBytesLeft, blocksLeft, sumBlocks, sumBytes;

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
    AddSummaryBlock(segPtr);
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

int
LfsSegSlowDiskAddress(segPtr, segElementPtr)
    register LfsSeg	*segPtr; 	/* Segment of interest. */
    LfsSegElement *segElementPtr; /* Segment element of interest. */
{
    int	elementNumber, blockOffset, diskAddress;
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
    diskAddress = LfsSegNumToDiskAddress(segPtr->lfsPtr, 
				segPtr->logRange.current);
    return diskAddress + LfsSegSizeInBlocks(segPtr->lfsPtr) - blockOffset;
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
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
InitSegmentMem(lfsPtr)
    Lfs	*lfsPtr;
{
    int	memSize;

    memSize = LfsBytesToBlocks(lfsPtr,LfsSegSize(lfsPtr)) * 
				    sizeof(LfsSegElement);
    lfsPtr->segMemoryPtr = malloc(memSize + sizeof(LfsSeg) +LfsSegSize(lfsPtr));
    lfsPtr->segMemInUse = FALSE;
    lfsPtr->cleaningMemPtr = lfsPtr->segMemoryPtr + memSize + sizeof(LfsSeg);
}

/*
 *----------------------------------------------------------------------
 *
 * AddSummaryBlock --
 *
 *  Add a summary block to a segment.
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
AddSummaryBlock(segPtr)
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
	segPtr->curSegSummaryPtr->size = segPtr->curSummaryPtr - 
				    (char *) (segPtr->curSegSummaryPtr);
	segPtr->curSegSummaryPtr->nextSummaryBlock = segPtr->curBlockOffset;
    } 
    segPtr->curSegSummaryPtr = newSummaryPtr;

    segPtr->curSummaryPtr = sumBufferPtr->address + sizeof(LfsSegSummary);
    segPtr->curSummaryLimitPtr = sumBufferPtr->address + sumBytes;
}
#define SUN4HACK


/*
 *----------------------------------------------------------------------
 *
 * WriteSegment --
 *
 * Results:
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteSegment(segPtr, full) 
    LfsSeg	*segPtr;	/* Segment to write. */
    Boolean	full;		/* TRUE if the segment is full. */
{
    Lfs	*lfsPtr = segPtr->lfsPtr;
    int element, offset, diskAddress;
    ReturnStatus status;
#ifdef SUN4HACK
    char	*buffer;
    static char	*dmaBuffer = (char *) NIL; 
    if (dmaBuffer == (char *) NIL) {
	dmaBuffer = malloc((int)LfsSegSize(lfsPtr));
    }
#endif

    if (lfsPtr->segCache.valid && 
	 (lfsPtr->segCache.segNum == segPtr->logRange.current)) {
	lfsPtr->segCache.valid = FALSE;
    }
    LFS_STATS_INC(lfsPtr->stats.log.segWrites);
    /*
     * Update the size of the last summary block.
     */
    segPtr->curSegSummaryPtr->size = segPtr->curSummaryPtr - 
					(char *) segPtr->curSegSummaryPtr;

    lfsPtr->activeBlockOffset = -1;
    if (!full) {
	if ((segPtr->numBlocks == 1) && 
	    (segPtr->curSegSummaryPtr->size == sizeof(LfsSegSummary))) {
	    /*
	     * The segment is totally empty.  We don't need to write
	     * this one yet.
	     */
	    LFS_STATS_INC(lfsPtr->stats.log.emptyWrites);
	    lfsPtr->activeLogRange = segPtr->logRange;
	    lfsPtr->activeBlockOffset = segPtr->startBlockOffset;
	    return SUCCESS;
	} else if ((segPtr->curDataBlockLimit -  segPtr->curBlockOffset) > 
			       lfsPtr->superBlock.usageArray.wasteBlocks) { 
	    /*
	     * If this is considered to be a partial segment write add the
	     * summary block we needed.
	     */
	    AddSummaryBlock(segPtr);
	    lfsPtr->activeLogRange = segPtr->logRange;
	    lfsPtr->activeBlockOffset = segPtr->curBlockOffset-1;
	    LFS_STATS_INC(lfsPtr->stats.log.partialWrites);
	}
    }
    LFS_STATS_ADD(lfsPtr->stats.log.wasteBlocks,
			segPtr->curDataBlockLimit -  segPtr->curBlockOffset);
    diskAddress = LfsSegNumToDiskAddress(lfsPtr, segPtr->logRange.current);
    offset = LfsSegSizeInBlocks(lfsPtr)	- segPtr->curBlockOffset;
#ifdef SUN4HACK
    buffer = dmaBuffer;
    for (element = segPtr->numElements-1; element >= 0; element--) {
	int	bytes = 
	    LfsBlocksToBytes(lfsPtr, 
			segPtr->segElementPtr[element].lengthInBlocks);
	bcopy(segPtr->segElementPtr[element].address, buffer, bytes);
	buffer += bytes;
    }
    status = LfsWriteBytes(lfsPtr,  diskAddress + offset,
			buffer - dmaBuffer, dmaBuffer);
    if (lfsSegWriteDebug) { 
	printf("LfsSeg wrote segment %d at %d - %d bytes.\n",
		segPtr->logRange.current, diskAddress + offset,
				buffer - dmaBuffer);
    }
#else
    for (element = segPtr->numElements-1; element >= 0; element--) {
	LfsSegElement *elPtr = segPtr->segElementPtr[element];
	status = LfsWriteBytes(lfsPtr, diskAddress + offset, 
			LfsBlocksToBytes(lfsPtr, elPtr->lengthInBlocks), 
				elPtr->address);
	if (status != SUCCESS) {
	    break;
	}
	offset += elPtr->lengthInBlocks;
    }
#endif
    LFS_STATS_ADD(lfsPtr->stats.log.blocksWritten, segPtr->numBlocks);
    LFS_STATS_ADD(lfsPtr->stats.log.bytesWritten, segPtr->activeBytes);
    LfsSetSegUsage(lfsPtr, segPtr->logRange.current, segPtr->activeBytes);
    return status;
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

    /*
     * See if we current have a current segment that is not full that we 
     * can use. Otherwise we get a clean segment.
     */
    if (lfsPtr->activeBlockOffset == -1) { 
	startBlock = 0;
	do { 
	    LFS_STATS_INC(lfsPtr->stats.log.newSegments);
	    status = LfsGetCleanSeg(lfsPtr, &segLogRange, dontBlock);
	    if ((status == FS_WOULD_BLOCK) && !dontBlock) {
		LFS_STATS_INC(lfsPtr->stats.log.cleanSegWait);
		LfsWaitDomain(lfsPtr, &lfsPtr->cleanSegments);
		continue;
	    }
	    if (status != SUCCESS) {
		LfsError(lfsPtr, status, 
		   "Can't get clean segments to write for cleaning.\n");
	    }
	} while (status != SUCCESS);
    } else {
	LFS_STATS_INC(lfsPtr->stats.log.useOldSegment);
	segLogRange = lfsPtr->activeLogRange;
	startBlock = lfsPtr->activeBlockOffset;
    }
    segPtr = GetSegStruct(lfsPtr, &segLogRange, startBlock);
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

    if ((char *) segPtr == segPtr->lfsPtr->segMemoryPtr) {
	segPtr->lfsPtr->segMemInUse = FALSE;
	return;
    }
    free((char *) segPtr);
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
GetSegStruct(lfsPtr, segLogRangePtr, startBlockOffset)
    Lfs	*lfsPtr;	/* File system. */
    LfsSegLogRange *segLogRangePtr; /* Log range of segment. */
    int		   startBlockOffset; /* Starting block offset into segment */
{
    char	*memPtr;
    LfsSeg	*segPtr;
    /*
     * Alloc an LfsSeg structure. Be sure to leave enought room for default 
     * size LfsSegElement.
     */
    if (lfsPtr->segMemInUse) {
	 int	 memSize;

	 memSize = LfsSegSizeInBlocks(lfsPtr) * 
					sizeof(LfsSegElement);
	 memPtr = malloc(memSize + sizeof(LfsSeg));
    } else {
	 lfsPtr->segMemInUse = TRUE;
	 memPtr = lfsPtr->segMemoryPtr;
    }
    segPtr = (LfsSeg *) memPtr;
    segPtr->lfsPtr = lfsPtr;
    segPtr->segMemPtr = memPtr;
    segPtr->segElementPtr = (LfsSegElement *) (memPtr + sizeof(LfsSeg));
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
    int	moduleType, startOffset, size, numCacheBlocks, next;
    Boolean full = FALSE;
    char	*summaryPtr, *endSummaryBlockPtr;
    LfsSegElement *bufferPtr;

    /*
     * We're doing a pass over an existing segment such as during a
     * cleaning IN phase or a WRITE_DONE callback. Initialize the 
     * moduleType from the summary region. 
     */
    endSummaryBlockPtr = (char *)segPtr->curSegSummaryPtr + 
			segPtr->curSegSummaryPtr->size;
    while(1) { 
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
		full = intPtr->clean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
#ifdef lint
		full = LfsDescMapClean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
		full = LfsSegUsageClean(segPtr, &size, &numCacheBlocks, 
			    clientDataPtr + moduleType);
		full = LfsFileLayoutClean(segPtr, &size, &numCacheBlocks, 
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
   return full;
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
    int	moduleType, startOffset, size, numCacheBlocks;
    Boolean full;
    char	*summaryPtr, *endSummaryPtr;

    full = FALSE;
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
	    full = intPtr->layout(segPtr, (type == SEG_CLEAN_OUT),
			clientDataPtr + moduleType);
#ifdef lint
	    full = LfsSegUsageLayout(segPtr, (type == SEG_CLEAN_OUT),
				clientDataPtr + moduleType);
	    full = LfsDescMapLayout(segPtr, (type == SEG_CLEAN_OUT),
				clientDataPtr + moduleType);
	    full = LfsFileLayoutProc(segPtr, (type == SEG_CLEAN_OUT),
				clientDataPtr + moduleType);
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
	  * If we didn't fill the segment in skip do the next module.
	  */
	 if (full) { 
	    if (LfsSegSummaryBytesLeft(segPtr) > MIN_SUMMARY_REGION_SIZE) {
		break;
	    }
	 } else {
	    moduleType++;
	 }
   }
   return full;
}



/*
 *----------------------------------------------------------------------
 *
 * SegmentWriteProc --
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
static void
SegmentWriteProc(lfsPtr, callInfoPtr)
    register Lfs *lfsPtr;            /* File system with dirty blocks. */
    Proc_CallInfo *callInfoPtr;         /* Not used. */
{
    Boolean	full;
    LfsSeg	*segPtr;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			i;
    ReturnStatus	status;
again:
    full = TRUE;
    for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	clientDataArray[i] = (ClientData) NIL;
    }
    LfsLockDomain(lfsPtr);
    while (full) {
	segPtr = CreateSegmentToWrite(lfsPtr, FALSE);
	full = DoOutCallBacks(SEG_LAYOUT, segPtr, 0, (char *) NIL,
				(int *) NIL, clientDataArray);
	status = WriteSegment(segPtr, full);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't write segment to log\n");
	}
	RewindCurPtrs(segPtr);
	(void) DoInCallBacks(SEG_WRITEDONE, segPtr, 0,
				(char *) NIL, (int *) NIL, clientDataArray);
	DestorySegStruct(segPtr);
    }
    LfsUnLockDomain(lfsPtr, (Sync_Condition *) NIL);
    LOCK_MONITOR;
    if (lfsPtr->checkForMoreWork) {
	lfsPtr->checkForMoreWork = FALSE;
	UNLOCK_MONITOR;
	goto again;
    }
    lfsPtr->writeBackActive = FALSE;
    UNLOCK_MONITOR;
    FscacheBackendIdle(lfsPtr->domainPtr->backendPtr);
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
SegmentCleanProc(lfsPtr, callInfoPtr)
    register Lfs *lfsPtr;            /* File system to clean blocks in */
    Proc_CallInfo *callInfoPtr;         /* Not used. */

{
#define	MAX_NUM_TO_CLEAN 100
    int		numSegsToClean;
    int		segNums[MAX_NUM_TO_CLEAN];
    int		i, numCacheBlocksUsed, totalCacheBlocksUsed;
    LfsSeg	 *segPtr;
    Boolean	full;
    LfsSegLogRange	segLogRange;
    ReturnStatus	status;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			numWritten, numCleaned, totalSize, cleanBlocks;
    Boolean		error;
    Time      		startTime, endTime;

    do { 
	for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	    clientDataArray[i] = (ClientData) NIL;
	}
	Timer_GetTimeOfDay(&startTime, (int *) NIL, (Boolean *) NIL);
	numSegsToClean = LfsGetSegsToClean(lfsPtr, 
			    lfsPtr->cleanBlocks, MAX_NUM_TO_CLEAN, segNums);

	if (numSegsToClean < 2) {
	    break;
	}
	LFS_STATS_INC(lfsPtr->stats.cleaning.getSegsRequests);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.segsToClean, numSegsToClean);
	if (lfsSegWriteDebug) { 
	    printf("Cleaning started\n", numSegsToClean);
	}
	/*
	 * Reading in segments to clean.
	 */
	totalSize = 0;
	totalCacheBlocksUsed = 0;
	for (i = 0; (i < numSegsToClean) && 
		    (totalCacheBlocksUsed < lfsPtr->cleanBlocks);i++) { 
	    int size;
	    segPtr = CreateSegmentToClean(lfsPtr, segNums[i]);
	    size = 0;
	    numCacheBlocksUsed = 0;
	    error = DoInCallBacks(SEG_CLEAN_IN, segPtr, 0, &size,
			    &numCacheBlocksUsed, clientDataArray);
	    DestorySegStruct(segPtr);
	    if (error) {
		LFS_STATS_ADD(lfsPtr->stats.cleaning.readErrors, 1);
		segNums[i] = -1;
	    } else if (size == 0) {
		LFS_STATS_INC(lfsPtr->stats.cleaning.readEmpty);
		LfsMarkSegClean(lfsPtr, segNums[i]);
		segNums[i] = -1;
		LfsNotify(lfsPtr, &lfsPtr->cleanSegments);
	    }
	    totalSize += size;
	    totalCacheBlocksUsed += numCacheBlocksUsed;
	}
	numCleaned = i;
	LFS_STATS_ADD(lfsPtr->stats.cleaning.segReads,numCleaned);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.bytesCleaned,totalSize);
	LFS_STATS_ADD(lfsPtr->stats.cleaning.cacheBlocksUsed,
			totalCacheBlocksUsed);
	/*
	 * Write out segments cleaned.
	 */
	numWritten = 0;
	LfsLockDomain(lfsPtr);
	if (totalSize > 0) { 
	    full = TRUE;
	    while (full) {
		segPtr = CreateSegmentToWrite(lfsPtr, TRUE);
		full = DoOutCallBacks(SEG_CLEAN_OUT, segPtr, 0, (char *) NIL,
				(int *) NIL, clientDataArray);

		status = WriteSegment(segPtr, full);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, "Can't write segment to log\n");
		}
		RewindCurPtrs(segPtr);
		(void) DoInCallBacks(SEG_WRITEDONE, segPtr, 0,
				(int *) NIL, (int *) NIL,  clientDataArray);
		numWritten++;
		LFS_STATS_ADD(lfsPtr->stats.cleaning.blocksWritten, 
				segPtr->numBlocks);
		LFS_STATS_ADD(lfsPtr->stats.cleaning.bytesWritten, 
				segPtr->activeBytes);
		LFS_STATS_ADD(lfsPtr->stats.cleaning.segWrites, numWritten);
		DestorySegStruct(segPtr);
	    }
	}

	for (i = 0; i < numCleaned; i++) { 
	    if (segNums[i] != -1) { 
		LfsMarkSegClean(lfsPtr, segNums[i]);
	    }
	}
	LfsUnLockDomain(lfsPtr, &lfsPtr->cleanSegments);
	Timer_GetTimeOfDay(&endTime, (int *) NIL, (Boolean *) NIL);
	if (lfsSegWriteDebug) { 
	printf("Cleaned %d segments in %d segments- time (%d,%d) -  (%d,%d)\n",
	    numCleaned, numWritten, startTime.seconds, startTime.microseconds,
	    endTime.seconds, endTime.microseconds);
	}
    } while (numSegsToClean > 2);
    lfsPtr->cleanActive = FALSE;
}


static LfsSeg *
CreateSegmentToClean(lfsPtr, segNumber)
    Lfs	*lfsPtr;	/* File system of segment. */
    int	segNumber;	/* Segment number to clean. */
{
    LfsSeg		*segPtr;
    LfsSegElement *segElementPtr;
    int			segSize;
    ReturnStatus	status;
    LfsSegLogRange	logRange;
    LfsSegSummary	*segSumPtr;

    logRange.prevSeg = -1;
    logRange.current = segNumber;
    logRange.nextSeg = -1;
    /*
     * Get a LfsSeg structure.
     */

    segPtr = GetSegStruct(lfsPtr, &logRange, 0);

    /*
     * Read in the segment in memory.
     */
    segSize = LfsSegSize(lfsPtr);

    status = LfsReadBytes(lfsPtr,
			(int)LfsSegNumToDiskAddress(lfsPtr, segNumber), 
				segSize, lfsPtr->cleaningMemPtr);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't read segment to clean.\n");
	return (LfsSeg *) NIL;
    }
    lfsPtr->segCache.segNum = segNumber;
    lfsPtr->segCache.startDiskAddress = 
			LfsSegNumToDiskAddress(lfsPtr, segNumber);
    lfsPtr->segCache.endDiskAddress = 
			LfsSegNumToDiskAddress(lfsPtr, segNumber+1);
    lfsPtr->segCache.memPtr = lfsPtr->cleaningMemPtr;
    lfsPtr->segCache.valid = TRUE;

    segSumPtr = (LfsSegSummary *)
		(lfsPtr->cleaningMemPtr + segSize - LfsBlockSize(lfsPtr));
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
     * XXX - Back out of error here but shutting down all module.
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
    LfsSegLogRange	segLogRange;
    ClientData		clientDataArray[LFS_MAX_NUM_MODS];
    int			i;
    ReturnStatus	status= SUCCESS;

    *checkPointSizePtr = 0;
    for (i = 0; i < LFS_MAX_NUM_MODS; i++) {
	clientDataArray[i] = (ClientData) NIL;
    }
    LFS_STATS_INC(lfsPtr->stats.checkpoint.count);
    LfsLockDomain(lfsPtr);
    while (full) {
	segPtr = CreateSegmentToWrite(lfsPtr, FALSE);
	full = DoOutCallBacks(SEG_CHECKPOINT, segPtr, flags,
			    checkPointPtr, checkPointSizePtr, clientDataArray);
	LFS_STATS_INC(lfsPtr->stats.checkpoint.segWrites);
	LFS_STATS_ADD(lfsPtr->stats.checkpoint.blocksWritten,
					segPtr->numBlocks);
	LFS_STATS_ADD(lfsPtr->stats.checkpoint.bytesWritten,
				segPtr->activeBytes);
	status = WriteSegment(segPtr, full);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't write segment to log\n");
	}
	RewindCurPtrs(segPtr);
	(void) DoInCallBacks(SEG_WRITEDONE, segPtr, 0, (int *) NIL, (int *) NIL,
				clientDataArray);
        DestorySegStruct(segPtr);
    }
    lfsPtr->dirty = FALSE;
    LfsUnLockDomain(lfsPtr, (Sync_Condition *) NIL);
    return status;

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
    free(lfsPtr->segMemoryPtr);
    return SUCCESS;

}

/*ARGSUSED*/
Boolean
LfsSegNullLayout(segPtr, cleaning)
    LfsSeg	*segPtr;
    Boolean 	cleaning;
{
	return FALSE;
}

LfsLockDomain(lfsPtr)
    Lfs	*lfsPtr;
{
    LOCK_MONITOR;
    LFS_STATS_INC(lfsPtr->stats.log.locks);
    while (lfsPtr->locked) {
	LFS_STATS_INC(lfsPtr->stats.log.lockWaits);
	Sync_Wait(&lfsPtr->lfsUnlock, FALSE);
    }
    lfsPtr->locked = TRUE;
    UNLOCK_MONITOR;
}
LfsUnLockDomain(lfsPtr, condPtr)
    Lfs	*lfsPtr;
	    Sync_Condition *condPtr;
{
    LOCK_MONITOR;
    lfsPtr->locked = FALSE;
    Sync_Broadcast(&lfsPtr->lfsUnlock);
    if (condPtr != (Sync_Condition *) NIL) {
	Sync_Broadcast(condPtr);
    }
    UNLOCK_MONITOR;
}

LfsNotify(lfsPtr, condPtr)
    Lfs	*lfsPtr;
	    Sync_Condition *condPtr;
{
    LOCK_MONITOR;
    Sync_Broadcast(condPtr);
    UNLOCK_MONITOR;
}

LfsWaitDomain(lfsPtr, condPtr)
    Lfs	*lfsPtr;
    Sync_Condition *condPtr;
{
    LOCK_MONITOR;
    lfsPtr->locked = FALSE;
    Sync_Broadcast(&lfsPtr->lfsUnlock);
    Sync_Wait(condPtr, FALSE);
    while (lfsPtr->locked) {
	Sync_Wait(&lfsPtr->lfsUnlock, FALSE);
    }
    lfsPtr->locked = TRUE;
    UNLOCK_MONITOR;
}

