/* 
 * lfsFileLayout.c --
 *
 *	Control the writing, cleaning, and checkpointing of file and 
 *	file descriptors.
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

#include "lfsInt.h"
#include "lfsSeg.h"
#include "lfsFileLayout.h"
#include "lfsDesc.h"

#include "fsdm.h"
#include "fscache.h"
#include "rpc.h"

Boolean	lfsFileLayoutDebug = FALSE;

typedef struct FileSegLayout {
    int	 numDescSlotsLeft;	/* Number of slots left in descriptor block. */
    LfsFileDescriptor *descBlockPtr;		
				/* Pointer to next slot in descriptor block. */
    int descDiskAddr;  /* Disk address of descriptor block. */
} FileSegLayout;

static Boolean PlaceFileInSegment();

static Boolean FileMatch(), BlockMatch();

static LfsSegIoInterface layoutIoInterface = 
	{ LfsFileLayoutAttach, LfsFileLayoutProc, LfsFileLayoutClean,
	  LfsFileLayoutCheckpoint, LfsFileLayoutWriteDone,  0};


/*
 *----------------------------------------------------------------------
 *
 * LfsFileLayoutInit --
 *
 *	Initialize the call back structure is for LfsFileLayout().  
 *
 * Results:
 *	None
 *	
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
LfsFileLayoutInit()
{
    LfsSegIoRegister(LFS_FILE_LAYOUT_MOD,&layoutIoInterface);
}


/*
 *----------------------------------------------------------------------
 *
 * FileLayoutAttach --
 *
 *	Attach routine for file layout module. Creates and initializes the
 *	data structures used for writeback for this file system.
 *
 * Results:
 *	SUCCESS if attaching is going ok.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
LfsFileLayoutAttach(lfsPtr, checkPointSize, checkPointPtr)
    Lfs   *lfsPtr;	     /* File system for attach. */
    int   checkPointSize;    /* Size of checkpoint data. */
    char  *checkPointPtr;     /* Data from last checkpoint before shutdown. */
{
    LfsFileLayout	      *layoutPtr = &(lfsPtr->fileLayout);
    LfsFileLayoutParams	      *paramsPtr = &(lfsPtr->superBlock.fileLayout);
    /*
     * Initialize block cache for this file system.
     */
    layoutPtr->params = *paramsPtr;
    layoutPtr->writeBackEverything = FALSE;

    LfsDescCacheInit(lfsPtr);

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileLayoutProc --
 *
 *	Routine to handle layout of file block in segments.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsFileLayoutProc(segPtr, cleaning, clientDataPtr)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    Boolean cleaning;		/* TRUE if cleaning. */
    ClientData	*clientDataPtr;
{
    Lfs		    *lfsPtr =   segPtr->lfsPtr;
    LfsFileLayout    *layoutPtr = &(lfsPtr->fileLayout);
    Fscache_FileInfo	*cacheInfoPtr;
    Boolean	    full, fsyncOnly;
    FileSegLayout  *segLayoutDataPtr;

     /*
      * Next spill the file with dirty blocks into the segment. 
      */

     full = FALSE;
     fsyncOnly = !cleaning;
     if (*clientDataPtr == (ClientData) NIL) {
	 segLayoutDataPtr = (FileSegLayout *) malloc(sizeof(FileSegLayout));
	 bzero((char *)segLayoutDataPtr, sizeof(FileSegLayout));
	 *clientDataPtr = (ClientData) segLayoutDataPtr;
     } else {
	 segLayoutDataPtr = (FileSegLayout *) *clientDataPtr;
     }
     (void) Fscache_GetDirtyFiles(lfsPtr->domainPtr->backendPtr, fsyncOnly,
			 FileMatch, (ClientData) cleaning, 1, &cacheInfoPtr);
     while (!full && (cacheInfoPtr != (Fscache_FileInfo *) NIL)) {
	   full = PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr,
			cleaning, segLayoutDataPtr);
	   if (full) {
	       break;
	   }
	 (void) Fscache_GetDirtyFiles(lfsPtr->domainPtr->backendPtr, 
			layoutPtr->writeBackEverything,
			 FileMatch, (ClientData) cleaning, 1, &cacheInfoPtr);
     }
     return full;

}



