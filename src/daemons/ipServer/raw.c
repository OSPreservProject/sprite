/* 
 * raw.c --
 *
 *	Protocol-dependent routines to implement the semantics of
 *	the socket operations defined in inetSocket.c. The routines in
 *	this file handle low-level IP-based protocols for "raw" sockets.
 *	A raw socket is used to send and receive non-UDP and non-TCP packets.
 *	For example, it can be used to send ICMP echo packets.
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/raw.c,v 1.5 90/07/09 11:03:56 tve Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "inet.h"
#include "ipServer.h"
#include "stat.h"
#include "raw.h"
#include "socket.h"
#include "sockInt.h"
#include "ip.h"
#include "route.h"

/*
 * Maximum amount of data that can be stored in a raw socket's send
 * and receive buffers. 
 */

#define RECV_BUF_SIZE	2048
#define SEND_BUF_SIZE	2048

static void Output();



/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketOpen --
 *
 *	Does IP-specific actions for opening a new socket. The client
 *	must have the super-user's user ID to open the socket.
 *
 * Results:
 *	SUCCESS			- the socket was initialized.
 *	GEN_NO_PERMISSION	- the client did not have the proper user ID.
 *
 * Side effects:
 *	The max sizes for the send and receive buffers are set.
 *
 *----------------------------------------------------------------------
 */

