/* 
 * LfsStableMem.c --
 *
 *	Generic routines for supporting an in memory data structure that
 *	is written to a LFS log at each checkpoint.  The blocks of the
 *	data structures are kept as file in the file cache.
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
#include <lfsStableMemInt.h>
#include <stdlib.h>
#include <fsdm.h>

static Boolean AddBlockToSegment _ARGS_((LfsStableMem *smemPtr, 
			Address address, int blockNum, ClientData clientData,
			LfsSeg *segPtr));
static Boolean BlockMatch _ARGS_((Fscache_Block *blockPtr, 
			ClientData clientData));



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
LfsStableMemLoad(lfsPtr, smemParamsPtr, checkPointSize, checkPointPtr, smemPtr)
    Lfs *lfsPtr;	   /* File system of metadata. */
    LfsStableMemParams  *smemParamsPtr; /* Parameters for this memory. */
    int  checkPointSize;   /* Size of checkpoint data. */
    char *checkPointPtr;   /* Data from last checkpoint before shutdown. */
    LfsStableMem	*smemPtr; /* In memeory index data structures. */
{
    int	blockNum, bufferSize; 
    LfsStableMemCheckPoint *cpPtr;
    Fscache_Attributes	   attr;
    static	int	nextMinorNumber = -1;

    cpPtr = (LfsStableMemCheckPoint *) 	checkPointPtr;
    /*
     * Do some bounds checking on the checkpoint buffer.
     */
    if ((checkPointSize < sizeof(LfsStableMemCheckPoint)) ||
        (checkPointSize < (cpPtr->numBlocks * sizeof(int) + 
			    sizeof(LfsStableMemCheckPoint))))  {
	return FAILURE;
    }
    /*
     * Fill in the LfsStableMem data structures.
     *
     */
    smemPtr->lfsPtr = lfsPtr;
    /*
     * Initialize the file handle used for storing blocks in the
     * file cache.
     */

    bzero((Address)&smemPtr->dataHandle, sizeof(smemPtr->dataHandle));
    smemPtr->dataHandle.hdr.fileID.serverID = rpc_SpriteID;
    smemPtr->dataHandle.hdr.fileID.major = lfsPtr->domainPtr->domainNumber;
    smemPtr->dataHandle.hdr.fileID.minor = nextMinorNumber--;
    smemPtr->dataHandle.hdr.fileID.type = FSIO_LCL_FILE_STREAM;
    smemPtr->dataHandle.hdr.name = "LfsStableMemFile";
    smemPtr->dataHandle.descPtr = (Fsdm_FileDescriptor *)NIL;

    bzero((Address)&attr, sizeof(attr));
    attr.lastByte = smemParamsPtr->blockSize * smemParamsPtr->maxNumBlocks;
    Fscache_FileInfoInit(&smemPtr->dataHandle.cacheInfo,
		    (Fs_HandleHeader *) &smemPtr->dataHandle,
		    0, TRUE, &attr, lfsPtr->domainPtr->backendPtr);

    /*
     * Allocate and copy the index for the metadata.
     */
    bufferSize = smemParamsPtr->maxNumBlocks * sizeof(int);
    smemPtr->blockIndexPtr = (LfsDiskAddr *) malloc(bufferSize);
    bcopy(checkPointPtr + sizeof(LfsStableMemCheckPoint), 
		 (char *) smemPtr->blockIndexPtr, 
		 cpPtr->numBlocks * sizeof(LfsDiskAddr));
    for (blockNum = cpPtr->numBlocks; blockNum < smemParamsPtr->maxNumBlocks;
			blockNum++) {
	LfsSetNilDiskAddr(&smemPtr->blockIndexPtr[blockNum]);
    }

    smemPtr->numCacheBlocksOut = 0;
    /*
     * Fillin the rest of the LfsStableMem data structure with a copy
     * of the checkPoint and Params data.
     */

    smemPtr->checkPoint = *cpPtr;
    smemPtr->params = *smemParamsPtr;

    return SUCCESS;
}


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
LfsStableMemDestory(lfsPtr, smemPtr)
    Lfs *lfsPtr;	   /* File system of metadata. */
    LfsStableMem	*smemPtr; /* In memeory index data structures. */
{
    Fscache_FileInvalidate(&smemPtr->dataHandle.cacheInfo, 0, 
			FSCACHE_LAST_BLOCK);

    free((char *) smemPtr->blockIndexPtr);

    return SUCCESS;
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
LfsStableMemClean(segPtr, sizePtr, numCacheBlocksPtr, clientDataPtr, smemPtr)
    LfsSeg *segPtr;	/* Segment containing data to clean. */
    int *sizePtr;
    int *numCacheBlocksPtr;
    ClientData *clientDataPtr;
    LfsStableMem *smemPtr;	/* Index pointer. */
{
    char *summaryPtr;
    int	 *blockPtr;
    int	 numBlocks, block, blockOffset;
    LfsDiskAddr blockAddress;
    ReturnStatus	status;
    LfsStableMemEntry entry;

    summaryPtr = LfsSegGetSummaryPtr(segPtr);
    numBlocks = LfsSegSummaryBytesLeft(segPtr) / sizeof(int);
    blockPtr = (int *) summaryPtr;
    blockAddress = LfsSegDiskAddress(segPtr, LfsSegGetBufferPtr(segPtr));
    blockOffset = 0;
    /*
     * For each block that hasn't moved already, fetch and release the
     * block marking it as dirty.
     */
    for (block = 0; block < numBlocks; block++) { 
	LfsDiskAddr newDiskAddr;
	 blockOffset += LfsBytesToBlocks(segPtr->lfsPtr, 
					smemPtr->params.blockSize);
	LfsDiskAddrPlusOffset(blockAddress, -blockOffset, &newDiskAddr);
	if (LfsSameDiskAddr(smemPtr->blockIndexPtr[blockPtr[block]], 
			newDiskAddr)) {
	    int entryNumber = blockPtr[block] * smemPtr->params.entriesPerBlock;
	    status = LfsStableMemFetch(smemPtr, entryNumber, 0, &entry);
	    if (status != SUCCESS) {
		LfsError(segPtr->lfsPtr, status,"Can't clean metadata block\n");
	    }
	    LfsStableMemRelease(smemPtr, &entry, TRUE);
	    (*sizePtr) += smemPtr->params.blockSize;
	 }
    }
    return FALSE;
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
LfsStableMemCheckpoint(segPtr, checkPointPtr, flags, checkPointSizePtr,
			clientDataPtr, smemPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   flags;		/* Flags. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    ClientData *clientDataPtr;
    LfsStableMem *smemPtr;	/* Stable memory description. */
{
    Boolean	full = FALSE;

    full = LfsStableMemLayout(segPtr, LFS_CHECKPOINT_LAYOUT,
			clientDataPtr, smemPtr);
    /*
     * If we didn't fill the segment, copy the index to the checkpoint buffer.
     */
    if (!full) {
	*(LfsStableMemCheckPoint *) checkPointPtr = smemPtr->checkPoint;
	bcopy((char *) smemPtr->blockIndexPtr, 
		checkPointPtr + sizeof(LfsStableMemCheckPoint), 
		sizeof(LfsDiskAddr) * smemPtr->checkPoint.numBlocks);
	*checkPointSizePtr = sizeof(int) * smemPtr->checkPoint.numBlocks + 
				sizeof(LfsStableMemCheckPoint);

    }
    return full;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemLayout --
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
LfsStableMemLayout(segPtr, flags, clientDataPtr, smemPtr)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    int		flags;		/* Layout flags. */
    ClientData	*clientDataPtr;
    LfsStableMem *smemPtr;	/* Stable memory description. */
{
    Boolean	full = FALSE;
    Fscache_FileInfo	*cacheInfoPtr;
    Fscache_Block	*blockPtr;
    int			lastDirtyBlock;
    Boolean		fsyncOnly;


    /*
     * Find and layout all the dirty metadata blocks. 
     */
    if (*clientDataPtr == (ClientData) NIL) { 
	fsyncOnly = ((flags  & LFS_CHECKPOINT_LAYOUT) == 0);
	cacheInfoPtr = Fscache_GetDirtyFile(
			    segPtr->lfsPtr->domainPtr->backendPtr,
			    fsyncOnly, LfsFileMatch, 
		    (ClientData) (smemPtr->dataHandle.hdr.fileID.minor));
	if (cacheInfoPtr == (Fscache_FileInfo *) NIL) {
	    return FALSE;
	}
	*clientDataPtr = (ClientData) cacheInfoPtr;
    } else {
	cacheInfoPtr = (Fscache_FileInfo *) *clientDataPtr;
    }

    while(!full) { 
	blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch,
				(ClientData) 0, &lastDirtyBlock);
	if (blockPtr == (Fscache_Block *) NIL) {
	    break;
	}
	full = AddBlockToSegment(smemPtr, blockPtr->blockAddr, 
			blockPtr->blockNum, (ClientData) blockPtr, segPtr);
	if (full) {
	    Fscache_ReturnDirtyBlock(blockPtr, GEN_EINTR);
	} else {
#ifdef SOSP91
	Fscache_AddBlockToStats(cacheInfoPtr, blockPtr);
#endif SOSP91
	    smemPtr->numCacheBlocksOut++;
	}

    }
#ifdef SOSP91
	cacheInfoPtr->flags &= ~FSCACHE_REASON_FLAGS;
#endif SOSP91
    if (!full && (smemPtr->numCacheBlocksOut == 0)) {
	Fscache_ReturnDirtyFile(cacheInfoPtr, FALSE);
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
LfsStableMemWriteDone(segPtr, flags, clientDataPtr, smemPtr)
    LfsSeg *segPtr;	/* Segment whose write finishes. */
    int	    flags;	/* Write done flags. */
    ClientData *clientDataPtr;
    LfsStableMem *smemPtr;	/* Index description. */
{
    LfsSegElement *bufferLimitPtr, *bufferPtr = LfsSegGetBufferPtr(segPtr);
    Fscache_Block *blockPtr;

    blockPtr = (Fscache_Block *) NIL;
    bufferLimitPtr = bufferPtr + LfsSegSummaryBytesLeft(segPtr) / sizeof(int);
    while (bufferPtr < bufferLimitPtr) {
	blockPtr = (Fscache_Block *) bufferPtr->clientData;
	Fscache_ReturnDirtyBlock(blockPtr, SUCCESS);
	bufferPtr++;
	smemPtr->numCacheBlocksOut--;
    }
    if (smemPtr->numCacheBlocksOut == 0) {
	Fscache_ReturnDirtyFile((Fscache_FileInfo *)(*clientDataPtr), FALSE);
    }
    LfsSegSetBufferPtr(segPtr, bufferPtr);
}
/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemFetch --
 *
 *	Routine to fetch a stable memory entry and optionally release
 *	a previously fetched entry.   This routine fetches the data block
 *	from the file cache reading it in if it is not present.
 *
 * Results:
 *	SUCCESS if entry is fetch. GEN_INVALID_ARG if entryNumber is not
 *	vaild. Other ReturnStatus if disk read fails.
 *
 * Side effects:
 *	Cache block fetched. Possibly disk I/O performed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsStableMemFetch(smemPtr, entryNumber, flags, entryPtr)
    LfsStableMem *smemPtr;	/* Index description. */
    int		 entryNumber;	/* Entry number wanted. */
    int		flags;	/* LFS_STABLE_MEM_MAY_DIRTY | LFS_STABLE_MEM_REL_ENTRY*/
    LfsStableMemEntry *entryPtr; /* IN/OUT: Stable memory entry returned. */
{
    Fscache_Block	    *blockPtr;
    Boolean		    found;
    int 		blockNum, offset;
    ReturnStatus	status = SUCCESS;
    Boolean		releaseEntry;
    LfsStableMemBlockHdr *hdrPtr;

    releaseEntry = ((flags & LFS_STABLE_MEM_REL_ENTRY) != 0);
    if ((entryNumber < 0) || (entryNumber >= smemPtr->params.maxNumEntries)) {
	if (releaseEntry) {
	    LfsStableMemRelease(smemPtr, entryPtr, entryPtr->modified);
	}
	return GEN_INVALID_ARG;
    }
    blockNum = entryNumber / smemPtr->params.entriesPerBlock;
    offset = (entryNumber % smemPtr->params.entriesPerBlock) * 
		smemPtr->params.entrySize + sizeof(LfsStableMemBlockHdr);
    blockPtr = (Fscache_Block *) NIL;
    if (releaseEntry) {
	if (entryPtr->blockNum == blockNum) { 
	    blockPtr = (Fscache_Block *) entryPtr->clientData;
	} else {
	    LfsStableMemRelease(smemPtr, entryPtr, entryPtr->modified);
	}
    }
    if (blockPtr == (Fscache_Block *) NIL) {
	Boolean dirtied;
	int	cacheFlags;
	/*
	 * Fetch the block from the cache reading it in if needed.
	 */
	cacheFlags = FSCACHE_DESC_BLOCK|FSCACHE_CANT_BLOCK;
	if (flags & LFS_STABLE_MEM_MAY_DIRTY) {
	    cacheFlags |= FSCACHE_IO_IN_PROGRESS;
	}
	Fscache_FetchBlock(&smemPtr->dataHandle.cacheInfo, blockNum, 
			     cacheFlags, &blockPtr, &found);
	dirtied = FALSE;
	if (!found && (blockPtr != (Fscache_Block *) NIL) ) {
	    if (LfsIsNilDiskAddr(smemPtr->blockIndexPtr[blockNum])) {
		bzero(blockPtr->blockAddr, smemPtr->params.blockSize);
		hdrPtr = (LfsStableMemBlockHdr *) blockPtr->blockAddr;
		hdrPtr->magic = LFS_STABLE_MEM_BLOCK_MAGIC;
		hdrPtr->memType = smemPtr->params.memType;
		hdrPtr->blockNum = blockNum;
		dirtied = TRUE;
	     } else {
		status = LfsReadBytes(smemPtr->lfsPtr, 
				     smemPtr->blockIndexPtr[blockNum],
				     smemPtr->params.blockSize, 
				     blockPtr->blockAddr);
#ifdef ERROR_CHECK
		 if (smemPtr->params.memType != LFS_SEG_USAGE_MOD) {
		     LfsCheckRead(smemPtr->lfsPtr, 
				smemPtr->blockIndexPtr[blockNum], 
				smemPtr->params.blockSize);
		  }
#endif
	     }

	     if (status != SUCCESS) {
		Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
	     }
	}
	entryPtr->modified = dirtied;
    }
    if (blockPtr != (Fscache_Block *) NIL) {
#ifdef ERROR_CHECK
	hdrPtr = (LfsStableMemBlockHdr *) blockPtr->blockAddr;
	if ((hdrPtr->magic != LFS_STABLE_MEM_BLOCK_MAGIC) || 
	    (hdrPtr->memType != smemPtr->params.memType) ||
	    (hdrPtr->blockNum != blockNum)) {
	    LfsError(smemPtr->lfsPtr, FAILURE, "Bad LfsStableMemBlockHdr\n");
	}
#endif /* ERROR_CHECK */
	entryPtr->addr = blockPtr->blockAddr + offset;
	entryPtr->blockNum = blockNum;
	entryPtr->clientData = (ClientData) blockPtr;
    } else {
	status = FS_WOULD_BLOCK;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsStableMemRelease --
 *
 *	Routine to release a previous fetched stable memory ebtrt,
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
LfsStableMemRelease(smemPtr, entryPtr, modified)
    LfsStableMem *smemPtr;	/* Index description. */
    LfsStableMemEntry *entryPtr; /*  Stable memory entry to return. */
    Boolean	modified;	/* TRUE if block was modified. */
{
    Fscache_Block	    *blockPtr;
    int			timeDirtied, blockNum;

    blockPtr = (Fscache_Block *) entryPtr->clientData;
    blockNum = blockPtr->blockNum;
    modified = modified || entryPtr->modified;
    timeDirtied = modified ? -1 : 0;
    Fscache_UnlockBlock(blockPtr, timeDirtied, blockNum,  
			smemPtr->params.blockSize, 0);
    entryPtr->addr = (Address)NIL;
    if (modified && !LfsIsNilDiskAddr(smemPtr->blockIndexPtr[blockNum])) {
        /*
	 * If the block was modified free the old address up.
	 */
	LfsSegUsageFreeBlocks(smemPtr->lfsPtr, 
		(int)(smemPtr->params.blockSize), 1, 
		(LfsDiskAddr *) smemPtr->blockIndexPtr + blockNum);

    }
    return;
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
AddBlockToSegment(smemPtr, address, blockNum, clientData, segPtr)
    LfsStableMem *smemPtr;	/* Index of block. */
    Address	  address;	/* Address of block. */
    int		blockNum;	/* Block number. */
    ClientData	clientData;	/* Client data for entry. */
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
    bufferPtr = LfsSegAddDataBuffer(segPtr, fsBlocks, address, clientData);
    *(int *)summaryPtr = blockNum;
    LfsSegSetSummaryPtr(segPtr,summaryPtr + sizeof(int));
#ifdef ERROR_CHECK
    if (!LfsIsNilDiskAddr(smemPtr->blockIndexPtr[blockNum])) {
	panic("StableMem:AddBlockToSegment disk address not NIL\n");
    }
#endif
    smemPtr->blockIndexPtr[blockNum] = LfsSegDiskAddress(segPtr, bufferPtr);
    segPtr->activeBytes += smemPtr->params.blockSize;
    if (blockNum >= smemPtr->checkPoint.numBlocks) {
	smemPtr->checkPoint.numBlocks = blockNum + 1;
    }
    return FALSE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * BlockMatch --
 *
 * 	Cache backend block type match.  
 *
 * Results:
 *	TRUE.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Boolean
BlockMatch(blockPtr, clientData)
    Fscache_Block *blockPtr;
    ClientData	   clientData;
{
    return TRUE;
}

