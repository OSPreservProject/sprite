/* 
 * fscanf.c --
 *
 *	Source code for the "fscanf" library procedure.
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
static char rcsid[] = "$Header: fscanf.c,v 1.6 88/07/28 17:18:37 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * fscanf --
 *
 *	Same as scanf, except take input from a given I/O stream
 *	instead of stdin.
 *
 * Results:
 *	The values indicated by va_alist are modified to hold
 *	information scanned from stream.  The return value is the
 *	number of fields successfully scanned, or EOF if the string
 *	is empty.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
int
fscanf(va_alist)
    va_dcl			/* FILE *stream, then char *format, then
				 * pointers to variables to be filled in
				 * with values scanned under control of
				 * format. */
{
    FILE *stream;
    char *format;
    va_list args;

    va_start(args);
    stream = va_arg(args, FILE *);
    format = va_arg(args, char *);
    return vfscanf(stream, format, args);
}
#else
/* VARARGS2 */
/* ARGSUSED */
int
fscanf(stream, format)
    FILE *stream;
    char *format;
{
    return 0;
}
#endif lint
