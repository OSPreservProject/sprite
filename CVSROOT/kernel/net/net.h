/*
 * net.h --
 *
 *	This defines the types and constants for the networking software.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _NET
#define _NET

#include "sprite.h"
#include "netEther.h"
#include "netRoute.h"
#include "user/net.h"
#include "sync.h"

/*
 * This define is used by the net module in maintaining routing
 * information.  It is needed by other modules that maintain state
 * about the various Sprite hosts.
 */

#define NET_NUM_SPRITE_HOSTS	50

/*
 * Scatter/gather vector element.  The network output routines take
 * an array of these elements as a specifier for the packet.  This
 * format lets clients of the network module save extra copies because
 * they can leave data objects where they lie.  The done and conditionPtr
 * parts are used by clients that don't otherwise know when it is safe
 * to release the buffers referenced by the scatter/gather element.  If
 * conditionPtr is non-NIL then it is notified and done is set to TRUE
 * after the packet has been transmitted (it may be queued for a bit).
 */

typedef struct {
    Address		bufAddr;	/* In - Buffer address */
    int			length;		/* In - Size of the buffer */
    Sync_Condition	*conditionPtr;	/* In - Condition to wait on. */
    Boolean		done;		/* Out - set when I/O completes */
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
 * Forward Declarations.
 */
extern	Net_EtherStats	net_EtherStats;

extern	void	Net_Init();
extern	void	Net_Reset();
extern	void	Net_Input();
extern	ReturnStatus	Net_Output();
extern	void	Net_OutputRawEther();
extern	void	Net_RecvPoll();
extern	void	Net_EtherOutputSync();
extern	void	Net_Intr();
extern	void	Net_GatherCopy();

extern	ReturnStatus Net_InstallRouteStub();
extern	ReturnStatus Net_InstallRoute();
extern	void	Net_SpriteIDToName();
extern	char *	Net_SpriteIDToMachType();
extern	void	Net_RouteInit();
extern	void	Net_NameToAddr();
extern	void	Net_AddrToName();

Net_Route	*Net_IDToRoute();
int		 Net_AddrToID();

extern Net_Route *Net_Arp();
extern int	Net_RevArp();

extern void	Net_HostPrint();	/* Moved to Sys_HostPrint */

#endif _NET
