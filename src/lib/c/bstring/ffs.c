/* 
 * ffs.c --
 *
 *	Source code for the "ffs" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/bstring/RCS/ffs.c,v 1.3 89/06/19 14:15:25 jhh Exp $ SPRITE (Berkeley)";
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

/*
 * Used to find the first set bit. Value of the array is the index of the
 * first set bit in the array index.
 */
static int lookup[16] = {
    0,	/* 0x0    */
    1,	/* 0x01   */
    2,	/* 0x10   */
    1,	/* 0x11   */
    3,	/* 0x100  */
    1,	/* 0x101  */
    2,	/* 0x110  */
    1,	/* 0x111  */
    4,	/* 0x1000 */
    1,	/* 0x1001 */
    2,	/* 0x1010 */
    1,	/* 0x1011 */
    3,	/* 0x1100 */
    1,	/* 0x1101 */
    2,	/* 0x1110 */
    1	/* 0x1111 */
};

/*
 *----------------------------------------------------------------------
 *
 * ffs --
 *
 *	Find the least-significant 1-bit in the argument.
 *
 * Results:
 *	If there are no one-bits in the argument, then 0 is returned.
 *	Otherwise the return value is the index of the least-significant
 *	1 bit, where "1" corresponds to the low-order bit.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
ffs(i)
    int i;			/* Value to check for ones. */
{
    register int value, shiftcount;

    value = (unsigned int) i;
    for (shiftcount = 0; value != 0; value >>= 4, shiftcount += 4) {
	if (value & 0xf) {
	    return (lookup[value & 0xf] + shiftcount);
	}
    }
    return 0;
}
