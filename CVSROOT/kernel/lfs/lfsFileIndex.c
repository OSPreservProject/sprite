/* 
 * lfsFileIndex.c --
 *
 *	Routines to allow moving through a files block pointers.  The method
 *	of using these routines is the following:
 *
 *	    1) Call LfsFile_GetIndex to get the index of a specified block
 *				of a specified file.
 *	    2) Call LfsFile_SetIndex to set the index of a specified block
 *				of a specified file.
 *	    2) Call LfsFile_TruncIndex to truncate the index of a specified
 *				file to the specified block number.
 *
 *
 *	The data structure operated on is the disk map kept in the disk
 *	file descriptor (LfsFileDescriptor).  This has 10 direct block pointers,
 *	then a singly indirect block full of direct block pointers,
 *	then a doubly indirect block full of singly indirect pointers.
 *	The triple indirect block pointer is not implemented, limiting
 *	the file size to 40K + 4Meg + 4Gigabytes.
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
#include "lfs.h"
#include "lfsInt.h"
#include "fsutil.h"
#include "fscache.h"
#include "fsdm.h"

/*
 * Index operation type for AccessBlock routine.
 */
enum IndexOp { GET_ADDR, SET_ADDR, GROW_ADDR};

static ReturnStatus GrowBlock _ARGS_((Lfs *lfsPtr, 
		Fsio_FileIOHandle *handlePtr, int blockNum, int blockSize,
		LfsDiskAddr diskAddress, int cacheFlags));

static ReturnStatus AccessBlock _ARGS_((enum IndexOp op, Lfs *lfsPtr, 
		Fsio_FileIOHandle *handlePtr, int blockNum, int blockSize,
		int cacheFlags, LfsDiskAddr *diskAddressPtr));
static ReturnStatus DeleteIndirectBlock _ARGS_((Lfs *lfsPtr, 
		Fsio_FileIOHandle *handlePtr, int virtualBlockNum,
		LfsDiskAddr *diskAddrPtr,
		int startBlockNum, int lastBlockNum, int step, 
		int lastByteBlock));


