/* 
 * Mig_GetStats.c --
 *
 *	Get statistics buffer from the global daemon.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_GetStats.c,v 2.0 90/03/10 13:12:45 douglis Stable Locker: douglis $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <stdio.h>
#include <mig.h>
#include <errno.h>

extern int errno;
extern char *strerror();


/*
 *----------------------------------------------------------------------
 *
 * Mig_GetStats --
 *
 *	Get the statistics buffer from migd.
 *
 * Results:
 *	0 for success, or -1 on error.
 *
 * Side effects:
 *	The global daemon is contacted if it has not been already.
 *
 *----------------------------------------------------------------------
 */
int
Mig_GetStats(statsPtr)
    Mig_Stats *statsPtr;	/* Pointer to stats buffer */
{
    int status;			/* Status of system calls. */
    int retry;			/* Whether to retry after failed ioctl. */


    if (mig_GlobalPdev < 0) {
	if (MigOpenPdev(TRUE) < 0) {
	    return(-1);
	}
    }

    for (retry = 1; retry >= 0; retry--) {
	if (MigSetAlarm() < 0) {
	    fprintf(stderr,
		    "Error setting alarm for contact with migd.\n");
	    return(-1);
	}
	status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_GET_STATS,
			      0, (char *) NULL, sizeof(Mig_Stats),
			      (char *) statsPtr);
	if (MigClearAlarm() < 0) {
	    fprintf(stderr,
		    "Error clearing alarm for contact with migd.\n");
	}
	if (status != SUCCESS) {
	    close(mig_GlobalPdev);
	    mig_GlobalPdev = 0;
	    if (retry == 0 || MigOpenPdev(TRUE) < 0) {
		fprintf(stderr,
		       "Mig_GetStats: error during ioctl to global master: %s\n",
		       Stat_GetMsg(status));
		errno = Compat_MapCode(status);
		return(-1);
	    }
	} else {
	    return(0);
	}
    }
    return(-1);
}
