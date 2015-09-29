/* 
 * toascii.c --
 *
 *	Contains the C library procedure "toascii".
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
static char rcsid[] = "$Header: /sprite/src/lib/c/ctype/RCS/toascii.c,v 1.1 89/06/13 16:44:02 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "ctype.h"

/*
 *----------------------------------------------------------------------
 *
 * toascii --
 *
 *	Return the ascii portion of a character.
 *
 * Results:
 *	The low-order 8 bits of the character are returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
toascii(c)
    int c;			/* Value to convert. */
{
    return (c & 0177);
}
