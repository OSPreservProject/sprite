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
#include "netInet.h"
#include "netEther.h"
#include "user/net.h"
#include "net.h"
#include "dbg.h"
#include "dbgInt.h"


Boolean			dbgGotPacket;	

static	Boolean		dbgValidatePacket;


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
    Dbg_RawRequest     	rawRequest;
    int        		dataLength;
    Net_EtherHdr        *etherHdrPtr;
    Boolean		goodPacket;
    int			type;

    /*
     * Toss random non IP packets.
     */
    led_display(GOT_PACKET_LED, LED_OK, FALSE);
    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    type = Net_NetToHostShort(NET_ETHER_HDR_TYPE(*etherHdrPtr));
    if (type != NET_ETHER_IP) {
        if (dbgTraceLevel >= 5) {
            printf("Non-IP (Type=0x%x) ", type);
        }
	led_display(0, LED_FOUR, FALSE);
        return;
    }
    /*
     * If we aleady have one, toss this one.
     */
    if (dbgGotPacket) {
        return;
    }
    if (dbgTraceLevel >= 4) {
        printf("Validating packet\n");
    }
    /*
     * Extract the headers from the packet. We have to do it this way
     * because structures may be padded on this machine.
     */
    NET_ETHER_HDR_COPY(*etherHdrPtr, rawRequest.etherHeader);
    /*
     * Ethernet header is 14 bytes. 
     */
    packetPtr += 14; 
    rawRequest.ipHeader = * ((Net_IPHeader *) packetPtr);
    /*
     * Length of IP header is stored in header in units of 4 bytes.
     */
    packetPtr += rawRequest.ipHeader.headerLen * 4;
    rawRequest.udpHeader = * ((Net_UDPHeader *) packetPtr);
    /*
     * UDP header is 8 bytes.
     */
    packetPtr += 8;
    goodPacket = DbgValidatePacket(packetLength - sizeof(Net_EtherHdr),
                                   &rawRequest, &dataLength);
    if (goodPacket) {
	rawRequest.request = * ((Dbg_Request *) packetPtr);
	/*
	 * If it is for us then save the ethernet header so we have
	 * an address to send the reply too.
	 */
        if (dbgTraceLevel >= 4) {
            printf("Got a packet: length=%d\n", dataLength);
        }
	if (rawRequest.request.header.magic != DBG_MAGIC) {
	    if (dbgTraceLevel >= 4) {
		printf("Got a packet with bad magic 0x%x\n", 
		       rawRequest.request.header.magic);
	    }
	    led_display(BAD_PACKET_MAGIC_LED, LED_OK, TRUE);
	    return;
	}
	if (dataLength != sizeof(Dbg_Request)) {
	    if (dbgTraceLevel >= 4) {
		printf("Got a request with wrong size (%d).\n", dataLength); 
	    }
	    led_display(BAD_PACKET_SIZE_LED, LED_OK, TRUE);
	    return;
	}
	dbgGotPacket = TRUE;
	dbgRawRequest = rawRequest;
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DbgValidatePacket --
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
DbgValidatePacket(size, reqPtr, lenPtr)

    int				size;		/* IP packet size in bytes. */
    register Dbg_RawRequest	*reqPtr;	/* Ptr to raw request */
    int				*lenPtr;	/* Size of data in bytes.(out)*/
{
    register Net_IPHeader	*ipPtr;
    register Net_UDPHeader	*udpPtr;
    register int		headerLenInBytes;

    if (size < sizeof(Net_IPHeader)) {
	if (dbgTraceLevel >= 4) {
	    printf("Validate_Packet: Bad size %d\n", size);
	}
	led_display(0, LED_THREE, TRUE);
	return(FALSE);
    }

    ipPtr = &reqPtr->ipHeader;
    headerLenInBytes = ipPtr->headerLen * 4;
    udpPtr = &reqPtr->udpHeader;

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
	led_display(1, LED_THREE, TRUE);
	return(FALSE);
    } else if (Net_NetToHostShort(ipPtr->totalLen) < ipPtr->headerLen) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 2: %d, %d\n", 
			Net_NetToHostShort(ipPtr->totalLen), ipPtr->headerLen);
	}
	led_display(2, LED_THREE, TRUE);
	return(FALSE);
    } else if (Net_InetChecksum(headerLenInBytes, (Address) ipPtr) != 0) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 3 (IP checksum: %x)\n", ipPtr->checksum);
	}
	led_display(3, LED_THREE, TRUE);
	return(FALSE);
    } else if (ipPtr->protocol != NET_IP_PROTOCOL_UDP) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 4: %d\n", ipPtr->protocol);
	}
	led_display(4, LED_THREE, TRUE);
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->len) < sizeof(Net_UDPHeader)) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 5: %d, %d\n",
		    Net_NetToHostShort(udpPtr->len), sizeof(Net_UDPHeader));
	}
	led_display(5, LED_THREE, TRUE);
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->destPort) != DBG_UDP_PORT) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 6: %d, %d\n", 
		    Net_NetToHostShort(udpPtr->destPort), DBG_UDP_PORT);
	}
	led_display(6, LED_THREE, FALSE);
	return(FALSE);
    } else if ((ipPtr->flags & NET_IP_MORE_FRAGS) || (ipPtr->fragOffset != 0)) {
	if (dbgTraceLevel >= 5) {
	    printf("Failed case 7: %d, %d\n",
		(ipPtr->flags & NET_IP_MORE_FRAGS), (ipPtr->fragOffset != 0));
	}
	led_display(7, LED_THREE, TRUE);
	return(FALSE);
    } 

    /*
     * If the UDP packet was sent with a checksum, the checksum will be
     * non-zero.
     */
