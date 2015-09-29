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
static char rcsid[] = "$Header: /sprite/src/lib/c/netlib/RCS/Net_StringToAddr.c,v 1.5 92/06/09 20:53:05 jhh Exp $ SPRITE (Berkeley)";
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
Net_StringToAddr(buffer, type, addressPtr)
    register char 	*buffer;
    Net_AddressType	type;
    Net_Address 	*addressPtr;
{
    ReturnStatus	status = SUCCESS;
    int			addr[10];

    switch(type) {
	case NET_ADDRESS_ETHER : {
	    Net_StringToEtherAddr(buffer, (Net_EtherAddress *) addr);
	    break;
	}
	case NET_ADDRESS_ULTRA: {
	    Net_StringToUltraAddr(buffer, (Net_UltraAddress *) addr);
	    break;
	}
	case NET_ADDRESS_FDDI: {
	    Net_StringToFDDIAddr(buffer, (Net_FDDIAddress *) addr);
	    break;
	}
	case NET_ADDRESS_INET : {
	    addr[0] = Net_StringToInetAddr(buffer);
	    break;
	}
	default:
	    return FAILURE;
    }
    status = Net_SetAddress(type, (Address) addr, addressPtr); 
    return status;
}
