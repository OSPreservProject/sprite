/* 
 * fsAlloc.c --
 *
 *	Block and fragment allocation and truncation.  This code is specific
 *	to 4Kbyte blocks with 1Kbyte fragments.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "fsutil.h"
#include "fscache.h"
#include "fslcl.h"
#include "fsNameOps.h"
#include "fsio.h"
#include "spriteTime.h"
#include "devFsOpTable.h"
#include "fsStat.h"
#include "timer.h"
#include "rpc.h"
#include "proc.h"
#include "string.h"
#include "fsdm.h"
#include "fsdmInt.h"

/*
 * Each domain, which is a separate piece of disk, is locked
 * during allocation.
 */
#define LOCKPTR (&domainPtr->dataBlockLock)

/*
 * A table indexed by a 4 bit value is used by the allocation routine to 
 * quickly determine the location of 1, 2, and 3K fragments in a byte.  
 * The indices of the fragments start from 0.  If there is no such fragment in 
 * the byte then a -1 is used.
 */

int fragTable[16][FSDM_NUM_FRAG_SIZES] = {
/* 0000 */ {-1, -1, -1},
/* 0001 */ {-1, -1, 0},
/* 0010 */ {3, 0, -1},
/* 0011 */ {-1, 0, -1},
/* 0100 */ {0, 2, -1},
/* 0101 */ {0, -1, -1},
/* 0110 */ {0, -1, -1},
/* 0111 */ {0, -1, -1},
/* 1000 */ {-1, -1, 1},
/* 1001 */ {-1, 1, -1},
/* 1010 */ {1, -1, -1},
/* 1011 */ {1, -1, -1},
/* 1100 */ {-1, 2, -1},
/* 1101 */ {2, -1, -1},
/* 1110 */ {3, -1, -1},
/* 1111 */ {-1, -1, -1}
};

/*
 * Array to provide the ability to set and extract bits out of a bitmap byte.
 */

static unsigned char bitMasks[8] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

/*
 * Macros to get to the 4-bit fragment masks of the two 4K blocks that are 
 * stored in a byte.
 */

#define	UpperBlockFree(byte)	(((byte) & 0xf0) == 0x00)
#define	LowerBlockFree(byte)	(((byte) & 0x0f) == 0x00)
#define	BothBlocksFree(byte)	(((byte) & 0xff) == 0x00)
#define	GetUpperFragMask(byte) (((byte) >> 4) & 0x0f)
#define	GetLowerFragMask(byte) ((byte) & 0x0f)

/*
 * Macro to get a pointer into the bit map for a particular block.
 */

#define	BlockToCylinder(domainPtr, blockNum) \
 (unsigned int) (blockNum) / (domainPtr)->headerPtr->geometry.blocksPerCylinder
    
#define	GetBitmapPtr(domainPtr, blockNum) \
    &((domainPtr)->dataBlockBitmap[BlockToCylinder(domainPtr, blockNum) * \
		  (domainPtr)->bytesPerCylinder + \
		  ((unsigned int) ((blockNum) % \
		  (domainPtr)->headerPtr->geometry.blocksPerCylinder) / 2)])

#define	LAST_FRAG	 (FS_FRAGMENTS_PER_BLOCK - 1)
#define	FRAG_OFFSET_MASK (FS_FRAGMENTS_PER_BLOCK - 1)

/*
 * Size of a fragment (1K).
 */
#define FRAG_SIZE (FS_BLOCK_SIZE / FS_FRAGMENTS_PER_BLOCK)

/*
 * Percent of disk to keep free.
 */
int	fsdmPercentFree = 10;

/*
 * Block allocation style (settable with FS_SET_ALLOC_GAP Fs_Command)
 *	CONTIGUOUS	Blocks are allocated coniguously, beginning at the
 *			beginning of the cylinder.
 *	SKIP_ONE	Blocks are allocated with one free block between
 *			each allocated block.  The extra block gives the
 *			software time to generate a new disk request
 *			before the next allocated block rotates past the head.
 */
#define CONTIGUOUS	0
#define SKIP_ONE	1

int fsdm_AllocGap = SKIP_ONE;

/*
 * Flag to determine whether to keep track of file deletions and read/writes
 * by file type, and deletions by size/age/type.
 */
Boolean	fsdmKeepTypeInfo = TRUE;

/*
 * Information about each set of buckets, to be used to determine dynamically
 * what the upper limit is for each bucket.
 */
static Fs_HistGroupInfo fsHistGroupInfo[] = {		/* cum. range: */
    {1, FS_HIST_SECONDS},				/* < 10 secs */
    {10, FS_HIST_TEN_SECONDS},				/* < 1 min */
    {60, FS_HIST_MINUTES},				/* < 10 min */
    {600, FS_HIST_TEN_MINUTES},				/* < 1 hr */
    {3600, FS_HIST_HOURS},				/* < 10 hr */
    {18000, FS_HIST_FIVE_HOURS},			/* < 20 hr */
    {14400, FS_HIST_REST_HOURS},			/* < 1 day */
    {86400, FS_HIST_DAYS},				/* < 10 days */
    {864000, FS_HIST_TEN_DAYS },			/* < 60 days */
    {2592000, FS_HIST_THIRTY_DAYS },			/* < 90 days */
    {5184000, FS_HIST_SIXTY_DAYS }};			/* < 360 days */

/*
 * Forward references.
 */
static ReturnStatus FragToBlock();
static ReturnStatus AllocateBlock();
void FsdmFragFree();
void FsdmBlockFree();
static int FindHistBucket();