/*
 *----------------------------------------------------------------------
 *
 * FileLayoutCheckpoint --
 *
 *	Routine to handle checkpointing of the file layout data.
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
/*ARGSUSED*/
Boolean
LfsFileLayoutCheckpoint(segPtr, flags, checkPointPtr,  checkPointSizePtr,
			clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    ClientData *clientDataPtr;
{
    LfsFileLayout  *layoutPtr = &(segPtr->lfsPtr->fileLayout);
    Boolean full;

    /*
     * Write-back everything that is dirty.
     */
    layoutPtr->writeBackEverything = TRUE;
    full = LfsFileLayoutProc(segPtr, FALSE, clientDataPtr);
    if (!full) {
	layoutPtr->writeBackEverything = FALSE;
    }
    return full;

}

/*
 *----------------------------------------------------------------------
 *
 * FileLayoutWriteDone --
 *
 *	Routine to handle finishing of file layout writes
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
LfsFileLayoutWriteDone(segPtr, flags, clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
    ClientData *clientDataPtr;
{
    Lfs		  *lfsPtr = segPtr->lfsPtr;
    LfsSegElement *bufferPtr = LfsSegGetBufferPtr(segPtr);
    char	 *summaryPtr =  LfsSegGetSummaryPtr(segPtr);
    char	 *limitPtr;

    if (*clientDataPtr != (ClientData) NIL) {
	free((char *) *clientDataPtr);
	*clientDataPtr = (ClientData) NIL;
    }
    limitPtr = summaryPtr + LfsSegSummaryBytesLeft(segPtr); 
     while (summaryPtr < limitPtr) { 
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
#ifdef notdef
	    LfsFileDescriptor	*descPtr;
	    int		i;
	    /*
	     * A block of descriptors.  For each descriptor we now can 
	     * update the descriptor map to point the descriptor at it.
	     */
	     descPtr = (LfsFileDescriptor *) bufferPtr->address;
	     for (i = 0; (i < layoutPtr->params.descPerBlock) && 
			 (descPtr->common.flags != 0); i++) {
		 UpdateIndex(lfsPtr, descPtr->fileNumber, 
					LfsSegDiskAddress(segPtr,bufferPtr));
	     }
#endif
	     free(bufferPtr->address);
	     bufferPtr++;
	     summaryPtr += sizeof(LfsFileLayoutDesc);
	     break;
	}
	case LFS_FILE_LAYOUT_DATA:  {
	    LfsFileLayoutSummary *fileSumPtr;
	    Fscache_Block *blockPtr;
	    int		block;
	    /* 
	     * All these records should be pointing to a cache blocks which
	     * must be released.
	     */
	    fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	    for (block = 0; block < fileSumPtr->numDataBlocks; block++) {
		blockPtr = (Fscache_Block *) bufferPtr->clientData;
		FscacheReturnDirtyBlocks(1,&blockPtr,SUCCESS);
		bufferPtr++;
	    }
	    if (fileSumPtr->numDataBlocks > 0) {
		FscacheReturnDirtyFiles(1, &(blockPtr->cacheInfoPtr), TRUE);
	    }
	    summaryPtr += (sizeof(LfsFileLayoutSummary) + 
			   fileSumPtr->numDataBlocks * sizeof(int));
	    break;
	}
	case LFS_FILE_LAYOUT_DIR_LOG: {
	    LfsFileLayoutLog	*logSumPtr = (LfsFileLayoutLog *) summaryPtr;
	     /*
	      * Because we copied and truncated the log during layout we 
	      * don't need to do anything on write complete.
	      */
	    summaryPtr = summaryPtr + logSumPtr->numBytes;
	    break;
	}
	case LFS_FILE_LAYOUT_DBL_INDIRECT: 
	case LFS_FILE_LAYOUT_INDIRECT: 
	default:
	    panic("Bad file block type in summary block");
	}
    }
    LfsSegSetBufferPtr(segPtr, bufferPtr);
    return;

}


