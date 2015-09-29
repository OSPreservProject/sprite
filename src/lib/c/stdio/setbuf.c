/* 
 * setbuf.c --
 *
 *	Source code for the "setbuf" library procedure.
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
static char rcsid[] = "$Header: setbuf.c,v 1.1 88/06/10 16:23:57 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * setbuf --
 *
 *	Use a user-allocated buffer for a stdio stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, buf will be used as the buffer for stream.  If
 *	buf is NULL, then the stream will be unbuffered.
 *
 *----------------------------------------------------------------------
 */

void
setbuf(stream, buf)
    FILE *stream;		/* Stream to be re-buffered. */
    char *buf;			/* Buffer area;  must hold at least BUFSIZ
				 * bytes, and must live as long as stream
				 * does.  NULL means stream should be
				 * unbuffered. */
{
    if (buf == 0) {
	(void) setvbuf(stream, buf, _IONBF, 1);
    } else {
	(void) setvbuf(stream, buf, _IOFBF, BUFSIZ);
    }
}
