/* 
 * Mig_Evict.c --
 *
 *	Force the daemon on the local host to evict foreign processes.
 *	This might be useful if one is remotely logged into a host
 *	and wishes to evict processes, since automatic migration is done
 *	when the user is physically present to interact with the host.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_Evict.c,v 2.1 90/06/22 14:58:20 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <stdio.h>
#include <mig.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * Mig_Evict --
 *
 *	Tell the local daemon to evict foreign processes.
 *
 * Results:
 *	The number of processes evicted is returned if the call is successful,
 *	or -1 is returned on error, with errno indicating the error.
 *
 * Side effects:
 *	Does ioctl to server.
 *
 *----------------------------------------------------------------------
 */
int
Mig_Evict()
{
    int status;
    int numHosts;
    
    if (mig_LocalPdev < 0) {
	if (MigOpenPdev(FALSE) < 0) {
	    return(-1);
	}
    }
	    
    if (MigSetAlarm() < 0) {
	fprintf(stderr,
		"Error setting alarm for contact with migd.\n");
	return(-1);
    }
    status = Fs_IOControl(mig_LocalPdev, IOC_MIG_EVICT,
			  0, (char *) NULL,
			  sizeof(int), (char *) &numHosts);
    if (MigClearAlarm() < 0) {
	fprintf(stderr,
		"Error clearing alarm for contact with migd.\n");
    }
    if (status != SUCCESS) {
	fprintf(stderr,
	       "Mig_Evict: error during ioctl to local daemon: %s\n",
	       Stat_GetMsg(status));
	errno = Compat_MapCode(status);
	return(-1);
    }
    return(numHosts);
}
