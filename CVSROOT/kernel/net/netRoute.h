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

#include <netInet.h>
/*
 * A Route: A mapping between a physical address and a Sprite Host ID.
 * The supported address type is just ethernet. Net_Routes are manipulated
 * by Net_InstallRoute and Net_AddrToID.  The main point of a Net_Route
 * is that it holds a pre-packaged transport header that is pre-pended
 * onto messages being sent to the Sprite Host.
 *
 * The second main point of this is to hold host-specific information
 * like its name and machine type.  The name is used for error reporting.
 * The machine type is queried by the file system when it has to expand
 * $MACHINE during pathname lookup.
 */
typedef struct Net_Route {
    int		flags;		/* values defined below */
    int		spriteID;	/* Universal Sprite ID */
    int		type;		/* values defined below */
    Address	data;		/* pointer to transport header */
    char 	*name;		/* Human recognizable name for the host */
    char	*machType;	/* Machine type used when expanding $MACHINE
				 * during pathname lookup */
} Net_Route;

/*
 * Types for the address union in the above struct.
 * NET_ROUTE_UNUSED	The route is empty or unitialized.
 * NET_ROUTE_GENERIC	The route contents are uninterpreted, (not implemented)
 * NET_ROUTE_ETHER	The route contains an ethernet address.  In this
 *			case the data of the Net_Route is a pointer to
 *			a Net_EtherHdr.
 * NET_ROUTE_INET	The route contains an Internet address.  In this
 *			case the data of the Net_Route is a pointer to 
 *			a Net_EtherHdr + Net_IPHeader.
 */
#define		NET_ROUTE_UNUSED	0x00
#define		NET_ROUTE_GENERIC	0x01
#define		NET_ROUTE_ETHER		0x02
#define		NET_ROUTE_INET		0x04

/*
 * Flags for the Route structure.
 * NET_ROUTE_BROAD	Using the route will result in a broadcast.
 */
#define		NET_ROUTE_BROAD		0x01
#define		NET_ROUTE_GATEWAY	0x02

/*
 * Define the special Sprite ID used for broadcasting.
 */
#define		NET_BROADCAST_HOSTID	0

/*
 * The routing table
 */
extern Net_Route *netRouteArray[];

/*
 * The routing information for an Internet route is the Internet
 * address and the ethernet address of the gateway machine.
 */
typedef struct NetInetRoute {
    Net_InetAddress inetAddr;
    Net_EtherAddress gatewayAddress;
} NetInetRoute;

/*
 * Sprite Address Resolution Protocol packet format.  These are used to
 * find out Sprite IDs for physical addresses and vice versa.  The ARP
 * protocol is simple:  a host broadcasts an Arp Request containing a
 * Sprite ID and waits for a reply that specifies the physical (ethernet)
 * address used to reach that Sprite host.  The Reverse Arp protocol is
 * similar.  A host broadcasts a request that contains a a physical
 * (ethernet) address and waits for a reply that specifies the matching
 * Sprite ID. 
 *
 * NOTE: This packet appears on the wire in network byte ordering.
 *
 */

#define	NUM_ARP_DATA_BYTES (2 * (sizeof(Net_EtherAddress) + sizeof(int)))

typedef struct NetSpriteArp {
    Net_ArpHeader arpHeader;	/* RFC826 standard header. The hardware addr
				 * space should be NET_ARP_TYPE_ETHER.  */
    unsigned char arpData[NUM_ARP_DATA_BYTES];
} NetSpriteArp;

/*
 * Macros for indexing into the arpData field.
 * ARP_SRC_ETHER_ADDR() - The address of the sender's ethernet address.
 * ARP_SRC_PROTO_ADDR() - The address of the sender's protocol address.
 * ARP_TARGET_ETHER_ADDR() - The address of the target's ethernet address.
 * ARP_TARGET_PROTO_ADDR() - The address of the target's protocol address.
 */

#define ARP_SRC_ETHER_ADDR(ap) ((char *) &((ap)->arpData[0]))
#define ARP_SRC_PROTO_ADDR(ap) \
		((char *) &((ap)->arpData[(ap)->arpHeader.hardwareAddrLen]))
#define	ARP_TARGET_ETHER_ADDR(ap) \
		((char *) &((ap)->arpData[(ap)->arpHeader.hardwareAddrLen + \
					 (ap)->arpHeader.protocolAddrLen]))
#define	ARP_TARGET_PROTO_ADDR(ap) \
		((char *) &((ap)->arpData[2*(ap)->arpHeader.hardwareAddrLen + \
					 (ap)->arpHeader.protocolAddrLen]))


/*
 * Forward declarations.
 */

extern void	 NetArpInput();

#endif /* _NETROUTE */
