/* 
 * netCode.c --
 *
 *	Various routines for initialzation, input and output.
 *
 *	TODO: This needs to be fixed to handle more than one interface.
 *	Update the route table accessed by spriteID to include a
 *	interface 
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
#endif

#include <sprite.h>
#include <list.h>
#include <net.h>
#include <netInt.h>
#include <devNet.h>
#include <sync.h>
#include <dbg.h>
#include <machMon.h>
#include <netInet.h>

/*
 * Network configuration table defined by machine dependent code.
 */
extern Net_Interface	netConfigInterfaces[];
extern int		netNumConfigInterfaces;
Net_Interface		*netInterfaces[NET_MAX_INTERFACES];
int			netNumInterfaces;

Net_Address 		netZeroAddress;
int			net_NetworkHeaderSize[NET_NUM_NETWORK_TYPES];

#define	INC_BYTES_SENT(gatherPtr, gatherLength) { \
	register	Net_ScatterGather	*gathPtr; \
	int					i; \
	net_EtherStats.bytesSent += sizeof(Net_EtherHdr); \
	for (i = gatherLength, gathPtr = gatherPtr; i > 0; i--, gathPtr++) { \
	    net_EtherStats.bytesSent += gathPtr->length; \
	} \
    }
/*
 * Macro to swap the fragOffset field.
 */
#define SWAP_FRAG_OFFSET_HOST_TO_NET(ptr) { \
    unsigned short	*shortPtr; \
    shortPtr = ((unsigned short *)&ptr->ident) + 1; \
    *shortPtr = Net_HostToNetShort(*shortPtr); \
} 

#define SWAP_FRAG_OFFSET_NET_TO_HOST(ptr) { \
    unsigned short	*shortPtr; \
    shortPtr = ((unsigned short *)&ptr->ident) + 1; \
    *shortPtr = Net_NetToHostShort(*shortPtr); \
} 

int netDebug = FALSE;
Boolean	ultra = FALSE;

static void NetAddStats _ARGS_((Net_Stats *aPtr, Net_Stats *bPtr, 
		    Net_Stats *sumPtr));
static void EnterDebugger _ARGS_((Net_Interface *interPtr, Address packetPtr,
				int packetLength));

/*
 *----------------------------------------------------------------------
 *
 * Net_Init --
 *
 *	Initialize the network module data structures.
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
Net_Init()
{
    register int i;
    char buffer[100];
    ReturnStatus status;
    int		counter[NET_NUM_NETWORK_TYPES];
    Net_Interface	*interPtr;

    NetEtherInit();

    for (i = 0; i < NET_NUM_NETWORK_TYPES; i++) {
	counter[i] = 0;
    }

    /*
     * Determine the number and kind of network interfaces by calling
     * each network interface initialization procedure.
     */
    bzero((char *) &netZeroAddress, sizeof(Net_Address));
    netNumInterfaces = 0;
    for (i = 0 ; i<netNumConfigInterfaces ; i++) {
	netConfigInterfaces[i].flags = 0;
	status = (netConfigInterfaces[i].init)(&(netConfigInterfaces[i]));
	if (status == SUCCESS) {
	    netInterfaces[netNumInterfaces] = &netConfigInterfaces[i];
	    interPtr = netInterfaces[netNumInterfaces];
	    netNumInterfaces++;
	    interPtr->packetProc = NILPROC;
	    interPtr->devNetData = (ClientData) NIL;
	    interPtr->number = counter[interPtr->netType];
	    counter[interPtr->netType]++;
	    Mach_MonPrintf("%s net interface %d at 0x%x\n",
		interPtr->name,
		interPtr->number,
		interPtr->ctrlAddr);
	    sprintf(buffer, "NetOutputMutex:%d", i);
	    Sync_SemInitDynamic(&interPtr->syncOutputMutex, 
		buffer);
	    sprintf(buffer, "NetMutex:%d", i);
	    Sync_SemInitDynamic(&interPtr->mutex, buffer);
	} 
    }
    net_NetworkHeaderSize[NET_NETWORK_ETHER] = sizeof(Net_EtherHdr);
    net_NetworkHeaderSize[NET_NETWORK_ULTRA] = sizeof(Net_UltraHeader);
    Net_ArpInit();
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_Bin --
 *
 *	Bin various memory sizes allocated by the net module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Mem_Bin to optimize memory allocation.
 *
 *----------------------------------------------------------------------
 */

