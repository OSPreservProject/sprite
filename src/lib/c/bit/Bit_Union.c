/* 
 * Bit_Union.c --
 *
 *	Source code for the Bit_Union library procedure.
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
static char rcsid[] = "$Header: Bit_Union.c,v 1.1 88/06/19 14:34:52 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include "bit.h"
#include "bitInt.h"


/*-
 *-----------------------------------------------------------------------
 *
 * Bit_Union --
 *
 *	Take the union of two bit masks and store it some place.
 *	If no destination is given (it's a NULL pointer), the union
 *	is taken but the result isn't stored anyplace.
 *
 * Results:
 *	Returns TRUE if the union is non-empty. Stores the
 *	union in the destination array, overwriting its previous
 *	contents.
 *
 * Side Effects:
 *	The destination is overwritten.
 *
 *-----------------------------------------------------------------------
 */

Boolean
Bit_Union(numBits, src1Ptr, src2Ptr, destPtr)
    int	    	  numBits;  	/* Number of bits in all arrays */
    register int  *src1Ptr; 	/* First source for union */
    register int  *src2Ptr; 	/* Second source for union */
    register int  *destPtr; 	/* Destination of union */
{
    register int  i;
    register Boolean rval = FALSE;
    register int  lastMask;

    GET_MASK_AND_INTS (numBits, i, lastMask);

    if (destPtr == (int *)NULL) {
	/*
	 * No destination, just go through the union and return TRUE
	 * as soon as we find a non-empty member
	 */
	while (i != 0) {
	    if (*src1Ptr | *src2Ptr) {
		return(TRUE);
	    }
	    src1Ptr++; src2Ptr++; i--;
	}
	if (lastMask && ((*src1Ptr | *src2Ptr) & lastMask)) {
	    return (TRUE);
	}
    } else {
	/*
	 * Have a destination. Form the union into it and set
	 * rval TRUE if any of the destination integers is non-zero.
	 */
	while (i != 0) {
	    *destPtr = *src1Ptr | *src2Ptr;
	    if (*destPtr) {
		rval = TRUE;
	    }
	    destPtr++; src1Ptr++; src2Ptr++; i--;
	}
	if (lastMask) {
	    int	  src1 = *src1Ptr;
	    int	  src2 = *src2Ptr;

	    *destPtr &= ~lastMask;
	    *destPtr |= (src1 | src2) & lastMask;
	    if (*destPtr & lastMask) {
		rval = TRUE;
	    }
	}
    }
    return(rval);
}
