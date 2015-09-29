/* 
 * route.c --
 *
 *	This file contains routines to handle low-level IP packet I/O and to
 *	maintain information about the network interfaces and packet routing.
 *	Currently, most routines are ethernet-specific.
 *
 *	When an IP packet is to be sent out on the ethernet, the packet's
 *	destination address must be translated to an ethernet address.
 *	This task is called routing and is performed by Rte_FindOutputNet.
 *	The routine uses a static table of <IP, ethernet> entries to map
 *	an IP address into an ethernet address.	The table is initialized
 *	from a file by Rte_AddressInit() and contains entires for hosts on
 *	the ethernet. The IP output routine calls Rte_FindOutputNet to get 
 *	a token (a Rte_NetID structure) that identifies	a route. Along with 
 *	the packet, the route token is passed to Rte_OutputPacket, which uses 
 *	it to insert the ethernet address into the packet. If a corresponding 
 *	ethernet address can't be found for an IP address, it is assumed that 
 *	host is on a different network. In that case, the Rte_NetID token 
 *	identifies a gateway host that can forward the packet to the proper 
 *	network.
 *
 *	(Aside: "Ethernet (capital E) is a specific Xerox protocol used for LAN,
 *	  whereas an ethernet (small e) refers to an Ethernet-like network."
 *	  Quarterman and Hoskins, CACM v29#10, 10/86, p.938)
 *
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/route.c,v 1.17 92/06/16 13:03:53 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "ip.h"
#include "route.h"
#include "stat.h"

#ifndef KERNEL
#include <errno.h>
#include <sys/file.h>
#include <host.h>
#endif

#include <netEther.h>
#include <list.h>
#include <string.h>
#include <fs.h>
#include <hash.h>


#ifdef TEST_DISCONNECT
extern int	ips_Disconnect;
#endif
/*
 * Comment character used in the config file. Lines beginning with this
 * character are ignored.
 */
#define COMMENT_CHAR 	'#'


/*
 * Address information kept in a hash table keyed on the internet address.
 * There are a few possibilities for each address
 * NET_NEIGHBOR		The host is a neighbor on the local network
 * NET_GATEWAY		The host is a gateway to other networks
 * NET_ALIAS		This is another alias for ourselves
 * NET_BROADCAST	This is a broadcast address
 * NET_UPDATE_ADDR	Update the physical address information when installing.
 * NET_ETHERNET		The ethernet field has the physical net address.
 */

typedef struct NetAddrInfo {
    Net_InetAddress	inet;		/* Hash key, must be first */
    int			flags;		/* NET_NEIGHBOR, NET_GATEWAY,
					 * NET_ALIAS, NET_BROADCAST,
					 * plus type for addr union */
    union {
	Net_EtherAddress ether;		/* NET_ETHERNET */
    } addr;
    struct NetInfo	*netPtr;	/* Interface information */
} NetAddrInfo;

/*
 * NET_GATEWAY is also defined to be 0x2 in the kernel's netRoute.h.  Don't
 * change one without the other.  Obviously this should be fixed.
 */
#define NET_NEIGHBOR	0x1
#define NET_GATEWAY	0x2
#define NET_ALIAS	0x4
#define NET_BROADCAST	0x8
#define NET_UPDATE_ADDR	0x10
#define NET_ETHERNET	0x20

Hash_Table addrHashTable;
Hash_Table *addrTable = &addrHashTable;

#ifdef KERNEL
    /*
     * UGH.  Kernel and user hash packages not the same.
     */
#define Hash_InitTable(table, numBuckets, keys) \
	Hash_Init(table, numBuckets, keys)

#define Hash_FindEntry(table, key) \
	Hash_LookOnly(table, key)
#endif

/*
 * Information about network interfaces.
 */

typedef struct NetInfo {
    char	*name;		/* Device name. */
    int		streamID;	/* Used with Read, Write. */
    int		maxOutputSize;	/* Size in bytes of max. packet size
				 * that this interface can output. */
    int		flags;		/* Defined below. */
    Net_InetAddress address;	/* Official IP address. */
    Net_InetAddress localBrdcastAddr;	/* Broadcast address for the subnet
					 * (directly-connected network). */
    Net_InetAddress netBrdcastAddr;	/* Broadcast address for all subnets
					 * on the logical network. */
    unsigned int netPart;	/* Network number but unshifted. */
    unsigned int netMask;	/* Mask to generate network # from an address.*/
    unsigned int subnetPart;	/* Subnet # (included net #) but unshifted. */
    unsigned int subnetMask;	/* Mask to generate subnet # from an addr. */

    NetAddrInfo	*gatewayPtr;	/* The address of the local gateway. */
} NetInfo;


static NetAddrInfo *localHostAddrPtr;	/* Our own address for loopback. */
/*
 * Values used in the flags field of NetInfo.
 */
#define NET_IN_USE	0x1


#define MAX_NUM_NETS	4
static NetInfo	netInfo[MAX_NUM_NETS];
static int	numNetInterfaces = 0;

/*
 * IP Address to look for route to.  When IPInput sees this address come
 * by it snarfs up the source ethernet address.  The localhost value will
 * never arrive as input, so it is the "do nothing" value.
 */
