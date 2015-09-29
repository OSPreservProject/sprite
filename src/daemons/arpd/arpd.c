/* 
 * arpd.c --
 *
 *	A program to reply to ARP and RARP requests.  This program
 *	only translates between Internet and Ethernet addresses.  See
 *	RFC826 for details of the ARP protocol, and RFC903 for details
 *	of the RARP protocol.
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
static char rcsid[] = "$Header: /sprite/src/daemons/arpd/RCS/arpd.c,v 1.5 92/06/16 21:17:19 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <host.h>
#include <option.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

/*
 * Command line options.
 */

static int verbose = 0;
static char *hostFile = NULL;

static Option optionArray[] = {
    {OPT_TRUE,   "v", (char *) &verbose, "Turn on informational output"},
    {OPT_STRING, "f", (char *) &hostFile, "Name of host database file"},
};
static int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 * One structure of the following type is created for each entry
 * in the host table.  This saves us from having to regenerate the
 * host table except when it changes.
 */

typedef struct HostInfo {
    char *name;			/* Textual name for this host. */
    Net_InetAddress inetAddr;	/* Internet address for this host. */
    Net_EtherAddress etherAddr;	/* Ethernet address for this host. */
    struct HostInfo *nextPtr;	/* Next in list of all known hosts (NULL
				 * for end of list). */
} HostInfo;

HostInfo *hostList = NULL;	/* First in list of all known hosts. */
int modTime = -1;		/* The information in memory corresponds to
				 * this modify time on the host file. */

/*
 * The ARP/RARP packet size is defined below.  Because of structure
 * alignment differences between machines, it isn't safe to use
 * sizeof with the structure.
 */

#define ARP_PACKET_SIZE 42

/*
 * The variable below holds the name of the host on which this program
 * is running, and a pointer to the corresponding entry from the hosts
 * file (NULL means no corresponding entry could be found in the hosts
 * file).
 */

char myName[MAXHOSTNAMELEN+1];
HostInfo *myInfoPtr = NULL;

