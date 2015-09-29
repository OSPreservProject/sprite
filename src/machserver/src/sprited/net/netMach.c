/* 
 * netMach.c --
 *
 *	Glue code between Sprite's Net_Interface (above) and Mach's device 
 *	interface (below).
 *	
 *	Note: some of the routines below are stubs.  They don't do 
 *	anything, and they don't need locking.  I put lock statements in 
 *	anyway, so that when the routines are fixed to actually do 
 *	something, nobody forgets to put the locking in.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/net/RCS/netMach.c,v 1.7 92/07/06 15:03:40 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

/* 
 * The packet filter support in Mach is, or at least used to be, pretty
 * limited.  In particular, the packet filter is not given access to the
 * actual packet header.  Also, there are bugs in the way the information
 * is passed to the user program.  Hopefully this will be fixed before we
 * have to worry about running on a network other than ethernet.
 */
#define DUMB_PACKET_FILTER	1

#include <ckalloc.h>
#include <mach.h>
#include <mach_error.h>
#include <device/device.h>
#include <device/device_request.h>
#include <sprite.h>
#include <status.h>
#include <stdio.h>
#include <string.h>

#ifdef DUMB_PACKET_FILTER
#include <netEther.h>
#endif

#include <dev.h>
#include <net.h>
#include <proc.h>
#include <sync.h>
#include <utils.h>
#include <utilsMach.h>

/* 
 * The receive ports for all network devices are put into a port set.
 */
static mach_port_t inputSet = MACH_PORT_NULL;

#define MAX_FILTER_SIZE	100	/* maximum number of words in the packet 
				 * filter */
/* 
 * Define the offsets into the array passed to the packet filter.
 * XXX This is expected to change soon to be more like standard (e.g., 
 * Ultrix, SunOS) packet filters.
 */
#define PACKET_LENGTH_OFFSET	0 /* packet length */
#define PACKET_TYPE_OFFSET	1 /* packet type */
#define PACKET_BODY_OFFSET	2 /* beginning of packet body (no header) */

/* temporary numbers for inline output packets: */
static int packets2048 = 0;	/* packets 301-2048 bytes */
static int packets300 = 0;	/* packets 201-300 bytes */
static int packets200 = 0;	/* packets 101-200 bytes */
static int smallPackets = 0;	/* packets <= 100 bytes */

/* 
 * Temporary declarations, until the Mach header files get it right.
 */

extern mach_msg_return_t mach_msg _ARGS_((mach_msg_header_t *msg,
			mach_msg_option_t option, mach_msg_size_t send_size,
			mach_msg_size_t rcv_size, mach_port_t rcv_name,
			mach_msg_timeout_t timeout, mach_port_t notify));

/* Forward references: */

static void ByteSwap _ARGS_((unsigned int *intPtr, unsigned int numInts));
static int MakeFilter _ARGS_((int maxWords, filter_t filterArray[]));
static void ReadPortSet _ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * NetMachInit --
 *
 *	Initialization for netMach code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the port set that incoming packets are read from and 
 *	creates a kernel process to monitor it.
 *
 *----------------------------------------------------------------------
 */

void
NetMachInit()
{
    kern_return_t kernStatus;

    kernStatus = mach_port_allocate(mach_task_self(),
				    MACH_PORT_RIGHT_PORT_SET,
				    &inputSet);
    if (kernStatus != KERN_SUCCESS) {
	panic("NetMachInit: couldn't allocate receive port set: %s\n",
	      mach_error_string(kernStatus));
    }

    (void)Proc_NewProc((Address) UTILSMACH_MAGIC_CAST ReadPortSet,
		       (Address)0, PROC_KERNEL,
		       FALSE, (Proc_PID *)NULL, "network input process");
}


