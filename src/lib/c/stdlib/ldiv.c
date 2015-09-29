/* 
 * ldiv.c --
 *
 *	Contains the source code for the "ldiv" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/ldiv.c,v 1.2 89/03/22 00:47:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdlib.h>

/*
 *----------------------------------------------------------------------
 *
 * ldiv --
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

ldiv_t
ldiv(numer, denom)
    long int numer;			/* Number to divide into. */
    long int denom;			/* Number that's divided into it. */
{
    ldiv_t result;

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
