/* 
 * scanf.c --
 *
 *	Source code for the "scanf" library procedure.
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
static char rcsid[] = "$Header: scanf.c,v 1.6 88/07/28 17:18:43 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * scanf --
 *
 *	Read characters from stdin and parse them into internal
 *	representations.
 *
 * Results:
 *	The values indicated by va_alist are modified to hold
 *	information scanned from stream.  The return value is the
 *	number of fields successfully scanned, or EOF if the string
 *	is empty.  See the man page for details.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
int
scanf(va_alist)
    va_dcl			/* char *format, then any number of pointers to
				 * values to be scanned from stdin under
				 * control of format. */
{
    char *format;
    va_list args;

    va_start(args);
    format = va_arg(args, char *);
    return vfscanf(stdin, format, args);
}
#else
/* VARARGS1 */
/* ARGSUSED */
int
scanf(format)
    char *format;
{
    return 0;
}
#endif lint
