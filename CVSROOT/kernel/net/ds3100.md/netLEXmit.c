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

#include <sprite.h>
#include <netLEInt.h>
#include <sys.h>
#include <vm.h>
#include <list.h>
#include <sync.h>
#include <machMon.h>


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
    register Net_ScatterGather	*scatterGatherPtr; /* Data portion of packet.*/
    int				scatterGatherLength;/* Length of data portion 
						     * gather array. */
    NetLEState		*statePtr;		/* The interface state. */
{
    register short		*inBufPtr;
    register volatile short	*outBufPtr;
    register Address		descPtr;
    register int		length;
    unsigned	char		*leftOverBytePtr = (unsigned char *)NIL;
    int				totLen;
    int				i;

    descPtr = statePtr->xmitDescFirstPtr;

    /*
     * Do a sanity check.
     */
    if (*BUF_TO_ADDR(descPtr, NET_LE_XMIT_STATUS1) & NET_LE_XMIT_CHIP_OWNED) {
	printf("LE ethernet: Transmit buffer owned by chip.\n");
	return (FAILURE);
    }

    statePtr->transmitting = TRUE;
    statePtr->curScatGathPtr = scatterGatherPtr;
    etherHdrPtr->source = statePtr->etherAddress;
    outBufPtr = (volatile short *)statePtr->xmitBufPtr;

    /* 
     * Copy the packet into the xmit buffer.  Don't be general, be fast.
     * First do the ethernet header.
     */
    inBufPtr = (short *)etherHdrPtr;
    *outBufPtr = *inBufPtr;
    *(outBufPtr + 2) = *(inBufPtr + 1);
    *(outBufPtr + 4) = *(inBufPtr + 2);
    *(outBufPtr + 6) = *(inBufPtr + 3);
    *(outBufPtr + 8) = *(inBufPtr + 4);
    *(outBufPtr + 10) = *(inBufPtr + 5);
    *(outBufPtr + 12) = *(inBufPtr + 6);
    outBufPtr += 14;
    totLen = sizeof(Net_EtherHdr);

    /*
     * Now do each element of the scatter/gather array.
     */
    for (i = 0; i < scatterGatherLength; i++,scatterGatherPtr++ ) {
	unsigned char *bufAddr;

	length = scatterGatherPtr->length;
	if (length == 0) {
	    continue;
	}
	totLen += length;
	if (totLen > NET_ETHER_MAX_BYTES) {
	    printf("OutputPacket: Packet too large\n");
	    statePtr->curScatGathPtr = (Net_ScatterGather *)NIL;
	    statePtr->transmitting = FALSE;
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

#define COPY_OUT(n) \
	*(outBufPtr + n) = *(bufAddr + n) | (*(bufAddr + n + 1) << 8)

#define COPY_OUT2(n) \
	*(outBufPtr + n) = *(short *)(bufAddr + n)

	if ((unsigned)bufAddr & 1) {
	    while (length > 64) {
		COPY_OUT(0);  COPY_OUT(2);  COPY_OUT(4);  COPY_OUT(6);
		COPY_OUT(8);  COPY_OUT(10); COPY_OUT(12); COPY_OUT(14);
		COPY_OUT(16); COPY_OUT(18); COPY_OUT(20); COPY_OUT(22);
		COPY_OUT(24); COPY_OUT(26); COPY_OUT(28); COPY_OUT(30);
		COPY_OUT(32); COPY_OUT(34); COPY_OUT(36); COPY_OUT(38);
		COPY_OUT(40); COPY_OUT(42); COPY_OUT(44); COPY_OUT(46);
		COPY_OUT(48); COPY_OUT(50); COPY_OUT(52); COPY_OUT(54);
		COPY_OUT(56); COPY_OUT(58); COPY_OUT(60); COPY_OUT(62);
		outBufPtr += 64;
		bufAddr += 64;
		length -= 64;
	    }

	    while (length > 16) {
		COPY_OUT(0);  COPY_OUT(2);  COPY_OUT(4);  COPY_OUT(6);
		COPY_OUT(8);  COPY_OUT(10); COPY_OUT(12); COPY_OUT(14);
		outBufPtr += 16;
		bufAddr += 16;
		length -= 16;
	    }
	    while (length > 1) {
		*outBufPtr = *bufAddr | (*(bufAddr + 1) << 8);
		outBufPtr += 2;
		bufAddr += 2;
		length -= 2;
	    }
	} else {
	    while (length >= 64) {
		COPY_OUT2(0);  COPY_OUT2(2);  COPY_OUT2(4);  COPY_OUT2(6);
		COPY_OUT2(8);  COPY_OUT2(10); COPY_OUT2(12); COPY_OUT2(14);
		COPY_OUT2(16); COPY_OUT2(18); COPY_OUT2(20); COPY_OUT2(22);
		COPY_OUT2(24); COPY_OUT2(26); COPY_OUT2(28); COPY_OUT2(30);
		COPY_OUT2(32); COPY_OUT2(34); COPY_OUT2(36); COPY_OUT2(38);
		COPY_OUT2(40); COPY_OUT2(42); COPY_OUT2(44); COPY_OUT2(46);
		COPY_OUT2(48); COPY_OUT2(50); COPY_OUT2(52); COPY_OUT2(54);
		COPY_OUT2(56); COPY_OUT2(58); COPY_OUT2(60); COPY_OUT2(62);
		outBufPtr += 64;
		bufAddr += 64;
		length -= 64;
	    }

	    while (length >= 16) {
		COPY_OUT2(0);  COPY_OUT2(2);  COPY_OUT2(4);  COPY_OUT2(6);
		COPY_OUT2(8);  COPY_OUT2(10); COPY_OUT2(12); COPY_OUT2(14);
		outBufPtr += 16;
		bufAddr += 16;
		length -= 16;
	    }
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
    if ((rpc_SanityCheck) && (etherHdrPtr->type == NET_ETHER_SPRITE)) {
	ReturnStatus	status;
	status = Rpc_SanityCheck(scatterGatherLength, scatterGatherPtr, totLen);
	if (status != SUCCESS) {
	    netLEDebugState = *statePtr;
	    NetLEReset(statePtr->interPtr);
	    panic("Sanity check failed.\n");
	}
    }
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_SIZE) = -totLen;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_ADDR_LOW) = 
			BUF_TO_CHIP_ADDR(statePtr->xmitBufPtr) & 0xFFFF;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS1) =
	    ((BUF_TO_CHIP_ADDR(statePtr->xmitBufPtr) >> 16) & 
			NET_LE_XMIT_BUF_ADDR_HIGH) |
			NET_LE_XMIT_START_OF_PACKET | 
			NET_LE_XMIT_END_OF_PACKET |
			NET_LE_XMIT_CHIP_OWNED;

    /*
     * Give the chip a little kick.
     */
    *statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *statePtr->regDataPortPtr =
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
    /*
     * Allocate a transmission buffer descriptor.  
     * The descriptor must start on an 8-byte boundary.  
     */
    statePtr->xmitDescFirstPtr = NetLEMemAlloc(NET_LE_XMIT_DESC_SIZE, FALSE);

    /*
     * Allocate a buffer for a transmitted packet.
     */
    statePtr->xmitBufPtr = NetLEMemAlloc(NET_ETHER_MAX_BYTES, TRUE);

    statePtr->xmitMemAllocated = TRUE;
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
    Address	descPtr;

    if (!statePtr->xmitMemAllocated) {
	AllocateXmitMem(statePtr);
    }
    statePtr->xmitMemInitialized = TRUE;

    /*
     * Initialize the state structure to point to the ring. xmitDescFirstPtr
     * is set by AllocateXmitMem() and never moved.
     */
    statePtr->xmitDescLastPtr = statePtr->xmitDescFirstPtr;
    statePtr->xmitDescNextPtr = statePtr->xmitDescFirstPtr;

    descPtr = statePtr->xmitDescFirstPtr;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_BUF_ADDR_LOW) = 0;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS1) = 0;
    *BUF_TO_ADDR(descPtr,NET_LE_XMIT_STATUS2) = 0;

    statePtr->transmitting = FALSE;
    statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
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
    register	NetXmitElement	*xmitElementPtr;
    register	Address		descPtr;
    ReturnStatus		status;
    unsigned			status1;
    unsigned			status2;

    descPtr = (Address)statePtr->xmitDescNextPtr;

    /*
     * Reset the interrupt.
     */
    *statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *statePtr->regDataPortPtr = 
		(NET_LE_CSR0_XMIT_INTR | NET_LE_CSR0_INTR_ENABLE);

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (statePtr->curScatGathPtr == (Net_ScatterGather *) NIL) {
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
	statePtr->stats.xmitPacketsDropped++;
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
	    statePtr->stats.xmitCollisionDrop++;
	    statePtr->stats.collisions += 16;
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
	statePtr->stats.collisions++;
    }
    if (status1 & NET_LE_XMIT_RETRIES) {
	/*
	 * Two is more than one.  
	 */
	statePtr->stats.collisions += 2;	/* Only a guess. */
    }

    statePtr->stats.packetsSent++;

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

