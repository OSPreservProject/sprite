/* 
 * free.c --
 *
 *	Source code for the "free" library procedure.  See memInt.h for
 *	overall information about how the allocator works.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdlib/RCS/free.c,v 1.3 91/12/12 21:56:41 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include "memInt.h"

/*
 * Variable that indicates whether or not it's acceptable to free
 * a block that's already free (for some reason, many UNIX programs,
 * damn them to hell, do this).
 */

int	memAllowFreeingFree = 1;

/*
 * ----------------------------------------------------------------------------
 *
 * free --
 *
 *      Return a previously-allocated block of storage to the free pool.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage pointed to by blockPtr is marked as free and returned
 *	to the free pool.  Nothing in the bytes pointed to by blockPtr is
 *	modified at this time:  no change will occur until at least the
 *	next call to malloc or realloc.  This means that callers may use
 *	the contents of a block for a short time after free-ing it (e.g.
 *	to read a "next" pointer).
 *
 * ----------------------------------------------------------------------------
 */

ENTRY
void
free(blockPtr)
    register Address blockPtr;	/* Pointer to storage to be freed.  Must
				 * have been the return value from Mem_Alloc
				 * at some previous time.  */
{
    register int  admin;
    register int  index;
    register int  size;

    LOCK_MONITOR;

#ifdef MEM_TRACE
    mem_NumFrees++;
#endif

    if (!memInitialized) {
        panic("Mem_Free: allocator not initialized!\n");
	return;		/* should never get here */
    }
    if (blockPtr == NULL) {
	UNLOCK_MONITOR;
	return;
    }

    /* 
     *  Make sure that this block bears some resemblance to a
     *  well-formed storage block.
     */

    blockPtr -= sizeof(AdminInfo);
    admin = GET_ADMIN(blockPtr);
    if (!IS_IN_USE(admin)) {
	if (IS_DUMMY(admin)) {
	    panic("Mem_Free: storage block corrupted\n");
	}
	if (!memAllowFreeingFree) {
	    panic("Mem_Free: storage block already free\n");
	}
	UNLOCK_MONITOR;
	return;
    }

    /* This procedure is easier for un-binned blocks (those without the
     * DUMMY bit set) than for the binned ones.  If un-binned, just clear
     * the use bit and record how many bytes were freed, for use later
     * when deciding whether or not to allocate more storage.
     */

    size = SIZE(admin);
    index = BLOCKSIZE_TO_INDEX(size);
    if (!IS_DUMMY(admin)) {
	SET_ADMIN(blockPtr, MARK_FREE(admin));
	memBytesFreed += SIZE(admin);
    } else {
	/* 
	 * For small blocks, add the block back onto its free list.
	 */

	index = BLOCKSIZE_TO_INDEX(SIZE(admin));
	SET_ADMIN(blockPtr, MARK_FREE((int) memFreeLists[index]));
	memFreeLists[index] = blockPtr;
    }

#ifdef MEM_TRACE
    MemDoTrace(FALSE, blockPtr, Mem_CallerPC(), size);
#endif MEM_TRACE

    UNLOCK_MONITOR;
}
