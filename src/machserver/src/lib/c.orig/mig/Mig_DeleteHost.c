/* 
 * Mig_DeleteHost.c --
 *
 *	Delete information about a host from the load average data base.
 *	This is done by performing an ioctl to the global server with
 *	the sprite ID of the host to be deleted.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_DeleteHost.c,v 2.1 90/06/22 14:58:17 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <stdio.h>
#include <mig.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * Mig_DeleteHost --
 *
 *	Tell the global daemon to remove a host from its records, so
 *	the host won't be listed as being down.  This is useful if
 *	a host is renamed or otherwise removed. 
 *
 * Results:
 *	0 if successful, or -1 on error, with errno indicating the error.
 *
 * Side effects:
 *	Does ioctl to server.
 *
 *----------------------------------------------------------------------
 */
int
Mig_DeleteHost(hostID)
    int hostID;			/* ID of host to remove. */
{
    int status;
    
    if (mig_GlobalPdev < 0) {
	if (MigOpenPdev(TRUE) < 0) {
	    return(-1);
	}
    }
	    
    if (MigSetAlarm() < 0) {
	fprintf(stderr,
		"Error setting alarm for contact with migd.\n");
	return(-1);
    }
    status = Fs_IOControl(mig_GlobalPdev, IOC_MIG_KILL,
				  sizeof(int), (char *) &hostID, 0,
				  (char *) NULL);
    if (MigClearAlarm() < 0) {
	fprintf(stderr,
		"Error clearing alarm for contact with migd.\n");
    }
    if (status != SUCCESS) {
	fprintf(stderr,
	       "Mig_DeleteHost: error during ioctl to global master: %s\n",
	       Stat_GetMsg(status));
	errno = Compat_MapCode(status);
	return(-1);
    }
    return(0);
}
