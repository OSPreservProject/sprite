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

#include "sprite.h"
#include "net.h"
#include "netInt.h"
#include "netLEInt.h"

NetInterface netInterface[] = {
    {"LE", 0, NET_LE_CONTROL_REG_ADDR, NetLEInit}
};
int numNetInterfaces = sizeof(netInterface) / sizeof(NetInterface);

