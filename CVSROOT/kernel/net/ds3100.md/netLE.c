/* 
 * netLE.c --
 *
 *	The main routines for the device driver for the AMD 7990 Ethernet 
 *	Controller.
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

#include <netInt.h>
#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <netLEInt.h>
#include <machAddrs.h>
#include <assert.h>

Address	NetLEMemAlloc();


/*
 *----------------------------------------------------------------------
 *
 * NetLEInit --
 *
 *	Initialize the LANCE AMD 7990 Ethernet chip.
 *
 * Results:
 *	SUCCESS if the LANCE controller was found and initialized,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the netEtherFuncs record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetLEInit(interPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
{
    Address 		ctrlAddr;/* Kernel virtual address of controller. */
    int 		i;
    List_Links		*itemPtr;
    NetLEState		*statePtr;
    char		buffer[32];

    assert(sizeof(NetLE_Reg) == 4);

    DISABLE_INTR();

    ctrlAddr = interPtr->ctrlAddr;
    /*
     * If the address is physical (not in kernel's virtual address space)
     * then we have to map it in.
     */
    if (interPtr->virtual == FALSE) {
	printf("NetLEInit: ds3100 does not support mapping in devices yet.\n");
	printf("NetLEInit: can't map in network device at 0x%x\n", ctrlAddr);
	return FAILURE;
#if 0
	ctrlAddr = (char *) VmMach_MapInDevice(ctrlAddr, 1);
#endif
    }
    statePtr = (NetLEState *) malloc (sizeof(NetLEState));
    statePtr->running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.
     */
    statePtr->regDataPortPtr = (unsigned short *)MACH_NETWORK_INTERFACE_ADDR;
    statePtr->regAddrPortPtr = statePtr->regDataPortPtr + 2;
    {
	/*
	 * Poke the controller by setting the RAP.
	 */
	short value = NET_LE_CSR0_ADDR;
	ReturnStatus status;
	status = Mach_Probe(sizeof(short), (char *) &value, 
			  (char *) statePtr->regAddrPortPtr);
	if (status != SUCCESS) {
	    /*
	     * Got a bus error.
	     */
	    free((char *) statePtr);
	    ENABLE_INTR();
	    return(FAILURE);
	}
    }
    Mach_SetHandler(interPtr->vector, Net_Intr, (ClientData) interPtr);
    /*
     * Initialize the transmission list.  
     */
    statePtr->xmitList = &statePtr->xmitListHdr;
    List_Init(statePtr->xmitList);

    statePtr->xmitFreeList = &statePtr->xmitFreeListHdr;
    List_Init(statePtr->xmitFreeList);

    for (i = 0; i < NET_LE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) malloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->xmitFreeList));
    }

    Mach_GetEtherAddress(&statePtr->etherAddress);
    (void) Net_EtherAddrToString(&statePtr->etherAddress, buffer);
    printf("%s-%d Ethernet address %s\n", interPtr->name, interPtr->number, 
	    buffer);
    /*
     * Allocate the initialization block.
     */
    statePtr->initBlockPtr = NetLEMemAlloc(NET_LE_INIT_SIZE, TRUE);

    interPtr->init	= NetLEInit;
    interPtr->output 	= NetLEOutput;
    interPtr->intr	= NetLEIntr;
    interPtr->reset 	= NetLERestart;
    interPtr->getStats	= NetLEGetStats;
    interPtr->netType	= NET_NETWORK_ETHER;
    interPtr->maxBytes	= NET_ETHER_MAX_BYTES - sizeof(Net_EtherHdr);
    interPtr->minBytes	= 0;
    interPtr->interfaceData = (ClientData) statePtr;
    NET_ETHER_ADDR_COPY(statePtr->etherAddress, 
	interPtr->netAddress[NET_PROTO_RAW].ether);
    interPtr->broadcastAddress.ether = netEtherBroadcastAddress.ether;
    interPtr->flags |= NET_IFLAGS_BROADCAST;
    statePtr->interPtr = interPtr;
    statePtr->recvMemInitialized = FALSE;
    statePtr->recvMemAllocated = FALSE;
    statePtr->xmitMemInitialized = FALSE;
    statePtr->xmitMemAllocated = FALSE;


    /*
     * Reset the world.
     */
    NetLEReset(interPtr);

    /*
     * Now we are running.
     */

    statePtr->running = TRUE;
    ENABLE_INTR();
    return (SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEReset --
 *
 *	Reset the world.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All of the pointers in the netLEState structure are initialized.
 *
 *----------------------------------------------------------------------
 */
void
NetLEReset(interPtr)
    Net_Interface	*interPtr; /* Interface to reset. */
{
    NetLEState		*statePtr;
    unsigned		addr;
    int			i;

    statePtr = (NetLEState *) interPtr->interfaceData;
    /* 
     * Reset (and stop) the chip.
     */
    *statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *statePtr->regDataPortPtr = NET_LE_CSR0_STOP; 

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit(statePtr);
     NetLEXmitInit(statePtr);

    /*
     * Zero out the mode word.
     */
    *BUF_TO_ADDR(statePtr->initBlockPtr,NET_LE_INIT_MODE)=0;
    /*
     * Insert the ethernet address.
     */
    *BUF_TO_ADDR(statePtr->initBlockPtr, 
			  NET_LE_INIT_ETHER_ADDR) = 
		    (unsigned char)statePtr->etherAddress.byte1 |
		    ((unsigned char)statePtr->etherAddress.byte2 << 8);
    *BUF_TO_ADDR(statePtr->initBlockPtr, 
			  NET_LE_INIT_ETHER_ADDR + 2) = 
		    (unsigned char)statePtr->etherAddress.byte3 | 
		    ((unsigned char)statePtr->etherAddress.byte4 << 8);
    *BUF_TO_ADDR(statePtr->initBlockPtr, 
			NET_LE_INIT_ETHER_ADDR + 4) = 
		    (unsigned char)statePtr->etherAddress.byte5 | 
		    ((unsigned char)statePtr->etherAddress.byte6 << 8);
    /*
     * Reject all multicast addresses.
     */
    for (i = 0; i < 4; i++) {
	*BUF_TO_ADDR(statePtr->initBlockPtr, 
		      NET_LE_INIT_MULTI_MASK + (sizeof(short) * i)) = 0;
    }
    /*
     * We want to get boot multicasts.
     * These are addr ab-00-00-01-00-00 = hash bit 31?
     */
    *BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_MULTI_MASK) = 0x8000;

    /*
     * Set up the receive ring pointer.
     */
    addr = BUF_TO_CHIP_ADDR(statePtr->recvDescFirstPtr);
    *BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_RECV_LOW) = addr & 0xffff;
    *BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_RECV_HIGH) =
				(unsigned)((addr >> 16) & 0xff) |
		((unsigned)((NET_LE_NUM_RECV_BUFFERS_LOG2 << 5) & 0xe0) << 8);
    if (*BUF_TO_ADDR(statePtr->initBlockPtr,
			      NET_LE_INIT_RECV_LOW) & 0x07) {
	printf("netLE: Receive list not on QUADword boundary\n");
	return;
    }

    /*
     * Set up the transmit ring pointer.
     */
    addr = BUF_TO_CHIP_ADDR(statePtr->xmitDescFirstPtr);
    *BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_XMIT_LOW) = addr & 0xffff;
    *BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_XMIT_HIGH) =
				(unsigned)((addr >> 16) & 0xff) |
		((unsigned)((NET_LE_NUM_XMIT_BUFFERS_LOG2 << 5) & 0xe0) << 8);
    if (*BUF_TO_ADDR(statePtr->initBlockPtr, NET_LE_INIT_XMIT_LOW) & 0x07) {
	printf("netLE: Transmit list not on QUADword boundary\n");
	return;
    }

    /*
     * Clear the Bus master control register (csr3).
     */
    *statePtr->regAddrPortPtr = NET_LE_CSR3_ADDR;
    *statePtr->regDataPortPtr = 0;

    /*
     * Set the init block pointer address in csr1 and csr2
     */
    addr = BUF_TO_CHIP_ADDR(statePtr->initBlockPtr);
    *statePtr->regAddrPortPtr = NET_LE_CSR1_ADDR;
    *statePtr->regDataPortPtr = (short)(addr & 0xffff);

    *statePtr->regAddrPortPtr = NET_LE_CSR2_ADDR;
    *statePtr->regDataPortPtr = (short)((addr >> 16) & 0xff);

    /*
     * Tell the chip to initialize and wait for results.
     */
    *statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *statePtr->regDataPortPtr = NET_LE_CSR0_INIT | NET_LE_CSR0_INIT_DONE;

    {
	int	i;
	volatile unsigned short *csr0Ptr = statePtr->regDataPortPtr;


	for (i = 0; ((*csr0Ptr & NET_LE_CSR0_INIT_DONE) == 0); i++) {
	    if (i > 50000) {
		panic( "LE ethernet: Chip will not initialize.\n");
	    }
	}

	/*
	 * Ack the interrupt.
	 */
	 *csr0Ptr = NET_LE_CSR0_INIT_DONE;
    }

    /*
     * Start the chip and enable interrupts.
     */
    *statePtr->regDataPortPtr = 
		    (NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    printf("LE ethernet: Reinitialized chip.\n");
    interPtr->flags |= NET_IFLAGS_RUNNING;

}


