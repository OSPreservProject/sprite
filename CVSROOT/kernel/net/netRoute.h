/*
 * netRoute.h --
 *
 *	Definitions for the routing part of the network module.
 *	Other modules specify a Sprite host ID when sending messages
 *	via the net module.  The net module maintains routing
 *	information that maps from these Sprite host IDs to physical addresses.
 *
 * Copyright 1986, 1988 Regents of the University of California
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

#ifndef _NETROUTE
#define _NETROUTE

#ifdef KERNEL
#include <sprite.h>
#include <list.h>
#include <netInet.h>
#include <user/net.h>
#include <netTypes.h>
#else
#include <sprite.h>
#include <list.h>
#include <netInet.h>
#include <net.h>
#include <kernel/netTypes.h>
#endif

/*
 * Maximum size for all headers of a packet.  We allow 8 extra bytes for
 * alignment purposes.
 */
#define NET_MAX_HEADER_SIZE (sizeof(Net_UltraHeader) + sizeof(Net_IPHeader) + 8)

/*
 * A Route: A mapping between a physical address and a Sprite Host ID.
 * The supported address type is just ethernet. Net_Routes are manipulated
 * by Net_InstallRoute and Net_AddrToID.  The main point of a Net_Route
 * is that it holds a pre-packaged transport header that is pre-pended
 * onto messages being sent to the Sprite Host.
 *
 */
typedef struct Net_Route {
    List_Links		links;		/* Used to add routes to a list. */
    int			routeID;	/* ID unique to this route. */
    int			protocol;	/* see values defined below */
    Net_Address		netAddress[NET_MAX_PROTOCOLS];/* host addresses */
    int			spriteID;	/* Universal Sprite ID */
    int			flags;		/* See below. */
    int			refCount;	/* Reference count. */
    char		desc[64];	/* Route description.  Useful
					 * for debugging. */
    Address		headerPtr[NET_MAX_PROTOCOLS]; /* Start of transport 
						       * headers*/
    Net_Interface	*interPtr;	/* Which network interface to use. */
    int			maxBytes;	/* Maximum transfer unit of route. */
    int			minBytes;	/* Minimum transfer unit of route. */
    ClientData		userData;	/* Space available for user program
					 * that manipulates routes. */
    char		buffer[NET_MAX_HEADER_SIZE];  /* Network packet 
						       * header(s). */
} Net_Route;

/*
 * Flag values for Net_Route.
 */

#define NET_RFLAGS_VALID	0x1	/* The route is valid. */

/*
 * The following two constants define the minimum and maximum
 * number of free routes on the free list.  Once the number drops
 * below the minimum we add routes to the list until there are the
 * maximum.  Make sure that the difference between the minimum and
 * maximum is enough to allocate all of the broadcast routes during
 * initialization since the callback stuff is initialized later.
 */

#define NET_MIN_FREE_ROUTES 8
#define NET_MAX_FREE_ROUTES (NET_MIN_FREE_ROUTES + NET_MAX_INTERFACES + 2)

/*
 *  Variables corresponding to the above two constants.
 */

extern	int	netMinFreeRoutes;
extern	int	netMaxFreeRoutes;

/*
 * This structure contains host information that is common to all routes
 * to the host. The name is used for error reporting.
 * The machine type is queried by the file system when it has to expand
 * $MACHINE during pathname lookup.
 */

typedef struct NetHostInfo {
    char	name[20];		/* The host name. */
    char	machType[12];		/* Host machine type. */
} NetHostInfo;

/*
 * The routing table
 */
extern List_Links netRouteArray[];
extern NetHostInfo netHostInfo[];


/*
 * Forward declarations.
 */

extern void NetArpInput _ARGS_((Net_Interface *interPtr, Address packetPtr, 
				int packetLength));
extern void NetAddToFreeRouteList _ARGS_((ClientData data, 
				Proc_CallInfo *infoPtr));
extern void NetFreeRoute _ARGS_((Net_Route *routePtr));

#endif /* _NETROUTE */
