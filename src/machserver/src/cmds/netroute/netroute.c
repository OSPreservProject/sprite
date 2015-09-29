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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/cmds/netroute/RCS/netroute.c,v 1.1 92/01/10 21:24:33 kupfer Exp $ SPRITE (Berkeley)";
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
#include <sysStats.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

/*
 * Parameters set by the command line
 */
Boolean table = FALSE;
Boolean keepDomain = FALSE;
int hostID = -1;
char *hostName;
char *etherString;
char *inetString;
char *inputFile;
char *machType;
char *netTypeString = "ether";
char *ultraString = NULL;
Boolean debug = FALSE;
unsigned int interval = 105;
unsigned int range = 30;
Boolean daemon = FALSE;
Boolean verbose = FALSE;
char *output = NULL;

Option optionArray[] = {
    {OPT_STRING, "f", (Address)&inputFile, "Specifies the host database file."},
    {OPT_TRUE, "p", (Address)&table, "Print out the route table"},
    {OPT_TRUE, "D", (Address)&keepDomain, "Keep domain suffix on host name"},
    {OPT_TRUE, "k", (Address)&debug, "Print debugging information"},
    {OPT_TRUE, "d", (Address)&daemon, 
	    "Run as a daemon, downloading routes when the database changes."},
    {OPT_INT, "T", (Address)&interval, 
	    "Set the interval for the daemon to check the database (seconds)."},
    {OPT_STRING, "h", (Address)&hostName, 
	    "Install route for Sprite host with given name or Sprite ID"},
    {OPT_STRING, "i", (Address)&inetString, 
	    "Internet address for route. (aa.bb.cc.dd)"},
    {OPT_STRING, "e", (Address)&etherString, 
	    "Ethernet address for host. (aa:bb:cc:dd:ee:ff)"},
    {OPT_STRING, "u", (Address) &ultraString, 
	    "Ultranet address for host. (a/b)"},
    {OPT_INT, "n", (Address)&hostID, "Sprite ID for host."},
    {OPT_STRING, "t", (Address)&netTypeString, "Specifies the type of route."},
    {OPT_STRING, "m", (Address)&machType, "Machine type (sun2, spur, etc.)"},
    {OPT_INT, "r", (Address)&range, 
	"Range of random values to add to interval."},
    {OPT_TRUE, "v", (Address)&verbose, "Print out verbose messages"},
    {OPT_STRING, "o", (Address)&output, "Output file for daemon."},
    {OPT_DOC, "\0", (Address)NULL, "If -h is not specified, routes are installed for all hosts in"},
    {OPT_DOC, "\0", (Address)NULL, "the host database."},
};
int numOptions = Opt_Number(optionArray);

void			FixHostName _ARGS_((char *charPtr));
ReturnStatus		InstallRoute _ARGS_((Host_Entry *hostPtr, 
				Net_UltraAddress *ultraAddressPtr));
ReturnStatus		DownloadFile _ARGS_((void));
void			Daemon _ARGS_((unsigned int interval));

extern	ReturnStatus	Proc_GetHostIDs _ARGS_((int *vID, int *pID));
extern	void		sleep _ARGS_((unsigned seconds));

