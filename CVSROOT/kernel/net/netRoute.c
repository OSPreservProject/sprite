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
#endif

#include <sprite.h>
#include <net.h>
#include <netRoute.h>
#include <netInt.h>
#include <netUltraInt.h>
#include <sys.h>
#include <stdlib.h>
#include <timer.h>
#include <sync.h>
#include <proc.h>
#include <rpc.h>
#include <string.h>
#include <vm.h>
#include <string.h>

/*
 * Wildcard address for the Ultranet.  This address matches any address.
 */

static Net_UltraTLAddress	wildcardAddress = 
			    {7, NET_ULTRA_TSAP_SIZE};
/*
 * The Route table.
 * The table is indexed by Sprite Host IDs.  Access to
 * the table is synchronized with a MASTER_LOCK to prevent
 * changes from interfering with packet handling.
 */
List_Links	netRouteArray[NET_NUM_SPRITE_HOSTS];
NetHostInfo	netHostInfo[NET_NUM_SPRITE_HOSTS];
int	 	netNumHosts = NET_NUM_SPRITE_HOSTS;
Sync_Semaphore	netRouteMutex = Sync_SemInitStatic("netRouteMutex");
Sync_Semaphore	netFreeRouteMutex = Sync_SemInitStatic("netFreeRouteMutex");

/*
 * Minimum and maximum number of free routes on free list.
 */

int	netMinFreeRoutes = NET_MIN_FREE_ROUTES;
int	netMaxFreeRoutes = NET_MAX_FREE_ROUTES;

/*
 * Macro to swap the fragOffset field.
 */
#define SWAP_FRAG_OFFSET_HOST_TO_NET(ptr) { \
    unsigned short	*shortPtr; \
    shortPtr = ((unsigned short *)&ptr->ident) + 1; \
    *shortPtr = Net_HostToNetShort(*shortPtr); \
} 

static List_Links	freeRouteList;
static int		netNumFreeRoutes;

static	void		FillRouteInfo _ARGS_((Net_Route *routePtr,
					Net_RouteInfo *infoPtr));


