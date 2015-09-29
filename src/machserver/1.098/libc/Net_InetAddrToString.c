/* 
 * Net_InetAddrToString.c --
 *
 *	Convert an internet address to a string.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_InetAddrToString.c,v 1.2 90/09/11 14:43:44 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"
#include <stdio.h>


/*
 *----------------------------------------------------------------------
 *
 * Net_InetAddrToString --
 *
 *	Convert network-format internet address to base 256 d.d.d.d 
 *	representation.
 *
 * Results:
 *	Address of the string buffer.
 *
 * Side effects:
 *	The buffer is overwritten.
 *
 *----------------------------------------------------------------------
 */

char *
Net_InetAddrToString(address, buffer)
    Net_InetAddress address;
    char	*buffer;
{
    register char *p;
    Net_InetAddress tmp = address;

#define	UC(b)	(((int)b)&0xff)

    p = (char *)&tmp;
    sprintf(buffer, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
    return(buffer);
}

