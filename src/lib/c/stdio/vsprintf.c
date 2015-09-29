/* 
 * vsprintf.c --
 *
 *	Source code for the "vsprintf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/vsprintf.c,v 1.2 89/05/18 17:15:51 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

/*
 * Forward references to procedure defined in this file:
 */

static void	WriteProc();


/*
 *----------------------------------------------------------------------
 *
 * vsprintf --
 *
 *	Format and print one or more values, placing the output into
 *	a string.  See the manual page for details of how the format
 *	string is interpreted.  This procedure is identical to sprintf
 *	except that the arguments have been prepackaged using varargs.
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
vsprintf(string, format, args)
    char *string;		/* Where to place result. */
    char *format;		/* How to format result.  See man page
				 * for details. */
    va_list args;		/* Variable-length list of arguments,
				 * assembled using the varargs macros. */
{
    FILE stream;

    Stdio_Setup(&stream, 0, 1, (unsigned char *) string, 5000, (void (*)()) 0,
	    WriteProc, (int (*)()) 0, (ClientData) 0);
    (void) vfprintf(&stream, format, args);
    putc(0, &stream);
    return string;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteProc --
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
WriteProc(stream)
    register FILE *stream;
{
    stream->writeCount = 5000;
}
