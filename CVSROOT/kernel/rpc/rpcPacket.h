/*
 * rpcPacket.h --
 *
 * This defines the RPC packet header format.  This header comes just
 * after the packet transport header.  It contains information about the
 * remote procedure call and is followed by parameters and data for the
 * RPC.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * CHANGES TO THIS FILE NEED TO MIRROR CHANGES MADE TO THE UNIX 
 * HEADER FILE "fsp.h" 
 *
 * sccsid "Sprite $Header$"
 */

#ifndef _RPCPACKET
#define _RPCPACKET

/*
 * The Rpc header.
 */
typedef struct RpcHdr {
    unsigned short	flags;		/* Protocol flags, explained below */
    unsigned short	delay;		/* Interfragment delay info */
    int			clientID;	/* Client Sprite host id */
    int			serverID;	/* Server Sprite host id */
    short		channel;	/* The client channel number */
    short		serverHint;	/* Server index hint. The server
					 * machine updates this on every
					 * packet it sends to a client.  The
					 * channel being used for the RPC
					 * should preserve the value sent
					 * and return it (in this field) with
					 * future messages to the server */
    unsigned int	ID;		/* ID/sequence number. This ID is the
					 * same on all packets pertaining
					 * to the same RPC. For a given
					 * channel number increases
					 * increase from one transaction
					 * to the next */
    unsigned short	transport;	/* Low-er level transport protocol ID.
					 * This should GO AWAY but the unix
					 * file server still depends on it */
    unsigned short	numFrags;	/* Number of fragments in packet (<=16).
					 * If the packet is complete, ie. no
					 * fragmenting, then this field should
					 * be ZERO */
    unsigned short	fragMask;	/* Fragment bitmask ID. The I'th frag
					 * has the I'th bit of this mask
					 * set.  If no fragments then this
					 * field should be ZERO.  On partial
					 * acknowledgments this contains
					 * the receiver's summary bitmask */
    int			command;	/* Rpc command, see rpcCall.h */
    unsigned int	bootID;		/* Client boot timestamp */
    short		paramSize;	/* Size of the parameter area */
    short		dataSize;	/* Size of the data data area */
    short		paramOffset;	/* This is the starting offset for the
					 * block of parameter bytes sent in
					 * this fragment.  This is zero for
					 * unfragmented sends and for the first
					 * fragment of a group */
    short		dataOffset;	/* Offset for the data area. */
} RpcHdr;

/*
 * Values for the transport field. These are the same as AF_* constants in
 * the Unix kernel.  This field types the packet delivery header that
 * comes just before the RPC header.  THIS SHOULD GO AWAY but it requires
 * re-working the UNIX implementation of RPC.
 *
 *  PROTO_INET		(same as Unix AF_INET)
 *			This indicates that IP is the transport level protocol.
 *  PROTO_ETHER		(same as Unix AF_ETHER)
 *			This indicates that raw ethernet packets are
 *			used to transport packets.
 */
#define PROTO_INET	2
#define PROTO_ETHER	12

/*
 * The flags field is used to type packets and for flags that
 * modify the packet type.
 *
 * Packet Types:
 *	RPC_REQUEST a request message sent by the client to the server.
 *
 *	RPC_REPLY a reply message returned to the client from the server.
 *
 *	RPC_ACK an explicit acknowledgment message for a request or reply.
 *	If the RPC_LASTFRAG flag is present then this is a partial
 *	acknowledgment and the summary bitmask is in the fragMask field.
 *
 *	RPC_ECHO a special packet that just is bounced off the server by
 *	the dispatcher.  A server process is not involved.
 *
 * Packet Flags:
 *	RPC_PLSACK demands an explicit acknowledgment message from the receiver.
 *
 *	RPC_LASTFRAG is present on the last fragment of a batch being sent,
 *	which is not always the last fragment in a message because a partial
 *	partial re-send may only involve some fragments in the middle
 *	of the message.  This flag set on partial acknowledgments.
 *
 *	RPC_CLOSE only valid on type RPC_ACK messages.  This means the client
 *	has successfully gotten its last reply and is ending the sequence
 *	of RPCs with the server.
 *
 *	RPC_ERROR indicates the service routine had an error, in this
 *	case the command field contains an error code.
 *
 *	RPC_NOT_ACTIVE means that the originating host is not fully alive.
 *	This means the packet is ignored by the recovery module, but still good.
 *
 * Flags only valid in trace records:
 *	RPC_SERVER the packet is bound for the server side of the rpc system.
 *
 *	RPC_LAST_REC the trace record is the oldest trace record in the
 *	circular buffer.
 */
#define RPC_NO_FLAGS	0x0
#define RPC_FLAG	0x00ff
#define RPC_PLSACK	0x0001
#define RPC_LASTFRAG	0x0002
#define RPC_CLOSE	0x0004
#define RPC_ERROR	0x0008
#define RPC_SERVER	0x0010
#define RPC_LAST_REC	0x0020
#define RPC_NOT_ACTIVE	0x0040

#define RPC_TYPE	0xff00
#define RPC_REQUEST	0x0100
#define RPC_ACK		0x0200
#define RPC_REPLY	0x0400
#define RPC_ECHO	0x0800

/*
 * Items related to Fragmenting.
 *
 * The maximum size supported by fragmentation is 16K of data plus
 * 1k of parameters.  This large chunk is broken into fragments.
 * The maximum size of a datagram is dependent on the transport medium.
 */
#define RPC_MAX_NUM_FRAGS	16
#define RPC_MAX_FRAG_SIZE	1024
#define RPC_MAX_DATASIZE	(RPC_MAX_NUM_FRAGS * RPC_MAX_FRAG_SIZE)
#define RPC_MAX_PARAMSIZE	(1 * RPC_MAX_FRAG_SIZE)
#define RPC_MAX_SIZE		(RPC_MAX_DATASIZE+RPC_MAX_PARAMSIZE)

/*
 * The following define is for ethernet.  The ethernet limit lets us
 * transmit 1K of data plus header information.
 */
#define RPC_MAX_PACKET_SIZE	NET_ETHER_MAX_BYTES
/*
 * An array of bitmasks is kept so the dispatcher can quickly determine if
 * a message is complete.  Indexed by the total number of fragments in the
 * packet, the array contains a complete bitmask for that many fragments.
 */
extern unsigned int rpcCompleteMask[];

/*
 * Machines have some minimum delay that they can handle between successive
 * packets before they start to drop them.  The current notion of this delay
 * for other hosts is kept in rpcDelay.  This machine's currently acceptable
 * delay is kept in rpcMyDelay, while its max speed at generating packets is
 * kept in rpcOutputRate.  This information is used to insert, if neccesary,
 * delays between transmission of fragments.
 */
extern short rpcDelay[];		/* Other host's inter-frag delay */
extern short rpcMyDelay;		/* This host's inter-frag delay */
extern short rpcOutputRate;		/* This machines output rate */

/*
 * Forward Declarations.
 */
ReturnStatus RpcOutput();
void RpcTrace();

#endif not _RPCPACKET
