/* 
 * netIE.c --
 *
 *	The main routines for the device driver for the Intel 82586 Ethernet 
 *	Controller as used on the TI Explorer's NuBus ethernet controller.
 *
 *		The following comment is from experences with the Sun 
 *		implementation of an ethernet controller using the 82586.
 *		It is currently not know if these pecularities hold true
 *		for the TI implementation. The current implementation
 *		copies the packet into a buffer on the ethernet board
 *		and hence is not effected by the following problems.
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
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "list.h"
#include "vm.h"
#include "vmMach.h"
#include "mach.h"
#include "machConfig.h"
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"
#include "sync.h"

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
 * Semaphore protecting the chip and driver's data structures.
 */

int netIEMutex	= 0;

/*
 * Macro to fetch a byte from the board's ROM at a 
 * specified slot offset.
 */

#define	ROM_GET_BYTE(offset)	(* (unsigned char *) NET_IE_SLOT_OFFSET(offset))

/*
 *----------------------------------------------------------------------
 *
 * ReadConfigROM --
 *
 *	Read the ethernet board's configuation ROM and update the netIEState 
 *	structure..
 *
 * Results:
 *	SUCCESS if read proceeded normally and netIEState updated.
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the netIEState record.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ReadConfigROM()
{
    unsigned	char	layoutNum;	/* Layout number of ROM */	
    unsigned	int	offset;		/* offset into device slot */

    /*
     * Make sure that the ROM is mapped into our address space.
     */
    if (!netIEState.mapped) {
	printf("Warning: Intel ethernet: Can not read unmapped ROM.\n");
	return(FAILURE);
    }
    /*
     * Make sure that the ROM is of the version that the code was written
     * for.  
     */
    layoutNum = ROM_GET_BYTE(IEROM_LAYOUT);
    if (layoutNum != IEROM_LAYOUT_NUMBER) {
	printf("Warning: Intel ethernet: Bad ROM layout number (%d)\n",
			layoutNum);
    }

     /*
      * Get the offsets from the start of the slot space of the configuration
      * and flags regs. These offsets are 3 byte values stored in the low order
      * byte of consecutive words.
      */
    offset = ROM_GET_BYTE(IEROM_CONFIG_REG_ADDR) | 
		(ROM_GET_BYTE(IEROM_CONFIG_REG_ADDR+4) << 8) | 
		(ROM_GET_BYTE(IEROM_CONFIG_REG_ADDR+8) << 16);

    netIEState.configAndFlagsReg =
		(NetTIConfigAndFlagsReg *) NET_IE_SLOT_OFFSET(offset);

    /*
     * Get our ethernet address from the ROM.
     */

    NET_ETHER_ADDR_BYTE1(netIEState.etherAddress) =
			 ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS);
    NET_ETHER_ADDR_BYTE2(netIEState.etherAddress) =
			 ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS+4);
    NET_ETHER_ADDR_BYTE3(netIEState.etherAddress) =
			ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS+8);
    NET_ETHER_ADDR_BYTE4(netIEState.etherAddress) =
			ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS+12);
    NET_ETHER_ADDR_BYTE5(netIEState.etherAddress) =
			ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS+16);
    NET_ETHER_ADDR_BYTE6(netIEState.etherAddress) =
			ROM_GET_BYTE(IEROM_ETHERNET_ADDRESS+20);

    /*
     * Setup the fixed location registers.  
     */

    netIEState.channelAttnReg = 
			(int *) NET_IE_SLOT_OFFSET(CHANNEL_ATTN_REG_OFFSET);

    netIEState.sysConfPtr = 
	(NetIESysConfPtr *) NET_IE_SLOT_OFFSET(SYS_CONF_PTR_OFFSET);

    /*
     * Finally, check to see that the board is configured with enought 
     * memory to support the algorithm used.
     */

    if (*netIEState.configAndFlagsReg & NET_IE_FLAG_32K_MEMORY) {
	printf("Warning: Intel ethernet: Bad Memory size bit.\n");
	return (FAILURE);
    }

    return (SUCCESS);

}



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
NetIEInit(name, number, slotId)
    char *name;			/* Device name.	 */
    int number;			/* Device number, not used. */
    unsigned int slotId;	/* Default slot ID of controller board. */
{
    int 	i;
    List_Links	*itemPtr;


    netIEState.running = FALSE;
    netIEState.mapped = FALSE;

    /*
     * Lookup the ethernet boards location from the machine configuration.
     */
    {
	Mach_Board	board;
	if (Mach_FindBoardDescription(MACH_CONFIG_ETHER_BOARD, 0, FALSE,
			&board) == SUCCESS) {
	    /*
	     * Override the default slot Id if board is specified in the
	     * machine configuration.
	     */
	    slotId = board.slotId;
	}
    }

    /*
     * Map the device into our address space.
     */

    NetIEMemMap(slotId);

    /*
     * Read the configuration information such as control register
     * location into the netIEState structure.
     */

    if (ReadConfigROM() != SUCCESS) {
	return (FALSE);
    }

    MASTER_LOCK(netIEMutex);
    /*
     * Initialize the transmission list.  
     */

    netIEState.xmitList = &xmitListHdr;
    List_Init(netIEState.xmitList);

    netIEState.xmitFreeList = &xmitFreeListHdr;
    List_Init(netIEState.xmitFreeList);

    for (i = 0; i < NET_IE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) Vm_RawAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(netIEState.xmitFreeList));
    }

    /*
     * Get ethernet address.
     * Mach_GetEtherAddress(&netIEState.etherAddress);
     * On the TI Board, the ROM contains the ethernet address.
     */
    printf("%s-%d Ethernet address %x:%x:%x:%x:%x:%x\n", name, number,
	      NET_ETHER_ADDR_BYTE1(netIEState.etherAddress) & 0xff,
	      NET_ETHER_ADDR_BYTE2(netIEState.etherAddress) & 0xff,
	      NET_ETHER_ADDR_BYTE3(netIEState.etherAddress) & 0xff,
	      NET_ETHER_ADDR_BYTE4(netIEState.etherAddress) & 0xff,
	      NET_ETHER_ADDR_BYTE5(netIEState.etherAddress) & 0xff,
	      NET_ETHER_ADDR_BYTE6(netIEState.etherAddress) & 0xff);
    /*
     * Reset the world.
     */

    NetIEReset();

    /*
     * Now we are running.
     */

    netIEState.running = TRUE;
    netEtherFuncs.init	 = NetIEInit;
    netEtherFuncs.output = NetIEOutput;
    netEtherFuncs.intr   = NetIEIntr;
    netEtherFuncs.reset  = NetIERestart;

    MASTER_UNLOCK(netIEMutex);
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
    bzero((char *) confCBPtr, sizeof(NetIEConfigureCB));
    confCBPtr->cmdNumber = NET_IE_CONFIG;
    confCBPtr->byteCount = 12;
    confCBPtr->fifoLimit = 12;
    confCBPtr->preamble = 2;
    confCBPtr->addrLen = 6;
    confCBPtr->atLoc = 0;
    confCBPtr->interFrameSpace = 96;
    confCBPtr->slotTime = 512;
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
 *	Buffer memory is allocated.
 *
 *----------------------------------------------------------------------
 */

