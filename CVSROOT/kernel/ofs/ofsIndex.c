/* 
 * fsIndex.c --
 *
 *	Routines to allow moving through a files block pointers.  The method
 *	of using these routines is the following:
 *
 *	    1) Call FsGetFirstIndex to get the first block.
 *	    2) Call FsGetNextIndex to get subsequent blocks.
 *	    3) Call FsEndIndex when finished.
 *
 *	There are flags to FsGetFirstIndex that allow indirect blocks
 *	to be deleted and allocated as appropriate.
 *
 *	The data structure operated on is the disk map kept in the disk
 *	file descriptor (FsFileDescriptor).  This has 10 direct block pointers,
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
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsDevice.h"
#include "fsLocalDomain.h"
#include "fsBlockCache.h"
#include "fsStat.h"
#include "spriteTime.h"

static ReturnStatus	FetchIndirectBlock();
static ReturnStatus	MakePtrAccessible();
static void		FreeIndirectBlock();


/*
 *----------------------------------------------------------------------
 *
 * FsGetFirstIndex --
 *
 *	Initialize the index structure.  This will set up the index info
 *	structure so that it contains a pointer to the desired block pointer.
 *
 * Results:
 *	A status indicating whether there was sufficient space to allocate
 *	indirect blocks.
 *
 * Side effects:
 *	The index structure is initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsGetFirstIndex(handlePtr, blockNum, indexInfoPtr, flags)
    FsLocalFileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * indexing. */
    int			      blockNum;      /* Where to start indexing. */
    register FsBlockIndexInfo *indexInfoPtr; /* Index structure to initialize.*/
    int			      flags;	     /* FS_ALLOC_INDIRECT_BLOCKS,
						FS_DELETE_INDIRECT_BLOCKS,
						FS_DELETE_EVERYTHING */
{
    register FsFileDescriptor 	*descPtr;
    int			      	indirectBlock;
    ReturnStatus		status;

    indexInfoPtr->domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major,
					    FALSE);
    if (indexInfoPtr->domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;
    indexInfoPtr->lastDiskBlock = FS_NIL_INDEX;
    indexInfoPtr->indInfo[0].blockPtr = (FsCacheBlock *) NIL;
    indexInfoPtr->indInfo[0].deleteBlock = 0;
    indexInfoPtr->indInfo[1].blockPtr = (FsCacheBlock *) NIL;
    indexInfoPtr->indInfo[1].deleteBlock = 0;
    indexInfoPtr->flags = flags;
    indexInfoPtr->blockNum = blockNum;

    if (blockNum < FS_NUM_DIRECT_BLOCKS) {
	/*
	 * This is a direct block.
	 */
	indexInfoPtr->indexType = FS_DIRECT;
	indexInfoPtr->directIndex = blockNum;
	indexInfoPtr->blockAddrPtr = &(descPtr->direct[blockNum]);
	return(SUCCESS);
    }

    /*
     * Is an indirect block.
     */

    blockNum -= FS_NUM_DIRECT_BLOCKS;
    indirectBlock = blockNum / FS_INDICES_PER_BLOCK;
    if (indirectBlock == 0) {
	/*
	 * This is a singly indirect block.
	 */
	indexInfoPtr->indexType = FS_INDIRECT;
	indexInfoPtr->indInfo[0].index = blockNum;
	if (flags & FS_DELETE_EVERYTHING) {
	    indexInfoPtr->indInfo[0].deleteBlock = FS_DELETE_BLOCK;
	}
    } else if (indirectBlock < FS_INDICES_PER_BLOCK + 1) {
	/*
	 * This a doubly indirect block.
	 */
	indexInfoPtr->indexType = FS_DBL_INDIRECT;
	indexInfoPtr->indInfo[0].index = indirectBlock - 1;
	indexInfoPtr->indInfo[1].index = 
			    blockNum - indirectBlock * FS_INDICES_PER_BLOCK;
	if (flags & FS_DELETE_EVERYTHING) {
	    indexInfoPtr->indInfo[0].deleteBlock = FS_DELETE_BLOCK;
	    indexInfoPtr->indInfo[1].deleteBlock = FS_DELETE_BLOCK;
	}
    } else {
	/*
	 * Past the largest file size that we support.
	 */
	return(FS_INVALID_ARG);
    }

    /*
     * Finish off by making the block pointer accessible.  This may include
     * reading indirect blocks into the cache.
     */

    status = MakePtrAccessible(handlePtr, indexInfoPtr);
    if (status != SUCCESS) {
	FsDomainRelease(handlePtr->hdr.fileID.major);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsGetNextIndex --
 *
 *	Put the correct pointers in the index structure to access the
 *	block after the current block.
 *
 * Results:
 *	A status indicating whether there was sufficient space to allocate
 *	indirect blocks if they were needed.
 *
 * Side effects:
 *	The allocation structure is modified.  Indirect blocks may
 *	be read into the cache.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsGetNextIndex(handlePtr, indexInfoPtr, dirty)
    FsLocalFileIOHandle	      *handlePtr;    /* Handle for file that is being
						indexed. */
    register FsBlockIndexInfo *indexInfoPtr; /* Index structure to set up. */
    Boolean		      dirty;	     /* True if allocated a new block
						so dirtied block pointer. */
{
    Boolean			accessible = FALSE;
    register FsFileDescriptor	*descPtr = handlePtr->descPtr;

    indexInfoPtr->blockNum++;

    if (indexInfoPtr->blockAddrPtr != (int *) NIL &&
	*indexInfoPtr->blockAddrPtr != FS_NIL_INDEX) {
	indexInfoPtr->lastDiskBlock = 
			*(indexInfoPtr->blockAddrPtr) / FS_FRAGMENTS_PER_BLOCK;
    } else {
	indexInfoPtr->lastDiskBlock = FS_NIL_INDEX;
    }

    if (dirty) {
	if (indexInfoPtr->indexType == FS_INDIRECT) {
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	} else if (indexInfoPtr->indexType == FS_DBL_INDIRECT) {
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	}
    }
    /*
     * Determine whether we are now in direct, indirect or doubly indirect
     * blocks.
     */

    switch (indexInfoPtr->indexType) {
	case FS_DIRECT:
	    if (indexInfoPtr->blockNum < FS_NUM_DIRECT_BLOCKS) {
		/*
		 * Still in the direct blocks.
		 */
		indexInfoPtr->directIndex++;
		indexInfoPtr->blockAddrPtr = 
				&(descPtr->direct[indexInfoPtr->directIndex]);
		accessible = TRUE;
	    } else {
		/*
		 * Moved into indirect blocks.
		 */
		indexInfoPtr->indexType = FS_INDIRECT;
		indexInfoPtr->indInfo[0].index = 0;
		if (indexInfoPtr->flags & FS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[0].deleteBlock = FS_DELETE_BLOCK;
		}
	    }
	    break;
	case FS_INDIRECT:
	    if (indexInfoPtr->blockNum < 
			FS_NUM_DIRECT_BLOCKS + FS_INDICES_PER_BLOCK) {
		/*
		 * Still in singly indirect blocks.
		 */
		indexInfoPtr->indInfo[0].index++;
		if (indexInfoPtr->indInfo[0].blockPtr != (FsCacheBlock *) NIL) {
		    indexInfoPtr->blockAddrPtr++;
		}
		accessible = TRUE;
		break;
	   } else {
		/*
		 * Moved into doubly indirect blocks.
		 */
		indexInfoPtr->indInfo[0].index = 0;
		indexInfoPtr->indInfo[1].index = 0;
		indexInfoPtr->indexType = FS_DBL_INDIRECT;
		/*
		 * Free up the indirect pointer block.
		 */
		FreeIndirectBlock(0, handlePtr, indexInfoPtr, 
		    &descPtr->indirect[0]);
		if (indexInfoPtr->flags & FS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[0].deleteBlock = FS_DELETE_BLOCK;
		    indexInfoPtr->indInfo[1].deleteBlock = FS_DELETE_BLOCK;
		}
	    }
	    break;
	case FS_DBL_INDIRECT:
	    indexInfoPtr->indInfo[1].index++;
	    if (indexInfoPtr->indInfo[1].index == FS_INDICES_PER_BLOCK) {
		indexInfoPtr->indInfo[0].index++;
		indexInfoPtr->indInfo[1].index = 0;
		/*
		 * Free up the indirect pointer block.
		 */
		FreeIndirectBlock(1, handlePtr, indexInfoPtr,
			(int *) (indexInfoPtr->indInfo[0].blockPtr->blockAddr +
			sizeof(int) * (indexInfoPtr->indInfo[0].index - 1)));
		if (indexInfoPtr->indInfo[0].index == FS_INDICES_PER_BLOCK) {
		    /*
		     * We are at the end of the doubly indirect block and the
		     * caller wants us to go off of the end.  Free up the
		     * indirect block and return an error.
		     */
		    FreeIndirectBlock(0, handlePtr, indexInfoPtr,
			     &(descPtr->indirect[FS_DBL_INDIRECT]));
		    return(FS_INVALID_ARG);
		}
		if (indexInfoPtr->flags & FS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[1].deleteBlock = FS_DELETE_BLOCK;
		}
	    } else {
		if (indexInfoPtr->indInfo[1].blockPtr != 
						(FsCacheBlock *) NIL) {
		    indexInfoPtr->blockAddrPtr++;
		}
		accessible = TRUE;
	    }
	    break;
    }

    /*
     * Make the block pointers accessible if necessary.
     */

    if (!accessible) {
	return(MakePtrAccessible(handlePtr, indexInfoPtr));
    } else {
	return(SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FsEndIndex --
 *
 *	Free up cache blocks locked for indexing.  This also frees
 *	a reference to the domain acquired with FsGetFirstIndex.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cache blocks unlocked.
 *
 *----------------------------------------------------------------------
 */

void
FsEndIndex(handlePtr, indexInfoPtr, dirty) 
    FsLocalFileIOHandle	      *handlePtr;    /* Handle for file that is being
						indexed. */
    register FsBlockIndexInfo *indexInfoPtr; /* Index structure to cleanup. */
    Boolean		      dirty;	     /* True if allocated a new block
						so dirtied block pointer. */
{
    register FsFileDescriptor	*descPtr = handlePtr->descPtr;

    if (dirty) {
	if (indexInfoPtr->indexType == FS_INDIRECT) {
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	} else if (indexInfoPtr->indexType == FS_DBL_INDIRECT) {
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	}
    }
	
    /*
     * Free up the second level indirect block and then the first level
     * indirect block.
     */

    if (indexInfoPtr->indInfo[1].blockPtr != (FsCacheBlock *) NIL) {
	FreeIndirectBlock(1, handlePtr, indexInfoPtr,
			(int *) (indexInfoPtr->indInfo[0].blockPtr->blockAddr +
			     sizeof(int) * indexInfoPtr->indInfo[0].index));
    }
    FreeIndirectBlock(0, handlePtr, indexInfoPtr,
		     &(descPtr->indirect[indexInfoPtr->indexType]));
    FsDomainRelease(handlePtr->hdr.fileID.major);
}

/*
 *----------------------------------------------------------------------
 *
 * MakePtrAccessible --
 *
 *	Make the block pointer in the file descriptor accessible.  This
 *	may entail reading in indirect blocks and locking them down in the
 *	cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Indirect blocks are locked down in the cache.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
MakePtrAccessible(handlePtr, indexInfoPtr)
    register	FsLocalFileIOHandle	 *handlePtr;
    register	FsBlockIndexInfo 	*indexInfoPtr;
{
    register	FsFileDescriptor 	*descPtr;
    register	int 			*blockAddrPtr;
    ReturnStatus			status;
    int					cacheBlockNum;

    descPtr = handlePtr->descPtr;

    if (indexInfoPtr->indexType == FS_INDIRECT) {
	blockAddrPtr = &(descPtr->indirect[0]);
	if (*blockAddrPtr == FS_NIL_INDEX && 
	    !(indexInfoPtr->flags & FS_ALLOC_INDIRECT_BLOCKS)) {
	    indexInfoPtr->blockAddrPtr = (int *) NIL;
	    return(SUCCESS);
	}
	cacheBlockNum = -1;
    } else {
	blockAddrPtr = &(descPtr->indirect[1]);
	cacheBlockNum = -2;
    }

    /* 
     * Lock the first level block into the cache.
     */
    status = FetchIndirectBlock(0, handlePtr, indexInfoPtr, blockAddrPtr,
				cacheBlockNum);
    if (status != SUCCESS) {
	return(status);
    }
    blockAddrPtr = (int *) (indexInfoPtr->indInfo[0].blockPtr->blockAddr + 
			    sizeof(int) * indexInfoPtr->indInfo[0].index);

    if (indexInfoPtr->indexType == FS_INDIRECT) {
	indexInfoPtr->blockAddrPtr = blockAddrPtr;
	return(SUCCESS);
    }

    /* 
     * Lock the second level block into the cache.
     */
    if (*blockAddrPtr == FS_NIL_INDEX &&
	!(indexInfoPtr->flags & FS_ALLOC_INDIRECT_BLOCKS)) {
	indexInfoPtr->blockAddrPtr = (int *) NIL;
	return(SUCCESS);
    }

    cacheBlockNum = 
	    -(3 + indexInfoPtr->indInfo[0].index);
    status = FetchIndirectBlock(1, handlePtr, indexInfoPtr, blockAddrPtr,
				cacheBlockNum);
    if (status == SUCCESS) {
	indexInfoPtr->blockAddrPtr = 
    		(int *) (indexInfoPtr->indInfo[1].blockPtr->blockAddr + 
			 sizeof(int) * indexInfoPtr->indInfo[1].index);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FetchIndirectBlock --
 *
 *	Read the indirect block into the cache.  Called by MakePtrAccessible
 *	to fetch both indirect and doubly-indirect blocks.
 *
 * Results:
 *	Error if could not read in or allocate indirect block.
 *
 * Side effects:
 *	Indirect block is locked down in the cache.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
FetchIndirectBlock(indBlockNum, handlePtr, indexInfoPtr, blockAddrPtr,
		   cacheBlockNum)
    int				indBlockNum;	/* 0 if first level, 1 if 
						 * second level indirect 
						 * block. */
    FsLocalFileIOHandle		*handlePtr;	/* File to fetch indirect
					  	 * block for. */
    register FsBlockIndexInfo 	*indexInfoPtr;  /* Indexing information for
						 * this file. */
    register int		*blockAddrPtr;	/* Disk block number. */
    int				cacheBlockNum;	/* The block number by which
						 * the cache knows this block.*/
{
    unsigned	char		*bitmapPtr;
    int				blockNum;
    Boolean			found;
    ReturnStatus		status = SUCCESS;
    register FsIndirectInfo	*indInfoPtr;
    register int		*intPtr;

    indInfoPtr = &(indexInfoPtr->indInfo[indBlockNum]);
    if (indInfoPtr->blockPtr == (FsCacheBlock *) NIL) {
	if (*blockAddrPtr == FS_NIL_INDEX) {
	    FsFindBlock(handlePtr->hdr.fileID.minor, indexInfoPtr->domainPtr, 
			-1, TRUE, &blockNum, &bitmapPtr);
	    if (blockNum == -1) {
		return(FS_NO_DISK_SPACE);
	    }
	    blockNum += indexInfoPtr->domainPtr->headerPtr->dataOffset;
	    *blockAddrPtr = blockNum * FS_FRAGMENTS_PER_BLOCK;
	    handlePtr->descPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
	    FsCacheFetchBlock(&handlePtr->cacheInfo, cacheBlockNum,
			FS_IND_CACHE_BLOCK, &(indInfoPtr->blockPtr), &found);
	    if (found) {
		/*
		 * The block should not be in the cache since we are just
		 * allocating it now.
		 */
		panic("Physical block already in cache.\n");
	    }
	    for (intPtr = (int *)indInfoPtr->blockPtr->blockAddr;
		 (int)intPtr < (int)indInfoPtr->blockPtr->blockAddr + FS_BLOCK_SIZE;
		 intPtr++) {
		*intPtr = FS_NIL_INDEX;
	    }
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	    FsCacheIODone(indInfoPtr->blockPtr);
	} else {
	    fsStats.blockCache.indBlockAccesses++;
	    FsCacheFetchBlock(&handlePtr->cacheInfo, cacheBlockNum,
			FS_IND_CACHE_BLOCK, &(indInfoPtr->blockPtr), &found);
	    if (!found) {
		status = FsDeviceBlockIO(FS_READ,
			&(indexInfoPtr->domainPtr->headerPtr->device), 
		       *blockAddrPtr, FS_FRAGMENTS_PER_BLOCK, 
		       indInfoPtr->blockPtr->blockAddr);
		if (status == SUCCESS) {
		    fsStats.gen.physBytesRead += FS_BLOCK_SIZE;
		}
		FsCacheIODone(indInfoPtr->blockPtr);
	    } else {
		fsStats.blockCache.indBlockHits++;
	    }
	    indInfoPtr->blockDirty = FALSE;
	}
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeIndirectBlock --
 *
 *	Remove the given block from the cache and, if appropriate, from disk.
 *	This is called whenever FsGetNextIndex or FsEndIndex are 
 *	finished with an indirect block that has previously been locked
 *	into the cache by MakePtrAccessible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Block may be freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeIndirectBlock(indBlockNum, handlePtr, indexInfoPtr, blockAddrPtr)
    int				indBlockNum;	/* Which indirect block to 
						 * free.*/
    FsLocalFileIOHandle		*handlePtr;	/* File to delete indirect
						 * block from. */
    register FsBlockIndexInfo 	*indexInfoPtr; 	/* Index structure to use. */
    int				*blockAddrPtr;	/* Pointer to block to free. */
{
    int				modTime;
    int				indBlock;
    register	FsIndirectInfo	*indInfoPtr;

    indInfoPtr = &indexInfoPtr->indInfo[indBlockNum];

    if (indInfoPtr->blockPtr != (FsCacheBlock *) NIL) {
	if (indInfoPtr->blockDirty) {
	    modTime = fsTimeInSeconds;
	    if (!indInfoPtr->deleteBlock) {
		fsStats.blockCache.indBlockWrites++;
	    }
	} else {
	    modTime = 0;
	}
	FsCacheUnlockBlock(indInfoPtr->blockPtr, 
	   (unsigned )modTime, -(*blockAddrPtr), FS_BLOCK_SIZE,
	   indInfoPtr->deleteBlock);
	if (indInfoPtr->deleteBlock) {
	    indBlock = *blockAddrPtr / FS_FRAGMENTS_PER_BLOCK;
	    FsFreeBlock(indexInfoPtr->domainPtr,
		    indBlock - indexInfoPtr->domainPtr->headerPtr->dataOffset);
	    *blockAddrPtr = FS_NIL_INDEX;
	    handlePtr->descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
	}
	indInfoPtr->blockPtr = (FsCacheBlock *) NIL;
    }
}
