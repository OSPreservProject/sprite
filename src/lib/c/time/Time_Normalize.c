/* 
 * Time_Normalize.c --
 *
 *	Source code for the Time_Normalize library procedure.
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
static char rcsid[] = "$Header: Time_Normalize.c,v 1.2 88/06/27 17:23:35 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>


/*
 *----------------------------------------------------------------------
 *
 * Time_Normalize --
 *
 *      Normalizes a time value such that the microseconds portion
 *	is less than 1 million.
 *
 * Results:
 *     A normalized time value.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

void
Time_Normalize(timePtr)
    register Time	*timePtr;
{
    while (timePtr->microseconds >= ONE_SECOND) {
	timePtr->seconds	+= 1;
	timePtr->microseconds	-= ONE_SECOND;
    }
    while (timePtr->microseconds < 0) {
	timePtr->seconds	-= 1;
	timePtr->microseconds	+= ONE_SECOND;
    }
}
