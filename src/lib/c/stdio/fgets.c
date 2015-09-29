/* 
 * fgets.c --
 *
 *	Source code for the "fgets" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/fgets.c,v 1.2 92/01/21 16:29:07 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * fgets --
 *
 *	Reads a line from a stream.
 *
 * Results:
 *	Characters are read from stream and placed at buf until a
 *	newline is encountered or maxChars-1 characters have been
 *	processed or an end of file or error is encountered.  The
 *	string at buf is left null-terminated.  The return value is
 *	a pointer to buf if all went well, or NULL if an end of file
 *	or error was encountered before reading any characters.
 *
 * Side effects:
 *	Characters are removed from stream.
 *
 *----------------------------------------------------------------------
 */

char *
fgets(bufferPtr, maxChars, stream)
    char *bufferPtr;		/* Where to place characters.  Must have
				 * at least maxChars bytes of storage. */
    register int maxChars;	/* Maximum number of characters to read
				 * from stream. */
    register FILE *stream;	/* Stream from which to read characters. */
{
    register char *destPtr = bufferPtr;
    register int c;

    for (maxChars--; maxChars > 0; maxChars--) {
	c = getc(stream);
	if (c < 0) {
	/* Other systems don't clear destPtr, and it breaks some programs,
	 * so I'm taking this out.
	 */
#if 0
	    *destPtr = 0;
#endif
	    return NULL;
	}
	*destPtr = c;
	destPtr++;
	if (c == '\n') {
	    break;
	}
    }
    *destPtr = 0;
    return bufferPtr;
}
