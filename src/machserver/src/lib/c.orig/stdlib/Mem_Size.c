/* 
 * Mem_Size.c --
 *
 *	Source code for the "Mem_Size" library procedure.  See memInt.h
 *	for overall information about how the allocator works.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_Size.c,v 1.3 88/07/25 11:10:55 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "memInt.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Mem_Size --
 *
 *      Return the size of a previously-allocated storage block.
 *
 * Results:
 *      The return value is the size of *blockPtr, in bytes.  This is
 *	the total usable size of the block.  It may be slightly greater
 *	than the size actually requested from malloc, since the size
 *	might have been rounded up to a convenient boundary.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY int
Mem_Size(blockPtr)
    Address blockPtr;	/* Pointer to storage block.  Must have been the
			 * return value from malloc at some previous time. */
{
    int admin;

    LOCK_MONITOR;

    if (!memInitialized) {
        panic("Mem_Size: allocator not initialized!\n");
	UNLOCK_MONITOR;
	return(0);			/* should never get here */
    }

    /* 
     *  Make sure that this block bears some resemblance to a
     *  well-formed storage block.
     */
    
    blockPtr -= sizeof(AdminInfo);
    admin = GET_ADMIN(blockPtr);
    if (!IS_IN_USE(admin)) {
	if (IS_DUMMY(admin)) {
	    panic("Mem_Size: storage block is corrupted\n");
	} else {
	    panic("Mem_Size: storage block is free\n");
	}
	UNLOCK_MONITOR;
	return(0);			/* (should never get here) */
    }

    UNLOCK_MONITOR;
    return(SIZE(admin) - sizeof(AdminInfo));
}
