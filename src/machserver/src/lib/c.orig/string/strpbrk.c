/* 
 * strpbrk.c --
 *
 *	Source code for the "strpbrk" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strpbrk.c,v 1.2 89/03/22 16:07:46 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strpbrk --
 *
 *	Search a string for a character from a given set.
 *
 * Results:
 *	The return value is the address of the first character
 *	in "string" that is also a character in "chars".  If there
 *	is no such character, then 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
strpbrk(string, chars)
    register char *string;		/* String to search. */
    char *chars;			/* Characters to look for in string. */
{
    register char c, *p;

    for (c = *string; c != 0; string++, c = *string) {
	for (p = chars; *p != 0; p++) {
	    if (c == *p) {
		return string;
	    }
	}
    }
    return 0;
}
