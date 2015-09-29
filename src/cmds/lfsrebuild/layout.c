

#ifdef __STDC__
#define _HAS_PROTOTYPES
#endif

#include <cfuncproto.h>
#include <varargs.h>
#include <sprite.h>
#include <stdio.h>
#include <sys/types.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <fs.h>
#include <kernel/fs.h>
#include <kernel/dev.h>
#include <kernel/fsdm.h>
#include <kernel/fslcl.h>
#include <kernel/devDiskLabel.h>

#include <kernel/lfsDesc.h>
#include <kernel/lfsDescMap.h>
#include <kernel/lfsFileLayout.h>
#include <kernel/lfsDirOpLog.h>
#include <kernel/lfsSegLayout.h>
#include <kernel/lfsStableMem.h>
#include <kernel/lfsSuperBlock.h>
#include <kernel/lfsUsageArray.h>
#include <kernel/lfsStats.h>

#include "layout.h"
#include "fscheck.h"

#define	MIN_SUMMARY_REGION_SIZE	16
/*
 * Macro returning TRUE if segment is completely empty.
 */
#define SegIsEmpty(segPtr) (((segPtr)->numBlocks == 1) && 		\
	    ((segPtr)->curSegSummaryPtr->size == sizeof(LfsSegSummary)))

static void AddNewSummaryBlock _ARGS_((LfsSeg *segPtr));
static Boolean WriteSegment _ARGS_((int diskId, LfsSeg *segPtr));

