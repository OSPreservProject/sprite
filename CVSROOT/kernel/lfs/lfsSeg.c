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


#define	LOCKPTR	&lfsPtr->lfsLock

enum CallBackType { SEG_LAYOUT, SEG_CLEAN_IN, SEG_CLEAN_OUT, SEG_CHECKPOINT, 
		   SEG_WRITEDONE};

LfsSegIoInterface *lfsSegIoInterfacePtrs[LFS_MAX_NUM_MODS];

static void SegmentWriteProc();
static void SegmentCleanProc();
static ReturnStatus PushSegmentToLog();
static LfsSeg	*CreateSegmentToClean();


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
    LOCK_MONITOR;
    if (lfsPtr->writeBackActive) {
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
    if (lfsPtr->cleanActive) {
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
 *	specified number of blocks and summary bytes.
 *
 * Results:
 *	A pointer to the summary bytes for this region or NIL if
 *	there is not enought room.
 *
 * Side effects:
 *	May allocate more space for the summary bytes.
 *
 *----------------------------------------------------------------------
 */

char *
LfsSegSlowGrowSummary(segPtr, dataBlocksNeeded, sumBytesNeeded)
    register LfsSeg	*segPtr;	/* Segment of interest. */
    int	 dataBlocksNeeded;		/* Number of data blocks needed. */
    int	sumBytesNeeded;			/* Number of summary bytes needed. */

{
    Lfs	*lfsPtr;
    int	 sumBytesLeft, blocksLeft, sumBlocks, sumBytesGrow;

    /*
     * Test the most common case first. Do the data and summary fit
     * in the current configuration. 
     */
    sumBytesLeft = LfsSegSummaryBytesLeft(segPtr);
    blocksLeft = LfsSegBlocksLeft(segPtr);
    if ((blocksLeft >= dataBlocksNeeded) && (sumBytesLeft >= sumBytesNeeded)) {
       return segPtr->curSummaryPtr;
    }
    lfsPtr = segPtr->lfsPtr;
    /*
     * Check to see if there is enought room to grow summary.
     */
    sumBytesGrow = sumBytesNeeded - sumBytesLeft;
    sumBlocks = (sumBytesGrow <= 0) ? 0 : 
			LfsBytesToBlocks(lfsPtr, sumBytesGrow);
    if ((blocksLeft - sumBlocks) < dataBlocksNeeded) {
	return (char *) NIL;
    }
    /*
     * Grow summary region.
     */
    segPtr->curDataBlockLimit -= sumBlocks;
    segPtr->curSummaryLimitPtr += LfsBlocksToBytes(lfsPtr, sumBlocks);
    segPtr->summaryLimitPtr += LfsBlocksToBytes(lfsPtr, sumBlocks);

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
    int	elementNumber;
     int blockOffset;
    /*
     * Check the common case that we are asking about the "current"
     * element. 
     */
    elementNumber = segElementPtr - segPtr->segElementPtr;

    if (elementNumber == segPtr->curElement) {
	blockOffset = segPtr->curBlockOffset - segElementPtr->lengthInBlocks;
    } else {
	int	i;
	blockOffset = 0;
	for (i = 0; i < elementNumber; i++) {
	    blockOffset += segPtr->segElementPtr[i].lengthInBlocks;
	}
    }
    return LfsSegNumToDiskAddress(segPtr->lfsPtr, segPtr->logRange.current) +
				blockOffset;
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
    char   *bufferPtr;	  /* Buffer to add. May be NIL if memory is to be
			   * allocated. */
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

    if (bufferPtr == (char *)NIL) {
	elementPtr->address = segPtr->segMemPtr + 
		(segPtr->lfsPtr->usageArray.params.segmentSize 
		 - LfsBlocksToBytes(segPtr->lfsPtr, segPtr->curBlockOffset));
    }
    segPtr->numElements = segPtr->curElement+1;
    segPtr->numDataBlocks += blocks;
    return elementPtr;
}

InitSegmentMem(lfsPtr)
    Lfs	*lfsPtr;
{
    int	elementSize, segmentSize;

    segmentSize = lfsPtr->usageArray.params.segmentSize;
    elementSize = LfsBytesToBlocks(lfsPtr,segmentSize) * 
				    sizeof(LfsSegElement);
    lfsPtr->segMemoryPtr = 
	    malloc(sizeof(LfsSeg) + elementSize + segmentSize);
    lfsPtr->segMemInUse = FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateEmptySegment --
 *
 *	Create an LfsSeg structure describing an empty segment to be 
 *	filled in by the callback routines.
 *
 * Results:
 *	A pointer to a lfsPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static LfsSeg *
CreateEmptySegment(lfsPtr, logRangePtr) 
    Lfs	*lfsPtr;		/* For which file system. */
    LfsSegLogRange *logRangePtr;/* Segments describing this range of the log. */
{
    LfsSeg	*segPtr;
    int		segmentSize;
     char	*memPtr;

     segmentSize = lfsPtr->usageArray.params.segmentSize;

    /*
     * Alloc an LfsSeg structure. Be sure to leave enought room for default 
     * size LfsSegElement.
     */
    if (lfsPtr->segMemInUse) {
	 int	 elementSize;

	 elementSize = LfsBytesToBlocks(lfsPtr,segmentSize) * 
					sizeof(LfsSegElement);
	 memPtr = malloc(sizeof(LfsSeg) + elementSize + segmentSize);
    } else {
	 lfsPtr->segMemInUse = TRUE;
	 memPtr = lfsPtr->segMemoryPtr;
    }
     segPtr = (LfsSeg *) memPtr;
     segPtr->lfsPtr = lfsPtr;
     segPtr->segMemPtr = memPtr + sizeof(LfsSeg);
     segPtr->segElementPtr = (LfsSegElement *)  
			    (memPtr + sizeof(LfsSeg) + segmentSize);
     segPtr->summaryPtr = segPtr->segMemPtr;
    /*
     * Fill in the segment descriptor used during the write. The segment
     * starts with the one summary block and the data section empty.
     */
    segPtr->logRange = *logRangePtr;
    segPtr->numElements = 0;
    segPtr->numDataBlocks = 0;
    segPtr->summaryLimitPtr = segPtr->summaryPtr + LfsBlocksToBytes(lfsPtr, 1);

    segPtr->activeBytes = 0;
    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *) NIL;
    segPtr->curElement = -1;
    segPtr->curBlockOffset = 0;
    segPtr->curDataBlockLimit = LfsBytesToBlocks(lfsPtr,segmentSize)-1;
    segPtr->curSummaryPtr = segPtr->summaryPtr;
    segPtr->curSummaryLimitPtr = segPtr->summaryLimitPtr - 
					sizeof(LfsSegSummary);

    return segPtr;
}

InitFullSegment(segPtr)
    LfsSeg	*segPtr;
{
    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *) segPtr->summaryPtr;
    segPtr->curElement = 0;
    segPtr->curBlockOffset = 0;
    segPtr->curDataBlockLimit = segPtr->curSummaryHdrPtr->numDataBlocks;
    segPtr->curSummaryPtr = (char *) (segPtr->curSummaryHdrPtr + 1);
    segPtr->curSummaryLimitPtr = (char *) (segPtr->curSummaryHdrPtr) +
			segPtr->curSummaryHdrPtr->lengthInBytes;

}

/*
 *----------------------------------------------------------------------
 *
 * DestorySegment --
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
DestorySegment(segPtr)
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
 * DoWriteCallBacks --
 *
 *	Perform the call backs done to the segment modules.
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
DoWriteCallBacks(type, segPtr, flags, checkPointPtr, sizePtr,
		numCacheBlocksPtr, clientDataPtr) 
    enum CallBackType type;	/* Type of segment operation. */
    LfsSeg	*segPtr;	/* Segment to fill in or out. */
    int		flags;		/* Flags used during checkpoint. */
    char	*checkPointPtr; /* Checkpoint buffer. */
    int	        *sizePtr; /* Size of checkpoint buffer or segment cleaned. */
    int *numCacheBlocksPtr;
    ClientData *clientDataPtr;

{
    int	moduleType, startOffset, size, numCacheBlocks;
    Boolean full = FALSE;
    Boolean segIn;
    char	*summaryPtr, *endSummaryPtr;

    /*
     * If we're doing a pass over an existing segment such as during a
     * cleaning IN phase or a WRITE_DONE callback we initialize the 
     * moduleType from the summary region. Otherwise start at module zero.
     */
    segIn = ((type == SEG_CLEAN_IN) || (type == SEG_WRITEDONE));
    moduleType = segIn ? segPtr->curSummaryHdrPtr->moduleType : 0;

    while(moduleType < LFS_MAX_NUM_MODS) {
	LfsSegIoInterface *intPtr = lfsSegIoInterfacePtrs[moduleType];
	if (!segIn) { 
	    /*
	     * If we filling in a segment be sure that there is an enought
	     * room for the LfsSegSummaryHdr.
	     */
	    summaryPtr = LfsSegGrowSummary(segPtr, 1,sizeof(LfsSegSummaryHdr));
	    if (summaryPtr == (char *) NIL) {
		full = TRUE;
		break;
	    }
	    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)  summaryPtr;
	    LfsSegSetSummaryPtr(segPtr, summaryPtr + sizeof(LfsSegSummaryHdr));
	    startOffset = segPtr->curBlockOffset;
	}
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
	 /*
	  * If the callback added data to the segment, fill in the summary 
	  * header. 
	  */
	 if (!segIn) { 
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
	}
	if (full) {
	    break;
	}
	/*
	 * Step to the next moduleType.
	 */
	if (segIn) { 
	    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)
				((char *)(segPtr->curSummaryHdrPtr) + 
				segPtr->curSummaryHdrPtr->lengthInBytes);
	    if (((char *)segPtr->curSummaryHdrPtr >= 
			segPtr->summaryLimitPtr) ||
		(segPtr->curSummaryHdrPtr->lengthInBytes == 0)) {
		break;
	    }
	    moduleType = segPtr->curSummaryHdrPtr->moduleType;
	    segPtr->curSummaryLimitPtr = 
				((char *)(segPtr->curSummaryHdrPtr) + 
				segPtr->curSummaryHdrPtr->lengthInBytes);
	    segPtr->curSummaryPtr = (char *) (segPtr->curSummaryHdrPtr + 1);
	} else {
	    moduleType++;
	}
    }
   return full;
}

