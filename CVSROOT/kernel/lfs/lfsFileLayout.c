/* 
 * lfsFileLayout.c --
 *
 *	Control the writing, cleaning, and checkpointing of files and 
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

#include <lfsInt.h>
#include <lfsSeg.h>
#include <lfsFileLayout.h>
#include <lfsDesc.h>
#include <lfsDescInt.h>

#include <fsdm.h>
#include <fscache.h>
#include <rpc.h>

#ifdef TRACING
#include <trace.h>
Trace_Header lfsTraceHdr;
Trace_Header *lfsTraceHdrPtr = &lfsTraceHdr;
int lfsTraceLength = 1000;
typedef struct LfsTraceRecord {
    int	fileNumber;
    LfsDiskAddr diskAddr;
    ReturnStatus status;
    int  curTruncVersion;
} LfsTraceRecord;
#endif

#define	LOCKPTR	&lfsPtr->lock

Boolean	lfsFileLayoutDebug = FALSE;

typedef struct FileSegLayout {
    int	 numDescSlotsLeft;	/* Number of slots left in descriptor block. */
    LfsFileDescriptor *descBlockPtr;		
				/* Pointer to next slot in descriptor block. */
    LfsDiskAddr descDiskAddr; 	/* Disk address of descriptor block. */
    int maxElements;	  	/* The maximum number of elements. */
    List_Links	fileList; /* List of files in this segment. */
    List_Links  blockList; /* List of cache blocks laidout in
				       * this segment.  */
    Fscache_FileInfo	*activeFilePtr;	/* File current being written. */
    List_Links	dirLogListHdr;	/* List of directory block blocks being 
				 * written. */
} FileSegLayout;

static Boolean PlaceFileInSegment _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr, 
	Fscache_FileInfo *cacheInfoPtr, LfsFileLayout *layoutPtr, 
	int token, FileSegLayout *segLayoutDataPtr));

static Boolean BlockMatch _ARGS_((Fscache_Block *blockPtr, 
				ClientData clientData));

static void DirLogInit _ARGS_((Lfs *lfsPtr));
static void DirLogDestory _ARGS_((Lfs *lfsPtr));
static void NewDirLogBlock _ARGS_((Lfs *lfsPtr));
static LfsDirOpLogEntry *FindLogEntry _ARGS_((Lfs *lfsPtr, int logSeqNum));
static Boolean AddDirLogBlocks _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr, 
			FileSegLayout *segLayoutDataPtr));
static void FreeDirLogBlocks _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr,
			FileSegLayout *segLayoutDataPtr));

extern ReturnStatus LfsFileLayoutAttach _ARGS_((Lfs *lfsPtr, 
			int checkPointSize, char *checkPointPtr));
extern Boolean LfsFileLayoutProc _ARGS_((LfsSeg *segPtr, int flags, 
			ClientData *clientDataPtr));
extern Boolean LfsFileLayoutCheckpoint _ARGS_((LfsSeg *segPtr, int flags, 
			char *checkPointPtr, int *checkPointSizePtr, 
			ClientData *clientDataPtr));
extern void LfsFileLayoutWriteDone _ARGS_((LfsSeg *segPtr, int flags, 
			ClientData *clientDataPtr));
extern Boolean LfsFileLayoutClean _ARGS_((LfsSeg *segPtr, int *sizePtr, 
			int *numCacheBlocksPtr, ClientData *clientDataPtr));

extern ReturnStatus LfsFileLayoutDetach _ARGS_((Lfs *lfsPtr));

static LfsSegIoInterface layoutIoInterface = 
	{ LfsFileLayoutAttach, LfsFileLayoutProc, LfsFileLayoutClean,
	  LfsFileLayoutCheckpoint, LfsFileLayoutWriteDone, 
	  LfsFileLayoutDetach, 0};

#define	WRITEBACK_TOKEN		0
#define	CLEANING_TOKEN		1
#define	CHECKPOINT_TOKEN	2

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
#ifdef TRACING
    Trace_Init(lfsTraceHdrPtr, lfsTraceLength, sizeof(LfsTraceRecord),
		TRACE_NO_TIMES);
#endif
}

