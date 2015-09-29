/* 
 * Mig_GetInfo.c --
 *
 *	Source code for the Mig_GetInfo procedure, which gets migration
 *	and load information for a particular host.  Note: if getting
 *	local information, the "state" of the machine is not guaranteed to
 *	be accurate; this variant is provided just for getting updated load
 *	averages periodically.  
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_GetInfo.c,v 2.1 90/06/22 14:58:23 douglis Exp $ SPRITE (Berkeley)";
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
 *----------------------------------------------------------------------
 *
 * Mig_GetInfo --
 *
 *	Get the record for the given host (PROC_MY_HOSTID implies the current
 * 	host).
 *
 * Results:
 *	A pointer to a static area (valid until the next call to Mig_GetInfo)
 *	is returned if the call is successful.  On error, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define INFO_BUF_SIZE (2 * sizeof(int) + sizeof(Mig_Info))
Mig_Info *
Mig_GetInfo(hostID)
    int hostID;
{
    int status;
    Mig_InfoRequest request;
    static Mig_Info info;	/* Migration data, returned to user. */
    static char *buffer = NULL;
    int retry = 1;

    if (hostID == PROC_MY_HOSTID) {
	if (mig_LocalPdev < 0) {
	    if (MigOpenPdev(FALSE) < 0) {
		return((Mig_Info *) NULL);
	    }
	}
	while (retry >= 0) {
	    status = read(mig_LocalPdev, (char *) &info, sizeof(Mig_Info));
	    if (status != sizeof(Mig_Info)) {
		close(mig_LocalPdev);
		mig_LocalPdev = -1;
		if (retry == 0 || MigOpenPdev(FALSE) < 0) {
		    fprintf(stderr,
			   "Mig_GetInfo: error reading load from migd: %s\n",
			   strerror(errno));
		    return((Mig_Info *) NULL);
		}
		retry--;
	    } else {
		break;
	    }
	}
    } else {
	if (mig_GlobalPdev < 0) {
	    if (MigOpenPdev(TRUE) < 0) {
		return((Mig_Info *) NULL);
	    }
	}
	if (buffer == (char *) NULL) {
	    buffer = malloc(INFO_BUF_SIZE);
	    if (buffer == (char *) NULL) {
		/*
		 * Out of memory?!
		 */
		return((Mig_Info *) NULL);
	    }
	}
	request.numRecs = 1;
	request.firstHost = hostID;
	while (retry >= 0) {
	    if (MigSetAlarm() < 0) {
		fprintf(stderr,
			"Error setting alarm for contact with migd.\n");
		return((Mig_Info *) NULL);
	    }
	    status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_GETINFO,
				  sizeof(Mig_InfoRequest),
				  (char *) &request,
				  INFO_BUF_SIZE, (char *) buffer);
	    if (MigClearAlarm() < 0) {
		fprintf(stderr,
			"Error clearing alarm for contact with migd.\n");
	    }
	    if (status != SUCCESS) {
		close(mig_GlobalPdev);
		mig_GlobalPdev = -1;
		if (retry == 0 || MigOpenPdev(TRUE) < 0) {
		    fprintf(stderr,
			   "Mig_GetInfo: error during ioctl to global master: %s\n",
			   Stat_GetMsg(status));
		    errno = Compat_MapCode(status);
		    return((Mig_Info *) NULL);
		}
		retry--;
	    } else {
		break;
	    }
	}
	if (*((int *) buffer) != 1) {
	    fprintf(stderr,
		   "Mig_GetInfo: Got %d hosts during ioctl to global master.\n",
		   *((int *) buffer));
	    return((Mig_Info *) NULL);
	}
	bcopy(buffer + 2 * sizeof(int), (char *) &info,
	      sizeof(Mig_Info));
    }
    return(&info);
}

