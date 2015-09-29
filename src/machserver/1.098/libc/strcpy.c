/* 
 * strcpy.c --
 *
 *	Source code for the "strcpy" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strcpy.c,v 1.3 89/03/22 16:06:49 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strcpy --
 *
 *	Copy a string from one location to another.
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
strcpy(dst, src)
    register char *src;		/* Place from which to copy. */
    char *dst;			/* Place to store copy. */
{
    register char *copy = dst;

    do {
	if (!(copy[0] = src[0]) || !(copy[1] = src[1])
		|| !(copy[2] = src[2]) || !(copy[3] = src[3])) {
	    break;
	}
	copy += 4;
	src += 4;
    } while (1);
    return dst;
}
