/* 
 * settimeofday.c --
 *
 *	Procedure to map from Unix settimeofday system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/settimeofday.c,v 1.3 88/11/07 14:43:30 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>

#include "compatInt.h"
#include <sys/time.h>



/*
 *----------------------------------------------------------------------
 *
 * settimeofday --
 *
 *	Procedure to map from Unix settimeofday system call to 
 *	Sprite Sys_SetTimeOfDay.
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
settimeofday(tp, tzp)
    struct timeval *tp;
    struct timezone *tzp;
{
    ReturnStatus status;	/* result returned by Sys_SetTimeOfDay */

    /*
     * Unix negates the local offset from UTC to make it positive
     * for locations west of the prime meridian. 
     */

    if (tzp == NULL) {
	int localOffset;
	Boolean DST;

	status = Sys_GetTimeOfDay((Time *) NULL, &localOffset, &DST);
	status = Sys_SetTimeOfDay(tp, localOffset, DST);
    } else if (tp == NULL) {
	Time currentTime;

	status = Sys_GetTimeOfDay(&currentTime, (int *) NULL,
				  (Boolean *) NULL);
	status = Sys_SetTimeOfDay(&currentTime,  -(tzp->tz_minuteswest),
				  tzp->tz_dsttime);
    } else {
	status = Sys_SetTimeOfDay(tp, -(tzp->tz_minuteswest), tzp->tz_dsttime);
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
