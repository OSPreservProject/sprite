/*
 * net.h --
 *
 *	Declarations of the network library code.
 *
 * Copyright 1987 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /r3/kupfer/spriteserver/include/user/RCS/net.h,v 1.2 91/11/11 23:08:48 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _NET_USER
#define _NET_USER

#include <sprite.h>
#include <machparam.h>
#include <netEther.h>
#include <netInet.h>
#include <netUltra.h>

/*
 * A network address.  The "generic" field must be at least as large as
 * any of the other fields and must be an integral number of integers.
 */
typedef union Net_Address {
    Net_EtherAddress		ether;
    Net_InetAddress		inet;
    Net_UltraAddress		ultra;
    struct { char data[8]; } 	generic;
} Net_Address;

/*
 * Macro to compare two Net_Address objects.
 */
#define NET_ADDRESS_COMPARE(a,b) \
    (bcmp((char *) &(a), (char *) &(b), sizeof(Net_Address)))

/*
 * Maximum number of network protocols.  Right now we support two,
 * raw (ethernet for example) and inet
 */

#define NET_MAX_PROTOCOLS 2

/*
 * The different protocols.
 */

#define NET_PROTO_RAW	0
#define NET_PROTO_INET	1


/* 
 * This is the version number stored in the route.  Set this before
 * installing a route and check it when looking at one.
 */
#define NET_ROUTE_VERSION 0x70500

/*
 * Number of different types of networks. 
 */

#define NET_NUM_NETWORK_TYPES 2

/*
 * Type of network.  See below.
 */

typedef int Net_NetworkType;

/*
 * Types of network (values for Net_NetworkType).
 */

#define NET_NETWORK_ETHER	((Net_NetworkType) 0)	/* Ethernet. */
#define NET_NETWORK_ULTRA	((Net_NetworkType) 1)	/* Ultranet. */

/*
 * This structure defines the packet headers.
 */
typedef union Net_Header {
    Net_EtherHdr	etherHdr;	/* Raw ethernet packet. */
    struct {				/* An IP packet on the ethernet. */
	Net_EtherHdr	etherHdr;
	Net_IPHeader	ipHdr;
    } inetHdr;		
} Net_Header;

/*
 * The user-level view of a route. This structure is used to both install
 * routes and to get their contents.  The fields are marked 'in' if they
 * must be set to install a route, and 'out' if they are set when 
 * getting a route's contents.
 */

typedef struct Net_RouteInfo {
    int			version;	/* Version number. (in/out)*/
    int			spriteID;	/* Sprite ID of route target. (in/out)*/
    int			interface;	/* The interface number to use. 
					 * (in/out) */
    int			protocol;	/* Route protocol. (in/out) */
    int			flags;		/* Flags. See below. (in/out) */
    int			refCount;	/* Number of references to route. 
					 * (out). */
    int			routeID;	/* Unique route ID. (in/out)*/
    int			maxBytes;	/* Maximum transfer unit for route. 
					 * This does not include any headers.
					 * (out) */
    int			minBytes;	/* Minimum transfer unit for route. 
					 * This does not include any headers.
					 * (out) */
    Net_NetworkType	netType;	/* Type of network. See above. (out) */
    Net_Address		netAddress[NET_MAX_PROTOCOLS];	/* Address of target
							 * for each protocol.
							   (in/out) */
    char		desc[64];	/* Route description for debugging.
					 * (out) */
    char		hostname[20];	/* Host name. (in/out) */
    char		machType[12];	/* Host machine type. (in/out) */
    ClientData		userData;	/* Data that is uninterpreted by 
					 * kernel. (in/out) */
    Net_Header		header;		/* The packet header. (out) */
} Net_RouteInfo;


/*
 * Define the flags field.
 */

#define NET_FLAGS_VALID 0x1

/*
 * Define the special Sprite ID used for broadcasting.
 */
#define		NET_BROADCAST_HOSTID	0


/* 
 * If we're building a kernel or server, don't include this declaration.  It 
 * clashes with the declaration for the internal routine.
 */

#if !defined(KERNEL) && !defined(SPRITED)

extern ReturnStatus Net_InstallRoute _ARGS_((int spriteID, int flags,
					     int type, ClientData clientData,
					     char *hostname, char
					     *machType));
#endif /* !KERNEL && !SPRITED */

/* 
 * The structures defined below here are obsolete and should not be used
 * in new programs.
 */

/*
 * A Generic network address...
 */
typedef struct {
    char    data[14];
} Net_GenericAddress;

/*
 * Definition of user visible Route structure that is returned
 * via the Test_Stats system call with the NET_GET_ROUTE command.
 */
typedef struct Net_SpriteRoute {
    int		flags;		/* Flags defined in kernel/net.h */
    int		spriteID;	/* Universal Sprite Host ID */
    int		type;		/* Types defined in kernel/net.h */
    union {
	Net_EtherHdr	etherHdr;	/* type == NET_ROUTE_ETHER */
	char		data[14];	/* type == NET_ROUTE_GENERIC */
	struct {
	    Net_EtherHdr	etherHdr;
	    Net_IPHeader	ipHdr;
	} inetHdr;			/* type == NET_ROUTE_INET */
    } route;
} Net_SpriteRoute;


/*
 * Declarations for -lnet library.
 */

extern Net_InetAddress	Net_StringToInetAddr _ARGS_((char *cp));
extern ReturnStatus	Net_StringToAddr _ARGS_((char *buffer, int protocol,
						 Net_NetworkType netType,
						 Net_Address *addressPtr));
extern char		*Net_InetAddrToString _ARGS_((Net_InetAddress address,
						      char *buffer));
extern char		*Net_AddrToString _ARGS_((Net_Address *netAddressPtr,
						  int protocol,
						  Net_NetworkType netType,
						  char *buffer));
extern unsigned int	Net_StringToNetNum _ARGS_((char *cp));
extern unsigned int	Net_InetAddrHostNum _ARGS_((Net_InetAddress inetAddr));
extern unsigned int	Net_InetAddrNetNum _ARGS_((Net_InetAddress addr));
extern unsigned int	Net_InetAddrNetMask _ARGS_((Net_InetAddress addr));
extern Net_InetAddress	Net_MakeInetAddr _ARGS_((unsigned int net,
						 unsigned int host));
extern char	*Net_EtherAddrToString _ARGS_((Net_EtherAddress *etherAddrPtr,
					       char buffer[18]));
extern void	Net_StringToEtherAddr _ARGS_((char *buffer,
					   Net_EtherAddress *etherAddressPtr));
extern unsigned short	Net_InetChecksum _ARGS_((int len, Address bufPtr));
extern unsigned short	Net_InetChecksum2 _ARGS_((int len, Address bufPtr,
					       Net_IPPseudoHdr *pseudoHdrPtr));

#if BYTE_ORDER == LITTLE_ENDIAN
extern unsigned int	Net_NetToHostInt _ARGS_((unsigned int longInt));
extern unsigned int	Net_HostToNetInt _ARGS_((unsigned int longInt));

extern unsigned short	Net_NetToHostShort _ARGS_((unsigned short shortInt));
extern unsigned short	Net_HostToNetShort _ARGS_((unsigned short shortInt));
#else 
#define Net_NetToHostInt(arg)	(arg)
#define Net_HostToNetInt(arg)	(arg)

#define Net_NetToHostShort(arg)	(arg)
#define Net_HostToNetShort(arg)	(arg)
#endif


#endif _NET_USER
