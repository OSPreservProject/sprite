/* 
 * tcpSock.c --
 *
 *	Protocol-dependent routines to implement the semantics of
 *	the socket operations defined in sockOps.c. The routines in
 *	this file handle TCP stream sockets.
 *
 *	Based on the following 4.3BSD files:
 *	"@(#)tcp_subr.c	  7.13 (Berkeley) 12/7/87"
 *	"@(#)tcp_usrreq.c 7.8 (Berkeley) 3/16/88"
 *
 * 	To do:
 *	1) fix bug in socket closing: sockPtr is not deallocated.
 *
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/tcpSock.c,v 1.12 91/11/12 20:56:31 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "inet.h"
#include "netInet.h"
#include "ipServer.h"
#include "stat.h"
#include "socket.h"
#include "sockInt.h"
#include "tcp.h"
#include "tcpInt.h"
#include "route.h"

/*
 * Default size (in bytes) of a socket's send and receive buffers.
 */
#define SEND_SPACE	TCP_BUF_SIZE
#define RECV_SPACE	TCP_BUF_SIZE


static unsigned int	GetPort();
static Boolean		ConnQueueFull();
static Boolean		ConnQueueNotEmpty();
static void		ConnQueueAppend();
static Sock_SharedInfo	*ConnQueueNext();
TCPControlBlock *curTCB;



/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketOpen --
 *
 *	This routine completes the TCP-dependent initialization of the
 *	socket. A TCP control block is allocated and initialized. 
 *	The send and receive buffers are also initialized.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	Memory for the TCP control block is allocated.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