FillInSummaryHdr(segPtr)
    LfsSeg	*segPtr;
{
    int	extraBytes;
    LfsSegSummary  *segSummaryPtr;
    char	    *endSummaryPtr;
    /*
     * Tack on the LfsSegSummary structure at the end of the last summary
     * block.
     */
    segSummaryPtr = (LfsSegSummary *) (segPtr->curSummaryLimitPtr);
    endSummaryPtr = LfsSegGetSummaryPtr(segPtr); 
    extraBytes = ((char *) segSummaryPtr) - endSummaryPtr;
    if (extraBytes < 0) {
	panic("Bad LfsSeg in DoWriteCallBacks\n");
    }
    bzero(endSummaryPtr, extraBytes);
    segSummaryPtr->magic = LFS_SEG_SUMMARY_MAGIC;
    segSummaryPtr->timestamp = LfsGetCurrentTimestamp(segPtr->lfsPtr);
    segSummaryPtr->prevSeg = segPtr->logRange.prevSeg;
    segSummaryPtr->nextSeg = segPtr->logRange.nextSeg;
    segSummaryPtr->size = ((char *) (segSummaryPtr+1)) -  segPtr->summaryPtr;

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
    LfsSegLogRange	segLogRange;
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
	status = LfsGetCleanSeg(lfsPtr, &segLogRange, FALSE);
	if (status == FS_WOULD_BLOCK) {
	    LfsWaitDomain(lfsPtr, &lfsPtr->cleanSegments);
	    continue;
	}
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't get clean segments to write\n");
	}
	segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
	full = DoWriteCallBacks(SEG_LAYOUT, segPtr, 0, (char *) NIL,
				(int *) NIL, (int *) NIL, clientDataArray);
	if ((segPtr->numDataBlocks > 0) ||
	    (segPtr->curSummaryPtr != segPtr->summaryPtr)) { 
	    status = PushSegmentToLog(segPtr);
	    if (status != SUCCESS) {
		LfsError(lfsPtr, status, "Can't write segment to log\n");
	    }
	    InitFullSegment(segPtr);
	    (void) DoWriteCallBacks(SEG_WRITEDONE, segPtr, 0,
				(char *) NIL, (int *) NIL,  (int *) NIL,
				clientDataArray);
	} else {
	    LfsReturnUnusedSeg(lfsPtr, &segLogRange);
	}
	DestorySegment(segPtr);
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
    int		i, numCacheBlocksUsed;
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
	printf("Cleaning started\n", numSegsToClean);
	/*
	 * Reading in segments to clean.
	 */
	totalSize = 0;
	numCacheBlocksUsed = 0;
	for (i = 0; (i < numSegsToClean) && 
		    (numCacheBlocksUsed < lfsPtr->cleanBlocks);i++) { 
	    int size;
	    segPtr = CreateSegmentToClean(lfsPtr, segNums[i]);
	    size = 0;
	    error = DoWriteCallBacks(SEG_CLEAN_IN, segPtr, 0, (char *) NIL, 
				&size,
			    &numCacheBlocksUsed, clientDataArray);
	    DestorySegment(segPtr);
	    if (error) {
		segNums[i] = -1;
	    }
	    totalSize += size;
	}
	numCleaned = i;
	/*
	 * Write out segments cleaned.
	 */
	numWritten = 0;
	LfsLockDomain(lfsPtr);
	if (totalSize > 0) { 
	    full = TRUE;
	    while (full) {
		status = LfsGetCleanSeg(lfsPtr, &segLogRange, TRUE);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, "Can't get clean segments to write\n");
		}
		segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
		full = DoWriteCallBacks(SEG_CLEAN_OUT, segPtr, 0, (char *) NIL,
				(int *) NIL, (int *) NIL, clientDataArray);
    
		numWritten++;
		status = PushSegmentToLog(segPtr);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, "Can't write segment to log\n");
		}
		InitFullSegment(segPtr);
		(void) DoWriteCallBacks(SEG_WRITEDONE, segPtr, 0,
					(char *) NIL, 
					(int *) NIL, (int *) NIL, clientDataArray);
    
		DestorySegment(segPtr);
	    }
	}
    
    
	for (i = 0; i < numCleaned; i++) { 
	    if (segNums[i] != -1) { 
		LfsSetSegUsage(lfsPtr, segNums[i], 0);
	    }
	}
	LfsUnLockDomain(lfsPtr, &lfsPtr->cleanSegments);
	Timer_GetTimeOfDay(&endTime, (int *) NIL, (Boolean *) NIL);
	printf("Cleaned %d segments in %d segments- time (%d,%d) -  (%d,%d)\n",
	    numCleaned, numWritten, startTime.seconds, startTime.microseconds,
	    endTime.seconds, endTime.microseconds);
    } while (numSegsToClean > 2);
    lfsPtr->cleanActive = FALSE;
}

