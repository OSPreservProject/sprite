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
#endif not lint

#include "sprite.h"

#include "list.h"
#include "net.h"
#include "netInt.h"
#include "devNet.h"
#include "sync.h"
#include "dbg.h"

Net_EtherStats	net_EtherStats;
NetEtherFuncs	netEtherFuncs;
static	Sync_Semaphore	outputMutex;
static void	EnterDebugger();

/*
 * Network configuration table defined by machine dependent code.
 */
extern NetInterface	netInterface[];
extern int		numNetInterfaces;

#define	INC_BYTES_SENT(gatherPtr, gatherLength) { \
	register	Net_ScatterGather	*gathPtr; \
	int					i; \
	net_EtherStats.bytesSent += sizeof(Net_EtherHdr); \
	for (i = gatherLength, gathPtr = gatherPtr; i > 0; i--, gathPtr++) { \
	    net_EtherStats.bytesSent += gathPtr->length; \
	} \
    }


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
    register int inter;

    /*
     * Zero out the statistics struct.
     */
    bzero((Address) &net_EtherStats, sizeof(net_EtherStats));

    /*
     * Determine the number and kind of network interfaces by calling
     * each network interface initialization procedure.
     */
    for (inter = 0 ; inter<numNetInterfaces ; inter++) {
	if ((*netInterface[inter].init)(netInterface[inter].name,
					netInterface[inter].number,
					netInterface[inter].ctrlAddr)) {
	    printf("%s-%d net interface at 0x%x\n",
		netInterface[inter].name,
		netInterface[inter].number,
		netInterface[inter].ctrlAddr);
	} 
    }
    Sync_SemInitDynamic(&outputMutex, "Net:outputMutex");
    /*
     * Pre-load some addresses, including the broadcast address.
     */
    Net_RouteInit();
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
    Mem_Bin(NET_ETHER_MAX_BYTES);
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
}



/*
 *----------------------------------------------------------------------
 *
 * Net_Reset --
 *
 *	Reset the network controllers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reinitializes the the ethernet chips.
 *
 *----------------------------------------------------------------------
 */
void
Net_Reset()
{
    netEtherFuncs.reset();
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
Net_Output(spriteID, gatherPtr, gatherLength, mutexPtr)
    int spriteID;			/* Host to which to send the packet */
    Net_ScatterGather *gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet */
    int gatherLength;			/* Number of elements in gatherPtr[] */
    Sync_Semaphore *mutexPtr;		/* Mutex that is released during the
					 * ARP transaction (if needed).  This
					 * doesn't mess up the caller because
					 * its packet isn't output until
					 * the ARP completes anyway */
{
    register Net_Route *routePtr;

    if (spriteID < 0 || spriteID >= NET_NUM_SPRITE_HOSTS) {
	return(NET_UNREACHABLE_NET);
    }

    /*
     * Check for a route to the indicated host.  Use ARP to find it if needed.
     */
    routePtr = netRouteArray[spriteID];
    if (routePtr == (Net_Route *)NIL) {
	routePtr = Net_Arp(spriteID, mutexPtr);
	if (routePtr == (Net_Route *)NIL) {
	    return(NET_UNREACHABLE_NET);
	}
    }

    switch(routePtr->type) {
	case NET_ROUTE_ETHER: {

		/*
		 * Still need to decide which interface to use on a 
		 * machine with more than one...
		 */

		INC_BYTES_SENT(gatherPtr, gatherLength);
		gatherPtr->done = FALSE;
		gatherPtr->mutexPtr = mutexPtr;
		(netEtherFuncs.output)((Net_EtherHdr *)routePtr->data, 
					    gatherPtr, gatherLength);
		while (!gatherPtr->done && mutexPtr != (Sync_Semaphore *)NIL) {
		    Sync_SlowMasterWait((unsigned int)mutexPtr, mutexPtr, 0);
		}
		return(SUCCESS);
	    }
	default:
	    printf("Warning: Net_Output: unsupported route type: %x\n", 
			routePtr->type);
	    return(FAILURE);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Net_OutputRawEther --
 *
 *	Send a packet directly onto the ethernet.
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
Net_OutputRawEther(etherHdrPtr, gatherPtr, gatherLength)
    Net_EtherHdr	*etherHdrPtr;	/* Ethernet header. */
    Net_ScatterGather	*gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet */
    int 		gatherLength;	/* Number of elements in gatherPtr[] */
{
    INC_BYTES_SENT(gatherPtr, gatherLength);
    (netEtherFuncs.output)(etherHdrPtr, gatherPtr, gatherLength);
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
Net_RecvPoll()
{
    netEtherFuncs.intr(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_EtherOutputSync --
 *
 *	Send a packet to a host identified by a Sprite Host ID and wait
 *	for the packet to be sent.
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
Net_EtherOutputSync(etherHdrPtr, gatherPtr, gatherLength)
    Net_EtherHdr	*etherHdrPtr;	/* Pointer to ethernet header. */
    Net_ScatterGather 	*gatherPtr;	/* Specifies buffers containing the
					 * pieces of the packet */
    int 		gatherLength;	/* Number of elements in gatherPtr[] */
{
    Sync_Condition	condition;

    gatherPtr->mutexPtr = &outputMutex;
    gatherPtr->done = FALSE;

    MASTER_LOCK(&outputMutex);

    INC_BYTES_SENT(gatherPtr, gatherLength);
    netEtherFuncs.output(etherHdrPtr, gatherPtr, gatherLength);
    while (!gatherPtr->done) {
	Sync_SlowMasterWait(&outputMutex, &outputMutex, FALSE);
    }

    MASTER_UNLOCK(&outputMutex);
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
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
    Sync_SlowBroadcast((unsigned int)mutexPtr, &waiting);
#else 	/* Mutiprocessor implementation */
   /*
    * Because the packet sent interrupt may come in before Net_Output
    * has a chance to MasterWait and after Net_Output has checked the
    * gatherPtr->done flag, the code should syncronize with the caller
    * by obtaining the master lock.
    */
    MASTER_LOCK(mutexPtr);
    Sync_SlowBroadcast((unsigned int) mutexPtr, &waiting);	
    MASTER_UNLOCK(mutexPtr);
#endif
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

void
Net_Intr()
{
    (netEtherFuncs.intr)(FALSE);
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
 * Results:
 *	None.
 *
 * Side effects:
 *	None.  
 *
 *----------------------------------------------------------------------
 */

void
Net_Input(packetPtr, packetLength)
    Address packetPtr;
    int packetLength;
{
    register Net_EtherHdr *etherHdrPtr;
    int		type;

    if (dbg_UsingNetwork) {
	Dbg_InputPacket(packetPtr, packetLength);
	return;
    }
    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    type = Net_NetToHostShort(NET_ETHER_HDR_TYPE(*etherHdrPtr));
    switch(type) {
        case NET_ETHER_SPRITE:
	    net_EtherStats.bytesReceived += packetLength;
            Rpc_Dispatch(packetPtr, packetLength);
            break;

        case NET_ETHER_SPRITE_ARP:
            NetArpInput(packetPtr, packetLength);
            break;

        case NET_ETHER_SPRITE_DEBUG:
            EnterDebugger(packetPtr, packetLength);
            break;

	default:
	    DevNetEtherHandler(packetPtr, packetLength);
	    break;
    }
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
EnterDebugger(packetPtr, packetLength)
    Address packetPtr;
    int packetLength;
{
    char *name;
    int  len;

    /*
     * Skip over the Ethernet header.
     */
    packetPtr += sizeof(Net_EtherHdr);
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
}
