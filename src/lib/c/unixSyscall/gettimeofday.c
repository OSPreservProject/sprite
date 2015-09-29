/* 
 * gettimeofday.c --
 *
 *	Procedure to map from Unix gettimeofday system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/gettimeofday.c,v 1.3 88/07/29 17:55:56 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"
#include <sys/time.h>
#include <spriteTime.h>



/*
 *----------------------------------------------------------------------
 *
 * gettimeofday --
 *
 *	Procedure to map from Unix gettimeofday system call to 
 *	Sprite Sys_GetTimeOfDay.
 *
 * Results:
 *	UNIX_SUCCESS 	- the call was successful.
 *	UNIX_ERROR 	- the call was not successful. 
 *			  The actual error code stored in errno.  
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
gettimeofday(tp, tzp)
    struct timeval *tp;
    struct timezone *tzp;
{
    ReturnStatus status;	/* result returned by Sys_GetTimeOfDay */
    int		localOffset;	/* offset in minutes from UTC */
    Boolean	DST;		/* TRUE if Daylight Savings Time is observed */

    status = Sys_GetTimeOfDay((Time *) tp, &localOffset, &DST);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	if (tzp != (struct timezone *) NULL) {
	    /*
	     * Unix negates the local offset from UTC to make it positive
	     * for locations west of the prime meridian. 
	     */
	    tzp->tz_minuteswest 	= -localOffset;
	    tzp->tz_dsttime 		= DST;
	}
	return(UNIX_SUCCESS);
    }
}
