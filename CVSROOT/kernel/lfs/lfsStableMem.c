/* 
 * LfsStableMemIndex.c --
 *
 *	Generic routines for supporting an in memory data structure that
 *	is written to a LFS log at each checkpoint.  
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

#include "sprite.h"
#include "lfs.h"
#include "lfsSeg.h"
#include "lfsStableMem.h"
#include "stdlib.h"
#include "fsdm.h"

static Boolean AddBlockToSegment();


/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemLoad --
 *
 * 	Allocate and load a LFS stable memory resident data structure.
 *
 * Results:
 *	SUCCESS if load succeed ok.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsStableMemLoad(lfsPtr, smemParamsPtr, checkPointSize, checkPointPtr,smemPtr)
    Lfs *lfsPtr;	   /* File system of metadata. */
    LfsStableMemParams  *smemParamsPtr; /* Parameters for this memory. */
    int  checkPointSize;   /* Size of checkpoint data. */
    char *checkPointPtr;   /* Data from last checkpoint before shutdown. */
    LfsStableMem	*smemPtr; /* In memeory index data structures. */
{
    int	blockNum, bufferSize; 
    ReturnStatus  status;
    LfsStableMemCheckPoint *cpPtr;
    char	*dataPtr;

    cpPtr = (LfsStableMemCheckPoint *) 	checkPointPtr;
    /*
     * Do some bounds checking on the checkpoint buffer.
     */
    if ((checkPointSize < sizeof(LfsStableMemCheckPoint)) ||
        (checkPointSize < (cpPtr->numBlocks * sizeof(unsigned int) + 
			    sizeof(LfsStableMemCheckPoint))))  {
	return FAILURE;
    }
    /*
     * Fill in the LfsStableMem data structures.
     *
     * Allocate space for the data buffer.
     */
    bufferSize = smemParamsPtr->blockSize * smemParamsPtr->maxNumBlocks;
    smemPtr->dataPtr = malloc(bufferSize);
    /*
     * Allocate and copy the index for the metadata. Be sure to allocated
     * enought space for the index  and dirty bits area.
     */
    bufferSize = (smemParamsPtr->maxNumBlocks * sizeof(int)) + 
			(smemParamsPtr->maxNumBlocks + 7)/8;
    smemPtr->blockIndexPtr = (int *) malloc(bufferSize);
    bcopy(checkPointPtr + sizeof(LfsStableMemCheckPoint), 
		 (char *) smemPtr->blockIndexPtr, 
		 cpPtr->numBlocks * sizeof(int));
    for (blockNum = cpPtr->numBlocks; blockNum < smemParamsPtr->maxNumBlocks;
			blockNum++) {
	smemPtr->blockIndexPtr[blockNum] = FSDM_NIL_INDEX;
    }

    /*
     * Mark all blocks as clean.
     */
    smemPtr->dirtyBlocksBitMapPtr = (char *)(smemPtr->blockIndexPtr) +
				     smemParamsPtr->maxNumBlocks * sizeof(int);
    bzero(smemPtr->dirtyBlocksBitMapPtr, (smemParamsPtr->maxNumBlocks+ 7)/8);

    smemPtr->blockSizeShift = LfsLogBase2(smemParamsPtr->blockSize);

    /*
     * Fillin the rest of the LfsStableMem data structure with a copy
     * of the checkPoint and Params data.
     */

    smemPtr->checkPoint = *cpPtr;
    smemPtr->params = *smemParamsPtr;
    /*
     * Read all the blocks of the stable memory from disk.
     */
    dataPtr = smemPtr->dataPtr;
    for (blockNum = 0; blockNum < cpPtr->numBlocks; blockNum++) {
	unsigned int blockIndex = smemPtr->blockIndexPtr[blockNum];
	if (blockIndex == FSDM_NIL_INDEX) {
	    bzero(dataPtr, smemParamsPtr->blockSize);
	} else {
	    status = LfsReadBytes(lfsPtr, blockIndex, 
				  smemParamsPtr->blockSize, dataPtr);
	    if (status != SUCCESS) {
		    break;
	    }
	}
	dataPtr += smemParamsPtr->blockSize;
    }
    /*
     * Zero out the rest of the buffer.
     */
    bzero(dataPtr, (smemParamsPtr->maxNumBlocks - cpPtr->numBlocks) * 
			smemParamsPtr->blockSize);
    if (status != SUCCESS) {
	free((char *) smemPtr->blockIndexPtr);
	smemPtr->blockIndexPtr = (int *) NIL;
	free((char *) smemPtr->dataPtr);
	smemPtr->dataPtr = (char *) NIL;
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemClean --
 *
 *	Routine to handle cleaning pieces containing data for this module.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsStableMemClean(segToCleanPtr, segPtr, smemPtr)
    LfsSeg *segToCleanPtr;	/* Segment containing data to clean. */
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    LfsStableMem *smemPtr;	/* Index pointer. */
{
    char *summaryPtr;
    int	 *blockPtr;
    int	 numBlocks, block, blockOffset;
    unsigned int blockAddress;
    Boolean	full;

    full = FALSE;
    summaryPtr = LfsSegGetSummaryPtr(segToCleanPtr);
    numBlocks = LfsSegSummaryBytesLeft(segToCleanPtr) / sizeof(int);
    blockPtr = (int *) summaryPtr;
    blockAddress = LfsSegDiskAddress(segToCleanPtr, 
					LfsSegGetBufferPtr(segToCleanPtr));
    blockOffset = 0;
    for (block = 0; block < numBlocks; block++) { 
	if (smemPtr->blockIndexPtr[blockPtr[block]] == 
			    (blockAddress + blockOffset)) {
	    full = AddBlockToSegment(smemPtr, block, segPtr);
	    if (full) {
		break;
	    }
	 }
	 blockOffset += LfsBytesToBlocks(segPtr->lfsPtr, 
					smemPtr->params.blockSize);
    }
    LfsSegSetSummaryPtr(segToCleanPtr, (char *) (blockPtr + block));
    LfsSegSetCurBlockOffset(segToCleanPtr, blockOffset);
    return full;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemCheckpoint --
 *
 *	Routine to handle checkpointing of data for this module.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	checkpointed.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsStableMemCheckpoint(segPtr, checkPointPtr, flags, checkPointSizePtr,smemPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   flags;		/* Flags. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    LfsStableMem *smemPtr;	/* Stable memory description. */
{
    register char *dirtyBitsPtr;
    register int  block, bitNum;
    Boolean	full = FALSE;
    int		numBlocks;

    /*
     * Find and layout all the dirty metadata blocks.
     */
    dirtyBitsPtr = smemPtr->dirtyBlocksBitMapPtr;
    numBlocks = smemPtr->params.maxNumBlocks;
    for (block = 0; block < numBlocks; ) {
	if (*dirtyBitsPtr == 0x0) {
	     /*
	      * If there are no bits set in this byte we can skip all 
	      * the block identified by it.
	      */
	     block += sizeof(*dirtyBitsPtr)*8;
	     dirtyBitsPtr++;
	     continue;
	 }
	 /*
	  * Layout of dirty blocks represented by this byte.
	  */
	 for (bitNum = 0; bitNum < sizeof(*dirtyBitsPtr)*8; bitNum++) {
		int bit = (1 << bitNum);
		if (!(*dirtyBitsPtr & bit)) {
		    block++;
		    continue;
		}
		/*
		 * Found one - lay it out. 
		 */
		full = AddBlockToSegment(smemPtr, block, segPtr);
		if (full) {
		    break;
		}
		if (smemPtr->checkPoint.numBlocks < (block+1)) {
		    smemPtr->checkPoint.numBlocks = block+1;
		}
		*dirtyBitsPtr ^= bit;
		block++;
	  }
	  if (full) {
	      break;
	  }
	  dirtyBitsPtr++;
    }
    /*
     * If we didn't fill the segment copy the index to the checkpoint buffer.
     */
    if (!full) {
	*(LfsStableMemCheckPoint *) checkPointPtr = smemPtr->checkPoint;
	bcopy((char *) smemPtr->blockIndexPtr, 
		checkPointPtr + sizeof(LfsStableMemCheckPoint), 
		sizeof(int) * smemPtr->checkPoint.numBlocks);
	*checkPointSizePtr = sizeof(int) * smemPtr->checkPoint.numBlocks + 
				sizeof(LfsStableMemCheckPoint);

    }
    return full;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemWriteDone --
 *
 *	Routine to inform this module that a write has finished.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

void
LfsStableMemWriteDone(segPtr, flags, smemPtr)
    LfsSeg *segPtr;	/* Segment whose write finishes. */
    int	    flags;	/* Write done flags. */
    LfsStableMem *smemPtr;	/* Index description. */
{
    /*
     * Free up space if we are detaching.
     */
    if (LFS_DETACH & flags) {
	free((char *)smemPtr->blockIndexPtr);
	smemPtr->blockIndexPtr = (int *) NIL;
	free((char *) smemPtr->dataPtr);
	smemPtr->dataPtr = (char *) NIL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemMarkDirty --
 *
 *	Routine to mark blocks of a metadata index as dirty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

void
LfsStableMemMarkDirty(smemPtr, startPtr, length)
    LfsStableMem *smemPtr;	/* Index description. */
    char	 *startPtr;	/* Starting addressed dirtied. */
    int		length;		/* Number of bytes dirtied. */
{
    int offset, startBlockNum, endBlockNum;

    offset = (int)(startPtr - smemPtr->dataPtr);
    startBlockNum = offset >> smemPtr->blockSizeShift;
    endBlockNum = (offset + length - 1) >> smemPtr->blockSizeShift;
    while (startBlockNum <= endBlockNum) {
	smemPtr->dirtyBlocksBitMapPtr[startBlockNum/(8*sizeof(char))] |= 
				(1 << (startBlockNum % (8*sizeof(char))));
	startBlockNum++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AddBlockToSegment --
 *
 *	Add a metadata block to the specified segment.
 *
 * Results:
 *	TRUE if segment is full so we couldn't add block.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


static Boolean
AddBlockToSegment(smemPtr, blockNum, segPtr)
    LfsStableMem *smemPtr;	/* Index of block. */
    int		blockNum;	/* Block to add. */
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
{
    int	fsBlocks;
    char *summaryPtr;
    LfsSegElement *bufferPtr;

    fsBlocks = LfsBytesToBlocks(segPtr->lfsPtr, smemPtr->params.blockSize);
    summaryPtr = LfsSegGrowSummary(segPtr, fsBlocks, sizeof(int));
    if (summaryPtr == (char *)NIL) {
	return TRUE;
    }
    bufferPtr = LfsSegAddDataBuffer(segPtr, fsBlocks,
	 smemPtr->dataPtr + (blockNum << smemPtr->blockSizeShift),
	 (ClientData) blockNum);
    *(int *)summaryPtr = blockNum;
    LfsSegSetSummaryPtr(segPtr,summaryPtr + sizeof(int));
    LfsSegUsageFreeBlocks(segPtr->lfsPtr, smemPtr->params.blockSize, 1, 
		smemPtr->blockIndexPtr + blockNum);
    smemPtr->blockIndexPtr[blockNum] = LfsSegDiskAddress(segPtr, bufferPtr);
    return FALSE;
}

