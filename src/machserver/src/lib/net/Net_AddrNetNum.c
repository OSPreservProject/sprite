/* 
 * Net_InetAddrNetNum.c --
 *
 *	Return the network part of an internet address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_AddrNetNum.c,v 1.1 88/11/21 09:09:53 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_InetAddrNetNum --
 *
 *	Return the network portion of an internet address.
 *	Handles class A/B/C network #'s.
 *
 * Results:
 *	The network number of an IP address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Net_InetAddrNetNum(addr)
    Net_InetAddress addr;
{
    register Net_InetAddress i = Net_NetToHostInt(addr);

    if (NET_INET_CLASS_A_ADDR(i)) {
	return (((i) & NET_INET_CLASS_A_NET_MASK) >> NET_INET_CLASS_A_SHIFT);
    } else if (NET_INET_CLASS_B_ADDR(i)) {
	return (((i) & NET_INET_CLASS_B_NET_MASK) >> NET_INET_CLASS_B_SHIFT);
    } else {
	return (((i) & NET_INET_CLASS_C_NET_MASK) >> NET_INET_CLASS_C_SHIFT);
    }
}

