/* 
 * fprintf.c --
 *
 *	Source code for the "fprintf" library procedure.
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
static char rcsid[] = "$Header: fprintf.c,v 1.6 88/07/28 17:18:33 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * fprintf --
 *
 *	Format and print one or more values, writing the output onto
 *	stream.  See the manual page for details of how the format
 *	string is interpreted.
 *
 * Results:
 *	The return value is a count of the number of characters
 *	written to stream.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
int
fprintf(va_alist)
    va_dcl			/* FILE *stream, then char *format, then any
				 * number of additional values to be printed
				 * as described by format. */ 
{
    FILE *stream;
    char *format;
    va_list args;

    va_start(args);
    stream = va_arg(args, FILE *);
    format = va_arg(args, char *);
    return vfprintf(stream, format, args);
}
#else
/* VARARGS2 */
/* ARGSUSED */
int
fprintf(stream, format)
    FILE *stream;
    char *format;
{
    return 0;
}
#endif lint
