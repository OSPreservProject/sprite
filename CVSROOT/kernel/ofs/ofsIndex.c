/* 
 * ofsIndex.c --
 *
 *	Routines to allow moving through a files block pointers.  The method
 *	of using these routines is the following:
 *
 *	    1) Call OfsGetFirstIndex to get the first block.
 *	    2) Call OfsGetNextIndex to get subsequent blocks.
 *	    3) Call OfsEndIndex when finished.
 *
 *	There are flags to OfsGetFirstIndex that allow indirect blocks
 *	to be deleted and allocated as appropriate.
 *
 *	The data structure operated on is the disk map kept in the disk
 *	file descriptor (Fsdm_FileDescriptor).  This has 10 direct block pointers,
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

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fslcl.h>
#include <fscache.h>
#include <fsStat.h>
#include <spriteTime.h>
#include <fsdm.h>
#include <ofs.h>

static ReturnStatus MakePtrAccessible _ARGS_((Fsio_FileIOHandle *handlePtr,
		 OfsBlockIndexInfo *indexInfoPtr));
static ReturnStatus FetchIndirectBlock _ARGS_((int indBlockNum,
		Fsio_FileIOHandle *handlePtr, 
		OfsBlockIndexInfo *indexInfoPtr, 
		int *blockAddrPtr, int cacheBlockNum));
static void FreeIndirectBlock _ARGS_((int indBlockNum, 
		Fsio_FileIOHandle *handlePtr, OfsBlockIndexInfo *indexInfoPtr,
		int *blockAddrPtr));



/*
 *----------------------------------------------------------------------
 *
 * OfsGetFirstIndex --
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
OfsGetFirstIndex(ofsPtr, handlePtr, blockNum, indexInfoPtr, flags)
    Ofs_Domain		      *ofsPtr;	     /* Domain of file. */
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that are 
					      * indexing. */
    int			      blockNum;      /* Where to start indexing. */
    register OfsBlockIndexInfo *indexInfoPtr; /* Index structure to initialize.*/
    int			      flags;	     /* OFS_ALLOC_INDIRECT_BLOCKS,
						OFS_DELETE_INDIRECT_BLOCKS,
						OFS_DELETE_EVERYTHING,
						FSCACHE_DONT_BLOCK */
{
    register Fsdm_FileDescriptor 	*descPtr;
    int			      	indirectBlock;
    ReturnStatus		status = SUCCESS;

    indexInfoPtr->ofsPtr = ofsPtr,

    descPtr = handlePtr->descPtr;
    indexInfoPtr->lastDiskBlock = FSDM_NIL_INDEX;
    indexInfoPtr->indInfo[0].blockPtr = (Fscache_Block *) NIL;
    indexInfoPtr->indInfo[0].deleteBlock = 0;
    indexInfoPtr->indInfo[1].blockPtr = (Fscache_Block *) NIL;
    indexInfoPtr->indInfo[1].deleteBlock = 0;
    indexInfoPtr->flags = flags;
    indexInfoPtr->blockNum = blockNum;

    if (blockNum < FSDM_NUM_DIRECT_BLOCKS) {
	/*
	 * This is a direct block.
	 */
	indexInfoPtr->indexType = OFS_DIRECT;
	indexInfoPtr->directIndex = blockNum;
	indexInfoPtr->blockAddrPtr = &(descPtr->direct[blockNum]);
	return(SUCCESS);
    }

    /*
     * Is an indirect block.
     */

    blockNum -= FSDM_NUM_DIRECT_BLOCKS;
    indirectBlock = blockNum / FSDM_INDICES_PER_BLOCK;
    if (indirectBlock == 0) {
	/*
	 * This is a singly indirect block.
	 */
	indexInfoPtr->indexType = OFS_INDIRECT;
	indexInfoPtr->indInfo[0].index = blockNum;
	if (flags & OFS_DELETE_EVERYTHING) {
	    indexInfoPtr->indInfo[0].deleteBlock = FSCACHE_DELETE_BLOCK;
	}
    } else if (indirectBlock < FSDM_INDICES_PER_BLOCK + 1) {
	/*
	 * This a doubly indirect block.
	 */
	indexInfoPtr->indexType = OFS_DBL_INDIRECT;
	indexInfoPtr->indInfo[0].index = indirectBlock - 1;
	indexInfoPtr->indInfo[1].index = 
			    blockNum - indirectBlock * FSDM_INDICES_PER_BLOCK;
	if (flags & OFS_DELETE_EVERYTHING) {
	    indexInfoPtr->indInfo[0].deleteBlock = FSCACHE_DELETE_BLOCK;
	    indexInfoPtr->indInfo[1].deleteBlock = FSCACHE_DELETE_BLOCK;
	}
    } else {
	/*
	 * Past the largest file size that we support.
	 */
	status = FS_INVALID_ARG;
    }

    /*
     * Finish off by making the block pointer accessible.  This may include
     * reading indirect blocks into the cache.
     */

    if (status == SUCCESS) {
	status = MakePtrAccessible(handlePtr, indexInfoPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * OfsGetNextIndex --
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
OfsGetNextIndex(handlePtr, indexInfoPtr, dirty)
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that is being
						indexed. */
    register OfsBlockIndexInfo *indexInfoPtr; /* Index structure to set up. */
    Boolean		      dirty;	     /* True if allocated a new block
						so dirtied block pointer. */
{
    Boolean			accessible = FALSE;
    register Fsdm_FileDescriptor	*descPtr = handlePtr->descPtr;

    indexInfoPtr->blockNum++;

    if (indexInfoPtr->blockAddrPtr != (int *) NIL &&
	*indexInfoPtr->blockAddrPtr != FSDM_NIL_INDEX) {
	indexInfoPtr->lastDiskBlock = 
			*(indexInfoPtr->blockAddrPtr) / FS_FRAGMENTS_PER_BLOCK;
    } else {
	indexInfoPtr->lastDiskBlock = FSDM_NIL_INDEX;
    }

    if (dirty) {
	if (indexInfoPtr->indexType == OFS_INDIRECT) {
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	} else if (indexInfoPtr->indexType == OFS_DBL_INDIRECT) {
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	}
    }
    /*
     * Determine whether we are now in direct, indirect or doubly indirect
     * blocks.
     */

    switch (indexInfoPtr->indexType) {
	case OFS_DIRECT:
	    if (indexInfoPtr->blockNum < FSDM_NUM_DIRECT_BLOCKS) {
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
		indexInfoPtr->indexType = OFS_INDIRECT;
		indexInfoPtr->indInfo[0].index = 0;
		if (indexInfoPtr->flags & OFS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[0].deleteBlock = FSCACHE_DELETE_BLOCK;
		}
	    }
	    break;
	case OFS_INDIRECT:
	    if (indexInfoPtr->blockNum < 
			FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK) {
		/*
		 * Still in singly indirect blocks.
		 */
		indexInfoPtr->indInfo[0].index++;
		if (indexInfoPtr->indInfo[0].blockPtr != (Fscache_Block *) NIL) {
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
		indexInfoPtr->indexType = OFS_DBL_INDIRECT;
		/*
		 * Free up the indirect pointer block.
		 */
		FreeIndirectBlock(0, handlePtr, indexInfoPtr, 
		    &descPtr->indirect[0]);
		if (indexInfoPtr->flags & OFS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[0].deleteBlock = FSCACHE_DELETE_BLOCK;
		    indexInfoPtr->indInfo[1].deleteBlock = FSCACHE_DELETE_BLOCK;
		}
	    }
	    break;
	case OFS_DBL_INDIRECT:
	    indexInfoPtr->indInfo[1].index++;
	    if (indexInfoPtr->indInfo[1].index == FSDM_INDICES_PER_BLOCK) {
		indexInfoPtr->indInfo[0].index++;
		indexInfoPtr->indInfo[1].index = 0;
		/*
		 * Free up the indirect pointer block.
		 */
		FreeIndirectBlock(1, handlePtr, indexInfoPtr,
			(int *) (indexInfoPtr->indInfo[0].blockPtr->blockAddr +
			sizeof(int) * (indexInfoPtr->indInfo[0].index - 1)));
		if (indexInfoPtr->indInfo[0].index == FSDM_INDICES_PER_BLOCK) {
		    /*
		     * We are at the end of the doubly indirect block and the
		     * caller wants us to go off of the end.  Free up the
		     * indirect block and return an error.
		     */
		    FreeIndirectBlock(0, handlePtr, indexInfoPtr,
			     &(descPtr->indirect[FSDM_DBL_INDIRECT]));
		    return(FS_INVALID_ARG);
		}
		if (indexInfoPtr->flags & OFS_DELETE_INDIRECT_BLOCKS) {
		    indexInfoPtr->indInfo[1].deleteBlock = FSCACHE_DELETE_BLOCK;
		}
	    } else {
		if (indexInfoPtr->indInfo[1].blockPtr != 
						(Fscache_Block *) NIL) {
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
 * OfsEndIndex --
 *
 *	Free up cache blocks locked for indexing.  This also frees
 *	a reference to the domain acquired with OfsGetFirstIndex.
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
OfsEndIndex(handlePtr, indexInfoPtr, dirty) 
    Fsio_FileIOHandle	      *handlePtr;    /* Handle for file that is being
						indexed. */
    register OfsBlockIndexInfo *indexInfoPtr; /* Index structure to cleanup. */
    Boolean		      dirty;	     /* True if allocated a new block
						so dirtied block pointer. */
{
    register Fsdm_FileDescriptor	*descPtr = handlePtr->descPtr;

    if (dirty) {
	if (indexInfoPtr->indexType == FSDM_INDIRECT) {
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	} else if (indexInfoPtr->indexType == FSDM_DBL_INDIRECT) {
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	}
    }
	
    /*
     * Free up the second level indirect block and then the first level
     * indirect block.
     */

    if (indexInfoPtr->indInfo[1].blockPtr != (Fscache_Block *) NIL) {
	FreeIndirectBlock(1, handlePtr, indexInfoPtr,
			(int *) (indexInfoPtr->indInfo[0].blockPtr->blockAddr +
			     sizeof(int) * indexInfoPtr->indInfo[0].index));
    }
    FreeIndirectBlock(0, handlePtr, indexInfoPtr,
		     &(descPtr->indirect[indexInfoPtr->indexType]));
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
    register	Fsio_FileIOHandle	 *handlePtr;
    register	OfsBlockIndexInfo 	*indexInfoPtr;
{
    register	Fsdm_FileDescriptor 	*descPtr;
    register	int 			*blockAddrPtr;
    ReturnStatus			status;
    int					cacheBlockNum;

    descPtr = handlePtr->descPtr;

    if (indexInfoPtr->indexType == FSDM_INDIRECT) {
	blockAddrPtr = &(descPtr->indirect[0]);
	if (*blockAddrPtr == FSDM_NIL_INDEX && 
	    !(indexInfoPtr->flags & OFS_ALLOC_INDIRECT_BLOCKS)) {
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

    if (indexInfoPtr->indexType == FSDM_INDIRECT) {
	indexInfoPtr->blockAddrPtr = blockAddrPtr;
	return(SUCCESS);
    }

    /* 
     * Lock the second level block into the cache.
     */
    if (*blockAddrPtr == FSDM_NIL_INDEX &&
	!(indexInfoPtr->flags & OFS_ALLOC_INDIRECT_BLOCKS)) {
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
    } else {
	FreeIndirectBlock(0, handlePtr, indexInfoPtr, &(descPtr->indirect[1]));
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
    Fsio_FileIOHandle		*handlePtr;	/* File to fetch indirect
					  	 * block for. */
    register OfsBlockIndexInfo 	*indexInfoPtr;  /* Indexing information for
						 * this file. */
    register int		*blockAddrPtr;	/* Disk block number. */
    int				cacheBlockNum;	/* The block number by which
						 * the cache knows this block.*/
{
    unsigned	char		*bitmapPtr;
    int				blockNum;
    Boolean			found;
    ReturnStatus		status = SUCCESS;
    register OfsIndirectInfo	*indInfoPtr;
    register int		*intPtr;
    Boolean			dontBlock;

    dontBlock = indexInfoPtr->flags & FSCACHE_DONT_BLOCK;
    indInfoPtr = &(indexInfoPtr->indInfo[indBlockNum]);
    if (indInfoPtr->blockPtr == (Fscache_Block *) NIL) {
	if (*blockAddrPtr == FSDM_NIL_INDEX) {
	    OfsBlockFind(handlePtr->hdr.fileID.minor, indexInfoPtr->ofsPtr, 
			-1, TRUE, &blockNum, &bitmapPtr);
	    if (blockNum == -1) {
		return(FS_NO_DISK_SPACE);
	    }
	    blockNum += indexInfoPtr->ofsPtr->headerPtr->dataOffset;
	    *blockAddrPtr = blockNum * FS_FRAGMENTS_PER_BLOCK;
	    handlePtr->descPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
	    Fscache_FetchBlock(&handlePtr->cacheInfo, cacheBlockNum,
		FSCACHE_IND_BLOCK|dontBlock, &(indInfoPtr->blockPtr), &found);
	    if (indInfoPtr->blockPtr == (Fscache_Block *)NIL) {
		return(FS_WOULD_BLOCK);
	    }
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
		*intPtr = FSDM_NIL_INDEX;
	    }
	    indexInfoPtr->indInfo[0].blockDirty = TRUE;
	    indexInfoPtr->indInfo[1].blockDirty = TRUE;
	    Fscache_IODone(indInfoPtr->blockPtr);
	} else {
	    fs_Stats.blockCache.indBlockAccesses++;
	    Fscache_FetchBlock(&handlePtr->cacheInfo, cacheBlockNum,
		FSCACHE_IND_BLOCK|dontBlock, &(indInfoPtr->blockPtr), &found);
	    if (indInfoPtr->blockPtr == (Fscache_Block *)NIL) {
		return(FS_WOULD_BLOCK);
	    }
	    if (!found) {
		status = OfsDeviceBlockIO(indexInfoPtr->ofsPtr, FS_READ,
		       *blockAddrPtr, FS_FRAGMENTS_PER_BLOCK, 
		       indInfoPtr->blockPtr->blockAddr);
		if (status == SUCCESS) {
		    fs_Stats.gen.physBytesRead += FS_BLOCK_SIZE;
		}
		Fscache_IODone(indInfoPtr->blockPtr);
	    } else {
		fs_Stats.blockCache.indBlockHits++;
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
 *	This is called whenever OfsGetNextIndex or OfsEndIndex are 
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
    Fsio_FileIOHandle		*handlePtr;	/* File to delete indirect
						 * block from. */
    register OfsBlockIndexInfo 	*indexInfoPtr; 	/* Index structure to use. */
    int				*blockAddrPtr;	/* Pointer to block to free. */
{
    int				modTime;
    int				indBlock;
    register	OfsIndirectInfo	*indInfoPtr;

    indInfoPtr = &indexInfoPtr->indInfo[indBlockNum];

    if (indInfoPtr->blockPtr != (Fscache_Block *) NIL) {
	if (indInfoPtr->blockDirty) {
	    modTime = Fsutil_TimeInSeconds();
	    if (!indInfoPtr->deleteBlock) {
		fs_Stats.blockCache.indBlockWrites++;
	    }
	} else {
	    modTime = 0;
	}
	Fscache_UnlockBlock(indInfoPtr->blockPtr, 
	   (unsigned )modTime, -(*blockAddrPtr), FS_BLOCK_SIZE,
	   indInfoPtr->deleteBlock);
	if (indInfoPtr->deleteBlock) {
	    indBlock = *blockAddrPtr / FS_FRAGMENTS_PER_BLOCK;
	    OfsBlockFree(indexInfoPtr->ofsPtr,
		    indBlock - indexInfoPtr->ofsPtr->headerPtr->dataOffset);
	    *blockAddrPtr = FSDM_NIL_INDEX;
	    handlePtr->descPtr->numKbytes -= FS_FRAGMENTS_PER_BLOCK;
	}
	indInfoPtr->blockPtr = (Fscache_Block *) NIL;
    }
}
