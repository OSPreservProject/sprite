/* 
 * netLE.c --
 *
 *	The main routines for the device driver for the AMD 7990 Ethernet 
 *	Controller.
 *
 *
 * TODO: Watch dogs to make sure that the chip does not get stuck.  Rumor has
 *	 it that because of bugs in the chip it can get stuck at any time for
 *	 no particular reason.
 *
 * Copyright 1988 Regents of the University of Californiaf
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
#include <sys.h>
#include <list.h>
#include <netInt.h>
#include <netLEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <machMon.h>
#include <dbg.h>
#include <assert.h>


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
    Address 	ctrlAddr;	/* Kernel virtual address of controller. */
    int 	i;
    List_Links	*itemPtr;
    NetLEState	*statePtr;

    assert(sizeof(NetLE_Reg) == 4);
    assert(sizeof(NetLEModeReg) == 2);
    assert(sizeof(NetLERingPointer) == 4);
    assert(sizeof(NetLEInitBlock) == 24);
    assert(sizeof(NetLERecvMsgDesc) == 8);
    assert(sizeof(NetLEXmitMsgDesc) == 8);

    DISABLE_INTR();

    ctrlAddr = interPtr->ctrlAddr;
    /*
     * If the address is physical (not in kernel's virtual address space)
     * then we have to map it in.
     */
    if (interPtr->virtual == FALSE) {
	ctrlAddr = (char *) VmMach_MapInDevice(ctrlAddr, 1);
    }
    statePtr = (NetLEState *) malloc (sizeof(NetLEState));
    bzero((char *) statePtr, sizeof(NetLEState));
    statePtr->running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    statePtr->regPortPtr = (volatile NetLE_Reg *) ctrlAddr;
    {
	/*
	 * Poke the controller by setting the RAP.
	 */
	short value = NET_LE_CSR0_ADDR;
	ReturnStatus status;
	status = Mach_Probe(sizeof(short), (char *) &value, 
			  (char *) (((short *)(statePtr->regPortPtr)) + 1));
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
	itemPtr = (List_Links *) VmMach_NetMemAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->xmitFreeList));
    }

    /*
     * Get ethernet address out of the rom.  
     */

    Mach_GetEtherAddress(&statePtr->etherAddress);
    printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", 
	      interPtr->name, interPtr->number,
	      statePtr->etherAddress.byte1 & 0xff,
	      statePtr->etherAddress.byte2 & 0xff,
	      statePtr->etherAddress.byte3 & 0xff,
	      statePtr->etherAddress.byte4 & 0xff,
	      statePtr->etherAddress.byte5 & 0xff,
	      statePtr->etherAddress.byte6 & 0xff);

    /*
     * Generate a byte-swapped copy of it.
     */
    statePtr->etherAddressBackward.byte2 = statePtr->etherAddress.byte1;
    statePtr->etherAddressBackward.byte1 = statePtr->etherAddress.byte2;
    statePtr->etherAddressBackward.byte4 = statePtr->etherAddress.byte3;
    statePtr->etherAddressBackward.byte3 = statePtr->etherAddress.byte4;
    statePtr->etherAddressBackward.byte6 = statePtr->etherAddress.byte5;
    statePtr->etherAddressBackward.byte5 = statePtr->etherAddress.byte6;

    /*
     * Allocate the initialization block.
     */
    statePtr->initBlockPtr = 
		    (NetLEInitBlock *)VmMach_NetMemAlloc(sizeof(NetLEInitBlock));
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
 *	Reset the interface.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All of the pointers in the interface structure are initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetLEReset(interPtr)
    Net_Interface	*interPtr; /* Interface to reset. */
{
    volatile NetLEInitBlock *initPtr;
    NetLEState		    *statePtr;

    statePtr = (NetLEState *) interPtr->interfaceData;
    /* 
     * Reset (and stop) the chip.
     */
    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR0_ADDR);
    statePtr->regPortPtr->dataPort = NET_LE_CSR0_STOP; 
#ifdef sun4c
    Dev_ScsiResetDMA();
