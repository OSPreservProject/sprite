/* 
 * isxdigit.c --
 *
 *	Contains the C library procedure "isxdigit".
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
static char rcsid[] = "$Header: isxdigit.c,v 1.1 88/04/27 18:03:42 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "ctype.h"
#undef isxdigit

/*
 *----------------------------------------------------------------------
 *
 * isxdigit --
 *
 *	Tell whether a character is a hexadecimal digit or not.
 *
 * Results:
 *	Returns non-zero if c is a hexadecimal digit, zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
isxdigit(c)
    int c;			/* Character value to test.  Must be an
				 * ASCII value or EOF. */
{
    return ((_ctype_bits+1)[c] & (CTYPE_DIGIT|CTYPE_HEX_DIGIT));
}
