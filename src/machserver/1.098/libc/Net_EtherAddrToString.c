/* 
 * Net_EtherAddrToString.c --
 *
 *	Convert an ethernet address to a string.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_EtherAddrToString.c,v 1.2 90/09/11 14:43:43 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include <stdio.h>
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_EtherAddrToString --
 *
 *	Convert Ethernet address to printable representation.
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
Net_EtherAddrToString(etherAddrPtr, buffer)
    register Net_EtherAddress *etherAddrPtr;
    char buffer[18];
{

    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
	NET_ETHER_ADDR_BYTE1(*etherAddrPtr) & 0xff, 
	NET_ETHER_ADDR_BYTE2(*etherAddrPtr) & 0xff, 
	NET_ETHER_ADDR_BYTE3(*etherAddrPtr) & 0xff, 
	NET_ETHER_ADDR_BYTE4(*etherAddrPtr) & 0xff, 
	NET_ETHER_ADDR_BYTE5(*etherAddrPtr) & 0xff, 
	NET_ETHER_ADDR_BYTE6(*etherAddrPtr) & 0xff
    );
    return(buffer);
}

