/* 
 * netLEXmit.c --
 *
 *	Routines to transmit packets on the LANCE interface.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "vm.h"
#include "list.h"
#include "sync.h"
#include "machMon.h"

/*
 * Pointer to scatter gather element for current packet being sent.
 */
static Net_ScatterGather *curScatGathPtr = (Net_ScatterGather *) NIL;

/*
 * A buffer that is used when handling loop back packets.
 */
static  char            loopBackBuffer[NET_ETHER_MAX_BYTES];

/*
 * Buffer that we shove the packet into.
 */
static volatile Address	xmitBufPtr;


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
 *	FAILURE if something went wrong.
 *
 * Side effects:
 *	Transmit command list is modified to contain the packet.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength)
    Net_EtherHdr		*etherHdrPtr;	/* Ethernet header of packet.*/
    register Net_ScatterGather	*scatterGatherPtr; /* Data portion of packet.*/
    int				scatterGatherLength;/* Length of data portion 
						     * gather array. */
{
    NetLEState		*netLEStatePtr;
    short		*inBufPtr;
    volatile short	*outBufPtr;
    Address		descPtr;
    int			length;
    unsigned	char	*leftOverBytePtr = (unsigned char *)NIL;
    int			totLen;

    netLEStatePtr = &netLEState;
    descPtr = netLEStatePtr->xmitDescFirstPtr;

    /*
     * Do a sanity check.
     */
    if (*BUF_TO_ADDR(descPtr, NET_LE_XMIT_STATUS1) & NET_LE_XMIT_CHIP_OWNED) {
	printf("LE ethernet: Transmit buffer owned by chip.\n");
	return (FAILURE);
    }

    netLEStatePtr->transmitting = TRUE;
    curScatGathPtr = scatterGatherPtr;
    etherHdrPtr->source = netLEStatePtr->etherAddress;
    outBufPtr = (volatile short *)xmitBufPtr;

    /* 
     * Copy the packet into the xmit buffer.  Don't be general, be fast.
     * First do the ethernet header.
     */
    inBufPtr = (short *)etherHdrPtr;
    for (totLen = 0; 
	 totLen < sizeof(Net_EtherHdr); 
	 totLen += 2, outBufPtr += 2, inBufPtr += 1) {
	*outBufPtr = *inBufPtr;
    }
    /*
     * Now do each element of the scatter/gather array.
     */
    for (; scatterGatherLength > 0; scatterGatherLength--,scatterGatherPtr++ ) {
	unsigned char *bufAddr;

	length = scatterGatherPtr->length;
	if (length == 0) {
	    continue;
	}
	totLen += length;
	if (totLen > NET_ETHER_MAX_BYTES) {
	    printf("OutputPacket: Packet too large\n");
	    curScatGathPtr = (Net_ScatterGather *)NIL;
	    netLEStatePtr->transmitting = FALSE;
	    return(FAILURE);
	}
	bufAddr = (unsigned char *)scatterGatherPtr->bufAddr;
	/*
	 * Copy the element into the buffer.
	 */
	if (leftOverBytePtr != (unsigned char *)NIL) {
	    /*
	     * We had one byte left over in the last piece of the packet.
	     * Concatenate this byte with the first byte of this piece.
	     */
	    *outBufPtr = *leftOverBytePtr | (*bufAddr << 8);
	    leftOverBytePtr = (unsigned char *)NIL;
	    bufAddr++;
	    outBufPtr += 2;
	    length--;
	}
	if ((unsigned)bufAddr & 1) {
	    while (length > 1) {
		*outBufPtr = *bufAddr | (*(bufAddr + 1) << 8);
		outBufPtr += 2;
		bufAddr += 2;
		length -= 2;
	    }
	} else {
	    while (length > 1) {
		*outBufPtr = *(short *)bufAddr;
		outBufPtr += 2;
		bufAddr += 2;
		length -= 2;
	    }
	}
	if (length == 1) {
	    leftOverBytePtr = bufAddr;
	}
    }
    if (leftOverBytePtr != (unsigned char *)NIL) {
	*outBufPtr = *leftOverBytePtr;
    }

    /*
     * Add the buffer to the ring.
     */
    if (totLen < NET_ETHER_MIN_BYTES) {
	totLen = NET_ETHER_MIN_BYTES;
    }
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_SIZE) = -totLen;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_ADDR_LOW) = 
			BUF_TO_CHIP_ADDR(xmitBufPtr) & 0xFFFF;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS1) =
	    ((BUF_TO_CHIP_ADDR(xmitBufPtr) >> 16) & NET_LE_XMIT_BUF_ADDR_HIGH) |
			NET_LE_XMIT_START_OF_PACKET | 
			NET_LE_XMIT_END_OF_PACKET |
			NET_LE_XMIT_CHIP_OWNED;

    /*
     * Give the chip a little kick.
     */
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *netLEStatePtr->regDataPortPtr =
		(NET_LE_CSR0_XMIT_DEMAND | NET_LE_CSR0_INTR_ENABLE);
    return (SUCCESS);

}


