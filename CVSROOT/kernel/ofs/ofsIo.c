/* 
 * ofsIo.c --
 *
 *	Routines providing I/O for OFS domain files.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsStat.h>
#include <fsdm.h>
#include <fscache.h>
#include <ofs.h>
#include <devBlockDevice.h>

#include <stdio.h>

static Sync_Lock ofsCleanerLock = Sync_LockInitStatic("Fs:ofsCleanerLock");
#define	LOCKPTR	&ofsCleanerLock

static int ofsBlockCleaners = 0; /* Number of block cleaner processes in 
				  * action. */

/*
 * ofsBlockWritesPerFile - maximum number of blocks written for a file
 * before switching to the next dirty file.
 */
int	ofsBlockWritesPerFile = 25;

extern void Ofs_CleanBlocks _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
static Boolean FileMatch _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
				ClientData clientData));


/*
 *----------------------------------------------------------------------
 *
 * Ofs_FileBlockRead --
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
Ofs_FileBlockRead(domainPtr, handlePtr, blockPtr)
    Fsdm_Domain		*domainPtr;	/* Domain of file. */
    register Fsio_FileIOHandle *handlePtr;	/* Handle on a local file. */
    Fscache_Block	*blockPtr;	/* Cache block to read in.  This assumes
					 * the blockNum, blockAddr (buffer area)
					 * and blockSize are set.  This modifies
					 * blockSize if less bytes were read
					 * because of EOF. */
{
    register Ofs_Domain		*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    register	Fsdm_FileDescriptor *descPtr;
    register			 offset;
    register int		 numBytes;
    ReturnStatus		 status;
    OfsBlockIndexInfo		 indexInfo;

    status = SUCCESS;
    blockPtr->blockSize = 0;
    numBytes = FS_BLOCK_SIZE;
    offset = blockPtr->blockNum * FS_BLOCK_SIZE;

    if (handlePtr->hdr.fileID.minor == 0) {
	/*
	 * If is a physical block address then read it in directly.
	 */
	status = OfsDeviceBlockIO(ofsPtr, FS_READ, 
			   offset / FS_FRAGMENT_SIZE, FS_FRAGMENTS_PER_BLOCK, 
			   blockPtr->blockAddr);
	fs_Stats.gen.physBytesRead += FS_BLOCK_SIZE;
    } else {
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

	status = OfsGetFirstIndex(ofsPtr, handlePtr, offset / FS_BLOCK_SIZE, 
				 &indexInfo, 0);
	if (status != SUCCESS) {
	    printf("Ofs_FileRead: Could not setup indexing\n");
	    goto exit;
	}

	if (indexInfo.blockAddrPtr != (int *) NIL &&
	    *indexInfo.blockAddrPtr != FSDM_NIL_INDEX) {
	    /*
	     * Read in the block.  Specify the device, the fragment index,
	     * the number of fragments, and the memory buffer.
	     */
	    status = OfsDeviceBlockIO(ofsPtr, FS_READ, 
		      *indexInfo.blockAddrPtr +
		      ofsPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		      (numBytes - 1) / FS_FRAGMENT_SIZE + 1,
		      blockPtr->blockAddr);
	} else {
	    /*
	     * Zero fill the block.  We're in a 'hole' in the file.
	     */
	    fs_Stats.blockCache.readZeroFills++;
	    bzero(blockPtr->blockAddr, numBytes);
	}
	OfsEndIndex(handlePtr, &indexInfo, FALSE);
	Fs_StatAdd(numBytes, fs_Stats.gen.fileBytesRead,
		   fs_Stats.gen.fileReadOverflow);
    }
exit:
    /*
     * Define the block size and error fill leftover space.
     */
    if (status == SUCCESS) {
	blockPtr->blockSize = numBytes;
    }
    if (blockPtr->blockSize < FS_BLOCK_SIZE) {
	fs_Stats.blockCache.readZeroFills++;
	bzero(blockPtr->blockAddr + blockPtr->blockSize,
		FS_BLOCK_SIZE - blockPtr->blockSize);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Ofs_FileBlockWrite --
 *
 *	Write out a cache block.  This understands about physical
 *	block writes as opposed to file block writes, and it understands
 *	that negative block numbers are used for indirect blocks (gag).
 *	Physical blocks are numbered from the beginning of the disk,
 *	and they are used for file descriptors and indirect blocks.
 *	File blocks are numbered from the beginning of the data block
 *	area, so an offset must be used to calculate their true address.
 *
 * Results:
 *	The return code from the driver, or FS_DOMAIN_UNAVAILABLE if
 *	the domain has been un-attached.
 *
 * Side effects:
 *	The device write.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Ofs_FileBlockWrite(domainPtr, handlePtr, blockPtr)
    register	Fsdm_Domain	 *domainPtr;
    register Fsio_FileIOHandle *handlePtr;	/* I/O handle for the file. */
    Fscache_Block *blockPtr;	/* Cache block to write out. */
{
    register Ofs_Domain	*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    ReturnStatus		status;
    int				diskBlock;

    if (handlePtr->hdr.fileID.minor == 0 || blockPtr->diskBlock < 0) {
	/*
	 * The block number is a raw block number counting from the
	 * beginning of the domain.
	 * Descriptor blocks are indicated by a handle with a 0 file number 
	 * and indirect a negative block number (indirect blocks).
	 */
	if (blockPtr->diskBlock < 0) {
	    diskBlock = -blockPtr->diskBlock;
	} else {
	    diskBlock = blockPtr->diskBlock;
	}
	fs_Stats.gen.physBytesWritten += blockPtr->blockSize;
	status = OfsDeviceBlockIO(ofsPtr, FS_WRITE,
		     diskBlock, FS_FRAGMENTS_PER_BLOCK, blockPtr->blockAddr);
    } else {
	/*
	 * The block number is relative to the start of the data blocks.
	 */
	status = OfsVerifyBlockWrite(ofsPtr, blockPtr);
	if (status == SUCCESS) {
	    status = OfsDeviceBlockIO(ofsPtr, FS_WRITE,
		   blockPtr->diskBlock + 
		   ofsPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		   (blockPtr->blockSize - 1) / FS_FRAGMENT_SIZE + 1,
		   blockPtr->blockAddr);
        }
	if (status == SUCCESS) {
	    Fs_StatAdd(blockPtr->blockSize, fs_Stats.gen.fileBytesWritten,
		   fs_Stats.gen.fileWriteOverflow);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * OfsDeviceBlockIO --
 *
 *	Map a file system block address to a block device block address 
 *	perform the requested operation.
 *
 * NOTE: This routine is temporary and should be replaced when the file system
 *	 is converted to use the async block io interface.
 *
 * Results:
 *	The return status of the operation.
 *
 * Side effects:
 *	Blocks may be written or read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
OfsDeviceBlockIO(ofsPtr, readWriteFlag, fragNumber, numFrags, buffer)
    Ofs_Domain	*ofsPtr;	/* Domain */
    int readWriteFlag;		/* FS_READ or FS_WRITE */
    int fragNumber;		/* CAREFUL, fragment index, not block index.
				 * This is relative to start of device. */
    int numFrags;		/* CAREFUL, number of fragments, not blocks */
    Address buffer;		/* I/O buffer */
{
    ReturnStatus status = SUCCESS;	/* General return code */
    int firstSector;		/* Starting sector of transfer */
    DevBlockDeviceRequest	request;
    int				transferCount;

    if ((fragNumber % FS_FRAGMENTS_PER_BLOCK) != 0) {
	/*
	 * The I/O doesn't start on a block boundary.  Transfer the
	 * first few extra fragments to get things going on a block boundary.
	 */
	register int extraFrags;

	extraFrags = FS_FRAGMENTS_PER_BLOCK -
		    (fragNumber % FS_FRAGMENTS_PER_BLOCK);
	if (extraFrags > numFrags) {
	    extraFrags = numFrags;
	}
	firstSector = OfsBlocksToSectors(fragNumber, 
			&ofsPtr->headerPtr->geometry);
	request.operation = readWriteFlag;
	request.startAddress = firstSector * DEV_BYTES_PER_SECTOR;
	request.startAddrHigh = 0;
	request.bufferLen = extraFrags * FS_FRAGMENT_SIZE;
	request.buffer = buffer;
	status = Dev_BlockDeviceIOSync(ofsPtr->blockDevHandlePtr, &request, 
					&transferCount);
	extraFrags = transferCount / FS_FRAGMENT_SIZE;
	fragNumber += extraFrags;
	buffer += transferCount;
	numFrags -= extraFrags;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    while (numFrags >= FS_FRAGMENTS_PER_BLOCK) {
	/*
	 * Transfer whole blocks.
	 */
	firstSector = OfsBlocksToSectors(fragNumber, 
			&ofsPtr->headerPtr->geometry);
	request.operation = readWriteFlag;
	request.startAddress = firstSector * DEV_BYTES_PER_SECTOR;
	request.startAddrHigh = 0;
	request.bufferLen = FS_BLOCK_SIZE;
	request.buffer = buffer;
	status = Dev_BlockDeviceIOSync(ofsPtr->blockDevHandlePtr, &request, 
					&transferCount);
	fragNumber += FS_FRAGMENTS_PER_BLOCK;
	buffer += FS_BLOCK_SIZE;
	numFrags -= FS_FRAGMENTS_PER_BLOCK;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (numFrags > 0) {
	/*
	 * Transfer the left over fragments.
	 */
	firstSector = OfsBlocksToSectors(fragNumber, 
			&ofsPtr->headerPtr->geometry);
	request.operation = readWriteFlag;
	request.startAddress = firstSector * DEV_BYTES_PER_SECTOR;
	request.startAddrHigh = 0;
	request.bufferLen = numFrags * FS_FRAGMENT_SIZE;
	request.buffer = buffer;
	status = Dev_BlockDeviceIOSync(ofsPtr->blockDevHandlePtr, &request, 
					&transferCount);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsioVerifyBlockWrite --
 *
 *	Double check this block to make sure it seems like were writing
 *	it to the right place.
 *
 * Results:
 *	Error code if an inconsistency was detected.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
OfsVerifyBlockWrite(ofsPtr, blockPtr)
    Ofs_Domain	*ofsPtr;
    Fscache_Block *blockPtr;		/* Block about to be written out */
{
    ReturnStatus status = SUCCESS;
    Fs_HandleHeader *hdrPtr = blockPtr->cacheInfoPtr->hdrPtr;
    OfsBlockIndexInfo		 indexInfo;

    if (blockPtr->fileNum != hdrPtr->fileID.minor) {
	printf("FsioVerifyBlockWrite: block being written to wrong file\n");
	printf("	Logical block %d block's file %d owning file %d \"%s\"\n",
	    blockPtr->blockNum, blockPtr->fileNum,
	    hdrPtr->fileID.minor, Fsutil_HandleName(hdrPtr));
	return(FS_INVALID_ARG);
    }
    status = OfsGetFirstIndex(ofsPtr, (Fsio_FileIOHandle *)hdrPtr, 
			blockPtr->blockNum, &indexInfo,
			(int)FSCACHE_DONT_BLOCK);
    if (status == FS_WOULD_BLOCK) {
	/*
	 * No room in the cache for the index blocks needed to check.
	 * assume the write is ok.
	 */
	return(SUCCESS);
    } else if (status != SUCCESS) {
	return(status);
    }
    if (indexInfo.blockAddrPtr == (int *)NIL ||
	*indexInfo.blockAddrPtr == FSDM_NIL_INDEX) {
	printf("FsioVerifyBlockWrite: no block index\n");
	panic("	Logical block %d owning file %d \"%s\"\n",
	    blockPtr->blockNum, hdrPtr->fileID.minor, Fsutil_HandleName(hdrPtr));
	status = FS_INVALID_ARG;
	goto exit;
    }
    if (*indexInfo.blockAddrPtr != blockPtr->diskBlock) {
	printf("OfsVerifyBlockWrite: disk block mismatch\n");
	panic("	Logical block %d old disk block %d new %d owning file %d \"%s\"\n",
	    blockPtr->blockNum, *indexInfo.blockAddrPtr,
	    blockPtr->diskBlock,
	    hdrPtr->fileID.minor, Fsutil_HandleName(hdrPtr));
	status = FS_INVALID_ARG;
	goto exit;
    }

exit:
    OfsEndIndex((Fsio_FileIOHandle *)hdrPtr, &indexInfo, FALSE);
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * BlockMatch --
 *
 * 	Cache backend block type match.  Ofs doesn't care about the 
 *	order of blocks returned by GetDirtyBlocks.
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
BlockMatch(blockPtr, clientData)
    Fscache_Block *blockPtr;
    ClientData	   clientData;
{
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FileMatch --
 *
 * 	Cache backend file match. Ofs will take any file the cache wants
 *	it to write.
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
    return TRUE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Ofs_StartWriteBack --
 *
 * 	Start a block cleaner process for the specified domain.
 *
 * Results:
 *	TRUE if a block cleaner was started.
 *
 * Side effects:
 *	Number of block cleaner processes may be incremented.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
Ofs_StartWriteBack(backendPtr)
    Fscache_Backend *backendPtr;	/* Backend to start writeback. */
{
    LOCK_MONITOR;
    if (ofsBlockCleaners < fscache_MaxBlockCleaners) {
	Proc_CallFunc(Ofs_CleanBlocks, (ClientData) backendPtr, 0);
	ofsBlockCleaners++;
	UNLOCK_MONITOR;
	return TRUE;
    }
    UNLOCK_MONITOR;
    return FALSE;
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to clean dirty blocks.
 *
 * ----------------------------------------------------------------------------
 */


/*
 * ----------------------------------------------------------------------------
 *
 * Ofs_CleanBlocks
 *
 *	Write all blocks on the dirty list to disk.  Called either from
 *	a block cleaner process or synchronously during system shutdown.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The dirty list is emptied.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Ofs_CleanBlocks(data, callInfoPtr)
    ClientData		data;		/* Background flag.  If TRUE it means
					 * we are called from a block cleaner
					 * process.  Otherwise we being called
					 * synchrounously during a shutdown */
    Proc_CallInfo	*callInfoPtr;	/* Not Used. */
{
    Fscache_Block	*blockPtr;
    ReturnStatus		status;
    int				lastDirtyBlock;
    Fscache_FileInfo		*cacheInfoPtr;
    Fscache_Backend		*backendPtr;
    int				numWrites;

    backendPtr = (Fscache_Backend *) data;
    cacheInfoPtr = Fscache_GetDirtyFile(backendPtr, TRUE, FileMatch, 
					(ClientData) NIL);
    while (cacheInfoPtr != (Fscache_FileInfo *)NIL) {
	blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch, 
					(ClientData) 0, &lastDirtyBlock);
	numWrites = 0;
	while (blockPtr != (Fscache_Block *) NIL) {
	    /*
	     * Write the block.
	     */
	    status = Fsdm_FileBlockWrite
		    (cacheInfoPtr->hdrPtr, blockPtr, lastDirtyBlock);
	    numWrites++;
	    Fscache_ReturnDirtyBlock(blockPtr, status);
	    if (numWrites < ofsBlockWritesPerFile) {
		blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch, 
					(ClientData) 0, &lastDirtyBlock);
	    } else {
		break;
	    }
	}
	Fscache_ReturnDirtyFile(cacheInfoPtr, FALSE);
	cacheInfoPtr = Fscache_GetDirtyFile(backendPtr, TRUE, FileMatch, 
					(ClientData) NIL);
    }
    FscacheBackendIdle(backendPtr);
    LOCK_MONITOR;
    ofsBlockCleaners--;
    UNLOCK_MONITOR;
}



