/* 
 * div.c --
 *
 *	Contains the source code for the "div" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/div.c,v 1.1 88/05/21 12:14:42 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdlib.h"

/*
 *----------------------------------------------------------------------
 *
 * div --
 *
 *	Compute the quotient and remainder of the division of numer
 *	by denom.
 *
 * Results:
 *	The return value is j, unless j is negative, in which case
 *	the return value is -j.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

div_t
div(numer, denom)
    int numer;			/* Number to divide into. */
    int denom;			/* Number that's divided into it. */
{
    div_t result;

    result.quot = numer/denom;
    result.rem = numer%denom;
    if ((result.rem ^ numer) < 0) {
	if (result.rem < 0) {
	    result.rem += denom;
	    result.quot -= 1;
	} else {
	    result.rem -= denom;
	    result.quot += 1;
	}
   }
   return result;
}