/*
 *----------------------------------------------------------------------
 *
 * NetLERestart --
 *
 *	Reinitialize the LANCE Ethernet chip.
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
NetLERestart(interPtr)
    Net_Interface	*interPtr; 	/* Interface to restart. */
{
    NetLEState	*statePtr = (NetLEState *) interPtr->interfaceData;

    DISABLE_INTR();

    /*
     * Drop the current packet so the sender does't get hung.
     */
    NetLEXmitDrop(statePtr);

    /*
     * Reset the world.
     */
    NetLEReset(interPtr);

    /*
     * Restart transmission of packets.
     */
    NetLEXmitRestart(statePtr);

    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 * NetLEIntr --
 *
 *	Process an interrupt from the LANCE chip.
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
NetLEIntr(interPtr, polling)
    Net_Interface	*interPtr;	/* Network interface.*/
    Boolean	polling;		/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    volatile register	NetLEState	*statePtr;
    ReturnStatus		statusXmit, statusRecv;
    register unsigned short	csr0;
    Boolean			reset;

    statePtr = (NetLEState *) interPtr->interfaceData;
    *statePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    csr0 = *statePtr->regDataPortPtr;

    /*
     * Check for errors.
     */

    if (csr0 & NET_LE_CSR0_ERROR) {
	reset = TRUE;
	if (csr0 & NET_LE_CSR0_MISSED_PACKET) {
	    printf("LE ethernet: Missed a packet.\n");
	    /*
	     * Clear interrupt bit but don't reset controller.
	     */
	    *statePtr->regDataPortPtr = NET_LE_CSR0_MISSED_PACKET;
	    reset = FALSE;
	}
	if (csr0 & NET_LE_CSR0_COLLISION_ERROR) {
	    /*
	     * Late collision error appear to happen when the machine
	     * is disconnected from the transceiver. When this happens
	     * we will complain about Lost of Carrier so the late
	     * collision message is uncessary.
	     *
	     * printf("LE ethernet: Late collision.\n");
	     */
	    reset = FALSE;
	}
	/*
	 * Check for fatal errors.  Kill the machine if we start babbling 
	 * (sending oversize ethernet packets). 
	 */
	if (csr0 & NET_LE_CSR0_BABBLE) {
	    panic("LE ethernet: Transmit babble.\n");
	}
	if (csr0 & NET_LE_CSR0_MEMORY_ERROR) {
	    panic("LE ethernet: Memory Error.\n");
	}
	/*
	 * Clear the error the easy way, reinitialize everything.
	 */
	if (reset == TRUE) {
	    NetLERestart(interPtr);
	    return;
	}
    }

    statusRecv = statusXmit = SUCCESS;
    /*
     * Did we receive a packet.
     */
    if (csr0 & NET_LE_CSR0_RECV_INTR) {
	statusRecv = NetLERecvProcess(FALSE, statePtr);
    }
    /*
     * Did we transmit a packet.
     */
    if (csr0 & NET_LE_CSR0_XMIT_INTR) {
	statusXmit = NetLEXmitDone(statePtr);
    }
    /*
     * Did the chip magically initialize itself?
     */
    if (csr0 & NET_LE_CSR0_INIT_DONE) {
	printf( "LE ethernet: Chip initialized itself!!\n");
	/*
	 * Better initialize it the way we want it.
	 */
	statusRecv = FAILURE;
    }

    if (statusRecv != SUCCESS || statusXmit != SUCCESS) {
	NetLERestart(interPtr);
	return;
    }
    /*
     * If interrupts aren't enabled or there is no interrupt pending, then
     * what are we doing here?
     */

    if ( !(csr0 & NET_LE_CSR0_INTR_ENABLE) || !(csr0 & NET_LE_CSR0_INTR) ) {
	/*
	 * We could be polling; that's why we were here.
	 */
	if (!polling) {
	    printf("LE ethernet: Spurious interrupt CSR0 = <%x>\n", csr0);
	} 
    } 
    return;

}

