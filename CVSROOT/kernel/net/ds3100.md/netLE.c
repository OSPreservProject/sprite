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

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "vmMach.h"
#include "mach.h"
#include "netLEInt.h"
#include "net.h"
#include "netInt.h"
#include "machAddrs.h"

/*
 * Define global variables.
 */

NetLEState	netLEState;

/*
 * Define the header that the transmit list will point to. 
 */

static 	List_Links	xmitListHdr;
static 	List_Links	xmitFreeListHdr;

Address	NetLEMemAlloc();


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
/*ARGSUSED*/
Boolean
NetLEInit(name, number, ctrlAddr)
    char 	*name;		/* Sprite name for controller. */	
    int 	number;		/* Unit number of device (not used). */
    unsigned int ctrlAddr;	/* Kernel virtual address of controller. */
{
    int 		i;
    List_Links		*itemPtr;
    volatile unsigned	*romPtr;

    DISABLE_INTR();

    netLEState.running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.
     */
    netLEState.regAddrPortPtr =
		    (unsigned short *)(MACH_NETWORK_INTERFACE_ADDR + 4);
    netLEState.regDataPortPtr = (unsigned short *)MACH_NETWORK_INTERFACE_ADDR;

    /*
     * Initialize the transmission list.  
     */
    netLEState.xmitList = &xmitListHdr;
    List_Init(netLEState.xmitList);

    netLEState.xmitFreeList = &xmitFreeListHdr;
    List_Init(netLEState.xmitFreeList);

    for (i = 0; i < NET_LE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) Vm_BootAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(netLEState.xmitFreeList));
    }

    /*
     * Get ethernet address out of the clocks RAM.
     */
    romPtr = (unsigned *)0xBD000000;
    netLEState.etherAddress.byte1 = (romPtr[0] >> 8) & 0xff;
    netLEState.etherAddress.byte2 = (romPtr[1] >> 8) & 0xff;
    netLEState.etherAddress.byte3 = (romPtr[2] >> 8) & 0xff;
    netLEState.etherAddress.byte4 = (romPtr[3] >> 8) & 0xff;
    netLEState.etherAddress.byte5 = (romPtr[4] >> 8) & 0xff;
    netLEState.etherAddress.byte6 = (romPtr[5] >> 8) & 0xff;
    printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", name, number,
	      netLEState.etherAddress.byte1 & 0xff,
	      netLEState.etherAddress.byte2 & 0xff,
	      netLEState.etherAddress.byte3 & 0xff,
	      netLEState.etherAddress.byte4 & 0xff,
	      netLEState.etherAddress.byte5 & 0xff,
	      netLEState.etherAddress.byte6 & 0xff);

    /*
     * Allocate the initialization block.
     */
    netLEState.initBlockPtr = NetLEMemAlloc(NET_LE_INIT_SIZE, TRUE);
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
    NetLEState		*netLEStatePtr = &netLEState;
    unsigned		addr;
    int			i;

    /* 
     * Reset (and stop) the chip.
     */
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *netLEStatePtr->regDataPortPtr = NET_LE_CSR0_STOP; 

    /*
     * Set up the receive and transmit rings. 
     */
     NetLERecvInit();
     NetLEXmitInit();

    /*
     * Zero out the mode word.
     */
    *BUF_TO_ADDR(netLEState.initBlockPtr,NET_LE_INIT_MODE)=0;
    /*
     * Insert the ethernet address.
     */
    *BUF_TO_ADDR(netLEState.initBlockPtr, 
			  NET_LE_INIT_ETHER_ADDR) = 
		    (unsigned char)netLEState.etherAddress.byte1 |
		    ((unsigned char)netLEState.etherAddress.byte2 << 8);
    *BUF_TO_ADDR(netLEState.initBlockPtr, 
			  NET_LE_INIT_ETHER_ADDR + 2) = 
		    (unsigned char)netLEState.etherAddress.byte3 | 
		    ((unsigned char)netLEState.etherAddress.byte4 << 8);
    *BUF_TO_ADDR(netLEState.initBlockPtr, 
			NET_LE_INIT_ETHER_ADDR + 4) = 
		    (unsigned char)netLEState.etherAddress.byte5 | 
		    ((unsigned char)netLEState.etherAddress.byte6 << 8);
    /*
     * Reject all multicast addresses.
     */
    for (i = 0; i < 4; i++) {
	*BUF_TO_ADDR(netLEState.initBlockPtr, 
		      NET_LE_INIT_MULTI_MASK + (sizeof(short) * i)) = 0;
    }

    /*
     * Set up the receive ring pointer.
     */
    addr = BUF_TO_CHIP_ADDR(netLEState.recvDescFirstPtr);
    *BUF_TO_ADDR(netLEState.initBlockPtr, NET_LE_INIT_RECV_LOW) = addr & 0xffff;
    *BUF_TO_ADDR(netLEState.initBlockPtr, NET_LE_INIT_RECV_HIGH) =
				(unsigned)((addr >> 16) & 0xff) |
		((unsigned)((NET_LE_NUM_RECV_BUFFERS_LOG2 << 5) & 0xe0) << 8);
    if (*BUF_TO_ADDR(netLEState.initBlockPtr,
			      NET_LE_INIT_RECV_LOW) & 0x07) {
	printf("netLE: Receive list not on QUADword boundary\n");
	return;
    }

    /*
     * Set up the transmit ring pointer.
     */
    addr = BUF_TO_CHIP_ADDR(netLEState.xmitDescFirstPtr);
    *BUF_TO_ADDR(netLEState.initBlockPtr, NET_LE_INIT_XMIT_LOW) = addr & 0xffff;
    *BUF_TO_ADDR(netLEState.initBlockPtr, NET_LE_INIT_XMIT_HIGH) =
				(unsigned)((addr >> 16) & 0xff) |
		((unsigned)((NET_LE_NUM_XMIT_BUFFERS_LOG2 << 5) & 0xe0) << 8);
    if (*BUF_TO_ADDR(netLEState.initBlockPtr, NET_LE_INIT_XMIT_LOW) & 0x07) {
	printf("netLE: Transmit list not on QUADword boundary\n");
	return;
    }

    /*
     * Clear the Bus master control register (csr3).
     */
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR3_ADDR;
    *netLEStatePtr->regDataPortPtr = 0;

    /*
     * Set the init block pointer address in csr1 and csr2
     */
    addr = BUF_TO_CHIP_ADDR(netLEState.initBlockPtr);
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR1_ADDR;
    *netLEStatePtr->regDataPortPtr = (short)(addr & 0xffff);

    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR2_ADDR;
    *netLEStatePtr->regDataPortPtr = (short)((addr >> 16) & 0xff);

    /*
     * Tell the chip to initialize and wait for results.
     */
    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    *netLEStatePtr->regDataPortPtr = NET_LE_CSR0_INIT | NET_LE_CSR0_INIT_DONE;

    {
	int	i;
	volatile unsigned short *csr0Ptr = netLEStatePtr->regDataPortPtr;


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
    *netLEStatePtr->regDataPortPtr = 
		    (NET_LE_CSR0_START | NET_LE_CSR0_INTR_ENABLE);

    printf("LE ethernet: Reinitialized chip.\n");

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
    register NetLEState		*netLEStatePtr;
    ReturnStatus		statusXmit, statusRecv;
    register unsigned short	csr0;
    Boolean			reset;

    netLEStatePtr = &netLEState;

    *netLEStatePtr->regAddrPortPtr = NET_LE_CSR0_ADDR;
    csr0 = *netLEStatePtr->regDataPortPtr;

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
	    *netLEStatePtr->regDataPortPtr = NET_LE_CSR0_MISSED_PACKET;
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
