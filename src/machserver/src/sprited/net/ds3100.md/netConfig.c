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
 * $Header: /user5/kupfer/spriteserver/src/sprited/net/ds3100.md/RCS/netConfig.c,v 1.3 92/04/02 21:32:57 kupfer Exp $ SPRITE (DECWRL)
 */

#include <sprite.h>
#include <mach.h>
#include <netTypes.h>
#include <netInt.h>

/* 
 * Mach uses "SE0" for the interface installed in the higher-numbered slot 
 * in the XPRS ds5000's.  We want the interface installed in slot 1 or 2, 
 * which Mach calls "SE1".
 */
Net_Interface netConfigInterfaces[] = {
    {"SE", 0, "SE1", MACH_PORT_NULL, MACH_PORT_NULL, NetEtherInitInterface}
};
int netNumConfigInterfaces = 
	    sizeof(netConfigInterfaces) / sizeof(Net_Interface);
