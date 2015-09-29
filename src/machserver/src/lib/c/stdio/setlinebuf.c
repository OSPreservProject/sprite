/* 
 * setlinebuf.c --
 *
 *	Source code for the "setlinebuf" library procedure.
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
static char rcsid[] = "$Header: setlinebuf.c,v 1.1 88/06/10 16:23:59 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * setlinebuf --
 *
 *	Turn on line buffering for a stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, the buffer for stream will be flushed whenever a
 *	newline character is written to it, and also whenever input is
 *	read from stdin.
 *
 *----------------------------------------------------------------------
 */

void
setlinebuf(stream)
    FILE *stream;		/* Stream to be re-buffered. */
{
    stream->flags |= STDIO_LINEBUF;
}
