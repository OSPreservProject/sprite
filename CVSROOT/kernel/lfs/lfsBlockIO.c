/* 
 * lfsBlockIO.c --
 *
 *	Routines for handling block allocate and access of files in a 
 *	LFS file system. This routines are used by the cache code or
 *	read and allocate files.
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

#include <sprite.h>
#include <lfs.h>
#include <lfsInt.h>
#include <fs.h>
#include <fsutil.h>
#include <fsio.h>
#include <fsioFile.h>
#include <fslcl.h>
#include <fscache.h>
#include <fsdm.h>
#include <fsStat.h>



/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileBlockRead --
 *
 *	Read in a cache block.  This routine uses the files'
 *	indexing structure to locate the file block on disk.
 *	This always attempts to read in a full block, but will read
 *	less if at the last block and it isn't full.  In this case,
 *	the remainder of the cache block is zero-filled.
 *
 * Results:
 *	The results of the disk read.
 *
 * Side effects:
 *	The blockPtr->blockSize is modified to
 *	reflect how much data was actually read in.  The unused part
 *	of the block is filled with zeroes so that higher levels can
 *	always assume the block has good stuff in all parts of it.
 *
 *----------------------------------------------------------------------
 */
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
    LfsDiskAddr			diskAddress;
    ReturnStatus		 status;

    LFS_STATS_INC(lfsPtr->stats.blockio.reads);
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
	numBytes = 0;
	goto exit;
    } else if (offset + numBytes - 1 > descPtr->lastByte) {
	numBytes = descPtr->lastByte - offset + 1;
    }

    status = LfsFile_GetIndex(handlePtr, offset / FS_BLOCK_SIZE, 0,
			     &diskAddress);
    if (status != SUCCESS) {
	printf("Lfs_FileBlockRead: Could not setup indexing\n");
	goto exit;
    }

    if (!LfsIsNilDiskAddr(diskAddress)) {
	/*
	 * Read in the block.  Specify the device, the fragment index,
	 * the number of fragments, and the memory buffer.
	 */
	int	ioSize = 
		LfsBlocksToBytes(lfsPtr, LfsBytesToBlocks(lfsPtr, numBytes));
	LfsCheckRead(lfsPtr, diskAddress, numBytes);

	status = LfsReadBytes(lfsPtr, diskAddress, ioSize, blockPtr->blockAddr);
	LFS_STATS_ADD(lfsPtr->stats.blockio.bytesReads, ioSize);
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
#ifdef SOSP91
    if (proc_RunningProcesses[0] != (Proc_ControlBlock *) NIL) {
	if ((proc_RunningProcesses[0]->state == PROC_MIGRATED) ||
		(proc_RunningProcesses[0]->genFlags &
		(PROC_FOREIGN | PROC_MIGRATING))) {
	    Fs_StatAdd(numBytes, fs_SospMigStats.gen.fileBytesRead, 
			fs_SospMigStats.gen.fileReadOverflow);
	}
    }
#endif SOSP91
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
 *      Write out a cache block.  Since lfs maintains its own cache
 *	write back mechanism, this routines should never be called.
 *
 * Results:
 *      FAILURE
 *
 * Side effects:
 *      It panic's.
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
    return FAILURE;
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_BlockAllocate --
 *
 *      Allocate disk space for the given file.  This routine only allocates
 *      one block beginning at offset and going for numBytes. 
 *
 * Results:
 *	SUCCESS or FS_NO_DISK_SPACE
 *      
 *
 * Side effects:
 *      The file descriptor is modified to contain pointers to the allocated
 *      blocks.  Also *blockAddrPtr is set to the block that was allocated.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
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

    LFS_STATS_INC(lfsPtr->stats.blockio.allocs);
    /*
     * Block allocates while checkpoints are active. This ensure that
     * the LFS cache backend will be able to clean all LFS files from the
     * cache. We only need worry about files, directory updates get stopped by
     * the dirlog mechanism. In fact, waiting for a checkpoint for a 
     * directory block allocate causes a possible deadlock because the
     * checkpoint waits for directory operations to finish. We also
     * use this mechanism to keep from filling up the cache with blocks
     * that we can't write out.
     */
    descPtr = handlePtr->descPtr;
    if (descPtr->fileType != FS_DIRECTORY) {
	LfsWaitForCleanSegments(lfsPtr);
    } 
    /*
     * First check to see if we can just allocate the bytes.
     */
    newLastByte = offset + numBytes - 1; 
    *blockAddrPtr = FSDM_NIL_INDEX;
    status = LfsFile_GrowBlock(lfsPtr, handlePtr, offset, numBytes);
    if (status == SUCCESS) {
	*newBlockPtr = FALSE;
	*blockAddrPtr = offset / FS_BLOCK_SIZE;
	 if (newLastByte > descPtr->lastByte) {
	    descPtr->lastByte = newLastByte;
	    dirty = TRUE;
	 }
    } 
    if ((status == SUCCESS) && descPtr->firstByte == -1 && 
	((descPtr->fileType == FS_NAMED_PIPE) ||
	 (descPtr->fileType == FS_PSEUDO_DEV) ||
	 (descPtr->fileType == FS_XTRA_FILE))) {
	descPtr->firstByte = 0;
	dirty = TRUE;
    }
    if (dirty) { 
	descPtr->descModifyTime = Fsutil_TimeInSeconds();
	descPtr->flags |= FSDM_FD_SIZE_DIRTY;
    } 
    return(status);
}
