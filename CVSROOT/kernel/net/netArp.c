/* 
 * netArp.c --
 *
 *	Routines for net arp and rarp.
 *
 * Copyright 1990 Regents of the University of California
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
#endif /* not lint */

#include <sprite.h>
#include <sys.h>
#include <stdlib.h>
#include <timer.h>
#include <sync.h>
#include <rpc.h>
#include <netInt.h>
#include <netArp.h>
#include <string.h>
#include <stdio.h>
/*
 * This flag turns on print statements in the ARP protocol
 */
Boolean arpDebug = FALSE;

ArpStatistics arpStatistics;

/*
 * Current ARP transactions have their state linked together in
 * a list so the packet handler can save the return address for
 * the sender.
 */
static List_Links arpList;
static List_Links revArpList;
static Sync_Semaphore arpListMutex;

static ArpOutputQueue arpOutputQueue[ARP_OUTPUT_QUEUE_LEN];
static int nextOutputIndex = 0;

static Sync_Lock arpOutputQueueLock; ;
#define LOCKPTR (&arpOutputQueueLock)
static ArpInputQueue arpInputQueue[ARP_INPUT_QUEUE_LEN];
static int nextInputIndex = 0;
static Sync_Semaphore arpInputMutex;

static	void	Net_ArpTimeout _ARGS_((Timer_Ticks time, ClientData data));
static	void	NetArpHandler _ARGS_((ClientData data, 
				Proc_CallInfo *callInfoPtr));
static	void	NetArpOutput _ARGS_((Net_Interface *interPtr,
				Net_EtherAddress *destEtherAddrPtr,
				int etherType, NetSpriteArp *requestPtr));
static	void	NetFillInArpRequest _ARGS_((int command, 
				Net_NetworkType	netType, int protocol,
				ClientData targetId, ClientData senderId,
				Net_Address *targetAddrPtr, 
				Net_Address *senderAddrPtr, 
				NetSpriteArp *requestPtr));

