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
#ifdef sun4c
#include <devSCSIC90.h>
#endif


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
    int 		i;
    List_Links		*itemPtr;
    NetLEState		*statePtr;
    ReturnStatus	status;
    void		NetLETrace();

    assert(sizeof(NetLE_Reg) == NET_LE_REG_SIZE);
    assert(sizeof(NetLEModeReg) == 2);
    assert(sizeof(NetLERingPointer) == 4);
    assert(sizeof(NetLEInitBlock) == 24);
    assert(sizeof(NetLERecvMsgDesc) == 8);
    assert(sizeof(NetLEXmitMsgDesc) == 8);

    statePtr = (NetLEState *) malloc (sizeof(NetLEState));
    bzero((char *) statePtr, sizeof(NetLEState));
    MASTER_LOCK(&interPtr->mutex)
    statePtr->running = FALSE;
    status = NetLEMachInit(interPtr, statePtr);
    if (status != SUCCESS) {
	MASTER_UNLOCK(&interPtr->mutex);
	free((char *) statePtr);
	return status;
    }

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

    /*
     * Allocate the initialization block.
     */
    statePtr->initBlockPtr = 
		(NetLEInitBlock *) BufAlloc(statePtr, sizeof(NetLEInitBlock));
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
    statePtr->resetPending = FALSE;

    /*
     * Reset the world.
     */

    NetLEReset(interPtr);

    /*
     * Now we are running.
     */

    statePtr->running = TRUE;
    MASTER_UNLOCK(&interPtr->mutex);
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
    int				i;
    int				j;
    unsigned short		csr0;

    statePtr = (NetLEState *) interPtr->interfaceData;
    /*
     * If there isn't a reset pending already, and the chip is currently
     * transmitting then just set the pending flag.  With any luck
     * this mechanism will prevent the chip from being reset right in
     * the middle of a packet.
     */
    if (!(statePtr->resetPending) && (statePtr->transmitting)) {
	printf("Deferring reset.\n");
	statePtr->resetPending = TRUE;
	return;
    }
    statePtr->resetPending = FALSE;
    /* 
     * Reset (and stop) the chip.
     */
    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR0_ADDR);
    Mach_EmptyWriteBuffer();
    statePtr->regPortPtr->dataPort = NET_LE_CSR0_STOP;
    interPtr->flags &= ~NET_IFLAGS_RUNNING;
#ifdef sun4c
    Dev_ScsiResetDMA();
#endif

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit(statePtr);
     NetLEXmitInit(statePtr);

    /*
     *  Fill in the initialization block. Make everything zero unless 
     *  told otherwise.
     */

    bzero( (Address) statePtr->initBlockPtr, sizeof(NetLEInitBlock));
    initPtr = statePtr->initBlockPtr;

    /*
     * Insert the ethernet address.
     */

#ifndef ds5000
    {
	Net_EtherAddress	revAddr;
	revAddr.byte2 = statePtr->etherAddress.byte1;
	revAddr.byte1 = statePtr->etherAddress.byte2;
	revAddr.byte4 = statePtr->etherAddress.byte3;
	revAddr.byte3 = statePtr->etherAddress.byte4;
	revAddr.byte6 = statePtr->etherAddress.byte5;
	revAddr.byte5 = statePtr->etherAddress.byte6;
	bcopy((char *) &revAddr, (char *) &initPtr->etherAddress, 
	sizeof(statePtr->etherAddress));
    }
#else
    bcopy((char *) &statePtr->etherAddress, (char *) &initPtr->etherAddress, 
	sizeof(statePtr->etherAddress));
