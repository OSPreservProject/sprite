/* 
 * StdioFileCloseProc.c --
 *
 *	Source code for the "StdioFileCloseProc" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/StdioFileCloseProc.c,v 1.5 90/09/11 14:27:18 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

/*
 *----------------------------------------------------------------------
 *
 * StdioFileCloseProc --
 *
 *	This procedure is used as the closeProc for all streams
 *	that are associated with files.  It just closes the file
 *	associated with the stream.
 *
 * Results:
 *	Returns 0 if all went well, or EOF if there was an error during
 *	the close operation.
 *
 * Side effects:
 *	A file is closed.
 *
 *----------------------------------------------------------------------
 */

int
StdioFileCloseProc(stream)
    register FILE *stream;	/* Stream to be closed.  The clientData
				 * field of the stream contains the id to
				 * use when talking to the operating system
				 * about the stream. */
{
    register FILE *prev;

    /*
     * Careful!  Don't free the buffer unless we allocated it.
     */

    if ((stream->buffer != stdioTempBuffer)
	    && (stream->buffer != stdioStderrBuffer)
	    && (stream->buffer != NULL)
	    && !(stream->flags & STDIO_NOT_OUR_BUF)) {
	free((char *) stream->buffer);
	stream->buffer = NULL;
	stream->bufSize = 0;
    }
    stream->flags = 0;
    stream->readCount = stream->writeCount = 0;
    if (close((int) stream->clientData) != 0) {
	stream->status = errno;
	return EOF;
    }

    /*
     * Free the stream's struct unless it's one of the ones statically
     * allocated for a standard channel.
     */

    if ((stream != stdin) && (stream != stdout) && (stream != stderr)) {
	if (stdioFileStreams == stream) {
	    stdioFileStreams = stream->nextPtr;
	} else {
	    for (prev = stdioFileStreams; prev != NULL;
		    prev = prev->nextPtr) {
		if (prev->nextPtr == stream) {
		    prev->nextPtr = stream->nextPtr;
		    break;
		}
	    }
	}
	free((char *) stream);
    }
    return 0;
}
