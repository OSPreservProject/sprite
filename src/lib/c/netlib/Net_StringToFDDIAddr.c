/* 
 * Net_StringToFDDIAddr.c --
 *
 *	Convert a string to an FDDI address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_StringToFDDIAddr.c,v 1.2 88/11/21 09:28:33 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_StringToFDDIAddr --
 *
 *	This routine takes a string form of an FDDI address and
 *	converts it to the Net_FDDIAddress form. The string must be
 *	null-terminated.
 *
 * Results:
 *	The FDDInet address in the Net_FDDIAddress form.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Net_StringToFDDIAddr(buffer, fddiAddressPtr)
    register char *buffer;
    Net_FDDIAddress *fddiAddressPtr;
{
    unsigned int byte[6];

    if (sscanf(buffer, "%2x:%2x:%2x:%2x:%2x:%2x",
	    &byte[0], &byte[1], &byte[2], &byte[3], &byte[4], &byte[5]) != 6) {
	bzero((Address)fddiAddressPtr, sizeof(Net_FDDIAddress) );
    } else {
	NET_FDDI_ADDR_BYTE1(*fddiAddressPtr) = byte[0];
	NET_FDDI_ADDR_BYTE2(*fddiAddressPtr) = byte[1];
	NET_FDDI_ADDR_BYTE3(*fddiAddressPtr) = byte[2];
	NET_FDDI_ADDR_BYTE4(*fddiAddressPtr) = byte[3];
	NET_FDDI_ADDR_BYTE5(*fddiAddressPtr) = byte[4];
	NET_FDDI_ADDR_BYTE6(*fddiAddressPtr) = byte[5];
    }
}

