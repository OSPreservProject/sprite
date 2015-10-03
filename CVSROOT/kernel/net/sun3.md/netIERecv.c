/* netIERecv.c -
 *
 * Routines to manage the receive unit of the Intel ethernet chip.
 *
 * Copyright 1985, 1988 Regents of the University of California
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
#endif

#include <sprite.h>
#include <netIEInt.h>
#include <sys.h>
#include <list.h>


/*
 *----------------------------------------------------------------------
 *
 * NetIERecvUnitInit --
 *
 *	Initialize the receive buffer lists for the receive unit and start
 *	it going.
 *
 *	NOTE: One more buffer descriptor is allocated than frame descriptor
 *	      because Sun claims that this gets rid of a microcode bug.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The receive frame descriptor and buffer lists are initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetIERecvUnitInit(statePtr)
    NetIEState		*statePtr;
{
    int i;
    register volatile NetIERecvBufDesc   *recvBufDescPtr;
    register volatile NetIERecvFrameDesc *recvFrDescPtr;
    register	volatile NetIESCB           *scbPtr;
    int		bufferSize;

    bufferSize = NET_IE_RECV_BUFFER_SIZE - sizeof(Net_EtherHdr);
    /*
     * Allocate the receive buffer descriptors.  
     */

    for (i = 0; i < NET_IE_NUM_RECV_BUFFERS; i++) {
	recvBufDescPtr = (volatile NetIERecvBufDesc *) NetIEMemAlloc(statePtr);
	if (recvBufDescPtr == (volatile NetIERecvBufDesc *) NIL) {
	    panic("No memory for a receive buffer descriptor pointer\n");
	}

	*(short *)recvBufDescPtr = 0;	/* Clear out the status word */

	if (i == 0) {
	    statePtr->recvBufDscHeadPtr = recvBufDescPtr;
	    statePtr->recvBufDscTailPtr = recvBufDescPtr;
	} else {
	    statePtr->recvBufDscTailPtr->nextRBD = 
			NetIEOffsetFromSUNAddr((int) recvBufDescPtr,
				statePtr);
	    statePtr->recvBufDscTailPtr->realNextRBD = recvBufDescPtr;
	    statePtr->recvBufDscTailPtr = recvBufDescPtr;
	}

	/*
	 * Point the header to its buffer.  It is pointed to the buffer plus
	 * the size of the ethernet header so that when we receive the 
	 * packet we can fill in the ethernet header.
	 */

	recvBufDescPtr->bufAddr = 
		NetIEAddrFromSUNAddr((int) (statePtr->netIERecvBuffers[i] + 
			sizeof(Net_EtherHdr)));
	recvBufDescPtr->realBufAddr = statePtr->netIERecvBuffers[i];
	NetBfShortSet(recvBufDescPtr->bits2, BufSizeHigh, bufferSize >> 8);
	NetBfShortSet(recvBufDescPtr->bits2, BufSizeLow, bufferSize & 0xff);
	NetBfShortSet(recvBufDescPtr->bits2, RBDEndOfList, 0);
    }

    /*
     * Link the last element to the first to make it circular and mark the last
     * element as the end of the list.
     */

    recvBufDescPtr->nextRBD = 
		NetIEOffsetFromSUNAddr((int) statePtr->recvBufDscHeadPtr,
			statePtr);
    recvBufDescPtr->realNextRBD = statePtr->recvBufDscHeadPtr;
    NetBfShortSet(recvBufDescPtr->bits2, RBDEndOfList, 1);

    /*
     * Now allocate the receive frame headers.
     */

    for (i = 0; i < NET_IE_NUM_RECV_BUFFERS - 1; i++) {
	recvFrDescPtr = (volatile NetIERecvFrameDesc *) NetIEMemAlloc(statePtr);
	if (recvFrDescPtr == (volatile NetIERecvFrameDesc *) NIL) {
	    panic("No memory for a receive frame descriptor pointer\n");
	}

	*(short *)recvFrDescPtr = 0;	/* Clear out the status word */

	NetBfWordSet(recvFrDescPtr->bits, EndOfList, 0);
	NetBfWordSet(recvFrDescPtr->bits, Suspend, 0);

	if (i == 0) {
	    statePtr->recvFrDscHeadPtr = recvFrDescPtr;
	    statePtr->recvFrDscTailPtr = recvFrDescPtr;

	    /*
	     * The first receive frame descriptor points to the list of buffer
	     * descriptors.
	     */

	    recvFrDescPtr->recvBufferDesc = 
		    NetIEOffsetFromSUNAddr((int) statePtr->recvBufDscHeadPtr,
			    statePtr);

	} else {
	    recvFrDescPtr->recvBufferDesc = NET_IE_NULL_RECV_BUFF_DESC;
	    statePtr->recvFrDscTailPtr->nextRFD = 
			NetIEOffsetFromSUNAddr((int) recvFrDescPtr,
				statePtr);
	    statePtr->recvFrDscTailPtr->realNextRFD = recvFrDescPtr;
	    statePtr->recvFrDscTailPtr = recvFrDescPtr;
	}
    }

    /*
     * Link the last element to the first to make it circular.
     */

    recvFrDescPtr->nextRFD = 
		    NetIEOffsetFromSUNAddr((int) statePtr->recvFrDscHeadPtr,
			    statePtr);
    recvFrDescPtr->realNextRFD = statePtr->recvFrDscHeadPtr;

    NetBfWordSet(recvFrDescPtr->bits, EndOfList, 1);

    scbPtr = statePtr->scbPtr;

    /*
     * Now start up the receive unit.  To do this we first make sure that
     * it is idle.  Then we start it up.
     */

    if (!NetBfShortTest(scbPtr->statusWord, RecvUnitStatus, NET_IE_RUS_IDLE)) {
	printf("Intel: The receive unit is not idle!!!\n");

	NetBfShortSet(scbPtr->cmdWord, RecvUnitCmd, NET_IE_RUC_ABORT);

	NET_IE_CHANNEL_ATTENTION(statePtr);
	NetIECheckSCBCmdAccept(scbPtr);
    }

    scbPtr->recvFrameAreaOffset = 
		    NetIEOffsetFromSUNAddr((int) statePtr->recvFrDscHeadPtr,
			    statePtr);
    NetBfShortSet(scbPtr->cmdWord, RecvUnitCmd, NET_IE_RUC_START);

    NET_IE_CHANNEL_ATTENTION(statePtr);
    NetIECheckSCBCmdAccept(scbPtr);

    NET_IE_DELAY(NetBfShortTest(scbPtr->statusWord, RecvUnitStatus, 
	NET_IE_RUS_READY));

    if (!NetBfShortTest(scbPtr->statusWord, RecvUnitStatus, NET_IE_RUS_READY)){
	printf("Intel: Receive unit never became ready.\n");
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIERecvProcess --
 *
 *	Process a newly received packet.
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
NetIERecvProcess(dropPackets, statePtr)
    Boolean	dropPackets;	/* Drop all packets. */
    NetIEState	*statePtr;
{
    register	volatile NetIERecvBufDesc	*recvBufDescPtr;
    register	volatile NetIERecvFrameDesc	*recvFrDescPtr;
    register	volatile Net_EtherHdr		*etherHdrPtr;
    volatile    NetIERecvFrameDesc		*newRecvFrDescPtr;
    int		size;
    int		num;


    recvFrDescPtr = statePtr->recvFrDscHeadPtr;

    /*
     * If not initialized then forget the interrupt.
     */

    if (recvFrDescPtr == (NetIERecvFrameDesc *) NIL) {
	return;
    }

    /*
     * Loop as long as there are packets to process.
     */

    while (NetBfWordTest(recvFrDescPtr->bits, Done, 1)) {

	statePtr->stats.packetsRecvd++;

	/*
	 * If this packet has a buffer associated with it then process it.
	 */

	if ((unsigned short) recvFrDescPtr->recvBufferDesc != 
			NET_IE_NULL_RECV_BUFF_DESC) {

	    recvBufDescPtr = statePtr->recvBufDscHeadPtr;
	    size = NetBfShortGet(recvBufDescPtr->bits1, CountLow) +
		(NetBfShortGet(recvBufDescPtr->bits1, CountHigh) << 8) + 
		sizeof(Net_EtherHdr);

	    /*
	     * Put the ethernet header into the packet.
	     */

	    etherHdrPtr = (Net_EtherHdr *) recvBufDescPtr->realBufAddr;
	    etherHdrPtr->source = recvFrDescPtr->srcAddr;
	    etherHdrPtr->destination = recvFrDescPtr->destAddr;
	    etherHdrPtr->type = recvFrDescPtr->type;

	    /*
	     * Call higher level protocol to process the packet.
	     */
	    if (!dropPackets) {
		Net_Input(statePtr->interPtr, (Address)etherHdrPtr, size);
	    }

	    /*
	     * Make the element that was just processed the last element in the
	     * list.  Since this is circular list, no relinking has to be done.
	     */

	    *(short *) recvBufDescPtr = 0;	/* Clear out the status word. */
	    NetBfShortSet(recvBufDescPtr->bits2, RBDEndOfList, 1);
	    NetBfShortSet(statePtr->recvBufDscTailPtr->bits2, RBDEndOfList, 0);
	    statePtr->recvBufDscTailPtr = recvBufDescPtr;
	    statePtr->recvBufDscHeadPtr = recvBufDescPtr->realNextRBD;
	}
	/*
	 * Make the element that was just processed the last element in the
	 * list.  Since this is circular list, no relinking has to be done.
	 */

	newRecvFrDescPtr = recvFrDescPtr->realNextRFD;
	recvFrDescPtr->recvBufferDesc = NET_IE_NULL_RECV_BUFF_DESC;
	NetBfWordSet(recvFrDescPtr->bits, EndOfList, 1);
	*(short *) recvFrDescPtr = 0;
	NetBfWordSet(statePtr->recvFrDscTailPtr->bits, EndOfList, 0);
	statePtr->recvFrDscTailPtr = recvFrDescPtr;

	statePtr->recvFrDscHeadPtr = newRecvFrDescPtr;
	recvFrDescPtr = newRecvFrDescPtr;
    }

    /*
     * Record statistics about packets.
     */

    if (statePtr->scbPtr->crcErrors != 0) {
	num = statePtr->scbPtr->crcErrors;
	statePtr->scbPtr->crcErrors = 0;
	statePtr->stats.crcErrors += NetIEShortSwap(num);
    }

    if (statePtr->scbPtr->alignErrors != 0) {
	num = statePtr->scbPtr->alignErrors;
	statePtr->scbPtr->alignErrors = 0;
	statePtr->stats.frameErrors += NetIEShortSwap(num);
    }

    if (statePtr->scbPtr->resourceErrors != 0) {
	num = statePtr->scbPtr->resourceErrors;
	statePtr->scbPtr->resourceErrors = 0;
	statePtr->stats.recvPacketsDropped += NetIEShortSwap(num);
    }

    if (statePtr->scbPtr->overrunErrors != 0) {
	num = statePtr->scbPtr->overrunErrors;
	statePtr->scbPtr->overrunErrors = 0;
	statePtr->stats.overrunErrors += NetIEShortSwap(num);
    }

    /*
     * See if the receive unit is ready.  If it is, then return.
     */

    if (NetBfShortTest(statePtr->scbPtr->statusWord, RecvUnitStatus, 
	NET_IE_RUS_READY)) {
	return;
    }

    /*
     * Otherwise reinitialize the receive unit.  To do this set the head
     * receive frame pointer to point to the head of the list of buffer
     * headers and give the reinit command to the chip.
     */

    printf("Reinit recv unit\n");

    statePtr->recvFrDscHeadPtr->recvBufferDesc = 
		NetIEOffsetFromSUNAddr((int) statePtr->recvBufDscHeadPtr,
			statePtr);
    statePtr->scbPtr->recvFrameAreaOffset =
		NetIEOffsetFromSUNAddr((int) statePtr->recvFrDscHeadPtr,
			statePtr);
    NET_IE_CHECK_SCB_CMD_ACCEPT(statePtr->scbPtr);
    NetBfShortSet(statePtr->scbPtr->cmdWord, RecvUnitCmd, NET_IE_RUC_START);
    NET_IE_CHANNEL_ATTENTION(statePtr);
    return;
}
