/* 
 * dbgIP.c --
 *
 *	Routines to handle the IP/UDP protocol for the debugger. Implements the
 *	kernel debugging stubs the use the ethernet driver. See dbgInt.h for
 *	a description of the protocol implemented.
 *
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
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "machparam.h"
#include "sprite.h"
#include "sys.h"
#include "dbg.h"
#include "dbgInt.h"
#include "netInet.h"
#include "netEther.h"
#include "user/net.h"
#include "net.h"


int	dbgTraceLevel;

/*
 * Have we gotten a packet yet.
 */
Boolean initialized = FALSE;

/*
 * Number for net polls before timing out. This value depends on the network,
 * ethernet driver, and machine speed.
 */

#define	TIMEOUT_VALUE	100000

/*
 * The ethernet driver works on a call-back basis and can't read like a 
 * normal device.  When debugging, the ethernet driver is call thru
 * Net_RecvPoll(). If a packet has arrived then the debugger is called back 
 * thru the routine Dbg_InputPacket(). Dbg_InputPacket validates the 
 * incomming packet and stores the header info in a Dbg_Packet structure
 * and the data in the specified data buffer.
 *
 * The following variables that start with "packet*" are filled in by 
 * Dbg_InputPacket.
 */

/*
 * Header info for the current packet received. The structure stores enough
 * information to allow the stub to reply to a request.
 */

typedef struct Dbg_Packet {
    Net_EtherHdr 	etherHdr;	/* Ethernet header of packet. */
    Net_InetAddress	myIPaddr;	/* What the debugger thinks my 
					 * IP address is.
					 */
    Net_InetAddress	debuggerIPaddr; /* The IP address of the debugger.*/
    int			debuggerPort;	/* The UDP port number of debugger. */
    int			type;		/* The debugger packet type. See 
					 * below for possible type values. 
					 */
    int			initial;	/* 1 if seqNumber is start of sequence.
					 * This allows for a debugger to exit
					 * and be restarted without having
					 * to remember this last seq number.
					 */
    unsigned int	seqNumber;	/* Sequence number of packet. */
    int			dataLength;	/* The length of the data buffer of
					 * the packet.
					 */
    Boolean		overflow;	/* TRUE if the packet's data part was
					 * too large for the specified data 
					 * buffer.
					 */
} Dbg_Packet;

/*
 * Information on the packet received. This is filled in by Dbg_InputPacket.
 */

static Dbg_Packet 	packet;

/*
 * The location of data buffer for the next packet. packetData is set by the
 * read routine to point to a buffer and filled in by Dbg_InputPacket.
 */
static  char	*packetData;

/*
 * The length of the buffer pointed too my packetData. 
 */
static int	packetDataLength = 0;

/*
 * Flag specifying if a new debugger packet is available.
 */
static 	Boolean	packetIsAvailable = FALSE;


/*
 * Next available command sequence number.
 */
static unsigned int nextSeqNum = 0;

/*
 * Forward declarations.
 */
static void Send_Packet();
static void Ack_Packet();
static void Format_Packet();
static Boolean Extract_Packet();
static Boolean Validate_Packet();


#ifdef DEBUG
/*
 * Forward declarations.
 */
static void 		TestInputProc();
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_InputPacket --
 *
 *     See if the current packet is for us. This routine is called from the
 *     ethernet driver when the machine is in the debugger.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     packetIsAvailable is set to true if we got a packet that we liked and
 *     the packet's content filled into the packet* variables above.
 *
 * ----------------------------------------------------------------------------
 */