/*
 * Flag to note if xmit memory has been initialized and allocated.
 */

static	Boolean	xmitMemInitialized = FALSE;
static	Boolean	xmitMemAllocated = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * AllocateXmitMem --
 *
 *	Allocate kernel memory for transmission ring.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Device state structure is updated.
 *
 *----------------------------------------------------------------------
 */
static void
AllocateXmitMem()
{
    /*
     * Allocate a transmission buffer descriptor.  
     * The descriptor must start on an 8-byte boundary.  
     */
    netLEState.xmitDescFirstPtr = NetLEMemAlloc(NET_LE_XMIT_DESC_SIZE, FALSE);

    /*
     * Allocate a buffer for a transmitted packet.
     */
    xmitBufPtr = NetLEMemAlloc(NET_ETHER_MAX_BYTES, TRUE);

    xmitMemAllocated = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEXmitInit --
 *
 *	Initialize the transmission queue structures.  This includes setting
 *	up the transmission ring buffers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The transmission ring is initialized.
 *
 *----------------------------------------------------------------------
 */
void
NetLEXmitInit()
{
    Address	descPtr;


    if (!xmitMemAllocated) {
	AllocateXmitMem();
    }
    xmitMemInitialized = TRUE;

    /*
     * Initialize the state structure to point to the ring. xmitDescFirstPtr
     * is set by AllocateXmitMem() and never moved.
     */
    netLEState.xmitDescLastPtr = netLEState.xmitDescFirstPtr;
    netLEState.xmitDescNextPtr = netLEState.xmitDescFirstPtr;

    descPtr = netLEState.xmitDescFirstPtr;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_ADDR_LOW) = 0;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS1) = 0;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS2) = 0;

    netLEState.transmitting = FALSE;
    curScatGathPtr = (Net_ScatterGather *) NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEXmitDone --
 *
 *	This routine will process a completed transmit command.  It will
 *	check for errors and update the transmission ring pointers.
 *
 * Results:
 *	FAILURE if problem is found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetLEXmitDone()
{
    register	NetXmitElement	*xmitElementPtr;
    register	Address		descPtr;
    register	NetLEState	*netLEStatePtr;
    ReturnStatus		status;
    unsigned			status1;
    unsigned			status2;

    netLEStatePtr = &netLEState;

    descPtr = (Address)netLEStatePtr->xmitDescNextPtr;

    /*
     * Reset the interrupt.
     */
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *netLEStatePtr->regDataPortPtr = 
		(NET_LE_CSR0_XMIT_INTR | NET_LE_CSR0_INTR_ENABLE);

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (curScatGathPtr == (Net_ScatterGather *) NIL) {
	printf( "NetLEXmitDone: No current packet\n.");
	return (FAILURE);
    }

    status1 = *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS1);
    status2 = *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS2);
    if (status1 & NET_LE_XMIT_CHIP_OWNED) {
	printf("netLE: Bogus transmit interrupt. Buffer owned by chip.\n");
	return (FAILURE);
    }

    /*
     * Check for errors.
     */
    if (status1 & NET_LE_XMIT_ERROR) {
	net_EtherStats.xmitPacketsDropped++;
	if (status2 & NET_LE_XMIT_LOST_CARRIER) {
	    printf("LE ethernet: Lost carrier.\n");
	}
	/*
	 * Loss of carrier seems to also causes late collision.
	 * Print only one of the messages.
	 */
	if ((status2 & NET_LE_XMIT_LATE_COLLISION) && 
	    !(status2 & NET_LE_XMIT_LOST_CARRIER)) {
	    printf("LE ethernet: Transmit late collision.\n");
	}
	if (status2 & NET_LE_XMIT_RETRY_ERROR) {
	    net_EtherStats.xmitCollisionDrop++;
	    net_EtherStats.collisions += 16;
	    printf("LE ethernet: Too many collisions.\n");
	}
	if (status2 & NET_LE_XMIT_UNDER_FLOW_ERROR) {
	    printf("LE ethernet: Memory underflow error.\n");
	    return (FAILURE);
	}
    }
    if (status2 & NET_LE_XMIT_BUFFER_ERROR) {
	printf("LE ethernet: Transmit buffering error.\n");
	return (FAILURE);
    }
    if (status1 & NET_LE_XMIT_ONE_RETRY) {
	net_EtherStats.collisions++;
    }
    if (status1 & NET_LE_XMIT_RETRIES) {
	/*
	 * Two is more than one.  
	 */
	net_EtherStats.collisions += 2;	/* Only a guess. */
    }

    net_EtherStats.packetsSent++;

    /*
     * Mark the packet as done.
     */
    curScatGathPtr->done = TRUE;
    if (curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	NetOutputWakeup(curScatGathPtr->mutexPtr);
    }

    /*
     * If there are more packets to send then send the first one on
     * the queue.  Otherwise there is nothing being transmitted.
     */
    status = SUCCESS;
    if (!List_IsEmpty(netLEState.xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(netLEState.xmitList);
	status = OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(netLEState.xmitFreeList));
    } else {
	netLEState.transmitting = FALSE;
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return (status);
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEOutput --
 *
 *	Output a packet.  The procedure is to either put the packet onto the 
 *	queue of outgoing packets if packets are already being sent, or 
 *	otherwise to send the packet directly.  The elements of the scatter 
 *	array which come into this routine must satisfy the following two 
 *	properties:
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Queue of packets modified.
 *
 *----------------------------------------------------------------------
 */

void
NetLEOutput(etherHdrPtr, scatterGatherPtr, scatterGatherLength)
    Net_EtherHdr	*etherHdrPtr;	/* Ethernet header for the packet. */
    register	Net_ScatterGather	*scatterGatherPtr; /* Data portion of 
							    * the packet. */
    int			scatterGatherLength;	/* Length of data portion gather
						 * array. */
{
    register	NetXmitElement		*xmitPtr;
    ReturnStatus			status;

    DISABLE_INTR();

    net_EtherStats.packetsOutput++;

    /*
     * See if the packet is for us.  In this case just copy in the packet
     * and call the higher level routine.
     */

    if (NET_ETHER_COMPARE(netLEState.etherAddress, etherHdrPtr->destination)) {
	int i, length;

        length = sizeof(Net_EtherHdr);
        for (i = 0; i < scatterGatherLength; i++) {
            length += scatterGatherPtr[i].length;
        }

        if (length <= NET_ETHER_MAX_BYTES) {
	    register Address bufPtr;

	    etherHdrPtr->source = netLEState.etherAddress;

	    bufPtr = (Address)loopBackBuffer;
	    bcopy((Address)etherHdrPtr, bufPtr, sizeof(Net_EtherHdr));
	    bufPtr += sizeof(Net_EtherHdr);
            Net_GatherCopy(scatterGatherPtr, scatterGatherLength, bufPtr);

	    Net_Input((Address)loopBackBuffer, length);
        }

        scatterGatherPtr->done = TRUE;

	ENABLE_INTR();
	return;
    }

    /*
     * If no packet is being sent then go ahead and send this one.
     */

    if (!netLEState.transmitting) {
	status = 
	    OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength);
	if (status != SUCCESS) {
		NetLERestart();
	}
	ENABLE_INTR();
	return;
    }

    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */

    if (List_IsEmpty(netLEState.xmitFreeList)) {
        scatterGatherPtr->done = TRUE;
	ENABLE_INTR();
	return;
    }

    xmitPtr = 
	(NetXmitElement *)List_First((List_Links *) netLEState.xmitFreeList);

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
    List_Insert((List_Links *) xmitPtr, LIST_ATREAR(netLEState.xmitList)); 

    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEXmitRestart --
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
NetLEXmitRestart()
{
    NetXmitElement	*xmitElementPtr;
    ReturnStatus	status;

    /*
     * Drop the current outgoing packet.
     */    
    if (curScatGathPtr != (Net_ScatterGather *) NIL) {
	curScatGathPtr->done = TRUE;
	if (curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    NetOutputWakeup(curScatGathPtr->mutexPtr);
	}
    }

    /*
     * Start output if there are any packets queued up.
     */
    if (!List_IsEmpty(netLEState.xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(netLEState.xmitList);
	status = OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength);
	if (status != SUCCESS) {
	    panic("LE ethernet: Can not output first packet on restart.\n");
	}
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(netLEState.xmitFreeList));
    } else {
	netLEState.transmitting = FALSE;
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
}
