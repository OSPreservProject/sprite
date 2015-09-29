/* 
 * Mig_Done.c --
 *
 *	Source code for the Mig_Done procedure, which is a backward-compatible
 *	interface to the migration daemon.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_Done.c,v 2.1 90/06/22 14:58:19 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <mig.h>
#include <host.h>
#include <errno.h>
#include <stdio.h>

extern int errno;
extern char *strerror();
extern char *malloc();


/*
 *----------------------------------------------------------------------
 *
 * Mig_Done --
 *
 *	Record that a migrated process has finished running on a particular
 *	host.
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
Mig_Done(hostID)
    int hostID;			/* Host to return. */
{
    int hostArray[1];

    hostArray[0] = hostID;
    
    return(Mig_ReturnHosts(1, hostArray));
}


