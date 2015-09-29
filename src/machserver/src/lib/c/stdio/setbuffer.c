/* 
 * setbuffer.c --
 *
 *	Source code for the "setbuffer" library procedure.
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
static char rcsid[] = "$Header: setbuffer.c,v 1.2 88/07/28 17:18:45 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * setbuffer --
 *
 *	Reset the buffering strategy to use for stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If buf is NULL, stream will be unbuffered from now on.  Otherwise,
 *	buf will be used as the buffer for stream.
 *
 *----------------------------------------------------------------------
 */

void
setbuffer(stream, buf, size)
    FILE *stream;		/* Stream to be re-buffered. */
    char *buf;			/* New buffer to use for stream.   NULL means
				 * make stream unbuffered.  Otherwise, this
				 * space must persist until the stream is
				 * closed or rebuffered. */
    int size;			/* Number of bytes of storage space at
				 * buf. */
{
    if (buf == 0) {
	(void) setvbuf(stream, (char *) 0, _IONBF, 1);
    } else {
	(void) setvbuf(stream, buf, _IOFBF, size);
    }
}
