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

#include "sprite.h"
#include "vm.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "list.h"
#include "machMon.h"

/*
 * Macro to step ring pointers.
 */
#define	NEXT_RECV(p)	((p) == netLEState.recvDescLastPtr) ? \
				netLEState.recvDescFirstPtr : \
		    (Address)BUF_TO_ADDR(p, NET_LE_RECV_DESC_SIZE)

/*
 * Receive data buffers.
 */
static	Address	recvDataBuffer[NET_LE_NUM_RECV_BUFFERS];

/*
 * Buffer to copy a packet into after we receive it.
 */
static	char	*recvBufPtr;
/*
 * Flag to note if recv memory has been initialized and/or allocated.
 */
static	Boolean	recvMemInitialized = FALSE;
static	Boolean	recvMemAllocated = FALSE;


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
AllocateRecvMem()
{
    int			i;

    /*
     * Allocate the ring of receive buffer descriptors.  The ring must start
     * on 8-byte boundary.  
     */
    netLEState.recvDescFirstPtr = 
	NetLEMemAlloc(NET_LE_NUM_RECV_BUFFERS * NET_LE_RECV_DESC_SIZE, FALSE);

    /*
     * Allocate the receive buffers.
     */
    for (i = 0; i < NET_LE_NUM_RECV_BUFFERS; i++) {
	recvDataBuffer[i] = NetLEMemAlloc(NET_LE_RECV_BUFFER_SIZE, TRUE);
    }
    recvMemAllocated = TRUE;
    /*
     * Allocate memory for the buffer to copy packets into after we
     * receive them.
     */
    recvBufPtr = (Address)Vm_BootAlloc(NET_LE_RECV_BUFFER_SIZE + 2);
    /*
     * 2 byte align the recvBufPtr so that we can 4 byte align all words
     * after the ethernet header.
     */
    recvBufPtr += 2;
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
NetLERecvInit()
{
    int 	bufNum;
    Address	descPtr;

    if (!recvMemAllocated) {
	AllocateRecvMem();
    }
    /*
     * Initialize the state structure to point to the ring. recvDescFirstPtr
     * is set by AllocateRecvMem() and never changed.
     */
    netLEState.recvDescLastPtr = (Address)
	BUF_TO_ADDR(netLEState.recvDescFirstPtr,
		    (NET_LE_NUM_RECV_BUFFERS - 1) * NET_LE_RECV_DESC_SIZE);
    netLEState.recvDescNextPtr = netLEState.recvDescFirstPtr;

    /* 
     * Initialize the ring buffer descriptors.
     */
    descPtr = netLEState.recvDescFirstPtr;
    for (bufNum = 0; 
	 bufNum < NET_LE_NUM_RECV_BUFFERS; 
	 bufNum++, descPtr += 2 * NET_LE_RECV_DESC_SIZE) { 
	 /*
	  * Point the descriptor at its buffer.
	  */
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_BUF_SIZE) =
			-NET_LE_RECV_BUFFER_SIZE;
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_BUF_ADDR_LOW) =
			BUF_TO_CHIP_ADDR(recvDataBuffer[bufNum]) & 0xFFFF;
	*BUF_TO_ADDR(descPtr,NET_LE_RECV_STATUS) =
		((BUF_TO_CHIP_ADDR(recvDataBuffer[bufNum]) >> 16) & 
						NET_LE_RECV_BUF_ADDR_HIGH) |
			NET_LE_RECV_START_OF_PACKET |
			NET_LE_RECV_END_OF_PACKET |
			NET_LE_RECV_CHIP_OWNED;
    }
    descPtr = netLEState.recvDescFirstPtr;
    recvMemInitialized = TRUE;
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
NetLERecvProcess(dropPackets)
    Boolean	dropPackets;	/* Drop all packets. */
{
    register	Address		descPtr;
    register	NetLEState	*netLEStatePtr;
    int				size;
    Boolean			tossPacket;
    unsigned			status;
    volatile short 		*inBufPtr;
    short			*outBufPtr;
    int				i;

    /*
     * If not initialized then forget the interrupt.
     */
    if (!recvMemInitialized) {
	return (FAILURE);
    }

    netLEStatePtr = &netLEState;

    descPtr = (Address)netLEStatePtr->recvDescNextPtr;

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
		net_EtherStats.overrunErrors++;
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
		    net_EtherStats.frameErrors++;
		    printf(
			"LE ethernet: Received packet with framing error.\n");
		}
		if (status & NET_LE_RECV_CRC_ERROR) {
		    net_EtherStats.crcErrors++;
		    printf("LE ethernet: Received packet with CRC error.\n");
		}

	     } 
	}

	/* 
	 * Once we gotten here, it means that the buffer contains a packet
	 * and the tossPacket flags says if it is good or not.
	 */

	net_EtherStats.packetsRecvd++;

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
	for (outBufPtr = (short *)recvBufPtr, i = size; 
	     i > 0; 
	     outBufPtr += 1, inBufPtr += 2, i -= 2) {
	    *outBufPtr = *inBufPtr;
	}
	 /*
	  * Call higher level protocol to process the packet.
	  */
	if (!tossPacket) {
	    Net_Input((Address)recvBufPtr, size);
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

	*netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
	*netLEStatePtr->regDataPortPtr = 
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

    netLEStatePtr->recvDescNextPtr = descPtr;

    /*
     * RETURN a success.
     */
    return (SUCCESS);
}
