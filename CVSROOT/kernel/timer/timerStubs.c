/* 
 * timerStubs.c --
 *
 *      Stubs for Unix compatible system calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <status.h>
#include <errno.h>
#include <user/sys/types.h>
#include <user/sys/time.h>
#include <mach.h>
#include <proc.h>
#include <timer.h>
#include <vm.h>

#ifndef Mach_SetErrno
#define Mach_SetErrno(err)
#endif

int debugTimerStubs;

int
Timer_GettimeofdayStub(tp, tzpPtr)
    struct timeval 	*tp;
    struct timezone 	*tzpPtr;
{
    struct timezone	tzp;
    int			localOffset;
    Boolean		DST;	
    ReturnStatus	status;
    Time		curTime;

    if (debugTimerStubs) {
	printf("Timer_GettimeofdayStub\n");
    }
    Timer_GetRealTimeOfDay(&curTime, &localOffset, &DST);

    status = Vm_CopyOut(sizeof(Time), (Address)&curTime, (Address)tp);
    if (status == SUCCESS) {
	if (tzpPtr != (struct timezone *) NULL) {
	    tzp.tz_minuteswest 	= -localOffset;
	    tzp.tz_dsttime 		= DST;
	    status = Vm_CopyOut(sizeof(struct timezone), (Address)&tzp,
		                (Address)tzpPtr);
	}
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

int
Timer_SettimeofdayStub(tp, tzpPtr)
    struct timeval *tp;
    struct timezone *tzpPtr;
{
    ReturnStatus status;	/* result returned by Sys_SetTimeOfDay */
    Time curTime;
    struct timezone tzp;


    if (debugTimerStubs) {
	printf("Timer_SettimeofdayStub\n");
    }
    /*
     * Unix negates the local offset from UTC to make it positive
     * for locations west of the prime meridian. 
     */

    if (tzpPtr == NULL) {
	int localOffset;
	Boolean DST;

	Timer_GetRealTimeOfDay(&curTime, &localOffset, &DST);
	status = Vm_CopyIn(sizeof(Time), (Address)tp, (Address)&curTime);
	if (status == SUCCESS) {
	    Timer_SetTimeOfDay(curTime, localOffset, DST);
	}
    } else if (tp == NULL) {
	Timer_GetRealTimeOfDay(&curTime, (int *) NULL, (Boolean *) NULL);
	status = Vm_CopyIn(sizeof(tzp), (Address)tzpPtr, (Address)&tzp);
	if (status == SUCCESS) {
	    Timer_SetTimeOfDay(curTime, -(tzp.tz_minuteswest), tzp.tz_dsttime);
	}
    } else {
	status = Vm_CopyIn(sizeof(Time), (Address)tp, (Address)&curTime);
	if (status == SUCCESS) {
	    status = Vm_CopyIn(sizeof(tzp), (Address)tzpPtr, (Address)&tzp);
	    if (status == SUCCESS) {
		Timer_SetTimeOfDay(curTime,
		                   -(tzp.tz_minuteswest), tzp.tz_dsttime);
	    }
	}
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*ARGSUSED*/
int
Timer_AdjtimeStub(delta, olddelta)

    struct timeval	*delta;
    struct timeval	*olddelta;
{
    printf("adjtime is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

