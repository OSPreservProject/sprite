/* 
 * strncmp.c --
 *
 *	Source code for the "strncmp" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strncmp.c,v 1.2 89/03/22 16:07:11 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strncmp --
 *
 *	Compares two strings lexicographically.
 *
 * Results:
 *	The return value is 0 if the strings are identical in their
 *	first s1 characters.  If they differ in their first s1
 *	characters, then the return value is 1 if the first string is
 *	greater than the second, and -1 if the second string is less
 *	than the first.  If one string is a prefix of the other then
 *	it is considered to be less (the terminating zero byte participates
 *	in the comparison).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
strncmp(s1, s2, numChars)
    register char *s1, *s2;		/* Strings to compare. */
    register int numChars;		/* Max number of chars to compare. */
{
    for ( ; numChars > 0; numChars -= 1) {
	if (*s1 != *s2) {
	    if (*s1 > *s2) {
		return 1;
	    } else {
		return -1;
	    }
	}
	if (*s1++ == 0) {
	    return 0;
	}
	s2 += 1;
    }
    return 0;
}
