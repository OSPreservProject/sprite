/* 
 * strcat.c --
 *
 *	Source code for the "strcat" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strcat.c,v 1.2 89/03/22 16:06:35 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strcat --
 *
 *	Copy one string (src) onto the end of another (dst).
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
strcat(dst, src)
    register char *src;		/* Place from which to copy. */
    char *dst;			/* Destination string:  *srcPtr gets added
				 * onto the end of this. */
{
    register char *copy = dst;

    do {
    } while (*copy++ != 0);
    copy -= 1;
    do {
    } while ((*copy++ = *src++) != 0);
    return dst;
}
