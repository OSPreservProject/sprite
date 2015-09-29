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
static char rcsid[] = "$Header: /sprite/src/lib/c/netlib/RCS/Net_AddrToString.c,v 1.6 92/06/09 20:54:01 jhh Exp $ SPRITE (Berkeley)";
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
Net_AddrToString(netAddressPtr, buffer)
    Net_Address		*netAddressPtr;	/* Network address. */
    char		*buffer;	/* The string buffer. */
{
    ReturnStatus status = SUCCESS;
    *buffer = '\0';
    switch(netAddressPtr->type) {
	case NET_ADDRESS_ETHER: {
	    Net_EtherAddress	etherAddress;
	    status = Net_GetAddress(netAddressPtr, (Address) &etherAddress);
	    if (status != SUCCESS) {
		break;
	    }
	    buffer = Net_EtherAddrToString(&etherAddress, buffer);
	    break;
	}
	case NET_ADDRESS_ULTRA: {
	    Net_UltraAddress	ultraAddress;
	    status = Net_GetAddress(netAddressPtr, (Address) &ultraAddress);
	    if (status != SUCCESS) {
		break;
	    }
	    buffer = Net_UltraAddrToString(&ultraAddress, buffer);
	    break;
	}
	case NET_ADDRESS_FDDI: {
	    Net_FDDIAddress	fddiAddress;
	    status = Net_GetAddress(netAddressPtr, (Address) &fddiAddress);
	    if (status != SUCCESS) {
		break;
	    }
	    buffer = Net_FDDIAddrToString(&fddiAddress, buffer);
	    break;
	}
	case NET_ADDRESS_INET: {
	    Net_InetAddress	inetAddress;
	    status = Net_GetAddress(netAddressPtr, (Address) &inetAddress);
	    if (status != SUCCESS) {
		break;
	    }
	    buffer = Net_InetAddrToString(inetAddress, buffer);
	    break;
	}
    }
    if (status != SUCCESS) {
#ifdef KERNEL
	return (char*) NIL;
#else
	return NULL;
#endif
    }
    return buffer;
}

