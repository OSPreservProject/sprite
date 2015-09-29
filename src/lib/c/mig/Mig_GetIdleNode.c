/* 
 * Mig_GetIdleNode.c --
 *
 *	Source code for the Mig_GetIdleNode procedure.
 *	This procedure returns an idle host that may be
 *	used for migration.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_GetIdleNode.c,v 2.0 90/03/10 13:12:48 douglis Stable $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <mig.h>



/*
 *----------------------------------------------------------------------
 *
 * Mig_GetIdleNode --
 *
 *	Obtain an idle node from the migration server.
 *	This is a backward-compatible interface to the new
 *	migration library.
 *
 * Results:
 *	On error, -1 is returned.  If no host is available, 0 is returned.
 *	If successful, the ID of an idle host is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Mig_GetIdleNode()
{
    int hostsAssigned;
    int hostNumbers[1];

    hostsAssigned = Mig_RequestIdleHosts(1, MIG_NORMAL_PRIORITY, 0,
					 (void (*)()) NULL, hostNumbers);
    if (hostsAssigned <= 0) {
	return(hostsAssigned);
    } 
    return(hostNumbers[0]);
}
