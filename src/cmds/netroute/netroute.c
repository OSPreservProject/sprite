/* 
 * netroute.c --
 *
 *	User program to install routes to Sprite Hosts.  'route' is a misnomer
 *	because the information also includes the hosts name and its
 *	machine type.  Plus, the route is just a local address. 
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/netroute/RCS/netroute.c,v 1.21 92/06/23 12:08:36 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <stdlib.h>
#include <string.h>
#include <bstring.h>
#include <fs.h>
#include <net.h>
#include <host.h>
#include <kernel/netTypes.h>
#include <stdio.h>
#include <option.h>
#include <rpc.h>
#include <kernel/rpcPacket.h>
#include <sysStats.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <gate.h>

Net_NetworkType rankedInterfaceTypes[HOST_MAX_INTERFACES] =
    { NET_ADDRESS_ETHER, NET_ADDRESS_FDDI, NET_ADDRESS_ULTRA };

static char *rankedInterfaceNames[HOST_MAX_INTERFACES] =
    { "ether", "fddi", "ultra" };

static char *netTypeNames[NET_NUM_NETWORK_TYPES] = 
    { "Ethernet", "UltraNet", "FDDI" };

#define NUM_NET_ADDRS	5
static char *netAddressNames[NUM_NET_ADDRS] =
    { "No Address", "Ethernet", "UltraNet", "FDDI", "Internet" };

static char *netProtocolNames[NET_MAX_PROTOCOLS] = 
    { "raw", "inet" };

/*
 * Parameters set by the command line
 */
char   *inputFilename = NULL;
char   *hostName = NULL;
int     first = -1;
int     last = -1;

char   *gateFilename;

Boolean install = TRUE;
int delete  = -1;
Boolean print   = FALSE;

char   *favorName = NULL;
char   *disfavorName = NULL;

char   *etherString = NULL;
char   *ultraString = NULL;
char   *fddiString = NULL;

Boolean verbose = FALSE;
Boolean execute = FALSE;
char   *machType;
int     hostID = -1;

Option optionArray[] = {
    {OPT_DOC, "\0", (Address)NULL, "  Hosts:"},
    {OPT_STRING, "f", (Address)&inputFilename, 
	 "Specifies the host database file."},
    {OPT_STRING, "h", (Address)&hostName, 
	 "Use Sprite host with given name or Sprite ID"},
    {OPT_STRING, "g", (Address)&gateFilename,
	 "Specifies the gateway database file."},
    {OPT_DOC, "\0", (Address)NULL, "  Actions:"},
    {OPT_TRUE, "i", (Address)&install,
	 "Install route(s)."},
    {OPT_INT, "d", (Address)&delete,
	 "Delete route."},
    {OPT_TRUE, "p", (Address)&print,
	 "Print routes."},
    {OPT_DOC, "\0", (Address)NULL, "  Interface favoring:"},
    {OPT_STRING, "favor", (Address)&favorName,
	 "Try to install route(s) using this type of network interface."},
    {OPT_STRING, "disfavor", (Address)&disfavorName,
	 "Try not to install route(s) using this type of network interface."},
    {OPT_DOC, "\0", (Address)NULL, "\tCurrently (from first to last):"},
    {OPT_DOC, "\0", (Address)NULL, "\t0. ether"},
    {OPT_DOC, "\0", (Address)NULL, "\t1. fddi"},
    {OPT_DOC, "\0", (Address)NULL, "\t2. ultra"},
    {OPT_DOC, "\0", (Address)NULL, "  Address overriding:"},
    {OPT_STRING, "ether", (Address)&etherString,
	 "Ethernet address for host. (ff:oo:oo:bb:aa:rr@aa.bb.cc.dd)"},
    {OPT_STRING, "ultra", (Address)&ultraString,
	 "Ethernet address for host. (hh/ii@aa.bb.cc.dd)"},
    {OPT_STRING, "fddi", (Address)&fddiString,
	 "Ethernet address for host. (qq:ww:ee:rr:tt:yy@aa.bb.cc.dd)"},
    {OPT_DOC, "\0", (Address)NULL, "  Misc:"},
    {OPT_TRUE, "v", (Address)&verbose, 
	 "Print out verbose messages"},
    {OPT_TRUE, "n", (Address)&execute, 
	 "Do not execute commands.  Just print what would be done."},
    {OPT_STRING, "m", (Address)&machType, 
	 "Machine type (sun2, spur, etc.)"},
};
int numOptions = Opt_Number(optionArray);

