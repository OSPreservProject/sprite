/* 
 * devNet.c --
 *
 *	Device interface to the network.  The routines here implement
 *	filesystem devices for the various ethernet protocols.  Input
 *	queues of received packets are maintainted here, although the
 *	device has to be open before packets are queued.  There is a
 *	different queue for each different ethernet protocol, and the
 *	device unit number is used to identify the protocol.
 *
 *	TODO: this needs to be fixed to understand more than one network
 *	interface.  It seems that at open time the correct interface
 *	should be chosen.  Also, some interface-data needs to be passed
 *	down to the output routines so they can choose the right interface.
 *
 * Copyright 1987 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"
#include "devNet.h"
#include "user/netInet.h"
#include "stdlib.h"
#include "sync.h"
#include "fs.h"
#include "user/net.h"

Boolean devNetEtherDebug = FALSE;

/*
 * The  packets are kept in a circular queue whose length is a power of 2.
 * Stuff gets added to the queues at interrupt time so removal requires
 * synchronization with MASTER_LOCK/UNLOCK.
 */

#define PACKET_QUEUE_LEN	16
typedef struct {
    int		head;
    int		tail;
    Address	packet[PACKET_QUEUE_LEN];
    int		size[PACKET_QUEUE_LEN];
} PacketQueue;

/*
 * A circular queue is full if its tail is right in front of its
 * head (mod length). A queue is empty if its tail is its head.
 */

#define NextTail(queue)		(((queue).tail + 1) & (PACKET_QUEUE_LEN -1))
#define NextHead(queue)		(((queue).head + 1) & (PACKET_QUEUE_LEN -1))

#define QueueFull(queue)	(NextTail(queue) == (queue).head)
#define QueueEmpty(queue)       ((queue).tail == (queue).head)

/*
 * Event counters kept on a per-protocol basis.
 */
typedef struct ProtoStats {
    int		shorts;		/* Number of short packets received */
    int		drops;		/* Number of packets dropped when queue full */
} ProtoStats;

/*
 * State for the protocols.  They are linked together and scanned when
 * a packet is received.  Packets with protocols that don't match any
 * of the protocols in this list will be dropped.
 */

typedef struct ProtocolState {
    List_Links		links;
    int			protocol;	/* Ethernet protocol number */
    Boolean		open;		/* TRUE is the device is open.  Packets
					 * are only queued if it is open. */
    PacketQueue		queue;		/* Queue of received packets */
    ProtoStats		stats;		/* Event counters */
    ClientData		fsReadyToken;	/* Used for filesystem callback that
					 * notifies waiting processes that
					 * packets are here */
} ProtocolState;

/*
 * The header for the list of protocols.
 */
List_Links etherProtos;
static Boolean initList = FALSE;

/*
 * A master lock is used to synchronize access to the list of protocols.
 */
static Sync_Semaphore protoMutex = Sync_SemInitStatic("Dev:protoMutex");


