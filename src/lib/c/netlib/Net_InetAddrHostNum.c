/* 
 * Net_InetAddrHostNum.c --
 *
 *	Extract the host part of an internet address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_InetAddrHostNum.c,v 1.1 88/11/21 09:10:12 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_InetAddrHostNum --
 *
 *	Return the host portion of an Internet address.
 *	Handles class A/B/C network formats.
 *
 * Results:
 *	The host portion an IP address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Net_InetAddrHostNum(inetAddr)
    Net_InetAddress inetAddr;
{
    register Net_InetAddress i = Net_NetToHostInt(inetAddr);

    if (NET_INET_CLASS_A_ADDR(i)) {
	return((i) & NET_INET_CLASS_A_HOST_MASK);
    } else if (NET_INET_CLASS_B_ADDR(i)) {
	return((i) & NET_INET_CLASS_B_HOST_MASK);
    } else {
	return((i) & NET_INET_CLASS_C_HOST_MASK);
    }
}

