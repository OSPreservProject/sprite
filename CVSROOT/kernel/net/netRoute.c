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
#include "stdlib.h"
#include "timer.h"
#include "sync.h"
#include "rpc.h"
#include "netInet.h"

/*
 * A broadcast address that will be put into a broadcast route.
 */
Net_EtherAddress netBroadcastAddr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
/*
 * This host's internet address. This needs to be kept per interface.
 * A value of NET_INET_ANY_ADDR means it hasn't been set yet.
 */
Net_InetAddress net_InetAddress = NET_INET_ANY_ADDR;

/*
 * The Route table. Invalid entries are NIL pointers.
 * The table is indexed by Sprite Host IDs.  Access to
 * the table is synchronized with a MASTER_LOCK to prevent
 * changes from interfering with packet handling.
 */
Net_Route	*netRouteArray[NET_NUM_SPRITE_HOSTS];
int	 	netNumRoutes = NET_NUM_SPRITE_HOSTS;
Sync_Semaphore	netRouteMutex = Sync_SemInitStatic("netRouteMutex");

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
    int			type;		/* Type of request. */
    ClientData		id;		/* Target ID, used to identify
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

Sync_Lock arpOutputQueueLock; ;
#define LOCKPTR (&arpOutputQueueLock)

/*
 * Another simple list of Arp packets used to pass info
 * from the interrupt handler that gets an ARP request to the
 * Proc_ServerProc that generates the reply.
 */
typedef NetSpriteArp  ArpInputQueue;

#define ARP_INPUT_QUEUE_LEN		5
ArpInputQueue arpInputQueue[ARP_INPUT_QUEUE_LEN];
static int nextInputIndex = 0;
Sync_Semaphore arpInputMutex;

