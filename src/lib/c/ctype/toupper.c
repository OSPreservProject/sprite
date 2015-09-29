/* 
 * toupper.c --
 *
 *	Contains the C library procedure "toupper".
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
static char rcsid[] = "$Header: toupper.c,v 1.2 88/09/14 17:18:20 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "ctype.h"

/*
 *----------------------------------------------------------------------
 *
 * toupper --
 *
 *	Return the upper-case equivalent of a character.
 *
 * Results:
 *	If c is an lower-case character, then its upper-case equivalent
 *	is returned.  Otherwise c is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
toupper(c)
    int c;			/* Character value to convert.  Must be an
				 * ASCII value or EOF. */
{
    if islower(c) {
	return c + 'A' - 'a';
    }
    return c;
}