ReturnStatus
NetLEOutput(interPtr, etherHdrPtr, scatterGatherPtr, scatterGatherLength, rpc,
	statusPtr)
    Net_Interface	*interPtr;	/* The network interface. */
    Net_EtherHdr	*etherHdrPtr;	/* Ethernet header for the packet. */
    register	Net_ScatterGather	*scatterGatherPtr; /* Data portion of 
							    * the packet. */
    int			scatterGatherLength;	/* Length of data portion gather
						 * array. */
    Boolean		rpc;		/* Is this an rpc packet? */
    ReturnStatus	*statusPtr;	/* Status from sending packet. */
{
    register	NetXmitElement		*xmitPtr;
    ReturnStatus			status;
    NetLEState				*statePtr;

    statePtr = (NetLEState *) interPtr->interfaceData;
    DISABLE_INTR();

    statePtr->stats.packetsOutput++;

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

	    Net_Input(interPtr,(Address)statePtr->loopBackBuffer, length);
        }

        scatterGatherPtr->done = TRUE;
	if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	}

	ENABLE_INTR();
	return SUCCESS;
    }

    /*
     * If no packet is being sent then go ahead and send this one.
     */

    if (!statePtr->transmitting) {
	status = 
	    OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength,
		    statePtr);
	if (status != SUCCESS) {
		NetLERestart(statePtr);
	}
	if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = status;
	}
	ENABLE_INTR();
	return status;
    }

    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */

    if (List_IsEmpty(statePtr->xmitFreeList)) {
        scatterGatherPtr->done = TRUE;
	ENABLE_INTR();
	return FAILURE;
    }

    xmitPtr = 
	(NetXmitElement *)List_First((List_Links *) statePtr->xmitFreeList);

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
    ENABLE_INTR();
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
		     xmitElementPtr->scatterGatherLength,
		     statePtr);
	if (status != SUCCESS) {
	    panic("LE ethernet: Can not output first packet on restart.\n");
	}
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
}
