/* 
 * Bit_FindFirstClear.c --
 *
 *	Source code for the Bit_FindFirstClear library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/bit/RCS/Bit_AnySet.c,v 1.1 88/06/19 14:34:46 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include "bit.h"
#include "bitInt.h"


/*-
 *-----------------------------------------------------------------------
 *
 * Bit_AnySet --
 *
 *	See if any bit in the mask is set
 *
 * Results:
 *	TRUE if one or more bit(s) is(are) set.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */

Boolean
Bit_AnySet(numBits, maskPtr)
    int	    	  numBits;  	/* Number of bits in array */
    register int  *maskPtr; 	/* The array of bits */
{
    register int  i;
    register int  lastMask;

    GET_MASK_AND_INTS(numBits, i, lastMask);

    while (i != 0) {
	if (*maskPtr) {
	    return (TRUE);
	}
	i--; maskPtr++;
    }

    if (lastMask && (*maskPtr & lastMask)) {
	return (TRUE);
    } else {
	return (FALSE);
    }
}
