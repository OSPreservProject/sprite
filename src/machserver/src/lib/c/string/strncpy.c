/* 
 * strncpy.c --
 *
 *	Source code for the "strncpy" library routine.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/string/RCS/strncpy.c,v 1.2 91/12/01 22:31:50 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strncpy --
 *
 *	Copy exactly numChars characters from src to dst.  If src doesn't
 *	contain exactly numChars characters, then the last characters are
 *	ignored (if src is too long) or filled with zeros (if src
 *	is too short).  If src is too long then dst will not be
 *	null-terminated.
 *
 * Results:
 *	The return value is a pointer to the destination string, dst.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
strncpy(dst, src, numChars)
    register _CONST char *src;	/* Place from which to copy. */
    char *dst;			/* Place to store copy. */
    int numChars;		/* Maximum number of characters to copy. */
{
    register char *copy = dst;

    while (--numChars >= 0) {
	if ((*copy++ = *src++) == '\0') {
	    while (--numChars >= 0) {
		*copy++ = '\0';
	    }
	    return dst;
	}
    }
    return dst;
}

