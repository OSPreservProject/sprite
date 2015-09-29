/*
 * netInet.h --
 *
 *	Declarations of data structures and constants for the DARPA Internet 
 *	protocol suite.
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/netInet.h,v 1.7 92/08/05 16:34:23 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _NETINET
#define _NETINET

#include "machparam.h"
#include "netEther.h"


/*
 * Definition of the 32-bit Internet Address.
 */

typedef unsigned int Net_InetAddress;

/*
 * Net_InetAddrCmp --
 *
 * Compare two internet addresses. Returns 0 if they are the same, 1 otherwise.
 */
#define Net_InetAddrCmp(a,b) ((a) != (b))

#define Net_InetAddrCopy(src, dest) ((dest) = (src))

/*
 * Structure of the Ethernet Address Resolution Protocol and Reverse ARP 
 * packets, according to RFC826 (ARP) and RFC903 (RARP).
 */
typedef struct { 
    unsigned short	hardwareType;		/* "Hardware address space (e.g.
						 * Ethernet, Packet Radio Net.)"
						 */
    unsigned short	protocolType;		/* "Protocol Address Space"
						 * specifies format of protocol
						 * addresses. Value is one
						 * of the defined packet types 
						 * in netEther.h. */
    unsigned char	hardwareAddrLen;	/* length in bytes of hardware
						 * addresses. */
    unsigned char	protocolAddrLen;	/* length in bytes of protocol
						 * addresses. */
    unsigned short	opcode;			/* see below. */
}  Net_ArpHeader;

typedef struct {
    Net_EtherHdr	header;			/* packet header */
    unsigned short	hardwareType;		/* "Hardware address space (e.g.
						 * Ethernet, Packet Radio Net.)"
						 */
    unsigned short	protocolType;		/* "Protocol Address Space"
						 * specifies format of protocol
						 * addresses. Value is one
						 * of the defined packet types 
						 * in netEther.h. */
    unsigned char	hardwareAddrLen;	/* length in bytes of hardware
						 * addresses. */
    unsigned char	protocolAddrLen;	/* length in bytes of protocol
						 * addresses. */
    unsigned short	opcode;			/* see below. */
    Net_EtherAddress	senderEtherAddr;	/* hardware address of sender
						 * of the packet. */
    Net_InetAddress	senderProtAddr;		/* protocol address of sender
						 * of the packet. */
    Net_EtherAddress	targetEtherAddr;	/* hardware address of target
						 * (if known). */
    Net_InetAddress	targetProtAddr;		/* protocol address of target.*/
} Net_ArpPacket;

/*
 * Value for the hardwareType field in Net_ArpPacket.
 *   NET_ARP_TYPE_ETHER		 using ethernet hardware.
 */

#define NET_ARP_TYPE_ETHER	1

/*
 * Values for the opcode field in Net_ArpPacket:
 *   NET_ARP_REQUEST		request ethernet address for a given IP address.
 *   NET_ARP_REPLY		answer for above request.
 *   NET_RARP_REQUEST		request IP address for a given ethernet address.
 *   NET_RARP_REPLY		answer for above request.
 */

#define NET_ARP_REQUEST		1
#define NET_ARP_REPLY		2
#define NET_RARP_REQUEST	3
#define NET_RARP_REPLY		4



#define NET_INET_BROADCAST_ADDR		((Net_InetAddress) 0xFFFFFFFF)
#define NET_INET_ANY_ADDR		((Net_InetAddress) 0)

#define NET_INET_CLASS_A_ADDR(addr) \
			(((unsigned int)(addr) & 0x80000000) == 0)
#define NET_INET_CLASS_A_HOST_MASK		0x00FFFFFF
#define NET_INET_CLASS_A_NET_MASK		0xFF000000
#define NET_INET_CLASS_A_SHIFT			24

#define NET_INET_CLASS_B_ADDR(addr) \
			(((unsigned int)(addr) & 0xC0000000) == 0x80000000)
#define NET_INET_CLASS_B_HOST_MASK		0x0000FFFF
#define NET_INET_CLASS_B_NET_MASK		0xFFFF0000
#define NET_INET_CLASS_B_SHIFT			16

#define NET_INET_CLASS_C_ADDR(addr) \
			(((unsigned int)(addr) & 0xE0000000) == 0xC0000000)
#define NET_INET_CLASS_C_HOST_MASK		0x000000FF
#define NET_INET_CLASS_C_NET_MASK		0xFFFFFF00
#define NET_INET_CLASS_C_SHIFT			8

