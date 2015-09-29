/*
 * netTypes.h --
 *
 *	This defines the types and contants for the networking software.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/net/RCS/netTypes.h,v 1.5 92/07/06 15:02:55 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _NETTYPES
#define _NETTYPES

#include <mach.h>
#include <device/net_status.h>

#if defined(KERNEL) || defined(SPRITED)
#include <user/netEther.h>
#include <user/net.h>
#include <syncTypes.h>
#include <fs.h>
#else
#include <netEther.h>
#include <net.h>
#include <kernel/syncTypes.h>
#include <kernel/fs.h>
#endif /* KERNEL || SPRITED */

/*
 * This define is used by the net module in maintaining routing
 * information.  It is needed by other modules that maintain state
 * about the various Sprite hosts.
 */

#define NET_NUM_SPRITE_HOSTS	200

/*
 * Maximum number of network interfaces a host may have.
 */

#define NET_MAX_INTERFACES	2

/*
 * Constants defining the different types of packets.
 */

typedef int Net_PacketType;

#define NET_PACKET_UNKNOWN	0x0
#define NET_PACKET_SPRITE	0x1
#define NET_PACKET_ARP		0x2
#define NET_PACKET_RARP		0x3
#define NET_PACKET_DEBUG	0x4
#define NET_PACKET_IP		0x5

/*
 * Scatter/gather vector element.  The network output routines take
 * an array of these elements as a specifier for the packet.  This
 * format lets clients of the network module avoid extra copies because
 * they can leave data objects where they lie.
 */

typedef struct {
    Address		bufAddr;	/* In - Buffer address */
    int			length;		/* In - Size of the buffer */
} Net_ScatterGather;

/*
 * Statistics - the ethernet drivers record the number of occurences
 *	of various events.
 */
typedef struct {
    int	packetsRecvd;		/* # packets received of any type */
    int	packetsSent;		/* # packets sent of any type */
    int	packetsOutput;		/* # packets output of any type */
    int broadRecvd;		/* # broadcast packets received */
    int broadSent;		/* # broadcast packets sent */
    int others;			/* # packets between two other machines */
    int overrunErrors;		/* # packets received with overrun errors. */
    int crcErrors;		/* # packets received with CRC errors. */
    int fcsErrors;		/* # packets received with FCS errors */
    int frameErrors;		/* # packets received with framing errors */
    int rangeErrors;		/* # packets received with range errors */
    int collisions;		/* # of collisions on transmissions */
    int xmitCollisionDrop;	/* # of packets dropped because of too many
				   collisions. */
    int	xmitPacketsDropped;	/* # transmitted packets that are dropped */
    int	recvPacketsDropped;	/* # transmitted packets that are dropped */
    int matches;		/* # of address match packets */
    int recvAvgPacketSize;	/* average size of packets received */
    int recvAvgLargeSize;	/*  ...  of more than 100 bytes */
    int recvAvgSmallSize;	/*  ...  of less than 100 bytes */
    int sentAvgPacketSize;	/* average size of packets sent */
    int sentAvgLargeSize;	/*  ...  of more than 100 bytes */
    int sentAvgSmallSize;	/*  ...  of less than 100 bytes */
    int	bytesSent;		/* Total number of bytes transmitted. */
    int	bytesReceived;		/* Total number of bytes received. */
} Net_EtherStats;

/*
 * Statistics for the UltraNet interface. 
 */
typedef struct Net_UltraStats {
    int		packetsSent;		/* Number of packets sent. */
    int		bytesSent;		/* Number of bytes sent. */
    int		sentHistogram[33];	/* Histogram of bytes sent 
					 * (1K buckets). */
    int		packetsReceived;	/* Number of packets received. */
    int		bytesReceived;		/* Number of bytes received. */
    int		receivedHistogram[33];	/* Histogram of bytes received
					 * (1K buckets). */
} Net_UltraStats;

/* 
 * Statistics based on the generic Mach device interface.
 * It'd probably be better to make the byte counts longs instead of 
 * ints, but that would break NetAddStats (XXX).
 */