/*
 *----------------------------------------------------------------------
 *
 * PrintBytes --
 *
 *	Print out an array of bytes.
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
PrintBytes(start, howMany)
    unsigned char *start;
    int howMany;
{
    int i;

    for (i = 0; i < howMany; i++, start++) {
	printf("%x ", *start);
	if (i != 1 && (i-1) % 20 == 0) {
	    printf("\n");
	}
    }
    printf("\n");
}


/*
 *----------------------------------------------------------------------
 *
 * NetInitDevice --
 *
 *	Initialization routine for network devices.
 *
 * Results:
 *	Returns SUCCESS or a mapped Mach failure code.
 *
 * Side effects:
 *	Opens the given network interface and adds it to the network input 
 *	port set.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetInitDevice(interPtr)
    Net_Interface	*interPtr; 	/* Network interface to init. */
{
    kern_return_t kernStatus;
    ReturnStatus status = SUCCESS;
    char machName[512];		/* name that Mach uses for the interface */
    struct net_status info;	/* information about the interface */
    mach_msg_type_number_t numInts; /* number of ints in status record */
    filter_t packetFilter[MAX_FILTER_SIZE]; /* packet filter */
    int packetFilterSize;	/* number of words used to define packet 
				 * filter */
    
    Sync_GetLock(&interPtr->mutex);

    if (interPtr->machName != NULL) {
	strcpy(machName, interPtr->machName);
    } else {
	sprintf(machName, "%s%d", interPtr->name, interPtr->number);
    }
    kernStatus = device_open(dev_ServerPort, 0, machName,
			     &interPtr->requestPort);
    if (kernStatus != D_SUCCESS) {
	printf("NetInitDevice: can't open Mach device %s: %s\n",
	       machName, mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }

    /*
     * Get status and address information for the interface from Mach.
     */

    numInts = NET_STATUS_COUNT;
    kernStatus = device_get_status(interPtr->requestPort, NET_STATUS,
				   (dev_status_t)&info, &numInts);
    if (kernStatus != D_SUCCESS) {
	printf("NetInitDevice: couldn't get status for Mach device %s: %s\n",
	       machName, mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }
    if (numInts < NET_STATUS_COUNT) {
	printf("%s: status record for Mach device %s too small (%d words).\n",
	       "NetInitDevice", machName, numInts);
	status = MACH_VERS_MISMATCH;
	goto bailOut;
    }
    numInts = sizeof(Net_Address)/sizeof(int);
    kernStatus = device_get_status(interPtr->requestPort, NET_ADDRESS,
				   (dev_status_t)&interPtr->
				       netAddress[NET_PROTO_RAW],
				   &numInts);
    if (kernStatus != D_SUCCESS) {
	printf("%s: couldn't get network address for Mach device %s: %s\n",
	       "NetInitDevice", machName,
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }
    /* 
     * As of MK63, Mach returns the network address padded out to an
     * integral number of integers, which have been byte-swapped on
     * Little-Endian machines.  Put the address back into a byte array in
     * the correct order.
     */
    ByteSwap((unsigned int *)(&interPtr->netAddress[NET_PROTO_RAW]),
	     numInts);

    status = (interPtr->init)(interPtr, &info);
    if (status != SUCCESS) {
	goto bailOut;
    }
    

    /*
     * Allocate a receive port for the interface, and set it up to receive 
     * Sprite packets.
     */
    
    kernStatus = mach_port_allocate_name(mach_task_self(),
					 MACH_PORT_RIGHT_RECEIVE,
					 (mach_port_t)interPtr);
    if (kernStatus != KERN_SUCCESS) {
	printf("NetInitDevice: can't allocate name (0x%x): %s\n",
	       interPtr, mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }
    interPtr->recvPort = (mach_port_t)interPtr;
    kernStatus = mach_port_move_member(mach_task_self(), interPtr->recvPort,
				       inputSet);
    if (kernStatus != KERN_SUCCESS) {
	panic("%s: couldn't add port for Mach device %s to input set: %s\n",
	      "NetInitDevice", machName,
	      mach_error_string(kernStatus));
    }
    (void)mach_port_set_qlimit(mach_task_self(), interPtr->recvPort,
			       MACH_PORT_QLIMIT_MAX);
    packetFilterSize = MakeFilter(MAX_FILTER_SIZE, packetFilter);
    /* 
     * We tried giving the filter a high priority (NET_HI_PRI+10), but 
     * this seemed to break the UX server's networking support.  So, try a 
     * priority higher than the UX server's filter, but not real high.
     */
    kernStatus = device_set_filter(interPtr->requestPort,
				   interPtr->recvPort, MACH_MSG_TYPE_MAKE_SEND,
				   10, packetFilter,
				   packetFilterSize);
    if (kernStatus != D_SUCCESS) {
	printf("NetInitDevice: couldn't set filter for Mach device %s: %s\n",
	       machName, mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }

    /* 
     * Initialize the instrumentation for the interface.
     */
    interPtr->interfaceData = (ClientData)ckalloc(sizeof(Net_GenStats));
    bzero(interPtr->interfaceData, sizeof(Net_GenStats));

 bailOut:
    if (status != SUCCESS) {
	if (interPtr->requestPort != MACH_PORT_NULL) {
	    (void)device_close(interPtr->requestPort);
	    (void)mach_port_deallocate(mach_task_self(),
				       interPtr->requestPort);
	    interPtr->requestPort = MACH_PORT_NULL;
	}
	if (interPtr->recvPort != MACH_PORT_NULL) {
	    (void)mach_port_deallocate(mach_task_self(),
				       interPtr->recvPort);
	    interPtr->recvPort = MACH_PORT_NULL;
	}
    }
    Sync_Unlock(&interPtr->mutex);
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * MakeFilter --
 *
 *	Set up a packet filter for Sprite.
 *
 * Results:
 *	Returns the number of words used in the filter array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
MakeFilter(maxWords, filterArray)
    int maxWords;		/* maximum number of elements in the array */
    filter_t filterArray[];	/* filter array to fill in */
{
    int numWords;

    numWords = 0;

    /* 
     * Succeed immediately if regular Sprite packet.
     */
    filterArray[numWords++] = NETF_PUSHWORD + PACKET_TYPE_OFFSET;
    filterArray[numWords++] = NETF_PUSHLIT | NETF_COR;
    filterArray[numWords++] = Net_HostToNetShort(NET_ETHER_SPRITE);

    /* 
     * Succeed immediately if regular ARP packet.
     */
    filterArray[numWords++] = NETF_PUSHWORD + PACKET_TYPE_OFFSET;
    filterArray[numWords++] = NETF_PUSHLIT | NETF_COR;
    filterArray[numWords++] = Net_HostToNetShort(NET_ETHER_ARP);

    /* 
     * Succeed immediately if reverse ARP packet.
     */
    filterArray[numWords++] = NETF_PUSHWORD + PACKET_TYPE_OFFSET;
    filterArray[numWords++] = NETF_PUSHLIT | NETF_COR;
    filterArray[numWords++] = Net_HostToNetShort(NET_ETHER_REVARP);

    /* 
     * Succeed if Sprite debug packet.  Otherwise discard the packet.
     */
    filterArray[numWords++] = NETF_PUSHWORD + PACKET_TYPE_OFFSET;
    filterArray[numWords++] = NETF_PUSHLIT | NETF_EQ;
    filterArray[numWords++] = Net_HostToNetShort(NET_ETHER_SPRITE_DEBUG);

    if (numWords > maxWords) {
	panic("MakeFilter: not enough room for packet filter.");
    }
    return numWords;
}


/*
 *----------------------------------------------------------------------
 *
 * NetMachOutput --
 *
 *	Output a network packet or (for loopback packets) hand it back to 
 *	the input side.
 *
 * Results:
 *	Returns SUCCESS if the packet was sent; returned both directly and 
 *	via statusPtr (if non-nil).  NET_PACKET_TOO_BIG means that the 
 *	packet was too big for the given interface.  
 *	DEV_EARLY_CMD_COMPLETION is returned for (synchronous) short 
 *	writes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetMachOutput(interPtr, asynch, hdrPtr, scatterGatherPtr, scatterGatherLength,
	      statusPtr)
    Net_Interface *interPtr;	/* The network interface. */
    Boolean asynch;		/* send asynchronously? (XXX unused) */
    Address hdrPtr;		/* Packet header. */
    Net_ScatterGather *scatterGatherPtr; /* Data portion of the packet. */
    int	scatterGatherLength;	/* elements in data portion gather array. */
    ReturnStatus *statusPtr;	/* Status from sending packet.*/
{
    int bodyLength;		/* length of data portion */
    int i;			/* index into scatter/gather array */
    int headerLength;		/* number of bytes in packet header */
    Boolean isLoopback;		/* packet is destined for the sender */
    ReturnStatus status = SUCCESS;
    int bytesWritten;		/* number of bytes actually sent */
    Net_GenStats *statsPtr;	/* stats for the interface */
    kern_return_t kernStatus;

    Sync_GetLock(&interPtr->mutex);
    statsPtr = (Net_GenStats *)interPtr->interfaceData;

    bodyLength = 0;
    for (i = 0; i < scatterGatherLength; i++) {
	bodyLength += scatterGatherPtr[i].length;
    }

    if (bodyLength > interPtr->maxBytes) {
	printf("NetMachOutput: packet for %s too big (%d bytes)\n",
	       interPtr->msgName, bodyLength);
	status = NET_PACKET_TOO_BIG;
	goto exit;
    }

    /* 
     * Pull the pieces of the packet together into one single buffer.
     */
    (interPtr->mergePacket)(interPtr, hdrPtr, scatterGatherPtr,
			    scatterGatherLength, &headerLength, &isLoopback);

    /* 
     * If this host is the destination of the packet (loopback), pass the 
     * packet back to the input routine.
     */
    if (isLoopback) {
	Net_Input(interPtr, interPtr->buffer, headerLength,
		  interPtr->buffer + headerLength, bodyLength);
	status = SUCCESS;
	goto exit;
    }

    /* 
     * Temporary instrumentation.
     */
    if (bodyLength + headerLength > 300) {
	packets2048++;
    } else if (bodyLength + headerLength > 200) {
	packets300++;
    } else if (bodyLength + headerLength > 100) {
	packets200++;
    } else {
	smallPackets++;
    }

    statsPtr->packetsOut++;
    statsPtr->bytesOut += bodyLength + headerLength;
    if (bodyLength + headerLength > IO_INBAND_MAX) {
	kernStatus = device_write_request(interPtr->requestPort,
					  MACH_PORT_NULL, 0, 0,
					  interPtr->buffer,
					  bodyLength + headerLength);
    } else {
	statsPtr->inlinePacketsOut++;
	kernStatus = device_write_request_inband(interPtr->requestPort,
						 MACH_PORT_NULL, 0, 0,
						 interPtr->buffer,
						 bodyLength + headerLength);
    }
    bytesWritten = bodyLength + headerLength;

    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }

 exit:
    Sync_Unlock(&interPtr->mutex);
    if (statusPtr != (ReturnStatus *)NIL) {
	*statusPtr = status;
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * NetMachReset --
 *
 *	Reset the network device, by bringing the interface down and then 
 *	up again.
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
NetMachReset(interPtr)
    Net_Interface *interPtr;	/* the interface to reset */
{
    Sync_GetLock(&interPtr->mutex);
    printf("NetMachReset: reset for %s not implemented yet.\n",
	   interPtr->msgName);
    Sync_Unlock(&interPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * NetMachIoControl --
 *
 *	Perform ioctls for the network device.  Right now we don't support 
 *	any.
 *
 * Results:
 *	DEV_INVALID_ARG
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
NetMachIoControl(interPtr, ioctlPtr, replyPtr)
    Net_Interface *interPtr;	/* Interface on which to perform ioctl. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    Sync_GetLock(&interPtr->mutex);
    Sync_Unlock(&interPtr->mutex);

    return DEV_INVALID_ARG;
}


/*
 *----------------------------------------------------------------------
 *
 * NetMachGetStats --
 *
 *	Return the statistics for the interface.
 *	XXX Should also ask the kernel for device stats and use them to 
 *	fill in statistics.
 *
 * Results:
 *	Returns a status code.  Fills in the given stats structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetMachGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    Sync_GetLock(&interPtr->mutex);
    bzero((Address)statPtr, sizeof(Net_Stats));
    bcopy(interPtr->interfaceData, &statPtr->generic, sizeof(Net_GenStats));
    Sync_Unlock(&interPtr->mutex);

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * ReadPortSet --
 *
 *	Get incoming packets from Mach and pass then up to the protocol 
 *	code.
 *
 * Results:
 *	None.  Never returns.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ReadPortSet()
{
    struct net_rcv_msg msg;	/* message containing packet */
    Net_Interface *interPtr;	/* interface the packet came in on */
    kern_return_t kernStatus;
    Address headerPtr;		/* start of packet header */
    int headerLength;		/* bytes in packet header */
    Address bodyPtr;		/* start of packet body */
    int bodyLength;		/* bytes in packet body */
    Net_GenStats *statsPtr;	/* instrumentation for the interface */

    for (;;) {
	kernStatus = mach_msg(&msg.msg_hdr, MACH_RCV_MSG,
			      0, sizeof(struct net_rcv_msg), inputSet,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	if (kernStatus != MACH_MSG_SUCCESS) {
	    printf("ReadPortSet failed: %s\n",
		   mach_error_string(kernStatus));
	    Proc_Exit(0);
	}
	interPtr = (Net_Interface *)msg.msg_hdr.msgh_local_port;
	if (!(interPtr->flags & NET_IFLAGS_RUNNING)) {
	    printf("ReadPortSet: ignoring packet from %s.\n",
		   interPtr->msgName);
	    continue;
	}

	statsPtr = (Net_GenStats *)interPtr->interfaceData;
	statsPtr->packetsIn++;
	headerPtr = msg.header;

#ifndef DUMB_PACKET_FILTER
	/* 
	 * XXX The message header is currently described as having 64 
	 * bytes.  This is a lie resulting from a design flaw in the 
	 * kernel/driver interface.
	 */
	headerLength = msg.header_type.msgt_number;
#else
	headerLength = sizeof(Net_EtherHdr);
#endif

#ifdef DUMB_PACKET_FILTER
	/* 
	 * Currently the packet filter doesn't have access to the source or 
	 * destination host of the packet.  For some reason Mach "reads" 
	 * back in a copy of a broadcast packet.  Sprite doesn't expect 
	 * this and may do the wrong thing, so drop those packets on the 
	 * floor. 
	 */
	if (NET_ETHER_COMPARE_PTR(&((Net_EtherHdr *)headerPtr)->source,
			   &(interPtr->netAddress[NET_PROTO_RAW].ether))) {
	    statsPtr->selfBroadcastIn++;
	    continue;
	}
#endif
	
	bodyPtr = msg.packet + sizeof(struct packet_header);
	bodyLength = msg.packet_type.msgt_number -
	    sizeof(struct packet_header);
	statsPtr->bytesIn += headerLength + bodyLength;
	Net_Input(interPtr, headerPtr, headerLength, bodyPtr, bodyLength);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ByteSwap --
 *
 *	Convert an integerized network address back to a byte array.
 *
 * Results:
 *	On Little-Endian machines, the network address is treated as an
 *	integer array and byte-swapped.  No-op for Big-Endian machines.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ByteSwap(intPtr, numInts)
    unsigned int *intPtr;	/* start of network address, as an array of 
				 * ints */ 
    unsigned int numInts;	/* number of elements in the array */
{
    int i;

    for (i = 0; i < numInts; i++, intPtr++) {
	*intPtr = Net_HostToNetInt(*intPtr);
    }
}
