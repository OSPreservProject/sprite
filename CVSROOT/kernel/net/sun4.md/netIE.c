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
#endif

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <netIEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <machMon.h>
#include <assert.h>

/*
 *----------------------------------------------------------------------
 *
 * NetIEInit --
 *
 *	Initialize the Intel Ethernet chip.
 *
 * Results:
 *	SUCCESS if the Intel controller was found and initialized,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the chip.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetIEInit(interPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
{
    Address 	ctrlAddr;	/* Kernel virtual address of controller. */
    int 	i;
    List_Links	*itemPtr;
    NetIEState	*statePtr;

    DISABLE_INTR();

    /*
     * Check that our structures are the correct size.  Some of the sizes
     * are different on the sun4 due to padding and alignment, but that
     * has been taken into account.
     */
#ifdef sun4
    assert(sizeof(NetIESysConfPtr) == 12);
    assert(sizeof(NetIERecvFrameDesc) == 28);
    assert(sizeof(NetIERecvBufDesc) == 20);
#else
    assert(sizeof(NetIESysConfPtr) == 10);
    assert(sizeof(NetIERecvFrameDesc) == 26);
    assert(sizeof(NetIERecvBufDesc) == 18);
#endif
    assert(sizeof(NetIEIntSysConfPtr) == 8);
    assert(sizeof(NetIESCBStatus) == 2);
    assert(sizeof(NetIESCBCommand) == 2);
    assert(sizeof(NetIESCB) == 16);
    assert(sizeof(NetIECommandBlock) == 6);
    assert(sizeof(NetIENOPCB) == 6);
    assert(sizeof(NetIEIASetupCB) == 12);
    assert(sizeof(NetIEConfigureCB) == 18);
    assert(sizeof(NetIETransmitCB) == 16);
    assert(sizeof(NetIETransmitBufDesc) == 8);
    assert(sizeof(NetIEControlRegister) == 1);


    ctrlAddr = interPtr->ctrlAddr;
    /*
     * If the address is physical (not in kernel's virtual address space)
     * then we have to map it in.
     */
    if (interPtr->virtual == FALSE) {
	ctrlAddr = (Address) VmMach_MapInDevice(ctrlAddr, 1);
    }
    statePtr = (NetIEState *) malloc (sizeof(NetIEState));
    bzero((char *) statePtr, sizeof(NetIEState));

    statePtr->running = FALSE;

    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    statePtr->controlReg = 
	(volatile NetIEControlRegister *) ctrlAddr;

     {
	/*
	 * Poke the controller by resetting it.
	 */
	char zero = 0;
	ReturnStatus status;

	status = Mach_Probe(sizeof(char), &zero,
	    (char *)statePtr->controlReg);
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

    netIEXmitFiller = (char *)VmMach_NetMemAlloc(NET_ETHER_MIN_BYTES);
    statePtr->netIEXmitTempBuffer = 
	    (char *)VmMach_NetMemAlloc(NET_ETHER_MAX_BYTES + 2);

    for (i = 0; i < NET_IE_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) VmMach_NetMemAlloc(sizeof(NetXmitElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->xmitFreeList));
    }

    /*
     * Get ethernet address out of the rom.  It is stored in a normal order
     * even though the chip is wired backwards because fortunately the chip
     * stores the ethernet address backwards from how we store it.  Therefore
     * two backwards makes one forwards, right?
     */

    Mach_GetEtherAddress(&statePtr->etherAddress);
    printf("%s Ethernet address %x:%x:%x:%x:%x:%x\n", 
	      interPtr->name,
	      statePtr->etherAddress.byte1 & 0xff,
	      statePtr->etherAddress.byte2 & 0xff,
	      statePtr->etherAddress.byte3 & 0xff,
	      statePtr->etherAddress.byte4 & 0xff,
	      statePtr->etherAddress.byte5 & 0xff,
	      statePtr->etherAddress.byte6 & 0xff);
    /*
     * Allocate space for the System Configuration Pointer.
     */

    VmMach_MapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));
    statePtr->sysConfPtr = (NetIESysConfPtr *) NET_IE_SYS_CONF_PTR_ADDR;

    /*
     * Allocate space for all of the receive buffers. The buffers are 
     * allocated on an odd short word boundry so that packet data (after
     * the ethernet header) will start on a long word boundry. This 
     * eliminates unaligned word fetches from the RPC module which would
     * cause alignment traps on SPARC processors such as the sun4.
     */ 

