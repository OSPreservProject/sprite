/* 
 * vprintf.c --
 *
 *	Source code for the "vprintf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/vprintf.c,v 1.1 91/12/02 20:06:08 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * vprintf --
 *
 *	Format and print one or more values, writing the output onto
 *	stdout.  See the manual page for details of how the format
 *	string is interpreted.
 *
 * Results:
 *	The return value is a count of the number of characters
 *	written to stdout.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
vprintf(format, args)
    char *format;		/* Format string.  See man page for details. */
    va_list args;		/* Variable-length list of arguments, already
				 * packaged using the varargs macros. */
{
    return vfprintf(stdout, format, args);
}
