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
#endif not lint

#include "sprite.h"
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "list.h"
#include "vmMach.h"

#include "sync.h"

/*
 * Pointer to scatter gather element for current packet being sent.
 */
static Net_ScatterGather *curScatGathPtr = (Net_ScatterGather *) NIL;

/*
 * The address of the array of buffer descriptor headers.
 */
static	NetIETransmitBufDesc *xmitBufAddr;

/*
 * Extra bytes for short packets.
 */
char	*netIEXmitFiller;

/*
 * Buffer for pieces of a packet that are too small or that start
 * on an odd boundary.  XMIT_TEMP_BUFSIZE limits how big a thing can
 * be and start on an odd address.
 */
#define XMIT_TEMP_BUFSIZE	(NET_ETHER_MAX_BYTES + 2)
char	*netIEXmitTempBuffer;

/*
 * Define the minimum size allowed for a piece of a transmitted packet.
 * There is a minimum size because the Intel chip has problems if the pieces
 * are too small.
 */
#define MIN_XMIT_BUFFER_SIZE	12

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
    register int			bufCount;
    int					totalLength;
    register int			length;
    register Address			bufAddr;
#define VECTOR_LENGTH	20
    int					borrowedBytes[VECTOR_LENGTH];
    int					*borrowedBytesPtr;
#ifdef sun3
    Net_ScatterGather			newScatGathArr[NET_IE_NUM_XMIT_BUFFERS];
#endif

    netIEState.transmitting = TRUE;
    curScatGathPtr = scatterGatherPtr;
#ifdef sun3
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

    xmitCBPtr = netIEState.xmitCBPtr;
    xmitBufDescPtr = xmitBufAddr;

    totalLength = sizeof(Net_EtherHdr);

    /*
     * If vector elements are two small we borrow bytes from the next
     * element.  The borrowedBytes array is used to remember this.  We
     * can't side-effect the main scatter-gather vector becuase that
     * screws up retransmissions.
     */
    borrowedBytesPtr = borrowedBytes;
    *borrowedBytesPtr = 0;
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

	    if (length > XMIT_TEMP_BUFSIZE) {
		netIEState.transmitting = FALSE;
		ENABLE_INTR();

		panic("IE OutputPacket: Odd addressed buffer too large.");
		return;
	    }
	    bcopy(bufAddr, netIEXmitTempBuffer, length);
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
			     &netIEXmitTempBuffer[length], numBorrowedBytes);
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
		 (int) (netIEXmitTempBuffer), (int) (xmitBufDescPtr->bufAddr));
	} else {
	    NET_IE_ADDR_FROM_SUN_ADDR(
		 (int) (bufAddr), (int) (xmitBufDescPtr->bufAddr));
	}

	xmitBufDescPtr->eof = 0;
	xmitBufDescPtr->countLow = length & 0xFF;
	xmitBufDescPtr->countHigh = length >> 8;

	totalLength += length;
	xmitBufDescPtr = 
	 (NetIETransmitBufDesc *) ((int) xmitBufDescPtr + NET_IE_CHUNK_SIZE);
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
	xmitBufDescPtr->countLow = length & 0xFF;
	xmitBufDescPtr->countHigh = length >> 8;
    } else {
	xmitBufDescPtr = 
	 (NetIETransmitBufDesc *) ((int) xmitBufDescPtr - NET_IE_CHUNK_SIZE);
    }

    /*
     * Finish off the packet.
     */

    xmitBufDescPtr->eof = 1;
    xmitCBPtr->destEtherAddr = etherHdrPtr->destination;
    xmitCBPtr->type = etherHdrPtr->type;

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
    netIEState.scbPtr->cmdWord.cmdUnitCmd = NET_IE_CUC_START;
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
    NetIETransmitBufDesc		*newXmitBufDescPtr;
    NetXmitElement	                *xmitElementPtr;
    int					i;

    /*
     * Initialize the transmit command header.
     */

    xmitCBPtr = (NetIETransmitCB *) netIEState.cmdBlockPtr;
    netIEState.xmitCBPtr = xmitCBPtr;
    xmitCBPtr->cmdNumber = NET_IE_TRANSMIT;
    xmitCBPtr->suspend = 0;

    /*
     * Now link in all of the buffer headers.
     */