static Net_InetAddress lookforIPAddress = NET_INET_LOCAL_HOST;
/*
 * Forward declaration of internal procedures.
 */
NetAddrInfo *RteInsertAddress();
NetAddrInfo *RteFetchAddress();

static void ReadIPPacket();
static void GetLineList();
static Net_EtherAddress broadcast = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};




/*
 *----------------------------------------------------------------------
 *
 * Rte_AddressInit --
 *
 *	Reads a file that contains the following information: 
 *	1) the name of IP network device,
 *	2) a bit mask used to generate a host number for hosts on the same
 *	   network (subnet mask).
 *	3) the maximum # of bytes the network device can output.
 *	4) a list of alias IP addresses for this program. Packets sent
 *	   to one of these addresses will be handled.
 *	5) a name of a gateway for the network. 
 *	Host Internet and ethernet addresses are obtained from 
 *	/etc/spritehosts. This information is used to initialize 
 *	the network and neighbor information tables.
 *
 *	To do: extend to multiple networks.
 *
 * Kernel Implementation:
 *	This doesn't read the configuration file when running in the kernel.
 *	Instead the Net_RouteInstall system uses RteInsertAddress to
 *	give us addressing information.  This just sets up the net stucture
 *	and installs the various broadcast addresses that map to it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The network table is initialized.  There is both a per-interface
 *	structure and a hash table of internet addresses.  Addresses
 *	are installed for our own aliases, for our local neighbors,
 *	for the gateway, and for various broadcast addresses.
 *
 *----------------------------------------------------------------------
 */

