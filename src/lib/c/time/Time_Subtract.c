/* 
 * Time_Subtract.c --
 *
 *	Source code for the Time_Subtract library procedure.
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
static char rcsid[] = "$Header: Time_Subtract.c,v 1.2 88/06/27 17:23:37 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>


/*
 *----------------------------------------------------------------------
 *
 * Time_Subtract --
 *
 *      Subtracts one time value from another.
 *
 * Results:
 *     The difference of the 2 times.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

void
Time_Subtract(time1, time2, resultPtr)
    Time 	  time1;
    Time 	  time2;
    register Time *resultPtr;
{
    resultPtr->seconds      = time1.seconds      - time2.seconds;
    resultPtr->microseconds = time1.microseconds - time2.microseconds;

    if (resultPtr->microseconds < 0) {
	resultPtr->seconds	-= 1;
	resultPtr->microseconds += ONE_SECOND;
    }
}
