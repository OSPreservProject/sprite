/* 
 * Net_StringToAddr.c --
 *
 *	Convert a string to an address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_StringToAddr.c,v 1.3 90/08/31 15:05:44 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_StringToAddr --
 *
 *	This routine takes a string form of a network address and
 *	converts it to the Net_Address form. The string must be
 *	null-terminated.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_StringToAddr(buffer, protocol, netType, addressPtr)
    register char 	*buffer;
    int			protocol;
    Net_NetworkType	netType;
    Net_Address 	*addressPtr;
{
    ReturnStatus	status = SUCCESS;

    switch(protocol) {
	case NET_PROTO_RAW : {
	    switch(netType) {
		case NET_NETWORK_ETHER:
		    Net_StringToEtherAddr(buffer, &addressPtr->ether);
		    break;
		case NET_NETWORK_ULTRA: {
		    int	group;
		    int unit;
		    int n;
		    n = sscanf(buffer, "%d/%d", &group, &unit);
		    if (n != 2) {
			return FAILURE;
		    }
		    Net_UltraAddressSet(&addressPtr->ultra, group, unit);
		    break;
		}
		default:
		    return FAILURE;
	    }
	    break;
	}
	case NET_PROTO_INET : {
	    addressPtr->inet = Net_StringToInetAddr(buffer);
	    break;
	}
    }
    return status;
}

