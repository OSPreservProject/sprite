/* 
 * socket.c --
 *
 *	Unix-compatibility routines to emulate 4.3 BSD socket-related
 *	system calls for IPC using the Internet protocol suite. The
 *	routines make calls to the Sprite Internet Server using Sprite
 *	system calls.
 *
 *	Note: there is some redundance with ioctl routines (IOControl, etc.)
 *	Some of these routines can be merged into the others.
 *
 * Copyright 1991 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <user/bit.h>
#include <user/dev/net.h>
#include <user/fs.h>
#include <user/inet.h>
#include <user/status.h>
#include <user/stdlib.h>
#include <user/sys.h>
#include <user/sys/types.h>
#include <user/sys/file.h>
#include <user/sys/uio.h>
#include <user/errno.h>
#include <mach.h>
#include <fsUnixStubs.h>
#include <vm.h>
#include <proc.h>
#include <fs.h>
#include <fsio.h>
#include <fsutil.h>
#include <compatInt.h>

extern Mach_State	*machCurStatePtr;

static ReturnStatus IOControlStub _ARGS_ ((int streamID, int command,
    int inBufSize, Address inBuffer, int outBufSize, Address outBuffer));

static char *errs[] = {"ENOERR", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO",
        "ENXIO", "E2BIG", "ENOEXEC", "EBADF", "ECHILD", "EAGAIN", "ENOMEM",
        "EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV",
        "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY",
        "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", "EROFS", "EMLINK", "EPIPE",
        "EDOM", "ERANGE", "EWOULDBLOCK", "EINPROGRESS", "EALREADY", "ENOTSOCK",
        "EDESTADDRREQ", "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT",
        "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", "EOPNOTSUPP", "EPFNOSUPPORT",
        "EAFNOSUPPORT", "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN",
        "ENETUNREACH", "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
        "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETIMEDOUT", "ECONNREFUSED",
        "ELOOP", "ENAMETOOLONG", "EHOSTDOWN", "EHOSTUNREACH", "ENOTEMPTY",
        "EPROCLIM", "EUSERS", "EDQUOT", "ESTALE", "EREMOTE"};

#undef Mach_SetErrno
#define Mach_SetErrno(err) if (debugFsStubs) { \
        printf("Error %d (%s) at %d in %s\n", err,\
        err<sizeof(errs)/sizeof(char *)?errs[err]:"",\
        __LINE__, __FILE__); } Proc_GetActualProc()->unixErrno = (err)

#if 1
#define	DebugMsg(status, string) printf("%x: %s\n", status, string)
#else
#define	DebugMsg(status, string)	;
#endif

static char streamDevice[276]; /* Max host name + extra format */
static char dgramDevice[276];
static char rawDevice[276];
static Boolean gotHostName = FALSE;

extern char sysHostName[];

extern int debugFsStubs;


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
#define UNIXRETURN(status) if (status==SUCCESS) { return 0; } else {\
	    Mach_SetErrno(Compat_MapCode(status)); return -1; }

