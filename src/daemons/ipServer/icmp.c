/* 
 * icmp.c --
 *
 *	Routines to handle the ICMP protocol, as specified in RFC 792 
 *	"Internet Control Message Protocol" (Sept. 1981) and RFC 950 
 *	"Internet Standard Subnetting Procedure" (Aug. 1985). 
 *	Incoming packets are validated and an action is taken depending 
 *	of the type of the packet. ICMP packets may be sent to a host 
 *	to indicate errors with the ICMP_SendErrorMsg routine.
 *
 *	Based on 4.3BSD  @(#)ip_icmp.c 7.6 (Berkeley) 8/31/87
 *
 * 	To do: handle
 *	 1) routing redirects.
 *	 2) source quench.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/icmp.c,v 1.10 90/08/06 17:17:56 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"
#include "ip.h"
#include "icmp.h"
#include "route.h"
#include "socket.h"
#include "raw.h"

static void	Input();
static void	ReturnToSender();
static void	SwapAddresses();

/*
 * Names of all ICMP types and names for the UNREACHABLE and REDIRECT codes.
 */
static char 	*typeNames[] = {
    "ECHO_REPLY",
    "type 1 - unused",
    "type 2 - unused",
    "UNREACHABLE",
    "SOURCE_QUENCH",
    "REDIRECT",
    "type 6 - unused",
    "type 7 - unused",
    "ECHO",
    "type 9 - unused",
    "type 10 - unused",
    "TIME_EXCEED",
    "PARAM_PROB",
    "TIMESTAMP",
    "TIMESTAMP_REPLY",
    "INFO_REQ",
    "INFO_REPLY",
    "MASK_REQ",
    "MASK_REPLY",
};

static char *unreachCodeNames[] = {
    "bad net", 
    "bad host", 
    "bad protocol", 
    "bad port", 
    "need to frag", 
    "src route failed",
};

static char *redirectCodeNames[] = {
    "network",
    "host",
    "type of service and network",
    "type of service and host",
};


/*
 *----------------------------------------------------------------------
 *
 * ICMP_Init --
 *
 *	Establishes the callback routine to handle incoming ICMP packets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A routine will be called whenever a ICMP packet arrives.
 *
 *----------------------------------------------------------------------
 */

void
ICMP_Init()
{
    IP_SetProtocolHandler(NET_IP_PROTOCOL_ICMP, Input);
}


