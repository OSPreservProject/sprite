/* 
 * Time_GetTime.c --
 *
 *	Time_GetTime library routine.  
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/time/RCS/Time_GetTime.c,v 1.1 91/04/21 22:43:56 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <spriteTime.h>
#include <sys/time.h>


/*
 *----------------------------------------------------------------------
 *
 * Time_GetTime --
 *
 *	Return the current time of day.  This is like gettimeofday(), 
 *	but it's defined for use with Time objects.
 *
 * Results:
 *	Returns the current time of day through resultPtr..
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
void
Time_GetTime(resultPtr)
    Time *resultPtr;
{
    struct timeval nowUnix;

    (void)gettimeofday(&nowUnix, (struct timezone *)0);
    resultPtr->seconds = nowUnix.tv_sec;
    resultPtr->microseconds = nowUnix.tv_usec;
}
