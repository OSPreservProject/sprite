/* 
 * Time_Divide.c --
 *
 *	Source code for the Time_Divide library procedure.
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
static char rcsid[] = "$Header: Time_Divide.c,v 1.2 88/06/27 17:23:31 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>

/*
 * OVERFLOW is used to see if multiple precision multiplication
 * and division are required in the Time_Multiply  and Time_Divide routines.
 * The number is equal to (2 ** 31 - 1) / 1000000.
 */

#define OVERFLOW 2147

/*
 *----------------------------------------------------------------------
 *
 * Time_Divide --
 *
 *      Computes a fraction of a time value.
 *	E.g. computes a time of a quarter second given the 
 *	constant time_OneSecond and a factor of 4.
 *
 * Results:
 *     The quotient of the division of the time value by the factor.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

void
Time_Divide(time, factor, resultPtr)
    Time          time;
    int	          factor;
    register Time *resultPtr;
{
    register int 	mod;

    if (factor != 0) {
	/*
	 * If there is a remainder from dividing the seconds portion by the
	 * factor, convert it to microseconds and add it to the microseconds 
	 * portion. This will make the division of microseconds by factor 
	 * accurate.
	 */

	resultPtr->seconds	= time.seconds / factor;
	mod       		= time.seconds % factor;

	/*
	 * Check to see if the multiplication involving mod will overflow.
	 * If so, use floating point (which is expensive).
	 */

	if (mod > OVERFLOW) {
	    resultPtr->microseconds = (((double) mod * (double) ONE_SECOND) +
	     			(double) time.microseconds) / (double) factor;
	} else {
	    resultPtr->microseconds = ((mod * ONE_SECOND) + 
					time.microseconds) / factor;
	}

	while (resultPtr->microseconds >= ONE_SECOND) {
	    resultPtr->seconds		+= 1;
	    resultPtr->microseconds	-= ONE_SECOND;
	}
	while (resultPtr->microseconds < 0) {
	    resultPtr->seconds		-= 1;
	    resultPtr->microseconds	+= ONE_SECOND;
	}
    } else {
	resultPtr->seconds	= 0;
	resultPtr->microseconds	= 0;
    }
}
