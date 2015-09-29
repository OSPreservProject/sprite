/* 
 * sockOps.c --
 *
 *	Routines to handle operations on sockets. The various operations
 *	include creating and destroying a socket, reading and writing,
 *	assigning local and remote addresses of a socket, connecting to
 *	remote hosts and accepting connections from remote hosts.
 *	Most of the routines in this file validate arguments and have
 *	protocol-dependent routines handle the semantics of the operation.
 *
 *	The routines in this file manipulate socket state information.
 *	The state information is stored in 2 different data structures 
 *	because some of the information can be shared among all clients 
 *	sharing the socket while a portion must be private to each client.
 *	Private information includes such things as the client's PID and 
 *	host ID, and values of flags to emulate the recvfrom and sento 
 *	calls. For each client sharing a socket, their Sock_PrivInfo 
 *	struct points to a Sock_SharedInfo struct. The shared state includes
 *	the send and receive buffers, the local and remote addresses, and
 *	protocol-specific information.
 *
 *	All sockets with the same type are kept on a linked list.
 *
 *	To do:
 *	1) stronger validation of token in ACCEPT_CONN_2 ioctl.
 *	2) complete get/set options.
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/sockOps.c,v 1.21 91/08/13 23:33:05 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev/net.h"
#include "ipServer.h"
#include "stat.h"
#include "socket.h"
#include "sockInt.h"
#include "tcp.h"
#include "udp.h"
#include "raw.h"

#include "fs.h"
#include "dev/pdev.h"

/*
 * The value of the socket address family used by Unix sockets. 
 * It is not used by Sprite programs.
 */
#define AF_INET	2

/*
 * Basic operations on sockets that must do protocol-dependent processing.
 * Only some of the operations are defined for some protocols.
 */
typedef struct {
    ReturnStatus	(*open)();	/* Create and initialize a socket. */
    ReturnStatus	(*close)();	/* Close and destroy a socket. */
    ReturnStatus	(*read)();	/* Read data from the peer. */
    ReturnStatus	(*write)();	/* Write data to the peer. */
    int			(*select)();	/* See if the socket is ready. */
    ReturnStatus	(*bind)();	/* Assign a local address. */
    ReturnStatus	(*listen)();	/* Make into a passive socket. */
    ReturnStatus	(*accept)();	/* On a passive socket, accept the first
					 * remote connection request. */
    ReturnStatus	(*connect)();	/* Create a virtual circuit to a remote
					 * host. */
    ReturnStatus	(*shutdown)();	/* Close a virtual circuit gracefully.*/
    void		(*destroy)();	/* Destroy a socket. */
    ReturnStatus	(*getOption)();	/* Get the value of an option. */
    ReturnStatus	(*setOption)();	/* Set the value of an option. */
} SocketOps;

/*
 * Information kept for each type of supported protocol.
 */
typedef struct {
    List_Links	list;	/* All sockets for the protocol are linked
			 * together on this queue. */
    SocketOps	ops;	/* Ptrs to basic functions for the protocol. */
} ProtocolInfo;

static ProtocolInfo	protoInfo[1 + MAX_PROTO_INDEX];


Sock_SharedInfo *curSock;


/*
 * A hack to convert a clientID into a Sock_PrivInfo ptr for dups.
 */

#define NUM_SOCKETS 256
static struct {
    int clientID;
    Sock_PrivInfo *privPtr;
} openSockets[NUM_SOCKETS];


/* Forward references. */
static ReturnStatus	HandleGenericIOC();
static ReturnStatus	GetOption();
static ReturnStatus	SetOption();

/*
 * Placeholder routines for protocols that don't handle some of 
 * the operations in the SocketOps structure.
 */
static void 		NoOp();
static ReturnStatus	DummyOp();
static ReturnStatus	BadOp();



/*
 *----------------------------------------------------------------------
 *
 * Sock_MemBin --
 *
 *	Calls Mem_Bin to optimize allocation of Sock structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	As above.
 *
 *----------------------------------------------------------------------
 */

