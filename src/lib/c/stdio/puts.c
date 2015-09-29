/* 
 * puts.c --
 *
 *	Source code for the "puts" library procedure.
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
static char rcsid[] = "$Header: puts.c,v 1.1 88/06/10 16:23:55 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * puts --
 *
 *	Writes a string out onto stdout.
 *
 * Results:
 *	The constant EOF is returned if any sort of error occurred
 *	while writing the string to stdout.
 *
 * Side effects:
 *	The characters of string are written to stdout, in order,
 *	up to but not including the terminating null character.
 *	An additional newline character is written to stdout after
 *	the string.
 *
 *----------------------------------------------------------------------
 */

int
puts(string)
    register char *string;		/* String to output. */
{
    while (*string != 0) {
	putchar(*string);
	string++;
    }
    return putchar('\n');
}
