/* 
 * fflush.c --
 *
 *	Source code for the "fflush" library procedure.
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
static char rcsid[] = "$Header: fflush.c,v 1.2 88/07/25 08:54:16 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * fflush --
 *
 *	Forces any buffered output data on stream to be output.
 *
 * Results:
 *	EOF is returned if the stream isn't writable of if an error
 *	occurred while writing it.
 *
 * Side effects:
 *	If there is buffered data, it is written out.
 *
 *----------------------------------------------------------------------
 */

int
fflush(stream)
    register FILE *stream;		/* Stream to be flushed. */
{
    if (!(stream->flags & STDIO_WRITE) || (stream->writeCount == 0)
	    || (stream->lastAccess == (stream->buffer-1))) {
	return 0;
    }
    if (stream->status != 0) {
	return EOF;
    }
    (*stream->writeProc)(stream, 1);
    if (stream->status != 0) {
	return EOF;
    }
    return 0;
}
