/* 
 * StdioFileOpenMode.c --
 *
 *	Source code for the "StdioFileOpenMode" procedur used internally
 *	in the stdio library.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/StdioFileOpenMode.c,v 1.3 89/03/10 18:50:51 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "fileInt.h"
#include "stdlib.h"
#include <sys/file.h>

/*
 *----------------------------------------------------------------------
 *
 * StdioFileOpenMode --
 *
 *	Given an access mode string, return the corresponding flags to
 *	pass to open.
 *
 * Results:
 *	The return value is a the flags to pass to open when opening
 *	a file in the given access mode.  -1 is returned if the
 *	access string isn't legal.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
StdioFileOpenMode(access)
    char *access;		/* Indicates type of access, as passed
				 * to fopen and freopen. */
{
    int 	flags;
    char	nextChar;

    nextChar = access[1];
    if (nextChar == 'b') {
	nextChar = access[2];
    }
    switch (access[0]) {
	case 'r':
	    if (nextChar == '+') {
		flags = O_RDWR;
	    } else {
		flags = O_RDONLY;
	    }
	    break;
	case 'w':
	    if (nextChar == '+') {
		flags = O_RDWR | O_CREAT | O_TRUNC;
	    } else {
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	    }
	    break;
	case 'a':
	    if (nextChar == '+') {
		flags = O_CREAT | O_RDWR;
	    } else {
		flags = O_CREAT | O_WRONLY;
	    }
	    break;
	default:
	    return -1;
	    break;
    }
    return flags;
}