#define	SUN4HACK
static ReturnStatus
PushSegmentToLog(segPtr)
    LfsSeg	*segPtr;	/* Segment to write to log. */
{
    Lfs	*lfsPtr = segPtr->lfsPtr;
    int	 diskAddress, offset;
    int	 sumBlocks, blockPerSeg, element, sumBytes;
    ReturnStatus status;
#ifdef SUN4HACK
    char	*buffer;
    static char	*dmaBuffer = (char *) NIL; 
    if (dmaBuffer == (char *) NIL) {
	dmaBuffer = malloc((int)lfsPtr->usageArray.params.segmentSize);
    }
#endif
    FillInSummaryHdr(segPtr);
    diskAddress = LfsSegNumToDiskAddress(lfsPtr, segPtr->logRange.current);
    blockPerSeg = LfsBytesToBlocks(lfsPtr, 
			lfsPtr->usageArray.params.segmentSize);
    sumBytes = segPtr->summaryLimitPtr - segPtr->summaryPtr;
    sumBlocks = LfsBytesToBlocks(lfsPtr,sumBytes);


#ifdef SUN4HACK
    offset = 0;
    buffer = dmaBuffer;
    for (element = 0; element < segPtr->numElements; element++) {
	int	bytes = 
	LfsBlocksToBytes(lfsPtr, 
			segPtr->segElementPtr[element].lengthInBlocks);
	bcopy(segPtr->segElementPtr[element].address, buffer, bytes);
	buffer += bytes;
	offset += bytes;
    }
    buffer = dmaBuffer + lfsPtr->usageArray.params.segmentSize - sumBytes;
    bcopy(segPtr->summaryPtr, buffer, sumBytes);
    status = LfsWriteBytes(lfsPtr,  diskAddress, 
			(int)lfsPtr->usageArray.params.segmentSize,
			dmaBuffer);
#else
    offset = 0;
    for (element = 0; element < segPtr->numElements; element++) {
	status = LfsWriteBytes(lfsPtr, diskAddress + offset, 
		LfsBlocksToBytes(lfsPtr, 
			segPtr->segElementPtr[element].lengthInBlocks),
		segPtr->segElementPtr[element].address);
	offset += segPtr->segElementPtr[element].lengthInBlocks;
    }

    status = LfsWriteBytes(lfsPtr, diskAddress + blockPerSeg - sumBlocks,
			LfsBlocksToBytes(lfsPtr, sumBlocks), 
			segPtr->summaryPtr);
#endif
    if (lfsSegWriteDebug) { 
    printf("LfsSeg wrote segment %d - %d data blocks %d summary blocks\n",
		segPtr->logRange.current, offset, sumBlocks);
    }
    LfsSetSegUsage(lfsPtr, segPtr->logRange.current, segPtr->activeBytes);
    return status;

}

