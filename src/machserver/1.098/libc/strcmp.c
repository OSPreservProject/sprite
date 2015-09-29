/* 
 * strcmp.c --
 *
 *	Source code for the "strcmp" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strcmp.c,v 1.2 89/03/22 16:06:45 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strcmp --
 *
 *	Compare two strings lexicographically.
 *
 * Results:
 *	The return value is 0 if the strings are identical, 1
 *	if the first string is greater than the second, and 
 *	-1 if the first string is less than the second.  If one
 *	string is a prefix of the other then it is considered
 *	to be less (the terminating zero byte participates in the
 *	comparison).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
strcmp(s1, s2)
    register char *s1, *s2;		/* Strings to compare. */
{
    while (1) {
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
}
