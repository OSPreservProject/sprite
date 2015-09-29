/* 
 * Net_MakeInetAddr.c --
 *
 *	Create an internet address from a host and network number.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_MakeInetAddr.c,v 1.1 88/11/21 09:10:19 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"


/*
 *----------------------------------------------------------------------
 *
 * Net_MakeInetAddr --
 *
 *	Formulate an Internet address from network and host numbers.  
 *
 * Results:
 *	An IP address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_InetAddress
Net_MakeInetAddr(net, host)
    unsigned int net, host;
{
    Net_InetAddress addr;

    if (net < 128) {
	addr =(net << NET_INET_CLASS_A_SHIFT) | 
				(host & NET_INET_CLASS_A_HOST_MASK);
    } else if (net < 65536) {
	addr =(net << NET_INET_CLASS_B_SHIFT) | 
				(host & NET_INET_CLASS_B_HOST_MASK);
    } else {
	addr =(net << NET_INET_CLASS_C_SHIFT) | 
				(host & NET_INET_CLASS_C_HOST_MASK);
    }
    return(Net_HostToNetInt(addr));
}

