/* 
 * netRoute.c --
 *
 *	This is the code that maintains a table of routes to
 *	Sprite hosts.  The routes contain the transport level
 *	headers that need to be pre-pended to messages so that
 *	they get to the desired Sprite Host.
 *      Routes are added via a system call or determined dynamically using
 *      Sprite's version of ARP (Address Resolution Protocol).  ARP is
 *      invoked when a call to Net_Output is made with a SpriteID for
 *      which there is no route.  Its use is hidden within Net_Output
 *      should not affect the caller of Net_Output.
 *
 *	Operations on routes are:
 *		Net_InstallRoute - Set the Sprite ID for an ethernet address
 *		Net_AddrToID - Get the Sprite ID for an ethernet address
 *		Net_IDToRoute - Return the route for a Sprite host.
 *	Furthermore, the Test_Stats system call will return a route
 *	to a user program with the NET_GET_ROUTE command.
 *
 *	No synchronization is done here yet.  Routes don't generally
 *	change much, so the only race that exists is between an
 *	initialization in InstallRoute and a lookup in AddrToID.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"
#include "netRoute.h"
#include "netInt.h"
#include "sys.h"
#include "mem.h"
#include "timer.h"
#include "sync.h"

/*
 * A broadcast address that will be put into a broadcast route.
 */
Net_EtherAddress netBroadcastAddr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
 * The Route table. Invalid entries are NIL pointers.
 * The table is indexed by Sprite Host IDs.
 */
Net_Route	*netRouteArray[NET_NUM_SPRITE_HOSTS];
int	 	netNumRoutes = NET_NUM_SPRITE_HOSTS;

/*
 * This flag turns on print statements in the ARP protocol
 */
Boolean arpDebug = FALSE;

/*
 * State for the Address Resolution Protocol.
 */
typedef struct ArpState {
    List_Links		links;		/* Chain for all current ARPs */
    int			state;		/* See bits defined below */
    Timer_QueueElement	timeout;	/* Used for the call-back upon timeout*/
    Sync_Semaphore	*mutexPtr;	/* Used for synchronization */
    Sync_Condition	condition;	/* Used for synchronization */
    int			spriteID;	/* Target Sprite ID, used to identify
					 * this ARP transaction from others */
    NetSpriteArp	packet;		/* Copy of reply packet */
} ArpState;

#define ARP_WANT_REPLY		0x1
#define ARP_HAVE_INPUT		0x2
#define ARP_IN_TIMEOUT_QUEUE	0x4

typedef struct ArpStatisitics {
    int	numArpRequests;		/* Number of these transmitted */
    int numArpReplies;		/* ditto */
    int numRevArpRequests;	/* ditto */
    int numRevArpReplies;	/* ditto */
    int numTimeouts;		/* Number of requests that were not responded
				 * to after a few broadcast attempts */
} ArpStatistics;
ArpStatistics arpStatistics;

/*
 * Current ARP transactions have their state linked together in
 * a list so the packet handler can save the return address for
 * the sender.
 */
static List_Links arpList;
static List_Links revArpList;

/*
 * A very simple packet list used to keep Arp packet headers and
 * the scatter/gather structure used to access them.  These must be
 * global data structures because the packet may be queued before output.
 * A monitor is used to synchronize access
 */
typedef struct ArpOutputQueue {
    Net_EtherHdr	etherHdr;
    NetSpriteArp	packet;
    Net_ScatterGather	gather;
} ArpOutputQueue;

#define ARP_OUTPUT_QUEUE_LEN		3
ArpOutputQueue arpOutputQueue[ARP_OUTPUT_QUEUE_LEN];
static int nextOutputIndex = 0;

Sync_Lock arpOutputQueueLock = SYNC_LOCK_INIT_STATIC();
#define LOCKPTR (&arpOutputQueueLock)

/*
 * Another simple list of SpriteID pairs is used to pass info
 * from the interrupt handler that gets an ARP request to the
 * Proc_ServerProc that generates the reply.
 */
typedef struct ArpInputQueue {
    int			spriteID;
    Net_EtherAddress	destination;
} ArpInputQueue;

