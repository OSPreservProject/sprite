/* 
 * netIE.c --
 *
 *	The main routines for the device driver for the Intel 82586 Ethernet 
 *	Controller.
 *
 *      This chip has several pecularities that have to be compensated for
 *      when using it.  First of all no element of the scatter-gather
 *      array which is passed into the output routine can be below a
 *      minimum size.  The minimum size is defined in the file netIEXmit.c
 *      and is called MIN_XMIT_BUFFER_SIZE.  Secondly none of the
 *      scatter-gather elements can begin on an odd boundary.  If they do,
 *      the chip drops the low order bit and sends one more byte than was
 *      specified.  There are warnings printed in each of these cases.
 *
 * TODO: Watch dogs to make sure that the chip does not get stuck.  Rumor has
 *	 it that because of bugs in the chip it can get stuck at any time for
 *	 no particular reason.
 *
 * Copyright 1985, 1988 Regents of the University of California
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
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"

/*
 * Define global variables.
 */

NetIEState	netIEState;
Address		netIERecvBuffers[NET_IE_NUM_RECV_BUFFERS];

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
 * NetIEInit --
 *
 *	Initialize the Intel Ethernet chip.
 *
 * Results:
 *	TRUE if the Intel controller was found and initialized,
 *	FALSE otherwise.
 *
 * Side effects:
 *	Initializes the netEtherFuncs record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */

