/*
 * netEther.h --
 *
 *	This defines the format of an ethernet packet.
 *
 *	
 * 	The symbol NET_ETHER_BAD_ALIGNMENT must defined for machines that
 *	pad or otherwise mess up the layout of the Net_EtherHdr structure.
 *	This must be defined for machines such as the SPUR where structures
 *	are padded to 4 byte boundries.  To test a machine to see if 
 *	NET_ETHER_BAD_ALIGNMENT is needed, check sizeof(Net_EtherHdr).  If
 *	this number is anything other than the size of an ethernet header 
 *	(14 bytes), NET_ETHER_BAD_ALIGNMENT must be defined in the 
 *	machparam.h file for the machine.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 *
 * $Header: /sprite/src/lib/include/RCS/netEther.h,v 1.15 92/08/05 16:34:40 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _NETETHER
#define _NETETHER


#include "machparam.h"

/*
 * Compare two ethernet addresses.
 */

#ifndef NET_ETHER_BAD_ALIGNMENT

#define	Net_EtherAddrCmp(e1,e2) Net_EtherAddrCmpPtr(&e1,&e2)

/*
 * Compare bytes backward because ethernet address tend to start with the
 * same few bytes.
 */

#define	Net_EtherAddrCmpPtr(e1,e2) \
   (!(((e1)->byte6 == (e2)->byte6) && ((e1)->byte5 == (e2)->byte5) && \
    ((e1)->byte4 == (e2)->byte4) && ((e1)->byte3 == (e2)->byte3) && \
    ((e1)->byte2 == (e2)->byte2) && ((e1)->byte1 == (e2)->byte1)))

#else 	/* NET_ETHER_BAD_ALIGNMENT */


#define	Net_EtherAddrCmp(e1,e2) (bcmp((e1),(e2), sizeof(Net_EtherAddress)))

#define	Net_EtherAddrCmpPtr(e1Ptr,e2Ptr) Net_EtherAddrCmp(*(e1Ptr),*(e2Ptr))

#endif /* NET_ETHER_BAD_ALIGNMENT */


/*
 * Ethernet Address - 6 bytes
 */
#ifndef NET_ETHER_BAD_ALIGNMENT

typedef struct {
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
    unsigned char byte4;
    unsigned char byte5;
    unsigned char byte6;
} Net_EtherAddress;

#define	NET_ETHER_ADDR_BYTE1(e)	((e).byte1)
#define	NET_ETHER_ADDR_BYTE2(e)	((e).byte2)
#define	NET_ETHER_ADDR_BYTE3(e)	((e).byte3)
#define	NET_ETHER_ADDR_BYTE4(e)	((e).byte4)
#define	NET_ETHER_ADDR_BYTE5(e)	((e).byte5)
#define	NET_ETHER_ADDR_BYTE6(e)	((e).byte6)

#ifdef sun4
#define	NET_ETHER_ADDR_COPY(src,dst)	\
	((dst).byte1 = (src).byte1);	\
	((dst).byte2 = (src).byte2);	\
	((dst).byte3 = (src).byte3);	\
	((dst).byte4 = (src).byte4);	\
	((dst).byte5 = (src).byte5);	\
	((dst).byte6 = (src).byte6)
#else
#define	NET_ETHER_ADDR_COPY(src,dst) ((dst) = (src))
#endif

#else 

typedef unsigned char Net_EtherAddress[6];

#define	NET_ETHER_ADDR_BYTE1(e)	((e)[0])
#define	NET_ETHER_ADDR_BYTE2(e)	((e)[1])
#define	NET_ETHER_ADDR_BYTE3(e)	((e)[2])
#define	NET_ETHER_ADDR_BYTE4(e)	((e)[3])
#define	NET_ETHER_ADDR_BYTE5(e)	((e)[4])
#define	NET_ETHER_ADDR_BYTE6(e)	((e)[5])

#define	NET_ETHER_ADDR_COPY(src,dst) \
			(bcopy((src),(dst),sizeof(Net_EtherAddress)))

#endif /* NET_ETHER_BAD_ALIGNMENT */

/*
 * Ethernet Header.- 14 bytes 
 */

#ifndef NET_ETHER_BAD_ALIGNMENT

typedef struct {
	Net_EtherAddress destination;
	Net_EtherAddress source;
	unsigned short   type;		/* valid types defined below */
} Net_EtherHdr;

#define	NET_ETHER_HDR_DESTINATION(e)	((e).destination)
#define	NET_ETHER_HDR_SOURCE(e)		((e).source)
#define	NET_ETHER_HDR_TYPE(e)		((e).type)

#define	NET_ETHER_HDR_DESTINATION_PTR(e)	&((e).destination)
#define	NET_ETHER_HDR_SOURCE_PTR(e)		&((e).source)
#define	NET_ETHER_HDR_TYPE_PTR(e)		&((e).type)

#define	NET_ETHER_HDR_COPY(src, dst) ((dst) = (src))

#else 

typedef unsigned char Net_EtherHdr[14];

#define	NET_ETHER_HDR_DESTINATION(e)	((unsigned char *) (e))
#define	NET_ETHER_HDR_SOURCE(e)		((unsigned char *) (e+6))
#define	NET_ETHER_HDR_TYPE(e)		(*((unsigned short *) (e+12)))

#define	NET_ETHER_HDR_DESTINATION_PTR(e)	((unsigned char *) (e))
#define	NET_ETHER_HDR_SOURCE_PTR(e)		((unsigned char *) (e+6))
#define	NET_ETHER_HDR_TYPE_PTR(e)		(*((unsigned short *) (e+12)))

#define	NET_ETHER_HDR_COPY(src, dst) (bcopy(src,dst,sizeof(Net_EtherHdr)))

#endif /* NET_ETHER_BAD_ALIGNMENT */

/*
 * Minimum and maximum packet sizes. The maximum is actually 1518, but
 * for some reason it is set here to 1514.  I wouldn't change it unless
 * you're sure you won't break something in the kernel. JHH
 */

#define	NET_ETHER_MIN_BYTES	64
#define	NET_ETHER_MAX_BYTES	1514

/*
 * Definitions of known ethernet packet types (from rfc990, except for SPRITE
 * and TRAIL).
 */
#define NET_ETHER_PUP			0x0200
#define NET_ETHER_PUP_ADDR_TRANS	0x0201
#define NET_ETHER_XNS_IDP		0x0600
#define NET_ETHER_IP			0x0800
#define NET_ETHER_ARP			0x0806
#define NET_ETHER_XNS_COMPAT		0x0807
#define NET_ETHER_SPRITE		0x0500
#define NET_ETHER_SPRITE_ARP		0x0502 /* deprecated */
#define NET_ETHER_SPRITE_DEBUG		0x0504
#define NET_ETHER_TRAIL			0x1000
#define NET_ETHER_REVARP		0x8035
#define NET_ETHER_MOP			0x6001

#endif /* _NETETHER */
