/* 
 * netIEXmit.c --
 *
 *	Routines to transmit packets on the Intel interface.
 *
 * Copyright 1985, 1988 Regents of the University of California
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
#include <netIEInt.h>
#include <sys.h>
#include <list.h>
#include <vmMach.h>
#include <sync.h>

/*
 * Extra bytes for short packets.
 */
char	*netIEXmitFiller;



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
OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength, statePtr)
    Net_EtherHdr                        *etherHdrPtr;
    register	Net_ScatterGather   	*scatterGatherPtr;
    int					scatterGatherLength;
    NetIEState				*statePtr;
{
    register	volatile NetIETransmitBufDesc	*xmitBufDescPtr;
    register	volatile NetIETransmitCB   	*xmitCBPtr;
    register int			bufCount;
    int					totalLength;
    register int			length;
    register Address			bufAddr;
#define VECTOR_LENGTH	20
    int					borrowedBytes[VECTOR_LENGTH];
    int					*borrowedBytesPtr;
    char				*tmpBuffer;
    int					tmpBufSize;
#if defined(sun3) || defined(sun4) 
    Net_ScatterGather			newScatGathArr[NET_IE_NUM_XMIT_BUFFERS];
#endif

    statePtr->transmitting = TRUE;
    statePtr->curScatGathPtr = scatterGatherPtr;
#if defined(sun3) || defined(sun4) 
    /*
     * Remap the packet into network addressible memory.
     */
    VmMach_NetMapPacket(scatterGatherPtr, scatterGatherLength, newScatGathArr);
    scatterGatherPtr = newScatGathArr;
#endif

    /*
     * There is already a prelinked command list.  A pointer to the list
     * and the array of buffer headers is gotten here.
     */

    xmitCBPtr = statePtr->xmitCBPtr;
    xmitBufDescPtr = statePtr->xmitBufAddr;

    totalLength = sizeof(Net_EtherHdr);

    /*
     * If vector elements are two small we borrow bytes from the next
     * element.  The borrowedBytes array is used to remember this.  We
     * can't side-effect the main scatter-gather vector becuase that
     * screws up retransmissions.
     */
    borrowedBytesPtr = borrowedBytes;
    *borrowedBytesPtr = 0;
    tmpBuffer = statePtr->netIEXmitTempBuffer;
    tmpBufSize = XMIT_TEMP_BUFSIZE;
    /*
     * Put all of the pieces of the packet into the linked list of xmit
     * buffers.
     */
    for (bufCount = 0 ; bufCount < scatterGatherLength ;
	 bufCount++, scatterGatherPtr++, borrowedBytesPtr++) {

	/*
	 * If is an empty buffer then skip it.  Length might even be negative
	 * if we have borrowed bytes from it to pad out to NET_IE_MIN_DMA_SIZE.
	 */
	borrowedBytesPtr[1] = 0;
	length = scatterGatherPtr->length - *borrowedBytesPtr;
	if (length <= 0) {
	    continue;
	}
	bufAddr = scatterGatherPtr->bufAddr + *borrowedBytesPtr;
	/*
	 * If the buffer is too small then it needs to be made bigger
	 * or the DMA hardware will overrun.  Also, check for buffers
	 * that start at odd addresses.  If one does, then it needs
	 * to be copied to another buffer with an even address.
	 * NB: There is only one temporary buffer.  Bad things will happen
	 * if more than one message uses this temporary buffer at once.
	 */
	if ((length < NET_IE_MIN_DMA_SIZE) || ((int)bufAddr & 0x1)) {

	    if (length > tmpBufSize) {
		statePtr->transmitting = FALSE;
		ENABLE_INTR();

		panic("IE OutputPacket: Odd addressed buffer too large.");
		return;
	    }
	    bcopy(bufAddr, tmpBuffer, length);
	    if (length < NET_IE_MIN_DMA_SIZE) {
		/*
		 * This element of the scatter/gather vector is too small;
		 * the controller DMA has to copy a minimum number of bytes.
		 * We take some bytes from the next non-zero sized element(s)
		 * to pad this one out.
		 */
		register int numBorrowedBytes;
		register int numAvailableBytes;
		while (bufCount < scatterGatherLength - 1) {
		    numBorrowedBytes = NET_IE_MIN_DMA_SIZE - length;
		    numAvailableBytes = scatterGatherPtr[1].length -
					borrowedBytesPtr[1];
		    if (numBorrowedBytes > numAvailableBytes) {
			numBorrowedBytes = numAvailableBytes;
		    }
		    if (numBorrowedBytes > 0) {
			bcopy(scatterGatherPtr[1].bufAddr,
			     &tmpBuffer[length], numBorrowedBytes);
			borrowedBytesPtr[1] = numBorrowedBytes;
			length += numBorrowedBytes;
		    }
		    if (length == NET_IE_MIN_DMA_SIZE) {
			break;
		    } else {
			bufCount++;
			scatterGatherPtr++;
			borrowedBytesPtr++;
			borrowedBytesPtr[1] = 0;
		    }
		}
		length = NET_IE_MIN_DMA_SIZE;
	    }

	    NET_IE_ADDR_FROM_SUN_ADDR(
		 (int) (tmpBuffer), (int) (xmitBufDescPtr->bufAddr));
	    /*
	     * Set up tmpBuffer for the next short segment.
	     */
	    tmpBuffer += length;
	    tmpBufSize -= length;
	    if ((int)tmpBuffer & 0x1) {
		tmpBuffer++;
		tmpBufSize--;
	    }
	} else {
	    NET_IE_ADDR_FROM_SUN_ADDR(
		 (int) (bufAddr), (int) (xmitBufDescPtr->bufAddr));
	}
	NetBfShortSet(xmitBufDescPtr->bits, Eof, 0);
	NetBfShortSet(xmitBufDescPtr->bits, CountLow, length & 0xFF);
	NetBfShortSet(xmitBufDescPtr->bits, CountHigh, length >> 8);

	totalLength += length;
	xmitBufDescPtr = (volatile NetIETransmitBufDesc *)
	    ((int) xmitBufDescPtr + NET_IE_CHUNK_SIZE);
    }

    /*
     * If the packet was too short, then hang some extra storage off of the
     * end of it.
     */

    if (totalLength < NET_ETHER_MIN_BYTES) {
        NET_IE_ADDR_FROM_SUN_ADDR((int) netIEXmitFiller, 
			            (int) xmitBufDescPtr->bufAddr); 
	length = NET_ETHER_MIN_BYTES - totalLength;
	if (length < MIN_XMIT_BUFFER_SIZE) {
	    length = MIN_XMIT_BUFFER_SIZE;
	}
	NetBfShortSet(xmitBufDescPtr->bits, CountLow, length & 0xFF);
	NetBfShortSet(xmitBufDescPtr->bits, CountHigh, length >> 8);
    } else {
	xmitBufDescPtr = (volatile NetIETransmitBufDesc *)
	    ((int) xmitBufDescPtr - NET_IE_CHUNK_SIZE);
    }

    /*
     * Finish off the packet.
     */

    NetBfShortSet(xmitBufDescPtr->bits, Eof, 1);
    xmitCBPtr->destEtherAddr = etherHdrPtr->destination;
    xmitCBPtr->type = etherHdrPtr->type;

    /*
     * Append the command onto the command queue.
     */

    *(short *) xmitCBPtr = 0;      /* Clear the status bits. */
    NetBfWordSet(xmitCBPtr->bits, EndOfList, 1);
    NetBfWordSet(xmitCBPtr->bits, Interrupt, 1);


    /*
     * Make sure that the last command was accepted and then
     * start the command unit.
     */

    NET_IE_CHECK_SCB_CMD_ACCEPT(statePtr->scbPtr);
    NetBfShortSet(statePtr->scbPtr->cmdWord, CmdUnitCmd, NET_IE_CUC_START);
    NET_IE_CHANNEL_ATTENTION(statePtr);
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
NetIEXmitInit(statePtr)
    NetIEState		*statePtr;
{
    register volatile NetIETransmitCB	    *xmitCBPtr;
    register volatile NetIETransmitBufDesc  *xmitBufDescPtr;
    volatile NetIETransmitBufDesc	    *newXmitBufDescPtr;
    volatile NetXmitElement	            *xmitElementPtr;
    int	     i;

    /*
     * Initialize the transmit command header.
     */

    xmitCBPtr = (NetIETransmitCB *) statePtr->cmdBlockPtr;
    statePtr->xmitCBPtr = xmitCBPtr;
    NetBfWordSet(xmitCBPtr->bits, CmdNumber, NET_IE_TRANSMIT);
    NetBfWordSet(xmitCBPtr->bits, Suspend, 0);

    /*
     * Now link in all of the buffer headers.
     */

    xmitBufDescPtr = (volatile NetIETransmitBufDesc *) NIL;
    for (i = 0; i < NET_IE_NUM_XMIT_BUFFERS; i++) {
	newXmitBufDescPtr = (volatile NetIETransmitBufDesc *) 
				NetIEMemAlloc(statePtr);
	if (newXmitBufDescPtr == (volatile NetIETransmitBufDesc *) NIL) {
	    panic( "Intel: No memory for the xmit buffers.\n");
	}

	if (i == 0) {
	    statePtr->xmitBufAddr = newXmitBufDescPtr;
	    xmitCBPtr->bufDescOffset = 
			NetIEOffsetFromSUNAddr((int) newXmitBufDescPtr,
				statePtr);
	} else {
	    xmitBufDescPtr->nextTBD = 
			NetIEOffsetFromSUNAddr((int) newXmitBufDescPtr,
				statePtr);
	}

	xmitBufDescPtr = newXmitBufDescPtr;
    }

    /*
     * If there are packets on the queue then go ahead and send 
     * the first one.
     */

    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(statePtr->xmitList);
	OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength, statePtr);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return;
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
NetIEXmitDone(statePtr)
    NetIEState	*statePtr;
{
    register	volatile NetXmitElement     *xmitElementPtr;
    register	volatile NetIETransmitCB    *cmdPtr;
    Net_ScatterGather	*curScatGathPtr;

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (statePtr->curScatGathPtr == (Net_ScatterGather *) NIL) {
#ifndef sun4
	/*
	 * Need to fix this for the sun4.
	 */
	printf( "NetIEXmitDone: No current packet\n.");
#endif
	return;
    }
    curScatGathPtr = statePtr->curScatGathPtr;

    statePtr->stats.packetsSent++;

    /*
     * Mark the packet as done.
     */
    curScatGathPtr->done = TRUE;
    if (curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	NetOutputWakeup(curScatGathPtr->mutexPtr);
    }

    /*
     * Record statistics about the packet.
     */
    cmdPtr = statePtr->xmitCBPtr;
    if (NetBfWordTest(cmdPtr->bits, TooManyCollisions, 1)) {
	statePtr->stats.xmitCollisionDrop++;
	statePtr->stats.collisions += 16;
    } else {
	statePtr->stats.collisions += NetBfWordGet(cmdPtr->bits, NumCollisions);
    }
    if (NetBfWordTest(cmdPtr->bits, CmdOK, 0)) {
	statePtr->stats.xmitPacketsDropped++;
    }

    /*
     * If there are more packets to send then send the first one on
     * the queue.  Otherwise there is nothing being transmitted.
     */
    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetXmitElement *) List_First(statePtr->xmitList);
	OutputPacket(xmitElementPtr->etherHdrPtr,
		     xmitElementPtr->scatterGatherPtr,
		     xmitElementPtr->scatterGatherLength, statePtr);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEOutput --
 *
 *	Output a packet.  The procedure is to either put the packet onto the 
 *	queue of outgoing packets if packets are already being sent, or 
 *	otherwise to send the packet directly.  The elements of the scatter 
 *	array which come into this routine must satisfy the following two 
 *	properties:
 *
 *	1) No buffer must be below the size MIN_XMIT_BUFFER_SIZE.  If one is
 *	   then a warning will be printed and there is a good chance that
 *	   the packet will not make it.
 *	2) No buffer can start on an odd boundary.  It appears that the Intel
 *	   chip drops the low order bit of the address.  Thus if a buffer
 *	   begins on an odd boundary the actual buffer sent will have one
 *	   extra byte at the front.  If a buffer does begin on an odd 
 *	   boundary then a warning message is printed and the packet is
 *	   sent anyway.
 *
 *	In theory the statusPtr argument should be filled in by NetIEXmitDone
 *	when the command completes.  We fill it in here because a 
 *	mechansism doesn't exist for getting the pointer to NetIEXmitDone,
 *	and because it doesn't appear that NetIEXmitDone would ever
 *	return anything other than SUCCESS.
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
NetIEOutput(interPtr, hdrPtr, scatterGatherPtr, scatterGatherLength, rpc,
	    statusPtr)
    Net_Interface			*interPtr;
    Address				hdrPtr;
    register	Net_ScatterGather	*scatterGatherPtr;
    int					scatterGatherLength;
    Boolean				rpc;		/* Is this an rpc? */
    ReturnStatus			*statusPtr;  /* Return status. */
{
    register volatile NetXmitElement    *xmitPtr;
    NetIEState				*statePtr;
    int					i;
    Net_ScatterGather			*gathPtr;
    Net_EtherHdr			*etherHdrPtr = (Net_EtherHdr *) hdrPtr;

    statePtr = (NetIEState *) interPtr->interfaceData;
    DISABLE_INTR();

    statePtr->stats.packetsOutput++;

    /*
     * Verify that the scatter gather array is not too large.  There is a fixed
     * upper bound because the list of transmit buffers is preallocated.
     */

    if (scatterGatherLength >= NET_IE_NUM_XMIT_BUFFERS) {
	scatterGatherPtr->done = TRUE;

	printf("Intel: Packet in too many pieces\n");
	ENABLE_INTR();
	return FAILURE;
    }
    statePtr->stats.bytesSent += sizeof(Net_EtherHdr);
    for (i = scatterGatherLength, gathPtr = scatterGatherPtr; 
	 i > 0; 
	 i--, gathPtr++) { 
	statePtr->stats.bytesSent += gathPtr->length; 
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

	    Net_Input(interPtr, (Address)statePtr->loopBackBuffer, 
		    length);
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
	OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength,
		statePtr);
	if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	}
	ENABLE_INTR();
	return SUCCESS;
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

    xmitPtr = (volatile NetXmitElement *)
	List_First((List_Links *) statePtr->xmitFreeList);

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
 * NetIEXmitDrop --
 *
 *	This drops the current output packet by marking its scatter/gather
 *	vector as DONE and notifying the process waiting for its
 *	output to complete.  This is called in the beginning of the
 *	Restart sequence.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets curScatGathPtr and notifies any process waiting on output.
 *
 *----------------------------------------------------------------------
 */

void
NetIEXmitDrop(statePtr)
    NetIEState		*statePtr;
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
