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

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "vmMach.h"
#include "mach.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"

/*
 * Define global variables.
 */

NetLEState	netLEState;

/*
 * Define the header that the transmit list will point to. 
 */

static 	List_Links	xmitListHdr;
static 	List_Links	xmitFreeListHdr;


/*
 *----------------------------------------------------------------------
 *
 * NetLEInit --
 *
 *	Initialize the LANCE AMD 7990 Ethernet chip.
 *
 * Results:
 *	TRUE if the LANCE controller was found and initialized,
 *	FALSE otherwise.
 *
 * Side effects:
 *	Initializes the netEtherFuncs record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */

Boolean
NetLEInit(name, number, ctrlAddr)
    char 	*name;		/* Sprite name for controller. */	
    int 	number;		/* Unit number of device (not used). */
    unsigned int ctrlAddr;	/* Kernel virtual address of controller. */
{
    int 	i;
    List_Links	*itemPtr;

    DISABLE_INTR();

#ifdef sun4c
    Mach_SetHandler(5, Net_Intr, (ClientData) 0);
#else
    Mach_SetHandler(27, Net_Intr, (ClientData) 0);
#endif
    netLEState.running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    netLEState.regPortPtr = (volatile NetLE_Reg *) ctrlAddr;

    {
	/*
	 * Poke the controller by setting the RAP.
	 */
	short value = NET_LE_CSR0_ADDR;
	ReturnStatus status;
	status = Mach_Probe(sizeof(short), (char *) &value, 
			  (char *) (((short *)(netLEState.regPortPtr)) + 1));
	if (status != SUCCESS) {
	    /*
	     * Got a bus error.
	     */
	    ENABLE_INTR();
	    return(FALSE);
	}
    } 
    /*
     * Initialize the transmission list.  
     */

    netLEState.xmitList = &xmitListHdr;
    List_Init(netLEState.xmitList);

    netLEState.xmitFreeList = &xmitFreeListHdr;
    List_Init(netLEState.xmitFreeList);

    for (i = 0; i < NET_LE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) VmMach_NetMemAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(netLEState.xmitFreeList));
    }

    /*
     * Get ethernet address out of the rom.  
     */

    Mach_GetEtherAddress(&netLEState.etherAddress);
    printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", name, number,
	      netLEState.etherAddress.byte1 & 0xff,
	      netLEState.etherAddress.byte2 & 0xff,
	      netLEState.etherAddress.byte3 & 0xff,
	      netLEState.etherAddress.byte4 & 0xff,
	      netLEState.etherAddress.byte5 & 0xff,
	      netLEState.etherAddress.byte6 & 0xff);

    /*
     * Generate a byte-swapped copy of it.
     */
    netLEState.etherAddressBackward.byte2 = netLEState.etherAddress.byte1;
    netLEState.etherAddressBackward.byte1 = netLEState.etherAddress.byte2;
    netLEState.etherAddressBackward.byte4 = netLEState.etherAddress.byte3;
    netLEState.etherAddressBackward.byte3 = netLEState.etherAddress.byte4;
    netLEState.etherAddressBackward.byte6 = netLEState.etherAddress.byte5;
    netLEState.etherAddressBackward.byte5 = netLEState.etherAddress.byte6;

    /*
     * Allocate the initialization block.
     */
    netLEState.initBlockPtr = 
		    (NetLEInitBlock *)VmMach_NetMemAlloc(sizeof(NetLEInitBlock));
    /*
     * Reset the world.
     */

    NetLEReset();

    /*
     * Now we are running.
     */

    netLEState.running = TRUE;
    netEtherFuncs.init	 = NetLEInit;
    netEtherFuncs.output = NetLEOutput;
    netEtherFuncs.intr   = NetLEIntr;
    netEtherFuncs.reset  = NetLERestart;

    ENABLE_INTR();
    return (TRUE);
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
NetLEReset()
{
    volatile NetLEInitBlock *initPtr;
    NetLEState		    *netLEStatePtr = &netLEState;

    /* 
     * Reset (and stop) the chip.
     */
    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR0_STOP; 
#ifdef sun4c
    {
	register volatile int *dmaReg = ((int *) 0xffd14000);
	*dmaReg = 0x80;
	MACH_DELAY(200);
	*dmaReg = *dmaReg & ~(0x80);
	MACH_DELAY(200);
	*dmaReg = 0x10;
	MACH_DELAY(200);
	printf("DMA reg = 0x%x\n", *dmaReg);
    }
#endif

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit();
     NetLEXmitInit();

    /*
     *  Fill in the initialization block. Make eeverything zero unless 
     *  told otherwise.
     */

    bzero( (Address) netLEState.initBlockPtr, sizeof(NetLEInitBlock));
    initPtr = netLEState.initBlockPtr;
    /*
     * Insert the byte swapped ethernet address.
     */

    initPtr->etherAddress = netLEState.etherAddressBackward;

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

    initPtr->recvRing.logRingLength = NET_LE_NUM_RECV_BUFFERS_LOG2;
    initPtr->recvRing.ringAddrLow = 
		NET_LE_SUN_TO_CHIP_ADDR_LOW(netLEState.recvDescFirstPtr);
    initPtr->recvRing.ringAddrHigh = 
		NET_LE_SUN_TO_CHIP_ADDR_HIGH(netLEState.recvDescFirstPtr);

    initPtr->xmitRing.logRingLength = NET_LE_NUM_XMIT_BUFFERS_LOG2;
    initPtr->xmitRing.ringAddrLow = 
		NET_LE_SUN_TO_CHIP_ADDR_LOW(netLEState.xmitDescFirstPtr);
    initPtr->xmitRing.ringAddrHigh = 
		NET_LE_SUN_TO_CHIP_ADDR_HIGH(netLEState.xmitDescFirstPtr);

    /*
     * Set the the Bus master control register (csr3) to have chip byte
     * swap for us. he sparcStation appears to need active low and
     * byte control on.
     */

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR3_ADDR;
#ifdef sun4c
     netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR3_BYTE_SWAP |
					  NET_LE_CSR3_ALE_CONTROL |
					  NET_LE_CSR3_BYTE_CONTROL;
#else
    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR3_BYTE_SWAP;
#endif
    /*
     * Set the init block pointer address in csr1 and csr2
     */
    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR1_ADDR;
    netLEStatePtr->regPortPtr->dataPort = 
			NET_LE_SUN_TO_CHIP_ADDR_LOW(initPtr);

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR2_ADDR;
    netLEStatePtr->regPortPtr->dataPort = 
			NET_LE_SUN_TO_CHIP_ADDR_HIGH(initPtr);

    /*
     * Tell the chip to initialize and wait for results.
     */

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR0_INIT;

    {
	int	i;
	unsigned volatile short	*csr0Ptr = 
	    &(netLEStatePtr->regPortPtr->dataPort);

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

    netLEStatePtr->regPortPtr->dataPort = 
		    (NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    printf("LE ethernet: Reinitialized chip.\n");
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
NetLERestart()
{

    DISABLE_INTR();

    /*
     * Drop the current packet so the sender does't get hung.
     */
    NetLEXmitDrop();

    /*
     * Reset the world.
     */
    NetLEReset();

    /*
     * Restart transmission of packets.
     */
    NetLEXmitRestart();

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
NetLEIntr(polling)
    Boolean	polling;	/* TRUE if are being polled instead of
				 * processing an interrupt. */
{
    volatile register	NetLEState	*netLEStatePtr;
    ReturnStatus		statusXmit, statusRecv;
    unsigned 	short		csr0;
    Boolean			reset;

    netLEStatePtr = &netLEState;

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    csr0 = netLEStatePtr->regPortPtr->dataPort;

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
	    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR0_MISSED_PACKET;
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
#ifdef sun4c
	    printf("netLEStatePtr: 0x%x, regPortPtr = 0x%x, dataPort = 0x%x, csr0: 0x%x\n", netLEStatePtr, netLEStatePtr->regPortPtr, netLEStatePtr->regPortPtr->dataPort, csr0);
#endif
	    panic("LE ethernet: Memory Error.\n");
	}
	/*
	 * Clear the error the easy way, reinitialize everything.
	 */
	if (reset == TRUE) {
	    NetLERestart();
	    return;
	}
    }

    statusRecv = statusXmit = SUCCESS;
    /*
     * Did we receive a packet.
     */
    if (csr0 & NET_LE_CSR0_RECV_INTR) {
	statusRecv = NetLERecvProcess(FALSE);
    }
    /*
     * Did we transmit a packet.
     */
    if (csr0 & NET_LE_CSR0_XMIT_INTR) {
	statusXmit = NetLEXmitDone();
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
	NetLERestart();
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
