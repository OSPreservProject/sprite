/* 
 * file.c --
 *
 *	File and directory manipulation routines for the lfsrecov program.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <hash.h>
#include <libc.h>

#include "lfsrecov.h"
#include "desc.h"
#include "fileop.h"
#include "usage.h"
#include "dirlog.h"

LfsFile *FindLost_FoundDir _ARGS_((Lfs *lfsPtr));

void CheckVersionTruncNum _ARGS_((int startAddress, int fileNumber, 
			int truncVersion, int numBlocks,
			int numDataBlocks, int *blockArray));


/*
 *----------------------------------------------------------------------
 *
 * RecordBlockUsageChange --
 *
 *	Record in the SegUsage map any changes that we caused by a
 *	new inode going out
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
RecordBlockUsageChange(fileNumber, oldFilePtr, newFilePtr)
    int	fileNumber;	/* File number being changed. */
    LfsFile	*oldFilePtr; /* Old version of file, NIL if didn't exist. */
    LfsFile	*newFilePtr; /* New version of file, NIL if deleted. */
{
    LfsFileDescriptor *oldDescPtr, *newDescPtr;	
    int numBytes[2], numBlocks[2], lastByteBlock[2], lastByteBlockSize[2], i;
    int oldBlock, newBlock;

    oldDescPtr = newDescPtr = (LfsFileDescriptor *) NIL;

    if (oldFilePtr != (LfsFile *) NIL) {
	oldDescPtr = LfsFileFetchDesc(oldFilePtr);
	if (oldDescPtr->common.lastByte == -1) {
	    oldFilePtr = (LfsFile *) NIL;
	}
    }
    if (newFilePtr != (LfsFile *) NIL) {
	newDescPtr = LfsFileFetchDesc(newFilePtr);
	if (newDescPtr->common.lastByte == -1) {
	    newFilePtr = (LfsFile *) NIL;
	}
    }
    if ((oldFilePtr == (LfsFile *) NIL) && (newFilePtr == (LfsFile *) NIL)) {
	/*
	 * Nothing to do if file blocks didn't change.
	 */
	return;
    }


    if (oldFilePtr == (LfsFile *) NIL) {
	numBytes[0] = -1;
	numBlocks[0] = -1;
	lastByteBlock[0] = -1;
	lastByteBlockSize[0] = -1;
    } else {
	numBytes[0] = (oldDescPtr->common.lastByte+1+blockSize-1)/blockSize;
	numBytes[0] *= blockSize;
	numBlocks[0] = (numBytes[0] + FS_BLOCK_SIZE - 1)/FS_BLOCK_SIZE;
	lastByteBlock[0] = oldDescPtr->common.lastByte/FS_BLOCK_SIZE;
	lastByteBlockSize[0] = numBytes[0] - lastByteBlock[0] * FS_BLOCK_SIZE;
    }
    if (newFilePtr == (LfsFile *) NIL) {
	numBytes[1] = -1;
	numBlocks[1] = -1;
	lastByteBlock[1] = -1;
	lastByteBlockSize[1] = -1;
    } else {
	numBytes[1] = (newDescPtr->common.lastByte+1+blockSize-1)/blockSize;
	numBytes[1] *= blockSize;
	numBlocks[1] = (numBytes[0] + FS_BLOCK_SIZE - 1)/FS_BLOCK_SIZE;
	lastByteBlock[1] = newDescPtr->common.lastByte/FS_BLOCK_SIZE;
	lastByteBlockSize[1] = numBytes[1] - lastByteBlock[1] * FS_BLOCK_SIZE;
    }


    /*
     * Record any DBL_INDIRECT blocks first.
     */
    if ((numBlocks[0] >= (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK)) ||
        (numBlocks[1] >= (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK))) {
	oldBlock = (oldFilePtr != (LfsFile *) NIL) ? 
		    oldDescPtr->common.indirect[1] : FSDM_NIL_INDEX;
	newBlock = (newFilePtr != (LfsFile *) NIL) ? 
		    newDescPtr->common.indirect[1] : FSDM_NIL_INDEX;
	if (oldBlock != newBlock) { 
	    RecordIndirectBlockUsage(oldFilePtr, newFilePtr, -2,
		    FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK,
		    FSDM_INDICES_PER_BLOCK, lastByteBlock, 
		    lastByteBlockSize);
	}
    } 
	/*
	 * Followed by any INDIRECT blocks.
	 */
    if ((numBlocks[0] >= FSDM_NUM_DIRECT_BLOCKS) ||
        (numBlocks[1] >=(FSDM_NUM_DIRECT_BLOCKS))) {
	oldBlock = (oldFilePtr != (LfsFile *) NIL) ? 
		    oldDescPtr->common.indirect[0] : FSDM_NIL_INDEX;
	newBlock = (newFilePtr != (LfsFile *) NIL) ? 
		    newDescPtr->common.indirect[0] : FSDM_NIL_INDEX;
	if (oldBlock != newBlock) { 
	    RecordIndirectBlockUsage(oldFilePtr, newFilePtr, -1,
		FSDM_NUM_DIRECT_BLOCKS, 1, lastByteBlock, 
		lastByteBlockSize);
	}
    }
    /*
     * Finally the DIRECT blocks.
     */
    for (i = 0; (i < FSDM_NUM_DIRECT_BLOCKS); i++) {
	oldBlock = (oldFilePtr != (LfsFile *) NIL) ? 
		    oldDescPtr->common.direct[i] : FSDM_NIL_INDEX;
	newBlock = (newFilePtr != (LfsFile *) NIL) ? 
		    newDescPtr->common.direct[i] : FSDM_NIL_INDEX;
	if (oldBlock != newBlock) {
	    if (lastByteBlock[0] != i) {
		LfsSegUsageAdjustBytes(lfsPtr,oldBlock, -FS_BLOCK_SIZE);
	    } else {
		LfsSegUsageAdjustBytes(lfsPtr,oldBlock, -lastByteBlockSize[0]);
	    }
	    if (lastByteBlock[1] != i) {
		LfsSegUsageAdjustBytes(lfsPtr,newBlock, FS_BLOCK_SIZE);
	    } else {
		LfsSegUsageAdjustBytes(lfsPtr,newBlock, lastByteBlockSize[1]);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RecordIndirectBlockUsage --
 *
 *	Record the usage of an file's indirect block.
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
RecordIndirectBlockUsage(oldFilePtr, newFilePtr, virtualBlockNum,
	startBlockNum, step, lastByteBlock, lastByteBlockSize) 
    LfsFile  *oldFilePtr;		/* old File being operatored on. */
    LfsFile  *newFilePtr;		/* new File being operatored on. */
    int	  virtualBlockNum;	/* Virtual block number for this indirect 
				 * block. */
    int	  startBlockNum;	/* Starting block number of this virtual
				 * Block. */
    int	  step;			/* Number of blocks covered by each virtual
				 * block entry. */
    int	  lastByteBlock[2];	/* Block containing last byte each  file. */
    int	  lastByteBlockSize[2];	/* Size of last block  of each file. */
{
    ReturnStatus status;
    LfsFileBlock	*oldFileBlockPtr, *newFileBlockPtr;
    int		*oldBlockPtrs, *newBlockPtrs, i, cstep, childBlockNum;
    int		oldBlock, newBlock;

    newBlockPtrs = oldBlockPtrs = (int *) NIL;
    if (oldFilePtr != (LfsFile *) NIL) { 
	status = LfsFileBlockFetch(oldFilePtr, virtualBlockNum, &oldFileBlockPtr);
	if (status == SUCCESS) {
	    oldBlockPtrs = (int *) LfsFileBlockMem(oldFileBlockPtr);
	} 
    }
    if (newFilePtr != (LfsFile *) NIL) { 
	status = LfsFileBlockFetch(newFilePtr, virtualBlockNum, &newFileBlockPtr);
	if (status == SUCCESS) {
	    newBlockPtrs = (int *) LfsFileBlockMem(newFileBlockPtr);
	}
    } 
    if (newBlockPtrs == oldBlockPtrs) {
	return;
    }
    if (step != 1) {
	cstep = step/FSDM_INDICES_PER_BLOCK;
	childBlockNum = -((FSDM_NUM_INDIRECT_BLOCKS+1));
	for (i = 0; i < FSDM_INDICES_PER_BLOCK; i++) { 
	    oldBlock = (oldBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
							oldBlockPtrs[i];
	    newBlock = (newBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
							newBlockPtrs[i];
	    if (oldBlock != newBlock) {
		    RecordIndirectBlockUsage(oldFilePtr, newFilePtr,
				childBlockNum, startBlockNum, cstep, 
				lastByteBlock, lastByteBlockSize);
	    }
	    startBlockNum += step;
	    childBlockNum--;
	}
    } else { 
	for (i = 0; i < FSDM_INDICES_PER_BLOCK; i++) {
	    oldBlock = (oldBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
							oldBlockPtrs[i];
	    newBlock = (newBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
							newBlockPtrs[i];
	    if (startBlockNum + i == lastByteBlock[0]) {
		  LfsSegUsageAdjustBytes(lfsPtr,oldBlock, -lastByteBlockSize[0]);
	    } else  {
		  LfsSegUsageAdjustBytes(lfsPtr,oldBlock, -FS_BLOCK_SIZE);
	    }
	    if (startBlockNum + i == lastByteBlock[1]) {
		  LfsSegUsageAdjustBytes(lfsPtr,newBlock, lastByteBlockSize[1]);
	    } else  {
		  LfsSegUsageAdjustBytes(lfsPtr,newBlock, FS_BLOCK_SIZE);
	    }
	}
    }
    oldBlock = (oldBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
				LfsFileBlockAddress(oldFileBlockPtr);
    newBlock = (newBlockPtrs == (int *) NIL) ? FSDM_NIL_INDEX : 
				LfsFileBlockAddress(newFileBlockPtr);
    if (oldBlock != newBlock) {
	 LfsSegUsageAdjustBytes(lfsPtr,oldBlock, -FS_BLOCK_SIZE);
	 LfsSegUsageAdjustBytes(lfsPtr,newBlock, FS_BLOCK_SIZE);
    } 
}

/*
 *----------------------------------------------------------------------
 *
 * AddEntryToDirectory --
 *
 *	Add a directory entry to the specified directory and offset. 
 *	This routine is used to redo operations on directory
 *	lost during a crash because the directory block didn't make
 *	it out. An offset of -1 means add to any available place in
 * 	the directory.
 *
 * Results:
 *	SUCCESS if operation completed. 
 *
 * Side effects:
 *	A block of the directory may be modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
AddEntryToDirectory(dirFileNumber, dirOffset, dirEntryPtr, isDirectory,mayExist)
    int	dirFileNumber;	/* Directory descriptor number. */
    int	dirOffset;	/* Offset into directory of entry. */
    Fslcl_DirEntry *dirEntryPtr; /* Directory entry to addr. */
    Boolean	isDirectory;	/* TRUE if directory entry is for a 
				 * subdirectory. */
    Boolean	mayExist;	 /* The directory entry may exist. */
{
    int	blockNum, offset, length,  blockOffset;
    int	newRecordLen;
    ReturnStatus	status;
    LfsFileDescriptor	*descPtr;
    Fslcl_DirEntry *newEntryPtr;
    LfsFile	*dirFilePtr;
    LfsFileBlock	*dirBlockPtr;
    char	*blockPtr;

    stats.dirEntryAdded++;
    if (verboseFlag) {
	char buffer[FS_MAX_NAME_LENGTH+1];
	bcopy(dirEntryPtr->fileName, buffer, dirEntryPtr->nameLength);
	buffer[dirEntryPtr->nameLength] = 0;
	printf("Adding file %d under name \"%s\" to directory %d\n",
		dirEntryPtr->fileNumber, buffer, dirFileNumber);
    }

    status = LfsFileOpen(lfsPtr, dirFileNumber, &dirFilePtr);
    if (status != SUCCESS) {
	return status;
    }


    descPtr = LfsFileFetchDesc(dirFilePtr);
    if (descPtr->common.fileType != FS_DIRECTORY) {
	panic("Trying to add entry to non-directory %d\n", dirFileNumber);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    length = descPtr->common.lastByte+1;
    if ((length % FSLCL_DIR_BLOCK_SIZE) != 0) {
	panic("Directory %d has bad length %d\n", dirFileNumber, length);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    if (dirOffset == -1) {
	int	recordLength;
	/*
	 * Find an available slot. 
	 */
	dirOffset = 0;
	recordLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
	for (blockNum = 0; !dirOffset; blockNum++) {
	    status = LfsFileBlockFetch(dirFilePtr, blockNum, &dirBlockPtr);
	    if (status != SUCCESS) {
		dirOffset = blockNum * FS_BLOCK_SIZE;
		break;
	    }
	    blockOffset = 0;
	    blockSize = LfsFileBlockSize(dirBlockPtr);
	    blockPtr = LfsFileBlockMem(dirBlockPtr);
	    while (blockOffset < blockSize) {
		newEntryPtr = (Fslcl_DirEntry *) (blockPtr + blockOffset);
		if (newEntryPtr->fileNumber != 0) {
		    int	extraBytes;
		    /*
		     * A valid directory record.
		     * Check the left-over bytes attached to this record.
		     */
		    extraBytes = newEntryPtr->recordLength -
				 Fslcl_DirRecLength(newEntryPtr->nameLength);
		    if (extraBytes >= recordLength) {
			/*
			 * Can fit new entry in the space left over.
			 */ 
			dirOffset = blockNum * FS_BLOCK_SIZE + blockOffset +
				Fslcl_DirRecLength(newEntryPtr->nameLength);
			break;
		    }
		} else {
		    /*
		     * A deleted name in the directory.
		     */
		    if (newEntryPtr->recordLength >= recordLength) {
			dirOffset = blockNum * FS_BLOCK_SIZE + blockOffset ;
			break;
		    }
		}
		blockOffset += newEntryPtr->recordLength;
	   }
	   if (!dirOffset && (blockSize != FS_BLOCK_SIZE)) {
		dirOffset = blockNum * FS_BLOCK_SIZE + blockSize;
	   }
	}
    }


    if (dirOffset >= descPtr->common.lastByte) {
	status = GrowDirectory(dirFilePtr, dirOffset);
	if (status != SUCCESS) {
	    LfsFileClose(dirFilePtr);
	    return FAILURE;
	}
    }
    blockNum = dirOffset/FS_BLOCK_SIZE;
    status = LfsFileBlockFetch(dirFilePtr, blockNum, &dirBlockPtr);
    if (status != SUCCESS) {
	panic("Can't find block %d of directory %d\n", blockNum, dirFileNumber);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    blockPtr = LfsFileBlockMem(dirBlockPtr);
    blockPtr += (dirOffset / FSLCL_DIR_BLOCK_SIZE) * FSLCL_DIR_BLOCK_SIZE - 
		((dirOffset / FS_BLOCK_SIZE) * FS_BLOCK_SIZE);
    blockOffset = 0;
    offset = dirOffset % FSLCL_DIR_BLOCK_SIZE;
    newEntryPtr = (Fslcl_DirEntry *) NULL;
    while (blockOffset < FSLCL_DIR_BLOCK_SIZE) {
	if (blockOffset >= offset) {
	    break;
	}
	newEntryPtr = (Fslcl_DirEntry *) (blockPtr + blockOffset);
	if (newEntryPtr->recordLength <= 0) {
	    panic("Corrupted directory %d\n", dirFileNumber);
	}
	blockOffset += newEntryPtr->recordLength;
    }
    if (blockOffset == offset) {
	/*
	 * A directory entry ends at this offset. We make sure something
	 * is not already valid there and add it.
	 */
	newEntryPtr = (Fslcl_DirEntry *) (blockPtr + offset);
	if (newEntryPtr->fileNumber != 0) {
		if (!mayExist) {
		    panic("Valid directory entry in %d at offset %d\n", 
				dirFileNumber, offset);
		}
		if ((newEntryPtr->fileNumber != dirEntryPtr->fileNumber) ||
		    (newEntryPtr->nameLength != dirEntryPtr->nameLength) ||
		    (strncmp(newEntryPtr->fileName, dirEntryPtr->fileName,
				dirEntryPtr->nameLength))) {
		    panic("Wrong directory entry in %d at offset %d\n",
				dirFileNumber, offset);
		}
		LfsFileClose(dirFilePtr);
		return SUCCESS;
	}
	newRecordLen = Fslcl_DirRecLength(dirEntryPtr->nameLength);
	if (newRecordLen > newEntryPtr->recordLength) {
	    panic("Directory entry in %d at %d is too large\n",
			dirFileNumber, offset);
	}
	newEntryPtr->fileNumber = dirEntryPtr->fileNumber;
	newEntryPtr->nameLength = dirEntryPtr->nameLength;
	bcopy(dirEntryPtr->fileName, newEntryPtr->fileName, 
			dirEntryPtr->nameLength);
	newEntryPtr->fileName[dirEntryPtr->nameLength] = 0;
    } else {
	/*
	 * We are inserting in the middle of a previous entry.  We must
	 * shortten it. 
	 */
	newRecordLen =  newEntryPtr->recordLength - (blockOffset - offset);
	if (newRecordLen < Fslcl_DirRecLength(newEntryPtr->nameLength)) {
	    panic("Insert pattern doesn't fit.\n");
	}
	newEntryPtr->recordLength = newRecordLen;
	newEntryPtr = (Fslcl_DirEntry *) (blockPtr + offset);
	newEntryPtr->fileNumber = dirEntryPtr->fileNumber;
	newEntryPtr->recordLength = blockOffset - offset;
	newEntryPtr->nameLength = dirEntryPtr->nameLength;
	bcopy(dirEntryPtr->fileName, newEntryPtr->fileName, 
			dirEntryPtr->nameLength);
	newEntryPtr->fileName[dirEntryPtr->nameLength] = 0;
    }
    if (isDirectory) {
	/*
	 * If we added a subdirectory we need to update the parent's link count
	 * to reflect the ".." entry pointing at it.
	 */
	descPtr->common.numLinks++;
	LfsFileStoreDesc(dirFilePtr);
    }
    status = LfsFileStoreBlock(dirBlockPtr, -1);
    LfsFileClose(dirFilePtr);
    return   status;

}

/*
 *----------------------------------------------------------------------
 *
 * RemovedEntryFromDirectory --
 *
 *	Remote a directory entry from the specified directory and offset. 
 *	This routine is used to redo operations on directory
 *	lost during a crash because the directory block didn't make
 *	it out. An offset -1 dirEntry may be anywhere in directory.
 *
 * Results:
 *	SUCCESS if operation completed. 
 *
 * Side effects:
 *	A block of the directory may be modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
RemovedEntryFromDirectory(dirFileNumber, dirOffset, dirEntryPtr, 
			  isDirectory, mayBeGone)
    int	dirFileNumber;	/* Directory descriptor number. */
    int	dirOffset;	/* Offset into directory of entry. */
    Fslcl_DirEntry *dirEntryPtr; /* Directory entry value. */
    Boolean	isDirectory;	/* TRUE if rmdir. */
    Boolean	mayBeGone;	 /* The directory entry be already deleted. */
{
    int	blockNum, offset, length, blockOffset;
    LfsFile	*dirFilePtr;
    LfsFileBlock	*dirBlockPtr;
    ReturnStatus	status;
    LfsFileDescriptor	*descPtr;
    Fslcl_DirEntry *lastDirEntryPtr, *newEntryPtr;
    char	*blockPtr;

    stats.dirEntryRemoved++;
    if (verboseFlag) {
	char buffer[FS_MAX_NAME_LENGTH+1];
	bcopy(dirEntryPtr->fileName, buffer, dirEntryPtr->nameLength);
	buffer[dirEntryPtr->nameLength] = 0;
	printf("Removing file %d under name \"%s\" from directory %d\n",
		dirEntryPtr->fileNumber, buffer, dirFileNumber);
    }

    blockNum = dirOffset/FS_BLOCK_SIZE;
    status = LfsFileOpen(lfsPtr, dirFileNumber, &dirFilePtr);
    if (status != SUCCESS) {
	return status;
    }
    descPtr = LfsFileFetchDesc(dirFilePtr);
    if (descPtr->common.fileType != FS_DIRECTORY) {
	panic("Trying to delete entry from non-directory %d\n", dirFileNumber);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    length = descPtr->common.lastByte+1;
    if ((length % FSLCL_DIR_BLOCK_SIZE) != 0) {
	panic("Directory %d has bad length %d\n", dirFileNumber, length);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    if (dirOffset == -1) {
	/*
	 * Find an available slot. 
	 */
	dirOffset = 0;
	for (blockNum = 0; !dirOffset; blockNum++) {
	    status = LfsFileBlockFetch(dirFilePtr, blockNum, &dirBlockPtr);
	    if (status != SUCCESS) {
		dirOffset = blockNum * FS_BLOCK_SIZE;
		break;
	    }
	    blockOffset = 0;
	    blockSize = LfsFileBlockSize(dirBlockPtr);
	    blockPtr = LfsFileBlockMem(dirBlockPtr);
	    while (blockOffset < blockSize) {
		newEntryPtr = (Fslcl_DirEntry *) (blockPtr + blockOffset);
		if (newEntryPtr->fileNumber == dirEntryPtr->fileNumber) {
		    dirOffset = blockNum * FS_BLOCK_SIZE + blockOffset;
		    break;
		} 
		blockOffset += newEntryPtr->recordLength;
	   }
	   if (blockSize != FS_BLOCK_SIZE) {
	       break;
	   }
	}
	if (dirOffset == 0) {
	    return FAILURE;
	}
    }

    if (dirOffset >= descPtr->common.lastByte) {
	panic("Delete entry from directory %d at nonexistant offset %d\n",
		dirFileNumber, blockNum);
    }
    status = LfsFileBlockFetch(dirFilePtr, blockNum, &dirBlockPtr);
    if (status != SUCCESS) {
	panic("Can't find block %d of directory %d\n", blockNum, dirFileNumber);
	LfsFileClose(dirFilePtr);
	return FAILURE;
    }
    blockPtr = LfsFileBlockMem(dirBlockPtr);
    offset = dirOffset % FSLCL_DIR_BLOCK_SIZE;
    blockPtr += (dirOffset / FSLCL_DIR_BLOCK_SIZE) * FSLCL_DIR_BLOCK_SIZE - 
		((dirOffset / FS_BLOCK_SIZE) * FS_BLOCK_SIZE);
    blockOffset = 0;
    lastDirEntryPtr = (Fslcl_DirEntry *) NULL;
    while (blockOffset < FSLCL_DIR_BLOCK_SIZE) {
	if (blockOffset >= offset) {
	    break;
	}
	lastDirEntryPtr = (Fslcl_DirEntry *) (blockPtr + blockOffset);
	if (lastDirEntryPtr->recordLength <= 0) {
	    panic("Corrupted directory %d\n", dirFileNumber);
	}
	blockOffset += lastDirEntryPtr->recordLength;
    }
    if (blockOffset == offset) {
	/*
	 * Make sure the entry is the correct one. We make sure something
	 * is not already valid there and add it.
	 */
	newEntryPtr = (Fslcl_DirEntry *) (blockPtr + offset);
	if ((newEntryPtr->fileNumber != dirEntryPtr->fileNumber) ||
	    (newEntryPtr->nameLength != dirEntryPtr->nameLength) ||
	    (strncmp(newEntryPtr->fileName, dirEntryPtr->fileName,
			dirEntryPtr->nameLength))) {
	    if (!mayBeGone) {
		panic("Wrong directory entry in %d at offset %d\n",
			dirFileNumber, offset);
	    }
	    return SUCCESS;
	}
	newEntryPtr->fileNumber = 0;
	if (lastDirEntryPtr != (Fslcl_DirEntry *)NULL) {
	    /*
	     * Grow the previous record so that it now includes
	     * this one.
	     */
	    lastDirEntryPtr->recordLength += newEntryPtr->recordLength;
	}
	if (isDirectory) {
	    descPtr->common.numLinks--;
	    LfsFileStoreDesc(dirFilePtr);
	}
    } else {
	/*
	 * We are deleting in the middle of an entry.  
	 */
	if (!mayBeGone) {
		panic("Can find directory entry in %d at %d to delete.\n",
			dirFileNumber, offset);
	}
    }
    status = LfsFileStoreBlock(dirBlockPtr, -1);
    LfsFileClose(dirFilePtr);
    return status;

}


/*
 *----------------------------------------------------------------------
 *
 * GrowDirectory --
 *
 *	Add a block to a directory so that an entry at the specified 
 *	offset can be created. The routine is used to add create room
 *	to add entries to a directory after a crash.
 *
 * Results:
 *	SUCCESS if operation completed. 
 *
 * Side effects:
 *	A block of the directory will be created.
 *
 *----------------------------------------------------------------------
 */


ReturnStatus
GrowDirectory(dirFilePtr, dirOffset)
    LfsFile *dirFilePtr;	/* Open directory to grow. */
    int	 dirOffset;	/* Offset of entry we would like to include. */
{
    LfsFileDescriptor	*descPtr;
    int			curBlockLength, neededBlockLength, blocksize;
    Fslcl_DirEntry	*newDirEntryPtr;
    LfsFileBlock		*dirBlockPtr;
    char		*blockPtr;
    ReturnStatus	status;

    descPtr = LfsFileFetchDesc(dirFilePtr);
    curBlockLength = (descPtr->common.lastByte + 1 + (FSLCL_DIR_BLOCK_SIZE-1)) /
				FSLCL_DIR_BLOCK_SIZE;
    neededBlockLength = (dirOffset + (FSLCL_DIR_BLOCK_SIZE-1)) / 
				FSLCL_DIR_BLOCK_SIZE;
    if (neededBlockLength - curBlockLength > 1) {
	/*
	 * One at most one block at a time. 
	 */
	panic("Directory offset more that one block past current lastbyte\n");
    }

    /*
     * DB_PER_FSB - Directory blocks per file system blocks
     */
#define	DB_PER_FSB 	(FS_BLOCK_SIZE/FSLCL_DIR_BLOCK_SIZE)

    if (neededBlockLength/DB_PER_FSB == curBlockLength/DB_PER_FSB) {
	/*
	 * Just growing the existing block.
	 */
	status = LfsFileBlockFetch(dirFilePtr, neededBlockLength/DB_PER_FSB,
				&dirBlockPtr);
	if (status != SUCCESS) {
	    panic("Can't find block %d of directory %d\n", 
			neededBlockLength/DB_PER_FSB, descPtr->fileNumber);
	}
	blockPtr = LfsFileBlockMem(dirBlockPtr);
	blocksize = LfsFileBlockSize(dirBlockPtr);
	newDirEntryPtr = (Fslcl_DirEntry *) (blockPtr + blocksize);
	blocksize += FSLCL_DIR_BLOCK_SIZE;
    } else {
	/*
	 * Need to add a new block to the file.
	 */
	blocksize = FSLCL_DIR_BLOCK_SIZE;
	status = LfsFileBlockAlloc(dirFilePtr,neededBlockLength/DB_PER_FSB,
				FSLCL_DIR_BLOCK_SIZE, &dirBlockPtr);
	blockPtr = LfsFileBlockMem(dirBlockPtr);
	newDirEntryPtr = (Fslcl_DirEntry *) blockPtr;
    }
    descPtr->common.lastByte += FSLCL_DIR_BLOCK_SIZE;
    bzero((char *) newDirEntryPtr, FSLCL_DIR_BLOCK_SIZE);
    newDirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
    LfsFileStoreDesc(dirFilePtr);
    return LfsFileStoreBlock(dirBlockPtr, blocksize);
#undef DB_PER_FSB
}


/*
 *----------------------------------------------------------------------
 *
 * FileTypeToString --
 *
 *	Covert a inode file type into a printable string.
 *
 * Results:
 *	A printable string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


char *
FileTypeToString(fileType)
    int fileType;
{
    static char *names[] = { "File", "Dir", "Symlink", "Rmtlink", "Dev", 
				 "Rmtdev", "Pipe", "NamedPipe", "Pdev", "Pfs",
				 "XtraFile", "Unknown"};

    if ((fileType < 0) || (fileType >= sizeof(names)/sizeof(names[0]))) {
	fileType = sizeof(names)/sizeof(names[0])-1;
    }
    return names[fileType];
}



/*
 *----------------------------------------------------------------------
 *
 * UpdateDescLinkCount --
 *
 *	Update descriptor link count on a descriptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UpdateDescLinkCount(opType, fileNumber, linkCount)
    enum DescOpType opType;	/* OP_ABS or OP_REL. */
    int		   fileNumber; /* Directory entry being affected. */
    int 	   linkCount;    /* New link count value or change. */
{
    LfsFile	*filePtr;
    LfsFileDescriptor	*descPtr;
    ReturnStatus status;


    status = LfsFileOpen(lfsPtr, fileNumber, &filePtr);
    if (status != SUCCESS) {
	return status;
    }
    descPtr = LfsFileFetchDesc(filePtr);
    if (opType == OP_ABS) {
	descPtr->common.numLinks = linkCount;
    } else if (opType == OP_REL) {
	descPtr->common.numLinks += linkCount;
    } else {
	panic("Unknown op type in UpdateDescLinkCount\n");
    }
    status = LfsFileStoreDesc(filePtr);
    LfsFileClose(filePtr);
    return status;
}

typedef struct LostFile {
    List_Links	links;	/* Link pointers for list. MUST BE FIRST. */
    int	dirFileNumber; /* Directory this file was to be created in. */
    int	fileNumber;    /* File number being created. */
    char fileName[FS_MAX_NAME_LENGTH+1];  
			/* File name length. */
} LostFile;

#define	ALLOC_SIZE(nameLength)	\
	(sizeof(LostFile)-(FS_MAX_NAME_LENGTH+1)+(nameLength)+1)

static Boolean lostFileListInit = FALSE;

List_Links lostFileListHdr;
List_Links addToLost_FoundListHdr;
#define	lostFileList	(&lostFileListHdr)
#define	addToLost_FoundList	(&addToLost_FoundListHdr)

/*
 *----------------------------------------------------------------------
 *
 * RecordLostCreate --
 *
 *	Record the loss on a create operation so we have the <name, inode>
 *	pairing for building up the lost+found directory.
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
RecordLostCreate(dirFileNumber,dirEntryPtr)
    int	dirFileNumber;	/* Directory of create. */
    Fslcl_DirEntry	*dirEntryPtr; /* Entry lost. */
{
    int	fileNumber = dirEntryPtr->fileNumber;
    LostFile	*lostFilePtr;

    if (!lostFileListInit) {
	List_Init(lostFileList);
	List_Init(addToLost_FoundList);
	lostFileListInit = TRUE;
    }
    lostFilePtr = (LostFile *) malloc(ALLOC_SIZE(dirEntryPtr->nameLength));
    List_InitElement((List_Links *) lostFilePtr);
    lostFilePtr->dirFileNumber = dirFileNumber;
    lostFilePtr->fileNumber = fileNumber;
    bcopy(dirEntryPtr->fileName,lostFilePtr->fileName, dirEntryPtr->nameLength);
    lostFilePtr->fileName[dirEntryPtr->nameLength] = '\000';

    List_Insert((List_Links *) lostFilePtr, LIST_ATREAR(lostFileList));

#ifdef notdef
    if (verboseFlag) {
	printf("Lost create in directory %d of file \"%s\" inode %d\n",
			dirFileNumber, lostFilePtr->fileName, fileNumber);
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * CreateLostDirectory --
 *
 *	Create a directory that was lost so that any files created
 *	will be recoverable.
 *
 * Results:
 *	SUCCESS if directory was created.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
CreateLostDirectory(fileNumber) 
    int	fileNumber;	/* File number of directory. */
{
    LostFile	*lostFilePtr;
    Boolean	foundIt = FALSE;
    ReturnStatus status;

    if (!lostFileListInit) {
	List_Init(lostFileList);
	List_Init(addToLost_FoundList);
	lostFileListInit = TRUE;
    }
    /*
     * Check to see if file is in our list of lost creats.
     */
    for (lostFilePtr = (LostFile *) List_Last(lostFileList); 
             !List_IsAtEnd(lostFileList,(List_Links *) lostFilePtr); 
             lostFilePtr = (LostFile *)List_Prev((List_Links *) lostFilePtr)) {
	if (lostFilePtr->fileNumber == fileNumber) {
	    foundIt = TRUE;
	    break;
	}
    }
    if (verboseFlag) {
	printf("Creating lost directory %d\n", fileNumber);
    }

    status = MakeEmptyDirectory(fileNumber, -1);
    if (status != SUCCESS) {
	return status;
    }
    if (!foundIt) {
        lostFilePtr = (LostFile *) malloc(ALLOC_SIZE(8));
	List_InitElement((List_Links *) lostFilePtr);
	lostFilePtr->dirFileNumber = fileNumber;
	sprintf(lostFilePtr->fileName, "%d", fileNumber);
	List_Insert((List_Links *) lostFilePtr, LIST_ATREAR(lostFileList));
    }
    List_Move((List_Links *)lostFilePtr, LIST_ATREAR(addToLost_FoundList));

    return status;
}



/*
 *----------------------------------------------------------------------
 *
 * SetParentDirectory --
 *
 *	Update a directory to reside in the specified directory fileNumber; 
 *
 * Results:
 *	SUCCESS if update worked.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
SetParentDirectory(dirFileNumber, parentFileNumber) 
    int	dirFileNumber;	/* File number of directory. */
    int	parentFileNumber;	/* Parent file number. */
{
    ReturnStatus status;
    LfsFile	*dirFilePtr;
    LfsFileBlock *dirBlockPtr;
    char	*dirBlock;
    Fslcl_DirEntry *dirEntryPtr;

    status = LfsFileOpen(lfsPtr, dirFileNumber, &dirFilePtr);
    if (status != SUCCESS) {
	return status;
    }
    status = LfsFileBlockFetch(dirFilePtr, 0, &dirBlockPtr);
    if (status != SUCCESS) {
	return status;
    }
    dirBlock = LfsFileBlockMem(dirBlockPtr);
    dirEntryPtr = (Fslcl_DirEntry *)dirBlock;
    dirEntryPtr = (Fslcl_DirEntry *) (dirBlock + dirEntryPtr->recordLength);
    if ((dirEntryPtr->nameLength != 2) ||
	(dirEntryPtr->fileName[0] != '.') || 
	(dirEntryPtr->fileName[1] != '.')) {
	panic("Corrupted directory %d\n", dirFileNumber);
    }
    dirEntryPtr->fileNumber = parentFileNumber;
    LfsFileStoreBlock(dirBlockPtr, -1);
    LfsFileClose(dirFilePtr);

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * MakeEmptyDirectory --
 *
 *	Initialize an empty directory.
 *
 * Results:
 *	SUCCESS if operation worked.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MakeEmptyDirectory(fileNumber, parentFileNumber)
    int	fileNumber;	/* File number of directory. */
    int	parentFileNumber;	/* File number of parent. */
{
    LfsDescMapEntry *descMapEntryPtr;
    LfsFileDescriptor *descPtr;
    Fsdm_FileDescriptor	*fileDescPtr;
    LfsFile	*dirFilePtr;
    LfsFileBlock *dirBlockPtr;
    Fslcl_DirEntry *dirEntryPtr;
    char		*dirBlock;
    int		index;
    ReturnStatus	status;

    descPtr = (LfsFileDescriptor *)malloc(sizeof(LfsFileDescriptor));

    fileDescPtr = &descPtr->common;
    fileDescPtr->magic = FSDM_FD_MAGIC;
    fileDescPtr->flags = FSDM_FD_ALLOC|FSDM_FD_DIRTY;
    fileDescPtr->fileType = FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSLCL_DIR_BLOCK_SIZE-1;
    fileDescPtr->lastByteXtra = 0;
    fileDescPtr->firstByte = -1;
    fileDescPtr->userType = FS_USER_TYPE_UNDEFINED;
    fileDescPtr->numLinks = 1;

    /*
     * Clear out device info.  It is set up properly by the make-device routine.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  These times should come from the client.
     */
    fileDescPtr->createTime = time(0);
    fileDescPtr->accessTime = fileDescPtr->createTime;
    fileDescPtr->descModifyTime = fileDescPtr->createTime;
    fileDescPtr->dataModifyTime = fileDescPtr->createTime;

    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 0;
    fileDescPtr->version = LfsGetCurrentTimestamp(lfsPtr);

    descPtr->fileNumber = fileNumber;

    status = LfsFileOpenDesc(lfsPtr, descPtr, &dirFilePtr);
    if (status == SUCCESS) {
	status = LfsFileBlockAlloc(dirFilePtr, 0, FSLCL_DIR_BLOCK_SIZE, 
				&dirBlockPtr);
    }
    if (status != SUCCESS) {
	free((char *) descPtr);
	return status;
    }
    dirBlock = LfsFileBlockMem(dirBlockPtr);
    dirEntryPtr = (Fslcl_DirEntry *)dirBlock;
    dirEntryPtr->fileNumber = fileNumber;
    dirEntryPtr->nameLength = strlen(".");
    dirEntryPtr->recordLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
    (void)strcpy(dirEntryPtr->fileName, ".");
    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
    dirEntryPtr->fileNumber = parentFileNumber;
    dirEntryPtr->nameLength = strlen("..");
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - Fslcl_DirRecLength(1);
    (void)strcpy(dirEntryPtr->fileName, "..");

    LfsFileStoreDesc(dirFilePtr);
    LfsFileStoreBlock(dirBlockPtr, -1);
    LfsFileClose(dirFilePtr);

    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (!(descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	lfsPtr->descMap.checkPoint.numAllocDesc++;
    }
    descMapEntryPtr->flags = LFS_DESC_MAP_ALLOCED;
    descMapEntryPtr->truncVersion++;
    descMapEntryPtr->blockAddress = FSDM_NIL_INDEX;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateLost_Found --
 *
 *	Create and upate the lost and Found directory.
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
UpdateLost_Found(lfsPtr) 
    Lfs	*lfsPtr;
{
    LfsFile	*dirFilePtr, *filePtr;
    ReturnStatus	status;
    ClientData	clientData;
    LostFile	*lostFilePtr;
    char	nameBuffer[FS_MAX_NAME_LENGTH+1];
    int		version, fileNumber, dirOffset;

    if (!lostFileListInit) {
	List_Init(lostFileList);
	List_Init(addToLost_FoundList);
	lostFileListInit = TRUE;
    }
    dirFilePtr = FindLost_FoundDir(lfsPtr);

    LIST_FORALL (addToLost_FoundList, (List_Links *) lostFilePtr) {

	stats.filesToLostFound++;
	sprintf(nameBuffer, "%s.%d", lostFilePtr->fileName, 
					lostFilePtr->fileNumber);
	version = 1;
	do {
	    status = LfsFileNameLookup(dirFilePtr, nameBuffer,
				&fileNumber,&dirOffset);
	    if (status == SUCCESS) {
		sprintf(nameBuffer, "%s.%d.%d", lostFilePtr->fileName, 
					lostFilePtr->fileNumber, version);
		version++;
	    }
	} while (status == SUCCESS);
	status = LfsFileOpen(lfsPtr, lostFilePtr->fileNumber, &filePtr);
	if (status != SUCCESS) {
	    panic("Can't open created lost directory.\n");
	}
	status = LfsFileAddToDirectory(dirFilePtr, filePtr, nameBuffer);
	if (status != SUCCESS) {
	    panic("Can't add name to lost+found directory.\n");
	}
	LfsFileClose(filePtr);
    }

     clientData = (ClientData) NIL;
     while (ScanUnrefDesc(&clientData, &fileNumber)) {
	 status = LfsFileOpen(lfsPtr, fileNumber, &filePtr);
	 if (status == FS_FILE_NOT_FOUND) {
		/*
		 * File has been deleted. Skip it.
		 */
		continue;
	 }
	 if (status != SUCCESS) {
		panic("Can't open unreferenced file.\n");
	 }
	 sprintf(nameBuffer, "unref.%d", fileNumber);
	 version = 1;
	 do {
	    status = LfsFileNameLookup(dirFilePtr, nameBuffer,
				&fileNumber,&dirOffset);
	    if (status == SUCCESS) {
		sprintf(nameBuffer, "unref,%d.%d", fileNumber, version); 
		version++;
	    }
	} while (status == SUCCESS);
	 status = LfsFileAddToDirectory(dirFilePtr, filePtr, nameBuffer);
	 if (status != SUCCESS) {
	     panic("Can't add name to lost+found directory.\n");
	 }
	 LfsFileClose(filePtr);
    }
    ScanUnrefDescEnd(&clientData);
    LfsFileClose(dirFilePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * LfsFileAddToDirectory --
 *
 *	Add the specified file to the specified directory under the
 *	specified name.
 *
 * Results:
 *	SUCCESS if everything worked.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus 
LfsFileAddToDirectory(dirFilePtr, filePtr, name)
    LfsFile *dirFilePtr;
    LfsFile *filePtr;
    char *name;
{
    Fslcl_DirEntry	dirEntry;
    LfsFileDescriptor	*descPtr, *dirDescPtr;
    ReturnStatus	status;


    dirDescPtr = LfsFileFetchDesc(dirFilePtr);
    descPtr = LfsFileFetchDesc(filePtr);

    dirEntry.fileNumber = descPtr->fileNumber;
    dirEntry.nameLength = strlen(name);
    strcpy(dirEntry.fileName, name);
    dirEntry.recordLength = Fslcl_DirRecLength(dirEntry.nameLength);

    status = AddEntryToDirectory(dirDescPtr->fileNumber, -1, &dirEntry, 
			(descPtr->common.fileType == FS_DIRECTORY), FALSE);
    if (status != SUCCESS) {
	panic("Can't add file to directory.\n");
    }
    descPtr->common.numLinks++;
    LfsFileStoreDesc(filePtr);
    if (descPtr->common.fileType == FS_DIRECTORY) {
	SetParentDirectory(descPtr->fileNumber, dirDescPtr->fileNumber);
    }
    return status;
}

LfsFile *
FindLost_FoundDir(lfsPtr)
    Lfs	*lfsPtr;
{
    LfsFile	*rootFilePtr, *dirFilePtr;
    int	fileNumber;
    ReturnStatus	status;
    int			dirOffset;

    status = LfsFileOpen(lfsPtr, FSDM_ROOT_FILE_NUMBER, &rootFilePtr);
    if (status != SUCCESS) {
	panic("Can't open root file system.\n");
    }
    status = LfsFileNameLookup(rootFilePtr, "lost+found", &fileNumber,
			&dirOffset);
    if (status != SUCCESS) {
	if (status == FS_FILE_NOT_FOUND) {
	    status = LfsGetNewFileNumber(lfsPtr, FSDM_ROOT_FILE_NUMBER,
				&fileNumber);
	    if (status != SUCCESS) {
		panic("Can't allocated file number for lost+found directory\n");
	    }
	    status = MakeEmptyDirectory(fileNumber, FSDM_ROOT_FILE_NUMBER);
	    if (verboseFlag) {
		printf("Can't find lost+found dir - creating at %d\n", fileNumber);
	    }
	    if (status != SUCCESS) {
		panic("Can't make new lost+found directory\n");
	    }
	} else {
	    panic("Can't open lost+found directory\n");
	}
	status = LfsFileOpen(lfsPtr, fileNumber, &dirFilePtr);
	if (status != SUCCESS) {
		panic("Can't open lost+found directory");
	}
	status = LfsFileAddToDirectory(rootFilePtr, dirFilePtr, "lost+found");
	if (status != SUCCESS) {
	    panic("Can't add file to directory");
	}
    } else {
	status = LfsFileOpen(lfsPtr, fileNumber, &dirFilePtr);
	if (status != SUCCESS) {
	    panic("Can't open lost+found file.\n");
	}
    }
    LfsFileClose(rootFilePtr);
    return dirFilePtr;
}




/*
 *----------------------------------------------------------------------
 *
 * RecoveryFile --
 *
 *	Roll forward changes to the descriptor map.
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
RecoveryFile(fileNumber, address, descPtr)
    int		fileNumber; 	/* File number to modified address of. */
    int		address;	/* Disk address of descriptor. */
    LfsFileDescriptor *descPtr; /* New file descriptor at address. */
{
    LfsDescMapEntry	*descMapEntryPtr;
    Boolean deallocate, wasAllocated;
    LfsFile		*oldFilePtr;
    LfsFile		*newFilePtr;
    ReturnStatus	status;

    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (descMapEntryPtr == (LfsDescMapEntry *) NIL) {
	panic("Bad file number %d in call to RecoveryFile\n", fileNumber);
	return;
    }
    oldFilePtr = newFilePtr = (LfsFile *) NIL;
    deallocate = (address == FSDM_NIL_INDEX);
    wasAllocated = (descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED);

    if (wasAllocated) {
	status = LfsFileOpen(lfsPtr, fileNumber, &oldFilePtr);
	if (status != SUCCESS) {
	    panic("RecoveryFile: Can't fetch old descriptor.\n");
	}
	LfsSegUsageAdjustBytes(lfsPtr,descMapEntryPtr->blockAddress,
				-sizeof(LfsFileDescriptor));
    }
    if (verboseFlag) {
	if (wasAllocated) {
	    printf("RecoveryFile %d was at %d ", fileNumber,	
				descMapEntryPtr->blockAddress);
	} else {
	    printf("RecoveryFile %d was not allocated ", fileNumber);
	}
	if (deallocate) {
	    printf("is now deleted at %d\n", address);
	} else {
	    printf("is now at %d\n", address);
	}
    }



    if (deallocate) {
	if (wasAllocated) {
	    descMapEntryPtr->flags ^= LFS_DESC_MAP_ALLOCED;
	    lfsPtr->descMap.checkPoint.numAllocDesc--;
	}
    } else {
	status = LfsFileOpenDesc(lfsPtr, descPtr, &newFilePtr);
	if (status != SUCCESS) {
	    panic("RecoveryFile: Can't fetch new file.\n");
	}
	if (!wasAllocated) {
	    descMapEntryPtr->flags |= LFS_DESC_MAP_ALLOCED;
	    lfsPtr->descMap.checkPoint.numAllocDesc++;
	} 
	LfsSegUsageAdjustBytes(lfsPtr,address, sizeof(LfsFileDescriptor));
    }
    descMapEntryPtr->blockAddress = address;
    LfsDescMapEntryModified(lfsPtr, fileNumber);
    RecordBlockUsageChange(fileNumber, oldFilePtr, newFilePtr);

    if (oldFilePtr != (LfsFile *) NIL) {
	LfsFileClose(oldFilePtr);
    }
    if ((newFilePtr != (LfsFile *) NIL) && (newFilePtr != oldFilePtr)) {
	LfsFileClose(newFilePtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DescExists --
 *
 *	Determine if a file descriptor exists for the specified file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
DescExists(fileNumber)
    int	fileNumber;	/* File number to check for descriptor for. */
{
    LfsDescMapEntry	*descMapEntryPtr;

    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (descMapEntryPtr == (LfsDescMapEntry *) NIL) {
	/*
	 * Didn't exist at checkpoint. Check is see if we found it
	 * in recovery log.
	 */
	return FALSE;
    }

    if (!(descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return 	FALSE;
    }
    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * DirBlockStatus --
 *
 *	Compute the status of the directory block refered to a log 
 *	entry.
 *
 * Results:
 *	FORWARD, BACKWARD, or UNKNOWN
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

enum LogStatus
DirBlockStatus(dirFileNumber, dirOffset, startAddr, addr)
    int	dirFileNumber;	/* Directory being modified. */
    int	dirOffset;	/* Offset into directory being modified. */
    int	startAddr;	/* Disk Address of log start entry. */
    int	addr;		/* Disk Address of log end entry. */
{
    int	blockAddr;
    ReturnStatus	status;
    LfsFile		*dirFilePtr;
    int			blocksize;


    status = LfsFileOpen(lfsPtr, dirFileNumber, &dirFilePtr);
    if (status != SUCCESS) {
	if (status == FS_FILE_NOT_FOUND) {
	    return BACKWARD;
	}
	panic("Can't find directory %d\n", dirFileNumber);
	return UNKNOWN;
    }
    blockAddr = LfsFileBlockAddr(dirFilePtr, dirOffset/FS_BLOCK_SIZE, &blocksize);
    LfsFileClose(dirFilePtr);
    if (blockAddr == FSDM_NIL_INDEX) {
	/*
	 * Block doesn't exist, must be backward.
	 */
	return BACKWARD;
    }


    if (AddrOlderThan(blockAddr, addr)) {
	return FORWARD;
    } 
    if (startAddr == addr) {
	return BACKWARD;
    } 

    if (AddrOlderThan(blockAddr, startAddr)) {
	return UNKNOWN;
    } else {
	return BACKWARD;
    }

}


/*
 *----------------------------------------------------------------------
 *
 * DescStatus --
 *
 *	Compute the status of the descriptor refered to a log 
 *	entry.
 *
 * Results:
 *	FORWARD, BACKWARD, or UNKNOWN
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

enum LogStatus
DescStatus(fileNumber, startAddr, addr)
    int	fileNumber;	/* Descriptor being looked for. */
    int	startAddr;	/* Disk Address of log start entry. */
    int	addr;		
{
    int	blockAddr;
    LfsDescMapEntry	*descMapEntryPtr;
    Boolean found;

    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (descMapEntryPtr == (LfsDescMapEntry *) NIL) {
	panic("Bad file number %d in log entry\n", fileNumber);
	return BACKWARD;
    }
    if (!(descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	LfsFileDescriptor	*descPtr;
	found = FindNewDesc(fileNumber, &blockAddr, &descPtr);
	if (found) {
	    /*
	     * This was a file that was deleted during the trace,
	     * mark it as forward.
	     */
	    return FORWARD;
	}  
	return BACKWARD;
    } else { 
	blockAddr = descMapEntryPtr->blockAddress;
    }

    if (AddrOlderThan(blockAddr, addr)) {
	return FORWARD;
    } 
    if (startAddr == addr) {
	return BACKWARD;
    } 
    if (AddrOlderThan(blockAddr, startAddr)) {
	return UNKNOWN;
    } else {
	return BACKWARD;
    }

}



/*
 *----------------------------------------------------------------------
 *
 * RecovFileLayoutSummary --
 *
 *	Check the segment summary regions for the file layout code.
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
RecovFileLayoutSummary(lfsPtr, pass, segPtr, startAddr, offset, 
			segSummaryHdrPtr) 
    Lfs	*lfsPtr;	/* File system description. */
    enum Pass pass;	/* Pass number of recovery. */
    LfsSeg *segPtr;	/* Segment being examined. */
    int startAddr;   /* Starting address being examined. */
    int offset;		/* Offset into segment being examined. */
    LfsSegSummaryHdr *segSummaryHdrPtr; /* Summary header pointer */
{
    char *summaryPtr, *limitPtr;
    int descBlocks;
    int startAddress;


    startAddress = startAddr;
    descBlocks = (lfsPtr->superBlock.fileLayout.descPerBlock * 
			sizeof(LfsFileDescriptor))/blockSize;
    summaryPtr = (char *) (segSummaryHdrPtr + 1);
    limitPtr = summaryPtr + segSummaryHdrPtr->lengthInBytes - 
			sizeof(LfsSegSummaryHdr);
    /*
     * Scan thru summary region.
     */
    while (summaryPtr < limitPtr) {
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    int		fileNumber, slot, descPerBlock;
	    LfsFileDescriptor	*descPtr;

	    startAddress -= descBlocks;
	    stats.descBlocks++;
	    if (pass == PASS1) { 
		/*
		 * During pass1 of recovery we simply record the location
		 * of each the descriptors we find.
		 */
		descPtr = (LfsFileDescriptor *) LfsSegFetchBlock(segPtr, offset, 
						descBlocks*blockSize);
		descPerBlock = lfsPtr->superBlock.fileLayout.descPerBlock;
		for (slot = 0; slot < descPerBlock;  slot++) {
		    /*
		     * The descriptor block is terminated by an unallocated
		     * descriptor.
		     */
		    if (!(descPtr[slot].common.flags & FSDM_FD_ALLOC)) {
			break;
		    }

		    fileNumber = descPtr[slot].fileNumber;
		    if ((fileNumber < 0) || 
			(fileNumber >= lfsPtr->superBlock.descMap.maxDesc)) {
		       fprintf(stderr,"%s:RecovFileLayoutSummary: bad file number %d in desc block at %d\n", deviceName, fileNumber, startAddress);
		       continue;
		    }
		    stats.numDesc++;
		    if (showLog) { 
			printf("Addr %d Descriptor %s %d len %d\n",
			    startAddress,
			    FileTypeToString(descPtr[slot].common.fileType),
			    descPtr[slot].fileNumber,
			    descPtr[slot].common.lastByte+1);
		    }
		    if (recreateDirEntries &&
		        (descPtr[slot].common.fileType == FS_DIRECTORY)) {
			continue;
		    }
		    /*
		     * Record this descriptor location. 
		     */
		    RecordNewDesc(fileNumber, startAddress, descPtr+slot);
		 }
		 LfsSegReleaseBlock(segPtr, (char *) descPtr);
	    }
	    offset += descBlocks;
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    break;
	}
	case LFS_FILE_LAYOUT_DATA: {
	    int	*blockArray;
	    int			  i;
	    LfsFileLayoutSummary *fileSumPtr;
	    int firstBlock;
	    /*
	     * Currently we don't do much with data blocks by themselves.
	     * We can only hope that the descriptor also made it out and
	     * points to them.
	     */
	    fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	    if ((fileSumPtr->fileNumber < 0) || 
		(fileSumPtr->fileNumber >= lfsPtr->superBlock.descMap.maxDesc)) {
	       fprintf(stderr,"%s:RecovFileLayoutSummary: bad file number %d at %d\n", deviceName, fileSumPtr->fileNumber, startAddress);
	       goto out;
	    }
	    blockArray = (int *)(summaryPtr + sizeof(LfsFileLayoutSummary));
	    if (pass == PASS1) { 
		if (showLog) { 
		    printf("Addr %d File %d blocks %d version %d dataBlocks %d\n",
		    startAddress - fileSumPtr->numBlocks,
		    fileSumPtr->fileNumber, fileSumPtr->numBlocks,
		    fileSumPtr->truncVersion,
		    fileSumPtr->numDataBlocks);
		    /*
		     * For each block ... 
		     */
		    firstBlock = fileSumPtr->numBlocks -
			 (fileSumPtr->numDataBlocks-1) *
						FS_BLOCK_SIZE/blockSize;
		    stats.numFileBlocks += fileSumPtr->numDataBlocks;
		    for (i = 0; i < fileSumPtr->numDataBlocks; i++) {
			int addr;
    
			addr = startAddress - i*FS_BLOCK_SIZE/blockSize - 
					firstBlock;
			    printf("Addr %d File %d block %d\n",
				addr, fileSumPtr->fileNumber, blockArray[i]);
		    }
		}
	    } else {
		CheckVersionTruncNum(startAddress, fileSumPtr->fileNumber, 
				fileSumPtr->truncVersion,
				 fileSumPtr->numBlocks,
				fileSumPtr->numDataBlocks, blockArray);

	    }
	    out:
	    startAddress = startAddress - fileSumPtr->numBlocks;
	    offset += fileSumPtr->numBlocks;
	    summaryPtr += sizeof(LfsFileLayoutSummary) + 
				fileSumPtr->numDataBlocks * sizeof(int); 
	    break;
	  }

	case LFS_FILE_LAYOUT_DIR_LOG: {
	    LfsFileLayoutLog	*logSumPtr;
	    int			numBlocks;
	    int			i, addr, blocks;
	    LfsDirOpLogBlockHdr *hdrPtr = (LfsDirOpLogBlockHdr *) NIL;

	    /* 
	     * Directory log info is needed during both passes.  
	     */
	     logSumPtr = (LfsFileLayoutLog *) summaryPtr;
	     summaryPtr = summaryPtr + sizeof(LfsFileLayoutLog);
	     numBlocks = logSumPtr->numBlocks;
	     addr = startAddress;
	     for (i = 0; i < logSumPtr->numDataBlocks; i++) {
		if (numBlocks > FS_BLOCK_SIZE/blockSize) {
		    blocks = FS_BLOCK_SIZE/blockSize;
		} else {
		    blocks = numBlocks;
		}

	        hdrPtr = (LfsDirOpLogBlockHdr *)
		     LfsSegFetchBlock(segPtr, offset, blocks*blockSize);
		addr -= blocks;
		offset += blocks;
		numBlocks -= blocks;
		if (hdrPtr->magic != LFS_DIROP_LOG_MAGIC) {
		    fprintf(stderr,"Bad dir op log magic number.\n");
		}
		if (pass == PASS1) { 
		    stats.numDirLogBlocks += blocks;
		    if (showLog) { 
			ShowDirLogBlock(hdrPtr, addr);
		    }
		}

		RecovDirLogBlock(hdrPtr, addr, pass);
		/*
		 * Could save log in memory for second pass.
		 */
		LfsSegReleaseBlock(segPtr, (char *) hdrPtr);
	    }

	    startAddress = startAddress - logSumPtr->numBlocks;
	    break;
	}
	case LFS_FILE_LAYOUT_DBL_INDIRECT: 
	case LFS_FILE_LAYOUT_INDIRECT: 
	default: {
	    panic("Unknown type");
	}
      }
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CheckVersionTruncNum --
 *
 *	Record the a truncate version number at a specified address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
CheckVersionTruncNum(startAddress, fileNumber, truncVersion, numBlocks,
			numDataBlocks, blockArray)
    int	 startAddress;
    int	fileNumber;	/* File number of descriptor. */
    int	truncVersion;	/* Truncate version number. */
    int	numBlocks;	/* Fs block. */
    int numDataBlocks;	/* Number of blocks in list.*/
    int	*blockArray;	/* List of blocks. */
{

    LfsDescMapEntry	*descMapEntryPtr;
    int		addr;
    int		firstBlock;
    LfsFile	*filePtr;
    int		size, i;
    ReturnStatus status;

    descMapEntryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (descMapEntryPtr == (LfsDescMapEntry *) NIL) {
	panic("Bad file number %d in call to RecordVersionTrunc\n", fileNumber);
	return;
    }
    if (!(descMapEntryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return;
    }
    if (descMapEntryPtr->truncVersion >= truncVersion) {
	return;
    }
    if (AddrOlderThan(startAddress, descMapEntryPtr->blockAddress)) {
	/*
	 * Blocks are before the descriptor in the log.  Assume that
	 * the descriptor includes these blocks. 
	 */
	descMapEntryPtr->truncVersion = truncVersion;
	LfsDescMapEntryModified(lfsPtr, fileNumber);
	return;
    }
    /*
     * Could be newer than. 
     */
     if (LfsDiskAddrToSegmentNum(lfsPtr, startAddress) !=
	 LfsDiskAddrToSegmentNum(lfsPtr, descMapEntryPtr->blockAddress)) {
	/*
	 * If it is new and in a different segment it can't be part
	 * of this file.
	 */
	 return;
    }
    /*
     * Same segment. Check the blocks to see if any are in file.
     */
    status = LfsFileOpen(lfsPtr, fileNumber, &filePtr);
    if (status != SUCCESS) {
	return;
    }
    firstBlock = numBlocks - ( numDataBlocks-1 ) *
				FS_BLOCK_SIZE/blockSize;
    for (i = 0; i < numDataBlocks; i++) {

	addr = startAddress - i*FS_BLOCK_SIZE/blockSize - 
			firstBlock;
	if (LfsFileBlockAddr(filePtr, blockArray[i], &size) == addr) {
	    descMapEntryPtr->truncVersion = truncVersion;
	    LfsDescMapEntryModified(lfsPtr, fileNumber);
	    LfsFileClose(filePtr);
	    return;
	}
    }
    LfsFileClose(filePtr);
}

