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

enum IndexOp { GET_ADDR, SET_ADDR};

static ReturnStatus	AccessBlock();


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
LfsFile_GetIndex(handlePtr, blockNum, cantBlock, diskAddressPtr)
    Fsio_FileIOHandle	*handlePtr;   /* Handle for file that are 
					      * interest in. */
    int		        blockNum;    /* Block number of interest. */
    Boolean	cantBlock;	     /* TRUE if we can't block. */
    int *diskAddressPtr; 	     /* Disk address returned. */
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
    status = AccessBlock(GET_ADDR, lfsPtr, handlePtr, blockNum, cantBlock,
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
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsFile_SetIndex(handlePtr, blockNum, cantBlock, diskAddress)
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int		    blockNum; 		     /* Block number of interest. */
    Boolean	cantBlock;	     /* TRUE if we can't block. */
    int		 diskAddress; 		     /* Disk address of block. */
{
    Lfs		    	*lfsPtr;
    Fsdm_Domain			*domainPtr;
    ReturnStatus		status;
    int				segNo;


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
   if (diskAddress != FSDM_NIL_INDEX) { 
	segNo = LfsBlockToSegmentNum(lfsPtr, diskAddress);
	if (!LfsValidSegmentNum(lfsPtr,segNo)) {
	    panic("LfsFile_SetIndex: bad segment number.\n");
	}
    }
#endif
    status = AccessBlock(SET_ADDR, lfsPtr, handlePtr, blockNum, cantBlock,
		&diskAddress);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * AccessBlock --
 *
 *	Access and perform a GET ro SET operation on the specified block
 *	of the specified file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
AccessBlock(op, lfsPtr, handlePtr, blockNum, cantBlock, diskAddressPtr)
    enum IndexOp 	      op;	     /* Operation to be performed. 
					      * Must be GET_ADDR or 
					      * SET_ADDR. */
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    Lfs		*lfsPtr;		     /* File system of file. */
    int		blockNum;		     /* Block number of file. */
    Boolean	cantBlock;	     /* TRUE if we can't block. */
    int *diskAddressPtr;	  	    /* Disk address in/out. */
{
    int parentIndex, parentBlockNum, parentDiskAddress;
    Fsdm_FileDescriptor 	*descPtr;
    Fscache_Block *parentblockPtr;
    int	modTime;
    Boolean	found;
    ReturnStatus	status;
    int		cacheFlags;

    descPtr = handlePtr->descPtr;

    /*
     * First process the data and indirect blocks that are pointed to 
     * by the descriptor.
     */
    if (blockNum < 0) {
	if (blockNum >= -FSDM_NUM_INDIRECT_BLOCKS) { 
	    /*
	     * This is a direct indirect block.
	     */
	    if (op == GET_ADDR) { 
		*diskAddressPtr = (descPtr->indirect[(-blockNum)-1]);
	    } else {
		int *addrPtr = (descPtr->indirect + ((-blockNum)-1));
		LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, addrPtr);
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
	    if (op == GET_ADDR) { 
		*diskAddressPtr = (descPtr->direct[blockNum]);
	    } else {
		int *addrPtr = (descPtr->direct + blockNum);
		LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, addrPtr);
		*addrPtr = *diskAddressPtr;
		descPtr->flags |= FSDM_FD_INDEX_DIRTY;
		(void) Fsdm_FileDescStore(handlePtr, FALSE);
	    }
	    return(SUCCESS);
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
    cacheFlags = (op == GET_ADDR) ? FSCACHE_IND_BLOCK :
		   (FSCACHE_IND_BLOCK|FSCACHE_IO_IN_PROGRESS);

    if (cantBlock) {
	cacheFlags |= (FSCACHE_CANT_BLOCK|FSCACHE_DONT_BLOCK);
    }

    /* 
     * Lookup the parent block in the cache.
     */
    if (op == GET_ADDR) {
	LFS_STATS_INC(lfsPtr->stats.index.getFetchBlock);
    } else {
	LFS_STATS_INC(lfsPtr->stats.index.setFetchBlock);
    } 
    Fscache_FetchBlock(&handlePtr->cacheInfo, parentBlockNum,
		cacheFlags, &parentblockPtr, &found);
    if (found) {
	 if (op == GET_ADDR) {
	    LFS_STATS_INC(lfsPtr->stats.index.getFetchHit);
	    *diskAddressPtr =  ((int *)parentblockPtr->blockAddr)[parentIndex];
	    modTime = 0;
	 } else  { /* SET_ADDR */
	    int *addrPtr = ((int *)parentblockPtr->blockAddr) + parentIndex; 
	    LFS_STATS_INC(lfsPtr->stats.index.setFetchHit);
	    LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, addrPtr);
	    *addrPtr = *diskAddressPtr;
	    modTime = fsutil_TimeInSeconds;
	 }
	 Fscache_UnlockBlock(parentblockPtr, (unsigned )modTime, parentBlockNum,
			     FS_BLOCK_SIZE, 0);
	 return SUCCESS;
    }
    /*
     * Not found in cache. Try to read it in. First we need to find the
     * address of the block.
     */
    status = AccessBlock(GET_ADDR, lfsPtr, handlePtr, parentBlockNum, cantBlock,
				(int *) &parentDiskAddress);
    if (parentDiskAddress == FSDM_NIL_INDEX) {
	if (op == GET_ADDR) {
	    *diskAddressPtr = FSDM_NIL_INDEX;
	     Fscache_UnlockBlock(parentblockPtr, (unsigned )0, parentBlockNum,
			     FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	} else { /* SET_ADDR */
	    register int *intPtr, *limitPtr;
	    limitPtr = (int *) (parentblockPtr->blockAddr + FS_BLOCK_SIZE);
	    for (intPtr = (int *)parentblockPtr->blockAddr; intPtr < limitPtr;
		intPtr++) {
		*intPtr = FSDM_NIL_INDEX;
	    }
	    ((int *)parentblockPtr->blockAddr)[parentIndex] = *diskAddressPtr;
	     Fscache_UnlockBlock(parentblockPtr, 
				(unsigned )fsutil_TimeInSeconds,
				parentBlockNum, FS_BLOCK_SIZE, 0);
	}
	return SUCCESS;
     }

     status = LfsReadBytes(lfsPtr, ( int)parentDiskAddress, 
		FS_BLOCK_SIZE,  parentblockPtr->blockAddr);
     if (status != SUCCESS) {
	 LfsError(lfsPtr, status, "Can't read indirect block.\n");
	 return status;
     }
     if (op == GET_ADDR) {
        *diskAddressPtr =  ((int *)parentblockPtr->blockAddr)[parentIndex];
	modTime = 0;
     } else {
	int *addrPtr = ((int *)parentblockPtr->blockAddr) + 
					parentIndex; 
         LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, addrPtr);
	*addrPtr = *diskAddressPtr;
	modTime = fsutil_TimeInSeconds;
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
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DeleteIndirectBlock(lfsPtr, handlePtr, virtualBlockNum, diskAddr, 
	startBlockNum, lastBlockNum, step) 
    Lfs	  *lfsPtr;
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int	  virtualBlockNum;	/* Virtual block number for this indirect 
				 * block. */
    int	  diskAddr;		/* Disk address of block. */
    int	  startBlockNum;	/* Starting block number of this virtual
				 * Block. */
    int	  lastBlockNum;		/* New last block number of file. */
    int	  step;			/* Number of blocks covered by each virtual
				 * block entry. */
{
    Fscache_Block	*cacheBlockPtr;
    Boolean		found;
    ReturnStatus	status = SUCCESS;
    int			startElement, cstep, childBlockNum, i;
    int	*blockArray;
    /*
     * If this index block hasn't been allocated yet and not in the  
     * cache we don't need to free anything.
     */
    LFS_STATS_INC(lfsPtr->stats.index.deleteFetchBlock);
    Fscache_FetchBlock(&handlePtr->cacheInfo, virtualBlockNum,
       (int)(FSCACHE_IO_IN_PROGRESS|FSCACHE_IND_BLOCK), &cacheBlockPtr,&found);
    if (!found && (diskAddr == FSDM_NIL_INDEX)) {
	Fscache_UnlockBlock(cacheBlockPtr, (unsigned )0, virtualBlockNum,
			     FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	return SUCCESS;
    }

    if (!found) {
	/*
	 * Read it into the cache if it on disk somewhere.
	 */
	LFS_STATS_INC(lfsPtr->stats.index.deleteFetchBlockMiss);
	status = LfsReadBytes(lfsPtr, diskAddr, FS_BLOCK_SIZE, 
		       cacheBlockPtr->blockAddr);
	 if (status != SUCCESS) {
	     LfsError(lfsPtr, status, "Can't read indirect block.\n");
	     return status;
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
	cstep = step/FSDM_INDICES_PER_BLOCK;
	startBlockNum = startBlockNum + startElement * step;
	childBlockNum = -((FSDM_NUM_INDIRECT_BLOCKS+1)+startElement);
	for (i = startElement; i < FSDM_INDICES_PER_BLOCK; i++) { 
	    status = DeleteIndirectBlock(lfsPtr, handlePtr, childBlockNum, 
		    ((int *) cacheBlockPtr->blockAddr)[i],
		     startBlockNum, lastBlockNum, cstep);
	    startBlockNum += step;
	    childBlockNum--;
	}
    }
    blockArray =  (int *) cacheBlockPtr->blockAddr + startElement;
    if (step == 1) {
	int lastByteBlock;
	/*
	 * Free the last block in the file handling the case that it
	 * is a fragment.
	 */
	lastByteBlock = handlePtr->descPtr->lastByte/FS_BLOCK_SIZE;
	if ((lastByteBlock >= startBlockNum) && 
	    (lastByteBlock < startBlockNum + FSDM_INDICES_PER_BLOCK) &&
	    (lastByteBlock >= lastBlockNum)) {
	    int fragSize;
	    fragSize = handlePtr->descPtr->lastByte - 
				(lastByteBlock * FS_BLOCK_SIZE);
	    fragSize = LfsBlocksToBytes(lfsPtr, LfsBytesToBlocks(lfsPtr, 
							fragSize));

	    (void) LfsSegUsageFreeBlocks(lfsPtr, fragSize, 1,
			blockArray + (lastByteBlock - startBlockNum));
	}
    }
    (void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 
			FSDM_INDICES_PER_BLOCK - startElement, blockArray);
    /*
     * If we deleted all the indexes in this block we can delete the block.
     */
    if (startElement == 0) {
	Fscache_UnlockBlock(cacheBlockPtr, (unsigned )0, virtualBlockNum,
			     FS_BLOCK_SIZE, FSCACHE_DELETE_BLOCK);
	(void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 1, 
			( int *) &diskAddr);
    } else {
	Fscache_UnlockBlock(cacheBlockPtr, (unsigned )fsutil_TimeInSeconds, 
			virtualBlockNum, FS_BLOCK_SIZE, 0);
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
LfsFile_TruncIndex(lfsPtr, handlePtr, numBlocks)
    Lfs		    	*lfsPtr;
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * interest in. */
    int			      numBlocks;      /* Number of blocks to 
					      * leave for file. */
{
    Fsdm_FileDescriptor	*descPtr;
    ReturnStatus	status = SUCCESS;
    int			lastByteBlock, fragSize;

    LFS_STATS_INC(lfsPtr->stats.index.truncs);

    descPtr = handlePtr->descPtr;
    /*
     * Delete any DBL_INDIRECT blocks first.
     */
    if (numBlocks < (FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK +
		     FSDM_INDICES_PER_BLOCK * FSDM_INDICES_PER_BLOCK)) {
	status = DeleteIndirectBlock(lfsPtr, handlePtr, -2, 
			descPtr->indirect[1],
			FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK,
			numBlocks, FSDM_INDICES_PER_BLOCK);
    }
    /*
     * Followed by any INDIRECT blocks.
     */
    if (numBlocks < (FSDM_INDICES_PER_BLOCK + FSDM_NUM_DIRECT_BLOCKS)) {
	status = DeleteIndirectBlock(lfsPtr, handlePtr, -1, 
			descPtr->indirect[0],
			FSDM_NUM_DIRECT_BLOCKS, numBlocks, 1);
    }
    /*
     * Finally the DIRECT blocks.
     */
    if (numBlocks < FSDM_NUM_DIRECT_BLOCKS) {
	/*
	 * The last block in the file may be a fragement. Free it first.
	 */
	lastByteBlock = descPtr->lastByte/FS_BLOCK_SIZE;
	if ((lastByteBlock < FSDM_NUM_DIRECT_BLOCKS) && 
	    (lastByteBlock >= numBlocks)) {
	    /*
	     * Compute the size of the fragment and round it into lfs
	     * blocks.
	     */
	    fragSize = descPtr->lastByte - (lastByteBlock * FS_BLOCK_SIZE);
	    /*
	     * Round to the number of LFS blocks it would talk.
	     */
	    fragSize = LfsBlocksToBytes(lfsPtr, 
				LfsBytesToBlocks(lfsPtr, fragSize));
	    (void) LfsSegUsageFreeBlocks(lfsPtr, fragSize, 1, 
		    descPtr->direct + lastByteBlock);
	}
	(void) LfsSegUsageFreeBlocks(lfsPtr, FS_BLOCK_SIZE, 
		    FSDM_NUM_DIRECT_BLOCKS - numBlocks,	
		    descPtr->direct + numBlocks);
    }

    return status;
}

