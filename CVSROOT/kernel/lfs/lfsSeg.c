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

Boolean	lfsSegWriteDebug = FALSE;

enum CallBackType { SEG_LAYOUT, SEG_CLEAN, SEG_CHECKPOINT, SEG_WRITEDONE};

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
 * LfsSegWriteStart --
 *
 *	Request that the segment manager start the segment write sequence
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
LfsSegWriteStart(lfsPtr)
    Lfs	 *lfsPtr;	/* File system with data to write. */
{
    if (!lfsPtr->writeActive) {
	Proc_CallFunc(SegmentWriteProc, (ClientData) lfsPtr, 0);
    }
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
    if (!lfsPtr->writeActive) {
	Proc_CallFunc(SegmentCleanProc, (ClientData) lfsPtr, 0);
    }
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
		 LfsBytesToBlocks(lfsPtr, 
			(sumBytesGrow + (lfsPtr->superBlock.hdr.blockSize-1)));
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

unsigned int
LfsSegSlowDiskAddress(segPtr, segElementPtr)
    register LfsSeg	*segPtr; 	/* Segment of interest. */
    LfsSegElement *segElementPtr; /* Segment element of interest. */
{
    int	elementNumber;
    unsigned int blockOffset;
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
    unsigned int startOffset;

    if (segPtr->curBlockOffset + blocks > segPtr->curDataBlockLimit) {
	return (LfsSegElement *) NIL;
    }
    segPtr->curElement++;
    startOffset = segPtr->curBlockOffset;
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
DoWriteCallBacks(type, outSegPtr, inSegPtr, flags, checkPointPtr,
					checkPointSizePtr) 

    enum CallBackType type;	/* Type of segment write. */
    LfsSeg	*outSegPtr;	/* Segment to fill in. */
    LfsSeg	*inSegPtr; 	/* Segment to clean for cleaning operations. */
    int		flags;		/* Flags used during checkpoint. */
    char	*checkPointPtr; /* Checkpoint buffer. */
    int	    *checkPointSizePtr; /* Size of checkpoint buffer for checkpoint 
				 * operations. */

{
    LfsSegSummary  *segSummaryPtr;
    int	moduleType, startOffset;
    Boolean full = FALSE;
    char	*summaryPtr, *endSummaryPtr;

    /*
     * If we're doing a cleaning pass initialize the moduleType from the
     * summary region. Otherwise start at module zero.
     */
    if (inSegPtr != (LfsSeg *) NIL) {
	moduleType = inSegPtr->curSummaryHdrPtr->moduleType;
    } else {
	moduleType = 0;
    }
    while(moduleType < LFS_MAX_NUM_MODS) {
	if (outSegPtr != (LfsSeg *) NIL) { 
	    /*
	     * If we filling in a segment be sure that there is an enought
	     * room for the LfsSegSummaryHdr.
	     */
	    summaryPtr = LfsSegGrowSummary(outSegPtr, 1,sizeof(LfsSegSummaryHdr));
	    if (summaryPtr == (char *) NIL) {
		full = TRUE;
		break;
	    }
	    outSegPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)  summaryPtr;
	    LfsSegSetSummaryPtr(outSegPtr,
					summaryPtr + sizeof(LfsSegSummaryHdr));
	    startOffset = outSegPtr->curBlockOffset;
	}
	switch (type) {
	case SEG_LAYOUT: 
	    full = lfsSegIoInterfacePtrs[moduleType]->layout(outSegPtr);
	    break;
	case SEG_CLEAN:
	    full = lfsSegIoInterfacePtrs[moduleType]->clean(inSegPtr,outSegPtr);
	    break;
	case SEG_CHECKPOINT: {
	    int	size;
	    LfsCheckPointRegion	*regionPtr;
	    regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	    size = 0;
	    full = lfsSegIoInterfacePtrs[moduleType]->
		checkpoint(outSegPtr, flags, (char *) (regionPtr + 1), &size);
	    if (size > 0) {
		regionPtr->type = moduleType;
		regionPtr->size = size + sizeof(LfsCheckPointRegion);
		*checkPointSizePtr += regionPtr->size;
		checkPointPtr += regionPtr->size;
	    }
	    break;
	 }
	case SEG_WRITEDONE: {
	    lfsSegIoInterfacePtrs[moduleType]->writeDone(inSegPtr, flags);
	    break;
	 }
	 default:
	     panic("lfsSeg.c: Bad case statement\n");
	 }
	 /*
	  * If the callback added data to the outSeg fill in the summary 
	  * header. 
	  */
	 if (outSegPtr != (LfsSeg *) NIL) { 
	     endSummaryPtr = LfsSegGetSummaryPtr(outSegPtr); 
	     if ((startOffset != outSegPtr->curBlockOffset) ||
		 ((summaryPtr + sizeof(LfsSegSummaryHdr)) != endSummaryPtr)) {
		outSegPtr->curSummaryHdrPtr->moduleType = moduleType;
		outSegPtr->curSummaryHdrPtr->lengthInBytes = endSummaryPtr - 
				(char *) summaryPtr;
		outSegPtr->curSummaryHdrPtr->numDataBlocks =  
				outSegPtr->curBlockOffset - startOffset;
	     } else {
		 LfsSegSetSummaryPtr(outSegPtr, summaryPtr);
	     }
	}
	if (full) {
	    break;
	}
	/*
	 * Step to the next moduleType.
	 */
	if (inSegPtr != (LfsSeg *) NIL) { 
	    inSegPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)
				((char *)(inSegPtr->curSummaryHdrPtr) + 
				inSegPtr->curSummaryHdrPtr->lengthInBytes);
	    if (((char *)inSegPtr->curSummaryHdrPtr >= 
			inSegPtr->summaryLimitPtr) ||
		(inSegPtr->curSummaryHdrPtr->lengthInBytes == 0)) {
		break;
	    }
	    moduleType = inSegPtr->curSummaryHdrPtr->moduleType;
	    inSegPtr->curSummaryLimitPtr = 
				((char *)(inSegPtr->curSummaryHdrPtr) + 
				inSegPtr->curSummaryHdrPtr->lengthInBytes);
	    inSegPtr->curSummaryPtr = (char *) 
				(inSegPtr->curSummaryHdrPtr + 1);
	} else {
	    moduleType++;
	}
    }
   return full;
}

