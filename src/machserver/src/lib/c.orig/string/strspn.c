/* 
 * strspn.c --
 *
 *	Source code for the "strspn" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strspn.c,v 1.2 89/03/22 16:07:53 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strspn --
 *
 *	Compute the length of the maximum initial segment of "string"
 *	whose characters all are in "chars".
 *
 * Results:
 *	The return value is the length of the initial segment (0 if the
 *	first character isn't in "chars".
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
strspn(string, chars)
    char *string;			/* String to search. */
    char *chars;			/* Characters to look for in string. */
{
    register char c, *p, *s;

    for (s = string, c = *s; c != 0; s++, c = *s) {
	for (p = chars; *p != 0; p++) {
	    if (c == *p) {
		goto next;
	    }
	}
	break;
	next: ;
    }
    return s-string;
}
