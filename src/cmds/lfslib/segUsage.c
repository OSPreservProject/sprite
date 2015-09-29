/* 
 * segUsage.c --
 *
 *	Routines for accessing LFS file systems seg usage table data
 *	structure from a user level program
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include "lfslibInt.h"
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#include <sys/types.h>
#endif /* _HAS_PROTOTYPES */
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#include <bstring.h>
#include <time.h>


/*
 *----------------------------------------------------------------------
 *
 * LfsLoadUsageArray --
 *
 *	Load the segment usage array into memory.
 *
 * Results:
 *	TRUE if array can not be loaded. FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsLoadUsageArray(lfsPtr, checkPointSize, checkPointPtr)
    Lfs	*lfsPtr;		/* File system. */
    int	checkPointSize; /* Size of the checkpoint region. */
    char *checkPointPtr; /* The checkpoint region. */
{
    LfsSegUsageCheckPoint	*cp;
    LfsSegUsageEntry		*entryPtr;
    LfsStableMemCheckPoint *cpPtr;
    LfsStableMemParams  *smemParamsPtr;
    Boolean ret = FALSE;

    if (checkPointSize < sizeof(LfsSegUsageCheckPoint)) {
	fprintf(stderr,"%s: Bad SegUsage checkpoint size %d\n", 
			lfsPtr->deviceName, checkPointSize);
    }
    smemParamsPtr = &(lfsPtr->superBlock.usageArray.stableMem);

    cp = (LfsSegUsageCheckPoint *) checkPointPtr;
    bcopy((char *) cp, (char *) &lfsPtr->usageArray.checkPoint, 
		sizeof(LfsSegUsageCheckPoint));
    cpPtr = (LfsStableMemCheckPoint *)
			(checkPointPtr + sizeof(LfsSegUsageCheckPoint));


    lfsPtr->usageArray.smemPtr =  LfsLoadStableMem(lfsPtr, smemParamsPtr, cpPtr);
    entryPtr = (LfsSegUsageEntry *) LfsGetUsageArrayEntry(lfsPtr,cp->currentSegment);
    entryPtr->activeBytes = cp->curSegActiveBytes;
    if (entryPtr->activeBytes <= cp->dirtyActiveBytes) {
	entryPtr->flags = LFS_SEG_USAGE_DIRTY;
    }
    /*
     * Compute the starting point of the recovery log.
     */
    if (cp->currentBlockOffset == -1) {
	lfsPtr->logEnd.segNo = cp->cleanSegList;
	lfsPtr->logEnd.blockOffset = 0;
    } else { 
	lfsPtr->logEnd.segNo = cp->currentSegment;
	lfsPtr->logEnd.blockOffset = cp->currentBlockOffset;
   }
   return ret;
}



/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageAdjustBytes --
 *
 *	Adjust the active bytes count of the specified segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
LfsSegUsageAdjustBytes(lfsPtr, diskAddr, changeInBytes)
    Lfs	*lfsPtr;	/* File system. */
    int	diskAddr;	/* Disk of address change. */
    int	changeInBytes;	/* Amount of change. */
{
    int segNumber;
    LfsSegUsageEntry *entryPtr;
    LfsSegUsageCheckPoint	*cp = &lfsPtr->usageArray.checkPoint;

    if (diskAddr == FSDM_NIL_INDEX) {
	return;
    }
    segNumber = LfsDiskAddrToSegmentNum(lfsPtr, diskAddr);

    if (cp->currentSegment == segNumber) {
	cp->curSegActiveBytes += changeInBytes;
	return;
    }

    entryPtr = LfsGetUsageArrayEntry(lfsPtr, segNumber);
    if (entryPtr->flags & LFS_SEG_USAGE_CLEAN) {
	panic("AdjustActiveBytes called on a clean segment.\n");
    }

    entryPtr->activeBytes += changeInBytes;

    if (entryPtr->activeBytes < 0) {
	entryPtr->activeBytes = 0;
    }
    if ((entryPtr->activeBytes < cp->dirtyActiveBytes) && 
	!(entryPtr->flags & LFS_SEG_USAGE_DIRTY)) {
	cp->numDirty++;
	entryPtr->flags |= LFS_SEG_USAGE_DIRTY;
    }
    if ((entryPtr->activeBytes >= cp->dirtyActiveBytes) && 
	(entryPtr->flags & LFS_SEG_USAGE_DIRTY)) {
	cp->numDirty--;
	entryPtr->flags ^= LFS_SEG_USAGE_DIRTY;
    }
    LfsUsageArrayEntryModified(lfsPtr, segNumber);
}