/*
 *----------------------------------------------------------------------
 *
 * LfsFile_GetIndex --
 *
 *	Return the disk address of the specified block of a specified file.
 *
 * Results:
 *	A status indicating whether there was sufficient space to allocate
 *	indirect blocks.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFile_GetIndex(handlePtr, blockNum, cacheFlags, diskAddressPtr)
    Fsio_FileIOHandle	*handlePtr;   /* Handle for file that are 
					      * interest in. */
    int		        blockNum;    /* Block number of interest. */
    int		cacheFlags;	     /* FSCACHE_CANT_BLOCK,FSCACHE_DONT_BLOCK.*/
    LfsDiskAddr *diskAddressPtr;      /* Disk address returned. */
{
    Lfs				*lfsPtr;
    Fsdm_Domain			*domainPtr;
    ReturnStatus		status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    lfsPtr = LfsFromDomainPtr(domainPtr);
    LFS_STATS_INC(lfsPtr->stats.index.get);
    status = AccessBlock(GET_ADDR, lfsPtr, handlePtr, blockNum, 0, cacheFlags,
				diskAddressPtr);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFile_SetIndex --
 *
 *	Set the disk address of the specified block of a specified file.
 *
 * Results:
 *	SUCCESS if operation worked. FS_WOULD_BLOCK if operation would
 *	block and cantBlock argument set.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFile_SetIndex(handlePtr, blockNum, blockSize, cacheFlags, diskAddress)
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int		    blockNum; 	     /* Block number of interest. */
    int		    blockSize;	     /* Size of block in bytes. */
    int		cacheFlags;	     /* FSCACHE_CANT_BLOCK,FSCACHE_DONT_BLOCK.*/
    LfsDiskAddr	 diskAddress; 		     /* Disk address of block. */
{
    Lfs		    	*lfsPtr;
    Fsdm_Domain			*domainPtr;
    ReturnStatus		status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    lfsPtr = LfsFromDomainPtr(domainPtr);
    LFS_STATS_INC(lfsPtr->stats.index.set);
    /*
     * Checking. 
     */
#ifdef ERROR_CHECK
   if (!LfsIsNilDiskAddr(diskAddress)) { 
        int segNo;
	segNo = LfsDiskAddrToSegmentNum(lfsPtr, diskAddress);
	if (!LfsValidSegmentNum(lfsPtr,segNo)) {
	    panic("LfsFile_SetIndex: bad segment number.\n");
	}
    }
#endif
    status = AccessBlock(SET_ADDR, lfsPtr, handlePtr, blockNum, blockSize,
		cacheFlags, &diskAddress);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * GrowBlock --
 *
 *	Grow the specified block to take occupy more space.
 *
 * Results:
 *	SUCCESS if operation worked ok.
 *	FS_WOULD_BLOCK if operation would block an cantBlock set.
 *
 * Side effects:
 *	The block may be fetched into the cache and marked as
 *	modified to get to be written to the log again.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GrowBlock(lfsPtr, handlePtr, blockNum, growthSize, diskAddr, cacheFlags)
    Lfs		*lfsPtr;		     /* File system of file. */
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int		blockNum;		     /* Block number of file. */
    int		growthSize;		/* Block growth size. */
    LfsDiskAddr diskAddr;	    	/* Disk address of block. */
    int		cacheFlags;	     /* FSCACHE_CANT_BLOCK,FSCACHE_DONT_BLOCK.*/
{
    Fsdm_FileDescriptor *descPtr;
    int origBlockSize;
    Fscache_Block *blockPtr;
    Boolean	found;
    ReturnStatus	status = SUCCESS;

    if (LfsIsNilDiskAddr(diskAddr)) {
	/*
	 * The block is not allocated on disk. Do nothing.
	 */
	return SUCCESS;
    }
    descPtr = handlePtr->descPtr;

    origBlockSize = (descPtr->lastByte + 1) - blockNum * FS_BLOCK_SIZE;
    if (origBlockSize <= 0) {
	LfsError(lfsPtr, FAILURE, "GrowBlock: Bad original size of block\n");
	return FAILURE;
    }
    origBlockSize = LfsBlocksToBytes(lfsPtr, 
				LfsBytesToBlocks(lfsPtr,origBlockSize));

    if (origBlockSize + growthSize > FS_BLOCK_SIZE) {
	LfsError(lfsPtr, FAILURE, "GrowBlock: Bad  size of block\n");
	return FAILURE;
    }

    if (descPtr->fileType != FS_DIRECTORY) { 
	/*
	 * If the file is not a directory we need to read the block in 
	 * and mark it as dirty so it will be written back. This
	 * is because the block now too small on disk and we must
	 * write it out with its new larger size. The reason why
	 * we must fetch the block is that the growth may be due
	 * to a write pass the end block of the file.  This causes the
	 * old last block to no longer be a fragment. The new bytes in
	 * this block must be zeros.  This can not happen for directories
	 * because the file system controls writes to directory blocks.
	 */
	Fscache_FetchBlock(&handlePtr->cacheInfo, blockNum,
		    cacheFlags|FSCACHE_DATA_BLOCK, &blockPtr, &found);
	if (!found) {
	    if (blockPtr == (Fscache_Block *) NIL) {
		status = FS_WOULD_BLOCK;
		return status;
	    }
	     status = LfsReadBytes(lfsPtr, diskAddr, origBlockSize, 
			   blockPtr->blockAddr);
	    bzero(blockPtr->blockAddr+origBlockSize, FS_BLOCK_SIZE-origBlockSize);
#ifdef ERROR_CHECK
	     LfsCheckRead(lfsPtr, diskAddr, origBlockSize);
#endif
	     if (status != SUCCESS) {
		 LfsError(lfsPtr, status, "Can't read block to grow.\n");
		 return status;
	     }
	}

	Fscache_UnlockBlock(blockPtr,(unsigned)Fsutil_TimeInSeconds(), 
				blockNum, origBlockSize+growthSize, 0);
    }
    /*
     * Grow the active bytes of the segment. The activeBytes will
     * be decremented when the file is deleted or the block is written
     * out to disk.
     */
    LfsSetSegUsage(lfsPtr, LfsDiskAddrToSegmentNum(lfsPtr, diskAddr),
		    growthSize);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * AccessBlock --
 *
 *	Access and perform a GET or SET operation on the specified block
 *	of the specified file.
 *
 * Results:
 *	SUCCESS if operation worked ok.
 *	FS_WOULD_BLOCK if operation would block an cantBlock set.
 *
 * Side effects:
 *	Index block may be allocated.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
AccessBlock(op, lfsPtr, handlePtr, blockNum, blockSize, cacheFlags, 
		diskAddressPtr)
    enum IndexOp 	      op;	     /* Operation to be performed. 
					      * Must be GET_ADDR,  
					      * SET_ADDR or GROW_ADDR. */
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    Lfs		*lfsPtr;		     /* File system of file. */
    int		blockNum;		     /* Block number of file. */
    int		blockSize;		/* Block size for set operation */
    int		cacheFlags;	     /* FSCACHE_CANT_BLOCK,FSCACHE_DONT_BLOCK.*/
    LfsDiskAddr *diskAddressPtr;	    /* Disk address in/out. */
{
    int parentIndex, parentBlockNum;
    LfsDiskAddr parentDiskAddress;
    Fsdm_FileDescriptor 	*descPtr;
    Fscache_Block *parentblockPtr;
    int	modTime;
    Boolean	found;
    ReturnStatus	status;
    LfsDiskAddr *indirectPtr, *directPtr;

    descPtr = handlePtr->descPtr;
    directPtr = (LfsDiskAddr *)(descPtr->direct);
    indirectPtr = (LfsDiskAddr *)(descPtr->indirect);

    /*
     * First process the data and indirect blocks that are pointed to 
     * by the descriptor.
     */
    if (blockNum < 0) {
	if (op == GROW_ADDR) {
	    panic("LfsAccessBlock - Can't grow indirect block\n");
	    return FAILURE;
	}
	if (blockNum >= -FSDM_NUM_INDIRECT_BLOCKS) { 
	    /*
	     * This is a direct indirect block.
	     */
	    if (op == GET_ADDR) { 
		*diskAddressPtr = indirectPtr[(-blockNum)-1];
	    } else { /* SET_ADDR */
		LfsDiskAddr *addrPtr = indirectPtr + ((-blockNum)-1);
		LfsSegUsageFreeBlocks(lfsPtr, blockSize, 1, addrPtr);
		*addrPtr = *diskAddressPtr;
		descPtr->flags |= FSDM_FD_INDEX_DIRTY;
		(void) Fsdm_FileDescStore(handlePtr, FALSE);
	    }
	    return(SUCCESS);
	 }
	 if (blockNum > -(FSDM_NUM_INDIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK)) {
	     parentIndex = (-blockNum) - (FSDM_NUM_INDIRECT_BLOCKS+1);
	     parentBlockNum = -2;
	 } else {
	    /*
	     * Past the largest file size that we support.
	     */
	    return(FS_INVALID_ARG);
	 }
    } else { 
	if (blockNum < FSDM_NUM_DIRECT_BLOCKS) {
	    /*
	     * This is a direct data block.
	     */
	    status = SUCCESS;
	    switch (op) {
		case GET_ADDR: { 
		    *diskAddressPtr = directPtr[blockNum];
		    break;
		}
		case SET_ADDR: {
		    LfsDiskAddr *addrPtr = (directPtr + blockNum);
		    LfsSegUsageFreeBlocks(lfsPtr, blockSize, 1, addrPtr);
		    *addrPtr = *diskAddressPtr;
		    descPtr->flags |= FSDM_FD_INDEX_DIRTY;
		    (void) Fsdm_FileDescStore(handlePtr, FALSE);
		    break;
		}
		case GROW_ADDR: {
		    status = GrowBlock(lfsPtr, handlePtr, blockNum, 
				blockSize, directPtr[blockNum], 
				cacheFlags & 
				   (FSCACHE_CANT_BLOCK|FSCACHE_DONT_BLOCK));
		    *diskAddressPtr = directPtr[blockNum];
		    break;
		}
	    }
	    return(status);
	}
	if (blockNum < (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK)) {
	    parentBlockNum = -1;
	    parentIndex = blockNum - FSDM_NUM_DIRECT_BLOCKS;
	} else if (blockNum < FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK +
				FSDM_INDICES_PER_BLOCK*FSDM_INDICES_PER_BLOCK) {
	    parentBlockNum = -((FSDM_NUM_INDIRECT_BLOCKS+1) +
		  (blockNum - FSDM_NUM_DIRECT_BLOCKS - FSDM_INDICES_PER_BLOCK)/
		    FSDM_INDICES_PER_BLOCK);
	    parentIndex = (blockNum - FSDM_NUM_DIRECT_BLOCKS - 
			   FSDM_INDICES_PER_BLOCK) % FSDM_INDICES_PER_BLOCK;
	} else {
	    /*
	     * Past the largest file size that we support.
	     */
	    return(FS_INVALID_ARG);
	}

    }
    cacheFlags |= ((op == SET_ADDR) ? 
				  (FSCACHE_IND_BLOCK|FSCACHE_IO_IN_PROGRESS) 
				    : FSCACHE_IND_BLOCK);


    switch (op) {
	case GET_ADDR: {
	    LFS_STATS_INC(lfsPtr->stats.index.getFetchBlock);
	    break;
	}
	case SET_ADDR: {
	    LFS_STATS_INC(lfsPtr->stats.index.setFetchBlock);
	    break;
	} 
	case GROW_ADDR: {
	    LFS_STATS_INC(lfsPtr->stats.index.growFetchBlock);
	    break;
	} 
    }
     /* 
     * Lookup the parent block in the cache.
     */
   Fscache_FetchBlock(&handlePtr->cacheInfo, parentBlockNum,
		cacheFlags, &parentblockPtr, &found);
    if ((parentblockPtr != (Fscache_Block *) NIL) && found) {
	modTime = 0;
	 status = SUCCESS;
	 switch (op) {
	     case GET_ADDR: {
		LFS_STATS_INC(lfsPtr->stats.index.getFetchHit);
		*diskAddressPtr = 
		    ((LfsDiskAddr *)parentblockPtr->blockAddr)[parentIndex];
		break;
	     } 
	     case SET_ADDR: {
		LfsDiskAddr *addrPtr;
		addrPtr = ((LfsDiskAddr *)parentblockPtr->blockAddr) + 
			   parentIndex; 
		LFS_STATS_INC(lfsPtr->stats.index.setFetchHit);
		LfsSegUsageFreeBlocks(lfsPtr, blockSize, 1, addrPtr);
		*addrPtr = *diskAddressPtr;
		modTime = Fsutil_TimeInSeconds();
		break;
	     }
	     case GROW_ADDR: {
		*diskAddressPtr = 
		    ((LfsDiskAddr *)parentblockPtr->blockAddr)[parentIndex];
		status = GrowBlock(lfsPtr, handlePtr, blockNum, 
				blockSize, *diskAddressPtr, 
				cacheFlags & 
				   (FSCACHE_CANT_BLOCK|FSCACHE_DONT_BLOCK));
		break;
	     } 
	 }
	 Fscache_UnlockBlock(parentblockPtr, (unsigned )modTime, parentBlockNum,
			     FS_BLOCK_SIZE, 0);
	 return status;
    }
    if (parentblockPtr == (Fscache_Block *) NIL) {
	return FS_WOULD_BLOCK;
    }
    /*
     * Not found in cache. Try to read it in. First we need to find the
     * address of the block.
     */
    status = AccessBlock(GET_ADDR, lfsPtr, handlePtr, parentBlockNum, 0,
				cacheFlags,
				&parentDiskAddress);
    if (status != SUCCESS) {
	 Fscache_UnlockBlock(parentblockPtr, (unsigned )0, parentBlockNum,
			     FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	return status;
    }
    if (LfsIsNilDiskAddr(parentDiskAddress)) {
	switch (op) {
	    case GROW_ADDR: 
	    case GET_ADDR: {
		 LfsSetNilDiskAddr(diskAddressPtr);
		 Fscache_UnlockBlock(parentblockPtr, (unsigned)0,
				 parentBlockNum,
				 FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
		 break;
	    }
	    case SET_ADDR: {
		register LfsDiskAddr *intPtr, *limitPtr;
		limitPtr = (LfsDiskAddr *) (parentblockPtr->blockAddr + 
					    FS_BLOCK_SIZE);
		for (intPtr = (LfsDiskAddr *)parentblockPtr->blockAddr;
			    intPtr < limitPtr; intPtr++) {
		    LfsSetNilDiskAddr(intPtr);
		}
		((LfsDiskAddr *)parentblockPtr->blockAddr)[parentIndex] = 
			    *diskAddressPtr;
		 Fscache_UnlockBlock(parentblockPtr, 
				    (unsigned )Fsutil_TimeInSeconds(),
				    parentBlockNum, FS_BLOCK_SIZE, 0);
		 break;
	    }
	}
	return SUCCESS;
     }

     status = LfsReadBytes(lfsPtr, parentDiskAddress, 
		FS_BLOCK_SIZE,  parentblockPtr->blockAddr);
#ifdef ERROR_CHECK
     LfsCheckRead(lfsPtr, parentDiskAddress, FS_BLOCK_SIZE);
#endif
     if (status != SUCCESS) {
	 Fscache_UnlockBlock(parentblockPtr, (unsigned )0, parentBlockNum,
			     FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	 LfsError(lfsPtr, status, "Can't read indirect block.\n");
	 return status;
     }
     modTime = 0;
     switch (op) {
	 case GET_ADDR: {
	    *diskAddressPtr =  
		    ((LfsDiskAddr *)parentblockPtr->blockAddr)[parentIndex];
	    break;
	 }
	 case SET_ADDR: {
	    LfsDiskAddr *addrPtr = ((LfsDiskAddr *)parentblockPtr->blockAddr) + 
					    parentIndex; 
	    LfsSegUsageFreeBlocks(lfsPtr, blockSize, 1, addrPtr);
	    *addrPtr = *diskAddressPtr;
	    modTime = Fsutil_TimeInSeconds();
	    break;
	 }
	 case GROW_ADDR: {
	    *diskAddressPtr =  
		    ((LfsDiskAddr *)parentblockPtr->blockAddr)[parentIndex];
	    status = GrowBlock(lfsPtr, handlePtr, blockNum, 
				blockSize, *diskAddressPtr, 
				cacheFlags & 
				   (FSCACHE_CANT_BLOCK|FSCACHE_DONT_BLOCK));
	    break;
	 }
    }
    Fscache_UnlockBlock(parentblockPtr, (unsigned) modTime, 
				parentBlockNum, FS_BLOCK_SIZE, 0);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteIndirectBlock --
 *
 *	Deallocate and remove from the cache all blocks of the specified
 *	file greater than the given block number.
 *
 * Results:
 *	SUCCESS if all goes well or some error returned from DiskRead
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DeleteIndirectBlock(lfsPtr, handlePtr, virtualBlockNum, diskAddrPtr, 
	startBlockNum, lastBlockNum, step, lastByteBlock) 
    Lfs	  *lfsPtr;
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int	  virtualBlockNum;	/* Virtual block number for this indirect 
				 * block. */
    LfsDiskAddr	  *diskAddrPtr;	/* Disk address of block. */
    int	  startBlockNum;	/* Starting block number of this virtual
				 * Block. */
    int	  lastBlockNum;		/* New last block number of file. */
    int	  step;			/* Number of blocks covered by each virtual
				 * block entry. */
    int	  lastByteBlock;	/* Block containing last byte of file. */
{
    Fscache_Block	*cacheBlockPtr;
    LfsDiskAddr		diskAddr = *diskAddrPtr;
    Boolean		found;
    ReturnStatus	status = SUCCESS;
    Boolean		blockInCache;
    int			startElement, cstep, childBlockNum, i;
    LfsDiskAddr	*blockArray;
    /*
     * If this index block hasn't been allocated yet and not in the  
     * cache we still need to check to see if any of its children
     * might be in the cache.  
     */
    LFS_STATS_INC(lfsPtr->stats.index.deleteFetchBlock);
    blockInCache = TRUE;
    Fscache_FetchBlock(&handlePtr->cacheInfo, virtualBlockNum,
       (int)(FSCACHE_IO_IN_PROGRESS|FSCACHE_IND_BLOCK), &cacheBlockPtr,&found);
    if (!found) {
	if (LfsIsNilDiskAddr(diskAddr)) {
	    Fscache_UnlockBlock(cacheBlockPtr, (unsigned )0, virtualBlockNum,
				 FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	    blockInCache = FALSE;
	} else {
	    /*
	     * Read it into the cache if it on disk somewhere.
	     */
	    LFS_STATS_INC(lfsPtr->stats.index.deleteFetchBlockMiss);
	    status = LfsReadBytes(lfsPtr, diskAddr, FS_BLOCK_SIZE, 
			   cacheBlockPtr->blockAddr);
#ifdef ERROR_CHECK
	     LfsCheckRead(lfsPtr, diskAddr, FS_BLOCK_SIZE);
#endif
	     if (status != SUCCESS) {
		 LfsError(lfsPtr, status, "Can't read indirect block.\n");
		 return status;
	     }
	}
    }
    /*
     * Compute the starting element of the block to start deleting at.
     * If step equals one we must be pointing to data blocks.
     */
    startElement = (lastBlockNum - startBlockNum)/step;
    if (startElement < 0) {
	startElement = 0;
    } else if (startElement > FSDM_INDICES_PER_BLOCK) {
	panic("Bad call to DeleteIndirectBlock\n");
    }
    if (step != 1) {
	static LfsDiskAddr nilAddr;
	LfsDiskAddr *addrPtr = &nilAddr;
	cstep = step/FSDM_INDICES_PER_BLOCK;
	startBlockNum = startBlockNum + startElement * step;
	childBlockNum = -((FSDM_NUM_INDIRECT_BLOCKS+1)+startElement);
	LfsSetNilDiskAddr(addrPtr);
	for (i = startElement; i < FSDM_INDICES_PER_BLOCK; i++) { 
	    if (blockInCache) { 
		addrPtr = ((LfsDiskAddr *) cacheBlockPtr->blockAddr) + i;
	    } 
	    status = DeleteIndirectBlock(lfsPtr, handlePtr, childBlockNum, 
				addrPtr, startBlockNum, lastBlockNum, cstep, 
				lastByteBlock);
	    startBlockNum += step;
	    childBlockNum--;
	}
    } else if (blockInCache) {
	blockArray =  ((LfsDiskAddr *) cacheBlockPtr->blockAddr) + startElement;
	/*
	 * Free the last block in the file handling the case that it
	 * is a fragment.
	 */
	if ((lastByteBlock >= startBlockNum) && 
	    (lastByteBlock < startBlockNum + FSDM_INDICES_PER_BLOCK) &&
	    (lastByteBlock >= lastBlockNum)) {
	    int fragSize;
	    fragSize = handlePtr->descPtr->lastByte - 
				(lastByteBlock * FS_BLOCK_SIZE) + 1;
	    fragSize = LfsBlocksToBytes(lfsPtr, LfsBytesToBlocks(lfsPtr, 
							fragSize));

	    (void) LfsSegUsageFreeBlocks(lfsPtr, fragSize, 1,
			blockArray + (lastByteBlock - startBlockNum));
	}
	(void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 
			FSDM_INDICES_PER_BLOCK - startElement, blockArray);
    }
    if (blockInCache) { 
	/*
	 * If we deleted all the indexes in this block we can delete the block.
	 */
	if (startElement == 0) {
	    Fscache_UnlockBlock(cacheBlockPtr, (unsigned )0, virtualBlockNum,
				 FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	    (void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, diskAddrPtr);
	} else {
	    Fscache_UnlockBlock(cacheBlockPtr,(unsigned)Fsutil_TimeInSeconds(), 
			    virtualBlockNum, FS_BLOCK_SIZE, 0);
	}
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsFile_TruncIndex --
 *
 *	Truncate the index of the specified file to only be the specified
 *	number of blocks in length.
 *
 * Results:
 *	SUCCESS if all goes well, a return status otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
LfsFile_TruncIndex(lfsPtr, handlePtr, length)
    Lfs		    	*lfsPtr;
    Fsio_FileIOHandle    *handlePtr;    /* Handle for file that are 
					 * interest in. */
    int			 length;      /* Number of bytes to 
					  * leave in file. */
{
    Fsdm_FileDescriptor	*descPtr;
    ReturnStatus	status = SUCCESS;
    int			lastByteBlock, fragSize;
    int			numBlocks;

    LFS_STATS_INC(lfsPtr->stats.index.truncs);
    numBlocks = (length + (FS_BLOCK_SIZE-1))/FS_BLOCK_SIZE;
    descPtr = handlePtr->descPtr;
    lastByteBlock = descPtr->lastByte/FS_BLOCK_SIZE;


    /*
     * Delete any DBL_INDIRECT blocks left over from this truncate. This is
     * necessary only if the old length had double indirect blocks.
     */
    if ((numBlocks < (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK +
		     FSDM_INDICES_PER_BLOCK * FSDM_INDICES_PER_BLOCK)) &&
	(lastByteBlock >= (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK))) {
	status = DeleteIndirectBlock(lfsPtr, handlePtr, -2, 
			&(descPtr->indirect[1]),
			FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK,
			numBlocks, FSDM_INDICES_PER_BLOCK, lastByteBlock);
    }
    /*
     * Followed by any INDIRECT blocks if the old length had
     * indirect blocks.
     */
    if ((numBlocks < (FSDM_INDICES_PER_BLOCK + FSDM_NUM_DIRECT_BLOCKS)) &&
        (lastByteBlock >= FSDM_NUM_DIRECT_BLOCKS)) {
	status = DeleteIndirectBlock(lfsPtr, handlePtr, -1, 
			&(descPtr->indirect[0]),
			FSDM_NUM_DIRECT_BLOCKS, numBlocks, 1, lastByteBlock);
    }
    /*
     * Finally the DIRECT blocks.
     */
    if (numBlocks < FSDM_NUM_DIRECT_BLOCKS) {
	/*
	 * The last block in the file may be a fragement. Free it first.
	 */
	if ((descPtr->lastByte >= 0) &&
	    (lastByteBlock < FSDM_NUM_DIRECT_BLOCKS) && 
	    (lastByteBlock >= numBlocks)) {
	    /*
	     * Compute the size of the fragment and round it into lfs
	     * blocks.
	     */
	    fragSize = descPtr->lastByte - (lastByteBlock * FS_BLOCK_SIZE) + 1;
	    /*
	     * Round to the number of LFS blocks it would take.
	     */
	    fragSize = LfsBlocksToBytes(lfsPtr, 
				LfsBytesToBlocks(lfsPtr, fragSize));
	    (void) LfsSegUsageFreeBlocks(lfsPtr, fragSize, 1, 
		    ((LfsDiskAddr *)descPtr->direct) + lastByteBlock);
	}
	(void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 
		    FSDM_NUM_DIRECT_BLOCKS - numBlocks,	
		    ((LfsDiskAddr *)descPtr->direct) + numBlocks);
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsFile_GrowBlock --
 *
 *	Grow the specified block of the specified file.
 *
 * Results:
 *	A status indicating whether there was sufficient space to allocate
 *	block
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFile_GrowBlock(lfsPtr, handlePtr, offset, numBytes)
    Lfs				*lfsPtr;
    Fsio_FileIOHandle	*handlePtr;   /* Handle for file that are 
				       * interest in. */
    int		offset;		     /* Offset to allocate at. */
    int		numBytes;	     /* Number of bytes to make block. */
{
    ReturnStatus		status;
    int	newLastByte,  blockNum;
    register	Fsdm_FileDescriptor *descPtr;
    LfsDiskAddr diskAddress;
    int	 oldSize, oldLastBlock;


    /*
     * Process the common case that we are appending to the end of the file
     * starting at a block boundry.  We know the block can't already exist
     * so we don't need to grow it. 
     */
    descPtr = handlePtr->descPtr;
    oldSize = descPtr->lastByte + 1;
    LFS_STATS_INC(lfsPtr->stats.blockio.fastAllocs);
    if ((offset % FS_BLOCK_SIZE == 0) && (offset == oldSize)) {
	return LfsSegUsageAllocateBytes(lfsPtr, numBytes);
    }
    newLastByte = offset + numBytes - 1;
    blockNum = offset / FS_BLOCK_SIZE;
    oldLastBlock = oldSize / FS_BLOCK_SIZE;
    if (newLastByte > descPtr->lastByte) {
	int blocksToGrow, bytesToGrow, lastFragSize, extraBytes;
	/*
	 * We are writing passed the end of the file. If the last block is 
	 * a fragment we must grow it.
	 */
	lastFragSize = oldSize % FS_BLOCK_SIZE;
	if (lastFragSize > 0) { 
	    int newLastFragSize;
	    newLastFragSize = lastFragSize + (newLastByte - descPtr->lastByte);
	    if (newLastFragSize > FS_BLOCK_SIZE) {
		newLastFragSize = FS_BLOCK_SIZE;
	    }
	    blocksToGrow = LfsBytesToBlocks(lfsPtr, newLastFragSize) - 
			    LfsBytesToBlocks(lfsPtr, lastFragSize);
	} else {
	    /*
	     * The last block in the file was not a fragment. No need to
	     * grow it.
	     */
	    blocksToGrow = 0;
	}
	if (blocksToGrow > 0) {
	    bytesToGrow = LfsBlocksToBytes(lfsPtr,blocksToGrow);
	    /*
	     * Allocate and grow the last block of the file. At the same
	     * time we can allocate space if a new last block is being 
	     * created.
	     */
	    extraBytes = (oldLastBlock == blockNum) ? 0 : numBytes;
	    status = LfsSegUsageAllocateBytes(lfsPtr, bytesToGrow + extraBytes);
	    if (status == SUCCESS) {
		 /* If this block is already allocated to disk and we are 
		  * growing it, increment the segment usage count to reflect 
		  * the larger block size so it will be correctly freed 
		  * when it is overwritten or deleted. 
		  */
		LFS_STATS_INC(lfsPtr->stats.blockio.slowAllocs);
		status = AccessBlock(GROW_ADDR, lfsPtr, handlePtr, 
				  oldLastBlock, bytesToGrow, 0, &diskAddress);
	    } 
	} else if (oldLastBlock != blockNum) {
	    /*
	     * The last block of the file doesn't need to be grown any but
	     * the write is creating a new last block that we must allocate
	     * space for.
	     */
	    status = LfsSegUsageAllocateBytes(lfsPtr, numBytes);
	} else {
	    /*
	     * The write is to the last block of the file but doesn't cause
	     * the file to take any more space on disk because space is 
	     * rounded to block sizes.
	     */
	    status = SUCCESS;
	}
    } else {
	/*
	 * We are writing into the middle of the file. Check to see if 
	 * the block previous existed. We can skip this check if we
	 * know that the file system has enough room for this block.
	 */
	LFS_STATS_INC(lfsPtr->stats.blockio.fastAllocs);
	status = LfsSegUsageAllocateBytes(lfsPtr, numBytes);
	if (status != SUCCESS) { 
	    LFS_STATS_INC(lfsPtr->stats.blockio.slowAllocs);
	    status = AccessBlock(GET_ADDR, lfsPtr, handlePtr, blockNum, 
				FS_BLOCK_SIZE, 0, &diskAddress);
	    if ((status != SUCCESS) || LfsIsNilDiskAddr(diskAddress)) {
		LFS_STATS_INC(lfsPtr->stats.blockio.slowAllocFails);
		status = FS_NO_DISK_SPACE;
	    }
	}

    }

    return status;
}