unsigned	bufAddr = MACH_NETWORK_BUFFER_ADDR;

/*
 *----------------------------------------------------------------------
 *
 * NetLEMemAlloc -- 
 *
 *	Allocate memory from the buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Address
NetLEMemAlloc(numBytes, wordAlign)
    unsigned int	numBytes;
    Boolean		wordAlign;
{
    Address	retVal;

    if (wordAlign) {
	bufAddr &= ~0x3;
    } else {
	bufAddr &= ~0xf;
    }
    retVal = (Address)bufAddr;
    bufAddr += numBytes * 2;

    return(retVal);
}

#ifdef notdef

/*
 *----------------------------------------------------------------------
 *
 * BUF_TO_CHIP_ADDR -- 
 *
 *	Convert a memory buffer address to an address for the chip.  
 *
 *	NOTE: This used to a macro but was changed to C for debugging
 *	      purposes, hence the weird name.  Should probably change back
 *	      to a macro.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
unsigned
BUF_TO_CHIP_ADDR(addr)
    Address	addr;
{
    int	off, base, tmp;
    int	retAddr;

    off = (int)addr & NET_LE_DMA_CHIP_ADDR_MASK;
    base = (int)NET_LE_DMA_BUFFER_ADDR;
    tmp = off / 2;
    if (off == tmp * 2) {
	retAddr = base + tmp;
    } else {
	printf("BUF_TO_CHIP_ADDR: odd offset\n");
	retAddr = base + tmp + 1;
    }
    return(retAddr);
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * NetLEGetStats --
 *
 *	Return the statistics for the interface.
 *
 * Results:
 *	A pointer to the statistics structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetLEGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    NetLEState	*statePtr;
    statePtr = (NetLEState *) interPtr->interfaceData;
    DISABLE_INTR();
    statPtr->ether = statePtr->stats;
    ENABLE_INTR();
    return SUCCESS;
}

