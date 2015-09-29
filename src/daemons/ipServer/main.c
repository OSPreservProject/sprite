/* 
 * main.c --
 *	
 *	This file contains the initialization and file-system interface
 *	routines for the Sprite Internet Protocol Server.
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/main.c,v 1.23 92/06/16 13:02:30 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "inet.h"
#include "ipServer.h"
#include "stat.h"
#include "socket.h"
#include "route.h"
#include "raw.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"

#include "netInet.h"
#include "dev/pdev.h"
#include "spriteTime.h"
#include "proc.h"

#include "option.h"
#include <stdio.h>
#include "fs.h"
#include "sys/file.h"

/*
 * Command line options.
 */

Boolean ips_Debug = FALSE;
static Boolean detach = TRUE;
static char defaultConfigFile[100] = "/etc/ipServer.config";
static char *configFile = defaultConfigFile;
static char *ipAddress = (char *) NULL;

static Option optionArray[] = {
    {OPT_TRUE,   "d", (Address)&ips_Debug, "Turn on debugging output"},
    {OPT_STRING, "c", (Address)&configFile, "Name of configuration file"},
    {OPT_STRING, "i", (Address)&ipAddress, "IP address"},
    {OPT_FALSE,  "b", (Address)&detach, "Don't run in background"},
};
static int numOptions = sizeof(optionArray) / sizeof(Option);

char		myHostName[100];

typedef struct Sock_PrivInfo *PrivPtr;

