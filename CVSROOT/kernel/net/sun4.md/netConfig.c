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
#include "netIEInt.h"

NetInterface netInterface[] = {
    {"IE", 0, NET_IE_CONTROL_REG_ADDR, NetIEInit},
};
int numNetInterfaces = sizeof(netInterface) / sizeof(NetInterface);

