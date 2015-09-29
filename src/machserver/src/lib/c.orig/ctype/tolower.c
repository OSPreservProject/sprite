/* 
 * tolower.c --
 *
 *	Contains the C library procedure "tolower".
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
static char rcsid[] = "$Header: tolower.c,v 1.1 88/04/27 18:03:44 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "ctype.h"

/*
 *----------------------------------------------------------------------
 *
 * tolower --
 *
 *	Return the lower-case equivalent of a character.
 *
 * Results:
 *	If c is an upper-case character, then its lower-case equivalent
 *	is returned.  Otherwise c is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
tolower(c)
    int c;			/* Character value to convert.  Must be an
				 * ASCII value or EOF. */
{
    if isupper(c) {
	return c + 'a' - 'A';
    }
    return c;
}
