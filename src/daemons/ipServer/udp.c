/* 
 * udp.c --
 *
 *	Routines to handle incoming and outgoing UDP packets
 *	as well as UDP-dependent  routines that implement the 
 *	semantics of the socket operations defined in sockOps.c.
 *
 *	The routines follow the User Datagram Protocol specification
 *	in RFC 768 (Aug. 1980).
 *
 *	Based on 4.3 BSD code:
 *	"@(#)udp_usrreq.c  7.5 (Berkeley) 3/11/88"
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/udp.c,v 1.16 91/05/21 17:01:35 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev/net.h"
#include "inet.h"
#include "ipServer.h"
#include "stat.h"
#include "ip.h"
#include "udp.h"
#include "socket.h"
#include "sockInt.h"
#include "route.h"

#ifndef KERNEL
#include <dev/pdev.h>
#endif

static void		Input();
static unsigned int 	GetPort();


/*
 *----------------------------------------------------------------------
 *
 * UDP_Init --
 *
 *	Causes a handler to be called whenever UDP packets arrive.
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
UDP_Init()
{
    IP_SetProtocolHandler(NET_IP_PROTOCOL_UDP, Input);
}

/*
 *----------------------------------------------------------------------
 *
 * Input --
 *
 *	This routine accepts IP datagrams and processes data for
 *	this protocol.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
Input(netID, packetPtr)
    Rte_NetID	netID;		/* ID of the network the packet arrived on. */
    IPS_Packet	*packetPtr;	/* Packet descriptor. */
{
    register Net_UDPHeader	*udpPtr;
    register Net_IPHeader	*ipPtr;
    Sock_SharedInfo 		*sockPtr;
    unsigned			udpLen;

    stats.udp.recv.total++;

    ipPtr  = packetPtr->ipPtr;
    udpPtr = (Net_UDPHeader *) ((Address) ipPtr + ipPtr->headerLen*4);

    udpLen = Net_NetToHostShort(udpPtr->len);
    if (udpLen < sizeof(Net_UDPHeader)) {
	stats.udp.recv.shortLen++;
	free(packetPtr->base);
	return;
    }
    packetPtr->data = ((Address)udpPtr) + sizeof(Net_UDPHeader);
    packetPtr->dataLen = udpLen - sizeof(Net_UDPHeader);

    /*
     * While debugging, drop packets from the rwho, sunrpc, and route daemons.
     */
    switch (Net_NetToHostShort(udpPtr->srcPort)) {
	case 111:	/* sunrpc */
	case 513: 	/* rhwod */
	case 520:	/* routed */
	    stats.udp.recv.daemon++;
	    if (ips_Debug) {
		free(packetPtr->base);
		return;
	    }
	    break;
    }

    /*
     * If the packet was sent with a checksum, the checksum will be
     * non-zero.
     */
    if (udpPtr->checksum != 0) {
	Net_IPPseudoHdr		pseudoHdr;
	unsigned short		sum;

	/*
	 * The checksum is computed for the IP "pseudo-header" and
	 * the UDP header and data. When the UDP checksum was calculated,
	 * the checksum field in the header was set to zero. When we 
	 * recalculate the value, we don't zero the field so the computed 
	 * value should be zero if the packet didn't get garbled.
	 */
	pseudoHdr.source	= (ipPtr->source);
	pseudoHdr.dest		= (ipPtr->dest);
	pseudoHdr.zero		= 0;
	pseudoHdr.protocol	= ipPtr->protocol;
	pseudoHdr.len		= udpPtr->len;

	if (udpLen & 1) {
	    ((Address)udpPtr)[udpLen] = 0;
	}
	sum = Net_InetChecksum2(udpLen, (Address)udpPtr, &pseudoHdr);

	if (sum != 0) {
	    if (ips_Debug) {
		(void) fprintf(stderr, "UDP: checksum %x != 0\n", sum);
	    }

	    stats.udp.recv.badChecksum++;
	    free(packetPtr->base);
	    return;
	}
    }
    stats.udp.recv.dataLen += udpLen;

    if (ips_Debug) {
	(void) fprintf(stderr, 
	    "UDP Input: %d bytes, <%x, %d> <-- <%x, %d>; checksum = %x\n",
		udpLen, 
		Net_NetToHostInt(ipPtr->dest), 
		Net_NetToHostShort(udpPtr->destPort), 
		Net_NetToHostInt(ipPtr->source), 
		Net_NetToHostShort(udpPtr->srcPort), 
		udpPtr->checksum);
	    
	    
    }

    sockPtr = Sock_Match(UDP_PROTO_INDEX, ipPtr->dest, 
		    udpPtr->destPort, ipPtr->source, udpPtr->srcPort, TRUE);


    if (sockPtr != (Sock_SharedInfo *) NULL) {
	stats.udp.recv.accepted++;
	stats.udp.recv.acceptLen += udpLen;
	UDP_SocketInput(sockPtr, ipPtr->source, udpPtr->srcPort, packetPtr);
    } else {
	free(packetPtr->base);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * UDP_Output --
 *
 *	This routine formats a UDP packet and causes it to be
 *	delivered using the IP protocol. The UDP header is formatted
 *	and a checksum is computed.
 *
 * Results:
 *	?	- result from IP_Output.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_Output(localAddr, remoteAddr, packetPtr)
    Net_InetSocketAddr	localAddr;	/* Local socket address. */
    Net_InetSocketAddr	remoteAddr;	/* Address of remote host. */
    register IPS_Packet *packetPtr;	/* Packet descriptor. */
{
    register Net_UDPHeader	*udpPtr;
    Net_IPPseudoHdr		pseudoHdr;
    short			len;

    /*
     * If a local address hasn't been assigned yet, use the official
     * address in the packet.
     */
    if (localAddr.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	localAddr.address = Rte_GetOfficialAddr(FALSE);
    }

    udpPtr = (Net_UDPHeader *) (packetPtr->data - sizeof(Net_UDPHeader));
    packetPtr->hdr.udpPtr = udpPtr;
    packetPtr->hdrLen = sizeof(Net_UDPHeader);

    /*
     * Compute the checksum for the header and pseudo-header.
     */
    len = packetPtr->dataLen + sizeof(Net_UDPHeader);
    pseudoHdr.source	= (localAddr.address);
    pseudoHdr.dest	= (remoteAddr.address);
    pseudoHdr.zero	= 0;
    pseudoHdr.protocol	= NET_IP_PROTOCOL_UDP;
    pseudoHdr.len	= Net_HostToNetShort(len);
    udpPtr->len		= pseudoHdr.len;
    udpPtr->srcPort	= (localAddr.port);
    udpPtr->destPort	= (remoteAddr.port);
    udpPtr->checksum	= 0;
    udpPtr->checksum	= Net_InetChecksum2((int) len, (Address)udpPtr, 
					&pseudoHdr);
    if (udpPtr->checksum == 0) {
	udpPtr->checksum = 0xFFFF;
    }

    IP_FormatPacket(NET_IP_PROTOCOL_UDP, NET_UDP_TTL, localAddr.address, 
			remoteAddr.address, packetPtr);

    if (ips_Debug) {
	(void) fprintf(stderr,
	"UDP_Output: %d bytes, <%x, %d> --> <%x, %d>\n",
	    Net_NetToHostShort(pseudoHdr.len), 
	    Net_NetToHostInt(localAddr.address), 
	    Net_NetToHostShort(localAddr.port),
	    Net_NetToHostInt(remoteAddr.address), 
	    Net_NetToHostShort(remoteAddr.port));
    }
    stats.udp.send.total++;
    stats.udp.send.dataLen += packetPtr->dataLen;

    (void) IP_Output(packetPtr, TRUE);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketOpen --
 *
 *	Does UDP-specific actions for opening a new socket. A port is 
 *	assigned in case the client never does bind or connect operations 
 *	on the socket.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	The socket is made ready for sending and receiving UDP packets.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
UDP_SocketOpen(sockPtr, userID)
    Sock_SharedInfo	*sockPtr;	/* Socket to be opened. */
    int			userID;		/* Client's user ID. Ignored. */
{
    sockPtr->local.port = GetPort(Net_HostToNetInt(NET_INET_ANY_ADDR));
    sockPtr->state = READY;
    Sock_BufAlloc(sockPtr, SOCK_RECV_BUF, UDP_MAX_DATAGRAM_SIZE);
    Sock_BufAlloc(sockPtr, SOCK_SEND_BUF, UDP_MAX_DATAGRAM_SIZE);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketClose --
 *
 *	Removes data from the socket's read and write queues.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	Deallocates buffers in the read and write queues.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_SocketClose(sockPtr)
    register Sock_SharedInfo	*sockPtr;	/* Socket to be closed. */
{
    Sock_BufRemove(sockPtr, SOCK_RECV_BUF, -1);
    Sock_BufRemove(sockPtr, SOCK_SEND_BUF, -1);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_ReadRequest --
 *
 *	Handle a read request from a client accessing the UDP pseudo-device.
 *	This is an optimized handling of the request-response protocol and
 *	the buffering structures associated with UDP.  Basically there is
 *	just a list of packets associated with the socket, and we point
 *	the kernel at the first one and then remove it.
 *
 *	This should be cleaned up so it isn't quite so pseudo-device
 *	dependent - especially if UDP is moved into the kernel.
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the first packet from the list.
 *
 *----------------------------------------------------------------------
 */
#ifndef KERNEL
void
UDP_ReadRequest(privPtr, requestPtr, streamID)
    Sock_PrivInfo *privPtr;	/* Socket state */
    Pdev_Request *requestPtr;	/* Pseudo-device request */
    int streamID;		/* Request-response stream */
{
    Pdev_Reply	reply;
    Sock_SharedInfo	*sharePtr = privPtr->sharePtr;
    Sock_BufInfo	*bufInfoPtr;
    Sock_BufDataInfo   *dataPtr = (Sock_BufDataInfo *)NULL;

    reply.replySize = 0;
    reply.status = SUCCESS;
    /*
     * Error checking.
     */
    if (requestPtr->hdr.replySize < 0) {
	reply.status = GEN_INVALID_ARG;
	reply.replyBuf = (Address) NULL;
	goto readReply;
    }
    if (sharePtr->error != SUCCESS) {
	reply.status = sharePtr->error;
	sharePtr->error = SUCCESS;
	goto readReply;
    }
    if ((sharePtr->state != READY) &&
	(sharePtr->state != CONNECTED)) {
	reply.status = NET_BAD_OPERATION;
	goto readReply;
    }
    if (privPtr->recvFlags & NET_OUT_OF_BAND) {
	reply.status = NET_BAD_OPERATION;
	goto readReply;
    }
    /*
     * Take the first packet from the socket buffer.
     */
    bufInfoPtr = &sharePtr->recvBuf;
    if (List_IsEmpty(&bufInfoPtr->links)) {
	if (Sock_IsRecvStopped(sharePtr)) {
	    reply.status = SUCCESS;
	} else {
	    reply.status = FS_WOULD_BLOCK;
	}
	goto readReply;
    }
    dataPtr = (Sock_BufDataInfo *)List_First(&bufInfoPtr->links);
    /*
     * Set up the reply to reference the packet directly.
     */
    reply.replyBuf = dataPtr->bufPtr;
    if (requestPtr->hdr.replySize > dataPtr->len) {
	reply.replySize = dataPtr->len;
    } else {
	reply.replySize = requestPtr->hdr.replySize;
    }
    /*
     * Save the address for recvfrom() emulation
     */
    privPtr->recvFrom = dataPtr->address;
readReply:
    /*
     * Reply to the client.  The kernel will copy the data from
     * the packet to the clients buffer as part of the reply.
     */
    reply.selectBits = Sock_Select(privPtr, TRUE); 
    if (ips_Debug) {
	fprintf(stderr, "UDP_ReadRequest: %d bytes, select 0x%x, status 0x%x\n",
		reply.replySize, reply.selectBits, reply.status);
    }
    PdevReply(streamID, &reply, "read");
    if (dataPtr != (Sock_BufDataInfo *) NULL) {
	/*
	 * Nuke the packet.
	 */
	if ((privPtr->recvFlags & SOCK_BUF_PEEK) == 0) {
	    bufInfoPtr->size -= dataPtr->len;
	    List_Remove((List_Links *) dataPtr);
	    free(dataPtr->base);
	    free(dataPtr);
	} else {
	    privPtr->recvFlags &= ~SOCK_BUF_PEEK;
	}
    }
}
#endif /* not KERNEL */


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketRead --
 *
 *	THIS HAS BEEN REPLACED BY UDP_ReadRequest
 *
 *	Supplies data to a client doing a read on a UDP socket.
 *	Normally the packet data is  copied to the buffer. If the
 *	packet is smaller than the buffer, no additional packets 
 *	are read so the client will get a short read. If the buffer
 *	is too small, the remaining data in the packet is not saved 
 *	for the next read. Normally, the packet is removed from the
 *	queue and the memory is deallocated once it is read. This
 *	is not the case when the flag NET_PEEK is given.
 *
 * Results:
 *	SUCCESS			- the data were copied to the buffer.
 *	FS_WOULD_BLOCK		- nothing to read.
 *	NET_BAD_OPERATION	- tried to read out-of-band data (not
 *				  defined for this protocol).
 *			  
 * Side effects:
 *	The read queue is shortened.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_SocketRead(privPtr, bufSize, buffer, amountReadPtr)
    Sock_PrivInfo	*privPtr;	/* Ptr to socket's per-process info. */
    int		bufSize;		/* # of bytes in buffer. */
    Address	buffer;			/* Place to put the data. */
    int		*amountReadPtr;		/* # of bytes actually put in buffer. */
{
    ReturnStatus status;
    Sock_SharedInfo	*sharePtr = privPtr->sharePtr;

    if ((sharePtr->state != READY) && (sharePtr->state != CONNECTED)) {
	return(NET_BAD_OPERATION);
    }

    if (privPtr->recvFlags & NET_OUT_OF_BAND) {
	return(NET_BAD_OPERATION);
    }

    status = Sock_BufFetch(sharePtr, privPtr->recvFlags, bufSize, buffer, 
		amountReadPtr, &privPtr->recvFrom);

    /*
     * Clear the recvFlags because they are only good for 1 read operation.
     */
    if (status == SUCCESS) {
	privPtr->recvFlags = 0;
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketInput --
 *
 *	This routine is called when a UDP packet arrives from the network.
 *	The packet is appended to the socket's read queue and the client
 *	is notified that there's data to be read.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New data is added to the read queue. The waiting client is
 *	woken up. The packet is freed if the buffer is full.
 *
 *----------------------------------------------------------------------
 */

void
UDP_SocketInput(sockPtr, srcAddr, srcPort, packetPtr)
    Sock_SharedInfo	*sockPtr;	/* Socket that should get the data. */
    Net_InetAddress	srcAddr;	/* IP address of sender. */
    unsigned int	srcPort;	/* UDP port of sender. */
    IPS_Packet		*packetPtr;	/* Packet descriptor. */
{
    Net_InetSocketAddr	sockAddr;
    int			amtWritten;

    sockAddr.address = srcAddr;
    sockAddr.port = srcPort;
    if ((Sock_BufAppend(sockPtr, SOCK_RECV_BUF, TRUE, packetPtr->dataLen, 
		packetPtr->data, packetPtr->base, &sockAddr, &amtWritten) 
	== SUCCESS) && (amtWritten == packetPtr->dataLen)) {
	Sock_NotifyWaiter(sockPtr, FS_READABLE);
    } else {
	free(packetPtr->base);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_WriteRequest --
 *
 *	Handle a request to write a UDP datagram to the network.
 *	(This replaces UDP_SocketWrite).
 *
 * Results:
 *	SUCCESS			- the data were sent.
 *	NET_BAD_OPERATION	- the send flags were not 0 (the flag
 *				  values aren't valid for this operation).
 *	?			- status from UDP_Output.
 *
 * Side effects:
 *	The data will be sent in a packet.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_WriteRequest(privPtr, procID, bufSize, buffer, amtWrittenPtr)
    register Sock_PrivInfo	*privPtr;
    Proc_PID procID;
    int bufSize;
    char *buffer;
    int *amtWrittenPtr;
{
    register Sock_SharedInfo *sharePtr = privPtr->sharePtr;
    register int headerLen;
    IPS_Packet packet;

    *amtWrittenPtr = 0;
    if (bufSize > UDP_MAX_DATAGRAM_SIZE) {
	return(FS_BUFFER_TOO_BIG);
    }
    if ((sharePtr->state != READY) && (sharePtr->state != CONNECTED)) {
	return(NET_BAD_OPERATION);
    }
    /*
     * Do sendto() emulation.  IOC_NET_SEND_INFO may have been used
     * to set the address to which to send this packet.  sendto cannot
     * be used if the socket is connected.
     */
    if (privPtr->sendInfoValid) {
	/*
	 * The send info flags are not valid for UDP sockets.
	 */
	if (privPtr->sendInfo.flags != 0) {
	    return(NET_BAD_OPERATION);
	}
	privPtr->sendInfoValid = FALSE;
	if (privPtr->sendInfo.addressValid) {
	    if (sharePtr->state == CONNECTED) {
		return(NET_ALREADY_CONNECTED);
	    }
	    sharePtr->sentTo = privPtr->sendInfo.address.inet;
    	    if (sharePtr->sentTo.address == 
		    Net_HostToNetInt(NET_INET_BROADCAST_ADDR) &&
		!(sharePtr->options & NET_OPT_BROADCAST)) {
		return(NET_BAD_OPERATION);
	    }
	} else {
	    /*
	     * If the socket's not connected, we don't have a valid 
	     * destination address.
	     */
	    if (sharePtr->state != CONNECTED) {
		return(NET_NOT_CONNECTED);
	    }
	    sharePtr->sentTo = sharePtr->remote;
	}

    } else {
	if (sharePtr->state != CONNECTED) {
	    return(NET_NOT_CONNECTED);
	}
	sharePtr->sentTo = sharePtr->remote;
    }
    /*
     * Initialize packet referencing data in place.
     */
    headerLen = sizeof(Net_UDPHeader) + NET_IP_MAX_HDR_SIZE +
			sizeof(IPS_PacketNetHdr);
    packet.dataLen = bufSize;
    packet.totalLen = bufSize + headerLen;
    packet.data = buffer;
    packet.base = packet.dbase = buffer - headerLen;

    *amtWrittenPtr = bufSize;
    return(UDP_Output(sharePtr->local, sharePtr->sentTo, &packet));
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketWrite --
 *
 *	Takes data from a write call by a client and causes the data
 *	to be sent out on the network.
 *
 * Results:
 *	SUCCESS			- the data were sent.
 *	NET_BAD_OPERATION	- the send flags were not 0 (the flag
 *				  values aren't valid for this operation).
 *	?			- status from UDP_Output.
 *
 * Side effects:
 *	The data will be sent in a packet.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_SocketWrite(privPtr, packetPtr, amtWrittenPtr)
    Sock_PrivInfo	*privPtr;
    IPS_Packet	*packetPtr;		/* Packet containing data to be
					 * sent on the network. */
    int		*amtWrittenPtr;		/* # of bytes sent to the remote host.*/
{
    ReturnStatus	status;
    Sock_SharedInfo	*sharePtr = privPtr->sharePtr;

    *amtWrittenPtr = 0;

    if (packetPtr->dataLen > 
	     Sock_BufSize(sharePtr, SOCK_SEND_BUF, SOCK_BUF_MAX_SIZE)) {
	return(FS_BUFFER_TOO_BIG);
    }

    if ((sharePtr->state != READY) && (sharePtr->state != CONNECTED)) {
	return(NET_BAD_OPERATION);
    }


    if (privPtr->sendInfoValid) {
	/*
	 * The send info flags are not valid for UDP sockets.
	 */
	if (privPtr->sendInfo.flags != 0) {
	    return(NET_BAD_OPERATION);
	}

	/*
	 * See if the client has given us an explicit address about
	 * where to send the packet. 
	 */

	privPtr->sendInfoValid = FALSE;
	if (privPtr->sendInfo.addressValid) {
	    /*
	     * If the socket's connected, the user can't specify a different
	     * destination.
	     */
	    if (sharePtr->state == CONNECTED) {
		return(NET_ALREADY_CONNECTED);
	    }
	    sharePtr->sentTo = privPtr->sendInfo.address.inet;

	} else {
	    /*
	     * If the socket's not connected, we don't have a valid 
	     * destination address.
	     */
	    if (sharePtr->state != CONNECTED) {
		return(NET_NOT_CONNECTED);
	    }
	    sharePtr->sentTo = sharePtr->remote;
	}

    } else {
	if (sharePtr->state != CONNECTED) {
	    return(NET_NOT_CONNECTED);
	}
	sharePtr->sentTo = sharePtr->remote;
    }

    status = UDP_Output(sharePtr->local, sharePtr->sentTo, packetPtr);
    free(packetPtr->base);

    if (status == SUCCESS) {
	*amtWrittenPtr = packetPtr->dataLen;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketSelect --
 *
 *	Handles UDP-specific checks to see if a socket is ready for
 *	reading. The socket is always writable.
 *
 * Results:
 *	An or'd combination of FS_READBLE and FS_WRITABLE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
UDP_SocketSelect(sockPtr)
    Sock_SharedInfo *sockPtr;	/* Socket to check the readiness of. */
{
    int flags = FS_WRITABLE;

    /*
     * The socket is readable only if there's data in the read queue.
     */
    if (Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_USED) > 0) {
	flags |= FS_READABLE;
    }
    return(flags);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketBind --
 *
 *	Does UDP-specific stuff for assigning the local address of a socket.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_ADDRESS_NOT_AVAIL	- the caller gave an address that doesn't
 *				  correspond to this host.
 *	NET_ADDRESS_IN_USE	- the given address is already in use.
 *	GEN_NO_PERMISSION	- tried to bind a port that is reserved
 *				  to the super-user.
 *
 * Side effects:
 *	The local socket address for a socket is assigned.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
UDP_SocketBind(sockPtr, addrPtr, userID)
    register Sock_SharedInfo *sockPtr;	/* Socket to be bound. */
    Net_InetSocketAddr	*addrPtr;	/* Local address to assign to the 
					 * socket. */
    int		userID;			/* User ID of the process. */
{
    Net_InetSocketAddr	temp;
    temp = *addrPtr;

    /*
     * If a specific Inetnet address is given, make sure it is valid for
     * this host (i.e. it is one of the addresses assigned to the host).
     */
    if (temp.address != Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	if (!Rte_ValidateAddress(temp.address)) {
	    return(NET_ADDRESS_NOT_AVAIL);
	}
    }

    if (temp.port == 0) {
	temp.port = GetPort(temp.address);
    } else {
	Boolean wildcard;

	/*
	 * Make sure the given port is OK to use. If the port # is in
	 * the reserved range then make sure the user has super-user
	 * privileges. 
	 */

	if ((Net_NetToHostShort(temp.port) < INET_PRIV_PORTS) && 
	    (userID != PROC_SUPER_USER_ID)) {
	    return(GEN_NO_PERMISSION);
	}

	/*
	 * Check for the uniqueness of the new local <address,port> tuple
	 * among all UDP sockets.  The REUSE_ADDR option specifies the
	 * strictness of the check.  If the option is not set, then the
	 * local tuple can't be used by us if any other UDP socket is
	 * using it.  A looser check is made if the option is set:  we can
	 * use the local tuple even if it is used by other sockets but
	 * only if their remote tuple is not the <NET_INET_ANY_ADDR,0>
	 * wildcard.
	 */

	if (Sock_IsOptionSet(sockPtr, NET_OPT_REUSE_ADDR)) {
	    wildcard = FALSE;
	} else {
	    wildcard = TRUE;
	}
	if (Sock_Match(UDP_PROTO_INDEX, temp.address, temp.port,
		    Net_HostToNetInt(NET_INET_ANY_ADDR), 0, wildcard) !=
				(Sock_SharedInfo *) NULL) {

	    return(NET_ADDRESS_IN_USE);
	}
    }

    sockPtr->local = temp;
    sockPtr->state = READY;
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketConnect --
 *
 *	Assigns a remote host <address,port> pair to a socket. 
 *	From now on, only packets from this remote host will be
 *	accepted for this socket.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_ADDRESS_NOT_AVAIL	- an invalid port was specified.
 *	NET_ALREADY_CONNECTED	- the socket is already connected.
 *	NET_ADDRESS_IN_USE	- the <local,remote> pair is already
 *				   in use by another socket.
 *	
 *
 * Side effects:
 *	The remote socket address for a socket is assigned.
 *	The local socket address may also be assigned.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
UDP_SocketConnect(sockPtr, remoteAddrPtr, initialize)
    register Sock_SharedInfo	*sockPtr;	/* Socket to be connected. */
    Net_InetSocketAddr	*remoteAddrPtr;	/* Remote address to send and receive
					 * datagrams from. */
    Boolean		initialize;	/* Ignored. */
{
    Net_InetSocketAddr	remote;
    remote = *remoteAddrPtr;


    /*
     * If the socket's already connected, disconnect it by zeroing 
     * the remote <address,port> and removing all data in the recv queue.
     */

    if (sockPtr->state == CONNECTED) {
	sockPtr->remote.address	= Net_HostToNetInt(NET_INET_ANY_ADDR);
	sockPtr->remote.port	= 0;
	sockPtr->state		= READY;
	Sock_BufRemove(sockPtr, SOCK_RECV_BUF, -1);
    }


    /*
     * A valid remote port must be specified.
     */
    if (remote.port == 0) {
	return(NET_ADDRESS_NOT_AVAIL);
    }

    /*
     * If the wildcard address or the broadcast wildcard are given, then
     * use the official address (explicit or broadcast) for the server.
     */

    if (ips_Debug) {
	fprintf(stderr, "Socket_Connect: remote_addr before = %x\n",
			remote.address);
    }
    if (remote.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	remote.address = Rte_GetOfficialAddr(FALSE);
    } else if (remote.address == Net_HostToNetInt(NET_INET_BROADCAST_ADDR)) {
	remote.address = Rte_GetOfficialAddr(TRUE);
    }
    if (ips_Debug) {
	fprintf(stderr, "Socket_Connect: remote_addr after = %x\n",
			remote.address);
    }

    if (sockPtr->local.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	Net_InetAddress	localAddr;

	/*
	 * The socket doesn't have a local address part yet so 
	 * use the official address for this server. Make sure the
	 * <local <address,port> and remote <address,port> tuple is
	 * unique among all open UDP sockets.
	 */

	localAddr = Rte_GetOfficialAddr(FALSE);
	if (Sock_Match(UDP_PROTO_INDEX, localAddr, 
		sockPtr->local.port, remote.address, remote.port, FALSE) !=
		(Sock_SharedInfo *) NULL) {
	    return(NET_ADDRESS_IN_USE);
	}

	/*
	 * The socket has a unique address tuple. 
	 * If a local port has not been chosen yet, assign an unused value.
	 */

	sockPtr->local.address = localAddr;
	if (sockPtr->local.port == 0) {
	    sockPtr->local.port = GetPort(localAddr);
	}
    } else {
	/*
	 * The socket has a valid local <address, port>. Make sure the
	 * the <local, remote> tuple is unique among all open UDP sockets.
	 */

	if (Sock_Match(UDP_PROTO_INDEX, sockPtr->local.address, 
		sockPtr->local.port, remote.address, remote.port, FALSE) !=
		(Sock_SharedInfo *) NULL) {
	    return(NET_ADDRESS_IN_USE);
	}
    }
    sockPtr->remote	= remote;
    sockPtr->state	= CONNECTED;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * UDP_SocketShutdown --
 *
 *	Called when the client will not send any more data on the socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket is prevented from sending data.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
UDP_SocketShutdown(sockPtr, how)
    Sock_SharedInfo 	*sockPtr;	/* Socket to be shutdown. */
    int		how;		/* Type of shutdown wanted. */
{
    Sock_StopSending(sockPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * GetPort --
 *
 *	Assign a port for a given address and make sure it doesn't 
 *	doesn't conflict with an existing socket's binding address.
 *
 * Results:
 *	A new port #.
 *
 * Side effects:
 *	The port value is incremented.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
GetPort(localAddr)
    Net_InetAddress	localAddr;	/* Address to check for uniqueness. */
{
    static unsigned int port = INET_PRIV_PORTS;

    do {
	port++;
	if ((port < INET_PRIV_PORTS) || (port > INET_SERVER_PORTS)) {
	    port = INET_PRIV_PORTS;
	}
    } while (Sock_Match(UDP_PROTO_INDEX, localAddr, Net_HostToNetShort(port), 
		    Net_HostToNetInt(NET_INET_ANY_ADDR), 0, FALSE) !=
				(Sock_SharedInfo *) NULL);
    return(Net_HostToNetShort(port));
}
