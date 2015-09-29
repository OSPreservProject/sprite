/* 
 * Time_Add.c --
 *
 *	Source code for the Time_Add library procedure.
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
static char rcsid[] = "$Header: Time_Add.c,v 1.2 88/06/27 17:23:21 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>


/*
 *----------------------------------------------------------------------
 *
 * Time_Add --
 *
 *      Adds two time values together. 
 *
 * Results:
 *     The sum of the 2 arguments.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

void
Time_Add(time1, time2, resultPtr)
    Time             time1;
    Time 	     time2;
    register	Time *resultPtr;
{
    resultPtr->seconds      = time1.seconds      + time2.seconds;
    resultPtr->microseconds = time1.microseconds + time2.microseconds;

    /*
     *  Normalize the microseconds portion to be less than 1 million.
     */
    if (resultPtr->microseconds >= ONE_SECOND) {
	resultPtr->seconds	 += 1;
	resultPtr->microseconds -= ONE_SECOND;
    }
}
