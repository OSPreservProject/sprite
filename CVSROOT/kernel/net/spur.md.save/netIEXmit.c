/* 
 * netIEXmit.c --
 *
 *	Routines to transmit packets on the Intel interface.
 *
 * Copyright 1988 Regents of the University of California
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
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"
#include "list.h"

#include "sync.h"

/*
 * Pointer to scatter gather element for current packet being sent.
 */
Net_ScatterGather *curScatGathPtr = (Net_ScatterGather *) NIL;

/*
 * The address of the buffer descriptor header.
 */
static	NetIETransmitBufDesc *xmitBufAddr;

/*
 * The address of the data for the current packet being sent.
 */
static Address xmitDataBuffer;

/*
 * A buffer that is used when handling loop back packets.
 */
static  char            loopBackBuffer[NET_ETHER_MAX_BYTES];


/*
 *----------------------------------------------------------------------
 *
 * OutputPacket --
 *
 *	Assemble and output the packet in the given scatter/gather element.
 *	The ethernet header contains the address of the destination host
 *	and the higher level protocol type already.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Transmit command list is modified to contain the packet.
 *
 *----------------------------------------------------------------------
 */

static void
OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength)
    Net_EtherHdr			*etherHdrPtr;
    register	Net_ScatterGather   	*scatterGatherPtr;
    int					scatterGatherLength;
{
    register	NetIETransmitBufDesc	*xmitBufDescPtr;
    register	NetIETransmitCB   	*xmitCBPtr;
    int					i;
    int					length;
    int					totalLength;

	static int	printLength;

    netIEState.transmitting = TRUE;
    curScatGathPtr = scatterGatherPtr;

    /*
     * There is already a prelinked command list.  A pointer to the list
     * and the array of buffer headers is gotten here.
     */

    xmitCBPtr = netIEState.xmitCBPtr;
    xmitBufDescPtr = xmitBufAddr;

    length = 0;
    for (i = 0; i < scatterGatherLength; i++) {
            length += scatterGatherPtr[i].length;
    }
    totalLength = length + sizeof(Net_EtherHdr);
    /*
     * Copy all of the pieces of the packet into the xmit buffer.
     */
    Net_GatherCopy(scatterGatherPtr, scatterGatherLength, xmitDataBuffer);
    xmitBufDescPtr->count = length;

     /*
     * If the packet was too short, then hang some extra storage off of the
     * end of it.
     */
    if (totalLength < NET_ETHER_MIN_BYTES) {
	xmitBufDescPtr->count = NET_ETHER_MIN_BYTES;
    }

	printLength =  xmitBufDescPtr->count;
    /*
     * Finish off the packet.
     */

    xmitBufDescPtr->eof = 1;
    NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
				xmitCBPtr->destEtherAddr);
    xmitCBPtr->type = NET_ETHER_HDR_TYPE(*etherHdrPtr);


    /*
     * Append the command onto the command queue.
     */

    *(short *) xmitCBPtr = 0;      /* Clear the status bits. */
    xmitCBPtr->endOfList = 1;      /* Mark this as the end of the list. */
    xmitCBPtr->interrupt = 1;      /* Have the command unit interrupt us when
                                      it is done. */

    /*
     * Make sure that the last command was accepted and then
     * start the command unit.
     */

    NET_IE_CHECK_SCB_CMD_ACCEPT(netIEState.scbPtr);
    netIEState.scbPtr->cmdUnitCmd = NET_IE_CUC_START;
    NET_IE_CHANNEL_ATTENTION;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEXmitInit --
 *
 *	Initialize the transmission queue structures.  This includes setting
 *	up a template transmission command block and then if any packets are
 *	ready starting to transmit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The transmission command block is initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetIEXmitInit()
{
    register	NetIETransmitCB		*xmitCBPtr;
    register	NetIETransmitBufDesc	*xmitBufDescPtr;
    NetXmitElement	                *xmitElementPtr;

    /*
     * Initialize the transmit command header.
     */

    xmitCBPtr = (NetIETransmitCB *) netIEState.cmdBlockPtr;
    netIEState.xmitCBPtr = xmitCBPtr;
    xmitCBPtr->cmdNumber = NET_IE_TRANSMIT;
    xmitCBPtr->suspend = 0;

    /*
     * Now link in all of the buffer headers. For SPUR, we only use one buffer.
     */

    xmitBufDescPtr = 
	(NetIETransmitBufDesc *) NetIEMemAlloc(sizeof(NetIETransmitBufDesc));
    if (xmitBufDescPtr == (NetIETransmitBufDesc *) NIL) {
	panic( "Intel: No memory for the xmit buffers.\n");
    }
    xmitBufAddr = xmitBufDescPtr;
    xmitCBPtr->bufDescOffset = 
		    NetIEOffsetFromSPURAddr((Address) xmitBufDescPtr);

    /*
     *  Allocate some space for the transmit data.
     */
    xmitDataBuffer = NetIEMemAlloc(NET_IE_XMIT_BUFFER_SIZE);
    xmitBufDescPtr->bufAddr = NetIEAddrFromSPURAddr(xmitDataBuffer);

    /*
     * If there are packets on the queue then go ahead and send 
     * the first one.
     */

    if (!List_IsEmpty(netIEState.xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(netIEState.xmitList);
	OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(netIEState.xmitFreeList));
    } else {
	netIEState.transmitting = FALSE;
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEXmitDone --
 *
 *	This routine will process a completed transmit command.  It will
 *	remove the command from the front of the transmit queue, 
 *	and wakeup any waiting process.
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
NetIEXmitDone()
{
    register	NetXmitElement	*xmitElementPtr;
    register	NetIETransmitCB	*cmdPtr;

    MASTER_LOCK(&netIEMutex);

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (curScatGathPtr == (Net_ScatterGather *) NIL) {
	printf("Warning: NetIEXmitDone: No current packet\n.");
	MASTER_UNLOCK(&netIEMutex);
	return;
    }

    net_EtherStats.packetsSent++;

    /*
     * Mark the packet as done.
     */
    curScatGathPtr->done = TRUE;
    if (curScatGathPtr->conditionPtr != (Sync_Condition *) NIL) {
	NetOutputWakeup(curScatGathPtr->conditionPtr);
    }

    /*
     * Record statistics about the packet.
     */
    cmdPtr = netIEState.xmitCBPtr;
    if (cmdPtr->tooManyCollisions) {
	net_EtherStats.xmitCollisionDrop++;
	net_EtherStats.collisions += 16;
    } else {
	net_EtherStats.collisions += cmdPtr->numCollisions;
    }

    if (!cmdPtr->cmdOK) {
	net_EtherStats.xmitPacketsDropped++;
    }

    /*
     * If there are more packets to send then send the first one on
     * the queue.  Otherwise there is nothing being transmitted.
     */
    if (!List_IsEmpty(netIEState.xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(netIEState.xmitList);
	OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(netIEState.xmitFreeList));
    } else {
	netIEState.transmitting = FALSE;
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    MASTER_UNLOCK(&netIEMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEOutput --
 *
 *	Output a packet.  The procedure is to either put the packet onto the 
 *	queue of outgoing packets if packets are already being sent, or 
 *	otherwise to send the packet directly.
 * Results:
 *	None.
 *
 * Side effects:
 *	Queue of packets modified.
 *
 *----------------------------------------------------------------------
 */

void
NetIEOutput(etherHdrPtr, scatterGatherPtr, scatterGatherLength)
    Net_EtherHdr			*etherHdrPtr;
    register	Net_ScatterGather	*scatterGatherPtr;
    int					scatterGatherLength;
{
    register	NetXmitElement		*xmitPtr;


    MASTER_LOCK(&netIEMutex);

    net_EtherStats.packetsOutput++;

    /*
     * See if the packet is for us.  In this case just copy in the packet
     * and call the higher level routine.
     */

    if (NET_ETHER_COMPARE(netIEState.etherAddress, *etherHdrPtr)) {
	int i, length;

        length = sizeof(Net_EtherHdr);
        for (i = 0; i < scatterGatherLength; i++) {
            length += scatterGatherPtr[i].length;
        }

        if (length <= NET_ETHER_MAX_BYTES) {
	    register Address bufPtr;

	    NET_ETHER_ADDR_COPY(netIEState.etherAddress,
				NET_ETHER_HDR_SOURCE(*etherHdrPtr));

	    bufPtr = (Address)loopBackBuffer;
	    bcopy((char *)*etherHdrPtr, (char *) bufPtr,sizeof(Net_EtherHdr));
	    bufPtr += sizeof(Net_EtherHdr);
            Net_GatherCopy(scatterGatherPtr, scatterGatherLength, bufPtr);

	    Net_Input((Address)loopBackBuffer, length);
        }

        scatterGatherPtr->done = TRUE;

	MASTER_UNLOCK(&netIEMutex);
	return;
    }

    /*
     * If no packet is being sent then go ahead and send this one.
     */

    if (!netIEState.transmitting) {
	OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength);
	MASTER_UNLOCK(&netIEMutex);
	return;
    }

    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */

    if (List_IsEmpty(netIEState.xmitFreeList)) {
        scatterGatherPtr->done = TRUE;
	MASTER_UNLOCK(&netIEMutex);
	return;
    }

    xmitPtr = (NetXmitElement *) List_First((List_Links *) netIEState.xmitFreeList);

    List_Remove((List_Links *) xmitPtr);

    /*
     * Initialize the list element.
     */

    xmitPtr->etherHdrPtr = etherHdrPtr;
    xmitPtr->scatterGatherPtr = scatterGatherPtr;
    xmitPtr->scatterGatherLength = scatterGatherLength;

    /* 
     * Put onto the transmission queue.
     */

    List_Insert((List_Links *) xmitPtr, LIST_ATREAR(netIEState.xmitList)); 

    MASTER_UNLOCK(&netIEMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEXmitRestart --
 *
 *	Restart transmission of packets after a chip reset.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Current scatter gather pointer is reset and new packets may be
 *	sent out.
 *
 *----------------------------------------------------------------------
 */
void
NetIEXmitRestart()
{
    NetXmitElement	*xmitElementPtr;

    /*
     * Assume that MASTER_LOCK on &netIEMutex is held by caller.
     */

    /*
     * Drop the current outgoing packet.
     */    
    if (curScatGathPtr != (Net_ScatterGather *) NIL) {
	curScatGathPtr->done = TRUE;
	if (curScatGathPtr->conditionPtr != (Sync_Condition *) NIL) {
	    NetOutputWakeup(curScatGathPtr->conditionPtr);
	}
    }

    /*
     * Start output if there are any packets queued up.
     */
    if (!List_IsEmpty(netIEState.xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(netIEState.xmitList);
	    OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(netIEState.xmitFreeList));
    } else {
	netIEState.transmitting = FALSE;
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
}
