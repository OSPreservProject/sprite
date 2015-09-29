/* 
 * getw.c --
 *
 *	Source code for the "getw" library procedure.
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
static char rcsid[] = "$Header: getw.c,v 1.1 88/06/13 10:00:29 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * getw --
 *
 *	Read an integer word from a stream, in order of increasing
 *	byte number.  This procedure should be avoided like the
 *	plague, since it's byte-order sensitive.
 *
 * Results:
 *	The return value is the word read, or EOF if there was an
 *	error (unfortunately, EOF looks just like an integer, so
 *	the caller really has to call ferror).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getw(stream)
    register FILE *stream;		/* Stream from which to read. */
{
    int result, i;
    register char *p;

    for (i = 0, p = (char *) &result; i < sizeof(int); i++, p++) {
	*p = getc(stream);
    }
    if (feof(stream)) {
	return EOF;
    }
    return result;
}