/*
 * Class D: multicast addressing (see RFC988, RFC966).
 */
#define NET_INET_CLASS_D_ADDR(addr) \
			(((unsigned int)(addr) & 0xF0000000) == 0xE0000000)
#define NET_INET_CLASS_D_NET_MASK		0x0FFFFFFF

/*
 * Class E: reserved.
 */
#define NET_INET_CLASS_E_ADDR(addr) \
			(((unsigned int)(addr) & 0xF0000000) == 0xF0000000)

/*
 * Synonym address for the local host.
 */
#define NET_INET_LOCAL_HOST		0x7F000001



/*
 * Structure of an Internet packet header, according to RFC791.
 */

typedef struct {
#if  BYTE_ORDER == LITTLE_ENDIAN
    unsigned int	headerLen:4,	/* Length of header in 32-bit words. */
			version:4;	/* Version. Defined below. */
#else
    unsigned int	version:4,
			headerLen:4;
#endif
    unsigned char	typeOfService;	/* See below. */
    unsigned short	totalLen;	/* Length in bytes of the 
					 * datagram including header and data.*/
    unsigned short	ident;		/* Used for fragmentation reassembly. */
#if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int	fragOffset:13,	/* Offset from beginning of unfragmented
					 * packet. */
			flags:3;	/* Fragment flags. See below. */
#else
    unsigned int	flags:3,
			fragOffset:13;
#endif
    unsigned char	timeToLive;	/* Max. time the packet is allowed to
					 * be in the internet system. When TTL
					 * goes to 0, the packet is destroyed.*/
    unsigned char	protocol;	/* Protocol used in the data portion.
					 * Defined below. */
    unsigned short	checksum;	/* Checksum of the header. */
    Net_InetAddress	source;		/* Source of the packet. */
    Net_InetAddress	dest;		/* Destination of the packet. */
    /*
     * Options may follow at this point, before the data.
     */
} Net_IPHeader;

/*
 * IP Pseudo Header is used by UDP and TCP checksums to make sure
 * the packet is not mis-routed.
 */
typedef struct {
    Net_InetAddress	source;		/* Source of the packet. */
    Net_InetAddress	dest;		/* Destination of the packet. */
    unsigned char	zero;		/* must be zero. */
    unsigned char	protocol;	/* Protocol # (UDP, TCP). */
    unsigned short	len;		/* # of bytes in header and data. */
} Net_IPPseudoHdr;

#define NET_IP_VERSION		4
#define NET_IP_MAX_HDR_SIZE	60

/*
 * IP implementation parameters:
 *  NET_IP_MAX_TTL		- Max. value for timeToLive field (in seconds).
 *  NET_IP_MAX_FRAG_TTL		- Max. time-to-live for fragments.
 *  NET_IP_TTL_DECR		- The value ttl is decremented by when a 
 *				   packet is forwarded.
 *  NET_IP_MAX_SEG_SIZE		- Max. segment size.
 *  NET_IP_MAX_PACKET_SIZE	- Largest packet size.
 */

#define NET_IP_MAX_TTL			255
#define NET_IP_MAX_FRAG_TTL		15
#define NET_IP_TTL_DECR			1
#define NET_IP_MAX_SEG_SIZE		576
#define NET_IP_MAX_PACKET_SIZE		65535

/*
 * Definitions of the typeOfService flags in the Internet header.
 * According to p. 12 of RFC791, the type of service field has the
 * the following subfields:
 *	precedence:	bits 0-2	(most sig. bits)
 * 	delay		bit  3
 * 	thruput		bit  4
 * 	reliability	bit  5
 * 	reserved	bit  6-7	(least sig. bits)
 * The value must be composed of the OR of one flag from each of the following
 * groups:
 */

#define NET_IP_SERV_PREC_NET_CTL	0xE0
#define NET_IP_SERV_PREC_INET_CTL	0xC0
#define NET_IP_SERV_PREC_CRITIC		0xA0
#define NET_IP_SERV_PREC_FLASH_OVR	0x80
#define NET_IP_SERV_PREC_FLASH		0x60
#define NET_IP_SERV_PREC_IMMED		0x40
#define NET_IP_SERV_PREC_PRIORITY	0x20
#define NET_IP_SERV_PREC_ROUTINE	0x00

#define NET_IP_SERV_NORM_DELAY		0x00
#define NET_IP_SERV_LOW_DELAY		0x10

#define NET_IP_SERV_NORM_THRUPUT	0x00
#define NET_IP_SERV_HIGH_THRUPUT	0x08

