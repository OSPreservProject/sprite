/*
 * netArp.h --
 *
 *	Declarations for arp and rarp.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _NETARP
#define _NETARP

/*
 * Sprite Address Resolution Protocol packet format.  These are used to
 * find out Sprite IDs for physical addresses and vice versa.  The ARP
 * protocol is simple:  a host broadcasts an Arp Request containing a
 * Sprite ID and waits for a reply that specifies the physical (ethernet)
 * address used to reach that Sprite host.  The Reverse Arp protocol is
 * similar.  A host broadcasts a request that contains a a physical
 * (ethernet) address and waits for a reply that specifies the matching
 * Sprite ID. 
 *
 * NOTE: This packet appears on the wire in network byte ordering.
 *
 */

#define	NUM_ARP_DATA_BYTES (2 * (sizeof(Net_EtherAddress) + sizeof(int)))

typedef struct NetSpriteArp {
    Net_ArpHeader arpHeader;	/* RFC826 standard header. The hardware addr
				 * space should be NET_ARP_TYPE_ETHER.  */
    unsigned char arpData[NUM_ARP_DATA_BYTES];
} NetSpriteArp;

/*
 * Macros for indexing into the arpData field.
 * ARP_SRC_ETHER_ADDR() - The address of the sender's ethernet address.
 * ARP_SRC_PROTO_ADDR() - The address of the sender's protocol address.
 * ARP_TARGET_ETHER_ADDR() - The address of the target's ethernet address.
 * ARP_TARGET_PROTO_ADDR() - The address of the target's protocol address.
 */

#define ARP_SRC_ETHER_ADDR(ap) ((Net_EtherAddress *) &((ap)->arpData[0]))
#define ARP_SRC_PROTO_ADDR(ap) \
		((char *) &((ap)->arpData[(ap)->arpHeader.hardwareAddrLen]))
#define	ARP_TARGET_ETHER_ADDR(ap) \
		((Net_EtherAddress *) \
		&((ap)->arpData[(ap)->arpHeader.hardwareAddrLen + \
					 (ap)->arpHeader.protocolAddrLen]))
#define	ARP_TARGET_PROTO_ADDR(ap) \
		((char *) &((ap)->arpData[2*(ap)->arpHeader.hardwareAddrLen + \
					 (ap)->arpHeader.protocolAddrLen]))




/*
 * State for the Address Resolution Protocol.
 */
typedef struct ArpState {
    List_Links		links;		/* Chain for all current ARPs */
    int			state;		/* See bits defined below */
    Timer_QueueElement	timeout;	/* Used for the call-back upon timeout*/
    Sync_Semaphore	*mutexPtr;	/* Used for synchronization */
    Sync_Condition	condition;	/* Used for synchronization */
    int			type;		/* Type of request. */
    ClientData		id;		/* Target ID, used to identify
					 * this ARP transaction from others */
    NetSpriteArp	packet;		/* Copy of reply packet */
} ArpState;

#define ARP_WANT_REPLY		0x1
#define ARP_HAVE_INPUT		0x2
#define ARP_IN_TIMEOUT_QUEUE	0x4

typedef struct ArpStatisitics {
    int	numArpRequests;		/* Number of these transmitted */
    int numArpReplies;		/* ditto */
    int numRevArpRequests;	/* ditto */
    int numRevArpReplies;	/* ditto */
    int numTimeouts;		/* Number of requests that were not responded
				 * to after a few broadcast attempts */
} ArpStatistics;

/*
 * A very simple packet list used to keep Arp packet headers and
 * the scatter/gather structure used to access them.  These must be
 * global data structures because the packet may be queued before output.
 * A monitor is used to synchronize access
 */
typedef struct ArpOutputQueue {
    Net_Interface	*interPtr;
    Net_EtherHdr	etherHdr;
    NetSpriteArp	packet;
    Net_ScatterGather	gather;
} ArpOutputQueue;

#define ARP_OUTPUT_QUEUE_LEN		3

/*
 * Another simple list of Arp packets used to pass info
 * from the interrupt handler that gets an ARP request to the
 * Proc_ServerProc that generates the reply.
 */

typedef struct ArpInputQueue {
    Net_Interface	*interPtr;
    NetSpriteArp	packet;
} ArpInputQueue;

#define ARP_INPUT_QUEUE_LEN		5

/*
 * Forward declarations.
 */

#endif /* _NETARP */

