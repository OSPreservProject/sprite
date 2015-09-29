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
 *		Net_AddrToID - Get the Sprite ID for a network address
 *		Net_IDToRoute - Return the route for a Sprite host.
 *	Furthermore, the Test_Stats system call will return a route
 *	to a user program with the NET_GET_ROUTE command.
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
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/net/netRoute.c,v 9.13 92/06/08 22:49:35 jhh Exp $ SPRITE (Berkeley)";
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
#include <rpcPacket.h>

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
 * Macro to swap the fragOffset field.
 */
#define SWAP_FRAG_OFFSET_HOST_TO_NET(ptr) { \
    unsigned short	*shortPtr; \
    shortPtr = ((unsigned short *)&ptr->ident) + 1; \
    *shortPtr = Net_HostToNetShort(*shortPtr); \
} 

static	void		FillUserRoute _ARGS_((Net_Route *routePtr,
					Net_UserRoute *userRoutePtr));
static	void		FillRouteInfoOld _ARGS_((Net_Route *routePtr,
					Net_RouteInfoOld *infoPtr));

/*
 * This variable is only needed for backwards compatibility with netroute.
 * Once the need for the OLD_NET stuff goes away so can this variable.
 */
static Boolean		oldMode;

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
	bzero((char *) &netHostInfo[spriteID], sizeof(NetHostInfo));
    }
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
			    0, RPC_MAX_SIZE,
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
 *	System call stub for Net_InstallRoute. The Net_RouteInfoOld
 *	stuff is for backwards compatibility and can be removed
 *	once netroute is updated to use Net_UserRoute instead of
 *	Net_RouteInfo.
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
Net_InstallRouteStub(size, userRoutePtr)
    int		  size;
    Net_UserRoute *userRoutePtr;	/* Route data */
{
    ReturnStatus 	status = SUCCESS;
    Net_Interface 	*interPtr;
    Net_UserRoute 	userRoute;

    if (userRoutePtr == USER_NIL) {
	return (SYS_ARG_NOACCESS);
    }
    /*
     * This stuff allows the new version of Net_Address and Net_UserRoute
     * to work with old applications. 
     */
    if (size == sizeof(Net_RouteInfoOld)) {
	Net_RouteInfoOld	routeInfoOld;
	status = Vm_CopyIn(sizeof(Net_RouteInfoOld), (Address)userRoutePtr, 
			  (Address)&routeInfoOld);
	if (status != SUCCESS) {
	    return(status);
	}
#define _COPY(field) userRoute.field = routeInfoOld.field
	_COPY(version);
	_COPY(spriteID);
	_COPY(protocol);
	_COPY(flags);
	_COPY(refCount);
	_COPY(routeID);
	_COPY(netType);
	_COPY(userData);

#undef _COPY
#define _COPY(field) bcopy(routeInfoOld.field, userRoute.field, sizeof(routeInfoOld.field))
	_COPY(desc);
	_COPY(hostname);
	_COPY(machType);

#undef _COPY
	userRoute.minPacket = routeInfoOld.minBytes;
	userRoute.maxPacket = routeInfoOld.maxBytes;
	userRoute.minRpc = 0;
	userRoute.maxRpc = RPC_MAX_SIZE;
	switch(userRoute.netType) {
	    case NET_NETWORK_ETHER: 
		status = Net_SetAddress(NET_ADDRESS_ETHER, 
		    (Address) &routeInfoOld.netAddress[NET_PROTO_RAW].ether,
		    &userRoute.netAddress[NET_PROTO_RAW]);
		break;
	    case NET_NETWORK_ULTRA: 
		status = Net_SetAddress(NET_ADDRESS_ULTRA, 
		    (Address) &routeInfoOld.netAddress[NET_PROTO_RAW].ultra,
		    &userRoute.netAddress[NET_PROTO_RAW]);
		break;
	    case NET_NETWORK_FDDI: 
		status = Net_SetAddress(NET_ADDRESS_FDDI, 
		    (Address) &routeInfoOld.netAddress[NET_PROTO_RAW].fddi,
		    &userRoute.netAddress[NET_PROTO_RAW]);
		break;
	}
	if (status != SUCCESS) {
	    panic("Net_InstallRouteStub: Net_SetAddress failed\n");
	}
	if (userRoute.protocol == NET_PROTO_INET) {
	    status = Net_SetAddress(NET_ADDRESS_INET, 
		(Address) &routeInfoOld.netAddress[NET_PROTO_INET].inet,
		&userRoute.netAddress[NET_PROTO_INET]);
	    if (status != SUCCESS) {
		panic("Net_InstallRouteStub: Net_SetAddress failed\n");
	    }
	}
	interPtr = Net_GetInterface(routeInfoOld.netType, 
			routeInfoOld.interface);
	if (interPtr == (Net_Interface *) NIL) {
	    printf("Net_InstallRouteStub: can't find interface %d\n",
		routeInfoOld.interface);
	    return(GEN_INVALID_ARG);
	}
	userRoute.interAddress = interPtr->netAddress[NET_PROTO_RAW];
	oldMode = TRUE;
    } else if (size == sizeof(Net_UserRoute)) {
	status = Vm_CopyIn(sizeof(Net_UserRoute), (Address)userRoutePtr, 
			  (Address)&userRoute);
	if (status != SUCCESS) {
	    return(status);
	}
	if (userRoute.version != NET_ROUTE_VERSION) {
	    return GEN_INVALID_ARG;
	}
	oldMode = FALSE;
    } else {
	return (GEN_INVALID_ARG);
    }
    interPtr = Net_GetInterfaceByAddr(&userRoute.interAddress);
    if (interPtr == (Net_Interface *) NIL) {
	char	buf[20];
	Net_AddrToString(&userRoute.interAddress, buf);
	printf("Net_InstallRouteStub: can't find interface with address %s\n",
	    buf);
	return(GEN_INVALID_ARG);
    }
    status = Net_InstallRoute(userRoute.spriteID,
		    interPtr, userRoute.netAddress, 
		    userRoute.protocol,
		    userRoute.hostname, userRoute.machType, 
		    userRoute.minRpc, userRoute.maxRpc,
		    userRoute.userData);
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
	hostname, machType, minRpc, maxRpc, clientData)
    int 		spriteID;	/* Sprite Host ID */
    Net_Interface	*interPtr;	/* Interface route is for. */
    Net_Address 	*netAddressPtr;	/* Network addresses (indexed by
					 * protocol). */
    int			protocol;	/* Protocol to use with route. */
    char 		*hostname;	/* Human recognizable name */
    char 		*machType;  	/* Machine type to expand $MACHINE */
    int			minRpc;		/* Minimum RPC size for route. */
    int			maxRpc;		/* Maximum RPC size for route. */
    ClientData		clientData; 	/* Data for user-level program. */
{
    register Net_Route 	*routePtr;
    Net_Route		*oldRoutePtr = (Net_Route *) NIL;
    ReturnStatus status = SUCCESS;
    char	*headerPtr;

    if (Mach_AtInterruptLevel()) {
	printf("Can't install route when at interrupt level (%d)\n",
	    spriteID);
	return FAILURE;
    }
    if (spriteID < 0) {
	printf("Invalid sprite id %d\n", spriteID);
	return (GEN_INVALID_ARG);
    }
    /*
     * If we are installing a route and we don't know are own spriteID see
     * if we can learn it from the route.
     */
    if (rpc_SpriteID == 0) {
	if (Net_AddrCmp(&interPtr->netAddress[protocol],
		&netAddressPtr[protocol]) == 0) {
	    rpc_SpriteID = spriteID;
	}
    }
    /*
     * If we know our spriteID and we are installing a route to ourselves we
     * can learn (or at least validate our data).
     */
    if ((rpc_SpriteID > 0) && (rpc_SpriteID == spriteID)) {
	char buffer[128];

	if (interPtr->netAddress[protocol].type == 0) {
	    interPtr->netAddress[protocol] = netAddressPtr[protocol];
	    Net_AddrToString(&interPtr->netAddress[protocol], buffer);
	    printf("Setting address to %s\n", buffer);
	} else if (Net_AddrCmp(&interPtr->netAddress[protocol], 
			&netAddressPtr[protocol])) {
	    Net_AddrToString(&interPtr->netAddress[protocol], buffer);
	    printf(
		"Warning: address on interface \"%s\" is currently %s\n",
		interPtr->name,
		buffer);
	    Net_AddrToString(&netAddressPtr[protocol], buffer);
	    printf("Attempt to install route using address %s ignored.\n",
		buffer);
	    status = FAILURE;
	}
	return status;
    }
    routePtr = (Net_Route *) malloc(sizeof(Net_Route));
    MASTER_LOCK(&netRouteMutex);

    /*
     * In order to install internet routes our internet address must be set.
     */
    if ((protocol == NET_PROTO_INET) && 
	(!Net_AddrCmp(&interPtr->netAddress[protocol], &netZeroAddress))) {

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
	interPtr->netAddress[protocol].type = NET_ADDRESS_INET;
	interPtr->netAddress[protocol].address.inet = addr;
    }
    if (spriteID >= NET_NUM_SPRITE_HOSTS) {
	printf("Net route table too small!\n");
	MASTER_UNLOCK(&netRouteMutex);
	return FAILURE;
    } else {
	Net_Route 	*tmpPtr;
	if (oldMode) {
	    oldRoutePtr = (Net_Route *)NIL;
	    LIST_FORALL(&netRouteArray[spriteID], (List_Links *) tmpPtr) {
		if ((tmpPtr->interPtr == interPtr) &&
		    (tmpPtr->protocol == protocol)) {
    
		    tmpPtr->flags &= ~NET_RFLAGS_VALID;
		    oldRoutePtr = tmpPtr;
		    break;
		}
	    }
	}
	List_InitElement((List_Links *) routePtr);
	List_Insert((List_Links *) routePtr, 
	    LIST_ATREAR((List_Links *) &netRouteArray[spriteID]));
	(void) strncpy(netHostInfo[spriteID].name, hostname, 20);
	(void) strncpy(netHostInfo[spriteID].machType, machType, 12);

	routePtr->flags = NET_RFLAGS_VALID;
	routePtr->refCount = 0;
	routePtr->spriteID = spriteID;
	routePtr->interPtr = interPtr;
	routePtr->maxPacket = interPtr->maxBytes;
	routePtr->minPacket = interPtr->minBytes;
	routePtr->minRpc = minRpc;
	routePtr->maxRpc = maxRpc;
	routePtr->protocol = protocol;
	routePtr->routeID = (spriteID << 16) | netHostInfo[spriteID].routes++;
	routePtr->userData = clientData;
	(void) sprintf(routePtr->desc, "Route to %s - ", hostname);
    }
    /*
     * Prepare the Route.  This includes the transport header that
     * will be used in messages sent to the Sprite Host.
     */
    headerPtr = (char *) routePtr->buffer;
    switch(interPtr->netType) {
	case NET_NETWORK_ETHER: {
	    Net_EtherHdr *etherHdrPtr;
	    Net_EtherAddress	etherAddress;

	    status = Net_GetAddress(&netAddressPtr[NET_PROTO_RAW], 
		(Address) &etherAddress);
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    (void) strcat(routePtr->desc, "ethernet, ");
	    if (oldMode) {
		/*
		 * Fill in an ethernet header for the route.
		 * The drivers fill in the source part of the ethernet header 
		 * each time they send out a packet.
		 */
		if (oldRoutePtr != (Net_Route *) NIL) {
		    etherHdrPtr = (Net_EtherHdr *)
				    oldRoutePtr->headerPtr[NET_PROTO_RAW];
		    if (Net_EtherAddrCmp(etherAddress,
			NET_ETHER_HDR_DESTINATION(*etherHdrPtr))) {
			char buf[20];
    
			Net_EtherAddrToString(
			    &NET_ETHER_HDR_DESTINATION(*etherHdrPtr), buf);
			printf(
		    "Warning: Net_InstallRoute: host <%d> changing ethernet addr\n",
				spriteID);
			printf("\tOld: %s, ",buf);
			Net_EtherAddrToString(&etherAddress, buf);
			printf("New: %s\n", buf);
		    }
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
	    status = Net_GetAddress(&netAddressPtr[NET_PROTO_RAW],
			(Address) &NET_ETHER_HDR_DESTINATION(*etherHdrPtr));
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    status = Net_GetAddress(&interPtr->netAddress[NET_PROTO_RAW],
			(Address) &NET_ETHER_HDR_SOURCE(*etherHdrPtr));
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    routePtr->netAddress[NET_PROTO_RAW] = netAddressPtr[NET_PROTO_RAW];
	    headerPtr = (char *) etherHdrPtr;
	    break;
	}
	case NET_NETWORK_ULTRA: {
	    Net_UltraHeader	*ultraHdrPtr;

	    ultraHdrPtr = (Net_UltraHeader *) routePtr->buffer;
	    bzero((char *) ultraHdrPtr, sizeof(Net_UltraHeader));
	    (void) strcat(routePtr->desc, "ultranet, ");
	    routePtr->headerPtr[NET_PROTO_RAW] = (Address) ultraHdrPtr;
	    ultraHdrPtr->remoteAddress = wildcardAddress;
	    ultraHdrPtr->remoteAddress.tsapSize=2;
	    ultraHdrPtr->remoteAddress.tsap[0]=0xff;
	    ultraHdrPtr->remoteAddress.tsap[1]=0xff;
	    status = Net_GetAddress(&netAddressPtr[NET_PROTO_RAW], 
		(Address) &ultraHdrPtr->remoteAddress.address);
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    ultraHdrPtr->localAddress = wildcardAddress;
	    ultraHdrPtr->localAddress.tsapSize=2;
	    ultraHdrPtr->localAddress.tsap[0]=0xff;
	    ultraHdrPtr->localAddress.tsap[1]=0xff;
	    status = Net_GetAddress(&interPtr->netAddress[NET_PROTO_RAW],
		(Address) &ultraHdrPtr->localAddress.address);
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    ultraHdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
	    headerPtr = (char *) ultraHdrPtr;
	    routePtr->netAddress[NET_PROTO_RAW] = netAddressPtr[NET_PROTO_RAW];
	    break;
	}
	case NET_NETWORK_FDDI: {
	    Net_FDDIHdr *fddiHdrPtr;

	    (void) strcat(routePtr->desc, "fddi, ");
	    fddiHdrPtr = (Net_FDDIHdr *) routePtr->buffer;
	    routePtr->headerPtr[NET_PROTO_RAW] = (Address)fddiHdrPtr;
	    bzero((char *) fddiHdrPtr, sizeof(Net_FDDIHdr));
	    fddiHdrPtr->frameControl = NET_FDDI_SPRITE;             /***/
	    NET_FDDI_ADDR_COPY(netAddressPtr[NET_PROTO_RAW].address.fddi,
			       fddiHdrPtr->dest);
	    NET_FDDI_ADDR_COPY(interPtr->netAddress[NET_PROTO_RAW].address.fddi,
			       fddiHdrPtr->source);
	    headerPtr = (char *) fddiHdrPtr;
	    routePtr->netAddress[NET_PROTO_RAW] = netAddressPtr[NET_PROTO_RAW];
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
	    (void) strcat(routePtr->desc, "raw");
	    break;
    case NET_PROTO_INET: {
	    Net_IPHeader *ipHeader;
	    (void) strcat(routePtr->desc, "IP");
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
	    status = Net_GetAddress(&interPtr->netAddress[protocol], 
		(Address) &ipHeader->source);
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    status = Net_GetAddress(&netAddressPtr[protocol], 
		(Address) &ipHeader->dest);
	    if (status != SUCCESS) {
		panic("Net_InstallRoute: Net_GetAddress failed\n");
	    }
	    /*
	     * Precompute the checksum for the ipHeader. This must be
	     * corrected when the totalLen field is updated. Note we
	     * store the checksum as the 16 bit sum of the packet
	     * header to permit easy updating.
	     */
	    ipHeader->checksum = Net_InetChecksum(sizeof(Net_IPHeader),
					       (Address) ipHeader);
	    ipHeader->checksum = ~ipHeader->checksum;
	    routePtr->maxPacket -= sizeof(Net_IPHeader);
	    routePtr->minPacket -= sizeof(Net_IPHeader);
	    if (routePtr->minPacket < 0) {
		routePtr->minPacket = 0;
	    }
	    routePtr->netAddress[NET_PROTO_INET] = 
		netAddressPtr[NET_PROTO_INET];
	    break;
	}
	default: {
	    if (oldMode) {
		if (oldRoutePtr != (Net_Route *) NIL) {
		    oldRoutePtr->flags |= NET_RFLAGS_VALID;
		}
	    }
	    printf("Warning: Unsupported route type in Net_InstallRoute\n");
	    routePtr->flags &= ~NET_RFLAGS_VALID;
	    status = FAILURE;
	    break;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
    if (oldMode) {
	if (oldRoutePtr != (Net_Route *) NIL) {
	    Net_DeleteRoute(oldRoutePtr);
	}
    }
    if ((status != SUCCESS) && (routePtr != (Net_Route *) NIL)) {
	Net_DeleteRoute(routePtr);
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
    Boolean 	freeIt = FALSE;
    MASTER_LOCK(&netRouteMutex);
    routePtr->refCount--;
    if (Mach_AtInterruptLevel()) {
	goto exit;
    }
    if ((!(routePtr->flags & NET_RFLAGS_VALID)) && 
	(routePtr->refCount <= 0) &&
	(!(routePtr->flags & NET_RFLAGS_DELETING))) {
	routePtr->flags |= NET_RFLAGS_DELETING;
	freeIt = TRUE;
    }
exit:
    MASTER_UNLOCK(&netRouteMutex);
    if (freeIt) {
	List_Remove((List_Links *) routePtr);
	free((char *) routePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Net_DeleteRouteStub --
 *
 *	System call interface for Net_DeleteRoute.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_DeleteRouteStub(routeID)
    int		routeID;	/* ID of the route to be deleted. */
{
    int		spriteID;
    Net_Route	*routePtr;
    Boolean	found = FALSE;

    spriteID = routeID >> 16;
    if (spriteID < 0 || spriteID >= netNumHosts) {
	return GEN_INVALID_ARG;
    }
    MASTER_LOCK(&netRouteMutex);
    LIST_FORALL(&netRouteArray[spriteID],(List_Links *) routePtr) {
	if (routePtr->routeID == routeID) {
	    found = TRUE;
	    break;
	}
    }
    MASTER_UNLOCK(&netRouteMutex);
    if (!found) {
	return GEN_INVALID_ARG;
    }
    Net_DeleteRoute(routePtr);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Net_DeleteRoute --
 *
 *	Deletes a route.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The indicated route is deleted.
 *
 *----------------------------------------------------------------------
 */

void
Net_DeleteRoute(routePtr)
    Net_Route	*routePtr;
{
    Boolean 	freeIt = FALSE;
    MASTER_LOCK(&netRouteMutex);
    routePtr->flags &= ~NET_RFLAGS_VALID;
    if (Mach_AtInterruptLevel()) {
	goto exit;
    }
    if ((routePtr->refCount <= 0) && 
	(!(routePtr->flags & NET_RFLAGS_DELETING))) {
	routePtr->flags |= NET_RFLAGS_DELETING;
	freeIt = TRUE;
    }
exit:
    MASTER_UNLOCK(&netRouteMutex);
    if (freeIt) {
	List_Remove((List_Links *) routePtr);
	free((char *) routePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Net_IDToRouteOldStub --
 *
 *	Stub for the Test_Stats system call, NET_GET_ROUTE command.
 *	This gets a route and copies it out to user space.
 *	This routine is made obsolete by Net_GetRoutes.
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
Net_IDToRouteOldStub(spriteID, size, argPtr)
    int spriteID;		/* option parameter to Test_Stats */
    int	size;			/* Size of user buffer. */
    Address argPtr;		/* User space buffer to hold route */

{
#define MIN(a,b) ((a) < (b) ? (a) : (b))

    ReturnStatus status;
    Net_Route *routePtr;
    Net_RouteInfoOld routeInfo;
    int toCopy;

    if (spriteID < 0 || spriteID >= netNumHosts) {
	return(SYS_INVALID_ARG);
    }
    routePtr = Net_IDToRoute(spriteID, 0, FALSE, (Sync_Semaphore *) NIL, 0);
    if (routePtr == (Net_Route *)NIL) {
	routeInfo.version = NET_ROUTE_VERSION;
	routeInfo.flags = 0;
    } else {
	FillRouteInfoOld(routePtr, &routeInfo);
    }
    toCopy = MIN(size, sizeof(Net_RouteInfoOld) - sizeof(Net_Header));
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
 * Net_GetRoutes --
 *
 *	System call to return routes to user-level. If the firstID and
 *	lastID are both -1 then all routes are returned, otherwise
 *	only those routes for hosts with IDs between firstID and
 *	lastID, inclusive, are returned. 
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
Net_GetRoutes(firstID, lastID, infoSize, bufferPtr, buffersUsedPtr)
    int 	firstID;		/* ID of first host. */
    int 	lastID;			/* ID of last host. */
    int 	infoSize;		/* The size of Net_RouteInfo. */
    Address	bufferPtr;		/* Buffer for route info. */
    int		*buffersUsedPtr;	/* Number of buffers used. */
{
#define MIN(a,b) ((a) < (b) ? (a) : (b))

    ReturnStatus 	status = SUCCESS;
    Net_Route 		*routePtr;
    Net_UserRoute	userRoute;
    int			i, j;
    int			size;
    int			count = 0;

    if ((firstID < -1) || (firstID >= netNumHosts)) {
	return SYS_INVALID_ARG;
    }
    if ((lastID < -1) || (lastID >= netNumHosts)) {
	return SYS_INVALID_ARG;
    }
    if (lastID < firstID) {
	return SYS_INVALID_ARG;
    }
    if (bufferPtr == USER_NIL) {
	return SYS_ARG_NOACCESS;
    }
    if (firstID == -1) {
	if (lastID != -1) {
	    return SYS_INVALID_ARG;
	} else {
	    firstID = 0;
	    lastID = netNumHosts - 1;
	}
    }
    size = MIN(infoSize, sizeof(Net_UserRoute));
    for (i = firstID; i <= lastID; i++) {
	for (j = 0;; j++) {
	    routePtr = Net_IDToRoute(i, j, FALSE, (Sync_Semaphore *) NIL, 0);
	    if (routePtr == (Net_Route *) NIL) {
		break;
	    }
	    FillUserRoute(routePtr, &userRoute);
	    Net_ReleaseRoute(routePtr);
	    status = Vm_CopyOut(size, (Address)&userRoute, bufferPtr);
	    if (status != SUCCESS) {
		goto done;
	    }
	    bufferPtr += size;
	    count++;
	}
    }
done:
    if (buffersUsedPtr != USER_NIL) {
	status = Vm_CopyOut(sizeof(int), (Address) &count, 
	    (Address) buffersUsedPtr);
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
/*ARGSUSED*/
Net_Route *
Net_IDToRoute(spriteID, index, doArp, mutexPtr, size)
    int 	spriteID;	/* Sprite id to find route for. */
    int 	index;		/* If >= 0 then the index of the route,
				 * otherwise use the size. */
    Boolean	doArp;		/* Do an arp to find a route? */
    Sync_Semaphore *mutexPtr;	/* Mutex to release when doing arp. */
    int 	size;		/* Size of RPC to be sent. */
{
    Net_Route 	*routePtr = (Net_Route *) NIL;
    register	Net_Route *tmpPtr;
    int			i;
    ReturnStatus	status;

    if (spriteID >= 0 && spriteID < netNumHosts) {
	while (1) {
	    i = 0;
	    MASTER_LOCK(&netRouteMutex);
	    if (index >= 0) {
		LIST_FORALL(&netRouteArray[spriteID],(List_Links *) tmpPtr) {
		    if (tmpPtr->flags & NET_RFLAGS_VALID) {
			if (i == index) {
			    routePtr = tmpPtr;
			    break;
			}
			i++;
		    }
		}
	    } else {
		LIST_FORALL(&netRouteArray[spriteID],(List_Links *) tmpPtr) {
		    if ((tmpPtr->minRpc <= size) && (tmpPtr->maxRpc >= size) &&
			(tmpPtr->flags & NET_RFLAGS_VALID)) {
			routePtr = tmpPtr;
			break;
		    }
		}
	    }
	    if (routePtr != (Net_Route *) NIL) {
		routePtr->refCount++;
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
 *      Determine the Sprite host ID from a network address.  This is
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
Net_AddrToID(addressPtr)
    Net_Address	*addressPtr;		/* Physical address */
{
    register Net_Route 	*routePtr;
    register int 	i;
    register int 	ID = -1;
    int			protocol;

    switch(addressPtr->type) {
	case NET_ADDRESS_INET: 
	    protocol = NET_PROTO_INET;
	    break;
	default:
	    protocol = NET_PROTO_RAW;
	    break;
    }

    MASTER_LOCK(&netRouteMutex);

    for (i=0 ; i<netNumHosts ; i++) {
	LIST_FORALL(&netRouteArray[i],(List_Links *) routePtr) {
	    if ((routePtr->protocol == protocol) &&
		(Net_AddrCmp(&routePtr->netAddress[protocol], 
			addressPtr) == 0)) {
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
 * Net_HdrToAddr --
 *
 *	Determine the Sprite host ID from a transport header. 
 *
 * Results:
 *	SUCCESS if the Sprite ID was found ok, FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_HdrToAddr(netType, protocol, headerPtr, netAddressPtr) 
    Net_NetworkType	netType;		/* Type of network. */
    int			protocol;		/* Network protocol. */
    Address		headerPtr;		/* The transport header. */
    Net_Address		*netAddressPtr;		/* The network address. */
{
    Address 		offsetPtr;
    ReturnStatus	status = SUCCESS;

    if (protocol == NET_PROTO_RAW) {
	switch(netType) {
	    case NET_NETWORK_ETHER: {
		status = Net_SetAddress(NET_ADDRESS_ETHER, 
		    (Address) 
			&NET_ETHER_HDR_SOURCE(*((Net_EtherHdr *) headerPtr)),
		    netAddressPtr);
		break;
	    }
	    case NET_NETWORK_ULTRA: {
		status = Net_SetAddress(NET_ADDRESS_ULTRA, 
		    (Address) &((Net_UltraHeader *) headerPtr)->remoteAddress,
		    netAddressPtr);
		break;
	    }
	    case NET_NETWORK_FDDI: {
	        status = Net_SetAddress(NET_ADDRESS_FDDI,
	            (Address) &((Net_FDDIHdr *) headerPtr)->source,
	            netAddressPtr);
	        break;
	    }
	    default:
		printf("Net_HdrToAddr: unknown netType %d\n", netType);
		return FAILURE;
	}
	if (status != SUCCESS) {
	    panic("Net_HdrToAddr: Net_SetAddress failed\n");
	}
    } else {
	offsetPtr = headerPtr + net_NetworkHeaderSize[netType];
	switch(protocol) {
	    case NET_PROTO_INET : {
		Net_InetAddress	inetAddress;
		inetAddress = (Net_InetAddress) 
		    Net_NetToHostInt(((Net_IPHeader *) offsetPtr)->dest);
		status = Net_SetAddress(NET_ADDRESS_INET, 
		    (Address) &inetAddress, netAddressPtr);
		break;
	    }
	    default:
		printf("Net_HdrToAddr: unknown protocol %d\n", protocol);
		return FAILURE;
	}
	if (status != SUCCESS) {
	    panic("Net_HdrToAddr: Net_SetAddress failed\n");
	}
    }
    return SUCCESS;
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
    Net_Address 	netAddress;
    ReturnStatus	status;

    status = Net_HdrToAddr(netType, protocol, headerPtr, &netAddress);
    if (status != SUCCESS) {
	printf("Net_HdrToID: unable to get address from header\n");
	return -1;
    }
    return Net_AddrToID(&netAddress);
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
    Net_Address 	netAddress;
    static char		tmpBuffer[128];
    ReturnStatus	status;

    *buffer = '\0';
    status = Net_HdrToAddr(netType, protocol, headerPtr, &netAddress);
    if (status != SUCCESS) {
	printf("Net_HdrDestString: unable to get address from header\n");
	return;
    }
    Net_AddrToString(&netAddress, tmpBuffer);
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
 * FillUserRoute --
 *
 *	Converts from a Net_Route to a Net_UserRoute. Net_UserRoute
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
FillUserRoute(routePtr, userRoutePtr)
    Net_Route		*routePtr; 	/* The route structure. */
    Net_UserRoute	*userRoutePtr;	/* The user version of the route. */
{
    int		i;

    userRoutePtr->version = NET_ROUTE_VERSION;
    userRoutePtr->spriteID = routePtr->spriteID;
    userRoutePtr->protocol = routePtr->protocol;
    userRoutePtr->interAddress = routePtr->interPtr->netAddress[NET_PROTO_RAW];
    userRoutePtr->netType = routePtr->interPtr->netType;
    userRoutePtr->refCount = routePtr->refCount;
    userRoutePtr->routeID = routePtr->routeID;
    userRoutePtr->flags = routePtr->flags;
    userRoutePtr->maxPacket = routePtr->maxPacket;
    userRoutePtr->minPacket = routePtr->minPacket;
    userRoutePtr->maxRpc = routePtr->maxRpc;
    userRoutePtr->minRpc = routePtr->minRpc;
    userRoutePtr->userData = routePtr->userData;
    for(i = 0; i < NET_MAX_PROTOCOLS; i++) {
	userRoutePtr->netAddress[i] = routePtr->netAddress[i];
    }
    (void) strncpy(userRoutePtr->hostname, 
		    netHostInfo[routePtr->spriteID].name, 20);
    (void) strncpy(userRoutePtr->machType,
	           netHostInfo[routePtr->spriteID].machType, 12);
    (void) strncpy(userRoutePtr->desc, routePtr->desc, 64);
}

/*
 *----------------------------------------------------------------------
 *
 * FillRouteInfoOld --
 *
 *	Converts from a Net_Route to a Net_RouteInfoOld. Net_RouteInfoOld
 * 	is the structure that is passed to user-level.
 *	This routine is here for backwards compatibility and can
 *	be removed as soon as Net_IDToRouteStub is gone.
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
FillRouteInfoOld(routePtr, infoPtr)
    Net_Route		*routePtr; 	/* The route structure. */
    Net_RouteInfoOld	*infoPtr;	/* The route info structure. */
{
    int			i;
    ReturnStatus	status;

    infoPtr->version = NET_ROUTE_VERSION;
    infoPtr->spriteID = routePtr->spriteID;
    infoPtr->protocol = routePtr->protocol;
    infoPtr->interface = routePtr->interPtr->number;
    infoPtr->netType = routePtr->interPtr->netType;
    infoPtr->refCount = routePtr->refCount;
    infoPtr->routeID = routePtr->routeID;
    infoPtr->flags = routePtr->flags;
    infoPtr->maxBytes = routePtr->maxPacket;
    infoPtr->minBytes = routePtr->minPacket;
    infoPtr->userData = routePtr->userData;
    for(i = 0; i < NET_MAX_PROTOCOLS; i++) {
	status = Net_GetAddress(&routePtr->netAddress[i], 
	    (Address) &infoPtr->netAddress[i]);
	if (status != SUCCESS) {
	    break;
	}
    }
    (void) strncpy(infoPtr->hostname, netHostInfo[routePtr->spriteID].name, 20);
    (void) strncpy(infoPtr->machType,
	           netHostInfo[routePtr->spriteID].machType, 12);
    (void) strncpy(infoPtr->desc, routePtr->desc, 64);
}

