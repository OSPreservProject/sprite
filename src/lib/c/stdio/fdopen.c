/* 
 * fdopen.c --
 *
 *	Source code for the "fdopen" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/fdopen.c,v 1.5 88/10/01 10:18:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include "fileInt.h"

/*
 *----------------------------------------------------------------------
 *
 * fdopen --
 *
 *	This procedure initializes a FILE structure for a file (or pipe
 *	or similar OS-supplied thing) that has been opened or inherited
 *	through some other mechanism.
 *
 * Results:
 *	The return value may be used to perform buffered I/O on
 *	streamID.  If streamID isn't valid, then NULL is returned.
 *
 * Side effects:
 *	The internal list stdioFileStreams gets a new element.
 *
 * Implementation Warning:
 *	To be compatible with BSD, this procedure must permit multiple
 *	streams on the same file stream!  This seems like a very bad
 *	idea for a seekable stream, but seems to get used for network
 *	connection.
 *
 *----------------------------------------------------------------------
 */

FILE *
fdopen(streamID, access)
    int streamID;		/* Index of a stream identifier, such as
				 * returned by open.  */
    char *access;		/* Indicates the type of access to be
				 * made on streamID, just as in fopen.
				 * Must match the permissions on streamID.  */
{
    register FILE * 	stream;
    int 		read, write;

    if (streamID < 0) {
	return NULL;
    }

    /*
     * If this is for stdin, stdout, or stderr, try to use the standard
     * FILE, unless it's already in use.  Otherwise allocate a new
     * FILE structure.
     */

    if (streamID <= 2) {
	if (streamID == 0) {
	    stream = stdin;
	} else if (streamID == 1) {
	    stream = stdout;
	} else if (streamID == 2) {
	    stream = stderr;
	}
	if (stream->flags == 0) {
	    goto gotStream;
	}
    }
    stream = (FILE *) malloc(sizeof(FILE));
    stream->nextPtr = stdioFileStreams;
    stdioFileStreams = stream;

    gotStream:
    read = write = 0;
    if ((access[1] == '+') || ((access[1] == 'b') && (access[2] == '+'))) {
	read = write = 1;
    } else if (access[0]  == 'r') {
	read = 1;
    } else {
	write = 1;
    }

    /*
     * Seek to the end of the file if we're in append mode (I'm not sure
     * this should be necessary, but UNIX programs seem to depend on it).
     */

    if (access[0] == 'a') {
	(void) lseek(streamID, 0, 2);
    }

    Stdio_Setup(stream, read, write, stdioTempBuffer, 0,
	    StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
	    (ClientData) streamID);
    return(stream);
}
