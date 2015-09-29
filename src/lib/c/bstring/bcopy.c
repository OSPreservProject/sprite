/* 
 * bcopy.c --
 *
 *	Source code for the "bcopy" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/bstring/RCS/bcopy.c,v 1.9 92/05/14 18:57:09 kupfer Exp $ SPRITE (Berkeley)";
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

/*
 * Just because we can do loads from half-word addresses on a sun3 doesn't
 * mean we want to.  Make sure the load address is word-aligned and you'll
 * bcopy twice as fast when the dest is greater than the source.
 */
#ifdef sun3
#define WORDMASK 0x3
#else
#define WORDMASK WORD_ALIGN_MASK
#endif

/*
 *----------------------------------------------------------------------
 *
 * bcopy --
 *
 *	Copy numBytes from *sourcePtr to *destPtr.  This routine is
 *	optimized to do transfers when sourcePtr and destPtr are both
 *	integer-aligned and point to large areas.
 *
 * Results:
 *	There is no return value.  The memory at *destPtr is modified.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
bcopy(sourceVoidPtr, destVoidPtr, numBytes)
    _CONST _VoidPtr sourceVoidPtr; /* Where to copy from */
    _VoidPtr destVoidPtr;	/* Where to copy to */
    register int numBytes;	/* The number of bytes to copy */
{
    register _CONST int *sPtr = (int *) sourceVoidPtr;
    register int *dPtr = (int *) destVoidPtr;
    _CONST char *sourcePtr = sourceVoidPtr;
    char *destPtr = destVoidPtr;

    /*
     * If the destination is below the source then it is safe to copy
     * in the forward direction.  Otherwise, we must start at the top
     * and work down, again optimizing for large transfers.
     */

    if (dPtr < sPtr) {
	/*
	 * If both the source and the destination point to aligned
	 * addresses then copy as much as we can in large transfers.  Once
	 * we have less than a whole int to copy then it must be done by
	 * byte transfers.  Furthermore, use an expanded loop to avoid
	 * the overhead of continually testing loop variables.
	 */

	if (!((((int) sPtr) | (int) dPtr) & WORDMASK)) {
	    while (numBytes >= 16*sizeof(int)) {
		dPtr[0] = sPtr[0];
		dPtr[1] = sPtr[1];
		dPtr[2] = sPtr[2];
		dPtr[3] = sPtr[3];
		dPtr[4] = sPtr[4];
		dPtr[5] = sPtr[5];
		dPtr[6] = sPtr[6];
		dPtr[7] = sPtr[7];
		dPtr[8] = sPtr[8];
		dPtr[9] = sPtr[9];
		dPtr[10] = sPtr[10];
		dPtr[11] = sPtr[11];
		dPtr[12] = sPtr[12];
		dPtr[13] = sPtr[13];
		dPtr[14] = sPtr[14];
		dPtr[15] = sPtr[15];
		sPtr += 16;
		dPtr += 16;
		numBytes -= 16*sizeof(int);
	    }
	    while (numBytes >= sizeof(int)) {
		*dPtr++ = *sPtr++;
		numBytes -= sizeof(int);
	    }
	    if (numBytes == 0) {
		return;
	    }
	}

	/*
	 * Copy the remaining bytes.
	 */

	sourcePtr = (char *) sPtr;
	destPtr = (char *) dPtr;
	while (numBytes > 0) {
	    *destPtr++ = *sourcePtr++;
	    numBytes--;
	}
    } else {
	/*
	 * Handle extra bytes at the top that are due to the transfer
	 * length rather than pointer misalignment.
	 */
	while (numBytes & WORDMASK) {
	    numBytes --;
	    destPtr[numBytes] = sourcePtr[numBytes];
	}
	sPtr = (int *) (sourcePtr + numBytes);
	dPtr = (int *) (destPtr + numBytes);

	if (!((((int) sPtr) | (int) dPtr) & WORDMASK)) {
	    while (numBytes >= 16*sizeof(int)) {
		sPtr -= 16;
		dPtr -= 16;
		dPtr[15] = sPtr[15];
		dPtr[14] = sPtr[14];
		dPtr[13] = sPtr[13];
		dPtr[12] = sPtr[12];
		dPtr[11] = sPtr[11];
		dPtr[10] = sPtr[10];
		dPtr[9] = sPtr[9];
		dPtr[8] = sPtr[8];
		dPtr[7] = sPtr[7];
		dPtr[6] = sPtr[6];
		dPtr[5] = sPtr[5];
		dPtr[4] = sPtr[4];
		dPtr[3] = sPtr[3];
		dPtr[2] = sPtr[2];
		dPtr[1] = sPtr[1];
		dPtr[0] = sPtr[0];
		numBytes -= 16*sizeof(int);
	    }
	    while (numBytes >= sizeof(int)) {
		*--dPtr = *--sPtr;
		numBytes -= sizeof(int);
	    }
	    if (numBytes == 0) {
		return;
	    }
	}

	/*
	 * Copy the remaining bytes.
	 */

	destPtr = (char *) dPtr;
	sourcePtr = (char *) sPtr;
	while (numBytes > 0) {
	    *--destPtr = *--sourcePtr;
	    numBytes--;
	}
    }
}
