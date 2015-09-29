/* 
 * strlen.c --
 *
 *	Source code for the "strlen" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strlen.c,v 1.4 89/03/22 16:07:00 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strlen --
 *
 *	Count the number of characters in a string.
 *
 * Results:
 *	The return value is the number of characters in the
 *	string, not including the terminating zero byte.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
strlen(string)
    char *string;		/* String whose length is wanted. */
{
    register char *p = string;

    while (1) {
	if (p[0] == 0) {
	    return p - string;
	}
	if (p[1] == 0) {
	    return p + 1 - string;
	}
	if (p[2] == 0) {
	    return p + 2 - string;
	}
	if (p[3] == 0) {
	    return p + 3 - string;
	}
	p += 4;
    }
}