/*
 *----------------------------------------------------------------------
 *
 * FileLayoutClean --
 *
 *	Routine to handle cleaning of file data blocks.
 *
 * Results:
 *	TRUE if we couldn't clean the segment.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsFileLayoutClean(segPtr, sizePtr, numCacheBlocksPtr, clientDataPtr)
    LfsSeg *segPtr;	/* Segment containing data to clean. */
    int	   *sizePtr;
    int    *numCacheBlocksPtr;
    ClientData *clientDataPtr;
{
    Lfs		   *lfsPtr = segPtr->lfsPtr;
    LfsFileLayout  *layoutPtr = &(lfsPtr->fileLayout);
    LfsFileLayoutSummary  *fileSumPtr;
    char	*summaryPtr, *limitPtr;
    int address, blockOffset, fsBlocks;
    Fscache_FileInfo	*cacheInfoPtr;
    ReturnStatus	status;
    Boolean	error;

     error = FALSE;
     fsBlocks = LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
     summaryPtr =  LfsSegGetSummaryPtr(segPtr);
     limitPtr = summaryPtr + LfsSegSummaryBytesLeft(segPtr);
     address = LfsSegDiskAddress(segPtr, LfsSegGetBufferPtr(segPtr));
     blockOffset = 0;

     while ((summaryPtr < limitPtr) && !error) { 
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    int diskAddr;
	    int		fileNumber;
	    int		slot, size;
	    LfsFileDescriptor	*descPtr;
	    char		*blockStartPtr;
	    Fscache_Block *descCacheblockPtr = (Fscache_Block *) NIL;
	    /*
	     * A block of descriptors.  For each descriptor that is live
	     * (being pointed to by the descriptor map) we bring it into
	     * the system and mark it as dirty. 
	     */
	    size = layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor);
	    blockOffset += LfsBytesToBlocks(lfsPtr, size);
	    blockStartPtr = LfsSegFetchBytes(segPtr, 
				segPtr->curBlockOffset + blockOffset, size);
	    descPtr = (LfsFileDescriptor *)blockStartPtr;
	    for (slot = 0; slot < layoutPtr->params.descPerBlock; slot++) {
		Fs_FileID fileID;
		Fsio_FileIOHandle *newHandlePtr;
		Boolean	found;
		/*
		 * The descriptor block is terminated by an unallocated
		 * descriptor.
		 */
		if (!(descPtr[slot].common.flags & FSDM_FD_ALLOC)) {
		    break;
		}
		fileNumber = descPtr[slot].fileNumber;
		status = LfsDescMapGetDiskAddr(lfsPtr, fileNumber, &diskAddr);
		/*
		 * If the file is not allocated or this descriptor is not
		 * the most current copy, skip it.
		 */
		if ((status == FS_FILE_NOT_FOUND) ||
		    ((status == SUCCESS) && 
				(diskAddr != address - blockOffset))) {
		    continue;
		}
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, 
			"Bad file number in descriptor block.\n");
		    continue;
		}
		if (descCacheblockPtr == (Fscache_Block *) NIL) {
		    /*
		     * We get to read it into the cache if its not already
		     * there. 
		     */
		    Fscache_FetchBlock(&lfsPtr->descCacheHandle.cacheInfo,
			    diskAddr, (FSCACHE_DESC_BLOCK|FSCACHE_CANT_BLOCK),
				    &descCacheblockPtr, &found);
		    if (descCacheblockPtr == (Fscache_Block *) NIL) {
			printf("Can't fetch descriptor block\n");
			error = TRUE;
			break;
		    }
		    if (!found) {
			bcopy(blockStartPtr, descCacheblockPtr->blockAddr,
			       lfsPtr->fileLayout.params.descPerBlock * 
				     sizeof(*descPtr));
			Fscache_IODone(descCacheblockPtr);
		    }
		}
		/*
		 * Grab a handle for this file.
		 */
		fileID.type = FSIO_LCL_FILE_STREAM;
		fileID.serverID = rpc_SpriteID;
		fileID.major = lfsPtr->domainPtr->domainNumber;
		fileID.minor = fileNumber;
		status = Fsio_LocalFileHandleInit(&fileID, (char *) NIL, 
				    (Fsdm_FileDescriptor *) NIL, TRUE, 
				    &newHandlePtr);
		if (status == FS_WOULD_BLOCK) {
		    error = TRUE;
		    break;
		}
		if (status == FS_FILE_REMOVED) {
		    continue;
		}
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, 
				"Can't get handle to clean file\n");
		}
		*sizePtr += sizeof(LfsFileDescriptor);
		Fscache_PutFileOnDirtyList(&newHandlePtr->cacheInfo, 
				    (int)(FSCACHE_FILE_BEING_CLEANED |
				    FSCACHE_FILE_DESC_DIRTY));
		Fsutil_HandleRelease(newHandlePtr, TRUE);
	    }
	    if (descCacheblockPtr != (Fscache_Block *) NIL) {
		Fscache_UnlockBlock(descCacheblockPtr,(unsigned)0, -1, 
					FS_BLOCK_SIZE, 0);
	    }
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    break;
	}
	case LFS_FILE_LAYOUT_DATA: {
	    int	*blockArray, diskAddress;
	    int			 startBlockOffset, i;
	    unsigned short	curTruncVersion;
	    /*
	     * We ran into a data block. If it is still alive bring it into
	     * the cache. 
	     */
	     fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	     startBlockOffset = blockOffset;
	     /*
	      * Liveness check.   First see if the version number is
	      * the same and the file is still allocated.
	      */
	     status = LfsDescMapGetVersion(lfsPtr, 
			(int)fileSumPtr->fileNumber, &curTruncVersion);
	     if ((status == SUCCESS) && 
		 (curTruncVersion == fileSumPtr->truncVersion)) {
		Fs_FileID fileID;
		Fsio_FileIOHandle *newHandlePtr;

		/*
		 * So far so good.  File is allocated and version number 
		 * says it hasn't been deleted or truncated. Grap a 
		 * handle for the file a check to see if the blocks 
		 * are still a member of the file.
		 */
		fileID.type = FSIO_LCL_FILE_STREAM;
		fileID.serverID = rpc_SpriteID;
		fileID.major = lfsPtr->domainPtr->domainNumber;
		fileID.minor = fileSumPtr->fileNumber;
		status = Fsio_LocalFileHandleInit(&fileID, (char *) NIL, 
				    (Fsdm_FileDescriptor *) NIL, TRUE, 
				    &newHandlePtr);
		if (status == FS_WOULD_BLOCK) {
		    error = TRUE;
		    break;
		}
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status,
			    "Can't get handle to clean file\n");
		}
		/*
		 * For each block ... 
		 */
		blockArray = (int *)(summaryPtr + sizeof(LfsFileLayoutSummary));
		for (i = 0; i < fileSumPtr->numDataBlocks; i++) {
		    int numBlocks;
		    numBlocks = (fileSumPtr->numBlocks - 
					  (blockOffset - startBlockOffset));
		    if (numBlocks > fsBlocks) {
			numBlocks = fsBlocks;
		    }
		    blockOffset += numBlocks;
		    status = LfsFile_GetIndex(newHandlePtr, blockArray[i],
						TRUE, &diskAddress);
		    if ((status == SUCCESS) &&
			(diskAddress == address - blockOffset)) { 
			int	blockSize, flags, blocksLeft;
			Boolean	 found;
			Fscache_Block *blockPtr;
			/*
			 * The block exists and is at the location we are
			 * cleaning, bring it into the cache. Be a little
			 * careful about short blocks.
			 */
		        blockSize = LfsBlocksToBytes(lfsPtr, numBlocks);
			flags = (FSCACHE_IO_IN_PROGRESS | FSCACHE_CANT_BLOCK |
			          ((blockArray[i] >= 0) ? FSCACHE_DATA_BLOCK :
							  FSCACHE_IND_BLOCK));

		        Fscache_FetchBlock(&newHandlePtr->cacheInfo,
				      blockArray[i], flags, &blockPtr, &found);
			if (blockPtr == (Fscache_Block *) NIL) {
			    printf("Can't fetch cache block for cleaning.\n");
			    error = TRUE;
			    break;
			}
			if (!found) {
			    char *dPtr;
			    /*
			     * Its not in the cache already, copy it in.
			     * Handle the cache that it in a short fragment.
			     */
			    if (blockArray[i] >= 0) {
				if (blockArray[i] * FS_BLOCK_SIZE + 
						blockSize - 1 >
					newHandlePtr->descPtr->lastByte) {
				    blockSize = newHandlePtr->descPtr->lastByte
					- blockArray[i] * FS_BLOCK_SIZE + 1;
				}
			    } else if (blockSize != FS_BLOCK_SIZE) {
				panic("Illegal sized indirect block.\n");
			    }
			    if (blockSize <= 0) { 
				panic("Illegal sized block.\n");
			    }

			    dPtr = LfsSegFetchBytes(segPtr, segPtr->curBlockOffset + blockOffset, 
					blockSize);
			    bcopy(dPtr, blockPtr->blockAddr, blockSize);
			    bzero(blockPtr->blockAddr + blockSize,
					FS_BLOCK_SIZE - blockSize);
			    Fscache_UnlockBlock(blockPtr, fsutil_TimeInSeconds,
						-1, blockSize, 
						FSCACHE_FILE_BEING_CLEANED);

			} else { 
			    /*
			     * Checking it out.
			     */
#define ERROR_CHECK
#ifdef ERROR_CHECK
			    char *dPtr;
			    Boolean bad;
			    dPtr = LfsSegFetchBytes(segPtr, blockOffset + segPtr->curBlockOffset, 
					blockSize);
			    bad = bcmp(dPtr, blockPtr->blockAddr, 
					blockPtr->blockSize);
			    if (bad && !(blockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
				panic("Block cleaned doesn't match block.\n");
			    }
#endif
			    Fscache_UnlockBlock(blockPtr, fsutil_TimeInSeconds,
						-1, blockPtr->blockSize, 
						FSCACHE_FILE_BEING_CLEANED);
			}
			*sizePtr += blockSize;
			*numCacheBlocksPtr++;
		    }
		}
		Fsutil_HandleRelease(newHandlePtr, TRUE);
	    }
	    blockOffset = startBlockOffset + fileSumPtr->numBlocks;
	    summaryPtr += sizeof(LfsFileLayoutSummary) + 
				fileSumPtr->numDataBlocks * sizeof(int); 
	    break;
	  }

	case LFS_FILE_LAYOUT_DIR_LOG: {
	    LfsFileLayoutLog	*logSumPtr;
	    /* 
	     * Directory log info is not needed during clean so we 
	     * just skip over it.
	     */
	     logSumPtr = (LfsFileLayoutLog *) summaryPtr;
	     summaryPtr = summaryPtr + logSumPtr->numBytes;
	     break;
	}
	case LFS_FILE_LAYOUT_DBL_INDIRECT: 
	case LFS_FILE_LAYOUT_INDIRECT: 
	default: {
	    panic("Unknown type");
	}
      }
    }

    return error;

}


