/* 
 * netLEXmit.c --
 *
 *	Routines to transmit packets on the LANCE interface.
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

#include "sprite.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "vm.h"
#include "vmMach.h"
#include "list.h"

#include "sync.h"

/*
 * Macros to step ring pointers.
 */

#define	NEXT_SEND(p)	( ((++p) > netLEState.xmitDescLastPtr) ? \
				netLEState.xmitDescFirstPtr : \
				(p))
#define	PREV_SEND(p)	( ((--p) < netLEState.xmitDescFirstPtr) ? \
				netLEState.xmitDescLastPtr : \
				(p))

/*
 * Pointer to scatter gather element for current packet being sent.
 */
static Net_ScatterGather *curScatGathPtr = (Net_ScatterGather *) NIL;

/*
 * A buffer that is used when handling loop back packets.
 */
static  char            loopBackBuffer[NET_ETHER_MAX_BYTES];

/*
 * A buffer that is used to insure that the first element of the transmit
 * buffer chain is of a minumum size.
 */

static char		*firstDataBuffer;


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
    register	Net_ScatterGather   	*scatterGatherPtr; /* Data portion of
							    * packet. */

    int			scatterGatherLength;	/* Length of data portion 
						 * gather array. */
{
    register volatile NetLEXmitMsgDesc	*descPtr;
    volatile NetLEState			*netLEStatePtr;
    register	char			*firstBufferPtr; 
    Address				bufPtr;
    int					bufCount;
    int					totalLength;
    int					length;
    int					amountNeeded;
#if defined(sun3) || defined(sun4)
    Net_ScatterGather			newScatGathArr[NET_LE_NUM_XMIT_BUFFERS];
#endif

    netLEStatePtr = &netLEState;

    descPtr = netLEStatePtr->xmitDescNextPtr;

    /*
     * Do some sanity checks.
     */
    if (descPtr->chipOwned) {
	printf("LE ethernet: Transmit buffer owned by chip.\n");
	return (FAILURE);
    }


    netLEState.transmitting = TRUE;
    curScatGathPtr = scatterGatherPtr;
#if defined(sun3) || defined(sun4)
    /*
     * Remap the packet into network addressible memory.
     */
    VmMach_NetMapPacket(scatterGatherPtr, scatterGatherLength, newScatGathArr);
    scatterGatherPtr = newScatGathArr;
#endif

    /*
     * Since the first part of a packet is always the ethernet header that
     * is less than NET_LE_MIN_FIRST_BUFFER_SIZE we must use the 
     * firstDataBuffer to build the first buffer of mimumum allowable size.
     */

    firstBufferPtr = firstDataBuffer;

    /*
     * Add the buffer to the ring.
     */
    descPtr->bufAddrLow = NET_LE_SUN_TO_CHIP_ADDR_LOW(firstDataBuffer);
    descPtr->bufAddrHigh = NET_LE_SUN_TO_CHIP_ADDR_HIGH(firstDataBuffer);
    descPtr->startOfPacket = 1;
    descPtr->endOfPacket = 0;
    descPtr->bufferSize = -NET_LE_MIN_FIRST_BUFFER_SIZE; /* May be wrong for 
							  * small packets. 
							  */

    /* 
     * First copy in the header making sure the source address field is set
     * correctly. (Such a fancy chip and it won't even set the source address
     * of the header for us.)
     */

    (* ((Net_EtherHdr *) firstBufferPtr)) = *etherHdrPtr;

    ((Net_EtherHdr *) firstBufferPtr)->source = netLEStatePtr->etherAddress;

    firstBufferPtr += sizeof(Net_EtherHdr);

    /*
     * Then copy enough data to bring buffer up to size.
     */

    totalLength = sizeof(Net_EtherHdr);

    bufCount = 0;
    for (bufCount = 0; bufCount < scatterGatherLength; 
			bufCount++,scatterGatherPtr++ ) {

	/*
	 * If is an empty buffer then skip it.
	 */

	length = scatterGatherPtr->length;
	if (length == 0) {
	    continue;
	}

	bufPtr = scatterGatherPtr->bufAddr;

	/*
	 * Compute the amount of data needed in the first buffer to make it a
	 * minumum size.
	 */

	amountNeeded = NET_LE_MIN_FIRST_BUFFER_SIZE - totalLength;
	if (amountNeeded > 0 ) {
	    /*
	     * Still need more padding in first buffer.
	     */
	    if (length <= amountNeeded) {
		 /*
		  * Needs this entire length.
		  */
		 bcopy(bufPtr, firstBufferPtr, length);
		 totalLength += length;
		 firstBufferPtr += length;
		 /*
		  * Get the next segment of the scatter.
		  */
		 continue;
	    } else {
		/*
		 * Needs only part of this buffer.
		 */
		 bcopy(bufPtr, firstBufferPtr, amountNeeded);
		 totalLength += amountNeeded;
		 /*
		  * Update the length and address for insertion into the
		  * buffer ring. firstBufferPtr is not used anymore in this
		  * loop.
		  */
		 length -= amountNeeded;
		 bufPtr += amountNeeded;
		 if (length == 0) {
			continue;
		 }

	    }
	 }
	/*
	 * Add bufPtr of length length to the next buffer of the chain.
	 */
	descPtr = NEXT_SEND(descPtr);
	if (descPtr->chipOwned) {
	    /*
	     * Along as we only transmit one packet at a time, a buffer
	     * own by the chip is a serious problem.
	     */
	    printf("LE ethernet: Transmit buffer owned by chip.\n");
	    return (FAILURE);
	}
	descPtr->bufAddrLow = NET_LE_SUN_TO_CHIP_ADDR_LOW(bufPtr);
	descPtr->bufAddrHigh = NET_LE_SUN_TO_CHIP_ADDR_HIGH(bufPtr);
	descPtr->startOfPacket = 0;
	descPtr->endOfPacket = 0;
	descPtr->bufferSize = -length;

	totalLength += length;
    }

    /*
     * See if we need to update the size of the first buffer.
     */

    if (totalLength < NET_LE_MIN_FIRST_BUFFER_SIZE) {
	/*
	 * Since totalLength < NET_LE_MIN_FIRST_BUFFER_SIZE, descPtr should
	 * still point at the buffer desciptor for the first block.
	 * Just update its size. If the size is less than the minimum possible
	 * length, increase the length and send the garbage in the buffer.
	 */
	descPtr->bufferSize = -totalLength;
	if (totalLength < NET_ETHER_MIN_BYTES) {
		descPtr->bufferSize = -NET_ETHER_MIN_BYTES;
	} 
    }

    /*
     * Finish off the packet.
     */

    descPtr->endOfPacket = 1;

    /*
     * Change the ownership to the chip. Avoid race conditions by doing it
     * with the last buffer first.
     */

    while (TRUE) {
	 descPtr->chipOwned = 1;
	 if (descPtr == netLEStatePtr->xmitDescNextPtr) {
	     break;
	 }
	 descPtr = PREV_SEND(descPtr);
    }

    /*
     * Give the chip a little kick.
     */

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    netLEStatePtr->regPortPtr->dataPort =
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
    unsigned int	memBase;		

    /*
     * Allocate the ring of transmission buffer descriptors.  
     * The ring must start on 8-byte boundary.  
     */
    memBase = (unsigned int) VmMach_NetMemAlloc(
		(NET_LE_NUM_XMIT_BUFFERS * sizeof(NetLEXmitMsgDesc)) + 8);
    /*
     * Insure ring starts on 8-byte boundary.
     */
    if (memBase & 0x7) {
	memBase = (memBase + 8) & ~0x7;
    }
    netLEState.xmitDescFirstPtr = (NetLEXmitMsgDesc *) memBase;

    /*
     * Allocate the first buffer for a packet.
     */
    firstDataBuffer = VmMach_NetMemAlloc(NET_LE_MIN_FIRST_BUFFER_SIZE);

    xmitMemAllocated = TRUE;
    return;
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
    int 		bufNum;
    volatile NetLEXmitMsgDesc	*descPtr;


    if (!xmitMemAllocated) {
	AllocateXmitMem();
    }
    xmitMemInitialized = TRUE;

    /*
     * Initialize the state structure to point to the ring. xmitDescFirstPtr
     * is set by AllocateXmitMem() and never moved.
     */
    netLEState.xmitDescLastPtr = 
		&(netLEState.xmitDescFirstPtr[NET_LE_NUM_XMIT_BUFFERS-1]);
    netLEState.xmitDescNextPtr = netLEState.xmitDescFirstPtr;

    descPtr = netLEState.xmitDescFirstPtr;
    for (bufNum = 0; bufNum < NET_LE_NUM_XMIT_BUFFERS; bufNum++, descPtr++) { 
	/*
	 * Clear out error fields. 
	 */
	descPtr->error = 0;
	descPtr->retries = 0;
	descPtr->oneRetry = 0;
	descPtr->deferred = 0;
	descPtr->bufferError = 0;
	descPtr->underflowError = 0;
	descPtr->retryError = 0;
	descPtr->lostCarrier = 0;
	descPtr->lateCollision = 0;
	/*
	 * Clear packet boundry bits.
	 */
	descPtr->startOfPacket = descPtr->endOfPacket = 0;
	/*
	 * Set ownership to the os.
	 */
	descPtr->chipOwned = 0;
    }

    netLEState.transmitting = FALSE;
    curScatGathPtr = (Net_ScatterGather *) NIL;
    return;
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
    register	volatile NetXmitElement     	*xmitElementPtr;
    register	volatile NetLEXmitMsgDesc	*descPtr;
    register	volatile NetLEState		*netLEStatePtr;
    ReturnStatus			status;

    netLEStatePtr = &netLEState;

    descPtr = netLEStatePtr->xmitDescNextPtr;

    /*
     * Reset the interrupt.
     */
    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    netLEStatePtr->regPortPtr->dataPort = 
		(NET_LE_CSR0_XMIT_INTR | NET_LE_CSR0_INTR_ENABLE);

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (curScatGathPtr == (Net_ScatterGather *) NIL) {
	printf( "NetLEXmitDone: No current packet\n.");
	return (FAILURE);
    }

    if (descPtr->chipOwned) {
	printf("LE ethernet: Bogus xmit interrupt. Buffer owned by chip.\n");
	return (FAILURE);
    }
    if (!descPtr->startOfPacket) {
	printf("LE ethernet: Bogus xmit interrupt. Buffer not start of packet.\n");
	return (FAILURE);
    }


    /*
     * Check for errors.
     */
    while (TRUE) {

	if (descPtr->error) {
	    net_EtherStats.xmitPacketsDropped++;
	    if (descPtr->lateCollision) {
		printf("LE ethernet: Transmit late collision.\n");
	    }
	    if (descPtr->lostCarrier) {
		printf("LE ethernet: Lost carrier.\n");
	    }
	    /*
	     * Lost of carrier seems to also causes late collision.
	     * Print only one of the messages.
	     */
	    if (descPtr->lateCollision && !descPtr->lostCarrier) {
		printf("LE ethernet: Transmit late collision.\n");
	    }
	    if (descPtr->retryError) {
		net_EtherStats.xmitCollisionDrop++;
		net_EtherStats.collisions += 16;
		printf("LE ethernet: Too many collisions.\n");
	    }
	    if (descPtr->underflowError) {
		printf("LE ethernet: Memory underflow error.\n");
		return (FAILURE);
	    }
	}
	if (descPtr->bufferError) {
	    printf("LE ethernet: Transmit buffering error.\n");
	    return (FAILURE);
	}
	if (descPtr->oneRetry) {
	    net_EtherStats.collisions++;
	}
	if (descPtr->retries) {
	    /*
	     * Two is more than one.  
	     */
	    net_EtherStats.collisions += 2;	/* Only a guess. */
	}

	if (descPtr->endOfPacket) {
		break;
	}

	descPtr = NEXT_SEND(descPtr);
	if (descPtr == netLEStatePtr->xmitDescNextPtr) {
	    panic("LE ethernet: Transmit ring with no end of packet.\n");
	}
	if (descPtr->chipOwned) {
		printf("LE ethernet: Transmit Buffer owned by chip.\n");
		return (FAILURE);
	}
    }

    net_EtherStats.packetsSent++;

    /*
     * Update the ring pointer to point at the next buffer to use.
     */

    netLEStatePtr->xmitDescNextPtr = NEXT_SEND(descPtr);

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
     * Verify that the scatter gather array is not too large.  There is a fixed
     * upper bound because the list of transmit buffers is preallocated.
     */

    if (scatterGatherLength >= NET_LE_NUM_XMIT_BUFFERS) {
	scatterGatherPtr->done = TRUE;

	printf("LE ethernet: Packet in too many pieces\n");
	ENABLE_INTR();
	return;
    }


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

    xmitPtr = (NetXmitElement *) List_First((List_Links *) netLEState.xmitFreeList);

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
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEXmitDrop --
 *
 *	Drop the current packet.  Called at the beginning of the
 *	restart sequence, before curScatGathPtr is reset to NIL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Current scatter gather pointer is reset and processes waiting
 *	for synchronous output are notified.
 *
 *----------------------------------------------------------------------
 */
void
NetLEXmitDrop()
{
    if (curScatGathPtr != (Net_ScatterGather *) NIL) {
	curScatGathPtr->done = TRUE;
	if (curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    NetOutputWakeup(curScatGathPtr->mutexPtr);
	}
	curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetLEXmitRestart --
 *
 *	Restart transmission of packets at the end of the restart
 *	sequence, after a chip reset.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output queue started up.
 *
 *----------------------------------------------------------------------
 */
void
NetLEXmitRestart()
{
    NetXmitElement	*xmitElementPtr;
    ReturnStatus	status;

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
    return;
}
