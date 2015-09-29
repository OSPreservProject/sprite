/* 
 * kmsg.c --
 *
 *	Program that sends debugging messages to a Sprite kernel.
 *	Can be used to put a kernel into the debugger, query it,
 *	and resume from the debugger.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/kmsg/RCS/kmsg.c,v 1.7 92/06/10 17:19:19 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <errno.h>
#include <host.h>
#include <net.h>
#include <option.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <kernel/dbg.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sgtty.h>

/*
 * Library imports:
 */

extern void panic();

/*
 * Message buffers.
 */
static Dbg_Msg	msg;
static int	msgSize;
#define	REPLY_BUFFER_SIZE	4096
static	char	replyBuffer[REPLY_BUFFER_SIZE];
static	char	requestBuffer[DBG_MAX_REQUEST_SIZE];
/*
 * The decstation currently supports both kdbx and kgdb.   Since we
 * are using the kgdb interface we must start the message number 
 * at 0x40000000 so the debugger stub uses the right interface.
 */
#if defined(ds3100) || defined(ds5000)
static	int	msgNum = 0x40000000;
#else
static	int	msgNum = 0;
#endif

extern void	RecvReply();

static	struct sockaddr_in	remote;
int				kdbxTimeout = 1;
static	int			netSocket;

/*
 *----------------------------------------------------------------------
 *
 * CreateSocket --
 *
 *	Creates a UDP socket connected to the Sprite host's kernel 
 *	debugger port.
 *
 * Results:
 *	The stream ID of the socket.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
CreateSocket(spriteHostName)
    char	*spriteHostName;
{
    int			socketID;
    struct hostent 	*hostPtr;

    hostPtr = gethostbyname(spriteHostName);
    if (hostPtr == (struct hostent *) NULL) {
	(void) fprintf(stderr, "CreateSocket: unknown host \"%s\"\n",
		spriteHostName);
	exit(1);
    }
    if (hostPtr->h_addrtype != AF_INET) {
	(void) fprintf(stderr, "CreateSocket: bad address type for host %s\n", 
		spriteHostName);
	exit(2);
    }

    socketID = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketID < 0) {
	perror("CreateSocket: socket");
	exit(3);
    }

    bzero((char *) &remote, sizeof(remote));
    bcopy((char *) hostPtr->h_addr, (char *) &remote.sin_addr,
	    hostPtr->h_length);
    remote.sin_port = htons(DBG_UDP_PORT);
    remote.sin_family = AF_INET;

    if (connect(socketID, (struct sockaddr *) &remote, sizeof(remote)) < 0) {
	perror("CreateSocket: connect");
	exit(4);
    }

    return(socketID);
}

/*
 * ----------------------------------------------------------------------------
 *
 *  SendRequest --
 *
 *     Send a request message to the kernel.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 * ----------------------------------------------------------------------------
 */
