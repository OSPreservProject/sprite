/* 
 * putchar.c --
 *
 *	Source code for a procedural version of the putchar macro.
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
static char rcsid[] = "$Header: proto.c,v 1.2 88/03/11 08:39:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#undef putchar

/*
 *----------------------------------------------------------------------
 *
 * putchar --
 *
 *	Output a character on the stdout stream.
 *
 * Results:
 *	The return value is EOF if an error occurred while writing
 *	to the stream, or if the stream isn't writable.
 *
 * Side effects:
 *	Characters are buffered up for stdout.
 *
 *----------------------------------------------------------------------
 */

int
putchar(c)
{
    return putc(c, stdout);
}
