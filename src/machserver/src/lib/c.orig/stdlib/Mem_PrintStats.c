/* 
 * Mem_PrintStats.c --
 *
 *	Source code for the "Mem_PrintStats" library procedure.  See memInt.h
 *	for overall information about how the allocator works..
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_PrintStats.c,v 1.5 89/02/10 09:54:31 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 * Data structure used to record statistics for the blocks managed by
 * the large block allocator.  Each entry holds information for all the
 * blocks of a given size.
 */

#define	MAX_TO_PRINT	256
static struct {
    int		size;		/* Size of the block. */
    int		num;		/* Number of blocks allocated. */
    int		free;		/* Number of blocks freed. */
    int		inUse;		/* Number of blocks still in use. */
    int		dummy;		/* Number of blocks used as dummy blocks. */
} topN[MAX_TO_PRINT + 1];

/*
 * Global variables that can be set to control thresholds for printing
 * statistics.
 */

int	mem_SmallMinNum = 1;		/* There must be at least this many
					 * binned objects of a size before info
					 * about its size gets printed. */
int	mem_LargeMinNum  = 1;		/* There must be at least this many
					 * non-binned objects of a size before
					 * info about the size gets printed. */
int	mem_LargeMaxSize = 10000;	/* Info is printed for non-binned
					 * objects larger than this regardless
					 * of how many of them there are. */

/*
 * Forward declarations to procedures defined in this file:
 */

extern void	PrintStatsSubr();

/*
 *----------------------------------------------------------------------
 *
 * Mem_PrintStats --
 *
 *	Print out memory statistics, using the default printing routine
 *	and default sizes.
 *
 *	See Mem_PrintStatsInt for details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Mem_PrintStats()
{
    LOCK_MONITOR;

    Mem_PrintStatsInt();

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Mem_PrintStatsInt --
 *
 *	Print out memory statistics.  Normally called only by
 *	Mem_PrintStats.  It's exported for use after a crash or when
 *	memory has been exhausted.  At this time the monitor lock is
 *	already down, so it better not be reacquired.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets printed, using the current print procedure
 *	and the parameter values stored in mem_SmallMinNum,
 *	mem_LargeMinNum, and mem_LargeMaxSize.
 *
 *----------------------------------------------------------------------
 */