/*
 *----------------------------------------------------------------------
 *
 * FsdmBlockAllocInit() --
 *
 *	Initialize the data structure needed for block allocation for the
 *	given domain on a local disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for the bit map, the cylinder map and the fragment
 *	lists.  The bit map is read in and the cylinder map and fragment lists 
 *	are initialized.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsdmBlockAllocInit(domainPtr)
    register	Fsdm_Domain	*domainPtr;	/* Domain to initialize block
						 * allocation for. */
{
    int				blocksPerCylinder;
    int				bitmapBytes;
    register	unsigned char 	*bitmapPtr;
    register	Fsdm_Cylinder	*cylinderPtr;
    register	FsdmFragment	*fragPtr;
    register	int		i;
    register	int		j;
    register	int		k;
    int				*fragOffsetPtr;
    ReturnStatus		status;

    Sync_LockInitDynamic(&(domainPtr->dataBlockLock), "Fs:dataBlockLock");
    /*
     * Ensure some free disk space for disk block allocation.
     */
    domainPtr->minKFree =
	(domainPtr->headerPtr->dataBlocks * FS_FRAGMENTS_PER_BLOCK) /
								fsdmPercentFree;

    blocksPerCylinder = domainPtr->headerPtr->geometry.blocksPerCylinder;

    /*
     * Allocate the bit map.
     */
    bitmapBytes = (unsigned int) (blocksPerCylinder + 1) / 2;
    domainPtr->bytesPerCylinder = bitmapBytes;
    domainPtr->dataBlockBitmap = (unsigned char *) 
	malloc(domainPtr->headerPtr->bitmapBlocks * FS_BLOCK_SIZE);

    /* 
     * Read in the bit map.  The Block I/O interface is based on 1K blocks,
     * but the header information is in terms of 4K blocks.
     */
    status = Fsio_DeviceBlockIO(FS_READ, &(domainPtr->headerPtr->device), 
		domainPtr->headerPtr->bitmapOffset * 4, 
		domainPtr->headerPtr->bitmapBlocks * 4,
		(Address) domainPtr->dataBlockBitmap);
    if (status != SUCCESS) {
	printf(
	    "FsdmBlockAllocInit: Could not read data block bitmap.\n");
	return(status);
    }

    /*
     * Initialize the 3 fragment lists (1K, 2K and 3K).
     */
    for (i = 0; i < FSDM_NUM_FRAG_SIZES; i++) {
	domainPtr->fragLists[i] = (List_Links *) malloc(sizeof(List_Links));
	List_Init(domainPtr->fragLists[i]);
    }

    /*
     * Allocate an array cylinder information.
     */
    domainPtr->cylinders = (Fsdm_Cylinder *) 
	malloc(sizeof(Fsdm_Cylinder) * domainPtr->headerPtr->dataCylinders);

    /*
     * Now go through the bit map finding all of the fragments and putting
     * them onto the appropriate lists.  Also determine cylinder information.
     */
    bitmapPtr = domainPtr->dataBlockBitmap;
    cylinderPtr = domainPtr->cylinders;
    for (i = 0; i < domainPtr->headerPtr->dataCylinders; i++, cylinderPtr++) {
	cylinderPtr->blocksFree = 0;
	for (j = 0; j < bitmapBytes; j++, bitmapPtr++) {
	    if (UpperBlockFree(*bitmapPtr)) {
		cylinderPtr->blocksFree++;
	    } else {
		fragOffsetPtr = fragTable[GetUpperFragMask(*bitmapPtr)];
		for (k = 0; k < FSDM_NUM_FRAG_SIZES; k++, fragOffsetPtr++) {
		    if (*fragOffsetPtr != -1) {
			fragPtr = (FsdmFragment *) malloc(sizeof(FsdmFragment));
			List_Insert((List_Links *) fragPtr, 
				    LIST_ATREAR(domainPtr->fragLists[k]));
			fragPtr->blockNum = i * blocksPerCylinder + j * 2;
		    }
		}
	    }

	    /*
	     * There may be an odd number of blocks per cylinder.  If so
	     * and are at the end of the bit map for this cylinder, then
	     * we can bail out now.
	     */

	    if (j == (bitmapBytes - 1) && (blocksPerCylinder & 0x1)) {
		continue;
	    }

	    if (LowerBlockFree(*bitmapPtr)) {
		cylinderPtr->blocksFree++;
	    } else {
		fragOffsetPtr = fragTable[GetLowerFragMask(*bitmapPtr)];
		for (k = 0; k < FSDM_NUM_FRAG_SIZES; k++, fragOffsetPtr++) {
		    if (*fragOffsetPtr != -1) {
			fragPtr = (FsdmFragment *) malloc(sizeof(FsdmFragment));
			List_Insert((List_Links *) fragPtr, 
				    LIST_ATREAR(domainPtr->fragLists[k]));
			fragPtr->blockNum = i * blocksPerCylinder + j * 2 + 1;
		    }
		}
	    }
	}
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_BlockAllocate --
 *
 *	Allocate disk space for the given file.  This routine only allocates
 *	one block beginning at offset and going for numBytes.   If 
 *	offset + numBytes crosses a block boundary then a panic will occur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The file descriptor is modified to contain pointers to the allocated
 *	blocks.  Also *blockAddrPtr is set to the block that was allocated.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_BlockAllocate(hdrPtr, offset, numBytes, flags, blockAddrPtr, newBlockPtr)
    Fs_HandleHeader	*hdrPtr;	/* Local file handle. */
    int 		offset;		/* Offset to allocate at. */
    int 		numBytes;	/* Number of bytes to allocate. */
    int			flags;		/* FSCACHE_DONT_BLOCK */
    int			*blockAddrPtr; 	/* Disk address of block allocated. */
    Boolean		*newBlockPtr;	/* TRUE if there was no block allocated
					 * before. */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)hdrPtr;
    register Fsdm_FileDescriptor	*descPtr;
    register int		blockNum;
    Fsdm_BlockIndexInfo		indexInfo;
    int				newLastByte;
    int				curLastBlock;
    int				curLastFrag;
    Boolean			dirtiedIndex = FALSE;
    ReturnStatus		status;

    descPtr = handlePtr->descPtr;

    *blockAddrPtr = FSDM_NIL_INDEX;
    newLastByte = offset + numBytes - 1;
    blockNum = (unsigned int) offset / FS_BLOCK_SIZE;

    if ((unsigned int) (newLastByte) / FS_BLOCK_SIZE != blockNum) {
	panic("FsFileAllocate: Trying to allocate more than one block\n");
    }

    if (descPtr->lastByte != -1) {
	curLastBlock = (unsigned int) (descPtr->lastByte) / FS_BLOCK_SIZE;
    } else {
	curLastBlock = -1;
    }

    /*
     * If are allocating past the current last block in the file, then
     * make the last block into a full block.
     */
    if (curLastBlock != -1 && curLastBlock < FSDM_NUM_DIRECT_BLOCKS &&
        blockNum > curLastBlock) {
	curLastFrag = (unsigned int) (descPtr->lastByte & FS_BLOCK_OFFSET_MASK)
				/ FS_FRAGMENT_SIZE;
	if (curLastFrag < LAST_FRAG) {
	    /*
	     * Upgrade the fragment to a full block.
	     */
	    status = FragToBlock(handlePtr, curLastBlock, flags);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
    }

    /*
     * Set up the indexing structure here.
     */
    if (blockNum == 0) {
	/*
	 * This is the first block of the file so there is no previous
	 * block.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, blockNum, &indexInfo,
				 FSDM_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	/*
	 * This is not the first block in the file, so determine the
	 * previous block and then go to the first block.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, blockNum - 1, &indexInfo,
				 FSDM_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Fsdm_GetNextIndex(handlePtr, &indexInfo, FALSE);
	if (status != SUCCESS) {
	    Fsdm_EndIndex(handlePtr, &indexInfo, FALSE);
	    return(status);
	}
    }

    *newBlockPtr = (*indexInfo.blockAddrPtr == FSDM_NIL_INDEX);

    /*
     * Allocate space for the block.
     */

    status = AllocateBlock(handlePtr, descPtr, &indexInfo, newLastByte, 
		       curLastBlock, flags, &dirtiedIndex);

    if (status == SUCCESS) {
	*blockAddrPtr = *indexInfo.blockAddrPtr;
    }

    Fsdm_EndIndex(handlePtr, &indexInfo, dirtiedIndex);

    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Update the size of the file.  A firstByte of 0 is used in
     * named pipes to note that there is data in the pipe.
     * NOTE:  We can almost check the stream flags FS_CONSUME here,
     * except that when remote clients flush back named pipe blocks
     * they clear that flag so we the server treat it like a regular
     * file and don't consume or append.  Hence the check against fileType.
     */

    if (newLastByte > descPtr->lastByte) {
	descPtr->lastByte = newLastByte;
    }
    if (descPtr->firstByte == -1 && 
	((descPtr->fileType == FS_NAMED_PIPE) ||
	 (descPtr->fileType == FS_PSEUDO_DEV) ||
	 (descPtr->fileType == FS_XTRA_FILE))) {
	descPtr->firstByte = 0;
    }
    descPtr->descModifyTime = fsutil_TimeInSeconds;
    descPtr->flags |= FSDM_FD_DIRTY;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileDescTrunc --
 *
 *	Shorten a file to length bytes.  This updates the descriptor
 *	and may free blocks and indirect blocks from the end of the file.
 *
 * Results:
 *	Error if had problem with indirect blocks, otherwise SUCCESS.
 *
 * Side effects:
 *	Any allocated blocks after the given size are deleted.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsdm_FileDescTrunc(handlePtr, size)
    Fsio_FileIOHandle	*handlePtr;	/* File to truncate. */
    int			size;		/* Size to truncate the file to. */
{
    register Fsdm_Domain	 	*domainPtr;
    register Fsdm_FileDescriptor 	*descPtr;
    int				firstBlock;
    int				firstFrag;
    int				lastBlock;
    int				lastFrag;
    ReturnStatus		status = SUCCESS;
    Fsdm_BlockIndexInfo		indexInfo;
    int				newLastByte;
    int				savedLastByte;
    int				flags;
    Boolean			dirty;
    int				fragsToFree;
    int				bytesToFree;

    if (size < 0) {
	return(GEN_INVALID_ARG);
    }
    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;

    bytesToFree = descPtr->lastByte - size + 1;
    savedLastByte = descPtr->lastByte;

#ifndef CLEAN
    if (bytesToFree > 0) {
	Fsdm_RecordDeletionStats(&handlePtr->cacheInfo, bytesToFree);
    }
#endif CLEAN

    newLastByte = size - 1;
    if (descPtr->lastByte <= newLastByte) {
	status = SUCCESS;
	goto exit;
    }

    /*
     * Determine the first block and number of fragments in the first block.
     * This is called from the pipe trunc routine if its length is zero,
     * hence the check against firstByte here.
     */

    if (descPtr->firstByte >= 0) {
	firstBlock = (unsigned int) descPtr->firstByte / FS_BLOCK_SIZE;
    } else if (newLastByte == -1) {
	firstBlock = 0;
    } else {
	firstBlock = (unsigned int) newLastByte / FS_BLOCK_SIZE;
	firstFrag = (unsigned int) (newLastByte & FS_BLOCK_OFFSET_MASK) / 
					FS_FRAGMENT_SIZE;
    }

    /*
     * Determine the last block and the number of fragments in it.
     */

    lastBlock = (unsigned int) descPtr->lastByte / FS_BLOCK_SIZE;
    if (lastBlock >= FSDM_NUM_DIRECT_BLOCKS) {
	lastFrag = LAST_FRAG;
    } else {
	lastFrag = (descPtr->lastByte & FS_BLOCK_OFFSET_MASK)/FS_FRAGMENT_SIZE;
    }

    /*
     * Determine if the file is already short enough in terms of blocks.
     */

    if (newLastByte != -1 && firstBlock == lastBlock && 
	(lastFrag <= firstFrag || firstBlock >= FSDM_NUM_DIRECT_BLOCKS)) {
	if (newLastByte < descPtr->lastByte) {
	    descPtr->lastByte = newLastByte;
	    descPtr->descModifyTime = fsutil_TimeInSeconds;
	    descPtr->flags |= FSDM_FD_DIRTY;
	}
	status = SUCCESS;
	goto exit;
    }

    flags = FSDM_DELETE_INDIRECT_BLOCKS;
    if (newLastByte == -1) {
	flags |= FSDM_DELETE_EVERYTHING;
    }

    /*
     * Loop through the blocks deleting them.
     */
    status = Fsdm_GetFirstIndex(handlePtr, firstBlock, &indexInfo, flags);
    if (status != SUCCESS) {
	printf( "Fsdm_FileDescTrunc: Status %x setting up index\n",
		  status);
	goto exit;
    }
    while (TRUE) {
	if (indexInfo.blockAddrPtr == (int *) NIL ||
	    *indexInfo.blockAddrPtr == FSDM_NIL_INDEX) {
	    goto nextBlock;
	}

	dirty = FALSE;

	if (newLastByte == -1) {
	    /*
	     * The file is being made empty.
	     */
	    if (indexInfo.blockNum == lastBlock && lastFrag < LAST_FRAG) {
		FsdmFragFree(domainPtr, lastFrag + 1, 
		    (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK),
		    *indexInfo.blockAddrPtr & FRAG_OFFSET_MASK);
		descPtr->numKbytes -= lastFrag + 1;
	    } else {
		FsdmBlockFree(domainPtr, 
		    (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK));
		descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
	    }
	    *indexInfo.blockAddrPtr = FSDM_NIL_INDEX;
	} else if (indexInfo.blockNum == firstBlock) {
	    /*
	     * The first block that we truncate becomes the last block in
	     * the file.  If we are still in direct blocks we have to
	     * chop this (new last block) into the right number of fragments.
	     */
	    if (firstBlock >= FSDM_NUM_DIRECT_BLOCKS) {
		goto nextBlock;
	    }
	    if (firstBlock != lastBlock) {
		fragsToFree = LAST_FRAG - firstFrag;
	    } else {
		fragsToFree = lastFrag - firstFrag;
	    }
	    FsdmFragFree(domainPtr, fragsToFree,
		  (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK),
			firstFrag + 1);
	    descPtr->numKbytes -= fragsToFree;
	} else if (indexInfo.blockNum >= FSDM_NUM_DIRECT_BLOCKS || 
	           indexInfo.blockNum < lastBlock || lastFrag == LAST_FRAG) {
	    /*
	     * This is a full block so delete it.
	     */
	    FsdmBlockFree(domainPtr, 
		 (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK));
	    descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
	    *indexInfo.blockAddrPtr = FSDM_NIL_INDEX;
	} else {
	    /*
	     * Delete a fragment.  Only get here if are on the last block in 
	     * the file.
	     */
	    FsdmFragFree(domainPtr, lastFrag + 1, 
	      (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK),
	        *indexInfo.blockAddrPtr & FRAG_OFFSET_MASK);
	    descPtr->numKbytes -= lastFrag + 1;
	    *indexInfo.blockAddrPtr = FSDM_NIL_INDEX;
	}

	dirty = TRUE;

nextBlock:

	if (indexInfo.blockNum == lastBlock) {
	    break;
	}

	status = Fsdm_GetNextIndex(handlePtr, &indexInfo, dirty);
	if (status != SUCCESS) {
	    printf( "Fsdm_FileDescTrunc: Could not truncate file.\n");
	    Fsdm_EndIndex(handlePtr, &indexInfo, FALSE);
	    goto exit;
	}
    }

    Fsdm_EndIndex(handlePtr, &indexInfo, dirty);

    descPtr->lastByte = newLastByte;
    descPtr->descModifyTime = fsutil_TimeInSeconds;
    descPtr->flags |= FSDM_FD_DIRTY;

exit:
    /*
     * Make sure we really deleted the file if size is zero.
     */
    if (size == 0) {
	register int index;

	for (index=0 ; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	    if (descPtr->direct[index] != FSDM_NIL_INDEX) {
		printf("Fsdm_FileDescTrunc abandoning (direct) block %d in <%d,%d> \"%s\" savedLastByte %d\n",
		    descPtr->direct[index],
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName((Fs_HandleHeader *)handlePtr),
		    savedLastByte);
		descPtr->direct[index] = FSDM_NIL_INDEX;
	    }
	}
	for (index = 0 ; index <= 2 ; index++) {
	    if (descPtr->indirect[index] != FSDM_NIL_INDEX) {
		printf("Fsdm_FileDescTrunc abandoning (indirect) block %d in <%d,%d> \"%s\" savedLastByte %d\n",
		    descPtr->indirect[index], 
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName((Fs_HandleHeader *)handlePtr),
		    savedLastByte);
		descPtr->indirect[index] = FSDM_NIL_INDEX;
	    }
	}
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_RecordDeletionStats --
 *
 *	Record information about bytes deleted from a file.  This can
 *	be a result of truncation or overwriting.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The counts of bytes deleted, overall and by file type & age, are
 *	updated.
 *
 *----------------------------------------------------------------------
 */

void
Fsdm_RecordDeletionStats(cacheInfoPtr, bytesToFree)
    Fscache_FileInfo *cacheInfoPtr;
    int bytesToFree;
{
    unsigned int fragsToFree;	/* number of fragment-sized chunks to free */
    int timeIndex;  	/* counters into histogram */
    int sizeIndex = 0;
    int type;
    int when;

    /*
     * Record information about the number of bytes deleted.  Take the
     * information from the handle rather than the descriptor, since
     * the handle is the most up-to-date record of the size and modified
     * date of the file.  There's no easy way to check for holes in the
     * file, but the number of Kbytes on disk (corresponding to blocks in the
     * cache) is an upper bound on the actual amount of data to be deleted.
     * Therefore, for swap files, use that figure to get around holes.
     */

    if (fsdmKeepTypeInfo) {

	type = Fsdm_FindFileType(cacheInfoPtr);
	
	if (type == FSUTIL_FILE_TYPE_SWAP &&
	    cacheInfoPtr->hdrPtr->fileID.type == FSIO_LCL_FILE_STREAM) {
	    register Fsio_FileIOHandle *handlePtr;
	    register Fsdm_FileDescriptor 	*descPtr;

	    handlePtr = (Fsio_FileIOHandle *) cacheInfoPtr->hdrPtr;
	    descPtr = handlePtr->descPtr;
	    if (bytesToFree > descPtr->numKbytes * FRAG_SIZE) {
		bytesToFree = descPtr->numKbytes * FRAG_SIZE;
	    }
	}
	fs_TypeStats.bytesDeleted[type] += bytesToFree;
        if (cacheInfoPtr->attr.modifyTime > cacheInfoPtr->attr.createTime) {
	    when = cacheInfoPtr->attr.modifyTime;
	} else {
	    when = cacheInfoPtr->attr.createTime;
	}
	timeIndex = FindHistBucket(fsutil_TimeInSeconds - when);
	fragsToFree = bytesToFree / FRAG_SIZE;
	while (fragsToFree != 0) {
	    sizeIndex ++;
	    fragsToFree = fragsToFree >> 1;
	}
	fs_TypeStats.deleteHist[timeIndex][sizeIndex][type] ++;
	/*
	 * Store the actual number of bytes freed in the last column.
	 * For this, save the number of 1K blocks actually affected,
	 * so round *up*.  (We could save the real number of bytes, but
	 * then we run a greater risk of overflowing the counter, and
	 * it makes little difference whether we delete 200 bytes or
	 * 400 in a shot.)
	 */
	fragsToFree = (bytesToFree + FRAG_SIZE - 1) / FRAG_SIZE;
	fs_TypeStats.deleteHist
		[timeIndex][FS_HIST_SIZE_BUCKETS -1][type] += fragsToFree;
    }
    Fs_StatAdd(bytesToFree, fs_Stats.gen.fileBytesDeleted,
	       fs_Stats.gen.fileDeleteOverflow);
}


/*
 *----------------------------------------------------------------------
 *
 * FindHistBucket --
 *
 *	Given a number of seconds, return an index into the deletion
 *	histogram.  Performs a binary search into a static array that
 *	delimits the range of each bucket.
 *
 * Results:
 *	The index is returned.
 *
 * Side effects:
 *	The static array of bucket ranges is initialized if necessary.
 *
 *----------------------------------------------------------------------
 */

static int
FindHistBucket(secs)
    int secs;		/* number of seconds to be mapped into index */
{
    static int init = FALSE;
    static int buckets[FS_HIST_TIME_BUCKETS];
    int i;
    int max;
    int min;

    if (!init) {
	int current = 0;	/* current bucket */
	int group = 0;		/* index into group array */
	int total = 0;		/* current subtotal */
	
	for (group = 0;
	     group < sizeof(fsHistGroupInfo) / sizeof(Fs_HistGroupInfo);
	     group ++) {
	    for (i = 0; i < fsHistGroupInfo[group].bucketsPerGroup; i++) {
		total += fsHistGroupInfo[group].secondsPerBucket;
		buckets[current] = total;
		current ++;
	    }
	}
	init = TRUE;
    }
    /*
     * Anything out of the range of the binary search is in the final bucket.
     */
    if (secs > buckets[FS_HIST_TIME_BUCKETS - 2]) {
	return(FS_HIST_TIME_BUCKETS - 1);
    }
    
    min = 0;
    max = FS_HIST_TIME_BUCKETS - 2;
    while (max > min) {
	i = min + (max - min) / 2;
	if (secs >= buckets[i]) {
	    min = i + 1;
	} else if (i > 0 && secs < buckets[i-1]) {
	    max = i - 1;
	} else {
	    return(i);
	}
    }
    return(max);
}
	

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FindFileType --
 *
 *	Map from flags in the handle to a constant corresponding to
 *	the file type for the kernel.  
 *
 * Results:
 *	The value corresponding to the file's type is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Fsdm_FindFileType(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;	/* File to determine type of */
{
    switch (cacheInfoPtr->attr.userType) {
	case FS_USER_TYPE_TMP:
	    return(FSUTIL_FILE_TYPE_TMP);
	case FS_USER_TYPE_SWAP:
	    return(FSUTIL_FILE_TYPE_SWAP);
	case FS_USER_TYPE_OBJECT:
	    return(FSUTIL_FILE_TYPE_DERIVED);
	case FS_USER_TYPE_BINARY:
	    return(FSUTIL_FILE_TYPE_BINARY);
	default:
            return(FSUTIL_FILE_TYPE_OTHER);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * FsdmWriteBackDataBlockBitmap --
 *
 *	Write the data block bit map to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
FsdmWriteBackDataBlockBitmap(domainPtr)
    register	Fsdm_Domain	*domainPtr;	/* Domain for which to write 
						 * back the bitmap. */
{
    ReturnStatus	status;

    LOCK_MONITOR;

    status = Fsio_DeviceBlockIO(FS_WRITE, &(domainPtr->headerPtr->device), 
		    domainPtr->headerPtr->bitmapOffset * 4, 
		    domainPtr->headerPtr->bitmapBlocks * 4,
		    (Address) domainPtr->dataBlockBitmap);
    if (status != SUCCESS) {
	printf( "FsdmWriteBackDataBlockBitmap: Could not write out data block bitmap.\n");
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsdmWriteBackSummaryInfo --
 *
 *	Write summary info to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
FsdmWriteBackSummaryInfo(domainPtr)
    register	Fsdm_Domain	*domainPtr;	/* Domain for which to write 
			s			 * back the bitmap. */
{
    ReturnStatus	status;
    Fs_IOParam		io;
    Fs_IOReply		reply;

    LOCK_MONITOR;

    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    io.buffer = (Address)domainPtr->summaryInfoPtr;
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = domainPtr->summarySector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(domainPtr->headerPtr->device.type)].write)
		(&domainPtr->headerPtr->device, &io, &reply); 
    if (status != SUCCESS) {
	printf("FsdmWriteBackSummaryInfo: Could not write out summary info.\n");
    }
    if (status == GEN_NO_PERMISSION) {
	printf("FsdmWriteBackSummaryInfo: Disk is write-protected.\n");
	status = SUCCESS;
    }
    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SelectCylinderInt --
 *
 *	Find a cylinder to use to allocate a block.  The search starts
 *	with cylinderNum.  If it is full then it searches neighboring 
 *	cylinders until it finds a cylinder with a free block.
 *
 * Results:
 *	A cylinder number that contains a free block, -1 if none found.
 *
 * Side effects:
 *	The free count on the found cylinder is decremented.
 *
 *----------------------------------------------------------------------
 */

INTERNAL int
SelectCylinderInt(hashSeed, domainPtr, cylinderNum)
    int				hashSeed;	/* Seed for the hash, usually
						 * the file number. */
    register	Fsdm_Domain	*domainPtr;	/* Domain to select cylinder 
						 * from. */
    int				cylinderNum;	/* Cylinder to try first, -1
						 * if no preferred cylinder. */
{
    register	int		i;
    register	Fsdm_Cylinder	*cylinderPtr;

    if (cylinderNum == -1) {
	/*
	 * Do a random hash to find the starting point to allocate at.
	 */
	fs_Stats.alloc.cylHashes++;
	cylinderNum = ((hashSeed * 1103515245 + 12345) & 0x7fffffff) %
			    domainPtr->headerPtr->dataCylinders;
    }

    /*
     * Search forward starting at the desired cylinder.
     */

    for (i = cylinderNum, cylinderPtr = &(domainPtr->cylinders[cylinderNum]); 
	 i < domainPtr->headerPtr->dataCylinders;
	 i++, cylinderPtr++) {
	fs_Stats.alloc.cylsSearched++;
	if (cylinderPtr->blocksFree > 0) {
	    domainPtr->cylinders[i].blocksFree--;
	    return(i);
	}
    }

    /*
     * No block forward from desired cylinder so search backward.
     */

    for (i = cylinderNum - 1, 
		cylinderPtr = &(domainPtr->cylinders[cylinderNum - 1]);
	 i >= 0;
	 i--, cylinderPtr--) {
	fs_Stats.alloc.cylsSearched++;
	if (cylinderPtr->blocksFree > 0) {
	    domainPtr->cylinders[i].blocksFree--;
	    return(i);
	}
    }

    return(-1);
}


/*
 *----------------------------------------------------------------------
 *
 * FindBlockInt --
 *
 *	Search the bit map starting at the given cylinder for a free block.
 *
 * Results:
 *	A logical 4K block number for the disk where the first data block 
 *	is block 0 and a pointer into the bitmap for the block.
 *
 * Side effects:
 *	If the allocate flag is set then the bit map is modified.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static void
FindBlockInt(hashSeed, domainPtr, nearBlock, allocate, blockNumPtr, 
	     bitmapPtrPtr)
    int			hashSeed;	/* Seed for cylinder hash. */
    register Fsdm_Domain  *domainPtr; 	/* Domain to allocate blocks in. */
    int			nearBlock;  	/* Block number where this block should
					 * be near. */
    Boolean		allocate;   	/* TRUE if allocating full block, FALSE
					 * if intend to fragment block. */
    int			*blockNumPtr;  	/* Block number that was found. */
    unsigned char	**bitmapPtrPtr;	/* Bit map entry that corresponds to
					 * the block. */
{
    unsigned char *bitmapPtr;
    int	   blockNum;
    int	   block;
    int	   startingBlockOffset;
    int			   blocksPerCylinder;
    int			   mask;
    int			   cylinderNum;

    blocksPerCylinder = domainPtr->headerPtr->geometry.blocksPerCylinder;
    if (nearBlock != -1) {
	cylinderNum = ((unsigned int) nearBlock) / blocksPerCylinder;
	startingBlockOffset = ((unsigned int) nearBlock) % blocksPerCylinder;
    } else {
	cylinderNum = -1;
	startingBlockOffset = -1;
    }
    cylinderNum = SelectCylinderInt(hashSeed, domainPtr, cylinderNum);
    if (cylinderNum == -1) {
	*blockNumPtr = -1;
	return;
    }
    if (fsdm_AllocGap == 0) {
	/*
	 * CONTIGUOUS allocation.
	 * This is the original code that simply starts at the beginning
	 * of the cylinder and stops when it finds a free block.
	 */
	bitmapPtr = 
	  &(domainPtr->dataBlockBitmap[cylinderNum * domainPtr->bytesPerCylinder]);
	blockNum = cylinderNum * blocksPerCylinder;
	while (TRUE) {
	    fs_Stats.alloc.cylBitmapSearches++;
	    if (UpperBlockFree(*bitmapPtr)) {
		mask = 0xf0;
		break;
	    }
	    if (LowerBlockFree(*bitmapPtr)) {
		mask = 0x0f;
		blockNum++;
		break;
	    }
	    bitmapPtr++;
	    blockNum += 2;
	}
    } else {
	/*
	 * SKIP BLOCK allocation.
	 * startingBlockOffset is the offset of the last allocated block, or -1
	 * Set block to be ahead of the startingBlockOffset,
	 * and set the bitmapPtr to the corresponding spot in the bitmap.
	 */
	if (startingBlockOffset >= 0) {
	    startingBlockOffset += 1;
	    block = startingBlockOffset + fsdm_AllocGap;
	} else {
	    block = 0;
	    startingBlockOffset = blocksPerCylinder;
	}
	bitmapPtr =
	    &(domainPtr->dataBlockBitmap[cylinderNum * domainPtr->bytesPerCylinder
					 + block / 2]);
	 /*
	 * Walk through the bitmap.  We are guaranteed from SelectCylinder that
	 * there is a free block in this cylinder.
	 * Each byte in the bitmap covers two 4K blocks.
	 * The 'UpperBlock' covered by the byte is an even numbered block,
	 * and the 'LowerBlock' is odd.
	 */
	for ( ; block != startingBlockOffset; block++) {
	    fs_Stats.alloc.cylBitmapSearches++;
	    if (block >= blocksPerCylinder) {
		 /*
		  * Wrap back to the beginning of the cylinder
		  */
		 block -= blocksPerCylinder;
		 bitmapPtr -= domainPtr->bytesPerCylinder;
	    }
	    if ((block & 0x1) == 0 && UpperBlockFree(*bitmapPtr)) {
		mask = 0xf0;
		goto haveFreeBlock;
	    }
	    if ((block & 0x1) != 0 && LowerBlockFree(*bitmapPtr)) {
		mask = 0x0f;
		goto haveFreeBlock;
	    }
	    if (block & 0x1) {
		bitmapPtr++;
	    }
	}
	panic("FindBlockInt: no block\n");
	*blockNumPtr = -1;
	return;

haveFreeBlock:
	blockNum = cylinderNum * blocksPerCylinder + block;
    }
    if (allocate) {
	if (*bitmapPtr & mask) {
	    printf("bitmap = <%x>, checkMask = <%x>\n",
			       *bitmapPtr & 0xff, mask & 0xff);
	    printf("FsFindBlockInt, error in {Upper/Lower}BlockFree, failing.\n");
	    *blockNumPtr = -1;
	    return;
	}
	*bitmapPtr |= mask;
    }

    *bitmapPtrPtr = bitmapPtr;
    *blockNumPtr = blockNum;
    fs_Stats.alloc.blocksAllocated++;
}


/*
 *----------------------------------------------------------------------
 *
 * FsdmBlockFind --
 *
 *	Search the bit map starting at the given cylinder for a free block.
 *
 * Results:
 *	Results from FindBlockInt.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsdmBlockFind(hashSeed, domainPtr, nearBlock, allocate, blockNumPtr, bitmapPtrPtr)
    int			hashSeed;	/* Seed for cylinder hash. */
    Fsdm_Domain 		*domainPtr; 	/* Domain to allocate blocks in . */
    int			nearBlock;  	/* Block number where this block should
					 * be near. */
    Boolean		allocate;   	/* TRUE if allocating full block, FALSE
					 * if intend to fragment block. */
    int			*blockNumPtr;  	/* Block number that was found. */
    unsigned char	**bitmapPtrPtr;	/* Bit map entry that corresponds to
					 * the block. */
{
    LOCK_MONITOR;

    if (domainPtr->summaryInfoPtr->numFreeKbytes - FS_FRAGMENTS_PER_BLOCK <
	    domainPtr->minKFree) {
	*blockNumPtr = -1;
	UNLOCK_MONITOR;
	return;
    }

    FindBlockInt(hashSeed, domainPtr, nearBlock, allocate, blockNumPtr, 
		 bitmapPtrPtr);
    if (*blockNumPtr != -1) {
	domainPtr->summaryInfoPtr->numFreeKbytes -= FS_FRAGMENTS_PER_BLOCK;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * FsdmBlockFree --
 *
 *	Put the given block back into the bit map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bit map is modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsdmBlockFree(domainPtr, blockNum)
    register Fsdm_Domain *domainPtr; 	/* Handle for file to alloc blocks 
					 * for. */
    int			  blockNum;   	/* Block number to free. */
{
    register unsigned char *bitmapPtr;
    int			   cylinderNum;
    register int	   mask;
    register int	   checkMask;

    LOCK_MONITOR;

    domainPtr->summaryInfoPtr->numFreeKbytes += FS_FRAGMENTS_PER_BLOCK;
    fs_Stats.alloc.blocksFreed++;
    bitmapPtr = GetBitmapPtr(domainPtr, blockNum);
    cylinderNum = (unsigned int) blockNum / 
			domainPtr->headerPtr->geometry.blocksPerCylinder;
    domainPtr->cylinders[cylinderNum].blocksFree++;
    if ((blockNum % domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	mask = 0xf0;
    } else {
	mask = 0x0f;
    }
    checkMask = ~mask & 0xff;
    if ((*bitmapPtr & checkMask) != checkMask) {
	printf("bitmap = <%x>, checkMask = <%x>\n",
			   *bitmapPtr & 0xff, checkMask & 0xff);
        UNLOCK_MONITOR;
	printf("FsdmBlockFree free block %d\n", blockNum);
	return;
    } else {
	*bitmapPtr &= mask;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * FsdmFragFind --
 *
 *	Allocate a fragment out of the bit map.  If possible the fragment
 *	is allocated where the last fragment was allocated.
 *
 * Results:
 *	A logical block number and offset into the block where the
 *	fragment begins.
 *
 * Side effects:
 *	The bit map and the fragment lists might be modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsdmFragFind(hashSeed, domainPtr, numFrags, lastFragBlock, lastFragOffset, 
	    lastFragSize, newFragBlockPtr, newFragOffsetPtr)
    int			hashSeed;		/* Seed for cylinder hash. */
    register Fsdm_Domain 	*domainPtr;		/* Domain out of which to 
						 * allocate the fragment. */
    int		      	numFrags;   		/* Number of fragments to get: 
						 * 1, 2, or 3 */
    int		      	lastFragBlock;		/* Block number where the last 
						 * fragment for this file was 
						 * allocated. */
    int		      	lastFragOffset;		/* Fragment offset in the 
						 * block. */
    int		      	lastFragSize;		/* Number of fragments in the 
						 * last fragment. */
    int		      	*newFragBlockPtr;	/* Where to return new fragment
					         * block number. */
    int		      	*newFragOffsetPtr;	/* Where to return new fragment
						 * offset. */
{
    register FsdmFragment	  	*fragPtr;
    register unsigned char 	*bitmapPtr;
    register int	   	*savedOffsets;
    unsigned char		savedBitmap;
    int			   	*fragOffsetPtr;
    int			   	fragOffset;
    int			   	fragBlock;
    unsigned char 	   	*tBitmapPtr;
    List_Links	   	   	*fragList;
    int			   	byteOffset;
    int			   	fragMask;
    int			   	i;
    int			   	blockNum = -1;
    int				fragsToAllocate;

    LOCK_MONITOR;

    if (lastFragBlock == -1)  {
	fragsToAllocate = numFrags;
    } else {
	fragsToAllocate = numFrags - lastFragSize;
    }
    if (domainPtr->summaryInfoPtr->numFreeKbytes - fragsToAllocate <
	    domainPtr->minKFree) {
	*newFragBlockPtr = -1;
	UNLOCK_MONITOR;
	return;
    }

    /*
     * First try the block where the last fragment was.
     */

    fs_Stats.alloc.fragsAllocated++;
    if (lastFragBlock != -1 && 
	lastFragOffset + numFrags <= FS_FRAGMENTS_PER_BLOCK ) {
	fs_Stats.alloc.fragUpgrades++;
	bitmapPtr = GetBitmapPtr(domainPtr, lastFragBlock);
	if ((lastFragBlock % domainPtr->headerPtr->geometry.blocksPerCylinder)
		& 0x1) {
	    byteOffset = lastFragOffset + 4;
	} else {
	    byteOffset = lastFragOffset;
	}
	fragMask = 0;
	blockNum = lastFragBlock;
	fragOffset = lastFragOffset;
	/*
	 * Now make sure that there are enough free fragments in the block. 
	 */
	for (i = byteOffset + lastFragSize; i < byteOffset + numFrags; i++) {
	    if (*bitmapPtr & bitMasks[i]) {
		blockNum = -1;
		break;
	    }
	    fragMask |= bitMasks[i];
	}
    }

    if (blockNum == -1) {
	/* 
	 * We couldn't find space in the block where the last fragment was.
	 * First try all fragment lists starting with the one of the 
	 * desired size.
	 */

	for (i = numFrags - 1; 
	     i < FSDM_NUM_FRAG_SIZES && blockNum == -1; 
	     i++) {
	    fragList = domainPtr->fragLists[i];
	    while (!List_IsEmpty(fragList)) {
		fragPtr = (FsdmFragment *) List_First(fragList);
		List_Remove((List_Links *) fragPtr);
		fragBlock = fragPtr->blockNum;
		free((Address) fragPtr);
		/*
		 * Check to make sure that there really is a fragment of the
		 * needed size in the block.  These fragment lists are hints.
		 */
		bitmapPtr = GetBitmapPtr(domainPtr, fragBlock);
		if ((fragBlock %
		     domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
		    fragOffset = fragTable[GetLowerFragMask(*bitmapPtr)][i];
		} else {
		    fragOffset = fragTable[GetUpperFragMask(*bitmapPtr)][i];
		}
		if (fragOffset != -1) {
		    /*
		     * There is a fragment of this size so use this block.
		     */
		    blockNum = fragBlock;
		    break;
		} else {
		    fs_Stats.alloc.badFragList++;
		}
	    }
	}

	if (blockNum == -1) {
	    fs_Stats.alloc.fullBlockFrags++;
	    /*
	     * We couldn't find a fragmented block to use so have to 
	     * fragment a full block.
	     */
	    FindBlockInt(hashSeed, domainPtr, -1, FALSE, &blockNum, 
			 &tBitmapPtr);
	    if (blockNum == -1) {
		*newFragBlockPtr = -1;
		UNLOCK_MONITOR;
		return;
	    }
	    bitmapPtr = tBitmapPtr;
	    fragOffset = 0;
	}
	/*
	 * See if the block number corresponds to the high or low
	 * end of the bitmap byte.  If, for example, there are an odd
	 * number of blocks per cylinder, an even block number may be
	 * odd relative to the start of the cylinder, and relative to
	 * the start of the bitmap for that cylinder.
	 */
	if ((blockNum % domainPtr->headerPtr->geometry.blocksPerCylinder)
		& 0x1) {
	    byteOffset = fragOffset + 4;
	} else {
	    byteOffset = fragOffset;
	}
	fragMask = 0;
	for (i = byteOffset; i < byteOffset + numFrags; i++) {
	    fragMask |= bitMasks[i];
	}
	domainPtr->summaryInfoPtr->numFreeKbytes -= numFrags;
    } else {
	domainPtr->summaryInfoPtr->numFreeKbytes -= numFrags - lastFragSize;
	fs_Stats.alloc.fragsUpgraded++;
    }

    /*
     * Now put the block on all appropriate fragment lists.  savedOffsets 
     * points to the fragment offsets before we allocated the new fragment out
     * of the block.  fragOffsetPtr points to the fragment offset after
     * we allocated the fragment out of the block.
     */

    if (*bitmapPtr & fragMask) {
	UNLOCK_MONITOR;
	panic("Find frag bitmap error\n");
	*newFragBlockPtr = -1;
	return;
    } else {
	savedBitmap = *bitmapPtr;
	*bitmapPtr |= fragMask;
    }
    if ((blockNum % domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	savedOffsets = fragTable[GetLowerFragMask(savedBitmap)];
	fragOffsetPtr = fragTable[GetLowerFragMask(*bitmapPtr)];
    } else {
	savedOffsets = fragTable[GetUpperFragMask(savedBitmap)];
	fragOffsetPtr = fragTable[GetUpperFragMask(*bitmapPtr)];
    }
    for (i = 0; i < FSDM_NUM_FRAG_SIZES; i++, savedOffsets++, fragOffsetPtr++) {
	if (*savedOffsets == -1 && *fragOffsetPtr != -1) {
	    /*
	     * The block was not on the fragment list of this size before we
	     * allocated a new fragment out of it, so put it there. 
	     */
	    fragPtr = (FsdmFragment *) malloc(sizeof(FsdmFragment));
	    List_Insert((List_Links *) fragPtr, 
			LIST_ATREAR(domainPtr->fragLists[i]));
	    fragPtr->blockNum = blockNum;
	}
    }

    *newFragBlockPtr = blockNum;
    *newFragOffsetPtr = fragOffset;

    if (fragOffset + numFrags > FS_FRAGMENTS_PER_BLOCK) {
	UNLOCK_MONITOR;
	panic("FsdmFragFind, fragment overrun, offset %d numFrags %d\n",
			fragOffset, numFrags);
	return;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * FsdmFragFree --
 *
 *	Free the given fragment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bit map and fragment lists are modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsdmFragFree(domainPtr, numFrags, fragBlock, fragOffset) 
    register Fsdm_Domain *domainPtr;	/* Domain out of which to allocate the
					   fragment. */
    int			  numFrags; 	/* Number of fragments to free: 1, 2,
					   or 3 */
    int			  fragBlock;	/* Block number where the fragment
					   was allocated. */
    int			  fragOffset;	/* Fragment offset in the block. */
{
    register int	   *fragOffsets;
    register int 	   *savedOffsets;
    register unsigned char *bitmapPtr;
    FsdmFragment		   *fragPtr;
    unsigned char 	    mask;
    int		            i;
    int		            byteOffset;
    int			    fragMask;

    LOCK_MONITOR;

    fs_Stats.alloc.fragsFreed++;

    domainPtr->summaryInfoPtr->numFreeKbytes += numFrags;

    bitmapPtr = GetBitmapPtr(domainPtr, fragBlock);

    /*
     * Determine whether should clear upper or lower 4 bits.
     */

    if ((fragBlock % domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	byteOffset = fragOffset + 4;
	savedOffsets = fragTable[GetLowerFragMask(*bitmapPtr)];
    } else {
	byteOffset = fragOffset;
	savedOffsets = fragTable[GetUpperFragMask(*bitmapPtr)];
    }

    /*
     * Determine the bits to unset and unset them.
     */

    mask = 0;
    for (i = byteOffset; i < byteOffset + numFrags; i++) {
	mask |= bitMasks[i];
    }
    if ((*bitmapPtr & mask) != mask) {
	printf("bitmap = <%x>, checkMask = <%x>\n",
			   *bitmapPtr & 0xff, mask & 0xff);
	UNLOCK_MONITOR;
	printf("FsdmFragFree: block not free, block %d, numFrag %d, offset %d\n",
		    fragBlock, numFrags, fragOffset);
	return;
    } else {
	*bitmapPtr &= ~mask;
    }

    /*
     * Determine the new state of the block and put things onto the
     * proper fragment lists.  savedOffsets points to the array of frag
     * offsets before we freed the fragment in the block and fragOffsets
     * points to the frag offsets after we freed the fragment.
     */

    if ((fragBlock % domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	fragMask = GetLowerFragMask(*bitmapPtr);
    } else {
	fragMask = GetUpperFragMask(*bitmapPtr);
    }
    if (fragMask == 0) {
	fs_Stats.alloc.fragToBlock++;
	/*
	 * The block has become totally free.
	 */
	domainPtr->cylinders[(unsigned int) fragBlock / 
	     domainPtr->headerPtr->geometry.blocksPerCylinder].blocksFree++;
	UNLOCK_MONITOR;
	return;
    }
    fragOffsets = fragTable[fragMask];
    for (i = 0; i < FSDM_NUM_FRAG_SIZES; i++, fragOffsets++, savedOffsets++) {
	if (*savedOffsets == -1 && *fragOffsets != -1) {
	    /*
	     * A fragment of this size did not exist before we freed the 
	     * fragment but it does exist now.
	     */
	    fragPtr = (FsdmFragment *) malloc(sizeof(FsdmFragment));
	    List_Insert((List_Links *) fragPtr, 
			LIST_ATREAR(domainPtr->fragLists[i]));
	    fragPtr->blockNum = fragBlock;
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * OnlyFrag --
 *
 *	Determine if the given fragment is the only one in the block.
 *
 * Results:
 *	TRUE if the given fragment is the only one in the block.
 *
 * Side effects:
 *	The rest of the block is marked as allocated if the given fragment
 *	is the only one in the block.
 *
 *----------------------------------------------------------------------
 */

ENTRY Boolean
OnlyFrag(domainPtr, numFrags, fragBlock, fragOffset) 
    register Fsdm_Domain *domainPtr;	/* Domain of fragment. */
    int			  numFrags; 	/* Number of fragments to free: 1, 2,
					   or 3 */
    int			  fragBlock;	/* Block number where the fragment
					   was allocated. */
    int			  fragOffset;	/* Fragment offset in the block. */
{
    register unsigned char	*bitmapPtr;
    unsigned char 	 	mask;
    int		            	i;
    int		            	byteOffset;
    int				blockMask;

    LOCK_MONITOR;

    bitmapPtr = GetBitmapPtr(domainPtr, fragBlock);

    /*
     * Determine whether should access upper or lower 4 bits.
     */
    if ((fragBlock % domainPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	byteOffset = fragOffset + 4;
	blockMask = 0x0f;
    } else {
	byteOffset = fragOffset;
	blockMask = 0xf0;
    }

    /*
     * Determine which bits are set for this fragment.
     */
    mask = 0;
    for (i = byteOffset; i < byteOffset + numFrags; i++) {
	mask |= bitMasks[i];
    }
    if ((*bitmapPtr & mask) != mask) {
	printf("bitmap = <%x>, checkMask = <%x>\n",
			   *bitmapPtr & 0xff, mask & 0xff);
	UNLOCK_MONITOR;
	panic("OnlyFrag: Frag block corrupted.\n");
	return(FALSE);
    }
    if (((*bitmapPtr & ~mask) & blockMask) != 0) {
	/*
	 * There is another fragment in this block so we can't put the block
	 * into the bad block file yet.
	 */
	UNLOCK_MONITOR;
	return(FALSE);
    }

    /*
     * We were the only fragment in this block.  Mark the rest of the block
     * as allocated because our caller is going to put this block into the
     * bad block file.
     */
    *bitmapPtr |= blockMask;
    domainPtr->summaryInfoPtr->numFreeKbytes -= 
					FS_FRAGMENTS_PER_BLOCK - numFrags;

    UNLOCK_MONITOR;
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * UpgradeFragment --
 *
 *	Take a fragment and make it of sufficient size to fit the new
 *	fragment size.
 *
 * Results:
 *	SUCCESS, FS_NO_DISK_SPACE, or FS_WOULD_BLOCK (if cache is full).
 *
 * Side effects:
 *	*indexInfoPtr may be modified along with *indexInfoPtr->blockAddrPtr.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
UpgradeFragment(handlePtr, indexInfoPtr, curLastBlock, newLastFrag, 
		dontWriteThru, dontBlock, dirtiedIndexPtr)
    Fsio_FileIOHandle		*handlePtr;	/* File to allocate blocks 
						 * for. */
    register Fsdm_BlockIndexInfo *indexInfoPtr;	/* Index info structure. */
    int			curLastBlock;		/* The current last block. */
    int			newLastFrag;		/* New last fragment for this
						 * file.  Fragments are numbered
						 * from 0. */
    Boolean		dontWriteThru;		/* TRUE => make sure that the
						 * cache block that contains
						 * upgraded block isn't forced
						 * through to disk. */
    int			dontBlock;		/* FSCACHE_DONT_BLOCK */
    Boolean		*dirtiedIndexPtr; 	/* TRUE if modified the block 
						 * pointer in the file index 
						 * structure. */
{
    register	Fsdm_Domain	 *domainPtr;
    register	Fsdm_FileDescriptor *descPtr;
    register	int	 	 blockAddr;
    int				 curLastFrag;	/* Current last fragment for
						 * this file. */
    int				 curFragBlock;	/* Current disk block used for 
						 * last block in file. */
    int				 curFragOffset;	/* Offset in the block where
						 * the fragment begins. */
    int				 newFragBlock;	/* New disk block used for last
						   block in file. */
    int				 newFragOffset;	/* Offset in new block where 
						 * the fragment begins. */
    unsigned char 		 *bitmapPtr;
    Fscache_Block		 *fragCacheBlockPtr;
    Boolean			 found;
    ReturnStatus		 status = SUCCESS;
    int				 flags;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;
    blockAddr = *(indexInfoPtr->blockAddrPtr);

    curLastFrag = (unsigned int) (descPtr->lastByte & FS_BLOCK_OFFSET_MASK) /
					FS_FRAGMENT_SIZE;

    if (curLastBlock >= FSDM_NUM_DIRECT_BLOCKS || curLastFrag == LAST_FRAG ||
	curLastFrag >= newLastFrag) {
	/*
	 * There is already enough space so return.
	 */
	goto exit;
    }

    curFragBlock = (unsigned int) blockAddr / FS_FRAGMENTS_PER_BLOCK;
    curFragOffset = blockAddr & FRAG_OFFSET_MASK;
    if (newLastFrag < LAST_FRAG) {
	/*
	 * Have to allocate a larger fragment.  Note that fragments are
	 * numbered from zero so that the fragment number + 1 is equal to the
	 * number of fragments in the block.
	 */
	FsdmFragFind(handlePtr->hdr.fileID.minor, domainPtr, newLastFrag + 1, 
		    curFragBlock, curFragOffset, curLastFrag + 1,
		    &newFragBlock, &newFragOffset);
	if (newFragBlock == -1) {
	    status = FS_NO_DISK_SPACE;
	    goto exit;
	}
	if (curFragBlock == newFragBlock && curFragOffset == newFragOffset) {
	    /*
	     * Were able to extend the old fragment so return.
	     */
	    descPtr->numKbytes += newLastFrag - curLastFrag;
	    goto exit;
	}
    } else {
	/*
	 * Allocate a full block.
	 */
	FsdmBlockFind(handlePtr->hdr.fileID.minor, domainPtr,
		    indexInfoPtr->lastDiskBlock,
		    TRUE, &newFragBlock, &bitmapPtr);
	if (newFragBlock == -1) {
	    status = FS_NO_DISK_SPACE;
	    goto exit;
	} else if (newFragBlock == 0 && handlePtr->hdr.fileID.minor != 2) {
	    printf("UpgradeFragment: tried to allocate block 0 to non-root file #%d\n",
			handlePtr->hdr.fileID.minor);
	    status = FAILURE;
	    goto exit;
	}
	newFragOffset = 0;
    }
	    
    /*
     * Copy over the old fragment into the new larger one.  This
     * is done by fetching the block into the cache, switching the value in 
     * the file descriptor and marking the block dirty.
     */
    Fscache_FetchBlock(&handlePtr->cacheInfo, 
		      curLastBlock, FSCACHE_DATA_BLOCK | dontBlock,
		      &fragCacheBlockPtr, &found);
    if (fragCacheBlockPtr == (Fscache_Block *)NIL) {
	status = FS_WOULD_BLOCK;
	goto exit;
    }
    fs_Stats.blockCache.fragAccesses++;
    if (!found) {
	status = Fsio_DeviceBlockIO(FS_READ, &domainPtr->headerPtr->device,
		   blockAddr +
		   domainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		   curLastFrag + 1, fragCacheBlockPtr->blockAddr);
	if (status != SUCCESS) {
	    Fscache_UnlockBlock(fragCacheBlockPtr, 0, -1, 0, 0);
	    FsdmFragFree(domainPtr, newLastFrag + 1, 
		       newFragBlock, newFragOffset);
	    goto exit;
	}
	fs_Stats.blockCache.fragZeroFills++;
	/*
	 * Zero fill the rest of the block.
	 */
	bzero(fragCacheBlockPtr->blockAddr +
		    (curLastFrag + 1) * FS_FRAGMENT_SIZE,
	    FS_BLOCK_SIZE - (curLastFrag + 1) * FS_FRAGMENT_SIZE);
    } else {
	fs_Stats.blockCache.fragHits++;
	if (fragCacheBlockPtr->flags & FSCACHE_READ_AHEAD_BLOCK) {
	    fs_Stats.blockCache.readAheadHits++;
	}
    }
    /*
     * Commit the change in the fragments location.
     * 1 - unlock the cache block, specifying the new location.
     *		This step blocks if I/O is in progress on the block.
     *		After any I/O (such as a writeback) completes, then
     *		the block will be put on the dirty list with the new address.
     * 2 - update the file descriptors indexing information to point to
     *		the new block.
     * 3 - free up the old fragment.
     *
     * (As a historical note, steps 1 & 2 used to be reversed.  Files
     *	were ending up with the wrong trailing fragment occasionally.)
     */

    if (dontWriteThru) {
	flags = FSCACHE_CLEAR_READ_AHEAD | FSCACHE_DONT_WRITE_THRU;
    } else {
	flags = FSCACHE_CLEAR_READ_AHEAD;
    }
    Fscache_UnlockBlock(fragCacheBlockPtr, (unsigned) fsutil_TimeInSeconds, 
		       *indexInfoPtr->blockAddrPtr, 
		       (newLastFrag + 1) * FS_FRAGMENT_SIZE, flags);

    *(indexInfoPtr->blockAddrPtr) = 
		    newFragBlock * FS_FRAGMENTS_PER_BLOCK + newFragOffset;
    descPtr->numKbytes += newLastFrag - curLastFrag;
    descPtr->flags |= FSDM_FD_DIRTY;
    *dirtiedIndexPtr = TRUE;

    FsdmFragFree(domainPtr, curLastFrag + 1, curFragBlock, curFragOffset);

exit:
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * AllocateBlock --
 *
 *	Allocate a block for the file.
 *
 * Results:
 *	FS_NO_DISK_SPACE if could not allocate block.  Otherwise,
 *	returns SUCCESS.
 *
 * Side effects:
 *	 Also *indexInfoPtr may be modified along with *indexInfoPtr->blockAddr.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
AllocateBlock(handlePtr, descPtr, indexInfoPtr, newLastByte, curLastBlock, 
	      dontBlock, dirtiedIndexPtr)
    Fsio_FileIOHandle		*handlePtr;	/* File to allocate block for.*/
    register Fsdm_FileDescriptor 	*descPtr;	/* Pointer to the file desc. */
    register Fsdm_BlockIndexInfo 	*indexInfoPtr; 	/* Index info structure. * /
    int				newLastByte;	/* The new last byte in the 
						 * file. */
    int				curLastBlock;	/* The last block in the file 
						 * before started allocating. */
    int				dontBlock;	/* FSCACHE_DONT_BLOCK */
    Boolean			*dirtiedIndexPtr;/* TRUE if a new block was 
						  * allocated. */
{
    register	Fsdm_Domain	 *domainPtr;
    register	int		 blockAddr;
    unsigned char 		 *bitmapPtr;
    int				 newFragIndex;	/* {0, 1, 2, 3} */
    int				 blockNum;	/* Disk block that is 
						 * allocated. */
    int				 newFragOffset;	/* Offset in disk block where
						 * fragment begins. */
    ReturnStatus		 status = SUCCESS;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    blockAddr = *(indexInfoPtr->blockAddrPtr);
    if (blockAddr == 0 && handlePtr->hdr.fileID.minor != 2) {
	/*
	 * The zero'th block belongs to the root directory which is
	 * created by the makeFilesystem program.
	 */
	printf("AllocateBlock: non-root file <%d,%d> with block 0\n",
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor);
	return(FAILURE);
    }

    if (indexInfoPtr->blockNum >= FSDM_NUM_DIRECT_BLOCKS ||
	indexInfoPtr->blockNum < curLastBlock) {
	newFragIndex = LAST_FRAG;
    } else {
	newFragIndex = 
	 (unsigned int) (newLastByte & FS_BLOCK_OFFSET_MASK) / FS_FRAGMENT_SIZE;
    }

    if (descPtr->lastByte == -1 || indexInfoPtr->blockNum != curLastBlock) {
	/*
	 * Empty file or we are allocating a block that is before the
	 * last block in the file.
	 */
	if (newFragIndex < LAST_FRAG) {
	    /*
	     * Fragment the last block.
	     */
	    FsdmFragFind(handlePtr->hdr.fileID.minor, domainPtr,
			newFragIndex + 1, -1, -1, -1,
			&blockNum, &newFragOffset);
	    if (blockNum != -1) {
		*(indexInfoPtr->blockAddrPtr) = 
			    blockNum * FS_FRAGMENTS_PER_BLOCK + newFragOffset;
		*dirtiedIndexPtr = TRUE;
		descPtr->numKbytes += newFragIndex + 1;
	    } else {
		status = FS_NO_DISK_SPACE;
	    }
	} else {
	    /*
	     * Allocate a full block if one isn't there already.
	     */
	    if (blockAddr == FSDM_NIL_INDEX) {
		FsdmBlockFind(handlePtr->hdr.fileID.minor, domainPtr,
			    indexInfoPtr->lastDiskBlock, 
			    TRUE, &blockNum, &bitmapPtr);
		if (blockNum == -1) {
		    status = FS_NO_DISK_SPACE;
		} else if (blockNum == 0 && handlePtr->hdr.fileID.minor != 2) {
		    /*
		     * The zero'th block belongs to the root directory which is
		     * created by the makeFilesystem program.
		     */
		    printf("AllocateBlock: non-root file <%d,%d> wants block 0\n",
				handlePtr->hdr.fileID.major,
				handlePtr->hdr.fileID.minor);
		    status = FAILURE;
		} else {
		    *(indexInfoPtr->blockAddrPtr) = 
					    blockNum * FS_FRAGMENTS_PER_BLOCK;
		    descPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
		    *dirtiedIndexPtr = TRUE;
		}
	    }
	}
    } else {
	/*
	 * Are allocating on top of the last block so make sure that the 
	 * last fragment is large enough.
	 */   
	status = UpgradeFragment(handlePtr, indexInfoPtr, 
				 curLastBlock, newFragIndex, TRUE,
				 dontBlock, dirtiedIndexPtr);
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FragToBlock --
 *
 *	Upgrade the given fragment to a block.
 *
 * Results:
 *	SUCCESS, FS_NO_DISK_SPACE, FS_WOULD_BLOCK.
 *
 * Side effects:
 *	The size in the file descriptor is updated so that it is on a block
 *	boundary.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
FragToBlock(handlePtr, blockNum, dontBlock)
    register Fsio_FileIOHandle	*handlePtr;
    int				blockNum;
    int				dontBlock;	/* FSCACHE_DONT_BLOCK */
{
    register Fsdm_FileDescriptor	*descPtr;
    Fsdm_BlockIndexInfo		indexInfo;
    ReturnStatus		status;
    Boolean			dirtiedIndex;

    /*
     * Set up the indexing structure.
     */
    descPtr = handlePtr->descPtr;
    if (blockNum == 0) {
	/*
	 * This is the first block of the file so there is no previous
	 * block.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, blockNum, &indexInfo,
				 FSDM_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	/*
	 * This is not the first block in the file, so determine the
	 * previous block and then go to the first block.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, blockNum - 1, &indexInfo,
				 FSDM_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Fsdm_GetNextIndex(handlePtr, &indexInfo, FALSE);
	if (status != SUCCESS) {
	    Fsdm_EndIndex(handlePtr, &indexInfo, FALSE);
	    return(status);
	}
    }

    /*
     * Now upgrade to a full block.
     */

    status = UpgradeFragment(handlePtr, &indexInfo, blockNum, LAST_FRAG,
			     FALSE, dontBlock, &dirtiedIndex);
    if (status == SUCCESS) {
	descPtr->lastByte = blockNum * FS_BLOCK_SIZE + FS_BLOCK_SIZE - 1;
	descPtr->descModifyTime = fsutil_TimeInSeconds;
	descPtr->flags |= FSDM_FD_DIRTY;
    }
    Fsdm_EndIndex(handlePtr, &indexInfo, dirtiedIndex);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_NamedPipeTrunc --
 *
 *	Truncate a named pipe.  This is defined to leave the length most
 *	recently written bytes in the pipe.  Another way of saying this
 *	is that it consumes data from the from of the pipe so there
 *	is length bytes in it.  Called via the Truncate system calls,
 *	or from the cache after reading from the named pipe.
 *
 *	THIS ROUTINE SHOULDN'T HAVE TO FIX UP THE cacheInfo !
 *
 * Results:
 *	Error  if can't go through file indexing structure.
 *
 * Side effects:
 *	Any allocated blocks in the affected range of bytes are deleted.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsdm_NamedPipeTrunc(handlePtr, length)
    Fsio_FileIOHandle	*handlePtr;	/* Handle for the pipes backing store */
    int		length;			/* Leave this many bytes at the end */
{
    register	Fsdm_Domain	 *domainPtr;
    register	Fsdm_FileDescriptor *descPtr;
    int				 firstByte;
    int				 curFirstBlock;
    int				 newFirstBlock;
    int				 lastDeadBlock;
    ReturnStatus		 status = SUCCESS;
    Fsdm_BlockIndexInfo		 indexInfo;
    Boolean			 dirty;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;

    if (descPtr->firstByte == -1) {
	/*
	 * There is no space allocated on disk for this pipe.  The handle
	 * may have values for firstByte and lastByte because a client
	 * gives us the size on close, but there doesn't have to be space
	 * allocated on disk.
	 */
	if (descPtr->lastByte != -1) {
	    panic("Fsdm_NamedPipeTrunc, first -1, last %d\n",
		    descPtr->lastByte);
	}
	handlePtr->cacheInfo.attr.firstByte = -1;
	handlePtr->cacheInfo.attr.lastByte = -1;
	status = SUCCESS;
	goto exit;
    }

    firstByte = descPtr->lastByte - length + 1;
    handlePtr->cacheInfo.attr.firstByte = firstByte;

    curFirstBlock = (unsigned int) descPtr->firstByte / FS_BLOCK_SIZE;
    newFirstBlock = firstByte / FS_BLOCK_SIZE;
    if (length == 0) {
	lastDeadBlock = newFirstBlock;
    } else {
	lastDeadBlock = newFirstBlock - 1;
    }

    if (curFirstBlock <= lastDeadBlock) {
	/*
	 * Delete any blocks before the new last block, or all of
	 * them if the pipe is being cleaned out.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, curFirstBlock, &indexInfo, 
				 FSDM_DELETE_INDIRECT_BLOCKS | 
				 FSDM_DELETING_FROM_FRONT);
	if (status != SUCCESS) {
	    printf( "Fsdm_NamedPipeTrunc: Couldn't get index.\n");
	    goto exit;
	}
    
	while (TRUE) {
	    dirty = FALSE;
	    if (indexInfo.blockAddrPtr != (int *) NIL &&
		*indexInfo.blockAddrPtr != FSDM_NIL_INDEX) {
		FsdmBlockFree(domainPtr,
		    (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK));
		*indexInfo.blockAddrPtr = FSDM_NIL_INDEX;
		descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
		dirty = TRUE;
	    }
	    if (indexInfo.blockNum == lastDeadBlock) {
		break;
	    }
	    status = Fsdm_GetNextIndex(handlePtr, &indexInfo, dirty);
	    if (status != SUCCESS) {
		panic("Fsdm_NamedPipeTrunc. Couldn't get next index.\n");
		Fsdm_EndIndex(handlePtr, &indexInfo, FALSE);
		goto exit;
	    }
	}
	Fsdm_EndIndex(handlePtr, &indexInfo, dirty);
    }

    descPtr->firstByte = firstByte;
    if (descPtr->firstByte > descPtr->lastByte) {
	descPtr->firstByte = descPtr->lastByte = -1;
	handlePtr->cacheInfo.attr.firstByte = -1;
	handlePtr->cacheInfo.attr.lastByte = -1;
    }
    descPtr->descModifyTime = fsutil_TimeInSeconds;
    descPtr->flags |= FSDM_FD_DIRTY;

exit:
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DomainInfo --
 *
 *	Return info about the given domain.
 *
 * Results:
 *	Error  if can't get to the domain.
 *
 * Side effects:
 *	The domain info struct is filled in.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_DomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID		*fileIDPtr;
    Fs_DomainInfo	*domainInfoPtr;
{
    int		domain = fileIDPtr->major;
    Fsdm_Domain	*domainPtr;

    if (domain >= FSDM_MAX_LOCAL_DOMAINS) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    domainPtr = Fsdm_DomainFetch(domain, FALSE);
    if (domainPtr == (Fsdm_Domain *) NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    domainInfoPtr->maxKbytes = 
		    domainPtr->headerPtr->dataBlocks * FS_FRAGMENTS_PER_BLOCK;
    domainInfoPtr->freeKbytes = domainPtr->summaryInfoPtr->numFreeKbytes;
    domainInfoPtr->maxFileDesc = domainPtr->headerPtr->numFileDesc;
    domainInfoPtr->freeFileDesc = domainPtr->summaryInfoPtr->numFreeFileDesc;
    domainInfoPtr->blockSize = FS_BLOCK_SIZE;
    domainInfoPtr->optSize = FS_BLOCK_SIZE;

    Fsdm_DomainRelease(domain);

    return(SUCCESS);
}

static	void	PutInBadBlockFile();


/*
 *----------------------------------------------------------------------
 *
 * FsdmBlockRealloc --
 *
 *	Allocate a new block on disk to replace the given block.  This is
 *	intended to be used by the cache when it can't write out a block
 *	because of a disk error.
 *
 * Results:
 *	The new disk block that is allocated.
 *
 * Side effects:
 *	The descriptor or indirect blocks are modified to point to the newly
 *	allocated block.
 *
 *----------------------------------------------------------------------
 */
int
FsdmBlockRealloc(hdrPtr, virtBlockNum, physBlockNum)
    Fs_HandleHeader	*hdrPtr;
    int			virtBlockNum;
    int			physBlockNum;
{
    register	Fsio_FileIOHandle	*handlePtr;
    Fsdm_BlockIndexInfo	indexInfo;
    Fsdm_Domain		*domainPtr;
    int			newBlockNum = -1;
    Boolean		dirtiedIndex = FALSE;
    Boolean		setupIndex = FALSE;
    unsigned char	*bitmapPtr;
    ReturnStatus	status;
    Fsdm_FileDescriptor	*descPtr;

    if (hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	panic("FsdmBlockRealloc, wrong handle type <%d>\n",
	    hdrPtr->fileID.type);
	return(-1);
    }
    handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    if (handlePtr->hdr.fileID.minor == 0) {
	/*
	 * This is a descriptor block.
	 */
	printf(
	    "FsdmBlockRealloc: Bad descriptor block.  Domain=%d block=%d\n",
		  handlePtr->hdr.fileID.major, physBlockNum);
	return(-1);
    }

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(-1);
    }

    Fsutil_HandleLock((Fs_HandleHeader *)handlePtr);
    descPtr = handlePtr->descPtr;
    if (virtBlockNum >= 0) {
	int		bytesInBlock;
	/*
	 * A normal data block.
	 */
	status = Fsdm_GetFirstIndex(handlePtr, virtBlockNum, &indexInfo, 0);
	if (status != SUCCESS) {
	    printf( 
	       "FsdmBlockRealloc: Setup index (1) failed status <%x>\n", status);
	    goto error;
	}
	setupIndex = TRUE;
	if (*indexInfo.blockAddrPtr != physBlockNum) {
	    panic("FsdmBlockRealloc: Bad physical block num.\n");
	}
	bytesInBlock = descPtr->lastByte - virtBlockNum * FS_BLOCK_SIZE + 1;
	if (bytesInBlock > FS_FRAGMENT_SIZE * (FS_FRAGMENTS_PER_BLOCK - 1) ||
	    virtBlockNum >= FSDM_NUM_DIRECT_BLOCKS) {
	    /* 
	     * Have a full block.
	     */
	    FsdmBlockFind(handlePtr->hdr.fileID.minor, domainPtr,
			physBlockNum / FS_FRAGMENTS_PER_BLOCK, TRUE,
			&newBlockNum, &bitmapPtr);
	    if (newBlockNum == -1) {
		printf( "FsdmBlockRealloc: No disk space (1)\n");
		goto error;
	    }
	    newBlockNum *= FS_FRAGMENTS_PER_BLOCK;
	    *indexInfo.blockAddrPtr = newBlockNum;
	    dirtiedIndex = TRUE;
	    descPtr->flags |= FSDM_FD_DIRTY;
	    PutInBadBlockFile(handlePtr, domainPtr, physBlockNum);
	} else {
	    int	newFragOffset;
	    int	numFrags;
	    /*
	     * Have a fragment.
	     */
	    numFrags = (bytesInBlock - 1) / FS_FRAGMENT_SIZE + 1;
	    FsdmFragFind(handlePtr->hdr.fileID.minor, domainPtr, numFrags,
			-1, -1, -1, &newBlockNum, &newFragOffset);
	    if (newBlockNum == -1) {
		printf( "FsdmBlockRealloc: No disk space (2)\n");
		goto error;
	    }
	    newBlockNum = newBlockNum * FS_FRAGMENTS_PER_BLOCK + newFragOffset;
	    *indexInfo.blockAddrPtr = newBlockNum;
	    dirtiedIndex = TRUE;
	    descPtr->flags |= FSDM_FD_DIRTY;
	    if (OnlyFrag(domainPtr, numFrags,
			   physBlockNum / FS_FRAGMENTS_PER_BLOCK,
			   physBlockNum & FRAG_OFFSET_MASK)) {
		PutInBadBlockFile(handlePtr, domainPtr,
				  physBlockNum & ~FRAG_OFFSET_MASK);
	    } else {
		/*
		 * The fragment is in a block with other valid fragments.
		 * We do nothing and just leave the fragment allocated
		 * in the bitmap but unreferenced by any file.
		 * This means checkFS should verify that an unreferenced
		 * fragment is readable before marking it free.
		 */
		printf( "Leaving bad frag #%d unreferenced\n",
			    physBlockNum);
	    }
	}
    } else {
	Fscache_Block	*blockPtr = (Fscache_Block *)NIL;
	int		*blockAddrPtr;

	physBlockNum = -physBlockNum;
	if (virtBlockNum == -1) {
	    /*
	     * This is the first indirect block.
	     */
	    blockAddrPtr = &descPtr->indirect[0];
	} else if (virtBlockNum == -2) {
	    /*
	     * Second indirect block.
	     */
	    blockAddrPtr = &descPtr->indirect[1];
	} else if (descPtr->indirect[1] == FSDM_NIL_INDEX) {
	    panic("FsdmBlockRealloc: Can't find indirect block\n");
	} else {
	    Boolean	found;
	    /*
	     * Read in the doubly indirect block  so that we can get to the
	     * indirect block that we want.
	     */
	    fs_Stats.blockCache.indBlockAccesses++;
	    Fscache_FetchBlock(&handlePtr->cacheInfo, -2,
			FSCACHE_IND_BLOCK, &blockPtr, &found);
	    if (!found) {
		status = Fsio_DeviceBlockIO(FS_READ,
			&(domainPtr->headerPtr->device), 
		       descPtr->indirect[1], FS_FRAGMENTS_PER_BLOCK, 
		       blockPtr->blockAddr);
		if (status != SUCCESS) {
		    printf( 
	"FsdmBlockRealloc: Could not read doubly indirect block, status <%x>\n", 
			status);
		    Fscache_UnlockBlock(blockPtr, 0, 0, 0, FSCACHE_DELETE_BLOCK);
		    goto error;
		} else {
		    fs_Stats.gen.physBytesRead += FS_BLOCK_SIZE;
		}
	    } else {
		fs_Stats.blockCache.indBlockHits++;
	    }
	    blockAddrPtr = (int *)blockPtr->blockAddr + (-virtBlockNum - 3);
	}
	if (*blockAddrPtr != physBlockNum) {
	    panic("FsdmBlockRealloc: Bad phys addr for indirect block (2)\n");
	}
	/*
	 * Allocate a new indirect block.
	 */
	FsdmBlockFind(handlePtr->hdr.fileID.minor, domainPtr, -1, TRUE, 
		    &newBlockNum, &bitmapPtr);
	if (newBlockNum == -1) {
	    printf( "FsdmBlockRealloc: No disk space (3)\n");
	    goto error;
	}
	newBlockNum = (newBlockNum + domainPtr->headerPtr->dataOffset) * 
			FS_FRAGMENTS_PER_BLOCK;
	*blockAddrPtr = newBlockNum;
	if (blockPtr == (Fscache_Block *)NIL) {
	    descPtr->flags |= FSDM_FD_DIRTY;
	} else {
	    Fscache_UnlockBlock(blockPtr, (unsigned int)fsutil_TimeInSeconds, 
			       -(descPtr->indirect[1]), FS_BLOCK_SIZE, 0);
	}
	PutInBadBlockFile(handlePtr, domainPtr,
			  physBlockNum - FS_FRAGMENTS_PER_BLOCK * 
				     domainPtr->headerPtr->dataOffset);
	newBlockNum = -newBlockNum;
    }

error:

    if (setupIndex) {
	Fsdm_EndIndex(handlePtr, &indexInfo, dirtiedIndex);
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    Fsutil_HandleUnlock((Fs_HandleHeader *)handlePtr);
    return(newBlockNum);
}


/*
 *----------------------------------------------------------------------
 *
 * PutInBadBlockFile --
 *
 *	Put the given physical block into the bad block file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Block appended to the bad block file.
 *
 *----------------------------------------------------------------------
 */
static void
PutInBadBlockFile(handlePtr, domainPtr, blockNum)
    Fsio_FileIOHandle	*handlePtr;	/* File which owned bad block. */
    Fsdm_Domain	*domainPtr;		/* Pointer to domain. */
    int		blockNum;	/* Block number to put in bad block file. */
{
    Fs_FileID		fileID;
    Fsio_FileIOHandle	*badBlockHandlePtr;
    Fsdm_FileDescriptor	*descPtr;
    Fsdm_BlockIndexInfo	indexInfo;
    ReturnStatus	status;
    int			lastBlock;

    fileID.serverID = rpc_SpriteID;
    fileID.type = FSIO_LCL_FILE_STREAM;
    fileID.major = handlePtr->hdr.fileID.major;
    fileID.minor = FSDM_BAD_BLOCK_FILE_NUMBER;
    badBlockHandlePtr = (Fsio_FileIOHandle *)Fsutil_HandleFetch(&fileID);
    if (badBlockHandlePtr == (Fsio_FileIOHandle *)NIL) {
	/*
	 * Have to make a new handle since we don't have one for this domain
	 * in memory.
	 */
	status = Fsio_LocalFileHandleInit(&fileID, "BadBlockFile",
			&badBlockHandlePtr);
	if (status != SUCCESS) {
	    printf("PutInBadBlockFile: error %x reading descriptor\n", status);
	    return;
	}
    }
    descPtr = badBlockHandlePtr->descPtr;
    if (descPtr->lastByte != -1) {
	lastBlock = descPtr->lastByte / FS_BLOCK_SIZE;
    } else {
	lastBlock = -1;
    }
    status = Fsdm_GetFirstIndex(handlePtr, lastBlock + 1, &indexInfo,
			     FSDM_ALLOC_INDIRECT_BLOCKS);
    if (status != SUCCESS) {
	printf( "PutInBadBlockFile: Could not fetch index\n");
    } else {
	*indexInfo.blockAddrPtr = blockNum;
	descPtr->lastByte += FS_BLOCK_SIZE;
	descPtr->flags |= FSDM_FD_DIRTY;
	descPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
	Fsdm_EndIndex(handlePtr, &indexInfo, TRUE);
    }

    Fsutil_HandleUnlock((Fs_HandleHeader *)badBlockHandlePtr);
}
