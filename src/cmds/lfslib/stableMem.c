/* 
 * stableMem.c --
 *
 *	Routines for access LFS stable memory data structure
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
#include <bstring.h>

/*
 * Data structure internal to the stable memory module.
 */
typedef struct StableMem {
    Lfs			*lfsPtr;     /* File system. */
    LfsStableMemParams  *paramsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *checkpointPtr;  /* Checkpoint pointer. */
    char		*dataPtr;	    /* Memory hold stable data. */
    int			*dirtyBlocksBitMap;    /* Bitmap of modified blocks. */
    int			*layoutBlocksBitMap;   /* Bitmap of blocks layed out. */
} StableMem;

/*
 *----------------------------------------------------------------------
 *
 * LfsLoadStableMem --
 *
 *	Load a stable memory data structure
 *
 * Results:
 *	A clientdata that can be used to access stable mem.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ClientData 
LfsLoadStableMem(lfsPtr, smemParamsPtr, cpPtr)
    Lfs	*lfsPtr;	/*  file system. */
    LfsStableMemParams  *smemParamsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *cpPtr;  /* Checkpoint pointer. */
{
    int arraySize;
    char *dataPtr;
    StableMem *stableMemPtr;
    int	*indexPtr, i;

    arraySize = smemParamsPtr->blockSize * smemParamsPtr->maxNumBlocks;
    dataPtr = calloc(1, arraySize);
    stableMemPtr = (StableMem *) malloc(sizeof(StableMem));
    stableMemPtr->lfsPtr = lfsPtr;
    stableMemPtr->paramsPtr = smemParamsPtr;
    stableMemPtr->checkpointPtr = 
	(LfsStableMemCheckPoint *) malloc(sizeof(LfsStableMemCheckPoint) +
			smemParamsPtr->maxNumBlocks * sizeof(int));
    bcopy((char *) cpPtr, (char *) stableMemPtr->checkpointPtr, 
		sizeof(LfsStableMemCheckPoint) + 
		sizeof(int) * cpPtr->numBlocks);
    indexPtr = (int *) (stableMemPtr->checkpointPtr+1);
    for (i = cpPtr->numBlocks; i < smemParamsPtr->maxNumBlocks; i++) {
	indexPtr[i] = FSDM_NIL_INDEX;
    }
    stableMemPtr->dataPtr = dataPtr;
    stableMemPtr->dirtyBlocksBitMap = (int *) NIL;
    stableMemPtr->layoutBlocksBitMap = (int *) NIL;
    return (ClientData) stableMemPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsGetStableMemEntry --
 *
 *	Return a pointer to a specified stable memory entry.
 *
 * Results:
 *	A pointer to the stable memory entry request, NIL if entry doesn't
 *	exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
LfsGetStableMemEntry(clientData, entryNumber)
    ClientData clientData;
    int entryNumber;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    LfsStableMemBlockHdr *hdrPtr;
    int blockNum, offset;
    int	blockIndex;

    if ((entryNumber < 0) || 
	(entryNumber >= stableMemPtr->paramsPtr->maxNumEntries)) {
	fprintf(stderr,"Bad stable memory entry number %d\n", entryNumber);
	return (char *) NULL;
    }
    blockNum = entryNumber / stableMemPtr->paramsPtr->entriesPerBlock;
    offset = (entryNumber % stableMemPtr->paramsPtr->entriesPerBlock) * 
		stableMemPtr->paramsPtr->entrySize + 
		   sizeof(LfsStableMemBlockHdr);
    hdrPtr = (LfsStableMemBlockHdr *) (stableMemPtr->dataPtr + 
		blockNum * stableMemPtr->paramsPtr->blockSize);
    if (hdrPtr->magic != LFS_STABLE_MEM_BLOCK_MAGIC) {
	blockIndex = LfsGetStableMemBlockIndex(clientData, blockNum);
	if (blockIndex != FSDM_NIL_INDEX) {
	    if (LfsDiskRead(stableMemPtr->lfsPtr, blockIndex, 
			stableMemPtr->paramsPtr->blockSize, (char *) hdrPtr) != 
		stableMemPtr->paramsPtr->blockSize) { 
		    fprintf(stderr, "%s:Can't read at %d\n", stableMemPtr->lfsPtr->deviceName, 
			    blockIndex);
	    }
	} else {
	    bzero((char *) hdrPtr, stableMemPtr->paramsPtr->blockSize);
	    hdrPtr->magic = LFS_STABLE_MEM_BLOCK_MAGIC;
	    hdrPtr->memType = stableMemPtr->paramsPtr->memType;
	    hdrPtr->blockNum = blockNum;
	}
    }

    return stableMemPtr->dataPtr + 
        blockNum * stableMemPtr->paramsPtr->blockSize + offset;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsGetStableMemBlockIndex --
 *
 *	Return the BlockIndex into the disk of the stable memory block.
 *
 * Results:
 *	The disk address. FSDM_NIL)INDEX if block doesn't exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

