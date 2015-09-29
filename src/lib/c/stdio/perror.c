/* 
 * perror.c --
 *
 *	Source code for the "perror" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/perror.c,v 1.2 88/07/25 13:12:59 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * perror --
 *
 *	Print a message describing the current error condition (as
 *	given by errno), with a given introductory message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff gets printed.
 *
 *----------------------------------------------------------------------
 */

void
perror(msg)
    char *msg;			/* Message to print before the message
				 * describing the error. */
{
    if ((msg != 0) && (*msg != 0)) {
	fprintf(stderr, "%s: ", msg);
    }
    fprintf(stderr, "%s\n", strerror(errno));
}
