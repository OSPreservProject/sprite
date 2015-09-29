/* 
 * ungetc.c --
 *
 *	Source code for the "ungetc" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/ungetc.c,v 1.3 88/12/29 01:02:47 rab Exp Locker: mendel $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * ungetc --
 *
 *	This procedure adds a single character back to the front of a
 *	stream, so that it will be the next character read from the
 *	stream.  This procedure is only guaranteed to work once in a
 *	row for any stream until getc is called again.  Successive
 *	calls may eventually back up to the beginning of the stream's
 *	buffer, in which case future attempts to put characters back will
 *	be ignored.
 *
 * Results:
 *	EOF is returned if there was insufficient space left in the
 *	buffer to push c back.  Otherwise c is returned.
 *
 * Side effects:
 *	The stream is modified so that the next getc call for the
 *	stream will return c.
 *
 *----------------------------------------------------------------------
 */

int
ungetc(c, stream)
    int c;				/* Character to put back. */
    register FILE *stream;		/* Stream in which to put c back. */
{
    if ((stream->writeCount > 0) ||
	    !(stream->flags & STDIO_READ) || (c == EOF) ||
	    (stream->status != 0)) {
	return EOF;
    }
    if (stream->lastAccess < stream->buffer) {
	if (stream->readCount != 0) {
	    return EOF;
	}
	stream->lastAccess = stream->buffer + stream->bufSize - 1;
    }
    if (stream->lastAccess < stream->buffer) {
	stream->lastAccess = stream->buffer + stream->bufSize - 1;
	stream->readCount = 0;
    }

    /*
     * Special case:  don't overwrite the character if it's already
     * got the correct value.  This is needed during sscanf, if the
     * string being scanned is read-only:  sscanf calls ungetc, but
     * always puts back the same value that was already there.
     */

    if (*(stream->lastAccess) != c) {
	*(stream->lastAccess) = c;
    }
    stream->lastAccess--;
    stream->readCount++;
    stream->flags &= ~STDIO_EOF;
    return c;
}
