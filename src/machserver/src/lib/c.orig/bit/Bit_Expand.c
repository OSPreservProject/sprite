/* 
 * Bit_Expand.c --
 *
 *	Source code for the Bit_Expand library procedure.
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
static char rcsid[] = "$Header: Bit_Expand.c,v 1.2 88/07/25 10:33:50 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <bit.h>
#include <stdlib.h>
#include "bitInt.h"


/*-
 *-----------------------------------------------------------------------
 *
 * Bit_Expand --
 *
 *	Expand a dynamically-allocated bit array and return the new
 *	location. If the new size of the array is not actually larger
 *	than the old size, nothing is done. If oldBitPtr is NULL, this
 *	is the equivalent of a Bit_Alloc call.
 *
 * Results:
 *	The new location of the bit array.
 *
 * Side Effects:
 *	The contents of the array may be moved and the old array freed.
 *
 *-----------------------------------------------------------------------
 */

int *
Bit_Expand(newNumBits, oldNumBits, oldBitPtr)
    int	    newNumBits;	    /* The new number of bits in the array */
    int	    oldNumBits;	    /* The number of bits in the oldBitPtr array */
    int	    *oldBitPtr;	    /* The old array */
{
    int	    *newBitPtr;
    
    Bit_Alloc(newNumBits, newBitPtr);
    if (oldBitPtr != (int *)NULL) {
	Bit_Copy(oldNumBits, oldBitPtr, newBitPtr);
	Bit_Free(oldBitPtr);
    }
    return(newBitPtr);
}
