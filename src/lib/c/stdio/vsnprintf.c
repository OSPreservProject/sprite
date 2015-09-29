/* 
 * vsnprintf.c --
 *
 *	Source code for the "vsnprintf" library procedure.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/vsnprintf.c,v 1.1 90/09/24 14:38:29 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>
#include <errno.h>

/*
 * Forward references to procedure defined in this file:
 */

static void	WriteProc();


/*
 *----------------------------------------------------------------------
 *
 * vsnprintf --
 *
 *	Format and print one or more values, placing the output into
 *	a string.  See the manual page for details of how the format
 *	string is interpreted.  This procedure is similar to sprintf
 *	except that the arguments have been prepackaged using varargs, 
 *	and we've been told how much room there is in the string.
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
vsnprintf(string, stringSize, format, args)
    char *string;		/* Where to place result. */
    int stringSize;		/* Number of bytes in "string". */
    char *format;		/* How to format result.  See man page
				 * for details. */
    va_list args;		/* Variable-length list of arguments,
				 * assembled using the varargs macros. */
{
    FILE stream;

    Stdio_Setup(&stream, 0, 1, (unsigned char *) string, stringSize,
		(void (*)()) 0, WriteProc, (int (*)()) 0, (ClientData) 0);
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
 *	fills up.  Complain to the user and set an error flag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is displayed, and the stream has been marked 
 *	so that no other characters will be put into it.
 *
 *----------------------------------------------------------------------
 */

static void
WriteProc(stream)
    register FILE *stream;
{
    fprintf(stderr, "vsnprintf: string overflow\n");
    stream->status = ENOMEM;
}
