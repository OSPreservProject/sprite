/*
 * netConfig.c --
 *
 *	Machine-type dependent initialization of the network interfaces.
 *
 * Copyright (C) 1987 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#include "sprite.h"
#include "net.h"
#include "netInt.h"
#include "netLEInt.h"

/*
 * On the sparcstations the control register isn't mapped by the prom.
 * It is at physical address 0x8c00000.
 */

Net_Interface netConfigInterfaces[] = {
    {"LE", 0, (Address) 0x8c00000, FALSE, 5, NetLEInit}
};
int netNumConfigInterfaces = 
	    sizeof(netConfigInterfaces) / sizeof(Net_Interface);

