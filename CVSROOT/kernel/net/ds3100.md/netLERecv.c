/* netLERecv.c -
 *
 *	Routines to manage the receive unit of the AMD LANCE ethernet chip.
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
#include <vm.h>
#include <netLEInt.h>
#include <sys.h>
#include <list.h>
#include <machMon.h>

/*
 * Macro to step ring pointers.
 */
#define	NEXT_RECV(p)	((p) == statePtr->recvDescLastPtr) ? \
				statePtr->recvDescFirstPtr : \
		    (Address)BUF_TO_ADDR(p, NET_LE_RECV_DESC_SIZE)


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
    int			i;

    printf("In AllocateRecvMem\n");
    /*
     * Allocate the ring of receive buffer descriptors.  The ring must start
     * on 8-byte boundary.  
     */
    statePtr->recvDescFirstPtr = 
	NetLEMemAlloc(NET_LE_NUM_RECV_BUFFERS * NET_LE_RECV_DESC_SIZE, FALSE);

    /*
     * Allocate the receive buffers.
     */
    for (i = 0; i < NET_LE_NUM_RECV_BUFFERS; i++) {
	statePtr->recvDataBuffer[i] = 
		NetLEMemAlloc(NET_LE_RECV_BUFFER_SIZE, TRUE);
    }
    statePtr->recvMemAllocated = TRUE;
    /*
     * Allocate memory for the buffer to copy packets into after we
     * receive them.
     */
    statePtr->recvBufPtr = (Address) malloc(NET_LE_RECV_BUFFER_SIZE + 2);
    /*
     * 2 byte align the recvBufPtr so that we can 4 byte align all words
     * after the ethernet header.
     */
    statePtr->recvBufPtr += 2;
    printf("Leaving AllocateRecvMem\n");
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
    Address	descPtr;

    if (!statePtr->recvMemAllocated) {
	AllocateRecvMem(statePtr);
    }
    /*
     * Initialize the state structure to point to the ring. recvDescFirstPtr
     * is set by AllocateRecvMem() and never changed.
     */
    statePtr->recvDescLastPtr = (Address)
	BUF_TO_ADDR(statePtr->recvDescFirstPtr,
		    (NET_LE_NUM_RECV_BUFFERS - 1) * NET_LE_RECV_DESC_SIZE);
    statePtr->recvDescNextPtr = statePtr->recvDescFirstPtr;

    /* 
     * Initialize the ring buffer descriptors.
     */
    descPtr = statePtr->recvDescFirstPtr;
    for (bufNum = 0; 
	 bufNum < NET_LE_NUM_RECV_BUFFERS; 
	 bufNum++, descPtr += 2 * NET_LE_RECV_DESC_SIZE) { 
	 /*
	  * Point the descriptor at its buffer.
	  */
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_BUF_SIZE) =
			-NET_LE_RECV_BUFFER_SIZE;
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_BUF_ADDR_LOW) =
			BUF_TO_CHIP_ADDR(statePtr->recvDataBuffer[bufNum]) & 
			0xFFFF;
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS) =
		((BUF_TO_CHIP_ADDR(statePtr->recvDataBuffer[bufNum]) >> 16) & 
						NET_LE_RECV_BUF_ADDR_HIGH) |
			NET_LE_RECV_START_OF_PACKET |
			NET_LE_RECV_END_OF_PACKET |
			NET_LE_RECV_CHIP_OWNED;
    }
    descPtr = statePtr->recvDescFirstPtr;
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
    register Address		descPtr;
    register volatile short 	*inBufPtr;
    register short		*outBufPtr;
    register unsigned		status;
    register int		i;
    int				size;
    Boolean			tossPacket;

    /*
     * If not initialized then forget the interrupt.
     */
    if (!statePtr->recvMemInitialized) {
	return (FAILURE);
    }

    descPtr = (Address)statePtr->recvDescNextPtr;

    status = *BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS);
    /*
     * First a few consistency check. 
     */
    if (status & NET_LE_RECV_CHIP_OWNED) {
	printf("LE ethernet: Bogus receive interrupt. Buffer owned by chip.\n");
	return (FAILURE);
    }

    if (!(status & NET_LE_RECV_START_OF_PACKET)) {
	printf(
	 "LE ethernet: Bogus receive interrupt. Buffer doesn't start packet.\n");
	return (FAILURE);
    }
    /*
     * Loop as long as there are packets to process.
     */
    tossPacket = dropPackets;
    while (TRUE) {
	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	if (status & NET_LE_RECV_CHIP_OWNED) {
		break;
	}
	/*
	 * Since we allocated very large receive buffers all packets must fit
	 * in one buffer. Hence all buffers should have startOfPacket.
	 */
	if (!(status & NET_LE_RECV_START_OF_PACKET)) {
		printf("LE ethernet: Receive buffer doesn't start packet.\n");
		return (FAILURE);
	}
	/*
	 * All buffers should also have an endOfPacket too.
	 */
	if (!(status & NET_LE_RECV_END_OF_PACKET)) {
	    /* 
	     * If not an endOfPacket see if it was an error packet.
	     */
	    if (!(status & NET_LE_RECV_ERROR)) {
		printf("LE ethernet: Receive buffer doesn't end packet.\n");
		return (FAILURE);
	    }
	    /*
	     * We have got a serious error with a packet. 
	     * Report the error and toss the packet.
	     */
	    tossPacket = TRUE;
	    if (status & NET_LE_RECV_OVER_FLOW_ERROR) {
		statePtr->stats.overrunErrors++;
		printf("LE ethernet: Received packet with overflow error.\n");
	    }
	    /*
	     * Should probably panic on buffer errors.
	     */
	    if (status & NET_LE_RECV_BUFFER_ERROR) {
		panic("LE ethernet: Received packet with buffer error.\n");
	    }
	} else { 
	    /*
	     * The buffer had an endOfPacket bit set. Check for CRC errors and
	     * the like.
	     */
	    if (status & NET_LE_RECV_ERROR) {
		tossPacket = TRUE;	/* Throw away packet on error. */
		if (status & NET_LE_RECV_FRAMING_ERROR) {
		    statePtr->stats.frameErrors++;
		    printf(
			"LE ethernet: Received packet with framing error.\n");
		}
		if (status & NET_LE_RECV_CRC_ERROR) {
		    statePtr->stats.crcErrors++;
		    printf("LE ethernet: Received packet with CRC error.\n");
		}

	     } 
	}

	/* 
	 * Once we gotten here, it means that the buffer contains a packet
	 * and the tossPacket flags says if it is good or not.
	 */

	statePtr->stats.packetsRecvd++;

	/*
	 * Remove the CRC check (4 bytes) at the end of the packet.
	 */
	size = *BUF_TO_ADDR(descPtr, NET_LE_RECV_PACKET_SIZE) - 4;
	/*
	 * Copy the data out.
	 */

	inBufPtr = (volatile short *) CHIP_TO_BUF_ADDR(
	    *BUF_TO_ADDR(descPtr,NET_LE_RECV_BUF_ADDR_LOW) | 
	    ((*BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS) & 
			    NET_LE_RECV_BUF_ADDR_HIGH) << 16));

