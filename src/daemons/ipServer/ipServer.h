/*
 * ipServer.h --
 *
 *	Global declarations of the Internet protocol server process.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/ipServer.h,v 1.7 89/08/15 19:55:32 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_IPSERVER
#define _IPS_IPSERVER

#include "sprite.h"
#include "status.h"
#include "stdlib.h"
#include "net.h"
#ifndef KERNEL
/*
 * We can only use the regular C library stdio in the user-level ipServer.
 * The kernel has its own printf routines.
 */
#include <stdio.h>
#else
#define stderr	0
#define stdout 0
typedef int FILE;
#endif

/* macros and constants */

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

/*
 * Internal representations of the protocol numbers.
 * The protocol numbers in netInet.h are not contiguous and too spread
 * out to be useful. If you add a protocol here, be sure to update
 * the Sock_Init routine in sockOps.c.
 */
#define RAW_PROTO_INDEX		0
#define UDP_PROTO_INDEX		1
#define TCP_PROTO_INDEX		2
#define MAX_PROTO_INDEX		TCP_PROTO_INDEX


/* data structures */


/*
 * The IPS_Packet structure describes the format of a memory buffer that
 * holds an incoming or outgoing packet. Every packet has an network header,
 * an IP header, possibly a header for a higher-level protocol, and data.
 * For an incoming packet, the headers begin at the base address of the
 * buffer. For an outgoing packet, the headers are added "bottom-up" so
 * there might be a gap of unused memory between the base and the network
 * header. The routine that allocates an outgoing packet must reserve enough
 * space to insert all the headers.
 *
 * The network header is not touched by the protocol routines. Only the
 * I/O routine is allowed to manipulate it.
 */

typedef struct {
    int			totalLen;	/* Total length (in bytes) of the
					 * buffer. */
    Address		base;		/* Base address returned by Mem_Alloc.
					 * This is the address that is given
					 * to Mem_Free to release the buffer. */
    Address		dbase;		/* Base address of data in buffer. */
    /*
     * The following pointers point to addresses within the buffer that
     * starts at base and ends at (base + totalLen - 1).
     */

    union {
	Net_EtherHdr	*etherPtr;	/* Ptr. to the ethernet header. */
	/*
	 * Ptrs to headers for other network types go here.
	 *   NOTE: If you add a field here, add a corresponding field in
	 *         IPS_PacketNetHdr below.
	 */
    } net;

    int			ipLen;		/* Length in bytes of the IP header. */
    Net_IPHeader	*ipPtr;		/* Ptr to the IP header. */
    int			hdrLen;		/* Length in bytes of the higher-level
					 * protocol header. May be 0. */
    union {				/* Ptr. to the HL protocol header. */
	Net_UDPHeader	*udpPtr;
	Net_TCPHeader	*tcpPtr;
	Net_ICMPHeader	*icmpPtr;
	Address		*hdrPtr;
    } hdr;
    int			dataLen;	/* Length in bytes of the data.
					 * May be 0. */
    Address		data;		/* Ptr to the data. */
} IPS_Packet;

/*
 * IPS_ROOM_FOR_HEADERS defines how much space might be used by the
 * protocol headers.  This is used to overallocate space so that the
 * packet data may be left in place during packet formatting.
 */
#define IPS_ROOM_FOR_HEADERS	( (NET_TCP_MAX_HDR_SIZE + \
				   NET_IP_MAX_HDR_SIZE + \
				   sizeof(IPS_PacketNetHdr) + \
				   sizeof(int)) & ~(sizeof(int) - 1) )
/*
 * This type is used by routines when they need to allocate space in an
 * IPS_Packet for the network header in a network-independent way.
 * Usage: sizeof(IPS_PacketNetHdr);
 */

typedef union {
    Net_EtherHdr	ether;		/* Ethernet header. */
    /*
     * Headers for other network types go here.
     *   NOTE: If you add a field here, add a corresponding field in
     *         net union in IPS_Packet above.
     */

} IPS_PacketNetHdr;

/* procedures and variables */

extern Boolean		ips_Debug;

extern int		IPS_GetTimeStamp();
extern void		IPS_InitPacket();

#endif /* _IPS_IPSERVER */
