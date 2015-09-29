/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_ether.h 1.20 88/02/08 SMI; from UCB 7.1 6/5/86
 */

#ifndef	_IF_ETHER_
#define	_IF_ETHER_

/*
 * The following include is for compatibility with SunOS 3.x and
 * 4.3bsd.  Newly written programs should include it separately.
 */
#include "if_arp.h"

/*
 * Ethernet address - 6 octets
 */
struct ether_addr {
	u_char	ether_addr_octet[6];
};

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	struct	ether_addr ether_dhost;
	struct	ether_addr ether_shost;
	u_short	ether_type;
};

#define	ETHERTYPE_PUP		0x0200		/* PUP protocol */
#define	ETHERTYPE_IP		0x0800		/* IP protocol */
#define	ETHERTYPE_ARP		0x0806		/* Addr. resolution protocol */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 */
struct	ether_arp {
	struct	arphdr ea_hdr;		/* fixed-size header */
	struct	ether_addr arp_sha;	/* sender hardware address */
	u_char	arp_spa[4];		/* sender protocol address */
	struct	ether_addr arp_tha;	/* target hardware address */
	u_char	arp_tpa[4];		/* target protocol address */
};
#define	arp_hrd	ea_hdr.ar_hrd
#define	arp_pro	ea_hdr.ar_pro
#define	arp_hln	ea_hdr.ar_hln
#define	arp_pln	ea_hdr.ar_pln
#define	arp_op	ea_hdr.ar_op

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.  For example, each ec_softc or il_softc
 * begins with this structure.
 *
 * The structure contains a pointer to an array of multicast addresses.
 * This pointer is NULL until the first successful SIOCADDMULTI ioctl
 * is issued for the interface.
 */
#define	MCADDRMAX	64		/* multicast addr table length */
struct	arpcom {
	struct	ifnet ac_if;		/* network-visible interface */
	struct	ether_addr ac_enaddr;	/* ethernet hardware address */
	struct	in_addr ac_ipaddr;	/* copy of ip address- XXX */
	struct	ether_addr *ac_mcaddr;	/* table of multicast addrs */
	u_short	ac_nmcaddr;		/* count of M/C addrs in use */
};

/*
 * Internet to ethernet address resolution table.
 */
struct	arptab {
	struct	in_addr at_iaddr;	/* internet address */
	struct	ether_addr at_enaddr;	/* ethernet address */
	u_char	at_timer;		/* minutes since last reference */
	u_char	at_flags;		/* flags */
	struct	mbuf *at_hold;		/* last packet until resolved/timeout */
};

/*
 * Compare two Ethernet addresses - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcmp instead.  Note that like
 * bcmp, we return zero if they are the SAME.
 */
#if defined(sun2) || defined(sun3)
/*
 * On 680x0 machines, we can do a longword compare that is NOT
 * longword aligned, as long as it is even aligned.
 */
#define ether_cmp(a,b) ( ((short *)a)[2] != ((short *)b)[2] || \
  *((long *)a) != *((long *)b) )
#endif

/*
 * On a sparc, functions are FAST
 */
#if defined(sparc)
#define ether_cmp(a,b) (sparc_ether_cmp((short *)a, (short *)b))
#endif 

#ifndef ether_cmp
#define ether_cmp(a,b) (bcmp((caddr_t)a,(caddr_t)b, 6))
#endif

/*
 * Copy Ethernet addresses from a to b - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcopy instead.
 */
#if defined(sun2) || defined(sun3)
#define ether_copy(a,b) { ((long *)b)[0]=((long *)a)[0]; \
 ((short *)b)[2]=((short *)a)[2]; }
#endif

#if defined(sparc)
#define ether_copy(a,b) { ((short *)b)[0]=((short *)a)[0]; \
 ((short *)b)[1]=((short *)a)[1]; ((short *)b)[2]=((short *)a)[2]; }
#endif

#ifndef ether_copy
#define ether_copy(a,b) (bccopy((caddr_t)a,(caddr_t)b, 6))
#endif

/*
 * Copy IP addresses from a to b - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcopy instead.
 */
#if defined(sun2) || defined(sun3)
#define ip_copy(a,b) { *((long *)b) = *((long *)a); }
#endif

#if defined(sparc)
#define ip_copy(a,b) { ((short *)b)[0]=((short *)a)[0]; \
 ((short *)b)[1]=((short *)a)[1]; }
#endif

#ifndef ip_copy
#define ip_copy(a,b) (bccopy((caddr_t)a,(caddr_t)b, 4))
#endif

#ifdef	KERNEL
struct	ether_addr etherbroadcastaddr;
struct	arptab *arptnew();
char *ether_sprintf();
#endif	KERNEL

#endif	_IF_ETHER_
