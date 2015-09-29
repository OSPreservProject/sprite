/* 
 * fseek.c --
 *
 *	Source code for the "fseek" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/fseek.c,v 1.5 89/06/15 22:37:53 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"

extern long lseek();

/*
 *----------------------------------------------------------------------
 *
 * fseek --
 *
 *	Modify the access position of a stream.
 *
 * Results:
 *	Returns 0 if the seek was completed successfully, -1 if any
 *	sort of error occurred.
 *
 * Side effects:
 *	The access position of stream (i.e. the place in the file where
 *	the next character will be read or written) is set to the sum
 *	of offset and a base value.  If base is 0, the base value is 0.
 *	If base is 1, the base value is the current access position.
 *	If base is 2, the base value is the length of the file.
 *
 *----------------------------------------------------------------------
 */

long
fseek(stream, offset, base)
    register FILE *stream;		/* Stream whose position is to
					 * be changed. */
    int offset;				/* See above for explanation. */
    int base;				/* See above for explanation. */
{
    int result;

    if ((stream->readProc != (void (*)()) StdioFileReadProc)
	    || ((stream->flags & (STDIO_READ|STDIO_WRITE)) == 0)) {
	return -1;
    }

    /*
     * Optimize for the case in which we are doing only reads and we
     * can reset the pointers without doing a real lseek.  (Don't
     * bother if relative to EOF, or if the buffer is invalid.)  This
     * is useful when people want to peek forward more than one
     * character at a time and use fseek to reset the buffer after
     * peeking.
     */
    if (((stream->flags & (STDIO_READ|STDIO_WRITE)) == STDIO_READ) &&
	(base != 2) && stream->readCount > 0) {
	int endAddr;		/* file pointer for end of read buffer  */
	int curAddr;		/* file pointer for current ptr into read
				   buffer  */
	int startAddr;		/* file pointer for start of read buffer  */
	int newAddr;		/* file pointer after seek */
	
	endAddr = lseek((int) stream->clientData, (long) 0, 1);
	if (endAddr == -1) {
	    return -1;
	}
	curAddr = endAddr - stream->readCount;
	startAddr = curAddr - (stream->lastAccess + 1 - stream->buffer);
	newAddr = offset;
	if (base == 1) {
	    newAddr += curAddr;
	}
	if (newAddr >= startAddr && newAddr <= endAddr) {
	    stream->readCount += curAddr - newAddr;
	    stream->lastAccess -= curAddr - newAddr;
	    stream->flags &= ~STDIO_EOF;
	    return 0;
	}
    }
	
    /*
     * I'm going to reset all the buffer pointers, so flush any pending
     * output.
     */
    
    result = fflush(stream);

    /*
     * Compute the offset and base to pass to the system to reposition.
     * This is a tricky if the base value is the current access position:
     * have to account for the characters that the system has passed to
     * me but that I haven't passed to the user.
     */
    
    if (base == 1) {
	offset -= stream->readCount;
    }

    stream->readCount = 0;
    stream->writeCount = 0;
    stream->lastAccess = stream->buffer - 1;
    stream->flags &= ~STDIO_EOF;

    if (lseek((int) stream->clientData, (long) offset, base) == -1) {
	return -1;
    }
    return result;
}