/*
 * In kernel module net/netTypes.h
 */
#ifndef NET_NUM_SPRITE_HOSTS
#define NET_NUM_SPRITE_HOSTS 200
#endif

/*
 * This is a hack. The subnet mask should be read in from a configuration 
 * file. 
 */

#define SUBNET_MASK 0xFFFFFF00
#define SAME_SUBNET(addr1, addr2) \
    (((addr1) & SUBNET_MASK) == ((addr2) & SUBNET_MASK))


/*
 *----------------------------------------------------------------------
 *
 * PrintRoutes --
 *
 *	Print the routes.
 *
 * Results:
 *	FAILURE if there was a problem getting the routes, SUCCESS
 *      otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PrintRoutes()
{
    Net_Route    routes[NET_NUM_SPRITE_HOSTS];
    Net_Route    *ptr;
    ReturnStatus status;
    int          i;
    int          count;
    char         buffer[64];

    status = Net_GetRoutes(-1, -1, sizeof(Net_Route), routes, &count);
    if (status != SUCCESS) {
	printf("Net_GetRoutes returned 0x%x: %s\n", status, 
	    Stat_GetMsg(status));
	return FAILURE;
    }

    for (i = 0; i < count; i++) {
	ptr = &routes[i];
	if (ptr->version != NET_ROUTE_VERSION) {
	    fprintf(stderr, "Route `%s' has invalid version number. ", 
		    ptr->desc);
	    fprintf(stderr, "It is %d, and should be %d.\n", ptr->version,
		    NET_ROUTE_VERSION);
	    continue;
	}
	if (!(ptr->flags & NET_FLAGS_VALID)) {
	    continue;
	}
	printf("%3d  %-20s %-8s\n", ptr->spriteID, ptr->hostname, 
	    ptr->machType);
	printf("\t0x%08x %-5s %-8s %s -> ", ptr->routeID, 
	    netProtocolNames[ptr->protocol], netTypeNames[ptr->netType],
	    Net_AddrToString(&ptr->interAddress, buffer));
	printf("%s\n", 
	    Net_AddrToString(&ptr->netAddress[NET_PROTO_RAW], buffer));
	printf("\tPacket (%d, %d) RPC (%d, %d)\n", 
	    ptr->minPacket, ptr->maxPacket, ptr->minRpc, ptr->maxRpc);
    }
    return SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * OnSameNetworkType --
 *
 *      Determine if the two hosts are on the same type of network.
 *
 * Results:
 *      TRUE if the hosts are on the same type of network, FALSE
 *      otherwise.  If TRUE, then the given Host_NetInt pointers
 *      will point to the corresponding Host_NetInt structures 
 *      that enable the hosts to be on that network.  Those pointers
 *      are only filled in if non-NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
OnSameNetworkType(type, srcPtr, srcIntPtrPtr, destPtr, destIntPtrPtr)
    Net_NetworkType type;
    Host_Entry      *srcPtr;
    Host_NetInt     **srcIntPtrPtr;  /* (out) */
    Host_Entry      *destPtr;
    Host_NetInt     **destIntPtrPtr;  /* (out) */
{
    register int i;
    register int j;

    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	if (srcPtr->nets[i].netAddr.type == type) {
	    for (j = 0; j < HOST_MAX_INTERFACES; j++) {
		if (destPtr->nets[j].netAddr.type == type) {
		    if (srcIntPtrPtr != (Host_NetInt **)NULL) {
			*srcIntPtrPtr = &srcPtr->nets[i];
		    }
		    if (destIntPtrPtr != (Host_NetInt **)NULL) {
			*destIntPtrPtr = &destPtr->nets[j];
		    }
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * InterfaceByType --
 *
 *      Find the network interface of the host that is of the
 *      given type.
 *
 * Results:
 *      The Host_NetInt structure of the given type, or NULL if
 *      the host does not have a network interface of the given type.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Host_NetInt *
InterfaceByType(hostPtr, netType)
    Host_Entry      *hostPtr;
    Net_AddressType netType;
{
    Host_NetInt *netIntPtr = NULL;
    int i;

    if (hostPtr == (Host_Entry *)NULL) {
	return netIntPtr;
    }

    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	netIntPtr = &hostPtr->nets[i];
	if (netIntPtr->netAddr.type == netType) {
	    return netIntPtr;
	}
    }
    return (Host_NetInt *)NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * EstablishInternetRoute --
 *
 *      Establish an internet route between two hosts.
 *
 * Results:
 *      SUCCESS iff the route was successfully established.  In this
 *      case, the given route structure is filled in with the route
 *      information.  Otherwise, FAILURE is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishInternetRoute(srcHostPtr, destHostPtr, routePtr)
    Host_Entry *srcHostPtr;
    Host_Entry *destHostPtr;
    Net_Route  *routePtr;
{
    Host_NetInt     	*srcIntPtr;
    Host_NetInt     	*destIntPtr;
    Gate_Entry      	*gatePtr;
    ReturnStatus    	status;
    static Boolean	initialized = FALSE;
    static int		numGates;
#define MAX_GATES	5
    static Gate_Entry	gates[MAX_GATES];
    Boolean		found;
    int			i,j;

    if (!initialized) {
	/*
	 * Read the gateway information.
	 */
	status = Gate_Start();
	if (status != SUCCESS) {
	    printf("netroute: couldn't set gate file.\n");
	    return FAILURE;
	}
	numGates = 0;
	gatePtr = Gate_Next();
	while (gatePtr != (Gate_Entry *)NULL) {
	    if (numGates == MAX_GATES) {
		fprintf(stderr, "netroute: too many gateways\n");
		return FAILURE;
	    }
	    gates[numGates] = *gatePtr;
	    gatePtr = Gate_Next();
	    numGates++;
	}
	Gate_End();
	initialized = TRUE;
    }

    /* 
     * Go through the interfaces on the local host and find one that
     * is connected to a gateway.
     */
    found = FALSE;
    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	srcIntPtr = InterfaceByType(srcHostPtr, rankedInterfaceTypes[i]);
	if (srcIntPtr != NULL) {
	    for (j = 0; j < numGates; j++) {
		gatePtr = &gates[j];
		if (SAME_SUBNET(Net_NetToHostInt(srcIntPtr->inetAddr), 
			Net_NetToHostInt(gatePtr->inetAddr))) {
		    found = TRUE;
		    goto endloop;
		}
	    }
	}
    }
endloop:
    if (!found) {
	if (verbose) {
	    printf("Couldn't find a gateway to use\n");
	}
	return FAILURE;
    }

    /*
     * Pick the best interface for the destination host. 
     */
    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	destIntPtr = InterfaceByType(destHostPtr, rankedInterfaceTypes[i]);
	if (destIntPtr != NULL) {
	    break;
	}
    }
    if (destIntPtr == NULL) {
	if (verbose) {
	    printf("Couldn't find an interface to use on the destination.\n");
	}
	return FAILURE;
    }
    routePtr->version = NET_ROUTE_VERSION;
    routePtr->spriteID = destHostPtr->id;
    routePtr->flags = NET_FLAGS_VALID;
    strcpy(routePtr->hostname, destHostPtr->aliases[0]);
    strcpy(routePtr->machType, destHostPtr->machType);
    routePtr->minRpc = 0;
    routePtr->maxRpc = RPC_MAX_SIZE;

    /*
     * The protocol for a host across a gateway is INET.  To set up the
     * route's addresses correctly, the INET address gets set to the internet
     * address of the ethernet network interface of the destination host.
     * The RAW address gets set to the ethernet address of the gateway.
     * And the route's `interAddress' gets set to the network address of
     * the interface to use.
     * Yahoo!  Sheer simplicity.
     */
    routePtr->protocol = NET_PROTO_INET;
    routePtr->netAddress[NET_PROTO_RAW] = gatePtr->netAddr;
    routePtr->interAddress = srcIntPtr->netAddr;
    status = Net_SetAddress(NET_ADDRESS_INET, (Address) &destIntPtr->inetAddr, 
		&routePtr->netAddress[NET_PROTO_INET]);
    if (status != SUCCESS) {
	fprintf(stderr, "netroute: Net_SetAddress failed: %s\n",
	    Stat_GetMsg(status));
	return FAILURE;
    }
    status = Net_InstallRoute(sizeof(Net_Route), routePtr);

    if (status != SUCCESS) {
	fprintf(stderr, "netroute: Net_InstallRoute failed (0x%x): %s\n",
		status, Stat_GetMsg(status));
	return FAILURE;
    } else {
	if (verbose) {
	    printf("netroute: established internet %s route to %s.\n", 
		   netAddressNames[destIntPtr->netAddr.type], 
		   *destHostPtr->aliases);
	}
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * EstablishLocalRoute --
 *
 *      Establish an local route between two hosts.
 *
 * Results:
 *      SUCCESS iff the route was successfully established.  In this
 *      case, the given route structure is filled in with the route
 *      information.  Otherwise, FAILURE is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishLocalRoute(srcHostPtr, destHostPtr, routePtr)
    Host_Entry *srcHostPtr;
    Host_Entry *destHostPtr;
    Net_Route  *routePtr;
{
    register int i;
    Host_NetInt  *destIntPtr = NULL;
    Host_NetInt  *srcIntPtr = NULL;
    ReturnStatus result;
    Boolean found;

    routePtr->version = NET_ROUTE_VERSION;
    routePtr->spriteID = destHostPtr->id;
    routePtr->flags = NET_FLAGS_VALID;
    strcpy(routePtr->hostname, destHostPtr->aliases[0]);
    strcpy(routePtr->machType, destHostPtr->machType);
    routePtr->minRpc = 0;
    routePtr->maxRpc = RPC_MAX_SIZE;

    /*
     * See if the machines are connected to the same local network.
     * If so, use it.
     */
    found = FALSE;
    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	if (OnSameNetworkType(rankedInterfaceTypes[i], srcHostPtr, &srcIntPtr, 
			      destHostPtr, &destIntPtr)) {
	    if (SAME_SUBNET(Net_NetToHostInt(srcIntPtr->inetAddr), 
		    Net_NetToHostInt(destIntPtr->inetAddr))) {
		found = TRUE;
		break;
	    }
	}
    }
    if (!found) {
	return FAILURE;
    }
    if (verbose) {
	printf("%s and %s are on the same %s \n",
	    srcHostPtr->name, destHostPtr->name, rankedInterfaceNames[i]);
    }
    /*
     * Local machines use the RAW protocol.  The network address of the
     * destination host is placed in the route's RAW protocol slot, and
     * the network address of the source host is placed in the route's
     * interface address slot.
     */
    routePtr->protocol = NET_PROTO_RAW;
    routePtr->netAddress[NET_PROTO_RAW] = destIntPtr->netAddr;
    routePtr->interAddress = srcIntPtr->netAddr;

    result = Net_InstallRoute(sizeof(Net_Route), routePtr);

    if (result != SUCCESS) {
	fprintf(stderr, "netroute: Net_InstallRoute failed (0x%x): %s\n",
		result, Stat_GetMsg(result));
	return FAILURE;
    } else {
	if (verbose) {
	    printf("netroute: established raw %s route to %s.\n", 
		   netAddressNames[destIntPtr->netAddr.type], 
		   *destHostPtr->aliases);
	}
	return SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * EstablishRoute --
 *
 *      Establish a route between two hosts.  If the two hosts
 *      are across a gateway, then an internet route is established.
 *      If they are not, then a local route is established.
 *
 * Results:
 *      SUCCESS iff the route was successfully established.  In this
 *      case, the given route structure is filled in with the route
 *      information.  Otherwise, FAILURE is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishRoute(srcHostPtr, destHostPtr, routePtr)
    Host_Entry *srcHostPtr;
    Host_Entry *destHostPtr;
    Net_Route  *routePtr;
{
    ReturnStatus status;

    status = EstablishLocalRoute(srcHostPtr, destHostPtr, routePtr);
    if (status == FAILURE) {
	status = EstablishInternetRoute(srcHostPtr, destHostPtr, routePtr);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * CopyHost --
 *
 *      Copy the contents of the source host structure into the
 *      destination host structure.
 *
 * Results:
 *      SUCCESS iff the copy was successful, FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CopyHost(srcHostPtr, destHostPtr)
    Host_Entry *srcHostPtr;
    Host_Entry *destHostPtr;
{
    bcopy((char *) srcHostPtr, (char *) destHostPtr, sizeof(Host_Entry));
    destHostPtr->name = strdup(srcHostPtr->name);
    destHostPtr->aliases = (char **)malloc(sizeof(char *) * 2);
    *destHostPtr->aliases = strdup(*srcHostPtr->aliases);
    *(destHostPtr->aliases + 1) = NULL;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * EstablishLocalHost --
 *
 *      Determine the Host_Entry for this host, i.e., the host that
 *      the routes are established from.  This information is needed
 *      before routes can be established to determine whether gateways
 *      are crossed, etc.
 *
 * Results:
 *      SUCCESS iff the local host could be found, FAILURE otherwise.
 *      If SUCCESS, then the given Host_Entry structure is filled in
 *      with the host information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishLocalHost(hostPtr)
    Host_Entry *hostPtr;
{
    ReturnStatus 	result;
    int          	localID;
    Host_Entry   	*localPtr;
    Net_Route    	route;
    int			i;
    int			installed = 0;

    result = Proc_GetHostIDs(NULL, &localID);
    if (result != SUCCESS) {
	fprintf(stderr, "netroute: could not get this host's sprite ID.\n");
	return FAILURE;
    }
    localPtr = Host_ByID(localID);
    if (localPtr == (Host_Entry *)NULL) {
	fprintf(stderr, "netroute: couldn't find local host by its ID (%d)\n",
		localID);
	return FAILURE;
    }
    if (verbose) {
	printf("netroute: establishing routes from %s.\n", localPtr->name);
    }
    result = CopyHost(localPtr, hostPtr);
    if (result != SUCCESS) {
	return result;
    }
    localPtr = hostPtr;
    /*
     * This ensures that we know about ourself by giving the net module
     * our internet address for each interface.
     * Check in netRoute.c:Net_InstallRoute() for the details.
     */
    bzero((char *)&route, sizeof(Net_Route));
    route.version = NET_ROUTE_VERSION;
    route.spriteID = hostPtr->id;
    route.flags = NET_FLAGS_VALID;
    strcpy(route.hostname, hostPtr->aliases[0]);
    strcpy(route.machType, hostPtr->machType);
    route.minRpc = 0;
    route.maxRpc = RPC_MAX_SIZE;
    route.protocol = NET_PROTO_INET;
    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	if (hostPtr->nets[i].netAddr.type != NET_ADDRESS_NONE) {
	    route.netAddress[NET_PROTO_RAW] = hostPtr->nets[i].netAddr;
	    route.interAddress = hostPtr->nets[i].netAddr;
	    Net_SetAddress(NET_ADDRESS_INET, 
		(Address) &hostPtr->nets[i].inetAddr, 
		&route.netAddress[NET_PROTO_INET]);
	    result = Net_InstallRoute(sizeof(Net_Route), &route);
	    if (result != SUCCESS) {
		fprintf(stderr, 
			"netroute: Net_InstallRoute failed (0x%x): %s\n",
			result, Stat_GetMsg(result));
	    } else {
		installed++;
	    }
	}
    }
    if (installed == 0) {
	return FAILURE;
    } 
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * EstablishRoutesRange --
 *
 *      Establish routes to all hosts in a given range.  If the
 *      endvalues of the range are both -1, then routes are established
 *      to all hosts in the database file.
 *
 * Results:
 *      SUCCESS iff the routes were established correctly, FAILURE
 *      otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishRoutesRange(firstIDa, lastIDa)
    int firstIDa;
    int lastIDa;
{
    int result;
    int count;
    int firstID = firstIDa, lastID = lastIDa;
    Host_Entry localHost;
    Host_Entry *localPtr;
    Host_Entry *hostPtr;
    Net_Route route;
    Net_Route oldRoutes[10];
    int		i;

    localPtr = &localHost;
    result = EstablishLocalHost(localPtr);
    if (result != SUCCESS) {
	fprintf(stderr, "netroute: couldn't establish address of this host\n");
	return FAILURE;
    }

    result = Host_Start();
    if (result != SUCCESS) {
	fprintf(stderr, "netroute: could not start host file.\n");
	return FAILURE;
    }
    hostPtr = Host_Next();
    while (hostPtr != (Host_Entry *)NULL) {
	/*
	 * If the host is outside of the given range, skip it.
	 */
	if ((firstID > 0) && (lastID > 0) && 
	    ((hostPtr->id < firstID) || (hostPtr->id > lastID))) {
	    hostPtr = Host_Next();
	    continue;
	}
	if (localPtr->id == hostPtr->id) {
	    hostPtr = Host_Next();
	    continue;
	}
	result = Net_GetRoutes(hostPtr->id, hostPtr->id, sizeof(oldRoutes), 
			       oldRoutes, &count);
	if (result != SUCCESS) {
	    count = 0;
	}
	route.userData = (ClientData) 0;
	result = EstablishRoute(localPtr, hostPtr, &route);
	if (result != SUCCESS) {
	    return result;
	}
	/*
	 * If we successfully established the route, then delete the
	 * previous ones. We only delete a previous route if its 
	 * userData is 0, meaning that it was installed by netroute
	 * from the host file. A userData value of 1 indicates that
	 * the route was installed by scratch (by the user) so we
	 * don't delete it lest the user get mad at us.
	 */
	for (i = 0; i < count; i++) {
	    if (oldRoutes[i].userData == (ClientData) 0) {
		if (verbose) {
		    printf("netroute: deleting previous route 0x%x\n", 
			oldRoutes[i].routeID);
		}
		result = Net_DeleteRoute(oldRoutes[i].routeID);
		if (result != SUCCESS) {
		    fprintf(stderr, "netroute: could not delete route 0x%x\n",
			oldRoutes[i].routeID);
		    return FAILURE;
		}
	    }
	}
	hostPtr = Host_Next();
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * EstablishRouteScratch --
 *
 *      Establish a route to a host from scratch, i.e., from the
 *      command line arguments.  
 *
 * Results:
 *      SUCCESS iff the route was established correctly, FAILURE
 *      otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
EstablishRouteScratch(spriteID)
    int spriteID;
{
    int count;
    ReturnStatus result;
    Host_Entry tmpEntry;
    Host_Entry *hostPtr;
    Host_Entry *localPtr;
    Host_Entry localHost;
    Net_Route  route;

    /*
     * Fill in the info for this host, and ensure that it knows
     * about itself.
     */
    localPtr = &localHost;
    result = EstablishLocalHost(localPtr);
    if (result != SUCCESS) {
	fprintf(stderr, "netroute: couldn't find info for local host.\n");
	return FAILURE;
    }
    /*
     * Put together the destination host from scratch.
     */
    hostPtr = &tmpEntry;
    bzero((char *)hostPtr, sizeof(Host_Entry));
    hostPtr->id = spriteID;
    hostPtr->machType = machType;
    hostPtr->name = strdup(hostName);
    hostPtr->aliases = (char **)malloc(sizeof(char *) * 2);
    *hostPtr->aliases = strdup(hostName);
    *(hostPtr->aliases + 1) = NULL;
    count = ManageAddressOverride(hostPtr);
    if (count == 0) {
	fprintf(stderr, "netroute: no addresses specified for installing a ");
	fprintf(stderr, "route from scratch.\n");
	return FAILURE;
    }
    route.userData = (ClientData) 1;
    result = EstablishRoute(localPtr, hostPtr, &route);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteRouteByID --
 *
 *      Delete the route denoted by its ID.
 *
 * Results:
 *      SUCCESS iff the route was deleted correctly, FAILURE
 *      otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DeleteRouteByID(routeID)
    int routeID;
{
    ReturnStatus status;

    if (verbose) {
	printf("netroute: deleting route 0x%x/0x%x.\n", routeID,
	       routeID >> 16);
    }
    status = Net_DeleteRoute(routeID);
    if (status != SUCCESS) {
	printf("Net_DeleteRoute returned 0x%x: %s\n", status,
	       Stat_GetMsg(status));
	return FAILURE;
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteRoutesRange --
 *
 *      Delete the routes to the hosts whose Sprite IDs fall within
 *      a given range.
 *
 * Results:
 *      SUCCESS iff the routes were deleted correctly, FAILURE
 *      otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DeleteRoutesRange(firstID, lastID)
    int firstID;
    int lastID;
{
    int          count;
    int          i;
    ReturnStatus status;
    Net_Route    routes[NET_NUM_SPRITE_HOSTS];

    status = Net_GetRoutes(firstID, lastID, sizeof(Net_Route), routes, &count);
    if (status != SUCCESS) {
	fprintf(stderr, "Net_GetRoutes returned 0x%x: %s\n", status, 
		Stat_GetMsg(status));
	return FAILURE;
    }
    for (i = 0; i < count; i++) {
	status = Net_DeleteRoute(routes[i].routeID);
	if (status != SUCCESS) {
	    printf("Net_DeleteRoute returned 0x%x: %s\n", status,
		   Stat_GetMsg(status));
	    return FAILURE;
	}
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * SplitOffInternetAddress --
 *
 *      Take a command line argument of the form
 *      "net_address@internet_address", NULL the '@', and
 *      return a pointer to the "internet_address".
 *      
 *
 * Results:
 *      A pointer to the "internet_address", or NULL if it could not
 *      be found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static char *
SplitOffInternetAddress(addrString)
    register char *addrString;
{
    while (*addrString != '\0') {
	if (*addrString == '@') {
	    *addrString  = '\0';
	    return (addrString + 1);
	}
	addrString++;
    }
    return NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * ManageAddressOverride --
 *
 *      Take the addresses given on the command line and convert
 *      them into Host_NetInt structures for the given host.
 *
 * Results:
 *      The number of network addresses converted from the command
 *      line.
 *      
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
ManageAddressOverride(hostPtr)
    Host_Entry *hostPtr;
{
    char *inetStr;
    int index = 0;
    Host_NetInt *netIntPtr;
    ReturnStatus result;

    if (etherString != NULL) {
	netIntPtr = &hostPtr->nets[index];
	inetStr = SplitOffInternetAddress(etherString);
	result = Net_StringToAddr(etherString, NET_ADDRESS_ETHER, 
				     &netIntPtr->netAddr);
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: couldn't convert %s to an ethernet ",
		    etherString);
	    fprintf(stderr, "address for host %d.\n", hostPtr->id);
	    return 0;
	}
	netIntPtr->inetAddr = Net_StringToInetAddr(inetStr);
	index++;
    }
    if (ultraString != NULL) {
	netIntPtr = &hostPtr->nets[index];
	inetStr = SplitOffInternetAddress(ultraString);
	result = Net_StringToAddr(ultraString, NET_ADDRESS_ULTRA, 
				     &netIntPtr->netAddr);
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: couldn't convert %s to an ultranet ",
		    ultraString);
	    fprintf(stderr, "address for host %d.\n", hostPtr->id);
	    return 0;
	}
	netIntPtr->inetAddr = Net_StringToInetAddr(inetStr);
	index++;
    }
    if (fddiString != NULL) {
	netIntPtr = &hostPtr->nets[index];
	inetStr = SplitOffInternetAddress(fddiString);
	result = Net_StringToAddr(fddiString, NET_ADDRESS_FDDI, 
				     &netIntPtr->netAddr);
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: couldn't convert %s to an FDDI ",
		    fddiString);
	    fprintf(stderr, "address for host %d.\n", hostPtr->id);
	    return 0;
	}
	netIntPtr->inetAddr = Net_StringToInetAddr(inetStr);
	index++;
    }
    return index;
}


/*
 *----------------------------------------------------------------------
 *
 * ManageFavorings --
 *
 *      Juggle the list of network types according to the favorings
 *      in the command line.  The program is compiled with a default
 *      list of network types that it will traverse when trying to
 *      establish routes to a host, and it will establish the first one
 *      that it can.  The user can change the list ordering
 *      using the command line to change this behavior.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
ManageFavorings()
{
    register int    i;
    register int    j;
    register char   *ptr;
    Net_NetworkType type;

    if (favorName != NULL) {
	for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	    if (!strcmp(favorName, rankedInterfaceNames[i])) {
		ptr = rankedInterfaceNames[i];
		type = rankedInterfaceTypes[i];
		for (j = i - 1; j >= 0; j--) {
		    rankedInterfaceNames[j + 1] = rankedInterfaceNames[j];
		    rankedInterfaceTypes[j + 1] = rankedInterfaceTypes[j];
		}
		rankedInterfaceNames[0] = ptr;
		rankedInterfaceTypes[0] = type;
		break;
	    }
	}
    }
    if (disfavorName != NULL) {
	for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	    if (!strcmp(disfavorName, rankedInterfaceNames[i])) {
		ptr = rankedInterfaceNames[i];
		type = rankedInterfaceTypes[i];
		for (j = i + 1; j < HOST_MAX_INTERFACES; j++) {
		    rankedInterfaceNames[j - 1] = rankedInterfaceNames[j];
		    rankedInterfaceTypes[j - 1] = rankedInterfaceTypes[j];
		}
		rankedInterfaceNames[HOST_MAX_INTERFACES - 1] = ptr;
		rankedInterfaceTypes[HOST_MAX_INTERFACES - 1] = type;
		break;
	    }
	}
    }
    if (verbose) {
	printf("netroute: Interface Rankings: ");
	for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	    printf("%d. %s ", i, rankedInterfaceNames[i]);
	}
	printf("\n");
    }				   
}


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      Establish, delete, or print routes to specified hosts.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus result;
    Host_Entry   *entryPtr;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    if (first < 0 && last > -1) {
	first = 0;
    }
    if (last < 0 && first > -1) {
	last = NET_NUM_SPRITE_HOSTS;
    }
    if (last < first) {
	fprintf(stderr, "netroute: `last' (%d) less than `first' (%d)\n", 
		last, first);
	exit(1);
    }

    if (print) {
	install = FALSE;
    }
    if (inputFilename != NULL) {
	Host_SetFile(inputFilename);
	result = Host_Start();
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: could not open host file %s.\n",
		    inputFilename);
	    exit(1);
	}
    }

    if (hostName != NULL) {
	entryPtr = Host_ByName(hostName);
	if (entryPtr == (Host_Entry *)NULL) {
	    hostID = atoi(hostName);
	    entryPtr = Host_ByID(hostID);
	    if (entryPtr == (Host_Entry *)NULL) {
		fprintf(stderr, "netroute: could not resolve host %s\n",
			hostName);
		exit(1);
	    }
	    hostName = strdup(entryPtr->name);
	} else {
	    hostID = entryPtr->id;
	}
    }

    /*
     * If a host is specified, then it overrides a range.
     */
    if (hostID > 0) {
	first = hostID;
	last = hostID;
    }

    if (delete != -1) {
	result = DeleteRouteByID(delete);
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: failed to delete routes.\n");
	    exit(1);
	} else {
	    printf("netroute: routes deleted.\n");
	}
	print = FALSE;
    }

    if (install) {
	ManageFavorings();
	if (etherString != NULL || ultraString != NULL || fddiString != NULL) {
	    result = EstablishRouteScratch(hostID);
	} else {
	    result = EstablishRoutesRange(-1, -1);
	}
	if (result != SUCCESS) {
	    fprintf(stderr, "netroute: failed installing routes.\n");
	    exit(1);
	} else {
	    printf("netroute: routes installed.\n");
	}
	print = FALSE;
    }

    if (print) {
	(void) PrintRoutes();
    }
    exit(0);
}    
