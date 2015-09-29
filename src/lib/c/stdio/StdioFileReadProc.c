/* 
 * StdioFileReadProc.c --
 *
 *	Source code for the "StdioFileReadProc" procedure, which is used
 *	internally by the stdio library to flush buffers for streams
 *	associated with files and other OS-supported streams.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/StdioFileReadProc.c,v 1.5 90/09/11 14:27:19 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"
#include "stdlib.h"
#include <errno.h>
#include <unistd.h>

/*
 *----------------------------------------------------------------------
 *
 * StdioFileReadProc --
 *
 *	This procedure is invoked when another character is needed
 *	from a stream and the buffer is empty.  It's used for all
 *	streams that are associated with files (or pipes, or anything
 *	for which the file-related system calls apply).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the stream is readable, a bunch of new bytes are read into
 *	stream's buffer and readCount is set to indicate how many
 *	bytes there are.  The error and end-of-file fields in stream
 *	are set if any problems occur.
 *
 *	If the stream is stdin, then any line-buffered file streams
 *	are flushed.
 *
 *----------------------------------------------------------------------
 */

void
StdioFileReadProc(stream)
    register FILE *stream;	/* Stream whose buffer needs filling.  The
				 * stream must be readable.  The clientData
				 * field of stream gives a Sprite stream index.
				 */
{
    register FILE *	stream2;

    /*
     * Before doing any input from stdin, flush line-buffered
     * streams.
     */
    
    if (stream == stdin) {
	for (stream2 = stdioFileStreams; stream2 != NULL;
		stream2 = stream2->nextPtr) {
	    if (stream2->flags & STDIO_LINEBUF) {
		fflush(stream2);
	    }
	}
    }

    /*
     * Create buffer space for this stream if there isn't any yet.
     */

    if (stream->bufSize == 0) {
	stream->bufSize = BUFSIZ;
	stream->buffer = (unsigned char *) malloc((unsigned) stream->bufSize);
    }

    stream->readCount = read((int) stream->clientData, (char *) stream->buffer,
	    stream->bufSize);
    stream->lastAccess = stream->buffer - 1;
    if (stream->readCount <= 0) {
	if (stream->readCount == 0) {
	    stream->flags |= STDIO_EOF;
	} else {
	    stream->readCount = 0;
	    stream->status = errno;
	}
    }
}