/*
 *----------------------------------------------------------------------
 *
 * PlaceFileInSegment --
 *
 *	Place specified file dirty in segment.
 *
 * Results:
 *	TRUE if the segment filled before the file was fully added.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean 
PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr, cleaning,
		segLayoutDataPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsSeg	*segPtr;	/* Segment to place data. */
    Fscache_FileInfo *cacheInfoPtr;	/* File to place in segment. */
    LfsFileLayout     *layoutPtr;   /* File layout info. */
    Boolean	     cleaning;	    /* TRUE if cleaning. */
    FileSegLayout  *segLayoutDataPtr; /* Current layout data for segment. */
{
    LfsFileLayoutDesc	*descSumPtr;
    LfsFileLayoutSummary *fileSumPtr;
    Boolean	full = FALSE;
    int	lfsBlocksPerBlock = LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
    int		blockType, blocksNeeded, descBlocks;
    char	*summaryPtr;
    ReturnStatus	status;
    Fscache_Block	*blockPtr;
    Boolean		fileNotUsed = TRUE;
    int			lastDirtyBlock;

    if (cacheInfoPtr == (Fscache_FileInfo *) NIL) {
	return FALSE;
    }
    /*
    * If the segment we are adding has no slots open in the descriptor
    * block and there is a change of the all the dirty blocks fitting in the 
    * segment we create a new descriptor block in the segment.
    */
    if (segLayoutDataPtr->numDescSlotsLeft == 0) {
	blocksNeeded = LfsBytesToBlocks(lfsPtr,
			cacheInfoPtr->numDirtyBlocks*FS_BLOCK_SIZE);
	descBlocks = LfsBytesToBlocks(lfsPtr, 
		layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor));
	if (LfsSegBlocksLeft(segPtr) >= (blocksNeeded + descBlocks)) { 
	    summaryPtr = LfsSegGrowSummary(segPtr, descBlocks, 
					    sizeof(LfsFileLayoutDesc));
	    if (summaryPtr != (char *) NIL) {
		LfsSegElement *descBufferPtr;
		/*
		 * Allocate space for the descriptor block and fill in a
		 * summary block describing it. 
		 */
		descBufferPtr = LfsSegAddDataBuffer(segPtr, descBlocks,
				malloc(LfsBlocksToBytes(lfsPtr, descBlocks)),
				(ClientData) NIL);
		descSumPtr = (LfsFileLayoutDesc *) summaryPtr;
    
		descSumPtr->blockType = LFS_FILE_LAYOUT_DESC;
		descSumPtr->numBlocks = descBlocks;

		summaryPtr += sizeof(LfsFileLayoutDesc);
		LfsSegSetSummaryPtr(segPtr, summaryPtr);

		segLayoutDataPtr->numDescSlotsLeft = 
						layoutPtr->params.descPerBlock;
		segLayoutDataPtr->descBlockPtr = (LfsFileDescriptor *) 
					    descBufferPtr->address;
		segLayoutDataPtr->descDiskAddr = 
				    LfsSegDiskAddress(segPtr, descBufferPtr);
		bzero(descBufferPtr->address, 
		   layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor));
	    }
	}
    }
    /*
    * Allocate room in the summary header for file and at least one 
    * data block for this type.
    */
    if (cacheInfoPtr->numDirtyBlocks > 0) { 
       int bytesNeeded;
       bytesNeeded = sizeof(LfsFileLayoutSummary) + sizeof(int);
       summaryPtr = LfsSegGrowSummary(segPtr, lfsBlocksPerBlock, bytesNeeded);
       if (summaryPtr == (char *) NIL) { 
	    full = TRUE;
       } else {
	   fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	   fileSumPtr->blockType = LFS_FILE_LAYOUT_DATA;
	   fileSumPtr->numBlocks = 0; /* Filled in later. */
	   fileSumPtr->fileNumber = cacheInfoPtr->hdrPtr->fileID.minor;
	   status = LfsDescMapGetVersion(lfsPtr, (int)fileSumPtr->fileNumber,
					    &fileSumPtr->truncVersion);
	   if (status != SUCCESS) {
	       LfsError(lfsPtr, status, "Can't get truncate version number\n");
	   }
	   fileSumPtr->numDataBlocks = 0; /* Filled in later. */
	   summaryPtr += sizeof(LfsFileLayoutSummary);
	   LfsSegSetSummaryPtr(segPtr, summaryPtr);
	   for (blockType = LFS_FILE_LAYOUT_DATA; 
		 (blockType <= LFS_FILE_LAYOUT_DBL_INDIRECT);
		 blockType++) { 
		/*
		 * Place all blocks of this type in the segment.
		 */
		blockPtr = (Fscache_Block *) NIL;
		while (!full) {
		     LfsSegElement *bufferPtr;
		     int blocks;
		     blocks = FscacheGetDirtyBlocks(cacheInfoPtr, BlockMatch,
				(ClientData) blockType, 1, 
				&blockPtr, &lastDirtyBlock);
		     if (blocks == 0) {
			break;
		     }
		     /*
		      * Compute the number of file system blocks this block
		      * will take.
		      */
#ifdef notdef
		     blocks = LfsBytesToBlocks(lfsPtr, blockPtr->blockSize);
#else
		     blocks = LfsBytesToBlocks(lfsPtr,FS_BLOCK_SIZE);
#endif
		     /*
		      * Be sure there is room for the block and summary info.
		      */
		     summaryPtr = LfsSegGrowSummary(segPtr, blocks,sizeof(int));
		     if (summaryPtr == (char *) NIL) {
			 FscacheReturnDirtyBlocks(1,&blockPtr, GEN_EINTR);
			 full = TRUE;
			 break;
		     }
		     fileNotUsed = FALSE;
		     *(int *) summaryPtr = blockPtr->blockNum;
		     summaryPtr += sizeof(int);
		     bufferPtr = LfsSegAddDataBuffer(segPtr, blocks, 
				    blockPtr->blockAddr, (ClientData) blockPtr);
		     status = LfsFile_SetIndex(
				(Fsio_FileIOHandle *)(cacheInfoPtr->hdrPtr), 
					       blockPtr->blockNum, TRUE,
				         LfsSegDiskAddress(segPtr, bufferPtr));
		     if (status != SUCCESS) {
			 LfsError(lfsPtr, status, "Can't update file index");
		     }
		     LfsSegSetSummaryPtr(segPtr,summaryPtr);
		     fileSumPtr->numDataBlocks++; 
		     fileSumPtr->numBlocks += blocks;
		     segPtr->activeBytes += LfsBlocksToBytes(lfsPtr,
			LfsBytesToBlocks(lfsPtr, blockPtr->blockSize));
;
	     if (lfsFileLayoutDebug ) { 
	     printf("LfsFileLayout: Adding block %d (%d bytes) of file %d\n",
				blockPtr->blockNum, blockPtr->blockSize, 
				cacheInfoPtr->hdrPtr->fileID.minor);
	      }
		 }
	  }
       }
    } 
     /*
      * If we successfully place this file in the segment add the descriptor
      * to the descriptor block. If this is no room then mark the segment 
      * as full.
      */
     if (!full) {
	 if (segLayoutDataPtr->numDescSlotsLeft > 0) {
	      Fsio_FileIOHandle *localHandlePtr;
	      Fsdm_FileDescriptor *descPtr;
	      localHandlePtr = (Fsio_FileIOHandle *) (cacheInfoPtr->hdrPtr);
	      descPtr = localHandlePtr->descPtr;
	      /*
	       * XXX - need to do this under lock. 
	       */
	      cacheInfoPtr->flags &= ~FSCACHE_FILE_DESC_DIRTY;
	      descPtr->flags &= ~FSDM_FD_DIRTY;	
	      bcopy((char *) descPtr, &(segLayoutDataPtr->descBlockPtr->common),
		    (int)sizeof(*descPtr));
	      segLayoutDataPtr->descBlockPtr->fileNumber = 
			      cacheInfoPtr->hdrPtr->fileID.minor;
	      status = LfsDescMapSetDiskAddr(lfsPtr, 
			    (int)segLayoutDataPtr->descBlockPtr->fileNumber, 
			    segLayoutDataPtr->descDiskAddr);
	      if (status != SUCCESS) {
		  LfsError(lfsPtr, status, "Can't update descriptor map.\n");
	      }
#ifdef notdef
	      LfsProcessCleanBlock(cacheInfoPtr,(Fscache_Block *) NIL,SUCCESS);
#endif
	      segLayoutDataPtr->descBlockPtr++;
	      segLayoutDataPtr->numDescSlotsLeft--;
	      segPtr->activeBytes += sizeof(LfsFileDescriptor);
	     if (lfsFileLayoutDebug ) { 
	      printf("LfsFileLayout: Adding desc of %d to block at %d\n",
			cacheInfoPtr->hdrPtr->fileID.minor, 
			segLayoutDataPtr->descDiskAddr);
	     }

	 } else {
	     full = TRUE;
	 }
     }
     if (fileNotUsed) {
	 /*
	  * If we didn't use any of the blocks for this file return 
	  * it to the dirty list.
	  */
	 FscacheReturnDirtyFiles(1,&cacheInfoPtr,TRUE);
     }
     return full;
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
    int blockLevel = (int) clientData;

    /*
     * The match fails if:
     * a) We want a data block and the block is an indirect block.
     *  	or
     * b) We want an indirect block and the blocks is a data or
     *	  double indirect block.
     *		or
     * c) We want a double indirect block and don't get it.
     */
    if ( ((blockLevel == LFS_FILE_LAYOUT_DATA) && (blockPtr->blockNum < 0)) ||
	 ((blockLevel == LFS_FILE_LAYOUT_INDIRECT) && 
		((blockPtr->blockNum >= 0) || (blockPtr->blockNum == -2))) || 
	 ((blockLevel == LFS_FILE_LAYOUT_DBL_INDIRECT) && 
		 (blockPtr->blockNum != -2))) { 
	return FALSE;
    } 

    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FileMatch --
 *
 * 	Cache backend file match for LFS.
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
FileMatch(cacheInfoPtr, clientData)
    Fscache_FileInfo *cacheInfoPtr;
    ClientData	clientData;
{
    Boolean	cleaning = (Boolean) clientData;
    Boolean 	retVal;

    if (cleaning) {
	retVal = ((cacheInfoPtr->flags & FSCACHE_FILE_BEING_CLEANED) != 0);
    } else {
	retVal = ((cacheInfoPtr->flags & FSCACHE_FILE_BEING_CLEANED) == 0);
    }
    return retVal;
}