#define NET_IP_SERV_NORM_RELIABL	0x00
#define NET_IP_SERV_HIGH_RELIABL	0x04

/*
 * Values for the flags field in the IP header.
 *	NET_IP_LAST_FRAG	last fragment of the bunch. implies the
 *				packet can be fragmented if necessary.
 *	NET_IP_MORE_FRAGS	expect more fragments.
 *	NET_IP_DONT_FRAG	the can't be fragmented so discard if can't
 *				transmit.
 *
 * The high-order bit of the field must be 0.
 */

#define NET_IP_LAST_FRAG		0x0
#define NET_IP_MORE_FRAGS		0x1
#define NET_IP_DONT_FRAG		0x2


/*
 * Values for the protocol field in the IP packet header from RFC990.
 * The RFC does not assign a number for IP, so we use 0 (even though
 * it is reserved).
 */

#define NET_IP_PROTOCOL_IP		0
#define NET_IP_PROTOCOL_ICMP		1
#define NET_IP_PROTOCOL_TCP		6
#define NET_IP_PROTOCOL_EGP		8
#define NET_IP_PROTOCOL_UDP		17
#define NET_IP_PROTOCOL_SPRITE		90


/*
 * Definitions of IP options.
 * There are two formats for the options:
 *   1) single octet of option-type.
 *   2) an option-type octet, an option length, and the actual data octets.
 *
 * The option type is composed of 3 fields: copied flag (1 bit), 
 * option class (2 bits), and an option number (5 bits).
 */

/*
 * Definitions of offsets within an option for the type, length and next-
 * option-offset octets. The minimum value for the next-option offset 
 * ("pointer") is 4.  The pointer is used in the 3 routing options and 
 * the timestamp option.
 */
#define NET_IP_OPT_TYPE_OFFSET		0
#define NET_IP_OPT_LEN_OFFSET		1
#define NET_IP_OPT_PTR_OFFSET		2
#define NET_IP_OPT_MIN_PTR		4

#define NET_IP_OPT_MAX_LEN		40

/*
 * Macros to access the option-type fields.
 *  NET_IP_OPT_COPIED	- 0 means not copied, 1 means copied.
 *  NET_IP_OPT_CLASS	- see below.
 *  NET_IP_OPT_NUMBER	- see below.
 */

#define NET_IP_OPT_COPIED(opt)		((opt) & 0x80)
#define NET_IP_OPT_CLASS(opt)		((opt) & 0x60)
#define NET_IP_OPT_NUMBER(opt)		((opt) & 0x1f)

/*
 * Values for NET_IP_OPT_CLASS as masks.
 */
#define NET_IP_OPT_CLASS_CONTROL		0x00
#define NET_IP_OPT_CLASS_RESERVED1		0x20
#define NET_IP_OPT_CLASS_DEBUG			0x40
#define NET_IP_OPT_CLASS_RESERVED2		0x60

/*
 * Complete values for the option types, including the copied and class fields
 * as well as the number field.   The definitions are listed in the same 
 * order as described in RFC791, p. 16ff.
 */

#define NET_IP_OPT_END_OF_LIST		0x00
#define NET_IP_OPT_NOP			0x01
#define NET_IP_OPT_SECURITY		0x82
#define NET_IP_OPT_LOOSE_ROUTE		0x83
#define NET_IP_OPT_STRICT_ROUTE		0x89
#define NET_IP_OPT_REC_ROUTE		0x07
#define NET_IP_OPT_STREAM_ID		0x88
#define NET_IP_OPT_TIMESTAMP		0x44

/*
 * Foramt of the timestamp option header.
 */
typedef struct {
    unsigned char	type;
    unsigned char	len;			/* Total len of option. */
    unsigned char	pointer;		/* Offset into option where
						 * to add t.s. */
#if  BYTE_ORDER == LITTLE_ENDIAN
    unsigned int	flags:4,		/* Defined below. */
			overflow:4;		/* # of hosts who weren't
						 * able to add a t.s. */
#else
    unsigned int	overflow:4,
			flags:4;
#endif
} Net_IPTimestampHdr;

/*
 *  Values of the flags field in Net_IPTimestampHdr.
 */
#define NET_IP_OPT_TS_ONLY		0
#define NET_IP_OPT_TS_AND_ADDR		1
#define NET_IP_OPT_TS_ADDR_SPEC		3

/*
 * Interet time value is a "right-justified, 32-bit" time in milliseconds
 * since midnight Universal Time (UT).
 */