void
Net_Bin()
{
    register int inter;
    for (inter = 0 ; inter < netNumInterfaces ; inter++) {
	Mem_Bin(netInterfaces[inter]->maxBytes);
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_GatherCopy --
 *
 *	Copy all of the data pointed to by the scatter/gather array into
 *	the destination.
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
Net_GatherCopy(scatterGatherPtr, scatterGatherLength, destAddr)
    register	Net_ScatterGather *scatterGatherPtr;
    int		  		  scatterGatherLength;
    register	Address		  destAddr;
{
    int i;
    int soFar = 0;

    for (i = 0; i < scatterGatherLength; i++, scatterGatherPtr++) {
	if (scatterGatherPtr->length == 0) {
	    continue;
	}

	bcopy((Address) scatterGatherPtr->bufAddr, 
	     (Address) &(destAddr[soFar]), 
	     scatterGatherPtr->length);
	soFar += scatterGatherPtr->length;
    }
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * Net_Reset --
 *
 *	Reset the network interface.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reinitializes the network controller..
 *
 *----------------------------------------------------------------------
 */
void
Net_Reset(interPtr)
    Net_Interface	*interPtr;
{
    interPtr->reset(interPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_Output --
 *
 *	Send a packet to a host identified by a Sprite Host ID.
 *
 * Results:
 *	SUCCESS 	- the operation was successful.
 *	FAILURE		- there was no route to the host or 
 *			  the Sprite host ID was bad or 
 *			  an unknown route type was found.
 *
 * Side effects:
 *	Sends the packet.
 *	If no route has been established to the SpriteID then the
 *	Address Resolution Protocol (ARP) is invoked to find the
 *	physical address corresponding to the SpriteID.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_Output(spriteID, gatherPtr, gatherLength, mutexPtr, routePtr)
    int spriteID;			/* Host to which to send the packet */
    Net_ScatterGather *gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet The first
					 * element of gatherPtr is assumed to 
					 * be a buffer large enought to 
					 * format any protocol headers
					 * we need. */
    int gatherLength;			/* Number of elements in gatherPtr[] */
    Sync_Semaphore *mutexPtr;		/* Mutex that is released during the
					 * ARP transaction (if needed).  This
					 * doesn't mess up the caller because
					 * its packet isn't output until
					 * the ARP completes anyway */
    Net_Route	*routePtr;		/* If non-NIL then used as the route
					 * to send the packet. */
{
    Net_Interface	*interPtr;
    Boolean		ourRoute = FALSE;
    ReturnStatus	status;

    if (spriteID < 0 || spriteID >= NET_NUM_SPRITE_HOSTS) {
	return(NET_UNREACHABLE_NET);
    }

    /*
     * Check for a route to the indicated host.  Use ARP to find it if needed.
     */
    if (routePtr == (Net_Route *) NIL) {
	ourRoute = TRUE;
	routePtr = Net_IDToRoute(spriteID, 0, TRUE, mutexPtr, 0);
	if (routePtr == (Net_Route *)NIL) {
	    return(NET_UNREACHABLE_NET);
	}
    }
    interPtr = routePtr->interPtr;
    switch(routePtr->protocol) {
	case NET_PROTO_RAW : {
		/*
		 * The first gather buffer contains the protocol header for
		 * which we have none for the ethernet.
		 */
		gatherPtr->length = 0; 	
		gatherPtr->done = FALSE;
		gatherPtr->mutexPtr = mutexPtr;
		status = (interPtr->output) (interPtr, 
		    routePtr->headerPtr[NET_PROTO_RAW], 
		    gatherPtr, gatherLength, TRUE, &status);
		if (status == SUCCESS) {
		    while (!gatherPtr->done && 
			mutexPtr != (Sync_Semaphore *)NIL) {
			(void) Sync_SlowMasterWait((unsigned int)mutexPtr, 
				mutexPtr, 0);
		    }
		}
		break;
	    }
	case NET_PROTO_INET: {
		/*
		 * For the INET routes we must fill in the ipHeader in the 
		 * first buffer of the gather array. 
		 */
		register Net_IPHeader		*ipHeaderPtr;
		register unsigned int length;
		register Net_ScatterGather	*gathPtr;
		register int	 i; 

		/*
		 * Compute the length of the gather vector. 
		 */
		length = sizeof(Net_IPHeader);
		gathPtr = gatherPtr + 1;
		for (i = 1; i < gatherLength;  i++, gathPtr++){
		    length += gathPtr->length; 
		}


		/*
		 * Fill the  ipHeader into the first element of the 
		 * scatter/gather from the template stored in the route
		 * data.
		 */
		gatherPtr->length = sizeof(Net_IPHeader);
		ipHeaderPtr = (Net_IPHeader *) gatherPtr->bufAddr;
		gatherPtr->done = FALSE;
		gatherPtr->mutexPtr = mutexPtr;

		*ipHeaderPtr = *(Net_IPHeader *) 
				    routePtr->headerPtr[NET_PROTO_INET];
		/*
		 * Update length and checksum. The template 
		 * ipHeaderPtr->checksum should contain the 16 bit sum of 
		 * the IP header with totalLen set to zero. We add the
		 * new total length and convert into one-complement.
		 * See Net_InetChecksum().
		 */
		length = Net_HostToNetShort(length);
		ipHeaderPtr->totalLen = length;

		length = ipHeaderPtr->checksum + length;
		ipHeaderPtr->checksum = ~(length + (length >> 16));

		status = (interPtr->output)(interPtr, 
		    routePtr->headerPtr[NET_PROTO_RAW], gatherPtr, 
		    gatherLength, FALSE, &status);
		if (status == SUCCESS) {
		    while (!gatherPtr->done && 
			mutexPtr != (Sync_Semaphore *)NIL) {
			(void) Sync_SlowMasterWait((unsigned int)mutexPtr, 
				    mutexPtr, 0);
		    }
		}
		break;
	    }
	default:
	    printf("Warning: Net_Output: unsupported route protocol: %x\n", 
			routePtr->protocol);
	    panic(NIL);
	    status = FAILURE;
    }
    if (ourRoute) {
	Net_ReleaseRoute(routePtr);
    }
    return status;
}



/*
 *----------------------------------------------------------------------
 *
 * Net_RawOutput --
 *
 *	Send a packet directly onto the network.
 *
 * Results:
 *	SUCCESS if the packet made it as far as the network interface,
 *	a failure code otherwise.  SUCCESS does not imply that the
 *	packet was actually sent because the interface could have
 *	rejected it.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_RawOutput(interPtr, headerPtr, gatherPtr, gatherLength)
    Net_Interface	*interPtr;	/* The network interface. */
    Address		headerPtr;	/* Packet header. */
    Net_ScatterGather	*gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet */
    int 		gatherLength;	/* Number of elements in gatherPtr[] */
{
    return (interPtr->output)(interPtr, headerPtr, gatherPtr, gatherLength, 
		FALSE, (ReturnStatus *) NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_RecvPoll --
 *
 *	See if a packet has come in.  If one has come in then it is assumed
 *	that the packet processing routine will get called.  Thus this routine
 *	does not return any value; it is up to the packet processing routine
 *	to set some global state.  This is intended to be used by the
 *	debugger.
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
Net_RecvPoll(interPtr)
    Net_Interface	*interPtr;	/* Network interface. */
{
    if (netDebug) {
	printf("Net_RecvPoll\n");
    }
    (interPtr->intr)(interPtr, TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_RawOutputSync --
 *
 * 	Send a packet on the network. Does not return until the packet
 *	is actually sent.
 *
 * Results:
 *	SUCCESS if the packet was sent correctly, otherwise a standard
 *	Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_RawOutputSync(interPtr, headerPtr, gatherPtr, gatherLength)
    Net_Interface	*interPtr;	/* Network interface. */
    Address		headerPtr;	/* Packet header. */
    Net_ScatterGather 	*gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet */
    int 		gatherLength;	/* Number of elements in gatherPtr[] */
{
    ReturnStatus	status;

    gatherPtr->mutexPtr = &(interPtr->syncOutputMutex);
    gatherPtr->done = FALSE;

    MASTER_LOCK(&(interPtr->syncOutputMutex));

    status = (interPtr->output)(interPtr, headerPtr, gatherPtr, gatherLength, 
	FALSE, &status);
    if (status == SUCCESS) {
	while (!gatherPtr->done) {
	    (void) Sync_SlowMasterWait(
			(unsigned int)&(interPtr->syncOutputMutex), 
			&(interPtr->syncOutputMutex), FALSE);
	}
    }
    MASTER_UNLOCK(&(interPtr->syncOutputMutex));
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetOutputWakeup --
 *
 *	Called to notify a waiter that a packet has been sent.  This is
 *	hacked up now, as the argument is really a mutexPtr which
 *	has been used as a raw event to wait on.  We have to use
 *	the raw SlowBroadcast procedure because of this.
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
NetOutputWakeup(mutexPtr)
    Sync_Semaphore	*mutexPtr;	/* Mutex from scatter/gather struct */
{
    int waiting;

    /*
     * FIX THIS.
     */
    if (dbg_UsingNetwork) {
	return;
    }

#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
    (void) Sync_SlowBroadcast((unsigned int)mutexPtr, &waiting);
#else 	/* Mutiprocessor implementation */
   /*
    * Because the packet sent interrupt may come in before Net_Output
    * has a chance to MasterWait and after Net_Output has checked the
    * gatherPtr->done flag, the code should syncronize with the caller
    * by obtaining the master lock.
    */
    MASTER_LOCK(mutexPtr);
    (void) Sync_SlowBroadcast((unsigned int) mutexPtr, &waiting);	
    MASTER_UNLOCK(mutexPtr);
#endif
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Net_Intr --
 *
 *	The interrupt routine which is called when the ethernet chip 
 *	interrupts the processor.  All this routine does is to branch
 *	to the interrupt handler for the type of ethernet device
 *	present on the machine.  The device driver, in turn, eventually
 *	calls Net_Input to pass the packet to the correct protocol handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Net_Intr(interPtr)
    Net_Interface	*interPtr;	/* Network interface. */
{
    if (netDebug) {
	printf("Received an interrupt on interface %s\n", interPtr->name);
    }
    (interPtr->intr)(interPtr, FALSE);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_Input --
 *
 *	A stub called by device drivers to pass off packets to protocols.
 *	This could be a macro.
 *
 *	The packet handler called must copy the packet to private buffers.
 *
 *	TODO:
 *		This routine, and the ones it calls, should not assume
 *		that there is a packet header at the start of the
 *		packet.  The header should be passed separately.
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
Net_Input(interPtr, packetPtr, packetLength)
    Net_Interface *interPtr;	/* Network interface. */
    Address packetPtr;		/* The packet. */
    int packetLength;		/* The length of the packet. */
{
    int		headerSize;
    int		packetType = NET_PACKET_UNKNOWN;
    Boolean	ultra = FALSE;

#if 0
    printf("in Net_Input\n");
    printf("Net_Input(0x%x, 0x%x, 0x%x)\n", interPtr, packetPtr, packetLength);
    printf("netType = %08x\n", interPtr->netType);
#endif

    if (netDebug) {
	printf("Net_Input(0x%x, 0x%x, 0x%x) netType = %08x\n",
	    interPtr, packetPtr, packetLength, interPtr->netType);
    }
    switch(interPtr->netType) {
	case NET_NETWORK_ETHER : {
	    Net_EtherHdr *etherHdrPtr;
	    int		type;
	    etherHdrPtr = (Net_EtherHdr *)packetPtr;
	    type = Net_NetToHostShort((unsigned short)
			NET_ETHER_HDR_TYPE(*etherHdrPtr));
	    switch(type) {
		case NET_ETHER_SPRITE:
		    packetType = NET_PACKET_SPRITE;
		    break;
		case NET_ETHER_ARP:
		    packetType = NET_PACKET_ARP;
		    break;
		case NET_ETHER_REVARP:
		    packetType = NET_PACKET_RARP;
		    break;
		case NET_ETHER_SPRITE_DEBUG:
		    packetType = NET_PACKET_DEBUG;
		    break;
		case NET_ETHER_IP:
		    packetType = NET_PACKET_IP;
		    break;
		default:
		    packetType = NET_PACKET_UNKNOWN;
		    break;
	    }
	    ultra = FALSE;
	    break;
	}
	case NET_NETWORK_ULTRA: {
	    Net_UltraHeader	*ultraHdrPtr;
	    ultraHdrPtr = (Net_UltraHeader *) packetPtr;
	    ultra = TRUE;
	    if ((ultraHdrPtr->localAddress.tsapSize == 2) &&
		(ultraHdrPtr->localAddress.tsap[0] == (unsigned char) 0xff) &&
		(ultraHdrPtr->localAddress.tsap[1] == (unsigned char) 0xff)) {
		packetType = NET_PACKET_SPRITE;
	    } else {
		packetType = NET_PACKET_UNKNOWN;
	    }
	    break;
	}
	default : 
	    printf("Net_Input: invalid net type %d\n", interPtr->netType);
    }
    headerSize = net_NetworkHeaderSize[interPtr->netType];
    if (dbg_UsingNetwork) {
	/*
	 * If the kernel debugger is running it gets all the packets. We
	 * process ARP requests to allow hosts to talk to the debugger.
	 */
	if (packetType == NET_PACKET_ARP) {
            NetArpInput(interPtr, packetPtr, packetLength);
	} else { 
	    Dbg_InputPacket(interPtr, packetPtr, packetLength);
	}
	return;
    }
    switch(packetType) {
        case NET_PACKET_SPRITE:
	    if (netDebug) {
		printf("Received a Sprite packet, calling Rpc_Dispatch\n");
	    }
            Rpc_Dispatch(interPtr, NET_PROTO_RAW, packetPtr, 
		     (Address) (((char *) packetPtr) + headerSize), 
		     packetLength - headerSize);
            break;

        case NET_PACKET_ARP:
	case NET_PACKET_RARP:
	    /*
	     * The kernel gets first shot at ARP packets and then they are
	     * forward to tbe /dev/net device.
	     */
	    if (netDebug) {
		if (packetType == NET_PACKET_ARP) {
		    printf("Received an ARP packet.\n");
		} else {
		    printf("Received a RARP packet.\n");
		}
	    }
            NetArpInput(interPtr, packetPtr, packetLength);
	    if (interPtr->packetProc != NILPROC) {
		(interPtr->packetProc)(interPtr, packetLength, packetPtr);
	    }
	    break;

        case NET_PACKET_DEBUG:
	    if (netDebug) {
		printf("Received a debug packet.\n");
	    }
            EnterDebugger(interPtr, packetPtr, packetLength);
            break;

	case NET_PACKET_IP: {
	    register Net_IPHeader *ipHeaderPtr = 
			(Net_IPHeader *) (packetPtr + headerSize);
	    /*
	     * The kernel steals IP packets with the Sprite RPC protocol number.
	     */
	    if (netDebug) {
		printf("Received an IP packet.\n");
	    }
	    if ( (packetLength > sizeof(Net_IPHeader)+headerSize) && 
	         (ipHeaderPtr->protocol == NET_IP_PROTOCOL_SPRITE)) {
		int    headerLenInBytes;
		int    totalLenInBytes;
		headerLenInBytes = ipHeaderPtr->headerLen * 4;
		totalLenInBytes = Net_NetToHostShort(ipHeaderPtr->totalLen);
		/*
		 * Validate the packet. We toss out the following cases:
		 * 1) Runt packets.
		 * 2) Bad checksums.
		 * 3) Fragments.
		 * Since we sent the packets with dont fragment set we 
		 * shouldn't get any fragments.
		 */
		if ((headerLenInBytes >= sizeof(Net_IPHeader)) &&
		     (totalLenInBytes > headerLenInBytes) &&
		     (totalLenInBytes <= (packetLength-headerSize)) &&
		     (Net_InetChecksum(headerLenInBytes, (Address)ipHeaderPtr)
		                        == 0)) {

		    SWAP_FRAG_OFFSET_NET_TO_HOST(ipHeaderPtr);
		    if((!(ipHeaderPtr->flags & NET_IP_MORE_FRAGS)) &&
		      (ipHeaderPtr->fragOffset == 0)) {
			if (netDebug) {
			    printf(
			"Received a Sprite IP packet, calling Rpc_Dispatch\n");
			}
			Rpc_Dispatch(interPtr, NET_PROTO_INET, packetPtr, 
		           (Address)(((char *) ipHeaderPtr) + headerLenInBytes),
				     totalLenInBytes-headerLenInBytes);
		     }
		}

	    } else {

		if (interPtr->packetProc != NILPROC) {
		    (interPtr->packetProc)(interPtr, packetLength, packetPtr);
		}
	    }
	    break;
	}
	default:
	    if (netDebug) {
		printf("Received a packet with unknown type 0x%x.\n",
		    packetType);
	    }
	    if (interPtr->packetProc != NILPROC) {
		(interPtr->packetProc)(interPtr, packetLength, packetPtr);
	    }
	    break;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * EnterDebugger --
 *
 *	Processes the special NET_ETHER_SPRITE_DEBUG packet type.
 *	Prints the data in the packet (which is the hostname of the sender)
 *	and then enters the debugger. The format of data in the packet is:
 *	 1) size of sender's name in bytes (4 bytes),
 *	 2) the sender's name (max 100 bytes).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Enters the debugger.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
EnterDebugger(interPtr, packetPtr, packetLength)
    Net_Interface	*interPtr;
    Address packetPtr;
    int packetLength;
{
    char *name;
    unsigned int  len;

    packetPtr += net_NetworkHeaderSize[interPtr->netType];
    /*
     * Copy the length out of the packet into a correctly aligned integer.
     * Correct its byte order.
     */
    bcopy( packetPtr, (Address) &len, sizeof(len));

    len = Net_NetToHostInt(len);

    /*
     * Validate the data length and make sure the name is null-terminated.
     */
    if (len < 100) {
	name = (char *) (packetPtr + sizeof(len));
	name[len] = '\0';
	printf("\n*** Got a debugger packet from %s ***\n", name);
    } else {
	printf("\n*** Got a debugger packet ***\n");
    }

    DBG_CALL;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_GetInterface --
 *
 *	Returns a pointer to the interface structure with the
 *	given number and type.
 *
 * Results:
 *	A pointer to a Net_Interface if one with the given number
 *	exists, NIL otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_Interface *
Net_GetInterface(netType, number)
    Net_NetworkType	netType;	/* Type of interface. */
    int			number;		/* Number of interface. */
{
    Net_Interface	*interPtr;
    register int	i;

    interPtr = (Net_Interface *) NIL;
    for (i = 0; i < netNumInterfaces; i++) {
	if (netInterfaces[i]->netType == netType &&
	    netInterfaces[i]->number == number) {
	    interPtr = netInterfaces[i];
	    break;
	}
    }
    return interPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_NextInterface --
 *
 * 	This routine can be used to iterate through all network
 *	interfaces, regardless of type.  The parameter 'index'
 *	is used to keep track of where we are in the interaction.
 *	A pointer will be returned to first interface whose index
 *	is equal to or greater than 'index' and that is running.
 *	The 'index' parameter will be set to the index of the
 *	interface upon return.  
 *
 * Results:
 *	A pointer to the first interface whose number if greater than
 *	or equal to the contents of indexPtr and is running. NIL
 *	if no such interface exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_Interface *
Net_NextInterface(running, indexPtr)
    Boolean		running;	/* TRUE => returned interface	
					 * must be in running state. */
    int			*indexPtr;	/* Ptr to index to use. */
{
    register int	i;

    for (i = *indexPtr; i < netNumInterfaces; i++) {
	if (netDebug) {
	    printf("i = %d, name = %s, running = %d\n", i, 
		netInterfaces[i]->name, 
		netInterfaces[i]->flags & NET_IFLAGS_RUNNING);
	}
	if (!running || netInterfaces[i]->flags & NET_IFLAGS_RUNNING) {
	    *indexPtr = i;
	    return netInterfaces[i];
	}
    }
    return (Net_Interface *) NIL;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_SetPacketHandler --
 *
 *	Routine to register a callback for each packet received
 *	on a particular network interface.
 *	Right now the Net_Input routine knows about the different
 * 	types of packets and knows which routines to call.  This
 *	routine is used to set the callback for generic packets
 *	so that the dev module sees them. 
 *
 *	The handler parameter should have the following definition:
 *		void	handler(interPtr, packetPtr, size)
 *			Net_Interface	*interPtr; 
 *			Address		packetPtr; 
 *			int		size;	   
 *
 *	TODO:  It would probably be nice to classify packets (rpc, ip,
 *		ether, etc) and register interest in the different
 *		classifications. That would make Net_Input more
 *		general.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The function parameter will be called for all 'normal' network
 *	packets.
 *
 *----------------------------------------------------------------------
 */

void
Net_SetPacketHandler(interPtr, handler)
    Net_Interface	*interPtr;	/* The interface. */
    void		(*handler)();	/* Packet handling routine. */
{
    interPtr->packetProc = handler;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_RemovePacketHandler --
 *
 *	Routine to remove a packet handler callback procedure.
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
Net_RemovePacketHandler(interPtr)
    Net_Interface	*interPtr;	/* The interface. */
{
    interPtr->packetProc = NILPROC;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_GetStats --
 *
 *	Returns the event statistics for the various network 
 *	interfaces.  The sum of the stats for all interfaces of the
 *	given type is returned.
 *
 * Results:
 *	SUCCESS if the statistics are returned, an error code otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_GetStats(netType, statPtr)
    Net_NetworkType	netType; 	/* Type of interfaces to get stats
					 * for. */
    Net_Stats		*statPtr;	/* Statistics to fill in. */ 
{
    int			i;
    ReturnStatus	status = SUCCESS;
    Net_Stats		tmpStats;

    bzero((char *) statPtr, sizeof(Net_Stats));
    for (i = 0; i < netNumInterfaces; i++) {
	if ((netInterfaces[i]->flags & NET_IFLAGS_RUNNING) &&
	    (netInterfaces[i]->netType == netType)) {
	    status = (netInterfaces[i]->getStats)(netInterfaces[i], &tmpStats);
	    if (status != SUCCESS) {
		break;
	    }
	    NetAddStats(&tmpStats, statPtr, statPtr);
	}
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * NetAddStats --
 *
 *	Add two stats structures together. This routine assumes that
 *	the structures are composed of only integers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
NetAddStats(aPtr, bPtr, sumPtr)
    Net_Stats *aPtr;			/* First addend. */
    Net_Stats *bPtr;			/* Second addend. */
    Net_Stats *sumPtr;			/* The sum of the two. */
{
    Net_Stats		tmp;
    int			i;

    bzero((char *) &tmp, sizeof(tmp));
    for (i = 0; i < sizeof(Net_Stats) / sizeof(int); i++) {
	((int *) &tmp)[i] = ((int *) aPtr)[i] + ((int *) bPtr)[i];
    }
    bcopy((char *) &tmp, (char *) sumPtr, sizeof(Net_Stats));
}

