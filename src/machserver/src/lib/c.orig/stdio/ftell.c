/* 
 * ftell.c --
 *
 *	Source code for the "ftell" library procedure.
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
static char rcsid[] = "$Header: ftell.c,v 1.5 88/07/29 18:56:38 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"
#include <sys/file.h>

extern long ftell(), lseek();

/*
 *----------------------------------------------------------------------
 *
 * ftell --
 *
 *	This procedure returns the current access position in a file
 *	stream, as a byte count from the beginning of the file.
 *
 * Results:
 *	The return value is the location (measured in bytes from the
 *	beginning of the file associated with stream) where the next
 *	byte will be read or written.  If the stream doesn't
 *	correspond to a file, or if there is an error during the operation,
 *	then -1 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

long
ftell(stream)
    register FILE *stream;
{
    int count;

    if ((stream->readProc != (void (*)()) StdioFileReadProc) ||
	((stream->flags & (STDIO_READ|STDIO_WRITE)) == 0)) {
	return -1;
    }

    count = lseek((int) stream->clientData, 0L, L_INCR);
    if (count < 0) {
	return -1;
    }

    /*
     * The code is different for reading and writing.  For writing,
     * we add the system's idea of current position to the number
     * of bytes waiting in the buffer.  For reading, subtract the
     * number of bytes still available in the buffer from the system's
     * idea of the current position.
     */

    if (stream->writeCount > 0) {
	count += stream->lastAccess + 1 - stream->buffer;
    } else if (stream->readCount > 0) {
	count -= stream->readCount;
    }

    return(count);
}