#ifdef TRACING
ReturnStatus
Lfs_PrintRec(clientData, event, printHeaderFlag)
    ClientData clientData;	/* Client data in the trace record */
    int event;			/* Type, or event, from the trace record */
    Boolean printHeaderFlag;	/* If TRUE, a header line is printed */
{
    LfsTraceRecord *recPtr = (LfsTraceRecord *) clientData;
    if (recPtr != (LfsTraceRecord *) NIL) { 
	printf("<%d,%d,%d,%d>\n", recPtr->fileNumber, recPtr->blockNumber,
			LfsDiskAddrToOffset(recPtr->diskAddr), recPtr->found);
    }
    return SUCCESS;
}
void
Lfs_PrintTrace(numRecs)
    int numRecs;
{
    if (numRecs < 0) {
	numRecs = lfsTraceLength;
    }
    printf("LFS TRACE\n");
    (void)Trace_Print(lfsTraceHdrPtr, numRecs, Lfs_PrintRec);
}
#endif

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
     * Initialize the filelayout structures for this file system.
     */
    layoutPtr->params = *paramsPtr;

    LfsDescCacheInit(lfsPtr);
    Sync_LockInitDynamic(&(lfsPtr->logLock), "LfsLogLock");
    DirLogInit(lfsPtr);

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsFileLayoutDetach --
 *
 *	Detach routine for file layout module. 
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
LfsFileLayoutDetach(lfsPtr)
    Lfs   *lfsPtr;	     /* File system for attach. */
{

    LfsDescCacheDestory(lfsPtr);
    DirLogDestory(lfsPtr);

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
LfsFileLayoutProc(segPtr, flags, clientDataPtr)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    int	flags;		/* Cleaning flags */
    ClientData	*clientDataPtr;
{
    Lfs		    *lfsPtr =   segPtr->lfsPtr;
    LfsFileLayout    *layoutPtr = &(lfsPtr->fileLayout);
    Fscache_FileInfo	*cacheInfoPtr;
    Boolean	    full, fsyncOnly;
    FileSegLayout  *segLayoutDataPtr;
    int		  token;

     /*
      * Next spill the file with dirty blocks into the segment. 
      */
     LFS_STATS_INC(lfsPtr->stats.layout.calls);
     full = FALSE;

     fsyncOnly = ((flags & (LFS_CLEANING_LAYOUT|LFS_CHECKPOINT_LAYOUT)) == 0);
     if (flags & LFS_CLEANING_LAYOUT) {
	 token = CLEANING_TOKEN;
     } else if (flags & LFS_CHECKPOINT_LAYOUT) {
	 token = CHECKPOINT_TOKEN;
     } else {
	 token = WRITEBACK_TOKEN;
     }
     if (*clientDataPtr == (ClientData) NIL) {

	/*
	 * Allocate a FileSegLayout data structure for this segment. 
	 */
	 segLayoutDataPtr = (FileSegLayout *) malloc(sizeof(FileSegLayout));
	 *clientDataPtr = (ClientData) segLayoutDataPtr;
	 segLayoutDataPtr->numDescSlotsLeft = 0;
	 segLayoutDataPtr->maxElements = LfsSegSizeInBlocks(lfsPtr);
	 List_Init(&segLayoutDataPtr->fileList);
	 List_Init(&segLayoutDataPtr->blockList);
	 segLayoutDataPtr->activeFilePtr = (Fscache_FileInfo *) NIL;
	 List_Init(&segLayoutDataPtr->dirLogListHdr);
     } else {
	 segLayoutDataPtr = (FileSegLayout *) *clientDataPtr;
     }
     cacheInfoPtr = (Fscache_FileInfo *) NIL;
     /*
      * Choose the first file. If the last call to layout data into this
      * segment ended with a partially layed out file, start with that file.
      */
     if (segLayoutDataPtr->activeFilePtr == (Fscache_FileInfo *) NIL) { 
	  full = AddDirLogBlocks(lfsPtr, segPtr, segLayoutDataPtr);
	  if (!full) { 
	      cacheInfoPtr = Fscache_GetDirtyFile(lfsPtr->domainPtr->backendPtr, 
			fsyncOnly, LfsFileMatch, (ClientData) token);
	  }
     } else {
	 cacheInfoPtr = segLayoutDataPtr->activeFilePtr;
     }
     while (!full && (cacheInfoPtr != (Fscache_FileInfo *) NIL)) {
	   LFS_STATS_INC(lfsPtr->stats.layout.dirtyFiles);
	   full = PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr,
			token,  segLayoutDataPtr);
	   if (full) {
	       LFS_STATS_INC(lfsPtr->stats.layout.filledRegion);
	       segLayoutDataPtr->activeFilePtr = cacheInfoPtr;
	       break;
	   }
	   List_Insert((List_Links *) cacheInfoPtr, 
				LIST_ATREAR(&segLayoutDataPtr->fileList));
	   full = AddDirLogBlocks(lfsPtr, segPtr, segLayoutDataPtr);
	   if (!full) { 
	       cacheInfoPtr = Fscache_GetDirtyFile(
			lfsPtr->domainPtr->backendPtr, 
			FALSE, LfsFileMatch, (ClientData) token);
	   }
    }
    if (!full && List_IsEmpty(&segLayoutDataPtr->fileList) &&
	   List_IsEmpty(&segLayoutDataPtr->dirLogListHdr)) { 
	free((char *) *clientDataPtr);
	*clientDataPtr = (ClientData) NIL;
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
    Boolean full;

    /*
     * Write-back everything that is dirty.
     */
    full = LfsFileLayoutProc(segPtr, LFS_CHECKPOINT_LAYOUT, clientDataPtr);
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
    Fscache_Block *blockPtr;
    Fscache_FileInfo *cacheInfoPtr;
    FileSegLayout  *segLayoutDataPtr;

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
	     LFS_STATS_INC(lfsPtr->stats.layout.descBlockWritten);
	     LfsDescCacheBlockRelease(segPtr->lfsPtr, bufferPtr->clientData, 
				FALSE);
	     bufferPtr++;
	     summaryPtr += sizeof(LfsFileLayoutDesc);
	     break;
	}
	case LFS_FILE_LAYOUT_DATA:  {
	    LfsFileLayoutSummary *fileSumPtr;
	    /* 
	     * All these records should be pointing to a cache blocks which
	     * must be released.
	     */
	    fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	    bufferPtr += fileSumPtr->numDataBlocks;
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
	    summaryPtr = summaryPtr + sizeof(LfsFileLayoutLog);
	    bufferPtr += logSumPtr->numDataBlocks;
	    break;
	}
	case LFS_FILE_LAYOUT_DBL_INDIRECT: 
	case LFS_FILE_LAYOUT_INDIRECT: 
	default:
	    panic("Bad file block type in summary block");
	}
    }
    LfsSegSetBufferPtr(segPtr, bufferPtr);
    if (*clientDataPtr != (ClientData) NIL) {
	LFS_STATS_INC(lfsPtr->stats.layout.segWrites);
	segLayoutDataPtr = (FileSegLayout *) *clientDataPtr;
	FreeDirLogBlocks(lfsPtr, segPtr, segLayoutDataPtr);
	while (!List_IsEmpty(&segLayoutDataPtr->blockList)) {
	    blockPtr = (Fscache_Block *)
				List_First(&segLayoutDataPtr->blockList);
	    List_Remove((List_Links *)blockPtr);
	    LFS_STATS_INC(lfsPtr->stats.layout.cacheBlocksWritten);
	    Fscache_ReturnDirtyBlock(blockPtr, SUCCESS);
	}
	while (!List_IsEmpty(&segLayoutDataPtr->fileList)) {
	    cacheInfoPtr = 
		(Fscache_FileInfo *) List_First(&segLayoutDataPtr->fileList);
	    List_Remove((List_Links *)cacheInfoPtr);
	    LFS_STATS_INC(lfsPtr->stats.layout.filesWritten);
	    Fscache_ReturnDirtyFile(cacheInfoPtr, TRUE);
	}
	if (segLayoutDataPtr->activeFilePtr != (Fscache_FileInfo *) NIL) {
	    Fscache_ReturnDirtyFile(segLayoutDataPtr->activeFilePtr, TRUE);
	}
	free((char *) *clientDataPtr);
	*clientDataPtr = (ClientData) NIL;
    }
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
    int  blockOffset, fsBlocks;
    LfsDiskAddr address;
    ReturnStatus	status;
    Boolean	error;

     error = FALSE;
     fsBlocks = LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
     summaryPtr =  LfsSegGetSummaryPtr(segPtr);
     limitPtr = summaryPtr + LfsSegSummaryBytesLeft(segPtr);
     address = LfsSegDiskAddress(segPtr, LfsSegGetBufferPtr(segPtr));
     blockOffset = 0;
     LFS_STATS_INC(lfsPtr->stats.layout.cleanings);
     while ((summaryPtr < limitPtr) && !error) { 
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    LfsDiskAddr diskAddr;
	    int		fileNumber;
	    int		slot, size;
	    LfsFileDescriptor	*descPtr;
	    char		*blockStartPtr;
	    ClientData descCachePtr = (ClientData) NIL;
	    /*
	     * A block of descriptors.  For each descriptor that is live
	     * (being pointed to by the descriptor map) we bring it into
	     * the system and mark it as dirty. 
	     */
	    LFS_STATS_INC(lfsPtr->stats.layout.descBlocksCleaned);
	    size = layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor);
	    blockOffset += LfsBytesToBlocks(lfsPtr, size);
	    blockStartPtr = LfsSegFetchBytes(segPtr, 
				segPtr->curBlockOffset + blockOffset, size);
	    descPtr = (LfsFileDescriptor *)blockStartPtr;
	    for (slot = 0; slot < layoutPtr->params.descPerBlock; slot++) {
		Fs_FileID fileID;
		Fsio_FileIOHandle *newHandlePtr;
		LfsDiskAddr newDiskAddr;
		/*
		 * The descriptor block is terminated by an inode
		 * with a zero magic number.
		 */
		if (descPtr[slot].common.magic == 0) {
		    break;
		}
		if (descPtr[slot].common.magic != FSDM_FD_MAGIC) {
		    LfsError(lfsPtr, FAILURE, "Bad descriptor magic number.\n");
		    continue;
		}
		if (!(descPtr[slot].common.flags & FSDM_FD_ALLOC)) {
		    /*
		     * Skip over any FREE inodes.
		     */
		    continue;
		}
		fileNumber = descPtr[slot].fileNumber;
		status = LfsDescMapGetDiskAddr(lfsPtr, fileNumber, &diskAddr);
		/*
		 * If the file is not allocated or this descriptor is not
		 * the most current copy, skip it.
		 */
		LfsDiskAddrPlusOffset(address,-blockOffset, &newDiskAddr);
		if ((status == FS_FILE_NOT_FOUND) || 
		    ((status == SUCCESS) && 
		      !LfsSameDiskAddr(diskAddr, newDiskAddr))) {
		    continue;
		}
		if (status != SUCCESS) {
		    LfsError(lfsPtr, status, 
			"Bad file number in descriptor block.\n");
		    continue;
		}
	        LFS_STATS_INC(lfsPtr->stats.layout.descCopied);
		if (descCachePtr == (ClientData) NIL) {
		    char *blockAddrPtr;
		    /*
		     * We get to read it into the cache if its not already
		     * there. 
		     */
		    blockAddrPtr = blockStartPtr;
		    descCachePtr = LfsDescCacheBlockInit(lfsPtr, diskAddr, 
							TRUE, 
							&blockAddrPtr);
		    if (descCachePtr == (ClientData) NIL) {
			printf("Can't fetch descriptor block\n");
			error = TRUE;
			break;
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
	    if (descCachePtr != (ClientData) NIL) {
		LfsDescCacheBlockRelease(lfsPtr, descCachePtr, TRUE);
	    }
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    break;
	}
	case LFS_FILE_LAYOUT_DATA: {
	    LfsDiskAddr diskAddress;
	    int		*blockArray;
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
	      LFS_STATS_INC(lfsPtr->stats.layout.fileCleaned);
	     status = LfsDescMapGetVersion(lfsPtr, 
			(int)fileSumPtr->fileNumber, &curTruncVersion);
	     if ((status == SUCCESS) && 
		 (curTruncVersion == fileSumPtr->truncVersion)) {
		Fs_FileID fileID;
		int	  numBlocks;
		Fsio_FileIOHandle *newHandlePtr;

	        LFS_STATS_INC(lfsPtr->stats.layout.fileVersionOk);
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
		    printf("Can't fetch handle for file %d for cleaning\n",
				fileSumPtr->fileNumber);

		    LFS_STATS_INC(lfsPtr->stats.layout.cleanLockedHandle);
		    error = TRUE;
		    break;
		}
		if (status != SUCCESS) {
		    if (status == FS_FILE_NOT_FOUND) {
			/*
			 * Someone just deleted the file out from under us.
			 */
			LFS_STATS_INC(lfsPtr->stats.layout.cleanNoHandle);
			goto noHandle;
		    }
		    LfsError(lfsPtr, status,
			    "Can't get handle to clean file\n");
		}
		/*
		 * For each block ... 
		 */
		blockArray = (int *)
			(summaryPtr + sizeof(LfsFileLayoutSummary));
		/*
		 * Careful with the first block because it could be a 
		 * fragment.
		 */
		numBlocks = fileSumPtr->numBlocks - 
				(fileSumPtr->numDataBlocks-1) * fsBlocks;
	        LFS_STATS_ADD(lfsPtr->stats.layout.blocksCleaned,
					fileSumPtr->numBlocks);
		for (i = 0; i < fileSumPtr->numDataBlocks; i++) {
		    LfsDiskAddr newDiskAddr;
		    blockOffset += numBlocks;
		    status = LfsFile_GetIndex(newHandlePtr, blockArray[i],
				FSCACHE_CANT_BLOCK|FSCACHE_DONT_BLOCK,
				&diskAddress);
		    LfsDiskAddrPlusOffset(address, -blockOffset, &newDiskAddr);
		    if ((status == SUCCESS) && 
			  LfsSameDiskAddr(diskAddress, newDiskAddr)) { 
			int	blockSize, flags;
			Boolean	 found;
			Fscache_Block *blockPtr;
			/*
			 * The block exists and is at the location we are
			 * cleaning, bring it into the cache. Be a little
			 * careful about short blocks.
			 */
		        LFS_STATS_ADD(lfsPtr->stats.layout.blocksCopied,numBlocks);
		        blockSize = LfsBlocksToBytes(lfsPtr, numBlocks);
			flags = (FSCACHE_IO_IN_PROGRESS | FSCACHE_CANT_BLOCK | 
						FSCACHE_DONT_BLOCK |
			          ((blockArray[i] >= 0) ? FSCACHE_DATA_BLOCK :
							  FSCACHE_IND_BLOCK));

		        Fscache_FetchBlock(&newHandlePtr->cacheInfo,
				      blockArray[i], flags, &blockPtr, &found);
			if (blockPtr == (Fscache_Block *) NIL) {
			    printf("Can't fetch cache block <%d,%d> for cleaning.\n", fileSumPtr->fileNumber, blockArray[i]);
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
			    Fscache_UnlockBlock(blockPtr, Fsutil_TimeInSeconds(),
						-1, blockSize, 
						FSCACHE_BLOCK_BEING_CLEANED);

			} else { 
			    /*
			     * Checking it out.
			     */
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
			    Fscache_UnlockBlock(blockPtr, Fsutil_TimeInSeconds(),
						-1, blockPtr->blockSize, 
						FSCACHE_BLOCK_BEING_CLEANED);
			   LFS_STATS_ADD(lfsPtr->stats.layout.blocksCopiedHit,
					numBlocks);
			}
			(*sizePtr) += blockSize;
			(*numCacheBlocksPtr)++;
		    } else if (status != SUCCESS) {
			printf("Can't fetch index for cleaning <%d,%d>\n",
					fileSumPtr->fileNumber, blockArray[i]);
			error = TRUE;
			break;
		    }
		    /*
		     * All the blocks after the first one must be FS_BLOCK_SIZE.
		     */
		    numBlocks = fsBlocks;
		}
		Fsutil_HandleRelease(newHandlePtr, TRUE);
	    } else {
#ifdef TRACING
			{
			    LfsTraceRecord rec;
			    rec.fileNumber = fileSumPtr->fileNumber;
			    rec.diskAddr = address;
			    rec.status = status;
			    rec.curTruncVersion = curTruncVersion;
			    Trace_Insert(lfsTraceHdrPtr, 0, (ClientData)&rec);
			}
#endif /* TRACING */
	    }
	 noHandle:
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
	     summaryPtr += sizeof(LfsFileLayoutLog);
	     blockOffset += logSumPtr->numBlocks;
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
PlaceFileInSegment(lfsPtr, segPtr, cacheInfoPtr, layoutPtr, token,
		segLayoutDataPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsSeg	*segPtr;	/* Segment to place data. */
    Fscache_FileInfo *cacheInfoPtr;	/* File to place in segment. */
    LfsFileLayout     *layoutPtr;   /* File layout info. */
    int		     token;	    /* Block selector token. */
    FileSegLayout  *segLayoutDataPtr; /* Current layout data for segment. */
{
    LfsFileLayoutSummary *fileSumPtr;
    Boolean	full;
    LfsSegElement	*bufferPtr;
    char	*summaryPtr;
    int		lfsBlocksPerBlock, lastDirtyBlock;
    int		blockType;
    int		blocksNeeded, bytesNeeded, blocksLeft;
    ReturnStatus	status;
    Fscache_Block	*blockPtr, *firstBlockPtr;
    Fsdm_FileDescriptor *descPtr;

    if (cacheInfoPtr == (Fscache_FileInfo *) NIL) {
	return FALSE;
    }
    descPtr = ((Fsio_FileIOHandle *)(cacheInfoPtr->hdrPtr))->descPtr;
    /*
     * Layout the blocks of the file into the segment starting with the
     * data blocks.
     */
    full = FALSE;
    fileSumPtr = (LfsFileLayoutSummary *) NIL;
    lfsBlocksPerBlock = LfsBytesToBlocks(lfsPtr, FS_BLOCK_SIZE);
    for (blockType = LFS_FILE_LAYOUT_DATA; 
	(blockType <= LFS_FILE_LAYOUT_DBL_INDIRECT) && !full;  blockType++) { 
	/*
	 * Do all of one blockType first before going to the next.
	 * Try to checkout just enought blocks that will fit in this
	 * segment. We prefetch the first block so that we know if
	 * we have zero blocks to layout and don't have to add a
	 * LfsFileLayoutSummary.
	 */
	firstBlockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch,
			    (ClientData) ((token << 16) | blockType),
			     &lastDirtyBlock);
	/*
	 * No more blocks of this type available for this file, go on to 
	 * the next blockType. 
	 */ 
	if (firstBlockPtr == (Fscache_Block *) NIL) {
	    continue;
	}
	if ((firstBlockPtr->blockSize < FS_BLOCK_SIZE) && 
	    (firstBlockPtr->blockNum != descPtr->lastByte/FS_BLOCK_SIZE)) {
	    /*
	     * Only the last block in the file is allowed to be less than 
	     * FS_BLOCK_SIZE. Sometimes the Fscache_Block->blockSize
	     * is incorrect so we patch it for them.
	     */
	    firstBlockPtr->blockSize = FS_BLOCK_SIZE;
	}

       /*
        * Allocate the layout summary bytes of this file if 
        * we haven't done so already.
        */
       if (fileSumPtr == (LfsFileLayoutSummary *) NIL) {
	   /*
	    * Since we haven't done so already, allocate a LfsFileLayoutSummary
	    * for this file in summary block.  Besure there is at least 
	    * enough space for one block. If the block that we justed got 
	    * (ie firstBlockPtr) is the last block in the cache we ensure
	    * that there is enough room for it.  Otherwise we require at 
	    * least a entire blocks worth.
	    */
	   blocksNeeded =  (lastDirtyBlock != 0) ? lfsBlocksPerBlock : 
				LfsBytesToBlocks(lfsPtr, 
						firstBlockPtr->blockSize);
	   bytesNeeded = sizeof(LfsFileLayoutSummary) + sizeof(int);
	   summaryPtr = LfsSegGrowSummary(segPtr, blocksNeeded, bytesNeeded);
	   if (summaryPtr == (char *) NIL) { 
	       /*
	        * No room in summary. Return block and exit loop.
		*/
		if (token == CLEANING_TOKEN) {
		    /* 
		     * Reset the flag marking this block as being cleaned
		     * before returning it to the cache.
		     */
		    firstBlockPtr->flags |= FSCACHE_BLOCK_BEING_CLEANED;
		}
	       Fscache_ReturnDirtyBlock(firstBlockPtr, GEN_EINTR);
	       full = TRUE;
	       break;
	   }
	   /*
	    * Fill in the LfsFileLayoutSummary with the value we 
	    * know now.
	    */
	   fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	   fileSumPtr->blockType = LFS_FILE_LAYOUT_DATA;
	   fileSumPtr->numDataBlocks = 0;
	   fileSumPtr->numBlocks = 0; /* Filled in later. */
	   fileSumPtr->fileNumber = cacheInfoPtr->hdrPtr->fileID.minor;
	   status = LfsDescMapGetVersion(lfsPtr, 
		       fileSumPtr->fileNumber, &fileSumPtr->truncVersion);
	   if (status != SUCCESS) {
	       LfsError(lfsPtr, status, "Can't get truncate version number\n");
	   }
	   summaryPtr += sizeof(LfsFileLayoutSummary);
	   LfsSegSetSummaryPtr(segPtr, summaryPtr);
       }
       /*
        * Place the blocks in the segment in reverse order so that
        * they will occur on disk in forward order. This is done
	* by looping until we have collected enough blocks to fill the
	* segments or we run out of cache blocks.  Note that we are
	* permitted to overrun the segment because the code below will
	* return the blocks to the cache.
	*
	* The first block we push on the list is the block we prefetched
	* above.
	*/
       blocksLeft = LfsSegBlocksLeft(segPtr);
       blockPtr = firstBlockPtr;
       do {  
	   LFS_STATS_INC(lfsPtr->stats.layout.dirtyBlocks);
	    if ((blockPtr->blockSize < FS_BLOCK_SIZE) && 
		(blockPtr->blockNum != descPtr->lastByte/FS_BLOCK_SIZE)) {
		/*
		 * Only the last block in the file is allowed to be less than 
		 * FS_BLOCK_SIZE. Sometimes the Fscache_Block->blockSize
		 * is incorrect so we patch it for them.
		 */
		blockPtr->blockSize = FS_BLOCK_SIZE;
	    }
	   List_Insert((List_Links *) blockPtr, 
			LIST_ATFRONT(&segLayoutDataPtr->blockList));
	   blocksLeft -= LfsBytesToBlocks(lfsPtr, blockPtr->blockSize);
	   if (blocksLeft > 0) {
	       blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, 
			BlockMatch, (ClientData) ((token << 16) | blockType), 
			&lastDirtyBlock);
	    }
	} while ((blocksLeft > 0) && (blockPtr != (Fscache_Block *) NIL));

	/*
	 * Interate forward thru the blocks we pushed on the list to 
	 * lay them out in the reverse order.  We allow the first
	 * block to layout to be a fragment, so we start off by
	 * computing the number of fs blocks needed by the first
	 * cache block.
	 */
	blockPtr = (Fscache_Block *) List_First(&segLayoutDataPtr->blockList);
	blocksNeeded = LfsBytesToBlocks(lfsPtr, blockPtr->blockSize);

	LIST_FORALL(&segLayoutDataPtr->blockList, (List_Links *) blockPtr) {
	    int bytesUsed;
	    int modTime;
	   /*
	    * Make sure there is enough room for both the data blocks in 
	    * the data region and the block number in the summary region.
	    */
	   summaryPtr = LfsSegGrowSummary(segPtr, blocksNeeded, sizeof(int));
	   if (summaryPtr == (char *) NIL) {
	       full = TRUE;
	       break;
	   }
	   /*
	    * Yes there is; add the cache block and fill in the summary region.
	    * Update the LfsFileLayoutSummary to reflect the data block being
	    * added and the number of fs blocks used.
	    */
	   *(int *) summaryPtr = blockPtr->blockNum;
	   summaryPtr += sizeof(int);
	   LfsSegSetSummaryPtr(segPtr,summaryPtr);
	   bufferPtr = LfsSegAddDataBuffer(segPtr, blocksNeeded, 
			    blockPtr->blockAddr, (ClientData) blockPtr);

	   fileSumPtr->numDataBlocks++; 
	   fileSumPtr->numBlocks += blocksNeeded;

	   /*
	    * Update the index for this file and increment the 
	    * active bytes of the segment by the size of the cache
	    * block rounded to file system blocks.
	    */
	   bytesUsed = LfsBlocksToBytes(lfsPtr,
		    LfsBytesToBlocks(lfsPtr, blockPtr->blockSize));
	   status = LfsFile_SetIndex(
				(Fsio_FileIOHandle *)(cacheInfoPtr->hdrPtr), 
				blockPtr->blockNum, bytesUsed, 
				FSCACHE_CANT_BLOCK,
				LfsSegDiskAddress(segPtr, bufferPtr));
	   if (status != SUCCESS) {
	     blockPtr->flags |= FSCACHE_BLOCK_DIRTY;
	     LfsError(lfsPtr, status, "Can't update file index");
	   }
	   segPtr->activeBytes += bytesUsed;
	   /*
	    * Update the modtime time of the segment to reflect this block
	    * if it is under than the rest.
	    */
	   modTime = descPtr->dataModifyTime;
	   if (segPtr->timeOfLastWrite < modTime) {
	       segPtr->timeOfLastWrite = modTime;
	   }
	   /*
	    * Any blocks after the first one must be of FS_BLOCK_SIZE 
	    * size.
	    */
	   blocksNeeded = lfsBlocksPerBlock;
	   /*
	    * Stop going down the list when we get to the first block we
	    * pushed on.
	    */
	   if (blockPtr == firstBlockPtr) {
		break;
	   }
	} 
	if (full) { 
	    while(1) {
		Fscache_Block *nextBlockPtr;
		/*
		 * We're not able to place all the blocks, return to the cache
		 * all blocks we couldn't place.
		 */
		LFS_STATS_INC(lfsPtr->stats.layout.dirtyBlocksReturned);
		nextBlockPtr = (Fscache_Block *) 
				List_Next((List_Links *)blockPtr);
		List_Remove((List_Links *) blockPtr);
		if (token == CLEANING_TOKEN) {
		    /* 
		     * Reset the flag marking this block as being cleaned
		     * before returning it to the cache.
		     */
		    blockPtr->flags |= FSCACHE_BLOCK_BEING_CLEANED;
		}
		Fscache_ReturnDirtyBlock(blockPtr, GEN_EINTR);
		if (blockPtr == firstBlockPtr) {
			break;
		}
		blockPtr = nextBlockPtr;
	    } 
	}
    }
    if (full) { 
	 return full;
    }
   /*
    * If the segment we are adding has no slots open in the descriptor
    * block try to allocate a new descriptor block.
    */
    if (segLayoutDataPtr->numDescSlotsLeft == 0) {
	LfsFileLayoutDesc	*descSumPtr;
	int		descBlocks, descBytes;

	/*
	 * Compute the size and add the descriptor block.
	 */
	descBytes = layoutPtr->params.descPerBlock * sizeof(LfsFileDescriptor);
	descBlocks = LfsBytesToBlocks(lfsPtr, descBytes);

	summaryPtr = LfsSegGrowSummary(segPtr, descBlocks, 
					    sizeof(LfsFileLayoutDesc));
	if (summaryPtr != (char *) NIL) {
	    LfsSegElement *descBufferPtr;
	    char	  *descMemPtr;
	    ClientData	clientData;
	    /*
	     * Allocate space for the descriptor block and fill in a
	     * summary block describing it. 
	     */
	    descBufferPtr = LfsSegAddDataBuffer(segPtr, descBlocks,
						(char *) NIL, (ClientData) NIL);
	    segLayoutDataPtr->numDescSlotsLeft = layoutPtr->params.descPerBlock;
	    segLayoutDataPtr->descDiskAddr = 
				LfsSegDiskAddress(segPtr, descBufferPtr);
	    descMemPtr = (char *) NIL;
	    clientData = LfsDescCacheBlockInit(lfsPtr, 
				segLayoutDataPtr->descDiskAddr, TRUE, 
				&descMemPtr);
	    segLayoutDataPtr->descBlockPtr = (LfsFileDescriptor *) descMemPtr;
	    descBufferPtr->address = descMemPtr;
	    descBufferPtr->clientData = clientData;

	    descSumPtr = (LfsFileLayoutDesc *) summaryPtr;

	    descSumPtr->blockType =  LFS_FILE_LAYOUT_DESC;
	    descSumPtr->numBlocks = descBlocks;

	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    LfsSegSetSummaryPtr(segPtr, summaryPtr);

	}
    }
    /*
     * If we successfully place this file in the segment add the descriptor
     * to the descriptor block. If this is no room then mark the segment 
     * as full.
     */
    if (segLayoutDataPtr->numDescSlotsLeft > 0) {
	  /*
	   * XXX - need to do this under lock. 
	   */
	  LFS_STATS_INC(lfsPtr->stats.layout.descWritten);
	  cacheInfoPtr->flags &= ~FSCACHE_FILE_DESC_DIRTY;
	  descPtr->flags &= ~FSDM_FD_DIRTY;	
	  bcopy((char *) descPtr, 
		(char *)&(segLayoutDataPtr->descBlockPtr->common),
		(int)sizeof(*descPtr));
	  segLayoutDataPtr->descBlockPtr->fileNumber = 
			  cacheInfoPtr->hdrPtr->fileID.minor;
	  status = LfsDescMapSetDiskAddr(lfsPtr, 
			(int)segLayoutDataPtr->descBlockPtr->fileNumber, 
			segLayoutDataPtr->descDiskAddr);
	  if (status != SUCCESS) {
	      LfsError(lfsPtr, status, "Can't update descriptor map.\n");
	  }
	  segLayoutDataPtr->descBlockPtr++;
	  segLayoutDataPtr->numDescSlotsLeft--;
	  segPtr->activeBytes += sizeof(LfsFileDescriptor);
     } else {
	 full = TRUE;
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
    int blockLevel = ((int) clientData) & 0xffff;
    int token = ((int) clientData) >> 16;

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

    if (token == CLEANING_TOKEN) {
	return ((blockPtr->flags & FSCACHE_BLOCK_BEING_CLEANED) != 0);
    } else {
	return TRUE;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * LfsFileMatch --
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
Boolean
LfsFileMatch(cacheInfoPtr, clientData)
    Fscache_FileInfo *cacheInfoPtr;
    ClientData	clientData;
{
    register int token = (int) clientData;

    if ((token < 0) || (cacheInfoPtr->hdrPtr->fileID.minor < 0)) {
	return  (cacheInfoPtr->hdrPtr->fileID.minor == token);
    }
    if (token == WRITEBACK_TOKEN) { 
	return ((cacheInfoPtr->flags & FSCACHE_FILE_BEING_CLEANED) == 0);
    } 
    if (token == CLEANING_TOKEN) {
	return ((cacheInfoPtr->flags & FSCACHE_FILE_BEING_CLEANED) != 0);
    } 
    /*
     * Assume CHECKPOINT_TOKEN token.
     */
    return TRUE;
}
#undef LOCKPTR
#define LOCKPTR &lfsPtr->logLock

/*
 *----------------------------------------------------------------------
 *
 * LfsDirLogEntryAlloc --
 *
 *	Allocate a directory log entry for a directory change operation.
 *	NOTE: This routines assumes the callers has LOCKPTR held.
 *
 * Results:
 *	A pointer to the allocated LfsDirOpLogEntry structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

LfsDirOpLogEntry *
LfsDirLogEntryAlloc(lfsPtr, entrySize, logSeqNum, foundPtr)
    Lfs		*lfsPtr;	/* LFS file system to allocate log entry for. */
    int		entrySize;	/* Size of log entry in bytes. */
    int		logSeqNum;	/* Log sequence number of entry. (-1) if new
				 * entry is needed. */
    Boolean	*foundPtr;	/* TRUE if logSeqNum award was found. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    LfsDirOpLogEntry *entryPtr;

    if (logSeqNum != -1) {
	LFS_STATS_INC(lfsPtr->stats.dirlog.entryAllocOld);
    } else {
	LFS_STATS_INC(lfsPtr->stats.dirlog.entryAllocNew);
    }
    /*
     * Wait until we are no longer writing or laying out blocks.
     */
    while (dirLogPtr->paused) {
	LFS_STATS_INC(lfsPtr->stats.dirlog.entryAllocWaits);
	Sync_Wait(&dirLogPtr->logPausedWait, FALSE);
    }
    /*
     * If the caller wants a brand new log record or the specified 
     * log record has been flushed from memory.
     */
    entryPtr = (logSeqNum != -1) ? FindLogEntry(lfsPtr, logSeqNum) : 
				   (LfsDirOpLogEntry *) NIL;
    if (entryPtr == (LfsDirOpLogEntry *) NIL) {
	if (entrySize > dirLogPtr->bytesLeftInBlock) {
	    LFS_STATS_INC(lfsPtr->stats.dirlog.newLogBlock);
	    NewDirLogBlock(lfsPtr);
	}
	entryPtr = (LfsDirOpLogEntry *) dirLogPtr->nextBytePtr;
	entryPtr->hdr.logSeqNum = dirLogPtr->nextLogSeqNum++;
	dirLogPtr->nextBytePtr += entrySize;
	dirLogPtr->bytesLeftInBlock -= entrySize;
	dirLogPtr->curBlockHdrPtr->size += entrySize;
	*foundPtr = FALSE;
	return entryPtr;
    } 
    /*
     * Called wanted a previous entry that is still in memory.
     */
    LFS_STATS_INC(lfsPtr->stats.dirlog.entryAllocFound);
    *foundPtr = TRUE;
    return entryPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DirLogInit --
 *
 *	Initialize the directory change log for this LFS file system.
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
DirLogInit(lfsPtr)
    Lfs	*lfsPtr;	/* File system to initialize dir log for. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    Fscache_Attributes		attr;

    dirLogPtr->nextLogSeqNum = 0;
    dirLogPtr->curBlockHdrPtr = (LfsDirOpLogBlockHdr *) NIL;
    dirLogPtr->nextBytePtr = (char *) NIL;
    dirLogPtr->bytesLeftInBlock = 0;
    List_Init(&dirLogPtr->activeListHdr);
    List_Init(&dirLogPtr->writingListHdr);
    /*
     * Initialize the file handle used to create descriptor blocks under
     */
    bzero((char *)(&dirLogPtr->handle),sizeof(dirLogPtr->handle));
    dirLogPtr->handle.hdr.fileID.major = lfsPtr->domainPtr->domainNumber;
    dirLogPtr->handle.hdr.fileID.minor = -1024;
    dirLogPtr->handle.hdr.fileID.type = FSIO_LCL_FILE_STREAM;
    dirLogPtr->handle.descPtr = (Fsdm_FileDescriptor *)NIL;

    bzero((Address)&attr, sizeof(attr));
    attr.lastByte = 0x7fffffff;
    Fscache_FileInfoInit(&dirLogPtr->handle.cacheInfo,
		    (Fs_HandleHeader *) &dirLogPtr->handle,
		    0, TRUE, &attr, lfsPtr->domainPtr->backendPtr);
    dirLogPtr->leastCachedSeqNum = 0;
    dirLogPtr->paused = FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * DirLogDestory --
 *
 *	Free up the directory change log for this LFS file system.
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
DirLogDestory(lfsPtr)
    Lfs	*lfsPtr;	/* File system to initialize dir log for. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;

    if (!List_IsEmpty(&dirLogPtr->activeListHdr) || 
	!List_IsEmpty(&dirLogPtr->writingListHdr) ||
	(dirLogPtr->curBlockHdrPtr !=  (LfsDirOpLogBlockHdr *) NIL)) {
	LfsError(lfsPtr, FAILURE,
		"DirLogDestory - directory log still active\n");
	return;
    }

    Fscache_FileInvalidate(&dirLogPtr->handle.cacheInfo, 0, FSCACHE_LAST_BLOCK);
}

/*
 *----------------------------------------------------------------------
 *
 * NewDirLogBlock --
 *
 *	Add a new directory change log blocks to the currently going log.
 *	This routine should be called when a log entry needs to be added 
 *	yet doesn't fit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fscache_Block blocks fetched from cache and put on activeList.
 *
 *----------------------------------------------------------------------
 */
static void
NewDirLogBlock(lfsPtr)
    Lfs	*lfsPtr;	/* File system of add dir log block for. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    LfsDirOpLogBlockHdr *curBlockHdrPtr;
    Fscache_Block	*blockPtr;
    Boolean	found;


   Fscache_FetchBlock(&dirLogPtr->handle.cacheInfo,
	    dirLogPtr->nextLogSeqNum, (FSCACHE_DESC_BLOCK|FSCACHE_DONT_BLOCK|FSCACHE_CANT_BLOCK),
		    &blockPtr, &found);
   if (blockPtr == (Fscache_Block *) NIL) {
	LfsError(lfsPtr, FAILURE, "No space for dir log block in cache");
	return;
   }
   if (found) {
	LfsError(lfsPtr, FAILURE, "Found dir log block in cache");
   }
   curBlockHdrPtr = dirLogPtr->curBlockHdrPtr =  
				(LfsDirOpLogBlockHdr *) blockPtr->blockAddr;

   curBlockHdrPtr->magic = LFS_DIROP_LOG_MAGIC;
   curBlockHdrPtr->size = sizeof(LfsDirOpLogBlockHdr);
   curBlockHdrPtr->nextLogBlock = 0;
   curBlockHdrPtr->reserved = 0;
   dirLogPtr->nextBytePtr = (char *) (curBlockHdrPtr+1);
   dirLogPtr->bytesLeftInBlock = FS_BLOCK_SIZE - sizeof(LfsDirOpLogBlockHdr);
   List_Insert((List_Links *) blockPtr, LIST_ATREAR(&dirLogPtr->activeListHdr));
}

/*
 *----------------------------------------------------------------------
 *
 * FindLogEntry --
 *
 *	Find a directory change operation log entry in the in memory 
 *	log buffers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static LfsDirOpLogEntry *
FindLogEntry(lfsPtr, logSeqNum)
    Lfs	*lfsPtr;	/* File system containing directory log. */
    int	logSeqNum;	/* Log sequent number we are looking for. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    Fscache_Block *blockPtr;
    LfsDirOpLogEntry *entryPtr, *limitPtr;
    LfsDirOpLogBlockHdr *curBlockHdrPtr;

    if (logSeqNum < dirLogPtr->leastCachedSeqNum) {
	LFS_STATS_INC(lfsPtr->stats.dirlog.fastFindFail);
	return (LfsDirOpLogEntry *) NIL;
    }

    blockPtr = (Fscache_Block *) List_Last(&dirLogPtr->activeListHdr);
    while(1) {  
	if (blockPtr->blockNum <= logSeqNum) {
	    curBlockHdrPtr = (LfsDirOpLogBlockHdr *) blockPtr->blockAddr;
	    limitPtr = (LfsDirOpLogEntry *) 
				(blockPtr->blockAddr + curBlockHdrPtr->size);
	    entryPtr = (LfsDirOpLogEntry *) (curBlockHdrPtr + 1);
	    while (entryPtr < limitPtr) { 
		LFS_STATS_INC(lfsPtr->stats.dirlog.findEntrySearch);
		if (entryPtr->hdr.logSeqNum == logSeqNum) {
			return entryPtr;
		}
		entryPtr = (LfsDirOpLogEntry *) 
				(((char *) entryPtr) + 
					LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr));
	    }
	}
	if (blockPtr == (Fscache_Block *)List_First(&dirLogPtr->activeListHdr)){
	    LfsError(lfsPtr, FAILURE, "Can't fine log entry in log.\n");
	    break;
	}
	blockPtr = (Fscache_Block *) List_Prev((List_Links *) blockPtr);
    }

    return (LfsDirOpLogEntry *) NIL;

}

/*
 *----------------------------------------------------------------------
 *
 * AddDirLogBlocks --
 *
 *	Add a directory change log blocks to a segment to be written.
 *
 * Results:
 *	TRUE if we filled up the segment. False otherwise.
 *
 * Side effects:
 *	Fscache_Blocks are moved from the activeList to the writingList.
 *
 *----------------------------------------------------------------------
 */
static Boolean
AddDirLogBlocks(lfsPtr, segPtr, segLayoutDataPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsSeg	*segPtr;	/* Segment to place data. */
    FileSegLayout  *segLayoutDataPtr; /* Layout data of segment. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    Fscache_Block *blockPtr;
    LfsDirOpLogBlockHdr *curBlockHdrPtr;
    LfsFileLayoutLog  *sumPtr;
    int		blocks;
    LOCK_MONITOR;
    sumPtr = (LfsFileLayoutLog *) NIL;
    while (!List_IsEmpty(&dirLogPtr->activeListHdr)) {
	   LfsSegElement *elementPtr;
	   blockPtr = (Fscache_Block *) List_First(&dirLogPtr->activeListHdr);
	   curBlockHdrPtr = (LfsDirOpLogBlockHdr *) blockPtr->blockAddr;
	   blocks = LfsBytesToBlocks(lfsPtr, curBlockHdrPtr->size);
	   if (sumPtr == (LfsFileLayoutLog *) NIL) {
	       sumPtr = (LfsFileLayoutLog *) LfsSegGrowSummary(segPtr, blocks, 
				sizeof(LfsFileLayoutLog));
	       if (sumPtr == (LfsFileLayoutLog *) NIL) {
		    UNLOCK_MONITOR;
		    return TRUE;
	       }
	       sumPtr->blockType = LFS_FILE_LAYOUT_DIR_LOG;
	       sumPtr->numDataBlocks = 0;
	       sumPtr->numBlocks = 0; /* Filled in later. */
	       sumPtr->reserved = 0;
	       dirLogPtr->paused = TRUE;
	   }
	   LfsSegSetSummaryPtr(segPtr, (char *) (sumPtr+1));
	   elementPtr = LfsSegAddDataBuffer(segPtr, blocks, 
			    blockPtr->blockAddr, (ClientData) blockPtr);
	   if (elementPtr == (LfsSegElement *) NIL) {
	        /*
		 * Could not fit block in segment.
		 */
		UNLOCK_MONITOR;
		return TRUE;
	    }
	   sumPtr->numDataBlocks++; 
	   sumPtr->numBlocks += blocks;
	   LFS_STATS_INC(lfsPtr->stats.dirlog.dataBlockWritten);
	   LFS_STATS_ADD(lfsPtr->stats.dirlog.blockWritten, blocks);
	   LFS_STATS_ADD(lfsPtr->stats.dirlog.bytesWritten, 
			curBlockHdrPtr->size);

	   List_Move((List_Links *) blockPtr, &segLayoutDataPtr->dirLogListHdr);
	   if  (curBlockHdrPtr == dirLogPtr->curBlockHdrPtr) {
		dirLogPtr->curBlockHdrPtr = (LfsDirOpLogBlockHdr *) NIL;
		dirLogPtr->nextBytePtr = (char *) NIL;
		dirLogPtr->bytesLeftInBlock = 0;
	   }
    }
    UNLOCK_MONITOR;
    return FALSE;

}

/*
 *----------------------------------------------------------------------
 *
 * FreeDirLogBlocks --
 *
 *	Free directory log blocks that were written to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fscache_Blocks on writingList are deleted from cache.
 *
 *----------------------------------------------------------------------
 */
static void
FreeDirLogBlocks(lfsPtr, segPtr, segLayoutDataPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsSeg	*segPtr;	/* Segment to place data. */
    FileSegLayout  *segLayoutDataPtr; /* Layout data of segment. */
{
    LfsDirLog *dirLogPtr = &lfsPtr->dirLog;
    Fscache_Block *blockPtr;

    LOCK_MONITOR;
    dirLogPtr->leastCachedSeqNum = dirLogPtr->nextLogSeqNum;
    while (!List_IsEmpty(&segLayoutDataPtr->dirLogListHdr)) {
	blockPtr = (Fscache_Block *) 
			List_First(&segLayoutDataPtr->dirLogListHdr);
	List_Remove((List_Links *)blockPtr);
	Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
    }
    dirLogPtr->paused = FALSE;
    Sync_Broadcast(&dirLogPtr->logPausedWait);
    UNLOCK_MONITOR;
}


