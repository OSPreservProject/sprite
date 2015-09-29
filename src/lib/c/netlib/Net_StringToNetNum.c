/* 
 * Net_StringToNetNum.c --
 *
 *	Convert a string to the network number part of the address.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_StringToNetNum.c,v 1.1 88/11/21 09:10:32 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"
#include "ctype.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_StringToNetNum --
 *
 *	Given a string containing an IP address in the Internet standard 
 *	"." notation, return  the network number for that address.
 *
 * Results:
 *	The network number of the address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Net_StringToNetNum(cp)
    register char *cp;
{
    register unsigned int value;
    register unsigned int base;
    unsigned int parts[4];
    register unsigned int *partsPtr = parts;
    register char c;
    register int i;
    unsigned int n;

again:
    value = 0; 
    base = 10;
    if (*cp == '0') {
	base = 8;
	cp++;
    }
    if (*cp == 'x' || *cp == 'X') {
	base = 16;
	cp++;
    }

    c = *cp;
    while (c != '\0') {
	if (isdigit(c)) {
	    value = (value * base) + (c - '0');
	} else if (base == 16 && isxdigit(c)) {
	    value = (value << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
	} else {
	    break;
	}
	cp++;
	c = *cp;
    }

    if (*cp == '.') {
	if (partsPtr >= parts + 4) {
	    return(-1);
	}
	*partsPtr = value;
	partsPtr++;
	cp++;
	goto again;
    }

    if (*cp != '\0' && !isspace(*cp)) {
	return(-1);
    }

    *partsPtr = value;
    partsPtr++;

    n = partsPtr - parts;
    if (n > 4) {
	return(-1);
    }
    for (value = 0, i = 0; i < n; i++) {
	    value <<= 8;
	    value |= parts[i] & 0xff;
    }
    return(value);
}

