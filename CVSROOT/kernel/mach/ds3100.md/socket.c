/* 
 * socket.c --
 *
 *	Routines to emulate 4.3 BSD socket-related system calls for IPC
 *	using the Internet protocol suite. The routines make calls to 
 *	the Sprite Internet Server using Sprite system calls.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "user/bit.h"
#include "user/dev/net.h"
#include "user/fs.h"
#include "user/inet.h"
#include "user/status.h"
#include "user/stdlib.h"
#include "user/sys.h"
#include "user/sys/types.h"
#include "user/sys/socket.h"
#include "user/sys/uio.h"
#include "user/netinet/in.h"
#include "machInt.h"
#include "mach.h"

#include "compatInt.h"

extern Mach_State	*machCurStatePtr;

static ReturnStatus Wait _ARGS_((int socketID, Boolean readSelect, Time *timeOutPtr));

#ifdef DEBUG
#  define	DebugMsg(status, string) 	printf("%x: %s\n", status, string)
#else
#  define	DebugMsg(status, string)	;
#endif

static Boolean	gotHostName = FALSE;
static char 	streamDevice[100];
static char 	dgramDevice[100];
static char 	rawDevice[100];


/*
 *----------------------------------------------------------------------
 *
 * accept --
 *
 *	Accept a stream connection request. This call will create a new
 *	connection to the Inet server upon notification that a remote
 *	connection request is pending.
 *
 *	If the socket is non-blocking and no remote connection requests are
 *	pending, EWOULDBLOCK is returned. If the socket is blockable,
 *	this routine will wait until a remote connection request arrives.
 *
 * Results:
 *	If > 0, the stream ID of the new connection.
 *	If UNIX_ERROR, errno = 
 *	EINVAL		- Bad size for *namePtr.
 *	EWOULDBLOCK	- Non pending requests.
 *
 * Side effects:
 *	A new stream is created.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXAccept(socketID, addrPtr, addrLenPtr)
    int			socketID;	/* Socket to listen on. */
    struct sockaddr_in	*addrPtr;	/* Address of newly-accepted 
					 * connection. (out) */
    int			*addrLenPtr;	/* Size of *addrPtr. (in/out) */
{
    ReturnStatus	status;
    int			newSocket;
    int			addrLen;

    Address		usp;
    int			*newSocketPtr;
    ClientData		*acceptTokenPtr;
    struct sockaddr_in	addr;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];
    newSocketPtr = (int *)(usp - sizeof(int));
    acceptTokenPtr = (ClientData *) (usp - sizeof(int) - sizeof(ClientData));

    if (addrLenPtr == (int *) NULL) {
	addrLen = 0;
	addrPtr = (struct sockaddr_in *) NULL;
    } else {
	/*
	 * We deal with just Internet sockets.
	 */
	status = Vm_CopyIn(sizeof(addrLen), (Address)addrLenPtr, 
			   (Address)&addrLen);
	if (status != SUCCESS) {
	    return(status);
	}
	if (addrLen != sizeof(struct sockaddr_in)) {
	    return(SYS_INVALID_ARG);
	}
    }


    /*
     * Tell the Inet server to accept connections on the socket.  If a
     * connection is made, a token is returned that is used to convert the
     * connection into a new socket.  If no connections are currently
     * available and the socket is non-blocking, FS_WOULD_BLOCK is
     * returned. If the socket is blockable, the ioctl returns
     * NET_NO_CONNECTS so and Wait() must be used to wait for a
     * connection.
     */

    status = Fs_IOControlStub(socketID, IOC_NET_ACCEPT_CONN_1, 
				0, (Address) NULL, 
		    sizeof(ClientData), (Address) acceptTokenPtr);

    switch (status) {
	case SUCCESS:
	    break;

	case FS_WOULD_BLOCK:
	    return(FS_WOULD_BLOCK);

        case NET_NO_CONNECTS:
	    /*
	     * Wait for the server to tell us that a request has arrived.
	     */
	    (void) Wait(socketID, TRUE, (Time *) NULL);

	    /*
	     * There's a pending connection so retry the ioctl.
	     */
	    status = Fs_IOControlStub(socketID, IOC_NET_ACCEPT_CONN_1, 
				0, (Address) NULL, 
			sizeof(ClientData), (Address) acceptTokenPtr);
	    if (status != SUCCESS) {
		DebugMsg(status, "accept (ioctl 1b)");
		return(status);
	    }
	    break;

	default:
	    DebugMsg(status, "accept (ioctl 1a)");
	    return(status);
    } 


    /*
     * Create the new socket. This socket will be converted into the new
     * connection.
     */

    status = Fs_OpenStub(streamDevice, FS_READ|FS_WRITE, 0666, newSocketPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "accept (open)");
	return(status);
    }
    (void)Vm_CopyIn(sizeof(newSocket), (Address)newSocketPtr, 
		    (Address)&newSocket);

    /*
     * Make the new socket have the same characteristics as the
     * connection socket. Also, find out who we are connected to.
     */
    status = Fs_IOControlStub(newSocket, IOC_NET_ACCEPT_CONN_2, 
		    sizeof(ClientData), (Address) acceptTokenPtr, 
		    addrLen, (Address) addrPtr);

    if (status != SUCCESS) {
	DebugMsg(status, "accept (ioctl 2)");
	return(status);
    }
    if (addrLen > 0) {
	(void)Vm_CopyIn(addrLen, (Address)addrPtr, (Address)&addr);
	addr.sin_family = AF_INET;
	(void)Vm_CopyOut(addrLen, (Address)&addr, (Address)addrPtr);
    }
    machCurStatePtr->userState.unixRetVal = newSocket;

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * bind --
 *
 *	Assigns a local <address,port> tuple to a socket that does 
 *	not have one.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size for *namePtr, already bound to an address.
 *	EADDRINUSE	- The address is already in use.
 *	EADDRNOTAVAIL	- The address is not valid for this host.
 *
 * Side effects:
 *	The socket local address is set.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXBind(socketID, namePtr, nameLen)
    int			socketID;	/* Stream ID of unnamed socket. */
    struct sockaddr	*namePtr;	/* Local address,port for this socket.*/
    int			nameLen;	/* Size of *namePtr. */
{
    ReturnStatus	status;

    if (nameLen != sizeof(struct sockaddr_in)) {
	return(SYS_INVALID_ARG);
    }

    status = Fs_IOControlStub(socketID, IOC_NET_SET_LOCAL_ADDR, 
			nameLen, (Address) namePtr, 
			0, (Address) NULL);
    if (status != SUCCESS) {
	DebugMsg(status, "bind");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * connect --
 *
 *	For a stream socket, create a connection to a remote host.
 *	For a datagram socket, only receive datagrams from this remote 
 *	address.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size for *namePtr,
 *	EADDRINUSE	- The address is already in use.
 *	EADDRNOTAVAIL	- The address is not valid for this host.
 *	EISCONN		- The socket is already connected.
 *	ETIMEDOUT	- The connection request timed-out.
 *	ECONNREFUSED	- The remote host refused the connection.
 *	ENETUNREACH	- The network isn't reachable from this host.
 *	EWOULDBLOCK	- If non-blocking, the connection can't be completed
 *				immediately.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	A local address for the socket is given if it did not have one 
 *	already.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXConnect(socketID, namePtr, nameLen)
    int			socketID;	/* Stream ID of socket. */
    struct sockaddr	*namePtr;	/* Remote address,port to connect to.*/
    int			nameLen;	/* Size of *namePtr. */
{
    ReturnStatus	status;

    if (nameLen != sizeof(struct sockaddr_in)) {
	return(SYS_INVALID_ARG);
    }

    status = Fs_IOControlStub(socketID, IOC_NET_CONNECT,
			      nameLen, (Address) namePtr, 
			      0, (Address) NULL);

    if (status == FS_WOULD_BLOCK) {
	int flags = 0;
	Address	usp;
	int *intPtr;
	int optionsArray[2];
	int *userOptionsArray;

	usp = (Address)machCurStatePtr->userState.regState.regs[SP];
	intPtr = (int *)(usp - sizeof(int));
	userOptionsArray = 
		    (int *)(machCurStatePtr->userState.regState.regs[SP] - 
		    3 * sizeof(int));

	/*
	 * The connection didn't immediately complete, so wait if
	 * we're blocking or return EWOULDBLOCK if we're non-blocking.
	 */
	status = Fs_IOControlStub(socketID, IOC_GET_FLAGS, 
			0, (Address) NULL,
			sizeof(flags), (Address) intPtr);

	if (status != SUCCESS) {
	    DebugMsg(status, "connect (ioctl)");
	    return(status);
	}
	(void)Vm_CopyIn(sizeof(flags), (Address)intPtr, (Address)&flags);

	if (flags & IOC_NON_BLOCKING) {
	    return(FS_WOULD_BLOCK);
	} 

	status = Wait(socketID, FALSE, &time_OneMinute);

	if (status == FS_TIMEOUT) {
	    DebugMsg(status, "connect (select)");
	    return(status);
	}

	/*
	 * See if the connection successfully completed.
	 */
	optionsArray[0] = SOL_SOCKET;
	optionsArray[1] = SO_ERROR;
	status = Vm_CopyOut(2 * sizeof(int), (Address)optionsArray, 
			    (Address)userOptionsArray);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Fs_IOControlStub(socketID, IOC_NET_GET_OPTION,
			sizeof(optionsArray), (Address) userOptionsArray, 
			sizeof(int), (Address) intPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "connect (getsockopt)");
	    return(status);
	}
	(void)Vm_CopyIn(sizeof(int), (Address)intPtr, (Address)&status);
    } else if (status != SUCCESS) {
	DebugMsg(status, "connect");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * getpeername --
 *
 *	Find out the remote address that this socket is connected to.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size for *namePtr.
 *	ENOTCONN	- The socket is not connected.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXGetPeerName(socketID, namePtr, nameLenPtr)
    int			socketID;	/* Stream ID of socket connected to 
					 * remote peer. */
    struct sockaddr	*namePtr;	/* Upon return, <addr,port> for 
					 * remote peer. */
    int			*nameLenPtr;	/* Size of *namePtr. (in/out) */
{
    ReturnStatus	status;
    int			nameLen;

    if (nameLenPtr == (int *) NULL) {
	return(SYS_INVALID_ARG);
    }
    status = Vm_CopyIn(sizeof(int), (Address)nameLenPtr, (Address)&nameLen);
    if (status != SUCCESS) {
	return(status);
    }
    if (nameLen < sizeof(struct sockaddr_in)) {
	return(SYS_INVALID_ARG);
    }
    if (nameLen > sizeof(struct sockaddr_in)) {
	nameLen = sizeof(struct sockaddr_in);
	status = Vm_CopyOut(sizeof(int),(Address)&nameLen, (Address)nameLenPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }

    status = Fs_IOControlStub(socketID, IOC_NET_GET_REMOTE_ADDR, 
			0, (Address) NULL, 
			nameLen, (Address) namePtr);
    if (status != SUCCESS) {
	DebugMsg(status, "getpeername");
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * getsockname --
 *
 *	Find out the local address for this socket, which was
 *	set with the bind routine or by the connect routine.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size for *namePtr.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXGetSockName(socketID, namePtr, nameLenPtr)
    int			socketID;	/* Stream ID of socket to get name of.*/
    struct sockaddr	*namePtr;	/* Upon return, current <addr,port> for
					 * this socket. */
    int			*nameLenPtr;	/* Size of *namePtr. (in/out) */
{
    ReturnStatus	status;
    int			nameLen;

    if (nameLenPtr == (int *) NULL) {
	return(SYS_INVALID_ARG);
    }
    status = Vm_CopyIn(sizeof(int), (Address)nameLenPtr, (Address)&nameLen);
    if (status != SUCCESS) {
	return(status);
    }
    if (nameLen < sizeof(struct sockaddr_in)) {
	return(SYS_INVALID_ARG);
    }
    if (nameLen > sizeof(struct sockaddr_in)) {
	nameLen = sizeof(struct sockaddr_in);
	status = Vm_CopyOut(sizeof(int),(Address)&nameLen, (Address)nameLenPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }

    status = Fs_IOControlStub(socketID, IOC_NET_GET_LOCAL_ADDR, 
			      0, (Address) NULL, 
			      nameLen, (Address) namePtr);

    if (status != SUCCESS) {
	DebugMsg(status, "getsockname");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * getsockopt --
 *
 *	Get the value for a socket option.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXGetSockOpt(socketID, level, optName, optVal, optLenPtr)
    int		socketID;	/* Stream ID of socket to get options on. */
    int		level;		/* Socket or protocol level to get the option.*/
    int		optName;	/* Type of option to get. */
    char	*optVal;	/* Address of buffer to store the result. */
    int		*optLenPtr;	/* In: Size of *optVal, out: # of bytes stored
				 * in *optVal. */
{
    ReturnStatus	status;
    int			optionsArray[2];
    int			*userOptionsArray;
    int			optLen;
    ReturnStatus	sockStatus;

    userOptionsArray = (int *)(machCurStatePtr->userState.regState.regs[SP] - 
		               2 * sizeof(int));
    /*
     * OptionsArray is used to give the server the values of "level"
     * and "optName". A buffer ("newBufPtr") is needed to get the option
     * value and the length of the value.
     */
    optionsArray[0] = level;
    optionsArray[1] = optName;
    status = Vm_CopyOut(2 * sizeof(int), (Address)optionsArray, 
			(Address)userOptionsArray);
    if (status != SUCCESS) {
	return(status);
    }
    status = Vm_CopyIn(sizeof(int), (Address)optLenPtr, (Address)&optLen);
    if (status != SUCCESS) {
	return(status);
    }

    status = Fs_IOControlStub(socketID, IOC_NET_GET_OPTION,
			sizeof(optionsArray), (Address) userOptionsArray, 
			optLen, (Address) optVal);

    if (status != SUCCESS) {
	DebugMsg(status, "getsockopt");
	return(status);
    }

    if (optName == SO_ERROR) {
	/*
	 * The error value is a Sprite ReturnStatus so we must convert it
	 * to the equivalent Unix value.
	 */
	(void)Vm_CopyIn(sizeof(int), (Address)optVal, (Address)&sockStatus);
	sockStatus = Compat_MapCode(sockStatus);
	(void)Vm_CopyOut(sizeof(int), (Address)&sockStatus, (Address)optVal);
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * setsockopt --
 *
 *	Set the value for a socket option.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*VARARGS  (makes lint happy) */
ReturnStatus
MachUNIXSetSockOpt(socketID, level, optName, optVal, optLen)
    int		socketID;	/* Stream ID of socket to set options on. */
    int		level;		/* Socket or protocol level to get the option.*/
    int		optName;	/* Type of option to get. */
    char	*optVal;	/* Address of buffer to store the result. */
    int		optLen;		/* Size of *optVal. */
{
    ReturnStatus	status;
    int			*newBufPtr;
    int			*kernBufPtr;

    /*
     * To pass the level, the type of option and the option value to the
     * server, we allocate a new buffer and put the level and type in the
     * first 2 slots and then copy the value to the rest of the buffer.
     */
    newBufPtr = (int *) (machCurStatePtr->userState.regState.regs[SP] -
			 (2 * sizeof(int) + optLen));
    kernBufPtr = (int *)malloc(2 * sizeof(int) + optLen);

    kernBufPtr[0] = level;
    kernBufPtr[1] = optName;
    status = Vm_CopyIn(optLen, (Address)optVal, (Address)&kernBufPtr[2]);
    if (status != SUCCESS) {
	free((Address)kernBufPtr);
	return(status);
    }
    status = Vm_CopyOut(2 * sizeof(int) + optLen, (Address)kernBufPtr,
			(Address)newBufPtr);
    if (status != SUCCESS) {
	free((Address)kernBufPtr);
	return(status);
    }

    status = Fs_IOControlStub(socketID, IOC_NET_SET_OPTION,
			      2 * sizeof(int) + optLen, (Address)newBufPtr,
			      0, (Address) NULL);

    free((Address)kernBufPtr);

    if (status != SUCCESS) {
	DebugMsg(status, "getsockopt");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * listen --
 *
 *	Allows a stream socket to accept remote connection requests and to
 *	specify how many such requests will be queued up before they 
 *	are refused.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EOPNOTSUPP	- The socket type doesn't allow a listen operation.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXListen(socketID, backlog)
    int	socketID;	/* Stream ID of socket to be put in listen mode. */
    int	backlog;	/* How many connection requests to queue. */
{
    ReturnStatus	status;
    int			*backLogPtr;

    backLogPtr = (int *)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_CopyOut(sizeof(int), (Address)&backlog, (Address)backLogPtr);
    if (status != SUCCESS) {
	return(status);
    }
    status = Fs_IOControlStub(socketID, IOC_NET_LISTEN,
			sizeof(backlog), (Address)backLogPtr,
			0, (Address) NULL);
    if (status != SUCCESS) {
	DebugMsg(status, "listen");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * recv --
 *
 *	Read data from a connected socket. 
 *
 * Results:
 *	See recvfrom().
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXRecv(socketID, bufPtr, bufSize, flags)
    int		socketID;
    char	*bufPtr;	/* Address of buffer to place the data in. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of MSG_OOB, MSG_PEEK*/
{
    return(MachUNIXRecvFrom(socketID, bufPtr, bufSize, flags, 
			    (struct sockaddr *) NULL, (int *) NULL));
}

/*
 *----------------------------------------------------------------------
 *
 * recvfrom --
 *
 *	Read data from a socket.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size or address for senderPtr, senderLenPtr.
 *	EWOULDBLOCK	- If non-blocking, no data are available.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXRecvFrom(socketID, bufPtr, bufSize, flags, senderPtr, 
		 senderLenPtr)
    int			socketID;	/* Socket to read. */
    char		*bufPtr;	/* Buffer to place the data in. */
    int			bufSize;	/* Size of *bufPtr. */
    int			flags;		/* Type of operatrion: OR of 
					 *  MSG_OOB, MSG_PEEK*/
    struct sockaddr	*senderPtr;	/* Address of sender of the data. */
    int			*senderLenPtr;	/* Size of *senderPtr. (in/out) */
{
    ReturnStatus	status;
    int			*intPtr;
    int			senderLen;

    intPtr = (int *)(machCurStatePtr->userState.regState.regs[SP] - 4);
    if (senderPtr != (struct sockaddr *) NULL) {
	if (senderLenPtr == (int *) NULL) {
	    return(SYS_INVALID_ARG);
	}
	status = Vm_CopyIn(sizeof(int), (Address)senderLenPtr,
			   (Address)&senderLen);
	if (status != SUCCESS) {
	    return(status);
	}
	if (senderLen != sizeof(struct sockaddr_in)) {
	    return(SYS_INVALID_ARG);
	}
    }

    /*
     * If there are flags, ship them to the server.
     */
    if (flags != 0) {
	status = Vm_CopyOut(sizeof(flags), (Address)&flags, (Address)intPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FLAGS,
			      sizeof(flags), (Address)intPtr,
			      0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvfrom (ioctl recv_flags)");
	    return(status);
	}
    }

    status = Fs_ReadStub(socketID, bufSize, bufPtr, intPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "recvfrom (read)");
	return(status);
    }
    (void)Vm_CopyIn(sizeof(int), (Address)intPtr,
		   (Address)&machCurStatePtr->userState.unixRetVal);

    /*
     * If the caller wants the address of the sender, ask the server for it.
     */
    if (senderPtr != (struct sockaddr *) NULL) {
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FROM,
				  0, (Address) NULL,
				  senderLen, (Address) senderPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvfrom (ioctl get_remote)");
	    return(status);
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * recvmsg --
 *
 *	Read data from a socket using multiple buffers.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size or address for msg_name, msg_namelen;
 *			    I/O vector length too big.
 *	EWOULDBLOCK	- If non-blocking, no data are available.
 *	EFAULT		- Invalid argument to the ioctl, null address 
 *			   for msgPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXRecvMsg(socketID, msgPtr, flags)
    int			socketID;	/* Sokect to read data from. */
    struct msghdr	*msgPtr;	/* I/O vector of buffers to store the
					 * data. */
    int			flags;		/* Type of operatrion: OR of 
					 *  MSG_OOB, MSG_PEEK*/
{
    ReturnStatus	status;
    struct msghdr	msg;
    int			*intPtr;

    if (msgPtr == (struct msghdr *) NULL) {
	return(SYS_ARG_NOACCESS);
    }
    status = Vm_CopyIn(sizeof(msg), (Address)msgPtr, (Address)&msg);
    if (status != SUCCESS) {
	return(status);
    }

    if (msg.msg_name != (Address) NULL) {
	if (msg.msg_namelen != sizeof(struct sockaddr_in)) {
	    return(SYS_INVALID_ARG);
	}
    }

    if (msg.msg_iovlen > MSG_MAXIOVLEN) {
	return(SYS_INVALID_ARG);
    }

    intPtr = (int *)(machCurStatePtr->userState.regState.regs[SP] - 4);
    /*
     * If there are flags, ship them to the server.
     */
    if (flags != 0) {
	status = Vm_CopyOut(sizeof(flags), (Address)&flags, (Address)intPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FLAGS,
			    sizeof(flags), (Address)intPtr,
			    0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvmsg (ioctl recv_flags)");
	    return(status);
	}
    }
    status = MachUNIXReadv(socketID, msg.msg_iov, msg.msg_iovlen);
    if (status != SUCCESS) {
	return(status);
    }
    if (machCurStatePtr->userState.unixRetVal < 0) {
	DebugMsg(errno, "recvmsg (readv)");
    }
    
    /*
     * If the caller wants the address of the sender, ask the server for it.
     */
    if (msg.msg_name != (Address) NULL) {
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FROM,
			0, (Address) NULL,
			msg.msg_namelen, (Address) msg.msg_name);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvmsg (ioctl recv_from)");
	    return(status);
	}
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * send --
 *
 *	Write data to a connected socket.
 *
 * Results:
 *	See sendto().
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXSend(socketID, bufPtr, bufSize, flags)
    int		socketID;	/* Socket to send data on. */
    char	*bufPtr;	/* Address of buffer to send. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of 
				 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
{
    return(MachUNIXSendTo(socketID, bufPtr, bufSize, flags,
			  (struct sockaddr *)NULL, 0));
}

/*
 *----------------------------------------------------------------------
 *
 * sendto --
 *
 *	Send a message from a socket.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size or address for destPtr, destLen..
 *	EWOULDBLOCK	- If non-blocking, the server buffers are too
 *			  full to accept the data.
 *	EMSGSIZE	- The buffer is too large to be sent in 1 packet.
 *	EFAULT		- Invalid argument to the ioctl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXSendTo(socketID, bufPtr, bufSize, flags, destPtr, destLen)
    int		socketID;	/* Socket to send data on. */
    char	*bufPtr;	/* Address of buffer to send. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of 
				 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
    struct sockaddr	*destPtr;	/* Destination to send the data to. */
    int			destLen;	/* Size of *destPtr.  */
{
    ReturnStatus	status;
    Address		usp;
    int			*intPtr;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP]);
    usp -= sizeof(int);
    intPtr = (int *)usp;
    /*
     * If either the flags or a destination are given, send them to the server.
     */
    if ((flags != 0) || (destPtr != (struct sockaddr *) NULL)) {
	Net_SendInfo	sendInfo;
	Net_SendInfo	*sendInfoPtr;

	usp -= sizeof(Net_SendInfo);
	sendInfoPtr = (Net_SendInfo *)usp;
	if (destPtr != (struct sockaddr *) NULL) {
	    if (destLen != sizeof(struct sockaddr_in)) {
		return(SYS_INVALID_ARG);
	    }
	    sendInfo.addressValid = TRUE;
	    status = Vm_CopyIn(sizeof(Net_InetSocketAddr), (Address)destPtr,
		      (Address)&sendInfo.address.inet);
	    if (status != SUCCESS) {
		return(status);
	    }
	} else {
	    sendInfo.addressValid = FALSE;
	}
	sendInfo.flags = flags;
	status = Vm_CopyOut(sizeof(sendInfo), (Address)&sendInfo,
			    (Address)sendInfoPtr);
	if (status != SUCCESS) {
	    return(status);
	}

	status = Fs_IOControlStub(socketID, IOC_NET_SEND_INFO,
			sizeof(sendInfo), (Address)sendInfoPtr,
			0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "sendto (ioctl)");
	    return(status);
	}
    }

    status = Fs_WriteStub(socketID, bufSize, bufPtr, intPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "sendto (write)");
	return(status);
    }
    (void)Vm_CopyIn(sizeof(int), (Address)intPtr,
		    (Address)&machCurStatePtr->userState.unixRetVal);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * sendmsg --
 *
 *	Send a message from a socket.
 *
 * Results:
 *	If 0		- Successful.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- Bad size or address for msg_name, msg_namelen;
 *			    I/O vector length too big.
 *	EWOULDBLOCK	- If non-blocking, the server buffers are too full 
 *			  accept the data.
 *	EFAULT		- Invalid argument to the ioctl, null address 
 *			   for msgPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXSendMsg(socketID, msgPtr, flags)
    int			socketID;	/* Socket to send data on. */
    struct msghdr	*msgPtr;	/* I/O vector of buffers containing
					 * data to send. */
    int			flags;		/* Type of operatrion: OR of 
					 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
{
    ReturnStatus	status;
    struct msghdr	msg;

    if (msgPtr == (struct msghdr *) NULL) {
	return(SYS_ARG_NOACCESS);
    }
    status = Vm_CopyIn(sizeof(msg), (Address)msgPtr, (Address)&msg);
    if (status != SUCCESS) {
	return(status);
    }

    if (msg.msg_iovlen > MSG_MAXIOVLEN) {
	return(SYS_INVALID_ARG);
    }

    if ((flags != 0) || (msg.msg_name != (Address) NULL)) {
	Net_SendInfo	sendInfo;
	Net_SendInfo	*sendInfoPtr;

	if (msg.msg_name != (Address) NULL) {
	    if (msg.msg_namelen != sizeof(struct sockaddr_in)) {
		return(SYS_INVALID_ARG);
	    }
	    sendInfo.addressValid = TRUE;
	    status = Vm_CopyIn(sizeof(Net_InetSocketAddr),
			       (Address)msg.msg_name,
		               (Address)&sendInfo.address.inet);
	    if (status != SUCCESS) {
		return(status);
	    }
	} else {
	    sendInfo.addressValid = FALSE;
	}
	sendInfo.flags = flags;
	sendInfoPtr = 
	    (Net_SendInfo *) (machCurStatePtr->userState.regState.regs[SP] - 
			      sizeof(Net_SendInfo));
	status = Vm_CopyOut(sizeof(sendInfo), (Address)&sendInfo,
			    (Address)sendInfoPtr);
	if (status != SUCCESS) {
	    return(status);
	}

	status = Fs_IOControlStub(socketID, IOC_NET_SEND_INFO,
			    sizeof(sendInfo), (Address)sendInfoPtr,
			    0, (Address) NULL);

	if (status != SUCCESS) {
	    DebugMsg(status, "sendmsg (ioctl)");
	    return(status);
	}
    }

    status = MachUNIXWritev(socketID, msg.msg_iov, msg.msg_iovlen);
    if (status != SUCCESS) {
	DebugMsg(errno, "sendmsg (writev)");
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * socket --
 *
 *	Create a socket in the Internet domain.
 *
 * Results:
 *	If > 0, the stream ID of the new socket.
 *	If UNIX_ERROR, then errno = 
 *	EINVAL		- "domain" did not specify the Internet domain.
 *	?		- Error from Fs_Open, Fs_IOControl.
 *
 * Side effects:
 *	A new stream is created.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXSocket(domain, type, protocol)
    int	domain;		/* Type of communications domain */
    int	type;		/* Type of socket: SOCK_STREAM, SOCK_DGRAM, SOCK_RAW. */
    int	protocol;	/* Specific protocol to use. */
{
    ReturnStatus	status;
    int			streamID;
    int			*intPtr;
    extern char		machHostName[];

    if (domain != PF_INET) {
	return(SYS_INVALID_ARG);
    }

    if (!gotHostName) {
	gotHostName = TRUE;
#ifdef notdef
	strcpy(streamDevice, "/hosts/");
	strcat(streamDevice, machHostName);
	strcat(streamDevice, "/netTCP");
	strcpy(dgramDevice, "/hosts/");
	strcat(dgramDevice, machHostName);
	strcat(dgramDevice, "/netUDP");
	strcpy(rawDevice, "/hosts/");
	strcat(rawDevice, machHostName);
	strcat(rawDevice, "/netIP");
#endif
	sprintf(streamDevice, INET_STREAM_NAME_FORMAT, machHostName);
	sprintf(dgramDevice, INET_DGRAM_NAME_FORMAT, machHostName);
	sprintf(rawDevice, INET_RAW_NAME_FORMAT, machHostName);
    }

    intPtr = (int *)(machCurStatePtr->userState.regState.regs[SP] - 4);

    if (type == SOCK_STREAM) {
	status = Fs_OpenStub(streamDevice, FS_READ|FS_WRITE, 0666, intPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "socket (stream)");
	    return(status);
	}
    } else if (type == SOCK_DGRAM) {
	status = Fs_OpenStub(dgramDevice, FS_READ|FS_WRITE, 0666, intPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "socket (datagram)");
	    return(status);
	}
    } else if (type == SOCK_RAW) {
	status = Fs_OpenStub(rawDevice, FS_READ|FS_WRITE, 0666, intPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "socket (raw)");
	    return(status);
	}
    } else {
	return(SYS_INVALID_ARG);
    }
    (void)Vm_CopyIn(sizeof(int), (Address)intPtr, (Address)&streamID);

    if (protocol != 0) {
	(void)Vm_CopyOut(sizeof(int), (Address)&protocol, (Address)intPtr);
	status = Fs_IOControlStub(streamID, IOC_NET_SET_PROTOCOL,
			    sizeof(protocol), (Address)intPtr,
			    0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "socket (ioctl)");
	    return(status);
	}
    }

    machCurStatePtr->userState.unixRetVal = streamID;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * shutdown --
 *
 *	Shut down part of a full-duplex connection.
 *
 * Results:
 *	0		- The action was successful.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXShutdown(socketID, action)
    int		socketID;	/* Socket to shut down. */
    int		action;		/* 0 -> disallow further recvs, 
				 * 1 -> disallow further sends,
				 * 2 -> combination of above. */
{
    ReturnStatus	status;
    int			*intPtr;

    intPtr = (int *)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_CopyOut(sizeof(int), (Address)&action, (Address)intPtr);
    if (status != SUCCESS) {
	return(status);
    }
    status = Fs_IOControlStub(socketID, IOC_NET_SHUTDOWN, 
			sizeof(action), (Address)intPtr,
			0, (Address) NULL);

    if (status != SUCCESS) {
	DebugMsg(status, "shutdown");
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Wait --
 *
 *	Wait for the Inet server to indicate that a socket is ready
 *	for some action.
 *
 * Results:
 *	SUCCESS, or FS_TIMEOUT if a timeout occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
Wait(socketID, readSelect, timeOutPtr)
    int 	socketID;	/* Socket to wait on. */
    Boolean	readSelect;	/* If TRUE, select for reading, else select for
				 *  writing. */
    Time	*timeOutPtr;	/* Timeout to use for select. */
{
    ReturnStatus	status;
    int			numReady;
    Address		usp;
    int			*numReadyPtr;
    int			*userMaskPtr;
    Time		*userTimeOutPtr;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];
    usp -= sizeof(int);
    numReadyPtr = (int *)usp;

    if (timeOutPtr == (Time *)NULL) {
	userTimeOutPtr = (Time *)NULL;
    } else {
	usp -= sizeof(Time);
	userTimeOutPtr = (Time *)usp;
	status = Vm_CopyOut(sizeof(Time), (Address)timeOutPtr, 
			    (Address)userTimeOutPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    /*
     * Wait until the Inet server indicates the socket is ready.
     */

    if (socketID < 32) {
	int	mask;

	usp -= sizeof(int);
	userMaskPtr = (int *)usp;
	mask = 1 << socketID;
	status = Vm_CopyOut(sizeof(int), (Address)&mask, (Address)userMaskPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	if (readSelect) {
	    status = Fs_SelectStub(socketID, userTimeOutPtr, userMaskPtr,
				(int *) NULL, (int *) NULL, numReadyPtr);
	} else {
	    status = Fs_SelectStub(socketID, userTimeOutPtr, (int *) NULL, 
				userMaskPtr, (int *) NULL, numReadyPtr);
	}
    } else {
	int	*maskPtr;
	int	numBytes;

	Bit_Alloc(socketID, maskPtr);
	Bit_Set(socketID, maskPtr);
	numBytes = Bit_NumBytes(socketID);
	usp -= numBytes;
	userMaskPtr = (int *)usp;
	status = Vm_CopyOut(numBytes, (Address)maskPtr, (Address)userMaskPtr);
	if (status != SUCCESS) {
	    return(status);
	}

	if (readSelect) {
	    status = Fs_SelectStub(socketID, userTimeOutPtr, userMaskPtr, 
				(int *) NULL, (int *) NULL, numReadyPtr);
	} else {
	    status = Fs_SelectStub(socketID, userTimeOutPtr, (int *) NULL,
				userMaskPtr, (int *) NULL, numReadyPtr);
	}
	free((char *) maskPtr);
    }

    if (status == FS_TIMEOUT) {
	return(status);
    } else if (status != SUCCESS) {
	printf("Wait (socket.c): Fs_Select failed.\n");
    }

    (void)Vm_CopyIn(sizeof(int), (Address)numReadyPtr, (Address)&numReady);
    if (numReady != 1) {
	printf("Wait (socket.c): Fs_Select returned %d ready\n",
			    numReady);
    }

    return(SUCCESS);
}
