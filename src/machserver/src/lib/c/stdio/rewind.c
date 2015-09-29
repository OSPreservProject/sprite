/* 
 * rewind.c --
 *
 *	Source code for the "rewind" library procedure.
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
static char rcsid[] = "$Header: rewind.c,v 1.1 88/06/10 16:23:56 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"

/*
 *----------------------------------------------------------------------
 *
 * rewind --
 *
 *	Reset the access position of a stream to its beginning.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The access position of the stream is reset so that the
 *	next character read will be the first character of the file.
 *
 *----------------------------------------------------------------------
 */

void
rewind(stream)
    register FILE *stream;		/* Stream to rewind. */
{
    (void) fseek(stream, 0, SEEK_SET);
    clearerr(stream);
}