extern int
LfsGetStableMemBlockIndex(clientData, blockNum)
    ClientData clientData;
    int blockNum;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int *blockIndexPtr;

    if ((blockNum < 0) || 
	(blockNum >= stableMemPtr->paramsPtr->maxNumBlocks)) {
	fprintf(stderr,"Bad stable memory block num %d\n", blockNum);
	return FSDM_NIL_INDEX;
    }
    blockIndexPtr = (int *)((char *) (stableMemPtr->checkpointPtr) + sizeof(LfsStableMemCheckPoint));
    return blockIndexPtr[blockNum];
}


/*
 *----------------------------------------------------------------------
 *
 * LfsMarkStableMemEntryDirty --
 *
 *	Mark a stable memory entry as being modified.
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
LfsMarkStableMemEntryDirty(clientData, entryNumber)
    ClientData clientData;
    int entryNumber;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int blockNum;

    if ((entryNumber < 0) || 
	(entryNumber >= stableMemPtr->paramsPtr->maxNumEntries)) {
	fprintf(stderr,"Bad stable memory entry number %d\n", entryNumber);
	return;
    }
    blockNum = entryNumber / stableMemPtr->paramsPtr->entriesPerBlock;
    if (stableMemPtr->dirtyBlocksBitMap == (int *) NIL) {
	Bit_Alloc(stableMemPtr->paramsPtr->maxNumBlocks, 
			stableMemPtr->dirtyBlocksBitMap);
    }
    Bit_Set(blockNum, stableMemPtr->dirtyBlocksBitMap);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSetStableMemBlockIndex --
 *
 *	Change the block index of a stable mem block.
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
LfsSetStableMemBlockIndex(clientData, blockNum, diskAddr)
    ClientData clientData;
    int blockNum;
    int	diskAddr;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int *blockIndexPtr;

    if ((blockNum < 0) || 
	(blockNum >= stableMemPtr->paramsPtr->maxNumBlocks)) {
	panic(stderr,"Bad stable memory block num %d\n", blockNum);
	return;
    }
    blockIndexPtr = (int *)((char *) (stableMemPtr->checkpointPtr) + sizeof(LfsStableMemCheckPoint));

    LfsSegUsageAdjustBytes(stableMemPtr->lfsPtr, blockIndexPtr[blockNum], 
		-stableMemPtr->paramsPtr->blockSize);

    blockIndexPtr[blockNum] = diskAddr;
    if (blockNum > stableMemPtr->checkpointPtr->numBlocks) {
	stableMemPtr->checkpointPtr->numBlocks = blockNum+1;
    }
    return;
}




static Boolean AddBlockToSegment _ARGS_((
	Address address, int blockNum, StableMem *stableMemPtr,LfsSeg *segPtr));


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
AddBlockToSegment(address, blockNum,  stableMemPtr, segPtr)
    Address	  address;	/* Address of block. */
    int		blockNum;	/* Block number. */
    StableMem *stableMemPtr;	/* Stable mem. */
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
{
    int	fsBlocks;
    char *summaryPtr;
    LfsSegElement *bufferPtr;

    fsBlocks =  stableMemPtr->paramsPtr->blockSize/LfsBlockSize(segPtr->lfsPtr);
    summaryPtr = LfsSegGrowSummary(segPtr, fsBlocks, sizeof(int));
    if (summaryPtr == (char *)NIL) {
	return TRUE;
    }
    bufferPtr = LfsSegAddDataBuffer(segPtr, fsBlocks, address,
				(ClientData) blockNum);
    if (stableMemPtr->paramsPtr->memType == LFS_SEG_USAGE_MOD) { 
	segPtr->lfsPtr->pstats.segUsageBlockWrite++;
    } else {
	segPtr->lfsPtr->pstats.descMapBlockWrite++;
    }

    *(int *)summaryPtr = blockNum;
    LfsSegSetSummaryPtr(segPtr,summaryPtr + sizeof(int));
    LfsSetStableMemBlockIndex((ClientData) (stableMemPtr), blockNum, 
			LfsSegDiskAddress(segPtr, bufferPtr));

    segPtr->activeBytes += stableMemPtr->paramsPtr->blockSize;
    return FALSE;
}