/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsOpen --
 *
 *	Open an ethernet protocol device.  Protocol state is set up and
 * 	linked in to a list of active protocols.  The protocol is marked
 * 	open to enable queueing of packets for this protocol.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_FILE_BUSY	- the device was already opened.
 *
 * Side effects:
 *	Storage for the protocol state is allocated and linked into
 *	the list of active protocols.  If this has already been done,
 *	then the protocol is simply marked as open.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevNet_FsOpen(devicePtr, useFlags, data)
    Fs_Device   *devicePtr;	/* Device info, unit number == protocol */
    int 	useFlags;	/* Flags from the stream being opened */
    ClientData  data;		/* Call-back for input notification */
{
    register ProtocolState *protoPtr;
    register int i;
    register unsigned int protocol;
    ReturnStatus status = SUCCESS;

    MASTER_LOCK(&protoMutex);
    if (!initList) {
	List_Init(&etherProtos);
	Sync_SemRegister(&protoMutex);
	initList = TRUE;
    }
    /*
     * We keep the protocol number in network byte order so as to match
     * the values coming off the net.
     */
    protocol = (unsigned int) 
		Net_HostToNetShort((unsigned short) (devicePtr->unit));  
    LIST_FORALL(&etherProtos, (List_Links *)protoPtr) {
	if (protoPtr->protocol == protocol) {
	    if (protoPtr->open) {
		printf("Warning: DevNet_FsOpen: Extra open of net device");
		status = FS_FILE_BUSY;
		goto exit;
	    }
	    protoPtr->open = TRUE;
	    devicePtr->data = (ClientData)protoPtr;
	    goto found;
	}
    }

    /*
     * Protocol not in the list.  Just stick it at the end, although it
     * would be possible to sort the list...
     */

    protoPtr = (ProtocolState *)malloc(sizeof(ProtocolState));
    List_InitElement((List_Links*) protoPtr);
    List_Insert((List_Links *)protoPtr, LIST_ATREAR(&etherProtos));

    protoPtr->protocol = protocol;
    bzero((Address)&protoPtr->stats,sizeof(ProtoStats));
    /*
     * Pre-allocate buffer space for the input queue.
     * Vm_RawAlloc is used because this queue space is never recycled.
     */

    for (i=0 ; i< PACKET_QUEUE_LEN ; i++) {
	protoPtr->queue.packet[i] = (Address) Vm_RawAlloc(NET_ETHER_MAX_BYTES);
    }


found:

    protoPtr->open = TRUE;
    protoPtr->queue.head = 0;
    protoPtr->queue.tail = 0;

    /*
     * These client data fields are set up for call backs to the filesystem
     * and so we can quickly get to the protocol state on read/write etc.
     */
    protoPtr->fsReadyToken = data;
    devicePtr->data = (ClientData) protoPtr;

exit:

    if (devNetEtherDebug) {
	printf("DevNet_FsOpen: Open proto 0x%x status 0x%x\n", 
			devicePtr->unit, status);
    }

    MASTER_UNLOCK(&protoMutex);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevNetEtherHandler --
 *
 *	Dispatcher for ethernet packets.  The list of active protocols
 *	is scanned for a matching protocol.  If found, the packet is
 *	enqueued for the protocol.
 *
 *	Note: This routine is called from the ethernet interrupt routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The packet is saved in the protocols queue (if there's room and
 *	the protocol device is currently open).
 *
 *----------------------------------------------------------------------
 */

void
DevNetEtherHandler(packetPtr, size)
    Address	packetPtr;	/* Pointer to the packet in the hardware
				 * receive buffer. */
    int		size;		/* Size of the packet. */
{
    register ProtocolState *protoPtr;
    register Net_EtherHdr *etherHdrPtr = (Net_EtherHdr *)packetPtr;

    MASTER_LOCK(&protoMutex);
    if (!initList) {
	List_Init(&etherProtos);
	initList = TRUE;
    }

    if (devNetEtherDebug) {
	printf("EtherHandler 0x%x %d\n", NET_ETHER_HDR_TYPE(*etherHdrPtr),
			size);
    }
    LIST_FORALL(&etherProtos, (List_Links *)protoPtr) {
	if (NET_ETHER_HDR_TYPE(*etherHdrPtr) == protoPtr->protocol && 
				protoPtr->open) { 
	    if (QueueFull(protoPtr->queue)) {
		protoPtr->stats.drops++;
	    } else {
		bcopy(packetPtr,protoPtr->queue.packet[protoPtr->queue.tail],
		      size);
		protoPtr->queue.size[protoPtr->queue.tail] = size;
		protoPtr->queue.tail = NextTail(protoPtr->queue);
		Fs_NotifyReader(protoPtr->fsReadyToken);
	    }
	    break;
	}
    }
    MASTER_UNLOCK(&protoMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsRead --
 *
 *	Read a packet for an ethernet protocol.  This returns the first
 *	packet from the protocol's input queue, if any.  This returns
 *	data from at most 1 network packet.  If the caller's buffer is
 *	too short, the packet is truncated.
 *
 * Results:
 *	SUCCESS		- if a packet was found in the queue.  
 *	FS_WOULD_BLOCK	- no packets found.
 *
 * Side effects:
 *	Removes the first packet from the queue and copies it into
 *	the receiver's buffer.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevNet_FsRead(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device	*devicePtr;
    int		offset;		/* ignored */
    int		bufSize;	/* size of buffer */
    Address	buffer;		/* place to store data */
    int		*lenPtr;	/* max. # of chars to read before returning */
{
    ReturnStatus status;
    register ProtocolState *protoPtr;
    register Address packetPtr;
    register int size;

    protoPtr = (ProtocolState *)devicePtr->data;

    MASTER_LOCK(&protoMutex);
    if (QueueEmpty(protoPtr->queue)) {
	size = 0;
	status = FS_WOULD_BLOCK;
	if (devNetEtherDebug) {
	    printf("DevNet_FsRead: empty queue, proto 0x%x\n",
				protoPtr->protocol);
	}
    } else {
	packetPtr = protoPtr->queue.packet[protoPtr->queue.head];
	size = protoPtr->queue.size[protoPtr->queue.head];

	protoPtr->queue.head = NextHead(protoPtr->queue);

	if (size > bufSize) {
	    size = bufSize;
	}
	bcopy(packetPtr, buffer,size);

	status = SUCCESS;
	if (devNetEtherDebug) {
	    printf("DevNet_FsRead: Found packet proto 0x%x, size %d\n",
				protoPtr->protocol, size);
	}
    }

    *lenPtr = size;
    MASTER_UNLOCK(&protoMutex);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsWrite --
 *
 *	Pass a packet off to the network driver for output.  The driver
 *	maintains a transmit queue so we don't have to.  The protocol
 *	in the output packet header is verified to be the one corresponding
 *	to the device file.
 *
 * Results:
 *	SUCCESS		- the packet was transmitted.
 *	SYS_INVALID_ARG	- packet is too small or the protocol in the
 *			  buffer doesn't match the one for the device.
 *
 * Side effects:
 *	Initiates transmission of the packet.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevNet_FsWrite(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device	*devicePtr;
    int		offset;		/* Ignored */
    int		bufSize;	/* Size of buffer */
    Address	buffer;		/* Place to find data */
    int		*lenPtr;	/* Number of chars written */
{
    register Net_EtherHdr *etherHdrPtr;
    register ProtocolState *protoPtr;
    Net_ScatterGather ioVector;

    if (bufSize < sizeof(Net_EtherHdr)) {
	return(SYS_INVALID_ARG);
    }
    protoPtr = (ProtocolState *)devicePtr->data;
    etherHdrPtr = (Net_EtherHdr *)buffer;

    /*
     * Verify the protocol type in the header.  The low level driver
     * will fill in the source address for us.
     */
    if (NET_ETHER_HDR_TYPE(*etherHdrPtr) != protoPtr->protocol) {
	return(SYS_INVALID_ARG);
    }

    ioVector.bufAddr	= (Address)((int)buffer + sizeof(Net_EtherHdr));
    ioVector.length	= bufSize - sizeof(Net_EtherHdr);

    Net_EtherOutputSync(etherHdrPtr, &ioVector, 1);

    *lenPtr = bufSize;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsClose --
 *
 *	Close the device.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	The protocol state is marked as closed, and any packets on the
 *	input queue are discarded.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevNet_FsClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;	/* Device info. */
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times device still open. */
    int		writerCount;	/* Number of writers still on the device. */
{
    ProtocolState *protoPtr;

    MASTER_LOCK(&protoMutex);

    protoPtr = (ProtocolState *)devicePtr->data;
    protoPtr->open = FALSE;
    protoPtr->fsReadyToken = (ClientData)NIL;
    /*
     * Nuke the queue here?
     */

    MASTER_UNLOCK(&protoMutex);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsSelect --
 *
 *	Perform device-specific select functions on the device.
 *	Always indicates that the device is writable. Indicates the
 *	device is readable if the queue is not empty.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevNet_FsSelect(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	*devicePtr;
    int		inFlags;		/* FS_READABLE and/or FS_WRITABLE. */
    int		*outFlagsPtr;		/* The above flags if the device
    					 * is ready now. */
{
    register ProtocolState *protoPtr;

    MASTER_LOCK(&protoMutex);
    protoPtr = (ProtocolState *)devicePtr->data;

    *outFlagsPtr = 0;
    if (inFlags & FS_WRITABLE) {
	*outFlagsPtr |= FS_WRITABLE;
    }
    if (inFlags & FS_READABLE) {
	if ( ! QueueEmpty(protoPtr->queue)) {
	    *outFlagsPtr |= FS_READABLE;
	}
    }
    MASTER_UNLOCK(&protoMutex);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsIOControl --
 *
 *	Not implemented yet.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	SYS_INVALID_ARG - bad command, or wrong buffer size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevNet_FsIOControl(devicePtr, command, inBufSize, inBuffer, outBufSize,
		outBuffer)
    Fs_Device	*devicePtr;
    int		command;
    int		inBufSize;
    Address	inBuffer;
    int		outBufSize;
    Address	outBuffer;
{
    return(FAILURE);
}