FillInSummaryHdr(outSegPtr)
    LfsSeg	*outSegPtr;
{
    int	extraBytes;
    LfsSegSummary  *segSummaryPtr;
    char	    *endSummaryPtr;
    Lfs	*lfsPtr = outSegPtr->lfsPtr;
    /*
     * Tack on the LfsSegSummary structure at the end of the last summary
     * block.
     */
    segSummaryPtr = (LfsSegSummary *) (outSegPtr->curSummaryLimitPtr);
    endSummaryPtr = LfsSegGetSummaryPtr(outSegPtr); 
    extraBytes = ((char *) segSummaryPtr) - endSummaryPtr;
    if (extraBytes < 0) {
	panic("Bad outSeg in DoWriteCallBacks\n");
    }
    bzero(endSummaryPtr, extraBytes);
    segSummaryPtr->magic = LFS_SEG_SUMMARY_MAGIC;
    segSummaryPtr->timestamp = LfsGetCurrentTimestamp(outSegPtr->lfsPtr);
    segSummaryPtr->prevSeg = outSegPtr->logRange.prevSeg;
    segSummaryPtr->nextSeg = outSegPtr->logRange.nextSeg;
    segSummaryPtr->size = ((char *) (segSummaryPtr+1)) - 
				    outSegPtr->summaryPtr;

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

/* ARGSUSED */
static void
SegmentWriteProc(lfsPtr, callInfoPtr)
    register Lfs *lfsPtr;            /* File system with dirty blocks. */
    Proc_CallInfo *callInfoPtr;         /* Not used. */
{
    Boolean	full = TRUE;
    LfsSeg	*segPtr;
    LfsSegLogRange	segLogRange;
    ReturnStatus	status;

    LfsLockDomain(lfsPtr);
    lfsPtr->writeActive = TRUE;
    while (full) {
	status = LfsGetCleanSeg(lfsPtr, &segLogRange);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't get clean segments to write\n");
	}
	segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
	full = DoWriteCallBacks(SEG_LAYOUT, segPtr, (LfsSeg *) NIL, 0,
			   (char *) NIL, (int *) NIL);
	status = PushSegmentToLog(segPtr);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't write segment to log\n");
	}
	InitFullSegment(segPtr);
	(void) DoWriteCallBacks(SEG_WRITEDONE, (LfsSeg *) NIL, segPtr, 0,
				(char *) NIL, (int *) NIL);
	DestorySegment(segPtr);
    }
    lfsPtr->writeActive = FALSE;
    LfsUnLockDomain(lfsPtr);
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
    int		segNumber, i;
    LfsSeg	*segToCleanPtr, *segPtr;
    Boolean	full;
    LfsSegLogRange	segLogRange;
    ReturnStatus	status;
    int			numWritten = 0;
    Time      		startTime, endTime;


    LfsLockDomain(lfsPtr);

    Timer_GetTimeOfDay(&startTime, (int *) NIL, (Boolean *) NIL);
