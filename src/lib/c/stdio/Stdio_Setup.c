/* 
 * Stdio_Setup.c --
 *
 *	Source code for the "Stdio_Setup" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/Stdio_Setup.c,v 1.2 90/10/11 22:10:13 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * Stdio_Setup --
 *
 *	This procedure is called by a client to initialize the information
 *	associated with a stream.  This procedure is used by special-purpose
 *	clients that provide their own low-level I/O;  for normal file I/O,
 *	use fopen, freopen, or fdopen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of stream are modified to set up for buffered I/O.
 *	After this call has been made, the stream may be used in other
 *	calls (e.g. getc and putc) to actually perform I/O.
 *
 *----------------------------------------------------------------------
 */

void
Stdio_Setup(stream, readable, writable, buffer, bufferSize, readProc,
	writeProc, closeProc, clientData)
    register FILE * stream;	/* Stream to be initialized. */
    int readable;		/* Non-zero means stream is to be readable.  */
    int writable;		/* Non-zero means stream is to be writable.
				 * It's OK for a stream to be both readable
				 * and writable.
				 */
    unsigned char *buffer;	/* Pointer to buffer space for the stream.
				 * NULL means the stream will initially be
				 * unbuffered. */
    int bufferSize;		/* Size of buffer area. */
    void (*readProc) _ARGS_((FILE *stream));
				/* Procedure to perform input for stream. */
    void (*writeProc) _ARGS_((FILE *stream, Boolean flush));
				/* Procedure to perform output for stream. */
    int (*closeProc) _ARGS_((FILE *stream));
				/* Procedure to close stream.  NULL means
				 * there aren't any stream-dependent actions
				 * to perform on close. */
    ClientData clientData;	/* Client-specific information to store in
				 * stream. */
{
    stream->lastAccess		= buffer-1;
    stream->readCount		= 0;
    stream->writeCount		= 0;
    stream->buffer		= buffer;
    stream->bufSize		= bufferSize;
    stream->readProc		= readProc;
    stream->writeProc		= writeProc;
    stream->closeProc		= closeProc;
    stream->clientData		= clientData;
    stream->status		= 0;
    stream->flags		= 0;
    if (readable) {
	stream->flags |= STDIO_READ;
    }
    if (writable) {
	stream->flags |= STDIO_WRITE;
	stream->writeCount = bufferSize;
    }
}
