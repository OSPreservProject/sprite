/* 
 * lfsSegUsage.c --
 *
 *	Routines and data structures providing knowledge more segments 
 *	block usage.  This module managers the segment usage array and
 *	implements the selection of the next segment to write and to
 *	clean.  This module should be notified of all block deallocation
 *	inorder to make intelligent choices.  The module also detects 
 *	"logwrap" and starts with block cleaner and generates "df"
 *	numbers.
 *
 * 	For each LFS, the module classifies segment into one of three classes:
 *	clean, dirty, or full.  The segment usage entries of the clean and 
 *	dirty segments are linked into list.  A segment is clean if it
 *	contains no live data.  A segment is dirty if it is not clean
 *	and has fewer activeBytes than permitable. A segment is full if
 *	it is neither clean nor dirty.
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

#include "lfsInt.h"
#include "lfsSeg.h"
#include "lfsSegUsageInt.h"

#include "fsdm.h"

static void RemoveFromList();
static void AddToList();


/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageFreeBlocks --
 *
 *	Inform the segment usage manager that blocks are no long needed.
 *
 * Results:
 *	SUCCESS if the blocks are valid.
 *
 * Side effects:
 *	Seg usage map.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsSegUsageFreeBlocks(lfsPtr, blockSize, blockArrayLen, blockArrayPtr)
    Lfs		*lfsPtr;	/* File system of interest. */
    int		blockSize;	/* Size in bytes of blocks to free. */
    int	         blockArrayLen; /* Number of elements in blockArrayPtr. */
    int *blockArrayPtr;	        /* Array of disk addresses. */
{
    int		i;

    for (i = 0; i < blockArrayLen; i++) {
	if (*blockArrayPtr != FSDM_NIL_INDEX) {
	    LFS_STATS_INC(lfsPtr->stats.segusage.blocksFreed);
	    LFS_STATS_ADD(lfsPtr->stats.segusage.bytesFreed,  blockSize);
	    LfsSetSegUsage(lfsPtr,
		(int)LfsBlockToSegmentNum(lfsPtr, *blockArrayPtr), -blockSize);
	}
	*blockArrayPtr = FSDM_NIL_INDEX;
	blockArrayPtr++;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * SegUsageAllocateBytes --
 *
 *	Inform the file system for the need to allocate some bytes.
 *
 * Results:
 *	SUCCESS if the allocation works, failure otherwise.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
LfsSegUsageAllocateBytes(lfsPtr, numBytes)
       Lfs	*lfsPtr;	/* File system of interest. */
       int	numBytes;       /* Number of file system bytes needed. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    int blocks;

    blocks = LfsBytesToBlocks(lfsPtr, numBytes);
    if (cp->freeBlocks - blocks > usagePtr->params.minFreeBlocks) { 
	return SUCCESS;
    }
    return FS_NO_DISK_SPACE;
}

/*
 *----------------------------------------------------------------------
 *
 * SegUsageFreeBytes --
 *
 *	Inform the file system that the previously allocated bytes are 
 *	nolonger needed or have already been allocated on disk.
 *
 * Results:
 *	SUCCESS if the allocation works, failure otherwise.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
LfsSegUsageFreeBytes(lfsPtr,numBytes)
       Lfs	*lfsPtr;	/* File system of interest. */
       int	numBytes;       /* Number of file system bytes to free. */
{
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsCheckRead --
 *
 *	Check to see if it is ok to read the specified byte range. 
 *
 * Results:
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */
LfsCheckRead(lfsPtr,diskAddress, numBytes)
       Lfs	*lfsPtr;	/* File system of interest. */
       int    diskAddress;
       int	numBytes;       
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    register LfsSegUsageEntry *array, *s;
	int segNo, segNo2;

    segNo = LfsBlockToSegmentNum(lfsPtr, diskAddress);
    if ((segNo < 0) || (segNo >=  usagePtr->params.numberSegments)) {
	panic("LfsOkToRead bad segment number %d\n", segNo);
	return;
    }
    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);
    s = array + segNo;
    if (s->flags & LFS_SEG_USAGE_CLEAN) {
	panic("LfsOkToRead read from clean segment\n");
    }
    segNo2 = LfsBlockToSegmentNum(lfsPtr,( diskAddress +
				LfsBytesToBlocks(lfsPtr, numBytes)-1));
    if (segNo2 != segNo) {
	panic("LfsOkToRead read over segment boundary.\n");
    }

}

/*
 *----------------------------------------------------------------------
 *
 * LfsSetSegUsage --
 *
 *	Set the usage level of the specified segment.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Seg usage array may be modified.
 *
 *----------------------------------------------------------------------
 */

void
LfsSetSegUsage(lfsPtr, segNumber, activeBytes)
    Lfs		*lfsPtr;	/* File system of interest. */
    int		segNumber; 	/* Segment number in file system. */
    int		activeBytes;	/* Usage level Change. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    register LfsSegUsageEntry *array, *s;

    if ((segNumber < 0) || (segNumber >= usagePtr->params.numberSegments)) {
	panic("LfsSetSegUsage bad segment number %d\n", segNumber);
	return;
    }
    if (activeBytes == 0) {
	printf("LfsSetSegUsage: SegNo %d activeBytes %d\n", segNumber, 
			activeBytes);
    }
    LFS_STATS_INC(lfsPtr->stats.segusage.usageSet);
    lfsPtr->dirty = TRUE;
    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);
    s = array + segNumber;

    activeBytes = s->activeBytes + activeBytes;
    if (activeBytes > LfsSegSize(lfsPtr)) {
	printf("LfsSetSegUsage: Warning activeBytes for segment %d is %d\n",
		segNumber, activeBytes);
	activeBytes = LfsSegSize(lfsPtr);
    }
    if (activeBytes <= 0) {
#ifdef notdef
	/*
	 * This currently occurs because we don't know the size of a block
	 * when we delete the file.
	 */
	if (activeBytes < 0) {
	    printf("LfsSetSegUsage: Warning activeBytes for segment %d is %d\n",
		segNumber, activeBytes);
	}
#endif
	activeBytes = 1;
    }
    cp->freeBlocks += (LfsBytesToBlocks(lfsPtr, s->activeBytes) - 
				LfsBytesToBlocks(lfsPtr, activeBytes));
    if (s->flags & LFS_SEG_USAGE_CLEAN) {
	RemoveFromList(array, cp->cleanLinks, segNumber);
	s->flags &= ~LFS_SEG_USAGE_CLEAN;
        cp->numClean--;
	if (cp->numClean <= lfsPtr->usageArray.params.minNumClean) {
	    LfsSegCleanStart(lfsPtr);
	}
    }
    /*
     * Is it moving onto dirty list?
     */
    if (activeBytes <= cp->dirtyActiveBytes) {
	if (s->flags & LFS_SEG_USAGE_DIRTY) { 
	    /*
	     * All ready on dirty list then do nothing.
	     */
	    s->activeBytes = activeBytes;
	    return;
	}
	AddToList(array, cp->dirtyLinks, segNumber);
        s->activeBytes = activeBytes;
	s->flags |= LFS_SEG_USAGE_DIRTY;
	cp->numDirty++;
	return;
    }
    /*
     * Segment is not clean or dirty just full.
     */
    if (s->flags & LFS_SEG_USAGE_DIRTY) { 
	RemoveFromList(array, cp->dirtyLinks, segNumber);
	s->flags &= ~LFS_SEG_USAGE_DIRTY;
	cp->numDirty--;
    }
    s->activeBytes = activeBytes;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsMarkSegClean --
 *
 *	Mark the specified segment as clean.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Seg usage array may be modified.
 *
 *----------------------------------------------------------------------
 */