static void 	PdevRequestHandler();
static void 	PdevControlHandler();
static int 	PrintMemStats();
static int 	ToggleDebug();
static int 	PrintInfoAndDebug();
#ifdef TEST_DISCONNECT
static int ToggleDisconnect();
int	ips_Disconnect = 0;
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      The main program for the Internet Protocol Server. The
 *      initialization routines of the important modules are called to
 *      set up their data structures. The socket pseudo-devices and the
 *      network device are opened and the FS dispatcher is called to
 *      process events from them.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Files are opened. Signal handlers are established.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int		argc;
    char	**argv;
{
    int			streamID;
    Sig_Action		sigAction;
    Proc_PID		pid;
    char		deviceName[100];
#ifdef DEBUG
    extern int		Sys_PrintNumCalls();
#endif DEBUG
    extern int		memAllowFreeingFree;

    /*
     * Turn on checks for duplicate frees in the memory allocator.
     */

    memAllowFreeingFree = 0;

    IP_MemBin();
    TCP_MemBin();
    Mem_Bin(sizeof(Sock_BufDataInfo));
    Mem_Bin(sizeof(Sock_SharedInfo));
    Mem_Bin(sizeof(Sock_PrivInfo));
    Mem_SetPrintProc(fprintf, (ClientData)stderr);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    /*
     * Print debugging information when the signals are caught. 
     */

#define SetHandler(sig, routine) \
    sigAction.action = SIG_HANDLE_ACTION; \
    sigAction.handler = routine; \
    sigAction.sigHoldMask = 1 << (sig-1); \
    if (Sig_SetAction(sig, &sigAction, NULL) != SUCCESS) { \
	(void) fprintf(stderr, "main: can't set signal handler %d\n", sig); \
    }

    SetHandler(25, ToggleDebug);
    SetHandler(26, Stat_PrintInfo);
    SetHandler(27, Sock_PrintInfo);
#ifdef TEST_DISCONNECT
    SetHandler(28, ToggleDisconnect);
#else
    SetHandler(28, PrintMemStats);
#endif
#ifdef DEBUG
    SetHandler(29, Sys_PrintNumCalls);
#endif DEBUG
    SetHandler(30, PrintInfoAndDebug);


    setlinebuf(stderr);
#ifndef sun4
    {
	char	*Version();
	(void) fprintf(stderr, "Sprite Internet Protocol Server: %s\n", 
			Version());
    }
#endif sun4

    if (gethostname(myHostName, sizeof(myHostName)) != 0) {
	panic("Can't find my hostname\n");
    }

    Rte_AddressInit(myHostName, configFile, ipAddress);

    Sock_Init();

    Sys_GetTimeOfDay(&stats.startTime, (int *) NULL, (Boolean *) NULL);

    IP_Init();
    ICMP_Init();
    UDP_Init();
    TCP_Init(stats.startTime.seconds);


    /*
     * Open the pseudo-devices that correspond to the stream (TCP), 
     * datagram (UDP) and raw (IP) socket abstractions. The raw socket
     * is opened with 0600 permission so only set-root-uid programs can 
     * access it.
     */

    (void) umask(0);

    (void) sprintf(deviceName, INET_STREAM_NAME_FORMAT, myHostName);
    streamID = open(deviceName, O_RDONLY|O_CREAT|O_MASTER, 0666);
    if (streamID < 0) {
	perror(deviceName);
	exit(1);
    }
    Fs_EventHandlerCreate(streamID, FS_READABLE, PdevControlHandler, 
			(ClientData) TCP_PROTO_INDEX);


    (void) sprintf(deviceName, INET_DGRAM_NAME_FORMAT, myHostName);
    streamID = open(deviceName, O_RDONLY|O_CREAT|O_MASTER, 0666);
    if (streamID < 0) {
	perror(deviceName);
	exit(1);
    }
    Fs_EventHandlerCreate(streamID, FS_READABLE, PdevControlHandler,
			(ClientData) UDP_PROTO_INDEX);


    (void) sprintf(deviceName, INET_RAW_NAME_FORMAT, myHostName);
    streamID = open(deviceName, O_RDONLY|O_CREAT|O_MASTER, 0600);
    if (streamID < 0) {
	perror(deviceName);
	exit(1);
    }
    Fs_EventHandlerCreate(streamID, FS_READABLE, PdevControlHandler, 
			(ClientData) RAW_PROTO_INDEX);


    pid = getpid(); 
    (void) fprintf(stderr, "PID = %x\n", pid);


    /*
     * Now that everything is set up, run in the background if the
     * debug flag is not set.
     */
    if (!ips_Debug && detach) {
	Proc_Detach(0);
    }


    /*
     * Everything's initialized. Start processing events from the
     * the socket pseudo-devices and the network device.
     */
    while (TRUE) {
	stats.misc.dispatchLoop++;
	Fs_Dispatch();
	IP_DelayedOutput();
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PdevControlHandler --
 *
 *	This routine handles the creation of new pseudo-device connections 
 *	to the server from client programs. It is called from the
 *	FS dispatcher whenever one of "stream/datagram/raw" control streams
 *	become readable. A socket is created for the connection. Further
 *	requests on the new stream will be handled by PdevRequestHandler()
 *	(or UdpRequestHandler)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new socket is created for the connection.
 *
 *----------------------------------------------------------------------
 */

static void
PdevControlHandler(clientData, streamID, eventMask)
    ClientData	clientData;	/* Protocol index # for the socket type. */
    int		streamID;	/* ID of stream that became readable, writable,
				 * or has an exception condition ready. */
    int		eventMask;	/* Mask to show what event happened. A combo of
				 * Should be just FS_READABLE. */
{
    ReturnStatus	status;
    Pdev_Notify		notify;
    Pdev_SetBufArgs	bufArgs;
    SockPdevState	*sockPdevPtr;
    int			amount;
    int			state;
    int			reqBufSize;
    int			bigWrites;
    int			writeBehind;
    void		(*handler)();

    if (!(eventMask & FS_READABLE)) {
	panic("PdevControlHandler: bad event mask %x\n", eventMask);
    }

    /*
     * Read the notify message from the control stream.
     */

    amount = read(streamID, (Address)&notify, sizeof(notify));
    if (amount < 0) {
	perror("PdevControlHandler: read notify");
	exit(1);
    } else if (amount != sizeof(notify)) {
	panic("PdevControlHandler: short read (%d) of the notify\n", amount);
	exit(1);
    } else if (notify.magic != PDEV_NOTIFY_MAGIC) {
	panic("PdevControlHandler: bad magic # (%x) in the notify\n", 
		notify.magic);
	exit(1);
    }

    sockPdevPtr = (SockPdevState *)malloc(sizeof(SockPdevState));
    switch ((int)clientData) {
	case TCP_PROTO_INDEX:
	    sockPdevPtr->reqBufSize = TCP_REQUEST_BUF_SIZE;
	    sockPdevPtr->protoIndex = TCP_PROTO_INDEX;
	    bigWrites = TRUE;
	    writeBehind = FALSE;
	    break;
	case UDP_PROTO_INDEX:
	    sockPdevPtr->reqBufSize = UDP_REQUEST_BUF_SIZE;
	    sockPdevPtr->protoIndex = UDP_PROTO_INDEX;
	    bigWrites = FALSE;
	    writeBehind = UDP_WRITE_BEHIND;
	    break;
	case RAW_PROTO_INDEX:
	    sockPdevPtr->reqBufSize = RAW_REQUEST_BUF_SIZE;
	    sockPdevPtr->protoIndex = RAW_PROTO_INDEX;
	    bigWrites = FALSE;
	    writeBehind = FALSE;
	    break;
    }
    /*
     * Allocate the pseudo-device request buffer, leaving room at the
     * front to create packet headers, and adding enough room to account
     * for the pdev message header in front of the data.
     * Make sure the request buffer is a multiple of the word size.
     * (The kernel should do this.)
     */
    sockPdevPtr->reqBufSize += sizeof(Pdev_Request) + sizeof(int);
    sockPdevPtr->reqBufSize &= ~(sizeof(int) - 1);
    sockPdevPtr->requestBuf = (Address) malloc(sockPdevPtr->reqBufSize +
						IPS_ROOM_FOR_HEADERS);
    bufArgs.requestBufAddr = sockPdevPtr->requestBuf + IPS_ROOM_FOR_HEADERS;
    bufArgs.requestBufSize = sockPdevPtr->reqBufSize;
    bufArgs.readBufAddr = (Address) NULL;
    bufArgs.readBufSize = 0;
    status = Fs_IOControl(notify.newStreamID, IOC_PDEV_SET_BUF,
		sizeof(bufArgs), (Address) &bufArgs, 
		0, (Address) NULL);
    if (status != SUCCESS){
	Stat_PrintMsg(status, "PdevControlHandler: ioc set buf");
	exit(1);
    }
    status = Fs_IOControl(notify.newStreamID, IOC_PDEV_BIG_WRITES,
		sizeof(state), (Address) &bigWrites, 
		0, (Address) NULL);
    if (status != SUCCESS){
	Stat_PrintMsg(status, "PdevControlHandler: ioc big write");
	exit(1);
    }
    status = Fs_IOControl(notify.newStreamID, IOC_PDEV_WRITE_BEHIND,
		sizeof(state), (Address) &writeBehind, 
		0, (Address) NULL);
    if (status != SUCCESS){
	Stat_PrintMsg(status, "PdevControlHandler: ioc write behind");
	exit(1);
    }

    Fs_EventHandlerCreate(notify.newStreamID, FS_READABLE, PdevRequestHandler,
			    (ClientData)sockPdevPtr);
}


static int origFirstByte;

/*
 *----------------------------------------------------------------------
 *
 * PdevRequestHandler --
 *
 *	Handle an request from a client accessing a socket pseudo-device.
 *	We read a short message off the request stream that indicates
 *	where in the buffer the next request message(s) are.  Depending
 *	on the client's operation, we may allocate a return data block
 *	that gets filled in by the socket routine.  We reply with a
 *	return status for the client's system call and, optionally, the
 *	return data block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The operation-specific routines may cause side-effects.
 *
 *----------------------------------------------------------------------
 */

static void
PdevRequestHandler(clientData, streamID, eventMask)
    ClientData	clientData;	/* Token passed the Sock_ routines to identify
				 * a socket. */
    int		streamID;	/* ID of stream that became readable, writable
				 * or has an exception condition ready. */
    int		eventMask;	/* Mask to show what event happened.
				 * Must be FS_READABLE. */
{
    ReturnStatus	status;
    register Pdev_Request	*requestPtr;
    Pdev_Reply	reply;
    Pdev_BufPtrs	bufPtrs;
    int			amount;
    Address		requestData;
    int			messageSize;
    int			replySize;
    Pdev_Op		operation;
    /*
     * All requests, except PDEV_OPEN, get passed the socket private pointer
     * as the client data from Fs_Dispatch.
     */
    Sock_PrivInfo	*privPtr = (Sock_PrivInfo *)clientData;


    if (!(eventMask & FS_READABLE)) {
	panic("PdevRequestHandler: bad event mask %x\n", 
				eventMask);
    }

    /*
     * Read the short request header that tells us where in the buffer
     * the next request message is.  Check size and magic number.
     */

    amount = read(streamID, (Address)&bufPtrs, sizeof(bufPtrs));
    if (amount  != sizeof(bufPtrs)) {
	perror("PdevRequestHandler: get ptr ioctl");
	exit(1);
    } else if (bufPtrs.magic != PDEV_BUF_PTR_MAGIC) {
	panic("PdevControlHandler: bad magic # (%x) in the bufPtrs\n", 
		bufPtrs.magic);
	exit(1);
    } 

    origFirstByte = bufPtrs.requestFirstByte;
    while (bufPtrs.requestFirstByte < bufPtrs.requestLastByte) {
	requestPtr = (Pdev_Request *)
			&bufPtrs.requestAddr[bufPtrs.requestFirstByte];
	if (requestPtr->hdr.magic != PDEV_REQUEST_MAGIC) {
	    printf("PdevControlHandler: bad magic # (%x) comitting hari-kari\n",
		    requestPtr->hdr.magic);
#ifdef notdef
	    /*
	     * This will put us more cleanly into the debugger than a panic
	     * will.
	     */
	    *(int *)0 = 100;
	    exit(1);
#endif
	    /*
	     * Hack to attempt to get around the server lieing to us by
	     * telling us that there is more data than there really is.
	     */
	    bufPtrs.requestFirstByte = bufPtrs.requestLastByte + 1;
	    break;
	} 
	requestData = (Address)((Address)requestPtr + sizeof(Pdev_Request));
	reply.replySize = 0;
	reply.replyBuf = (Address) NULL;

	/*
	 * Decode the operation and call a routine for the operation.
	 * The operation handler may overwrite the request header with
	 * a packet header, so we are careful not to use anything from
	 * the request header after calling the service procedure.
	 */
	messageSize = requestPtr->hdr.messageSize;
	replySize = requestPtr->hdr.replySize;
	operation = requestPtr->hdr.operation;
	switch (operation) {


	    case PDEV_OPEN: {
		    /*
		     * Create a new socket structure. If the open was 
		     * successful, create an event handler to handle future 
		     * requests on the stream.  Sock_Open returns a token 
		     * for the new socket, which is associated with the event 
		     * handler for the socket's stream.
		     *
		     * The first arg. to Sock_Open ("clientData") is a pointer
		     * to some state about the pseudo-device setup.
		     */

		    reply.status = Sock_Open((SockPdevState *) clientData, 
				    requestPtr->hdr.operation == PDEV_OPEN,
				    streamID,
				    requestPtr->param.open.flags, 
				    requestPtr->param.open.pid, 
				    requestPtr->param.open.hostID, 
				    requestPtr->param.open.uid, 
				    -1, /* no more client ID */
				    &privPtr);
		    free((char *)clientData);
		    if (reply.status == SUCCESS) {
			/*
			 * Save the address of the socket data structure
			 * with the stream so subsequent requests can
			 * access it.
			 */
			(void)Fs_EventHandlerChangeData(streamID,
							(ClientData)privPtr);
			reply.selectBits = Sock_Select(privPtr, TRUE); 
		    } else {
			reply.selectBits = 0;
		    }
		    PdevReply(streamID, &reply, "open");
		}
		break;


	    case PDEV_CLOSE:
		(void) Sock_Close(privPtr);
		Fs_EventHandlerDestroy(streamID);
		reply.status == SUCCESS;
		PdevReply(streamID, &reply, "close");
		(void) close(streamID);
		break;


	    /*
	     * The client wants data from the server.
	     */
	    case PDEV_READ:
		if (privPtr->sharePtr->protoIndex == UDP_PROTO_INDEX) {
		    /*
		     * Special case optimized UDP read request handler.
		     * It sends its own reply
		     */
		    UDP_ReadRequest(privPtr, requestPtr, streamID);
		} else {
		    /*
		     * Allocate a reply buffer to hold the data from the socket.
		     */
		    reply.replyBuf = 
				malloc((unsigned int)requestPtr->hdr.replySize);
		    reply.status = Sock_Read( privPtr, 
				requestPtr->hdr.replySize, reply.replyBuf,
				&reply.replySize);
		    /*
		     * Make sure we don't supply more than what was requested.
		     */
		    if (reply.replySize > replySize) {
			(void) fprintf(stderr, 
			  "PdevRequestHandler: gave too much data for READ.\n");
		    }
		    reply.selectBits = Sock_Select(privPtr, TRUE); 
		    PdevReply(streamID, &reply, "read");
		    if (reply.replyBuf != (Address) NULL) {
			free(reply.replyBuf);
		    }
		}
		break;


	    /*
	     * The client wants to send data to the server.
	     */
	    case PDEV_WRITE_ASYNC:
	    case PDEV_WRITE:
		if (ips_Debug) {
		    (void) fprintf(stderr, "Pdev_write: %d\n", 
				requestPtr->hdr.requestSize);
		}
		amount = 0;
		reply.replySize = sizeof(int);
		reply.replyBuf = (Address)&amount;
		if (requestPtr->hdr.requestSize < 0) {
		    reply.status = GEN_INVALID_ARG;
		} else {
		    reply.status = Sock_Write( privPtr, 
					requestPtr->param.write.procID,
					requestPtr->hdr.requestSize,
					requestData,
					&amount);
		}
		if (operation == PDEV_WRITE) {
		    reply.selectBits = Sock_Select(privPtr, TRUE); 
		    PdevReply(streamID, &reply, "write");
		}
		break;

	    case PDEV_IOCTL: {
		    Address outBuffer;

		    if (requestPtr->hdr.requestSize < 0) {
			reply.status = GEN_INVALID_ARG;
			outBuffer = (Address) NULL;
		    } else {
			outBuffer = malloc((unsigned int)requestPtr->hdr.replySize);

			reply.status = Sock_IOControl(privPtr,
					requestPtr->param.ioctl.command,
				        requestPtr->param.ioctl.uid, 
					requestPtr->hdr.requestSize,
					requestData,
					requestPtr->hdr.replySize, outBuffer);
		    }
		    reply.selectBits = Sock_Select(privPtr, TRUE); 

		    if (reply.status == SUCCESS) {
			reply.replySize = replySize;
			reply.replyBuf = outBuffer;
		    } else {
			reply.replySize = 0;
			reply.replyBuf = (Address) NULL;
		    }
		    PdevReply(streamID, &reply, "ioctl");
		    if (outBuffer != (Address) NULL) {
			free(outBuffer);
		    }
		}
		break;

	    default:
		reply.status = FAILURE;
		PdevReply(streamID, &reply, "??");
		(void) fprintf(stderr, 
			"PdevRequestHandler: unknown operation %d\n",
				requestPtr->hdr.operation);
		break;
	}
	bufPtrs.requestFirstByte += messageSize;
    }
    if (operation != PDEV_CLOSE) {
	status = Fs_IOControl(streamID, IOC_PDEV_SET_PTRS,
		sizeof(bufPtrs), (Address) &bufPtrs, 
		0, (Address) NULL);
	if (status != SUCCESS){
	    Stat_PrintMsg(status, "set ptrs");
	    exit(1);
	}
    }
}

PdevReply(streamID, replyPtr, caller)
    int			streamID;
    Pdev_Reply	*replyPtr;
    char		*caller;
{
    ReturnStatus	status;

    replyPtr->magic = PDEV_REPLY_MAGIC;
    replyPtr->signal = 0;
    replyPtr->code = 0;
    status = Fs_IOControl(streamID, IOC_PDEV_REPLY,
		sizeof(*replyPtr), (Address) replyPtr, 
		0, (Address) NULL);
    if (status != SUCCESS && status != SYS_ARG_NOACCESS){
	Stat_PrintMsg(status, caller);
	exit(1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintInfoAndDebug --
 *
 *	Calls the Stat_PrintInfo and Sock_PrintInfo routines and then
 *	turns on debugging. This routine is useful when debugging
 *	is normally off and something goes wrong.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is printed and debugging turned on.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
PrintInfoAndDebug(sigNum, sigCode)
    int		sigNum;
    int		sigCode;
{
    Stat_PrintInfo(0,0);
    Sock_PrintInfo(0,0);
    ips_Debug = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintMemStats --
 *
 *	Prints a summary of the memory allocator statistics.
 *	Can be called as a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Statistics are printed on the standard error stream.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
PrintMemStats(sigNum, sigCode)
    int		sigNum;		/* Ignored. */
    int		sigCode;	/* Ignored. */
{
    Mem_PrintStats();
    Mem_PrintInUse();
}


/*
 *----------------------------------------------------------------------
 *
 * ToggleDebug --
 *
 *	Toggles the state of the ips_Debug flag.
 *	Can be called as a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The debug flag is toggled.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
ToggleDebug(sigNum, sigCode)
    int		sigNum;		/* Ignored. */
    int		sigCode;	/* Ignored. */
{
    ips_Debug = !ips_Debug;
    (void) fprintf(stderr, "*****  Debugging now  %s  *****\n",
		ips_Debug ? "ON" : "off");
}

#ifdef TEST_DISCONNECT

/*
 *----------------------------------------------------------------------
 *
 * ToggleDisconnect --
 *
 *	Toggles the state of the ips_Disconnect flag.
 *	Can be called as a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The  flag is toggled.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
ToggleDisconnect(sigNum, sigCode)
    int		sigNum;		/* Ignored. */
    int		sigCode;	/* Ignored. */
{
    ips_Disconnect = !ips_Disconnect;
    (void) fprintf(stderr, "*****  Disconnect now  %s  *****\n",
		ips_Disconnect ? "ON" : "off");
}

#endif


/*
 *----------------------------------------------------------------------
 *
 * IPS_GetTimestamp --
 *
 *	Returns the number of milliseconds since midnight.
 *
 * Results:
 *	The # of milliseconds since midnight.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
IPS_GetTimestamp()
{
    Time	time;
    Time_Parts	parts;

    Sys_GetTimeOfDay(&time, (int *) NULL, (Boolean *) NULL);

    Time_ToParts(time.seconds, FALSE, &parts);
    return((((parts.hours * 3600) + (parts.minutes * 60) + 
	     parts.seconds) * 1000) + (time.microseconds/ONE_MILLISECOND));
}


/*
 *----------------------------------------------------------------------
 *
 * IPS_InitPacket --
 *
 *	Initializes a packet by allocating memory for the data and
 *	packet headers and setting up the pointer to the data part
 *	of the packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *----------------------------------------------------------------------
 */

void
IPS_InitPacket(dataLen, packetPtr)
    int		dataLen;
    register IPS_Packet	*packetPtr;
{
    packetPtr->dataLen = dataLen;
    packetPtr->totalLen = dataLen + IPS_ROOM_FOR_HEADERS;
    packetPtr->base = malloc((unsigned int) packetPtr->totalLen + 4);
    packetPtr->dbase = packetPtr->base + 2;
    packetPtr->data = packetPtr->dbase + IPS_ROOM_FOR_HEADERS;
    if (((unsigned)packetPtr->data & 0x3) != 0) {
	packetPtr->data += 2;
	packetPtr->totalLen += 2;
    }
}

f1(iPtr)
    int	*iPtr;
{
    printf("i: %x = %x\n", iPtr, *iPtr);
}
