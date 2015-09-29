/* 
 * Mig_GetAllInfo.c --
 *
 *	Get info about one or more hosts from the global daemon.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_GetAllInfo.c,v 2.1 90/06/22 14:58:21 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <stdio.h>
#include <mig.h>
#include <kernel/net.h>
#include <errno.h>

extern int errno;
extern char *strerror();
extern char *malloc();

/*
 * The maximum number of records we transfer in a single ioctl.
 */
#define MAX_RECS 20

/*
 *----------------------------------------------------------------------
 *
 * Mig_GetAllInfo --
 *
 *	Get the records for all hosts, up to the number of hosts specified.
 *
 * Results:
 *	The number of valid records found is returned, and the data are
 *	placed in *infoPtr.  Upon error, -1 is returned.
 *
 * Side effects:
 *	The global daemon is contacted if it has not been already.
 *
 *----------------------------------------------------------------------
 */
int
Mig_GetAllInfo(infoPtr, numRecs)
    Mig_Info *infoPtr;		/* Pointer to array of structures */
    int numRecs;		/* number of structures in *infoPtr */
{
    Mig_InfoRequest request;
    static int init = 0;	/* Initialized? */
    static char *buffer; 	/* Dynamically-allocated buffer for result
				   of ioctl. */
    static unsigned int bufSize;/* Size of buffer. */
    int status;			/* Status of system calls. */
    int maxRecs = MAX_RECS;	/* Maximum number of recs in one ioctl. */
    int obtained;		/* Number obtained thus far. */
    int got;			/* Number obtained in one iteration. */
    int nextHost;		/* Next entry to look for. */
    int retry = 1;		/* Whether to retry after failed ioctl. */


    if (mig_GlobalPdev < 0) {
	if (MigOpenPdev(TRUE) < 0) {
	    return(-1);
	}
    }

    if (!init) {
	init = 1;
	bufSize = 2 * sizeof(int) +  maxRecs * sizeof(Mig_Info);
	buffer = malloc(bufSize);
	if (buffer == (char *) NULL) {
	    errno = ENOMEM;
	    init = 0;
	    return(-1);
	}
    }
    nextHost = 1;

    for (obtained = 0; obtained < numRecs;) {
	request.numRecs = maxRecs;
	request.firstHost = nextHost;

	if (MigSetAlarm() < 0) {
	    fprintf(stderr,
		    "Error setting alarm for contact with migd.\n");
	    return(-1);
	}
	status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_GETINFO,
			      sizeof(Mig_InfoRequest),
			      (char *) &request,
			      bufSize, buffer);
	if (MigClearAlarm() < 0) {
	    fprintf(stderr,
		    "Error clearing alarm for contact with migd.\n");
	}
	if (status != SUCCESS) {
	    close(mig_GlobalPdev);
	    mig_GlobalPdev = 0;
	    if (retry == 0 || MigOpenPdev(TRUE) < 0) {
		fprintf(stderr,
		       "Mig_GetAllInfo: error during ioctl to global master: %s\n",
		       Stat_GetMsg(status));
		errno = Compat_MapCode(status);
		return(-1);
	    }
	    retry = 0;
	    continue;
	}
	retry = 1;
	got = *((int *) buffer);
	if (got == 0) {
	    break;
	}
	obtained += got;
	bcopy(buffer + 2 * sizeof(int), (char *) infoPtr,
	      got * sizeof(Mig_Info));
	if (got < maxRecs) {
	    break;
	}
	nextHost = infoPtr[got-1].hostID + 1;
	infoPtr += got;
    }
    
    return(obtained);
    
}
