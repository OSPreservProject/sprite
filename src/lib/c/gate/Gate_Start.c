/* 
 * Gate_Start.c --
 *
 *	Source code for the Gate_Start library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/gate/RCS/Gate_Start.c,v 1.1 92/06/04 22:03:23 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <gate.h>

/*
 * Information about the current gateway database file:
 */

FILE *		gateFile = (FILE *) NULL;
char *		gateFileName = "/etc/gateway";

/*
 *-----------------------------------------------------------------------
 *
 * Gate_Start --
 *
 *	Begin reading from the the current gateway file.
 *
 * Results:
 *	0 is returned if all went well.  Otherwise -1 is returned
 *	and errno tells what went wrong.
 *
 * Side Effects:
 *	If the file was open, it is reset to the beginning. If it was not
 *	open, it is now.
 *
 *-----------------------------------------------------------------------
 */

int
Gate_Start()
{
    if (gateFile != (FILE *) NULL) {
	rewind(gateFile);
    } else {
	gateFile = fopen(gateFileName, "r");
	if (gateFile == (FILE *) NULL) {
	    return -1;
	}
    }
    return 0;
}