#define ARP_INPUT_QUEUE_LEN		5
ArpInputQueue arpInputQueue[ARP_INPUT_QUEUE_LEN];
static int nextInputIndex = 0;
Sync_Semaphore arpInputMutex = SYNC_SEM_INIT_STATIC("arpInputMutex");

void Net_ArpTimeout();
void NetArpOutput();
void NetRevArpHandler();
void NetArpHandler();

/*
 *----------------------------------------------------------------------
 *
 * Net_RouteInit --
 *
 *      Initialize the broadcast route.  The rest of the routes are
 *      installed via the netRoute user program.
 *
 *	This uses malloc, so it should be called after Mem_Init.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initialize the route table and add the broadcast route.
 *
 *----------------------------------------------------------------------
 */
void
Net_RouteInit()
{
    register int i;
    int spriteID;

    /*
     * The route table.  An array of routes is kept for directing
     * messages to other machines.  It is indexed by Sprite host ids.
     */
    for (spriteID=0 ; spriteID<netNumRoutes ; spriteID++) {
	netRouteArray[spriteID] = (Net_Route *)NIL;
    }
    /*
     * Mark the arp output queue as all done (output).
     */
    for (i=0; i<ARP_OUTPUT_QUEUE_LEN ; i++) {
	arpOutputQueue[i].gather.done = TRUE;
    }
    List_Init(&arpList);
    List_Init(&revArpList);

    /*
     * Install the broadcast route so we can do our first broadcast rpcs.
     */
    (void) Net_InstallRoute(NET_BROADCAST_HOSTID, NET_ROUTE_BROAD, 
			    NET_ROUTE_ETHER,
			   (ClientData) &netBroadcastAddr, "broadcast",
			   "unknown");
}

