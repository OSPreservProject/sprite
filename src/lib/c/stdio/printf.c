/* 
 * printf.c --
 *
 *	Source code for the "printf" library procedure.
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
static char rcsid[] = "$Header: printf.c,v 1.6 88/07/28 17:18:40 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * printf --
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

#ifndef lint
int
printf(va_alist)
    va_dcl			/* char *format, then any number of additional
				 * values to be printed as described by
				 * format. */
{
    char *format;
    va_list args;

    va_start(args);
    format = va_arg(args, char *);
    return vfprintf(stdout, format, args);
}
#else
/* VARARGS1 */
/* ARGSUSED */
int
printf(format)
    char *format;
{
    return 0;
}
#endif lint
