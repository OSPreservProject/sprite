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

#include <sprite.h>
#include <net.h>
#include <netInt.h>
#include <netLEInt.h>
#include <machAddrs.h>

/*
 * Fields are name, unit, slot, virtual flag, vector, and init routine.
 * For the ds5000 only the slot and init routine are used.
 */
Net_Interface netConfigInterfaces[] = {
    {"LE", 0, (Address) 0, TRUE, -1, NetLEInit},
    {"LE", 0, (Address) 1, TRUE, -1, NetLEInit},
    {"LE", 0, (Address) 2, TRUE, -1, NetLEInit},
    {"LE", 0, (Address) 6, TRUE, -1, NetLEInit},
};
int netNumConfigInterfaces = 
	    sizeof(netConfigInterfaces) / sizeof(Net_Interface);

