/* 
 * dbgIP.c --
 *
 *	Routines to handle the IP/UDP protocol for the debugger. Validates
 *	input packets and formats pacjkets for output.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dbg.h"
#include "user/net.h"

extern	int	dbgTraceLevel;

#ifdef DEBUG
/*
 * Forward declarations.
 */
static void 		TestInputProc();
#endif


/*
 *----------------------------------------------------------------------
 *
 * Dbg_ValidatePacket --
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

Boolean
Dbg_ValidatePacket(size, ipPtr, lenPtr, dataPtrPtr, 
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
	    Sys_Printf("Dbg_ValidatePacket: Bad size %d\n", size);
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
	    Sys_Printf("Failed case 1: %d\n", headerLenInBytes);
	}
	return(FALSE);
    } else if (Net_NetToHostShort(ipPtr->totalLen) < ipPtr->headerLen) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 2: %d, %d\n", 
			Net_NetToHostShort(ipPtr->totalLen), ipPtr->headerLen);
	}
	return(FALSE);
    } else if (Net_InetChecksum(headerLenInBytes, (Address) ipPtr) != 0) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 3\n");
	}
	return(FALSE);
    } else if (ipPtr->protocol != NET_IP_PROTOCOL_UDP) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 4: %d\n", ipPtr->protocol);
	}
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->len) < sizeof(Net_UDPHeader)) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 5: %d, %d\n",
		    Net_NetToHostShort(udpPtr->len), sizeof(Net_UDPHeader));
	}
	return(FALSE);
    } else if (Net_NetToHostShort(udpPtr->destPort) < DBG_UDP_PORT) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 6: %d, %d\n", 
		    Net_NetToHostShort(udpPtr->destPort), DBG_UDP_PORT);
	}
	return(FALSE);
    } else if ((ipPtr->flags & NET_IP_MORE_FRAGS) || (ipPtr->fragOffset != 0)) {
	if (dbgTraceLevel >= 5) {
	    Sys_Printf("Failed case 7: %d, %d\n",
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
	if (~(Net_InetChecksum(sizeof(pseudoHdr), (Address) &pseudoHdr) +
		Net_InetChecksum((int) udpPtr->len, (Address) udpPtr)) != 0) {
	    if (dbgTraceLevel >= 4) {
		Sys_Printf("Dbg_ValidatePacket: Bad checksum\n");
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
	Sys_Printf("Dbg_ValidatePacket: Good packet\n");
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Dbg_FormatPacket --
 *
 *	Formats an IP/UDP packet for sending on the network.
 *	The first Dbg_PacketHdrSize() bytes of *dataPtr must not be used -- 
 *	this area is filled in with the IP and UDP headers.
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
Dbg_FormatPacket(srcIPAddress, destIPAddress, destPort, dataSize, dataPtr)

    Net_InetAddress	srcIPAddress;	/* IP address of this machine. */
    Net_InetAddress	destIPAddress;	/* IP address of destination. */
    unsigned int	destPort;	/* UDP port of destination. */
    int			dataSize;	/* Size in bytes of data in *dataPtr. */
    Address		dataPtr;	/* Buffer containing data. */
{
    register Net_IPHeader *ipPtr;	/* Ptr to IP header. */
    register Net_UDPHeader *udpPtr;	/* Ptr to UDP header. */
    static	int	ident = 0;

    ipPtr  = (Net_IPHeader *) dataPtr;
    udpPtr = (Net_UDPHeader *) (dataPtr + sizeof(Net_IPHeader));

    ipPtr->version	= NET_IP_VERSION;
    ipPtr->headerLen	= sizeof(Net_IPHeader) / 4;
    ipPtr->typeOfService = 0;
    ipPtr->totalLen	= Net_HostToNetShort(sizeof(Net_IPHeader) + 
					sizeof(Net_UDPHeader) + dataSize);
    ipPtr->ident	= ident++;
    ipPtr->fragOffset	= 0;
    ipPtr->flags	= 0;
    ipPtr->timeToLive	= NET_IP_MAX_TTL;
    ipPtr->protocol	= NET_IP_PROTOCOL_UDP;
    ipPtr->source	= Net_HostToNetInt(srcIPAddress);
    ipPtr->dest		= Net_HostToNetInt(destIPAddress);
    ipPtr->checksum	= 0;
    ipPtr->checksum	= 
	    Net_HostToNetShort(Net_InetChecksum(sizeof(Net_IPHeader), 
					(Address) ipPtr));

    udpPtr->srcPort	= Net_HostToNetShort(DBG_UDP_PORT);
    udpPtr->destPort	= Net_HostToNetShort(destPort);
    udpPtr->len		= Net_HostToNetShort(sizeof(Net_UDPHeader) + dataSize);
    udpPtr->checksum	= 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Dbg_PacketHdrSize --
 *
 *	Returns the number of bytes that Dbg_FormatPacket expects to
 *	format.
 *
 * Results:
 *	The size of the packet header.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Dbg_PacketHdrSize()
{
    return(sizeof(Net_IPHeader) + sizeof(Net_UDPHeader));
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

    Sys_Printf("IP Packet: size = %d\n", size);
    Sys_Printf("Protocol, version:	%s, %d\n", 
		    ProtNumToName(headerPtr->protocol),
		    headerPtr->version);
    Sys_Printf("Src, dest addrs:	%s, %s\n", srcAddr, destAddr);
    Sys_Printf("Header, total len:	%d, %d\n", 
		    headerPtr->headerLen, headerPtr->totalLen);

    checksum = headerPtr->checksum, 
    headerPtr->checksum = 0;
    Sys_Printf("checksum, recomp:	%x, %x\n", checksum, 
		Net_InetChecksum((int)headerPtr->headerLen*4, 
					(Address)headerPtr));
    Sys_Printf("Frag flags, offset, ID:	%x, %d, %x\n", 
		    headerPtr->flags, headerPtr->fragOffset, 
		    headerPtr->ident);
    Sys_Printf("\n");

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
