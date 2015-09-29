/* 
 * migAlarm.c --
 *
 *	Routines to manage the interval timer so that
 *	operations to a remote host do not wait for recovery,
 *	and so that the program that uses this library is generally
 *	unaffected by the use of the interval timer.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <errno.h>
#include "migInt.h"

    
static int (*oldHandler) ();
static struct itimerval oldItimer;
static int alarmSet = 0;

/*
 * Set the default timeout, in seconds.
 */
#ifndef TIMEOUT
#define TIMEOUT 10
#endif /* TIMEOUT */


/*
 *----------------------------------------------------------------------
 *
 * AlarmHandler --
 *
 *	Routine to service a SIGALRM signal.  This routine disables
 *	the alarm (letting the caller reenable it when appropriate).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The alarm is disabled, and a warning message is printed.
 *
 *----------------------------------------------------------------------
 */
static int
AlarmHandler()
{
    
    alarm(0);
    fprintf(stderr, "Warning: remote migd operation timed out.");
    (void) signal (SIGALRM, SIG_IGN);
}


/*
 *----------------------------------------------------------------------
 *
 * MigSetAlarm --
 *
 *	Set the interval timer, storing away any previous timer and signal
 *	handler info.
 *
 * Results:
 *	If any error is encountered, returns -1, else 0.
 *
 * Side effects:
 *	Stores previous info in static variables.
 *
 *----------------------------------------------------------------------
 */

int
MigSetAlarm()
{
    int error;
    struct itimerval itimer;

    if (alarmSet) {
	printf("warning: mig alarm already set.\n");
    } else {
	alarmSet = 1;
    }
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = TIMEOUT;
    itimer.it_value.tv_usec = 0;

    oldHandler = (int (*)()) signal(SIGALRM, AlarmHandler);
    if (oldHandler == (int (*)()) -1) {
	fprintf(stderr, "Error setting signal handler.\n");
	return(-1);
    }
    if (setitimer(ITIMER_REAL, &itimer, &oldItimer) == -1) {
	error = errno;
	(void) signal(SIGALRM, oldHandler);
	errno = error;
	return(-1);
    }
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * MigClearAlarm --
 *
 *	Restore the interval timer and SIGALRM handler.
 *
 * Results:
 *	If any error is encountered, returns -1, else 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
MigClearAlarm()
{
    int error;
    struct itimerval itimer;

    if (!alarmSet) {
	fprintf(stderr, "warning: Mig alarm not set.\n");
	return(-1);
    } else {
	alarmSet = 0;
    }
    if (setitimer(ITIMER_REAL, &oldItimer, (struct itimerval *) NULL) == -1) {
	return(-1);
    }
    if (signal(SIGALRM, oldHandler) == (int (*)()) -1) {
	return(-1);
    }
}
