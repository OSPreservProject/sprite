/* 
 * Mig_ReturnHosts.c --
 *
 *	Source code for the Mig_ReturnHosts procedure.
 *	This procedure returns the specified hosts to the
 *	pool of idle hosts.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_ReturnHosts.c,v 2.2 90/09/24 14:46:48 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <mig.h>
#include <host.h>
#include <errno.h>
#include <stdio.h>
#include "migInt.h"

extern int errno;
extern char *strerror();
extern char *malloc();


/*
 *----------------------------------------------------------------------
 *
 * Mig_ReturnHosts --
 *
 *	Return hosts to the pool of idle hosts.  
 *
 * Results:
 *	A non-zero status indicates an error, in which case errno reflects
 *	the error from the migration daemon.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Mig_ReturnHosts(numHosts, hostArray)
    int numHosts;		/* Number of hosts to return, or
				 * MIG_ALL_HOSTS. */
    int *hostArray;		/* Array of hostIDs, or NULL if all hosts. */
{
    int status;			/* Status of system calls. */
    int dummyArray[1];		/* Array used in the event of all hosts being
				 * returned. */
    int *arrayPtr;		/* Pointer specifying which array to use. */
    int size;			/* Size of buffer for ioctl. */
    int i;			/* Counter. */
    int actualHosts;		/* Number of hosts we really want to return. */
    int *allocArray;		/* Array used in the event of specific hosts
				   being returned.  */


    if (mig_GlobalPdev < 0) {
	errno = EINVAL;
	return(-1);
    }
    if (numHosts <= 0 && hostArray != (int *) NULL) {
	errno = EINVAL;
	return(-1);
    }

    if (hostArray == (int *) NULL) {
	MigHostCache(0, MIG_CACHE_REMOVE_ALL);
	arrayPtr = dummyArray;
	allocArray = (int *) NULL;
	dummyArray[0] = MIG_ALL_HOSTS;
	size = sizeof(int);
    } else {
	allocArray = (int *) malloc(numHosts * sizeof(int));
	if (allocArray == (int *) NULL) {
	    errno = ENOMEM;
	    return(-1);
	}
	for (i = 0, actualHosts = 0; i < numHosts; i++) {
	    int host = hostArray[i];
	    if (Mig_ConfirmIdle(host) &&
		MigHostCache(host, MIG_CACHE_REMOVE, FALSE)) {
		allocArray[actualHosts] = host;
		actualHosts++;
	    }
	}
	if (actualHosts == 0) {
	    return(0);
	}
	arrayPtr = allocArray;
	size = actualHosts * sizeof(int);
    }
    if (MigSetAlarm() < 0) {
	fprintf(stderr,
		"Error setting alarm for contact with migd.\n");
	return(-1);
    }
    status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_DONE,
			  size, (char *) arrayPtr, 0, (char *) NULL);
    if (MigClearAlarm() < 0) {
	fprintf(stderr,
		"Error clearing alarm for contact with migd.\n");
    }
    if (allocArray != (int *) NULL) {
	free((char *) allocArray);
    }
    if (status != SUCCESS) {
	fprintf(stderr,
	       "Mig_ReturnHosts: error during ioctl to global master: %s\n",
	       Stat_GetMsg(status));
	errno = Compat_MapCode(status);
	return(-1);
    }
    return(0);
}