static void		DoArp();
static void		DoRarp();
static void		UpdateTable();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main loop of the ARP request handler. Packets are 
 *	read from the netwok and examined to see if we can make a 
 *	reply.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int		argc;
    char	**argv;
{
    int			arpID, rarpID;

    if (Opt_Parse(argc, argv,  optionArray, numOptions, 0) > 1) {
	printf("arpd ignoring extra arguments\n");
    }

    if (hostFile != NULL) {
	if (Host_SetFile(hostFile)) {
	    perror("arpd couldn't set host file to \"%s\"", hostFile);
	    exit(1);
	}
    }

    /*
     * Open the network devices, and read in the host database.  If
     * either of these fails, then just quit right now.
     */

    arpID = open("/dev/etherARP", O_RDWR, 0600); 
    if (arpID < 0) {
	perror("arpd couldn't open /dev/etherARP");
	exit(1);
    }
    rarpID = open("/dev/etherRARP", O_RDWR, 0600); 
    if (rarpID < 0) {
	perror("arpd couldn't open /dev/etherRARP");
	exit(1);
    }
    if (gethostname(myName, MAXHOSTNAMELEN+1) < 0) {
	perror("arpd couldn't get host name from gethostname()");
	exit(1);
    }
    UpdateTable();
    if (modTime == -1) {
	exit(1);
    }

    /*
     * Enter an infinite loop servicing ARP and RARP requests.
     */

    while (1) {
	int readMask, numReady;

	readMask = (1 << arpID) | (1 << rarpID);
	numReady = select(rarpID+1, &readMask, (int *) NULL,
		(int *) NULL, (struct timeval *) NULL);
	if ((numReady < 0) && (errno != EINTR)) {
	    printf("arpd couldn't select on network devices: %s\n",
		    strerror(errno));
	    exit(1);
	}

	if (readMask & (1 << arpID)) {
	    DoArp(arpID);
	}
	if (readMask & (1 << rarpID)) {
	    DoRarp(rarpID);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DoArp --
 *
 *	Handle an ARP request.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A response is transmitted for the ARP request.
 *
 *----------------------------------------------------------------------
 */

static void
DoArp(streamID)
    int streamID;		/* Integer stream # for ARP device. */
{
    struct {
	struct ether_header hdr;
	struct ether_arp arp;
    } packet;
    int bytesRead, bytesWritten, etherType, protocolType, hardwareType, arpOp;
    unsigned long senderAddr, targetAddr;
    register HostInfo *infoPtr;

    /*
     * Read and validate the ARP packet.
     */

    bytesRead = read(streamID, (char *) &packet, ARP_PACKET_SIZE);
    if (bytesRead < 0) {
	printf("arpd couldn't read ARP packet: %s\n",
		strerror(errno));
	return;
    }
    if (bytesRead < ARP_PACKET_SIZE) {
	printf("arpd got short ARP packet: only %d bytes\n",
		bytesRead);
	return;
    }
    etherType = ntohs(packet.hdr.ether_type);
    if (etherType != ETHERTYPE_ARP) {
	printf("arpd got ARP packet with ether_type 0x%x\n",
		etherType);
	return;
    }
    arpOp = ntohs(packet.arp.ea_hdr.ar_op);
    if (arpOp != ARPOP_REQUEST) {
	if (verbose) {
	    printf("arpd got ARP packet with unknown op %d\n",
		    arpOp);
	}
	return;
    }
    hardwareType = ntohs(packet.arp.ea_hdr.ar_hrd);
    if (hardwareType != ARPHRD_ETHER) {
	if (verbose) {
	    printf("arpd got ARP packet with unknown hardware type 0x%x\n",
		    hardwareType);
	}
	return;
    }
    protocolType = ntohs(packet.arp.ea_hdr.ar_pro);
    if (protocolType != ETHERTYPE_IP) {
	if (verbose) {
	    printf("arpd got ARP packet with unknown protocol type 0x%x\n",
		    protocolType);
	}
	return;
    }
    UpdateTable();

    /*
     * Look for an entry in our host database that matches the given
     * protocol address.  The bcopy calls below are needed because
     * integers may not be properly aligned in the packet.
     */

    bcopy((char *) packet.arp.arp_spa, (char *) &senderAddr,
	    sizeof(senderAddr));
    bcopy((char *) packet.arp.arp_tpa, (char *) &targetAddr,
	    sizeof(targetAddr));
    if (verbose) {
	struct timeval time;
	char *string;
	char buffer1[32];
	char buffer2[32];

	gettimeofday(&time, (struct timezone *) NULL);
	string = ctime(&time.tv_sec);
	string[24] = 0;
	printf("ARP at %s from %s for %s", string, 
	    Net_InetAddrToString(senderAddr, buffer1), 
	    Net_InetAddrToString(targetAddr, buffer2));
    }
    for (infoPtr = hostList; infoPtr != NULL; infoPtr = infoPtr->nextPtr) {
	if (infoPtr->inetAddr == targetAddr) {
	    break;
	}
    }
    if (infoPtr == NULL) {
	if (verbose) {
	    printf(": unknown target\n");
	}
	return;
    }
    if (verbose) {
	char	buffer[20];
	printf(": %s\n", Net_EtherAddrToString(&infoPtr->etherAddr, buffer));
    }

    /*
     * Reverse sender and target fields, and respond with the appropriate
     * Ethernet address.  No need to fill in the source in the packet
     * header:  the kernel automatically overwrites it.
     */

    bcopy((char *) &targetAddr, (char *) packet.arp.arp_spa,
	    sizeof(targetAddr));
    bcopy((char *) &senderAddr, (char *) packet.arp.arp_tpa,
	    sizeof(senderAddr));
    bcopy((char *) packet.arp.arp_sha, (char *) packet.arp.arp_tha,
	    sizeof(Net_EtherAddress));
    bcopy((char *) &infoPtr->etherAddr, (char *) packet.arp.arp_sha,
	    sizeof(Net_EtherAddress));
    packet.arp.ea_hdr.ar_op = htons(ARPOP_REPLY);
    bcopy((char *) packet.hdr.ether_shost, (char *) packet.hdr.ether_dhost,
	    sizeof(Net_EtherAddress));
    bytesWritten = write(streamID, (char *) &packet, ARP_PACKET_SIZE);
    if (bytesWritten < 0) {
	printf("arpd couldn't send ARP response: %s\n",
		strerror(errno));
	return;
    }
    if (bytesWritten != ARP_PACKET_SIZE) {
	printf("arpd short write of ARP response: %d bytes\n",
		bytesWritten);
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DoRarp --
 *
 *	Handle a RARP request.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A response is transmitted for the RARP request.
 *
 *----------------------------------------------------------------------
 */

static void
DoRarp(streamID)
    int streamID;		/* Integer stream # for ARP device. */
{
    struct {
	struct ether_header hdr;
	struct ether_arp arp;
    } packet;
    int bytesRead, bytesWritten, etherType, protocolType, hardwareType, arpOp;
    unsigned long senderAddr, targetAddr;
    register HostInfo *infoPtr;

    /*
     * Read and validate the ARP packet.
     */

    bytesRead = read(streamID, (char *) &packet, ARP_PACKET_SIZE);
    if (bytesRead < 0) {
	printf("arpd couldn't read RARP packet: %s\n",
		strerror(errno));
	return;
    }
    if (bytesRead < ARP_PACKET_SIZE) {
	printf("arpd got short RARP packet: only %d bytes\n",
		bytesRead);
	return;
    }
    etherType = ntohs(packet.hdr.ether_type);
    if (etherType != ETHERTYPE_RARP) {
	printf("arpd got ARP packet with ether_type 0x%x\n",
		etherType);
	return;
    }
    arpOp = ntohs(packet.arp.ea_hdr.ar_op);
    if (arpOp != REVARP_REQUEST) {
	if (verbose) {
	    printf("arpd got RARP packet with unknown op %d\n", arpOp);
	}
	return;
    }
    hardwareType = ntohs(packet.arp.ea_hdr.ar_hrd);
    if (hardwareType != ARPHRD_ETHER) {
	if (verbose) {
	    printf("arpd got RARP packet with unknown hardware type 0x%x\n",
		    hardwareType);
	}
	return;
    }
    protocolType = ntohs(packet.arp.ea_hdr.ar_pro);
    if (protocolType != ETHERTYPE_IP) {
	if (verbose) {
	    printf("arpd got RARP packet with unknown protocol type 0x%x\n",
		    protocolType);
	}
	return;
    }
    UpdateTable();

    /*
     * Look for an entry in our host database that matches the given
     * protocol address.  The bcopy calls below are needed because
     * integers may not be properly aligned in the packet.
     */

    if (verbose) {
	struct timeval time;
	char *string;
	char ether1[20];
	char ether2[20];

	gettimeofday(&time, (struct timezone *) NULL);
	string = ctime(&time.tv_sec);
	string[24] = 0;
	printf("RARP at %s from %s for %s", string, 
		Net_EtherAddrToString((Net_EtherAddress *) packet.arp.arp_sha,
		    ether1),
		Net_EtherAddrToString((Net_EtherAddress *) packet.arp.arp_tha,
		    ether2));
    }
    for (infoPtr = hostList; infoPtr != NULL; infoPtr = infoPtr->nextPtr) {
	if (Net_EtherAddrCmpPtr((Net_EtherAddress *) packet.arp.arp_sha, 
		&infoPtr->etherAddr) == 0) {
	    break;
	}
    }
    if (infoPtr == NULL) {
	if (verbose) {
	    printf(": unknown target\n");
	}
	return;
    }
    targetAddr = infoPtr->inetAddr;
    if (verbose) {
	char	inet[32];
	printf(": %s\n", Net_InetAddrToString((Net_InetAddress) targetAddr,
		    inet));
    }

    /*
     * Reverse sender and target fields, and respond with the appropriate
     * Internet address.  No need to fill in the source in the packet
     * header:  the kernel automatically overwrites it.
     */

    bcopy((char *) packet.arp.arp_sha, (char *) packet.arp.arp_tha,
	    sizeof(Net_EtherAddress));
    bcopy((char *) &targetAddr, (char *) packet.arp.arp_tpa,
	    sizeof(targetAddr));
    if (myInfoPtr != NULL) {
	bcopy((char *) &myInfoPtr->etherAddr, (char *) packet.arp.arp_sha,
		sizeof(Net_EtherAddress));
	senderAddr = myInfoPtr->inetAddr;
	bcopy((char *) &senderAddr, (char *) packet.arp.arp_spa,
		sizeof(senderAddr));
    } else {
	bzero((char *) packet.arp.arp_sha, sizeof(Net_EtherAddress));
	bzero((char *) packet.arp.arp_spa, sizeof(senderAddr));
    }
    packet.arp.ea_hdr.ar_op = htons(REVARP_REPLY);
    bcopy((char *) packet.hdr.ether_shost, (char *) packet.hdr.ether_dhost,
	    sizeof(Net_EtherAddress));
    bytesWritten = write(streamID, (char *) &packet, ARP_PACKET_SIZE);
    if (bytesWritten < 0) {
	printf("arpd couldn't send RARP response: %s\n",
		strerror(errno));
	return;
    }
    if (bytesWritten != ARP_PACKET_SIZE) {
	printf("arpd short write of RARP response: %d bytes\n",
		bytesWritten);
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateTable --
 *
 *	Make sure that the host information in memory is up-to-date.
 *	If not, throw away what we've got and read in fresh stuff.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The information in the list pointed to by hostList is
 *	potentially modified, as is the modTime variable.  The pointer
 *	myInfoPtr is (re-)set to point to the information for the
 *	host named "myName".
 *
 *----------------------------------------------------------------------
 */

static void
UpdateTable()
{
    struct stat buf;
    register HostInfo *infoPtr;
    register Host_Entry *hostPtr;
    int			i;
    ReturnStatus	status;

    /*
     * See if the current database is up-to-date.  If so,
     * then just return.
     */

    if (Host_Stat(&buf) != 0) {
	printf("arpd couldn't stat host file: %s\n", strerror(errno));
	return;
    }
    if (buf.st_mtime == modTime) {
	return;
    }

    /*
     * Throw away everything that's currently in memory.
     */

    while (hostList != NULL) {
	infoPtr = hostList;
	hostList = infoPtr->nextPtr;
	free(infoPtr->name);
	free((char *) infoPtr);
    }

    /*
     * Rebuild the database from the host database file.
     */

    if (verbose) {
	char *timeString;
	timeString = ctime(&buf.st_mtime);
	timeString[24] = 0;
	printf("arpd reloading database from host file (modified %s)\n",
	    timeString);
    }

    if (Host_Start() != 0) {
	printf("arpd: Host_Start failed : %s\n", strerror(errno));
	return;
    }

    myInfoPtr = NULL;
    for (hostPtr = Host_Next(); hostPtr != NULL; hostPtr = Host_Next()) {
	for (i = 0; i < hostPtr->numNets; i++) {
	    if (hostPtr->nets[i].netAddr.type == NET_ADDRESS_ETHER) {
		infoPtr = (HostInfo *) malloc(sizeof(HostInfo));
		infoPtr->name = malloc((unsigned) (strlen(hostPtr->name) + 1));
		strcpy(infoPtr->name, hostPtr->name);
		infoPtr->inetAddr = hostPtr->nets[i].inetAddr;
		status = Net_GetAddress(&hostPtr->nets[i].netAddr, 
			    &infoPtr->etherAddr);
		if (status != SUCCESS) {
		    printf("arpd: Net_GetAddress failed\n");
		    goto done;
		}
		infoPtr->nextPtr = hostList;
		hostList = infoPtr;
		if (strcmp(myName, infoPtr->name) == 0) {
		    myInfoPtr = infoPtr;
		}
		break;
	    }
	}
    }
done:
    Host_End();
    modTime = buf.st_mtime;
}
