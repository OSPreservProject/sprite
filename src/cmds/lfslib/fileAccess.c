/* 
 * fileAccess.c --
 *
 *	Routines for access LFS files from user level using
 *	the raw disk interface. These routines are intended 
 *	to be called by crash recovery and other user programs
 *	that needed to scan the file.
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
#include <varargs.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <bstring.h>
#include <string.h>

enum AccessFileOp { OP_GET, OP_SET};

static Boolean AccessFileBlockAddr _ARGS_((enum AccessFileOp op, 
    LfsFile *filePtr, int blockNum, int *blockaddrPtr, int *blocksizePtr));

static LfsFile	*GetDirtyFile _ARGS_((Lfs *lfsPtr));
static void ReturnDirtyFile _ARGS_((LfsFile *filePtr, Boolean onFront));
static LfsFileBlock *GetDirtyBlock _ARGS_((LfsFile *filePtr, int blockType, 
			int *lastDirtyBlockPtr));
static void ReturnDirtyBlock _ARGS_((LfsFileBlock *fileBlockPtr,
			Boolean written));


/*
 *----------------------------------------------------------------------
 *
 * LfsFileOpenDesc --
 *
 *	Open a file specified by the LfsDescriptor for access.
 *
 * Results:
 *	SUCCESS if file can be access. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
LfsFileOpenDesc(lfsPtr, descPtr, filePtrPtr)
    Lfs	*lfsPtr;	/*  File system of file. */
    LfsFileDescriptor	*descPtr; /* Descriptor of file we want. */
    LfsFile **filePtrPtr;	/* OUT: Filled in File structure. */
{
    LfsFile		*filePtr;
    int	fileNumber;	

    fileNumber = descPtr->fileNumber;
    /*
     * First we lookup the entry in the cache of recently accessed
     * and/or modified files.
     */
    filePtr = (LfsFile *) NULL; /* Quiet down SaberC lint. */
    LIST_FORALL(&lfsPtr->fileLayout.activeFileListHdr, (List_Links *) filePtr) { 
	if ((filePtr->desc.fileNumber == fileNumber) &&
	    (bcmp((char *) descPtr, (char *) &(filePtr->desc), 
		 sizeof(LfsFileDescriptor)) == 0)) {
	    (*filePtrPtr) = filePtr;
	    return SUCCESS;
	}
    }

    filePtr = (LfsFile *) malloc(sizeof(LfsFile));
    List_InitElement((List_Links *) filePtr);
    List_Insert((List_Links *) filePtr, 
		LIST_ATFRONT(&lfsPtr->fileLayout.activeFileListHdr));
    filePtr->lfsPtr = lfsPtr;
    filePtr->desc = *descPtr;
    List_Init(&filePtr->blockListHdr);
    filePtr->descModified = FALSE;
    (*filePtrPtr) = filePtr;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileOpen --
 *
 *	Open a file (specifed by fileNumber) for access.
 *
 * Results:
 *	SUCCESS if file can be access. FS_FILE_NOT_FOUND if we can't
 *	file the file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
LfsFileOpen(lfsPtr, fileNumber, filePtrPtr)
    Lfs	*lfsPtr;	/*  File system of file. */
    int	fileNumber;	/* File we are looking for. */
    LfsFile **filePtrPtr;	/* OUT: Filled in File structure. */
{
    LfsDescMapEntry	*descMapEntryPtr;
    LfsFileDescriptor	*descPtr;
    int			bufSize, j;
    char		*descBuf;
    LfsFile		*filePtr;

    /*
     * First we lookup the entry in the cache of recently accessed
     * and/or modified files.
     */
    filePtr = (LfsFile *) NULL; /* Quiet down SaberC lint. */
    LIST_FORALL(&lfsPtr->fileLayout.activeFileListHdr, 
		    (List_Links *) filePtr) { 
	if ((filePtr->desc.fileNumber == fileNumber)) {
	    (*filePtrPtr) = filePtr;
	    return SUCCESS;
	}
    }
    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if ((descMapEntryPtr == (LfsDescMapEntry *) NIL) ||
        !(descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FS_FILE_NOT_FOUND;
    }

    bufSize = lfsPtr->superBlock.fileLayout.descPerBlock * sizeof(*descPtr);
    descBuf = alloca(bufSize);
    if (LfsDiskRead(lfsPtr, descMapEntryPtr->blockAddress, bufSize, descBuf)
		!= bufSize) {
	return FS_FILE_NOT_FOUND;
    }
    descPtr = (LfsFileDescriptor *)descBuf;
    for (j = 0; j < lfsPtr->superBlock.fileLayout.descPerBlock; j++) {
	if (descPtr->common.magic != FSDM_FD_MAGIC){
	    break;
	}
	if (descPtr->fileNumber == fileNumber) {
	    break;
	}
	descPtr++;
    }
    if ((j >= lfsPtr->superBlock.fileLayout.descPerBlock) ||
	!(descPtr->common.flags & FSDM_FD_ALLOC) ||
	(descPtr->fileNumber != fileNumber)) {
	return FS_FILE_NOT_FOUND;
    }
    return LfsFileOpenDesc(lfsPtr, descPtr, filePtrPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileClose --
 *
 *	Close a previously allocated File.  This routine free resources
 *	allocated by a File routine.
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
LfsFileClose(filePtr)
    LfsFile	*filePtr;	/* File to close. */
{
    LfsFileBlock	*fileBlockPtr;
    List_Links	*nextPtr;
    int		dirtyBlocks;

    /*
     * Free up all nondirty blocks fetched.
     */
    dirtyBlocks = 0;
    fileBlockPtr = (LfsFileBlock *) List_First(&filePtr->blockListHdr);
    while (!List_IsAtEnd(&filePtr->blockListHdr, (List_Links *) fileBlockPtr)) {
	nextPtr = List_Next((List_Links *) fileBlockPtr);
	if (fileBlockPtr->modified) {
	    dirtyBlocks++;
	} else {
	    List_Remove((List_Links *) fileBlockPtr);
	    free((char *) fileBlockPtr);
	}
	fileBlockPtr = (LfsFileBlock *) nextPtr;
    }
    /*
     * Free up the File data structure if nothing is dirty on the file.
     */
    if (filePtr->descModified || dirtyBlocks) {
	return;
    }
    List_Remove((List_Links *) filePtr);
    free((char *) filePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileBlockFetch --
 *
 *	Fetch a specified block of a File.
 *
 * Results:
 *	ReturnStatus.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFileBlockFetch(filePtr, blockNum, fileBlockPtrPtr)
    LfsFile	*filePtr;	/* File to fetch block from. */
    int		blockNum;	/* Block number to fetch. */
    LfsFileBlock	**fileBlockPtrPtr;  /* OUT: Allocated file block. */

{
    LfsFileBlock	*fileBlockPtr;
    int blockAddr, blocksize;

    /*
     * First check to see if we already have the block to this file 
     * in our list.
     */
    fileBlockPtr = (LfsFileBlock *) NULL;
    LIST_FORALL(&filePtr->blockListHdr, (List_Links *) fileBlockPtr) {
	if (fileBlockPtr->blockNum == blockNum) {
		(*fileBlockPtrPtr) = fileBlockPtr;
		return SUCCESS;
	}
    }
    /*
     * Fetch the block from disk.
     */
    blockAddr = LfsFileBlockAddr(filePtr, blockNum, &blocksize);
    if (blockAddr == FSDM_NIL_INDEX) {
	return FS_FILE_NOT_FOUND;
    }
    fileBlockPtr = (LfsFileBlock *) malloc(sizeof(LfsFileBlock));
    List_InitElement((List_Links *) fileBlockPtr);
    fileBlockPtr->filePtr = filePtr;
    fileBlockPtr->blockNum = blockNum;
    fileBlockPtr->blockSize = blocksize;
    fileBlockPtr->blockAddr = blockAddr;
    fileBlockPtr->modified = FALSE;
    if (LfsDiskRead(filePtr->lfsPtr, blockAddr, blocksize, 
		fileBlockPtr->contents) != blocksize) {
	free((char *) fileBlockPtr);
	return DEV_HARD_ERROR;
    }
    List_Insert((List_Links *) fileBlockPtr, 
		LIST_ATREAR(&filePtr->blockListHdr));
    (*fileBlockPtrPtr) = fileBlockPtr;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileBlockAlloc --
 *
 *	Allocate a new block to a file. The block is the specified size.
 *	The caller is responsible for initializing the new block.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFileBlockAlloc(filePtr,blockNum, blocksize, fileBlockPtrPtr)
    LfsFile	*filePtr; /* File to allocate block for. */
    int		blockNum; /* Block number of allocate. */
    int		blocksize; /* Size in bytes of new block. */
    LfsFileBlock	**fileBlockPtrPtr; /* OUT: New allocated file block. */
{
    LfsFileBlock	*fileBlockPtr;

    /*
     * First check to see if we already have the block to this file 
     * in our list.
     */
    fileBlockPtr = (LfsFileBlock *) NULL;
    LIST_FORALL(&filePtr->blockListHdr, (List_Links *) fileBlockPtr) {
	if (fileBlockPtr->blockNum == blockNum) {
		(*fileBlockPtrPtr) = fileBlockPtr;
		return SUCCESS;
	}
    }
    /*
     * Allocate and returned the block.
     */
    fileBlockPtr = (LfsFileBlock *) malloc(sizeof(LfsFileBlock));
    bzero((char *) fileBlockPtr, sizeof(LfsFileBlock));
    List_InitElement((List_Links *) fileBlockPtr);
    fileBlockPtr->filePtr = filePtr;
    fileBlockPtr->blockNum = blockNum;
    fileBlockPtr->blockSize = blocksize;
    fileBlockPtr->blockAddr = FSDM_NIL_INDEX;
    fileBlockPtr->modified = TRUE;
    List_Insert((List_Links *) fileBlockPtr, 
		LIST_ATREAR(&filePtr->blockListHdr));
    (*fileBlockPtrPtr) = fileBlockPtr;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileBlockAddr --
 *
 *	Return the address of a file block.
 *
 * Results:
 *	Address of block. FSDM_NIL_INDEX if block is not found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
LfsFileBlockAddr(filePtr, blockNum, blocksizePtr)
    LfsFile	*filePtr;	/* File to compute block address of. */
    int		blockNum;	/* Block number we would like the address of. */
    int		*blocksizePtr;	/* OUT: Size in bytes of block blockNum. */
{
    int blockAddr;
    Boolean error;

    error = AccessFileBlockAddr(OP_GET, filePtr, blockNum, 
			&blockAddr, blocksizePtr);
    if (error) {
	panic("LfsFileBlockAddr: block error.\n");
    }
    return blockAddr;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileSetBlockAddr --
 *
 *	Set the address of a file block.
 *
 * Results:
 *	Address of block. FSDM_NIL_INDEX;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
LfsFileSetBlockAddr(filePtr, blockNum, blockAddr)
    LfsFile	*filePtr;	/* File to compute block address of. */
    int		blockNum;	/* Block number we would like the address of. */
    int		blockAddr;	/* New block address. */
{
    Boolean error;
    int blocksize;

    error = AccessFileBlockAddr(OP_SET, filePtr, blockNum, 
			&blockAddr, &blocksize);
    if (error) {
	panic("LfsFileBlockAddr: block error.\n");
    }
    return blockAddr;
}

/*
 *----------------------------------------------------------------------
 *
 * AccessFileBlockAddr --
 *
 *	Set or Get the address of a file block.
 *
 * Results:
 *	TRUE if we found an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
AccessFileBlockAddr(op, filePtr, blockNum, blockaddrPtr, blocksizePtr)
    enum AccessFileOp op;	/* OP_GET or OP_SET. */
    LfsFile	*filePtr;	/* File to compute block address of. */
    int		blockNum;	/* Block number we would like the address of. */
    int		*blockaddrPtr;	/* OUT: Addresss of blockNum. */
    int		*blocksizePtr;	/* OUT: Size in bytes of block blockNum. */
{
    LfsFileBlock	*indirBlockPtr;
    int		*blockPtrs;
    int		parentBlockNum, parentIndex;
    LfsFileDescriptor *descPtr;
    ReturnStatus status;

    (*blocksizePtr) = FS_BLOCK_SIZE;
    descPtr = LfsFileFetchDesc(filePtr);
    if (blockNum >= 0) { 
	int	fileSize;
	/*
	 * Block is a data block. 
	 */
	fileSize = descPtr->common.lastByte+1;
	if (blockNum * FS_BLOCK_SIZE > fileSize) {
	    /*
	     * Block is passed the end of the file.
	     */
	    if (op == OP_GET) {
		(*blockaddrPtr) =  FSDM_NIL_INDEX;
		return FALSE;
	    } else {
		return TRUE;
	    }
	} 
	if (blockNum * FS_BLOCK_SIZE > fileSize - FS_BLOCK_SIZE) {
	    /*
	     * Block might be less than FS_BLOCK_SIZE because the next block
	     * is passed the end of the file.
	     */
	    (*blocksizePtr) = fileSize - blockNum * FS_BLOCK_SIZE;
	}
	if (blockNum < FSDM_NUM_DIRECT_BLOCKS) {
	    if (op == OP_GET) {
		(*blockaddrPtr) = descPtr->common.direct[blockNum];
	    } else {
		descPtr->common.direct[blockNum] = (*blockaddrPtr);
		LfsFileStoreDesc(filePtr);
	    }
	    return FALSE;
	}
	if (blockNum < (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK)) {
	    /*
	     * Block address pointer is in the first indirect block.
	     */
	    parentBlockNum = -1;
	    parentIndex = blockNum - FSDM_NUM_DIRECT_BLOCKS;
	    status = LfsFileBlockFetch(filePtr, parentBlockNum, &indirBlockPtr);
	    if (status != SUCCESS) {
		return TRUE;
	    }
	    blockPtrs = (int *) LfsFileBlockMem(indirBlockPtr);
	    if (op == OP_GET) {
		(*blockaddrPtr) = blockPtrs[parentIndex];
	    } else {
		blockPtrs[parentIndex] = (*blockaddrPtr);
		LfsFileStoreBlock(indirBlockPtr, -1);
	    }
	    return FALSE;
	}
	if (blockNum < (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK +
			   FSDM_INDICES_PER_BLOCK*FSDM_INDICES_PER_BLOCK)) {
	    /*
	     * Block address pointer is in a second level indirect block.
	     */
	    parentBlockNum = -((FSDM_NUM_INDIRECT_BLOCKS+1) +
		  (blockNum - FSDM_NUM_DIRECT_BLOCKS - FSDM_INDICES_PER_BLOCK)/
		    FSDM_INDICES_PER_BLOCK);
	    parentIndex = (blockNum - FSDM_NUM_DIRECT_BLOCKS - 
			   FSDM_INDICES_PER_BLOCK) % FSDM_INDICES_PER_BLOCK;

	    status = LfsFileBlockFetch(filePtr, parentBlockNum, &indirBlockPtr);
	    if (status != SUCCESS) {
		return TRUE;
	    }
	    blockPtrs = (int *) LfsFileBlockMem(indirBlockPtr);
	    if (op == OP_GET) {
		(*blockaddrPtr) = blockPtrs[parentIndex];
	    } else {
		blockPtrs[parentIndex] = (*blockaddrPtr);
		LfsFileStoreBlock(indirBlockPtr, -1);
	    }
	    return FALSE;
	}
    } else {
	/*
	 * Block is an indirect block.
	 */
	if (blockNum >= -FSDM_NUM_INDIRECT_BLOCKS) { 
	    /*
	     * This is a direct indirect block.
	     */
	    if (op == OP_GET) {
		(*blockaddrPtr) = descPtr->common.indirect[(-blockNum)-1];
	    } else {
		descPtr->common.indirect[(-blockNum)-1] = (*blockaddrPtr);
		LfsFileStoreDesc(filePtr);
	    }
	    return FALSE;
	}
	if (blockNum > -(FSDM_NUM_INDIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK)) {
	    parentIndex = (-blockNum) - (FSDM_NUM_INDIRECT_BLOCKS+1);
	    parentBlockNum = -2;
	    status = LfsFileBlockFetch(filePtr, parentBlockNum, &indirBlockPtr);
	    if (status != SUCCESS) {
		return TRUE;
	    }
	    blockPtrs = (int *) LfsFileBlockMem(indirBlockPtr);
	    if (op == OP_GET) {
		(*blockaddrPtr) = blockPtrs[parentIndex];
	    } else {
		blockPtrs[parentIndex] = (*blockaddrPtr);
		LfsFileStoreBlock(indirBlockPtr, -1);
	    }
	    return FALSE;
	}
    }
    /*
     * Past the largest file size that we support.
     */
   return(TRUE);

}


/*
 *----------------------------------------------------------------------
 *
 * LfsFileStoreDesc --
 *
 *	Mark the file to write back changes to the descriptor.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
LfsFileStoreDesc(filePtr)
    LfsFile	*filePtr;	/* File for each descriptor was changed. */
{
    filePtr->descModified = TRUE;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFileStoreBlock --
 *
 *	Mark the file block as dirty and to write back changes.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
LfsFileStoreBlock(fileBlockPtr, newSize)
    LfsFileBlock *fileBlockPtr;	/* Block block that was changed. */
    int	newSize;
{
    fileBlockPtr->modified = TRUE;
    if (newSize >= 0) {
	fileBlockPtr->blockSize = newSize;
    }
    return SUCCESS;
}

ReturnStatus 
LfsFileNameLookup(dirFilePtr, name, fileNumberPtr, dirOffsetPtr)
    LfsFile *dirFilePtr;
    char *name;
    int *fileNumberPtr;
    int *dirOffsetPtr;
{
    int	blockNum, length,  blockOffset;
    ReturnStatus	status;
    LfsFileDescriptor	*descPtr;
    Fslcl_DirEntry *newEntryPtr;
    LfsFileBlock	*dirBlockPtr;
    int	 dirOffset, compLen, blockSize;
    char	*blockPtr;

    descPtr = LfsFileFetchDesc(dirFilePtr);
    if (descPtr->common.fileType != FS_DIRECTORY) {
	return FS_NOT_DIRECTORY;
    }
    length = descPtr->common.lastByte+1;
    if ((length % FSLCL_DIR_BLOCK_SIZE) != 0) {
	panic("Directory %d has bad length %d\n", descPtr->fileNumber, length);
	return FAILURE;
    }
    /*
     * Find an available slot. 
     */
    dirOffset = -1;
    compLen = strlen(name);
    for (blockNum = 0; blockNum*FS_BLOCK_SIZE < length; blockNum++) {
	status = LfsFileBlockFetch(dirFilePtr, blockNum, &dirBlockPtr);
	if (status != SUCCESS) {
	    return FS_FILE_NOT_FOUND;
	}
	blockOffset = 0;
	blockSize = LfsFileBlockSize(dirBlockPtr);
	blockPtr = LfsFileBlockMem(dirBlockPtr);
	while (blockOffset < blockSize) {
	    newEntryPtr = (Fslcl_DirEntry *) (blockPtr + blockOffset);
	    if ((newEntryPtr->fileNumber != 0) &&
	        (newEntryPtr->nameLength == compLen) &&
		(strcmp(newEntryPtr->fileName, name) == 0)) {
		    (*fileNumberPtr) = newEntryPtr->fileNumber;
		    (*dirOffsetPtr) = blockNum * FS_BLOCK_SIZE + blockOffset;
		    return SUCCESS;
	    }
	    blockOffset += newEntryPtr->recordLength;
        }
    }
    return FS_FILE_NOT_FOUND;
}



static LfsFile	*
GetDirtyFile(lfsPtr)
    Lfs	*lfsPtr;	/* File system to return dirty file from. */
{
    LfsFile	*filePtr;

    if (List_IsEmpty(&lfsPtr->fileLayout.activeFileListHdr)) {
	return (LfsFile *) NIL;
    }
    filePtr = (LfsFile *) List_First(&lfsPtr->fileLayout.activeFileListHdr);
    List_Remove((List_Links *) filePtr);
    filePtr->beingWritten = TRUE;

    return filePtr;
}


static void
ReturnDirtyFile(filePtr, onFront)
    LfsFile	*filePtr;	/* File to return. */
    Boolean	onFront;	/* Of front of list. */
{
    Lfs	*lfsPtr = filePtr->lfsPtr;

    if (!List_IsEmpty(&filePtr->blockListHdr) || filePtr->descModified) {
	if (onFront) {
	    List_Insert((List_Links *) filePtr, 
		LIST_ATFRONT(&lfsPtr->fileLayout.activeFileListHdr));
	} else {
	    List_Insert((List_Links *) filePtr, 
		LIST_ATREAR(&lfsPtr->fileLayout.activeFileListHdr));
	}
	filePtr->beingWritten = FALSE;
	return;
    }

    free((char *) filePtr);
    return;
}

static 
LfsFileBlock *
GetDirtyBlock(filePtr, blockType, lastDirtyBlockPtr)
    LfsFile *filePtr;	/* File to get dirty block of. */
    int blockType;	/* Type of block wanted. */
    int *lastDirtyBlockPtr; /* OUT: TRUE if this is the last dirty block. */
{
    LfsFileBlock	*fileBlockPtr;
    int			blockNum;

    LIST_FORALL(&filePtr->blockListHdr, (List_Links *) fileBlockPtr) {
	blockNum = fileBlockPtr->blockNum;
	if (((blockType == LFS_FILE_LAYOUT_DATA) && (blockNum < 0)) ||
	    ((blockType == LFS_FILE_LAYOUT_INDIRECT) && 
				  ((blockNum >= 0) || (blockNum == -2))) || 
	    ((blockType == LFS_FILE_LAYOUT_DBL_INDIRECT) && (blockNum != -2))) { 
	    continue;
	} 
	if (!fileBlockPtr->modified) {	
	    panic("Non-modified block on list in GetDirtyBlock");
	}
	List_Remove((List_Links *) fileBlockPtr);
	(*lastDirtyBlockPtr) = List_IsEmpty(&filePtr->blockListHdr);
	return fileBlockPtr;
    }

    return (LfsFileBlock *) NIL;

}

static void
ReturnDirtyBlock(fileBlockPtr, written)
    LfsFileBlock *fileBlockPtr;
    Boolean	written;
{
    if (written) {
	free((char *) fileBlockPtr);
	return;
    }
    fileBlockPtr->modified = TRUE;
    List_Insert((List_Links *) fileBlockPtr, 
		LIST_ATFRONT(&fileBlockPtr->filePtr->blockListHdr));
}


typedef struct FileSegLayout {
    int	 numDescSlotsLeft;	/* Number of slots left in descriptor block. */
    LfsFileDescriptor *descBlockPtr;		
				/* Pointer to next slot in descriptor block. */
    int		 descDiskAddr; 	/* Disk address of descriptor block. */
    int maxElements;	  	/* The maximum number of elements. */
    List_Links	fileList; /* List of files in this segment. */
    List_Links  blockList; /* List of cache blocks laidout in
				       * this segment.  */
    LfsFile	*activeFilePtr;	/* File current being written. */
} FileSegLayout;

static Boolean PlaceFileInSegment _ARGS_((LfsSeg *segPtr, 
	LfsFile *filePtr,  FileSegLayout *segLayoutDataPtr));

    static FileSegLayout  *segLayoutDataPtr = (FileSegLayout *) NIL;


/*
 *----------------------------------------------------------------------
 *
 * LfsFileLayoutCheckpoint --
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
LfsFileLayoutCheckpoint(segPtr, checkPointPtr,  checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    Lfs		    *lfsPtr =   segPtr->lfsPtr;
    Boolean	    full;
    LfsFile	    *filePtr;

     /*
      * Next spill the file with dirty blocks into the segment. 
      */
     full = FALSE;
     if (segLayoutDataPtr == (FileSegLayout *) NIL) {

	/*
	 * Allocate a FileSegLayout data structure for this segment. 
	 */
	 segLayoutDataPtr = (FileSegLayout *) malloc(sizeof(FileSegLayout));
	 segLayoutDataPtr->numDescSlotsLeft = 0;
	 List_Init(&segLayoutDataPtr->fileList);
	 List_Init(&segLayoutDataPtr->blockList);
	 segLayoutDataPtr->maxElements = LfsSegSizeInBlocks(lfsPtr);
	 segLayoutDataPtr->activeFilePtr = (LfsFile *) NIL;
     } 
     /*
      * Choose the first file. If the last call to layout data into this
      * segment ended with a partially layed out file, start with that file.
      */
     if (segLayoutDataPtr->activeFilePtr == (LfsFile *) NIL) { 
	  filePtr = GetDirtyFile(lfsPtr);
	  lfsPtr->pstats.fileWritten++;
     } else {
	  filePtr = segLayoutDataPtr->activeFilePtr;
     }
     while (!full && (filePtr != (LfsFile *) NIL)) {
	   full = PlaceFileInSegment(segPtr, filePtr, segLayoutDataPtr);
	   if (full) {
	       segLayoutDataPtr->activeFilePtr = filePtr;
	       break;
	   } 
	   List_Insert((List_Links *) filePtr, 
				LIST_ATREAR(&segLayoutDataPtr->fileList));
	   filePtr = GetDirtyFile(lfsPtr);
    }
    if (!full && List_IsEmpty(&segLayoutDataPtr->fileList)) {
	free((char *)segLayoutDataPtr);
	segLayoutDataPtr = (FileSegLayout *) NIL;
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
PlaceFileInSegment(segPtr, filePtr, segLayoutDataPtr)
    LfsSeg	*segPtr;	/* Segment to place data. */
    LfsFile	 *filePtr;	/* File to place in segment. */
    FileSegLayout  *segLayoutDataPtr; /* Current layout data for segment. */
{
    Lfs		*lfsPtr = segPtr->lfsPtr;
    LfsFileLayoutSummary *fileSumPtr;
    Boolean	full;
    LfsSegElement	*bufferPtr;
    char	*summaryPtr;
    int		lfsBlocksPerBlock, lastDirtyBlock;
    int		blockType, oldAddr;
    int		blocksNeeded, bytesNeeded, blocksLeft;
    ReturnStatus	status;
    LfsFileBlock		*firstBlockPtr;
    LfsFileBlock		*blockPtr;

    if (filePtr == (LfsFile *) NIL) {
	return FALSE;
    }
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
	firstBlockPtr = GetDirtyBlock(filePtr,  blockType,
			     &lastDirtyBlock);
	/*
	 * No more blocks of this type available for this file, go on to 
	 * the next blockType. 
	 */ 
	if (firstBlockPtr == (LfsFileBlock *) NIL) {
	    continue;
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
			LfsBytesToBlocks(lfsPtr, LfsFileBlockSize(firstBlockPtr));
	   bytesNeeded = sizeof(LfsFileLayoutSummary) + sizeof(int);
	   summaryPtr = LfsSegGrowSummary(segPtr, blocksNeeded, bytesNeeded);
	   if (summaryPtr == (char *) NIL) { 
	       /*
	        * No room in summary. Return block and exit loop.
		*/
	       ReturnDirtyBlock(firstBlockPtr, FALSE);
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
	   fileSumPtr->fileNumber = filePtr->desc.fileNumber;
	   status = LfsDescMapGetVersion(lfsPtr, 
		       fileSumPtr->fileNumber, &fileSumPtr->truncVersion);
	   if (status != SUCCESS) {
	       panic( "Can't get truncate version number\n");
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
	   List_Insert((List_Links *) blockPtr, 
			LIST_ATFRONT(&segLayoutDataPtr->blockList));
	   blocksLeft -= LfsBytesToBlocks(lfsPtr, LfsFileBlockSize(blockPtr));
	   if (blocksLeft > 0) {
	       blockPtr = GetDirtyBlock(filePtr,  blockType, 
			&lastDirtyBlock);
	    }
	} while ((blocksLeft > 0) && (blockPtr != (LfsFileBlock *) NIL));

	/*
	 * Interate forward thru the blocks we pushed on the list to 
	 * lay them out in the reverse order.  We allow the first
	 * block to layout to be a fragment, so we start off by
	 * computing the number of fs blocks needed by the first
	 * cache block.
	 */
	blockPtr = (LfsFileBlock *) List_First(&segLayoutDataPtr->blockList);
	blocksNeeded = LfsBytesToBlocks(lfsPtr, LfsFileBlockSize(blockPtr));

	LIST_FORALL(&segLayoutDataPtr->blockList, (List_Links *) blockPtr) {
	    int bytesUsed;
	   /*
	    * Make sure there is enough room for both the data blocks in 
	    * the data region and the block number in the summary region.
	    */
	   summaryPtr = LfsSegGrowSummary(segPtr, blocksNeeded, sizeof(int));
	   if (summaryPtr == (char *) NIL) {
	       full = TRUE;
	       break;
	   }
	   segPtr->lfsPtr->pstats.fileBlockWritten += blocksNeeded;
	   /*
	    * Yes there is; add the cache block and fill in the summary region.
	    * Update the LfsFileLayoutSummary to reflect the data block being
	    * added and the number of fs blocks used.
	    */
	   *(int *) summaryPtr = blockPtr->blockNum;
	   summaryPtr += sizeof(int);
	   LfsSegSetSummaryPtr(segPtr,summaryPtr);
	   bufferPtr = LfsSegAddDataBuffer(segPtr, blocksNeeded, 
			    LfsFileBlockMem(blockPtr), (ClientData) blockPtr);

	   fileSumPtr->numDataBlocks++; 
	   fileSumPtr->numBlocks += blocksNeeded;

	   /*
	    * Update the index for this file and increment the 
	    * active bytes of the segment by the size of the cache
	    * block rounded to file system blocks.
	    */
	   bytesUsed = LfsBlocksToBytes(lfsPtr,
		    LfsBytesToBlocks(lfsPtr, blockPtr->blockSize));
	   oldAddr = LfsFileBlockAddress(blockPtr);
	   if (oldAddr != FSDM_NIL_INDEX) {
	       LfsSegUsageAdjustBytes(lfsPtr, oldAddr, -bytesUsed);
	   }
	   (void) LfsFileSetBlockAddr(filePtr,
				blockPtr->blockNum,
				LfsSegDiskAddress(segPtr, bufferPtr));
	   segPtr->activeBytes += bytesUsed;
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
		LfsFileBlock *nextBlockPtr;
		/*
		 * We're not able to place all the blocks, return to the cache
		 * all blocks we couldn't place.
		 */
		nextBlockPtr = (LfsFileBlock *) 
				List_Next((List_Links *)blockPtr);
		List_Remove((List_Links *) blockPtr);
		ReturnDirtyBlock(blockPtr, FALSE);
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
	descBytes = lfsPtr->superBlock.fileLayout.descPerBlock * 
			sizeof(LfsFileDescriptor);
	descBlocks = LfsBytesToBlocks(lfsPtr, descBytes);

	summaryPtr = LfsSegGrowSummary(segPtr, descBlocks, 
					    sizeof(LfsFileLayoutDesc));
	if (summaryPtr != (char *) NIL) {
	    LfsSegElement *descBufferPtr;
	    char	  *descMemPtr;
	    /*
	     * Allocate space for the descriptor block and fill in a
	     * summary block describing it. 
	     */
	    descBufferPtr = LfsSegAddDataBuffer(segPtr, descBlocks,
						(char *) NIL, (ClientData) NIL);
	    segLayoutDataPtr->numDescSlotsLeft = lfsPtr->superBlock.fileLayout.descPerBlock;
	    segLayoutDataPtr->descDiskAddr = 
				LfsSegDiskAddress(segPtr, descBufferPtr);
	    descMemPtr = malloc(descBytes);
	    segLayoutDataPtr->descBlockPtr = (LfsFileDescriptor *) descMemPtr;
	    descBufferPtr->address = descMemPtr;

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
	  LfsFileDescriptor *lfsDescPtr;
	  int		diskAddr;
	  lfsDescPtr = LfsFileFetchDesc(filePtr);
	  lfsDescPtr->common.flags &= ~FSDM_FD_DIRTY;
	  filePtr->descModified = FALSE;
	  bcopy((char *) &(lfsDescPtr->common), 
		(char *)&(segLayoutDataPtr->descBlockPtr->common),
		(int)sizeof(lfsDescPtr->common));
	  segLayoutDataPtr->descBlockPtr->fileNumber = filePtr->desc.fileNumber;
	  status = LfsDescMapGetDiskAddr(lfsPtr, 
			(int)segLayoutDataPtr->descBlockPtr->fileNumber, 
			&diskAddr);
	  if (diskAddr != FSDM_NIL_INDEX) {
	       LfsSegUsageAdjustBytes(lfsPtr, diskAddr, 
				-sizeof(LfsFileDescriptor));
	  }
	  status = LfsDescMapSetDiskAddr(lfsPtr, 
			(int)segLayoutDataPtr->descBlockPtr->fileNumber, 
			segLayoutDataPtr->descDiskAddr);
	  if (status != SUCCESS) {
	      panic("Can't update descriptor map.\n");
	  }
	  segLayoutDataPtr->descBlockPtr++;
	  segLayoutDataPtr->numDescSlotsLeft--;
	  segPtr->activeBytes += sizeof(LfsFileDescriptor);
	  segPtr->lfsPtr->pstats.descWritten++;
     } else {
	 full = TRUE;
     }
     return full;
}



/*
 *----------------------------------------------------------------------
 *
 * LfsFileLayoutWriteDone --
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
LfsFileLayoutWriteDone(segPtr, flags)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
{
    LfsSegElement *bufferPtr = LfsSegGetBufferPtr(segPtr);
    char	 *summaryPtr =  LfsSegGetSummaryPtr(segPtr);
    char	 *limitPtr;
    LfsFileBlock*blockPtr;
    LfsFile *filePtr;

    limitPtr = summaryPtr + LfsSegSummaryBytesLeft(segPtr); 
     while (summaryPtr < limitPtr) { 
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	     free(bufferPtr->address);
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
    if (segLayoutDataPtr != (FileSegLayout *) NIL) {
	while (!List_IsEmpty(&segLayoutDataPtr->blockList)) {
	    blockPtr = (LfsFileBlock *)
				List_First(&segLayoutDataPtr->blockList);
	    List_Remove((List_Links *)blockPtr);
	    ReturnDirtyBlock(blockPtr, TRUE);
	}
	while (!List_IsEmpty(&segLayoutDataPtr->fileList)) {
	    filePtr = 
		(LfsFile *) List_First(&segLayoutDataPtr->fileList);
	    List_Remove((List_Links *)filePtr);
	    ReturnDirtyFile(filePtr, TRUE);
	}
	if (segLayoutDataPtr->activeFilePtr != (LfsFile *) NIL) {
	    ReturnDirtyFile(segLayoutDataPtr->activeFilePtr, TRUE);
	}
	free((char *) segLayoutDataPtr);
	segLayoutDataPtr = (FileSegLayout *) NIL;
    }
    return;

}


