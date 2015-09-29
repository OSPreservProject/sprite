/* 
 * fsIndex.c --
 *
 *	Routines to allow moving through a files block pointers.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/decprom/RCS/fsIndex.c,v 1.1 90/02/16 16:14:13 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fsBoot.h"
#include "kernel/byte.h"

char	firstBlockBuffer[FS_BLOCK_SIZE];
char	secondBlockBuffer[FS_BLOCK_SIZE];


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
MakePtrAccessible(handlePtr, indexInfoPtr, descPtr)
    register	Fsio_FileIOHandle	 *handlePtr;
    register	BlockIndexInfo 	 *indexInfoPtr;
    register	Fsdm_FileDescriptor *descPtr;
{
    register 	int 		 *blockAddrPtr;

    /* 
     * Read in the first block.
     */

    if (indexInfoPtr->firstBlockNil) {
	FsDeviceBlockIO(FS_READ, &fsDevice,
		      descPtr->indirect[indexInfoPtr->indexType],
		      FS_FRAGMENTS_PER_BLOCK, firstBlockBuffer);
    }

    blockAddrPtr = 
	(int *) (firstBlockBuffer + sizeof(int) * indexInfoPtr->firstIndex);

    if (indexInfoPtr->indexType == FS_INDIRECT) {
	indexInfoPtr->blockAddrPtr = blockAddrPtr;
	return(SUCCESS);
    }

    /* 
     * Get the second level block.
     */

    FsDeviceBlockIO(FS_READ, &fsDevice, *blockAddrPtr,
		       FS_FRAGMENTS_PER_BLOCK, secondBlockBuffer);
    indexInfoPtr->blockAddrPtr = 
	(int *) (secondBlockBuffer + sizeof(int) * indexInfoPtr->secondIndex);

    return(SUCCESS);
}


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
FsGetFirstIndex(handlePtr, blockNum, indexInfoPtr)
    register Fsio_FileIOHandle	    *handlePtr;    /* Handle for file that are 
					      indexing. */
    register int	    blockNum;      /* Where to start indexing. */
    register BlockIndexInfo *indexInfoPtr; /* Index structure to initialize.*/
{
    register Fsdm_FileDescriptor *descPtr;
    register int	      indirectBlock;

    descPtr = handlePtr->descPtr;
    indexInfoPtr->firstBlockNil = TRUE;
    indexInfoPtr->blockNum = blockNum;

    if (blockNum < FSDM_NUM_DIRECT_BLOCKS) {
	/*
	 * This is a direct block.
	 */
	indexInfoPtr->indexType = FS_DIRECT;
	indexInfoPtr->blockAddrPtr = &descPtr->direct[blockNum];
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
	indexInfoPtr->indexType = FS_INDIRECT;
	indexInfoPtr->firstIndex = blockNum;
    } else {
	/*
	 * This a doubly indirect block.
	 */
	indexInfoPtr->indexType = FS_DBL_INDIRECT;
	indexInfoPtr->firstIndex = indirectBlock - 1;
	indexInfoPtr->secondIndex = blockNum -
				    indirectBlock * FSDM_INDICES_PER_BLOCK;
    }

    /*
     * Finish off by making the block pointer accessible.  This may include
     * reading indirect blocks into the cache.
     */

    return(MakePtrAccessible(handlePtr, indexInfoPtr, descPtr));
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
 *	The allocation structure is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsGetNextIndex(handlePtr, indexInfoPtr)
    register Fsio_FileIOHandle       *handlePtr;    /* Handle for file that is being
					      indexed. */
    register BlockIndexInfo *indexInfoPtr; /* Index structure to set up. */
{
    register Boolean	      accessible = FALSE;
    register Fsdm_FileDescriptor *descPtr;

    descPtr = handlePtr->descPtr;
    indexInfoPtr->blockNum++;

    /*
     * Determine whether we are now in direct, indirect or doubly indirect
     * blocks.
     */

    switch (indexInfoPtr->indexType) {
	case FS_DIRECT:
	    if (indexInfoPtr->blockNum < FSDM_NUM_DIRECT_BLOCKS) {
		/*
		 * Still in the direct blocks.
		 */
		indexInfoPtr->blockAddrPtr++;
		accessible = TRUE;
	    } else {
		/*
		 * Moved into indirect blocks.
		 */
		indexInfoPtr->indexType = FS_INDIRECT;
		indexInfoPtr->firstIndex = 0;
	    }
	    break;
	case FS_INDIRECT:
	    if (indexInfoPtr->blockNum < 
			FSDM_NUM_DIRECT_BLOCKS + FSDM_INDICES_PER_BLOCK) {
		/*
		 * Still in singly indirect blocks.
		 */
		indexInfoPtr->blockAddrPtr++;
		accessible = TRUE;
		break;
	   } else {
		/*
		 * Moved into doubly indirect blocks.
		 */
		indexInfoPtr->firstIndex = 0;
		indexInfoPtr->secondIndex = 0;
		indexInfoPtr->indexType = FS_DBL_INDIRECT;
		indexInfoPtr->firstBlockNil = TRUE;
	    }
	    break;
	case FS_DBL_INDIRECT:
	    indexInfoPtr->secondIndex++;
	    if (indexInfoPtr->secondIndex == FSDM_INDICES_PER_BLOCK) {
		indexInfoPtr->firstIndex++;
		indexInfoPtr->secondIndex = 0;
	    } else {
		indexInfoPtr->blockAddrPtr++;
		accessible = TRUE;
	    }
	    break;
    }

    /*
     * Make the block pointers accessible if necessary.
     */

    if (!accessible) {
	return(MakePtrAccessible(handlePtr, indexInfoPtr, descPtr));
    } else {
	return(SUCCESS);
    }
}