/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemCheckpoint --
 *
 *	Routine to handle writing of data for this module.
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
LfsStableMemCheckpoint(segPtr,  checkPointPtr, checkPointSizePtr, clientData)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    ClientData	clientData;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    Boolean	full = FALSE;
    int		blockNum;
    char	*bufPtr;


    if (stableMemPtr->dirtyBlocksBitMap == (int *) NIL) {
	return FALSE;
    }
    if (stableMemPtr->layoutBlocksBitMap == (int *) NIL) {
	Bit_Alloc(stableMemPtr->paramsPtr->maxNumBlocks, 
			stableMemPtr->layoutBlocksBitMap);
    }

    blockNum = Bit_FindFirstSet(stableMemPtr->paramsPtr->maxNumBlocks,
				stableMemPtr->dirtyBlocksBitMap);
    while(!full && (blockNum != -1)) { 
	if (Bit_IsClear(blockNum, stableMemPtr->layoutBlocksBitMap)) {
	    bufPtr = stableMemPtr->dataPtr + 
			    blockNum * stableMemPtr->paramsPtr->blockSize;
	    full = AddBlockToSegment(bufPtr, blockNum, stableMemPtr, segPtr);
	}
	if (!full) {
	    Bit_Set(blockNum, stableMemPtr->layoutBlocksBitMap);
	    Bit_Clear(blockNum, stableMemPtr->dirtyBlocksBitMap);
	    blockNum = Bit_FindFirstSet(stableMemPtr->paramsPtr->maxNumBlocks,
				stableMemPtr->dirtyBlocksBitMap);
	} 

    }
    if (!full) {
	*(LfsStableMemCheckPoint *) checkPointPtr = 
			*(stableMemPtr->checkpointPtr);
	bcopy(((char *) (stableMemPtr->checkpointPtr) + 
			sizeof(LfsStableMemCheckPoint)),
		checkPointPtr + sizeof(LfsStableMemCheckPoint), 
		sizeof(int) * stableMemPtr->checkpointPtr->numBlocks);
	*checkPointSizePtr = sizeof(int) * stableMemPtr->checkpointPtr->numBlocks + 
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
LfsStableMemWriteDone(segPtr, flags, clientData)
    LfsSeg *segPtr;	/* Segment whose write finishes. */
    int	    flags;	/* Write done flags. */
    ClientData clientData;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int	blockNum;
    LfsSegElement *bufferLimitPtr, *bufferPtr = LfsSegGetBufferPtr(segPtr);

    bufferLimitPtr = bufferPtr + LfsSegSummaryBytesLeft(segPtr) / sizeof(int);
    while (bufferPtr < bufferLimitPtr) {
	blockNum = (int) (bufferPtr->clientData);
	Bit_Clear(blockNum, stableMemPtr->layoutBlocksBitMap);
	bufferPtr++;
    }
    if (!Bit_AnySet(stableMemPtr->paramsPtr->maxNumBlocks,
			stableMemPtr->layoutBlocksBitMap)) {
	free((char *) stableMemPtr->layoutBlocksBitMap);
	stableMemPtr->layoutBlocksBitMap = (int *) NIL;
    }
    LfsSegSetBufferPtr(segPtr, bufferPtr);
}

