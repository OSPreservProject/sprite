/* 
 * Net_StringToInetAddr.c --
 *
 *	Convert a string to an internet address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_StringToInetAddr.c,v 1.1 88/11/21 09:10:29 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_StringToInetAddr --
 *
 *	This routine takes a string form of an Internet address and
 *	converts it to the Net_InetAddress form. The string must be
 *	null-terminated.
 *
 *	"All the network library routines call this routine to interpret 
 *	entries in the data bases which are expected to be an address.
 * 	The value returned is in network order."
 *
 * Results:
 *	The Internet address in the Net_InetAddress form.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_InetAddress
Net_StringToInetAddr(cp)
    register char *cp;
{
    register unsigned int value;
    register unsigned int base;
    register char c;
    unsigned int parts[4];
    register unsigned int *partsPtr = parts;

again:
    /*
     * Collect number up to ``.''.
     * Values are specified as for C: 0x=hex, 0=octal, other=decimal.
     */

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
	/*
	 * Internet format:
	 *	a.b.c.d
	 *	a.b.c	(with c treated as 16-bits)
	 *	a.b	(with b treated as 24 bits)
	 */
	if (partsPtr >= parts + 4) {
	    return(NET_INET_BROADCAST_ADDR);
	}
	*partsPtr = value;
	partsPtr++;
	cp++;
	goto again;
    }

    /*
     * Check for trailing characters.
     */
    if ((*cp != '\0') && (!isspace(*cp))) {
	return(NET_INET_BROADCAST_ADDR);
    }
    *partsPtr = value;
    partsPtr++;

    /*
     * Concoct the address according to the number of parts specified.
     */

    switch (partsPtr - parts) {

	case 1:				/* a -- 32 bits */
		value = parts[0];
		break;

	case 2:				/* a.b -- 8.24 bits */
		value = (parts[0] << 24) | (parts[1] & 0xffffff);
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		value = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
			(parts[2] & 0xffff);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		value = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
		      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
		break;

	default:
		return(NET_INET_BROADCAST_ADDR);
    }
    return(Net_HostToNetInt(value));
}

