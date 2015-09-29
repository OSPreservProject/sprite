/* 
 * clearerr.c --
 *
 *	Source code for the "clearerr" library procedure.
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
static char rcsid[] = "$Header: clearerr.c,v 1.1 88/06/10 16:23:39 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * clearerr --
 *
 *	This procedure clears out any error conditions associated with
 *	a stream, allowing further I/Os to be done (or at least
 *	attempted) on the stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The error and end-of-file fields are cleared in stream.  Once
 *	an error has been recorded for a stream, no I/O will be attempted
 *	on the stream until the error has been cleared.  Similarly, once
 *	an EOF has occurred on a stream, no further input will occur
 *	until the EOF has explicitly been cleared.
 *
 *----------------------------------------------------------------------
 */

void
clearerr(stream)
    FILE *stream;
{
    stream->status = 0;
    stream->flags &= ~STDIO_EOF;
}