void
Rte_AddressInit(myHostName, filename, newOfficialAddr)
    char *myHostName;
    char *filename;		/* Name of configuration file. */
    char *newOfficialAddr;	/* If not NULL, use this as the official 
				 * IP address. */
{
#ifndef KERNEL
    /*
     * Only the user-level version reads the config file.
     * The kernel gets routing information via the Net_Route system call.
     */

#define LINE_LEN	80
#define ARRAY_SIZE	100

    FILE		*stream;
    char		*array[ARRAY_SIZE];
    int			numGateways;
    int			numAliases;
    char		devName[LINE_LEN];
    char		etherString[LINE_LEN];
    char		inetString[LINE_LEN];
    char		line[LINE_LEN];
    char		host[LINE_LEN];
    Host_Entry 		*hostPtr;
#endif
    Net_InetAddress	address;
    Net_EtherAddress	etherAddr;
    NetAddrInfo		*addrPtr;
    Boolean		foundOfficialAddr = FALSE;
    register NetInfo	*infoPtr;
    int			i;
    int			numHosts;
    unsigned int	subpart;

#ifndef KERNEL
    stream = fopen(filename, "r");
    if (stream == (FILE *) NULL) {
	(void) fprintf(stderr, "Can't open net info file '%s'\n", filename);
	exit(1);
    }
#endif

    /*
     * For now, assume just one net in the netinfo file.
     */

    infoPtr = &netInfo[0];

#ifndef KERNEL
    do {
	(void) fgets(line, LINE_LEN, stream);
    } while (line[0] == COMMENT_CHAR);

    /*
     * Look for the config file version and print it if debug is set.
     */

    if (strncmp("version", line, strlen("version")) != 0) {
	(void) fprintf(stderr, "Bad format in %s: version.\n", filename);
	exit(1);
    }
    (void) printf("Config file: %s\n   %s\n", filename, line);


    /*
     ***********************************************************************
     *
     * Group 1:  network device name, subnet mask,
     *		 max. # bytes output on the network device.
     *	Example: /dev/etherIP    0xffffff00      1500
     *
     ***********************************************************************
     */

    do {
	(void) fgets(line, LINE_LEN, stream);
    } while (line[0] == COMMENT_CHAR);

    i = sscanf(line, "%s 0x%x %d", devName, &infoPtr->subnetMask, 
			&infoPtr->maxOutputSize);
    if (i != 3) {
	panic("Bad format in %s: device name.\n", filename);
    }

    infoPtr->streamID = open(devName, O_RDWR | O_NDELAY , 0); 
    if (infoPtr->streamID < 0) {
	perror("Rte_AddressInit (open net)");
	exit(1);
    }

    /*
     * Tell the dispatcher we're interested when packets arrive on
     * this stream.
     */
    Fs_EventHandlerCreate(infoPtr->streamID, FS_READABLE, ReadIPPacket, 
			(ClientData) 0);

    infoPtr->name = malloc((unsigned int) strlen(devName) + 1);
    (void) strcpy(infoPtr->name, devName);
#endif

    infoPtr->flags		|= NET_IN_USE;

    /*
     * Initialize the address table and insert our official address.
     */
    Hash_InitTable(addrTable, 64, HASH_ONE_WORD_KEYS);

    if (newOfficialAddr != (char *) NULL) {
	infoPtr->address = 
		Net_NetToHostInt(Net_StringToInetAddr(newOfficialAddr));
	foundOfficialAddr = TRUE;
	(void)RteInsertAddress(infoPtr->address, (Net_EtherAddress *)NULL,
		NET_ALIAS, infoPtr);
    }

    /*
     ***********************************************************************
     *
     * Group 2:  List of aliases for this machine.
     *		 Format:  <name> <inet address>
     *  	 Delimited by "Start_Aliases" and "End_Aliases"
     *
     ***********************************************************************
     */
#ifndef KERNEL
    GetLineList(stream, "Start_Aliases", "End_Aliases", ARRAY_SIZE, array, 
		&numAliases);
    for (i = 0; i < numAliases; i++) {
	(void) sscanf(array[i], "%s %s", host, inetString);
	address = Net_NetToHostInt(Net_StringToInetAddr(inetString));
	(void)RteInsertAddress(address, (Net_EtherAddress *)NULL, NET_ALIAS,
		infoPtr);
	free(array[i]);
    }
#endif
    /*
     ************************************************************************
     *
     * Read /etc/spritehosts to get local host (neighbor) information
     *
     ************************************************************************
     */
#ifndef KERNEL
    if (!foundOfficialAddr) {
	hostPtr = Host_ByName(myHostName);
	if (hostPtr != NULL) {
	    /*
	     * Look for the ethernet interface and get its internet address.
	     * TODO: in reality the physical address for the interface
	     * should be obtained via an ioctl on the open device. This
	     * address would then be used to determine the internet address
	     * for the interface, and which hosts are local as well.
	     */
	    for (i = 0; i < hostPtr->numNets; i++) {
		if (hostPtr->nets[i].netAddr.type == NET_ADDRESS_ETHER) {
		    infoPtr->address = 
			Net_NetToHostInt(hostPtr->nets[i].inetAddr);
		    foundOfficialAddr = TRUE;
		}
	    }
	} else {
	    panic("Rte_AddressInit: could not find myself in the host file\n");
	}
    }
    if (!foundOfficialAddr) {
	panic("Rte_AddressInit: can't find IP address for %s\n", myHostName);
    }
    subpart = infoPtr->subnetMask & infoPtr->address;
    if (Host_Start() != SUCCESS) {
	panic("Rte_AddressInit: Host_Start failed (?no /etc/spritehosts?)\n");
    }
    for (hostPtr = Host_Next();
         hostPtr != (Host_Entry *)NULL;
	 hostPtr = Host_Next()) {
	register int flags;

	flags = NET_NEIGHBOR;
	for (i = 0; i < hostPtr->numNets; i++) {
	    /*
	     * Only install the route if the interfaces are on the same 
	     * subnet. We assume that a host will have only one interface
	     * on the same subnet.
	     */
	    address = Net_NetToHostInt(hostPtr->nets[i].inetAddr);
	    if ((infoPtr->subnetMask & address) == subpart) {
		if (address == infoPtr->address) {
		    flags |= NET_ALIAS;
		}
		(void)RteInsertAddress(address, 
			&hostPtr->nets[i].netAddr.address.ether, 
			flags, infoPtr);
		break;
	    }
	}
    }
    Host_End();

#endif /* not KERNEL */

    /*
     * Now that we know our own ethernet address we add the "localhost"
     * address that points back to ourself.
     */
    addrPtr = RteFetchAddress(infoPtr->address);
    if ((addrPtr != (NetAddrInfo *)NULL) && (addrPtr->flags & NET_ETHERNET)) {
	(void)RteInsertAddress(NET_INET_LOCAL_HOST, &addrPtr->addr.ether,
		NET_ALIAS|NET_UPDATE_ADDR, infoPtr);
    }
    /*
     ***********************************************************************
     *
     * Group 3: Gateway info
     *		Format:  <name> <ip-addr> <ether-addr>
     * 		Delimited by "Start_Gateways" and "End_Gateways"
     *
     *	 Assume just 1 gateway for now.
     *
     ***********************************************************************
     */
#ifndef KERNEL
    GetLineList(stream, "Start_Gateways", "End_Gateways", ARRAY_SIZE, array,
		&numGateways);
    if (numGateways == 0) {
	panic("Rte_AddressInit: can't find gateway name in '%s'\n", filename);
    }
    for (i=0 ; i<numGateways ; i++) {
	(void) sscanf(array[i], "%s %s %s", host, inetString, etherString);
	address = Net_NetToHostInt(Net_StringToInetAddr(inetString));  
	Net_StringToEtherAddr(etherString, &etherAddr);
	infoPtr->gatewayPtr = RteInsertAddress(address, &etherAddr,
				NET_NEIGHBOR|NET_GATEWAY, infoPtr);
	free(array[i]);
    }
#endif
    /*
     * Note our address and the various pieces of it for sub-netting.
     */

    address			= infoPtr->address;
    infoPtr->netMask		= Net_InetAddrNetMask(address);
    infoPtr->netPart		= infoPtr->netMask & address;
    infoPtr->subnetMask		|= infoPtr->netMask;
    infoPtr->subnetPart		= infoPtr->subnetMask & address;

    infoPtr->localBrdcastAddr	= (infoPtr->subnetPart | 
			    (NET_INET_BROADCAST_ADDR & ~infoPtr->subnetMask));

    infoPtr->netBrdcastAddr	= (infoPtr->netPart |
				(NET_INET_BROADCAST_ADDR & ~infoPtr->netMask));

    /*
     * Install various BROADCAST addresses that we should accept.
     * The old-style broadcast address uses a host part of 0, hence
     * the use of the (sub)netPart values.
     */
    (void)RteInsertAddress(NET_INET_BROADCAST_ADDR, &broadcast,
			    NET_BROADCAST, infoPtr);
    (void)RteInsertAddress(NET_INET_ANY_ADDR, (Net_EtherAddress *)NULL,
			    NET_BROADCAST, infoPtr);
    (void)RteInsertAddress(infoPtr->localBrdcastAddr, &broadcast,
			    NET_BROADCAST, infoPtr);
    (void)RteInsertAddress(infoPtr->netBrdcastAddr, &broadcast,
			    NET_BROADCAST, infoPtr);
    (void)RteInsertAddress(infoPtr->subnetPart, (Net_EtherAddress *)NULL,
			    NET_BROADCAST, infoPtr);
    (void)RteInsertAddress(infoPtr->netPart, (Net_EtherAddress *)NULL,
			    NET_BROADCAST, infoPtr);

#ifndef KERNEL
    if (ips_Debug) {
	(void) fprintf(stderr, "Host name = %s\n", myHostName);
	(void) fprintf(stderr, "Official address = %s\n", 
			Net_InetAddrToString(infoPtr->address, line));
	(void) fprintf(stderr, "Local broadcast address = %s\n",
			Net_InetAddrToString(infoPtr->localBrdcastAddr, line));
	(void) fprintf(stderr, "Net broadcast address = %s\n",
			Net_InetAddrToString(infoPtr->netBrdcastAddr, line));
    }
#endif

    numNetInterfaces++;
}