#ifdef	lint
    xmitBufDescPtr = (NetIETransmitBufDesc *) NIL;
#endif

    for (i = 0; i < NET_IE_NUM_XMIT_BUFFERS; i++) {
	newXmitBufDescPtr = (NetIETransmitBufDesc *) NetIEMemAlloc();
	if (newXmitBufDescPtr == (NetIETransmitBufDesc *) NIL) {
	    panic( "Intel: No memory for the xmit buffers.\n");
	}

	if (i == 0) {
	    xmitBufAddr = newXmitBufDescPtr;
	    xmitCBPtr->bufDescOffset = 
			NetIEOffsetFromSUNAddr((int) newXmitBufDescPtr);
	} else {
	    xmitBufDescPtr->nextTBD = 
			NetIEOffsetFromSUNAddr((int) newXmitBufDescPtr);
	}

	xmitBufDescPtr = newXmitBufDescPtr;
    }

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

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if (curScatGathPtr == (Net_ScatterGather *) NIL) {
	printf( "NetIEXmitDone: No current packet\n.");
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

    DISABLE_INTR();

    net_EtherStats.packetsOutput++;

    /*
     * Verify that the scatter gather array is not too large.  There is a fixed
     * upper bound because the list of transmit buffers is preallocated.
     */

    if (scatterGatherLength >= NET_IE_NUM_XMIT_BUFFERS) {
	scatterGatherPtr->done = TRUE;

	printf("Intel: Packet in too many pieces\n");
	ENABLE_INTR();
	return;
    }

    /*
     * Now make sure that each element in the scatter/gather is big enough.
     * There is a problem with the Intel chip that if the buffers are too 
     * small it will overrun memory.  The first buffer is a special case
     * because it contains the ethernet header which is removed before 
     * sending.  Also make sure that none of the buffers begin on odd 
     * boundaries.
     */

#ifdef not_used
    {
	register	Net_ScatterGather	*tmpPtr;
	register	int			i;

	if (scatterGatherPtr->length > sizeof(Net_EtherHdr) &&
	    scatterGatherPtr->length - sizeof(Net_EtherHdr) < 
							MIN_XMIT_BUFFER_SIZE) {
	     printf( 
		"NetIEXmit: Scatter/gather element too small (1)\n");
	}
	if ((int) (scatterGatherPtr->bufAddr) & 0x1) {
	    printf( 
		      "NetIEXmit: Ethernet Header begins on odd boundary %x\n",
		      (int) (scatterGatherPtr->bufAddr));
	}

	for (i = 1; i < scatterGatherLength; i++) {
	    tmpPtr = &(scatterGatherPtr[i]);
	    if (tmpPtr->length > 0 && tmpPtr->length  < MIN_XMIT_BUFFER_SIZE) {
		printf( 
			"NetIEXmit: Scatter/gather element too small (2)\n");
	    }
	    if (tmpPtr->length > 0 && (int) (tmpPtr->bufAddr) & 0x1) {
		printf( 
			  "NetIEXmit: Buffer begins on odd boundary %x\n",
			  (int) (tmpPtr->bufAddr));
	    }
	}
    }
#endif not_used

    /*
     * See if the packet is for us.  In this case just copy in the packet
     * and call the higher level routine.
     */

    if (NET_ETHER_COMPARE(netIEState.etherAddress, etherHdrPtr->destination)) {
	int i, length;

        length = sizeof(Net_EtherHdr);
        for (i = 0; i < scatterGatherLength; i++) {
            length += scatterGatherPtr[i].length;
        }

        if (length <= NET_ETHER_MAX_BYTES) {
	    register Address bufPtr;

	    etherHdrPtr->source = netIEState.etherAddress;

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

    if (!netIEState.transmitting) {
	OutputPacket(etherHdrPtr, scatterGatherPtr, scatterGatherLength);
	ENABLE_INTR();
	return;
    }

    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */

    if (List_IsEmpty(netIEState.xmitFreeList)) {
        scatterGatherPtr->done = TRUE;
	ENABLE_INTR();
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

    ENABLE_INTR();
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
