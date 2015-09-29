/* 
 * strcspn.c --
 *
 *	Source code for the "strcspn" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strcspn.c,v 1.2 89/03/22 16:06:53 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strcspn --
 *
 *	Count how many of the leading characters there are in "string"
 *	before there's one that's also in "chars".
 *
 * Results:
 *	The return value is the index of the first character in "string"
 *	that is also in "chars".  If there is no such character, then
 *	the return value is the length of "string".
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
strcspn(string, chars)
    char *string;			/* String to search. */
    char *chars;			/* Characters to look for in string. */
{
    register char c, *p, *s;

    for (s = string, c = *s; c != 0; s++, c = *s) {
	for (p = chars; *p != 0; p++) {
	    if (c == *p) {
		return s-string;
	    }
	}
    }
    return s-string;
}
