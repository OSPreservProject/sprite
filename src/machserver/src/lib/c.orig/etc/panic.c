/* 
 * panic.c --
 *
 *	Source code for the "panic" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/panic.c,v 1.8 90/10/29 13:19:03 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include <varargs.h>

/*
 *----------------------------------------------------------------------
 *
 * panic --
 *
 *	Print an error message and kill the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
void
panic(va_alist)
    va_dcl			/* char *format, then any number of additional
				 * values to be printed under the control of
				 * format.  This is all just the same as you'd
				 * pass to printf. */
{
    char *format;
    va_list args;

    va_start(args);
    format = va_arg(args, char *);
    (void) vfprintf(stderr, format, args);
    (void) fflush(stderr);
    abort();
    va_end(args);
}
#else
/* VARARGS1 */
/* ARGSUSED */
void
panic(format)
    char *format;
{
    return;
}
#endif lint