int
Fs_AcceptStub(socketID, addrPtr, addrLenPtr)
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

    usp = (Address)Mach_UserStack();
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
	    UNIXRETURN(status);
	}
	if (addrLen < sizeof(struct sockaddr_in)) {
	    UNIXRETURN(SYS_INVALID_ARG);
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
	    UNIXRETURN(FS_WOULD_BLOCK);

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
		UNIXRETURN(status);
	    }
	    break;

	default:
	    DebugMsg(status, "accept (ioctl 1a)");
	    UNIXRETURN(status);
    } 


    /*
     * Create the new socket. This socket will be converted into the new
     * connection.
     */

    status = Fs_OpenStub(streamDevice, FS_READ|FS_WRITE, 0666, newSocketPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "accept (open)");
	UNIXRETURN(status);
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
	UNIXRETURN(status);
    }
    if (addrLen > 0) {
	(void)Vm_CopyIn(addrLen, (Address)addrPtr, (Address)&addr);
	addr.sin_family = AF_INET;
	(void)Vm_CopyOut(addrLen, (Address)&addr, (Address)addrPtr);
    }
    return newSocket;
}
/*
 *----------------------------------------------------------------------
 *
 * Fs_BindStub --
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

int 
Fs_BindStub(socketID, namePtr, nameLen)
    int			socketID;	/* Stream ID of unnamed socket. */
    struct sockaddr	*namePtr;	/* Local address,port for this socket.*/
    int			nameLen;	/* Size of *namePtr. */
{
    ReturnStatus	status;
    Proc_ControlBlock *procPtr;
    Fs_Stream 	 *streamPtr;
    Fs_IOCParam ioctl;
    Fs_IOReply reply;

    if (debugFsStubs) {
	printf("Fs_BindStub(%d, %x, %d)\n", socketID, namePtr, nameLen);
    }
    if (nameLen != sizeof(struct sockaddr_in)) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, socketID, &streamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    if (!Fsutil_HandleValid(streamPtr->ioHandlePtr)) {
	/* FS_STALE_HANDLE */
	Mach_SetErrno(EINVAL);
	return -1;
    }
    ioctl.command = IOC_NET_SET_LOCAL_ADDR;
    ioctl.format = mach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;
    ioctl.outBufSize = 0;
    ioctl.outBuffer = NULL;
    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM) {
	ioctl.inBufSize = nameLen;
	ioctl.inBuffer = (Address)namePtr;
	ioctl.flags = FS_USER_IN|FS_USER_OUT;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
	return 0;
    }
    ioctl.inBuffer = malloc(nameLen);
    status = Vm_CopyIn(nameLen, (Address)namePtr, ioctl.inBuffer);
    if (status != SUCCESS) {
	free(ioctl.inBuffer);
	Mach_SetErrno(EFAULT);
	return -1;
    }
    status = Fs_IOControl(streamPtr, &ioctl, &reply);
    free(ioctl.inBuffer);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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