void
LfsMarkSegClean(lfsPtr, segNumber)
    Lfs		*lfsPtr;	/* File system of interest. */
    int		segNumber; 	/* Segment number to mark clean. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    register LfsSegUsageEntry *array, *s;

    if ((segNumber < 0) || (segNumber >= usagePtr->params.numberSegments)) {
	panic("LfsMarkSegClean bad segment number %d\n", segNumber);
	return;
    }
    lfsPtr->dirty = TRUE;
    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);
    s = array + segNumber;

    cp->freeBlocks += LfsBytesToBlocks(lfsPtr, s->activeBytes);
    /*
     * Segment is being marked clean. Remove from dirty list if necessary.
     */
    if (s->flags & LFS_SEG_USAGE_DIRTY) { 
	RemoveFromList(array, cp->dirtyLinks, segNumber);
	s->flags &= ~LFS_SEG_USAGE_DIRTY;
	cp->numDirty--;
    }
    if (!(s->flags & LFS_SEG_USAGE_CLEAN)) { 
	AddToList(array, cp->cleanLinks, segNumber);
	cp->numClean++;
    }
    s->flags = LFS_SEG_USAGE_CLEAN;
    s->activeBytes = 0;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSetDirtyLevel --
 *
 *	Set the usage level below which a segment is considered dirty.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Seg usage array may be modified.
 *
 *----------------------------------------------------------------------
 */