Boolean
LayoutCheckpoint(segPtr, checkPointPtr, sizePtr)
    LfsSeg	*segPtr;
    char *checkPointPtr;
    int *sizePtr;
{
    int	moduleType, startOffset;
    Boolean full;
    char	*summaryPtr, *endSummaryPtr;
    int		newStartBlockOffset;
    LfsCheckPointRegion	*segUsageCheckpointRegionPtr;

    full = FALSE;
    segUsageCheckpointRegionPtr = (LfsCheckPointRegion *) NIL;
    for(moduleType = 0; moduleType < LFS_MAX_NUM_MODS; moduleType++) {
	/*
	 * Filling in a segment, be sure that there is enough
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
	{
	    int	size;
	    LfsCheckPointRegion	*regionPtr;
	    regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	    size = 0;
	    if (moduleType == LFS_SEG_USAGE_MOD) {
		full = LfsSegUsageCheckpoint(segPtr, (char *)(regionPtr + 1),
				     &size);
	    } else if (moduleType == LFS_DESC_MAP_MOD) {
		full = LfsDescMapCheckpoint(segPtr, (char *)(regionPtr + 1),
				     &size);
	    } else {
		full = FALSE;
	    }
	    if (full) {	
		break;
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
	if (full) {	
	    if (LfsSegSummaryBytesLeft(segPtr) > MIN_SUMMARY_REGION_SIZE) {
		break;
	    }
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
	    newStartBlockOffset = segPtr->startBlockOffset;
	} else if ((segPtr->curDataBlockLimit -  segPtr->curBlockOffset) > 
		       10) { 
	    /*
	     * If this is considered to be a partial segment write add the
	     * summary block we needed.
	     */
	    AddNewSummaryBlock(segPtr);
	    newStartBlockOffset = segPtr->curBlockOffset-1;
	}
   }
   LfsSetLogTail(&segPtr->logRange, newStartBlockOffset, 
					segPtr->activeBytes);  
   if (segUsageCheckpointRegionPtr != (LfsCheckPointRegion *) NIL) {
       LfsSegUsageCheckpointUpdate( 
			(char *) (segUsageCheckpointRegionPtr + 1),
			segUsageCheckpointRegionPtr->size - 
					sizeof(*segUsageCheckpointRegionPtr));
    }

    return full;
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
    int	newDiskAddr;

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
    newDiskAddr = segPtr->logRange.current * 
			superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize + 
		  superBlockPtr->hdr.logStartOffset;
    newDiskAddr += superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize - blockOffset;
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

    sumBytes = superBlockPtr->hdr.blockSize;
    sumBufferPtr = LfsSegAddDataBuffer(segPtr, 1, malloc(sumBytes),
					(ClientData) NIL);
    newSummaryPtr =  (LfsSegSummary *) sumBufferPtr->address;
    newSummaryPtr->magic = LFS_SEG_SUMMARY_MAGIC;
    newSummaryPtr->timestamp = ++currentTimestamp;
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

    *lenPtr = 0;
    offset = 0;
    while ((segPtr->curElement >= 0)) {
	elementPtr = segPtr->segElementPtr + segPtr->curElement;
	bytes = elementPtr->lengthInBlocks  * superBlockPtr->hdr.blockSize;
	 bcopy(elementPtr->address, bufferPtr + *lenPtr, bytes);
	*lenPtr += bytes;
	segPtr->curElement--;
    }
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


    free((char *) segPtr->segElementPtr);
    free((char *)segPtr);
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
GetSegStruct(segLogRangePtr, startBlockOffset)
    LfsSegLogRange *segLogRangePtr; /* Log range of segment. */
    int		   startBlockOffset; /* Starting block offset into segment */
{
    LfsSeg	*segPtr;

    segPtr = (LfsSeg *) calloc(1, sizeof(LfsSeg));
    segPtr->segElementPtr = 
		(LfsSegElement *) malloc(sizeof(LfsSegElement) * 
			superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize);
    segPtr->logRange = *segLogRangePtr;
    segPtr->numElements = 0;
    segPtr->numBlocks = 0;
    segPtr->startBlockOffset = startBlockOffset;
    segPtr->activeBytes = 0;
    segPtr->curSegSummaryPtr = (LfsSegSummary *) NIL;
    segPtr->curSummaryHdrPtr = (LfsSegSummaryHdr *) NIL;
    segPtr->curElement = -1;
    segPtr->curBlockOffset = segPtr->startBlockOffset;
    segPtr->curDataBlockLimit = superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize;
    segPtr->curSummaryPtr = (char *) NIL;
    segPtr->curSummaryLimitPtr = (char *) NIL;

    return segPtr;
}
Boolean
WriteSegCheckpoint(diskId, checkPointPtr, checkPointSizePtr)
    int	diskId;
    char *checkPointPtr;
    int	*checkPointSizePtr;
{
    LfsSeg	*segPtr;
    LfsSegLogRange	segLogRange;
    int		startBlock;
    Boolean full;

    full = TRUE;
    while (full) {
	LfsGetLogTail( &segLogRange, &startBlock);
	segPtr = GetSegStruct(&segLogRange, startBlock);
	*checkPointSizePtr = 0;
	full = LayoutCheckpoint(segPtr, checkPointPtr, checkPointSizePtr);

	WriteSegment(diskId, segPtr);
        DestorySegStruct(segPtr);
    }
    return TRUE;
}
static Boolean
WriteSegment(diskId, segPtr)
	int	diskId;
	LfsSeg	*segPtr;
{
    char	*segMemPtr;
    int		len;
    int		diskAddress;

    if (SegIsEmpty(segPtr)) { 
	return TRUE;
    }
    /*
     * Compute the starting disk address of this I/O.
     */
    diskAddress = segPtr->logRange.current * 
			superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize + 
		  superBlockPtr->hdr.logStartOffset;
    diskAddress += superBlockPtr->usageArray.segmentSize/superBlockPtr->hdr.blockSize - segPtr->curBlockOffset;

    segPtr->curElement = segPtr->numElements-1;
    segPtr->curBlockOffset = 0;

    segMemPtr = alloca(superBlockPtr->usageArray.segmentSize);
    CopySegToBuffer(segPtr, superBlockPtr->usageArray.segmentSize, 
			segMemPtr, &len);
    if (DiskWrite(diskId, diskAddress, len, segMemPtr) != len) {
	fprintf(stderr, "Can't write checkpoint to disk\n");
	return FALSE;
    }
    return TRUE;
}

