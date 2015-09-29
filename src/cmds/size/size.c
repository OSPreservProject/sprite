/*
 * size.c --
 *
 *	This file contains the "size" command, which prints out
 *	the sizes of the segments of one or more object files.
 *
 *	Adding a new machine type: Write a routine that prints out the
 *	desired information if it recognizes the exec header. Look
 *	at one of the existing routines for the parameter specifications. 
 *      This routine should return SUCCESS if it printed out the size, 
 *	and FAILURE otherwise. Add the name of this routine to the 
 *	printProc array.
 *
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
static char rcsid[] = "$Header: /sprite/src/cmds/size/RCS/size.c,v 1.7 90/02/16 13:46:45 rab Exp $";
#endif /* not lint */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include "size.h"

/*
 * Array of routines to print out size.
 */
ReturnStatus (*(printProc[])) () = {
    Print68k,   /* This should be `PrintSun' since it works for sparc too. */
    PrintSpur,
    PrintMips,
};

#define MACHINECOUNT (sizeof (printProc) / sizeof(*printProc))

int hostFmt = HOST_FMT;

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "size".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints info on standard output.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    int 		i;
    int			j;
    Boolean 		printName;
    char		*fileName;
    int			filesToDo;
    int			targetType;
    int			lastType;
    char		headerBuffer[HEADERSIZE];
    int			amountRead;
    FILE		*fp;
    ReturnStatus	status;

    lastType = -1;
    if (argc < 2) {
	filesToDo = 1;
	fileName = "a.out";
	printName = FALSE;
    } else if (argc == 2) {
	filesToDo = 1;
	fileName = NULL;
	printName = FALSE;
    } else {
	filesToDo = argc - 1;
	fileName = NULL;
	printName = TRUE;
    }
    for (i = 0; i < filesToDo; i++,fileName = NULL) {
	if (fileName == NULL) {
	    fileName = argv[i+1];
	}
	fp = fopen(fileName, "r");
	if (fp == NULL) {
	    fprintf(stderr, "Couldn't open \"%s\": %s.\n",
		    fileName, strerror(errno));
	    exit(1);
	}
	amountRead = fread(headerBuffer, sizeof(char), HEADERSIZE, fp);
	if (amountRead < 0) {
	    fprintf(stderr, "Couldn't read header for \"%s\": %s.\n",
		    fileName, strerror(errno));
	    exit(1);
	}
	status = FAILURE;
	rewind(fp);
	for (j = 0; j < MACHINECOUNT; j++) {
	    status = printProc[j](fp, printName, fileName, 
				 (j == lastType) ? FALSE : TRUE,
				 amountRead, headerBuffer);
	    if (status == SUCCESS) {
		lastType = j;
		break;
	    }
	}
	if (status == FAILURE) {
	    fprintf(stderr, 
		  "\"%s\" isn't an object file for any known machine.\n",
		    fileName);
	}
    }
    exit(0);
}
