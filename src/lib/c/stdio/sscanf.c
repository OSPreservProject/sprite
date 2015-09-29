/* 
 * sscanf.c --
 *
 *	Source code for the "sscanf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/sscanf.c,v 1.8 90/09/11 14:27:26 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <cfuncproto.h>

#ifndef lint

/* 
 * Forward declarations:
 */
static void StringReadProc _ARGS_((FILE *stream));


/*
 *----------------------------------------------------------------------
 *
 * StringReadProc --
 *
 *	This procedure is invoked when an attempt is made to read
 *	past the end of a string.  It returns an end-of-file
 *	condition.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stream is marked with an end-of-file condition.
 *
 *----------------------------------------------------------------------
 */

static void
StringReadProc(stream)
    register FILE *stream;
{
    stream->readCount = 0;
    stream->flags |= STDIO_EOF;
}

/*
 *----------------------------------------------------------------------
 *
 * sscanf --
 *
 *	Same as scanf, except take input from a string rather than
 *	an I/O stream.
 *
 * Results:
 *	The values indicated by va_alist are modified to hold
 *	information scanned from string.  The return value is the
 *	number of fields successfully scanned, or EOF if the string
 *	is empty.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
sscanf(va_alist)
    va_dcl			/* char *string, then char *format, then any
				 * number of pointers to values to be scanned
				 * from string under control of format. */
{
    char *string;
    char *format;
    FILE stream;
    va_list args;

    va_start(args);
    string = va_arg(args, char *);
    format = va_arg(args, char *);
    Stdio_Setup(&stream, 1, 0, (unsigned char *)string, strlen(string),
		StringReadProc, (void (*)()) 0, (int (*)()) 0, (ClientData) 0);
    stream.readCount = strlen(string);
    return vfscanf(&stream, format, args);
}
#else
/* VARARGS2 */
/* ARGSUSED */
int
sscanf(string, format)
    char *string;
    char *format;
{
    return 0;
}
#endif lint