Raw_SocketOpen(sockPtr, userID)
    Sock_SharedInfo	*sockPtr;	/* Socket to be opened. */
    int			userID;		/* Client's user ID. */
{
    if (userID != PROC_SUPER_USER_ID) {
	return(GEN_NO_PERMISSION);
    }

    sockPtr->state = READY;
    sockPtr->protocol = NET_IP_PROTOCOL_IP;
    Sock_BufAlloc(sockPtr, SOCK_RECV_BUF, RECV_BUF_SIZE);
    Sock_BufAlloc(sockPtr, SOCK_SEND_BUF, SEND_BUF_SIZE);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketClose --
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
Raw_SocketClose(sockPtr)
    Sock_SharedInfo	*sockPtr;	/* Socket to be closed. */
{
    Sock_BufRemove(sockPtr, SOCK_RECV_BUF, -1);
    Sock_BufRemove(sockPtr, SOCK_SEND_BUF, -1);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketRead --
 *
 *	Supplies data to a client doing a read on a raw socket.
 *	Normally the packet data is copied to the buffer. If the
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
 *				  defined for this socket type).
 *			  
 * Side effects:
 *	The read queue is shortened.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Raw_SocketRead(privPtr, bufSize, buffer, amountReadPtr)
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
 * Raw_SocketWrite --
 *
 *	Takes data from a write call by a client and causes the data
 *	to be sent out on the network.
 *
 * Results:
 *	SUCCESS			- the data were sent.
 *	NET_BAD_OPERATION	- the send flags were not 0 (the flag
 *				  values aren't valid for this operation).
 *	FS_BUFFER_TOO_BIG	- the amount of data to send was larger than 
 *				  the amount allowed.
 *
 * Side effects:
 *	The data will be sent in a packet.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Raw_SocketWrite(privPtr, packetPtr, amtWrittenPtr)
    Sock_PrivInfo	*privPtr;
    IPS_Packet	*packetPtr;		/* Packet containing data to be
					 * sent on the network. */
    int		*amtWrittenPtr;		/* # of bytes sent to the remote host.*/
{
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
	 * The send info flags are not valid for raw sockets.
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

    *amtWrittenPtr = packetPtr->dataLen;

    Output(sharePtr, sharePtr->local.address, sharePtr->sentTo.address, 
		packetPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketSelect --
 *
 *	Handles raw socket-specific checks to see if a socket is ready for
 *	reading.  The socket is always writable because buffering is not done.
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
Raw_SocketSelect(sockPtr)
    Sock_SharedInfo *sockPtr;	/* Socket to check the readiness of. */
{
    int flags = FS_WRITABLE;

    /*
     * The socket is readble only if there's data in the read queue.
     */
    if (Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_USED) > 0) {
	flags |= FS_READABLE;
    }
    return(flags);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketBind --
 *
 *	Assigns the local IP address for the socket. This address is used
 *	to reject incoming packets with an unwanted destination address.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_ADDRESS_NOT_AVAIL	- the caller gave an address that doesn't
 *				  correspond to this host.
 * Side effects:
 *	The local IP address for a socket is assigned.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Raw_SocketBind(sockPtr, addrPtr, userID)
    register Sock_SharedInfo *sockPtr;	/* Socket to be bound. */
    Net_InetSocketAddr	*addrPtr;	/* Local address to assign to the 
					 * socket. */
    int		userID;			/* User ID of the client. Ignored. */
{
    /*
     * If a specific IP address is given, make sure it is valid for
     * this host (i.e., it is one of the addresses assigned to the host).
     *
     * Only set the SOCK_RAW_HAVE_LOCAL_ADDR flag if a non-wildcard address
     * is given. The port value in addrPtr is ignored.
     */
    if (addrPtr->address != Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	if (!Rte_ValidateAddress(addrPtr->address)) {
	    return(NET_ADDRESS_NOT_AVAIL);
	}
	sockPtr->flags |= SOCK_RAW_HAVE_LOCAL_ADDR;
    }
    sockPtr->local = *addrPtr;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketConnect --
 *
 *	Assigns a remote host address to a socket. From now on, only 
 *	packets from this remote host will be accepted for this socket.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	The remote IP address for a socket is assigned.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Raw_SocketConnect(sockPtr, remoteAddrPtr, initialize)
    register Sock_SharedInfo	*sockPtr;	/* Socket to be connected. */
    Net_InetSocketAddr	*remoteAddrPtr;	/* Remote address to send and receive
					 * datagrams from. */
    Boolean		initialize;	/* Ignored. */
{
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

    sockPtr->remote = *remoteAddrPtr;
    sockPtr->state  = CONNECTED;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_SocketShutdown --
 *
 *	Called by the client when it will not write any more data to 
 *	the socket. Attempts to write data to the socket will fail.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	The socket is prevented from sending data.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Raw_SocketShutdown(sockPtr, how)
    Sock_SharedInfo 	*sockPtr;	/* Socket to be shutdown. */
    int			how;		/* Type of shutdown wanted. */
{
    Sock_StopSending(sockPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Raw_Input --
 *
 *	This routine is called when an IP packet arrives from the network
 *	with a protocol that does not have a handler. The list of open
 *	raw sockets is examined to see if any client wants the packet.
 *	If the client wants the packet, a copy is made and appended to the 
 *	socket's read queue and the client is notified that there's data 
 *	to be read.
 *
 *	The caller of this routine must free the packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New data is added to the read queue. The waiting client is
 *	woken up.
 *
 *----------------------------------------------------------------------
 */

void
Raw_Input(protocol, srcAddr, destAddr, packetPtr)
    int 		protocol;	/* Protocol # in IP header. */
    Net_InetAddress	srcAddr;	/* IP address of sender. */
    Net_InetAddress	destAddr;	/* IP address of destination. */
    IPS_Packet		*packetPtr;	/* Packet descriptor. */
{
    register Sock_SharedInfo	*sockPtr;
    Net_InetSocketAddr		sockAddr;
    Address			data;

    sockAddr.address = srcAddr;
    sockAddr.port = 0;

    /*
     * Go through all open raw sockets and see if anyone wants the packet.
     */
    sockPtr = (Sock_SharedInfo *) NULL;
    while (TRUE) {
	sockPtr = Sock_ScanList(RAW_PROTO_INDEX, sockPtr);
	if (sockPtr == (Sock_SharedInfo *) NULL) {
	    break;
	}

	/*
	 * If the socket wants a specific protocol, make sure the packet's
	 * protocol # matches. If the socket's protocol is NET_IP_PROTOCOL_IP,
	 * then it will get packets with NET_IP_PROTOCOL_IP and any other
	 * protocol.
	 */

	if ((sockPtr->protocol != NET_IP_PROTOCOL_IP) && 
	    (sockPtr->protocol != protocol)) {
	    continue;
	}

	if ((sockPtr->flags & SOCK_RAW_HAVE_LOCAL_ADDR) &&
	    (destAddr != sockPtr->local.address)) {
	    continue;
	}
	if ((sockPtr->state == CONNECTED) &&
	    (srcAddr != sockPtr->remote.address)) {
	    continue;
	}

	/*
	 * Make sure there's enough room in the buffer before we
	 * make a copy of the packet. It's a hack but saves a copy
	 * if the buffer can't take the packet.
	 */
#ifndef	REAL_RAW
	if (packetPtr->dataLen > Sock_BufSize(sockPtr, SOCK_RECV_BUF, 
			SOCK_BUF_FREE)) {

	    stats.sock.buffer.append++;
	    stats.sock.buffer.appendFail++;
	    continue;
	}
	data = malloc((unsigned int) packetPtr->dataLen);
	bcopy(packetPtr->data, data, packetPtr->dataLen);

	(void) Sock_BufAppend(sockPtr, SOCK_RECV_BUF, TRUE, packetPtr->dataLen, 
		    data, data, &sockAddr, (int *)NULL);
#else	REAL_RAW
	if (packetPtr->dataLen+sizeof(Net_IPHeader) > Sock_BufSize(sockPtr,
		SOCK_RECV_BUF, SOCK_BUF_FREE)) {

	    stats.sock.buffer.append++;
	    stats.sock.buffer.appendFail++;
	    continue;
	}
	data = malloc((unsigned int) packetPtr->dataLen+sizeof(Net_IPHeader));
	bcopy(packetPtr->ipPtr, data, sizeof(Net_IPHeader));
	bcopy(packetPtr->data, data+sizeof(Net_IPHeader), packetPtr->dataLen);

	(void) Sock_BufAppend(sockPtr, SOCK_RECV_BUF, TRUE,
		    packetPtr->dataLen+sizeof(Net_IPHeader),
		    data, data, &sockAddr, (int *)NULL);
#endif	REAL_RAW
	Sock_NotifyWaiter(sockPtr, FS_READABLE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Output --
 *
 *	This routine formats an IP packet and causes it to be queued for
 *	output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An IP packet is queued for output.
 *
 *----------------------------------------------------------------------
 */

static void
Output(sockPtr, local, remote, packetPtr)
    register Sock_SharedInfo *sockPtr;	/* Socket info for the sender. */
    Net_InetAddress	local;		/* Source address for the packet. */
    Net_InetAddress	remote;		/* Destination of the packet. */
    register IPS_Packet *packetPtr;	/* Packet descriptor. */
{
    int	timeToLive = NET_IP_MAX_TTL;
    int protocol = sockPtr->protocol;

    if (!(sockPtr->flags & SOCK_RAW_HAVE_LOCAL_ADDR) ||
	(local == Net_HostToNetInt(NET_INET_ANY_ADDR))) {
	local = Rte_GetOfficialAddr(FALSE);
    }

#ifdef	REAL_RAW
    /* If REAL_RAW is defined, we expect the user to give us a complete
     * packet, INCLUDING the (partially filled-in) ip header. For the moment,
     * all we care about in the ip header is the time-to-live and protocol
     * fields. This hack is to support the traceroute program.
     */
    timeToLive = ((Net_IPHeader *)packetPtr->data)->timeToLive;
    protocol = ((Net_IPHeader *)packetPtr->data)->protocol;
    (Address)packetPtr->data += sizeof(Net_IPHeader);
    packetPtr->dataLen -= sizeof(Net_IPHeader);
#endif	REAL_RAW

    packetPtr->hdrLen = 0;
    IP_FormatPacket(protocol, timeToLive, local, remote,
			packetPtr);

    if (ips_Debug) {
	(void) fprintf(stderr, "(Raw)Output: %d bytes, <%x> --> <%x>\n",
	    packetPtr->dataLen, local, remote);
    }
    stats.raw.send.total++;
    stats.raw.send.dataLen += packetPtr->dataLen;

    IP_QueueOutput(packetPtr);
}
