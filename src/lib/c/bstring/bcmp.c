/* 
 * bcmp.c --
 *
 *	Source code for the "bcmp" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/bstring/RCS/bcmp.c,v 1.5 92/05/14 18:58:01 kupfer Exp $ SPRITE (Berkeley)";
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
 * bcmp --
 *
 *	Compare two blocks of memory for equality.  This routine is
 *	optimized to do integer compares.  However, if either sourcePtr
 *	or destPtr points to non-word-aligned addresses then it is
 *	forced to do single-byte compares.
 *
 * Results:
 *	The return value is zero if the blocks at sourcePtr and destPtr
 *	are identical, non-zero if they differ.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
bcmp(sourceVoidPtr, destVoidPtr, numBytes)
    _CONST _VoidPtr sourceVoidPtr; 	/* Where to compare from */
    _CONST _VoidPtr destVoidPtr;	/* Where to compare to */
    register int numBytes;		/* The number of bytes to compare */
{
    register _CONST char *sourcePtr = sourceVoidPtr;
    register _CONST char *destPtr = destVoidPtr;

    /*
     * If both the sourcePtr and the destPtr point to aligned addesses then
     * compare as much as we can in integer units.  Once we have less than
     * a whole int to compare then it must be done by byte compares.
     */

    if ((((int) sourcePtr & WORDMASK) == 0)
	    && (((int) destPtr & WORDMASK) == 0)) {
	while (numBytes >= sizeof(int)) {
	    if (*(int *) destPtr != *(int *) sourcePtr) {
		return 1;
	    }
	    sourcePtr += sizeof(int);
	    destPtr += sizeof(int);
	    numBytes -= sizeof(int);
	}
    }

    /*
     * Compare the remaining bytes
     */

    while (numBytes > 0) {
	if (*destPtr != *sourcePtr) {
	    return 1;
	}
	++destPtr;
	++sourcePtr;
	numBytes--;
    }

    return 0;
}