#if 0
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
	    led_display(8, LED_THREE, TRUE);
	    return(FALSE);
	}
    }
#endif
    *lenPtr	   = Net_NetToHostShort(udpPtr->len) - sizeof(Net_UDPHeader);

    if (dbgTraceLevel >= 4) {
	printf("Validate_Packet: Good packet\n");
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Dbg_FormatPacket --
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

void
Dbg_FormatPacket(dataSize, gatherPtr)

    int			dataSize;
    Net_ScatterGather	*gatherPtr;
{
    Net_EtherHdr		*etherPtr;
    Net_IPHeader 	*ipPtr;		/* Ptr to IP header. */
    Net_UDPHeader 	*udpPtr;	/* Ptr to UDP header. */
    static	int		ident = 0;

    etherPtr = (Net_EtherHdr *) dbgRawReply.etherHeader;
    ipPtr  = &dbgRawReply.ipHeader;
    udpPtr = &dbgRawReply.udpHeader;

    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_SOURCE(dbgRawRequest.etherHeader),
			NET_ETHER_HDR_DESTINATION(*etherPtr));
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(dbgRawRequest.etherHeader),
			NET_ETHER_HDR_SOURCE(*etherPtr));
    NET_ETHER_HDR_TYPE(*etherPtr) = 
	NET_ETHER_HDR_TYPE(dbgRawRequest.etherHeader);
    ipPtr->version	= NET_IP_VERSION;
    ipPtr->headerLen	= sizeof(Net_IPHeader) / 4;
    ipPtr->typeOfService = 0;
    ipPtr->totalLen	= Net_HostToNetShort(sizeof(*ipPtr) + 
					     sizeof(*udpPtr) + dataSize);
    ipPtr->ident	= ident++;
    ipPtr->fragOffset	= 0;
    ipPtr->flags	= 0;
    ipPtr->timeToLive	= NET_IP_MAX_TTL;
    ipPtr->protocol	= NET_IP_PROTOCOL_UDP;
    ipPtr->source	= dbgRawRequest.ipHeader.dest;
    ipPtr->dest		= dbgRawRequest.ipHeader.source;
    ipPtr->checksum	= 0;
    ipPtr->checksum	= Net_InetChecksum(sizeof(Net_IPHeader),
					   (Address) ipPtr);
    udpPtr->srcPort	= Net_HostToNetShort(DBG_UDP_PORT);
    udpPtr->destPort	= dbgRawRequest.udpHeader.srcPort;
    udpPtr->len		= Net_HostToNetShort(sizeof(*udpPtr) + dataSize);
    udpPtr->checksum	= 0;
    gatherPtr[0].bufAddr = (Address) ipPtr;
    gatherPtr[0].length = sizeof(*ipPtr);
    gatherPtr[0].mutexPtr = (Sync_Semaphore *) NIL;
    gatherPtr[1].bufAddr = (Address) udpPtr;
    gatherPtr[1].length = sizeof(*udpPtr);
    gatherPtr[1].mutexPtr = (Sync_Semaphore *) NIL;
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