Boolean
NetIEInit(name, number, ctrlAddr)
    char 	*name;		/* Sprite name for controller. */	
    int 	number;		/* Unit number of device (not used). */
    unsigned int ctrlAddr;	/* Kernel virtual address of controller. */
{
    int 	i;
    List_Links	*itemPtr;
    Address	(*allocFunc)();

#ifdef sun2
    allocFunc = Vm_RawAlloc;
#endif
#ifdef sun3
    allocFunc = VmMach_NetMemAlloc;
#endif

    DISABLE_INTR();

    netIEState.running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    netIEState.controlReg = (NetIEControlRegister *) ctrlAddr;

    if (Mach_SetJump(&setJumpState) == SUCCESS) {
	/*
	 * Poke the controller by resetting it.
	 */
	*(char *)netIEState.controlReg = 0;
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

    netIEState.xmitList = &xmitListHdr;
    List_Init(netIEState.xmitList);

    netIEState.xmitFreeList = &xmitFreeListHdr;
    List_Init(netIEState.xmitFreeList);

    netIEXmitFiller = (char *)allocFunc(NET_ETHER_MIN_BYTES);
    netIEXmitTempBuffer = (char *)allocFunc(NET_ETHER_MAX_BYTES + 2);

    for (i = 0; i < NET_IE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) allocFunc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(netIEState.xmitFreeList));
    }

    /*
     * Get ethernet address out of the rom.  It is stored in a normal order
     * even though the chip is wired backwards because fortunately the chip
     * stores the ethernet address backwards from how we store it.  Therefore
     * two backwards makes one forwards, right?
     */

    Mach_GetEtherAddress(&netIEState.etherAddress);
    printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", name, number,
	      netIEState.etherAddress.byte1 & 0xff,
	      netIEState.etherAddress.byte2 & 0xff,
	      netIEState.etherAddress.byte3 & 0xff,
	      netIEState.etherAddress.byte4 & 0xff,
	      netIEState.etherAddress.byte5 & 0xff,
	      netIEState.etherAddress.byte6 & 0xff);
    /*
     * Allocate space for the System Configuration Pointer.
     */

    VmMach_MapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));
    netIEState.sysConfPtr = (NetIESysConfPtr *) NET_IE_SYS_CONF_PTR_ADDR;

    /*
     * Allocate space for all of the receive buffers.
     */

    for (i = 0; i < NET_IE_NUM_RECV_BUFFERS; i++) {
	netIERecvBuffers[i] = allocFunc(NET_IE_RECV_BUFFER_SIZE);
    }

    /*
     * Reset the world.
     */

    NetIEReset();

    /*
     * Unmap the extra page.
     */

    VmMach_UnmapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    /*
     * Now we are running.
     */

    netIEState.running = TRUE;
    netEtherFuncs.init	 = NetIEInit;
    netEtherFuncs.output = NetIEOutput;
    netEtherFuncs.intr   = NetIEIntr;
    netEtherFuncs.reset  = NetIERestart;

    ENABLE_INTR();
    return (TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEDefaultConfig --
 *
 *	Set up the default configuration for the intel chip as specified
 *	in the intel manual.  The only difference from the Intel manual
 *	is the fifo length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The global command block is modified.
 *
 *----------------------------------------------------------------------
 */

void
NetIEDefaultConfig()
{
    NetIEConfigureCB	*confCBPtr;

    confCBPtr = (NetIEConfigureCB *) netIEState.cmdBlockPtr;
    bzero((Address) confCBPtr, sizeof(NetIEConfigureCB));
    confCBPtr->cmdBlock.cmdNumber = NET_IE_CONFIG;
    confCBPtr->byteCount = 12;
    confCBPtr->fifoLimit = 12;
    confCBPtr->preamble = 2;
    confCBPtr->addrLen = 6;
    confCBPtr->atLoc = 0;
    confCBPtr->interFrameSpace = 96;
    confCBPtr->slotTimeHigh = 512 >> 8;
    confCBPtr->minFrameLength = 64;
    confCBPtr->numRetries = 15;

/*
    confCBPtr->intLoopback = 1;
*/

    NetIEExecCommand((NetIECommandBlock *) confCBPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEReset --
 *
 *	Reset the world.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All of the pointers in the netIEState structure are initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetIEReset()
{
    NetIEIASetupCB	*addressCommandPtr;
    NetIECommandBlock	*diagCmdPtr;

    /*
     * Nil out all pointers.
     */

    netIEState.intSysConfPtr = (NetIEIntSysConfPtr *) NIL;
    netIEState.scbPtr = (NetIESCB *) NIL;
    netIEState.recvFrDscHeadPtr = (NetIERecvFrameDesc *) NIL;
    netIEState.recvFrDscTailPtr = (NetIERecvFrameDesc *) NIL;
    netIEState.recvBufDscHeadPtr = (NetIERecvBufDesc *) NIL;
    netIEState.recvBufDscTailPtr = (NetIERecvBufDesc *) NIL;

    /* 
     * Reset the chip.
     */

    NET_IE_CHIP_RESET;

    /*
     * Initialize memory.
     */

    NetIEMemInit();

    /*
     * Allocate the system intermediate configuration pointer and the 
     * system control block.
     */

    netIEState.intSysConfPtr = (NetIEIntSysConfPtr *) NetIEMemAlloc();
    if (netIEState.intSysConfPtr == (NetIEIntSysConfPtr *) NIL) {
	panic("Intel: No memory for the scp.\n");
    }

    netIEState.scbPtr = (NetIESCB *) NetIEMemAlloc();
    if (netIEState.scbPtr == (NetIESCB *) NIL) {
	panic("Intel: No memory for the scb.\n");
    }


    while (TRUE) {
	/*
	 * Initialize the system configuration pointer.
	 */

	bzero((Address) netIEState.sysConfPtr, sizeof(NetIESysConfPtr));
	netIEState.sysConfPtr->intSysConfPtr = 
			NetIEAddrFrom68000Addr((int) netIEState.intSysConfPtr);

	/* 
	 * Initialize the intermediate system configuration pointer.
	 */

	bzero((Address) netIEState.intSysConfPtr, sizeof(NetIEIntSysConfPtr));
	netIEState.intSysConfPtr->busy = 1;
	netIEState.intSysConfPtr->base = 
			    NetIEAddrFrom68000Addr(netIEState.memBase);
	netIEState.intSysConfPtr->scbOffset = 
			    NetIEOffsetFrom68000Addr((int) netIEState.scbPtr);

	/*
	 * Initialize the system control block.
	 */

	bzero((Address) netIEState.scbPtr, sizeof(NetIESCB));

	/*
	 * Turn off the reset bit.
	 */

	netIEState.controlReg->noReset = 1;
	MACH_DELAY(200);

	/* 
	 * Get the attention of the chip so that it will initialize itself.
	 */

	NET_IE_CHANNEL_ATTENTION;

	/* 
	 * Ensure that that the chip gets the intermediate initialization
	 * stuff and that the scb is updated.
	 */
	
	NET_IE_DELAY(!netIEState.intSysConfPtr->busy);
	NET_IE_DELAY(netIEState.scbPtr->statusWord.cmdUnitNotActive);

	/*
	 * Wait for an interrupt.
	 */

	NET_IE_DELAY(netIEState.controlReg->intrPending);

	/*
	 * Make sure that the chip was initialized properly.
	 */

	if (netIEState.intSysConfPtr->busy || 
	    !netIEState.scbPtr->statusWord.cmdUnitNotActive ||
	    !netIEState.controlReg->intrPending) {
	    printf("Warning: Could not initialize Intel chip.\n");
	}
	if (netIEState.scbPtr->statusWord.cmdUnitStatus == NET_IE_CUS_IDLE) {
	    break;
	}

	printf("Warning: Intel cus not idle after reset\n");
	NET_IE_CHIP_RESET;
    }

    /*
     * Allocate a single command block to be used by all commands.
     */

    netIEState.cmdBlockPtr = (NetIECommandBlock *) NetIEMemAlloc();
    if (netIEState.cmdBlockPtr == (NetIECommandBlock *) NIL) {
	panic("NetIE: No memory for the command block.\n");
    }
    netIEState.scbPtr->cmdListOffset =
			NetIEOffsetFrom68000Addr((int) netIEState.cmdBlockPtr);

    /*
     * Do a diagnose command on the interface.
     */

    diagCmdPtr = netIEState.cmdBlockPtr;
    bzero((Address) diagCmdPtr, sizeof(*diagCmdPtr));
    diagCmdPtr->cmdNumber = NET_IE_DIAGNOSE;
    NetIEExecCommand(diagCmdPtr);
    if (!diagCmdPtr->cmdOK) {
	panic("Intel failed diagnostics.\n");
    }

    /*
     * Let the interface know its address.
     */

    addressCommandPtr = (NetIEIASetupCB *) netIEState.cmdBlockPtr;
    bzero((Address) addressCommandPtr, sizeof(NetIEIASetupCB));
    addressCommandPtr->cmdBlock.cmdNumber = NET_IE_IA_SETUP;
    addressCommandPtr->etherAddress = netIEState.etherAddress;
    NetIEExecCommand((NetIECommandBlock *) addressCommandPtr);

    /*
     * Set up the default configuration values.
     */

    NetIEDefaultConfig();

    /*
     * Set up the receive queues.
     */

    NetIERecvUnitInit();

    /*
     * Enable interrupts and get out of loop back mode.  Make sure that don't
     * get out of loop back mode before because the Intel is supposed to
     * be unpredictable until we initialize things.
     */

    netIEState.controlReg->intrEnable = 1;
    netIEState.controlReg->noLoopback = 1;

    /*
     * Initialize the transmit queues and start transmitting if anything ready
     * to tranmit.
     */

    NetIEXmitInit();
}


/*
 *----------------------------------------------------------------------
 *
 * NetIERestart --
 *
 *	Reinitialize the Intel Ethernet chip.
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
NetIERestart()
{

    DISABLE_INTR();

    /*
     * Allocate space for the System Configuration Pointer.
     */
    VmMach_MapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    /*
     * Reset the world.
     */
    NetIEReset();

    /*
     * Unmap the extra page.
     */
    VmMach_UnmapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    /*
     * Restart transmission of packets.
     */
    NetIEXmitRestart();

    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEIntr --
 *
 *	Process an interrupt from the Intel chip.
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
NetIEIntr(polling)
    Boolean	polling;	/* TRUE if are being polled instead of
				 * processing an interrupt. */
{
    register	NetIEState	*netIEStatePtr;
    register	NetIESCB	*scbPtr;
    register	int		status;

    netIEStatePtr = &netIEState;
    scbPtr = netIEState.scbPtr;

    /*
     * If we got a bus error then panic.
     */
    if (netIEStatePtr->controlReg->busError) {
	printf("Warning: Intel: Bus error on chip.\n");
	NetIERestart();
	return;
    }

    /*
     * If interrupts aren't enabled or there is no interrupt pending, then
     * what are we doing here?
     */
    if (!netIEStatePtr->controlReg->intrEnable || 
	!netIEStatePtr->controlReg->intrPending) {
	/*
	printf("Intel: Spurious interrupt <%x>\n", status);
	return;
	*/
    } 

    status = NET_IE_CHECK_STATUS(scbPtr->statusWord);
    if (status == 0) {
	if (!polling) {
	    printf("Intel: Spurious interrupt (2)\n");
	}
	return;
    }

    /*
     * Go ahead and ack the events that got us here.
     */
    NET_IE_CHECK_SCB_CMD_ACCEPT(scbPtr);
    NET_IE_ACK(scbPtr->cmdWord, status);
    NET_IE_CHANNEL_ATTENTION;

    /*
     * If we got a packet, then process it.
     */
    if (NET_IE_RECEIVED(status)) {
	NetIERecvProcess(FALSE);
    }

    /*
     * If a transmit command completed then process it.
     */
    if (NET_IE_TRANSMITTED(status)) {
	NetIEXmitDone();
    }
}
