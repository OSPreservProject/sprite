/* 
 * fputc.c --
 *
 *	Source code for the "fputc" library procedure.
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
static char rcsid[] = "$Header: fputc.c,v 1.2 88/07/21 10:49:10 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * fputc --
 *
 *	This procedure outputs a character onto a stream.  It is a
 *	procedural version of the putc macro, and also gets
 *	called by putc when the output buffer has filled.
 *
 * Results:
 *	The return value is EOF if an error occurred while writing
 *	to the stream, or if the stream isn't writable.  Otherwise
 *	it's the value of the character written.
 *
 * Side effects:
 *	Characters are buffered up for stream.
 *
 *----------------------------------------------------------------------
 */

int
fputc(c, stream)
    char c;				/* Character to output. */
    register FILE *stream;		/* Stream on which to output. */
{
    if ((stream->status != 0) || !(stream->flags & STDIO_WRITE)) {
	return EOF;
    }

    /*
     * This is tricky because of two things:
     *    a) The stream could be used both for reading and writing.  If
     *       the last access was a read access, or if the stream has never
     *	     been used for writing, "turn the stream around" before doing
     *	     the write. 
     *    b) The stream may be unbuffered (want to output each character
     *       as it comes).  To handle this, call the writeProc as soon
     *       as the buffer fills, rather than delaying until a character
     *	     arrives that doesn't fit.
     *	  c) Keep the notion of "writeCount" separate from the notion of
     *	     "all buffer space in use".  That way, the stream's I/O mgr
     *	     can arrange for itself to be called anytime it wants (even if
     *	     the buffer isn't full) just by making writeCount 1.
     */

    if (stream->writeCount == 0) {
	stream->readCount = 0;
	stream->lastAccess = stream->buffer - 1;
    }

    stream->writeCount--;
    stream->lastAccess++;
    *(stream->lastAccess) = c;
    if ((c == '\n') && (stream->flags & STDIO_LINEBUF)) {
	(*stream->writeProc)(stream, 1);
    } else if (stream->writeCount <= 0) {
	(*stream->writeProc)(stream, 0);
    }
    if (stream->status != 0) {
	return EOF;
    }
    return (unsigned char) c;
}