/*
 *----------------------------------------------------------------------
 *
 * Net_RouteInit --
 *
 *      Initialize the broadcast routes.  The rest of the routes are
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
    for (spriteID=0 ; spriteID<netNumHosts ; spriteID++) {
	List_Init(&netRouteArray[spriteID]);
    }
    List_Init(&freeRouteList);
    NetAddToFreeRouteList((ClientData) NIL, (Proc_CallInfo *) NIL);
    /*
     * Install the broadcast route(s) so we can do our first broadcast rpcs.
     */
    for (i = 0; i < netNumInterfaces;i++) {
	Net_Interface	*interPtr;
	ReturnStatus	status;

	interPtr = netInterfaces[i];
	if (interPtr->flags & NET_IFLAGS_BROADCAST) {
	    status = Net_InstallRoute(NET_BROADCAST_HOSTID, 
			    interPtr, &interPtr->broadcastAddress, 
			    NET_PROTO_RAW, "broadcast", "unknown", 
			    (ClientData) 0);
	    if (status != SUCCESS) {
		printf(
	"Net_RouteInit: unable to install broadcast route for interface %d\n",
		    i);
	    }
	}
    }
    return;
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
Net_InstallRouteStub(size, routeInfoPtr)
    int		  size;
    Net_RouteInfo *routeInfoPtr;	/* Route data */
{
    ReturnStatus 	status;
    Net_RouteInfo 	routeInfo;
    Net_Interface 	*interPtr;

    if (routeInfoPtr == USER_NIL) {
	return (SYS_ARG_NOACCESS);
    }
    if (size != sizeof(Net_RouteInfo)) {
	return (GEN_INVALID_ARG);
    }
    status = Vm_CopyIn(sizeof(Net_RouteInfo), (Address)routeInfoPtr, 
		      (Address)&routeInfo);
    if (status != SUCCESS) {
	return(status);
    }
    if (routeInfo.version != NET_ROUTE_VERSION) {
	return GEN_INVALID_ARG;
    }
    if (routeInfo.interface < 0 || routeInfo.interface >= netNumInterfaces) {
	printf("Net_InstallRouteStub: interface %d does not exist\n");
	return(GEN_INVALID_ARG);
    }
    interPtr = Net_GetInterface(routeInfo.netType, routeInfo.interface);
    status = Net_InstallRoute(routeInfo.spriteID,
		    interPtr, routeInfo.netAddress, 
		    routeInfo.protocol,
		    routeInfo.hostname, routeInfo.machType, 
		    routeInfo.userData);
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
Net_InstallRoute(spriteID, interPtr, netAddressPtr, protocol, 
	hostname, machType, clientData)
    int 		spriteID;	/* Sprite Host ID */
    Net_Interface	*interPtr;	/* Interface route is for. */
    Net_Address 	*netAddressPtr;	/* Network addresses (indexed by
					 * protocol). */
    int			protocol;	/* Protocol to use with route. */
    char 		*hostname;	/* Human recognizable name */
    char 		*machType;  	/* Machine type to expand $MACHINE */
    ClientData		clientData; 	/* Data for user-level program. */
{
    register Net_Route 	*routePtr;
    Net_Route		*oldRoutePtr;
    ReturnStatus status = SUCCESS;
    char	*headerPtr;

    if (spriteID < 0) {
	printf("Invalid sprite id %d\n", spriteID);
	return (GEN_INVALID_ARG);
    }
    /*
     * If we are installing a route and we don't know are own spriteID see
     * if we can learn it from the route.
     */
    if (rpc_SpriteID == 0) {
	if (!NET_ADDRESS_COMPARE(interPtr->netAddress[protocol],
		netAddressPtr[protocol])) {
	    rpc_SpriteID = spriteID;
	}
    }
    /*
     * If we know our spriteID and we are installing a route to ourselves we
     * can learn (or at least validate our data).
     */
    if ((rpc_SpriteID > 0) && (rpc_SpriteID == spriteID)) {
	char buffer[128];

	if (!(NET_ADDRESS_COMPARE(interPtr->netAddress[protocol],
		netZeroAddress))) {
	    interPtr->netAddress[protocol] = netAddressPtr[protocol];
	    Net_AddrToString(&interPtr->netAddress[protocol], protocol, 
		    interPtr->netType, buffer);
	    printf("Setting address to %s\n", buffer);
	} else if (NET_ADDRESS_COMPARE(interPtr->netAddress[protocol], 
			netAddressPtr[protocol])) {
	    Net_AddrToString(&interPtr->netAddress[protocol], protocol, 
		interPtr->netType, buffer);
	    printf(
		"Warning: address on interface \"%s\" is currently %s\n",
		interPtr->name,
		buffer);
	    Net_AddrToString(&netAddressPtr[protocol], protocol, 
		interPtr->netType, buffer);
	    printf("Attempt to install route using address %s ignored.\n",
		buffer);
	    status = FAILURE;
	}
	return status;
    }
    routePtr = NetAllocRoute();
    if (routePtr == (Net_Route *) NIL) {
	printf("Net_InstallRoute: Out of free routes!!\n");
	printf("Net_InstallRoute: Route to %d not installed\n", spriteID);
	return FAILURE;
    }
    MASTER_LOCK(&netRouteMutex);

    /*
     * In order to install internet routes our internet address must be set.
     */
    if ((protocol == NET_PROTO_INET) && 
	(interPtr->netAddress[protocol].inet == netZeroAddress.inet)) {

	Boolean 	found = FALSE;
	int		addr;
	Net_Route	*tmpPtr;
	/*
	 * Try to do a reverse arp to get the internet address.  
	 */
	LIST_FORALL(&netRouteArray[NET_BROADCAST_HOSTID], 
	    (List_Links *) tmpPtr){

	    if (tmpPtr->interPtr == interPtr) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    printf("Can't find broadcast route for interface %s\n",
		interPtr->name);
	    MASTER_UNLOCK(&netRouteMutex);
	    return FAILURE;
	}
	addr = Net_RevArp(tmpPtr, protocol, (Net_Address *) NIL, 
			&netRouteMutex);
	if (addr == -1) {
	    printf("Internet address for interface \"%s\" is not set and\n",
		interPtr->name);
	    printf("reverse arp failed.  Can't install route.\n");
	    MASTER_UNLOCK(&netRouteMutex);
	    return FAILURE;
	}
	interPtr->netAddress[protocol].inet = addr;
    }
    if (spriteID >= NET_NUM_SPRITE_HOSTS) {
	printf("Net route table too small!\n");
	MASTER_UNLOCK(&netRouteMutex);
	return FAILURE;
    } else {
	Net_Route 	*tmpPtr;
	oldRoutePtr = (Net_Route *)NIL;
	LIST_FORALL(&netRouteArray[spriteID], (List_Links *) tmpPtr) {
	    if ((tmpPtr->interPtr == interPtr) &&
		(tmpPtr->protocol == protocol)) {

		tmpPtr->flags &= ~NET_RFLAGS_VALID;
		oldRoutePtr = tmpPtr;
		break;
	    }
	}
	List_InitElement((List_Links *) routePtr);
	List_Insert((List_Links *) routePtr, 
	    LIST_ATFRONT((List_Links *) &netRouteArray[spriteID]));
	(void) strncpy(netHostInfo[spriteID].name, hostname, 20);
	(void) strncpy(netHostInfo[spriteID].machType, machType, 12);

	routePtr->flags = NET_RFLAGS_VALID;
	routePtr->refCount = 0;
	routePtr->spriteID = spriteID;
	routePtr->interPtr = interPtr;
	routePtr->maxBytes = interPtr->maxBytes;
	routePtr->minBytes = interPtr->minBytes;
	routePtr->protocol = protocol;
	routePtr->routeID |= spriteID << 16;
	routePtr->userData = clientData;
	sprintf(routePtr->desc, "Route to %s - ", hostname);
    }
    /*
     * Prepare the Route.  This includes the transport header that
     * will be used in messages sent to the Sprite Host.
     */
    headerPtr = (char *) routePtr->buffer;
    switch(interPtr->netType) {
	case NET_NETWORK_ETHER: {
	    Net_EtherHdr *etherHdrPtr;
	    strcat(routePtr->desc, "ethernet, ");
	    /*
	     * Fill in an ethernet header for the route.
	     * The drivers fill in the source part of the ethernet header 
	     * each time they send out a packet.
	     */
	    if (oldRoutePtr != (Net_Route *) NIL) {
		etherHdrPtr = (Net_EtherHdr *)oldRoutePtr->headerPtr;
		if (NET_ETHER_COMPARE(
		    NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
		    netAddressPtr[NET_PROTO_RAW].ether)) {
		    printf(
		"Warning: Net_InstallRoute, host <%d> changing ethernet addr\n",
			    spriteID);
		}
	    }
	    if (protocol == NET_PROTO_RAW) {
		etherHdrPtr = (Net_EtherHdr *)routePtr->buffer;
		NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
			Net_HostToNetShort(NET_ETHER_SPRITE);
	    } else {
		/*
		 * Make the ethernet header start on an odd 16 bit boundary so
		 * that the (internet) header that follows starts on a
		 * 32 bit boundary.
		 */
		etherHdrPtr = (Net_EtherHdr *)
		    ((((int) routePtr->buffer + 5) & ~0x3) - 2);
		NET_ETHER_HDR_TYPE(*etherHdrPtr) = 
				    Net_HostToNetShort(NET_ETHER_IP);
	    }
	    routePtr->headerPtr[NET_PROTO_RAW] = (Address)etherHdrPtr;
	    NET_ETHER_ADDR_COPY(netAddressPtr[NET_PROTO_RAW].ether,
				NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
	    NET_ETHER_ADDR_COPY(interPtr->netAddress[NET_PROTO_RAW].ether,
				NET_ETHER_HDR_SOURCE(*etherHdrPtr));
	    routePtr->netAddress[NET_PROTO_RAW].ether = 
		netAddressPtr[NET_PROTO_RAW].ether;
	    headerPtr = (char *) etherHdrPtr;
	    break;
	}
	case NET_NETWORK_ULTRA: {
	    Net_UltraHeader	*ultraHdrPtr;
	    Net_Address		*addressPtr;

	    ultraHdrPtr = (Net_UltraHeader *) routePtr->buffer;
	    bzero((char *) ultraHdrPtr, sizeof(Net_UltraHeader));
	    strcat(routePtr->desc, "ultranet, ");
	    routePtr->headerPtr[NET_PROTO_RAW] = (Address) ultraHdrPtr;
	    ultraHdrPtr->remoteAddress = wildcardAddress;
	    ultraHdrPtr->remoteAddress.tsapSize = 2;
	    ultraHdrPtr->remoteAddress.tsap[1] = 1;
	    ultraHdrPtr->remoteAddress.address = 
		netAddressPtr[NET_PROTO_RAW].ultra;
	    addressPtr = (Net_Address *) &ultraHdrPtr->remoteAddress.address;
	    addressPtr->generic.data[1] = 0x49;
	    addressPtr->generic.data[6] = 0xfe;
	    ultraHdrPtr->localAddress = wildcardAddress;
	    ultraHdrPtr->localAddress.tsapSize = 2;
	    ultraHdrPtr->localAddress.tsap[1] = 1;
	    ultraHdrPtr->localAddress.address = 
		interPtr->netAddress[NET_PROTO_RAW].ultra;
	    addressPtr = (Net_Address *) &ultraHdrPtr->localAddress.address;
	    addressPtr->generic.data[1] = 0x49;
	    addressPtr->generic.data[6] = 0xfe;
	    ultraHdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
	    headerPtr = (char *) ultraHdrPtr;
	    routePtr->netAddress[NET_PROTO_RAW].ultra = 
		netAddressPtr[NET_PROTO_RAW].ultra;
	    break;
	}
	default:
	    printf("Net_InstallRoute: Unknown interface type %d\n", 
		interPtr->netType);
	    return FAILURE;
    }
    headerPtr += net_NetworkHeaderSize[interPtr->netType];
    switch (protocol) {
	case NET_PROTO_RAW:
	    strcat(routePtr->desc, "raw");
	    break;
    case NET_PROTO_INET: {
	    Net_IPHeader *ipHeader;
	    strcat(routePtr->desc, "IP");
	    ipHeader = (Net_IPHeader *) headerPtr;
	    routePtr->headerPtr[protocol] = (Address)ipHeader;
	    /*
	     * Initialize the template ipHeader.
	     */
	    bzero((char *)ipHeader, sizeof(Net_IPHeader));
	    ipHeader->headerLen = sizeof(Net_IPHeader) / 4;
	    ipHeader->version = NET_IP_VERSION;
	    ipHeader->typeOfService = 0;
	    /*
	     * Kernel IP doesn't handle fragmented IP packets (yet).
	     */
	    ipHeader->flags = NET_IP_DONT_FRAG;
	    SWAP_FRAG_OFFSET_HOST_TO_NET(ipHeader);
	    ipHeader->timeToLive = NET_IP_MAX_TTL;
	    ipHeader->protocol = NET_IP_PROTOCOL_SPRITE;
	    ipHeader->source = interPtr->netAddress[protocol].inet;
	    ipHeader->dest = netAddressPtr[NET_PROTO_INET].inet;
	    /*
	     * Precompute the checksum for the ipHeader. This must be
	     * corrected when the totalLen field is updated. Note we
	     * store the checksum as the 16 bit sum of the packet
	     * header to permit easy updating.
	     */
	    ipHeader->checksum = Net_InetChecksum(sizeof(Net_IPHeader),
					       (Address) ipHeader);
	    ipHeader->checksum = ~ipHeader->checksum;
	    routePtr->maxBytes -= sizeof(Net_IPHeader);
	    routePtr->minBytes -= sizeof(Net_IPHeader);
	    if (routePtr->minBytes < 0) {
		routePtr->minBytes = 0;
	    }
	    routePtr->netAddress[NET_PROTO_INET].inet = 
		netAddressPtr[NET_PROTO_INET].inet;
	    break;
	}
	default: {
	    if (oldRoutePtr != (Net_Route *) NIL) {
		oldRoutePtr->flags |= NET_RFLAGS_VALID;
	    }
	    routePtr->flags &= ~NET_RFLAGS_VALID;
	    oldRoutePtr = routePtr;
	    printf("Warning: Unsupported route type in Net_InstallRoute\n");
	    break;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
    if (oldRoutePtr != (Net_Route *) NIL) {
	Net_DeleteRoute(oldRoutePtr);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_ReleaseRoute --
 *
 *	Releases a route by decrementing its reference count.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The route if freed if it is invalid and the reference count is zero.
 *
 *----------------------------------------------------------------------
 */

void
Net_ReleaseRoute(routePtr)
    Net_Route 	*routePtr;
{
    MASTER_LOCK(&netRouteMutex);
    routePtr->refCount--;
    if ((!(routePtr->flags & NET_RFLAGS_VALID)) && (routePtr->refCount <= 0)) {
	List_Remove((List_Links *) routePtr);
	MASTER_UNLOCK(&netRouteMutex);
	NetFreeRoute(routePtr);
	return;
    }
    MASTER_UNLOCK(&netRouteMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_DeleteRoute --
 *
 *	Deletes a route.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The route is marked as invalid if it has a positive reference
 *	count, otherwise it is deleted.
 *
 *----------------------------------------------------------------------
 */

void
Net_DeleteRoute(routePtr)
    Net_Route	*routePtr;
{
    MASTER_LOCK(&netRouteMutex);
    routePtr->flags &= ~NET_RFLAGS_VALID;
    if (routePtr->refCount <= 0) {
	List_Remove((List_Links *) routePtr);
	MASTER_UNLOCK(&netRouteMutex);
	NetFreeRoute(routePtr);
	return;
    }
    MASTER_UNLOCK(&netRouteMutex);
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
Net_IDToRouteStub(spriteID, size, argPtr)
    int spriteID;		/* option parameter to Test_Stats */
    int	size;			/* Size of user buffer. */
    Address argPtr;		/* User space buffer to hold route */

{
#define MIN(a,b) ((a) < (b) ? (a) : (b))

    ReturnStatus status;
    Net_Route *routePtr;
    Net_RouteInfo routeInfo;
    int toCopy;

    if (spriteID < 0 || spriteID >= netNumHosts) {
	return(SYS_INVALID_ARG);
    }
    routePtr = Net_IDToRoute(spriteID, 0, FALSE, (Sync_Semaphore *) NIL, 0);
    if (routePtr == (Net_Route *)NIL) {
	routeInfo.version = NET_ROUTE_VERSION;
	routeInfo.flags = 0;
    } else {
	FillRouteInfo(routePtr, &routeInfo);
    }
    toCopy = MIN(size, sizeof(Net_RouteInfo) - sizeof(Net_Header));
    status = Vm_CopyOut(toCopy, (Address)&routeInfo, argPtr);
    if (routePtr == (Net_Route *) NIL) {
	return status;
    }
    if (status == SUCCESS) {
	argPtr += toCopy;
	size -= toCopy;
	toCopy = MIN(size, net_NetworkHeaderSize[routePtr->interPtr->netType]);
	status = Vm_CopyOut(toCopy, 
			(Address)routePtr->headerPtr[NET_PROTO_RAW], 
			argPtr);
	argPtr += toCopy;
	size -= toCopy;
	switch(routePtr->protocol) {
	    case NET_PROTO_INET:
		toCopy = MIN(size, sizeof(Net_IPHeader));
		status = Vm_CopyOut(sizeof(Net_IPHeader), 
				 (Address)routePtr->headerPtr[NET_PROTO_INET],
				 argPtr);
		argPtr += toCopy;
		size -= toCopy;
		break;
	    default:
		break;
	}
    }
    Net_ReleaseRoute(routePtr);
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
/*ARGSUSED*/
Net_Route *
Net_IDToRoute(spriteID, index, doArp, mutexPtr, size)
    int 	spriteID;	/* Sprite id to find route for. */
    int 	index;		/* Which route of many. */
    Boolean	doArp;		/* Do an arp to find a route? */
    Sync_Semaphore *mutexPtr;	/* Mutex to release when doing arp. */
    int 	size;		/* Size of data to send (not used yet). */
{
    Net_Route 	*routePtr = (Net_Route *) NIL;
    register	Net_Route *tmpPtr;
    int			i;
    ReturnStatus	status;

    if (spriteID >= 0 && spriteID < netNumHosts) {
	while (1) {
	    i = 0;
	    MASTER_LOCK(&netRouteMutex);
	    LIST_FORALL(&netRouteArray[spriteID],(List_Links *) tmpPtr) {
		if (tmpPtr->flags & NET_RFLAGS_VALID) {
		    if (index >= 0 && i == index) {
			tmpPtr->refCount++;
			routePtr = tmpPtr;
			break;
		    } 
		    i++;
		}
	    }
	    MASTER_UNLOCK(&netRouteMutex);
	    if (routePtr == (Net_Route *) NIL && doArp) {
		status = Net_Arp(spriteID, mutexPtr);
		if (status != SUCCESS) {
		    break;
		}
		doArp = FALSE;
	    } else {
		break;
	    }
	}
    }
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
 *      match with the input address.  
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
Net_AddrToID(netType, protocol, addressPtr)
    Net_NetworkType netType;
    int		protocol;
    Net_Address	*addressPtr;		/* Physical address */
{
    register Net_Route 	*routePtr;
    register int 	i;
    register int 	ID = -1;

    MASTER_LOCK(&netRouteMutex);

    for (i=0 ; i<netNumHosts ; i++) {
	LIST_FORALL(&netRouteArray[i],(List_Links *) routePtr) {
	    if ((routePtr->protocol == protocol) &&
		(routePtr->interPtr->netType == netType) &&
		(!NET_ADDRESS_COMPARE(routePtr->netAddress[protocol], 
			*addressPtr))) {
		ID = routePtr->spriteID;
		break;
	    }
	}
    }
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
Net_HdrToID(netType, protocol, headerPtr)
    Net_NetworkType	netType;
    int			protocol;
    Address headerPtr;	/* Transport header. */
{
    Address 		offsetPtr;
    Net_Address 	netAddress;

    if (protocol == NET_PROTO_RAW) {
	switch(netType) {
	    case NET_NETWORK_ETHER: {
		NET_ETHER_ADDR_COPY(
		    NET_ETHER_HDR_SOURCE(*((Net_EtherHdr *) headerPtr)), 
		    netAddress.ether);
		break;
	    }
	    case NET_NETWORK_ULTRA: {
		netAddress.ultra = 
		    ((Net_UltraHeader *) headerPtr)->remoteAddress.address;
		break;
	    }
	    default:
		printf("Net_HdrToID: unknown netType %d\n", netType);
		return -1;
	}
    } else {
	offsetPtr = headerPtr + net_NetworkHeaderSize[netType];
	switch(protocol) {
	    case NET_PROTO_INET : {
		netAddress.inet = 
		    Net_NetToHostInt(((Net_IPHeader *) offsetPtr)->dest);
		break;
	    }
	    default:
		printf("Net_HdrToID: unknown protocol %d\n", protocol);
		return -1;
	}
    }
    return Net_AddrToID(netType, protocol, &netAddress);
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
Net_HdrDestString(netType, protocol, headerPtr, bufferLen, buffer)
    Net_NetworkType	netType;
    int			protocol;
    Address headerPtr;	/* Transport header. */
    int	bufferLen;	/* Length of buffer. */
    char *buffer;	/* Destination memory for destination string. */

{
    Address 		offsetPtr;
    Net_Address 	netAddress;
    static char		tmpBuffer[128];

    *buffer = '\0';
    if (protocol == NET_PROTO_RAW) {
	switch(netType) {
	    case NET_NETWORK_ETHER: {
		NET_ETHER_ADDR_COPY(
		    NET_ETHER_HDR_SOURCE(*((Net_EtherHdr *) headerPtr)), 
		    netAddress.ether);
		break;
	    }
	    case NET_NETWORK_ULTRA: {
		netAddress.ultra = 
		    ((Net_UltraHeader *) headerPtr)->remoteAddress.address;
		break;
	    }
	    default:
		printf("Net_HdrDestString: unknown netType %d\n", netType);
		return;
	}
    } else {
	offsetPtr = headerPtr + net_NetworkHeaderSize[netType];
	switch(protocol) {
	    case NET_PROTO_INET : {
		netAddress.inet = 
		    Net_NetToHostInt(((Net_IPHeader *) offsetPtr)->dest);
		break;
	    }
	    default:
		printf("Net_HdrDestString: unknown protocol %d\n", protocol);
		return;
	}
    }
    Net_AddrToString(&netAddress, netType, protocol, tmpBuffer);
    (void) strncpy(buffer, tmpBuffer, bufferLen-1);
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
Net_SpriteIDToName(spriteID, bufferLen, buffer)
    int 	spriteID;
    int		bufferLen;
    char 	*buffer;
{
    *buffer = '\0';
    MASTER_LOCK(&netRouteMutex);
    if (spriteID >= 0 && spriteID < netNumHosts) {
	(void) strncpy(buffer, netHostInfo[spriteID].name, bufferLen-1);
    }
    MASTER_UNLOCK(&netRouteMutex);
    return;
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

void
Net_SpriteIDToMachType(spriteID, bufferLen, buffer)
    int 	spriteID;
    int		bufferLen;
    char	*buffer;
{
    *buffer = '\0';
    MASTER_LOCK(&netRouteMutex);
    if (spriteID >= 0 && spriteID < netNumHosts) {
	(void) strncpy(buffer, netHostInfo[spriteID].machType, bufferLen-1);
    }
    MASTER_UNLOCK(&netRouteMutex);
    return;
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
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetAllocRoute --
 *
 *	This routine allocates a route from the list of free routes.
 *	If the number of free routes falls below netMinFreeRoutes
 *	then we schedule a callback to add routes to the
 *	free list. The reason we can't allocate routes using
 *	malloc() is that this routine is called with a master lock
 *	held, and malloc() uses a monitor lock.
 *
 * Results:
 *	A pointer to an unused route if we have one, NIL otherwise
 *
 * Side effects:
 *	A callback is scheduled.
 *
 *----------------------------------------------------------------------
 */

Net_Route *
NetAllocRoute()
{
    Net_Route	*routePtr = (Net_Route *) NIL;
    static	int routeCounter = 1;

    MASTER_LOCK(&netFreeRouteMutex);
    if (netNumFreeRoutes <= 0) {
	printf("Net_GetFreeRoute: no more routes.\n");
    } else {
	routePtr = (Net_Route *) List_Next(&freeRouteList);
	List_Remove((List_Links *) routePtr);
	routePtr->routeID = routeCounter;
	routeCounter++;
	netNumFreeRoutes--;
	if (netNumFreeRoutes < netMinFreeRoutes) {
	    Proc_CallFunc(NetAddToFreeRouteList, (ClientData) 0, 0);
	}
    }
    MASTER_UNLOCK(&netFreeRouteMutex);
    return routePtr;
}


/*
 *----------------------------------------------------------------------
 *
 * NetAddToFreeRouteList --
 *
 *	This routine is used to allocate free routes using malloc()
 *	and put them on the free route list.  It is called via a
 *	callback that is scheduled in NetAllocRoute.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The free list is modified. Malloc is called.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
NetAddToFreeRouteList(data, infoPtr)
    ClientData		data;		/* Not used. */
    Proc_CallInfo	*infoPtr;	/* Not used. */
{
    Net_Route	*routePtr;

    MASTER_LOCK(&netFreeRouteMutex);
    while (netNumFreeRoutes < netMaxFreeRoutes) {
	/*
	 * We have to unlock the master lock in order to do a malloc().
	 * Somebody may slip in and free a route, but then we just
	 * end up with too many free routes.
	 */
	MASTER_UNLOCK(&netFreeRouteMutex);
	routePtr = (Net_Route *) malloc(sizeof(Net_Route));
	bzero((char *) routePtr, sizeof(Net_Route));
	List_InitElement((List_Links *) routePtr);
	MASTER_LOCK(&netFreeRouteMutex);
	List_Insert((List_Links *) routePtr, LIST_ATREAR(&freeRouteList));
	netNumFreeRoutes++;
    }
    MASTER_UNLOCK(&netFreeRouteMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * NetFreeRoute --
 *
 *	Put the route on the free list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The free list is modified.
 *
 *----------------------------------------------------------------------
 */

void
NetFreeRoute(routePtr)
    Net_Route		*routePtr;	/* The route being freed. */
{
    MASTER_LOCK(&netFreeRouteMutex);
    List_Insert((List_Links *) routePtr, LIST_ATREAR(&freeRouteList));
    netNumFreeRoutes++;
    MASTER_UNLOCK(&netFreeRouteMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FillRouteInfo --
 *
 *	Converts from a Net_Route to a Net_RouteInfo. Net_RouteInfo
 * 	is the structure that is passed to user-level.
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
FillRouteInfo(routePtr, infoPtr)
    Net_Route		*routePtr; 	/* The route structure. */
    Net_RouteInfo	*infoPtr;	/* The route info structure. */
{
    int		i;

    infoPtr->version = NET_ROUTE_VERSION;
    infoPtr->spriteID = routePtr->spriteID;
    infoPtr->protocol = routePtr->protocol;
    infoPtr->interface = routePtr->interPtr->number;
    infoPtr->netType = routePtr->interPtr->netType;
    infoPtr->refCount = routePtr->refCount;
    infoPtr->routeID = routePtr->routeID;
    infoPtr->flags = routePtr->flags;
    infoPtr->maxBytes = routePtr->maxBytes;
    infoPtr->minBytes = routePtr->minBytes;
    infoPtr->userData = routePtr->userData;
    for(i = 0; i < NET_MAX_PROTOCOLS; i++) {
	infoPtr->netAddress[i] = routePtr->netAddress[i];
    }
    strncpy(infoPtr->hostname, netHostInfo[routePtr->spriteID].name, 20);
    strncpy(infoPtr->machType, netHostInfo[routePtr->spriteID].machType, 12);
    strncpy(infoPtr->desc, routePtr->desc, 64);
}

