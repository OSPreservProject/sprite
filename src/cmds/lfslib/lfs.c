/* 
 * lfs.c --
 *
 *	Routines for accessing LFS file systems data structures from
 *	an user level program.
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
#include <time.h>
#include <hash.h>
#include <list.h>

extern int open();

/*
 *----------------------------------------------------------------------
 *
 * LfsLoadFileSystem --
 *
 *	Load an LFS file system into memory.
 *
 * Results:
 *	A Lfs structure, NULL if the load didn't complete.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Lfs *
LfsLoadFileSystem(programName, deviceName, blockSize,  superBlockOffset, flags)
    char *programName;	/* Program name. */
    char *deviceName;	/* Device name to open. */
    int	 blockSize;	/* Block size of file system. */
    int	 superBlockOffset; /* Offset of the super block. */
    int	 flags;		/* Flags for open. */
{
    Lfs	*lfsPtr;
    int	 diskFd;
    int	   maxCheckPointSize;
    LfsCheckPointHdr	checkPointHdr[2], *checkPointHdrPtr;
    char		*checkPointPtr, *trailerPtr;
    LfsCheckPointRegion *regionPtr;
    LfsCheckPointTrailer *trailPtr;
    char		buffer[LFS_SUPER_BLOCK_SIZE];
    int			choosenOne;
    Boolean		error;

    diskFd = open(deviceName, flags, 0);
    if (diskFd < 0) {
	fprintf(stderr,"%s:", programName);
	perror("opening device");
	return (Lfs *) NULL;
    }
    lfsPtr = (Lfs *) calloc(1, sizeof(Lfs));
    if (lfsPtr == (Lfs *) NULL) {
	fprintf(stderr,"%s: Out of memory\n", programName);
	goto	cleanupAndReturnError;
    }
    lfsPtr->diskFd = diskFd;
    lfsPtr->diskCache = (ClientData) NULL;
    lfsPtr->superBlockOffset = superBlockOffset;
    lfsPtr->deviceName = deviceName;
    lfsPtr->programName = programName;
    /*
     * Set to block size so LfsDiskRead will work.
     */
    lfsPtr->superBlock.hdr.blockSize = blockSize;

    /*
     * Read in the super block header. We put much trust in the magic number
     * of the file system.
     */
    if (LfsDiskRead(lfsPtr, superBlockOffset, LFS_SUPER_BLOCK_SIZE, 
		(char *)buffer) != LFS_SUPER_BLOCK_SIZE) {
	fprintf(stderr,"%s:Can't read superblock.\n", deviceName);
	goto	cleanupAndReturnError;
    }
    bcopy(buffer, (char *) &lfsPtr->superBlock, sizeof(lfsPtr->superBlock));
    if (lfsPtr->superBlock.hdr.magic != LFS_SUPER_BLOCK_MAGIC) {
	fprintf(stderr,"%s:Bad magic number for filesystem\n", deviceName);
	goto	cleanupAndReturnError;
    }

    /*
     * Examine the two checkpoint areas to locate the checkpoint area with the
     * newest timestamp.
     */
    if (LfsDiskRead(lfsPtr, lfsPtr->superBlock.hdr.checkPointOffset[0],
		sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+0)) != 
	sizeof(LfsCheckPointHdr)) {
	fprintf(stderr,"%s:Can't read checkPointHeader 0.\n", deviceName);
	checkPointHdr[0].timestamp = 0;
    }
    if (LfsDiskRead(lfsPtr, lfsPtr->superBlock.hdr.checkPointOffset[1],
		sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+1))  != 
	sizeof(LfsCheckPointHdr)) {
	fprintf(stderr,"%s:Can't read checkPointHeader 1.\n", deviceName);
	checkPointHdr[1].timestamp = 0;
    }
    /*
     * Choose a checkpoint and read it in.
     */
    if (checkPointHdr[0].timestamp < checkPointHdr[1].timestamp) {
	 choosenOne = 1;
    } else {
	 choosenOne = 0;
    }
    lfsPtr->checkPoint.timestamp = checkPointHdr[choosenOne].timestamp;
    lfsPtr->checkPoint.nextArea = !choosenOne;

    maxCheckPointSize = lfsPtr->superBlock.hdr.maxCheckPointBlocks * 
				blockSize;
    checkPointPtr = malloc(maxCheckPointSize);
    if (checkPointPtr == (char *) NULL) {
	fprintf(stderr,"%s:Out of memory\n", programName);
	goto	cleanupAndReturnError;
    }
    lfsPtr->checkPoint.buffer = checkPointPtr;
    lfsPtr->checkPoint.maxSize = maxCheckPointSize;

    lfsPtr->checkPointHdrPtr = (LfsCheckPointHdr *) checkPointPtr;

    if (LfsDiskRead(lfsPtr, lfsPtr->superBlock.hdr.checkPointOffset[choosenOne],
		maxCheckPointSize, checkPointPtr) != maxCheckPointSize) {
	fprintf(stderr,"%s:Can't read checkPoint %d\n", deviceName, choosenOne);
	goto	cleanupAndReturnError;
    }


    checkPointHdrPtr = (LfsCheckPointHdr *) checkPointPtr;
    trailerPtr = checkPointPtr + checkPointHdrPtr->size - 
					sizeof(LfsCheckPointTrailer);
    trailPtr = (LfsCheckPointTrailer *) trailerPtr;
    if (trailPtr->timestamp != checkPointHdrPtr->timestamp) {
	fprintf(stderr,"%s:Header timestamp %d doesn't match trailer timestamp %d\n", deviceName, checkPointHdrPtr->timestamp, trailPtr->timestamp);
	goto	cleanupAndReturnError;
    }
    lfsPtr->name = checkPointHdrPtr->domainPrefix;
    bcopy(checkPointPtr + checkPointHdrPtr->size, (char *) &lfsPtr->stats,
		sizeof(lfsPtr->stats));
    checkPointPtr = checkPointPtr + sizeof(LfsCheckPointHdr);

    /*
     * Load the LFS metadata from the last checkpoint. 
     * For each checkpoint region .....
     */
    error = FALSE;
    while ((checkPointPtr < trailerPtr) && !error) { 
	regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	if (regionPtr->size == 0) {
	    break;
	}
	switch (regionPtr->type) {
	    case LFS_SEG_USAGE_MOD:
		error = LfsLoadUsageArray(lfsPtr, 
				     regionPtr->size - sizeof(*regionPtr),
				    (char *) (regionPtr+1));
		break;
	    case LFS_DESC_MAP_MOD:
		error = LfsLoadDescMap(lfsPtr, 
				   regionPtr->size - sizeof(*regionPtr),
				    (char *) (regionPtr+1));
		break;
	    case LFS_FILE_LAYOUT_MOD:
		if (regionPtr->size != sizeof(*regionPtr)) {
		    error = TRUE;
		    fprintf(stderr,"%s:Bad size %d for FILELAYOUT checkpoint\n",
				    deviceName,regionPtr->size);
		}
		break;
	    default: 
		fprintf(stderr,"%s:Unknown region type %d of size %d\n",
			    deviceName,
			    regionPtr->type, regionPtr->size);
		break;
	}
	checkPointPtr += regionPtr->size;
    }
    List_Init(&lfsPtr->fileLayout.activeFileListHdr);
    if (!error) {
	return lfsPtr;
    }