#define	ALIGNMENT_PADDING	(sizeof(Net_EtherHdr)&0x3)
    for (i = 0; i < NET_IE_NUM_RECV_BUFFERS; i++) {
	statePtr->netIERecvBuffers[i] = 
		VmMach_NetMemAlloc(NET_IE_RECV_BUFFER_SIZE + ALIGNMENT_PADDING)
		    + ALIGNMENT_PADDING;
    }
#undef ALIGNMENT_PADDING

    interPtr->init	= NetIEInit;
    interPtr->output 	= NetIEOutput;
    interPtr->intr	= NetIEIntr;
    interPtr->ioctl	= NetIEIOControl;
    interPtr->reset 	= NetIERestart;
    interPtr->getStats	= NetIEGetStats;
    interPtr->netType	= NET_NETWORK_ETHER;
    interPtr->maxBytes	= NET_ETHER_MAX_BYTES - sizeof(Net_EtherHdr);
    interPtr->minBytes	= 0;
    interPtr->interfaceData = (ClientData) statePtr;
    NET_ETHER_ADDR_COPY(statePtr->etherAddress, 
	interPtr->netAddress[NET_PROTO_RAW].ether);
    interPtr->broadcastAddress.ether = netEtherBroadcastAddress.ether;
    interPtr->flags |= NET_IFLAGS_BROADCAST;
    statePtr->interPtr = interPtr;

    /*
     * Reset the world.
     */

    NetIEReset(interPtr);

    /*
     * Unmap the extra page.
     */

    VmMach_UnmapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    /*
     * Now we are running.
     */

    statePtr->running 	= TRUE;
    ENABLE_INTR();
    return (SUCCESS);
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
NetIEDefaultConfig(statePtr)
    NetIEState		*statePtr;
{
    NetIEConfigureCB	*confCBPtr;

    confCBPtr = (NetIEConfigureCB *) statePtr->cmdBlockPtr;
    bzero((Address) confCBPtr, sizeof(NetIEConfigureCB));
    NetBfShortSet(confCBPtr->cmdBlock.bits, CmdNumber, NET_IE_CONFIG);
    NetBfShortSet(confCBPtr->bits, ByteCount, 12);
    NetBfShortSet(confCBPtr->bits, FifoLimit, 12);
    NetBfShortSet(confCBPtr->bits, Preamble, 2);
    NetBfShortSet(confCBPtr->bits, AddrLen, 6);
    NetBfShortSet(confCBPtr->bits, AtLoc, 0);
    NetBfShortSet(confCBPtr->bits, InterFrameSpace, 96);
    NetBfShortSet(confCBPtr->bits, SlotTimeHigh , 512 >> 8);
    NetBfShortSet(confCBPtr->bits, MinFrameLength, 64);
    NetBfShortSet(confCBPtr->bits, NumRetries, 15);

/*
    confCBPtr->intLoopback = 1;
*/

    NetIEExecCommand((NetIECommandBlock *) confCBPtr, statePtr);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEReset --
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
NetIEReset(interPtr)
    Net_Interface	*interPtr; 	/* Interface to reset. */
{
    NetIEIASetupCB	        *addressCommandPtr;
    volatile NetIECommandBlock	*diagCmdPtr;
    NetIEState			*statePtr;

    /*
     * Nil out all pointers.
     */
    statePtr = (NetIEState *) interPtr->interfaceData;
    statePtr->intSysConfPtr = (NetIEIntSysConfPtr *) NIL;
    statePtr->scbPtr = (NetIESCB *) NIL;
    statePtr->recvFrDscHeadPtr = (NetIERecvFrameDesc *) NIL;
    statePtr->recvFrDscTailPtr = (NetIERecvFrameDesc *) NIL;
    statePtr->recvBufDscHeadPtr = (NetIERecvBufDesc *) NIL;
    statePtr->recvBufDscTailPtr = (NetIERecvBufDesc *) NIL;

    /* 
     * Reset the chip.
     */

    NET_IE_CHIP_RESET(statePtr);

    /*
     * Initialize memory.
     */

    NetIEMemInit(statePtr);

    /*
     * Allocate the system intermediate configuration pointer and the 
     * system control block.
     */

    statePtr->intSysConfPtr = (NetIEIntSysConfPtr *) NetIEMemAlloc(statePtr);
    if (statePtr->intSysConfPtr == (NetIEIntSysConfPtr *) NIL) {
	panic("Intel: No memory for the scp.\n");
    }

    statePtr->scbPtr = (NetIESCB *) NetIEMemAlloc(statePtr);
    if (statePtr->scbPtr == (NetIESCB *) NIL) {
	panic("Intel: No memory for the scb.\n");
    }


    while (TRUE) {
	/*
	 * Initialize the system configuration pointer.
	 */

	bzero((Address) statePtr->sysConfPtr, sizeof(NetIESysConfPtr));
	statePtr->sysConfPtr->intSysConfPtr = 
			NetIEAddrFromSUNAddr((int) statePtr->intSysConfPtr);

	/* 
	 * Initialize the intermediate system configuration pointer.
	 */

	bzero((Address) statePtr->intSysConfPtr, sizeof(NetIEIntSysConfPtr));
	statePtr->intSysConfPtr->busy = 1;
	statePtr->intSysConfPtr->base = 
			    NetIEAddrFromSUNAddr((int) statePtr->memBase);
	statePtr->intSysConfPtr->scbOffset = 
			    NetIEOffsetFromSUNAddr((int) statePtr->scbPtr,
				    statePtr);

	/*
	 * Initialize the system control block.
	 */

	bzero((Address) statePtr->scbPtr, sizeof(NetIESCB));

	/*
	 * Turn off the reset bit.
	 */

	NetBfByteSet(statePtr->controlReg, NoReset, 1);
	MACH_DELAY(200);

	/* 
	 * Get the attention of the chip so that it will initialize itself.
	 */

	NET_IE_CHANNEL_ATTENTION(statePtr);

	/* 
	 * Ensure that that the chip gets the intermediate initialization
	 * stuff and that the scb is updated.
	 */
	
	NET_IE_DELAY(!statePtr->intSysConfPtr->busy);
	NET_IE_DELAY(NetBfShortTest(statePtr->scbPtr->statusWord, 
	    CmdUnitNotActive, 1));

	/*
	 * Wait for an interrupt.
	 */

	NET_IE_DELAY(NetBfByteTest(statePtr->controlReg, IntrPending, 1));

	/*
	 * Make sure that the chip was initialized properly.
	 */

	if (statePtr->intSysConfPtr->busy || 
	    NetBfShortTest(statePtr->scbPtr->statusWord, CmdUnitNotActive, 0) ||
	    NetBfByteTest(statePtr->controlReg, IntrPending, 0)) {

	    printf("Warning: Could not initialize Intel chip.\n");
	}
	if (NetBfShortTest(statePtr->scbPtr->statusWord, CmdUnitStatus, 
		NET_IE_CUS_IDLE)) {
	    break;
	}

	printf("Warning: Intel cus not idle after reset\n");
	NET_IE_CHIP_RESET(statePtr);
    }

    /*
     * Allocate a single command block to be used by all commands.
     */

    statePtr->cmdBlockPtr = (NetIECommandBlock *) NetIEMemAlloc(statePtr);
    if (statePtr->cmdBlockPtr == (NetIECommandBlock *) NIL) {
	panic("NetIE: No memory for the command block.\n");
    }
    statePtr->scbPtr->cmdListOffset =
			NetIEOffsetFromSUNAddr((int) statePtr->cmdBlockPtr,
				statePtr);

    /*
     * Do a diagnose command on the interface.
     */

    diagCmdPtr = statePtr->cmdBlockPtr;
    bzero((Address) diagCmdPtr, sizeof(*diagCmdPtr));
    NetBfShortSet(diagCmdPtr->bits, CmdNumber,  NET_IE_DIAGNOSE);
    NetIEExecCommand(diagCmdPtr, statePtr);
    if (NetBfShortTest(diagCmdPtr->bits, CmdOK, 0)) {
	panic("Intel failed diagnostics.\n");
    }

    /*
     * Let the interface know its address.
     */

    addressCommandPtr = (NetIEIASetupCB *) statePtr->cmdBlockPtr;
    bzero((Address) addressCommandPtr, sizeof(NetIEIASetupCB));
    NetBfShortSet(addressCommandPtr->cmdBlock.bits, CmdNumber, NET_IE_IA_SETUP);
    addressCommandPtr->etherAddress = statePtr->etherAddress;
    NetIEExecCommand((NetIECommandBlock *) addressCommandPtr, statePtr);

    /*
     * Set up the default configuration values.
     */

    NetIEDefaultConfig(statePtr);

    /*
     * Set up the receive queues.
     */

    NetIERecvUnitInit(statePtr);

    /*
     * Enable interrupts and get out of loop back mode.  Make sure that don't
     * get out of loop back mode before because the Intel is supposed to
     * be unpredictable until we initialize things.
     */

    NetBfByteSet(statePtr->controlReg, IntrEnable, 1);
    NetBfByteSet(statePtr->controlReg, NoLoopback, 1);

    /*
     * Initialize the transmit queues and start transmitting if anything ready
     * to tranmit.
     */

    NetIEXmitInit(statePtr);
    interPtr->flags |= NET_IFLAGS_RUNNING;
    return;
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
NetIERestart(interPtr)
    Net_Interface	*interPtr; 	/* Interface to restart. */
{
    NetIEState	*statePtr = (NetIEState *) interPtr->interfaceData;

    DISABLE_INTR();

    /*
     * Drop the current packet so the transmitting process doesn't hang.
     */
    NetIEXmitDrop(statePtr);

    /*
     * Allocate space for the System Configuration Pointer.
     */
    VmMach_MapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    /*
     * Reset the world.
     */
    NetIEReset(interPtr);

    /*
     * Unmap the extra page.
     */
    VmMach_UnmapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR));

    ENABLE_INTR();
    return;
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
NetIEIntr(interPtr, polling)
    Net_Interface	*interPtr;	/* Interface to process. */
    Boolean		polling;	/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    register	NetIEState	*statePtr;
    volatile register NetIESCB	*scbPtr;
    register	int		status;

    statePtr = (NetIEState *) interPtr->interfaceData;
    scbPtr = statePtr->scbPtr;
    /*
     * If we got a bus error then panic.
     */
    if (NetBfByteTest(statePtr->controlReg, BusError, 1)) {
	printf("Warning: Intel: Bus error on chip.\n");
	NetIERestart(interPtr);
	return;
    }

    /*
     * If interrupts aren't enabled or there is no interrupt pending, then
     * what are we doing here?
     */
    if (!(NetBfByteTest(statePtr->controlReg, IntrEnable, 1) && 
	NetBfByteTest(statePtr->controlReg, IntrPending, 1))) {
	/*
	 * I'm not sure why this is commented out. That's the way I found
	 * it.  JHH
	 */
#if 0
	if (!polling) {
	    printf("Intel: Spurious interrupt <%x>\n", 
		(unsigned int) (*((unsigned char *) statePtr->controlReg)));
	}
	return;
#endif 
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
    NET_IE_CHANNEL_ATTENTION(statePtr);

    /*
     * If we got a packet, then process it.
     */
    if (NET_IE_RECEIVED(status)) {
	NetIERecvProcess(FALSE, statePtr);
    }

    /*
     * If a transmit command completed then process it.
     */
    if (NET_IE_TRANSMITTED(status)) {
	NetIEXmitDone(statePtr);
    }
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * NetIEGetStats --
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
NetIEGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    NetIEState	*statePtr;
    statePtr = (NetIEState *) interPtr->interfaceData;
    DISABLE_INTR();
    statPtr->ether = statePtr->stats;
    ENABLE_INTR();
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetIEIOControl --
 *
 *	Perform ioctls for the adapter.  Right now we don't support any.
 *
 * Results:
 *	DEV_INVALID_ARG
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
NetIEIOControl(interPtr, ioctlPtr, replyPtr)
    Net_Interface *interPtr;	/* Interface on which to perform ioctl. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    return DEV_INVALID_ARG;
}

/*
 *----------------------------------------------------------------------
 *
 * NetIEStatePrint --
 *
 *	Prints out the contents of a NetIEState..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed.
 *
 *----------------------------------------------------------------------
 */

void
NetIEStatePrint(statePtr)
    NetIEState		*statePtr;
{
    printf("statePtr = 0x%x\n", statePtr);
    printf("membase = 0x%x\n", statePtr->memBase);
    printf("sysConfPtr = 0x%x\n", statePtr->sysConfPtr);
    printf("intSysConfPtr = 0x%x\n", statePtr->intSysConfPtr);
    printf("scbPtr = 0x%x\n", statePtr->scbPtr);
    printf("cmdBlockPtr = 0x%x\n", statePtr->cmdBlockPtr);
    printf("recvFrDscHeadPtr = 0x%x\n", statePtr->recvFrDscHeadPtr);
    printf("recvFrDscTailPtr = 0x%x\n", statePtr->recvFrDscTailPtr);
    printf("recvBufDscHeadPtr = 0x%x\n", statePtr->recvBufDscHeadPtr);
    printf("recvBufDscTailPtr = 0x%x\n", statePtr->recvBufDscTailPtr);
    printf("xmitList = 0x%x\n", statePtr->xmitList);
    printf("xmitFreeList = 0x%x\n", statePtr->xmitFreeList);
    printf("xmitCBPtr = 0x%x\n", statePtr->xmitCBPtr);
    printf("transmitting = %d\n", statePtr->transmitting);
    printf("running = %d\n", statePtr->running);
    printf("controlReg = 0x%x\n", statePtr->controlReg);
    printf("netIEXmitTempBuffer = 0x%x\n", statePtr->netIEXmitTempBuffer);
    printf("xmitBufAddr = 0x%x\n", statePtr->xmitBufAddr);
    printf("curScatGathPtr = 0x%x\n", statePtr->curScatGathPtr);
    printf("interPtr = 0x%x\n", statePtr->interPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NetIEIntSysConfPtrPrint --
 *
 *	Prints the contents of a NetIEIntSysConfPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed
 *
 *----------------------------------------------------------------------
 */

void
NetIEIntSysConfPtrPrint(confPtr) 
    volatile NetIEIntSysConfPtr	*confPtr;
{
    printf("busy = %d\n", (int) confPtr->busy);
    printf("scbOffset = 0x%x\n",  confPtr->scbOffset);
    printf("base = 0x%x\n", confPtr->base); 
}

/*
 *----------------------------------------------------------------------
 *
 * NetIESCBPrint --
 *
 *	description.
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
NetIESCBPrint(scbPtr)
    volatile NetIESCB	*scbPtr;
{
    printf("status = 0x%x\n", * ((unsigned short *) &scbPtr->statusWord));
    printf("cmd = 0x%x\n", * ((unsigned short *) &scbPtr->cmdWord));
    printf("cmdListOffset = 0x%x\n",  scbPtr->cmdListOffset);
    printf("recvFrameAreaOffset = 0x%x\n",  scbPtr->recvFrameAreaOffset);
    printf("crcErrors = %d\n",  scbPtr->crcErrors);
    printf("alignErrors = %d\n",  scbPtr->alignErrors);
    printf("resourceErrors = %d\n",  scbPtr->resourceErrors);
    printf("overrunErrors = %d\n",  scbPtr->overrunErrors);
}

/*
 *----------------------------------------------------------------------
 *
 * NetIETransmitCBPrint --
 *
 *	Print the contents of a NetIETransmitCB.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	stuff is printed.
 *
 *----------------------------------------------------------------------
 */

void
NetIETransmitCBPrint(xmitCBPtr)
    NetIETransmitCB	*xmitCBPtr;
{
    char buffer[32];
    printf("xmitCBPtr = 0x%x\n", xmitCBPtr);
    printf("bits = 0x%x\n", xmitCBPtr->bits[0]);
    printf("nextCmdBlock = 0x%x\n", xmitCBPtr->nextCmdBlock);
    printf("bufDescOffset = 0x%x\n", xmitCBPtr->bufDescOffset);
    printf("etherAddress = %s\n", 
	Net_AddrToString((Net_Address *) &xmitCBPtr->destEtherAddr,
	NET_PROTO_RAW, NET_NETWORK_ETHER, buffer));
    printf("type = 0x%x\n", xmitCBPtr->type);
}

void
NetIERecvBufDescPrint(recvBufDescPtr)
    NetIERecvBufDesc	*recvBufDescPtr;
{
    printf("recvBufDescPtr = 0x%x\n", recvBufDescPtr);
    printf("bits1 = 0x%x\n", recvBufDescPtr->bits1[0]);
    printf("nextRBD = 0x%x\n", recvBufDescPtr->nextRBD);
    printf("bufAddr = 0x%x\n", recvBufDescPtr->bufAddr);
    printf("bits2 = 0x%x\n", recvBufDescPtr->bits2[0]);
    printf("realBufAddr = 0x%x\n", recvBufDescPtr->realBufAddr);
    printf("realNextRBD = 0x%x\n", recvBufDescPtr->realNextRBD);
}

void
NetIERecvFrameDescPrint(recvFrDescPtr) 
    NetIERecvFrameDesc 	*recvFrDescPtr;
{
    char	buffer[32];
    printf("NetIERecvFrameDesc = 0x%x\n", recvFrDescPtr);
    printf("bits = 0x%x\n", recvFrDescPtr->bits[0]);
    printf("nextRFD = 0x%x\n", recvFrDescPtr->nextRFD);
    printf("recvBufferDesc = 0x%x\n", recvFrDescPtr->recvBufferDesc);
    printf("destAddr = %s\n", 
	Net_AddrToString((Net_Address *) &recvFrDescPtr->destAddr,
	NET_PROTO_RAW, NET_NETWORK_ETHER, buffer));
    printf("srcAddr = %s\n", 
	Net_AddrToString((Net_Address *) &recvFrDescPtr->srcAddr,
	NET_PROTO_RAW, NET_NETWORK_ETHER, buffer));
    printf("type = 0x%x\n", recvFrDescPtr->type);
}

void
NetIETransmitBufDescPrint(xmitBufDescPtr)
    NetIETransmitBufDesc	*xmitBufDescPtr;
{
    printf("xmitBufDescPtr = 0x%x\n", xmitBufDescPtr);
    printf("bits = 0x%x\n", * (unsigned short *) xmitBufDescPtr);
    printf("nextTBD = 0x%x\n", xmitBufDescPtr->nextTBD);
    printf("bufAddr = 0x%x\n", xmitBufDescPtr->bufAddr);
}

