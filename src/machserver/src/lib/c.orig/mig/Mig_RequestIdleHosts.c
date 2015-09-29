/* 
 * Mig_RequestIdleHosts.c --
 *
 *	Source code for the Mig_RequestIdleHosts procedure.
 *	This procedure returns one or more idle hosts that may be
 *	used for migration with a specified priority.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_RequestIdleHosts.c,v 2.4 90/09/24 14:46:49 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <mig.h>
#include <host.h>
#include <errno.h>
#include <stdio.h>
#include <status.h>
#include "migInt.h"

extern int errno;
extern char *strerror();
extern char *malloc();

void (*migCallBackPtr)() = NULL;/* Procedure to call if idle hosts become
				   available, or NULL. */

/*
 * Define some state values to keep track of what we know about the
 * global daemon.
 *
 * MIGD_OKAY	- we believe everything's okay.
 * MIGD_WAITING	- waiting for the local migration daemon to talk to the global
 * 		  daemon.
 * MIGD_ERROR	- we've already hit an error and we don't want to announce 
 * 		  further errors.
 */
typedef enum {
    MIGD_OKAY,
    MIGD_WAITING,
    MIGD_ERROR,
} MigdState;

/*
 *----------------------------------------------------------------------
 *
 * StartMigd --
 *
 *	Fork a process to become the local migration daemon.
 *
 * Results:
 *	0 for successful completion, -1 for error, in which case
 *	errno indicates the nature of the error.  The parent
 *	will return success as long as it can successfully fork.  That
 * 	doesn't necessarily mean the migration daemon has been started,
 *	at least by the child, but the parent will go
 * 	ahead and try to contact the daemon again in any case.
 *
 * Side effects:
 *	A new process is spawned and it tries to invoke migd.
 *
 *----------------------------------------------------------------------
 */

static int
StartMigd()
{
    int pid;
    char *argArray[4];
    
    pid = fork();
    if (pid < 0) {
	fprintf(stderr, "couldn't fork\n");
	return(-1);
    }
    if (pid > 0) {
	/*
	 * We use the sprite Proc_Wait because we don't want to
	 * find out about any other children.
	 */
	ReturnStatus status =
	    Proc_Wait(1, &pid, PROC_WAIT_BLOCK, (Proc_PID *) NULL,
		      (int *) NULL, (int *) NULL, (int *) NULL,
		      (Proc_ResUsage *) NULL);
	if (status != SUCCESS) {
	    fprintf(stderr, "Error waiting for child to start migd: %s.",
		   Stat_GetMsg(status));
	    return(-1);
	}
	return(0);
    }

    /*
     * We are the child, and will try to become the migration daemon.
     * First, sleep for just a moment so that we don't exit before our
     * parent has waited for us... we don't want a user's signal handler to
     * find out about us.  Then try to invoke migd.
     */
    sleep(1);
    (void) system("/sprite/daemons/migd -D 2 -L");
    exit(0);
}





/*
 *----------------------------------------------------------------------
 *
 * Mig_RequestIdleHosts --
 *
 *	Obtain one or more idle hosts from the migration server.
 *	The caller specifies the number of hosts requested, the
 *	priority at which they'll be used, flags to tell the daemon,
 *	a callback procedure if a host is reclaimed or more hosts are
 *	available, and an array to hold the identifiers of hosts.
 *
 * Results:
 *	On error, -1 is returned, else the number of hosts in hostIDArray
 *	is returned.  0 indicates no hosts were available.
 *
 * Side effects:
 *      If the connection to the global server has not been opened, then
 *	it is opened.  A callback is registered if one is specified.
 *	If the local daemon isn't running, it is started.
 *
 *----------------------------------------------------------------------
 */