cleanupAndReturnError:
    return (Lfs *) NULL;

}


/*
 *----------------------------------------------------------------------
 *
 * LfsCheckPointFileSystem --
 *
 *	Checkpoint the state of a file system generated by a user  program.
 *
 * Results:
 *	SUCCESS if the file system was successfully checkpointed. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsCheckPointFileSystem(lfsPtr, flags)
    Lfs		*lfsPtr;	/* File system to be checkpointed. */
    int flags;		/* Flags for checkpoint. */
{
    LfsCheckPointHdr	*checkPointHdrPtr;
    int			size, blocks, bytes;
    LfsCheckPointTrailer *trailerPtr;
    int		diskAddr;
    ReturnStatus	status, count;
    char		*bufferPtr;

    bufferPtr = lfsPtr->checkPoint.buffer;
    checkPointHdrPtr = (LfsCheckPointHdr *) bufferPtr;


    status = LfsSegCheckPoint(lfsPtr, (char *)(checkPointHdrPtr+1),
				&size);
    if ((status != SUCCESS) || (size < 0)) {
	return status;
    }
    /*
     * Fill in check point header and trailer.
     */
    checkPointHdrPtr->timestamp = LfsGetCurrentTimestamp(lfsPtr);
    checkPointHdrPtr->size = size + sizeof(LfsCheckPointHdr) + 
			 sizeof(LfsCheckPointTrailer);
    checkPointHdrPtr->version = 1;
    checkPointHdrPtr->detachSeconds = time(0);
    trailerPtr = (LfsCheckPointTrailer *) 
		(bufferPtr + size + sizeof(LfsCheckPointHdr));
    trailerPtr->timestamp = checkPointHdrPtr->timestamp;
    trailerPtr->checkSum = 0;

    /*
     * Append the stats to the checkpoint regions.
     */
    bytes = checkPointHdrPtr->size + sizeof(Lfs_Stats);
    blocks = LfsBytesToBlocks(lfsPtr, bytes);
    bcopy ((char *) &lfsPtr->stats, (char *) (trailerPtr + 1), 
		sizeof(lfsPtr->stats));
    LfsOffsetToDiskAddr(
	lfsPtr->superBlock.hdr.checkPointOffset[lfsPtr->checkPoint.nextArea],
		&diskAddr);
    count = LfsDiskWrite(lfsPtr, diskAddr, 
	LfsBlocksToBytes(lfsPtr, blocks), (char *) checkPointHdrPtr);
    if (count == LfsBlocksToBytes(lfsPtr, blocks)) {
	/*
	 * Set the file system up to use the other checkpoint buffer next time.
	 */
	lfsPtr->checkPoint.nextArea = !lfsPtr->checkPoint.nextArea;
    } else {
	return FAILURE;
    }
    return status;
}