#define COPY_IN(n) *(outBufPtr + n) = *(inBufPtr + (2 * n))

	outBufPtr = (short *)statePtr->recvBufPtr;
	i = size;
	while (i >= 64) {
	    COPY_IN(0);  COPY_IN(1);  COPY_IN(2);  COPY_IN(3);
	    COPY_IN(4);  COPY_IN(5);  COPY_IN(6);  COPY_IN(7);
	    COPY_IN(8);  COPY_IN(9);  COPY_IN(10); COPY_IN(11);
	    COPY_IN(12); COPY_IN(13); COPY_IN(14); COPY_IN(15);
	    COPY_IN(16); COPY_IN(17); COPY_IN(18); COPY_IN(19);
	    COPY_IN(20); COPY_IN(21); COPY_IN(22); COPY_IN(23);
	    COPY_IN(24); COPY_IN(25); COPY_IN(26); COPY_IN(27);
	    COPY_IN(28); COPY_IN(29); COPY_IN(30); COPY_IN(31);
	    outBufPtr += 32;
	    inBufPtr += 64;
	    i -= 64;
	}
	while (i >= 16) {
	    COPY_IN(0);  COPY_IN(1);  COPY_IN(2);  COPY_IN(3);
	    COPY_IN(4);  COPY_IN(5);  COPY_IN(6);  COPY_IN(7);
	    outBufPtr += 8;
	    inBufPtr += 16;
	    i -= 16;
	}
	while (i > 0) {
	    *outBufPtr = *inBufPtr;
	    outBufPtr++;
	    inBufPtr += 2;
	    i -= 2;
	}
	/*
	* Call higher level protocol to process the packet.
	*/
	if (!tossPacket) {
	    Net_Input(statePtr->interPtr,(Address)statePtr->recvBufPtr, size);
	}
	/*
	 * We're finished with it, give the buffer back to the chip. 
	 */
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS) |= NET_LE_RECV_CHIP_OWNED;

	/*
	 * Clear the interrupt bit for the packet we just got before 
	 * we check the ownership of the next packet. This prevents the 
	 * following race condition: We check the buffer and it is own
	 * by the chip; the chip gives the buffer to us and sets the
	 * interrupt; we clear the interrupt. 
	 * The fix is to always clear the interrupt and then check
	 * for owership as the chip sets owership and then sets the
	 * interrupt.
	 */

	*statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
	*statePtr->regDataPortPtr = 
			NET_LE_CSR0_RECV_INTR | NET_LE_CSR0_INTR_ENABLE;

	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	descPtr = NEXT_RECV(descPtr);
	status = *BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS);
	if (status & NET_LE_RECV_CHIP_OWNED) {
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