/*
 *----------------------------------------------------------------------
 *
 * Net_ArpInit --
 *
 *	Initializes the arp data structures.
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
Net_ArpInit()
{
    int i;

    printf("In Net_ArpInit.\n");

    Sync_LockInitDynamic(&arpOutputQueueLock, "Net:arpOutputQueueLock");
    Sync_SemInitDynamic(&arpInputMutex, "Net:arpInputMutex");
    Sync_SemInitDynamic(&arpListMutex, "Net:arpListMutex"); 
    /*
     * Mark the arp output queue as all done (output).
     */
    for (i=0; i<ARP_OUTPUT_QUEUE_LEN ; i++) {
	arpOutputQueue[i].gather.done = TRUE;
    }
    List_Init(&arpList);
    List_Init(&revArpList);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_Arp --
 *
 *      Sprite's Address Resolution Protocol.  Broadcast a Sprite ARP
 *      packet to find the physical address that is used to get to the
 *      host identified by the Sprite ID.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A broadcast, and the route table is updated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_Arp(spriteID, mutexPtr)
    int	spriteID;			/* ID to find the route for */
    Sync_Semaphore *mutexPtr;		/* Address of the mutex that the
					 * caller of Net_Output used for
					 * synchronization.  This needs to
					 * be released during the ARP so that
					 * we can receive our reply. */
{
    ReturnStatus status = FAILURE;
    NetSpriteArp request;		/* The Sprite ARP request packet data */
    NetSpriteArp reply;			/* The Sprite ARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */
    Net_Interface *interPtr;
    Net_Route	*routePtr;
    int		i;

    for (i = 0; ; i++) {
	routePtr = Net_IDToRoute(NET_BROADCAST_HOSTID, i, FALSE, 
	    (Sync_Semaphore *) NIL, 0);
	if (routePtr == (Net_Route *) NIL) {
	    break;
	}
	interPtr = routePtr->interPtr;
	NetFillInArpRequest(NET_ARP_REQUEST, interPtr->netType,
		NET_PROTO_RAW,
		(ClientData) spriteID, (ClientData) rpc_SpriteID, 
		&netZeroAddress, &interPtr->netAddress[NET_PROTO_RAW],
		&request);
	gather.bufAddr = (Address)&request;
	gather.length = sizeof(NetSpriteArp);
	gather.done = FALSE;
	gather.mutexPtr = (Sync_Semaphore *) NIL;
    
	status = NetDoArp(routePtr, mutexPtr, NET_ARP_REQUEST, &gather, 
			&reply);
	Net_ReleaseRoute(routePtr);
	if (status == SUCCESS) {
	    Net_Address netAddress;

	    NET_ETHER_ADDR_COPY(* ARP_SRC_ETHER_ADDR(&reply),
		netAddress.ether);
	    (void) Net_InstallRoute(spriteID, interPtr, &netAddress,
			NET_PROTO_RAW, "noname", "unknown", 
			(ClientData) 0);
	    return status;
	}
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_RevArp --
 *
 *      Sprite's Reverse Address Resolution Protocol.  Broadcast a Sprite
 *      Reverse ARP packet to find the Sprite ID or Internet address
 *	that corresponds to our network address.
 *
 * Results:
 *	A Sprite host ID
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Net_RevArp(routePtr, protocol, netAddressPtr, mutexPtr)
    Net_Route		*routePtr;	/* The route to use for the arp.*/
    int			protocol;	/* Which protocol we are trying
					 * to resolve an address for. */
    Net_Address		*netAddressPtr; /* The address we are trying to 
					 * resolve. */
    Sync_Semaphore	*mutexPtr;	/* Mutex to release. */
{
    ReturnStatus status;
    NetSpriteArp request;		/* Sprite RARP request packet data */
    NetSpriteArp reply;			/* Sprite RARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */
    Net_Interface	*interPtr;

    interPtr = routePtr->interPtr;
    if (netAddressPtr == (Net_Address *) NIL) {
	netAddressPtr = &interPtr->netAddress[NET_PROTO_RAW];
    }
    NetFillInArpRequest(NET_RARP_REQUEST, interPtr->netType,
		protocol, 
		(ClientData) 0, (ClientData) 0,
		netAddressPtr,
		&interPtr->netAddress[NET_PROTO_RAW], 
		&request);
    gather.bufAddr = (Address)&request;
    gather.length = sizeof(NetSpriteArp);
    gather.done = FALSE;
    gather.mutexPtr = (Sync_Semaphore *) NIL;

    status = NetDoArp(routePtr, mutexPtr, NET_RARP_REQUEST, &gather, 
	&reply);
    if (status == SUCCESS) {
	unsigned int value;
	bcopy(ARP_TARGET_PROTO_ADDR(&reply),(char *) &value,4);
	return(Net_NetToHostInt(value));
    } 
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDoArp --
 *
 *	The broadcast-retry-wait loop for ARP and Reverse Arp.
 *
 * Results:
 *	Fills in the callers packet buffer with a copy of the reply.
 *	Returns FAILURE if there is no response after a timeout.
 *
 * Side effects:
 *	The packet broadcast, wait, etc.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetDoArp(routePtr, mutexPtr, command, gatherPtr, packetPtr)
    Net_Route		*routePtr;	/* The route to use for the arp. */
    Sync_Semaphore *mutexPtr;		/* Address of the mutex that the
					 * caller of Net_Output used for
					 * synchronization.  This needs to
					 * be released during the ARP so that
					 * we can receive our reply. */
    int command;			/* NET_ARP_REQUEST or NET_RARP_REQUEST*/
    Net_ScatterGather *gatherPtr;	/* Specifies the output packet */
    NetSpriteArp *packetPtr;		/* Filled in with the reply packet */
{
    register Net_EtherHdr *etherHdrPtr;	/* Regular RPC broadcast header */
    Net_EtherHdr etherHdr;		/* Header for Sprite ARP packet */
    int retries = 0;			/* Retry counter */
    ArpState arp;			/* State for the protocol */
    NetSpriteArp *requestPtr;		/* Pointer to request data */
    List_Links *listPtr;		/* Either arpList or revArpList */
    Net_Interface 	*interPtr;	/* Network interface. */

    /*
     * If we are aren't passed a mutexPtr then assume we can't do an
     * Arp.  This is because we are either at interrupt level or there
     * are high-level locks that cannot be released.
     */
    if (mutexPtr == (Sync_Semaphore *) NIL) {
	return FAILURE;
    }
    if (Mach_AtInterruptLevel()) {
	panic("NetDoArp: at interrupt level!!!");
    }
    interPtr = routePtr->interPtr;
    switch(interPtr->netType) {
	case NET_NETWORK_ETHER : {
	    /*
	     * Set up the ethernet header for the Arp request packet.  We can't
	     * use the regular broadcast route because the ethernet protocol 
	     * type is different, Arp.  The broadcast destination address,
	     * however, is obtained from the regular broadcast route.
	     */
	    etherHdrPtr = (Net_EtherHdr *)routePtr->headerPtr[NET_PROTO_RAW];
	    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
				NET_ETHER_HDR_DESTINATION(etherHdr));
	    if (command == NET_ARP_REQUEST) {
		NET_ETHER_HDR_TYPE(etherHdr) = 
		    Net_HostToNetShort(NET_ETHER_ARP);
	    } else {
		NET_ETHER_HDR_TYPE(etherHdr) = 
		    Net_HostToNetShort(NET_ETHER_REVARP);
	    }
	    break;
	}
	default:
	    printf("NetDoArp :Invalid route (bad network type %d)\n", 
		routePtr->interPtr->netType);
	    return FAILURE;
    }
    requestPtr = (NetSpriteArp *)gatherPtr->bufAddr;
    /*
     * Use a simple retry loop to get our reply.  The ARP protocol state
     * is set up and put on a list so the packet handler can find it.
     */

    arp.state = ARP_WANT_REPLY;
    arp.mutexPtr = mutexPtr;
    arp.type = Net_NetToHostShort((requestPtr->arpHeader.protocolType));

    if (command == NET_ARP_REQUEST) {
	int spriteID;
	bcopy(ARP_TARGET_PROTO_ADDR(requestPtr),(char *) &spriteID,4);
	arp.id =  (ClientData) Net_NetToHostInt((unsigned int)spriteID);
	listPtr = &arpList;
    } else {
	arp.id = (ClientData) ARP_TARGET_ETHER_ADDR(requestPtr);
	listPtr = &revArpList;
    }
    MASTER_LOCK(&arpListMutex);
    List_InitElement((List_Links *) &arp);
    List_Insert((List_Links *)&arp, LIST_ATREAR(listPtr));
    MASTER_UNLOCK(&arpListMutex);

    while (retries < 4 && ((arp.state & ARP_HAVE_INPUT) == 0)) {
	retries++;
	if (command == NET_ARP_REQUEST) {
	    if (arpDebug) {
		printf("Sending arp request for %d\n", arp.id);
	    }
	    arpStatistics.numArpRequests++;
	} else {
	    if (arpDebug) {
		printf("Sending rev arp request\n");
	    }
	    arpStatistics.numRevArpRequests++;
	}
	(void) (interPtr->output)(interPtr, (Address) &etherHdr, gatherPtr, 1, 
		    FALSE, (ReturnStatus *) NIL);

	arp.state |= ARP_IN_TIMEOUT_QUEUE ;
	arp.timeout.routine = Net_ArpTimeout;
	arp.timeout.interval = 500 * timer_IntOneMillisecond;
	arp.timeout.clientData = (ClientData)&arp;
	Timer_ScheduleRoutine(&arp.timeout, TRUE);
	do {
	    Sync_MasterWait(&arp.condition, arp.mutexPtr, FALSE);
	} while (((arp.state & ARP_HAVE_INPUT) == 0) &&
		 (arp.state & ARP_IN_TIMEOUT_QUEUE));
    }
    MASTER_LOCK(&arpListMutex);
    List_Remove((List_Links *)&arp);
    MASTER_UNLOCK(&arpListMutex);
    if (arp.state & ARP_IN_TIMEOUT_QUEUE) {
	Timer_DescheduleRoutine(&arp.timeout);
    }

    if ((arp.state & ARP_HAVE_INPUT) == 0) {
	arpStatistics.numTimeouts++;
	return(FAILURE);
    } else {
	*packetPtr = arp.packet;
	return(SUCCESS);
    }	
}

/*
 *----------------------------------------------------------------------
 *
 * NetArpInput --
 *
 *	Handler for the Address Resolution Protocol.  This looks through
 *	the list of arpState's to find a match between the Sprite IDs.
 *	Upon a match the physical address from the reply is stored in
 *	the state and the waiting process is notified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Copies the data from the arp reply into the arp state that
 *	was enqueued in arpList by Net_Arp.  The arpState is marked
 *	as having received input.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
NetArpInput(interPtr, packetPtr, packetLength)
    Net_Interface	*interPtr;	/* The interface that got the 
					 * packet. */
    Address packetPtr;		/* Pointer to the packet in the hardware
				 * recieve buffers */
    int packetLength;		/* Length of the packet */
{
    register NetSpriteArp *arpDataPtr;
    Boolean	forKernel = TRUE;
    short opcode, type;

    switch(interPtr->netType) {
	case NET_NETWORK_ETHER: {
	    arpDataPtr = (NetSpriteArp *)(packetPtr + sizeof(Net_EtherHdr));
	    break;
	}
	default :
	    return;
    }
    opcode = Net_NetToHostShort(arpDataPtr->arpHeader.opcode);
    type = Net_NetToHostShort(arpDataPtr->arpHeader.protocolType);
    /*
     * This packet is for the kernel ARP if the following 
     * condition are true:
     *	1) The hardwareType of the request is for the ethernet.
     *  2) The protocolType is ether for IP or Sprite type.
     *  3) The opcode is one we can handle (1 thru 4).
     */
    forKernel = (Net_NetToHostShort(arpDataPtr->arpHeader.hardwareType) == 
						NET_ARP_TYPE_ETHER)      &&
		 ((type == NET_ETHER_IP) || (type == NET_ETHER_SPRITE)) &&
		 ((opcode > 0) && (opcode < 5)) ;

    if (!forKernel) {
	return;
    }

    switch (opcode) {
	case NET_ARP_REQUEST: {
	    unsigned int id;
	    /*
	     * Received a request for the address corresponding to a
	     * sprite ID. Look to see if it is for us.
	     * If it is then we reply with that info.
	     */
	    bcopy(ARP_TARGET_PROTO_ADDR(arpDataPtr),(char *)&id,4);
	    id = Net_NetToHostInt(id);
	    if (arpDebug) {
		printf("Got ARP request for Sprite ID 0x%x\n", id);
	    }
	    if (type == NET_ETHER_SPRITE) { 
		forKernel = (id == rpc_SpriteID);
	    } else {
		forKernel = (id == interPtr->netAddress[NET_PROTO_INET].inet);
	    }
	    if (forKernel) { 
		/*
		 * We might overrun ourselves if we get a whole
		 * bunch of arp requests.  We synchronize, however,
		 * so that the call-back procedure sees a 
		 * consistent view.
		 */
		register ArpInputQueue *arpInputPtr;
		MASTER_LOCK(&arpInputMutex);
		arpInputPtr = &arpInputQueue[nextInputIndex];
		arpInputPtr->interPtr = interPtr;
		arpInputPtr->packet = *arpDataPtr;
		nextInputIndex = (nextInputIndex + 1) % ARP_INPUT_QUEUE_LEN;
		MASTER_UNLOCK(&arpInputMutex);
		Proc_CallFunc(NetArpHandler, (ClientData)arpInputPtr, 0);
	    }
	    break;
	}
	case NET_ARP_REPLY: {
	    ArpState *arpPtr;
	    unsigned int id;
	    Net_EtherAddress *targetEtherAddrPtr;

	    /*
	     * Make sure this REPLY is targeted for us.
	     */
	    targetEtherAddrPtr = 
			(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(arpDataPtr);
	    if (!NET_ETHER_COMPARE_PTR(
		    &interPtr->netAddress[NET_PROTO_RAW].ether,
		    targetEtherAddrPtr)) {
		break;
	    }

	    /*
	     * Look through the list of active arp requests for the one
	     * that matches this reply.  Then just copy the arp data to
	     * that state and notify the waiting process.
	     * Note: we'll probably get many replies to each arp request
	     * and only the first one updates the waiting process's arp
	     * state.  This is probably overly paranoid, but we don't want
	     * to be messing with things once we've notified the waiter.
	     */
	    bcopy(ARP_SRC_PROTO_ADDR(arpDataPtr),(char *)&id,4);
	    id = Net_NetToHostInt(id);
	    if (arpDebug) {
		printf("Got ARP reply for type %d Id 0x%x\n", type, id);
	    }
	    MASTER_LOCK(&arpListMutex);
	    LIST_FORALL(&arpList, (List_Links *)arpPtr) {
		if ((arpPtr->id == (ClientData) id) && (arpPtr->type == type)) {
		    if ((arpPtr->state & ARP_HAVE_INPUT) == 0) {
			arpPtr->packet = *arpDataPtr;
			arpPtr->state |= ARP_HAVE_INPUT;
			Sync_MasterBroadcast(&arpPtr->condition);
			if (arpDebug) {
			    printf("Woke ARP list type %d id %d\n",arpPtr->type,
				arpPtr->id);
			}
		    }
		}
	    }
	    MASTER_UNLOCK(&arpListMutex);
	    break;
	}
	case NET_RARP_REQUEST: {
	    /*
	     * Look in our route table for an entry with the ethernet
	     * address of the sender.  If one is found, return a reply
	     * containing the corresponding Sprite ID. The kernel only
	     * handles NET_ETHER_SPRITE requests.
	     */
	    int 	spriteID;
	    Net_Address netAddress;
	    if (type == NET_ETHER_SPRITE) { 
		NET_ETHER_ADDR_COPY(* (Net_EtherAddress *) 
		    ARP_TARGET_ETHER_ADDR(arpDataPtr),
		    netAddress.ether);

		spriteID = Net_AddrToID(interPtr->netType, NET_PROTO_RAW,
			&netAddress);
		if (arpDebug) {
		    printf("Got REV_ARP request for Sprite ID 0x%x\n",
			    spriteID);
		}
		if (spriteID > 0) {
		    register ArpInputQueue *arpInputPtr;
		    MASTER_LOCK(&arpInputMutex);
		    arpInputPtr = &arpInputQueue[nextInputIndex];
		    arpInputPtr->interPtr = interPtr;
		    arpInputPtr->packet = *arpDataPtr;
		    nextInputIndex = (nextInputIndex + 1) % 
						    ARP_INPUT_QUEUE_LEN;
		    MASTER_UNLOCK(&arpInputMutex);
		    Proc_CallFunc(NetArpHandler, (ClientData)arpInputPtr,0);
		}
	     }
	}
	case NET_RARP_REPLY: {
	    ArpState *arpPtr;
	    Net_EtherAddress *targetEtherAddrPtr;

	    targetEtherAddrPtr = 
			(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(arpDataPtr);
	    if (!NET_ETHER_COMPARE_PTR(
		    &interPtr->netAddress[NET_PROTO_RAW].ether,
		    targetEtherAddrPtr)) {
		break;
	    }
	    /*
	     * Make sure there is still a waiting process for this reply,
	     * then copy the reply into the waiting arp state.
	     */
	    if (arpDebug) {
		  printf("Got REV_ARP reply for type %d\n",type);
	    }
	    MASTER_LOCK(&arpListMutex);
	    LIST_FORALL(&revArpList, (List_Links *)arpPtr) {
		if ((arpPtr->type == type) && 
		    NET_ETHER_COMPARE_PTR(targetEtherAddrPtr,
				     (Net_EtherAddress *) (arpPtr->id))) {
		    if ((arpPtr->state & ARP_HAVE_INPUT) == 0) {
			arpPtr->packet = *arpDataPtr;
			arpPtr->state |= ARP_HAVE_INPUT;
			Sync_MasterBroadcast(&arpPtr->condition);
			if (arpDebug) {
			  printf("Woke REV_ARP reply for type %d\n",type);
			}
		    }
		}
	    }
	    MASTER_UNLOCK(&arpListMutex);
	    break;
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetArpHandler --
 *
 *	Routine to send an arp reply.  Called via Proc_CallFunc.
 *	This returns an ethernet address given a Sprite ID.  The
 *	interrupt handler has already checked in the netRouteArray for
 *	a good route for the spriteID.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates a  arp reply packet.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
NetArpHandler(data, callInfoPtr)
    ClientData data;		/* Pointer into arpInputQueue */
    Proc_CallInfo *callInfoPtr;
{
    Net_Interface	*interPtr;
    ArpInputQueue *arpInputPtr = (ArpInputQueue *)data;
    NetSpriteArp   *arpDataPtr, request;
    unsigned short opcode, type;

    MASTER_LOCK(&arpInputMutex);

    interPtr = arpInputPtr->interPtr;
    arpDataPtr = &arpInputPtr->packet;
    opcode = Net_NetToHostShort(arpDataPtr->arpHeader.opcode);
    type = Net_NetToHostShort(arpDataPtr->arpHeader.protocolType);

    if ((type != NET_ETHER_SPRITE) && (type != NET_ETHER_IP)) {
	MASTER_UNLOCK(&arpInputMutex);
	panic("Bad type %d in NetArpHandler\n", type);
	return;
    } 
    if (opcode == NET_ARP_REQUEST) {
	Net_Address	netAddress;

	NET_ETHER_ADDR_COPY(
	    * (Net_EtherAddress *) ARP_SRC_ETHER_ADDR(arpDataPtr), 
	    netAddress.ether);
	if (type == NET_ETHER_SPRITE) {
	    int	spriteID;
	    bcopy(ARP_SRC_PROTO_ADDR(arpDataPtr),(char*)&spriteID,sizeof(int));

	    NetFillInArpRequest(NET_ARP_REPLY, interPtr->netType,
		NET_PROTO_RAW,
		(ClientData) spriteID, (ClientData) rpc_SpriteID, 
		&netAddress, &interPtr->netAddress[NET_PROTO_RAW],
		&request);
	} else {
	    Net_Address		inetAddress;
	    Net_InetAddress inetAddr;
	    bcopy(ARP_SRC_PROTO_ADDR(arpDataPtr), (char *)&inetAddress.inet, 
			sizeof(inetAddr));

	    NetFillInArpRequest(NET_ARP_REPLY, interPtr->netType, 
		NET_PROTO_INET,
		(ClientData) inetAddress.inet, 
		(ClientData)  interPtr->netAddress[NET_PROTO_INET].inet, 
		&netAddress, &interPtr->netAddress[NET_PROTO_RAW],
		&request);
	}
        NetArpOutput(interPtr, &netAddress.ether, NET_ETHER_ARP, &request);
    } else if (opcode == NET_RARP_REQUEST) {
	Net_Address netAddress;
	int	spriteID;
	NET_ETHER_ADDR_COPY(
		*(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(arpDataPtr),
		netAddress.ether);
	spriteID = Net_AddrToID(interPtr->netType, NET_PROTO_RAW, &netAddress);
	if (spriteID > 0) { 
	    NetFillInArpRequest(NET_RARP_REPLY, interPtr->netType,
		    NET_PROTO_RAW,
		    (ClientData) spriteID, (ClientData) rpc_SpriteID,
		    &netAddress, &interPtr->netAddress[NET_PROTO_RAW],
		    &request);
	    NetArpOutput(interPtr, (Net_EtherAddress *) ARP_SRC_ETHER_ADDR(arpDataPtr),
			NET_ETHER_REVARP, &request);
	}
    } else {
	MASTER_UNLOCK(&arpInputMutex);
	panic ("Bad opcode %d in NetArpHandler\n", opcode);
	return;
    }
    MASTER_UNLOCK(&arpInputMutex);

    callInfoPtr->interval = 0;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetArpOutput --
 *
 *	Routine to send an arp packet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates a Sprite arp packet.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
NetArpOutput(interPtr, destEtherAddrPtr, etherType, requestPtr)
    Net_Interface	*interPtr;
    Net_EtherAddress *destEtherAddrPtr;	/* Host to send to */
    int		etherType;		/* Type of ethernet packet to send. */
    NetSpriteArp *requestPtr;		/* Request to send. */
{
    register Net_EtherHdr  *etherHdrPtr;
    register Net_ScatterGather *gatherPtr;
    register NetSpriteArp *packetPtr;

    if (interPtr->netType != NET_NETWORK_ETHER) {
	printf("NetArpOutput: invalid network type %d\n", interPtr->netType);
	return;
    }
    LOCK_MONITOR;


    etherHdrPtr = &arpOutputQueue[nextOutputIndex].etherHdr;
    packetPtr = &arpOutputQueue[nextOutputIndex].packet;
    gatherPtr = &arpOutputQueue[nextOutputIndex].gather;
    if (! gatherPtr->done) {
	printf("Warning: NetArpOutput can't queue packet");
	UNLOCK_MONITOR;
	return;
    }
    nextOutputIndex = (nextOutputIndex + 1) % ARP_OUTPUT_QUEUE_LEN;

    NET_ETHER_ADDR_COPY(*destEtherAddrPtr, 
			NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
    NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
		Net_HostToNetShort((unsigned short)etherType);
#ifdef sun4
    /*
     * Gcc for the sun4 currently allows these structures to be on unaligned
     * boundaries and then generates loads and stores as if they were aligned,
     * so I have to copy them byte by byte.
     */
     bcopy((char *)requestPtr, (char *)packetPtr, sizeof (NetSpriteArp));
#else
    *packetPtr = *requestPtr;
#endif sun4

    gatherPtr->bufAddr = (Address)packetPtr;
    gatherPtr->length = sizeof(NetSpriteArp);
    gatherPtr->done = FALSE;
    gatherPtr->mutexPtr = (Sync_Semaphore *) NIL;

    arpStatistics.numRevArpReplies++;
    (void) (interPtr->output)(interPtr, (Address) etherHdrPtr, gatherPtr, 1, 
	FALSE, (ReturnStatus *) NIL);

    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_ArpTimeout --
 *
 *	Timeout routine for ARP.  This just notifies the waiting process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Wakes up the process waiting for an ARP reply.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
Net_ArpTimeout(time, data)
    Timer_Ticks time;		/* The time we timed out at. */
    ClientData data;		/* Out private data is a pointer to the
				 * arp protocol state. */
{
    ArpState *arpPtr = (ArpState *)data;    

    MASTER_LOCK(arpPtr->mutexPtr);
    if (arpDebug) {
	printf("Arp timeout\n");
    }
    arpPtr->state &= ~ARP_IN_TIMEOUT_QUEUE;
    Sync_MasterBroadcast(&arpPtr->condition);
    MASTER_UNLOCK(arpPtr->mutexPtr);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetFillInArpRequest --
 *
 * 	Build a ARP or RARP packet in the provided request buffer.
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
NetFillInArpRequest(command, netType, protocol, targetId, senderId, 
    targetAddrPtr, senderAddrPtr, requestPtr)
    short	command;	/* ARP opcode to perform. */
    Net_NetworkType	netType;
    int			protocol;
    ClientData	targetId;	/* Target protocol address. */
    ClientData  senderId;	/* Sender's protocol ID. */
    Net_Address	*targetAddrPtr; /* Target network address. */
    Net_Address	*senderAddrPtr; /* Sender'network address. */
    NetSpriteArp *requestPtr;	/* Arp request packet to fill in. */
{
    unsigned int tid;
    unsigned int sid;

    switch (netType) {
	case NET_NETWORK_ETHER: {
	    requestPtr->arpHeader.hardwareType = 
		    Net_HostToNetShort(NET_ARP_TYPE_ETHER);
	    requestPtr->arpHeader.hardwareAddrLen = sizeof(Net_EtherAddress);
	    requestPtr->arpHeader.opcode = 
		    Net_HostToNetShort((unsigned short)command);
	    switch (protocol) {
		case NET_PROTO_RAW: {
		    requestPtr->arpHeader.protocolType = 
			    Net_HostToNetShort(NET_ETHER_SPRITE);
		    requestPtr->arpHeader.protocolAddrLen = sizeof(int);
		    break;
		}
		case NET_PROTO_INET: {
		    requestPtr->arpHeader.protocolType = 
			    Net_HostToNetShort(NET_ETHER_IP);
		    requestPtr->arpHeader.protocolAddrLen = 
			sizeof(Net_InetAddress);
		    break;
		}
		default:
		    panic("NetFillInArpRequest: Unknown protocol %d\n", 
			protocol);
	    }
	    NET_ETHER_ADDR_COPY(targetAddrPtr->ether,
			*(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(requestPtr));
	    NET_ETHER_ADDR_COPY(senderAddrPtr->ether,
			*(Net_EtherAddress *)ARP_SRC_ETHER_ADDR(requestPtr));
	    break;
	}
	default: {
	    panic("Warning: NetFillInArpRequest: bad netType %d\n", netType);
	}
    }
    tid = Net_HostToNetInt((unsigned int) targetId);
    sid = Net_HostToNetInt((unsigned int) senderId);
    bcopy((char *) &sid, ARP_SRC_PROTO_ADDR(requestPtr),sizeof(int));
    bcopy((char *) &tid, ARP_TARGET_PROTO_ADDR(requestPtr),sizeof(int));
    return;
}