typedef struct DiskCache {
    List_Links	blockList; /* List of blocks cached. */
    Hash_Table	blockHashTable; /* Hash table for blocks. */
    int	bytesCached;	/* Number of bytes of data in disk cache. */
    int maxCacheBytes;  /* Maxnumbe of bytes that should be cached. */
} DiskCache;

typedef struct DiskBlock {
    List_Links links;	/* For list of blocks. MUST BE FIRST. */
    int	blockOffset;    /* Offset into device of block. */
    int blockSize;	/* Size of block in bytes. */
    /*
     * Data follows structure. 
     */
} DiskBlock;

static void DeleteCacheBlock _ARGS_((DiskCache *diskCachePtr, 
				DiskBlock *cacheBlockPtr));


/*
 *----------------------------------------------------------------------
 *
 * LfsDiskCache --
 *
 *	Set amount of data to cache in application
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
LfsDiskCache(lfsPtr, maxCacheBytes)
    Lfs	*lfsPtr;
    int	maxCacheBytes;
{
    DiskCache	*diskCachePtr = (DiskCache *) lfsPtr->diskCache;

    if (diskCachePtr == (DiskCache *) NULL) {
	diskCachePtr = (DiskCache *) calloc(1, sizeof(DiskCache));
	lfsPtr->diskCache = (ClientData) diskCachePtr;
	List_Init(&(diskCachePtr->blockList));
	Hash_InitTable(&(diskCachePtr->blockHashTable), 0, HASH_ONE_WORD_KEYS);
	diskCachePtr->bytesCached = 0;
    }
    diskCachePtr->maxCacheBytes = maxCacheBytes;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteCacheBlock --
 *
 *	Delete a block from the disk cache.
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
DeleteCacheBlock(diskCachePtr, cacheBlockPtr)
    DiskCache	*diskCachePtr;
    DiskBlock	*cacheBlockPtr;
{
    Hash_Entry *entryPtr;

    List_Remove((List_Links *) cacheBlockPtr);
    entryPtr = Hash_FindEntry(&(diskCachePtr->blockHashTable),
				cacheBlockPtr->blockOffset);
    Hash_DeleteEntry(&(diskCachePtr->blockHashTable), entryPtr);
    diskCachePtr->bytesCached -= cacheBlockPtr->blockSize;
    free((char *) cacheBlockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * LfsDiskCacheInvalidate --
 *
 *	Invalidate a block range in the disk cache
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
LfsDiskCacheInvalidate(lfsPtr, blockOffset, blockSize)
    Lfs	*lfsPtr;
    int	blockOffset;
    int	blockSize;
{
    DiskCache	*diskCachePtr = (DiskCache *) lfsPtr->diskCache;
    DiskBlock *cacheBlockPtr;
    Hash_Entry *entryPtr;
    int	endBlock;

    if (diskCachePtr == (DiskCache *) NULL) {
	return;
    }
    if (blockOffset == -1) {
	LIST_FORALL(&(diskCachePtr->blockList), (List_Links *) cacheBlockPtr) {
		DeleteCacheBlock(diskCachePtr, cacheBlockPtr);
	}
	return;
    }
    endBlock = blockOffset + 
		(blockSize + LfsBlockSize(lfsPtr) - 1)/LfsBlockSize(lfsPtr);
    for (; blockOffset < endBlock; blockOffset++) {
	entryPtr = Hash_FindEntry(&(diskCachePtr->blockHashTable), 
				(Address)blockOffset);
	if (entryPtr == (Hash_Entry *) NULL) {
	    continue;
	}
	DeleteCacheBlock(diskCachePtr, (DiskBlock *) Hash_GetValue(entryPtr));
    }
}



/*
 *----------------------------------------------------------------------
 *
 * LfsDiskRead --
 *
 *	Read data from disk.
 *
 * Results:
 *	The number of bytes returned.  -1 if error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
LfsDiskRead(lfsPtr, blockOffset, bufferSize, bufferPtr)
    Lfs	*lfsPtr;	/* File system. */
    int	blockOffset;	/* Block offset to start read. */
    char *bufferPtr;	/* Buffer to place data. */
    int	 bufferSize;	/* Size of buffer. */
{
    int	diskFd, blockSize, status, blocks;
    char *bufPtr;
    DiskBlock	*cacheBlockPtr;
    DiskCache	*diskCachePtr = (DiskCache *) lfsPtr->diskCache;


    cacheBlockPtr = (DiskBlock *) NULL;
    if (diskCachePtr != (DiskCache *) NULL) {
	Hash_Entry *entryPtr;
	Boolean new;
	int	newBytes;
	entryPtr = Hash_CreateEntry(&(diskCachePtr->blockHashTable),
					(Address) blockOffset, &new);
	if (!new) {
	    cacheBlockPtr = (DiskBlock *) Hash_GetValue(entryPtr);
	    if (cacheBlockPtr->blockSize >= bufferSize) {
		bcopy((char *) (cacheBlockPtr+1), bufferPtr, bufferSize);
		List_Move((List_Links *) cacheBlockPtr, 
				LIST_ATFRONT(&(diskCachePtr->blockList)));
		return bufferSize;
	    }
	    List_Remove((List_Links *) cacheBlockPtr);
	    cacheBlockPtr = (DiskBlock *) realloc((char *) cacheBlockPtr,
						sizeof(DiskBlock) + bufferSize);
	    newBytes = bufferSize - cacheBlockPtr->blockSize;
	    cacheBlockPtr->blockSize = bufferSize;
	} else { 
	    newBytes = bufferSize;
	    cacheBlockPtr = (DiskBlock *) malloc(sizeof(DiskBlock)+bufferSize);
	    List_InitElement(&(cacheBlockPtr->links));
	    cacheBlockPtr->blockOffset = blockOffset;
	    cacheBlockPtr->blockSize = bufferSize;
	}
	Hash_SetValue(entryPtr, (ClientData) cacheBlockPtr);
	while ((newBytes + diskCachePtr->bytesCached > 
			diskCachePtr->maxCacheBytes) && 
		!List_IsEmpty(&(diskCachePtr->blockList))) {
	    DeleteCacheBlock(diskCachePtr, 
			(DiskBlock *) List_Last(&(diskCachePtr->blockList)));
	}
	List_Insert((List_Links *) cacheBlockPtr, 
		    LIST_ATFRONT(&(diskCachePtr->blockList)));
	diskCachePtr->bytesCached += newBytes;
    }

    diskFd = lfsPtr->diskFd;
    blockSize = LfsBlockSize(lfsPtr);
    /*
     * Seek to the start of the blocks to read.
     */
    status = lseek(diskFd, blockOffset*blockSize, L_SET);
    if (status < 0) {
	fprintf(stderr,"%s:", lfsPtr->deviceName);
	perror("lseek");
	if (cacheBlockPtr != (DiskBlock *) NULL) {
	    DeleteCacheBlock(diskCachePtr,cacheBlockPtr);
	}
	return status;
    }
    /*
     * Read the blocks handling the case the a request that is not a 
     * multiple number of blocks by reading to a temp buffer and copying.
     */
    blocks = (bufferSize + blockSize-1)/blockSize;
    if (bufferSize != blocks * blockSize) { 
	bufPtr = malloc(blocks*blockSize);
    } else {
	bufPtr = bufferPtr;
    }
    status = read(diskFd, bufPtr, blocks*blockSize);
    if (status != blocks*blockSize) {
	if (status < 0) {
	    fprintf(stderr,"%s:", lfsPtr->deviceName);
	    perror("read device");
	    if (cacheBlockPtr != (DiskBlock *) NULL) {
		DeleteCacheBlock(diskCachePtr,cacheBlockPtr);
	    }
	    return status;
	}
	fprintf(stderr,"%s:Short read on device %d != %d\n", lfsPtr->deviceName,
		status, blocks*blockSize);
	if (cacheBlockPtr != (DiskBlock *) NULL) {
	    DeleteCacheBlock(diskCachePtr,cacheBlockPtr);
	    cacheBlockPtr =  (DiskBlock *) NULL;
	}
    } else {
	status = bufferSize;
    }
    if (bufPtr != bufferPtr) { 
	bcopy(bufPtr, bufferPtr, bufferSize);
	free(bufPtr);
    }
    if (cacheBlockPtr != (DiskBlock *) NULL) {
	bcopy(bufferPtr, (char *) (cacheBlockPtr+1), bufferSize);
    }
    lfsPtr->pstats.blocksReadDisk += blocks;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsDiskWrite --
 *
 *	Write data to disk.
 *
 * Results:
 *	The number of bytes returned.  -1 if error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
LfsDiskWrite(lfsPtr, blockOffset, bufferSize, bufferPtr)
    Lfs	*lfsPtr;	/* File system. */
    int	blockOffset;	/* Block offset to start read. */
    char *bufferPtr;	/* Buffer to place data. */
    int	 bufferSize;	/* Size of buffer. */
{
    int	status, blockSize, diskFd;

    LfsDiskCacheInvalidate(lfsPtr, blockOffset, bufferSize);
    blockSize = LfsBlockSize(lfsPtr);
    diskFd = lfsPtr->diskFd;

    /*
     * Seek to the start of the blocks to read.
     */
    status = lseek(diskFd, blockOffset*blockSize, L_SET);
    if (status < 0) {
	fprintf(stderr,"%s:", lfsPtr->deviceName);
	perror("lseek");
	return status;
    }
    status = write(diskFd, bufferPtr, bufferSize);
    if (status != bufferSize) {
	if (status < 0) {
	    fprintf(stderr,"%s:", lfsPtr->deviceName);
	    perror("write device");
	    return status;
	}
	fprintf(stderr,"%s:Short write on device %d != %d\n",lfsPtr->deviceName,
		status, bufferSize);
    } 
    lfsPtr->pstats.blocksWrittenDisk += bufferSize/512;
    return status;
}