static LfsSeg *
CreateSegmentToClean(lfsPtr, segNumber)
    Lfs	*lfsPtr;	/* File system of segment. */
    int	segNumber;	/* Segment number to clean. */
{
    LfsSegLogRange	segLogRange;
    LfsSeg		*segPtr;
    LfsSegSummary	*segSummaryPtr;
    int			segSize;
    ReturnStatus	status;

    segLogRange.prevSeg = -1;
    segLogRange.current = segNumber;
    segLogRange.nextSeg = -1;
    segSize = lfsPtr->usageArray.params.segmentSize;
    segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
    status = LfsReadBytes(lfsPtr, LfsSegNumToDiskAddress(lfsPtr, segNumber), 
				segSize, segPtr->segMemPtr);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, 
			"CreateSegmentToClean: can't read segment to clean.\n");
    }
    segSummaryPtr = (LfsSegSummary *)
			(segPtr->segMemPtr + segSize - sizeof(LfsSegSummary));
    segPtr->summaryPtr = (segPtr->segMemPtr + segSize - segSummaryPtr->size);
    segPtr->summaryLimitPtr = (segPtr->segMemPtr + segSize);
    segPtr->numDataBlocks = LfsBytesToBlocks(lfsPtr, segSize) -
				LfsBytesToBlocks(lfsPtr, segSummaryPtr->size);

    segPtr->segElementPtr[0].lengthInBlocks = 0;
    segPtr->segElementPtr[0].address = segPtr->segMemPtr;
    segPtr->segElementPtr[0].clientData = (ClientData) NIL;

    InitFullSegment(segPtr);
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
    LfsLockDomain(lfsPtr);
    while (full) {
	status = LfsGetCleanSeg(lfsPtr, &segLogRange, FALSE);
	if (status == FS_WOULD_BLOCK) {
	    LfsWaitDomain(lfsPtr, &lfsPtr->cleanSegments);
	    continue;
	}
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't get clean segments to write\n");
	}
	segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
	full = DoWriteCallBacks(SEG_CHECKPOINT, segPtr, flags,
			    checkPointPtr, checkPointSizePtr, (int *) NIL,
			    clientDataArray);
	if ((segPtr->numDataBlocks > 0) ||
	    (segPtr->curSummaryPtr != segPtr->summaryPtr)) { 
	    status = PushSegmentToLog(segPtr);
	    if (status != SUCCESS) {
		LfsError(lfsPtr, status, "Can't write segment to log\n");
	    }
	    InitFullSegment(segPtr);
	    (void) DoWriteCallBacks(SEG_WRITEDONE,  segPtr, flags,
				    (char *) NIL, (int *) NIL, (int *) NIL,
				clientDataArray);
	} else {
	    LfsReturnUnusedSeg(lfsPtr, &segLogRange);
	}
	DestorySegment(segPtr);
    }
    lfsPtr->dirty = FALSE;
    LfsUnLockDomain(lfsPtr, (Sync_Condition *) NIL);
    return status;

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
    while (lfsPtr->locked) {
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

