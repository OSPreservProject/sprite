/* 
 * Bit_FindFirstSet.c --
 *
 *	Source code for the Bit_FindFirstSet library procedure.
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
static char rcsid[] = "$Header: Bit_FindFirstSet.c,v 1.1 88/06/19 14:34:50 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include "bit.h"
#include "bitInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Bit_FindFirstSet --
 *
 *	Returns the index of the first rightmost instance of a '1' bit in the
 *	argument. The index of the rightmost bit is 0.
 *
 *	Ideally, this routine should take advantage of a hardware instruction
 *	to do the operation (e.g. BFFFO on the 68020).
 *
 * Results:
 *	if mask != 0	An index starting from 0 of the first rightmost set bit.
 *	if mask == 0	-1.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Bit_FindFirstSet(numBits, arrayPtr)
    int 	 numBits;	/* # of bits in arrayPtr */
    register int *arrayPtr;	/* The bit array as an array of ints. */
{
#define TEST(mask)	SET_TEST(mask)
#define QUICK_TEST 	SET_QUICK_TEST

	FIND_FIRST(numBits, arrayPtr)

#undef TEST
#undef QUICK_TEST
}
