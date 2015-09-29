/* 
 * netEther.c --
 *
 *	Contains ethernet specific definitions.
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
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/net/netEther.c,v 1.3 92/06/03 22:47:54 voelker Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <netInt.h>


Net_Address	netEtherBroadcastAddress;


/*
 *----------------------------------------------------------------------
 *
 * NetEtherInit --
 *
 *	Initializes stuff for the ethernet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
NetEtherInit()
{
    static Net_EtherAddress	tmp = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    ReturnStatus status;

    status = Net_SetAddress(NET_ADDRESS_ETHER, (Address) &tmp, 
	&netEtherBroadcastAddress);
    if (status != SUCCESS) {
	panic("NetEtherInit: Net_SetAddress failed\n");
    }
}