static void
SendRequest(numBytes)
    int	numBytes;
{
    Dbg_Opcode	opcode;

    msgSize = numBytes;
    msgNum++;
    *(int *)requestBuffer = msgNum;
    bcopy((char *) &msg, requestBuffer + 4, numBytes);
    if (write(netSocket, requestBuffer, numBytes + 4) < numBytes + 4) {
	panic("SendRequest: Couldn't write to the kernel socket\n");
	return;
    }
    opcode = (Dbg_Opcode) msg.opcode;
    if (opcode == DBG_DETACH) {
	int	dummy;
	/*
	 * Wait for explicit acknowledgments of these packets.
	 */
	RecvReply(opcode, 4, (char *) &dummy, (int *) NULL, 1);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 *  RecvReply --
 *
 *     Receive a reply from the kernel.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
RecvReply(opcode, numBytes, destAddr, readStatusPtr, timeout)
    Dbg_Opcode	opcode;
    int		numBytes;
    char	*destAddr;
    int		*readStatusPtr;
    int		timeout;
{
    int		status;
    int		readMask;
    struct	timeval	interval;
    int		bytesRead;


    if (numBytes + 8 > REPLY_BUFFER_SIZE) {
	panic("numBytes <%d> > REPLY_BUFFER_SIZE <%d>\n",
		    numBytes + 8, REPLY_BUFFER_SIZE);
    }
    interval.tv_sec = kdbxTimeout;
    interval.tv_usec = 0;
    do {
	if (timeout) {
	    int	numTimeouts;

	    numTimeouts = 0;
	    /*
	     * Loop timing out and sending packets until a new packet
	     * has arrived.
	     */
	    do {
		readMask = 1 << netSocket;
		status = select(32, &readMask, (int *) NULL,
			(int *) NULL, &interval);
		if (status == 1) {
		    break;
		} else if (status == -1) {
		    panic("RecvReply: Couldn't select on socket.\n");
		} else if (status == 0) {
		    SendRequest(msgSize);
		    numTimeouts++;
		    if (numTimeouts % 10 == 0) {
			(void) fprintf(stderr, "Timing out and resending\n");
			(void) fflush(stderr);
		    }
		}
	    } while (1);
	}
	if (opcode == DBG_GET_VERSION_STRING) {
	    /*
	     * Returning the version string returns a variable length packet.
	     */
	    bytesRead = read(netSocket, replyBuffer, numBytes);
	    if (bytesRead < 0) {
		perror("RecvReply: Error reading socket.");
		panic("exiting");
	    }
	    (void) strcpy(destAddr, (char *)(replyBuffer + 4));
	    return;
	} else {
	    /*
	     * Normal request so just read in the message which includes
	     * the message number.
	     */
	    bytesRead = read(netSocket, replyBuffer, numBytes + 4);
	    if (bytesRead < 0) {
		panic("RecvReply: Error reading socket (2).");
	    }
	    /*
	     * Check message number before size because it could be
	     * an old packet.
	     */
	    if (*(int *)replyBuffer != msgNum) {
		continue;
	    }
	    if (bytesRead != numBytes + 4) {
		(void) printf("RecvReply: Short read (2): op=%d exp=%d read=%d",
			opcode, numBytes + 4, bytesRead);
	    }
	    if (*(int *)replyBuffer != msgNum) {
		continue;
	    }
	    bcopy(replyBuffer + 4, destAddr, numBytes);
	    return;
	}
    } while (1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * SendCommand --
 *
 *     Write the command over to the kernel.  
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
void
SendCommand(opcode, srcAddr, destAddr, numBytes)
    Dbg_Opcode	opcode;		/* Which command */
    char	*srcAddr;	/* Where to read data from */
    char	*destAddr;	/* Where to write data to */
    int		numBytes;	/* The number of bytes to read or write */
{
    msg.opcode = (short) opcode;

    switch (opcode) {
	case DBG_GET_STOP_INFO:
	    SendRequest(sizeof(msg.opcode));
	    RecvReply(opcode, numBytes, destAddr, (int *) NULL, 1);
	    break;
	case DBG_DETACH:
	    msg.data.pc = *(int *) srcAddr;
	    SendRequest(sizeof(msg.opcode) + sizeof(msg.data.pc));
	    break;
	case DBG_GET_VERSION_STRING:
	    SendRequest(sizeof(msg.opcode));
	    RecvReply(opcode, numBytes, destAddr, (int *) NULL, 1);
	    break;
	case DBG_REBOOT:
	    if (numBytes > 0) {
		(void) strcpy(msg.data.reboot.string, (char *)srcAddr);
	    }
	    msg.data.reboot.stringLength = numBytes;
	    SendRequest(sizeof(msg.opcode) +
			sizeof(msg.data.reboot.stringLength) +
			msg.data.reboot.stringLength);
	    break;
	default:
	    (void) printf("Unknown opcode %d\n", opcode);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SendDebug --
 *
 *	Given a host name, this procedure sends a command to that host
 *	that causes it to enter the debugger.
 *
 * Results:
 *	0 if everything went well, 1 if there was some sort of error
 *	(in this case an error message is printed).
 *
 * Side effects:
 *	The given host will enter the debugger, if the host exists and
 *	is running Sprite.
 *
 *----------------------------------------------------------------------
 */

int
SendDebug(hostName)
    char *hostName;		/* Name of Sprite host. */
{
    Host_Entry *host;
    int streamID, amtWritten;

    /*
     * A debug packet contains the name of the host that is issuing
     * the "enter debugger" command.  Changed 'int nameLen' to be
     * 'char nameLenChar[]' in the packet struct to remove padding
     * bytes which were confusing the kernel. JMS
     */

#define MAX_NAME_LEN	100
    struct {
	Net_EtherHdr	etherHdr;
	char 		nameLenChar[sizeof(int)];
	char		name[MAX_NAME_LEN];
    } packet;

    int nameLen;
    int i;

    /*
     * Fill in our name in the debug packet.
     */

    if (gethostname(packet.name, MAX_NAME_LEN-1) != 0) {
	(void) fprintf(stderr, "Couldn't find my host name: %s\n",
		strerror(errno));
	return 1;
    }
    packet.name[MAX_NAME_LEN-1] = 0;
    nameLen = strlen(packet.name);
    bcopy((char *) &nameLen, packet.nameLenChar, sizeof(int));
    if (strcmp(packet.name, hostName) == 0) {
	(void) fprintf(stderr, "Can't send a debug packet to yourself.\n");
	return 1;
    }

    /*
     * Set up the ethernet packet header. The source address is filled
     * in by the kernel.
     */
    host = Host_ByName(hostName);
    if (host == (Host_Entry *)NULL) {
	(void) fprintf(stderr, "Unknown host: %s\n", hostName);
	return 1;
    }
    for (i = 0; i < host->numNets; i++) {
	if (host->nets[i].netAddr.type == NET_ADDRESS_ETHER) {
	    bcopy((char *) &host->nets[i].netAddr.address.ether,
		(char *) &packet.etherHdr.destination, 
		sizeof(Net_EtherAddress));
	    break;
	}
    }
    packet.etherHdr.type = htons(NET_ETHER_SPRITE_DEBUG);

    streamID = open("/dev/etherSpriteDebug", O_WRONLY, 0666);
    if (streamID < 0) {
	(void) fprintf(stderr, "Couldn't open raw ethernet device: %s\n",
		strerror(errno));
	return 1;
    }

    amtWritten = write(streamID, (char *) &packet, sizeof(packet));
    if (amtWritten < 0) {
	(void) fprintf(stderr, "Error sending debug packet: %s\n",
		strerror(errno));
	return 1;
    }
    if (amtWritten != sizeof(packet)) {
	(void) fprintf(stderr, "Short write for packet: %d\n", amtWritten);
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "kmsg".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes things to happen in a target machine.  See the man page
 *	for details.
 *
 *----------------------------------------------------------------------
 */

int	getVersion = 0;
int	debug = 0;
int	detach = 0;
char	*versionString = (char *)NULL;
int	reboot = 0;
char	*rebootString = (char *)NULL;

Option optionArray[] = {
    {OPT_TRUE, "c", (char *) &detach,
	"Continue the kernel"},
    {OPT_TRUE, "d", (char *) &debug,
	"Force kernel into the debugger"},
    {OPT_TRUE, "r", (char *) &reboot,
	"Reboot the machine using the empty string"},
    {OPT_STRING, "R", (char *) &rebootString,
	"Reboot the machine using the given string"},
    {OPT_STRING, "s", (char *) &versionString,
	"Version string to compare version of kernel to (implies -v)"},
    {OPT_TRUE, "v", (char *) &getVersion,
	"Print out the version of the kernel."},
};

main(argc, argv)
    int		argc;
    char	**argv;
{
    StopInfo	stopInfo;
    char	buffer[100];

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) {
	(void) fprintf(stderr, "Usage:  %s [options] hostname\n", argv[0]);
	exit(1);
    }
    if (debug) {
	if (SendDebug(argv[1]) != 0) {
	    exit(1);
	}
    }
    netSocket = CreateSocket(argv[1]);
    if (detach) {
	SendCommand(DBG_GET_STOP_INFO, (char *)NULL, (char *)&stopInfo,
			sizeof(stopInfo));
#ifdef sun3
	SendCommand(DBG_DETACH, (char *) &stopInfo.pc, (char *)NULL,
			sizeof(stopInfo.pc));
#else
	SendCommand(DBG_DETACH, (char *) &stopInfo.regs.pc, (char *)NULL,
			sizeof(stopInfo.regs.pc));
#endif /* sun3 */
    }
    if (getVersion || versionString != (char *)NULL) {
	SendCommand(DBG_GET_VERSION_STRING, (char *)0, buffer, 100);
	(void) printf("%s\n", buffer);
	if (versionString != (char *)NULL) {
	    exit(strcmp(buffer, versionString));
	}
    }
    if (rebootString != (char *)NULL) {
	SendCommand(DBG_REBOOT, rebootString, (char *)NULL,
		    strlen(rebootString));
    } else if (reboot) {
	SendCommand(DBG_REBOOT, (char *)NULL, (char *)NULL, 0);
    }

    exit(0);
}