char	*myname;
Net_UltraAddress	*ultraAddressPtr = NULL;


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Grab command line arguments and install routes.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int	argc;
    char *argv[];
{
    ReturnStatus	status = SUCCESS;
    Host_Entry		*host;
    Host_Entry		tmpEntry;
    Host_NetType	netType = HOST_ETHER;
    Net_EtherAddress	*etherAddressPtr = (Net_EtherAddress *) 0;
    Net_InetAddress	inetAddress = 0;
    extern int		errno;
    static Net_Address	netAddress;
    Net_RouteInfo	routeInfo;
    Boolean		newHost = FALSE;

    myname = argv[0];
    argc = Opt_Parse(argc, argv, optionArray, numOptions, OPT_ALLOW_CLUSTERING);
    /*
     * Startup the host file.
     */
    if (inputFile != (char *)0) {
	if (Host_SetFile(inputFile) < 0) {
	    perror("Host_SetFile");
	    exit(errno);
	}
	Host_End();
    }
    routeInfo.version = NET_ROUTE_VERSION;
    bzero((char *) &netAddress, sizeof(netAddress));
    /*
     * Translate and validate the etherString, inetString, and netTypeString
     * arguments.
     */
    if (etherString != (char *) 0) {
	static Net_EtherAddress etherAddress;
	static Net_EtherAddress zeroAddress;
	etherAddressPtr = &etherAddress;
	Net_StringToEtherAddr(etherString, etherAddressPtr);
	if (NET_ETHER_COMPARE(zeroAddress,etherAddress)) {
	    fprintf(stderr,"netroute: Malformed ether address %s\n",
			etherString); 
	    exit(1);
	}
    }

    if (inetString != (char *) 0) {
	inetAddress = Net_StringToInetAddr(inetString);
	if (inetAddress == NET_INET_BROADCAST_ADDR) {
	    fprintf(stderr,"netroute: Malformed inet address %s\n",
			inetString); 
	    exit(1);
	}
    }
    if (ultraString != NULL) {
	ultraAddressPtr = &netAddress.ultra;
	status = Net_StringToAddr(ultraString, NET_PROTO_RAW, NET_NETWORK_ULTRA,
			&netAddress);
	if (status != SUCCESS) {
	    printf(stderr, "netroute: malformed ultranet address %s\n",
		ultraString);
	    exit(1);
	}
    }
    if (strcmp(netTypeString, "ether") == 0) {
	netType = HOST_ETHER;
    } else if (strcmp(netTypeString, "inet") == 0) {
	netType = HOST_INET;
    } else if (strcmp(netTypeString, "ultra") == 0) {
	/* 
	 * TODO: fill this in.
	 */
    } else {
	fprintf("netroute: Bad net type '%s', must be 'ether' or 'inet'.\n", 
		netTypeString);
    }

    if (table) {
	/*
	 * Print out the route table.
	 */
	Net_RouteInfo route;
	int spriteID;
	char	buffer[100];
	char	*typePtr;

	printf("Sprite Route Table:\n");
	Host_Start();
	for (spriteID = 0; ; spriteID++) {

	    status = Sys_Stats(SYS_NET_GET_ROUTE, spriteID, &route);
	    if (status != SUCCESS) {
		break;
	    }
	    if (route.version != NET_ROUTE_VERSION) {
		exit(1);
	    }
	    if (!(route.flags & NET_FLAGS_VALID)) {
		continue;
	    }
	    host = Host_ByID(spriteID);
	    if (host && !keepDomain) {
		FixHostName(host->name);
	    }
	    printf("%5d %-20s", spriteID, 
		(host != NULL ? host->name :
		    (spriteID == NET_BROADCAST_HOSTID ? "BROADCAST" : "???")));
	    printf("%-8s", (host != NULL ? host->machType :
		    (spriteID == NET_BROADCAST_HOSTID ? "" : "???")));
	    switch(route.netType) {
		case NET_NETWORK_ETHER: 
		    typePtr = "ether";
		    break;
		case NET_NETWORK_ULTRA:
		    typePtr = "ultra";
		    break;
		default:
		    typePtr = "unknown";
	    }
	    printf("%-8s", typePtr);
	    (void) Net_AddrToString(&route.netAddress[NET_PROTO_RAW],
		    NET_PROTO_RAW, route.netType, buffer);
	    printf("%s\n", buffer);
	    switch(route.protocol) {
		case NET_PROTO_RAW: 
		    break;
		case NET_PROTO_INET: 
		    (void) Net_AddrToString(&route.netAddress[NET_PROTO_INET],
			NET_PROTO_INET, route.netType, buffer);
		    printf("%34s%-8s%s\n", "", "inet", buffer);
		    break;
		default:
		    printf("Unknown protocol %d\n", route.protocol);
		    break;
	    }
	    printf("%10s flags = 0x%x, refCount = %d, maxBytes = %d\n", "", 
		    route.flags, route.refCount, route.maxBytes);
	}
	Host_End();
	if (status != SUCCESS) {
	    exit(1);
	}
	exit(0);
    } 
    if (daemon) {
	Daemon(interval, range);
    } else if (hostName == (char *)0) {
	DownloadFile();
    }  else {
	/*
	 * Otherwise, Install a route for a single Sprite host. See if 
	 * we can find the default values of the route from the Host 
	 * database.  We know the user specified either a hostID or 
	 * hostName so we lookup in the Host database until we find it.
	 */
	Host_Start();

	/*
	 * First see if it is a valid hostname if not a name then a number.
	 */
	host = Host_ByName(hostName);
	if (host == (Host_Entry *)NULL) {
	    hostID = atoi(hostName);
	    host = Host_ByID(hostID);
	}
	/*
	 * Fill in hostID, hostName, and etherAddress letting the user
	 * specified values override the values in the data base.
	 */
	if (hostID < 0) {
	    if (host == (Host_Entry *)NULL) {
		fprintf(stderr, "netroute: Can not compute spriteID for %s\n",
			 hostName);
		 status = FAILURE;
	    }
	    hostID = host->id;
       }
       if (host == NULL) {
	    host = &tmpEntry;
	    bzero((char *) host , sizeof(host));
	    newHost = TRUE;
       }
       if (hostName != (char *) 0) {
	   host->name = hostName;
       } else if (newHost) {
	    static char nameBuffer[64];
	    sprintf(nameBuffer, "host%d", hostID);
	    host->name = nameBuffer;
	}
    
       if (etherAddressPtr != NULL) {
	   bcopy((char *) etherAddressPtr, host->netAddr.etherAddr, 
	       HOST_ETHER_ADDRESS_SIZE);
       } else if (newHost) {
	    fprintf(stderr, "%s: Need -e option.\n", argv[0]);
	    status = FAILURE;
       }
       if (machType != NULL) {
	  host->machType = machType;
       } else if (newHost) {
	    fprintf(stderr, "%s: Need -e option.\n", argv[0]);
	    status = FAILURE;
       }
       if (inetAddress != 0) {
	  bcopy(&inetAddress, &(host->inetAddr), sizeof(host->inetAddr));
       } else if (newHost && (netType == HOST_INET)) {
	    fprintf(stderr, "netroute: Need -i option.\n");
	    status = FAILURE;
	}
       if (!keepDomain) {
	    FixHostName(host->name);
       }
       if (status == SUCCESS) {
	   status = InstallRoute(host, ultraAddressPtr);
	   if (status != SUCCESS) {
		printf("Couldn't install route to host %s : %s\n", 
		    host->name, Stat_GetMsg(status));
	    }
       }
    }
    Host_End();
    if (status != SUCCESS) {
	exit(1);
    }
   return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FixHostName --
 *
 *	Remove the domain part from a host name.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	String is modified in place.
 *
 *----------------------------------------------------------------------
 */

void
FixHostName(charPtr)
     register char *charPtr;
{

    for (; *charPtr != '\0' ; charPtr++) {
	if (*charPtr == '.') {
	    *charPtr = '\0';
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InstallRoute --
 *
 *	description.
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
InstallRoute(hostPtr, ultraAddressPtr)
    Host_Entry		*hostPtr;
    Net_UltraAddress	*ultraAddressPtr;
{
    Net_RouteInfo	routeInfo;
    ReturnStatus	status = SUCCESS;
    Net_InetAddress	inetAddress;

    bzero((char *) &routeInfo, sizeof(routeInfo));
    routeInfo.spriteID = hostPtr->id;
    routeInfo.version = NET_ROUTE_VERSION;
    /*
     * This is kind of a hack.  We have to be smarter about which
     * interface the route is for.
     */
    routeInfo.interface = 0;
    routeInfo.netType = NET_NETWORK_ETHER;
    strncpy(routeInfo.hostname, hostPtr->name, sizeof(routeInfo.hostname));
    routeInfo.hostname[sizeof(routeInfo.hostname)-1] = '\0';
    strncpy(routeInfo.machType, hostPtr->machType, sizeof(routeInfo.machType));
    routeInfo.machType[sizeof(routeInfo.machType)-1] = '\0';
    if (ultraAddressPtr == NULL) {
	if (hostPtr->netType == HOST_ETHER) {
	    routeInfo.protocol = NET_PROTO_RAW;
	    /*
	     * This is a hack because the types don't match.
	     */
	    bcopy(&hostPtr->netAddr.etherAddr,
		    &routeInfo.netAddress[NET_PROTO_RAW].ether, 6);
	} else if (hostPtr->netType == HOST_INET) {
    
	    routeInfo.protocol = NET_PROTO_INET;
	    bcopy(&hostPtr->netAddr.etherAddr,
		    &routeInfo.netAddress[NET_PROTO_RAW].ether, 6);
	    bcopy(&(hostPtr->inetAddr), &inetAddress, sizeof(hostPtr->inetAddr));
	    inetAddress = Net_HostToNetInt(inetAddress);
	    routeInfo.netAddress[NET_PROTO_INET].inet = inetAddress;
	} else {
	    fprintf(stderr, "Unknown net type %d for %s\n",
		    hostPtr->netType, hostPtr->name);
	    return status;
	}
    } else {
	routeInfo.netType = NET_NETWORK_ULTRA;
	routeInfo.netAddress[NET_PROTO_RAW].ultra = *ultraAddressPtr;
	routeInfo.protocol = NET_PROTO_RAW;
    }
    status = Net_InstallRoute(sizeof(Net_RouteInfo), &routeInfo);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DownloadFile --
 *
 *	Installs all routes from the host database
 *
 * Results:
 *	SUCCESS if all routes were installed, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DownloadFile()
{

    int			spriteID;
    ReturnStatus	status;
    Host_Entry		*hostPtr;

    if (verbose) {
	printf("netroute: Downloading routes from host database.\n");
    }
    /*
     * First we try an internet route to ourselves.  This allows the
     * kernel to know its internet address, which is needed to
     * install internet routes to other machines.  If this step
     * is skipped then the kernel will reverse arp for its address,
     * but that can't always be guaranteed to work.
     */
    Host_Start();
    status = Proc_GetHostIDs(NULL, &spriteID);
    if (status != SUCCESS) {
	printf("Couldn't get sprite ID of the current host.\n");
    } else {
	hostPtr = Host_ByID(spriteID);
	hostPtr->netType = HOST_INET;
	status = InstallRoute(hostPtr, ultraAddressPtr);
	if (status != SUCCESS) {
	    printf("Couldn't install local inet route: %s\n", 
		Stat_GetMsg(status));
	}
    }
    Host_Start();
    while ((hostPtr = Host_Next()) != (Host_Entry *)NULL) {
	if (!keepDomain) {
	    FixHostName(hostPtr->name);
	}
	status = InstallRoute(hostPtr, ultraAddressPtr);
	if (status != SUCCESS) {
	    printf("Couldn't install route to host %s : %s\n", 
		hostPtr->name, Stat_GetMsg(status));
	}
    }
    if (verbose) {
	printf("netroute: Closing host database\n");
    }
    Host_End();
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Daemon --
 *
 *	This procedure is a daemon that will check the status of the
 *	host database every interval, and download the routes if
 *	the database has been modified.
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
Daemon(interval, range)
    unsigned int	interval;
    unsigned int	range;
{
    struct stat		stats;
    time_t		modifyTime;
    Boolean		detach = TRUE;
    int			spriteID;
    struct timeval	bedtime, wakeup;
    int			snooze;
    FILE		*tmp;

    bzero((char *) &stats, sizeof(stats));
    bzero((char *) &modifyTime, sizeof(time_t));
    srandom(getpid());
    printf("Netroute daemon starting up.\n");
    if (output != NULL) {
	tmp = freopen(output, "w", stdout);
	if (tmp == NULL) {
	    printf("netroute: can't open output file \"%s\"\n", output);
	}
    }
    while(1) {
	if (debug) {
	    printf("Checking host database.\n");
	}
	if (Host_Stat(&stats) != 0) {
#if 0
	    printf("%s: stat of host database file failed: %s\n", myname,
		sys_errlist[errno]);
	    /*
	     * Don't exit because stat() will fail if the server is down.
	     * Assume that the server is down and keep on going.
	     */
	    printf("%s exiting\n", myname);
	    exit(1);
#endif
	} else if (modifyTime != stats.st_mtime) {
	    if (verbose) {
		printf(
		"netroute: spritehosts modified, old time %d, new time %d\n",
		    modifyTime, stats.st_mtime);
	    }
	    DownloadFile();
	    modifyTime = stats.st_mtime;
	}
	if (detach) {
	    Proc_Detach(0);
	    detach = FALSE;
	}
	snooze = interval +  (random() % range);
	if (debug) {
	    printf("Snoozing for %d seconds\n", snooze);
	    gettimeofday(&bedtime, NULL);
	}
	sleep(snooze);
	if (debug) {
	    gettimeofday(&wakeup, NULL);
	    printf("snooze = %d\n", snooze);
	    printf("slept = %d\n", wakeup.tv_sec - bedtime.tv_sec);
	}
    }
}

