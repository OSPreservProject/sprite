/* netLERecv.c -
 *
 * Routines to manage the receive unit of the AMD LANCE ethernet chip.
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
#endif not lint

#include <sprite.h>
#include <netLEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <sys.h>
#include <list.h>
#include <machMon.h>

/*
 * Macro to step ring pointers.
 */

#define	NEXT_RECV(p)	( ((p+1) > statePtr->recvDescLastPtr) ? \
				statePtr->recvDescFirstPtr : \
				(p+1))
#define	PREV_RECV(p)	( ((p-1) < statePtr->recvDescFirstPtr) ? \
				statePtr->recvDescLastPtr : \
				(p-1))


static void AllocateRecvMem _ARGS_((NetLEState *statePtr));

/*
 *----------------------------------------------------------------------
 *
 * AllocateRecvMem --
 *
 *	Allocate kernel memory for receive ring and data buffers.	
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
AllocateRecvMem(statePtr)
    NetLEState		*statePtr; /* The state of the interface. */
{
    unsigned int	memBase;
    int			i;

    /*
     * Allocate the ring of receive buffer descriptors.  The ring must start
     * on 8-byte boundary.  
     */
    memBase = (unsigned int) BufAlloc(statePtr, 
		(NET_LE_NUM_RECV_BUFFERS * sizeof(NetLERecvMsgDesc)) + 8);
    /*
     * Insure ring starts on 8-byte boundary.
     */
    if (memBase & 0x7) {
	memBase = (memBase + 8) & ~0x7;
    }
    statePtr->recvDescFirstPtr = (volatile NetLERecvMsgDesc *) memBase;

    /*
     * Allocate the receive buffers. The buffers are
     * allocated on an odd short word boundry so that packet data (after
     * the ethernet header) will start on a long word boundry. This
     * eliminates unaligned word fetches from the RPC module which would
     * cause alignment traps on SPARC processors such as the sun4.
     *
     * This code assumes that ethernet headers are an odd number of short
     * (2-byte) words long.
     */

    for (i = 0; i < NET_LE_NUM_RECV_BUFFERS; i++) {
	int 	addr;
	addr = (int) BufAlloc(statePtr, NET_LE_RECV_BUFFER_SIZE + 3);
	addr += 1;
	addr &= ~0x3;
	addr += 2;
        statePtr->recvDataBuffer[i] = (Address) addr;
	bzero((char *) statePtr->recvDataBuffer[i], NET_LE_RECV_BUFFER_SIZE);
    }
#undef ALIGNMENT_PADDING

    statePtr->recvMemAllocated = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * NetLERecvInit --
 *
 *	Initialize the receive buffer lists for the receive unit allocating
 *	memory if need.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The receive ring is initialized and the device state structure is
 *	updated.
 *
 *----------------------------------------------------------------------
 */

void
NetLERecvInit(statePtr)
    NetLEState		*statePtr; /* The state of the interface. */
{
    int 	bufNum;
    volatile NetLERecvMsgDesc	*descPtr;

    if (!statePtr->recvMemAllocated) {
	AllocateRecvMem(statePtr);
    }
    /*
     * Initialize the state structure to point to the ring. recvDescFirstPtr
     * is set by AllocateRecvMem() and never changed.
     */
    statePtr->recvDescLastPtr = 
		&(statePtr->recvDescFirstPtr[NET_LE_NUM_RECV_BUFFERS-1]);
    statePtr->recvDescNextPtr = statePtr->recvDescFirstPtr;

    /* 
     * Initialize the ring buffer descriptors.
     */
    descPtr = statePtr->recvDescFirstPtr;
    for (bufNum = 0; bufNum < NET_LE_NUM_RECV_BUFFERS; bufNum++, descPtr++) { 
	bzero((char *) descPtr, sizeof(NetLERecvMsgDesc));
	 /*
	  * Point the descriptor at its buffer.
	  */
	descPtr->bufAddrLow = 
		NET_LE_TO_CHIP_ADDR_LOW(statePtr->recvDataBuffer[bufNum]);
	descPtr->bufAddrHigh = 
		NET_LE_TO_CHIP_ADDR_HIGH(statePtr->recvDataBuffer[bufNum]);
	/* 
	 * Insert its size. Note the 2's complementness of the bufferSize.
	 */
	descPtr->bufferSize = -NET_LE_RECV_BUFFER_SIZE;
	/*
	 * Set ownership to the chip.
	 */
	NetBfByteSet(descPtr->bits, ChipOwned, 1);
    }
    statePtr->lastRecvCnt = 0;
    statePtr->recvMemInitialized = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * NetLERecvProcess --
 *
 *	Process a newly received packet.
 *
 * Results:
 *	FAILURE if something went wrong, SUCCESS otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetLERecvProcess(dropPackets, statePtr)
    Boolean		dropPackets;	/* Drop all packets. */
    NetLEState		*statePtr; 	/* The state of the interface. */
{
    register	volatile NetLERecvMsgDesc	*descPtr;
    register	Net_EtherHdr		*etherHdrPtr;
    int					size;
    Boolean				tossPacket;
    int					numResets;

    /*
     * If not initialized then forget the interrupt.
     */

    if (!statePtr->recvMemInitialized) {
	return (FAILURE);
    }
    descPtr = statePtr->recvDescNextPtr;

    if (NetBfByteTest(descPtr->bits, ChipOwned, 1)) {
	if (statePtr->lastRecvCnt == 0) {
	    printf(
	"LE ethernet: Bogus receive interrupt. Buffer 0x%x owned by chip.\n",
		descPtr);
	    return (FAILURE);
	} else {
	    statePtr->lastRecvCnt = 0;
	    return (SUCCESS);
	}
    }
    if (NetBfByteTest(descPtr->bits, StartOfPacket, 0)) {
	printf(
	 "LE ethernet: Bogus receive interrupt. Buffer doesn't start packet.\n");
	return (FAILURE);
    }
    /*
     * Loop as long as there are packets to process.
     */

    tossPacket = dropPackets;
    numResets = statePtr->numResets;
    statePtr->lastRecvCnt = 0;
    while (TRUE) {
	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	if (NetBfByteTest(descPtr->bits, ChipOwned, 1)) {
	    break;
	}
	/*
	 * Since we allocated very large receive buffers all packets must fit
	 * in one buffer. Hence all buffers should have startOfPacket.
	 */
	if (NetBfByteTest(descPtr->bits, StartOfPacket, 0)) {
		printf(
		     "LE ethernet: Receive buffer doesn't start packet.\n");
		return (FAILURE);
	}
	/*
	 * All buffers should also have an endOfPacket too.
	 */
	if (NetBfByteTest(descPtr->bits, EndOfPacket, 0)) {
	    /* 
	     * If not an endOfPacket see if it was an error packet.
	     */
	    if (NetBfByteTest(descPtr->bits, Error, 0)) {
		printf(
		     "LE ethernet: Receive buffer doesn't end packet.\n");
		return (FAILURE);
	    }
	    /*
	     * We have got a serious error with a packet. 
	     * Report the error and toss the packet.
	     */
	    tossPacket = TRUE;
	    if (NetBfByteTest(descPtr->bits, OverflowError, 1)) {
		statePtr->stats.overrunErrors++;
		printf(
		       "LE ethernet: Received packet with overflow error.\n");
	    }
	    /*
	     * Should probably panic on buffer errors.
	     */
	    if (NetBfByteTest(descPtr->bits, RecvBufferError, 1)) {
		panic(
		       "LE ethernet: Received packet with buffer error.\n");
	    }
	} else { 
	    /*
	     * The buffer had an endOfPacket bit set. Check for CRC errors and
	     * the like.
	     */
	    if (NetBfByteTest(descPtr->bits, Error, 1)) {
		tossPacket = TRUE;	/* Throw away packet on error. */
		if (NetBfByteTest(descPtr->bits, FramingError, 1)) {
		    statePtr->stats.frameErrors++;
		    printf(
		       "LE ethernet: Received packet with framing error.\n");
		}
		if (NetBfByteTest(descPtr->bits, CrcError, 1)) {
		    statePtr->stats.crcErrors++;
		    printf(
		       "LE ethernet: Received packet with CRC error.\n");
		}

	     } 
	}

	/* 
	 * Once we gotten here, it means that the buffer contains a packet
	 * and the tossPacket flags says if it is good or not.
	 */

	statePtr->stats.packetsRecvd++;

	etherHdrPtr = (Net_EtherHdr *) 
	    NET_LE_FROM_CHIP_ADDR(statePtr, descPtr->bufAddrHigh,
		descPtr->bufAddrLow);

	 /*
	  * Call higher level protocol to process the packet. Remove the
	  * CRC check (4 bytes) at the end of the packet.
	  */
	size = descPtr->packetSize - 4;
	if (!tossPacket) {
		Net_Input(statePtr->interPtr, (Address)etherHdrPtr, size);
	}
	/*
	 * If the number of resets has changed then somebody reset the chip
	 * in Net_Input. In that case we can't keep processing packets because
	 * the packet pointers have been reset.  
	 */
	if (numResets != statePtr->numResets) {
	    return SUCCESS;
	}
	/*
	 * We're finish with it, give the buffer back to the chip. 
	 */
	NetBfByteSet(descPtr->bits, ChipOwned, 1);

	statePtr->lastRecvCnt++;
	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	descPtr = NEXT_RECV(descPtr);
	if (NetBfByteTest(descPtr->bits, ChipOwned, 1)) {
		break;
	}
    }
    /*
     * Update the the ring pointer. We should be pointer at the chip owned 
     * buffer in that the next packet will be put.
     */

    statePtr->recvDescNextPtr = descPtr;

    /*
     * RETURN a success.
     */
    return (SUCCESS);
}