#define TE
    lfsPtr->writeActive = TRUE;
    numSegsToClean = LfsGetSegsToClean(lfsPtr, MAX_NUM_TO_CLEAN, segNums);

    if (numSegsToClean == 0) {
	lfsPtr->writeActive = FALSE;
	LfsUnLockDomain(lfsPtr);
	return;
    }
    status = LfsGetCleanSeg(lfsPtr, &segLogRange);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, 
		    "Can't get clean segments to write\n");
    }
    segPtr = CreateEmptySegment(lfsPtr, &segLogRange);

    for (i = 0; i < numSegsToClean; i++) { 
	segNumber = segNums[i];

	segToCleanPtr = CreateSegmentToClean(lfsPtr, segNumber);
	do {
	    full = DoWriteCallBacks(SEG_CLEAN, segPtr, segToCleanPtr, 0,
			   (char *) NIL, (int *) NIL);
	    if (full) {
		numWritten++;
		status = PushSegmentToLog(segPtr);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, "Can't write segment to log\n");
		}
		InitFullSegment(segPtr);
		(void) DoWriteCallBacks(SEG_WRITEDONE, (LfsSeg *) NIL, segPtr,
					0, (char *) NIL, (int *) NIL);
		DestorySegment(segPtr);
		status = LfsGetCleanSeg(lfsPtr, &segLogRange);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, 
				"Can't get clean segments to write\n");
		}
		segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
	    }
	} while (full);
	DestorySegment(segToCleanPtr);
    }
    numWritten++;
    status = PushSegmentToLog(segPtr);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't write segment to log\n");
    }
    InitFullSegment(segPtr);
    (void) DoWriteCallBacks(SEG_WRITEDONE, (LfsSeg *) NIL, segPtr,
			    0, (char *) NIL, (int *) NIL);
    DestorySegment(segPtr);

    for (i = 0; i < numSegsToClean; i++) { 
	LfsSetSegUsage(lfsPtr, segNums[i], 0);
    }
    Timer_GetTimeOfDay(&endTime, (int *) NIL, (Boolean *) NIL);
    printf("Cleaned %d segments in %d segments- time (%d,%d) -  (%d,%d)\n",
	numSegsToClean, numWritten, startTime.seconds, startTime.microseconds,
	endTime.seconds, endTime.microseconds);

    lfsPtr->writeActive = FALSE;
    LfsUnLockDomain(lfsPtr);
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
	dmaBuffer = malloc(lfsPtr->usageArray.params.segmentSize);
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
    status = LfsWriteBytes(lfsPtr, diskAddress, 
			lfsPtr->usageArray.params.segmentSize,
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
    LfsSetSegUsage(lfsPtr, segPtr->logRange.current, 
		    LfsBlocksToBytes(lfsPtr, segPtr->numDataBlocks) + sumBytes);
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
				regionPtr->size - sizeof(*regionPtr),
				(char *) (regionPtr+1));
	} else {
	    status = segIoPtr->attach(lfsPtr, 0, (char *)NIL);
	}
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't attach module %d", moduleType);
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
    ReturnStatus	status= SUCCESS;

    *checkPointSizePtr = 0;
    LfsLockDomain(lfsPtr);
    lfsPtr->writeActive = TRUE;
    while (full) {
	status = LfsGetCleanSeg(lfsPtr, &segLogRange);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't get clean segments to write\n");
	}
	segPtr = CreateEmptySegment(lfsPtr, &segLogRange);
	full = DoWriteCallBacks(SEG_CHECKPOINT, segPtr, (LfsSeg *) NIL, flags,
			    checkPointPtr, checkPointSizePtr);
	status = PushSegmentToLog(segPtr);
	if (status != SUCCESS) {
	    LfsError(lfsPtr, status, "Can't write segment to log\n");
	}
	InitFullSegment(segPtr);
	(void) DoWriteCallBacks(SEG_WRITEDONE, (LfsSeg *) NIL, segPtr, flags,
				(char *) NIL, (int *) NIL);
	DestorySegment(segPtr);
    }
    lfsPtr->writeActive = FALSE;
    lfsPtr->dirty = FALSE;

    LfsUnLockDomain(lfsPtr);
    return status;

}

Boolean
LfsSegNullLayout(segPtr)
    LfsSeg	*segPtr;
{
	return FALSE;
}

static Sync_Condition lfsUnlock;
static Sync_Lock      lfsLock =   Sync_LockInitStatic("LfsLock");
#define	LOCKPTR	&lfsLock
static Boolean	locked = FALSE;

LfsLockDomain(lfsPtr)
    Lfs	*lfsPtr;
{
    LOCK_MONITOR;
    while (locked) {
	Sync_Wait(&lfsUnlock, FALSE);
    }
    locked = TRUE;
    UNLOCK_MONITOR;
}
LfsUnLockDomain(lfsPtr)
    Lfs	*lfsPtr;
{
    LOCK_MONITOR;
    locked = FALSE;
    Sync_Broadcast(&lfsUnlock);
    UNLOCK_MONITOR;
}


