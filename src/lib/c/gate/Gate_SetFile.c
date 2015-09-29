/* 
 * Gate_SetFile.c --
 *
 *	Source code for the Gate_SetFile library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/gate/RCS/Gate_SetFile.c,v 1.1 92/06/04 22:03:22 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <gate.h>
#include <gateInt.h>
#include <stdlib.h>
#include <string.h>

/*
 *-----------------------------------------------------------------------
 *
 * Gate_SetFile --
 *
 *	Change the file being used to fetch info.
 *
 * Results:
 *	Zero is returned if all went well.  If an error occurred,
 *	then -1 is returned and errno tells exactly what went wrong.
 *
 * Side Effects:
 *	The old file is closed. A Gate_Start need not be given as the
 *	file will be open already.
 *
 *-----------------------------------------------------------------------
 */

int
Gate_SetFile(fileName)
    char    	  *fileName;	/* File to use as the database */
{
    if (gateFile != (FILE *) NULL) {
	fclose(gateFile);
    }
    gateFile = fopen(fileName, "r");
    if (gateFile == (FILE *) NULL) {
	return -1;
    } else {
	gateFileName = malloc((unsigned) (strlen(fileName) + 1));
	strcpy(gateFileName, fileName);
    }
    return 0;
}
