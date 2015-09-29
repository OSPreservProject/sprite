/* 
 * Host_End.c --
 *
 *	Source code for the Host_End library procedure.
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
static char rcsid[] = "$Header: Host_End.c,v 1.2 88/07/25 13:31:51 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <host.h>
#include <hostInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Host_End --
 *
 *	Finish using the current host database.
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
Host_End()
{
    if (hostFile != (FILE *) NULL) {
	fclose(hostFile);
	hostFile = (FILE *) NULL;
    }
}
