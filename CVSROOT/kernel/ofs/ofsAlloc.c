/* 
 * ofsAlloc.c --
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

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fscache.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <spriteTime.h>
#include <devFsOpTable.h>
#include <fsStat.h>
#include <timer.h>
#include <rpc.h>
#include <proc.h>
#include <string.h>
#include <fsdm.h>
#include <ofs.h>

#include <stdio.h>

/*
 * Each domain, which is a separate piece of disk, is locked
 * during allocation.
 */
#define LOCKPTR (&ofsPtr->dataBlockLock)

/*
 * A table indexed by a 4 bit value is used by the allocation routine to 
 * quickly determine the location of 1, 2, and 3K fragments in a byte.  
 * The indices of the fragments start from 0.  If there is no such fragment in 
 * the byte then a -1 is used.
 */

static int fragTable[16][OFS_NUM_FRAG_SIZES] = {
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

#define	BlockToCylinder(ofsPtr, blockNum) \
 (unsigned int) (blockNum) / (ofsPtr)->headerPtr->geometry.blocksPerCylinder

#define	GetBitmapPtr(ofsPtr, blockNum) \
    &((ofsPtr)->dataBlockBitmap[BlockToCylinder(ofsPtr, blockNum) * \
		  (ofsPtr)->bytesPerCylinder + \
		  ((unsigned int) ((blockNum) % \
		  (ofsPtr)->headerPtr->geometry.blocksPerCylinder) / 2)])

#define	LAST_FRAG	 (FS_FRAGMENTS_PER_BLOCK - 1)
#define	FRAG_OFFSET_MASK (FS_FRAGMENTS_PER_BLOCK - 1)

/*
 * Size of a fragment (1K).
 */
#define FRAG_SIZE (FS_BLOCK_SIZE / FS_FRAGMENTS_PER_BLOCK)

/*
 * Percent of disk to keep free.
 */
int	ofsPercentFree = 10;

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

int ofs_AllocGap = CONTIGUOUS;

/*
 * Forward references.
 */
static void FindBlockInt _ARGS_((int hashSeed, Ofs_Domain *ofsPtr,
		int nearBlock, Boolean allocate, int *blockNumPtr, 
		unsigned char **bitmapPtrPtr));
static ReturnStatus UpgradeFragment _ARGS_((Ofs_Domain *ofsPtr, 
		Fsio_FileIOHandle *handlePtr, OfsBlockIndexInfo *indexInfoPtr,
		int curLastBlock, int newLastFrag, Boolean dontWriteThru, 
		int dontBlock, Boolean *dirtiedIndexPtr));
static ReturnStatus AllocateBlock _ARGS_((Fsio_FileIOHandle *handlePtr, 
		Fsdm_FileDescriptor *descPtr, OfsBlockIndexInfo *indexInfoPtr,
		int newLastByte, int curLastBlock, int dontBlock,
		Boolean *dirtiedIndexPtr));
static ReturnStatus FragToBlock _ARGS_((Ofs_Domain *ofsPtr, 
		Fsio_FileIOHandle *handlePtr, int blockNum, int dontBlock));
static void PutInBadBlockFile _ARGS_((Fsio_FileIOHandle *handlePtr, 
		Ofs_Domain *ofsPtr, int blockNum));


/*
 *----------------------------------------------------------------------
 *
 * OfsBlockAllocInit() --
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
OfsBlockAllocInit(ofsPtr)
    register	Ofs_Domain	*ofsPtr;	/* Domain to initialize block
						 * allocation for. */
{
    int				blocksPerCylinder;
    int				bitmapBytes;
    register	unsigned char 	*bitmapPtr;
    register	OfsCylinder	*cylinderPtr;
    register	OfsFragment	*fragPtr;
    register	int		i;
    register	int		j;
    register	int		k;
    int				*fragOffsetPtr;
    ReturnStatus		status;

    Sync_LockInitDynamic(&(ofsPtr->dataBlockLock), "Fs:ofsDataBlockLock");
    /*
     * Ensure some free disk space for disk block allocation.
     */
    ofsPtr->minKFree =
	(ofsPtr->headerPtr->dataBlocks * FS_FRAGMENTS_PER_BLOCK) /
				ofsPercentFree;

    blocksPerCylinder = ofsPtr->headerPtr->geometry.blocksPerCylinder;

    /*
     * Allocate the bit map.
     */
    bitmapBytes = (unsigned int) (blocksPerCylinder + 1) / 2;
    ofsPtr->bytesPerCylinder = bitmapBytes;
    ofsPtr->dataBlockBitmap = (unsigned char *) 
	malloc(ofsPtr->headerPtr->bitmapBlocks * FS_BLOCK_SIZE);

    /* 
     * Read in the bit map.  The Block I/O interface is based on 1K blocks,
     * but the header information is in terms of 4K blocks.
     */
    status = OfsDeviceBlockIO(ofsPtr, FS_READ, 
		ofsPtr->headerPtr->bitmapOffset * 4, 
		ofsPtr->headerPtr->bitmapBlocks * 4,
		(Address) ofsPtr->dataBlockBitmap);
    if (status != SUCCESS) {
	printf(
	    "OfsBlockAllocInit: Could not read data block bitmap.\n");
	return(status);
    }

    /*
     * Initialize the 3 fragment lists (1K, 2K and 3K).
     */
    for (i = 0; i < OFS_NUM_FRAG_SIZES; i++) {
	ofsPtr->fragLists[i] = (List_Links *) malloc(sizeof(List_Links));
	List_Init(ofsPtr->fragLists[i]);
    }

    /*
     * Allocate an array cylinder information.
     */
    ofsPtr->cylinders = (OfsCylinder *) 
	malloc(sizeof(OfsCylinder) * ofsPtr->headerPtr->dataCylinders);

    /*
     * Now go through the bit map finding all of the fragments and putting
     * them onto the appropriate lists.  Also determine cylinder information.
     */
    bitmapPtr = ofsPtr->dataBlockBitmap;
    cylinderPtr = ofsPtr->cylinders;
    for (i = 0; i < ofsPtr->headerPtr->dataCylinders; i++, cylinderPtr++) {
	cylinderPtr->blocksFree = 0;
	for (j = 0; j < bitmapBytes; j++, bitmapPtr++) {
	    if (UpperBlockFree(*bitmapPtr)) {
		cylinderPtr->blocksFree++;
	    } else {
		fragOffsetPtr = fragTable[GetUpperFragMask(*bitmapPtr)];
		for (k = 0; k < OFS_NUM_FRAG_SIZES; k++, fragOffsetPtr++) {
		    if (*fragOffsetPtr != -1) {
			fragPtr = (OfsFragment *) malloc(sizeof(OfsFragment));
			List_Insert((List_Links *) fragPtr, 
				    LIST_ATREAR(ofsPtr->fragLists[k]));
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
		for (k = 0; k < OFS_NUM_FRAG_SIZES; k++, fragOffsetPtr++) {
		    if (*fragOffsetPtr != -1) {
			fragPtr = (OfsFragment *) malloc(sizeof(OfsFragment));
			List_Insert((List_Links *) fragPtr, 
				    LIST_ATREAR(ofsPtr->fragLists[k]));
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
 * Ofs_BlockAllocate --
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
Ofs_BlockAllocate(domainPtr, handlePtr, offset, numBytes, flags, blockAddrPtr,
		newBlockPtr)
    Fsdm_Domain		*domainPtr;	/* Domain of file. */
    register Fsio_FileIOHandle *handlePtr;	/* Local file handle. */
    int 		offset;		/* Offset to allocate at. */
    int 		numBytes;	/* Number of bytes to allocate. */
    int			flags;		/* FSCACHE_DONT_BLOCK */
    int			*blockAddrPtr; 	/* Disk address of block allocated. */
    Boolean		*newBlockPtr;	/* TRUE if there was no block allocated
					 * before. */
{
    Ofs_Domain			*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    register Fsdm_FileDescriptor	*descPtr;
    register int		blockNum;
    OfsBlockIndexInfo		indexInfo;
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
	panic("OfsFileAllocate: Trying to allocate more than one block\n");
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
	    status = FragToBlock(ofsPtr, handlePtr, curLastBlock, flags);
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
	status = OfsGetFirstIndex(ofsPtr, handlePtr, blockNum, &indexInfo,
				 OFS_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	/*
	 * This is not the first block in the file, so determine the
	 * previous block and then go to the first block.
	 */
	status = OfsGetFirstIndex(ofsPtr, handlePtr, blockNum - 1, &indexInfo,
				 OFS_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
	status = OfsGetNextIndex(handlePtr, &indexInfo, FALSE);
	if (status != SUCCESS) {
	    OfsEndIndex(handlePtr, &indexInfo, FALSE);
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

    OfsEndIndex(handlePtr, &indexInfo, dirtiedIndex);

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
    descPtr->descModifyTime = Fsutil_TimeInSeconds();
    descPtr->flags |= (FSDM_FD_INDEX_DIRTY|FSDM_FD_SIZE_DIRTY);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Ofs_FileTrunc --
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
/*ARGSUSED*/
ReturnStatus
Ofs_FileTrunc(domainPtr, handlePtr, size, delete)
    Fsdm_Domain		*domainPtr;
    Fsio_FileIOHandle	*handlePtr;	/* File to truncate. */
    int			size;		/* Size to truncate the file to. */
    Boolean		delete;		/* TRUE if Truncate for delete. */
{
    register Ofs_Domain	 *ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    register Fsdm_FileDescriptor 	*descPtr;
    int				firstBlock;
    int				firstFrag = 0;
    int				lastBlock;
    int				lastFrag;
    ReturnStatus		status = SUCCESS;
    OfsBlockIndexInfo		indexInfo;
    int				newLastByte;
    int				savedLastByte;
    int				flags;
    Boolean			dirty = FALSE;
    int				fragsToFree;

    descPtr = handlePtr->descPtr;

    savedLastByte = descPtr->lastByte;

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
	    descPtr->descModifyTime = Fsutil_TimeInSeconds();
	    descPtr->flags |= FSDM_FD_SIZE_DIRTY;
	}
	status = SUCCESS;
	goto exit;
    }

    flags = OFS_DELETE_INDIRECT_BLOCKS;
    if (newLastByte == -1) {
	flags |= OFS_DELETE_EVERYTHING;
    }

    /*
     * Loop through the blocks deleting them.
     */
    status = OfsGetFirstIndex(ofsPtr, handlePtr, firstBlock, &indexInfo, flags);
    if (status != SUCCESS) {
	printf( "Ofs_FileTrunc: Status %x setting up index\n",
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
		OfsFragFree(ofsPtr, lastFrag + 1, 
		    (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK),
		    *indexInfo.blockAddrPtr & FRAG_OFFSET_MASK);
		descPtr->numKbytes -= lastFrag + 1;
	    } else {
		OfsBlockFree(ofsPtr, 
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
	    if (fragsToFree > 0) {
		OfsFragFree(ofsPtr, fragsToFree,
		      (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK),
			    (*indexInfo.blockAddrPtr & FRAG_OFFSET_MASK)
			     + firstFrag + 1);
		descPtr->numKbytes -= fragsToFree;
	    }
	} else if (indexInfo.blockNum >= FSDM_NUM_DIRECT_BLOCKS || 
	           indexInfo.blockNum < lastBlock || lastFrag == LAST_FRAG) {
	    /*
	     * This is a full block so delete it.
	     */
	    OfsBlockFree(ofsPtr, 
		 (int) (*indexInfo.blockAddrPtr / FS_FRAGMENTS_PER_BLOCK));
	    descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
	    *indexInfo.blockAddrPtr = FSDM_NIL_INDEX;
	} else {
	    /*
	     * Delete a fragment.  Only get here if are on the last block in 
	     * the file.
	     */
	    OfsFragFree(ofsPtr, lastFrag + 1, 
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

	status = OfsGetNextIndex(handlePtr, &indexInfo, dirty);
	if (status != SUCCESS) {
	    printf( "Ofs_FileTrunc: Could not truncate file.\n");
	    OfsEndIndex(handlePtr, &indexInfo, FALSE);
	    goto exit;
	}
    }

    OfsEndIndex(handlePtr, &indexInfo, dirty);

    descPtr->lastByte = newLastByte;
    descPtr->descModifyTime = Fsutil_TimeInSeconds();
    descPtr->flags |= FSDM_FD_SIZE_DIRTY;

exit:
    /*
     * Make sure we really deleted the file if size is zero.
     */
    if (size == 0) {
	register int index;

	for (index=0 ; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	    if (descPtr->direct[index] != FSDM_NIL_INDEX) {
		printf("Ofs_FileTrunc abandoning (direct) block %d in <%d,%d> \"%s\" savedLastByte %d\n",
		    descPtr->direct[index],
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName((Fs_HandleHeader *)handlePtr),
		    savedLastByte);
		descPtr->direct[index] = FSDM_NIL_INDEX;
	    }
	}
	for (index = 0 ; index <= 2 ; index++) {
	    if (descPtr->indirect[index] != FSDM_NIL_INDEX) {
		printf("Ofs_FileTrunc abandoning (indirect) block %d in <%d,%d> \"%s\" savedLastByte %d\n",
		    descPtr->indirect[index], 
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName((Fs_HandleHeader *)handlePtr),
		    savedLastByte);
		descPtr->indirect[index] = FSDM_NIL_INDEX;
	    }
	}
    }
    return(status);
}




/*
 *----------------------------------------------------------------------
 *
 * OfsWriteBackDataBlockBitmap --
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
OfsWriteBackDataBlockBitmap(ofsPtr)
    register	Ofs_Domain	*ofsPtr;	/* Domain for which to write 
						 * back the bitmap. */
{
    ReturnStatus	status;

    LOCK_MONITOR;

    status = OfsDeviceBlockIO(ofsPtr, FS_WRITE, 
		    ofsPtr->headerPtr->bitmapOffset * 4, 
		    ofsPtr->headerPtr->bitmapBlocks * 4,
		    (Address) ofsPtr->dataBlockBitmap);
    if (status != SUCCESS) {
	printf( "OfsWriteBackDataBlockBitmap: Could not write out data block bitmap.\n");
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * OfsWriteBackSummaryInfo --
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
OfsWriteBackSummaryInfo(ofsPtr)
    register	Ofs_Domain	*ofsPtr;	/* Domain for which to write 
						 * back the bitmap. */
{
    ReturnStatus	status;
    Fs_IOParam		io;
    Fs_IOReply		reply;

    LOCK_MONITOR;

    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    io.buffer = (Address)ofsPtr->summaryInfoPtr;
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = ofsPtr->summarySector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(ofsPtr->headerPtr->device.type)].write)
		(&ofsPtr->headerPtr->device, &io, &reply); 
    if (status != SUCCESS) {
	printf("OfsWriteBackSummaryInfo: Could not write out summary info.\n");
    }
    if (status == GEN_NO_PERMISSION) {
	printf("OfsWriteBackSummaryInfo: Disk is write-protected.\n");
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
SelectCylinderInt(hashSeed, ofsPtr, cylinderNum)
    int				hashSeed;	/* Seed for the hash, usually
						 * the file number. */
    register	Ofs_Domain	*ofsPtr;	/* Domain to select cylinder 
						 * from. */
    int				cylinderNum;	/* Cylinder to try first, -1
						 * if no preferred cylinder. */
{
    register	int		i;
    register	OfsCylinder	*cylinderPtr;

    if (cylinderNum == -1) {
	/*
	 * Do a random hash to find the starting point to allocate at.
	 */
	fs_Stats.alloc.cylHashes++;
	cylinderNum = ((hashSeed * 1103515245 + 12345) & 0x7fffffff) %
			    ofsPtr->headerPtr->dataCylinders;
    }

    /*
     * Search forward starting at the desired cylinder.
     */

    for (i = cylinderNum, cylinderPtr = &(ofsPtr->cylinders[cylinderNum]); 
	 i < ofsPtr->headerPtr->dataCylinders;
	 i++, cylinderPtr++) {
	fs_Stats.alloc.cylsSearched++;
	if (cylinderPtr->blocksFree > 0) {
	    ofsPtr->cylinders[i].blocksFree--;
	    return(i);
	}
    }

    /*
     * No block forward from desired cylinder so search backward.
     */

    for (i = cylinderNum - 1, 
		cylinderPtr = &(ofsPtr->cylinders[cylinderNum - 1]);
	 i >= 0;
	 i--, cylinderPtr--) {
	fs_Stats.alloc.cylsSearched++;
	if (cylinderPtr->blocksFree > 0) {
	    ofsPtr->cylinders[i].blocksFree--;
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
FindBlockInt(hashSeed, ofsPtr, nearBlock, allocate, blockNumPtr, 
	     bitmapPtrPtr)
    int			hashSeed;	/* Seed for cylinder hash. */
    register Ofs_Domain  *ofsPtr; 	/* Domain to allocate blocks in. */
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

    blocksPerCylinder = ofsPtr->headerPtr->geometry.blocksPerCylinder;
    if (nearBlock != -1) {
	cylinderNum = ((unsigned int) nearBlock) / blocksPerCylinder;
	startingBlockOffset = ((unsigned int) nearBlock) % blocksPerCylinder;
    } else {
	cylinderNum = -1;
	startingBlockOffset = -1;
    }
    cylinderNum = SelectCylinderInt(hashSeed, ofsPtr, cylinderNum);
    if (cylinderNum == -1) {
	*blockNumPtr = -1;
	return;
    }
    if (ofs_AllocGap == 0) {
	/*
	 * CONTIGUOUS allocation.
	 * This is the original code that simply starts at the beginning
	 * of the cylinder and stops when it finds a free block.
	 */
	bitmapPtr = 
	  &(ofsPtr->dataBlockBitmap[cylinderNum * ofsPtr->bytesPerCylinder]);
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
	    block = startingBlockOffset + ofs_AllocGap;
	} else {
	    block = 0;
	    startingBlockOffset = blocksPerCylinder;
	}
	bitmapPtr = 
		&(ofsPtr->dataBlockBitmap[cylinderNum * ofsPtr->bytesPerCylinder
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
		 bitmapPtr -= ofsPtr->bytesPerCylinder;
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
	UNLOCK_MONITOR;
	panic("FindBlockInt: no block\n");
	*blockNumPtr = -1;
	ofs_AllocGap = CONTIGUOUS;
	LOCK_MONITOR;
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
 * OfsBlockFind --
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
OfsBlockFind(hashSeed, ofsPtr, nearBlock, allocate, blockNumPtr, bitmapPtrPtr)
    int			hashSeed;	/* Seed for cylinder hash. */
    Ofs_Domain 	*ofsPtr; 	/* Domain to allocate blocks in . */
    int			nearBlock;  	/* Block number where this block should
					 * be near. */
    Boolean		allocate;   	/* TRUE if allocating full block, FALSE
					 * if intend to fragment block. */
    int			*blockNumPtr;  	/* Block number that was found. */
    unsigned char	**bitmapPtrPtr;	/* Bit map entry that corresponds to
					 * the block. */
{
    LOCK_MONITOR;

    if (ofsPtr->summaryInfoPtr->numFreeKbytes - FS_FRAGMENTS_PER_BLOCK <
	    ofsPtr->minKFree) {
	*blockNumPtr = -1;
	UNLOCK_MONITOR;
	return;
    }

    FindBlockInt(hashSeed, ofsPtr, nearBlock, allocate, blockNumPtr, 
		 bitmapPtrPtr);
    if (*blockNumPtr != -1) {
	ofsPtr->summaryInfoPtr->numFreeKbytes -= FS_FRAGMENTS_PER_BLOCK;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * OfsBlockFree --
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
OfsBlockFree(ofsPtr, blockNum)
    register Ofs_Domain *ofsPtr; 	/* Handle for file to alloc blocks 
					 * for. */
    int			  blockNum;   	/* Block number to free. */
{
    register unsigned char *bitmapPtr;
    int			   cylinderNum;
    register int	   mask;
    register int	   checkMask;

    LOCK_MONITOR;

    ofsPtr->summaryInfoPtr->numFreeKbytes += FS_FRAGMENTS_PER_BLOCK;
    fs_Stats.alloc.blocksFreed++;
    bitmapPtr = GetBitmapPtr(ofsPtr, blockNum);
    cylinderNum = (unsigned int) blockNum / 
			ofsPtr->headerPtr->geometry.blocksPerCylinder;
    ofsPtr->cylinders[cylinderNum].blocksFree++;
    if ((blockNum % ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	mask = 0xf0;
    } else {
	mask = 0x0f;
    }
    checkMask = ~mask & 0xff;
    if ((*bitmapPtr & checkMask) != checkMask) {
	printf("bitmap = <%x>, checkMask = <%x>\n",
			   *bitmapPtr & 0xff, checkMask & 0xff);
        UNLOCK_MONITOR;
	printf("OfsBlockFree free block %d\n", blockNum);
	return;
    } else {
	*bitmapPtr &= mask;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * OfsFragFind --
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
OfsFragFind(hashSeed, ofsPtr, numFrags, lastFragBlock, lastFragOffset, 
	    lastFragSize, newFragBlockPtr, newFragOffsetPtr)
    int			hashSeed;		/* Seed for cylinder hash. */
    register Ofs_Domain *ofsPtr;		/* Domain out of which to 
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
    register OfsFragment	  	*fragPtr;
    register unsigned char 	*bitmapPtr = (unsigned char *) NIL;
    register int	   	*savedOffsets;
    unsigned char		savedBitmap;
    int			   	*fragOffsetPtr;
    int			   	fragOffset = 0;
    int			   	fragBlock;
    unsigned char 	   	*tBitmapPtr;
    List_Links	   	   	*fragList;
    int			   	byteOffset;
    int			   	fragMask = 0;
    int			   	i;
    int			   	blockNum = -1;
    int				fragsToAllocate;

    LOCK_MONITOR;

    if (lastFragBlock == -1)  {
	fragsToAllocate = numFrags;
    } else {
	fragsToAllocate = numFrags - lastFragSize;
    }
    if (ofsPtr->summaryInfoPtr->numFreeKbytes - fragsToAllocate <
	    ofsPtr->minKFree) {
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
	bitmapPtr = GetBitmapPtr(ofsPtr, lastFragBlock);
	if ((lastFragBlock % ofsPtr->headerPtr->geometry.blocksPerCylinder)
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
	     i < OFS_NUM_FRAG_SIZES && blockNum == -1; 
	     i++) {
	    fragList = ofsPtr->fragLists[i];
	    while (!List_IsEmpty(fragList)) {
		fragPtr = (OfsFragment *) List_First(fragList);
		List_Remove((List_Links *) fragPtr);
		fragBlock = fragPtr->blockNum;
		free((Address) fragPtr);
		/*
		 * Check to make sure that there really is a fragment of the
		 * needed size in the block.  These fragment lists are hints.
		 */
		bitmapPtr = GetBitmapPtr(ofsPtr, fragBlock);
		if ((fragBlock %
		     ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
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
	    FindBlockInt(hashSeed, ofsPtr, -1, FALSE, &blockNum, 
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
	if ((blockNum % ofsPtr->headerPtr->geometry.blocksPerCylinder)
		& 0x1) {
	    byteOffset = fragOffset + 4;
	} else {
	    byteOffset = fragOffset;
	}
	fragMask = 0;
	for (i = byteOffset; i < byteOffset + numFrags; i++) {
	    fragMask |= bitMasks[i];
	}
	ofsPtr->summaryInfoPtr->numFreeKbytes -= numFrags;
    } else {
	ofsPtr->summaryInfoPtr->numFreeKbytes -= numFrags - lastFragSize;
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
    if ((blockNum % ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	savedOffsets = fragTable[GetLowerFragMask(savedBitmap)];
	fragOffsetPtr = fragTable[GetLowerFragMask(*bitmapPtr)];
    } else {
	savedOffsets = fragTable[GetUpperFragMask(savedBitmap)];
	fragOffsetPtr = fragTable[GetUpperFragMask(*bitmapPtr)];
    }
    for (i = 0; i < OFS_NUM_FRAG_SIZES; i++, savedOffsets++, fragOffsetPtr++) {
	if (*savedOffsets == -1 && *fragOffsetPtr != -1) {
	    /*
	     * The block was not on the fragment list of this size before we
	     * allocated a new fragment out of it, so put it there. 
	     */
	    fragPtr = (OfsFragment *) malloc(sizeof(OfsFragment));
	    List_Insert((List_Links *) fragPtr, 
			LIST_ATREAR(ofsPtr->fragLists[i]));
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
 * OfsFragFree --
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
OfsFragFree(ofsPtr, numFrags, fragBlock, fragOffset) 
    register Ofs_Domain *ofsPtr;	/* Domain out of which to allocate the
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
    OfsFragment		   *fragPtr;
    unsigned char 	    mask;
    int		            i;
    int		            byteOffset;
    int			    fragMask;

    LOCK_MONITOR;

    fs_Stats.alloc.fragsFreed++;

    ofsPtr->summaryInfoPtr->numFreeKbytes += numFrags;

    bitmapPtr = GetBitmapPtr(ofsPtr, fragBlock);

    /*
     * Determine whether should clear upper or lower 4 bits.
     */

    if ((fragBlock % ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
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
	printf("OfsFragFree: bitmap = <%x>, checkMask = <%x>\n",
			   *bitmapPtr & 0xff, mask & 0xff);
	UNLOCK_MONITOR;
	printf("OfsFragFree: block not free, block %d, numFrag %d, offset %d\n",
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

    if ((fragBlock % ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
	fragMask = GetLowerFragMask(*bitmapPtr);
    } else {
	fragMask = GetUpperFragMask(*bitmapPtr);
    }
    if (fragMask == 0) {
	fs_Stats.alloc.fragToBlock++;
	/*
	 * The block has become totally free.
	 */
	ofsPtr->cylinders[(unsigned int) fragBlock / 
	     ofsPtr->headerPtr->geometry.blocksPerCylinder].blocksFree++;
	UNLOCK_MONITOR;
	return;
    }
    fragOffsets = fragTable[fragMask];
    for (i = 0; i < OFS_NUM_FRAG_SIZES; i++, fragOffsets++, savedOffsets++) {
	if (*savedOffsets == -1 && *fragOffsets != -1) {
	    /*
	     * A fragment of this size did not exist before we freed the 
	     * fragment but it does exist now.
	     */
	    fragPtr = (OfsFragment *) malloc(sizeof(OfsFragment));
	    List_Insert((List_Links *) fragPtr, 
				LIST_ATREAR(ofsPtr->fragLists[i]));
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
OnlyFrag(ofsPtr, numFrags, fragBlock, fragOffset) 
    register Ofs_Domain *ofsPtr;	/* Domain of fragment. */
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

    bitmapPtr = GetBitmapPtr(ofsPtr, fragBlock);

    /*
     * Determine whether should access upper or lower 4 bits.
     */
    if ((fragBlock % ofsPtr->headerPtr->geometry.blocksPerCylinder) & 0x1) {
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
    ofsPtr->summaryInfoPtr->numFreeKbytes -= 
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
UpgradeFragment(ofsPtr, handlePtr, indexInfoPtr, curLastBlock, newLastFrag, 
		dontWriteThru, dontBlock, dirtiedIndexPtr)
    Ofs_Domain		*ofsPtr;		/* Domain of file. */
    Fsio_FileIOHandle		*handlePtr;	/* File to allocate blocks 
						 * for. */
    register OfsBlockIndexInfo *indexInfoPtr;	/* Index info structure. */
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
	OfsFragFind(handlePtr->hdr.fileID.minor, ofsPtr, newLastFrag + 1, 
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
	OfsBlockFind(handlePtr->hdr.fileID.minor, ofsPtr,
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
	status = OfsDeviceBlockIO(ofsPtr, FS_READ, 
		   blockAddr +
		   ofsPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		   curLastFrag + 1, fragCacheBlockPtr->blockAddr);
	if (status != SUCCESS) {
	    Fscache_UnlockBlock(fragCacheBlockPtr, 0, -1, 0, 0);
	    OfsFragFree(ofsPtr, newLastFrag + 1, 
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
    blockAddr = newFragBlock * FS_FRAGMENTS_PER_BLOCK + newFragOffset;
    Fscache_UnlockBlock(fragCacheBlockPtr, (unsigned) Fsutil_TimeInSeconds(), 
		       blockAddr, 
		       (newLastFrag + 1) * FS_FRAGMENT_SIZE, flags);

    *(indexInfoPtr->blockAddrPtr) = blockAddr;
    descPtr->numKbytes += newLastFrag - curLastFrag;
    descPtr->flags |= FSDM_FD_SIZE_DIRTY;
    *dirtiedIndexPtr = TRUE;

    OfsFragFree(ofsPtr, curLastFrag + 1, curFragBlock, curFragOffset);

exit:
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
    register OfsBlockIndexInfo 	*indexInfoPtr; 	/* Index info structure. */
    int				newLastByte;	/* The new last byte in the 
						 * file. */
    int				curLastBlock;	/* The last block in the file 
						 * before started allocating. */
    int				dontBlock;	/* FSCACHE_DONT_BLOCK */
    Boolean			*dirtiedIndexPtr;/* TRUE if a new block was 
						  * allocated. */
{
    register	int		 blockAddr;
    register	Ofs_Domain	 *ofsPtr;
    unsigned char 		 *bitmapPtr;
    int				 newFragIndex;	/* {0, 1, 2, 3} */
    int				 blockNum;	/* Disk block that is 
						 * allocated. */
    int				 newFragOffset;	/* Offset in disk block where
						 * fragment begins. */
    ReturnStatus		 status = SUCCESS;

    ofsPtr = indexInfoPtr->ofsPtr;
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
	    OfsFragFind(handlePtr->hdr.fileID.minor, ofsPtr,
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
		OfsBlockFind(handlePtr->hdr.fileID.minor, ofsPtr,
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
	status = UpgradeFragment(ofsPtr, handlePtr, indexInfoPtr, 
				 curLastBlock, newFragIndex, TRUE,
				 dontBlock, dirtiedIndexPtr);
    }
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
FragToBlock(ofsPtr, handlePtr, blockNum, dontBlock)
    Ofs_Domain			*ofsPtr;
    register Fsio_FileIOHandle	*handlePtr;
    int				blockNum;
    int				dontBlock;	/* FSCACHE_DONT_BLOCK */
{
    register Fsdm_FileDescriptor	*descPtr;
    OfsBlockIndexInfo		indexInfo;
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
	status = OfsGetFirstIndex(ofsPtr, handlePtr, blockNum, &indexInfo,
				 OFS_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	/*
	 * This is not the first block in the file, so determine the
	 * previous block and then go to the first block.
	 */
	status = OfsGetFirstIndex(ofsPtr, handlePtr, blockNum - 1, &indexInfo,
				 OFS_ALLOC_INDIRECT_BLOCKS);
	if (status != SUCCESS) {
	    return(status);
	}
	status = OfsGetNextIndex(handlePtr, &indexInfo, FALSE);
	if (status != SUCCESS) {
	    OfsEndIndex(handlePtr, &indexInfo, FALSE);
	    return(status);
	}
    }

    /*
     * Now upgrade to a full block.
     */

    status = UpgradeFragment(ofsPtr, handlePtr, &indexInfo, blockNum, LAST_FRAG,
			     FALSE, dontBlock, &dirtiedIndex);
    if (status == SUCCESS) {
	descPtr->lastByte = blockNum * FS_BLOCK_SIZE + FS_BLOCK_SIZE - 1;
	descPtr->descModifyTime = Fsutil_TimeInSeconds();
	descPtr->flags |= FSDM_FD_SIZE_DIRTY;
    }
    OfsEndIndex(handlePtr, &indexInfo, dirtiedIndex);
    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * Ofs_DomainInfo --
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
Ofs_DomainInfo(domainPtr, domainInfoPtr)
    Fsdm_Domain	*domainPtr;
    Fs_DomainInfo	*domainInfoPtr;
{
    Ofs_Domain	*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);

    domainInfoPtr->maxKbytes = 
		    ofsPtr->headerPtr->dataBlocks * FS_FRAGMENTS_PER_BLOCK;
    domainInfoPtr->freeKbytes = ofsPtr->summaryInfoPtr->numFreeKbytes;
    domainInfoPtr->maxFileDesc = ofsPtr->headerPtr->numFileDesc;
    domainInfoPtr->freeFileDesc = ofsPtr->summaryInfoPtr->numFreeFileDesc;
    domainInfoPtr->blockSize = FS_BLOCK_SIZE;
    domainInfoPtr->optSize = FS_BLOCK_SIZE;

    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * Ofs_ReallocBlock --
 *
 *	Allocate a new block on disk to replace the given block.  This is
 *	intended to be used by the cache when it can't write out a block
 *	because of a disk error.
 *
 * Results:
 * 	None
 *
 * Side effects:
 *	The descriptor or indirect blocks are modified to point to the newly
 *	allocated block.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Ofs_ReallocBlock(data, callInfoPtr)
    ClientData		data;			/* Block to move */
    Proc_CallInfo	*callInfoPtr;		/* Not used. */
{
    Fscache_Block 	*blockPtr = (Fscache_Block *) data;
    Fscache_FileInfo    *cacheInfoPtr = blockPtr->cacheInfoPtr;
    Fsio_FileIOHandle	*handlePtr = (Fsio_FileIOHandle *) cacheInfoPtr->hdrPtr;
    int			virtBlockNum, physBlockNum;
    OfsBlockIndexInfo	indexInfo;
    Fsdm_Domain		*domainPtr;
    int			newBlockNum = -1;
    Boolean		dirtiedIndex = FALSE;
    Boolean		setupIndex = FALSE;
    unsigned char	*bitmapPtr;
    ReturnStatus	status;
    Fsdm_FileDescriptor	*descPtr;
    Ofs_Domain		*ofsPtr;

    virtBlockNum = blockPtr->blockNum;
    physBlockNum = blockPtr->diskBlock;
    if (handlePtr->hdr.fileID.minor == 0) {
	/*
	 * This is a descriptor block.
	 */
	printf(
	    "OfsBlockRealloc: Bad descriptor block.  Domain=%d block=%d\n",
		  handlePtr->hdr.fileID.major, physBlockNum);
	goto error1;
    }

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	goto error;

    }
    ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    Fsutil_HandleLock((Fs_HandleHeader *)handlePtr);
    descPtr = handlePtr->descPtr;
    if (virtBlockNum >= 0) {
	int		bytesInBlock;
	/*
	 * A normal data block.
	 */
	status = OfsGetFirstIndex(ofsPtr, handlePtr, virtBlockNum, &indexInfo, 0);
	if (status != SUCCESS) {
	    printf( 
	       "OfsBlockRealloc: Setup index (1) failed status <%x>\n", status);
	    goto error;
	}
	setupIndex = TRUE;
	if (*indexInfo.blockAddrPtr != physBlockNum) {
	    panic("OfsBlockRealloc: Bad physical block num.\n");
	}
	bytesInBlock = descPtr->lastByte - virtBlockNum * FS_BLOCK_SIZE + 1;
	if (bytesInBlock > FS_FRAGMENT_SIZE * (FS_FRAGMENTS_PER_BLOCK - 1) ||
	    virtBlockNum >= FSDM_NUM_DIRECT_BLOCKS) {
	    /* 
	     * Have a full block.
	     */
	    OfsBlockFind(handlePtr->hdr.fileID.minor, ofsPtr,
			physBlockNum / FS_FRAGMENTS_PER_BLOCK, TRUE,
			&newBlockNum, &bitmapPtr);
	    if (newBlockNum == -1) {
		printf( "FsdmBlockRealloc: No disk space (1)\n");
		goto error;
	    }
	    newBlockNum *= FS_FRAGMENTS_PER_BLOCK;
	    *indexInfo.blockAddrPtr = newBlockNum;
	    dirtiedIndex = TRUE;
	    descPtr->flags |= FSDM_FD_INDEX_DIRTY;
	    PutInBadBlockFile(handlePtr, ofsPtr, physBlockNum);
	} else {
	    int	newFragOffset;
	    int	numFrags;
	    /*
	     * Have a fragment.
	     */
	    numFrags = (bytesInBlock - 1) / FS_FRAGMENT_SIZE + 1;
	    OfsFragFind(handlePtr->hdr.fileID.minor, ofsPtr, numFrags,
			-1, -1, -1, &newBlockNum, &newFragOffset);
	    if (newBlockNum == -1) {
		printf( "FsdmBlockRealloc: No disk space (2)\n");
		goto error;
	    }
	    newBlockNum = newBlockNum * FS_FRAGMENTS_PER_BLOCK + newFragOffset;
	    *indexInfo.blockAddrPtr = newBlockNum;
	    dirtiedIndex = TRUE;
	    descPtr->flags |= FSDM_FD_INDEX_DIRTY;
	    if (OnlyFrag(ofsPtr, numFrags,
			   physBlockNum / FS_FRAGMENTS_PER_BLOCK,
			   physBlockNum & FRAG_OFFSET_MASK)) {
		PutInBadBlockFile(handlePtr, ofsPtr,
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
	    panic("OfsBlockRealloc: Can't find indirect block\n");
	    blockAddrPtr = (int *) NIL;
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
		status = OfsDeviceBlockIO(ofsPtr, FS_READ,
		       descPtr->indirect[1], FS_FRAGMENTS_PER_BLOCK, 
		       blockPtr->blockAddr);
		if (status != SUCCESS) {
		    printf( 
	"OfsBlockRealloc: Could not read doubly indirect block, status <%x>\n", 
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
	    panic("OfsBlockRealloc: Bad phys addr for indirect block (2)\n");
	}
	/*
	 * Allocate a new indirect block.
	 */
	OfsBlockFind(handlePtr->hdr.fileID.minor, ofsPtr, -1, TRUE, 
		    &newBlockNum, &bitmapPtr);
	if (newBlockNum == -1) {
	    printf( "FsdmBlockRealloc: No disk space (3)\n");
	    goto error;
	}
	newBlockNum = (newBlockNum + ofsPtr->headerPtr->dataOffset) * 
			FS_FRAGMENTS_PER_BLOCK;
	*blockAddrPtr = newBlockNum;
	if (blockPtr == (Fscache_Block *)NIL) {
	    descPtr->flags |= FSDM_FD_INDEX_DIRTY;
	} else {
	    Fscache_UnlockBlock(blockPtr, (unsigned int)Fsutil_TimeInSeconds(), 
			       -(descPtr->indirect[1]), FS_BLOCK_SIZE, 0);
	}
	PutInBadBlockFile(handlePtr, ofsPtr,
			  physBlockNum - FS_FRAGMENTS_PER_BLOCK * 
				     ofsPtr->headerPtr->dataOffset);
	newBlockNum = -newBlockNum;
    }

error:

    if (setupIndex) {
	OfsEndIndex(handlePtr, &indexInfo, dirtiedIndex);
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    Fsutil_HandleUnlock((Fs_HandleHeader *)handlePtr);
error1:
    FscacheFinishRealloc(blockPtr, newBlockNum);
    return;
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
PutInBadBlockFile(handlePtr, ofsPtr, blockNum)
    Fsio_FileIOHandle	*handlePtr;	/* File which owned bad block. */
    Ofs_Domain	*ofsPtr;		/* Pointer to domain. */
    int		blockNum;	/* Block number to put in bad block file. */
{
    Fs_FileID		fileID;
    Fsio_FileIOHandle	*badBlockHandlePtr;
    Fsdm_FileDescriptor	*descPtr;
    OfsBlockIndexInfo	indexInfo;
    ReturnStatus	status;
    int			lastBlock;

    fileID.serverID = rpc_SpriteID;
    fileID.type = FSIO_LCL_FILE_STREAM;
    fileID.major = handlePtr->hdr.fileID.major;
    fileID.minor = OFS_BAD_BLOCK_FILE_NUMBER;
    badBlockHandlePtr = (Fsio_FileIOHandle *)Fsutil_HandleFetch(&fileID);
    if (badBlockHandlePtr == (Fsio_FileIOHandle *)NIL) {
	/*
	 * Have to make a new handle since we don't have one for this domain
	 * in memory.
	 */
	status = Fsio_LocalFileHandleInit(&fileID, "BadBlockFile",
			(Fsdm_FileDescriptor *) NIL, FALSE, 
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
    status = OfsGetFirstIndex(ofsPtr, handlePtr, lastBlock + 1, &indexInfo,
			     OFS_ALLOC_INDIRECT_BLOCKS);
    if (status != SUCCESS) {
	printf( "PutInBadBlockFile: Could not fetch index\n");
    } else {
	*indexInfo.blockAddrPtr = blockNum;
	descPtr->lastByte += FS_BLOCK_SIZE;
	descPtr->flags |= (FSDM_FD_INDEX_DIRTY|FSDM_FD_SIZE_DIRTY);
	descPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
	OfsEndIndex(handlePtr, &indexInfo, TRUE);
    }

    Fsutil_HandleUnlock((Fs_HandleHeader *)badBlockHandlePtr);
}
