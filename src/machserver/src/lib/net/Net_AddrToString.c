/* 
 * Net_AddrToString.c --
 *
 *	Converts a Net_Address into a printable string.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_AddrToString.c,v 1.2 90/09/11 14:43:47 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdio.h>
#include <net.h>

/*
 *----------------------------------------------------------------------
 *
 * Net_AddrToString --
 *
 *	Converts a Net_Address into a printable string.
 *
 * Results:
 *	Pointer to the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char	*
Net_AddrToString(netAddressPtr, protocol, netType, buffer)
    Net_Address		*netAddressPtr;	/* Network address. */
    int			protocol;	/* Network protocol (eg raw, inet). */
    Net_NetworkType	netType;	/* Type of network (eg ether). */
    char		*buffer;	/* The string buffer. */
{
    *buffer = '\0';
    switch(protocol) {
	case NET_PROTO_RAW: {
	    switch(netType) {
		case NET_NETWORK_ETHER:
		    return Net_EtherAddrToString(&netAddressPtr->ether, buffer);
		    break;
		case NET_NETWORK_ULTRA: {
		    int		group;
		    int		unit;
		    Net_UltraAddressGet(&netAddressPtr->ultra, &group, &unit);
		    sprintf(buffer, "%d/%d", group, unit);
		    break;
		}
		default:
		    return buffer;
	    }
	    break;
	}
	case NET_PROTO_INET: {
	    return Net_InetAddrToString(netAddressPtr->inet, buffer);
	    break;
	}
	default:
	    return buffer;
	    break;
    }
    return buffer;
}