TCP_SocketOpen(sockPtr, userID)
    Sock_SharedInfo	*sockPtr;	/* Socket to be initialized. */
    int			userID;		/* Client's user ID. Ignored. */
{
    register TCPControlBlock *tcbPtr;

    tcbPtr = (TCPControlBlock *) malloc(sizeof(TCPControlBlock));
    sockPtr->protoData = (ClientData) tcbPtr;

    bzero( (Address) tcbPtr, sizeof(*tcbPtr));
    tcbPtr->state		= CLOSED;
    tcbPtr->maxSegSize		= TCP_MAX_SEG_SIZE;
    tcbPtr->flags		= 0;
    /*
     * Initialize the smoothed RTT to 0 so we can tell that we have no
     * RTT estimate.  Set rttvar so that smooth RTT + 2 * rttVar gives
     * reasonable initial retransmit time.
     */
    tcbPtr->srtt		= TCP_SRTT_BASE_TIME;
    tcbPtr->rttvar		= TCP_SRTT_DEFLT_TIME << 2;
    TCP_TIMER_RANGESET(tcbPtr->rxtcur, 
	((TCP_SRTT_BASE_TIME >> 2) + (TCP_SRTT_DEFLT_TIME << 2)) >> 1,
	TCP_MIN_REXMT_TIME, TCP_MAX_REXMT_TIME);
    tcbPtr->send.congWindow	= SEND_SPACE;
    tcbPtr->send.cwSizeThresh	= 65535	/* XXX */;
    List_Init(&tcbPtr->reassList);

    Sock_BufAlloc(sockPtr, SOCK_RECV_BUF, RECV_SPACE);
    Sock_BufAlloc(sockPtr, SOCK_SEND_BUF, SEND_SPACE);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketClose --
 *
 *	Remove data from the socket's read and write queues and
 *	deallocate the TCP control block.
 *
 *  To do:
 *      1) make sure can free read/write queues (tcp may need to wait
 *         for acks before the data can be freed).
 *      2) reject waiting connection attempts
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_WOULD_BLOCK	- the socket can't be closed yet because some data
 *			  in the send buffer has not been ACKed.
 *
 * Side effects:
 *	The TCB may be destroyed. A packet may be sent to the remote host.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketClose(sockPtr)
    Sock_SharedInfo	*sockPtr;	/* Socket to be closed. */
{
    register TCPControlBlock *tcbPtr;
    tcbPtr = (TCPControlBlock *) sockPtr->protoData;

    /*
     * The TCB will be NULL if the connection gets reset before
     * being fully established.
     */
    if (tcbPtr == (TCPControlBlock *) NULL) {
	return(SUCCESS);
    }


    switch (tcbPtr->state) {
	/*
	 * If accepting connections, go through the queue of waiting 
	 * connections and drop them.
	 */
	case LISTEN:
	    while (ConnQueueNotEmpty(sockPtr, FALSE)) {
		Sock_InfoPtr    connPtr = ConnQueueNext(sockPtr);

		TCPDropConnection(connPtr, 
				(TCPControlBlock *) connPtr->protoData, 
				(ReturnStatus)NET_CONNECTION_RESET);
	    }

	    /* Fall into ... */

	case CLOSED:
	case SYN_SENT:
	case SYN_RECEIVED:
	    TCPCloseConnection(sockPtr, tcbPtr);
	    return(SUCCESS);
	    break;


	default:
	    if ((sockPtr->options & NET_OPT_LINGER) && (sockPtr->linger == 0)) {
		/*
		 * If linger wanted and not lingering then drop the connection.
		 */
		TCPDropConnection(sockPtr, tcbPtr, 
					(ReturnStatus)NET_CONNECTION_RESET);
	    } else {
		Sock_Disconnecting(sockPtr);
		Sock_BufRemove(sockPtr, SOCK_RECV_BUF, -1);

		switch (tcbPtr->state) {
		    case ESTABLISHED:
			tcbPtr->state = FIN_WAIT_1;
			break;

		    case CLOSE_WAIT:
			tcbPtr->state = LAST_ACK;
			break;

		    case FIN_WAIT_2:
		    case TIME_WAIT:
			Sock_Disconnected(sockPtr);
			break;

		    case FIN_WAIT_1:
		    case LAST_ACK:
			break;
		}
		(void) TCPOutput(sockPtr, tcbPtr);
	    }
	    break;
    }
    /*
     * Tell the client that it has to wait for the remote host to 
     * acknowledge our close.
     */
    return(FS_WOULD_BLOCK);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketRead --
 *
 *	Return data to a client doing a read on a TCP socket.
 *	Normal or urgent ("out-of-band") data can be retreived.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_WOULD_BLOCK	- the read queue was empty or didn't have
 *			  enough data to fill the buffer.
 *
 * Side effects:
 *	Data can be removed from the receive buffer.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketRead(privPtr, bufSize, buffer, amountReadPtr)
    register Sock_PrivInfo *privPtr;	/* Socket with data. */
    int		bufSize;		/* Amount of data to read. */
    Address	buffer;			/* Place to store the data. */
    int		*amountReadPtr;		/* Actual amount placed in buffer. */
{
    ReturnStatus status;
    Sock_SharedInfo	*sharePtr = privPtr->sharePtr;
    TCPControlBlock	*tcbPtr   = (TCPControlBlock *)sharePtr->protoData;
    int recvFlags = privPtr->recvFlags | SOCK_BUF_STREAM;
    int amtRead = 0;

    if (sharePtr->state != CONNECTED) {
	return(NET_NOT_CONNECTED);
    }

    /*
     * The user wants urgent ("out-of-band") data.
     */
    if (recvFlags & NET_OUT_OF_BAND) {
	/*
	 * If there's no urgent data or urgent data is supposed to remain
	 * in-line with other data or the urgent data is not available
	 * any more, then return an error.
	 */
	if (((tcbPtr->urgentBufPos == 0) &&
	      !(sharePtr->flags & SOCK_URGENT_DATA_NEXT)) ||
	    Sock_IsOptionSet(sharePtr, NET_OPT_OOB_INLINE) ||
	    tcbPtr->flags & TCP_HAD_URGENT_DATA) {

	    status = GEN_INVALID_ARG;
	} else if (!(tcbPtr->flags & TCP_HAVE_URGENT_DATA)) {
	    /*
	     * There's urgent data but it's not yet 
	     */
	    status = FS_WOULD_BLOCK;
	} else {

	    amtRead = 1;
	    *buffer = tcbPtr->urgentData;
	    if (!(recvFlags & NET_PEEK)) {
		/*
		 * If this is a destructive read, then turn off the HAVE bit
		 * and turn on the HAD bit to indicate the urgent data's 
		 * been read.
		 */
		tcbPtr->flags ^= (TCP_HAVE_URGENT_DATA | TCP_HAD_URGENT_DATA);
	    }
	    status = SUCCESS;
	}
    } else {

	/*
	 * Return "normal" data to the user. 
	 */

	sharePtr->flags &= ~SOCK_URGENT_DATA_NEXT;
	if ((tcbPtr->urgentBufPos > 0) && (bufSize > tcbPtr->urgentBufPos)) {
	    /*
	     * There's urgent data in the buffer -- just read enough normal 
	     * data until the position of the urgent data is reached.
	     */
	    bufSize = tcbPtr->urgentBufPos;
	}

	status = Sock_BufFetch(sharePtr, recvFlags, bufSize, buffer, &amtRead, 
			(Net_InetSocketAddr *) NULL);

	if (!(recvFlags & NET_PEEK)) {
	    tcbPtr->urgentBufPos -= amtRead;
	    if (tcbPtr->urgentBufPos == 0) {
		sharePtr->flags |= SOCK_URGENT_DATA_NEXT;
	    }

	    if (status == SUCCESS) {
		/*
		 * See if we need to send a window update to the peer.
		 */
		(void) TCPOutput(sharePtr, tcbPtr);
	    }
	}
    }

    if (amountReadPtr != (int *) NULL) {
	*amountReadPtr = amtRead;
    }

    if (status != FS_WOULD_BLOCK) {
	privPtr->recvFlags = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketWrite --
 *
 *	This routine appends the data to be written to the socket's
 *	send buffer and calls the output routine to send the data to
 *	the remote host.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_WOULD_BLOCK	- no room in the buffer to hold the data.
 *
 * Side effects:
 *	Packets containing the data may be sent.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketWrite(privPtr, packetPtr, amtWrittenPtr)
    Sock_PrivInfo	*privPtr;	/* Socket to write to. */ 
    IPS_Packet		*packetPtr;	/* Packet descriptor containing 
					 * data to be written. */
    int			*amtWrittenPtr;	/* Actual amount of data written. */
{
    ReturnStatus status;
    Sock_SharedInfo	*sharePtr = privPtr->sharePtr;

    if (sharePtr->state != CONNECTED) {
	return(NET_NOT_CONNECTED);
    }

    if ((privPtr->sendInfoValid) && 
	(privPtr->sendInfo.flags & NET_OUT_OF_BAND)) {

	TCPControlBlock	*tcbPtr   = (TCPControlBlock *)sharePtr->protoData;

	status = Sock_BufAppend(sharePtr, SOCK_SEND_BUF, FALSE,
		packetPtr->dataLen, packetPtr->data, packetPtr->base, 
		(Net_InetSocketAddr *) NULL, amtWrittenPtr);

	/*
	 * According to RFC1011 "Assigned Internet Protocols" (May 1987), 
	 * the urgent pointer points to the last octet of urgent data.  
	 * We continue, however, to consider it to indicate the first octet 
	 * of data past the urgent section.  Otherwise, send.urgentPtr 
	 * should be one lower. (This is the same behavior as 4.3BSD).
	 */
	if (status == SUCCESS) {
	    tcbPtr->send.urgentPtr = tcbPtr->send.unAck + 
			Sock_BufSize(sharePtr, SOCK_SEND_BUF, SOCK_BUF_USED);
	    tcbPtr->force = TRUE;
	    (void) TCPOutput(sharePtr, tcbPtr);
	    tcbPtr->force = FALSE;
	} else if (*amtWrittenPtr != 0) {
	    TCPOutput(sharePtr, tcbPtr);
	}
    } else {
	status = Sock_BufAppend(sharePtr, SOCK_SEND_BUF, FALSE,
		packetPtr->dataLen, packetPtr->data, packetPtr->base, 
		(Net_InetSocketAddr *) NULL, amtWrittenPtr);
	if (*amtWrittenPtr != 0) {
	    (void) TCPOutput(sharePtr, (TCPControlBlock *)sharePtr->protoData);
	}
    }

    if (status == SUCCESS) {
	privPtr->sendInfoValid = FALSE;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketSelect --
 *
 *	This routine checks the socket to see if there is
 *	1) data in the receive buffer to be read (active sockets),
 *	2) a new connection waiting (passive sockets),
 *	3) space in the send buffer for new data.
 *	4) urgent data to be read.
 *
 * Results:
 *	An or'd combination of FS_READBLE, FS_WRITABLE, FS_EXCEPTION.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TCP_SocketSelect(sockPtr)
    register Sock_SharedInfo	*sockPtr;	/* Socket to be selected. */
{
    int flags = 0;
    TCPControlBlock	*tcbPtr = (TCPControlBlock *)sockPtr->protoData;

    if (sockPtr->state == LISTENING) {
	if (ConnQueueNotEmpty(sockPtr, TRUE)) {
	    flags |= FS_READABLE;
	}
    } else if (Sock_IsRecvStopped(sockPtr) || 
	      ((sockPtr->state == CONNECTED) &&
	      (Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_USED) > 0))) {
	flags |= FS_READABLE;
    }

    if (Sock_IsSendStopped(sockPtr) || 
       ((sockPtr->state == CONNECTED) &&
       (Sock_BufSize(sockPtr, SOCK_SEND_BUF, SOCK_BUF_FREE) > 0))) {
	flags |= FS_WRITABLE;
    }

    /*
     * If the socket has been connecting to a remote peer and the
     * connection has just become established, then say the socket is
     * writable (the client waits for establishment by selecting for the 
     * for writing). 
     *
     * This flag is good for 1 select call: it has to be reset so the socket
     * won't be writable forerver.
     */
    if (sockPtr->justEstablished) {
	flags |= FS_WRITABLE;
	sockPtr->justEstablished = FALSE;
    }

    if ((sockPtr->state == CONNECTED) &&
	((sockPtr->flags & SOCK_URGENT_DATA_NEXT) ||
	(tcbPtr->urgentBufPos > 0))) {
	flags |= FS_EXCEPTION;
    }
    return(flags);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketBind --
 *
 *	Does TCP-specific stuff for assigning the local address of a socket.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_ADDRESS_NOT_AVAIL	- an invalid address was given.
 *	GEN_NO_PERMISSION	- the user tried to use a privileged port
 *				  and did not have super-user privilege.
 *	NET_ADDRESS_IN_USE	- the local <address,port> is already in use.
 *
 * Side effects:
 *	The socket's state changes and the local address is changed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketBind(sockPtr, addrPtr, userID)
    register Sock_SharedInfo	*sockPtr;/* Socket to be bound. */
    Net_InetSocketAddr		*addrPtr;/* The local address for the socket. */
    int				userID;	/* User ID of the process. */
{
    Net_InetSocketAddr	temp;
    temp = *addrPtr;

    /*
     * If a specific Inetnet address is given, make sure it is valid for
     * this host (i.e. it is one of the addresses assigned to the host).
     */

    if (temp.address != Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	if (!Rte_ValidateAddress(temp.address)) {
	    if (ips_Debug) {
		(void) fprintf(stderr, "TCP_SocketBind: address not valid\n");
	    }
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
	    if (ips_Debug) {
		(void) fprintf(stderr,
			       "TCP_SocketBind: no permission for port %d, user %d\n",
			       temp.port, userID);
	    }
	    return(GEN_NO_PERMISSION);
	}

	/*
	 * Check for the uniqueness of the new local <address,port> tuple
	 * among all TCP sockets.  The REUSE_ADDR option specifies the
	 * strictness of the check.  If the option is not set, then the
	 * local tuple can't be used by us if any other TCP socket is
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

	if (Sock_Match(TCP_PROTO_INDEX, temp.address, temp.port,
		    Net_HostToNetInt(NET_INET_ANY_ADDR), 0, wildcard) != 
					(Sock_InfoPtr    ) NULL) {
	    if (ips_Debug) {
		(void) fprintf(stderr, "TCP_SocketBind: address in use\n");
	    }
	    return(NET_ADDRESS_IN_USE);
	}
    }

    sockPtr->local = temp;
    sockPtr->state = HAVE_LOCAL_ADDR;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketConnect --
 *
 *	Establishes a connection with a remote host.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_ALREADY_CONNECTED	- the socket is already connected to a remote
 *				  host.
 *	NET_ADDRESS_NOT_AVAIL	- an invalid remote port was used.
 *	NET_ADDRESS_IN_USE	- the remote <address, port> is already used
 *				  by another socket.
 *	FS_WOULD_BLOCK		- a packet has been sent to the remote
 *				  host but the user must wait until the host
 *				  accepts the connection.
 *	
 * Side effects:
 *	A packet is sent to the remote host to initiate the connection
 *	handshake.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketConnect(sockPtr, remoteAddrPtr, initialize)
    register Sock_SharedInfo	*sockPtr;	/* Initialized socket. */
    Net_InetSocketAddr		*remoteAddrPtr;	/* Remote <address,port> to 
					 	 * connect to. */
    Boolean			initialize;	/* If TRUE, initialize the TCB*/
{
    TCPControlBlock	*tcbPtr;
    Net_InetSocketAddr	remote;

    remote = *remoteAddrPtr;
    /*
     * The socket's can only connect once so if it's in the process of 
     * connecting or is already connected, return an error.
     */

    if ((sockPtr->state == CONNECTING) || (sockPtr->state == CONNECTED))  {
	return(NET_ALREADY_CONNECTED);
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

    if (remote.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	remote.address = Rte_GetOfficialAddr(FALSE);
    } else if (remote.address == Net_HostToNetInt(NET_INET_BROADCAST_ADDR)) {
	remote.address = Rte_GetOfficialAddr(TRUE);
    }

    if (sockPtr->local.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	Net_InetAddress	localAddr;

	/*
	 * The socket doesn't have a local address part yet so 
	 * use the official address for this server. Make sure the
	 * <local <address,port> and remote <address,port>> tuple is
	 * unique among all open TCP sockets.
	 */

	localAddr = Rte_GetOfficialAddr(FALSE);
	if (Sock_Match(TCP_PROTO_INDEX, localAddr, 
		sockPtr->local.port, remote.address, remote.port, FALSE) !=
		(Sock_InfoPtr    ) NULL) {
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
	 * the <local, remote> tuple is unique among all open TCP sockets.
	 */

	if (Sock_Match(TCP_PROTO_INDEX, sockPtr->local.address, 
		sockPtr->local.port, remote.address, remote.port, FALSE) !=
		(Sock_InfoPtr    ) NULL) {
	    return(NET_ADDRESS_IN_USE);
	}
    }
    sockPtr->remote	= remote;
    sockPtr->state	= CONNECTING;

    if (!initialize) {
	/*
	 * The caller will handle the initialization.
	 */
	return(SUCCESS);
    } 

    stats.tcp.connAttempts++;
    tcbPtr = (TCPControlBlock *) sockPtr->protoData;
    TCPMakeTemplateHdr(sockPtr, tcbPtr);
    tcbPtr->state = SYN_SENT;
    tcbPtr->timer[TCP_TIMER_KEEP_ALIVE] = TCP_KEEP_TIME_INIT;
    tcbPtr->send.initial = tcpISS;
    tcpISS += TCP_INIT_SEND_SEQ_INCR/2;
    TCP_SEND_SEQ_INIT(tcbPtr);
    (void) TCPOutput(sockPtr, tcbPtr);

    /*
     * Tell the client to wait until out request is accepted.
     * The client should call Fs_Select waiting for the socket
     * to become writable.
     */
    return(FS_WOULD_BLOCK);
}

/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketListen --
 *
 *	Make the the TCP socket into a passive socket by changing to the 
 *	LISTENING state. In passive mode, the socket can wait for connection
 *	requests from remote hosts. The backlog argument specifies how
 *	many requests can be queued and not be immediately refused.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_BAD_OPERATION	- The socket was in the wrong state.
 *
 * Side effects:
 *	The socket state is changed to LISTEN, a connect info struct
 *	is allocated and initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketListen(sockPtr, backlog, userID)
    Sock_SharedInfo	*sockPtr;
    int			backlog;	/* How many pending requests to 
				 	 * queue before rejecting them. */
    int			userID;		/* User ID of the process. */
{
    TCPControlBlock		*tcbPtr;
    register TCPConnectInfo	*connectPtr; 

    /*
     * The socket must have been just created or had a local address given to
     * in order to go into listen mode. This means the socket can't be
     * connected.
     */

    if (sockPtr->state == CREATED) {
	ReturnStatus		status;
	Net_InetSocketAddr	sockAddr;

	/*
	 * Have to give the socket a local address.
	 */
	sockAddr.address = Net_HostToNetInt(NET_INET_ANY_ADDR);
	sockAddr.port = 0;
	status = TCP_SocketBind(sockPtr, &sockAddr, userID);
	if (status != SUCCESS) {
	    return(status);
	}
    } else if (sockPtr->state != HAVE_LOCAL_ADDR) {
	return(NET_BAD_OPERATION);
    }

    /*
     * Make sure the backlog value is in the proper range.
     */
    if (backlog <= 0) {
	backlog = 1;
    } else if (backlog > TCP_MAX_NUM_CONNECTS) {
	backlog = TCP_MAX_NUM_CONNECTS;
    }

    connectPtr = (TCPConnectInfo *) malloc(sizeof(TCPConnectInfo));
    connectPtr->head = 0;
    connectPtr->tail = 0;
    /*
     * The queue is circular so it requires 1 slot to be unused in order
     * to tell if the queue is full.
     */
    connectPtr->maxQueueSize = backlog + 1;

    tcbPtr = (TCPControlBlock *) sockPtr->protoData;
    tcbPtr->connectPtr	= connectPtr;
    tcbPtr->state	= LISTEN;
    sockPtr->state	= LISTENING;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketAccept --
 *
 *	This routine handles the first part of accepting a waiting connection 
 *	request. If there's a waiting request, a token is returned in 
 *	outBuffer. This token must be used with the NET_IOC_ACCEPT_CONN_2
 *	ioctl to complete the connection.
 *
 * Results:
 *	SUCCESS			- a connect token was returned in outBuffer.
 *	NET_BAD_OPERATION	- the socket is in the listening state.
 *	NET_NO_CONNECTS		- no pending connect requests.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketAccept(sockPtr, outBuffer)
    register Sock_SharedInfo	*sockPtr;	/* Listening socket. */
    Address			outBuffer;	/* Place to hold connect token*/
{
    if (sockPtr->state != LISTENING) {
	return(NET_BAD_OPERATION);
    }

    if (!ConnQueueNotEmpty(sockPtr, TRUE)) {
	return(NET_NO_CONNECTS);
    }

    *(ClientData *) outBuffer = (ClientData) ConnQueueNext(sockPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketDestroy --
 *
 *	Destroys all TCP-related memory allocated to a socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TCB memory is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TCP_SocketDestroy(data)
    ClientData	data;		/* Really a ptr to the TCB. */
{
    TCPControlBlock *tcbPtr;
    tcbPtr = (TCPControlBlock *) data;

    /*
     * The TCB apparently may be NULL if the process owning the socket
     * was killed asynchronously, so check against this so we don't
     * deference a NULL pointer.
     */
    if (tcbPtr == (TCPControlBlock *) NULL) {
	(void) fprintf(stderr, "TCP_SocketDestroy: tcb == NULL\n");
	return;
    }

    if (tcbPtr->connectPtr != (TCPConnectInfo *)NULL) {
	free((char *) tcbPtr->connectPtr);
    }
    if (tcbPtr->templatePtr != (Net_TCPHeader *)NULL) {
	free((char *) tcbPtr->templatePtr);
    }
    if (tcbPtr->IPTemplatePtr != (Net_IPHeader *)NULL) {
	free((char *) tcbPtr->IPTemplatePtr);
    }
    TCPCleanReassList(tcbPtr);
    TCPCancelTimers(tcbPtr);

    free((char *) tcbPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketShutdown --
 *
 *	This routine gracefully closes down a TCP connection.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	? 		- result from TCPOutput.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketShutdown(sockPtr, how)
    Sock_SharedInfo	*sockPtr;
    int			how;	   /* What operations to shut down. */
{
    register TCPControlBlock *tcbPtr;
    tcbPtr = (TCPControlBlock *) sockPtr->protoData;

    Sock_StopSending(sockPtr);

    switch (how) {
	case 0:
	    Sock_StopRecv(sockPtr);
	    break;

	case 1:
	    Sock_StopSending(sockPtr);
	    break;

	case 2:
	    Sock_StopRecv(sockPtr);
	    Sock_StopSending(sockPtr);
	    break;
    }

    if (tcbPtr != (TCPControlBlock *) NULL) {
	switch (tcbPtr->state) {
	    case CLOSED:
	    case LISTEN:
	    case SYN_SENT:
		TCPCloseConnection(sockPtr, tcbPtr);
		tcbPtr = (TCPControlBlock *) NULL;
		break;

	    case SYN_RECEIVED:
	    case ESTABLISHED:
		tcbPtr->state = FIN_WAIT_1;
		break;

	    case CLOSE_WAIT:
		tcbPtr->state = LAST_ACK;
		break;

	    case FIN_WAIT_2:
	    case TIME_WAIT:
		Sock_Disconnected(sockPtr);
		break;

	    case FIN_WAIT_1:
	    case LAST_ACK:
		break;
	}
	if (tcbPtr != (TCPControlBlock *) NULL) {
	    return(TCPOutput(sockPtr, tcbPtr));
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketSetOpt --
 *
 *	This routine changes the value of a TCP socket option.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	GEN_INVALID_ARG - bad option or invalid buffer size.
 *
 * Side effects:
 *	The tcb flags are updated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketSetOpt(sockPtr, optName, inBufSize, inBuffer)
    Sock_SharedInfo	*sockPtr;
    int		optName;		/* Option. */
    int		inBufSize;		/* Size in bytes of inBuffer. */
    Address	inBuffer;		/* Buffer containing input data. */
{
    TCPControlBlock *tcbPtr;

    /*
     * We expect at least 4 bytes of data
     * and this better be a TCP protocol socket,
     * or it probably won't have a valid protoData field.
     */
    if ((inBufSize < sizeof(int)) ||
	(sockPtr->protoIndex != TCP_PROTO_INDEX )) {
	return(GEN_INVALID_ARG);
    }

    tcbPtr = (TCPControlBlock *) sockPtr->protoData;

    switch (optName) {
	case NET_OPT_TCP_NO_DELAY:
	    if (*(int *)inBuffer) {
		tcbPtr->flags |= TCP_NO_DELAY;
	    } else {
		tcbPtr->flags &= ~TCP_NO_DELAY;
	    }
	    break;

	case NET_OPT_TCP_MAX_SEG_SIZE:
	    return(GEN_INVALID_ARG);	/* not implemented yet. */
	    break;

	default:
	    return(GEN_INVALID_ARG);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_SocketGetOpt --
 *
 *	This routine retreives the value of a TCP socket option.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	GEN_INVALID_ARG	- outbuffer too small or invalid option.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCP_SocketGetOpt(sockPtr, optName, outBufSize, outBuffer)
    Sock_SharedInfo	*sockPtr;
    int		optName;		/* Option. */
    int		outBufSize;		/* Size in bytes of outBuffer. */
    Address	outBuffer;		/* Buffer to store output data. */
{
    TCPControlBlock *tcbPtr;
    tcbPtr = (TCPControlBlock *) sockPtr->protoData;

    /*
     * We want to return at least  4 bytes of data.
     */
    if (outBufSize < sizeof(int)) {
	return(GEN_INVALID_ARG);
    }

    switch (optName) {
	case NET_OPT_TCP_NO_DELAY:
	    *(int *)outBuffer = tcbPtr->flags & TCP_NO_DELAY;
	    break;

	case NET_OPT_TCP_MAX_SEG_SIZE:
	    *(int *)outBuffer = tcbPtr->maxSegSize;
	    break;

	default:
	    return(GEN_INVALID_ARG);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * GetPort --
 *
 *	Given a local address, this routine finds a unused port and makes 
 *	sure it doesn't doesn't conflict with an existing socket's 
 *	binding address.
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
    Net_InetAddress	localAddr;
{
    static unsigned int port = INET_PRIV_PORTS;
    unsigned	int		netPort;
    unsigned	int		anyAddr = Net_HostToNetInt(NET_INET_ANY_ADDR);

    do {
	port++;
	if ((port < INET_PRIV_PORTS) || (port > INET_SERVER_PORTS)) {
	    port = INET_PRIV_PORTS;
	}
	netPort = Net_HostToNetShort(port);
    } while (Sock_Match(TCP_PROTO_INDEX, localAddr, netPort, anyAddr, 0, 
			FALSE) != (Sock_InfoPtr    ) NULL);
    return(netPort);
}


/*
 *----------------------------------------------------------------------
 *
 * TCPCloneConnection --
 *
 *	Creates a new socket for a connected TCP socket in the 
 *	listening state.
 *
 * Results:
 *	A pointer to the Sock_SharedInfo struct for the new socket or
 *	NULL, if there was a problem.
 *
 * Side effects:
 *	A new TCP socket was created.
 *
 *----------------------------------------------------------------------
 */

Sock_InfoPtr    
TCPCloneConnection(sockPtr, localAddr, localPort, remoteAddr, remotePort)
    Sock_SharedInfo	*sockPtr;	/* Socket to be cloned. */
    Net_InetAddress	localAddr;	/* The new socket's local address,port*/
    unsigned int	localPort;
    Net_InetAddress	remoteAddr;	/* The remote host's address, port. */
    unsigned int	remotePort;
{
    Sock_SharedInfo	*newSockPtr;
    Net_InetSocketAddr	sockAddr;

    if (sockPtr->state != LISTENING) {
	panic(
		"TCPCloneConnection: old socket not listening %d\n",
		sockPtr->state);
    }

    /*
     * See if there's space to accept the connection.
     */
    if (ConnQueueFull(sockPtr)) {
	return((Sock_InfoPtr    )NULL);
    }

    newSockPtr = Sock_Clone(sockPtr, TRUE);
    /*
     * We don't know the user ID of the client, so just lie and
     * say it's the super-user. This is not a security hole because
     * the true permissions check was made when the socket was bound.
     */
    (void) TCP_SocketOpen(newSockPtr, PROC_SUPER_USER_ID);

    /*
     * Connect the socket to the remote peer. If the connect routine
     * discovers a problem, then destroy the new socket.
     */

    newSockPtr->local.address = localAddr;
    newSockPtr->local.port = localPort;
    newSockPtr->state = HAVE_LOCAL_ADDR;

    sockAddr.address = remoteAddr;
    sockAddr.port = remotePort;
    if (TCP_SocketConnect(newSockPtr, &sockAddr, FALSE) != SUCCESS) {
	Sock_Destroy(newSockPtr);
	return((Sock_InfoPtr    )NULL);
    }

    ConnQueueAppend(sockPtr, newSockPtr);

    return(newSockPtr);
}



/*
 *----------------------------------------------------------------------
 *
 * TCPSockToTCB --
 *
 *	Returns the pointer to the TCP control block for a TCP socket.
 *
 * Results:
 *	The tcb pointer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TCPControlBlock *
TCPSockToTCB(sockPtr)
    Sock_InfoPtr    sockPtr;
{
    if (sockPtr->protoIndex != TCP_PROTO_INDEX) {
	panic("TCPSockToTCB: bad socket ptr\n");
    }
    return((TCPControlBlock *) sockPtr->protoData);
}

/*
 *----------------------------------------------------------------------
 *
 * TCPMakeTemplateHdr --
 *
 *	Create template to be used to send tcp packets on a connection.
 *	Call after host entry created, allocates an mbuf and fills
 *	in a skeletal tcp/ip header, minimizing the amount of work
 *	necessary when the connection is used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the headers is allocated.
 *
 *----------------------------------------------------------------------
 */

void
TCPMakeTemplateHdr(sockPtr, tcbPtr)
    Sock_SharedInfo 	*sockPtr;
    TCPControlBlock	*tcbPtr;
{
    register Net_TCPHeader *tcpHdrPtr;
    register Net_IPHeader *ipHdrPtr;

    ipHdrPtr = (Net_IPHeader *) malloc(sizeof(Net_IPHeader));
    ipHdrPtr->protocol	= NET_IP_PROTOCOL_TCP;
    ipHdrPtr->source	= sockPtr->local.address;
    ipHdrPtr->dest	= sockPtr->remote.address;
    tcbPtr->IPTemplatePtr = ipHdrPtr;

    tcpHdrPtr = (Net_TCPHeader *) malloc(sizeof(Net_TCPHeader));
    tcpHdrPtr->srcPort	= sockPtr->local.port;
    tcpHdrPtr->destPort	= sockPtr->remote.port;
    tcpHdrPtr->seqNum	= 0;
    tcpHdrPtr->ackNum	= 0;
    tcpHdrPtr->dataOffset = sizeof(Net_TCPHeader) / 4;
    tcpHdrPtr->flags	= 0;
    tcpHdrPtr->window	= 0;
    tcpHdrPtr->checksum	= 0;
    tcpHdrPtr->urgentOffset= 0;
    tcbPtr->templatePtr = tcpHdrPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TCPDropConnection --
 *
 *	Drop a TCP connection, reporting the specified error to the user.
 *	If the connection is synchronized, then send a RESET to the remote
 *	peer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TCB is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
TCPDropConnection(sockPtr, tcbPtr, status)
    Sock_SharedInfo		*sockPtr;
    register TCPControlBlock	*tcbPtr;	/* TCB to be dropped. */
    ReturnStatus		status;		/* Error to give
						   to client. */
{
    if (tcbPtr == (TCPControlBlock *) NULL) {
	(void) fprintf(stderr, "TCPDropConnection: tcb == NULL\n");
	return;
    }
    if (TCP_HAVE_RECVD_SYN(tcbPtr->state)) {
	tcbPtr->state = CLOSED;
	(void) TCPOutput(sockPtr, tcbPtr);
	stats.tcp.drops++;
    } else {
	stats.tcp.connDrops++;
    }
    Sock_SetError(sockPtr, status);
    TCPCloseConnection(sockPtr, tcbPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TCPCloseConnection --
 *
 *	Close a TCP control block by  discarding all space held by the tcb,
 *	and waking any waiters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TCB and socket may be destroyed.
 *
 *----------------------------------------------------------------------
 */

void
TCPCloseConnection(sockPtr, tcbPtr)
    Sock_SharedInfo		*sockPtr;
    register TCPControlBlock 	*tcbPtr;
{
    if (!Sock_HasUsers(sockPtr)) {
	Sock_Destroy(sockPtr);
    } else {
	TCP_SocketDestroy((ClientData) tcbPtr);
	sockPtr->protoData = (ClientData) NULL;
	Sock_Disconnected(sockPtr);
	sockPtr->local.address = 0;
	sockPtr->local.port = 0;
	sockPtr->remote.address = 0;
	sockPtr->remote.port = 0;
    }
    stats.tcp.closed++;
}

/*
 *----------------------------------------------------------------------
 *
 * TCP_Quench --
 *
 *	When a source quench is received, close congestion window
 *	to one segment.  We will gradually open it again as we proceed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TCP_Quench(tcbPtr)
    TCPControlBlock *tcbPtr;
{
    if (tcbPtr != NULL) {
	tcbPtr->send.congWindow = tcbPtr->maxSegSize;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ConnQueueFull --
 *
 *	Checks if the connection queue of a listening socket is full.
 *
 * Results:
 *	TRUE	- the queue is full.
 *	FALSE	- the queue is not full.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
ConnQueueFull(sockPtr)
    Sock_InfoPtr    sockPtr;	/* Listening socket. */
{
    register TCPConnectInfo *connPtr;
    connPtr = ((TCPControlBlock *)sockPtr->protoData)->connectPtr;

    if (((connPtr->tail+1) % connPtr->maxQueueSize) == connPtr->head) {
	return(TRUE);
    } 
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * ConnQueueNotEmpty --
 *
 *	Sees if the connection queue of a listening socket is not empty.
 *	If the mustBeConnected argument is TRUE, then if the queue
 *	is not empty, make sure the first socket in the queue is
 *	connected.
 *
 * Results:
 *	TRUE	- the queue is not empty.
 *	FALSE	- the queue is empty.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
ConnQueueNotEmpty(sockPtr, mustBeConnected)
    Sock_InfoPtr    sockPtr;	 /* Listening socket. */
    Boolean	    mustBeConnected; /* If TRUE, make sure the first socket
				  * on the connection queue is connected. */
{
    register TCPConnectInfo *connPtr;
    connPtr = ((TCPControlBlock *)sockPtr->protoData)->connectPtr;

    if (connPtr->tail != connPtr->head) {
	if (!mustBeConnected) {
	    return(TRUE);
	}
	/*
	 * If the socket is still connecting, don't return TRUE yet.
	 */
	if (connPtr->queue[connPtr->head]->state == CONNECTED) {
	    return(TRUE);
	}
    } 
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * ConnQueueAppend --
 *
 *	Appends a sockPtr to a connection queue of listening socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The queue is lengthened by 1.
 *
 *----------------------------------------------------------------------
 */

static void
ConnQueueAppend(sockPtr, newSockPtr)
    Sock_InfoPtr    sockPtr;		/* Listening socket. */
    Sock_InfoPtr    newSockPtr;	/* Socket for the new connection. */
{
    register TCPConnectInfo *connPtr;
    connPtr = ((TCPControlBlock *)sockPtr->protoData)->connectPtr;
    
    connPtr->queue[connPtr->tail] = newSockPtr;
    connPtr->tail = (connPtr->tail+1) % connPtr->maxQueueSize;
}


/*
 *----------------------------------------------------------------------
 *
 * ConnQueueNext --
 *
 *	Gets the next sockPtr on the connection queue of listening socket.
 *
 * Results:
 *	A Sock_SharedInfo ptr.
 *
 * Side effects:
 *	The queue is shortened by 1.
 *
 *----------------------------------------------------------------------
 */

static Sock_InfoPtr    
ConnQueueNext(sockPtr)
    Sock_InfoPtr    sockPtr;		/* Listening socket. */
{
    Sock_InfoPtr    newPtr;
    register TCPConnectInfo *connPtr;

    connPtr = ((TCPControlBlock *)sockPtr->protoData)->connectPtr;
    
    newPtr = connPtr->queue[connPtr->head];
    connPtr->head = (connPtr->head+1) % connPtr->maxQueueSize;
    return(newPtr);
}