static void Net_ArpTimeout();
static void NetArpOutput();
static void NetRevArpHandler();
static void NetArpHandler();
static void NetFillInArpRequest();

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

    Sync_LockInitDynamic(&arpOutputQueueLock, "Net:arpOutputQueueLock");
    Sync_SemInitDynamic(&arpInputMutex, "Net:arpInputMutex");
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
	    /*
	     * An ethernet route consists of a single ethernet address.
	     */
	    localData = (ClientData)malloc(sizeof(Net_EtherAddress));
	    status = Vm_CopyIn(sizeof(Net_EtherAddress), (Address)clientData,
				      (Address)localData);
	    if (status != SUCCESS) {
		free((Address)localData);
		return(status);
	    }
	    break;
	}
	case NET_ROUTE_INET: {
	    /*
	     * An internet route consists of internet address followed by
	     * the ethernet address of the gateway machine.
	     */
	    localData = (ClientData)malloc(sizeof(NetInetRoute));
	    status = Vm_CopyIn(sizeof(NetInetRoute), (Address)clientData, 
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
    Address oldData;

    /*
     * If we are installing a route and we don't know are own spriteID see
     * if we can learn it from the route.
     */
    if ((rpc_SpriteID == 0) && (type == NET_ROUTE_ETHER)) {
	Net_EtherAddress	etherAddress;
        Mach_GetEtherAddress(&etherAddress);

	if (NET_ETHER_COMPARE(etherAddress,*(Net_EtherAddress *)clientData)) {
		rpc_SpriteID = spriteID;
	}
    } 
    /*
     * If we know our spriteID and we are installing a route to ourselves we
     * can learn (or at least validate our data).
     */
    if ((rpc_SpriteID > 0) && (rpc_SpriteID == spriteID)) {
	Net_EtherAddress	etherAddress;

	Mach_GetEtherAddress(&etherAddress);
	if (type == NET_ROUTE_ETHER) {
	    if (!NET_ETHER_COMPARE(etherAddress,
		    *(Net_EtherAddress *)clientData)) {
		printf("Warning: Route to SpriteID %d is incorrect.\n",
			spriteID);
	       return FAILURE;
	    }
	} else if (type == NET_ROUTE_INET) {
	    NetInetRoute 	*inetRoute= (NetInetRoute *)clientData;
	    char	buffer[128];
	    if (net_InetAddress == NET_INET_ANY_ADDR)  { 
		net_InetAddress = inetRoute->inetAddr;
		Net_InetAddrToString(net_InetAddress, buffer);
		printf("Setting internet address to %s\n",buffer);
	    } else {
		if (net_InetAddress != inetRoute->inetAddr) {
		    printf(
		    "Warning: Internet address for SpriteID %d is incorrect.\n",
		    spriteID);
		   return FAILURE;
		}

	    }
	    return SUCCESS;
	}
    }

    /*
     * In order to install INET routes our net_InetAddress must be set.
     */
    if ((type == NET_ROUTE_INET) && (net_InetAddress == NET_INET_ANY_ADDR)) {
	Net_EtherAddress	etherAddress;
	Net_InetAddress		addr;

	/*
	 * Try to use RevArp to determine our inet address.
	 */
	Mach_GetEtherAddress(&etherAddress);
	addr = Net_RevArp(NET_ROUTE_INET,&etherAddress);
	if (addr != (Net_InetAddress) -1) { 
	    char	buffer[128];
	    net_InetAddress = addr;
	    Net_InetAddrToString(net_InetAddress, buffer);
	    printf("Setting internet address to %s\n",buffer);
	} else {
	    printf("Warning: RARP failed, Can't install INET route to %d\n",
		  spriteID);
	    return FAILURE;
	}
    }

   MASTER_LOCK(&netRouteMutex);

    if (spriteID < 0 || spriteID >= NET_NUM_SPRITE_HOSTS) {
	return FAILURE;
    } else {
	routePtr = netRouteArray[spriteID];
	if (routePtr != (Net_Route *)NIL) {
	    oldData = routePtr->data;
	} else {
	    oldData = (Address)NIL;
	    routePtr = (Net_Route *)malloc(sizeof(Net_Route));
	    netRouteArray[spriteID] = routePtr;
	}
	/*
	 * Silently discard old name and machine types.  We'll holler below
	 * if the ethernet address changes.  Names and types may be set
	 * to "noname" and "unknown" during bootstrap routing.
	 */
	routePtr->name = (char *)malloc(strlen(hostname) + 1);
	(void) strcpy(routePtr->name, hostname);
	routePtr->machType = (char *)malloc(strlen(machType) + 1);
	(void) strcpy(routePtr->machType, machType);

	routePtr->flags = flags;
	routePtr->spriteID = spriteID;
	routePtr->type = type;
    }
    /*
     * Prepare the Route.  This includes the transport header that
     * will be used in messages sent to the Sprite Host.
     */
    switch(type) {
	case NET_ROUTE_ETHER: {
	    Net_EtherHdr *etherHdrPtr;
	    /*
	     * The route data for a raw ethernet route is an ethernet
	     * header that the network driver can use directly.  The
	     * drivers fill in the source part of the ethernet header each
	     * time they send out a packet.
	     */
	    if (oldData != (Address)NIL) {
		etherHdrPtr = (Net_EtherHdr *)oldData;
		if ( ! NET_ETHER_COMPARE_PTR(
		    &NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
		    (Net_EtherAddress *)clientData)) {
		    printf(
		"Warning: Net_InstallRoute, host <%d> changing ethernet addr\n",
			    spriteID);
		    oldData = (Address)NIL;
		}
	    }
	    if (oldData == (Address)NIL) {
		etherHdrPtr = (Net_EtherHdr *)malloc(sizeof(Net_EtherHdr));
		NET_ETHER_ADDR_COPY(*(Net_EtherAddress *)clientData,
				    NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
		NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
			    Net_HostToNetShort(NET_ETHER_SPRITE);
		routePtr->data = (Address)etherHdrPtr;
	    }
	    break;
	}
	case NET_ROUTE_INET: {
	    Net_EtherHdr *etherHdrPtr;
	    Net_IPHeader *ipHeader;
	    NetInetRoute *inetRoute;
	    char *buffer;
	    /*
	     * The route data for a INET route is an ethernet
	     * header that the network driver can use directly and
	     * an template ipHeader. 
	     * The drivers fill in the source part of the ethernet header and
	     * use the template ipHeader each time they send out a packet. 
	     * Allocate space for a ethernet header and ipheader such that
	     * the ethernet header starts on an odd 16 bit boundry. This
	     * causes the ipheader to start on a 32 bit word boundry.
	     */
	     buffer = malloc(sizeof(Net_EtherHdr) + sizeof(Net_IPHeader) + 2);
	     etherHdrPtr = (Net_EtherHdr *) (buffer + 2);
	     ipHeader = (Net_IPHeader *) 
				(((char *)etherHdrPtr)+sizeof(Net_EtherHdr));
	     inetRoute = (NetInetRoute *)clientData;
	     /*
	      * Fill in the ethernet header to point at the gateway machine
	      * and set the protocol type to IP.
	      */
	     NET_ETHER_ADDR_COPY(inetRoute->gatewayAddress,
				 NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
	     NET_ETHER_HDR_TYPE(*etherHdrPtr) =  
				Net_HostToNetShort(NET_ETHER_IP);
	     /*
	      * Initiailize the template ipHeader.
	      */
	     bzero((char *)ipHeader, sizeof(Net_IPHeader));
	     ipHeader->headerLen = sizeof(Net_IPHeader) / 4;
	     ipHeader->version = NET_IP_VERSION;
	     ipHeader->typeOfService = 0;
	     /*
	      * Kernel IP doesn't handle fragmented IP packets (yet).
	      */
	     ipHeader->flags = NET_IP_DONT_FRAG;
	     ipHeader->timeToLive = NET_IP_MAX_TTL;
	     ipHeader->protocol = NET_IP_PROTOCOL_SPRITE;
	     ipHeader->source = Net_HostToNetInt(net_InetAddress);
	     ipHeader->dest = Net_HostToNetInt(inetRoute->inetAddr);
	     /*
	      * Precompute the checksum for the ipHeader. This must be
	      * corrected when the totalLen field is updated. Note we
	      * store the checksum as the 16 bit sum of the packet
	      * header to permit easy updating.
	      */
	     ipHeader->checksum = Net_InetChecksum(sizeof(Net_IPHeader),
						   (Address) ipHeader);
	     ipHeader->checksum = ~ipHeader->checksum;
	     routePtr->data = (Address)etherHdrPtr;
	     break;
	}
	default: {
	    routePtr->data = (Address)NIL;
	    printf("Warning: Unsupported route type in Net_InstallRoute\n");
	    break;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
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
	case NET_ROUTE_INET:
	    status = Vm_CopyOut(sizeof(Net_EtherHdr) + sizeof(Net_IPHeader), 
			     (Address)routePtr->data,
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
 *	This synchronizes with installation of new routes, although it
 *	returns a pointer into the route table making it difficult
 *	to change routing information without leaking memory - we can't
 *	free anything referenced by the route table.
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
    register Net_Route *routePtr;
    MASTER_LOCK(&netRouteMutex);
    if (spriteID < 0 || spriteID >= netNumRoutes) {
	routePtr = (Net_Route *)NIL;
    } else {
	routePtr = netRouteArray[spriteID];
    }
    MASTER_UNLOCK(&netRouteMutex);
    return(routePtr);
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
    register int ID;

    MASTER_LOCK(&netRouteMutex);

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
		ID = routePtr->spriteID;
		goto found;
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
			ID = routePtr->spriteID;
			goto found;
		    }
		    break;
		}
		case NET_ROUTE_INET: {
		    Net_EtherHdr *etherHdrPtr;
		    Net_IPHeader *ipHeaderPtr;

		    etherHdrPtr = (Net_EtherHdr *)routePtr->data;
		    ipHeaderPtr = (Net_IPHeader *) 
				(((char *)etherHdrPtr)+sizeof(Net_EtherHdr));
		    if (Net_NetToHostInt(ipHeaderPtr->dest) == 
				(Net_InetAddress)routeData) {
			ID = routePtr->spriteID;
			goto found;
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
    MASTER_UNLOCK(&netRouteMutex);
    return(-1);
found:
    MASTER_UNLOCK(&netRouteMutex);
    return(ID);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_HdrToID --
 *
 *      Determine the Sprite host ID from a transport header.  This is
 *      used by a server to determine a client's Sprite
 *      ID from the transport header.
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
Net_HdrToID(headerType, headerPtr)
    int	headerType;	/* Route type of the header. */
    Address headerPtr;	/* Transport header. */
{
    Net_EtherHdr *etherHdrPtr = (Net_EtherHdr *) headerPtr;

   if (headerType == NET_ROUTE_ETHER) {
	Net_EtherAddress srcAddress;
	NET_ETHER_ADDR_COPY(NET_ETHER_HDR_SOURCE(*etherHdrPtr), srcAddress);
	return Net_AddrToID(0, headerType, (ClientData) &srcAddress);
    } else if (headerType == NET_ROUTE_INET) {
	Net_IPHeader *ipHeaderPtr;
	Net_InetAddress dest;
	ipHeaderPtr = (Net_IPHeader *) 
			(((char *)etherHdrPtr)+sizeof(Net_EtherHdr));
	dest = Net_NetToHostInt(ipHeaderPtr->dest);
	return Net_AddrToID(0, headerType, (ClientData)dest);
    } else {
	printf("Unknown header type %d in Net_HdrToID\n", headerType);
    }
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Net_HdrDestString --
 *
 *	Build a printable message of the destination address from a 
 *	transport header.
 *
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
Net_HdrDestString(headerType, headerPtr, bufferLen, buffer)
    int	headerType;	/* Route type of the header. */
    Address headerPtr;	/* Transport header. */
    int	bufferLen;	/* Length of buffer. */
    char *buffer;	/* Destination memory for destination string. */

{
   Net_EtherHdr *etherHdrPtr = (Net_EtherHdr *) headerPtr;
   char	tmpBuffer[128];

   if (headerType == NET_ROUTE_ETHER) {
	Net_EtherAddress srcAddress;
	NET_ETHER_ADDR_COPY(NET_ETHER_HDR_SOURCE(*etherHdrPtr), srcAddress);
	Net_EtherAddrToString(&srcAddress, tmpBuffer);
    } else if (headerType == NET_ROUTE_INET) {
	Net_IPHeader *ipHeaderPtr;
	Net_InetAddress dest;
	ipHeaderPtr = (Net_IPHeader *) 
			(((char *)etherHdrPtr)+sizeof(Net_EtherHdr));
	dest = Net_NetToHostInt(ipHeaderPtr->dest);
	Net_InetAddrToString(dest, tmpBuffer);
    } else {
	printf("Unknown header type %d in Net_HdrDestString\n", headerType);
	strcpy(tmpBuffer, "UNKNOWN");
    }
    strncpy(buffer, tmpBuffer, bufferLen-1);
    return;
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
Net_AddrToName(clientData, namePtrPtr)
    ClientData clientData;
    char **namePtrPtr;
{
    register Net_Route *routePtr;
    register int i;

    MASTER_LOCK(&netRouteMutex);

    *namePtrPtr = (char *) NIL;
    for (i=0 ; i<netNumRoutes ; i++) {
	routePtr = netRouteArray[i];
	if (routePtr == (Net_Route *)NIL) {
	    continue;
	}
	if (routePtr->type == NET_ROUTE_ETHER) {
	    Net_EtherAddress *etherAddressPtr;
	    Net_EtherHdr *etherHdrPtr = (Net_EtherHdr *)routePtr->data;
	    etherAddressPtr = (Net_EtherAddress *) clientData;
	    if (NET_ETHER_COMPARE_PTR(
			&NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
			etherAddressPtr)) {
		*namePtrPtr = routePtr->name;
		break;
	    }
	} else if (routePtr->type == NET_ROUTE_INET) {
	    Net_EtherHdr *etherHdrPtr;
	    Net_IPHeader *ipHeaderPtr;

	   etherHdrPtr = (Net_EtherHdr *)routePtr->data;
	   ipHeaderPtr = (Net_IPHeader *) 
				(((char *)etherHdrPtr)+sizeof(Net_EtherHdr));
	   if (Net_NetToHostInt(ipHeaderPtr->dest) == 
				(Net_InetAddress)clientData) {
		*namePtrPtr = routePtr->name;
		break;
	   }
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
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

    MASTER_LOCK(&netRouteMutex);
    *namePtrPtr = (char *) NIL;
    if (spriteID >= 0 && spriteID < netNumRoutes) {
	routePtr = netRouteArray[spriteID];
	if (routePtr != (Net_Route *)NIL) {
	    *namePtrPtr = routePtr->name;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
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

    MASTER_LOCK(&netRouteMutex);
    machTypeString = (char *)NIL;
    if (spriteID >= 0 && spriteID < netNumRoutes) {
	routePtr = netRouteArray[spriteID];
	if (routePtr != (Net_Route *)NIL) {
	    machTypeString = routePtr->machType;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
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
    int	spriteID;			/* ID to find the route for */
    Sync_Semaphore *mutexPtr;		/* Address of the mutex that the
					 * caller of Net_Output used for
					 * synchronization.  This needs to
					 * be released during the ARP so that
					 * we can receive our reply. */
{
    ReturnStatus status;
    NetSpriteArp request;		/* The Sprite ARP request packet data */
    NetSpriteArp reply;			/* The Sprite ARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */
    static Net_EtherAddress	zeroAddress = {0,0,0,0,0,0};
    Net_EtherAddress	myEtherAddress;

    Mach_GetEtherAddress(&myEtherAddress);
    NetFillInArpRequest(NET_ARP_REQUEST, NET_ROUTE_ETHER,
			(ClientData) spriteID, (ClientData) rpc_SpriteID,
			zeroAddress, myEtherAddress, &request);
    gather.bufAddr = (Address)&request;
    gather.length = sizeof(NetSpriteArp);
    gather.done = FALSE;
    gather.mutexPtr = (Sync_Semaphore *) NIL;
    
    status = NetDoArp(mutexPtr, NET_ARP_REQUEST, &gather, &reply);
    if (status == SUCCESS) {
	(void) Net_InstallRoute(spriteID, 0, NET_ROUTE_ETHER, 
			(ClientData) ARP_SRC_ETHER_ADDR(&reply),
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
Net_RevArp(type, etherAddrPtr)
    int		    type;		/* Type of address wanted. */
    Net_EtherAddress *etherAddrPtr;	/* Physical address to map to Sprite
					 * ID */
{
    ReturnStatus status;
    NetSpriteArp request;		/* Sprite RARP request packet data */
    NetSpriteArp reply;			/* Sprite RARP reply packet data */
    Net_ScatterGather gather;		/* Points to packet data */
    static Sync_Semaphore mutex =
	Sync_SemInitStatic("Net_RevArp.mutex"); /* Reverse arp is called
					 * during initialization when there is
					 * no mutex held (unlike regular arp)
					 * so we need our own mutex for sync */
    Net_EtherAddress	myEtherAddress;

    Mach_GetEtherAddress(&myEtherAddress);

    if (!((type == NET_ROUTE_ETHER) || (type == NET_ROUTE_INET))) {
	panic("Bad route type passed to Net_RevArp.\n");
	return -1;
    }
    NetFillInArpRequest(NET_RARP_REQUEST, type, (ClientData) 0,
		(ClientData) 0, *etherAddrPtr, myEtherAddress,&request);
    gather.bufAddr = (Address)&request;
    gather.length = sizeof(NetSpriteArp);
    gather.done = FALSE;
    gather.mutexPtr = (Sync_Semaphore *) NIL;

    MASTER_LOCK(&mutex);
    Sync_SemRegister(&mutex);
    status = NetDoArp(&mutex, NET_RARP_REQUEST, &gather, &reply);
    MASTER_UNLOCK(&mutex);
    if (status == SUCCESS) {
	unsigned int spriteID;
	bcopy(ARP_TARGET_PROTO_ADDR(&reply),(char *) &spriteID,4);
	return(Net_NetToHostInt(spriteID));
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
    int command;			/* NET_ARP_REQUEST or NET_RARP_REQUEST*/
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
     * is different, Arp.  The broadcast destination address,
     * however, is obtained from the regular broadcast route.
     */
    routePtr = netRouteArray[NET_BROADCAST_HOSTID];
    etherHdrPtr = (Net_EtherHdr *)routePtr->data;
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
			NET_ETHER_HDR_DESTINATION(etherHdr));
    if (command == NET_ARP_REQUEST) {
	NET_ETHER_HDR_TYPE(etherHdr) = Net_HostToNetShort(NET_ETHER_ARP);
    } else {
	NET_ETHER_HDR_TYPE(etherHdr) = Net_HostToNetShort(NET_ETHER_REVARP);
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
	arp.id =  (ClientData) Net_NetToHostInt(spriteID);
	listPtr = &arpList;
    } else {
	arp.id = (ClientData) ARP_TARGET_ETHER_ADDR(requestPtr);
	listPtr = &revArpList;
    }
    List_InitElement((List_Links *) &arp);
    List_Insert((List_Links *)&arp, LIST_ATREAR(listPtr));

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
    register NetSpriteArp *arpDataPtr;
    Boolean	forKernel = TRUE;
    unsigned short opcode, type;

    arpDataPtr = (NetSpriteArp *)(packetPtr + sizeof(Net_EtherHdr));
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
		forKernel = (id == net_InetAddress);
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
		*arpInputPtr = *arpDataPtr;
		nextInputIndex = (nextInputIndex + 1) % ARP_INPUT_QUEUE_LEN;
		MASTER_UNLOCK(&arpInputMutex);
		Proc_CallFunc(NetArpHandler, (ClientData)arpInputPtr, 0);
	    }
	    break;
	}
	case NET_ARP_REPLY: {
	    ArpState *arpPtr;
	    unsigned int id;
	    Net_EtherAddress myEtherAddr, *targetEtherAddrPtr;

	    /*
	     * Make sure this REPLY is targeted for us.
	     */
	    targetEtherAddrPtr = 
			(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(arpDataPtr);
	    Mach_GetEtherAddress(&myEtherAddr);
	    if (!NET_ETHER_COMPARE_PTR(&myEtherAddr, targetEtherAddrPtr)) {
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
	    break;
	}
	case NET_RARP_REQUEST: {
	    /*
	     * Look in our route table for an entry with the ethernet
	     * address of the sender.  If one is found, return a reply
	     * containing the corresponding Sprite ID. The kernel only
	     * handles NET_ETHER_SPRITE requests.
	     */
	    int spriteID;
	    if (type == NET_ETHER_SPRITE) { 
		spriteID = Net_AddrToID(0, NET_ROUTE_ETHER, 
			(ClientData) ARP_TARGET_ETHER_ADDR(arpDataPtr));
		if (arpDebug) {
		    printf("Got REV_ARP request for Sprite ID 0x%x\n",
			    spriteID);
		}
		if (spriteID > 0) {
		    register ArpInputQueue *arpInputPtr;
		    MASTER_LOCK(&arpInputMutex);
		    arpInputPtr = &arpInputQueue[nextInputIndex];
		    *arpInputPtr = *arpDataPtr;
		    nextInputIndex = (nextInputIndex + 1) % 
						    ARP_INPUT_QUEUE_LEN;
		    MASTER_UNLOCK(&arpInputMutex);
		    Proc_CallFunc(NetArpHandler, (ClientData)arpInputPtr,0);
		}
	     }
	}
	case NET_RARP_REPLY: {
	    ArpState *arpPtr;
	    Net_EtherAddress myEtherAddr, *targetEtherAddrPtr;

	    /*
	     * Make sure this REPLY is targeted for us.
	     */
	    targetEtherAddrPtr = 
			(Net_EtherAddress *) ARP_TARGET_ETHER_ADDR(arpDataPtr);
	    Mach_GetEtherAddress(&myEtherAddr);
	    if (!NET_ETHER_COMPARE_PTR(&myEtherAddr, targetEtherAddrPtr)) {
		break;
	    }
	    /*
	     * Make sure there is still a waiting process for this reply,
	     * then copy the reply into the waiting arp state.
	     */
	    if (arpDebug) {
		  printf("Got REV_ARP reply for type %d\n",type);
	    }
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
    ArpInputQueue *arpInputPtr = (ArpInputQueue *)data;
    NetSpriteArp   *arpDataPtr, request;
    unsigned short opcode, type;

    MASTER_LOCK(&arpInputMutex);

    arpDataPtr = arpInputPtr;
    opcode = Net_NetToHostShort(arpDataPtr->arpHeader.opcode);
    type = Net_NetToHostShort(arpDataPtr->arpHeader.protocolType);


    if ((type != NET_ETHER_SPRITE) && (type != NET_ETHER_IP)) {
	MASTER_UNLOCK(&arpInputMutex);
	panic("Bad type %d in NetArpHandler\n", type);
	return;
    } 
    if (opcode == NET_ARP_REQUEST) {
	Net_EtherAddress etherAddress;
	Net_EtherAddress myEtherAddr;

	Mach_GetEtherAddress(&myEtherAddr);
	NET_ETHER_ADDR_COPY(
	    *(Net_EtherAddress *) ARP_SRC_ETHER_ADDR(arpDataPtr), 
	    etherAddress);
	if (type == NET_ETHER_SPRITE) {
	    int	spriteID;
	    bcopy(ARP_SRC_PROTO_ADDR(arpDataPtr), &spriteID, sizeof(int));
	    NetFillInArpRequest(NET_ARP_REPLY, NET_ROUTE_ETHER, 
		(ClientData) spriteID, (ClientData) rpc_SpriteID, 
		etherAddress, myEtherAddr, &request);
	} else {
	    Net_InetAddress inetAddr;
	    bcopy(ARP_SRC_PROTO_ADDR(arpDataPtr), &inetAddr, sizeof(inetAddr));

	    NetFillInArpRequest(NET_ARP_REPLY, NET_ROUTE_INET, 
		(ClientData) inetAddr, (ClientData)  net_InetAddress, 
		etherAddress, myEtherAddr, &request);
	}
        NetArpOutput(etherAddress, NET_ETHER_ARP, &request);
    } else if (opcode == NET_RARP_REQUEST) {
	Net_EtherAddress etherAddress;
	int	spriteID;
	Net_EtherAddress myEtherAddr;

	Mach_GetEtherAddress(&myEtherAddr);
	NET_ETHER_ADDR_COPY(
		*(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(arpDataPtr),
		etherAddress);
	spriteID = Net_AddrToID(0, NET_ROUTE_ETHER, (ClientData) &etherAddress);
	if (spriteID > 0) { 
	    NetFillInArpRequest(NET_RARP_REPLY, NET_ROUTE_ETHER,
			(ClientData) spriteID, (ClientData) rpc_SpriteID,
			etherAddress, myEtherAddr, &request);
	    NetArpOutput(*(Net_EtherAddress *) ARP_SRC_ETHER_ADDR(arpDataPtr),
			NET_ETHER_REVARP, &request);
	}
    } else {
	MASTER_UNLOCK(&arpInputMutex);
	panic ("Bad opcode %d in NetArpHandler\n", opcode);
	return;
    }
    MASTER_UNLOCK(&arpInputMutex);

    callInfoPtr->interval = 0;
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
NetArpOutput(destEtherAddress, etherType, requestPtr)
    Net_EtherAddress destEtherAddress;	/* Host to send to */
    int		etherType;		/* Type of ethernet packet to send. */
    NetSpriteArp *requestPtr;		/* Request to send. */
{
    register Net_EtherHdr  *etherHdrPtr;
    register Net_ScatterGather *gatherPtr;
    register NetSpriteArp *packetPtr;

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

    NET_ETHER_ADDR_COPY(destEtherAddress, 
			NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
    NET_ETHER_HDR_TYPE(*etherHdrPtr) = Net_HostToNetShort(etherType);
    *packetPtr = *requestPtr;

    gatherPtr->bufAddr = (Address)packetPtr;
    gatherPtr->length = sizeof(NetSpriteArp);
    gatherPtr->done = FALSE;
    gatherPtr->mutexPtr = (Sync_Semaphore *) NIL;

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
NetFillInArpRequest(command, type, targetId, senderId, targetEtherAddr, 
		    senderEtherAddr, requestPtr)
    short	command;	/* ARP opcode to perform. */
    int		type;		/* Protocol route type. */
    ClientData	targetId;	/* Target protocol address. */
    ClientData  senderId;	/* Sender's protocol ID. */
    Net_EtherAddress	targetEtherAddr; /* Target ether address. */
    Net_EtherAddress	senderEtherAddr; /* Sender's ether address. */
    NetSpriteArp *requestPtr;	/* Arp request packet to fill in. */
{
   unsigned int tid;
   unsigned int sid;

    requestPtr->arpHeader.hardwareType = Net_HostToNetShort(NET_ARP_TYPE_ETHER);
    requestPtr->arpHeader.hardwareAddrLen = sizeof(Net_EtherAddress);
    requestPtr->arpHeader.opcode = Net_HostToNetShort(command);

    switch(type) {
	case NET_ROUTE_ETHER: {
	    requestPtr->arpHeader.protocolType = 
			Net_HostToNetShort(NET_ETHER_SPRITE);
	    requestPtr->arpHeader.protocolAddrLen = sizeof(int);
	    break;
	}
	case NET_ROUTE_INET: {
	    requestPtr->arpHeader.protocolType = 
					Net_HostToNetShort(NET_ETHER_IP);
	    requestPtr->arpHeader.protocolAddrLen = sizeof(Net_InetAddress);
	    break;
	}
	default: {
	    panic("Warning: NetFillInArpRequest: bad route type %d\n", type);
	}
    }

    tid = Net_HostToNetInt((unsigned int) targetId);
    sid = Net_HostToNetInt((unsigned int) senderId);
    bcopy((char *) &sid, ARP_SRC_PROTO_ADDR(requestPtr),sizeof(int));
    bcopy((char *) &tid, ARP_TARGET_PROTO_ADDR(requestPtr),sizeof(int));
    NET_ETHER_ADDR_COPY(targetEtherAddr,
			*(Net_EtherAddress *)ARP_TARGET_ETHER_ADDR(requestPtr));
    NET_ETHER_ADDR_COPY(senderEtherAddr,
			*(Net_EtherAddress *)ARP_SRC_ETHER_ADDR(requestPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * Net_MaxProtoHdrSize() --
 *
 *	Return the size of the maximum number of bytes needed for headers
 *	for any of the possible routing protocols.
 *
 * Results:
 *	A size in bytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Net_MaxProtoHdrSize()
{
    /*
     * The current large protocol header is for the ROUTE_INET.
     */
    return sizeof(Net_IPHeader);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_RouteMTU() --
 *
 *	Return the Maximum transfer unit (MTU) of the route to the specified
 *	sprite host.
 *
 * Results:
 *	A size in bytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Net_RouteMTU(spriteID)
    register int spriteID;
{
    register Net_Route *routePtr;

    routePtr = netRouteArray[spriteID];
    /*
     * If no route exists use etherNet routing otherwise assume 
     * type is ROUTE_INET.
     */
    if ((routePtr == (Net_Route *)NIL) || (routePtr->type == NET_ROUTE_ETHER)) {
	return (NET_ETHER_MAX_BYTES - sizeof(Net_EtherHdr));
    } 
    return (NET_ETHER_MAX_BYTES - sizeof(Net_EtherHdr) - sizeof(Net_IPHeader));

}

