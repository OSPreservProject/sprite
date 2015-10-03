/*
 * netConfig.c --
 *
 *	Machine-type dependent initialization of the network interfaces.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#include <sprite.h>
#include <net.h>
#include <netInt.h>
#include <netLEInt.h>
#include <machAddrs.h>

Net_Interface netConfigInterfaces[] = {
    {"LE", 0, (Address) MACH_NETWORK_INTERFACE_ADDR, TRUE, 1, NetLEInit}
};
int netNumConfigInterfaces = 
	    sizeof(netConfigInterfaces) / sizeof(Net_Interface);