int
Fs_GetpeernameStub(socketID, namePtr, nameLenPtr)
    int			socketID;	/* Stream ID of socket connected to 
					 * remote peer. */
    struct sockaddr	*namePtr;	/* Upon return, <addr,port> for 
					 * remote peer. */
    int			*nameLenPtr;	/* Size of *namePtr. (in/out) */
{
    ReturnStatus	status;
    int			nameLen;

    if (debugFsStubs) {
	printf("Fs_GetpeernameStub(%d, %x, %x)\n", socketID, namePtr,
		nameLenPtr);
    }
    if (nameLenPtr == (int *) NULL) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Vm_CopyIn(sizeof(int), (Address)nameLenPtr, (Address)&nameLen);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    if (nameLen < sizeof(struct sockaddr_in)) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    if (nameLen > sizeof(struct sockaddr_in)) {
	nameLen = sizeof(struct sockaddr_in);
	status = Vm_CopyOut(sizeof(int),(Address)&nameLen, (Address)nameLenPtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    status = Fs_IOControlStub(socketID, IOC_NET_GET_REMOTE_ADDR, 
			0, (Address) NULL, 
			nameLen, (Address) namePtr);
    if (status != SUCCESS) {
	DebugMsg(status, "getpeername");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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

int
Fs_GetsocknameStub(socketID, namePtr, nameLenPtr)
    int			socketID;	/* Stream ID of socket to get name of.*/
    struct sockaddr	*namePtr;	/* Upon return, current <addr,port> for
					 * this socket. */
    int			*nameLenPtr;	/* Size of *namePtr. (in/out) */
{
    ReturnStatus	status;
    int			nameLen;

    if (debugFsStubs) {
	printf("Fs_GetsocknameStub(%d, %x, %x)\n", socketID, namePtr,
		nameLenPtr);
    }
    if (nameLenPtr == (int *) NULL) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Vm_CopyIn(sizeof(int), (Address)nameLenPtr, (Address)&nameLen);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    if (nameLen < sizeof(struct sockaddr_in)) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    if (nameLen > sizeof(struct sockaddr_in)) {
	nameLen = sizeof(struct sockaddr_in);
	status = Vm_CopyOut(sizeof(int),(Address)&nameLen, (Address)nameLenPtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }

    status = Fs_IOControlStub(socketID, IOC_NET_GET_LOCAL_ADDR, 
			      0, (Address) NULL, 
			      nameLen, (Address) namePtr);

    if (status != SUCCESS) {
	DebugMsg(status, "getsockname");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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
int
Fs_GetsockoptStub(socketID, level, optName, optVal, optLenPtr)
    int		socketID;	/* Stream ID of socket to get options on. */
    int		level;		/* Socket or protocol level to get the option.*/
    int		optName;	/* Type of option to get. */
    char	*optVal;	/* Address of buffer to store the result. */
    int		*optLenPtr;	/* In: Size of *optVal, out: # of bytes stored
				 * in *optVal. */
{
    ReturnStatus	status;
    int			optionsArray[2];
    int			optLen;
    Address             tmp;

    if (debugFsStubs) {
	printf("Fs_GetsockoptStub(%d, %d, %d, %x, %x)\n", socketID,
		level, optName, optVal, optLenPtr);
    }

    /*
     * OptionsArray is used to give the server the values of "level"
     * and "optName". A buffer ("newBufPtr") is needed to get the option
     * value and the length of the value.
     */
    optionsArray[0] = level;
    optionsArray[1] = optName;

    status = Vm_CopyIn(sizeof(int), (Address)optLenPtr, (Address)&optLen);
    if (status != SUCCESS) {
	return(status);
    }
    tmp = malloc(optLen);
    status = IOControlStub(socketID, IOC_NET_GET_OPTION,
			sizeof(optionsArray), (Address) optionsArray, 
			optLen, tmp);

    if (status != SUCCESS) {
	printf("getsockopt: option %d returned status %d\n", optName, status);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    if (optName == SO_ERROR) {
	/*
	 * The error value is a Sprite ReturnStatus so we must convert it
	 * to the equivalent Unix value.
	 */
	* (int *) tmp = Compat_MapCode(* (int *) tmp);
    }
    status = Vm_CopyOut(optLen, tmp, optVal);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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

/*VARARGS*/
int
Fs_SetsockoptStub(socketID, level, optName, optVal, optLen)
    int		socketID;	/* Stream ID of socket to set options on. */
    int		level;		/* Socket or protocol level to get the option.*/
    int		optName;	/* Type of option to get. */
    char	*optVal;	/* Address of buffer to store the result. */
    int		optLen;		/* Size of *optVal. */
{
    ReturnStatus	status;
    int			*kernBufPtr;

    if (debugFsStubs) {
	printf("Fs_SetsockoptStub(%d, %d, %d, %x, %d)\n", socketID,
		level, optName, optVal, optLen);
    }
    kernBufPtr = (int *)malloc(2 * sizeof(int) + optLen);
    kernBufPtr[0] = level;
    kernBufPtr[1] = optName;
    status = Vm_CopyIn(optLen, (Address)optVal, (Address)&kernBufPtr[2]);
    if (status != SUCCESS) {
	free((Address)kernBufPtr);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    status = IOControlStub(socketID, IOC_NET_SET_OPTION,
			      2 * sizeof(int) + optLen, (Address)kernBufPtr,
			      0, (Address) NULL);
    free((Address)kernBufPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "getsockopt");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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
Fs_ListenStub(socketID, backlog)
    int	socketID;	/* Stream ID of socket to be put in listen mode. */
    int	backlog;	/* How many connection requests to queue. */
{
    ReturnStatus	status;

    if (debugFsStubs) {
	printf("Fs_ListenStub(%d, %d)\n", socketID, backlog);
    }
    status = IOControlStub(socketID, IOC_NET_LISTEN,
			sizeof(backlog), (Address) &backlog,
			0, (Address) NULL);
    if (status != SUCCESS) {
	DebugMsg(status, "listen");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
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

int
Fs_RecvStub(socketID, bufPtr, bufSize, flags)
    int		socketID;
    char	*bufPtr;	/* Address of buffer to place the data in. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of MSG_OOB, MSG_PEEK*/
{
    if (debugFsStubs) {
	printf("Fs_RecvStub(%d, %x, %d, %d)\n", socketID, bufPtr, bufSize,
		flags);
    }
    return Fs_RecvfromStub(socketID, bufPtr, bufSize, flags, 
			    (struct sockaddr *) NULL, (int *) NULL);
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

int
Fs_RecvfromStub(socketID, bufPtr, bufSize, flags, senderPtr, 
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
    int			value;

    if (debugFsStubs) {
	printf("Fs_RecvfromStub(%d, %x, %d, %d, %x, %x)\n",
		socketID, bufPtr, bufSize, flags, senderPtr, senderLenPtr);
    }
    intPtr = (int *)(Mach_UserStack() - sizeof(int));
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
	    Mach_SetErrno(EINVAL);
	    return -1;
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
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }

    status = Fs_ReadStub(socketID, bufSize, bufPtr, intPtr);
    if (status != SUCCESS) {
	DebugMsg(status, "recvfrom (read)");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    (void)Vm_CopyIn(sizeof(int), (Address)intPtr, (Address) &value);

    /*
     * If the caller wants the address of the sender, ask the server for it.
     */
    if (senderPtr != (struct sockaddr *) NULL) {
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FROM,
				  0, (Address) NULL,
				  senderLen, (Address) senderPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvfrom (ioctl get_remote)");
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    return value;
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

int
Fs_RecvmsgStub(socketID, msgPtr, flags)
    int			socketID;	/* Sokect to read data from. */
    struct msghdr	*msgPtr;	/* I/O vector of buffers to store the
					 * data. */
    int			flags;		/* Type of operatrion: OR of 
					 *  MSG_OOB, MSG_PEEK*/
{
    int			*intPtr;
    ReturnStatus	status;
    struct msghdr	msg;

    if (debugFsStubs) {
	printf("Fs_RecvmsgStub(%d, %x, %d)\n", socketID, msgPtr, flags);
    }
    if (msgPtr == (struct msghdr *) NULL) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    status = Vm_CopyIn(sizeof(msg), (Address)msgPtr, (Address)&msg);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }

    if (msg.msg_name != (Address) NULL) {
	if (msg.msg_namelen != sizeof(struct sockaddr_in)) {
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
    }

    if (msg.msg_iovlen > MSG_MAXIOVLEN) {
	    Mach_SetErrno(EINVAL);
	    return -1;
    }

    intPtr = (int *)(Mach_UserStack() - sizeof(int));
    /*
     * If there are flags, ship them to the server.
     */
    if (flags != 0) {
	status = Vm_CopyOut(sizeof(flags), (Address)&flags, (Address)intPtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
	status = Fs_IOControlStub(socketID, IOC_NET_RECV_FLAGS,
			    sizeof(flags), (Address)intPtr,
			    0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "recvmsg (ioctl recv_flags)");
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    status = Fs_ReadvStub(socketID, msg.msg_iov, msg.msg_iovlen);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
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
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    return 0;
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

int
Fs_SendStub(socketID, bufPtr, bufSize, flags)
    int		socketID;	/* Socket to send data on. */
    char	*bufPtr;	/* Address of buffer to send. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of 
				 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
{

    return Fs_SendtoStub(socketID, bufPtr, bufSize, flags,
	                 (struct sockaddr *)NULL, 0);
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

int
Fs_SendtoStub(socketID, bufPtr, bufSize, flags, destPtr, destLen)
    int		socketID;	/* Socket to send data on. */
    char	*bufPtr;	/* Address of buffer to send. */
    int		bufSize;	/* Size of *bufPtr. */
    int		flags;		/* Type of operatrion: OR of 
				 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
    struct sockaddr	*destPtr;	/* Destination to send the data to. */
    int			destLen;	/* Size of *destPtr.  */
{
    ReturnStatus	status;

    if (debugFsStubs) {
	printf("Fs_SendtoStub(%d, %x, %d, %d)\n", socketID, bufPtr,
	    bufSize, flags);
    }
    /*
     * If either the flags or a destination are given, send them to the server.
     */
    if ((flags != 0) || (destPtr != (struct sockaddr *) NULL)) {
	Net_SendInfo	sendInfo;

	if (destPtr != (struct sockaddr *) NULL) {
	    if (destLen != sizeof(struct sockaddr_in)) {
		Mach_SetErrno(EINVAL);
		return -1;
	    }
	    sendInfo.addressValid = TRUE;
	    status = Vm_CopyIn(sizeof(Net_InetSocketAddr), (Address)destPtr,
		      (Address)&sendInfo.address.inet);
	    if (status != SUCCESS) {
		Mach_SetErrno(EFAULT);
		return -1;
	    }
	} else {
	    sendInfo.addressValid = FALSE;
	}
	sendInfo.flags = flags;
	status = IOControlStub(socketID, IOC_NET_SEND_INFO,
			sizeof(sendInfo), (Address) &sendInfo,
			0, (Address) NULL);
	if (status != SUCCESS) {
	    DebugMsg(status, "sendto (ioctl)");
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    return Fs_NewWriteStub(socketID, bufPtr, bufSize);
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

int
Fs_SendmsgStub(socketID, msgPtr, flags)
    int			socketID;	/* Socket to send data on. */
    struct msghdr	*msgPtr;	/* I/O vector of buffers containing
					 * data to send. */
    int			flags;		/* Type of operatrion: OR of 
					 *  MSG_OOB, MSG_PEEK, MSG_DONTROUTE. */
{
    ReturnStatus	status;
    struct msghdr	msg;

    if (debugFsStubs) {
	printf("Fs_SendmsgStub(%d, %x, %d)\n", socketID, msgPtr, flags);
    }

    if (msgPtr == (struct msghdr *) NULL) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    status = Vm_CopyIn(sizeof(msg), (Address)msgPtr, (Address)&msg);
    if (status != SUCCESS) {
	Mach_SetErrno(EFAULT);
	return -1;
    }
    if (msg.msg_iovlen > MSG_MAXIOVLEN) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    if ((flags != 0) || (msg.msg_name != (Address) NULL)) {
	Net_SendInfo	sendInfo;

	if (msg.msg_name != (Address) NULL) {
	    if (msg.msg_namelen != sizeof(struct sockaddr_in)) {
		Mach_SetErrno(EINVAL);
		return -1;
	    }
	    sendInfo.addressValid = TRUE;
	    status = Vm_CopyIn(sizeof(Net_InetSocketAddr),
			       (Address)msg.msg_name,
		               (Address)&sendInfo.address.inet);
	    if (status != SUCCESS) {
		Mach_SetErrno(EFAULT);
		return -1;
	    }
	} else {
	    sendInfo.addressValid = FALSE;
	}
	sendInfo.flags = flags;
	status = IOControlStub(socketID, IOC_NET_SEND_INFO,
			    sizeof(sendInfo), (Address) &sendInfo,
			    0, (Address) NULL);

	if (status != SUCCESS) {
	    DebugMsg(status, "sendmsg (ioctl)");
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }

    return Fs_WritevStub(socketID, msg.msg_iov, msg.msg_iovlen);
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
Fs_ShutdownStub(socketID, action)
    int		socketID;	/* Socket to shut down. */
    int		action;		/* 0 -> disallow further recvs, 
				 * 1 -> disallow further sends,
				 * 2 -> combination of above. */
{
    ReturnStatus	status;

    if (debugFsStubs) {
	printf("Fs_ShutdownStub(%d, %d)\n", socketID, action);
    }
    status = IOControlStub(socketID, IOC_NET_SHUTDOWN, 
			sizeof(action), (Address) &action,
			0, (Address) NULL);

    if (status != SUCCESS) {
	DebugMsg(status, "shutdown");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

static ReturnStatus
IOControlStub(streamID, command, inBufSize, inBuffer,
			   outBufSize, outBuffer)
    int 	streamID;	/* User's handle on the stream */
    int 	command;	/* IOControl command */
    int 	inBufSize;	/* Size of inBuffer */
    Address 	inBuffer;	/* Command specific input parameters */
    int 	outBufSize;	/* Size of outBuffer */
    Address 	outBuffer;	/* Command specific output parameters */
{
    Proc_ControlBlock *procPtr;
    Fs_ProcessState *fsPtr;
    Fs_Stream 	 *streamPtr;
    register ReturnStatus status = SUCCESS;
    Fs_IOCParam ioctl;
    Fs_IOReply reply;

    if (debugFsStubs) {
	printf("IOControlStub(%d, %x)\n", streamID, command);
    }

    /*
     * Get a stream pointer.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }

    if (!Fsutil_HandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }

    ioctl.command = command;
    ioctl.format = mach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;

    /*
     * Fast path for non-generic I/O controls to pseudo-devices.
     * We don't copy in/out the user's parameter blocks because the
     * pseudo-device code does direct cross-address-space copy later.
     * We also skip the check against large parameter blocks so arbitrary
     * amounts of data can be fed to and from a pseudo-device.
     */
    if ((streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM) &&
	(command > IOC_GENERIC_LIMIT)) {
	ioctl.inBufSize = inBufSize;
	ioctl.inBuffer = inBuffer;
	ioctl.outBufSize = outBufSize;  
	ioctl.outBuffer = outBuffer;
	ioctl.flags = FS_USER_IN|FS_USER_OUT;
	return(Fs_IOControl(streamPtr, &ioctl, &reply));
    }

    if (inBufSize > IOC_MAX_BYTES || outBufSize > IOC_MAX_BYTES) {
	return(SYS_INVALID_ARG);
    }
    ioctl.flags = 0;	/* We'll copy buffer's into/out of the kernel */


    if ((outBufSize > 0) && (outBuffer != (Address)0) &&
			    (outBuffer != (Address)NIL)){
	ioctl.outBuffer = outBuffer;
	ioctl.outBufSize = outBufSize;
    } else {
	ioctl.outBuffer = (Address)NIL;
	ioctl.outBufSize = outBufSize = 0;
    }
    if ((inBufSize > 0) && (inBuffer != (Address)0) &&
			   (inBuffer != (Address)NIL)) {
	ioctl.inBuffer  = inBuffer;
	ioctl.inBufSize = inBufSize;
    } else {
	ioctl.inBuffer = (Address)NIL;
	ioctl.inBufSize = 0;
    }
    status = Fs_IOControl(streamPtr, &ioctl, &reply);
    if (status == SUCCESS) {
	/*
	 * Post process the set/get flags stuff because the close-on-exec
	 * flag is not kept down at the stream level, but up along
	 * with the streamID.
	 */
	fsPtr = procPtr->fsPtr;
	switch(command) {

	    case IOC_GET_FLAGS: {
		if (fsPtr->streamFlags[streamID] & FS_CLOSE_ON_EXEC) {
		    *(int *)ioctl.outBuffer |= IOC_CLOSE_ON_EXEC;
		}
		break;
	    }

	    case IOC_SET_BITS:
	    case IOC_SET_FLAGS: {
		int flags;
		flags = *(int *)ioctl.inBuffer;

		if (flags & IOC_CLOSE_ON_EXEC) {
		    fsPtr->streamFlags[streamID] |= FS_CLOSE_ON_EXEC;
		} else if (command == IOC_SET_FLAGS) {
		    fsPtr->streamFlags[streamID] &= ~FS_CLOSE_ON_EXEC;
		}
		break;
	    }

	    case IOC_CLEAR_BITS:{
		int flags;
		flags = *(int *)ioctl.inBuffer;
		if (flags & IOC_CLOSE_ON_EXEC) {
		    fsPtr->streamFlags[streamID] &= ~FS_CLOSE_ON_EXEC;
		}
		break;
	    }
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_SocketStub --
 *
 *	The stub for the "socket" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SocketStub(domain, type, protocol)
    int domain;         /* Type of communications domain */
    int type;           /* Type of socket: SOCK_STREAM, SOCK_DGRAM, SOCK_RAW. */
    int protocol;       /* Specific protocol to use. */
{
    int        		status = 0;
    int                 streamID;
    int			err;
    Fs_IOReply          reply;

    if (debugFsStubs) {
	printf("Fs_SocketStub(%d, %d, %d)\n",domain, type, protocol);
    }
    if (domain != PF_INET) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    if (!gotHostName) {
	if (*sysHostName == '\0') {
	    printf("*** host name uninitialized\n");
	} else {
	    gotHostName = TRUE;
	}
        (void) sprintf(streamDevice, INET_STREAM_NAME_FORMAT, sysHostName);
        (void) sprintf(dgramDevice, INET_DGRAM_NAME_FORMAT, sysHostName);
        (void) sprintf(rawDevice, INET_RAW_NAME_FORMAT, sysHostName);
    }

    if (type == SOCK_STREAM) {
        streamID = Fs_NewOpenStubInt(streamDevice, FS_READ|FS_WRITE, 0666);
    } else if (type == SOCK_DGRAM) {
        streamID = Fs_NewOpenStubInt(dgramDevice, FS_READ|FS_WRITE, 0666);
    } else if (type == SOCK_RAW) {
        streamID = Fs_NewOpenStubInt(rawDevice, FS_READ|FS_WRITE, 0666);
    } else {
	Mach_SetErrno(EINVAL);
        return -1;
    }
    if (streamID<0) {
	printf("Fs_SocketStub: open failure\n");
	return streamID;
    }

    if (protocol != 0) {
        status = Fs_IoctlInt(streamID, IOC_NET_SET_PROTOCOL,
		sizeof(int), (Address)&protocol, 0, NULL, &reply, &err);
    }

    if (status < 0) {
	printf("Fs_SocketStub: ioctl failure\n");
	return -1;
    }
    return streamID;
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

int
Fs_ConnectStub(socketID, namePtr, nameLen)
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

	usp = (Address)Mach_UserStack();
	intPtr = (int *)(usp - sizeof(int));
	userOptionsArray = 
		    (int *)(Mach_UserStack() - 3 * sizeof(int));

	/*
	 * The connection didn't immediately complete, so wait if
	 * we're blocking or return EWOULDBLOCK if we're non-blocking.
	 */
	status = Fs_IOControlStub(socketID, IOC_GET_FLAGS, 
			0, (Address) NULL,
			sizeof(flags), (Address) intPtr);

	if (status != SUCCESS) {
	    DebugMsg(status, "connect (ioctl)");
	    UNIXRETURN(status);
	}
	(void)Vm_CopyIn(sizeof(flags), (Address)intPtr, (Address)&flags);

	if (flags & IOC_NON_BLOCKING) {
	    UNIXRETURN(FS_WOULD_BLOCK);
	} 

	status = Wait(socketID, FALSE, &time_OneMinute);

	if (status == FS_TIMEOUT) {
	    DebugMsg(status, "connect (select)");
	    UNIXRETURN(status);
	}

	/*
	 * See if the connection successfully completed.
	 */
	optionsArray[0] = SOL_SOCKET;
	optionsArray[1] = SO_ERROR;
	status = Vm_CopyOut(2 * sizeof(int), (Address)optionsArray, 
			    (Address)userOptionsArray);
	if (status != SUCCESS) {
	    UNIXRETURN(status);
	}
	status = Fs_IOControlStub(socketID, IOC_NET_GET_OPTION,
			sizeof(optionsArray), (Address) userOptionsArray, 
			sizeof(int), (Address) intPtr);
	if (status != SUCCESS) {
	    DebugMsg(status, "connect (getsockopt)");
	    UNIXRETURN(status);
	}
	(void)Vm_CopyIn(sizeof(int), (Address)intPtr, (Address)&status);
    } else if (status != SUCCESS) {
	DebugMsg(status, "connect");
    }
    UNIXRETURN(status);
}

/*ARGSUSED*/
/*
 *----------------------------------------------------------------------
 *
 * Fs_SocketpairStub --
 *
 *      The stub for the "socketpair" Unix system call.
 *
 * Results:
 *      Returns -1 on failure.
 *
 * Side effects:
 *      Side effects associated with the system call.
 *     
 *
 *----------------------------------------------------------------------
 */
int
Fs_SocketpairStub(d, type, protocol, sv)
    int d, type, protocol;
    int sv[2];
{
    printf("socketpair is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
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

    if (debugFsStubs) {
	printf("Wait(%d, %d, %x)\n", socketID, readSelect, timeOutPtr);
    }
    usp = Mach_UserStack();
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
	    status = Fs_SelectStub(socketID+1, userTimeOutPtr, userMaskPtr,
				(int *) NULL, (int *) NULL, numReadyPtr);
	} else {
	    status = Fs_SelectStub(socketID+1, userTimeOutPtr, (int *) NULL, 
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
	    status = Fs_SelectStub(socketID+1, userTimeOutPtr, userMaskPtr, 
				(int *) NULL, (int *) NULL, numReadyPtr);
	} else {
	    status = Fs_SelectStub(socketID+1, userTimeOutPtr, (int *) NULL,
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
