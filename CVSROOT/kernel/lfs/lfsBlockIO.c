/* 
 * lfsBlockRead.c --
 *
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

#include "sprite.h"
#include "lfs.h"
#include "lfsInt.h"
#include "fs.h"
#include "fsutil.h"
#include "fsio.h"
#include "fsioFile.h"
#include "fslcl.h"
#include "fscache.h"
#include "fsdm.h"
#include "fsStat.h"



/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileBlockRead --
 *
 *	Read in a cache block.  This does a direct disk read if the
 *	file is the 'physical file' used for file descriptors and
 *	indirect blocks.  If it is a regular file data block, then
 *	the indexing structure is used to locate the file on disk.
 *	This always attempts to read in a full block, but will read
 *	less if at the last block and it isn't full.  In this case,
 *	the remainder of the cache block is zero-filled.
 *
 * Results:
 *	The results of the disk read.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the bufSize parameter.  The blockPtr->blockSize is modified to
 *	reflect how much data was actually read in.  The unused part
 *	of the block is filled with zeroes so that higher levels can
 *	always assume the block has good stuff in all parts of it.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Lfs_FileBlockRead(domainPtr, handlePtr, blockPtr)
    Fsdm_Domain		*domainPtr;	/* Domain of file. */
    register Fsio_FileIOHandle *handlePtr;	/* Handle on a local file. */
    Fscache_Block       *blockPtr;      /* Cache block to read in.  This assumes
                                         * the blockNum, blockAddr (buffer area)
                                         * and blockSize are set.  This modifies
                                         * blockSize if less bytes were read
                                         * because of EOF. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    register	Fsdm_FileDescriptor *descPtr;
    register			 offset;
    register int		 numBytes;
    int				diskAddress;
    ReturnStatus		 status;

    status = SUCCESS;
    blockPtr->blockSize = 0;
    numBytes = FS_BLOCK_SIZE;
    offset = blockPtr->blockNum * FS_BLOCK_SIZE;

    /*
     * Is a logical file read. Round the size down to the actual
     * last byte in the file.
     */

    descPtr = handlePtr->descPtr;
    if (offset > descPtr->lastByte) {
	goto exit;
    } else if (offset + numBytes - 1 > descPtr->lastByte) {
	numBytes = descPtr->lastByte - offset + 1;
    }

    status = LfsFile_GetIndex(handlePtr, offset / FS_BLOCK_SIZE, FALSE,
			     &diskAddress);
    if (status != SUCCESS) {
	printf("Lfs_FileBlockRead: Could not setup indexing\n");
	goto exit;
    }

    if (diskAddress != FSDM_NIL_INDEX) {
	/*
	 * Read in the block.  Specify the device, the fragment index,
	 * the number of fragments, and the memory buffer.
	 */
	int	numFrag = (numBytes - 1) / FS_FRAGMENT_SIZE + 1;
	LfsCheckRead(lfsPtr, diskAddress, numBytes);
	status = LfsReadBytes(lfsPtr, diskAddress, 
			numFrag * FS_FRAGMENT_SIZE,  blockPtr->blockAddr);
    } else {
	/*
	 * Zero fill the block.  We're in a 'hole' in the file.
	 */
#ifdef STATS
	fs_Stats.blockCache.readZeroFills++;
#endif
	bzero(blockPtr->blockAddr, numBytes);
    }
#ifdef STATS
    Fs_StatAdd(numBytes, fs_Stats.gen.fileBytesRead,
	       fs_Stats.gen.fileReadOverflow);
#endif
exit:
    /*
     * Define the block size and error fill leftover space.
     */
    if (status == SUCCESS) {
	blockPtr->blockSize = numBytes;
    }
    if (blockPtr->blockSize < FS_BLOCK_SIZE) {
#ifdef STATS
	fs_Stats.blockCache.readZeroFills++;
#endif
	bzero(blockPtr->blockAddr + blockPtr->blockSize,
		FS_BLOCK_SIZE - blockPtr->blockSize);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileBlockWrite --
 *
 *      Write out a cache block.  This understands about physical
 *      block writes as opposed to file block writes, and it understands
 *      that negative block numbers are used for indirect blocks (gag).
 *      Physical blocks are numbered from the beginning of the disk,
 *      and they are used for file descriptors and indirect blocks.
 *      File blocks are numbered from the beginning of the data block
 *      area, so an offset must be used to calculate their true address.
 *
 * Results:
 *      The return code from the driver, or FS_DOMAIN_UNAVAILABLE if
 *      the domain has been un-attached.
 *
 * Side effects:
 *      The device write.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Lfs_FileBlockWrite(domainPtr, handlePtr, blockPtr)
    Fsdm_Domain	 *domainPtr;
    Fsio_FileIOHandle *handlePtr;	/* I/O handle for the file. */
    Fscache_Block *blockPtr;	/* Cache block to write out. */
{
    panic("Lfs_FileBlockWrite called\n");
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_BlockAllocate --
 *
 *      Allocate disk space for the given file.  This routine only allocates
 *      one block beginning at offset and going for numBytes.   If
 *      offset + numBytes crosses a block boundary then a panic will occur.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The file descriptor is modified to contain pointers to the allocated
 *      blocks.  Also *blockAddrPtr is set to the block that was allocated.
 *
 *----------------------------------------------------------------------
 */
Lfs_BlockAllocate(domainPtr, handlePtr, offset, numBytes, flags, blockAddrPtr,
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
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    int	newLastByte;
    ReturnStatus status;
    Boolean	dirty = FALSE;
    register	Fsdm_FileDescriptor *descPtr;

    status = LfsSegUsageAllocateBytes(lfsPtr, numBytes);
    if (status != SUCCESS) {
	*blockAddrPtr = FSDM_NIL_INDEX;
	return status;
    }
    status = SUCCESS;
    descPtr = handlePtr->descPtr;
    newLastByte = offset + numBytes - 1; 
    *newBlockPtr = FALSE;
    *blockAddrPtr = offset / FS_BLOCK_SIZE;
    if (newLastByte > descPtr->lastByte) {
	descPtr->lastByte = newLastByte;
	dirty = TRUE;
    }
    if (descPtr->firstByte == -1 && 
	((descPtr->fileType == FS_NAMED_PIPE) ||
	 (descPtr->fileType == FS_PSEUDO_DEV) ||
	 (descPtr->fileType == FS_XTRA_FILE))) {
	descPtr->firstByte = 0;
	dirty = TRUE;
    }
    if (dirty) { 
	descPtr->descModifyTime = fsutil_TimeInSeconds;
	descPtr->flags |= FSDM_FD_SIZE_DIRTY;
    } 
    return(status);
}
