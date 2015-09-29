/* 
 * setvbuf.c --
 *
 *	Source code for the "setvbuf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/setvbuf.c,v 1.4 89/06/19 14:15:14 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include "fileInt.h"

/*
 *----------------------------------------------------------------------
 *
 * setvbuf --
 *
 *	This procedure may be used to amount of buffering used for
 *	a stdio stream.  A buffer size of 0 or 1, or a mode of _IONBF,
 *	means the stream is unbuffered: each byte of I/O results in a
 *	system call.  Needless to say, this is likely to be inefficient.
 *	A buffer size of N means that N characters are read from the
 *	system at a time, regardless of how many are actually needed,
 *	and on writes, nothing is written to the system until N bytes
 *	have accumulated, at which point all are written in one operation.
 *
 * Results:
 *	Returns 0 on success, -1 if there was an error in the arguments
 *	or if buffer space couldn't be setup properly.
 *
 * Side effects:
 *	The stream is modified to use size bytes of buffering.
 *	Old buffer space is returned to the storage allocator, if it
 *	was allocated by stdio rather than the client.
 *
 *----------------------------------------------------------------------
 */

int
setvbuf(stream, buf, mode, size)
    register FILE *stream;	/* Stream whose buffering is to be changed. */
    char *buf;			/* Buffer to use for stream. */
    int mode;			/* Mode for buffering:  _IOFBF, _IOLBF, or
				 * _IONBF. */
    int size;			/* Number of characters of space in buffer;
				 * 0 or 1 means unbuffered. */
{
    int result;

    if (stream->readProc != (void (*)()) StdioFileReadProc) {
	return -1;
    }

    result = fflush(stream);

    /*
     * Careful!  Don't free the statically-allocated buffers for
     * standard streams.
     */

    if ((stream->buffer != stdioTempBuffer)
	    && (stream->buffer != stdioStderrBuffer)
	    && (stream->buffer != NULL)
	    && !(stream->flags & STDIO_NOT_OUR_BUF)) {
	free((char *) stream->buffer);
    }
    stream->flags &= ~(STDIO_LINEBUF|STDIO_NOT_OUR_BUF);

    if (size < 1) {
	size = 1;
    }
    if (mode == _IONBF) {
	size = 1;
    } else if (mode == _IOLBF) {
	stream->flags |= STDIO_LINEBUF;
    } else if (mode != _IOFBF) {
	return -1;
    }

    if (buf != 0) {
	stream->buffer = (unsigned char *) buf;
	stream->flags |= STDIO_NOT_OUR_BUF;
    } else {
	stream->buffer = (unsigned char *) malloc((unsigned) size);
    }
    stream->bufSize = size;
    stream->lastAccess = stream->buffer - 1;
    stream->readCount = 0;
    stream->writeCount = 0;
    return result;
}