void
LfsSetDirtyLevel(lfsPtr, dirtyActiveBytes)
    Lfs	*lfsPtr;
    int	 dirtyActiveBytes; /* New level. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    register LfsSegUsageEntry  *array, *s;
    register int	segNum;

    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);

    cp->dirtyActiveBytes = dirtyActiveBytes;
    for (segNum = 0; segNum < usagePtr->params.numberSegments; segNum++) {
	s = array + segNum;
	if (s->activeBytes <= dirtyActiveBytes) {
	    if (s->flags & (LFS_SEG_USAGE_DIRTY|LFS_SEG_USAGE_CLEAN)) { 
		/*
		 * All ready on dirty or clean list then do nothing.
		 */
		continue;
	    }
	    LfsSetSegUsage(lfsPtr, segNum, 0);
	} 
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_DomainInfo --
 *
 *	Return info about the given domain lfsDomain.
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
Lfs_DomainInfo(domainPtr, domainInfoPtr)
    Fsdm_Domain	*domainPtr;
    Fs_DomainInfo	*domainInfoPtr;
{
    Lfs		*lfsPtr;

    lfsPtr = LfsFromDomainPtr(domainPtr);

    domainInfoPtr->maxKbytes = (LfsSegSize(lfsPtr)/1024) *
				lfsPtr->usageArray.params.numberSegments;

    domainInfoPtr->freeKbytes = 
	LfsBlocksToBytes(lfsPtr, lfsPtr->usageArray.checkPoint.freeBlocks)/1024;
    domainInfoPtr->maxFileDesc = lfsPtr->descMap.params.maxDesc;
    domainInfoPtr->freeFileDesc = lfsPtr->descMap.params.maxDesc -
				  lfsPtr->descMap.checkPoint.numAllocDesc;
    domainInfoPtr->blockSize = FS_BLOCK_SIZE;
    domainInfoPtr->optSize = FS_BLOCK_SIZE;


    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * RemoveFromList --
 *
 *	Remove segment from a list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveFromList(array, linksPtr, segNumber)
    LfsSegUsageEntry *array; /* Usage array. */
    int *linksPtr; /* List start and end. */
    int  segNumber; /* Segment number to remove from list. */
{
    register LfsSegUsageEntry  *s;

    s = array + segNumber;

    /*
     * See if segNumber is at the start of the list.
     */
    if (segNumber == linksPtr[LFS_SEG_USAGE_NEXT]) {
	linksPtr[LFS_SEG_USAGE_NEXT] = s->links[LFS_SEG_USAGE_NEXT];
	if (linksPtr[LFS_SEG_USAGE_NEXT] != NIL) { 
	    array[linksPtr[LFS_SEG_USAGE_NEXT]].links[LFS_SEG_USAGE_PREV] = NIL;
	} else {
	    linksPtr[LFS_SEG_USAGE_PREV] = NIL;
	}
    } else {
	int	prev, next;
	prev = s->links[LFS_SEG_USAGE_PREV];
	next = s->links[LFS_SEG_USAGE_NEXT];
	if (prev == NIL) {
	    panic("RemoveFromList is malformed.\n");
	}
	array[prev].links[LFS_SEG_USAGE_NEXT] = next;
	if (segNumber != linksPtr[LFS_SEG_USAGE_PREV]) { 
	    if (next == NIL) {
		panic("RemoveFromList is malformed.\n");
	    }
	    array[next].links[LFS_SEG_USAGE_PREV] = prev;
	} else {
	    linksPtr[LFS_SEG_USAGE_PREV] = prev;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AddToList --
 *
 *	Add segment to the end of the specified list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AddToList(array, linksPtr, segNumber)
    LfsSegUsageEntry *array; /* Usage array. */
    int *linksPtr; /* List start and end. */
    int  segNumber; /* Segment number to remove from list. */
{
    register LfsSegUsageEntry  *s;

    s = array + segNumber;

    /*
     * Add it to the end of the list.
     */
    s->links[LFS_SEG_USAGE_NEXT] = NIL;
    s->links[LFS_SEG_USAGE_PREV] = linksPtr[LFS_SEG_USAGE_PREV];
    if (linksPtr[LFS_SEG_USAGE_PREV] == NIL) {
	/*
	 * List was empty.
	 */
	linksPtr[LFS_SEG_USAGE_NEXT] = segNumber;
    } else { 
	array[linksPtr[LFS_SEG_USAGE_PREV]].links[LFS_SEG_USAGE_NEXT] = 
					segNumber;
    }
    linksPtr[LFS_SEG_USAGE_PREV] = segNumber;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsGetCleanSeg --
 *
 *	Get the next available clean segment to write.
 *
 * Results:
 *	A clean segment number.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsGetCleanSeg(lfsPtr, logRangePtr, cantWait)
    Lfs	*lfsPtr;	/* File system of interest. */
    LfsSegLogRange *logRangePtr;  /* Segments numbers returned. */
    Boolean	cantWait;	  /* TRUE if we can't wait for a clean segment.
				   */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    LfsSegUsageEntry *s, *array;

    if (!cantWait && (cp->numClean <= lfsPtr->usageArray.params.minNumClean)) {
	LfsSegCleanStart(lfsPtr);
	return FS_WOULD_BLOCK;
    }
    if (cp->numClean == 0) {
	return FS_NO_DISK_SPACE;
    }
    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);
    logRangePtr->prevSeg = cp->currentSegment;
    logRangePtr->current = cp->currentSegment = 
				cp->cleanLinks[LFS_SEG_USAGE_NEXT];
    RemoveFromList(array, cp->cleanLinks, logRangePtr->current);
    logRangePtr->nextSeg =  cp->cleanLinks[LFS_SEG_USAGE_NEXT];
    cp->numClean--;
    array[logRangePtr->current].flags  &= ~LFS_SEG_USAGE_CLEAN;
    return SUCCESS;
}
int	lfsCleanRangeLow = 0;

/*
 *----------------------------------------------------------------------
 *
 * LfsGetSegsToClean --
 *
 *	Return a set of segments to clean.
 *
 * Results:
 *	Number of segments returned.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

int
LfsGetSegsToClean(lfsPtr, maxBlocks, maxSegArrayLen, segArrayPtr)
    Lfs	  *lfsPtr;	/* File system of interest. */
    int	  maxBlocks;	/* Maximum number of file system blocks to clean. */
    int	  maxSegArrayLen; 	/* The maximum number of segment to clean to 
				 * return. */
    int	 *segArrayPtr;		/* Array of length maxSegArrayLen to return
				 * segments to clean. */
{
    int	numberSegs, segNum, numBlocks, blockSize;
    int blocks, i;
    LfsSegUsageEntry *s, *array;
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);

    array = (LfsSegUsageEntry *)(usagePtr->stableMem.dataPtr);
    blockSize = LfsBlockSize(lfsPtr);
    /*
     * For each dirty segment.
     */
    for (segNum = usagePtr->checkPoint.dirtyLinks[LFS_SEG_USAGE_NEXT];
	 segNum != NIL;
	 segNum = s->links[LFS_SEG_USAGE_NEXT]) {
	int j;
	s = array + segNum;
	/*
	 * Execpt the one currently being written.
	 */
	if (usagePtr->checkPoint.currentSegment == segNum) { 
	    continue;
	}
	/*
	 * Find the proper position in the list for this segment.
	 */

        for (i = numberSegs-1; i >= 0; i--) {
	    if (array[segArrayPtr[i]].activeBytes < s->activeBytes) {
		break;
	    } 
	}
	/*
	 * Insert it at that position by moving all others down. Extend the
	 * array if it is not full already.
	 */
	if (numberSegs < maxSegArrayLen) {
	    numberSegs++;
	}
	for (j = numberSegs-2; j > i; j--) {
	    segArrayPtr[j+1] = segArrayPtr[j];
	}
	segArrayPtr[i+1] = segNum;
    }
    numBlocks = 0;
    for (i = 0; i < numberSegs; i++) {
	s = array + segArrayPtr[i];
	numBlocks +=  LfsBytesToBlocks(lfsPtr, s->activeBytes);
	if (numBlocks > maxBlocks) {
	    return i;
	}
   }
   return numberSegs;
}


static LfsSegIoInterface segUsageIoInterface = 
	{ LfsSegUsageAttach, LfsSegUsageLayout, LfsSegUsageClean,
	  LfsSegUsageCheckpoint, LfsSegUsageWriteDone,  0};


/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageInit --
 *
 *	Initialize the segment usage array  data structures.  
 *
 * Results:
 *	None
 *	
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
LfsSegUsageInit()
{
    LfsSegIoRegister(LFS_SEG_USAGE_MOD,&segUsageIoInterface);
}


/*
 *----------------------------------------------------------------------
 *
 * SegUsageAttach --
 *
 *	Attach routine for the descriptor map. Creates and initializes the
 *	map for this file system.
 *
 * Results:
 *	SUCCESS if attaching is going ok.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsSegUsageAttach(lfsPtr, checkPointSize, checkPointPtr)
    Lfs   *lfsPtr;	     /* File system for attach. */
    int   checkPointSize;    /* Size of checkpoint data. */
    char  *checkPointPtr;     /* Data from last checkpoint before shutdown. */
{
    LfsSegUsage	      *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = (LfsSegUsageCheckPoint *) checkPointPtr;
    ReturnStatus status;

    /*
     * Allocate and fill in memory data structure for descriptor map.
     */
    usagePtr->params = lfsPtr->superBlock.usageArray;
    usagePtr->checkPoint = *cp;
    /*
     * Load the stableMem and buffer using the LfsStableMem routines.
     */
    status = LfsStableMemLoad(lfsPtr, &(usagePtr->params.stableMem), 
			checkPointSize - sizeof(LfsSegUsageCheckPoint), 
			checkPointPtr + sizeof(LfsSegUsageCheckPoint), 
			&(usagePtr->stableMem));
    if (status != SUCCESS) {
	LfsError(lfsPtr, status,"Can't loading descriptor map stableMem\n");
	return status;
    }
    printf("LfsSegUsageAttach - logEnd %d numClean %d numDirty %d numFull %d\n",
		cp->currentSegment, cp->numClean, cp->numDirty,
		usagePtr->params.numberSegments - cp->numClean - cp->numDirty);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * SegUsageCheckpoint --
 *
 *	Routine to handle checkpointing of the descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	checkpointed.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsSegUsageCheckpoint(segPtr, flags,checkPointPtr, checkPointSizePtr, 
			clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   flags;		/* Flags. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    ClientData *clientDataPtr;
{
    LfsSegUsage	      *usagePtr = &(segPtr->lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = (LfsSegUsageCheckPoint *) checkPointPtr;
    int		size;
    Boolean	full;

    /*
     * Mark the entire array as dirty.
     */
    LfsStableMemMarkDirty(&(usagePtr->stableMem), 
			usagePtr->stableMem.dataPtr, 
			(int ) (usagePtr->params.numberSegments *
			sizeof(LfsSegUsageEntry)));
    *cp = usagePtr->checkPoint;
    size = sizeof(LfsSegUsageCheckPoint);

    full = LfsStableMemCheckpoint(segPtr, checkPointPtr + size, flags,
			checkPointSizePtr, clientDataPtr,&(usagePtr->stableMem));
    *checkPointSizePtr = (*checkPointSizePtr) + size;
    return full;

}

/*
 *----------------------------------------------------------------------
 *
 * SegUsageWriteDone --
 *
 *	Routine to handle finishing of a checkpoint.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

void
LfsSegUsageWriteDone(segPtr, flags, clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
    ClientData *clientDataPtr;
{
    LfsSegUsage	      *usagePtr = &(segPtr->lfsPtr->usageArray);

    LFS_STATS_ADD(segPtr->lfsPtr->stats.segusage.blocksWritten, 
		(LfsSegSummaryBytesLeft(segPtr) / sizeof(int)));
    LfsStableMemWriteDone(segPtr, flags, clientDataPtr, &(usagePtr->stableMem));
    return;

}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageClean --
 *
 *	Routine to handle cleaning of descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsSegUsageClean(segPtr, sizePtr, numCacheBlocksPtr, clientDataPtr)
    LfsSeg *segPtr;	/* Segment containing data to clean. */
    int *sizePtr;		/* Size of cleaning. */
    int *numCacheBlocksPtr;
    ClientData *clientDataPtr;
{
    LfsSegUsage	      *usagePtr = &(segPtr->lfsPtr->usageArray);
    Boolean	full;

    full =  LfsStableMemClean(segPtr, sizePtr, numCacheBlocksPtr, clientDataPtr,
			&(usagePtr->stableMem));
    LFS_STATS_ADD(segPtr->lfsPtr->stats.segusage.blocksCleaned, 
		*sizePtr/usagePtr->stableMem.params.blockSize);
    return full;

}



/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageLayout --
 *
*	Routine to handle layingout of segUsage data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsSegUsageLayout(segPtr, cleaning, clientDataPtr)
    LfsSeg *segPtr;	/* Segment containing data to clean. */
    Boolean cleaning; /* TRUE if layout for cleaning. */
    ClientData *clientDataPtr;
{
    return FALSE;
}

