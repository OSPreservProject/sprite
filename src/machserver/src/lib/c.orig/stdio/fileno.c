/* 
 * fileno.c --
 *
 *	Source code for the "fileno" library procedure.
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
static char rcsid[] = "$Header: fileno.c,v 1.1 88/06/10 16:23:45 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"

/*
 *----------------------------------------------------------------------
 *
 * fileno --
 *
 *	Returns the stream identifier associated with a stream.
 *
 * Results:
 *	If stream isn't a file-related stream then -1 is returned.
 *	Otherwise the return value is the stream identifier associated
 *	with stream.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
fileno(stream)
    FILE * stream;		/* Stream for which id is desired. */
{
    if ((stream->readProc != (void (*)()) StdioFileReadProc) ||
	((stream->flags & (STDIO_READ|STDIO_WRITE)) == 0)) {
	return(-1);
    }
    return((int) stream->clientData);
}
