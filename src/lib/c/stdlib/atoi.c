/* 
 * atoi.c --
 *
 *	Source code for the "atoi" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/atoi.c,v 1.2 89/03/22 00:46:58 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdlib.h>
#include <ctype.h>

/*
 *----------------------------------------------------------------------
 *
 * atoi --
 *
 *	Convert an ASCII string into an integer.
 *
 * Results:
 *	The return value is the integer equivalent of string.  If there
 *	are no decimal digits in string, then 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
atoi(string)
    register char *string;	/* String of ASCII digits, possibly
				 * preceded by white space.  For bases
				 * greater than 10, either lower- or
				 * upper-case digits may be used.
				 */
{
    register int result = 0;
    register unsigned int digit;
    int sign;

    /*
     * Skip any leading blanks.
     */

    while (isspace(*string)) {
	string += 1;
    }

    /*
     * Check for a sign.
     */

    if (*string == '-') {
	sign = 1;
	string += 1;
    } else {
	sign = 0;
	if (*string == '+') {
	    string += 1;
	}
    }

    for ( ; ; string += 1) {
	digit = *string - '0';
	if (digit > 9) {
	    break;
	}
	result = (10*result) + digit;
    }

    if (sign) {
	return -result;
    }
    return result;
}
