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

#include "sprite.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "list.h"

/*
 * Macro to step ring pointers.
 */

#define	NEXT_RECV(p)	( ((++p) > netLEState.recvDescLastPtr) ? \
				netLEState.recvDescFirstPtr : \
				(p))
/*
 * Receive data buffers.
 */
static	Address	recvDataBuffer[NET_LE_RECV_BUFFER_SIZE];

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
    unsigned int	memBase;
    int			i;

    /*
     * Allocate the ring of receive buffer descriptors.  The ring must start
     * on 8-byte boundary.  
     */
    memBase = (unsigned int) Vm_RawAlloc(
		(NET_LE_NUM_RECV_BUFFERS * sizeof(NetLERecvMsgDesc)) + 8);
    /*
     * Insure ring starts on 8-byte boundary.
     */
    if (memBase & 0x7) {
	memBase = (memBase + 8) & ~0x7;
    }
    netLEState.recvDescFirstPtr = (NetLERecvMsgDesc *) memBase;

    /*
     * Allocate the receive buffers.
     */
     for (i = 0; i < NET_LE_NUM_RECV_BUFFERS; i++) {
	recvDataBuffer[i] = (Address) Vm_RawAlloc(NET_LE_RECV_BUFFER_SIZE);
    }
    recvMemAllocated = TRUE;
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
    NetLERecvMsgDesc	*descPtr;

    if (!recvMemAllocated) {
	AllocateRecvMem();
    }
    /*
     * Initialize the state structure to point to the ring. recvDescFirstPtr
     * is set by AllocateRecvMem() and never changed.
     */
    netLEState.recvDescLastPtr = 
		&(netLEState.recvDescFirstPtr[NET_LE_NUM_RECV_BUFFERS-1]);
    netLEState.recvDescNextPtr = netLEState.recvDescFirstPtr;

    /* 
     * Initialize the ring buffer descriptors.
     */
    descPtr = netLEState.recvDescFirstPtr;
    for (bufNum = 0; bufNum < NET_LE_NUM_RECV_BUFFERS; bufNum++, descPtr++) { 
	 /*
	  * Point the descriptor at its buffer.
	  */
	descPtr->bufAddrLow = 
			NET_LE_SUN_TO_CHIP_ADDR_LOW(recvDataBuffer[bufNum]);
	descPtr->bufAddrHigh = 
			NET_LE_SUN_TO_CHIP_ADDR_HIGH(recvDataBuffer[bufNum]);
	/* 
	 * Insert its size. Note the 2's complementness of the bufferSize.
	 */
	descPtr->bufferSize = -NET_LE_RECV_BUFFER_SIZE;
	/*
	 * Clear out error fields. 
	 */
	descPtr->error = 0;
	descPtr->frammingError = 0;
	descPtr->overflowError = 0;
	descPtr->crcError = 0;
	descPtr->bufferError = 0;
	/*
	 * Clear packet boundry bits.
	 */
	descPtr->startOfPacket = descPtr->endOfPacket = 0;
	
	/*
	 * Set ownership to the chip.
	 */
	descPtr->chipOwned = 1;
    }
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
    register	NetLERecvMsgDesc	*descPtr;
    register	NetLEState		*netLEStatePtr;
    register	Net_EtherHdr		*etherHdrPtr;
    int					size;
    Boolean				tossPacket;

    /*
     * If not initialized then forget the interrupt.
     */

    if (!recvMemInitialized) {
	return (FAILURE);
    }

    netLEStatePtr = &netLEState;

    descPtr = netLEStatePtr->recvDescNextPtr;

    /*
     * First a few consistency check. 
     */
    if (descPtr->chipOwned) {
	Sys_Panic(SYS_WARNING,
	    "LE ethernet: Bogus receive interrupt. Buffer owned by chip.\n");
	return (FAILURE);
    }

    if (!descPtr->startOfPacket) {
	Sys_Panic(SYS_WARNING,
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
	if (descPtr->chipOwned) {
		break;
	}
	/*
	 * Since we allocated very large receive buffers all packets must fit
	 * in one buffer. Hence all buffers should have startOfPacket.
	 */
	if (!descPtr->startOfPacket) {
		Sys_Panic(SYS_WARNING,
		     "LE ethernet: Receive buffer doesn't start packet.\n");
		return (FAILURE);
	}
	/*
	 * All buffers should also have an endOfPacket too.
	 */
	if (!descPtr->endOfPacket) {
	    /* 
	     * If not an endOfPacket see if it was an error packet.
	     */
	    if (!descPtr->error) { 
		Sys_Panic(SYS_WARNING,
		     "LE ethernet: Receive buffer doesn't end packet.\n");
		return (FAILURE);
	    }
	    /*
	     * We have got a serious error with a packet. 
	     * Report the error and toss the packet.
	     */
	    tossPacket = TRUE;
	    if (descPtr->overflowError) {
		net_EtherStats.overrunErrors++;
		Sys_Panic(SYS_WARNING,
		       "LE ethernet: Received packet with overflow error.\n");
	    }
	    /*
	     * Should probably panic on buffer errors.
	     */
	    if (descPtr->bufferError) {
		Sys_Panic(SYS_FATAL,
		       "LE ethernet: Received packet with buffer error.\n");
	    }
	} else { 
	    /*
	     * The buffer had an endOfPacket bit set. Check for CRC errors and
	     * the like.
	     */
	    if (descPtr->error) {
		tossPacket = TRUE;	/* Throw away packet on error. */
		if (descPtr->frammingError) {
		    net_EtherStats.frameErrors++;
		    Sys_Printf(
		       "LE ethernet: Received packet with framming error.\n");
		}
		if (descPtr->crcError) {
		    net_EtherStats.crcErrors++;
		    Sys_Printf(
		       "LE ethernet: Received packet with CRC error.\n");
		}

	     } 
	}

	/* 
	 * Once we gotten here, it means that the buffer contains a packet
	 * and the tossPacket flags says if it is good or not.
	 */

	net_EtherStats.packetsRecvd++;

	etherHdrPtr = (Net_EtherHdr *) 
	    NET_LE_SUN_FROM_CHIP_ADDR(descPtr->bufAddrHigh,descPtr->bufAddrLow);

	 /*
	  * Call higher level protocol to process the packet. Remove the
	  * CRC check (4 bytes) at the end of the packet.
	  */
	size = descPtr->packetSize - 4;
	if (!tossPacket) {
		Net_Input((Address)etherHdrPtr, size);
	}
	/*
	 * We're finish with it, give the buffer back to the chip. 
	 */
	descPtr->chipOwned = 1;

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

	netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
	netLEStatePtr->regPortPtr->dataPort = 
			NET_LE_CSR0_RECV_INTR | NET_LE_CSR0_INTR_ENABLE;

	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	descPtr = NEXT_RECV(descPtr);
	if (descPtr->chipOwned) {
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
