/* 
 * fopen.c --
 *
 *	Source code for the "fopen" library procedure.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdio/RCS/fopen.c,v 1.2 91/12/12 22:10:13 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "fileInt.h"
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/file.h>

extern long lseek();

/*
 *----------------------------------------------------------------------
 *
 * fopen --
 *
 *	Open a file and associate a buffered stream with the open file.
 *
 * Results:
 *	The return value is a stream that may be used to access
 *	the file, or NULL if an error occurred in opening the file.
 *
 * Side effects:
 *	A file is opened, and a stream is initialized.
 *
 *----------------------------------------------------------------------
 */

FILE *
fopen(fileName, access)
    char *fileName;		/* Name of file to be opened. */
    char *access;		/* Indicates type of access:  "r" for reading,
				 * "w" for writing, "a" for appending, "r+"
				 * for reading and writing, "w+" for reading
				 * and writing with initial truncation, "a+"
				 * for reading and writing with initial
				 * position at the end of the file.  The
				 * letter "b" may also appear in the string,
				 * for ANSI compatibility, but only after
				 * the first letter.  It is ignored. */
{
    int 	streamID, flags;

    flags = StdioFileOpenMode(access);
    if (flags == -1) {
	return (FILE *) NULL;
    }

    streamID = open(fileName, flags, 0666);
    if (streamID < 0) {
	return (FILE *) NULL;
    }
    if (access[0] == 'a') {
	(void) lseek(streamID, 0L, L_XTND);
    }

    /*
     * Initialize the stream structure.
     */

    return fdopen(streamID, access);
}
