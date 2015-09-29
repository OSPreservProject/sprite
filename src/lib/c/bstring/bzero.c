/* 
 * bzero.c --
 *
 *	Source code for the "bzero" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/bstring/RCS/bzero.c,v 1.7 92/05/14 18:56:24 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <bstring.h>
#include <machparam.h>

/*
 * The following mask is used to detect proper alignment of addresses
 * for doing word operations instead of byte operations.  It is
 * machine-dependent.  If none of the following bits are set in an
 * address, then word-based operations may be used. This value is imported
 * from machparam.h
 */

#define WORDMASK WORD_ALIGN_MASK

/*
 *----------------------------------------------------------------------
 *
 * bzero --
 *
 *	Clear a block of memory to zeroes.  This routine is optimized
 *	to do the clear in integer units, if the block is properly
 *	aligned.
 *
 * Results:
 *	Nothing is returned.  The memory at destPtr is cleared.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
bzero(destVoidPtr, numBytes)
    _VoidPtr destVoidPtr;		/* Where to zero. */
    register int numBytes;		/* How many bytes to zero. */
{
    register int *dPtr = (int *) destVoidPtr;
    char *destPtr = destVoidPtr;

    /*
     * If the address is on an aligned boundary then zero as much
     * as we can in big transfers (and also avoid loop overhead by
     * storing many zeros per iteration).  Once we have less than
     * a whole int to zero then it must be done by byte zeroes.
     */

    if (((int) dPtr & WORDMASK) == 0) {
	while (numBytes >= 16*sizeof(int)) {
	    dPtr[0] = 0;
	    dPtr[1] = 0;
	    dPtr[2] = 0;
	    dPtr[3] = 0;
	    dPtr[4] = 0;
	    dPtr[5] = 0;
	    dPtr[6] = 0;
	    dPtr[7] = 0;
	    dPtr[8] = 0;
	    dPtr[9] = 0;
	    dPtr[10] = 0;
	    dPtr[11] = 0;
	    dPtr[12] = 0;
	    dPtr[13] = 0;
	    dPtr[14] = 0;
	    dPtr[15] = 0;
	    dPtr += 16;
	    numBytes -= 16*sizeof(int);
	}
	while (numBytes >= sizeof(int)) {
	    *dPtr++ = 0;
	    numBytes -= sizeof(int);
	}
	if (numBytes == 0) {
	    return;
	}
	destPtr = (char *) dPtr;
    }

    /*
     * Zero the remaining bytes
     */

    while (numBytes > 0) {
	*destPtr++ = 0;
	numBytes--;
    }
}
