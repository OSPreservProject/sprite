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
 * $Header: /user5/kupfer/spriteserver/src/sprited/net/sun3.md/RCS/netConfig.c,v 1.3 92/04/02 21:32:30 kupfer Exp $ SPRITE (Berkeley)
 */

#include <sprite.h>
#include <mach.h>
#include <netTypes.h>
#include <netInt.h>

Net_Interface netConfigInterfaces[] = {
    {"ie", 0, NULL, MACH_PORT_NULL, MACH_PORT_NULL, NetEtherInitInterface},
    {"le", 0, NULL, MACH_PORT_NULL, MACH_PORT_NULL, NetEtherInitInterface},
};
int netNumConfigInterfaces = 
	    sizeof(netConfigInterfaces) / sizeof(Net_Interface);