/*
 *----------------------------------------------------------------------
 *
 * RteInsertAddress --
 *
 *	Allocate and insert an address entry in the table.  This can be
 *	used to update the ethernet address for an existing entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocate and insert an address entry in the table.
 *
 *----------------------------------------------------------------------
 */

NetAddrInfo *
RteInsertAddress(inet, etherPtr, flags, netPtr)
    Net_InetAddress inet;		/* Internet address, the address KEY */
    Net_EtherAddress *etherPtr;		/* Ethernet address, or NULL */
    int flags;				/* NET_NEIGHBOR, NET_GATEWAY, NET_ALIAS,
					 * NET_BROADCAST, NET_UPDATE_ADDR */
    NetInfo *netPtr;			/* Pointer to interface info */
{
    register NetAddrInfo *addrPtr;
    register Hash_Entry *hashEntryPtr;
    int created;

#ifdef KERNEL
    if (netPtr == (NetInfo *)NIL) {
	netPtr = &netInfo[0];
    }
    if (etherPtr == (Net_EtherAddress *)NIL) {
	etherPtr = (Net_EtherAddress *)NULL;
    }
    hashEntryPtr = Hash_Find(addrTable, inet);
    addrPtr = (NetAddrInfo *)Hash_GetValue(hashEntryPtr);
    if (addrPtr == (NetAddrInfo *)NIL) {
	created = TRUE;
    }
#else
    hashEntryPtr = Hash_CreateEntry(addrTable, inet, &created);
#endif
    if (created) {
	addrPtr = (NetAddrInfo *)malloc(sizeof(NetAddrInfo));
	addrPtr->inet = inet;
	addrPtr->flags = flags & ~NET_ETHERNET;
	addrPtr->netPtr = netPtr;
	Hash_SetValue(hashEntryPtr, addrPtr);
    } else {
	addrPtr = (NetAddrInfo *)Hash_GetValue(hashEntryPtr);
    }
    /*
     * Update the ethernet address if we are told to, or if we are given
     * an address and we don't already have one.
     */
    if ((((addrPtr->flags & NET_ETHERNET) == 0) || (flags & NET_UPDATE_ADDR)) &&
	(etherPtr != (Net_EtherAddress *)NULL)) {
	NET_ETHER_ADDR_COPY(*etherPtr, addrPtr->addr.ether);
	addrPtr->flags |= NET_ETHERNET;
    }
    return(addrPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * RteFetchAddress --
 *
 *	Fetch an address entry from the table.
 *
 * Results:
 *	NULL, or a pointer to the address information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

NetAddrInfo *
RteFetchAddress(inet)
    Net_InetAddress inet;		/* Internet address, the address KEY */
{
    register NetAddrInfo *addrPtr;
    register Hash_Entry *hashEntryPtr;

    hashEntryPtr = Hash_FindEntry(addrTable, inet);
    if (hashEntryPtr != (Hash_Entry *)NULL) {
	addrPtr = (NetAddrInfo *)Hash_GetValue(hashEntryPtr);
	return(addrPtr);
    }
    return((NetAddrInfo *)NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_AddrIsForUs --
 *
 *	Returns TRUE if the specified address can be used to name 
 *	this server. The address is checked to see if it's a broadcast
 *	address and if not, it is compared with official and alias
 *	IP addresses for this server.
 *
 * Results:
 *	TRUE	- the address is for us.
 *	FALSE	- the address is not for us.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Rte_AddrIsForUs(addr)
    Net_InetAddress	addr;
{
    register NetAddrInfo *addrPtr;

    addr = Net_NetToHostInt(addr);
    addrPtr = RteFetchAddress(addr);

    if ((addrPtr != (NetAddrInfo *)0) &&
	(addrPtr->flags & (NET_BROADCAST|NET_ALIAS))) {
	return(TRUE);
    }
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_ValidateAddress --
 *
 *	This routine returns TRUE if the specified address corresponds
 *	to one of the addresses (official or alias) of this server.
 *	The address has to be an exact match -- that is it can't be
 *	a broadcast address.
 *
 * Results:
 *	TRUE	- the address is a valid for this server.
 *	FALSE	- the address doesn't match one of the server's addresses.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Rte_ValidateAddress(addr)
    Net_InetAddress	addr;
{
    register NetAddrInfo *addrPtr;

    addr = Net_NetToHostInt(addr);
    addrPtr = RteFetchAddress(addr);

    if ((addrPtr != (NetAddrInfo *)0) && (addrPtr->flags & NET_ALIAS)) {
	return(TRUE);
    }
    return(FALSE);
}



/*
 *----------------------------------------------------------------------
 *
 * Rte_FindOutputNet --
 *
 *	This routine determines the network interface to use when sending
 *	an IP datagram to the given destination (i.e. address routing).
 *	The caller is given a netID to pass to Rte_OutputPacket routine
 *	that contains the required addressing info.
 *
 *	For the time being, a very simple routing method is used: if the 
 *	destination address is on the same network, and we have an ethernet 
 *	address for it, then the datagram can be sent directly. In all other 
 *	cases, a Unix gateway machine is chosen to receive the datagram.
 *
 *	In the future, this routine should 1) use the address resolution
 *	protocol to find ethernet addresses of directly-connected hosts
 *	and 2) use a routing tables to do proper routing when there's more
 *	than 1 gateway.
 *
 * Results:
 *	TRUE		- a route was found.
 *	FALSE		- no route could be found for the address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Rte_FindOutputNet(dest, netIDPtr, maxOutSizePtr)
    Net_InetAddress	dest;
    register Rte_NetID	*netIDPtr;
    int			*maxOutSizePtr;
{
    register NetInfo	*infoPtr;
    register NetAddrInfo *addrPtr;

#ifdef notdef
    static Net_InetAddress	prevDest;
    static Rte_NetID		prevID;
    static int			prevOutSize = -1;
#endif notdef

    dest = Net_NetToHostInt(dest);
    stats.misc.routeCalls++;

    /*
     * If the destination is the same as the one in the last call,
     * return the same information.
     */
#ifdef notdef
    if (dest == prevDest && prevOutSize != -1) {
	*netIDPtr = prevID;
	*maxOutSizePtr = prevOutSize;
	stats.misc.routeCacheHits++;
	return(TRUE);
    }
#endif notdef

    addrPtr = RteFetchAddress(dest);
    if (addrPtr == (NetAddrInfo *)NULL) {
	if (ips_Debug) {
	    fprintf(stderr, "RteFindOutputNet: !found, inserting gateways\n");
	}
	/*
	 * No address found.  Create an entry with the physical address
	 * of the gateway.  When we get a redirect ICMP then we save
	 * the physical address.  This uses a hack in IPInput
	 * to grab the ethernet source address of packets from lookforIPAddress.
	 */
	infoPtr = &netInfo[0];
	addrPtr = RteInsertAddress(dest, &infoPtr->gatewayPtr->addr.ether,
			0, infoPtr);
	lookforIPAddress = dest;
    } else if (ips_Debug) {
	fprintf(stderr, "RteFindOutputNet: (%x:%x:%x:%x:%x:%x)\n",
		(unsigned char)NET_ETHER_ADDR_BYTE1(addrPtr->addr.ether),
		(unsigned char)NET_ETHER_ADDR_BYTE2(addrPtr->addr.ether),
		(unsigned char)NET_ETHER_ADDR_BYTE3(addrPtr->addr.ether),
		(unsigned char)NET_ETHER_ADDR_BYTE4(addrPtr->addr.ether),
		(unsigned char)NET_ETHER_ADDR_BYTE5(addrPtr->addr.ether),
		(unsigned char)NET_ETHER_ADDR_BYTE6(addrPtr->addr.ether));
    }
    netIDPtr->net = (int)addrPtr->netPtr;
    netIDPtr->host = (int)addrPtr;
    *maxOutSizePtr = addrPtr->netPtr->maxOutputSize;

found:
#ifdef notdef
    prevID = *netIDPtr;
    prevDest = dest;
    prevOutSize = *maxOutSizePtr;
#endif notdef
    return(TRUE);
}



/*
 *----------------------------------------------------------------------
 *
 * Rte_UpdateRoute --
 *
 *	Update a route.  This tells IPInput to look for the 'useAddr'
 *	so we can get a good ethernet address for it.
 * 	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

int
Rte_UpdateRoute(toAddr, useAddr)
    Net_InetAddress	toAddr;	/* What we were using.  */
    Net_InetAddress	useAddr;	/* What we should use. */
{

        register NetInfo	*infoPtr;
	int			i;

    for (i=0; i < numNetInterfaces; i++) {
	infoPtr = &netInfo[i];
	if ((infoPtr->subnetMask & toAddr) == infoPtr->subnetPart) {
	    /*
	     * The address is for our subnet.
	     */
	    lookforIPAddress = toAddr;
	}
    }
    return (0);

}




/*
 *----------------------------------------------------------------------
 *
 * Rte_OutputPacket --
 *
 *	Sends an IP packet on the network. The ethernet header is
 *	formatted to send the packet to the given destination. The
 *	packet type is set to the IP type value.
 *		
 *	This routine only works for ethernets. 
 *
 * Results:
 *	SUCCESS 	- the packet was sent.
 *	FAILURE		- the write failed.
 *
 * Side effects:
 *	A packet is sent out on the wire.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Rte_OutputPacket(netID, packetPtr)
    Rte_NetID	netID;
    register IPS_Packet	*packetPtr;
{
    register NetInfo	*infoPtr;
    register NetAddrInfo *addrPtr;
    ReturnStatus	status;
    int			numWritten;
    int			len;
#ifdef KERNEL
    Net_ScatterGather	ioVector;
#endif

    infoPtr = (NetInfo *)netID.net;
    addrPtr = (NetAddrInfo *)netID.host;

    if ((addrPtr->flags & NET_ETHERNET) == 0) {
	/*
	 * We don't have an ethernet address for this guy.
	 */
	return(FAILURE);
    }

    packetPtr->net.etherPtr = (Net_EtherHdr *)(((Address) packetPtr->ipPtr) - 
					sizeof(*packetPtr->net.etherPtr));
    NET_ETHER_HDR_TYPE(*(packetPtr->net.etherPtr)) = Net_HostToNetShort(NET_ETHER_IP);
    NET_ETHER_ADDR_COPY(addrPtr->addr.ether,
			NET_ETHER_HDR_DESTINATION(*(packetPtr->net.etherPtr)));

    len = packetPtr->ipLen + packetPtr->hdrLen + packetPtr->dataLen;

    if (len > infoPtr->maxOutputSize) {
	panic(
    "Rte_OutputPacket: tried to send too much: ip = %d, hdr = %d, data= %d\n",
		    packetPtr->ipLen, packetPtr->hdrLen, packetPtr->dataLen);
	return(FAILURE);
    }
			
    if (ips_Debug) {
	(void) fprintf(stderr, "Rte_OutputPacket: %d\n", len);
    }

#ifdef TEST_RANDOM_OUTPUT_LOST
    if ((random()&0x1) == 0) {
	printf("Losing a output packet\n");
	return(SUCCESS);
    }
#endif

#ifdef TEST_DISCONNECT

	if (ips_Disconnect) {
	    printf("Losing a output packet\n");
	    return(SUCCESS);
	}
#endif

#ifdef KERNEL
    /*
     * Call the network output routine.   By now the packet has been assembled
     * contiguously.  However, we have separate pointers to the ethernet
     * header and the IP header that follows immediately.
     */
    ioVector.bufAddr 	= (Address)packetPtr->ipPtr;
    ioVector.length	= len;

    Net_EtherOutputSync(packetPtr->net.etherPtr, &ioVector, 1);
    numWritten = len;
    status = SUCCESS;
#else
    /*
     * Write the packet to the network device file.
     */
    len += sizeof(*packetPtr->net.etherPtr);
    numWritten = write(infoPtr->streamID, (Address)packetPtr->net.etherPtr,len);
    status = (numWritten == len) ? SUCCESS : FAILURE; 
    if (status != SUCCESS) {
        perror("Rte_OutputPacket");
    }
    if (numWritten != len) {
	(void) fprintf(stderr, 
	"Rte_OutputPacket: write failed: num written (%d) != len (%d)\n",
	    numWritten, len);
    }
#endif
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * ReadIPPacket --
 *
 *	Reads one or more packets from the IP network device and passes them
 *	to the IP input routine. Called from the FS dispatcher.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory which might not be deallocated immediately.
 *	If the incomming address is lookforIPAddress we update the
 *	routing information for it.
 *
 *----------------------------------------------------------------------
 */

#ifndef KERNEL

/*ARGSUSED*/
static void
ReadIPPacket(data, streamID, eventMask)
    ClientData	data;			/* Network interface number. */
    int		streamID;		/* ID of stream that became readable.*/
    int		eventMask;		/* Mask to show what event happened.
					 * Must be FS_READABLE. */
{
    Rte_NetID		netID;
    int			amtRead;
    int			bufLen = NET_ETHER_MAX_BYTES;
    Boolean		reusePacket = FALSE;
    int			packets = 0;
    NetInfo		*infoPtr;
    NetAddrInfo		*addrPtr;
    IPS_Packet		packet;

#define MAX_PACKETS	4

    infoPtr = &netInfo[(int)data];
    while (packets < MAX_PACKETS) {
	packets++;
	netID.net = (int)infoPtr;
	netID.host = -1;
    
	/*
	 * We have to allocate a buffer from the heap and not from the
	 * stack because the IP routine may need to hang on to it after this
	 * routine returns. To make the SPUR happy, we insure that the ethernet
	 * header starts on a short-word boundry.
	 */
    
	bufLen = NET_ETHER_MAX_BYTES;
    
	if (!reusePacket) {
	    packet.base = malloc((unsigned int) bufLen + 2);
	    packet.dbase = packet.base+2;
	    reusePacket = TRUE;
	}
    
	amtRead = read(streamID, packet.dbase, bufLen);
	if (amtRead < 0) {
	    if (errno == EWOULDBLOCK) {
		break;
	    } else {
		perror("ReadIPPacket (Fs_Read)");
		exit(1);
	    }
	}
	if (amtRead <= 0) {
	    panic("ReadIPPacket: EOF reading network");
	}

	packet.totalLen  = amtRead;
	packet.ipLen = amtRead - sizeof(*packet.net.etherPtr);
	packet.ipPtr = (Net_IPHeader *) (packet.dbase + sizeof(*packet.net.etherPtr));
    
#ifdef TEST_RANDOM_INPUT_LOST
	if ((random()&0x1) == 0) {
	    fprintf(stderr, "Losing an input packet\n");
	    continue;
	}
#endif
#ifdef TEST_DISCONNECT
       if (ips_Disconnect) {
	    printf(stderr, "Losing an input packet\n");
	    continue;
	}
#endif
	/*
	 * PRIMITIVE ROUTING.  Snarf up the ethernet source address of
	 * packets comming from hosts (1) that we want to know about.
	 */
	if (Net_NetToHostInt(packet.ipPtr->source) == lookforIPAddress) {
	    RteInsertAddress(lookforIPAddress,
		NET_ETHER_HDR_SOURCE_PTR(*(Net_EtherHdr *) packet.dbase),
		NET_UPDATE_ADDR, infoPtr);
	    lookforIPAddress = NET_INET_LOCAL_HOST;
	}
    
	if (IP_Input(netID, &packet) == SUCCESS) {
	    reusePacket = FALSE;
	} else {
	    reusePacket = TRUE;;
	}
    }
    if (reusePacket) {
	free(packet.base);
    }
}
#endif /* not KERNEL */


/*
 *----------------------------------------------------------------------
 *
 * IP_PacketInput --
 *
 *	Called to take a packet off the input queue.  We are called
 *	via a Proc_CallFunc initiated from interrupt handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory which might not be deallocated immediately.
 *	If the incomming address is lookforIPAddress we update the
 *	routing information for it.
 *
 *----------------------------------------------------------------------
 */

#ifdef KERNEL

/*ARGSUSED*/
void
IP_PacketInput(protoPtr, callInfoPtr)
    ClientData protoPtr;
    Proc_CallInfo *callInfoPtr;
{
    Rte_NetID		netID;
    NetInfo		*infoPtr;
    NetAddrInfo		*addrPtr;
    Fs_Device		device;
    IPS_Packet		packet;
    int			amtRead;
    ReturnStatus	status;

    infoPtr = &netInfo[0];
    netID.net = (int)infoPtr;
    netID.host = -1;
    
    /*
     * We have to allocate a buffer from the heap and not from the
     * stack because the IP routine may need to hang on to it after this
     * routine returns. To make the SPUR happy, we insure that the ethernet
     * header (dbase) starts on a short word boundary causing the rest of
     * the packet to be word aligned.
     */

    amtRead = NET_ETHER_MAX_BYTES;
    
    packet.base = malloc((unsigned int) amtRead + 2);
    packet.dbase = packet.base+2;

    /*
     * Fake a Fs_Device structure and pass it to DevNet_FsRead to
     * get the packet off of the input queue.
     */
    device.data = protoPtr;
    status = DevNet_FsRead(&device, 0, amtRead, packet.dbase, &amtRead);
    if (status != SUCCESS) {
	goto exit;
    }

    packet.totalLen  = amtRead;
    packet.ipLen = amtRead - sizeof(*packet.net.etherPtr);
    packet.ipPtr = (Net_IPHeader *) (packet.dbase + sizeof(*packet.net.etherPtr));
    
#ifdef TEST_RANDOM_INPUT_LOST
    if ((random()&0x1) == 0) {
	printf("IP_PacketInput: Losing an input packet\n");
	status = FAILURE;
	goto exit;
    }
#endif
#ifdef TEST_DISCONNECT
   if (ips_Disconnect) {
	printf("IP_PacketInput: Losing an input packet\n");
	status = FAILURE;
	goto exit;
    }
#endif
    /*
     * PRIMITIVE ROUTING.  Snarf up the ethernet source address of
     * packets comming from hosts (1) that we want to know about.
     */
    if (Net_NetToHostInt(packet.ipPtr->source) == lookforIPAddress) {
	RteInsertAddress(lookforIPAddress,
	    &NET_ETHER_HDR_SOURCE(*(Net_EtherHdr *) packet.dbase),
	    NET_UPDATE_ADDR, infoPtr);
	lookforIPAddress = NET_INET_LOCAL_HOST;
    }
    status = IP_Input(netID, &packet);
exit:
    if (status != SUCCESS) {
	free(packet.base);
    }
}
#endif /* KERNEL */


/*
 *----------------------------------------------------------------------
 *
 * Rte_GetNetNum --
 *
 *	Returns the Internet network # for a route address specified
 *	by the argument.
 *
 * Results:
 *	An Internet network #.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Rte_GetNetNum(netID)
    Rte_NetID	netID;
{
    return(((NetInfo *)netID.net)->netPart);
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_GetBroadcastAddr --
 *
 *	Returns the official Internet address for a route address specified
 *	by the argument.
 *
 * Results:
 *	An Internet address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_InetAddress
Rte_GetBroadcastAddr(netID)
    Rte_NetID	netID;
{
    return(Net_HostToNetInt( ((NetInfo *)netID.net)->localBrdcastAddr ));
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_GetSubnetMask --
 *
 *	Returns the official Internet address for the route address specified
 *	by the argument.
 *
 * Results:
 *	An Internet address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Rte_GetSubnetMask(netID)
    Rte_NetID	netID;
{
    return(((NetInfo *)netID.net)->subnetMask);
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_GetOfficialAddr --
 *
 *	Returns the official Internet (broadcast) address for this host.
 *
 * Results:
 *	An Internet address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Net_InetAddress
Rte_GetOfficialAddr(broadcastWanted)
    Boolean 	broadcastWanted;	/* If TRUE, return the official 
					 * broadcast address, else the
					 * official address. */
{
    if (broadcastWanted) {
	return(Net_HostToNetInt(netInfo[0].localBrdcastAddr));
    } else {
	return(Net_HostToNetInt(netInfo[0].address));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_IsLocalAddr --
 *	Determines whether an address is local.
 *
 * Results:
 *	Returns TRUE if the address is on the local net (including different
 *	subnets), FALSE otherwise.
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Rte_IsLocalAddr(netID)
    Rte_NetID	netID;			/* Net identifier for remote host */
{
    register NetAddrInfo *addrPtr = (NetAddrInfo *)netID.host;
    int remoteNet = Net_HostToNetInt(addrPtr->inet) &
	((NetInfo *)netID.net)->netMask;
    int localNet = Net_HostToNetInt(netInfo[0].address) & netInfo[0].netMask;

    return(remoteNet == localNet);
}


/*
 *----------------------------------------------------------------------
 *
 * Rte_IsBroadcastAddr --
 *
 *	Checks an Internet address to see if it's a broadcast address.
 *	There are two types of general broadcast addresses: the old style 
 *	(0.0.0.0) and the new style (255.255.255.255). In addition, 
 *	there are broadcast addresses for the local network: e.g.
 *	(128.32.0.0) and (128.32.255.255). If subnetting is in use,
 *	(128.32.150.0) and (128.32.150.255) are examples of subnet
 *	broadcast addresses using the old and new styles.
 *
 *	This depends on these various addresses having been installed.
 *
 * Results:
 *	TRUE	- it is a broadcast address.
 *	FALSE	- it is an address of a host.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Rte_IsBroadcastAddr(address)
    Net_InetAddress	address;
{
    NetAddrInfo *addrPtr;

    address = Net_NetToHostInt(address);
    addrPtr = RteFetchAddress(address);
    if ((addrPtr != (NetAddrInfo *)NULL) && (addrPtr->flags & NET_BROADCAST)) {
	return(TRUE);
    }
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * GetLineList --
 *
 *	Reads in a group of lines delimited by startDelim and endDelim
 *	into arrayPtr. Each line in the array is stored in a separate 
 *	buffer that must be freed when it is no longer needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the lines is allocated.
 *
 *----------------------------------------------------------------------
 */

#ifndef KERNEL
static void
GetLineList(stream, startDelim, endDelim, arraySize, arrayPtr, numLinesPtr)
    FILE	*stream;	/* Stream to read from. */
    char	*startDelim;	/* Starting delimiter, rest of line ignored. */
    char	*endDelim;	/* Ending delimiter, rest of line ignored. */
    int		arraySize;	/* Number of lines in arrayPtr. */
    char	**arrayPtr;	/* Array to store ptrs to each line. */
    int		*numLinesPtr;	/* Actual number of lines stored in arrayPtr.*/
{
    char 	*linePtr;
    int		lineNum = -1;

    /*
     * Search for the start delimiter, ignoring the rest of the line.
     */

    linePtr = malloc(LINE_LEN+1);
    do {
	if (fgets(linePtr, LINE_LEN, stream) == NULL) {
	    goto done;
	}
    } while (strncmp(startDelim, linePtr, strlen(startDelim)) != 0);
    /*

     * Now save all lines until the ending delimiter is seen.
     * Lines starting with # are not saved.
     */

    if (fgets(linePtr, LINE_LEN, stream) == NULL) {
	goto done;
    }
    while (strncmp(endDelim, linePtr, strlen(endDelim)) != 0) {
	if (linePtr[0] != COMMENT_CHAR) {
	    lineNum++;
	    if (lineNum == arraySize) {
		(void) fprintf(stderr, 
			"GetLineList: max num lines reached %d\n", arraySize);
		lineNum--;
		break;
	    }
	    arrayPtr[lineNum] = linePtr;
	    linePtr = malloc(LINE_LEN+1);
	}
	if (fgets(linePtr, LINE_LEN, stream) == NULL) {
	    break;
	}
    }
    /*
     * Free the line that contains the end delimiter.
     */
done:
    free(linePtr);

    *numLinesPtr = lineNum + 1;
}
#endif
