/* 
 * sprintf.c --
 *
 *	Source code for the "sprintf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/sprintf.c,v 1.10 90/10/19 15:23:05 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>
#include <cfuncproto.h>
#include <sprite.h>		/* for Boolean typedef */

#ifndef lint

/* 
 * Forward declarations:
 */
static void StringWriteProc _ARGS_((FILE *stream, Boolean flush));


/*
 *----------------------------------------------------------------------
 *
 * StringWriteProc --
 *
 *	This procedure is invoked when the "buffer" for the string
 *	fills up.  Just give the string more space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stream's "buffer" gets enlarged.
 *
 *----------------------------------------------------------------------
 */

static void
StringWriteProc(stream, flush)
    register FILE *stream;
    Boolean flush;		/* ignored */
{
    stream->writeCount = 5000;
}

/*
 *----------------------------------------------------------------------
 *
 * sprintf --
 *
 *	Format and print one or more values, placing the output into
 *	a string.  See the manual page for details of how the format
 *	string is interpreted.
 *
 * Results:
 *	The return value is a pointer to string, which has been
 *	overwritten with formatted information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
sprintf(va_alist)
    va_dcl			/* char *string, then char *format, then any
				 * number of additional values to be printed
				 * in string under control of format. */
{
    char *string;
    char *format;
    FILE stream;
    va_list args;

    va_start(args);
    string = va_arg(args, char *);
    format = va_arg(args, char *);
    Stdio_Setup(&stream, 0, 1, (unsigned char *)string, 5000, (void (*)()) 0,
		StringWriteProc, (int (*)()) 0, (ClientData) 0);
    (void) vfprintf(&stream, format, args);
    putc(0, &stream);
    return string;
}
#else
/* VARARGS2 */
/* ARGSUSED */
char *
sprintf(string, format)
    char *string;
    char *format;
{
    return string;
}
#endif lint
