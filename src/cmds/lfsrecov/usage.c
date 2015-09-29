/* 
 * usage.c --
 *
 *	Segment usage manipulation routines for the lfsrecov program.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <hash.h>
#include <libc.h>

#include "lfsrecov.h"
#include "usage.h"


typedef struct SegInfo { 
    int	logPosition;	/* Current position to the list of segment written since
			 * the last checkpoint. */
    int	startLogBlockOffset; /* Starting block offset into segment of log. Only
			      * valid if logPosition is nonzero. */

} SegInfo;
static SegInfo	*segInfoArray = (SegInfo *) NULL;
static int logPosition = 0;


/*
 *----------------------------------------------------------------------
 *
 * RecordSegInLog --
 *
 *	Record the fact that the specified segment is part of the 
 *	recovery log.
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
RecordSegInLog(segNo, blockOffset)
    int segNo;		/* Segment number that is part of recovery log. */
    int	blockOffset;	/* Starting block offset into that segment. */
{
    if (segInfoArray == (SegInfo *) NULL) {
	segInfoArray = (SegInfo *) 
		    calloc(lfsPtr->superBlock.usageArray.numberSegments, 
			   sizeof(segInfoArray[0]));
	if (segInfoArray == (SegInfo *) NULL) {
	    fprintf(stderr,"Can't allocate memory for segInfoArray\n");
	    exit(1);
	}
	logPosition = 0;
    }

    if (segInfoArray[segNo].logPosition == 0) {
	segInfoArray[segNo].logPosition = ++logPosition;
	segInfoArray[segNo].startLogBlockOffset = blockOffset;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * SegIsPartOfLog --
 *
 *	Check to see if segment is part of the recovery log.
 *
 * Results:
 *	TRUE if part of recovery log, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean 
SegIsPartOfLog(segNo)
    int	segNo;	/* Segment number of check. */
{
    return (segInfoArray[segNo].logPosition != 0);
}

/*
 *----------------------------------------------------------------------
 *
 * AddrOlderThan --
 *
 *	Return TRUE if address addr1 was written later in the log than
 *	addr2.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
AddrOlderThan(addr1, addr2)
    int	   addr1;
    int	   addr2;
{
    int segNo1, segNo2;

    segNo1 = LfsDiskAddrToSegmentNum(lfsPtr, addr1);
    segNo2 = LfsDiskAddrToSegmentNum(lfsPtr, addr2);
    if (segNo1 == segNo2) {
	/*
	 * Both blocks are in the same segment. Since we grow the
	 * segments backward a smaller address means an older block.
	 */
	 return (addr1 < addr2);
    } else {
	int age1, age2;
	LfsSegUsageEntry *entryPtr;
	/*
	 * The blocks are in different segments so we are use the
	 * segment log position to determine the relative age.
	 * A higher log position number means a newer block.  
	 * If neither address is part of the recover segment
	 * chain we use the timestamps the the seg usage 
	 * block to determine the age.
	 */
	if ( segInfoArray[segNo1].logPosition + 
	     segInfoArray[segNo2].logPosition > 0) {
	     return (segInfoArray[segNo1].logPosition > 
				segInfoArray[segNo2].logPosition);
	 } 
	 entryPtr = LfsGetUsageArrayEntry(lfsPtr, segNo1);
	 age1 = entryPtr->timeOfLastWrite;
	 entryPtr = LfsGetUsageArrayEntry(lfsPtr, segNo2);
	 age2 = entryPtr->timeOfLastWrite;
	 return (age1 < age2);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * MarkSegDirty --
 *
 *	Mark the specified segment as nolonger being clean.
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
MarkSegDirty(segNumber)
    int	segNumber;	/* The segment which should be marked not clean. */
{
    LfsSegUsageEntry *entryPtr, *previousEntryPtr;
    int	segNo, previousSegNo;
    LfsSegUsageCheckPoint	*cp = &lfsPtr->usageArray.checkPoint;

    assert((segNumber >= 0) && 
	   (segNumber < lfsPtr->superBlock.usageArray.numberSegments));

    if (verboseFlag) {
	printf("Marking segment %d as dirty\n", segNumber);
    }
    entryPtr = LfsGetUsageArrayEntry(lfsPtr, segNumber);
    if (!(entryPtr->flags & LFS_SEG_USAGE_CLEAN)) {
	printf("MarkSegDirty: Segment %d not clean.\n", segNumber);
	return;
    }
    previousEntryPtr = (LfsSegUsageEntry *) NIL;
    previousSegNo = -1;
    segNo = cp->cleanSegList;
    while ((segNo != segNumber) && (segNo != -1)) {
	previousEntryPtr = LfsGetUsageArrayEntry(lfsPtr,segNo);
	previousSegNo = segNo;
	segNo = previousEntryPtr->activeBytes;
    }
    if (segNo != segNumber) {
	panic("MarkSegDirty: Can't find segment on clean list.\n");
	return;
    }

    if (previousEntryPtr == (LfsSegUsageEntry *) NIL) {
	cp->cleanSegList = entryPtr->activeBytes;
    } else {
	previousEntryPtr->activeBytes = entryPtr->activeBytes;
	LfsUsageArrayEntryModified(lfsPtr, previousSegNo);
    }

    entryPtr->flags = LFS_SEG_USAGE_DIRTY;
    entryPtr->activeBytes = 0;
    cp->numClean--;
    cp->numDirty++;
    LfsUsageArrayEntryModified(lfsPtr, segNumber);
}

/*
 *----------------------------------------------------------------------
 *
 * RecovSegUsageSummary --
 *
 *	Check the segment summary regions for the seg usage map. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
extern void
RecovSegUsageSummary(lfsPtr, pass, segPtr, startAddress, offset, 
			segSummaryHdrPtr) 
    Lfs	*lfsPtr;	/* File system description. */
    enum Pass pass;	/* Pass number of recovery. */
    LfsSeg *segPtr;	/* Segment being examined. */
    int startAddress;   /* Starting address being examined. */
    int offset;		/* Offset into segment being examined. */
    LfsSegSummaryHdr *segSummaryHdrPtr; /* Summary header pointer */
{
    int blocks, *blockArray, i, startAddr, fsBlocks;

    if (pass == PASS2) {
	/*
	 * Ignore seg usage array blocks during pass2 recovery.   These don't
	 * contain anything we wont recovery already with the current
	 * algorithm.
	 */
	return;
    }
    /*
     * During PASS1, we simply check to insure that things are not
     * corrupted.
     */
    fsBlocks = lfsPtr->superBlock.usageArray.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	fprintf(stderr,"%s:RecovSegUsageSummary: Wrong block count; is %d should be %s\n", deviceName, blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > lfsPtr->superBlock.usageArray.stableMem.maxNumBlocks)){
	   fprintf(stderr,"%s:RecovSegUsageSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	if (showLog) {
	    printf("Addr %d UsageArray Block %d\n", startAddr, blockArray[i]);
	}
    }
    stats.segUsageBlocks += blocks;

}

void
RollSegUsageForward(lfsPtr)
    Lfs	*lfsPtr;
{
    int	segNo, numSegs, oldCurrentSegment, currentSegment;
    LfsSegUsageEntry *entryPtr;
    LfsSegUsageCheckPoint	*cp = &lfsPtr->usageArray.checkPoint;

    if (verboseFlag) {
	printf("RollSegUsageForward: Log end moving from <%d,%d> to <%d,%d>\n",
		cp->currentSegment, cp->currentBlockOffset, logEnd.segNo,
		logEnd.blockOffset);
    }
    /*
     * Move the tail of log forward. To new logend.
     */
    currentSegment = oldCurrentSegment = cp->currentSegment;
    if (oldCurrentSegment == logEnd.segNo) {
	/*
	 * Log stops in the same segment as last checkpoint.
	 */
	cp->currentBlockOffset = logEnd.blockOffset;
    } else {
	entryPtr = LfsGetUsageArrayEntry(lfsPtr, oldCurrentSegment);
	entryPtr->activeBytes = cp->curSegActiveBytes;
	if (!(entryPtr->flags & LFS_SEG_USAGE_DIRTY) && 
		(entryPtr->activeBytes <= cp->dirtyActiveBytes)) {
	    entryPtr->flags |= LFS_SEG_USAGE_DIRTY;
	    cp->numDirty++;
	}
	LfsUsageArrayEntryModified(lfsPtr, oldCurrentSegment);
	MarkSegDirty(logEnd.segNo);
	cp->currentSegment = logEnd.segNo;
	cp->currentBlockOffset = logEnd.blockOffset;
	cp->curSegActiveBytes = 0;
	cp->previousSegment = -1;

	currentSegment = cp->currentSegment;
	stats.numLogSegments++;
    }


    /*
     * First besure that any segment that is part of the recovery
     * log is marked as not clean. 
     */
    numSegs = lfsPtr->superBlock.usageArray.numberSegments;
    for (segNo = 0; segNo < numSegs; segNo++) {
	/*
	 * Skip segments that are not part of the recovery log or
	 * are the current segment being written. 
	 */
	if (SegIsPartOfLog(segNo) && (segNo != currentSegment)) {
	    stats.numLogSegments++;
	    MarkSegDirty(segNo);
	}
    }
}

