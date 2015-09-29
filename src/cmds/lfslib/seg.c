/* 
 * seg.c --
 *
 *	Routines for accessing LFS  segments from a user level program
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include "lfslibInt.h"
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#include <sys/types.h>
#endif /* _HAS_PROTOTYPES */
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#include <bstring.h>


/*
 *----------------------------------------------------------------------
 *
 * LfsSegInit --
 *
 *	Initialize a LfsSeg structure to allow access to an LFS segment.
 *	Supports only one segment at a time.
 *
 * Results:
 *	A pointer to a LfsSeg structure
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

LfsSeg *
LfsSegInit(lfsPtr, segNumber)
    Lfs	*lfsPtr;	/* File system. */
    int	segNumber;	/* Segment number to operate on. */
{

    LfsSeg *segPtr;

    segPtr = (LfsSeg *) calloc(1, sizeof(LfsSeg));

    segPtr->lfsPtr = lfsPtr;
    segPtr->segNo = segNumber;
    return segPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegStartAddr --
 *
 *	REturn the staring address of the specified LfsSeg on disk.
 *
 * Results:
 *	A disk block address
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
LfsSegStartAddr(segPtr)
    LfsSeg	*segPtr;
{
    return  segPtr->segNo * LfsSegSizeInBlocks(segPtr->lfsPtr) + 
			segPtr->lfsPtr->superBlock.hdr.logStartOffset;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegFetchBlock --
 *
 *	Fetch blocks from an LFS segment on disk.
 *
 * Results:
 *	A malloc'ed buffer containing the data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
LfsSegFetchBlock(segPtr, blockOffset, size)
    LfsSeg		*segPtr;
    int		blockOffset;
    int		size;
{
    char *buf;
    int	  startAddr;

    startAddr = LfsSegStartAddr(segPtr) + LfsSegSizeInBlocks(segPtr->lfsPtr) - 
		    blockOffset - size/LfsBlockSize(segPtr->lfsPtr);

    buf = malloc(size);

    if (LfsDiskRead(segPtr->lfsPtr, startAddr , size, buf) != size) {
	fprintf(stderr,"%s:LfsSegFetchBlock: Can't read seg %d offset %d.\n",
		segPtr->lfsPtr->deviceName, segPtr->segNo, blockOffset);
	return (char *) NULL;
    }
    return buf;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegReleaseBlock --
 *
 *	Release the memory allocated by LfsFetchBlock.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegReleaseBlock(segPtr, memPtr)
    LfsSeg		*segPtr;
    char	*memPtr;
{
    free(memPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegRelease --
 *
 *	Release the initialize segment.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsSegRelease(segPtr)
    LfsSeg		*segPtr;
{
    free((char *) segPtr);
}


static LfsSeg *CreateSegmentToWrite _ARGS_((Lfs *lfsPtr, Boolean dontBlock));
static LfsSeg *GetSegStruct _ARGS_((Lfs *lfsPtr, LfsSegLogRange 
			*segLogRangePtr, int startBlockOffset));
static void AddNewSummaryBlock _ARGS_((LfsSeg *segPtr));
static ReturnStatus WriteSegment _ARGS_((LfsSeg *segPtr));
static void DestorySegStruct _ARGS_((LfsSeg *segPtr));
static void CopySegToBuffer _ARGS_((LfsSeg *segPtr, int maxSize, 
			char *bufferPtr, int *lenPtr));
static void DoWriteDoneCalls _ARGS_((LfsSeg *segPtr, int flags));

static Boolean LayoutNewSeg _ARGS_((LfsSeg *segPtr, char *checkPointPtr, 
		int *sizePtr));

static void RewindCurPtrs _ARGS_((LfsSeg *segPtr));
static void FreeSegmentMem _ARGS_((LfsSeg *segPtr));
static void InitSegmentMem _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr));

/*
 * Macro returning TRUE if segment is completely empty.
 */
#define SegIsEmpty(segPtr) (((segPtr)->numBlocks == 1) && 		\
	    ((segPtr)->curSegSummaryPtr->size == sizeof(LfsSegSummary)))


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

int
LfsSegSlowDiskAddress(segPtr, segElementPtr)
    register LfsSeg	*segPtr; 	/* Segment of interest. */
    LfsSegElement *segElementPtr; /* Segment element of interest. */
{
    int	elementNumber, blockOffset;
    int diskAddress, newDiskAddr;
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
    diskAddress = LfsSegNumToDiskAddress(segPtr->lfsPtr, segPtr->logRange.current);

    blockOffset = LfsSegSizeInBlocks(segPtr->lfsPtr) - blockOffset;
    newDiskAddr = diskAddress + blockOffset;
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
    segPtr->lfsPtr->pstats.writeSummaryBlock++;
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

    do { 
	status = LfsGetLogTail(lfsPtr, dontBlock, &segLogRange, &startBlock);
	if ((status == FS_WOULD_BLOCK) && !dontBlock) {
	    panic("Ran out of segments.");
	} 
    } while ((status == FS_WOULD_BLOCK) && !dontBlock);

    if (status == SUCCESS) { 
	segPtr = GetSegStruct(lfsPtr, &segLogRange, startBlock);
    } else {
	segPtr = (LfsSeg *) NIL;
    }
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

    FreeSegmentMem(segPtr);
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
    LfsSeg	*segPtr;

    segPtr = (LfsSeg *) malloc(sizeof(LfsSeg));
    InitSegmentMem(lfsPtr, segPtr);
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
static void
InitSegmentMem(lfsPtr, segPtr)
    Lfs	*lfsPtr;
    LfsSeg	*segPtr;
{
    int		maxSegElementSize;

    /*
     * Compute the maximum size of the seg element array. It can't be
     * bigger than one element per block in segment.
     */

    maxSegElementSize = LfsBytesToBlocks(lfsPtr,LfsSegSize(lfsPtr)) * 
				    sizeof(LfsSegElement);
    /*
     * Fill in the fixed fields of the preallocated segments.
     */
    segPtr->lfsPtr = lfsPtr;
    segPtr->segElementPtr = (LfsSegElement *) malloc(maxSegElementSize);
    segPtr->memPtr = malloc(LfsSegSize(lfsPtr));

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
static void
FreeSegmentMem(segPtr)
    LfsSeg	*segPtr;
{
    free(segPtr->memPtr);
    free((char *)(segPtr->segElementPtr));

}


/*
 *----------------------------------------------------------------------
 *
 * LayoutNewSeg --
 *
 *	Fillin a new segment to be written as part of recovery.
 *
 * Results:
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
LayoutNewSeg(segPtr, checkPointPtr, sizePtr)
    LfsSeg	*segPtr;
    char	    *checkPointPtr;	/* Checkpoint area to be filled in.*/
    int		    *sizePtr; /* Size of checkpoint buffer. */
{
    char	*summaryPtr, *endSummaryPtr;
    Boolean full;
    int		  moduleType, startOffset, size;
    LfsCheckPointRegion	*regionPtr;
    int		newStartBlockOffset;
    LfsCheckPointRegion	*segUsageCheckpointRegionPtr;


    full = FALSE;
    segUsageCheckpointRegionPtr = (LfsCheckPointRegion *) NIL;
    for(moduleType = 0; moduleType < LFS_MAX_NUM_MODS; ) {
      regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	summaryPtr = LfsSegSlowGrowSummary(segPtr, 1,
			sizeof(LfsSegSummaryHdr) + 16, TRUE);
	if (summaryPtr == (char *) NIL) {
	    full = TRUE;
	    break;
	}
	segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *)  summaryPtr;
	LfsSegSetSummaryPtr(segPtr, summaryPtr + sizeof(LfsSegSummaryHdr));
	startOffset = segPtr->curBlockOffset;
	size = 0;
	switch (moduleType) {
	case LFS_SEG_USAGE_MOD:
	    full = LfsUsageCheckpoint(segPtr, (char *)(regionPtr + 1),
				 &size);
	     break;
	case LFS_DESC_MAP_MOD:
	    full = LfsDescMapCheckpoint(segPtr,  (char *)(regionPtr + 1),
				 &size);
	     break;
	case LFS_FILE_LAYOUT_MOD:
	    full = LfsFileLayoutCheckpoint(segPtr, (char *)(regionPtr + 1),
			    &size);
	     break;
	default:
	    panic("Unknown module type %d\n", moduleType);
	}
	if (size > 0) {
	    if (moduleType == LFS_SEG_USAGE_MOD) {
		    segUsageCheckpointRegionPtr = regionPtr;
	    }
	    regionPtr->type = moduleType;
	    regionPtr->size = size + sizeof(LfsCheckPointRegion);
	    *sizePtr += regionPtr->size;
	    checkPointPtr += regionPtr->size;
	}
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
	 if (full) { 
	    if (LfsSegSummaryBytesLeft(segPtr) > 16) {
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
 * DoWriteDoneCalls --
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
static void
DoWriteDoneCalls( segPtr, flags) 
    LfsSeg	*segPtr;	/* Segment to fill in or out. */
    int		flags;		/* Flags used during checkpoint. */

{
    int	moduleType,  next;
    char	 *endSummaryBlockPtr;
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
	    moduleType = segPtr->curSummaryHdrPtr->moduleType;
	    segPtr->curSummaryLimitPtr = ((char *)(segPtr->curSummaryHdrPtr) + 
				segPtr->curSummaryHdrPtr->lengthInBytes);
	    segPtr->curSummaryPtr = (char *) (segPtr->curSummaryHdrPtr + 1);
	    switch (moduleType) {
		case LFS_SEG_USAGE_MOD:
		    LfsSegUsageWriteDone(segPtr, flags);
		     break;
		case LFS_DESC_MAP_MOD:
		    LfsDescMapWriteDone(segPtr, flags);
		     break;
		case LFS_FILE_LAYOUT_MOD:
		    LfsFileLayoutWriteDone(segPtr, flags);
		     break;
		default:
		    panic("Unknown module type %d\n", moduleType);
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
	    /*
	     * Free up any memory we allocated during the layout
	     */
	    free((char *) segPtr->curSegSummaryPtr);
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
   return;
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
LfsSegCheckPoint(lfsPtr, checkPointPtr, checkPointSizePtr)
    Lfs	  	    *lfsPtr;		/* File system to checkpoint. */
    char	    *checkPointPtr;	/* Checkpoint area to be filled in.*/
    int		    *checkPointSizePtr; /* Size of checkpoint buffer. */
{
    Boolean	full = TRUE;
    LfsSeg	*segPtr;
    ReturnStatus	status= SUCCESS;

    while (full) {
	segPtr = CreateSegmentToWrite(lfsPtr, 1);
	*checkPointSizePtr = 0;

	full = LayoutNewSeg(segPtr,checkPointPtr, checkPointSizePtr);
	status = WriteSegment(segPtr);
	if (status != SUCCESS) {
	    panic("Can't write segment to log\n");
	}
	RewindCurPtrs(segPtr);
	DoWriteDoneCalls(segPtr, 0);
        DestorySegStruct(segPtr);
    }
    return status;

}


/*
 *----------------------------------------------------------------------
 *
 * WriteSegment --
 *
 *	Write a segment to the log.
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
WriteSegment(segPtr) 
    LfsSeg	*segPtr;	/* Segment to write. */
{
    Lfs	*lfsPtr = segPtr->lfsPtr;
    int offset, len, count;
    int  diskAddress;
    static char *memPtr = (char *) NULL;

    if (SegIsEmpty(segPtr)) { 
	/*
	 * The segment being written is empty so we don't have
	 * to write anytime. 
	 */
	return SUCCESS;
    }

    /*
     * Compute the starting disk address of this I/O.
     */
    diskAddress = LfsSegNumToDiskAddress(lfsPtr, segPtr->logRange.current);
    offset = LfsSegSizeInBlocks(lfsPtr)	- segPtr->curBlockOffset;
    diskAddress += offset;

    segPtr->curElement = segPtr->numElements-1;
    segPtr->curBlockOffset = 0;


    if (memPtr == (char *) NULL) {
	memPtr = malloc(LfsSegSize(lfsPtr));
    }
    CopySegToBuffer(segPtr, LfsSegSize(lfsPtr), memPtr, &len);

    count = LfsDiskWrite(lfsPtr, diskAddress, len, memPtr);
    if (count != len) {
	return FAILURE;
    } else {
	return SUCCESS;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * CopySegToBuffer --
 *
 *	Copy a segment into a buffer.
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


