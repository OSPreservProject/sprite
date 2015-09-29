/* 
 * freopen.c --
 *
 *	Source code for the "freopen" library procedure.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdio/RCS/freopen.c,v 1.3 91/12/13 12:01:36 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "fileInt.h"
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <unistd.h>

/*
 *----------------------------------------------------------------------
 *
 * freopen --
 *
 *	Close the file currently associated with a stream and then re-open
 *	the stream on a new file.
 *
 * Results:
 *	The return value is NULL if an error occurred in opening the
 *	new file, or stream otherwise.
 *
 * Side effects:
 *	A file is opened, and a stream is initialized.  Errors in closing
 *	the old stream are ignored.
 *
 *----------------------------------------------------------------------
 */

FILE *
freopen(fileName, access, stream)
    char *fileName;		/* Name of file to be opened. */
    char *access;		/* Indicates type of access, just as for
				 * fopen. */
    FILE *stream;		/* Name of old stream to re-use. */
{
    int id, flags, read, write, oldFlags;

    if (stream->readProc != (void (*)()) StdioFileReadProc) {
	return (FILE *) NULL;
    }

    if (stream->flags != 0) {
	fflush(stream);
	id = (int) stream->clientData;
	close(id);
    }

    /*
     * Open a new stream and let it re-use the old stream's structure.
     */

    flags = StdioFileOpenMode(access);
    if (flags == -1) {
	return (FILE *) NULL;
    }
    id = open(fileName, flags, 0666);
    if (id < 0) {
	return (FILE *) NULL;
    }
    read = write = 0;
    if ((access[1] == '+') || ((access[1] == 'b') && (access[2] == '+'))) {
	read = write = 1;
    } else if (access[0]  == 'r') {
	read = 1;
    } else {
	write = 1;
    }
    if (access[0] == 'a') {
	(void) lseek(id, 0L, L_XTND);
    }
    oldFlags = stream->flags & (STDIO_NOT_OUR_BUF | STDIO_LINEBUF);
    if (!(stream->flags & STDIO_NOT_OUR_BUF)) {
	Stdio_Setup(stream, read, write, stdioTempBuffer, 0,
		StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
		(ClientData) id);
    } else {
	Stdio_Setup(stream, read, write, stream->buffer, stream->bufSize,
		StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
		(ClientData) id);
    }
    stream->flags |= oldFlags;
    return stream;
}