void
Sock_MemBin()
{
    Mem_Bin(sizeof(Sock_BufDataInfo));
    Mem_Bin(sizeof(Sock_SharedInfo));
    Mem_Bin(sizeof(Sock_PrivInfo));
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Init --
 *
 *	Initializes the socket-related data structures and routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The protocol-dependent information struct is initialized.
 *
 *----------------------------------------------------------------------
 */

void
Sock_Init()
{
    register ProtocolInfo	*protoPtr;

    if (ips_Debug) {
	(void) fprintf(stderr, "Sock: sizeof(Sock_SharedInfo) = %d\n", 
				sizeof(Sock_SharedInfo));
	(void) fprintf(stderr, "Sock: sizeof(Sock_PrivInfo) = %d\n", 
				sizeof(Sock_PrivInfo));
    }

    protoPtr = &(protoInfo[RAW_PROTO_INDEX]);
    List_Init(&(protoPtr->list));
    protoPtr->ops.open		= Raw_SocketOpen;
    protoPtr->ops.close		= Raw_SocketClose;
    protoPtr->ops.read		= Raw_SocketRead;
    protoPtr->ops.write		= Raw_SocketWrite;
    protoPtr->ops.select	= Raw_SocketSelect;
    protoPtr->ops.bind		= Raw_SocketBind;
    protoPtr->ops.connect	= Raw_SocketConnect;
    protoPtr->ops.listen	= BadOp;
    protoPtr->ops.accept	= BadOp;
    protoPtr->ops.shutdown	= Raw_SocketShutdown;
    protoPtr->ops.destroy	= NoOp;
    protoPtr->ops.getOption	= DummyOp;
    protoPtr->ops.setOption	= DummyOp;

    protoPtr = &(protoInfo[UDP_PROTO_INDEX]);
    List_Init(&(protoPtr->list));
    protoPtr->ops.open		= UDP_SocketOpen;
    protoPtr->ops.close		= UDP_SocketClose;
    protoPtr->ops.read		= UDP_SocketRead;
    protoPtr->ops.write		= UDP_SocketWrite;
    protoPtr->ops.select	= UDP_SocketSelect;
    protoPtr->ops.bind		= UDP_SocketBind;
    protoPtr->ops.connect	= UDP_SocketConnect;
    protoPtr->ops.listen	= BadOp;
    protoPtr->ops.accept	= BadOp;
    protoPtr->ops.shutdown	= UDP_SocketShutdown;
    protoPtr->ops.destroy	= NoOp;
    protoPtr->ops.getOption	= DummyOp;
    protoPtr->ops.setOption	= DummyOp;

    protoPtr = &(protoInfo[TCP_PROTO_INDEX]);
    List_Init(&(protoPtr->list));
    protoPtr->ops.open		= TCP_SocketOpen;
    protoPtr->ops.close		= TCP_SocketClose;
    protoPtr->ops.read		= TCP_SocketRead;
    protoPtr->ops.write		= TCP_SocketWrite;
    protoPtr->ops.select	= TCP_SocketSelect;
    protoPtr->ops.bind		= TCP_SocketBind;
    protoPtr->ops.connect	= TCP_SocketConnect;
    protoPtr->ops.listen	= TCP_SocketListen;
    protoPtr->ops.accept	= TCP_SocketAccept;
    protoPtr->ops.shutdown	= TCP_SocketShutdown;
    protoPtr->ops.destroy	= TCP_SocketDestroy;
    protoPtr->ops.getOption	= TCP_SocketGetOpt;
    protoPtr->ops.setOption	= TCP_SocketSetOpt;
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_Open --
 *
 *	Allocate and initialize the Sock_PrivInfo and Sock_SharedInfo 
 *	structures for the new socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the structures is allocated. The structures are 
 *	initialized and the Sock_SharedInfo struct is added to a queue 
 *	of sockets with the same protocol.
 *
 *----------------------------------------------------------------------
 */

static char *protoNames[] = { "Raw", "UDP", "TCP"};

ReturnStatus
Sock_Open(sockPdevPtr, isNewOpen, streamID, fsFlags, 
		pid, hostID, userID, clientID, privPtrPtr)
    SockPdevState *sockPdevPtr;	/* Contains protocol # and info about the
				 * pseudo-device request buffer */
    Boolean 	isNewOpen;	/* If TRUE, the client is opening the socket
				 * for the first time. Otherwise, it's a 
				 * dup. operation due to a fork. */
    int		streamID;	/* Stream ID of the psuedo-device. */
    int		fsFlags;	/* Flags passed to Fs_Open by client. */
    Proc_PID	pid;		/* Process ID of the client. */
    int		hostID;		/* Host ID of client's machine. */
    int		userID;		/* User ID of the client. */
    int		clientID;	/* Unique ID for the client. */
    Sock_PrivInfo **privPtrPtr;	/* Out: ptr to the Sock_PrivInfo struct just 
				 *  created.*/
{
    ReturnStatus status;
    register Sock_PrivInfo	*privPtr;
    register Sock_SharedInfo	*sharePtr;
    register int		protoIndex = sockPdevPtr->protoIndex;

    stats.sock.open++;
    *privPtrPtr = (Sock_PrivInfo *)NULL;

    if (ips_Debug) {
	(void) fprintf(stderr,"Sock_Open(%d,%d,%x): %s  %s\n", 
		streamID, clientID, pid, 
		isNewOpen?"new":"dup", protoNames[protoIndex]);
    }

    if (protoIndex < 0 || protoIndex > MAX_PROTO_INDEX) {
	panic("Sock_Open: illegal protoIndex %d\n", protoIndex);
    }

    privPtr = (Sock_PrivInfo *) malloc(sizeof(Sock_PrivInfo));
    bzero((Address) privPtr, sizeof(Sock_PrivInfo) );
#ifndef KERNEL
    privPtr->streamID	= streamID;
#endif
    privPtr->fsFlags	= fsFlags;
    privPtr->pid	= pid;
    privPtr->hostID	= hostID;
    privPtr->userID	= userID;
    privPtr->clientID	= clientID;
    privPtr->recvFlags	= 0;
    privPtr->sendInfoValid = FALSE;
    List_InitElement(&privPtr->links);

    if (!isNewOpen) {
	fprintf(stderr, "Sock_Open: got obsolete dup request.\n");
	exit(1);
    }

    /*
     * Create the shared socket state.
     */

    sharePtr = (Sock_SharedInfo *) malloc(sizeof(Sock_SharedInfo));
    bzero( (Address) sharePtr, sizeof(Sock_SharedInfo));

    sharePtr->protoData	= (ClientData) NULL;
    sharePtr->protoIndex	= protoIndex;
    sharePtr->requestBuf	= sockPdevPtr->requestBuf;
    sharePtr->reqBufSize	= sockPdevPtr->reqBufSize;
    sharePtr->state		= CREATED;
    sharePtr->flags		= 0;
    sharePtr->owner.id	= 0;
    sharePtr->options	= NET_OPT_BROADCAST;
    sharePtr->clientCount	= 1;
    sharePtr->local.port	= 0;
    sharePtr->local.address	= Net_HostToNetInt(NET_INET_ANY_ADDR);
    sharePtr->remote.port	= 0;
    sharePtr->remote.address = Net_HostToNetInt(NET_INET_ANY_ADDR);
    sharePtr->linger	= 0;
    sharePtr->justEstablished	= FALSE;
    List_Init(&sharePtr->clientList);
    List_InitElement(&sharePtr->protoLinks);
    List_Insert(&sharePtr->protoLinks,
		    LIST_ATREAR(&protoInfo[protoIndex].list));
    privPtr->sharePtr = sharePtr;
#ifndef KERNEL
    openSockets[streamID].clientID = clientID;
    openSockets[streamID].privPtr = privPtr;
#endif

    /*
     * Now call the protocol-dependent open routine. It may reject
     * the open by returning a non-SUCCESS status.
     */

    status = protoInfo[protoIndex].ops.open(sharePtr, userID);
#ifdef lint
    status = TCP_SocketOpen(sharePtr, userID);
    status = UDP_SocketOpen(sharePtr, userID);
    status = Raw_SocketOpen(sharePtr, userID);
#endif

    if (status == SUCCESS) {
	*privPtrPtr = privPtr;
	List_Insert(&privPtr->links, LIST_ATREAR(&sharePtr->clientList));
	curSock = sharePtr;
    } else {
	free((char *) privPtr);
	if (sharePtr != (Sock_SharedInfo *) NULL) {
	    if (sharePtr->clientCount == 1) {
		List_Remove(&sharePtr->protoLinks);
		free((char *) sharePtr);
	    }
	    openSockets[streamID].clientID = 0;
	}
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_Close --
 *
 *	Clean up state associated with the socket if the protocol-dependent
 *	routine says it's OK.
 *
 * Results:
 *	SUCCESS		- the socket has been destroyed.
 *	FS_WOULD_BLOCK	- the socket is in the processing of closing.
 *
 * Side effects:
 *	Memory for the socket structure may be freed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sock_Close(privPtr)
    Sock_PrivInfo	*privPtr;	/* Socket to be closed. */
{
    register Sock_SharedInfo *sharePtr;
    ReturnStatus	status;

    stats.sock.close++;

#ifndef KERNEL
    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_Close(%d,%d,%x)\n",
		privPtr->streamID, privPtr->clientID, privPtr->pid);
    }
    openSockets[privPtr->streamID].clientID = 0;
#endif
    List_Remove(&privPtr->links);

    sharePtr = privPtr->sharePtr;

    if (sharePtr->clientCount > 1) {
	(void) fprintf(stderr, "Sock_Close got clientCount > 1 (%d)\n",
		sharePtr->clientCount);
	exit(1);
    }

    status = protoInfo[sharePtr->protoIndex].ops.close(sharePtr);
#ifdef lint
    status = TCP_SocketClose(sharePtr);
    status = UDP_SocketClose(sharePtr);
    status = Raw_SocketClose(sharePtr);
#endif
    /*
     * Clean up the socket structure.  A TCP connection
     * might be lingering in the close protocol so we don't
     * always nuke the socket descriptor.
     */
    sharePtr->clientCount--;
    free((char *) privPtr);
    if (sharePtr->requestBuf != (char *) NIL) {
	free((char *) sharePtr->requestBuf);
	sharePtr->requestBuf = (char *)NIL;
    }
    if (status == SUCCESS) {
	List_Remove(&sharePtr->protoLinks);
	free((char *) sharePtr);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_Read --
 *
 *	Supply data to the client's Fs_Read call.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_BAD_OPERATION	- the socket was not in a state that could
 *				  could receive data.
 *	GEN_INVALID_ARG		- bad bufSize.
 *	FS_WOULD_BLOCK		- no data are available.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sock_Read(privPtr, bufSize, buffer, amountReadPtr)
    Sock_PrivInfo	*privPtr;	/* Socket to be read from. */
    int			bufSize;	/* Amount to read. */
    Address		buffer;		/* Place to put the data. */
    int			*amountReadPtr;	/* Actual # of bytes stored in buffer.*/
{
    Sock_SharedInfo *sharePtr = privPtr->sharePtr;
    ReturnStatus	status;

    stats.sock.read++;

#ifndef KERNEL
    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_Read(%d,%d,%x): %d  flags=%x\n",
		privPtr->streamID, privPtr->clientID, privPtr->pid, bufSize,
		privPtr->recvFlags);
    }
#endif

    *amountReadPtr = 0;

    if (bufSize < 0) {
	return(GEN_INVALID_ARG);
    }

    if (sharePtr->error != SUCCESS) {
	status = sharePtr->error;
	sharePtr->error = SUCCESS;
    } else {
	status = protoInfo[sharePtr->protoIndex].ops.read(privPtr, 
			    bufSize, buffer, amountReadPtr);
#ifdef lint
    	status = TCP_SocketRead(privPtr, bufSize, buffer, amountReadPtr);
    	status = UDP_SocketRead(privPtr, bufSize, buffer, amountReadPtr);
    	status = Raw_SocketRead(privPtr, bufSize, buffer, amountReadPtr);
#endif
    }
    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_Read: status %x, amtRead %d\n",
			status, *amountReadPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_Write --
 *
 *	Takes data from the client and causes it to sent out on the network.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	NET_BAD_OPERATION	- the socket was not in a state that could
 *				  could send data.
 *	GEN_INVALID_ARG		- bad bufSize.
 *	FS_WOULD_BLOCK		- no data are available.
 *	
 *
 * Side effects:
 *	A packet will be sent out on the network.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sock_Write(privPtr, procID, bufSize, buffer, amtWrittenPtr)
    Sock_PrivInfo	*privPtr;	/* Socket to be written to. */
    Proc_PID		procID;		/* Process performing write. */
    int			bufSize;	/* Size in bytes of buffer. */
    Address		buffer;		/* Data to be sent. */
    int			*amtWrittenPtr;	/* Amount of data actually sent */
{
    ReturnStatus	status;
    Sock_SharedInfo *sharePtr = privPtr->sharePtr;

    stats.sock.write++;

#ifndef KERNEL
    if (ips_Debug) {
	(void) fprintf(stderr, 
		"Sock_Write(%d,%d,%x): %d  '%.*s'  flags=%x\n", 
		privPtr->streamID, privPtr->clientID, privPtr->pid, 
		bufSize, MIN(25, bufSize), buffer, privPtr->sendInfo.flags);
    }
#endif

    if (bufSize < 0) {
	return(GEN_INVALID_ARG);
    }

    /*
     * See if we're allowed to send data to the peer. If the socket is 
     * connection-based, it won't be writable if the peer has said to 
     * not send more data.
     */

    if (Sock_IsSendStopped(sharePtr)) {
#ifndef KERNEL
	Sig_Send(SIG_PIPE, procID, FALSE);
#endif
	return(FS_BROKEN_PIPE);
    } else if (sharePtr->error != SUCCESS) {
	status = sharePtr->error;
	sharePtr->error = SUCCESS;
    } else if (sharePtr->protoIndex == UDP_PROTO_INDEX) {
	/*
	 * Experimental optimized write-path for UDP.  We'll leave the
	 * data in the pseudo-device request buffer, set up the
	 * packet header there too, and even do fragmenting in
	 * place there.
	 */
	status = UDP_WriteRequest(privPtr, procID, bufSize, buffer,
				 amtWrittenPtr);
    } else {
	IPS_Packet	packet;

	IPS_InitPacket(bufSize, &packet);
	bcopy( buffer, packet.data, bufSize);
	status = protoInfo[sharePtr->protoIndex].ops.write(privPtr, &packet,
			     amtWrittenPtr);
#ifdef lint
	status = TCP_SocketWrite(privPtr, &packet, amtWrittenPtr);
	status = UDP_SocketWrite(privPtr, &packet, amtWrittenPtr);
	status = Raw_SocketWrite(privPtr, &packet, amtWrittenPtr);
#endif
	/*
	 * If there's no room to write all of the data, free the packet.
	 */
	if (status == FS_WOULD_BLOCK) {
	    /*
	     * If nothing was written, then free the buffer.
	     */
	    if (*amtWrittenPtr == 0) {
		free(packet.base);
	    }
	}
    }
    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_Write: status %x, amtWritten %d\n",
		    status, *amtWrittenPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_IOControl --
 *
 *	Performs various actions based on the command argument.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	other values from the called subroutines.
 *
 * Side effects:
 *	The socket information may be changed.
 *
 *----------------------------------------------------------------------
 */

static char *cmdToName[] = {
    "?",
    "LISTEN",
    "ACCEPT_1",
    "ACCEPT_2",
    "GET_LOCAL_ADDR",
    "SET_LOCAL_ADDR",
    "CONNECT",
    "GET_REMOTE_ADDR",
    "GET_OPTION",
    "SET_OPTION",
    "RECV_FLAGS",
    "RECV_FROM",
    "SEND_INFO",
    "SHUTDOWN",
    "SET_PROTOCOL",
};

ReturnStatus
Sock_IOControl(privPtr, command, userID, inBufSize, inBuffer, outBufSize,
	       outBuffer)
    Sock_PrivInfo	*privPtr;	/* Ptr to socket to do command on. */
    int		command;		/* Command to perform. */
    int 	userID;			/* userID of client at time of call */
    int		inBufSize;		/* Size in bytes of inBuffer. */
    Address	inBuffer;		/* Buffer of input data. */
    int		outBufSize;		/* Size in bytes of outBuffer. */
    Address	outBuffer;		/* Buffer to store output data. */
{
    Sock_SharedInfo *sharePtr = privPtr->sharePtr;
    ReturnStatus status;
    int type;

    stats.sock.ioctl++;

    /*
     * Figure out what type of command it is. If it's a generic command,
     * then let HandleGenericIOC deal with it. If it's a NET command,
     * then we deal with it here.
     */
#ifndef KERNEL
    if (ips_Debug) {
	(void) fprintf(stderr,"Sock_IOC(%d,%d,%x):", 
	    privPtr->streamID, privPtr->clientID, privPtr->pid);
    }
#endif

    type = command & 0xffff0000;

    if (type == 0) {
	return(HandleGenericIOC(privPtr, command, inBufSize, inBuffer,
				outBufSize, outBuffer));
    } else if (type != IOC_NET) {
	if (ips_Debug) {
	    (void) fprintf(stderr,"  INVALID: %x\n", command);
	} else {
	    /*
	    (void) fprintf(stderr, 
		"Sock_IOControl: invalid ioctl %x on (%d,%d,%x)\n", 
		command, privPtr->streamID, privPtr->clientID, privPtr->pid);
	    */
	}
	return(GEN_INVALID_ARG);
    }

    if (ips_Debug) {
	(void) fprintf(stderr," %s\n", cmdToName[command & 0xf]);
    }

    switch (command) {

	/*
	 * Convert an active socket into a passive one. The socket will
	 * only be able to accept incoming connection requests after 
	 * this call.
	 *
	 * In:   int	backlog;
	 */

	case IOC_NET_LISTEN:
	    if (inBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    if (sharePtr->error != SUCCESS) {
		status = sharePtr->error;
		sharePtr->error = SUCCESS;
		return(status);
	    } 
#ifdef lint
	    status = TCP_SocketListen(sharePtr, *(int *)inBuffer, 
			userID);
#endif
	    return(protoInfo[sharePtr->protoIndex].ops.listen(sharePtr, 
					*(int *)inBuffer, userID));
	    /*NOTREACHED*/
	    break;


	/*
	 * For a passive socket, wait for connection request from 
	 * any remote host.  This is the first half of acceptance sequence.
	 *
	 * Out:  ClientData *tokenPtr;
	 *       The token is used with the IOC_NET_ACCEPT_CONN_2 ioctl to 
	 *	 create a new socket for the connection.
	 */

	case IOC_NET_ACCEPT_CONN_1:
	    if (sharePtr->error != SUCCESS) {
		status = sharePtr->error;
		sharePtr->error = SUCCESS;
		return(status);
	    } 
	    if (outBufSize != sizeof(ClientData)) {
		return(GEN_INVALID_ARG);
	    }
#ifdef lint
	    status = TCP_SocketAccept(sharePtr, outBuffer);
#endif
	    status = protoInfo[sharePtr->protoIndex].ops.accept(sharePtr,
					outBuffer);
	    if ((status == NET_NO_CONNECTS) && 
		(privPtr->fsFlags & FS_NON_BLOCKING)) {

		status = FS_WOULD_BLOCK;
	    }
	    return(status);
	    break;


	/*
	 * This ioctl performs the second half of the accept sequence.
	 * Given the Sock_SharedInfo token from ACCEPT_CONN_2, it makes the 
	 * current sockPtr refer to the socket identified by the token.
	 *
	 * In: ClientData token;
	 * Out: Net_InetSocketAddr *addrPtr;
	 */
	case IOC_NET_ACCEPT_CONN_2: {
		Sock_SharedInfo *connSockPtr;

		/*
		 * We have two sockets now: *sockPtr is a socket that
		 * was just created opening the server's pseudo-device.
		 * It does not have any of the protocol-dependent information 
		 * initialized. *connSockPtr is a socket that was created when
		 * a connect request came in. It has the protocol-dependent 
		 * info initialized but none of the client-related info. 
		 * In this section we merge the 2 sockets into *connSockPtr,
		 * then destroy *sockPtr and finally, we cause *connSockPtr 
		 * to be used whenever the client uses this socket 
		 * pseudo-device.
		 */

		if (inBufSize != sizeof(ClientData)) {
		    return(GEN_INVALID_ARG);
		}
		connSockPtr = (Sock_SharedInfo *) *(ClientData *) inBuffer;

		/*
		 * Try to validate that the token is really a Sock_SharedInfo 
		 * ptr. We're at the mercy of the client to not mangle 
		 * the token...
		 *
		 * To do: stronger validation.
		 */
		if ((connSockPtr == (Sock_SharedInfo *) NULL) ||
		    (connSockPtr->state != CONNECTED)) {
		    return(GEN_INVALID_ARG);
		}

		/*
		 * Make sure the out buffer (socket address) is 
                 * at least the minimum size.
		 */
		if (outBufSize != 0) {
		    if (outBufSize < sizeof(Net_InetSocketAddr)) {
			return(GEN_INVALID_ARG);
		    }
		    connSockPtr->remote.addrFamily = AF_INET;
		    * (Net_InetSocketAddr *)outBuffer = connSockPtr->remote;
		}

		if (privPtr->sharePtr->clientCount != 1) {
		    panic("ACCEPT_2: client count = %d\n",
			privPtr->sharePtr->clientCount);
		    return(FAILURE);
		}

		/*
		 * Transfer the pdev request buffer from the old socket
		 * to the new one, so it can be freed properly when the
		 * new socket is deleted.
		 */

		connSockPtr->reqBufSize = privPtr->sharePtr->reqBufSize;
		connSockPtr->requestBuf = privPtr->sharePtr->requestBuf;
		Sock_Destroy(privPtr->sharePtr);

		connSockPtr->clientCount = 1;
		List_Insert(&privPtr->links, 
				LIST_ATREAR(&connSockPtr->clientList));
		privPtr->sharePtr = connSockPtr;
	    }
	    break;



	/*
	 * Get the local <address,port> tuple assigned to the socket.
	 */
	case IOC_NET_GET_LOCAL_ADDR:
	    /*
	     * States above CREATED have the local address.
	     */
	    if ((int)sharePtr->state <= (int)CREATED) {
		return(FAILURE);
	    }
	    if (outBufSize < sizeof(Net_InetSocketAddr)) {
		return(GEN_INVALID_ARG);
	    }
	    sharePtr->local.addrFamily = AF_INET;
	    *(Net_InetSocketAddr *)outBuffer = sharePtr->local;
	    break;


	/*
	 * Assign the local <address,port> tuple to the socket.
	 */
	case IOC_NET_SET_LOCAL_ADDR:

	    if (inBufSize != sizeof(Net_InetSocketAddr)) {
		return(GEN_INVALID_ARG);
	    }
	    /*
	     * It is an error to bind to an already bound socket.
	     */
	    if (sharePtr->flags & SOCK_HAVE_BOUND_ADDR) {
		return(GEN_INVALID_ARG);
	    }
#ifdef lint
	    status = TCP_SocketBind(sharePtr, (Net_InetSocketAddr *)inBuffer,
			userID);
	    status = UDP_SocketBind(sharePtr, (Net_InetSocketAddr *)inBuffer,
			userID);
	    status = Raw_SocketBind(sharePtr, (Net_InetSocketAddr *)inBuffer,
			userID);
#endif
	    status = protoInfo[sharePtr->protoIndex].ops.bind(sharePtr, 
			(Net_InetSocketAddr *)inBuffer, userID);
	    if (status == SUCCESS) {
		sharePtr->flags |= SOCK_HAVE_BOUND_ADDR;
	    }
	    return (status);
	    /*NOTREACHED*/
	    break;


	/*
	 * Restrict incoming packets to the given remote host.
	 * Optionally sets up a virtual circuit to the host.
	 */
	case IOC_NET_CONNECT:
	    if (inBufSize != sizeof(Net_InetSocketAddr)) {
		return(GEN_INVALID_ARG);
	    }
	    if (sharePtr->error != SUCCESS) {
		status = sharePtr->error;
		sharePtr->error = SUCCESS;
		return(status);
	    } 
#ifdef lint
	    status = TCP_SocketConnect(sharePtr, 
			(Net_InetSocketAddr *)inBuffer, TRUE);
	    status = UDP_SocketConnect(sharePtr, 
			(Net_InetSocketAddr *)inBuffer, TRUE);
	    status = Raw_SocketConnect(sharePtr, 
			(Net_InetSocketAddr *)inBuffer, TRUE);
#endif
	    return(protoInfo[sharePtr->protoIndex].ops.connect(sharePtr, 
			(Net_InetSocketAddr *)inBuffer, TRUE));
	    /*NOTREACHED*/
	    break;


	/*
	 * Get the <address,port> tuple of the remote host for a connected
	 * socket.
	 */
	case IOC_NET_GET_REMOTE_ADDR:
	    if ((int)sharePtr->state < (int)CONNECTED) {
		return(NET_NOT_CONNECTED);
	    }
	    if (outBufSize < sizeof(Net_InetSocketAddr)) {
		return(GEN_INVALID_ARG);
	    }
	    sharePtr->remote.addrFamily = AF_INET;
	    *(Net_InetSocketAddr *)outBuffer = sharePtr->remote;
	    break;


	/*
	 * Get the value of a socket option.
	 */
	case IOC_NET_GET_OPTION:
	    return(GetOption(sharePtr, inBufSize, inBuffer, outBufSize, 
				outBuffer));
	    /*NOTREACHED*/
	    break;


	/*
	 * Set the value of a socket option.
	 */
	case IOC_NET_SET_OPTION:
	    return(SetOption(sharePtr, inBufSize, inBuffer));
	    /*NOTREACHED*/
	    break;


	/*
	 * Sets the flags used in the next Sock_Read call.
	 */
	case IOC_NET_RECV_FLAGS:
	    if (inBufSize != sizeof(privPtr->recvFlags)) {
		return(GEN_INVALID_ARG);
	    }
	    privPtr->recvFlags = *(int *) inBuffer;
	    break;


	/*
	 * Returns the <address, port> tuple of the most recently received  
	 * packet.
	 */
	case IOC_NET_RECV_FROM:
	    if (sharePtr->error != SUCCESS) {
		status = sharePtr->error;
		sharePtr->error = SUCCESS;
		return(status);
	    } else if (outBufSize != sizeof(privPtr->recvFrom)) {
		return(GEN_INVALID_ARG);
	    }
	    privPtr->recvFrom.addrFamily = AF_INET;
	    *(Net_InetSocketAddr *)outBuffer = privPtr->recvFrom;
	    break;


	/*
	 * Sets the flag and the remote address for the next Sock_Write call.
	 */
	case IOC_NET_SEND_INFO:
	    if (inBufSize != sizeof(privPtr->sendInfo)) {
		return(GEN_INVALID_ARG);
	    } 
	    privPtr->sendInfo = *(Net_SendInfo *) inBuffer;
	    privPtr->sendInfoValid = TRUE;
	    break;


	/*
	 * Causes the socket to be closed down.
	 */
	case IOC_NET_SHUTDOWN:
	    if (inBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
#ifdef lint
    	    status = TCP_SocketShutdown(sharePtr, *(int *)inBuffer);
    	    status = UDP_SocketShutdown(sharePtr, *(int *)inBuffer);
    	    status = Raw_SocketShutdown(sharePtr, *(int *)inBuffer);
#endif
	    return(protoInfo[sharePtr->protoIndex].ops.shutdown(sharePtr, 
				*(int *)inBuffer));
	    /*NOTREACHED*/
	    break;


	/*
	 * Specify a protocol to be used by the socket.
	 */
	case IOC_NET_SET_PROTOCOL:
	    if (inBufSize != sizeof(sharePtr->protocol)) {
		return(GEN_INVALID_ARG);
	    } 
	    sharePtr->protocol = *(int *)inBuffer;
	    break;

	/*
	 * Return a flag to indicate if out-of-band data will be
	 * read at the next read call.
	 */
	case IOC_NET_IS_OOB_DATA_NEXT:
	    if (outBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    } 
	    *(int *)outBuffer = (sharePtr->flags & SOCK_URGENT_DATA_NEXT) != 0;
	    break;

	case IOC_NET_STATS:
	    if (inBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    Stat_Command(*(unsigned int *)inBuffer);
	    break;

	default:
	    (void) fprintf(stderr, "Warning: Sock_IOControl: unknown command %x\n",
			command);
	    break;
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * HandleGenericIOC --
 *
 *	Handles generic I/O controls.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	GEN_INVALID_ARG	- the in or out buffer size was wrong.
 *
 * Side effects:
 *	Socket state may be updated.
 *
 *----------------------------------------------------------------------
 */
static char *genericCommands[] = {
    "???",
    "REPOSITION",
    "GET_FLAGS",
    "SET_FLAGS",
    "SET_BITS",
    "CLEAR_BITS",
    "TRUNCATE",
    "LOCK",
    "UNLOCK",
    "NUM_READABLE",
    "GET_OWNER",
    "SET_OWNER",
    "MAP",
};

static ReturnStatus
HandleGenericIOC(privPtr, command, inBufSize, inBuffer, outBufSize, outBuffer)
    register Sock_PrivInfo *privPtr;	/* Socket to perform command on. */
    int		command;		/* Command to perform. */
    int		inBufSize;		/* Size in bytes of inBuffer. */
    Address	inBuffer;		/* Buffer of input data. */
    int		outBufSize;		/* Size in bytes of outBuffer. */
    Address	outBuffer;		/* Buffer to store output data. */
{
    int		flags;

    if (ips_Debug) {
	(void) fprintf(stderr,"  generic: %s\n", genericCommands[command]);
    }

    switch (command) {
	case IOC_GET_FLAGS:
	    if (outBufSize != sizeof(flags)) {
		return(GEN_INVALID_ARG);
	    }
	    *(int *)outBuffer = 0;
	    break;
	    
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	    if (inBufSize != sizeof(flags)) {
		return(GEN_INVALID_ARG);
	    }
	    flags = *(int *)inBuffer;
	    if (flags & IOC_NON_BLOCKING) {
		privPtr->fsFlags |= FS_NON_BLOCKING;
	    }
	    break;

	case IOC_CLEAR_BITS:
	    if (inBufSize != sizeof(flags)) {
		return(GEN_INVALID_ARG);
	    }
	    flags = *(int *)inBuffer;
	    if (flags & IOC_NON_BLOCKING) {
		privPtr->fsFlags &= ~FS_NON_BLOCKING;
	    }
	    break;

	case IOC_GET_OWNER:
	    if (outBufSize != sizeof(privPtr->sharePtr->owner)) {
		return(GEN_INVALID_ARG);
	    }
	    *(Ioc_Owner *)outBuffer = privPtr->sharePtr->owner;
	    break;

	case IOC_SET_OWNER:
	    if (inBufSize != sizeof(privPtr->sharePtr->owner)) {
		return(GEN_INVALID_ARG);
	    }
	    privPtr->sharePtr->owner = *(Ioc_Owner *)inBuffer;
	    break;

	case IOC_NUM_READABLE:
	    if (outBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    *(int *)outBuffer = Sock_BufSize(privPtr->sharePtr, SOCK_RECV_BUF,
					    SOCK_BUF_USED);
	    break;
	default:
	    break;
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * GetOption --
 *
 *	Handles the IOC_NET_GET_OPTION I/O control.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	GEN_INVALID_ARG	- the in or out buffer size was wrong.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
GetOption(sockPtr, inBufSize, inBuffer, outBufSize, outBuffer)
    register Sock_SharedInfo *sockPtr;	/* Socket to perform command on. */
    int		inBufSize;		/* Size in bytes of inBuffer. */
    Address	inBuffer;		/* Buffer of input data. */
    int		outBufSize;		/* Size in bytes of outBuffer. */
    Address	outBuffer;		/* Buffer to store output data. */
{
    int level;
    int optName;

    if (inBufSize != 2 * sizeof(int)) {
	return(GEN_INVALID_ARG);
    }
    level = ((int *)inBuffer)[0];
    optName = ((int *)inBuffer)[1];


    /*
     * See if the option is for the socket or a protocol.
     */
    if (level != NET_OPT_LEVEL_SOCKET) {
	int protoIndex;

	switch (level) {
	    case NET_IP_PROTOCOL_IP:
		protoIndex = RAW_PROTO_INDEX;
		break;

	    case NET_IP_PROTOCOL_TCP:
		protoIndex = TCP_PROTO_INDEX;
		break;

	    case NET_IP_PROTOCOL_UDP:
		protoIndex = UDP_PROTO_INDEX;
		break;

	    default:
		return(GEN_INVALID_ARG);
		break;
	}
	return(protoInfo[protoIndex].ops.getOption(sockPtr, 
		    optName, outBufSize, outBuffer));
    } else {

	/*
	 * We want to return at least  4 bytes of data.
	 */
	if (outBufSize < sizeof(int)) {
	    return(GEN_INVALID_ARG);
	}

	switch (optName) {
	    case NET_OPT_DEBUG:
	    case NET_OPT_REUSE_ADDR:
	    case NET_OPT_KEEP_ALIVE:
	    case NET_OPT_DONT_ROUTE:
	    case NET_OPT_BROADCAST:
	    case NET_OPT_USE_LOOPBACK:
	    case NET_OPT_OOB_INLINE:
		*(Boolean *)outBuffer = sockPtr->options & optName;
		break;

	    case NET_OPT_LINGER: {
		    Net_LingerInfo linger;

		    if (outBufSize != sizeof(linger)) {
			return(GEN_INVALID_ARG);
		    }

		    linger.onOff = sockPtr->options & NET_OPT_LINGER;
		    linger.linger = sockPtr->linger;
		    *(Net_LingerInfo *)outBuffer = linger;
		}
		break;

	    case NET_OPT_SEND_BUF_SIZE:
		*(int *)outBuffer = Sock_BufSize(sockPtr, SOCK_SEND_BUF, 
					SOCK_BUF_MAX_SIZE);
		break;

	    case NET_OPT_RECV_BUF_SIZE:
		*(int *)outBuffer = Sock_BufSize(sockPtr, SOCK_RECV_BUF, 
					SOCK_BUF_MAX_SIZE);
		break;

	    case NET_OPT_SEND_LOWAT:
		*(int *)outBuffer = 0;
				/* Sock_BufSize(sockPtr, SOCK_SEND_BUF, 
					SOCK_BUF_MIN_USED); */
		break;

	    case NET_OPT_RECV_LOWAT:
		*(int *)outBuffer = 0;
				/* Sock_BufSize(sockPtr, SOCK_RECV_BUF, 
					SOCK_BUF_MIN_USED); */
		break;

	    case NET_OPT_SEND_TIMEOUT:
		*(int *)outBuffer = 0;
		break;

	    case NET_OPT_RECV_TIMEOUT:
		*(int *)outBuffer = 0;
		break;

	    case NET_OPT_ERROR:
		*(int *)outBuffer = sockPtr->error;
		sockPtr->error = SUCCESS;
		break;

	    case NET_OPT_TYPE:
		*(int *)outBuffer = 0;
		break;


	    default:
		return(GEN_INVALID_ARG);
		break;
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * SetOption --
 *
 *	Handles the IOC_NET_SET_OPTION I/O control.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	GEN_INVALID_ARG	- the in buffer size was wrong.
 *
 * Side effects:
 *	Socket state is updated.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SetOption(sockPtr, inBufSize, inBuffer)
    register Sock_SharedInfo *sockPtr;	/* Socket to perform command on. */
    int		inBufSize;		/* Size in bytes of inBuffer. */
    Address	inBuffer;		/* Buffer of input data. */
{
    int level;
    int optName;

    /*
     * The input buffer has the following format:
     *  first 4 bytes:	level
     *  second 4 bytes:	option name
     *  rest:  		option data
     *
     * The option name implies the size of the option data.
     */

    if (inBufSize <= 2 * sizeof(int)) {
	return(GEN_INVALID_ARG);
    }
    level = ((int *)inBuffer)[0];
    optName = ((int *)inBuffer)[1];

    inBuffer  += 2 * sizeof(int);
    inBufSize -= 2 * sizeof(int);

    if (level != NET_OPT_LEVEL_SOCKET) {
	int protoIndex;

	switch (level) {
	    case NET_IP_PROTOCOL_IP:
		protoIndex = RAW_PROTO_INDEX;
		break;

	    case NET_IP_PROTOCOL_TCP:
		protoIndex = TCP_PROTO_INDEX;
		break;

	    case NET_IP_PROTOCOL_UDP:
		protoIndex = UDP_PROTO_INDEX;
		break;

	    default:
		return(GEN_INVALID_ARG);
		break;
	}
	return(protoInfo[protoIndex].ops.setOption(sockPtr, 
		    optName, inBufSize, inBuffer));
    } else {

	if (inBufSize < sizeof(int)) {
	    return(GEN_INVALID_ARG);
	}

	switch (optName) {
	    case NET_OPT_DEBUG:
		break;
	    case NET_OPT_REUSE_ADDR:
	    case NET_OPT_KEEP_ALIVE:
	    case NET_OPT_DONT_ROUTE:
	    case NET_OPT_BROADCAST:
	    case NET_OPT_USE_LOOPBACK:
	    case NET_OPT_OOB_INLINE:
		if (*(Boolean *)inBuffer) {
		    sockPtr->options |= optName;
		} else {
		    sockPtr->options &= ~optName;
		}
		break;

	    case NET_OPT_LINGER: 
		if (inBufSize != sizeof(Net_LingerInfo)) {
		    return(GEN_INVALID_ARG);
		}

		if (((Net_LingerInfo *)inBuffer)->onOff) {
		    sockPtr->options |= optName;
		} else {
		    sockPtr->options &= ~optName;
		}
		sockPtr->linger = ((Net_LingerInfo *)inBuffer)->linger;
		if (sockPtr->linger > SOCK_MAX_LINGER_TIME) {
		    sockPtr->linger = SOCK_MAX_LINGER_TIME;
		}
		break;

	    /*
	     * To do: implement the following options.
	     */
	    case NET_OPT_SEND_BUF_SIZE:
		break;

	    case NET_OPT_RECV_BUF_SIZE:
		break;

	    case NET_OPT_SEND_LOWAT:
		break;

	    case NET_OPT_RECV_LOWAT:
		break;

	    case NET_OPT_SEND_TIMEOUT:
		break;

	    case NET_OPT_RECV_TIMEOUT:
		break;

	    default:
		return(GEN_INVALID_ARG);
		break;
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Select --
 *
 *	Handles a select request from the client. The protocol-dependent
 *	select routine does the real work of checking if the socket is
 *	readable, writable or has an exception condition.
 *
 * Results:
 *	The state of the select bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_Select(ptr, isPrivPointer)
    ClientData    	ptr;		/* Sock_SharedInfo or PrivInfo ptr. */
    Boolean		isPrivPointer;	/* If TRUE, ptr is really a privPtr
					 * otherwise it's a sharePtr. */
{
    int flags = 0;
    Sock_SharedInfo *sharePtr;

    if (isPrivPointer) {
	sharePtr = ((Sock_PrivInfo *) ptr)->sharePtr;
    } else {
	sharePtr = (Sock_SharedInfo *) ptr;
    }

    stats.sock.select++;

    if (sharePtr->error != SUCCESS) {
	/*
	 * If there's an error, waiting to be discovered, lie and tell
	 * the client that what he wants is ready so he'll attempt the
	 * operation and then find the error.
	 */
	return(FS_READABLE|FS_WRITABLE|FS_EXCEPTION);
    }

#ifdef lint
    flags = TCP_SocketSelect(sharePtr);
    flags = UDP_SocketSelect(sharePtr);
    flags = Raw_SocketSelect(sharePtr);
#endif
    flags = protoInfo[sharePtr->protoIndex].ops.select(sharePtr);

    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_Select: out = %c%c%c\n", 
	    (flags & FS_READABLE)  ? 'R': '-',
	    (flags & FS_WRITABLE)  ? 'W': '-',
	    (flags & FS_EXCEPTION) ? 'E': '-');
    }
    return(flags);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_NotifyWaiter --
 *
 *	Issues an I/O control to tell the kernel that the select state
 *	of socket has changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The kernel's notion of the socket select state is changed.
 *
 *----------------------------------------------------------------------
 */

void
Sock_NotifyWaiter(sharePtr, flags)
    Sock_SharedInfo    *sharePtr;	/* This socket has new select state. */
    int			flags;		/* ignored. */
{
#ifndef KERNEL
    ReturnStatus status;
    register Sock_PrivInfo	*privPtr;

    int selectBits;

    if (ips_Debug) {
	(void) fprintf(stderr, "Sock_NotifyWaiter:  %c%c%c\n",
	    (flags & FS_READABLE)  ? 'R': '-',
	    (flags & FS_WRITABLE)  ? 'W': '-',
	    (flags & FS_EXCEPTION) ? 'E': '-');
    }

    /*
     * Use the OR'd combination of flags and Sock_Select because the
     * caller might want to indicate that something's ready that
     * isn't returned by the select routine. This is subject to race
     * conditions so the caller be prepared.
     */
    selectBits = flags | Sock_Select((ClientData) sharePtr, FALSE);

    /*
     * Go thought the list of clients and tell the kernel that 
     * the select flags have changed.
     */

    LIST_FORALL(&sharePtr->clientList, (List_Links *)privPtr) {
	if (ips_Debug) {
	    (void) fprintf(stderr, "\tnotifying (%d,%d,%x)\n",
		privPtr->streamID, privPtr->clientID, privPtr->pid);
	}

	status = Fs_IOControl(privPtr->streamID, IOC_PDEV_READY, 
			sizeof(int), (Address) &selectBits, 0, (Address) NULL);

	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Sock_NotifyWaiter: PDEV_READY ioctl");
	}
    }
#else
    /*
     * KERNEL code.
     */
    register FsSocketIOHandle *sockHandlePtr;
    register Sock_PrivInfo *privPtr;

    flags |= Sock_Select((ClientData)sharePtr, FALSE);

    LIST_FORALL(&sharePtr->clientList, (List_Links *)privPtr) {
	sockHandlePtr = privPtr->sockHandlePtr;
	if (flags & FS_READABLE) {
	    FsWaitListNotify(&sockHandlePtr->readWaitList);
	}
	if (flags & FS_WRITABLE) {
	    FsWaitListNotify(&sockHandlePtr->writeWaitList);
	}
	if (flags & FS_EXCEPTABLE) {
	    FsWaitListNotify(&sockHandlePtr->exceptWaitList);
	}
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Match --
 *
 *	Searchs the list of sockets for a protocol to see if any of
 *	them have the same <address,port> tuple for the given source and
 *	destination tuples. This routine is used by the protocol-dependent
 *	input procedures whenever a packet arrives in order to find the socket 
 *	information necessary to handle the packet.
 *
 * Results:
 *	A pointer to the socket info that matches the addresses.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Sock_SharedInfo *
Sock_Match(protIndex, localAddr, localPort, remoteAddr, remotePort,
	wildcardSearchWanted)
    int			protIndex;	/* Type of socket to look at. */
    Net_InetAddress	localAddr;	/* Local IP address to match. */
    unsigned int	localPort;	/* Local port address to match. */
    Net_InetAddress	remoteAddr;	/* Remote IP address to match. */
    unsigned int	remotePort;	/* Remote port address to match. */
    Boolean		wildcardSearchWanted;	/* If TRUE, allow wildcard
						 * matches. */
{
    register Sock_SharedInfo	*sockPtr;
    Sock_SharedInfo	*wildPtr = (Sock_SharedInfo *) NULL;

    int numWildcardMatches;
    int prevNumWildcardMatches = 3;

    /*
     * Search the list of sockets for the given protocol and try
     * to match the local and remote <address,port> tuples to the
     * ones in the socket. The addresses my be a wildcard value, that
     * is, a value that matches any address. If wildcarding is allowed,
     * try to find the socket with the highest number of exact matches.
     * If wildcarding is not allowed, then find the socket that has exact
     * matches.
     */

    LIST_FORALL(&protoInfo[protIndex].list, (List_Links *) sockPtr){
	if (sockPtr->protoIndex != protIndex) {
	    panic("Sock_Match: bad protocol %d\n", 
				sockPtr->protoIndex);
	}

	/*
	 * The local ports must always match.
	 */
	if (localPort != sockPtr->local.port) {
	    continue;
	}

	numWildcardMatches = 0;
	if (sockPtr->local.address != localAddr) {
	    /*
	     * If they don't match, see if one of them is a wildcard.
	     */
	    if ((sockPtr->local.address == 
		Net_HostToNetInt(NET_INET_ANY_ADDR)) ||
		(localAddr == Net_HostToNetInt(NET_INET_ANY_ADDR))) {

		numWildcardMatches += 1;
	    } else {
		/*
		 * Neither's a wildcard, so go on to the next socket.
		 */
		continue;
	    }
	} /* else {
	   *   The addresses match. They are real or wildcard addresses.
	   * }
	   *
	   */

	if (sockPtr->remote.address != remoteAddr ||
	    remoteAddr == Net_HostToNetInt(NET_INET_ANY_ADDR)) {
	    /*
	     * If they don't match or the remoteAddr is a wildcard (this is
	     * only used by the ds3100's when they boot), see if one of them 
	     * is a wildcard.
	     */
	    if ((sockPtr->remote.address == Net_HostToNetInt(NET_INET_ANY_ADDR)) ||
		(remoteAddr == Net_HostToNetInt(NET_INET_ANY_ADDR))) {

		numWildcardMatches += 1;
	    } else {
		/*
		 * Neither's a wildcard, so go on to the next socket.
		 */
		continue;
	    }
	} else {
	    /*
	     * The addresses match but make sure the port numbers match, too.
	     */
	    if (sockPtr->remote.port != remotePort) {
		continue;
	    }
	}

	if (numWildcardMatches == 0) {
	    /*
	     * Got an exact match!
	     */

	    return((Sock_SharedInfo *) sockPtr);

	} else if (!wildcardSearchWanted) {

	    /*
	     * We want an exact match but there was at least 1 wild card
	     * match, so go on to the next socket.
	     */
	    continue;

	} else if (numWildcardMatches < prevNumWildcardMatches) {
	    /*
	     * Got a wildcard match, but see if the other sockets
	     * are an exact match or have a smaller number of wildcard
	     * matches.
	     */

	    wildPtr = sockPtr;
	    prevNumWildcardMatches = numWildcardMatches;
	}
    }
    return((Sock_SharedInfo *) wildPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ReturnError --
 *
 *	Notifies sockets communicating with the given destination that 
 *	an error has occured. Called by the ICMP layer when it receives
 *	an error packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error status is saved in the socket info struct and the
 *	client is notified so it if was waiting, it can find out
 *	about the problem.
 *
 *----------------------------------------------------------------------
 */

void
Sock_ReturnError(status, protocol, destAddr, dataPtr)
    ReturnStatus	status;		/* Error status to be saved. */
    int			protocol;	/* Look at all sockets of this 
					 * protocol. */
    Net_InetAddress	destAddr;	/* Address that caused the error. */
    Address 		dataPtr;	/* Protocol-dependent value. */
{
    register Sock_SharedInfo	*sockPtr;
    int	destPort = 0;

    /*
     * Convert the official IP protocol numbers into its corresponding
     * internal number.
     */
    switch (protocol) {
	case NET_IP_PROTOCOL_IP:
	    protocol = RAW_PROTO_INDEX;
	    break;

	case NET_IP_PROTOCOL_TCP:
	    protocol = TCP_PROTO_INDEX;
	    destPort = ((Net_UDPHeader *)dataPtr)->destPort;
	    break;

	case NET_IP_PROTOCOL_UDP:
	    protocol = UDP_PROTO_INDEX;
	    destPort = ((Net_TCPHeader *)dataPtr)->destPort;
	    break;

	default:
	    (void) fprintf(stderr, "Warning: Sock_ReturnError: bad protocol %d\n",
			protocol);
	    return;
    }

    /*
     * Look at every socket for this protocol to see if the socket has
     * dealt with error-producing address. If a socket is connected, it's
     * easy to determine this.  There's only 1 remote address that the
     * socket deals with and its address is available.  If a socket isn't
     * connected, it can communicate will many hosts. Only the last
     * destination address is saved so if the socket sends many packets to
     * several addresses before the error comes in, the error value will
     * be missed.
     */
    LIST_FORALL(&protoInfo[protocol].list, (List_Links *) sockPtr) {
	if ((int)sockPtr->state >= (int)CONNECTED) {
	    if (sockPtr->remote.address == destAddr) {
		sockPtr->error = status;
		Sock_NotifyWaiter(sockPtr,FS_READABLE|FS_WRITABLE|FS_EXCEPTION);
	    }
	} else {
	    if ((sockPtr->sentTo.port == destPort)  &&
		(sockPtr->sentTo.address == destAddr)) {
		sockPtr->error = status;
		Sock_NotifyWaiter(sockPtr,FS_READABLE|FS_WRITABLE|FS_EXCEPTION);
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Clone --
 *
 *	Creates a new socket and copies the state information from
 *	an existing socket. Called by a protocol-dependent routine
 *	to duplicate a passive socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the new socket info struct is allocated.
 *
 *----------------------------------------------------------------------
 */

Sock_SharedInfo *
Sock_Clone(sockPtr, attachParent)
    Sock_SharedInfo	*sockPtr;	/* Socket to be cloned. */
    Boolean		attachParent;	/* If TRUE, save a ptr to the parent. */
{
    Sock_SharedInfo *newSockPtr;

    newSockPtr = (Sock_SharedInfo *) malloc(sizeof(Sock_SharedInfo));
    bzero((Address) newSockPtr, sizeof(Sock_SharedInfo));

    *newSockPtr = *sockPtr;
    newSockPtr->reqBufSize	= 0;
    newSockPtr->requestBuf	= 0;
    newSockPtr->state		= CREATED;
    newSockPtr->clientCount	= 0;
    newSockPtr->local.port	= 0;
    newSockPtr->local.address	= Net_HostToNetInt(NET_INET_ANY_ADDR);
    newSockPtr->remote.port	= 0;
    newSockPtr->remote.address	= Net_HostToNetInt(NET_INET_ANY_ADDR);
    newSockPtr->protoData	= (ClientData) NULL;
    if (attachParent) {
	newSockPtr->parentPtr = sockPtr;
    }

    List_Init(&newSockPtr->clientList);
    List_InitElement(&newSockPtr->protoLinks);
    List_Insert(&newSockPtr->protoLinks,
			LIST_ATREAR(&protoInfo[sockPtr->protoIndex].list));

    return(newSockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Destroy --
 *
 *	Frees all memory allocated to a Sock_SharedInfo struct.
 *	The protocol-dependent routine is called to destroy
 *	protocol data first.
 *
 *	The sockPtr should not be used after this routine completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
Sock_Destroy(sockPtr)
    Sock_SharedInfo *sockPtr;		/* Socket to be destroyed. */
{
    protoInfo[sockPtr->protoIndex].ops.destroy(sockPtr->protoData);
    List_Remove(&sockPtr->protoLinks);
    Sock_BufRemove(sockPtr, SOCK_RECV_BUF, -1);
    Sock_BufRemove(sockPtr, SOCK_SEND_BUF, -1);
    free((char *) sockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ScanList --
 *
 *	Called to scan a list of sockets of given protocol type.
 *
 * Results:
 *	The next socket on the list or NULL if the list is empty or
 *	the end of the list has been reached. The value should be
 *	passed in to the routine on the next call.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Sock_SharedInfo *
Sock_ScanList(protoIndex, sockPtr)
    int		protoIndex;		/* Search the list of sockets belonging
					 * to this protocol. */
    Sock_SharedInfo	*sockPtr;	/* Used to keep track of the position
					 * in the scan. */
{
    if (protoIndex < 0 || protoIndex > MAX_PROTO_INDEX) {
	panic("Sock_ScanInit: bad protocol index %d\n", 
			protoIndex);
    }

    if (sockPtr == (Sock_SharedInfo *) NULL) {
	/*
	 * Start of a scan. Return the first socket in the list or
	 * NULL if the list is empty.
	 */
	if (!List_IsEmpty(&protoInfo[protoIndex].list)) {
	    sockPtr = (Sock_SharedInfo *) 
				List_First(&protoInfo[protoIndex].list);
	} 
    } else {
	/*
	 * Continuing a scan. Return the next socket or NULL if the
	 * current socket is at the end of the list.
	 */
	sockPtr = (Sock_SharedInfo *) List_Next(&sockPtr->protoLinks);
	if (List_IsAtEnd(&protoInfo[protoIndex].list, &sockPtr->protoLinks)) {
	    sockPtr = (Sock_SharedInfo *) NULL;
	}
    }
    return(sockPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * NoOp --
 *
 *	A dummy routine that is used when a protocol does not implement
 *	a certain socket-related function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*VARARGS*/
static void
NoOp()
{
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * DummyOp --
 *
 *	A dummy routine that is used when a protocol does not implement
 *	a certain socket-related function.
 *
 * Results:
 *	Always returns SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*VARARGS*/
static ReturnStatus
DummyOp()
{
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * BadOp --
 *
 *	A dummy routine that is used when a protocol does not implement
 *	a certain socket-related function.
 *
 * Results:
 *	Always returns NET_BAD_OPERATION.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*VARARGS*/
static ReturnStatus
BadOp()
{
    return(NET_BAD_OPERATION);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_PrintInfo --
 *
 *	Scans the 3 lists of sockets (TCP,UDP,Raw) and prints the contents 
 *	of each active socket's  data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *stateNames[] = {
    "UNUSED",
    "CREATED",
    "HAVE_LOCAL_ADDR",
    "READY",
    "LISTENING",
    "CONNECTING",
    "CONNECTED",
    "DISCONNECTING",
    "DISCONNECTED",
};

/*ARGSUSED*/
int
Sock_PrintInfo(sigNum, sigCode)
    int sigNum;		/* Ignored. */
    int sigCode;	/* Ignored. */
{
    Sock_SharedInfo	*sharePtr;	/* Socket with the bad route. */

    (void) fprintf(stderr, "\nActive Sockets:\n\n");

    (void) fprintf(stderr, "%13s %10s %10s %10s %13s %13s\n",
	"TCP: State", "Shared", "ProtoPtr", "#Clients", "local", "remote");
    LIST_FORALL(&protoInfo[TCP_PROTO_INDEX].list, (List_Links *)sharePtr) {
	(void) fprintf(stderr,
	    "\n %-13s %10x %10x %10d %8x,%4d  %8x,%4d\n",
	    stateNames[(int)sharePtr->state],
	    sharePtr,
	    sharePtr->protoData,
	    sharePtr->clientCount,
	    sharePtr->local.address,
	    sharePtr->local.port,
	    sharePtr->remote.address,
	    sharePtr->remote.port);
	TCP_PrintInfo(sharePtr->protoData);
    }
    (void) fprintf(stderr, "\n");

    if (!List_IsEmpty(&protoInfo[UDP_PROTO_INDEX].list)) {
	(void) fprintf(stderr, "%13s %10s %10s %13s %13s\n",
	    "UDP: Shared", "State", "#Clients", "local", "remote");
	LIST_FORALL(&protoInfo[UDP_PROTO_INDEX].list, (List_Links *)sharePtr) {
	    (void) fprintf(stderr,
		"%10x %10s %10d %8x,%d  %8x,%d\n",
		sharePtr,
		stateNames[(int)sharePtr->state],
		sharePtr->clientCount,
		sharePtr->local.address,
		sharePtr->local.port,
		sharePtr->remote.address,
		sharePtr->remote.port);
	}
	(void) fprintf(stderr, "\n");
    }

    if (!List_IsEmpty(&protoInfo[RAW_PROTO_INDEX].list)) {
	(void) fprintf(stderr, "%13s %10s %10s %13s %13s\n",
	    "Raw: Shared", "State", "#Clients", "local", "remote");
	LIST_FORALL(&protoInfo[RAW_PROTO_INDEX].list, (List_Links *)sharePtr) {
	    (void) fprintf(stderr,
		"%10x %10s %10d %8x,%d  %8x,%d\n",
		sharePtr,
		stateNames[(int)sharePtr->state],
		sharePtr->clientCount,
		sharePtr->local.address,
		sharePtr->local.port,
		sharePtr->remote.address,
		sharePtr->remote.port);
	}
	(void) fprintf(stderr, "\n");
    }
}
