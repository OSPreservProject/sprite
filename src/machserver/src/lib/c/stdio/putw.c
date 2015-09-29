/* 
 * putw.c --
 *
 *	Source code for the "putw" library procedure.
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
static char rcsid[] = "$Header: putw.c,v 1.1 88/06/13 10:00:27 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * putw --
 *
 *	Write an integer word out to a stream, in character byte order.
 *
 * Results:
 *	Normally returns zero.  If an error occurs, then it returns
 *	EOF.
 *
 * Side effects:
 *	Stuff gets written on stream.
 *
 *----------------------------------------------------------------------
 */

int
putw(w, stream)
    int w;			/* Value to write. */
    register FILE *stream;	/* Where to write it. */
{
    register char *p;
    int i;

    for (i = 0, p = (char *) &w; i < sizeof(int); i++, p++) {
	if (putc(*p, stream) == EOF) {
	    return EOF;
	}
    }
    return 0;
}
