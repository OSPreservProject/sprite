/* 
 * Gate_End.c --
 *
 *	Source code for the Gate_End library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/gate/RCS/Gate_End.c,v 1.1 92/06/04 22:03:21 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <gate.h>
#include <gateInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Gate_End --
 *
 *	Finish using the current gateway database.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The file open to the database is closed.
 *
 *-----------------------------------------------------------------------
 */

void
Gate_End()
{
    if (gateFile != (FILE *) NULL) {
	fclose(gateFile);
	gateFile = (FILE *) NULL;
    }
}
