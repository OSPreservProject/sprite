/* 
 * Host_SetFile.c --
 *
 *	Source code for the Host_SetFile library procedure.
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
static char rcsid[] = "$Header: Host_SetFile.c,v 1.1 88/06/30 11:06:48 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <host.h>
#include <hostInt.h>
#include <stdlib.h>
#include <string.h>


/*
 *-----------------------------------------------------------------------
 *
 * Host_SetFile --
 *
 *	Change the file being used to fetch info.
 *
 * Results:
 *	Zero is returned if all went well.  If an error occurred,
 *	then -1 is returned and errno tells exactly what went wrong.
 *
 * Side Effects:
 *	The old file is closed. A Host_Start need not be given as the
 *	file will be open already.
 *
 *-----------------------------------------------------------------------
 */

int
Host_SetFile(fileName)
    char    	  *fileName;	/* File to use as the database */
{
    if (hostFile != (FILE *) NULL) {
	fclose(hostFile);
    }
    hostFile = fopen(fileName, "r");
    if (hostFile == (FILE *) NULL) {
	return -1;
    } else {
	hostFileName = malloc((unsigned) (strlen(fileName) + 1));
	strcpy(hostFileName, fileName);
    }
    return 0;
}
