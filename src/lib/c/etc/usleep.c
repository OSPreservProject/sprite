/* 
 * usleep.c --
 *
 *	Sleep for specified number of microseconds.
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
static char rcsid[] = "$Header$";
#endif /* not lint */

#include <sys/time.h>

/*
 *----------------------------------------------------------------------
 *
 * usleep --
 *
 *    Suspend the current process for the number  of  microseconds
 *    specified  by  the argument.  The actual suspension time may
 *    be an arbitrary amount longer because of other  activity  in
 *    the  system,  or because of the time spent in processing the
 *    call.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
usleep(usec)
    unsigned long usec;
{

    struct timeval timeout;
    timeout.tv_usec = usec % (unsigned long) 1000000;
    timeout.tv_sec = usec / (unsigned long) 1000000;
    select(0, (void *) 0, (void *) 0, (void *) 0, &timeout);
    return 0;
}