/*
 *----------------------------------------------------------------------
 *
 * Net_InstallRouteStub --	
 *
 *	System call stub for Net_InstallRoute
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	See Net_InstallRoute
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_InstallRouteStub(spriteID, flags, type, clientData, name, machType)
    int spriteID;		/* Sprite Host ID */
    int flags;			/* NET_ROUTE_BROAD */
    int type;			/* NET_ROUTE_ETHER */
    ClientData clientData;	/* Route data */
    char *name;		/* The human understandable name of the host.  This
			 * only gets used in error messages to the console. */
    char *machType;	/* Machine type.  This is kept here for convenience,
			 * and returned to the file system so it can
			 * expand $MACHINE in pathnames. */
{
    ReturnStatus status;
    ClientData localData = (ClientData)NIL;
    char hostname[128];
    char machineType[128];
    int length;

    if (Vm_StringNCopy(128, name, hostname, &length) != SUCCESS) {
	printf("Warning: Net_InstallRoute: bad name arg\n");
	return(SYS_ARG_NOACCESS);
    }
    hostname[127] = '\0';
    switch(type) {
	case NET_ROUTE_ETHER: {
	    localData = (ClientData)malloc(sizeof(Net_EtherAddress));
	    status = Vm_CopyIn(sizeof(Net_EtherAddress), (Address)clientData,
				      (Address)localData);
	    if (status != SUCCESS) {
		free((Address)localData);
		return(status);
	    }
	    break;
	}
	default:
	    printf("Warning: Net_InstallRoute: bad route type %d\n", type);
	    return(GEN_INVALID_ARG);
    }
    if (Vm_StringNCopy(128, machType, machineType, &length) != SUCCESS) {
	printf("Warning: Net_InstallRoute: bad machType arg\n");
	(void) strcpy(machineType,"sun3");
    }
    machineType[127] = '\0';
    status = Net_InstallRoute(spriteID, flags, type, localData,
		    hostname, machineType);
    if (localData != (ClientData)NIL) {
	free((Address)localData);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_InstallRoute --
 *
 *	Install a route to a Sprite host at the specified address.
 *	After installing the route the Sprite ID can be used as an
 *	argument to Rpc_Call to direct an RPC request to that Sprite host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initialize the entry in the route table which is indexed
 *	by the Sprite ID to point to a route for the Sprite Host.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_InstallRoute(spriteID, flags, type, clientData, hostname, machType)
    int spriteID;		/* Sprite Host ID */
    int flags;			/* NET_ROUTE_BROAD */
    int type;			/* NET_ROUTE_ETHER */
    ClientData clientData;	/* Route data */
    char *hostname;		/* Human recognizable name */
    char *machType;		/* Machine type used to expand $MACHINE */
{
    register Net_Route *routePtr;
    
    if (spriteID < 0 || spriteID >= NET_NUM_SPRITE_HOSTS) {
	return(SYS_INVALID_ARG);
    }
    routePtr = netRouteArray[spriteID];
    if (routePtr != (Net_Route *)NIL) {
	if (routePtr->data != (Address)NIL) {
	    free((Address)routePtr->data);
	}
	if (routePtr->name != (char *)NIL) {
	    free((Address) routePtr->name);
	}
	if (routePtr->machType != (char *)NIL) {
	    free((Address) routePtr->machType);
	}
	free((Address)routePtr);
    }
    routePtr = (Net_Route *)malloc(sizeof(Net_Route));
    netRouteArray[spriteID] = routePtr;
    routePtr->name = (char *)malloc(strlen(hostname) + 1);
    (void) strcpy(routePtr->name, hostname);
    routePtr->machType = (char *)malloc(strlen(machType) + 1);
    (void) strcpy(routePtr->machType, machType);
    /*
     * Prepare the Route.  This includes the transport header that
     * will be used in messages sent to the Sprite Host.
     */
    routePtr->flags = flags;
    routePtr->spriteID = spriteID;
    routePtr->type = type;
    switch(type) {
	case NET_ROUTE_ETHER: {
	    Net_EtherHdr *etherHdrPtr;
	    /*
	     * The route data for a raw ethernet route is an ethernet
	     * header that the network driver can use directly.  The
	     * drivers fill in the source part of the ethernet header each
	     * time they send out a packet.
	     */
	    etherHdrPtr = (Net_EtherHdr *)malloc(sizeof(Net_EtherHdr));
	    NET_ETHER_ADDR_COPY(*(Net_EtherAddress *)clientData,
				NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
	    NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
			Net_HostToNetShort(NET_ETHER_SPRITE);
	    routePtr->data = (Address)etherHdrPtr;
	    break;
	}
	default: {
	    routePtr->data = (Address)NIL;
	    printf("Warning: Unsupported route type in Net_InstallRoute\n");
	    break;
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_IDToRouteStub --
 *
 *	Stub for the Test_Stats system call, NET_GET_ROUTE command.
 *	This gets a route and copies it out to user space.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Copies to user space.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_IDToRouteStub(spriteID, argPtr)
    int spriteID;		/* option parameter to Test_Stats */
    Address argPtr;		/* User space buffer to hold route */
{
    ReturnStatus status;
    Net_Route *routePtr;
    Net_Route unusedRoute;

    if (spriteID < 0 || spriteID >= netNumRoutes) {
	return(SYS_INVALID_ARG);
    }
    routePtr = Net_IDToRoute(spriteID);
    if (routePtr == (Net_Route *)NIL) {
	unusedRoute.flags = 0;
	unusedRoute.spriteID = -1;
	unusedRoute.type = NET_ROUTE_UNUSED;
	routePtr = &unusedRoute;
    }
    status = Vm_CopyOut(3 * sizeof(int), (Address)routePtr, argPtr);
    switch(routePtr->type) {
	case NET_ROUTE_ETHER:
	    status = Vm_CopyOut(sizeof(Net_EtherHdr), (Address)routePtr->data,
			     (Address)((int)argPtr + 3 * sizeof(int)));
	    break;
	default:
	    break;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_IDToRoute --
 *
 *	Return the route to the host specified by the input sprite id.
 *
 * Results:
 *	A pointer to the route for the host.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Net_Route *
Net_IDToRoute(spriteID)
    int spriteID;
{
    /*
     * Synchronization with RouteInstall etc?
     */
    if (spriteID < 0 || spriteID >= netNumRoutes) {
	return((Net_Route *)NIL);
    }
    return(netRouteArray[spriteID]);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_AddrToID --
 *
 *      Determine the Sprite host ID from a physical address.  This is
 *      used by a server, or Reverse Arp, to determine a client's Sprite
 *      ID from the client's network address.
 *
 *      This routine scans the route table looking for an address
 *      match with the input address.  If the flags field of the route
 *      specifies that the address is a broadcast address, this will
 *      return the correct hostid to use for a broadcast.
 *
 * Results:
 *      A Sprite hostid for the host at the address.  If the physical
 *      address isn't in the table we return a hostid of -1.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Net_AddrToID(flags, type, routeData)
    int flags;				/* 0 or NET_ROUTE_BROAD */
    int type;				/* NET_ROUTE_ETHER */
    ClientData routeData;		/* Route type specific address */
{
    register Net_Route *routePtr;
    register int i;

    /*
     * Monitor?
     */
    
    for (i=0 ; i<netNumRoutes ; i++) {
	routePtr = netRouteArray[i];
	if (routePtr == (Net_Route *)NIL) {
	    continue;
	}
	if (type == routePtr->type) {
	    /*
	     * Look for the broadcast flag first so our caller
	     * doesn't have to know the value of the broadcast address.
	     */
	    if ((flags & NET_ROUTE_BROAD) &&
	        (routePtr->flags & NET_ROUTE_BROAD)) {
		return(routePtr->spriteID);
	    }
	    /*
	     * The address types of the table and input match,
	     * look for an address value match.
	     */
	    switch (type) {
		case NET_ROUTE_ETHER: {
		    Net_EtherHdr *etherHdrPtr;
		    Net_EtherAddress *etherAddrPtr;

		    etherHdrPtr = (Net_EtherHdr *)routePtr->data;
		    etherAddrPtr = (Net_EtherAddress *)routeData;
		    if (NET_ETHER_COMPARE_PTR(etherAddrPtr,
				 &(NET_ETHER_HDR_DESTINATION(*etherHdrPtr)))){
			return(routePtr->spriteID);
		    }
		    break;
		}
		default:
		    /*
		     * Don't find other kinds of routes
		     */
		    break;
	    }
	}
    }
    /*
     * We didn't find an address match on a route.
     */
    return(-1);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_EtherCompare --
 *
 *	Compare two ethernet addresses to see if they are the same.
 *
 * Results:
 *	TRUE if the two addresses are the same, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Net_EtherCompare(addrPtr1, addrPtr2)
    Net_EtherAddress *addrPtr1, *addrPtr2;
{
    return(NET_ETHER_COMPARE_PTR(addrPtr1, addrPtr2));
}
/*
 *----------------------------------------------------------------------------
 *
 * Net_AddrToName --
 *
 *	Map from an ethernet addresses to a host name.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------------
 */

void
Net_AddrToName(etherAddressPtr, namePtrPtr)
    Net_EtherAddress *etherAddressPtr;
    char **namePtrPtr;
{
    register Net_Route *routePtr;
    register int i;

    /*
     * Monitor?
     */
    
    for (i=0 ; i<netNumRoutes ; i++) {
	routePtr = netRouteArray[i];
	if (routePtr == (Net_Route *)NIL) {
	    continue;
	}
	if (routePtr->type == NET_ROUTE_ETHER) {
	    Net_EtherHdr *etherHdrPtr = (Net_EtherHdr *)routePtr->data;
	    if (NET_ETHER_COMPARE_PTR(
			&NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
			etherAddressPtr)) {
		*namePtrPtr = routePtr->name;
		return;
	    }
	}
    }
    *namePtrPtr = (char *) NIL;
    return;
}

/*
 *----------------------------------------------------------------------------
 *
 * Net_SpriteIDToName --
 *
 *	Map from a Sprite ID to a host name.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------------
 */

void
Net_SpriteIDToName(spriteID, namePtrPtr)
    int spriteID;
    char **namePtrPtr;
{
    register Net_Route *routePtr;

    /*
     * Monitor?
     */
    *namePtrPtr = (char *) NIL;
    if (spriteID >= 0 && spriteID < netNumRoutes) {
	routePtr = netRouteArray[spriteID];
	if (routePtr != (Net_Route *)NIL) {
	    *namePtrPtr = routePtr->name;
	}
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * Net_SpriteIDToMachType --
 *
 *	Map from a Sprite ID to a machine type string.  This string is
 *	used by the filesystem when expanding $MACHINE in pathnames.
 *
 * Results:
 *      A pointer to a string identifying the machine type, i.e. "sun3".
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------------
 */

char *
Net_SpriteIDToMachType(spriteID)
    int spriteID;
{
    register Net_Route *routePtr;
    register char *machTypeString;

    /*
     * Monitor?
     */
    machTypeString = (char *)NIL;
    if (spriteID >= 0 && spriteID < netNumRoutes) {
	routePtr = netRouteArray[spriteID];
	if (routePtr != (Net_Route *)NIL) {
	    machTypeString = routePtr->machType;
	}
    }
    return(machTypeString);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_HostPrint --
 *
 *	Print out a statement concerning a host.  This maps to a
 *	string hostname if possible, and prints out the message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	printf.
 *
 *----------------------------------------------------------------------
 */

void
Net_HostPrint(spriteID, string)
    int spriteID;
    char *string;
{
    Sys_HostPrint(spriteID, string);
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
 *	A pointer to a route.
 *
 * Side effects:
 *	A broadcast, and the route table is updated.
 *
 *----------------------------------------------------------------------
 */

Net_Route *
Net_Arp(spriteID, mutexPtr)
    int spriteID;			/* Sprite ID to find the route for */
    int *mutexPtr;			/* Address of the mutex that the
					 * caller of Net_Output used for
					 * synchronization.  This needs to
					 * be released during the ARP so that
					 * we can receive our reply. */
{
    ReturnStatus status;
    NetSpriteArp request;		/* The Sprite ARP request packet data */
    NetSpriteArp reply;			/* The Sprite ARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */

    request.flags = NET_SPRITE_ARP_REQUEST;
    request.spriteHostID = spriteID;
    gather.bufAddr = (Address)&request;
    gather.length = sizeof(NetSpriteArp);
    gather.done = FALSE;
    gather.conditionPtr = (Sync_Condition *) NIL;
    
    status = NetDoArp(mutexPtr, NET_SPRITE_ARP_REQUEST, &gather, &reply);
    if (status == SUCCESS) {
	(void) Net_InstallRoute(spriteID, 0, NET_ROUTE_ETHER, 
				(ClientData) &reply.etherAddr,
				"noname", "unknown");
	return(netRouteArray[spriteID]);
    } else {
	return((Net_Route *)NIL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetRevArp --
 *
 *      Sprite's Reverse Address Resolution Protocol.  Broadcast a Sprite
 *      Reverse ARP packet to find the Sprite ID that corresponds to our
 *      ethernet adddress.
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
Net_RevArp(etherAddrPtr)
    Net_EtherAddress *etherAddrPtr;	/* Physical address to map to Sprite
   					 * ID */
{
    ReturnStatus status;
    NetSpriteArp request;		/* Sprite RARP request packet data */
    NetSpriteArp reply;			/* Sprite RARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */
    static Sync_Semaphore mutex =
	SYNC_SEM_INIT_STATIC("Net_RevArp.mutex"); /* Reverse arp is called
					 * during initialization when there is
					 * no mutex held (unlike regular arp)
					 * so we need our own mutex for sync */

    request.flags = NET_SPRITE_REV_ARP_REQUEST;
    request.spriteHostID = 0;
    NET_ETHER_ADDR_COPY(*etherAddrPtr,request.etherAddr);
    gather.bufAddr = (Address)&request;
    gather.length = sizeof(NetSpriteArp);
    gather.done = FALSE;
    gather.conditionPtr = (Sync_Condition *) NIL;

    MASTER_LOCK(&mutex);
    status = NetDoArp(&mutex, NET_SPRITE_REV_ARP_REQUEST, &gather, &reply);
    MASTER_UNLOCK(&mutex);
    if (status == SUCCESS) {
	return(reply.spriteHostID);
    } else {
	return(-1);
    }
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
NetDoArp(mutexPtr, command, gatherPtr, packetPtr)
    Sync_Semaphore *mutexPtr;		/* Address of the mutex that the
					 * caller of Net_Output used for
					 * synchronization.  This needs to
					 * be released during the ARP so that
					 * we can receive our reply. */
    int command;			/* NET_SPRITE_ARP_REQUEST or
					 * NET_SPRITE_REV_ARP_REQUEST */
    Net_ScatterGather *gatherPtr;	/* Specifies the output packet */
    NetSpriteArp *packetPtr;		/* Filled in with the reply packet */
{
    register Net_Route *routePtr;	/* Route for RPC broadcast */
    register Net_EtherHdr *etherHdrPtr;	/* Regular RPC broadcast header */
    Net_EtherHdr etherHdr;		/* Header for Sprite ARP packet */
    int retries = 0;			/* Retry counter */
    ArpState arp;			/* State for the protocol */
    NetSpriteArp *requestPtr;		/* Pointer to request data */
    List_Links *listPtr;		/* Either arpList or revArpList */

    if (mutexPtr == (Sync_Semaphore *)NIL) {
	return(FAILURE);
    }
    /*
     * Set up the ethernet header for the Arp request packet.  We can't
     * use the regular broadcast route because the ethernet protocol type
     * is different, Sprite Arp.  The broadcast destination address,
     * however, is obtained from the regular broadcast route.
     */
    routePtr = netRouteArray[NET_BROADCAST_HOSTID];
    etherHdrPtr = (Net_EtherHdr *)routePtr->data;
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
			NET_ETHER_HDR_DESTINATION(etherHdr));
    NET_ETHER_HDR_TYPE(etherHdr) = 
			Net_HostToNetShort(NET_ETHER_SPRITE_ARP);

    /*
     * Use a simple retry loop to get our reply.  The ARP protocol state
     * is set up and put on a list so the packet handler can find it.
     */

    requestPtr = (NetSpriteArp *)gatherPtr->bufAddr;
    arp.state = ARP_WANT_REPLY;
    arp.mutexPtr = mutexPtr;
    arp.spriteID = requestPtr->spriteHostID;
    if (command == NET_SPRITE_ARP_REQUEST) {
	listPtr = &arpList;
    } else {
	listPtr = &revArpList;
    }
    List_InitElement((List_Links *) &arp);
    List_Insert((List_Links *)&arp, LIST_ATREAR(listPtr));

    requestPtr->spriteHostID = Net_HostToNetInt(requestPtr->spriteHostID);
    requestPtr->flags = Net_HostToNetInt(requestPtr->flags);
    while (retries < 4 && ((arp.state & ARP_HAVE_INPUT) == 0)) {
	retries++;
	if (command == NET_SPRITE_ARP_REQUEST) {
	    if (arpDebug) {
		printf("Sending arp request\n");
	    }
	    arpStatistics.numArpRequests++;
	} else {
	    if (arpDebug) {
		printf("Sending rev arp request\n");
	    }
	    arpStatistics.numRevArpRequests++;
	}
	(netEtherFuncs.output)(&etherHdr, gatherPtr, 1);

	arp.state |= ARP_IN_TIMEOUT_QUEUE ;
	arp.timeout.routine = Net_ArpTimeout;
	arp.timeout.interval = 100 * timer_IntOneMillisecond;
	arp.timeout.clientData = (ClientData)&arp;
	Timer_ScheduleRoutine(&arp.timeout, TRUE);
	do {
	    Sync_MasterWait(&arp.condition, arp.mutexPtr, FALSE);
	} while (((arp.state & ARP_HAVE_INPUT) == 0) &&
		 (arp.state & ARP_IN_TIMEOUT_QUEUE));
    }
    requestPtr->spriteHostID = Net_NetToHostInt(requestPtr->spriteHostID);
    requestPtr->flags = Net_NetToHostInt(requestPtr->flags);
    List_Remove((List_Links *)&arp);
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
int
NetArpInput(packetPtr, packetLength)
    Address packetPtr;		/* Pointer to the packet in the hardware
				 * recieve buffers */
    int packetLength;		/* Length of the packet */
{
    register Net_EtherHdr *inputEtherHdrPtr = (Net_EtherHdr *)packetPtr;
    register NetSpriteArp *arpDataPtr;

    arpDataPtr = (NetSpriteArp *)(packetPtr + sizeof(Net_EtherHdr));
    arpDataPtr->flags = Net_NetToHostInt(arpDataPtr->flags);
    arpDataPtr->spriteHostID = Net_NetToHostInt(arpDataPtr->spriteHostID);
    switch (arpDataPtr->flags) {
	case NET_SPRITE_ARP_REQUEST: {
	    /*
	     * Received a request for the address corresponding to a
	     * sprite ID.  Look in our own route table to see if we know.
	     * If we do then we reply with that info.
	     */
	    if (arpDebug) {
		printf("Got ARP request for Sprite ID %d\n",
				arpDataPtr->spriteHostID);
	    }
	    if (arpDataPtr->spriteHostID > 0 &&
		arpDataPtr->spriteHostID < NET_NUM_SPRITE_HOSTS) {
		Net_Route *routePtr;
		routePtr = netRouteArray[arpDataPtr->spriteHostID];
		if (routePtr != (Net_Route *)NIL) {
		    /*
		     * We might overrun ourselves if we get a whole
		     * bunch of arp requests.  We synchronize, however,
		     * so that the call-back procedure sees a consistent view.
		     */
		    register ArpInputQueue *arpInputPtr;
		    MASTER_LOCK(&arpInputMutex);
		    arpInputPtr = &arpInputQueue[nextInputIndex];
		    arpInputPtr->spriteID = arpDataPtr->spriteHostID;
		    NET_ETHER_ADDR_COPY(
			    NET_ETHER_HDR_DESTINATION(*inputEtherHdrPtr),
			    arpInputPtr->destination);
		    nextInputIndex = (nextInputIndex + 1) % ARP_INPUT_QUEUE_LEN;
		    MASTER_UNLOCK(&arpInputMutex);
		    Proc_CallFunc(NetArpHandler, (ClientData)arpInputPtr, 0);
		}
	    }
	    break;
	}
	case NET_SPRITE_ARP_REPLY: {
	    ArpState *arpPtr;
	    /*
	     * Look through the list of active arp requests for the one
	     * that matches this reply.  Then just copy the arp data to
	     * that state and notify the waiting process.
	     * Note: we'll probably get many replies to each arp request
	     * and only the first one updates the waiting process's arp
	     * state.  This is probably overly paranoid, but we don't want
	     * to be messing with things once we've notified the waiter.
	     */
	    if (arpDebug) {
		printf("Got ARP reply for Sprite ID %d\n",
				arpDataPtr->spriteHostID);
	    }
	    LIST_FORALL(&arpList, (List_Links *)arpPtr) {
		if (arpPtr->spriteID == arpDataPtr->spriteHostID) {
		    if ((arpPtr->state & ARP_HAVE_INPUT) == 0) {
			arpPtr->packet = *arpDataPtr;
			arpPtr->state |= ARP_HAVE_INPUT;
			Sync_MasterBroadcast(&arpPtr->condition);
		    }
		}
	    }
	    break;
	}
	case NET_SPRITE_REV_ARP_REQUEST: {
	    /*
	     * Look in our route table for an entry with the ethernet
	     * address of the sender.  If one is found, return a reply
	     * containing the corresponding Sprite ID.
	     */
	    int spriteID;

	    spriteID = Net_AddrToID(0, NET_ROUTE_ETHER, 
					(ClientData) &arpDataPtr->etherAddr);
	    if (arpDebug) {
		printf("Got REV_ARP request for Sprite ID %d\n",
				arpDataPtr->spriteHostID);
	    }
	    if (spriteID > 0) {
		Proc_CallFunc(NetRevArpHandler, spriteID, 0);
	    }
	}
	case NET_SPRITE_REV_ARP_REPLY: {
	    ArpState *arpPtr;
	    /*
	     * Make sure there is still a waiting process for this reply,
	     * then copy the reply into the waiting arp state.
	     */
	    if (arpDebug) {
		printf("Got REV_ARP reply for Sprite ID %d",
			    arpDataPtr->spriteHostID);
	    }
	    LIST_FORALL(&revArpList, (List_Links *)arpPtr) {
		if ((arpPtr->state & ARP_HAVE_INPUT) == 0) {
		    arpPtr->packet = *arpDataPtr;
		    arpPtr->state |= ARP_HAVE_INPUT;
		    Sync_MasterBroadcast(&arpPtr->condition);
		}
	    }
	    break;
	}
    }
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
 *	Generates a Sprite arp reply packet.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
NetArpHandler(data, callInfoPtr)
    ClientData data;		/* Pointer into arpInputQueue */
    Proc_CallInfo *callInfoPtr;
{
    ArpInputQueue *arpInputPtr = (ArpInputQueue *)data;
    Net_EtherAddress destination;
    int spriteID;

    MASTER_LOCK(&arpInputMutex);
    NET_ETHER_ADDR_COPY(arpInputPtr->destination,destination);
    spriteID = arpInputPtr->spriteID;
    MASTER_UNLOCK(&arpInputMutex);

    NetArpOutput(spriteID, &destination, NET_SPRITE_ARP_REPLY);
    callInfoPtr->interval = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * NetRevArpHandler --
 *
 *	Routine to send a reverse arp reply.  Called via Proc_CallFunc.
 *	Reverse arp returns a Sprite ID given an ethernet address.  The
 *	interrupt handler has already checked in the netRouteArray for
 *	a match between the sender's address and a spriteID.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates a Sprite reverse arp packet.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
NetRevArpHandler(data, callInfoPtr)
    ClientData data;		/* Our private data is sprite ID */
    Proc_CallInfo *callInfoPtr;
{
    int spriteID = (int)data;

    callInfoPtr->interval = 0;
    NetArpOutput(spriteID, (Net_EtherAddress *)NIL, NET_SPRITE_REV_ARP_REPLY);
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
ENTRY void
NetArpOutput(spriteID, destPtr, flags)
    int spriteID;			/* SpriteID in question */
    Net_EtherAddress *destPtr;		/* Host to send to */
    int flags;				/* ARP packet type */
{
    register NetSpriteArp *packetPtr;
    register Net_EtherHdr *etherHdrPtr;
    register Net_EtherHdr *routeEtherHdrPtr;
    register Net_ScatterGather *gatherPtr;
    register Net_Route *routePtr;

    LOCK_MONITOR;

    routePtr = netRouteArray[spriteID];
    if (routePtr == (Net_Route *)NIL ||
	routePtr->data == (Address)NIL) {
	UNLOCK_MONITOR;
	return;
    }
    routeEtherHdrPtr = (Net_EtherHdr *)routePtr->data;

    etherHdrPtr = &arpOutputQueue[nextOutputIndex].etherHdr;
    packetPtr = &arpOutputQueue[nextOutputIndex].packet;
    gatherPtr = &arpOutputQueue[nextOutputIndex].gather;
    if (! gatherPtr->done) {
	printf("Warning: NetArpOutput can't queue packet");
	UNLOCK_MONITOR;
	return;
    }
    nextOutputIndex = (nextOutputIndex + 1) % ARP_OUTPUT_QUEUE_LEN;

    packetPtr->flags = flags;
    packetPtr->spriteHostID = spriteID;
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*routeEtherHdrPtr), 
			packetPtr->etherAddr);

    gatherPtr->bufAddr = (Address)packetPtr;
    gatherPtr->length = sizeof(NetSpriteArp);
    gatherPtr->done = FALSE;
    gatherPtr->conditionPtr = (Sync_Condition *) NIL;

    /*
     * The destination comes from the route for reverse arp, destPtr is NIL.
     * For regular arp the destination is the sender of the request.
     */
    if (destPtr == (Net_EtherAddress *)NIL) {
	NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*routeEtherHdrPtr),
			    NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
    } else {
	NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*routeEtherHdrPtr),
			    *destPtr);
    }
    NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
			Net_HostToNetShort(NET_ETHER_SPRITE_ARP);

    if (arpDebug) {
	printf("Sending%sARP reply for Sprite ID %d\n",
	    flags == NET_SPRITE_REV_ARP_REPLY ? " REV_" : " ", spriteID);
    }
    arpStatistics.numRevArpReplies++;
    (netEtherFuncs.output)(etherHdrPtr, gatherPtr, 1);

    UNLOCK_MONITOR;
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
void
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
}
