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
    unsigned int descDiskAddr;  /* Disk address of descriptor block. */
} FileSegLayout;

static Boolean PlaceFileInSegment();

static ReturnStatus FileLayoutAttach();
static Boolean	FileLayout(), FileLayoutClean(), FileLayoutCheckpoint();
static void FileLayoutWriteDone();

static LfsSegIoInterface layoutIoInterface = 
	{ FileLayoutAttach, FileLayout, FileLayoutClean,
	  FileLayoutCheckpoint, FileLayoutWriteDone,  0};


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

static ReturnStatus
FileLayoutAttach(lfsPtr, checkPointSize, checkPointPtr)
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
 * FileLayout --
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

static Boolean
FileLayout(segPtr)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
{
    Lfs		    *lfsPtr =   segPtr->lfsPtr;
    LfsFileLayout    *layoutPtr = &(lfsPtr->fileLayout);
    Fscache_FileInfo	*cacheInfoPtr;
    Boolean	    full;
    FileSegLayout  segLayoutData;

     /*
      * Next spill the file with dirty blocks into the segment. 
      */

     full = FALSE;
     bzero((char *) &segLayoutData, sizeof(segLayoutData));
     LfsGetDirtyFile(&cacheInfoPtr);
     while (!full && (cacheInfoPtr != (Fscache_FileInfo *) NIL)) {
	   full = PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr,
			&segLayoutData);
	   if (full) {
	       break;
	   }
	   LfsGetDirtyFile(&cacheInfoPtr);
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

static Boolean
FileLayoutCheckpoint(segPtr, flags, checkPointPtr,  checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags. 
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    LfsFileLayout  *layoutPtr = &(segPtr->lfsPtr->fileLayout);
    Boolean full;

    /*
     * Write-back everything that is dirty.
     */
    layoutPtr->writeBackEverything = TRUE;
    full = FileLayout(segPtr);
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

static void
FileLayoutWriteDone(segPtr, flags)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
{
    Lfs		  *lfsPtr = segPtr->lfsPtr;
    LfsFileLayout	      *layoutPtr = &(lfsPtr->fileLayout);
    LfsSegElement *bufferPtr = LfsSegGetBufferPtr(segPtr);
    char	 *summaryPtr =  LfsSegGetSummaryPtr(segPtr);
    char	 *limitPtr;

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
		LfsProcessCleanBlock(blockPtr->cacheInfoPtr,blockPtr,SUCCESS);
		bufferPtr++;
	    }
	    if (fileSumPtr->numDataBlocks > 0) {
		LfsReturnDirtyFile(blockPtr->cacheInfoPtr);
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
    LfsSegSetSummaryPtr(segPtr, summaryPtr);
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
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

static Boolean
FileLayoutClean(segToCleanPtr, segPtr)
    LfsSeg *segToCleanPtr;	/* Segment containing data to clean. */
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
{
    Lfs		   *lfsPtr = segToCleanPtr->lfsPtr;
    LfsFileLayout  *layoutPtr = &(lfsPtr->fileLayout);
    LfsSegSummaryHdr	*summaryHdrPtr;
    LfsFileLayoutSummary  *fileSumPtr;
    char	*summaryPtr, *limitPtr;
    unsigned int address, blockOffset;
    Fscache_FileInfo	*cacheInfoPtr;
    ReturnStatus	status;
    Boolean	full;
    static FileSegLayout	cleaningLayoutData; /* Only works with one
							 file system. */

     full = FALSE;
     LfsGetDirtyFileToClean(&cacheInfoPtr);

     if (cacheInfoPtr != (Fscache_FileInfo *) NIL) {
	 /*
	  * Since we read all the active parts of the segment into the
	  * cache on the initial pass we just need to complete the 
	  * layout.
	  */
	 while (!full && (cacheInfoPtr != (Fscache_FileInfo *) NIL)) {
	       full = PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr,
				layoutPtr, &cleaningLayoutData);
	     if (full) {
		   cleaningLayoutData.numDescSlotsLeft = 0;
		   break;
	     }
	     LfsGetDirtyFileToClean(&cacheInfoPtr);
	 }
	 return full;
     }

     summaryPtr =  LfsSegGetSummaryPtr(segToCleanPtr);
     limitPtr = summaryPtr + LfsSegSummaryBytesLeft(segToCleanPtr);
     address = LfsSegDiskAddress(segToCleanPtr, 
			LfsSegGetBufferPtr(segToCleanPtr));
     blockOffset = 0;
     while (summaryPtr < limitPtr) { 
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    unsigned int diskAddr;
	    int		fileNumber;
	    int		slot;
	    LfsFileDescriptor	*descPtr;
	    char		*blockStartPtr;
	    Fscache_Block *blockPtr = (Fscache_Block *) NIL;
	    /*
	     * A block of descriptors.  For each descriptor that is live
	     * (being pointed to by the descriptor map) we bring it into
	     * the system and mark it as dirty. We must be sure there is
	     * enought room in the output segment. 
	     */
	    descPtr = (LfsFileDescriptor *)
		blockStartPtr = LfsSegFetchBytes(segToCleanPtr, blockOffset, 
				layoutPtr->params.descPerBlock * 
				 sizeof(LfsFileDescriptor));
	    for (slot = 0; slot < layoutPtr->params.descPerBlock; slot++) {
		if (!(descPtr[slot].common.flags & FSDM_FD_ALLOC)) {
		    break;
		}
		fileNumber = descPtr[slot].fileNumber;
		status = LfsDescMapGetDiskAddr(lfsPtr, fileNumber, &diskAddr);
		if (status != SUCCESS) {
		    continue;
		}
	        if (diskAddr == address + blockOffset) {
		    Fs_FileID fileID;
		    Fsio_FileIOHandle *newHandlePtr;
		    Boolean	found;
		    if (blockPtr == (Fscache_Block *) NIL) {
			/*
			 * We get to read it into the cache if its not already
			 * there. 
			 */
			Fscache_FetchBlock(&lfsPtr->descCacheHandle.cacheInfo,
					diskAddr, FSCACHE_DESC_BLOCK, 
					&blockPtr, &found);
			if (!found) {
			    bcopy(blockStartPtr, blockPtr->blockAddr,
				   lfsPtr->fileLayout.params.descPerBlock * 
					 sizeof(*descPtr));
			    Fscache_IODone(blockPtr);
			}
			Fscache_UnlockBlock(blockPtr, 0, -1, FS_BLOCK_SIZE, 0);
		    }

		    fileID.type = FSIO_LCL_FILE_STREAM;
		    fileID.serverID = rpc_SpriteID;
		    fileID.major = lfsPtr->domainPtr->domainNumber;
		    fileID.minor = fileNumber;
		    status = Fsio_LocalFileHandleInit(&fileID, (char *) NIL, 
					(Fsdm_FileDescriptor *) NIL, 
					&newHandlePtr);
		    if (status != SUCCESS) {
			LfsError(lfsPtr, status,
				"Can't get handle to clean file\n");
		    }
		    Fscache_MarkToClean(newHandlePtr, 0,
					 0, (char *) NIL);
		    Fsutil_HandleRelease(newHandlePtr, TRUE);
		}
	    }
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    blockOffset += LfsBytesToBlocks(lfsPtr, 
		(layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor)));
	    break;
	}
	case LFS_FILE_LAYOUT_DATA: {
	    LfsFileLayoutSummary *fileLayoutPtr;
	    unsigned int	*blockArray, diskAddress;
	    int			 startBlockOffset, i;
	    unsigned short	curTruncVersion;
	    /*
	     * We ran into a data block. If it is still alive bring it into
	     * the cache. 
	     */
	     fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	    startBlockOffset = blockOffset;
	     status = LfsDescMapGetVersion(lfsPtr, 
			fileSumPtr->fileNumber, &curTruncVersion);
	     if ((status == SUCCESS) && 
		 (curTruncVersion == fileSumPtr->truncVersion)) {
		Fs_FileID fileID;
		Fsio_FileIOHandle *newHandlePtr;

		fileID.type = FSIO_LCL_FILE_STREAM;
		fileID.serverID = rpc_SpriteID;
		fileID.major = lfsPtr->domainPtr->domainNumber;
		fileID.minor = fileSumPtr->fileNumber;
		status = Fsio_LocalFileHandleInit(&fileID, (char *) NIL, 
				    (Fsdm_FileDescriptor *) NIL, 
				    &newHandlePtr);
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status,
			    "Can't get handle to clean file\n");
		}
		 blockArray = (unsigned int *) (
				summaryPtr + sizeof(LfsFileLayoutSummary));
		for (i = 0; i < fileSumPtr->numDataBlocks; i++) {
		    status = LfsFile_GetIndex(newHandlePtr, blockArray[i],
					      &diskAddress);
		    if ((status == SUCCESS) &&
			(diskAddress == address + blockOffset)) { 
			int	blockSize, blocksLeft, bytesLeft;
			blockSize = FS_BLOCK_SIZE;
			blocksLeft = (fileSumPtr->numBlocks - 
					(blockOffset - startBlockOffset));
			bytesLeft = LfsBlocksToBytes(lfsPtr, blocksLeft);
			if (blockSize > bytesLeft) { 
			    blockSize = bytesLeft;
			}
			Fscache_MarkToClean(newHandlePtr, 
					blockArray[i], blockSize, 
				    LfsSegFetchBytes(segToCleanPtr, 
					  blockOffset, blockSize));
		    }
		    blockOffset += LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
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
    LfsSegSetSummaryPtr(segToCleanPtr, summaryPtr);
    LfsSegSetCurBlockOffset(segToCleanPtr,blockOffset);
    bzero(&cleaningLayoutData,sizeof(cleaningLayoutData));

    LfsGetDirtyFileToClean(&cacheInfoPtr);

    if (cacheInfoPtr != (Fscache_FileInfo *) NIL) {
	 /*
	  * Since we read all the active parts of the segment into the
	  * cache on the initial pass we just need to complete the 
	  * layout.
	  */
	 while (!full && (cacheInfoPtr != (Fscache_FileInfo *) NIL)) {
	       full = PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr,
				layoutPtr, &cleaningLayoutData);
	     if (full) {
		   cleaningLayoutData.numDescSlotsLeft = 0;
		   break;
	     }
	     LfsGetDirtyFileToClean(&cacheInfoPtr);
	 }
    }
    return full;

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
PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr, segLayoutDataPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsSeg	*segPtr;	/* Segment to place data. */
    Fscache_FileInfo *cacheInfoPtr;	/* File to place in segment. */
    LfsFileLayout     *layoutPtr;   /* File layout info. */
    FileSegLayout  *segLayoutDataPtr; /* Current layout data for segment. */
{
    LfsFileDescriptor	*descBlockPtr;
    LfsFileLayoutDesc	*descSumPtr;
    LfsFileLayoutSummary *fileSumPtr;
    Boolean	full = FALSE;
    int	lfsBlocksPerBlock = LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
    int		blockType, blocksNeeded, descBlocks;
    char	*summaryPtr;
    ReturnStatus	status;
    Fscache_Block	*blockPtr;
    Boolean		fileNotUsed = TRUE;

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
					    NIL,  NIL);
		descSumPtr = (LfsFileLayoutDesc *) summaryPtr;
    
		descSumPtr->blockType = LFS_FILE_LAYOUT_DESC;
		descSumPtr->numBlocks = descBlocks;

		summaryPtr += sizeof(LfsFileLayoutDesc);
		LfsSegSetSummaryPtr(segPtr,	summaryPtr);

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
	   status = LfsDescMapGetVersion(lfsPtr, fileSumPtr->fileNumber,
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
		     LfsGetDirtyBlock(cacheInfoPtr, blockType, &blockPtr);
		     if (blockPtr == (Fscache_Block *) NIL) {
			break;
		     }
		     /*
		      * Compute the number of file system blocks this block
		      * will take.
		      */
		     blocks = LfsBytesToBlocks(lfsPtr, 
			(blockPtr->blockSize + 
					lfsPtr->superBlock.hdr.blockSize-1));
		     /*
		      * Be sure there is room for the block and summary info.
		      */
		     summaryPtr = LfsSegGrowSummary(segPtr, blocks,sizeof(int));
		     if (summaryPtr == (char *) NIL) {
			 LfsReturnDirtyBlock(blockPtr);
			 full = TRUE;
			 break;
		     }
		     fileNotUsed = FALSE;
		     *(int *) summaryPtr = blockPtr->blockNum;
		     summaryPtr += sizeof(int);
		     bufferPtr = LfsSegAddDataBuffer(segPtr, blocks, 
				    blockPtr->blockAddr, (ClientData) blockPtr);
		     status = LfsFile_SetIndex(cacheInfoPtr->hdrPtr, 
					       blockPtr->blockNum,
				         LfsSegDiskAddress(segPtr, bufferPtr));
		     if (status != SUCCESS) {
			 LfsError(lfsPtr, status, "Can't update file index");
		     }
		     LfsSegSetSummaryPtr(segPtr,summaryPtr);
		     fileSumPtr->numDataBlocks++; 
		     fileSumPtr->numBlocks += blocks;
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
	      descPtr->flags &= ~FSDM_FD_DIRTY;	
	      bcopy((char *) descPtr, &(segLayoutDataPtr->descBlockPtr->common),
		    sizeof(*descPtr));
	      segLayoutDataPtr->descBlockPtr->fileNumber = 
			      cacheInfoPtr->hdrPtr->fileID.minor;
	      status = LfsDescMapSetDiskAddr(lfsPtr, 
			    segLayoutDataPtr->descBlockPtr->fileNumber, 
			    segLayoutDataPtr->descDiskAddr);
	      if (status != SUCCESS) {
		  LfsError(lfsPtr, status, "Can't update descriptor map.\n");
	      }
	      LfsProcessCleanBlock(cacheInfoPtr,(Fscache_Block *) NIL,SUCCESS);
	      segLayoutDataPtr->descBlockPtr++;
	      segLayoutDataPtr->numDescSlotsLeft--;
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
	 LfsReturnDirtyFile(cacheInfoPtr);
     }
     return full;
}

