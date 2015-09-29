/* 
 * MemInit.c --
 *
 *	Source code for "MemInit", a library procedure used internally
 *	by the storage allocator.  Also contains static variables shared
 *	between the allocator routines.  See memInt.h for overall
 *	information about how the allocator works.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/MemInit.c,v 1.1 88/05/20 15:49:17 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 * ----------------------------------------------------------------------------
 *
 * MemInit --
 *
 *      Initializes the dynamic storage allocator.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage allocation structures are initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
MemInit()
{
    int i;

    memInitialized = TRUE;

    /*
     * Clear out all of the bins.
     */

    for (i = 0; i < BIN_BUCKETS; i++) {
	memFreeLists[i] = NOBIN;
	mem_NumBlocks[i] = 0;
#ifdef MEM_TRACE
	mem_NumBinnedAllocs[i] = 0;
#endif
    }

    /*
     * Mark all the small buckets except 0 as available for binning.
     * Don't mark 0:  it's used as a special trigger in malloc to
     * return NULL when allocating zero bytes without affecting
     * the inner loop for binned allocations.
     */

    for (i = 1; i < SMALL_BUCKETS; i++) {
	memFreeLists[i] = (Address) NULL;
    }

    /* 
     * Initialize the large-object free list with two blocks that
     * mark its beginning and end.
     */

    memFirst = MemChunkAlloc(MIN_REGION_SIZE);
    memLast = memFirst + MIN_REGION_SIZE - sizeof(AdminInfo);
    SET_ADMIN(memFirst, MARK_FREE(memLast - memFirst));
    SET_ADMIN(memLast, MARK_DUMMY(0));
    memCurrentPtr	= memFirst;
    memLargestFree	= 0;
    memBytesFreed	= 0;
    mem_NumLargeBytes	= MIN_REGION_SIZE;

#ifdef MEM_TRACE
    mem_NumLargeAllocs	= 0;
    mem_NumLargeLoops	= 0;
#endif
}
