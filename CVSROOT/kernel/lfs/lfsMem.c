/* 
 * LfsMem.c --
 *
 *	Manage the memory used for segment layout and cleaning for the
 *	lfs file systems
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

#include <fs.h>
#include <fsconsist.h>
#include <lfsInt.h>
#include <sync.h>

static Sync_Lock lfsMemLock = Sync_LockInitStatic("LfsMemLock");
#define	LOCKPTR (&lfsMemLock)

static Boolean memInUse = FALSE;   /* TRUE if the resources are in uses.
				    * FALSE otherwise. */

Sync_Condition	memWait;	/* Condition to wait for  
				 * memory to become available. */

static int numFileSystems = 0;	     /* Number of file system sharing the
				      * memory. */
static int maxNumCacheBlocksRes = 0; /* Number of cache blocks reserved by this
				      * module for cleaning. */

/*
 * Cleaning memory variables.
 */

static char *cleaningMemPtr = (char *) NIL; /* Pointer to allocated memory
					     * for reading in segments to 
					     * clean.  */
static int  cleaningMemSize = 0;	/* Size of the current allocated 
					 * cleaning memory. */

/*
 *----------------------------------------------------------------------
 *
 * LfsMemInit --
 *
 *	Initialize the memory resources needed by an LFS file system.
 *	This routine should be called a file system attach time to 
 *	insure enough memory and cache blocks exist for the file system
 *	to perform segment writes and cleans.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File cache blocks may be reserved.
 *
 *----------------------------------------------------------------------
 */

void
LfsMemInit(lfsPtr)
    Lfs	*lfsPtr;	/* File system to initial memory for. */
{
    int	segSizeInBlocks, numReserved, segSize;

    LOCK_MONITOR;
    segSizeInBlocks = LfsSegSizeInBlocks(lfsPtr);
    segSize = LfsSegSize(lfsPtr);


    numReserved = 0;
    /*
     * Reserve the blocks we need for cleaning.   We need only add to the
     * number of reserved blocks.
     */
    if (maxNumCacheBlocksRes < lfsPtr->superBlock.hdr.maxNumCacheBlocks) {
	numReserved = lfsPtr->superBlock.hdr.maxNumCacheBlocks - 
					maxNumCacheBlocksRes;
    }
    numReserved = Fscache_ReserveBlocks(lfsPtr->domainPtr->backendPtr, 
				numReserved, 0);
    maxNumCacheBlocksRes += numReserved;
    /*
     * Reserve a segment worth of cache blocks for writing and insure
     * that two segments worth of cache blocks exist to act as a
     * write buffer for the file system.
     */
    lfsPtr->mem.cacheBlocksReserved = Fscache_ReserveBlocks(
				lfsPtr->domainPtr->backendPtr, 
				segSizeInBlocks, 2*segSizeInBlocks);

    /*
     * If the previously allocated cleaning memory is not large enough,
     * free it and malloc one of the correct size. Besure to wait until
     * the memory is not in use. 
     */
    if (segSize > cleaningMemSize) {
	while(memInUse) {
	    Sync_Wait(&memWait, FALSE);
	}
	if (cleaningMemSize > 0) {
	    free(cleaningMemPtr);
	}
	cleaningMemSize = segSize;
	cleaningMemPtr = malloc(cleaningMemSize);
    }
    numFileSystems++;
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsMemDetach --
 *
 *	Release the memory and cache resources reserved by a LFS file
 *	system. This routine should be called at file system detach 
 *	time.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reserved cache blocks and memory may be released.
 *
 *----------------------------------------------------------------------
 */

void
LfsMemDetach(lfsPtr)
    Lfs	*lfsPtr;	/* File system to release memory for. */
{
    LOCK_MONITOR;
    Fscache_ReleaseReserveBlocks(lfsPtr->domainPtr->backendPtr,
					lfsPtr->mem.cacheBlocksReserved);
    numFileSystems--;
    /*
     * If this is the last file system, release reserved blocks.
     */
    if  (numFileSystems == 0) {
	Fscache_ReleaseReserveBlocks(lfsPtr->domainPtr->backendPtr,
					maxNumCacheBlocksRes);
	maxNumCacheBlocksRes = 0;
	free(cleaningMemPtr);
	cleaningMemSize = 0;
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsMemReserve --
 *
 *	Reserved cache blocks and memory for a file system to perform
 *	cleaning.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and cache blocks reserved.  May wait for resources to
 *	become available.
 *
 *----------------------------------------------------------------------
 */

void
LfsMemReserve(lfsPtr, cacheBlocksPtr, memPtrPtr)
    Lfs	*lfsPtr;	/* File system to reserve memory. */
    int	 *cacheBlocksPtr; /* OUT: Number of cache blocks reserved. */
    char **memPtrPtr;   /* OUT:  Memory for segment data. */
{
    LOCK_MONITOR;
    /*
     * First wait for the segment memory to become available.
     */
    while(memInUse) {
	Sync_Wait(&memWait, FALSE);
    }
    memInUse = TRUE;
    *cacheBlocksPtr = maxNumCacheBlocksRes;
    *memPtrPtr = cleaningMemPtr;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsMemRelease --
 *
 *	Release the cache blocks and memory previous reserved by 
 *	LfsMemReserve
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and cache blocks released.  
 *
 *----------------------------------------------------------------------
 */

void
LfsMemRelease(lfsPtr, cacheBlocks, memPtr)
    Lfs	*lfsPtr; 	/* File system to release memory resources. */
    int	cacheBlocks;	/* Number of cache blocks to release. */
    char *memPtr;	/* Memory allocate for segment data. */
{
    LOCK_MONITOR;
    memInUse = FALSE;
    Sync_Broadcast(&memWait);
    UNLOCK_MONITOR;
}
