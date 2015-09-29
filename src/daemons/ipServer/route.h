/*
 * route.h --
 *
 *	Global declarations of the routing routines.
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
 *
 * $Header: /sprite/src/daemons/ipServer/RCS/route.h,v 1.4 89/08/15 19:55:36 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_ROUTE
#define _IPS_ROUTE


#include "sprite.h"
#include "net.h"

/*
 * Rte_NetID identifies the network interface that a  packet came in on
 * or should be sent to. The fields are private to the Route module.
 */

typedef struct {
    int         net;    /* Network interface number. */
    int         host;   /* Host number on that network. */
} Rte_NetID;

extern void		Rte_AddressInit();
extern Boolean		Rte_AddrIsForUs();
extern Boolean		Rte_FindOutputNet();
extern ReturnStatus	Rte_OutputPacket();
extern ReturnStatus	Rte_RegisterNet();
extern ReturnStatus	Rte_RegisterAddr();
extern unsigned int	Rte_GetNetNum();
extern Net_InetAddress	Rte_GetBroadcastAddr();
extern Net_InetAddress	Rte_GetOfficialAddr();
extern unsigned int	Rte_GetSubnetMask();
extern Boolean		Rte_IsBroadcastAddr();
extern Boolean		Rte_IsLocalAddr();


#endif /* _IPS_ROUTE */
