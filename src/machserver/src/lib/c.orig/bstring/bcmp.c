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
static char rcsid[] = "$Header: /sprite/src/lib/c/bstring/RCS/bcmp.c,v 1.3 91/03/24 19:02:17 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

/*
 * The following mask is used to detect proper alignment of addresses
 * for doing word operations instead of byte operations.  It is
 * machine-dependent.  If none of the following bits are set in an
 * address, then word-based operations may be used. This value is imported
 * from machparam.h
 */

#include "machparam.h"

#define WORDMASK WORD_ALIGN_MASK

#ifdef KERNEL
#include <vmHack.h>
#endif

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
bcmp(sourcePtr, destPtr, numBytes)
    register char *sourcePtr;		/* Where to compare from */
    register char *destPtr;		/* Where to compare to */
    register int numBytes;		/* The number of bytes to compare */
{
#ifdef VM_CHECK_BSTRING_ACCESS
    Vm_CheckAccessible(sourcePtr, numBytes);
    Vm_CheckAccessible(destPtr, numBytes);
#endif
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
	if (*destPtr++ != *sourcePtr++) {
	    return 1;
	}
	numBytes--;
    }

    return 0;
}
