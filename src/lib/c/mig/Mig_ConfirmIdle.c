/* 
 * Mig_ConfirmIdle.c --
 *
 *	Confirm that a host is available.
 *
 * Copyright 1988, 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_ConfirmIdle.c,v 2.3 90/09/24 14:46:46 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */


#include <stdio.h>
#include <sprite.h>
#include <status.h>
#include <bit.h>
#include <mig.h>
#include <kernel/net.h>
#include <sys/time.h>
#include <sysStats.h>
#include <host.h>
#include <errno.h>
#include "migInt.h"

/* 
 * Keep track of whether we should get new hosts, or whether we know none
 * are available.  Defaults to asking, of course.  Set to 0 if too few
 * hosts are available.
 */
int migGetNewHosts = 1;

/*
 *----------------------------------------------------------------------
 *
 * Mig_ConfirmIdle --
 *
 *	Double-check that a host is still accepting migration.  Otherwise
 *	we could keep migrating to a host that is no longer idle.  We check
 *	to see if the stream is selectable, in which case we get
 *	any updates to host availability and modify our cache, and in any
 *	case we then check the host cache.  The same logic is used to
 *	check for msgs indicating a host is now available, in which
 * 	case a hostID  of 0 is used.
 *
 * Results:
 *	TRUE if the host is still available (or a new host is available,
 *	if hostID == 0), FALSE otherwise or if an error occurs.
 *
 * Side effects:
 *	May do ioctl to server.
 *
 *----------------------------------------------------------------------
 */
Boolean
Mig_ConfirmIdle(hostID)
    int hostID;			/* ID of host to confirm availability of */
{
    static int *bitArray = NULL;
    static int bitSize = 0;
    int numReady;
    int status;
    int msgHostID;
    struct timeval time;
    int retries = 0;
    
#ifdef DEBUG
    fprintf(stderr, "Mig_ConfirmIdle(%d) called.\n", hostID);
#endif /* DEBUG */

    if (mig_GlobalPdev < 0) {
#ifdef DEBUG
	fprintf(stderr, "Mig_ConfirmIdle: no pdev connection.\n");
#endif /* DEBUG */
	return(FALSE);
    }
    if (bitSize <= mig_GlobalPdev) {
	bitArray = Bit_Expand(mig_GlobalPdev + 1, bitSize, bitArray);
	bitSize = mig_GlobalPdev + 1;
    }
	    
    while (1) {
	Bit_Set(mig_GlobalPdev, bitArray);
	time.tv_sec = 0;
	time.tv_usec = 0;
#ifdef DEBUG
    fprintf(stderr, "Mig_ConfirmIdle: calling select.\n");
#endif /* DEBUG */
	numReady = select(bitSize, bitArray, (int *) NULL, (int *) NULL,
		 &time);
#ifdef DEBUG
    fprintf(stderr, "Mig_ConfirmIdle: select returned %d.\n", numReady);
#endif /* DEBUG */
	if (numReady <= 0) {
	    break;
	} else {
#ifdef DEBUG
	    fprintf(stderr, "Mig_ConfirmIdle: calling Fs_IOControl.\n");
#endif /* DEBUG */
	    if (MigSetAlarm() < 0) {
		fprintf(stderr,
			"Error setting alarm for contact with migd.\n");
		return(FALSE);
	    }
	    status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_GET_UPDATE,
				  0, (char *) NULL, sizeof(int),
				  (char *) &msgHostID);
	    if (MigClearAlarm() < 0) {
		fprintf(stderr,
			"Error clearing alarm for contact with migd.\n");
	    }
		
		
#ifdef DEBUG
	    fprintf(stderr, "Mig_ConfirmIdle: Fs_IOControl returned %x.\n",
		   status);
#endif /* DEBUG */
	    if (status != SUCCESS) {
		fprintf(stderr,
		       "Mig_ConfirmIdle: error during ioctl to global master: %s\n",
		       Stat_GetMsg(status));
		if (status & 0xf0000 || status == GEN_ABORTED_BY_SIGNAL) {
		    /*
		     * fs/dev/... error, or timeout,
		     * rather than FAILURE or INVALID_ARG.
		     */
		    close(mig_GlobalPdev);
		    mig_GlobalPdev = 0;
		    if (retries > 0 || MigOpenPdev(TRUE) < 0) {
			return(FALSE);
		    }
		    retries = 1;
		} else {
		    return(FALSE);
		}
		continue;
	    }
	    if (msgHostID == 0) {
#ifdef DEBUG
		fprintf(stderr, "Mig_ConfirmIdle: new host(s) available.\n");
#endif /* DEBUG */
		migGetNewHosts = 1;
		if (migCallBackPtr != NULL) {
		    (*migCallBackPtr)(msgHostID);
		}
	    } else 
#ifdef DEBUG
		fprintf(stderr, "Mig_ConfirmIdle: host %d unavailable.\n",
		       msgHostID);
#endif /* DEBUG */
		(void) MigHostCache(msgHostID, MIG_CACHE_REMOVE, TRUE);
	}
    }
    if (hostID == 0) {
#ifdef DEBUG
	fprintf(stderr, "Mig_ConfirmIdle: returning %d.\n", migGetNewHosts);
#endif /* DEBUG */
	return(migGetNewHosts);
    }
#ifdef DEBUG
    fprintf(stderr, "Mig_ConfirmIdle: returning %d.\n",
	   MigHostCache(hostID, MIG_CACHE_VERIFY, FALSE));
#endif /* DEBUG */
    return(MigHostCache(hostID, MIG_CACHE_VERIFY, FALSE));
}
