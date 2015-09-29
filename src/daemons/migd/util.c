/* 
 * util.c --
 *
 *	General utilities for the migration daemon.
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
static char rcsid[] = "$Header: /sprite/src/daemons/migd/RCS/util.c,v 1.2 90/02/28 10:54:46 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <migd.h>

extern int debug;


/*
 *----------------------------------------------------------------------
 *
 * Malloc --
 *
 *	Centralized location for calling malloc and checking for
 *	out of memory error.  
 *
 * Results:
 *	A pointer to the allocated memory is returned.  NULL is never
 *	returned.
 *
 * Side effects:
 *	If out of memory, we panic if debugging is enabled or exit
 *	otherwise.
 *
 *----------------------------------------------------------------------
 */

char *
Malloc(numBytes)
    int numBytes;	/* How many bytes to allocate. */
{
    char *buffer;

    buffer = malloc(numBytes);
    if (buffer == (char *) NULL) {
	SYSLOG0(LOG_ERR, "Out of memory!  Exiting.");
	if (debug) {
	    panic("Out of memory.\n");
	}
	exit(ENOMEM);
    }
    return(buffer);
}
