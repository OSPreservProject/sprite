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
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "vmMach.h"
#include "mach.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "byte.h"

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
 * Setup state for probing the existence of the device
 */
static Mach_SetJumpState setJumpState;


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

    netLEState.running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    netLEState.regPortPtr = (NetLE_Reg *) ctrlAddr;

    if (Mach_SetJump(&setJumpState) == SUCCESS) {
	/*
	 * Poke the controller by setting the RAP.
	 */
	netLEState.regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    } else {
	/*
	 * Got a bus error.
	 */
	Mach_UnsetJump();
	ENABLE_INTR();
	return(FALSE);
    }
    Mach_UnsetJump();

    /*
     * Initialize the transmission list.  
     */

    netLEState.xmitList = &xmitListHdr;
    List_Init(netLEState.xmitList);

    netLEState.xmitFreeList = &xmitFreeListHdr;
    List_Init(netLEState.xmitFreeList);

    for (i = 0; i < NET_LE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) Vm_RawAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(netLEState.xmitFreeList));
    }

    /*
     * Get ethernet address out of the rom.  
     */

    Mach_GetEtherAddress(&netLEState.etherAddress);
    Sys_Printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", name, number,
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
    netLEState.initBlockPtr = (NetLEInitBlock *) 
			Vm_RawAlloc(sizeof(NetLEInitBlock));
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
    NetLEInitBlock	*initPtr;
    NetLEState		*netLEStatePtr = &netLEState;

    /* 
     * Reset (and stop) the chip.
     */
    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR0_ADDR;
    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR0_STOP; 

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit();
     NetLEXmitInit();

    /*
     *  Fill in the initialization block. Make eeverything zero unless 
     *  told otherwise.
     */

    Byte_Zero(sizeof(NetLEInitBlock), (Address) netLEState.initBlockPtr);
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
     * swap for us.
     */

    netLEStatePtr->regPortPtr->addrPort = NET_LE_CSR3_ADDR;
    netLEStatePtr->regPortPtr->dataPort = NET_LE_CSR3_BYTE_SWAP;

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
	unsigned short 	*csr0Ptr = &(netLEStatePtr->regPortPtr->dataPort);


	for (i = 0; ((*csr0Ptr & NET_LE_CSR0_INIT_DONE) == 0); i++) {
	    if (i > 50000) {
		Sys_Panic(SYS_FATAL, "LE ethernet: Chip will not initialized.\n");
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

    netLEStatePtr->regPortPtr->dataPort = 
		    (NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    Sys_Panic(SYS_WARNING,"LE ethernet: Reinitialized chip.\n");

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
     * Reset the world.
     */
    NetLEReset();

    /*
     * Restart transmission of packets.
     */
    NetLEXmitRestart();

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
NetLEIntr(polling)
    Boolean	polling;	/* TRUE if are being polled instead of
				 * processing an interrupt. */
{
    register	NetLEState	*netLEStatePtr;
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
	    Sys_Panic(SYS_WARNING,"LE ethernet: Missed a packet.\n");
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
	     * Sys_Panic(SYS_WARNING,"LE ethernet: Late collision.\n");
	     */
	    reset = FALSE;
	}
	/*
	 * Check for fatal errors.  Kill the machine if we start babbling 
	 * (sending oversize ethernet packets). 
	 */
	if (csr0 & NET_LE_CSR0_BABBLE) {
	    Sys_Panic(SYS_FATAL,"LE ethernet: Transmit babble.\n");
	}
	if (csr0 & NET_LE_CSR0_MEMORY_ERROR) {
	    Sys_Panic(SYS_FATAL,"LE ethernet: Memory Error.\n");
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
	Sys_Panic(SYS_WARNING, "LE ethernet: Chip initialized itself!!\n");
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
	    Sys_Printf("LE ethernet: Spurious interrupt CSR0 = <%x>\n", csr0);
	} 
    } 
    return;

}