/*
 *----------------------------------------------------------------------
 *
 * LfsUsageCheckpoint --
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
LfsUsageCheckpoint(segPtr, checkPointPtr, checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    LfsSegUsageCheckPoint *cp = (LfsSegUsageCheckPoint *) checkPointPtr;
    int		size;
    Boolean	full;

    *cp = (segPtr->lfsPtr->usageArray.checkPoint);
    size = sizeof(LfsSegUsageCheckPoint);

    full = LfsStableMemCheckpoint(segPtr, checkPointPtr + size,
		checkPointSizePtr, segPtr->lfsPtr->usageArray.smemPtr);
    if (!full) { 
	*checkPointSizePtr = (*checkPointSizePtr) + size;
    }
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
LfsSegUsageWriteDone(segPtr, flags)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
{
    LfsSegUsage	      *usagePtr = &(segPtr->lfsPtr->usageArray);

    LfsStableMemWriteDone(segPtr, flags, usagePtr->smemPtr);

    return;

}




/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageCheckpointUpdate --
 *
 *	This routine is used to update fields of the seg usage 
 *	checkpoint that change when the checkpoint itself is 
 *	written to the log.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
LfsSegUsageCheckpointUpdate(lfsPtr, checkPointPtr, size)
    Lfs	 *lfsPtr;	 /* File system being checkpointed. */
    char *checkPointPtr; /* Checkpoint region for SegUsage. */
    int	 size;		 /* Size of checkpoint region. */
{
    LfsSegUsage	      *usagePtr = &(lfsPtr->usageArray);

    if (size < sizeof(LfsSegUsageCheckPoint)) {
	panic("LfsSegUsageCheckpointUpdate bad checkpoint size.\n");
    }
    (*(LfsSegUsageCheckPoint *) checkPointPtr) = usagePtr->checkPoint;
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * LfsGetLogTail --
 *
 *	Get the next available clean blocks to write the log to.
 *
 * Results:
 *	SUCCESS if log space was retrieved. 
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsGetLogTail(lfsPtr, cantWait, logRangePtr, startBlockPtr)
    Lfs	*lfsPtr;	/* File system of interest. */
    Boolean	cantWait;	  /* TRUE if we can't wait for a clean seg. */
    LfsSegLogRange *logRangePtr;  /* Segments numbers returned. */
    int		   *startBlockPtr; /* OUT: Starting offset into segment. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);
    LfsSegUsageEntry *s;
    int		segNumber;


    if (!cantWait && (cp->numClean <=
		lfsPtr->superBlock.usageArray.minNumClean)) {
	return FS_WOULD_BLOCK;
    }
    if (cp->currentBlockOffset != -1) {
	/*
	 * There is still room in the existing segment. Use it.
	 */
	logRangePtr->prevSeg = cp->previousSegment;
	logRangePtr->current = cp->currentSegment;
	logRangePtr->nextSeg =  cp->cleanSegList;
	(*startBlockPtr) = cp->currentBlockOffset;
	return SUCCESS;
    }
    /*
     * Need to location a new segment.
     */
    if (cp->numClean == 0) {
	return FS_NO_DISK_SPACE;
    }
    /*
     * Update the active bytes of the current segment the usage array.
     */
    s = LfsGetUsageArrayEntry(lfsPtr, cp->currentSegment);
	s->activeBytes = cp->curSegActiveBytes;
	if (s->activeBytes <= cp->dirtyActiveBytes) {
	    s->flags |= LFS_SEG_USAGE_DIRTY;
	    cp->numDirty++;
	}
	s->timeOfLastWrite = time(0);
    LfsUsageArrayEntryModified(lfsPtr, cp->currentSegment);

    segNumber = cp->cleanSegList;
    s = LfsGetUsageArrayEntry(lfsPtr, cp->cleanSegList);
    cp->cleanSegList = s->activeBytes;

    logRangePtr->prevSeg = cp->previousSegment = cp->currentSegment;
    logRangePtr->current = cp->currentSegment = segNumber;
    logRangePtr->nextSeg =  cp->cleanSegList;

    cp->numClean--;
    s->activeBytes = cp->curSegActiveBytes = 0;
    s->flags  &= ~LFS_SEG_USAGE_CLEAN;
    LfsUsageArrayEntryModified(lfsPtr, segNumber);
    (*startBlockPtr) = 0;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSetLogTail --
 *
 *	Set the next available clean blocks to write the log to.
 *
 * Results:
 *	None
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

void
LfsSetLogTail(lfsPtr, logRangePtr, startBlock, activeBytes)
    Lfs	*lfsPtr;	/* File system of interest. */
    LfsSegLogRange *logRangePtr;  /* Segments numbers returned. */
    int	startBlock; /* Starting offset into segment. */
    int	activeBytes;	/* Number of bytes written. */
{
    LfsSegUsage *usagePtr = &(lfsPtr->usageArray);
    LfsSegUsageCheckPoint *cp = &(usagePtr->checkPoint);

    cp->currentBlockOffset = startBlock;
    if (activeBytes > 0) {
	LfsSegUsageAdjustBytes(lfsPtr, 
		LfsSegNumToDiskAddress(lfsPtr,logRangePtr->current)+1,
		activeBytes);
   }
}

