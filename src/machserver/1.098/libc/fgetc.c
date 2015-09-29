/* 
 * fgetc.c --
 *
 *	Source code for the "fgetc" library procedure.
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
static char rcsid[] = "$Header: fgetc.c,v 1.1 88/06/10 16:23:43 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * fgetc --
 *
 *	This procedure returns the next input character from a stream.
 *	It's a procedural version of the getc macro, and also
 *	gets invoked by getc when the buffer needs to be refilled.
 *
 * Results:
 *	The result is an integer value that is equal to EOF if an end
 *	of file or error condition was encountered on the stream.
 *	Otherwise, it is the value of the next input character from
 *	stream.
 *
 * Side effects:
 *	A character is removed from stream.
 *
 *----------------------------------------------------------------------
 */

int
fgetc(stream)
    register FILE *stream;	/* Stream from which to read character. */
{
    if (!(stream->flags & STDIO_READ)) {
	return(EOF);
    }
    while (stream->readCount <= 0) {
	if ((stream->status != 0) || (stream->flags & STDIO_EOF)) {
	    return(EOF);
	}

	/*
	 * If the stream has been getting used for writing lately,
	 * "turn it around" by flushing the write data.  Then read
	 * in a buffer-full of read data.
	 */

	if ((stream->writeCount > 0)
		&& (stream->lastAccess >= stream->buffer)) {
	    (*stream->writeProc)(stream, 1);
	    stream->writeCount = 0;
	}
	(*stream->readProc)(stream);
    }
    stream->readCount--;
    stream->lastAccess++;
    return *stream->lastAccess;
}