/*
 *----------------------------------------------------------------------
 *
 * Input --
 *
 *	This routine is called whenever an ICMP packet arrives.
 *	Once the packet is validated by checking that the length, checksum
 *	and the type are proper, the type value is used to decide what action 
 *	to take. There are 2 kinds of ICMP types:
 *	1) informational: echo, timestamp, mask and address info requests and
 *	    replies.
 *	2) error: unreachable host, net, or port; bad options format;
 *	     route redirect, source quench, etc.
 *	The informational requests are handled in a separate routine. 
 *	Informational replies are passed to raw socket handler. The 
 *	error types are handled here and they indicate that there was a 
 *	problem with a packet we have sent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Info request packets are returned to the sender. Error packets pass
 *	information to higher-level protocols. Replies pass to the raw 
 *	socket layer.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
Input(netID, packetPtr)
    Rte_NetID  netID;		/* ID of the network the packet arrived on. */
    IPS_Packet	*packetPtr;	/* Packet descriptor. */
{
    register Net_ICMPPacket	*icmpPtr;
    register Net_IPHeader	*ipPtr;
    int				dataLen;
    unsigned char		type;
    ReturnStatus		status;

    stats.icmp.total++;

    /*
     * Determine the start of the ICMP part of the packet. Fill in the
     * packet descriptor in case we need to ship the packet back to the 
     * sender.
     */

    ipPtr   = packetPtr->ipPtr;
    icmpPtr = (Net_ICMPPacket *) ((Address)ipPtr + (ipPtr->headerLen * 4));
    packetPtr->data = (Address) icmpPtr;

    /*
     * We assume the ipPtr->totalLen is in host byte order.
     */
    dataLen = ipPtr->totalLen - (ipPtr->headerLen * 4);
    packetPtr->dataLen = dataLen;
    packetPtr->ipLen = ipPtr->headerLen * 4;
    packetPtr->hdrLen = 0;

    /*
     * Make sure the packet has the proper length.
     */
    if (dataLen < NET_ICMP_MIN_LEN) {
	stats.icmp.shortLen++;
	free(packetPtr->base);
	return;
    }

    /*
     * See if the checksum is OK. Since the checksum value in the packet is 
     * not zeroed when calculating the value here, the result should be 0
     * if the packet was not garbled.
     */
    if (Net_InetChecksum(dataLen, (Address) icmpPtr) != 0) {
	stats.icmp.badChecksum++;
	free(packetPtr->base);
	return;
    }

    /*
     * Make sure the type is in range before incrementing the counter. 
     * A unknown type is not rejected in order to give the packet
     * to the raw socket layer.  This allows development of new ICMP types.
     */

    type = icmpPtr->header.type; 

    if  (type > NET_ICMP_MAX_TYPE) {
	stats.icmp.badType++;
    } else {
	stats.icmp.inHistogram[type]++;
    }


    if (ips_Debug) {
	(void) fprintf(stderr, "ICMP Input: <%x>  <---  <%x>  %s", 
		Net_NetToHostInt(ipPtr->dest), 
		Net_NetToHostInt(ipPtr->source), 
		typeNames[type]);
	if (type ==  NET_ICMP_UNREACHABLE) {
	    (void) fprintf(stderr, " -- %s\n", unreachCodeNames[icmpPtr->header.code]);
	} else {
	    (void) fprintf(stderr, " -- %d\n", icmpPtr->header.code);
	}
    }

    switch (type) {

	/*
	 * The following types involve the return of some information
	 * to the sender. Users don't ever get to see these packets.
	 */

	case NET_ICMP_ECHO:
	case NET_ICMP_TIMESTAMP:
	case NET_ICMP_INFO_REQ:
	case NET_ICMP_MASK_REQ:
	    ReturnToSender(netID, type, dataLen, icmpPtr, packetPtr);
	    break;



	/*
	 * The following types are feedback about problems with packets
	 * that have been sent out. The higher-level protocol that sent
	 * the packet needs to be told about the problem.
	 */

	case NET_ICMP_UNREACHABLE:
	    switch (icmpPtr->header.code) {
		case NET_ICMP_UNREACH_NET:
		    status = NET_UNREACHABLE_NET;
		    break;

		case NET_ICMP_UNREACH_HOST:
		    status = NET_UNREACHABLE_HOST;
		    break;

		case NET_ICMP_UNREACH_PROTOCOL:
		    status = NET_CONNECT_REFUSED;
		    break;

		case NET_ICMP_UNREACH_PORT:
#ifdef	REAL_RAW
		    if(ips_Debug)
			(void) fprintf(stderr,
			    "ICMP Input: <%x>  <---  <%x> %s -- bad port\n", 
			    Net_NetToHostInt(ipPtr->dest), 
			    Net_NetToHostInt(ipPtr->source), 
			    typeNames[type]);
		    Raw_Input(NET_IP_PROTOCOL_ICMP, ipPtr->source, ipPtr->dest, 
				packetPtr);
#endif	REAL_RAW
		    status = NET_CONNECT_REFUSED;
		    break;

		case NET_ICMP_UNREACH_NEED_FRAG:
		    status = FS_BUFFER_TOO_BIG;
		    break;

		case NET_ICMP_UNREACH_SRC_ROUTE:
		    status = NET_UNREACHABLE_HOST;
		    break;

		default:
		    stats.icmp.badCode++;
		    status = SUCCESS;
		    break;
	    }
	    if (status != SUCCESS) {
		/*
		 * Pass the protocol, destination and the first 64 bits
		 * of the data to the error handler.
		 */
		int hdrLen = icmpPtr->data.unreach.ipHeader.headerLen * 4;

		Sock_ReturnError(status,
			(int)icmpPtr->data.unreach.ipHeader.protocol, 
			icmpPtr->data.unreach.ipHeader.dest,
			(Address) ((Address) &icmpPtr->data.unreach.ipHeader) + 
					hdrLen);
	    }
	    break;

	/*
	 * A source quench packet is sent by a gateway if it ran out of buffers
	 * or by a host if we have sent many packets too quickly. We need
	 * to cut back on the rate of delivery.
	 */
	case NET_ICMP_SOURCE_QUENCH:
	    /* To do: pass info to offending protocol layer. */
	    break;

	/*
	 * A gateway has informed us of a shorter path to a destination.
	 */
	case NET_ICMP_REDIRECT:
	    if (icmpPtr->header.code > NET_ICMP_REDIRECT_TOS_NET) {
		stats.icmp.badCode++;
	    } else {
		/*
		 * To do: give the information to the routing handler.
		 */
		if (icmpPtr->header.code == NET_ICMP_REDIRECT_HOST) {
			Rte_UpdateRoute(
			    Net_NetToHostInt(icmpPtr->data.redirect.ipHeader.dest),
					icmpPtr->data.redirect.gatewayAddr);
		}
		if (ips_Debug) {
		    (void) fprintf(stderr, 
			"ICMP Redirect: %s  new=<%x> old=<%x>\n",
			redirectCodeNames[icmpPtr->header.code],
			icmpPtr->data.redirect.gatewayAddr,
			icmpPtr->data.redirect.ipHeader.dest);
		}
	    }
	    break;

#ifndef	REAL_RAW
	/*
	 * The Time Exceeded type is sent when a gateway has found that 
	 * a packet's time to live value is 0 or when a host trying to 
	 * reassemble a fragmented packet has not received all of the 
	 * fragments within a certain amount of time.
	 */
	case NET_ICMP_TIME_EXCEED:
	    /*
	     * This message type is ignored.
	     */
	    if (icmpPtr->header.code > NET_ICMP_TIME_EXCEED_REASS) {
		stats.icmp.badCode++;
	    }
	    break;
#endif	REAL_RAW

	/*
	 * NET_ICMP_PARAM_PROB means there was a problem in the header 
	 * parameters of a packet we sent. The packet had to be discarded.
	 */
	case NET_ICMP_PARAM_PROB: {
		int hdrLen = icmpPtr->data.param.ipHeader.headerLen * 4;

		Sock_ReturnError((ReturnStatus) NET_BAD_OPTION, 
			    (int)icmpPtr->data.param.ipHeader.protocol, 
			    icmpPtr->data.param.ipHeader.dest,
			    (Address)((Address) &icmpPtr->data.param.ipHeader) +
					hdrLen);
	    }
	    break;


	/*
	 * The following types must be made available to the clients.
	 * Also give packets with unknown types to the clients in case
	 * they're using a new type we don't know about yet.
	 */
#ifdef	REAL_RAW
	case NET_ICMP_TIME_EXCEED:
	    if(ips_Debug)
		(void) fprintf(stderr,
		    "ICMP Input: <%x>  <---  <%x> %s\n", 
		    Net_NetToHostInt(ipPtr->dest), 
		    Net_NetToHostInt(ipPtr->source), 
		    typeNames[type]);
#endif	REAL_RAW
	case NET_ICMP_ECHO_REPLY:
	case NET_ICMP_TIMESTAMP_REPLY:
	case NET_ICMP_INFO_REPLY:
	case NET_ICMP_MASK_REPLY:
	default:
	    Raw_Input(NET_IP_PROTOCOL_ICMP, ipPtr->source, ipPtr->dest, 
			packetPtr);
    }
    free(packetPtr->base);
}