typedef int Net_InetTime;


/*
 * Structure of an TCP packet header, according to RFC793.
 */

typedef struct {
    unsigned short	srcPort;	/* Source port # */
    unsigned short	destPort;	/* Destination port # */
    unsigned int	seqNum;		/* Seq. # of 1st byte in the segment */
    unsigned int	ackNum;		/* Next seq. # we expect */
#if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int	reserved1:4,	/* Must be zero. */
			dataOffset:4,	/* # of 32-bit words in this header */
			flags:6,	/* Control flags, define below. */
			reserved2:2;	/* Must be zero */
#else
    unsigned int	dataOffset:4,
			reserved:6,
			flags:6;
#endif
    unsigned short	window;		/* # of bytes we are willing to accept*/
    unsigned short	checksum;	/* Checksum of IP pseudo-header, 
					 * TCP header and data. */
    unsigned short	urgentOffset;	/* Start of urgent data as an offset
					 * from the seq. # of this segment. */
} Net_TCPHeader;

#define NET_TCP_MAX_HDR_SIZE	60
#define NET_TCP_TTL		30

/*
 * TCP header control flags: (p. 16, rfc793)
 *
 *	URG -  Urgent Pointer field significant
 *	ACK -  Acknowledgment field significant
 *	PSH -  Push Function
 *	RST -  Reset the connection
 *	SYN -  Synchronize sequence numbers
 *	FIN -  No more data from sender
 */

#define NET_TCP_FIN_FLAG	0x01
#define NET_TCP_SYN_FLAG	0x02
#define NET_TCP_RST_FLAG	0x04
#define NET_TCP_PSH_FLAG	0x08
#define NET_TCP_ACK_FLAG	0x10
#define NET_TCP_URG_FLAG	0x20


/*
 * TCP options flags:		(p. 16, rfc793)
 *	EOL		- End of option list
 *	NOP		- No-operation
 *	MAX_SEG_SIZE	- Maximum segment size
 */

#define NET_TCP_OPTION_EOL		0x0
#define NET_TCP_OPTION_NOP		0x1
#define NET_TCP_OPTION_MAX_SEG_SIZE	0x2

#define NET_TCP_MAX_SEG_SIZE		512



/*
 * Structure of an UDP packet header, according to RFC768.
 */

typedef struct {
    unsigned short	srcPort;	/* Source port. */
    unsigned short	destPort;	/* Destination port. */
    unsigned short	len;		/* # of bytes in header and data. */
    unsigned short	checksum;	/* Checksum of IP pseudo-header,
					 * UDP header and data. */
} Net_UDPHeader;

#define NET_UDP_TTL			30


/*
 * Structure of various ICMP packets, according to RFC792.
 * The data structures for ICMP data is broken into 2 parts: 
 * a 4-byte header that is common to all ICMP packet types and
 * a type-dependent data part.
 *
 * The packets that include the IP header + the first 64 bits of data
 * consist of the basic header, plus room for the header options followed
 * by space for the data. Enough space to hold the maximum amount of
 * header options is reserved; if the size of the options is smaller than
 * maximum, the 64 bits of data will be placed immediately after the end
 * of the options. Therefore the "char data[8]" field is there to reserve
 * space in case of a maximum-sized header is present.
 */

typedef struct {
    unsigned char	type;		/* Type of message, define below. */
    unsigned char	code;		/* Type sub code, define below. */
    unsigned short	checksum;	/* Checksum of ICMP message. */
} Net_ICMPHeader;

/*
 * Data for "Destination unreachable", "Time exceeded", 
 *  "Source Quench" messages.
 */
