/* 
 * StdioFileWriteProc.c --
 *
 *	Source code for the "StdioFileWriteProc" library procedure.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/stdio/RCS/StdioFileWriteProc.c,v 1.3 92/03/23 15:05:08 kupfer Exp $ SPRITE (Berkeley)"; 
#endif not lint

#include <stdio.h>
#include "fileInt.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

/*
 * Before the first I/O on stdin, stdout, or stderr their buffers
 * aren't initialized.  For the output streams, there must be someplace
 * to buffer the first character, temporarily, until the buffer-flush
 * routine is called.  That's what the variable below is for.
 */

unsigned char stdioTempBuffer[4];

/*
 * Stderr cannot have a dynamically allocated buffer since we may end
 * up calling malloc at at bad time (like inside of panic in the kernel).
 * Allocate a static buffer for stderr.
 */

#define STDERR_BUFSIZE	128
unsigned char stdioStderrBuffer[STDERR_BUFSIZE];

/*
 * Space is allocated here for the structures for stdin, stdout, and
 * stderr, and also for the array that holds pointers to all the
 * streams asociated with files.
 */

FILE stdioInFile = {
    0, 0, 0, 0, 0,
    StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
    (ClientData) 0, 0, STDIO_READ, NULL
};

FILE stdioOutFile = {
    stdioTempBuffer-1, 0, 0, stdioTempBuffer, 0,
    StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
    (ClientData) 1, 0, STDIO_WRITE, &stdioInFile
};

FILE stdioErrFile = {
    stdioTempBuffer-1, 0, 0, stdioTempBuffer, 0,
    StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
    (ClientData) 2, 0, STDIO_WRITE, &stdioOutFile
};

FILE *stdioFileStreams = &stdioErrFile;


/*
 *----------------------------------------------------------------------
 *
 * StdioFileWriteProc --
 *
 *	This procedure is invoked when the last character of space
 *	in a stream's buffer is filled.  Its job is to write out the
 *	contents of the buffer to the file system.  This procedure is
 *	used for all streams that are associated with files (or pipes,
 *	or anything for which the file-related system calls apply).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the buffer is REALLY full (which it isn't the first time
 *	a byte is written to stdOut or stdErr:  we fake a full
 *	condition to ensure that this procedure gets called so it
 *	can do initialization), then the bytes in stream's buffer
 *	are written to the stream's file.  The status and end-of-file
 *	fields in stream are set if any problems occur.
 *
 *----------------------------------------------------------------------
 */

void
StdioFileWriteProc(stream, flush)
    register FILE *stream;	/* Stream whose buffer needs to be emptied.
				 * The stream must be writable.  The clientData
				 * field of stream gives a stream index to
				 * pass to the operating system. */
    int flush;			/* Non-zero means it's important to really
				 * write everything out.  Otherwise, this
				 * procedure only needs to write things if
				 * the buffer is full. */
{
    int count;

    /*
     * If this stream doesn't have a buffer associated with it, create
     * a new one, and retrieve the character just written (it was put in
     * stdioTempBuffer).
     */

    if (stream->bufSize == 0) {
	stream->bufSize = BUFSIZ;
	if ((stream == stderr) || (stream == stdout)) {
	    if (isatty((int) stream->clientData)) {
		stream->flags |= STDIO_LINEBUF;
	    }
	}
	if (stream != stderr) {
	    stream->buffer = (unsigned char *) 
		    malloc((unsigned) stream->bufSize);
	} else { 
	    stream->buffer = stdioStderrBuffer;
	    stream->bufSize = STDERR_BUFSIZE;
	}
	stream->lastAccess = stream->buffer;
	*stream->buffer = stdioTempBuffer[0];
    }

    count = stream->lastAccess + 1 - stream->buffer;
    if ((count  == stream->bufSize) || flush) {
	int	written;
	stream->lastAccess = stream->buffer - 1;
	do {
	    written = write((int) stream->clientData, (char *) stream->buffer, 
		count);
	    if (written <= 0 ) {
		stream->writeCount = 0;
		stream->status = errno;
		return;
	    }
	    count = count - written;
	} while (count > 0);
	stream->lastAccess = stream->buffer - 1;
	stream->writeCount = stream->bufSize;
    } else {
	stream->writeCount = stream->bufSize - count;
    }
}