void
Dbg_InputPacket(packetPtr, packetLength)
    Address     packetPtr;	/* The packet. */
    int         packetLength;	/* Its length. */
{
    Address     dataPtr;
    int         dataLength;
    Net_EtherHdr        *etherHdrPtr;
    Boolean	goodPacket;
    static char	alignedBuffer[NET_ETHER_MAX_BYTES];
    int		type;

    initialized = TRUE;

    /*
     * If we aleady have one, toss this one.
     */
    if (packetIsAvailable) {
        return;
    }
    /*
     * Sanity check to make sure the compiler is not padding structures.
     */
    if (sizeof(Net_EtherHdr) != 14) {
	panic( "Ethernet header wrong size!!\n");
    }
    /*
     * Toss random non IP packets.
     */
    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    type = Net_HostToNetShort(NET_ETHER_HDR_TYPE(*etherHdrPtr));
    if (type != NET_ETHER_IP) {
        if (dbgTraceLevel >= 5) {
            printf("Non-IP (Type=0x%x) ", type);
        }
        return;
    }
    if (dbgTraceLevel >= 4) {
        printf("Validating packet\n");
    }
    /*
     * Check to see if the packet is for us. If it is then Valiadate_Packet
     * will fill in the pointers passed to it.  
     */

    /*
     * Make sure the packet starts on a 32-bit boundry so that we can 
     * use structures for describe the data.
     */
    if ( (unsigned int) (packetPtr + sizeof(Net_EtherHdr)) & 0x3 ) {
	  bcopy (packetPtr + sizeof(Net_EtherHdr), alignedBuffer, 
			packetLength - sizeof(Net_EtherHdr));
	  packetPtr = alignedBuffer;
    }

    goodPacket = Validate_Packet(packetLength - sizeof(Net_EtherHdr),
               (Net_IPHeader *)packetPtr, &dataLength, &dataPtr,
              &packet.myIPaddr, &packet.debuggerIPaddr, 
	      (unsigned *)&packet.debuggerPort);
    if (goodPacket) {
	/*
	 * If it is for us then save the ethernet header so we have
	 * an address to send the reply too.
	 */
        if (dbgTraceLevel >= 4) {
            printf("Got a packet: length=%d\n", dataLength);
        }
	NET_ETHER_HDR_COPY(*etherHdrPtr, packet.etherHdr);
	/*
	 * Extract the rest of the packet including the dbg header.
	 */
        packetIsAvailable =  Extract_Packet(dataPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Dbg_IPgetpkt --
 *
 *	This routine reads a SPUR dbg "packet" from the ethernet driver
 *	and stores it in buf. The size of buffer is assume to be at least 
 *	as big as the data portition of the packet. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Dbg_IPgetpkt (buf)
     char *buf;
{
    Boolean	gotPkt = FALSE;

    /*
     * Loop until we get a good packet.
     */
    while (!gotPkt) { 
	/*
	 * Set the "packet*" pointers and poll the network until we get a 
	 * packet. 
	 */
	packetDataLength = DBG_MAX_BUFFER_SIZE;
	packetData =  buf;

	packetIsAvailable = FALSE;
	/*
	 * Spin polling for a packet. Debugger is repondsible for timing out.
	 */
	if (dbgTraceLevel >= 1) {
	    printf("getpkt: Waiting for a packet of seq number %d\n",
					nextSeqNum);
	}
	while (!packetIsAvailable) {
	    Net_RecvPoll();
		asm("cmp_trap always, r0, r0, $3");
	}
	if (dbgTraceLevel >= 1) {
	    printf("getpkt: Got a packet of seq number %d and type %d\n",
				packet.seqNumber, packet.type);
	}
	/*
	 * If it was a download packet just ACK it.
	 */
	if (packet.type == DBG_DOWNLOAD_PACKET) {
	    Ack_Packet();
	    continue;
	}

	/*
	 * If we have already seen the packet before 
	 * and its not an initial packet then toss it.
	 */
	if (packet.seqNumber <= nextSeqNum && !packet.initial) {
	    /*
	     * If it an old command packet assume our last ACK was lost and
	     * reAck it.
	     */
	     if (packet.type == DBG_DATA_PACKET) {
		 Ack_Packet();
	     } else if (packet.type == DBG_ACK_PACKET) {
		printf(
			  "getpkt: Ack with sequence number %d ignored\n",
			   packet.seqNumber);
	    } else {
		printf( 
			"getpkt: Got packet with bogus type field (%d)\n",
		        packet.type);
	    }
	    continue;
	}
	/*
	 * We got a packet with a seq num we like. 
	 */

	if (packet.type == DBG_DATA_PACKET) {
	    /* 
	    * This is what we been waiting for.
	    */
	    gotPkt = TRUE;
	    nextSeqNum = packet.seqNumber;
	    Ack_Packet();
	} else {
	    /*
	     * We got a packet with a bugs type field or an ACK with too 
	     * large of sequnce number.
	     */
	    printf( "getpkt: Bad packet seq = %d type = %d\n",
			packet.seqNumber, packet.type);
	}
    }

}

/*
 *----------------------------------------------------------------------
 *
 * Dbg_IPputpkt --
 *
 *	This routine puts a SPUR dbg "packet" to the ethernet driver
 *	with the address of the last incomming packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Packet is sent and seqNumbers updated.
 *
 *----------------------------------------------------------------------
 */
void
Dbg_IPputpkt (buf)
     char *buf;		/* Null terminated string to send. */
{
    Boolean	gotAck = FALSE;
    int		length;
    static	int	timeOut;
    int		seqNum;

    if (!initialized) {
	return;
    }
    /*
     * Get a fresh seq number for this data packet.
     */

    seqNum = ++nextSeqNum;

    length = strlen(buf) + 1;	/* Include null terminator. */

    if (length > DBG_MAX_BUFFER_SIZE) {
	panic( "dbg putpkt: buffer too large (%d)\n",length);
    }

    /*
     * Loop sending until we get the ack back.
     */
    while (!gotAck) { 
	if (dbgTraceLevel >= 1) {
	    printf("putpkt: Putting packet with seq number %d\n",seqNum);
	}

	Send_Packet(buf, length, DBG_DATA_PACKET, seqNum);

	packetIsAvailable = FALSE;
	/*
	 * Spin polling for a packet. We're responsible for timing out.
	 */
	timeOut = TIMEOUT_VALUE;
	while (!packetIsAvailable && timeOut > 0) {
		asm("cmp_trap always, r0, r0, $3");
	    Net_RecvPoll();
	    timeOut--;
	}
	if (!packetIsAvailable) {
	    if (dbgTraceLevel >= 1) {
		printf("putpkt: Timeout - resending packet.\n");
	    }
	    continue;	/* Resend the packet. */
	}
	/*
	 * If we have already seen the packet - toss it.
	 */
	if (dbgTraceLevel >= 1) {
	    printf("putpkt: Got a packet of seq number %d and type %d\n",
				packet.seqNumber, packet.type);
	}


	if (packet.seqNumber < nextSeqNum) {
	    /*
	     * If it an old data packet assume our last ACK was lost and
	     * reAck it.
	     */
	     if (packet.type == DBG_DATA_PACKET) {
		 Ack_Packet();
	     } else if (packet.type == DBG_ACK_PACKET) {
		/*
		 * It's safe to throw away old ack packets.
		 */
	    } else if (packet.type == DBG_DOWNLOAD_PACKET) {
		panic("putpkt: Bad packet type = %d\n",
				packet.type);
	    } else {
		printf("putpkt: Bad packet type = %d\n",
				packet.type);
	    }

	    continue;
	}
	/*
	 * Is this the ack we want?
	 */
	if (packet.seqNumber == seqNum) {
		gotAck = (packet.type == DBG_ACK_PACKET);
		if (!gotAck) { 
		    /*
		     * Sequence numbering is messed up.
		     */
		    panic( "putptk: Bad packet type %d seq %d\n",
				packet.seqNumber, packet.type);
		} 
	}

    }

}

/*
 * ----------------------------------------------------------------------------
 *
 * Extract_Packet --
 *
 *	Extract the debugger header and packet data from an incomming packet.
 *
 * Results:
 *     TRUE if the packet is good.
 *
 * Side effects:
 *	packetHeader and packetData are filled in.
 *
 * ----------------------------------------------------------------------------
 */
static Boolean
Extract_Packet(dataPtr)
    Address     dataPtr;	/* Start of packet data. */
{
    Dbg_PacketHeader	*header;
    static		Dbg_PacketHeader	alignedHeader;
    Address		startAddress;
    int			length, command;

    header = (Dbg_PacketHeader *) dataPtr;
    /*
     * Check to see if pointer is correctly aligned. If it is not aligned we
     * must make a copy.
     */
    if ((unsigned int) header & 0x3) {
	bcopy(dataPtr, (char *) &alignedHeader, sizeof(alignedHeader));
	header = &alignedHeader;
    }

    /*
     * We want the packet. Extract its header info and (data if any).
     */
    if (Net_NetToHostInt(header->magic) != DBG_HEADER_MAGIC) {
	printf(
		"Extract_Packet: Got packet with bad magic number (0x%x)\n",
		Net_NetToHostInt(header->magic));
	return(FALSE);
    }
    packet.type  = Net_NetToHostInt(header->type);
    packet.seqNumber = Net_NetToHostInt(header->sequenceNumber);
    packet.initial = Net_NetToHostInt(header->initial);
    startAddress = (Address) Net_NetToHostInt(header->startAddress);
    length = Net_NetToHostInt(header->dataLength);
    command = Net_NetToHostInt(header->command);
    /*
     * Process the DOWNLOAD command here.
     */
    if (packet.type == DBG_DOWNLOAD_PACKET) { 
	switch (command) {
	case DBG_DOWNLOAD_PING:
		/*
		 * Ping request - do nothing.
		 */
#ifdef		DEBUG_DOWNLOAD
		 printf("Download command PING\n");
#endif		DEBUG_DOWNLOAD
		 break;
	case DBG_DOWNLOAD_DATA_XFER:
		/*
		 * Data transfer request. 
		 */
#ifdef		DEBUG_DOWNLOAD
		 printf("Download command DATA_XFER start 0x%x length %d\n",			startAddress,length);
#else
		bcopy(dataPtr + sizeof(Dbg_PacketHeader), startAddress,length);
#endif		DEBUG_DOWNLOAD
		break;
	case DBG_DOWNLOAD_ZERO_MEM:
		/*
		 * Zero memory request.
		 */
#ifdef		DEBUG_DOWNLOAD
		 printf("Download command ZERO_MEM start 0x%x length %d\n",			startAddress,length);
#else
		 bzero(startAddress,length);
#endif		DEBUG_DOWNLOAD
		 break;
	case DBG_DOWNLOAD_JUMP:
		/*
		 * Start execute request.
		 */
		 Dbg_Start_Execution(startAddress);
		 break;
	default:
		printf( "Unknown DOWNLOAD command recieved\n");
	}
    } else {
	packet.overflow = FALSE;
	if (length > packetDataLength-1) {
	    packet.overflow = TRUE;
	    length = packetDataLength;
	}
	packet.dataLength = length;
	bcopy(dataPtr + sizeof(Dbg_PacketHeader), packetData, length);
    }
    return (TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * Send_Packet --
 *
 *	Send a packet of the specified type with the specified data 
 *	area.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Packet is sent.
 *
 *----------------------------------------------------------------------
 */
static void
Send_Packet (dataPtr, dataSize, type, seqNum)
    Address	dataPtr;	/* Start of data for packet. May be NIL */
    int		dataSize;	/* Size of the data for the packet. */
    int		type;		/* Packet type. */
    unsigned int	seqNum;	/* Packet sequence number. */
{
    Dbg_PacketHeader	header;
    Net_EtherHdr 	etherHdr;
    static Net_ScatterGather   gather;
    static char	packetBuffer[NET_ETHER_MAX_BYTES];
    int		packetLength;
    static char	emptyBuf;

    if (dataPtr == (Address) NIL) {
	dataPtr = (Address) &emptyBuf;
    }

    /*
     * Fill in the dbg header.
     */

    header.magic = Net_HostToNetInt(DBG_HEADER_MAGIC);
    header.sequenceNumber = Net_HostToNetInt(seqNum);
    header.dataLength = Net_HostToNetInt(dataSize);
    header.initial = Net_HostToNetInt(0);
    header.type = Net_HostToNetInt(type);

    /* 
     * Build the packet in packetBuffer.
     */
    Format_Packet(packet.myIPaddr, packet.debuggerIPaddr,
	   packet.debuggerPort, &header, dataSize, dataPtr, packetBuffer, 
	   &packetLength);

    /*
     * Turn the ethernet source and dest around from what we got.
     */
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(packet.etherHdr),
			NET_ETHER_HDR_SOURCE(etherHdr));
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_SOURCE(packet.etherHdr),
			NET_ETHER_HDR_DESTINATION(etherHdr));

    NET_ETHER_HDR_TYPE(etherHdr) = NET_ETHER_HDR_TYPE(packet.etherHdr);

    gather.bufAddr = (Address) packetBuffer;
    gather.length = packetLength;
    gather.mutexPtr = (Sync_Semaphore *) NIL;

    Net_OutputRawEther(&etherHdr, &gather, 1);

}

/*
 *----------------------------------------------------------------------
 *
 * Ack_Packet --
 *
 *	Acknowledges the data packet just received.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
Ack_Packet ()
{
	Send_Packet((Address) NIL, 0, DBG_ACK_PACKET, packet.seqNumber);
}



/*
 *----------------------------------------------------------------------
 *
 * Validate_Packet --
 *
 *	This routine checks to see if an IP/UDP packet is proper. This
 *	involves checking for the the proper sizes and that the packet 
 *	has not been corrupted. The packet is checked to see if has the 
 *	right UDP port. If the port matches then it is assumed that the 
 *	packet is addressed to us.
 *
 *	Note: IP options processing and fragment reasssembly are not done.
 *
 * Results:
 *	TRUE if packet processed successfully.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
Validate_Packet(size, ipPtr, lenPtr, dataPtrPtr, 
		destIPAddrPtr, srcIPAddrPtr, srcPortPtr)

    int				size;		/* IP packet size in bytes. */
    register Net_IPHeader	*ipPtr;		/* Ptr to IP packet buffer. */
    int				*lenPtr;	/* Size of data in bytes.(out)*/
    Address			*dataPtrPtr;	/* Address of data in the
						 * in the packet. (out) */
    Net_InetAddress		*destIPAddrPtr;	/* IP addr of this machine. 
						 * (out) */
    Net_InetAddress		*srcIPAddrPtr;	/* IP addr of sender. (out) */
    unsigned int		*srcPortPtr;	/* UDP port from the sender
						 * (needed to reply to the 
						 * sender). */
{
    register Net_UDPHeader	*udpPtr;
    register int		headerLenInBytes;

    if (size < sizeof(Net_IPHeader)) {
	if (dbgTraceLevel >= 4) {
	    printf("Validate_Packet: Bad size %d\n", size);
	}
	return(FALSE);
    }

    headerLenInBytes = ipPtr->headerLen * 4;
    udpPtr = (Net_UDPHeader *) ((Address) ipPtr + headerLenInBytes);

    /*
     * Validate the IP/UDP packet. The packet is checked for the following:
     *  1) have a proper IP header length,
     *  2) the total length of the packet must be larger than the IP header,
     *  3) the IP checksum is ok,
     *  4) the protocol is UDP,
     *  5) the UDP packet length is proper,
     *  6) the UDP dest. port matches the given port #,
     *  7) the IP packet is not a fragment.
     *
     * The checks are done in order of importance and likelihood of being false.
     * For instance, the header length should be validated before accessing
     * fields in the header and it is more likely that the UDP port won't match
     * than the packet is a fragment.
     */
    if (headerLenInBytes < sizeof(Net_IPHeader)) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 1: %d\n", headerLenInBytes);
	}
	return(FALSE);
    } else if (Net_NetToHostShort(ipPtr->totalLen) < ipPtr->headerLen) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 2: %d, %d\n", 
			Net_NetToHostShort(ipPtr->totalLen), ipPtr->headerLen);
	}
	return(FALSE);
    } else if (Net_InetChecksum(headerLenInBytes, (Address) ipPtr) != 0) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 3 (IP checksum: %x)\n", ipPtr->checksum);
	}
	return(FALSE);
    } else if (ipPtr->protocol != NET_IP_PROTOCOL_UDP) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 4: %d\n", ipPtr->protocol);
	}
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->len) < sizeof(Net_UDPHeader)) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 5: %d, %d\n",
		    Net_NetToHostShort(udpPtr->len), sizeof(Net_UDPHeader));
	}
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->destPort) != DBG_UDP_PORT) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 6: %d, %d\n", 
		    Net_NetToHostShort(udpPtr->destPort), DBG_UDP_PORT);
	}
	return(FALSE);
    } else if ((ipPtr->flags & NET_IP_MORE_FRAGS) || (ipPtr->fragOffset != 0)) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 7: %d, %d\n",
		(ipPtr->flags & NET_IP_MORE_FRAGS), (ipPtr->fragOffset != 0));
	}
	return(FALSE);
    } 

    /*
     * If the UDP packet was sent with a checksum, the checksum will be
     * non-zero.
     */
    if (udpPtr->checksum != 0) {
	Net_IPPseudoHdr		pseudoHdr;

	/*
	 * The checksum is computed for the IP "pseudo-header" and
	 * the UDP header and data. When the UDP checksum was calculated,
	 * the checksum field in the header was set to zero. When we 
	 * recalculate the value, we don't zero the field so the computed 
	 * value should be zero if the packet didn't get garbled.
	 */
	pseudoHdr.source	= ipPtr->source;
	pseudoHdr.dest		= ipPtr->dest;
	pseudoHdr.zero		= 0;
	pseudoHdr.protocol	= ipPtr->protocol;
	pseudoHdr.len		= udpPtr->len;
	if (Net_InetChecksum2((int) udpPtr->len, (Address) udpPtr, 
		&pseudoHdr) != 0) {

	    if (dbgTraceLevel >= 4) {
		printf("Validate_Packet: Bad UDP checksum: %x\n", 
				udpPtr->checksum);
	    }
	    return(FALSE);
	}
    }

    *lenPtr	   = Net_NetToHostShort(udpPtr->len) - sizeof(Net_UDPHeader);
    *dataPtrPtr	   = ((Address) udpPtr) + sizeof(Net_UDPHeader);
    *destIPAddrPtr = Net_NetToHostInt(ipPtr->dest);
    *srcIPAddrPtr  = Net_NetToHostInt(ipPtr->source);
    *srcPortPtr	   = Net_NetToHostShort(udpPtr->srcPort);

    if (dbgTraceLevel >= 4) {
	printf("Validate_Packet: Good packet\n");
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Format_Packet --
 *
 *	Formats  an IP/UDP/DBG packet for sending on the network.
 *	The IP addresses and UDP port arguments are assumed to be in the
 *	machine's native byte order. They are converted to network
 *	byte order when the header is formatted.
 *
 *	Note: The IP header checksum is computed but the UDP checksum is not. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
Format_Packet(srcIPAddr, destIPAddr, destPort, header, dataSize, dataPtr, 
		 packetBufferPtr, lengthPtr)

    Net_InetAddress	srcIPAddr;	/* IP address of this machine. */
    Net_InetAddress	destIPAddr;	/* IP address of destination. */
    unsigned int	destPort;	/* UDP port of destination. */
    Dbg_PacketHeader	*header;	/* DGB packet header. */
    int			dataSize;	/* Size in bytes of data in *dataPtr. */
    Address		dataPtr;	/* Buffer containing data. */
    Address		packetBufferPtr; /* Location to place packet. */
    int			*lengthPtr;	/* Packet length (OUT) */
{
    register Net_IPHeader *ipPtr;	/* Ptr to IP header. */
    register Net_UDPHeader *udpPtr;	/* Ptr to UDP header. */
    Dbg_PacketHeader	*dbgPtr;	/* Ptr to DBG header. */
    Address	dataAreaPtr;		/* Ptr to data area of packet. */
    static	int	ident = 0;

    ipPtr  = (Net_IPHeader *) packetBufferPtr;
    udpPtr = (Net_UDPHeader *) (packetBufferPtr + sizeof(Net_IPHeader));
    dbgPtr = (Dbg_PacketHeader *) (packetBufferPtr + sizeof(Net_UDPHeader)
				 + sizeof(Net_IPHeader));
    dataAreaPtr = packetBufferPtr +  sizeof(Net_UDPHeader) +
		 sizeof(Net_IPHeader) + sizeof(Dbg_PacketHeader);

    ipPtr->version	= NET_IP_VERSION;
    ipPtr->headerLen	= sizeof(Net_IPHeader) / 4;
    ipPtr->typeOfService = 0;
    ipPtr->totalLen	= Net_HostToNetShort(sizeof(Net_IPHeader) + 
					sizeof(Net_UDPHeader) + 
					sizeof(Dbg_PacketHeader) + dataSize);
    ipPtr->ident	= ident++;
    ipPtr->fragOffset	= 0;
    ipPtr->flags	= 0;
    ipPtr->timeToLive	= NET_IP_MAX_TTL;
    ipPtr->protocol	= NET_IP_PROTOCOL_UDP;
    ipPtr->source	= Net_HostToNetInt(srcIPAddr);
    ipPtr->dest		= Net_HostToNetInt(destIPAddr);
    ipPtr->checksum	= 0;
    ipPtr->checksum	= Net_InetChecksum(sizeof(Net_IPHeader), 
					(Address) ipPtr);

    udpPtr->srcPort	= Net_HostToNetShort(DBG_UDP_PORT);
    udpPtr->destPort	= Net_HostToNetShort(destPort);
    udpPtr->len		= Net_HostToNetShort(sizeof(Net_UDPHeader) +
					     sizeof(Dbg_PacketHeader) + 
					      dataSize);
    udpPtr->checksum	= 0;

    *dbgPtr = *header;

    bcopy(dataPtr, dataAreaPtr,dataSize);

    *lengthPtr = sizeof(Net_IPHeader) + sizeof(Net_UDPHeader) + 
		    sizeof(Dbg_PacketHeader) + dataSize; 
}



/*
 *----------------------------------------------------------------------
 *
 * TestInputProc --
 *
 *	Debugging code to print the header of an IP datagram.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef DEBUG
#include "sys.h"

static char srcAddr[18];
static char destAddr[18];
static char *ProtNumToName();

static void
TestInputProc(size, headerPtr)
    int				size;
    register Net_IPHeader       *headerPtr;
{
    unsigned short		checksum;


    (void) Net_InetAddrToString(&(headerPtr->source), srcAddr);
    (void) Net_InetAddrToString(&(headerPtr->dest), destAddr);

    printf("IP Packet: size = %d\n", size);
    printf("Protocol, version:	%s, %d\n", 
		    ProtNumToName(headerPtr->protocol),
		    headerPtr->version);
    printf("Src, dest addrs:	%s, %s\n", srcAddr, destAddr);
    printf("Header, total len:	%d, %d\n", 
		    headerPtr->headerLen, headerPtr->totalLen);

    checksum = headerPtr->checksum, 
    headerPtr->checksum = 0;
    printf("checksum, recomp:	%x, %x\n", checksum, 
		Net_InetChecksum((int)headerPtr->headerLen*4, 
					(Address)headerPtr));
    printf("Frag flags, offset, ID:	%x, %d, %x\n", 
		    headerPtr->flags, headerPtr->fragOffset, 
		    headerPtr->ident);
    printf("\n");

    return;
}

static char *
ProtNumToName(num) 
    unsigned char num;
{
    switch (num) {
	case NET_IP_PROTOCOL_IP:
	    return("IP");
	case NET_IP_PROTOCOL_ICMP:
	    return("ICMP");
	case NET_IP_PROTOCOL_TCP:
	    return("TCP");
	case NET_IP_PROTOCOL_EGP:
	    return("EGP");
	case NET_IP_PROTOCOL_UDP:
	    return("UDP");
	default:
	    return("???");
    }
}
#endif DEBUG

