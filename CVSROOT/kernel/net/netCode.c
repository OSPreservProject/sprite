/* 
 * netCode.c --
 *
 *	Various routines for initialzation, input and output.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "machine.h"

#include "sys.h"
#include "list.h"
#include "net.h"
#include "netInt.h"
#include "byte.h"
#include "dbg.h"

Net_EtherStats	net_EtherStats;
NetEtherFuncs	netEtherFuncs;
static	int	outputMutex = 0;
static void	EnterDebugger();

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

	Byte_Copy(scatterGatherPtr->length,
		  (Address) scatterGatherPtr->bufAddr, 
		  (Address) &(destAddr[soFar]));
	soFar += scatterGatherPtr->length;
    }
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
    int machineType;

    /*
     * Zero out the statistics struct.
     */
    Byte_Zero(sizeof(net_EtherStats), (Address) &net_EtherStats);

    /*
     * Set up the struct of functions to be called depending on the 
     * machine type.
     */
    machineType = Mach_GetMachineType();
    switch(machineType) {
	case SYS_SUN_2_120:
	    netEtherFuncs.init   = Net3CInit;
	    netEtherFuncs.output = Net3COutput;
	    netEtherFuncs.intr   = Net3CIntr;
	    netEtherFuncs.reset  = Net3CRestart;
	    break;

	case SYS_SUN_2_50: /* SYS_SUN_2_160 has the same value */
	case SYS_SUN_3_75: /* SYS_SUN_3_160 has the same value */
	    netEtherFuncs.init   = NetIEInit;
	    netEtherFuncs.output = NetIEOutput;
	    netEtherFuncs.intr   = NetIEIntr;
	    netEtherFuncs.reset  = NetIERestart;
	    break;

	default:
	    Sys_Panic(SYS_FATAL, 
			"Net_Init: unknown machine type: %d\n", machineType);
	    break;
    }

    /*
     * Call the initialization routine.
     */
    (netEtherFuncs.init)();

    /*
     * Pre-load some addresses, including the broadcast address.
     */
    Net_RouteInit();
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
    int *mutexPtr;			/* Mutex that is released during the
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
	routePtr = NetArp(spriteID, mutexPtr);
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
		(netEtherFuncs.output)((Net_EtherHdr *)routePtr->data, 
					    gatherPtr, gatherLength);
		return(SUCCESS);
	    }
	default:
	    Sys_Panic(SYS_WARNING, 
		"Net_Output: unsupported route type: %x\n", routePtr->type);
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

    gatherPtr->conditionPtr = &condition;
    gatherPtr->done = FALSE;

    MASTER_LOCK(outputMutex);

    INC_BYTES_SENT(gatherPtr, gatherLength);
    netEtherFuncs.output(etherHdrPtr, gatherPtr, gatherLength);
    while (!gatherPtr->done) {
	Sync_MasterWait(&condition, &outputMutex, FALSE);
    }

    MASTER_UNLOCK(outputMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * NetOutputWakeup --
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
NetOutputWakeup(conditionPtr)
    Sync_Condition	*conditionPtr;
{
    MASTER_LOCK(outputMutex);
    Sync_MasterBroadcast(conditionPtr);
    MASTER_UNLOCK(outputMutex);
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

    if (dbg_UsingNetwork) {
	Dbg_InputPacket(packetPtr, packetLength);
	return;
    }
    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    switch(etherHdrPtr->type) {
        case NET_ETHER_SPRITE:
	    net_EtherStats.bytesReceived += packetLength;
            Rpc_Dispatch(packetPtr, packetLength);
            break;

        case NET_ETHER_SPRITE_ARP:
            Net_ArpInput(packetPtr, packetLength);
            break;

        case NET_ETHER_SPRITE_DEBUG:
            EnterDebugger(packetPtr, packetLength);
            break;

	default:
	    NetEtherHandler(packetPtr, packetLength);
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

    len = *(int *) packetPtr;

    /*
     * Validate the data length and make sure the name is null-terminated.
     */
    if (len < 100) {
	name = (char *) (packetPtr + sizeof(len));
	name[len] = '\0';
	Sys_Printf("\n*** Got a debugger packet from %s ***\n", name);
    } else {
	Sys_Printf("\n*** Got a debugger packet ***\n");
    }

    DBG_CALL;
}
