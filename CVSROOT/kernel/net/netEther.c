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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
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

    NET_ETHER_ADDR_COPY(tmp, netEtherBroadcastAddress.ether);
}

