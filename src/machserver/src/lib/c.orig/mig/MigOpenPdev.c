/* 
 * MigOpenPdev.c --
 *
 *	This file contains the MigOpenPdev procedure, which
 *	opens the pseudo-device that communicates with
 *	either the global server or the local daemon.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/MigOpenPdev.c,v 2.3 90/09/24 14:46:50 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <fs.h>
#include <stdio.h>
#include <sys/file.h>
#include <mig.h>
#include "migInt.h"

extern int errno;
extern char *strerror();

/* 
 * Define the global variables that refer to the pdevs used.  Initialize
 * them to -1 to indicate they haven't been opened.
 */
int mig_GlobalPdev = -1;
int mig_LocalPdev = -1;


/*
 *----------------------------------------------------------------------
 *
 * MigOpenPdev --
 *
 *	Open the specified pseudo-device.  If global
 *	is non-zero, open the global pdev, else
 *	open the pdev for this host.
 *
 * Results:
 *	If successful, 0 is returned. If an error is encountered,
 *	then -1 is returned and errno indicates the error.
 *
 * Side effects:
 *	One of the global variables defined above is updated to
 *	store the descriptor.  This variable is used in subsequent
 *	accesses.  Also, if we give up after exceeding the maximum
 * 	number of retries, we set a flag so in the future we only
 *	try once rather than going through the full sleep-open-sleep
 * 	ritual.
 *
 *----------------------------------------------------------------------
 */
int
MigOpenPdev(global)
    int global;			/* Whether to open the global pdev. */
{
    int desc;
    char *name;
    int retries;
    int sleepTime;
    static int gaveUp = 0;
    

    if (global) {
	/*
	 * Assume no hosts are assigned to us, and that any hosts previously
	 * assigned have been revoked.
	 */
	MigHostCache(0, MIG_CACHE_REMOVE_ALL, TRUE);
    }
    name = Mig_GetPdevName(global);
    if (name == (char *) NULL) {
	fprintf(stderr,
	       "MigOpenPdev: Error getting name of pdev to open: %s.\n",
	       strerror(errno));
	fflush(stderr);
	return(-1);
    }
    desc = open(name, O_RDONLY, 0);
    if (desc < 0) {
	if (!gaveUp) {
	    gaveUp = 1;
	    fprintf(stderr,
		   "MigOpenPdev: Error opening pdev %s (still trying): %s.\n",
		   name, strerror(errno));
	    fflush(stderr);
	    for (retries = 0, sleepTime = 1;
		 retries < MIG_DAEMON_RETRY_COUNT;
		 retries++, sleepTime *= 2) {
		sleep(sleepTime);
		desc = open(name, O_RDONLY, 0);
		if (desc >= 0) {
		    fprintf(stderr,
			    "MigOpenPdev: Succeeded in opening pdev.\n");
		    fflush(stderr);
		    break;
		}
	    }
	    if (retries == MIG_DAEMON_RETRY_COUNT) {
		fprintf(stderr,
		       "MigOpenPdev: Unable to contact daemon.\n");
		fflush(stderr);
		return(-1);
	    }
	} else {
	    return(-1);
	}
    }
    if (global) {
	mig_GlobalPdev = desc;
    } else {
	mig_LocalPdev = desc;
    }
    gaveUp = 0;
    return(0);
}
