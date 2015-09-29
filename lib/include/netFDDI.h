/*
 * netFDDI.h --
 *
 *	This defines the format of an FDDI packet.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/netFDDI.h,v 1.6 92/06/05 12:33:43 voelker Exp $
 */

#ifndef _NETFDDI
#define _NETFDDI


#include "machparam.h"

/*
 * Compare two FDDI addresses.
 */

#define	Net_FDDIAddrCmp(e1,e2) Net_FDDIAddrCmpPtr(&e1,&e2)

/*
 * Compare bytes backward because FDDI addresses tend to start with the
 * same few bytes.
 */

#define	Net_FDDIAddrCmpPtr(e1,e2) \
   (!(((e1)->byte6 == (e2)->byte6) && ((e1)->byte5 == (e2)->byte5) && \
    ((e1)->byte4 == (e2)->byte4) && ((e1)->byte3 == (e2)->byte3) && \
    ((e1)->byte2 == (e2)->byte2) && ((e1)->byte1 == (e2)->byte1)))

/*
 * FDDI Address - 6 bytes
 */

typedef struct {
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
    unsigned char byte4;
    unsigned char byte5;
    unsigned char byte6;
} Net_FDDIAddress;

#define	NET_FDDI_ADDR_BYTE1(e)	((e).byte1)
#define	NET_FDDI_ADDR_BYTE2(e)	((e).byte2)
#define	NET_FDDI_ADDR_BYTE3(e)	((e).byte3)
#define	NET_FDDI_ADDR_BYTE4(e)	((e).byte4)
#define	NET_FDDI_ADDR_BYTE5(e)	((e).byte5)
#define	NET_FDDI_ADDR_BYTE6(e)	((e).byte6)

#ifdef sun4
#define	NET_FDDI_ADDR_COPY(src,dst)	\
	((dst).byte1 = (src).byte1);	\
	((dst).byte2 = (src).byte2);	\
	((dst).byte3 = (src).byte3);	\
	((dst).byte4 = (src).byte4);	\
	((dst).byte5 = (src).byte5);	\
	((dst).byte6 = (src).byte6)
#else
#define	NET_FDDI_ADDR_COPY(src,dst) ((dst) = (src))
#endif

/*
 * FDDI Header - 16 bytes 
 */

typedef struct Net_FDDIHdr {
    unsigned char   prh[3];        /* Packet request headers */
    unsigned char   frameControl;  /* Frame control byte */
    Net_FDDIAddress dest;          /* Destination Address of packet */
    Net_FDDIAddress source;        /* Source Address of packet */
} Net_FDDIHdr;

#define	NET_FDDI_HDR_DESTINATION(e)	((e).destination)
#define	NET_FDDI_HDR_SOURCE(e)		((e).source)
#define	NET_FDDI_HDR_TYPE(e)		((e).type)

#define	NET_FDDI_HDR_DESTINATION_PTR(e)	&((e).destination)
#define	NET_FDDI_HDR_SOURCE_PTR(e)	&((e).source)
#define	NET_FDDI_HDR_TYPE_PTR(e)	&((e).type)

#define	NET_FDDI_HDR_COPY(src, dst) ((dst) = (src))

/*
 * The docs say that the minimum for an LLC packet is 20.
 */
#define	NET_FDDI_MIN_BYTES	20
#define	NET_FDDI_MAX_BYTES	4480

/*
 * This value is used to distinguish Sprite FDDI packets from other
 * FDDI packets on the same ring.  This right here is a major kludge
 * until we can get the Data Link Specification.  This actually is an LLC
 * of priority 3 frame control, but oh well.  We should be using the Data
 * Link Specs, but we never got ahold of them.
 *
 * This value is placed in the header of each packet in 
 * netRoute.c:Net_InstallRoute().
 *
 * FDDI packets are checked for this value in netDFRecv.c:GotAPacket().
 */

#define NET_FDDI_SPRITE                 0x53

/*
 * Definitions of known Ethernet packet types (from rfc990, except for SPRITE
 * and TRAIL).
 *
 * These mean nothing to the FDDI driver until encapsulated ethernet
 * is attempted.
 */
#define NET_FDDI_PUP			0x0200
#define NET_FDDI_PUP_ADDR_TRANS		0x0201
#define NET_FDDI_XNS_IDP		0x0600
#define NET_FDDI_IP			0x0800
#define NET_FDDI_ARP			0x0806
#define NET_FDDI_XNS_COMPAT		0x0807
/* #define NET_FDDI_SPRITE		0x0500 */
#define NET_FDDI_SPRITE_ARP		0x0502 /* deprecated */
#define NET_FDDI_SPRITE_DEBUG		0x0504
#define NET_FDDI_TRAIL			0x1000
#define NET_FDDI_REVARP			0x8035
#define NET_FDDI_MOP			0x6001

#endif /* _NETFDDI */




