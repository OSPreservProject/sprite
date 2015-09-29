/*
 * netUltra.h --
 *
 *	Definitions for sending and receiving Ultranet packets.  Packets
 *	are sent by putting a "Net_UltraHeader" at the start of the packet
 *	then doing a write() on the Ultranet device.  Packets are read
 *	by doing a read.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/netUltra.h,v 1.5 92/08/05 16:34:54 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _NETULTRA
#define _NETULTRA

#include "sprite.h"
#include "machparam.h"

/*
 * The following macros are used to manipulate an Ultranet address.
 */

#define Net_UltraAddressSet(addrPtr, group, unit) { 		\
    ((unsigned char *) (addrPtr))[4] = (group) >> 2; 		\
    ((unsigned char *) (addrPtr))[5] = ((group) << 6) | (unit); \
    ((unsigned char *) (addrPtr))[1] = 0x49;			\
    ((unsigned char *) (addrPtr))[6] = 0xfe;			\
}

#define Net_UltraAddressGet(addrPtr, groupPtr, unitPtr) { 	\
    *(groupPtr) = (((unsigned char *) (addrPtr))[4] << 2) | 	\
		  (((unsigned char *) (addrPtr))[5] >> 6); 	\
    *(unitPtr) = (((unsigned char *) (addrPtr))[5] & 0x3f); 	\
}


/*
 * Definition of an Ultranet address.
 */
typedef struct Net_UltraAddress {
    char		data[8];
} Net_UltraAddress;

#define Net_UltraAddrCmp(a,b) \
    (!(((a).data[4] == (b).data[4]) && ((a).data[5] == (b).data[5])))

#define Net_UltraAddrCopy(src, dest) ((dest) = (src))

/*
 * Ultranet Transport Layer Address Format.  This structure is used in
 * the various request blocks that are sent to the adapter.
 * Here are a few tips on filling this in.  The "addressSize" is 7.
 * TSAP stands for "Transport Service Access Point".  The standard
 * Ultranet software sets the size to 2 and puts the port in the TSAP.
 * You'll have to follow suit if you want to write to a particular port
 * on a Unix machine with an Ultranet.  On Sprite: the "localAddress" field of
 * the header you just set the tsapSize to 2, and zero out the tsap.
 * For the "remoteAddress" you should set the tsapSize to 2, and tsap[0] 
 * and tsap[1] to something either than {0,0} and {0xff, 0xff}.
 * Use the above macros to set the "address" field.
 */

#define NET_ULTRA_ADDR_SIZE		7 /* Value for "addressSize". */
#define NET_ULTRA_TSAP_SIZE		4 /* Size of the TSAP. */

typedef struct Net_UltraTLAddress {
    unsigned char	addressSize;	/* Size of the address field. */
    unsigned char	tsapSize;	/* NET_ULTRA_TSAP_SIZE */
    unsigned char	tsap[4];	/* The TSAP, whatever that is. */
    Net_UltraAddress	address;	/* The network address. */
    char		pad[2];		/* Pad this out to 16 bytes. */
} Net_UltraTLAddress;

/*
 * A "wildcard" transport layer address that matches any address. 
 * This can be used in the "localAddress" field, as well as to initialize
 * Net_UltraTLAddress structures before filling them in.
 */

#define Net_UltraTLWildcard(addrPtr) {				\
    bzero((char *) (addrPtr), sizeof(Net_UltraTLAddress));	\
    (addrPtr)->addressSize = 7;					\
    (addrPtr)->tsapSize = 4;					\
}

/*
 * Header on ultranet packets. If you want to use any of the fields other
 * than the remote and local address you should look at the file
 * "kernel/netUltra.h".  Those fields marked "unused" do not need to be
 * used to send packets to or receive packets from the net device and must
 * be set to zero.  Your best bet is to bzero the header before filling
 * it in.
 */

typedef struct Net_UltraHeader {
    unsigned char	cmd;		/* Unused. */
    unsigned char	status;		/* Unused. */
    unsigned short	reference;	/* Unused. */
    int			foo;		/* Unused. */
    Address		buffer;		/* Unused. */
    int			size;		/* Unused. */
    Net_UltraTLAddress	remoteAddress;	/* Address of remote host. */
    Net_UltraTLAddress	localAddress;	/* Address of local host. */
    unsigned short	options;	/* Unused. */
    unsigned short	quality;	/* Unused. */
    unsigned int	pad3;
} Net_UltraHeader;

/*
 * Minimum and maximum Ultranet packet sizes (including the Net_UltraHeader).
 */

#define NET_ULTRA_MIN_BYTES 	0
#define NET_ULTRA_MAX_BYTES 	(32768 + sizeof(Net_UltraHeader))

#endif /* _NETULTRA */

