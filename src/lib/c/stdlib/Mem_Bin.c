/* 
 * Mem_Bin.c --
 *
 *	Source code for the "Mem_Bin" library procedure.  See memInt.h
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_Bin.c,v 1.2 88/07/25 14:19:20 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_Bin --
 *
 *	Make objects of the given size be binned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bin corresponding to blocks of the given size is initialized.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Mem_Bin(numBytes)
    int	numBytes;
{
    int	index;

    LOCK_MONITOR;

    if (!memInitialized) {
	MemInit();
    } 
    numBytes = BYTES_TO_BLOCKSIZE(numBytes);
    if (numBytes > BIN_SIZE) {
	UNLOCK_MONITOR;
	return;
    }
    index = BLOCKSIZE_TO_INDEX(numBytes);
    if (memFreeLists[index] == NOBIN) {
	memFreeLists[index] = (Address) NULL;
    }

    UNLOCK_MONITOR;
}