#endif

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit(statePtr);
     NetLEXmitInit(statePtr);

    /*
     *  Fill in the initialization block. Make eeverything zero unless 
     *  told otherwise.
     */

    bzero( (Address) statePtr->initBlockPtr, sizeof(NetLEInitBlock));
    initPtr = statePtr->initBlockPtr;
    /*
     * Insert the byte swapped ethernet address.
     */

    initPtr->etherAddress = statePtr->etherAddressBackward;

    /*
     * Reject all multicast addresses.
     */

    initPtr->multiCastFilter[0] = 0;
    initPtr->multiCastFilter[1] = 0;
    initPtr->multiCastFilter[2] = 0;
    initPtr->multiCastFilter[3] = 0;

    /*
     * Set up the ring pointers.
     */

    NetBfWordSet(initPtr->recvRing, LogRingLength, 
		NET_LE_NUM_RECV_BUFFERS_LOG2);
    NetBfWordSet(initPtr->recvRing, RingAddrLow, 
		NET_LE_SUN_TO_CHIP_ADDR_LOW(statePtr->recvDescFirstPtr));
    NetBfWordSet(initPtr->recvRing, RingAddrHigh, 
		NET_LE_SUN_TO_CHIP_ADDR_HIGH(statePtr->recvDescFirstPtr));

    NetBfWordSet(initPtr->xmitRing, LogRingLength, 
		NET_LE_NUM_XMIT_BUFFERS_LOG2);
    NetBfWordSet(initPtr->xmitRing, RingAddrLow, 
		NET_LE_SUN_TO_CHIP_ADDR_LOW(statePtr->xmitDescFirstPtr));
    NetBfWordSet(initPtr->xmitRing, RingAddrHigh, 
		NET_LE_SUN_TO_CHIP_ADDR_HIGH(statePtr->xmitDescFirstPtr));

    /*
     * Set the the Bus master control register (csr3) to have chip byte
     * swap for us. he sparcStation appears to need active low and
     * byte control on.
     */

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR3_ADDR);
#ifdef sun4c
     statePtr->regPortPtr->dataPort = NET_LE_CSR3_BYTE_SWAP |
					  NET_LE_CSR3_ALE_CONTROL |
					  NET_LE_CSR3_BYTE_CONTROL;
#else
    statePtr->regPortPtr->dataPort = NET_LE_CSR3_BYTE_SWAP;
#endif
    /*
     * Set the init block pointer address in csr1 and csr2
     */
    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR1_ADDR);
    statePtr->regPortPtr->dataPort = 
			NET_LE_SUN_TO_CHIP_ADDR_LOW(initPtr);

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR2_ADDR);
    statePtr->regPortPtr->dataPort = 
			NET_LE_SUN_TO_CHIP_ADDR_HIGH(initPtr);

    /*
     * Tell the chip to initialize and wait for results.
     */

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR0_ADDR);
    statePtr->regPortPtr->dataPort = NET_LE_CSR0_INIT;

    {
	int	i;
	unsigned volatile short	*csr0Ptr = 
	    &(statePtr->regPortPtr->dataPort);

	for (i = 0; ((*csr0Ptr & NET_LE_CSR0_INIT_DONE) == 0); i++) {
	    if (i > 5000) {
		panic("LE ethernet: Chip will not initialize, csr = 0x%x \n",
			*csr0Ptr);
	    }
	    MACH_DELAY(100);
	}

	/*
	 * Ack the interrupt.
	 */

	 *csr0Ptr = NET_LE_CSR0_INIT_DONE;
    }

    /*
     * Start the chip and enable interrupts.
     */

    statePtr->regPortPtr->dataPort = 
		    (NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    printf("LE ethernet: Reinitialized chip.\n");
    interPtr->flags |= NET_IFLAGS_RUNNING;
    return;
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
    return;
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
    Net_Interface	*interPtr;	/* Interface to process. */
    Boolean		polling;	/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    register	NetLEState	*statePtr;
    ReturnStatus		statusXmit, statusRecv;
    unsigned 	short		csr0;
    Boolean			reset;

    statePtr = (NetLEState *) interPtr->interfaceData;

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR0_ADDR);
    csr0 = statePtr->regPortPtr->dataPort;
    if (netDebug) {
	printf("NetLEIntr: %s\n", (polling == TRUE) ? "polling" : 
	    "not polling");
    }

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
	    statePtr->regPortPtr->dataPort = NET_LE_CSR0_MISSED_PACKET;
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
	    NetLEReset(interPtr);
	    panic("LE ethernet: Transmit babble\n");
	}
	if (csr0 & NET_LE_CSR0_MEMORY_ERROR) {
#ifdef sun4c
	    printf("statePtr: 0x%x, regPortPtr = 0x%x, dataPort = 0x%x, csr0: 0x%x\n", statePtr, statePtr->regPortPtr, statePtr->regPortPtr->dataPort, csr0);
#endif
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


