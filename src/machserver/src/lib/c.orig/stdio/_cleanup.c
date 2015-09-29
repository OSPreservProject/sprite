/* 
 * _cleanup.c --
 *
 *	Source code for the "_cleanup" library procedure.
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
static char rcsid[] = "$Header: _cleanup.c,v 1.2 88/07/20 18:12:15 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"

/*
 *----------------------------------------------------------------------
 *
 * _cleanup --
 *
 *	This procedure is invoked once just before the process exits.
 *	It flushes all of the open streams.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Streams get flushed.
 *
 *----------------------------------------------------------------------
 */

void
_cleanup()
{
    register FILE *stream;

    for (stream = stdioFileStreams; stream != NULL; stream = stream->nextPtr) {
	if (!(stream->flags & STDIO_WRITE)) {
	    continue;
	}
	fflush(stream);
    }
}