typedef struct {
    int			unused;	
    Net_IPHeader	ipHeader;
    char		hdrOptions[NET_IP_MAX_HDR_SIZE - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataMisc;

/*
 * Data for "Redirect" message.
 */
typedef struct {
    Net_InetAddress	gatewayAddr;
    Net_IPHeader	ipHeader;
    char		hdrOptions[NET_IP_MAX_HDR_SIZE - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataRedir;

/*
 * Data for "Parameter Problem" message.
 */
typedef struct {
    unsigned char	paramOffset;
    char		unused1;
    short		unused2;
    Net_IPHeader	ipHeader;
    char		hdrOptions[NET_IP_MAX_HDR_SIZE - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataParam;

/*
 * Data for "Information Request", "Information Reply", "Echo" and "Echo Reply"
 * messages. The echo messages also have additional data following.
 */
typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
} Net_ICMPDataInfoEcho;


/*
 * Data for "Address Mask Request" and "Address Mask Reply" messages.
 */
typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
    unsigned int	addrMask;
} Net_ICMPDataMask;

/*
 * Data for "Timestamp" and "Timestamp Reply" message.
 */
typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
    unsigned int	origTime;
    unsigned int	recvTime;
    unsigned int	transmitTime;
} Net_ICMPDataTime;


/*
 * A complete definition of an ICMP packet.
 */

typedef struct {
    Net_ICMPHeader	header;
    union {
	Net_ICMPDataMisc	overlay;	/* used to format packets that
						 * return the IP header & data*/
	Net_ICMPDataMisc	unreach;	/* NET_ICMP_UNREACHABLE */
	Net_ICMPDataMisc	timeExceed;	/* NET_ICMP_TIME_EXCEED */
	Net_ICMPDataMisc	quench;		/* NET_ICMP_SOURCE_QUENCH */
	Net_ICMPDataParam	param;		/* NET_ICMP_PARAM_PROB */
	Net_ICMPDataRedir	redirect;	/* NET_ICMP_REDIRECT */
	Net_ICMPDataTime	timeStamp;	/* NET_ICMP_TIMESTAMP, _REPLY */
	Net_ICMPDataMask	mask;		/* NET_ICMP_MASK_REQ, _REPLY */
	Net_ICMPDataInfoEcho	info;		/* NET_ICMP_INFO_REQ , _REPLY */
	Net_ICMPDataInfoEcho	echo;		/* NET_ICMP_ECHO, _REPLY */
    } data;
} Net_ICMPPacket;

#define NET_ICMP_MIN_LEN	8

/*
 * Definition of type and code field values.
 */

#define	NET_ICMP_ECHO_REPLY		0	/* Echo reply */

#define	NET_ICMP_UNREACHABLE		3	/* Dest unreachable, codes: */
#define	 NET_ICMP_UNREACH_NET		   0	/*  - Bad net */
#define	 NET_ICMP_UNREACH_HOST		   1	/*  - Bad host */
#define	 NET_ICMP_UNREACH_PROTOCOL	   2	/*  - Bad protocol */
#define	 NET_ICMP_UNREACH_PORT		   3	/*  - Bad port */
#define	 NET_ICMP_UNREACH_NEED_FRAG	   4	/*  - IP_DF caused drop */
#define	 NET_ICMP_UNREACH_SRC_ROUTE	   5	/*  - Source route failed */

#define	NET_ICMP_SOURCE_QUENCH		4	/* Source quench */
#define	NET_ICMP_REDIRECT		5	/* Shorter route, codes: */
#define	 NET_ICMP_REDIRECT_NET		   0	/*  - For network */
#define	 NET_ICMP_REDIRECT_HOST		   1	/*  - For host */
#define	 NET_ICMP_REDIRECT_TOS_NET	   2	/*  - For type of service and 
						 *    net. */
#define	 NET_ICMP_REDIRECT_TOS_HOST	   3	/*  - For type of service and 
						 *     host. */
#define	NET_ICMP_ECHO			8	/* Echo service */
#define	NET_ICMP_TIME_EXCEED		11	/* Time exceeded, code: */
#define	 NET_ICMP_TIME_EXCEED_TTL	   0	/*  - ttl==0 in transit */
#define	 NET_ICMP_TIME_EXCEED_REASS	   1	/*  - ttl==0 in reassembly */
#define	NET_ICMP_PARAM_PROB		12	/* IP header bad */
#define	NET_ICMP_TIMESTAMP		13	/* Timestamp request */
#define	NET_ICMP_TIMESTAMP_REPLY	14	/* Timestamp reply */
#define	NET_ICMP_INFO_REQ		15	/* Information request */
#define	NET_ICMP_INFO_REPLY		16	/* Information reply */
#define	NET_ICMP_MASK_REQ		17	/* Address mask request */
#define	NET_ICMP_MASK_REPLY		18	/* Address mask reply */

#define	NET_ICMP_MAX_TYPE		18


/*
 * Definition of a socket address. To be compatible with the Unix sockaddr_in
 * structure, there are address family and padding fields. They aren't used
 * by the Sprite Internet server.
 */
typedef struct {
    short		addrFamily;	/* For Unix compatibilty, == AF_INET */
    unsigned short	port;
    Net_InetAddress	address;
    char		padding[8];	/* Not used. */
} Net_InetSocketAddr;

#endif /* _NETINET */