INTERNAL void
Mem_PrintStatsInt()
{
    register Address	ptr;
    register int	i;
    int		totalBlocks, totalFree;
#ifdef MEM_TRACE
    int totalAllocs;
#endif
    int		allocBytes = 0;
    int		freeBytes = 0;
    int		warnedAboutOverflow;

    if (!memInitialized) {
	(*memPrintProc)(memPrintData, "Allocator not initialized yet.\n");
	return;
    }

#ifdef MEM_TRACE
    (*memPrintProc)(memPrintData, "\nTotal allocs = %d, frees = %d\n\n",
	    mem_NumAllocs, mem_NumFrees);
    totalAllocs = 0;
#else
    (*memPrintProc)(memPrintData, "Memory tracing not enabled, so %s",
	    "some statistics won't be printed.\n");
#endif

    (*memPrintProc)(memPrintData, "Small object allocator:\n");
    (*memPrintProc)(memPrintData,
	    "    Size     Total    Allocs    In Use\n");
    totalBlocks = totalFree = 0;
    for (i = 2; i < BIN_BUCKETS; i++) {
	int	numFree = 0;

	if ((memFreeLists[i] == NOBIN) || (mem_NumBlocks[i] == 0)) {
	    continue;
	}
	allocBytes += mem_NumBlocks[i] * INDEX_TO_BLOCKSIZE(i);
	for (ptr = memFreeLists[i]; 
	     ptr != (Address) NULL; 
	     ptr = (Address) GET_ADMIN(ptr)) {

	    freeBytes += INDEX_TO_BLOCKSIZE(i);
	    numFree += 1;
	}
	if (mem_NumBlocks[i] >= mem_SmallMinNum) {
#ifdef MEM_TRACE
	    (*memPrintProc)(memPrintData, "%8d%10d%10d%10d\n",
		    INDEX_TO_BLOCKSIZE(i),
		    mem_NumBlocks[i], mem_NumBinnedAllocs[i],
		    mem_NumBlocks[i] - numFree);
#else
	    (*memPrintProc)(memPrintData, "%8d%10d%10s%10d\n", 
		    INDEX_TO_BLOCKSIZE(i),
		    mem_NumBlocks[i], "??", mem_NumBlocks[i] - numFree);
#endif MEM_TRACE
	}
	totalBlocks += mem_NumBlocks[i];
#ifdef MEM_TRACE
	totalAllocs += mem_NumBinnedAllocs[i];
#endif
	totalFree += numFree;
    }
#ifdef MEM_TRACE
    (*memPrintProc)(memPrintData, "   Total%10d%10d%10d\n", totalBlocks,
	    totalAllocs, totalBlocks - totalFree);
#else
    (*memPrintProc)(memPrintData, "   Total%10d%10s%10d\n", totalBlocks,
	    "??", totalBlocks - totalFree);
#endif
    (*memPrintProc)(memPrintData, "Bytes allocated = %d, free = %d\n\n",
				allocBytes, freeBytes);

    /*
     * Initialize the largest N-sizes buffer.
     */
    for (i = 0; i < MAX_TO_PRINT + 1; i++) {
	topN[i].size = -1;
	topN[i].free = 0;
	topN[i].dummy = 0;
    }

    warnedAboutOverflow = 0;
    freeBytes = 0;
    for (ptr = memFirst + SIZE(GET_ADMIN(memFirst)); 
	 ptr != memLast;
	 ptr += SIZE(GET_ADMIN(ptr))) {

	int	admin;
	int	size;
	int	found;

	admin = GET_ADMIN(ptr);
	if (!IS_DUMMY(admin) && !IS_IN_USE(admin)) {
	    freeBytes += SIZE(admin);
	}

	size = SIZE(admin);
#ifdef MEM_TRACE
	if ((size - GET_ORIG_SIZE(ptr) < 4) && 
	    (size - GET_ORIG_SIZE(ptr) >= 0)) {
	    size = GET_ORIG_SIZE(ptr);
	}
#endif MEM_TRACE

	found = 0;
	for (i = 0; i < MAX_TO_PRINT; i++) {
	    if (size == topN[i].size) {
		found = 1;
		topN[i].num++;
		if (IS_DUMMY(admin)) {
		    topN[i].dummy++;
		} else if (IS_IN_USE(admin)) {
		    topN[i].inUse++;
		} else {
		    topN[i].free++;
		}
		break;
	    } else if (topN[i].size == -1) {
		found = 1;
		topN[i].size = size;
		topN[i].num = 1;
		if (IS_DUMMY(admin)) {
		    topN[i].dummy = 1;
		} else if (IS_IN_USE(admin)) {
		    topN[i].inUse = 1;
		} else {
		    topN[i].free = 1;
		}
		break;
	    }
	}

	if (!found && !warnedAboutOverflow) {
	    (*memPrintProc)(memPrintData,
		    "Ran out of large-object bins: needed more than %d.\n",
		    size);
	    warnedAboutOverflow = 1;
	}
    }
    (*memPrintProc)(memPrintData, "Large object allocator:\n");
    (*memPrintProc)(memPrintData, "   Total bytes managed: %d\n",
	    mem_NumLargeBytes);
    (*memPrintProc)(memPrintData, "   Bytes in use:        %d\n",
				    mem_NumLargeBytes - freeBytes);
    (*memPrintProc)(memPrintData, "%10s%10s%10s%10s\n", 
#ifdef MEM_TRACE
	    "Orig. Size", "Num", "Free", "In Use");
#else
	    "Size", "Num", "Free", "In Use");
#endif MEM_TRACE
    for (i = 0; topN[i].size != -1; i++) {
	if (((topN[i].num >= mem_LargeMinNum)
		|| (topN[i].size >= mem_LargeMaxSize))
		&& (topN[i].num != topN[i].dummy)) {
	    (*memPrintProc)(memPrintData, "%10d%10d%10d%10d\n",
		topN[i].size, topN[i].num, topN[i].free, topN[i].inUse);
	}
    }
}