#endif

    /*
     * Reject all multicast addresses, except for those generated by bootp.
     * These are address ab-00-00-01-00-00 = hash bit 31, maybe?
     */

    initPtr->multiCastFilter[0] = 0x8000; /* Bit 31 apparently. */
    initPtr->multiCastFilter[1] = 0;
    initPtr->multiCastFilter[2] = 0;
    initPtr->multiCastFilter[3] = 0;

    /*
     * Set up the ring pointers.
     */

    NetBfShortSet(initPtr->recvRing, LogRingLength, 
		NET_LE_NUM_RECV_BUFFERS_LOG2);
    NetBfShortSet(initPtr->recvRing, RingAddrLow, 
		NET_LE_TO_CHIP_ADDR_LOW(statePtr->recvDescFirstPtr));
    NetBfShortSet(initPtr->recvRing, RingAddrHigh, 
		NET_LE_TO_CHIP_ADDR_HIGH(statePtr->recvDescFirstPtr));

    NetBfShortSet(initPtr->xmitRing, LogRingLength, 
		NET_LE_NUM_XMIT_BUFFERS_LOG2);
    NetBfShortSet(initPtr->xmitRing, RingAddrLow, 
		NET_LE_TO_CHIP_ADDR_LOW(statePtr->xmitDescFirstPtr));
    NetBfShortSet(initPtr->xmitRing, RingAddrHigh, 
		NET_LE_TO_CHIP_ADDR_HIGH(statePtr->xmitDescFirstPtr));

    /*
     * Set the the Bus master control register (csr3) to have chip byte
     * swap for us. he sparcStation appears to need active low and
     * byte control on.
     */

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR3_ADDR);
    Mach_EmptyWriteBuffer();
    statePtr->regPortPtr->dataPort = 0;
    Mach_EmptyWriteBuffer();
    statePtr->regPortPtr->dataPort = NET_LE_CSR3_VALUE;

    /*
     * Set the init block pointer address in csr1 and csr2
     */
    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR1_ADDR);
    Mach_EmptyWriteBuffer();
    NetBfShortSet(&statePtr->regPortPtr->dataPort, DataCSR1, 
	    NET_LE_TO_CHIP_ADDR_LOW(initPtr));
    Mach_EmptyWriteBuffer();

    NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, NET_LE_CSR2_ADDR);
    Mach_EmptyWriteBuffer();
    NetBfShortSet(&statePtr->regPortPtr->dataPort, DataCSR2, 
		NET_LE_TO_CHIP_ADDR_HIGH(initPtr));
    Mach_EmptyWriteBuffer();

    /*
     * Tell the chip to initialize and wait for results.
     */
    csr0 = 0;
    for (j = 0; (j < 100) && ((csr0 & NET_LE_CSR0_INIT_DONE) == 0); j++) {
	NetBfShortSet(statePtr->regPortPtr->addrPort, AddrPort, 
		NET_LE_CSR0_ADDR);
	Mach_EmptyWriteBuffer();
	statePtr->regPortPtr->dataPort = NET_LE_CSR0_INIT;
	Mach_EmptyWriteBuffer();


	for (i = 0; i < 10000; i++) {
	    csr0 = statePtr->regPortPtr->dataPort;
	    if (csr0 & NET_LE_CSR0_INIT_DONE) {
		break;
	    }
	}
	if (csr0 & NET_LE_CSR0_INIT_DONE) {
	    break;
	}
    }
    if (!(csr0 & NET_LE_CSR0_INIT_DONE)) {
	panic("LE ethernet: Chip will not initialize, csr0 = 0x%x\n", csr0);
    }
    MACH_DELAY(100);

    /*
     * Ack the interrupt.
     */
    statePtr->regPortPtr->dataPort = NET_LE_CSR0_INIT_DONE;

    /*
     * Start the chip and enable interrupts.
     */
    statePtr->regPortPtr->dataPort = 
	(NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    printf("LE ethernet: Reinitialized chip.\n");
    statePtr->numResets++;
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

    MASTER_LOCK(&interPtr->mutex);

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

    MASTER_UNLOCK(&interPtr->mutex);
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
    Mach_EmptyWriteBuffer();
    csr0 = statePtr->regPortPtr->dataPort;
    if (netDebug) {
	printf("NetLEIntr: %s, csr0 = 0x%x\n", (polling == TRUE) ? "polling" : 
	    "not polling", csr0);
    }
    if (csr0 & NET_LE_CSR0_STOP) {
	printf("NetLEIntr: chip is stopped.\n");
	NetLERestart(interPtr);
	return;
    }

    csr0 &= ~NET_LE_CSR0_INTR_ENABLE;
    statePtr->regPortPtr->dataPort = csr0;
    statePtr->regPortPtr->dataPort = NET_LE_CSR0_INTR_ENABLE;
    if ( !((csr0 & NET_LE_CSR0_ERROR) || (csr0 & NET_LE_CSR0_INTR)) ) {
	/*
	 * We could be polling; that's why we were here.
	 */
	if (!polling) {
	    printf("LE ethernet: Spurious interrupt CSR0 = <%x>\n", csr0);
	    NetLERestart(interPtr);
	} 
	return;
    } 

    /*
     * Check for errors.
     */

    if (csr0 & NET_LE_CSR0_ERROR) {
	reset = TRUE;
	if (csr0 & NET_LE_CSR0_MISSED_PACKET) {
	    printf("LE ethernet: Missed a packet.\n");
	    /*
	     * Don't reset controller.
	     */
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
	    printf(
	"statePtr: 0x%x, regPortPtr = 0x%x, dataPort = 0x%x, csr0: 0x%x\n", 
		statePtr, statePtr->regPortPtr, statePtr->regPortPtr->dataPort, 
		csr0);
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
    MASTER_LOCK(&interPtr->mutex);
    statPtr->ether = statePtr->stats;
    MASTER_UNLOCK(&interPtr->mutex);
    return SUCCESS;
}