/*
 *----------------------------------------------------------------------
 *
 * ReturnToSender --
 *
 *	Handles ICMP echo, timestamp, info request and mask request packets.
 *	The packet is processed and then returned to the sender.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A packet is returned to the sender after a bit of processing.
 *
 *----------------------------------------------------------------------
 */

static void
ReturnToSender(netID, type, dataLen, icmpPtr, packetPtr)
    Rte_NetID  			netID;		/* ID of the network the 
						 * packet came arrived on. */
    unsigned char		type;		/* ICMP type. */
    int				dataLen;	/* Size in bytes of the data
						 * in the packet. */
    register Net_ICMPPacket	*icmpPtr;	/* Ptr to ICMP packet. */
    IPS_Packet	*packetPtr;			/* Packet descriptor. */
{
    register Net_IPHeader *ipHdrPtr = packetPtr->ipPtr;

    switch (type) {

	/*
	 * ECHO: just return the packet as is (but change the type to reply).
	 */
	case NET_ICMP_ECHO:
	    icmpPtr->header.type = NET_ICMP_ECHO_REPLY;
	    break;


	/*
	 * TIMESTAMP: set the times.
	 */
	case NET_ICMP_TIMESTAMP:
	    if (dataLen < sizeof(Net_ICMPHeader) + sizeof(Net_ICMPDataTime)) {
		stats.icmp.shortLen++;
		return;
	    }
	    icmpPtr->header.type = NET_ICMP_TIMESTAMP_REPLY;

	    /*
	     * The timestamp should record when we received the packet and
	     * when it was sent out.  Just set the transmit time
	     * to the receive time -- it'll probably be close enough to
	     * the real transmit time...
	     */
	    icmpPtr->data.timeStamp.recvTime = 
				Net_HostToNetInt(IPS_GetTimestamp());
	    icmpPtr->data.timeStamp.transmitTime = 
					icmpPtr->data.timeStamp.recvTime;
	    break;



	/*
	 * INFO: adjust the source address if it was a broadcast.
	 */
	case NET_ICMP_INFO_REQ:
	    icmpPtr->header.type = NET_ICMP_INFO_REPLY;
	    if (Net_InetAddrNetNum(ipHdrPtr->source) == 0) {
		/*
		 * Broadcast to this network: 
		 */
		ipHdrPtr->source = Net_MakeInetAddr(Rte_GetNetNum(netID), 
				Net_InetAddrHostNum(ipHdrPtr->source));
	    }
	    break;



	/*
	 * MASK: return the subnet mask for the network the packet came in on.
	 */
	case NET_ICMP_MASK_REQ:
	    if (dataLen < sizeof(Net_ICMPHeader) + sizeof(Net_ICMPDataMask)) {
		stats.icmp.shortLen++;
		return;
	    }
	    icmpPtr->header.type = NET_ICMP_MASK_REPLY;
	    icmpPtr->data.mask.addrMask = 
				Net_HostToNetInt(Rte_GetSubnetMask(netID));

	    /*
	     * RFC950 ("Internet Standard Subnetting Procedure") says on p.10 
	     * that if the source address is 0, then the destination address 
	     * for the reply should be a broadcast address. Since the source 
	     * and dest. will swapped below, set the source here.
	     */
	    if (ipHdrPtr->source == 0) {
		ipHdrPtr->source = Rte_GetBroadcastAddr(netID);
	    }
	    break;



	default:
	    panic("ReturnToSender: bad type %d\n", type);
	    return;
	    break;
    }


    /*
     * Swap the source and destination addresses since we're sending 
     * the packet back.
     */
    SwapAddresses(ipHdrPtr);

    /*
     * We have to recompute the checksum because of the changes made above.
     */
    icmpPtr->header.checksum = 0;
    icmpPtr->header.checksum = Net_InetChecksum(dataLen, (Address) icmpPtr);
    packetPtr->ipPtr->timeToLive = NET_IP_MAX_TTL;
    (void) IP_Output(packetPtr, FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * ICMP_SendErrorMsg --
 *
 *	Sends an ICMP packet to the source of a bad IP packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An ICMP error packet is sent to the offending host.
 *
 *----------------------------------------------------------------------
 */

void
ICMP_SendErrorMsg(ipHdrPtr, type, code)
    Net_IPHeader	*ipHdrPtr;	/* Ptr to IP header for the packet. */
    int			type;		/* ICMP type to return. */
    int			code;		/* ICMP code to return. */
{
    Net_ICMPPacket	*icmpPtr;
    IPS_Packet		packet;
    int			ipHdrLen;
    Net_InetAddress	dest;
    int			srcRteLen;
    Address		srcRoutePtr;
    int			ipDataLen;
    int			len;

    if (type > NET_ICMP_MAX_TYPE) {
	panic("ICMP_SendErrorMsg: bad type: %d\n", type);
	return;
    }

    /*
     * Only send an error if the packet is the first fragment.
     */
    if (ipHdrPtr->fragOffset != 0) {
	return;
    }

    ipHdrLen = ipHdrPtr->headerLen * 4;

    /*
     * Don't send if the packet's protocol was ICMP and the error type is
     * not a redirect and the packet's ICMP type was not an informational
     * type.
     */
    if ((ipHdrPtr->protocol == NET_IP_PROTOCOL_ICMP) &&
	(type != NET_ICMP_REDIRECT)) {

	switch (((Net_ICMPHeader *)(((Address) ipHdrPtr) + ipHdrLen))->type) {
	    case NET_ICMP_ECHO_REPLY:
	    case NET_ICMP_ECHO:
	    case NET_ICMP_TIMESTAMP:
	    case NET_ICMP_TIMESTAMP_REPLY:
	    case NET_ICMP_INFO_REQ:
	    case NET_ICMP_INFO_REPLY:
	    case NET_ICMP_MASK_REQ:
	    case NET_ICMP_MASK_REPLY:
		break;

	    default:
		return;
		break;
	}
    }


    /*
     * We need to format an ICMP packet that is composed of
     *	1) an IP header: we use the header + source route options from the 
     *     error packet, with the src and dest addresses swapped.
     *  2) the ICMP header: we use the type and code arguments to this routine.
     *	3) a type-dependent value (an integer).
     *  4) the IP header of the error packet: this includes the basic
     *	    header and options.
     *  5) the first 8 octets of data from the error packet.
     *
     * It is assumed that only error-type packets are sent, hence the
     * use of the overlay to include the IP header and data.
     */

    IPS_InitPacket(sizeof(Net_ICMPPacket), &packet);

    packet.data = packet.dbase + (packet.totalLen - sizeof(Net_ICMPPacket));
    icmpPtr     = (Net_ICMPPacket *) packet.data;
    
    ipDataLen	= ipHdrPtr->totalLen - ipHdrLen;
    len		= ipHdrLen + MIN(8, ipDataLen);
    packet.dataLen = sizeof(Net_ICMPHeader) + sizeof(int) + len;

    bcopy( (Address) ipHdrPtr, 
		  (Address) &icmpPtr->data.overlay.ipHeader, len);

    if (type == NET_ICMP_REDIRECT) {
	icmpPtr->data.redirect.gatewayAddr = 0;
    } else {
	icmpPtr->data.overlay.unused = 0;
    }
    if (type == NET_ICMP_PARAM_PROB) {
	icmpPtr->data.param.paramOffset = code;
	code = 0;
    }
    icmpPtr->header.type = type;
    icmpPtr->header.code = code;
    icmpPtr->header.checksum = 0;
    icmpPtr->header.checksum = Net_InetChecksum(packet.dataLen, 
					(Address) icmpPtr);

    /*
     * Now we format the IP header for the packet. Swap the src and dest
     * addresses since we're returning the packet to the sender.  Include
     * the source-route header options if they're present; other options
     * in the header are not returned.
     */

    SwapAddresses(ipHdrPtr);

    if (ipHdrLen > sizeof(Net_IPHeader)) {
	srcRoutePtr = IP_GetSrcRoute(&srcRteLen, &dest);
    }

    packet.ipLen = sizeof(Net_IPHeader) + srcRteLen;
    packet.ipPtr = (Net_IPHeader *) ((Address) packet.data) - packet.ipLen;
    *packet.ipPtr = *ipHdrPtr;
    if (srcRteLen > 0) {
	bcopy( srcRoutePtr, 
		((Address) (packet.ipPtr)) + sizeof(Net_IPHeader), srcRteLen);
	free(srcRoutePtr);
	packet.ipPtr->dest = dest;
    }

    packet.ipPtr->headerLen = packet.ipLen / 4;
    packet.ipPtr->totalLen = packet.ipLen + packet.dataLen;
    packet.ipPtr->timeToLive = NET_IP_MAX_TTL;
    packet.ipPtr->protocol = NET_IP_PROTOCOL_ICMP;
    (void) IP_Output(&packet, TRUE);
    free(packet.base);
}


/*
 *----------------------------------------------------------------------
 *
 * SwapAddresses --
 *
 *	Swap the source and destination addresses of an IP header. If the
 *	original destination/new source is a broadcast address, then
 *	change it to our official IP address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The addresses in an IP header are changed.
 *
 *----------------------------------------------------------------------
 */

static void
SwapAddresses(ipPtr)
    Net_IPHeader	*ipPtr;
{
    Net_InetAddress	temp;

    temp = ipPtr->dest;
    ipPtr->dest = ipPtr->source;

    if (Rte_IsBroadcastAddr(temp)) {
	ipPtr->source = Rte_GetOfficialAddr(FALSE);
    } else {
	ipPtr->source = temp;
    }
}
