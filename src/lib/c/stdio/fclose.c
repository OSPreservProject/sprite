/* 
 * fclose.c --
 *
 *	Source code for the "fclose" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/fclose.c,v 1.2 91/06/03 17:40:00 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * fclose --
 *
 *	Flush any remaining I/O and perform stream-dependent operations
 *	to close off stream.  From this point on, no further I/O should
 *	be performed on stream.
 *
 * Results:
 *	EOF is returned if there is an error condition pending for
 *	the stream, or if an error occurred during the close.
 *
 * Side effects:
 *	The stream is closed.
 *
 *----------------------------------------------------------------------
 */

int
fclose(stream)
    FILE *stream;		/* Stream to be closed. */
{
    int result;

    /* 
     * If stream is NULL, we will eventually get a segmentation fault. 
     * This is intentional, to point out a probable bug in the calling 
     * program.  One could make a case for making fclose() more 
     * forgiving about NULL streams, but this way we track what the 
     * BSD guys are doing.
     */

    result = fflush(stream);
    if (stream->closeProc == NULL) {
	return result;
    }
    if ((*stream->closeProc)(stream) == EOF) {
	return EOF;
    }
    return result;
}