void
NetIEReset()
{
    NetIEIASetupCB	*addressCommandPtr;
    NetIECommandBlock	*diagCmdPtr;
    int			i;

    /*
     * Nil out all pointers.
     */

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

    netIEState.intSysConfPtr = 
	    (NetIEIntSysConfPtr *) NetIEMemAlloc(sizeof(NetIEIntSysConfPtr));
    if (netIEState.intSysConfPtr == (NetIEIntSysConfPtr *) NIL) {
	panic("Intel Ethernet: No memory for the scp.\n");
    }

    netIEState.scbPtr = (NetIESCB *) NetIEMemAlloc(sizeof(NetIESCB));
    if (netIEState.scbPtr == (NetIESCB *) NIL) {
	panic("Intel Ethernet: No memory for the scb.\n");
    }


    while (TRUE) {
	/*
	 * Initialize the system configuration pointer.
	 */

	bzero((char *) netIEState.sysConfPtr,sizeof(NetIESysConfPtr));
	netIEState.sysConfPtr->intSysConfPtr = 
		NetIEAddrFromSPURAddr((Address) netIEState.intSysConfPtr);

	/* 
	 * Initialize the intermediate system configuration pointer.
	 */

	bzero((char *) netIEState.intSysConfPtr,sizeof(NetIEIntSysConfPtr));
	netIEState.intSysConfPtr->busy = 1;
	netIEState.intSysConfPtr->base = 
			NetIEAddrFromSPURAddr(netIEState.memBase);
	netIEState.intSysConfPtr->scbOffset = 
			NetIEOffsetFromSPURAddr((Address) netIEState.scbPtr);

	/*
	 * Initialize the system control block.
	 */

	bzero( (char *) netIEState.scbPtr, sizeof(NetIESCB));


	/* 
	 * Get the attention of the chip so that it will initialize itself.
	 */

	NET_IE_CHANNEL_ATTENTION;

	/* 
	 * Ensure that that the chip gets the intermediate initialization
	 * stuff and that the scb is updated.
	 */


	NET_IE_DELAY(!netIEState.intSysConfPtr->busy);
	NET_IE_DELAY(netIEState.scbPtr->cmdUnitNotActive);


	/*
	 * Make sure that the chip was initialized properly.
	 */

	if (netIEState.intSysConfPtr->busy || 
	    !netIEState.scbPtr->cmdUnitNotActive) {
	    printf("Warning: Could not initialize Intel Ethernet chip.\n");
	}
	if (netIEState.scbPtr->cmdUnitStatus == NET_IE_CUS_IDLE) {
	    break;
	}

	printf("Warning: Intel cus not idle after reset\n");
	NET_IE_CHIP_RESET;
    }

    /*
     * Allocate a single command block to be used by all commands.
     */

    netIEState.cmdBlockPtr = 
		(NetIECommandBlock *) NetIEMemAlloc(NET_IE_MAX_CMD_BLOCK_SIZE);
    if (netIEState.cmdBlockPtr == (NetIECommandBlock *) NIL) {
	panic("NetIE: No memory for the command block.\n");
    }
    netIEState.scbPtr->cmdListOffset =
		NetIEOffsetFromSPURAddr((Address) netIEState.cmdBlockPtr);

    /*
     * Do a diagnose command on the interface.
     */

    diagCmdPtr = netIEState.cmdBlockPtr;
    bzero((Address) diagCmdPtr,sizeof(*diagCmdPtr));
    diagCmdPtr->cmdNumber = NET_IE_DIAGNOSE;
    NetIEExecCommand(diagCmdPtr);
    if (!diagCmdPtr->cmdOK) {
	panic("Intel Ethernet failed diagnostics.\n");
    }

    /*
     * Let the interface know its address.
     */

    addressCommandPtr = (NetIEIASetupCB *) netIEState.cmdBlockPtr;
    bzero( (char *) addressCommandPtr,sizeof(NetIEIASetupCB));
    addressCommandPtr->cmdNumber = NET_IE_IA_SETUP;
    NET_ETHER_ADDR_COPY(netIEState.etherAddress,
			addressCommandPtr->etherAddress);
    NetIEExecCommand((NetIECommandBlock *) addressCommandPtr);

    /*
     * Set up the default configuration values.
     */

    NetIEDefaultConfig();

    /*
     * Allocate space for all of the receive buffers. The buffers are 
     * allocated on an odd short word boundry so that packet data (after
     * the ethernet header) will start on a long word boundry.
     */

    for (i = 0; i < NET_IE_NUM_RECV_BUFFERS; i++) {
	netIERecvBuffers[i] = (NetIEMemAlloc(NET_IE_RECV_BUFFER_SIZE+2))+2;
    }
    /*
     * Set up the receive queues.
     */

    NetIERecvUnitInit();

    /*
     * Initialize the event register.
     */

     {
	unsigned int	*eventRegPtr;
	unsigned int	mySlotId;
	unsigned int	intrNum;


	mySlotId = Mach_GetSlotId();
	intrNum = MACH_EXT_INTERRUPT_ANY;
	if (Mach_AllocExtIntrNumber(Net_Intr,&intrNum) != SUCCESS) {
	    panic("Intel Ethernet: Can not allocate interrupt number.\n");
	}
	eventRegPtr = (unsigned int *)NET_IE_SLOT_OFFSET(EVENT_ADDR_REG_OFFSET);
	*eventRegPtr = 0xf0000000 | (mySlotId << 24) | (intrNum << 2);
    }

    /*
     * Enable interrupts and get out of loop back mode.  Make sure that don't
     * get out of loop back mode before because the Intel is supposed to
     * be unpredictable until we initialize things. Turn LED off to symbolize
     * start of board.
     */

    *netIEState.configAndFlagsReg = NET_IE_CONFIG_INTR_ENABLE;

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

    MASTER_LOCK(netIEMutex);

    /*
     * Reset the world.
     */
    NetIEReset();

    /*
     * Restart transmission of packets.
     */
    NetIEXmitRestart();

    MASTER_UNLOCK(netIEMutex);
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
    register	NetIESCB	*scbPtr;
    register	int		status;

    scbPtr = netIEState.scbPtr;


    MASTER_LOCK(netIEMutex);
    status = NET_IE_CHECK_STATUS(scbPtr);
    if (status == 0) {
	MASTER_UNLOCK(netIEMutex);
	return;
    }

    /*
     * Go ahead and ack the events that got us here.
     */
    NET_IE_CHECK_SCB_CMD_ACCEPT(scbPtr);
    NET_IE_ACK(scbPtr, status);
    NET_IE_CHANNEL_ATTENTION;

    MASTER_UNLOCK(netIEMutex);

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


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetEtherAddress --
 *
 *	Return the ethernet address of the machine.
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
Mach_GetEtherAddress(etherAddrPtr)
    Net_EtherAddress 	*etherAddrPtr;		/* Where to place address. */
{
    if (!netIEState.running) { 
	panic("Mach_GetEtherAddress called before Net_Init()\n");
    }
    /*
     * netIEState.etherAddress is never modified after system boot so
     * no lock is needed here.
     */
    NET_ETHER_ADDR_COPY(netIEState.etherAddress, *etherAddrPtr);

}