int
Mig_RequestIdleHosts(numHosts, priority, flags, callBackPtr, hostArray)
    int numHosts;		/* Number of hosts requested. */
    int priority;		/* Priority of tasks; see mig.h */
    int flags;			/* Flags for mig daemon; ditto. */
    void (*callBackPtr)();	/* Procedure to call when getting
				 * messages from mig daemon. */
    int hostArray[];		/* Array of integers to fill with hostIDs. */
{
    Mig_IdleRequest request;
    int virtualHost;
    int physicalHost;
    char *buffer;	 	/* Dynamically-allocated buffer for result
				   of ioctl. */
    unsigned int bufSize;	/* Size of buffer. */
    int *intPtr; 		/* Pointer into buffer. */
    int i;			/* Counter. */
    int status;			/* Status of system calls. */
    int retries;		/* Count of retries if error during ioctl. */
    int numWanted;		/* Number of hosts we wanted, set to
				   numHosts. */
    static MigdState migdState
	= MIGD_OKAY; 		/* We think migd is doing fine. */


#ifdef DEBUG
    fprintf(stderr, "Mig_RequestIdleHosts called.\n");
#endif /* DEBUG */
    if (mig_GlobalPdev < 0) {
	if (MigOpenPdev(TRUE) < 0) {
#ifdef DEBUG
	    fprintf(stderr, "Mig_RequestIdleHosts encountered error contacting global daemon.\n");
#endif /* DEBUG */
	    return(-1);
	}
    }


    if (!migGetNewHosts && !Mig_ConfirmIdle(0)) {
#ifdef DEBUG
	fprintf(stderr, "Mig_RequestIdleHosts -- no new hosts available.\n");
#endif /* DEBUG */
	return(0);
    }
    /*
     * Tell the daemon what our physical host is so it doesn't try to tell
     * us to migrate to this host.
     */
    status = Proc_GetHostIDs(&virtualHost, &physicalHost);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    }

    request.numHosts = numHosts;
    request.priority = priority;
    request.flags = flags;
    request.virtHost = virtualHost;

    bufSize = sizeof(int) * (1 + numHosts);
    buffer = malloc(bufSize);
    if (buffer == (char *) NULL) {
	errno = ENOMEM;
	return(-1);
    }
    for (retries = 2; retries >= 0; retries--) {
#ifdef DEBUG
	fprintf(stderr, "Mig_RequestIdleHosts starting ioctl.\n");
#endif				/* DEBUG */
	if (MigSetAlarm() < 0) {
	    fprintf(stderr,
		    "Error setting alarm for contact with migd.\n");
	    return(-1);
	}
	status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_GETIDLE,
			      sizeof(Mig_IdleRequest),
			      (char *) &request,
			      bufSize, buffer);
	if (MigClearAlarm() < 0) {
	    fprintf(stderr,
		    "Error clearing alarm for contact with migd.\n");
	}
#ifdef DEBUG
	fprintf(stderr, "Mig_RequestIdleHosts ioctl returned %x.\n", status);
#endif				/* DEBUG */
	if (status != SUCCESS) {
	    if (status == NET_NOT_CONNECTED) {
		if (migdState == MIGD_OKAY) {
		    fprintf(stderr,
			    "No migd daemon running on your host.  Waiting to see if it starts.\n");
		    migdState = MIGD_WAITING;
		    sleep(10);

		} else if (migdState == MIGD_WAITING) {
		    fprintf(stderr,
			    "Starting a new migd.\n");
		    migdState = MIGD_ERROR;
		    if (StartMigd() < 0) {
			return(0);
		    }
		    sleep(5);
		}
	    }
	    close(mig_GlobalPdev);
	    mig_GlobalPdev = 0;
	    if (retries == 0 || MigOpenPdev(TRUE) < 0) {
		if (migdState != MIGD_ERROR) {
		    fprintf(stderr,
			    "Mig_RequestIdleHosts: error during ioctl to global master: %s\n",
			    Stat_GetMsg(status));
		    migdState = MIGD_ERROR;
		}
		errno = Compat_MapCode(status);
		free(buffer);
		return(-1);
	    }
	} else {
	    break;
	}
    }

    migdState == MIGD_OKAY;
    intPtr = (int *) buffer;
    numWanted = numHosts;
    numHosts = *intPtr;
#ifdef DEBUG_REQUEST
    fprintf(stderr, "numHosts = %d\n", numHosts);
    fflush(stderr);
#endif /* DEBUG_REQUEST */

    intPtr++;

    for (i = 0; i < numHosts; i++) {
	hostArray[i] = *intPtr;
	(void) MigHostCache(*intPtr, MIG_CACHE_ADD, FALSE);
#ifdef DEBUG
	fprintf(stderr, "hostArray[%d] = %d\n", i, *intPtr);
	fflush(stderr);
#endif /* DEBUG */
	intPtr++;
    }

    free(buffer);
    if (numHosts < numWanted) {
#ifdef DEBUG
	fprintf(stderr, "Mig_RequestIdleHosts didn't get enough hosts.\n");
#endif /* DEBUG */
	migGetNewHosts = 0;
    }
    if (callBackPtr != NULL) {
	migCallBackPtr = callBackPtr;
    }
#ifdef DEBUG
    fprintf(stderr, "Mig_RequestIdleHosts returning %d hosts.\n", numHosts);
#endif /* DEBUG */
    return(numHosts);
    
}