typedef struct {
    int packetsOut;		/* total packets written */
    int inlinePacketsOut;	/* num. packets written using inline
				 * interface */
    int bytesOut;		/* total bytes written */
    int packetsIn;		/* total packets read */
    int selfBroadcastIn;	/* num. packets read that are our broadcast 
				 * packets */ 
    int bytesIn;		/* total bytes read */
} Net_GenStats;


/*
 * Statistics in general.
 */

typedef struct Net_Stats {
    Net_EtherStats	ether;
    Net_UltraStats	ultra;
    Net_GenStats	generic;
} Net_Stats;

/*
 * Structure that defines a network interface.  The Mach name is 
 * potentially decoupled from the Sprite name/number because (a) on the
 * ds5000 Mach names the interfaces in the backward order from Sprite and
 * (b) some Sprite software (e.g., netroute) assumes that there is always a
 * usable interface 0.
 */

typedef struct Net_Interface {
    char		*name;		/* Sprite name for the interface. */
    int		 	number;		/* interface number of device. */
    char		*machName; 	/* Mach name and number for the 
					 * interface; null if same as 
					 * Sprite <name><number> */ 
    mach_port_t		requestPort;	/* port for making I/O requests */
    mach_port_t		recvPort; 	/* port that receives packets */

			/* Device-specific initialization routine. */
    ReturnStatus	(*init) _ARGS_((struct Net_Interface *interPtr,
					struct net_status *infoPtr));

			/* Output a packet. */
    ReturnStatus	(*output) _ARGS_((struct Net_Interface *interPtr,
				Boolean asynch, Address packetHeader, 
				Net_ScatterGather *scatterGatherPtr,
				int scatterGatherLength,
				ReturnStatus *statusPtr));

			/* Reset the interface */
    void 		(*reset) _ARGS_ ((struct Net_Interface *interPtr));

			/* Perform ioctls on interface. */
    ReturnStatus	(*ioctl) _ARGS_((struct Net_Interface *interPtr,
				Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));

			/* Get performance statistics. */
    ReturnStatus	(*getStats) _ARGS_((struct Net_Interface *interPtr,
				Net_Stats *statPtr));	

    			/* 
    			 * Merge the header and scatter/gather array into 
    			 * the interface's buffer.  Returns header length.
    			 * Also determines if packet destination equals
    			 * packet source and fixes up the header if
    			 * necessary.
			 */
    void		(*mergePacket) _ARGS_((struct Net_Interface
				*interPtr, Address hdrPtr, 
				Net_ScatterGather *scatterGatherPtr, 
		    		int scatterGatherLength, 
				int *headerLengthPtr, Boolean *isLoopbackPtr));

			/*
			 * Convert from network packet type to 
			 * network-independent packet type.
			 */
    Net_PacketType	(*getPacketType) _ARGS_((Address headerPtr,
				int headerLength));

    char		*msgName; 	/* name to use for error messages */
    Net_NetworkType	netType;	/* Type of interface. */
    int			flags;		/* Status flags. See below. */
    Sync_Semaphore	mutex;		/* Protects network interface board
					 * and related data structures. */
    int			maxBytes;	/* Maximum transfer unit 
					 * (packet size), w/o header. */
    int			minBytes;	/* Minimum transfer unit w/o header. */
    Address		buffer;		/* Holds assembled packet. */

			/* Packet handler for network device driver. */
    void		(*packetProc) _ARGS_((struct Net_Interface *interPtr,
				int packetLength, Address packetPtr));

    ClientData		interfaceData;	/* for device-specific data (e.g., 
					 * instrumentation) */ 
    ClientData		devNetData;	/* Place for the network device
					 * driver to store stuff. */
    Net_Address		netAddress[NET_MAX_PROTOCOLS];
    Net_Address		broadcastAddress; /* Broadcast address for this
					   * interface. */
} Net_Interface;

/*
 * Flag values for Net_Interface.
 */

#define NET_IFLAGS_RUNNING	0x1	/* The interface is active. */
#define NET_IFLAGS_BROADCAST	0x2	/* Interface supports broadcast. */


#endif /* _NETTYPES */
