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


#include <sprite.h>
#include <stdio.h>
#include <net.h>
#include <fs.h>
#include <vm.h>
#include <fsio.h>
#include <devNet.h>
#include <user/netInet.h>
#include <stdlib.h>
#include <sync.h>
#include <fsioDevice.h>
#include <user/net.h>
#include <bstring.h>

Boolean devNetDebug = FALSE;

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
 * Event counters kept on a per-device basis.
 */
typedef struct DeviceStats {
    int		shorts;		/* Number of short packets received */
    int		drops;		/* Number of packets dropped when queue full */
} DeviceStats;

/*
 * State for the devices.  They are linked together and scanned when
 * a packet is received.  Packets with protocols that don't match any
 * of the protocols in this list will be dropped.
 */

typedef struct DeviceState {
    List_Links		links;
    Net_Interface 	*interPtr;	/* Interface associated with the 
					 * device; */
    int			protocol;	/* Protocol associated with device. */
    Boolean		kernel;		/* TRUE if kernel is using this proto */
    PacketQueue		queue;		/* Queue of received packets */
    DeviceStats		stats;		/* Event counters */
    Fs_NotifyToken	fsReadyToken;	/* Used for filesystem callback that
					 * notifies waiting processes that
					 * packets are here */
    int			(*inputProc)();	/* Used when the kernel is using the
					 * protocol. */
} DeviceState;


/*
 * Queue of all devices associated with an interface. 
 */

typedef struct DeviceQueueState {
    List_Links		links;		/* Queue of DeviceState structures
					 * for this interface. */
    Sync_Semaphore	mutex;		/* Synchronizes access to the queue. */
} DeviceQueueState;

