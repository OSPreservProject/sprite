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

#include <sprite.h>
#include <sys.h>
#include <netLEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <list.h>
#include <sync.h>
#include <machMon.h>

/*
 * Macros to step ring pointers.
 */

#define	NEXT_SEND(p)	( ((p+1) > statePtr->xmitDescLastPtr) ? \
				statePtr->xmitDescFirstPtr : \
				(p+1))
#define	PREV_SEND(p)	( ((p-1) < statePtr->xmitDescFirstPtr) ? \
				statePtr->xmitDescLastPtr : \
				(p-1))
static	ReturnStatus	OutputPacket _ARGS_((Net_EtherHdr *etherHdrPtr,
			    Net_ScatterGather *scatterGatherPtr,
			    int scatterGatherLength,
			    NetLEState *statePtr));
static	void		AllocateXmitMem _ARGS_((NetLEState *statePtr));

char	foo[NET_ETHER_MAX_BYTES];

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
OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength, statePtr)
    Net_EtherHdr		*etherHdrPtr;	/* Ethernet header of packet.*/
    register	Net_ScatterGather   	*scatterGatherPtr; /* Data portion of
							    * packet. */

    int			scatterGatherLength;	/* Length of data portion 
						 * gather array. */
    NetLEState		*statePtr;		/* The interface state. */
{
    register volatile NetLEXmitMsgDesc	*descPtr;
    register	char			*firstBufferPtr; 
    Address				bufPtr;
    int					bufCount;
    int					totalLength;
    int					length;
    int					amountNeeded;
#if defined(sun3) || defined(sun4)
    Net_ScatterGather			newScatGathArr[NET_LE_NUM_XMIT_BUFFERS];
    register Boolean			reMapped = FALSE;
#endif
    int					i;

    descPtr = statePtr->xmitDescNextPtr;

    /*
     * Do some sanity checks.
     */
    if (NetBfByteTest(descPtr->bits1, ChipOwned, 1)) {
	printf("LE ethernet: Transmit buffer owned by chip.\n");
	return (FAILURE);
    }

    statePtr->transmitting = TRUE;
    statePtr->curScatGathPtr = scatterGatherPtr;
    firstBufferPtr = statePtr->firstDataBuffer;

    /*
     * Add the first data buffer to the ring.
     */
    descPtr->bufAddrLow = NET_LE_TO_CHIP_ADDR_LOW(firstBufferPtr);
    descPtr->bufAddrHigh = NET_LE_TO_CHIP_ADDR_HIGH(firstBufferPtr);
    NetBfByteSet(descPtr->bits1, StartOfPacket, 1);
    NetBfByteSet(descPtr->bits1, EndOfPacket, 0);

    /*
     * Since the first part of a packet is always the ethernet header that
     * is less than NET_LE_MIN_FIRST_BUFFER_SIZE we must use the 
     * firstDataBuffer to build the first buffer of mimumum allowable size.
     */


    descPtr->bufferSize = -NET_LE_MIN_FIRST_BUFFER_SIZE; /* May be wrong 
							  * for small 
							  * packets. 
							  */

    /* 
     * First copy in the header making sure the source address field is set
     * correctly. (Such a fancy chip and it won't even set the source 
     * address
     * of the header for us.)
     */

    (* ((Net_EtherHdr *) firstBufferPtr)) = *etherHdrPtr;

    ((Net_EtherHdr *) firstBufferPtr)->source = statePtr->etherAddress;

    firstBufferPtr += sizeof(Net_EtherHdr);

    if (NET_LE_COPY_PACKET) {
	totalLength = sizeof(Net_EtherHdr);
	for (i = 0; i < scatterGatherLength; i++) {
	    totalLength += scatterGatherPtr[i].length;
	} 
	if (totalLength <= NET_ETHER_MAX_BYTES) {
	    Net_GatherCopy(scatterGatherPtr, scatterGatherLength, 
		firstBufferPtr);
	    descPtr->bufferSize = -totalLength;
	} else {
	    panic("OutputPacket: packet too large (%d)\n", totalLength);
	}
    } else {

    
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
	     * Compute the amount of data needed in the first buffer to make it
	     * a minumum size.
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
#if defined(sun3) || defined(sun4)
	    if (!reMapped) { 
		/*
		 * Remap the packet into network addressible memory.
		 */
		VmMach_NetMapPacket(scatterGatherPtr, 
		    scatterGatherLength-bufCount, newScatGathArr);
		bufPtr = newScatGathArr->bufAddr + 
			    (bufPtr - scatterGatherPtr->bufAddr);
		scatterGatherPtr = newScatGathArr;
		reMapped = TRUE;
	    }
#endif
	    /*
	     * Add bufPtr of length length to the next buffer of the chain.
	     */
	    descPtr = NEXT_SEND(descPtr);
	    if (NetBfByteTest(descPtr->bits1, ChipOwned, 1)) {
		/*
		 * Along as we only transmit one packet at a time, a buffer
		 * own by the chip is a serious problem.
		 */
		printf("LE ethernet: Transmit buffer owned by chip.\n");
		return (FAILURE);
	    }
	    descPtr->bufAddrLow = NET_LE_TO_CHIP_ADDR_LOW(bufPtr);
	    descPtr->bufAddrHigh = NET_LE_TO_CHIP_ADDR_HIGH(bufPtr);
	    NetBfByteSet(descPtr->bits1, StartOfPacket, 0);
	    NetBfByteSet(descPtr->bits1, EndOfPacket, 0);
	    descPtr->bufferSize = -length;

	    totalLength += length;
	}
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
    if (rpc_SanityCheck && (etherHdrPtr->type == NET_ETHER_SPRITE)) {
	ReturnStatus 	status;
	status = Rpc_SanityCheck(scatterGatherLength, 
		    statePtr->curScatGathPtr, totalLength);
	if (status != SUCCESS) {
	    panic("Sanity check failed on outgoing packet.\n");
	}
    }

    /*
     * Finish off the packet.
     */

    NetBfByteSet(descPtr->bits1, EndOfPacket, 1);

    /*
     * Change the ownership to the chip. Avoid race conditions by doing it
     * with the last buffer first.
     */

    while (TRUE) {
	NetBfByteSet(descPtr->bits1, ChipOwned, 1);
	if (descPtr == statePtr->xmitDescNextPtr) {
	    break;
	}
	descPtr = PREV_SEND(descPtr);
    }

    /*
     * Give the chip a little kick.
     */
    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR0_ADDR);
    Mach_EmptyWriteBuffer();
    statePtr->regPortPtr->dataPort = 
	    (NET_LE_CSR0_XMIT_DEMAND | NET_LE_CSR0_INTR_ENABLE);
    return (SUCCESS);

}


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
AllocateXmitMem(statePtr)
    NetLEState		*statePtr; 	/* State of the interface. */
{
    unsigned int	memBase;	

    /*
     * Allocate the ring of transmission buffer descriptors.  
     * The ring must start on 8-byte boundary.  
     */
    memBase = (unsigned int) BufAlloc(statePtr, 
	(NET_LE_NUM_XMIT_BUFFERS * sizeof(NetLEXmitMsgDesc)) + 8);
    /*
     * Insure ring starts on 8-byte boundary.
     */
    if (memBase & 0x7) {
	memBase = (memBase + 8) & ~0x7;
    }
    statePtr->xmitDescFirstPtr = (NetLEXmitMsgDesc *) memBase;

    /*
     * Allocate the first buffer for a packet.
     */
    statePtr->firstDataBuffer = BufAlloc(statePtr, ((NET_LE_COPY_PACKET) ? 
	NET_ETHER_MAX_BYTES : NET_LE_MIN_FIRST_BUFFER_SIZE));
    statePtr->xmitMemAllocated = TRUE;
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
NetLEXmitInit(statePtr)
    NetLEState		*statePtr; 	/* State of the interface. */
{
    int 		bufNum;
    volatile NetLEXmitMsgDesc	*descPtr;


    if (!statePtr->xmitMemAllocated) {
	AllocateXmitMem(statePtr);
    }
    statePtr->xmitMemInitialized = TRUE;

    /*
     * Initialize the state structure to point to the ring. xmitDescFirstPtr
     * is set by AllocateXmitMem() and never moved.
     */
    statePtr->xmitDescLastPtr = 
		&(statePtr->xmitDescFirstPtr[NET_LE_NUM_XMIT_BUFFERS-1]);
    statePtr->xmitDescNextPtr = statePtr->xmitDescFirstPtr;

    descPtr = statePtr->xmitDescFirstPtr;
    for (bufNum = 0; bufNum < NET_LE_NUM_XMIT_BUFFERS; bufNum++, descPtr++) { 
	bzero((char *) descPtr, sizeof(NetLEXmitMsgDesc));
    }
    statePtr->transmitting = FALSE;
    statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
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
NetLEXmitDone(statePtr)
    NetLEState		*statePtr; 	/* State of the interface. */
{
    register	volatile NetXmitElement     	*xmitElementPtr;
    register	volatile NetLEXmitMsgDesc	*descPtr;
    ReturnStatus			status;
    char				*buffer; 

    descPtr = statePtr->xmitDescNextPtr;

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (statePtr->curScatGathPtr == (Net_ScatterGather *) NIL) {
	printf( "NetLEXmitDone: No current packet\n.");
	status = FAILURE;
	goto exit;
    }

    if (NetBfByteTest(descPtr->bits1, ChipOwned, 1)) {
	printf("LE ethernet: Bogus xmit interrupt. Buffer owned by chip.\n");
	status = FAILURE;
	goto exit;
    }
    if (NetBfByteTest(descPtr->bits1, StartOfPacket, 0)) {
	printf("LE ethernet: Bogus xmit interrupt. Buffer not start of packet.\n");
	status = FAILURE;
	goto exit;
    }

    /*
     * Check for errors.
     */
    while (TRUE) {

	if (NetBfByteTest(descPtr->bits1, Error, 1)) {
	    statePtr->stats.xmitPacketsDropped++;
	    if (NetBfShortTest(descPtr->bits2, LateCollision, 1)) {
		printf("LE ethernet: Transmit late collision.\n");
	    }
	    if (NetBfShortTest(descPtr->bits2, LostCarrier, 1)) {
		printf("LE ethernet: Lost carrier.\n");
	    }
	    /*
	     * Lost of carrier seems to also causes late collision.
	     * Print only one of the messages.
	     */
	    if ((NetBfShortTest(descPtr->bits2, LateCollision, 1)) &&
		(NetBfShortTest(descPtr->bits2, LostCarrier, 0))) {
		printf("LE ethernet: Transmit late collision.\n");
	    }
	    if (NetBfShortTest(descPtr->bits2, RetryError, 1)) {
		statePtr->stats.xmitCollisionDrop++;
		statePtr->stats.collisions += 16;
		printf("LE ethernet: Too many collisions.\n");
	    }
	    if (NetBfShortTest(descPtr->bits2, UnderflowError, 1)) {
		printf("LE ethernet: Memory underflow error.\n");
		status = FAILURE;
		goto exit;
	    }
	}
	if (NetBfShortTest(descPtr->bits2, XmitBufferError, 1)) {
	    printf("LE ethernet: Transmit buffering error.\n");
	    status = FAILURE;
	    goto exit;
	}
	if (NetBfByteTest(descPtr->bits1, OneRetry, 1)) {
	    statePtr->stats.collisions++;
	}
	if (NetBfByteTest(descPtr->bits1, Retries, 1)) {
	    /*
	     * Two is more than one.  
	     */
	    statePtr->stats.collisions += 2;	/* Only a guess. */
	}

	buffer = (char *) NET_LE_FROM_CHIP_ADDR(statePtr, 
	    descPtr->bufAddrHigh, descPtr->bufAddrLow);

	if (NetBfByteTest(descPtr->bits1, EndOfPacket, 1)) {
		break;
	}

	descPtr = NEXT_SEND(descPtr);
	if (descPtr == statePtr->xmitDescNextPtr) {
	    panic("LE ethernet: Transmit ring with no end of packet.\n");
	}
	if (NetBfByteTest(descPtr->bits1, ChipOwned, 1)) {
		printf("LE ethernet: Transmit Buffer owned by chip.\n");
		status = FAILURE;
		goto exit;
	}
    }

    statePtr->stats.packetsSent++;

    /*
     * Update the ring pointer to point at the next buffer to use.
     */

    statePtr->xmitDescNextPtr = NEXT_SEND(descPtr);

    /*
     * Mark the packet as done.
     */
    statePtr->curScatGathPtr->done = TRUE;
    if (statePtr->curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	NetOutputWakeup(statePtr->curScatGathPtr->mutexPtr);
    }

    /*
     * If there are more packets to send then send the first one on
     * the queue.  Otherwise there is nothing being transmitted.
     */
    status = SUCCESS;
    if (statePtr->resetPending == TRUE) {
	goto exit;
    }
    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(statePtr->xmitList);
	status = OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength, statePtr);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
exit:
    if (statePtr->resetPending == TRUE) {
	statePtr->transmitting = FALSE;
	NetLEReset(statePtr->interPtr);
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
 *	SUCCESS if the packet was queued to the chip correctly, otherwise
 *	a standard Sprite error code.
 *
 * Side effects:
 *	Queue of packets modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetLEOutput(interPtr, hdrPtr, scatterGatherPtr, scatterGatherLength, rpc,
	    statusPtr)
    Net_Interface	*interPtr;	/* The network interface. */
    Address		hdrPtr;		/* Packet header. */
    register	Net_ScatterGather	*scatterGatherPtr; /* Data portion of 
							    * the packet. */
    int			scatterGatherLength;	/* Length of data portion gather
						 * array. */
    Boolean		rpc;			/* Is this an RPC packet? */
    ReturnStatus	*statusPtr;		/* Status from sending packet.*/
{
    register	NetXmitElement		*xmitPtr;
    ReturnStatus			status;
    NetLEState				*statePtr;
    Net_EtherHdr			*etherHdrPtr = (Net_EtherHdr *) hdrPtr;

    statePtr = (NetLEState *) interPtr->interfaceData;
    MASTER_LOCK(&interPtr->mutex);

    statePtr->stats.packetsOutput++;

    /*
     * Verify that the scatter gather array is not too large.  There is a fixed
     * upper bound because the list of transmit buffers is preallocated.
     */

    if (scatterGatherLength >= NET_LE_NUM_XMIT_BUFFERS) {
	scatterGatherPtr->done = TRUE;

	printf("LE ethernet: Packet in too many pieces\n");
	status = FAILURE;
	goto exit;
    }


    /*
     * See if the packet is for us.  In this case just copy in the packet
     * and call the higher level routine.
     */

    if (NET_ETHER_COMPARE(statePtr->etherAddress, etherHdrPtr->destination)) {
	int i, length;

        length = sizeof(Net_EtherHdr);
        for (i = 0; i < scatterGatherLength; i++) {
            length += scatterGatherPtr[i].length;
        }

        if (length <= NET_ETHER_MAX_BYTES) {
	    register Address bufPtr;

	    etherHdrPtr->source = statePtr->etherAddress;

	    bufPtr = (Address)statePtr->loopBackBuffer;
	    bcopy((Address)etherHdrPtr, bufPtr, sizeof(Net_EtherHdr));
	    bufPtr += sizeof(Net_EtherHdr);
            Net_GatherCopy(scatterGatherPtr, scatterGatherLength, bufPtr);

	    Net_Input(interPtr, (Address)statePtr->loopBackBuffer, length);
        }

        scatterGatherPtr->done = TRUE;

	status = SUCCESS;
	if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	}
	goto exit;
    }

    /*
     * If no packet is being sent then go ahead and send this one.
     */

    if (!statePtr->transmitting) {
	status = 
	    OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength,
		    statePtr);
	if (status != SUCCESS) {
		NetLERestart(interPtr);
	} else if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	}
	goto exit;
    }
    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */

    if (List_IsEmpty(statePtr->xmitFreeList)) {
        scatterGatherPtr->done = TRUE;
	status = FAILURE;
	goto exit;
    }

    xmitPtr = (NetXmitElement *) List_First((List_Links *) statePtr->xmitFreeList);

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

    List_Insert((List_Links *) xmitPtr, LIST_ATREAR(statePtr->xmitList)); 

    if (statusPtr != (ReturnStatus *) NIL) {
	*statusPtr = SUCCESS;
    }
    status = SUCCESS;
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return SUCCESS;
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
NetLEXmitDrop(statePtr)
    NetLEState		*statePtr; 	/* State of the interface. */
{
    if (statePtr->curScatGathPtr != (Net_ScatterGather *) NIL) {
	statePtr->curScatGathPtr->done = TRUE;
	if (statePtr->curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    NetOutputWakeup(statePtr->curScatGathPtr->mutexPtr);
	}
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
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
NetLEXmitRestart(statePtr)
    NetLEState		*statePtr; 	/* State of the interface. */
{
    NetXmitElement	*xmitElementPtr;
    ReturnStatus	status;

    /*
     * Start output if there are any packets queued up.
     */
    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(statePtr->xmitList);
	status = OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength, statePtr);
	if (status != SUCCESS) {
	    panic("LE ethernet: Can not output first packet on restart.\n");
	}
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return;
}
