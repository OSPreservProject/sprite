/* 
 * fputs.c --
 *
 *	Source code for the "fputs" library procedure.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdio/RCS/fputs.c,v 1.2 91/12/12 22:09:21 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>

/*
 *----------------------------------------------------------------------
 *
 * fputs --
 *
 *	Writes a string out onto a stream.
 *
 * Results:
 *	The return value is EOF if an error occurred while writing
 *	the stream.
 *
 * Side effects:
 *	The characters of string are written to stream, in order,
 *	up to but not including the terminating null character.
 *
 *----------------------------------------------------------------------
 */

int
fputs(string, stream)
    register char *string;		/* String to output. */
    register FILE *stream;		/* Stream on which to write string. */
{
    register int result = 0;

    while (*string != 0) {
	result = putc(*string, stream);
	string++;
    }
    return result;
}