extern void		DevNetHandler();
static ReturnStatus	ProtocolFromPacket();
static ReturnStatus	ProtocolFromDevice();
static DeviceQueueState *QueueFromInterface();

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
 *	the list of active protocols.  
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevNet_FsOpen(devicePtr, useFlags, data, flagsPtr)
    Fs_Device   *devicePtr;	/* Device info, unit number == protocol */
    int 	useFlags;	/* Flags from the stream being opened */
    Fs_NotifyToken  data;	/* Call-back for input notification */
    int		*flagsPtr;	/* OUT: Device flags. */
{
    register int i;
    unsigned int protocol;
    ReturnStatus status = SUCCESS;
    DeviceState		*statePtr;
    DeviceQueueState *deviceQueuePtr;
    Boolean		newFormat = FALSE;
    Net_NetworkType	netType;
    int			interface;
    int			unitProto = 0;
    Net_Interface	*interPtr;
    DeviceState		*itemPtr;
    int			maxSize;

    if (devNetDebug) {
	printf("DevNet_FsOpen: opening device 0x%x 0x%x\n",
	    devicePtr->type, devicePtr->unit);
    }
    statePtr = (DeviceState *) (devicePtr->data);
    if (statePtr == (DeviceState *) NIL) {
	statePtr = (DeviceState *) malloc(sizeof(DeviceState));
	bzero((char *) statePtr, sizeof(DeviceState));
	List_InitElement((List_Links *) statePtr);
    } else {
	/*
	 * If the device has a state structure then it must be in use.
	 */
	if (devNetDebug) {
	    printf("Extra open\n");
	}
	return FS_FILE_BUSY;
    }
    if (!(devicePtr->unit & DEV_NET_COMPAT_BIT)) {
	interface = 0;
	netType = NET_NETWORK_ETHER;
	protocol = devicePtr->unit;
	if (devNetDebug) {
	    printf("Device has old unit number\n");
	}
    } else {
	int	tmp = (devicePtr->unit & ~DEV_NET_COMPAT_BIT);
	netType = DEV_NET_NETTYPE_FROM_UNIT(tmp);
	interface = DEV_NET_NUMBER_FROM_UNIT(tmp);
	unitProto = DEV_NET_PROTO_FROM_UNIT(tmp);
	if (devNetDebug) {
	    printf("Device has new unit number\n");
	}
	newFormat = TRUE;
    }
    interPtr = Net_GetInterface(netType, interface);
    if (interPtr == (Net_Interface *) NIL) {
	if (devNetDebug) {
	    printf("No interface.\n");
	}
	free((char *) statePtr);
	return DEV_NO_DEVICE;
    }
    statePtr->interPtr = interPtr;
    if (newFormat) {
	status = ProtocolFromDevice(unitProto, interPtr, &protocol);
	if (status != SUCCESS) {
	    if (devNetDebug) {
		printf("No such protocol %d\n", protocol);
	    }
	    free((char *) statePtr);
	    return DEV_NO_DEVICE;
	}
    }
    deviceQueuePtr = QueueFromInterface(interPtr);
    switch(netType) {
	case NET_NETWORK_ETHER: 
	    /*
	     * We keep the protocol number in network byte order so as to match
	     * the values coming off the net.
	     */
	    protocol = (unsigned int) 
			    Net_HostToNetShort((unsigned short) protocol);
	    break;
    }
    statePtr->protocol = protocol;
    /*
     * Pre-allocate buffer space for the input queue.
     * This is probably a bad idea for anything but an ethernet since
     * packet sizes may get large.
     */
    switch(netType) {
	case NET_NETWORK_ETHER:
	    maxSize = NET_ETHER_MAX_BYTES;
	    break;
	case NET_NETWORK_ULTRA:
	    maxSize = NET_ULTRA_MAX_BYTES;
	    break;
	default:
	    printf("DevNet_FsOpen: unknown net type %d\n", netType);
	    return DEV_NO_DEVICE;
    }
    for (i=0 ; i< PACKET_QUEUE_LEN ; i++) {
	statePtr->queue.packet[i] = (Address) malloc (maxSize);
    }

    /*
     * Differentiate between user-level reads and the kernel.
     * DevNetEtherHandler will handle packets differently in
     * the two cases.  It notifies the user-level process, or
     * it calls the kernel protocol handler.
     */
    if (useFlags & FS_USER) {
	statePtr->kernel = FALSE;
	statePtr->fsReadyToken = data;
    } else {
	statePtr->kernel = TRUE;
#ifndef lint
	statePtr->inputProc = (int(*)())data;
#endif /* lint */
    }
    statePtr->queue.head = 0;
    statePtr->queue.tail = 0;
    devicePtr->data = (ClientData) statePtr;
    MASTER_LOCK(&deviceQueuePtr->mutex);
    LIST_FORALL((List_Links *) deviceQueuePtr, (List_Links *) itemPtr) {
	if (itemPtr->protocol == protocol) {
	    panic("DevNet_FsOpen: found protocol already in use.\n");
	}
    }
    List_Insert((List_Links *)statePtr, 
	LIST_ATREAR((List_Links *) deviceQueuePtr));
    Net_SetPacketHandler(interPtr, DevNetHandler);
    MASTER_UNLOCK(&deviceQueuePtr->mutex);
    if (devNetDebug) {
	printf("DevNet_FsOpen: Open proto 0x%x status 0x%x\n", 
			protocol, status);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsReopen --
 *
 *	Reopen an ethernet protocol device.  Call the regular
 *	open routine to do all the work.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_FILE_BUSY	- the device was already opened.
 *
 * Side effects:
 *	Storage for the protocol state is allocated and linked into
 *	the list of active protocols.  
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevNet_FsReopen(devicePtr, refs, writers, data, flagsPtr)
    Fs_Device   *devicePtr;	/* Device info, unit number == protocol */
    int 	refs;		/* Number of open network streams */
    int		writers;	/* Number that are open for writing */
    Fs_NotifyToken  data;	/* Call-back for input notification */
    int		*flagsPtr;	/* OUT: Device flags. */
{
    int useFlags = FS_READ;

    if (writers) {
	useFlags |= FS_WRITE;
    }
    return( DevNet_FsOpen(devicePtr, useFlags, data, flagsPtr) );
}


/*
 *----------------------------------------------------------------------
 *
 * DevNetHandler --
 *
 *	Dispatcher for packets.  The list of active protocols
 *	is scanned for a matching protocol.  If found, the packet is
 *	enqueued for the protocol.
 *
 *	Note: This routine is called from the network interrupt routine.
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

/*ARGSUSED*/
void
DevNetHandler(interPtr, size, packetPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
    int			size;		/* Size of the packet. */
    Address		packetPtr;	/* Pointer to the packet in the hardware
					 * receive buffer. */
{
    register DeviceQueueState *deviceQueuePtr;
    int	protocol;
    DeviceState		*statePtr;
    ReturnStatus	status;


    deviceQueuePtr = (DeviceQueueState *) interPtr->devNetData;
    if (deviceQueuePtr == (DeviceQueueState *) NIL) {
	return;
    }
    status = ProtocolFromPacket(packetPtr, interPtr->netType, &protocol);
    if (status != SUCCESS) {
	printf("DevNetHandler: couldn't get protocol from packet.\n");
	return;
    }
    if (devNetDebug) {
	printf("DevNetHandler %d:%d 0x%x %d\n", interPtr->netType, 
		interPtr->number, protocol, size);
    }
    MASTER_LOCK(&deviceQueuePtr->mutex);
    LIST_FORALL((List_Links *) deviceQueuePtr, (List_Links *)statePtr) {
	if (protocol == statePtr->protocol) {
	    if (QueueFull(statePtr->queue)) {
		statePtr->stats.drops++;
	    } else {
		bcopy(packetPtr,statePtr->queue.packet[statePtr->queue.tail],
		      size);
		statePtr->queue.size[statePtr->queue.tail] = size;
		statePtr->queue.tail = NextTail(statePtr->queue);
		if (statePtr->kernel) {
		    /*
		     * Indirect to a process to pull the packet off of
		     * the input queue.  We can't call the protocol handlers
		     * directly because they use malloc and free. 
		     */
		    Proc_CallFunc((void (*)())statePtr->inputProc, 
			(ClientData)statePtr, 0);
		} else {
		    Fsio_DevNotifyReader(statePtr->fsReadyToken);
		}
	    }
	    break;
	}
    }
    MASTER_UNLOCK(&deviceQueuePtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * DevNet_FsRead --
 *
 *	Read a packet from a network device. This returns the first
 *	packet from the device's input queue, if any.  This returns
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
DevNet_FsRead(devicePtr, readPtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus status;
    register DeviceState *statePtr;
    register Address packetPtr;
    register int size;
    DeviceQueueState	*deviceQueuePtr;

    statePtr = (DeviceState *) devicePtr->data;
    deviceQueuePtr = (DeviceQueueState *) statePtr->interPtr->devNetData;

    MASTER_LOCK(&deviceQueuePtr->mutex);
    if (QueueEmpty(statePtr->queue)) {
	size = 0;
	status = FS_WOULD_BLOCK;
	if (devNetDebug) {
	    printf("DevNet_FsRead: empty queue, %d:%d proto 0x%x\n",
				statePtr->interPtr->netType,
				statePtr->interPtr->number,
				statePtr->protocol);
	}
    } else {
	packetPtr = statePtr->queue.packet[statePtr->queue.head];
	size = statePtr->queue.size[statePtr->queue.head];

	statePtr->queue.head = NextHead(statePtr->queue);

	if (size > readPtr->length) {
	    size = readPtr->length;
	}
	bcopy(packetPtr, readPtr->buffer, size);

	status = SUCCESS;
	if (devNetDebug) {
	    printf("DevNet_FsRead: Found packet %d:%d proto 0x%x, size %d\n",
				statePtr->interPtr->netType,
				statePtr->interPtr->number,
				statePtr->protocol, size);
	}
    }

    replyPtr->length = size;
    MASTER_UNLOCK(&deviceQueuePtr->mutex);
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
DevNet_FsWrite(devicePtr, writePtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
    register DeviceState *statePtr;
    Net_ScatterGather ioVector;
    Net_Interface	*interPtr;
    int			dataSize;
    int			protocol;
    ReturnStatus	status;

    statePtr = (DeviceState *) devicePtr->data;
    interPtr = statePtr->interPtr;
    dataSize = writePtr->length - net_NetworkHeaderSize[interPtr->netType];
    if (devNetDebug) {
	printf(
    "DevNet_FsWrite: Writing packet %d:%d proto 0x%x, size %d (%d)\n",
	    statePtr->interPtr->netType, statePtr->interPtr->number,
	    statePtr->protocol, writePtr->length, dataSize);
    }
    if (dataSize < interPtr->minBytes ||
	dataSize >  interPtr->maxBytes) {
	if (devNetDebug) {
	    printf("DevNet_FsWrite: bad dataSize %d, maxBytes %d, minBytes\n",
		dataSize, interPtr->maxBytes, interPtr->minBytes);
	}
	return(SYS_INVALID_ARG);
    }
    status = ProtocolFromPacket((Address) writePtr->buffer, 
		    interPtr->netType, &protocol);
    if (status != SUCCESS) {
	printf("DevNet_FsWrite: can't get protocol from packet\n");
	return SYS_INVALID_ARG;
    }
    if (protocol != statePtr->protocol) {
	if (devNetDebug) {
	    printf("DevNet_FsWrite: packet protocol %d != device protocol %d\n",
		protocol, statePtr->protocol);
	}
	return(SYS_INVALID_ARG);
    }
    ioVector.bufAddr = (Address)((int)writePtr->buffer + 
	net_NetworkHeaderSize[interPtr->netType]);
    ioVector.length  = dataSize;
    status = Net_RawOutputSync(interPtr, writePtr->buffer, &ioVector, 1);
    replyPtr->length = writePtr->length;
    return(status);
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
    int		openCount;
    /* Number of times device still open. */
    int		writerCount;	/* Number of writers still on the device. */
{
    DeviceState		*statePtr;
    DeviceQueueState 	*deviceQueuePtr;
    int			i;

    statePtr = (DeviceState *) (devicePtr->data);
    deviceQueuePtr = (DeviceQueueState *) statePtr->interPtr->devNetData;
    MASTER_LOCK(&deviceQueuePtr->mutex);
    List_Remove((List_Links *) statePtr);
    if (List_IsEmpty((List_Links *) deviceQueuePtr)) {
	Net_RemovePacketHandler(statePtr->interPtr);
    }
    MASTER_UNLOCK(&deviceQueuePtr->mutex);
    for (i=0 ; i< PACKET_QUEUE_LEN ; i++) {
	free ((char *) statePtr->queue.packet[i]);
    }
    free((char *) statePtr);
    devicePtr->data = (ClientData) NIL;
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
DevNet_FsSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device	*devicePtr;
    int			*readPtr;
    int			*writePtr;
    int			*exceptPtr;
{
    DeviceState		*statePtr;
    DeviceQueueState 	*deviceQueuePtr;

    statePtr = (DeviceState *) (devicePtr->data);
    deviceQueuePtr = (DeviceQueueState *) statePtr->interPtr->devNetData;

    MASTER_LOCK(&deviceQueuePtr->mutex);

    if (*readPtr) {
	if (QueueEmpty(statePtr->queue)) {
	    *readPtr = 0;
	}
    }
    *exceptPtr = 0;
    MASTER_UNLOCK(&deviceQueuePtr->mutex);
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
DevNet_FsIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOCParam *ioctlPtr;
    Fs_IOReply *replyPtr;

{
    DeviceState		*statePtr;
    Net_Interface	*interPtr;
    ReturnStatus	status = SUCCESS;

    statePtr = (DeviceState *) (devicePtr->data);
    interPtr = statePtr->interPtr;

    /*
     * Call the ioctl routine specific to the interface.
     */
    status = (interPtr->ioctl)(interPtr, ioctlPtr, replyPtr);

    /*
     * Here should be the handling of any ioctls that are specific to
     * the protocol or network devices in general.
     */

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ProtocolFromPacket --
 *
 *	Retrieves the protocol from a packet.  For the Ethernet
 *	the protocol is the type field in the header.  In general
 *	the protocol can be an arbitrary characteristic of the
 *	packet.  
 *
 * Results:
 *	The protocol associated with the packet.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ProtocolFromPacket(packetPtr, netType, protoPtr)
    Address		packetPtr;	/* Network-specific packet. */
    Net_NetworkType	netType;	/* Type of network. */
    int			*protoPtr;	/* Place to return protocol. */
{
    int			protocol = 0;
    ReturnStatus	status = SUCCESS;

    switch(netType) {
	case NET_NETWORK_ETHER:
	    protocol = NET_ETHER_HDR_TYPE(*(Net_EtherHdr *)packetPtr);
	    break;
	case NET_NETWORK_ULTRA:
	    protocol = 0;
	    break;
	default:
	    status = FAILURE;
    }
    if (status == SUCCESS) {
	*protoPtr = protocol;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ProtocolFromDevice--
 *
 *	Determine the protocol number from the protocol field in the
 *	device unit number.  This is also a function of the network
 *	type.
 *
 * Results:
 *	SUCCESS if the protocol was found, FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ProtocolFromDevice(unitProto, interPtr, protoPtr)
    int			unitProto;	/* Value of proto field in unit. */
    Net_Interface 	*interPtr;	/* Network interface. */
    int			*protoPtr;	/* Place to return protocol. */
{
    static int	mapping[NET_NUM_NETWORK_TYPES][DEV_NET_NUM_PROTO] = {

	/* NET_NETWORK_ETHER */ 
	    {0, /* Doesn't match any. */
	     NET_ETHER_ARP, 
	     NET_ETHER_REVARP,
	     NET_ETHER_IP, 
	     NET_ETHER_SPRITE_DEBUG,
	     NET_ETHER_MOP},
	/* NET_NETWORK_ULTRA */
	    {0, /* Doesn't match any. */
	     0,
	     0,
	     0}
     };

    if (unitProto < 0 || unitProto >= DEV_NET_NUM_PROTO) {
	return FAILURE;
    }
    *protoPtr = mapping[interPtr->netType][unitProto];
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * QueueFromInterface --
 *
 *	This routine gets the queue structure from the interface if
 *	one exists, or allocates one otherwise.  We need to have 
 *	this separate routine because many devices share the same
 *	queue structure of an interface and we have to avoid race
 *	conditions when two devices are opened.
 *
 * Results:
 *	Pointer to the queue structure.
 *
 * Side effects:
 *	The queue structure is allocated.
 *
 *----------------------------------------------------------------------
 */

static DeviceQueueState *
QueueFromInterface(interPtr)
    Net_Interface	*interPtr;  /* The network interface. */
{
    static Sync_Lock  lock = Sync_LockInitStatic("QueueFromInterface:lock");
#define LOCKPTR (&lock)

    DeviceQueueState *deviceQueuePtr;

    LOCK_MONITOR;
    deviceQueuePtr = (DeviceQueueState *) interPtr->devNetData;
    if (deviceQueuePtr == (DeviceQueueState *) NIL) {
	deviceQueuePtr = (DeviceQueueState *) 
				malloc(sizeof(DeviceQueueState));
	List_Init((List_Links *) deviceQueuePtr);
	Sync_SemInitDynamic(&deviceQueuePtr->mutex, 
	    "Dev:queueStateMutex");
	interPtr->devNetData = (ClientData) deviceQueuePtr;
    }
    UNLOCK_MONITOR;
    return deviceQueuePtr;
}

