/* 
 * netUltra.c --
 *
 *	Routines for handling the Ultranet VME Adapter card..
 *
 * Copyright 1990 Regents of the University of California
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
#endif /* not lint */

#include <sprite.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <netUltraInt.h>
#include <dev/ultra.h>
#include <fmt.h>
#include <sync.h>
#include <dbg.h>
#include <rpcPacket.h>
#include <assert.h>

/*
 * "Borrow" the Vax format for the format of the Ultranet adapter (it
 * is really a 386.  We need to swap some of the words in the
 * diagnostic and information structures.
 */

#define NET_ULTRA_FORMAT FMT_VAX_FORMAT

Boolean		netUltraDebug = FALSE;
Boolean		netUltraTrace = TRUE;
int		netUltraMapThreshold = NET_ULTRA_MAP_THRESHOLD;
static int	packetsSunk = 0;
static Time	sinkStartTime;
static Time	sinkEndTime;

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define WAIT_FOR_REPLY(ptr, count) {		\
    int	i;					\
    for(i = (count); i > 0; i -= 100) {		\
	MACH_DELAY(100);			\
	if (*((volatile int *)ptr) != 0) {	\
	    break;				\
	}					\
    }						\
}

/*
 * The following are the names of the diagnostic tests as listed in the
 * uvm man page provided in the Ultranet documentation.
 */
static char	*diagNames[] = {
    "EPROM checksum",
    "Abbreviated RAM check",
    "Interrupt controller and interval timer",
    "Internal loopback",
    "FIFO RAM check",
    "Checksum gate arrays",
    "NMI control logic"
};
/*
 * The following are the names of the extended diagnostic tests as
 * listed in the uvm man page.
 */
static char	*extDiagNames[] = {
    "EPROM checksum",
    "Full RAM check",
    "Interrupt controller and interval timer",
    "Internal or external loopback",
    "FIFO RAM check",
    "Checksum gate arrays",
    "NMI control logic",
    "DMA to hosts memory using VME bus",
    "Extended FIFO RAM check",
    "FIFO Control logic"
};

/*
 * This is a "wildcard" address that matches any TL address.
 * Once again we have to lie about the size of a Net_Address
 * since the adapter will puke if it isn't 7 bytes (why have it
 * if you can't change it?).
 */
static Net_UltraTLAddress	wildcardAddress = 
			    {7, NET_ULTRA_TSAP_SIZE};

/*
 * Forward declarations.
 */

static Sync_Condition	dsndTestDone;
static int		dsndCount;

static char 		*GetStatusString _ARGS_ ((int status));
static void		InitQueues _ARGS_((NetUltraState *statePtr));
static void		StandardDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static void		ReadDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static void		EchoDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static ReturnStatus	NetUltraSendDgram _ARGS_((Net_Interface *interPtr,
				Net_Address *netAddressPtr, int count,
				int bufSize, Address buffer, Time *timePtr));
static void		DgramSendDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static void		OutputDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static void		SourceDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static ReturnStatus	NetUltraSource _ARGS_((Net_Interface *interPtr,
				Net_Address *netAddressPtr, int count,
				int bufSize, Address buffer, Time *timePtr));

/*
 * Macros for mapping between kernel, DVMA and VME addresses.
 */

#define	DVMA_TO_BUFFER(addr, statePtr) 		\
    ((Address) (((((unsigned int) (addr)) - 	\
    ((unsigned int) (statePtr)->buffersDVMA)) + \
    ((int) (statePtr)->buffers))))

#define BUFFER_TO_DVMA(addr, statePtr) 		\
    ((Address) ((((unsigned int) (addr)) - 	\
    ((unsigned int) (statePtr)->buffers)) + 	\
    ((unsigned int) (statePtr)->buffersDVMA)))

#define DVMA_TO_VME(addr, statePtr) 		\
    ((Address) (((unsigned int) (addr)) - 	\
    ((unsigned int) VMMACH_DMA_START_ADDR)))

#define VME_TO_DVMA(addr, statePtr) 		\
    ((Address) (((unsigned int) (addr)) + 	\
    ((unsigned int) VMMACH_DMA_START_ADDR)))

#define BUFFER_TO_VME(addr, statePtr) \
    ((Address) (DVMA_TO_VME(BUFFER_TO_DVMA(addr,statePtr), (statePtr))))

#define VME_TO_BUFFER(addr, statePtr) \
    ((Address) (DVMA_TO_BUFFER(VME_TO_DVMA((addr),statePtr),(statePtr))))

#define DVMA_ADDRESS(addr, statePtr) \
    ((unsigned int) (addr) < ((unsigned int)VMMACH_DMA_START_ADDR)? FALSE :TRUE)



/*
 *----------------------------------------------------------------------
 *
 * NetUltraInit --
 *
 *	Initialize the Ultranet VME adapter card.
 *
 * Results:
 *	SUCCESS if the Ultranet card was found and initialized.
 *	FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraInit(interPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
{
    unsigned int		ctrlAddr;
    NetUltraState		*statePtr;
    ReturnStatus		status = SUCCESS;
    int				zero = 0;
    NetUltraExtDiagCommand 	extDiagCmd;
    NetUltraInfoCommand		infoCmd;
    char			*buffer;

    /*
     * Make sure that we agree with the adapter as to the size of the 
     * command blocks and other structures.
     */
    assert(sizeof(Net_UltraAddress) == 8);
    assert(sizeof(Net_UltraTLAddress) == 16);
    assert(sizeof(Net_UltraHeader) == 56);
    assert(sizeof(NetUltraDMAInfo) == 16);
    assert(sizeof(NetUltraRequestHdr) == 16);
    assert(sizeof(NetUltraDatagramRequest) == 56);
    assert(sizeof(NetUltraStartRequest) == 112);
    assert(sizeof(NetUltraStopRequest) == 16);
    assert(sizeof(NetUltraRequest) == 112);
    assert(sizeof(NetUltraXRB) == 132);
    assert(sizeof(NetUltraInitCommand) == 64);
    assert(sizeof(NetUltraDiagCommand) == 36);
    assert(sizeof(NetUltraExtDiagCommand) == 24);
    assert(sizeof(NetUltraLoadCommand) == 24);
    assert(sizeof(NetUltraGoCommand) == 12);

    ctrlAddr = (unsigned int) interPtr->ctrlAddr;
    /*
     * If the address is physical (not in kernel's virtual address space)
     * then we have to map it in.
     */
    if (interPtr->virtual == FALSE) {
	ctrlAddr = (unsigned int) VmMach_MapInDevice((Address) ctrlAddr, 3);
    }
    statePtr = (NetUltraState *) malloc (sizeof(NetUltraState));
    bzero((char *) statePtr, sizeof(NetUltraState));
    /*
     * The first register is the interrupt register, then
     * the reset register, separated by 12 bytes.
     */
    statePtr->intrReg = (int *) ctrlAddr;
    statePtr->resetReg = (int *) (ctrlAddr + 0x10);
    /*
     * Now poke the reset register.
     */
    status = Mach_Probe(sizeof(int), (char *) &zero,
		(char *) statePtr->resetReg);
    if (status != SUCCESS) {
	/* 
	 * Got a bus error.
	 */
	goto exit;
    }
    if (netUltraDebug) {
	printf("NetUltraInit: adapter at 0x%x probed successfully\n",
	    statePtr->intrReg);
    }
    MACH_DELAY(NET_ULTRA_RESET_DELAY);
    statePtr->magic = NET_ULTRA_STATE_MAGIC;
    statePtr->flags = NET_ULTRA_STATE_EXIST | NET_ULTRA_STATE_EPROM |
			NET_ULTRA_STATE_NORMAL;
    statePtr->interPtr = interPtr;
    interPtr->interfaceData = (ClientData) statePtr;
    statePtr->tracePtr = statePtr->traceBuffer;
    statePtr->traceSequence = 0;
    /*
     * Get the adapter information.
     */
    status = NetUltraInfo(statePtr, &infoCmd);
    if (status == SUCCESS) {
	printf(
    "UltraNet adapter %d.%d, Revision %d, Option %d, Firmware %d, Serial %d\n",
	    infoCmd.hwModel, infoCmd.hwVersion, 
	    infoCmd.hwRevision, infoCmd.hwOption,
	    infoCmd.version, infoCmd.hwSerial);
	if (infoCmd.error == 0) {
	    printf("Ultranet adapter passed all diagnostics.\n");
	} else {
	    int	tmp = infoCmd.error;
	    int i;
	    printf(
	    "Ultranet adapter failed diagnostics (0x%x).\n", tmp);
	    printf("The following tests failed:\n");
	    for (i = 0; i < sizeof(int) * 8; i++) {
		if (tmp & 1) {
		    printf("%2d: %s\n", i+1, diagNames[i]);
		}
		tmp >>= 1;
	    }
	    status = FAILURE;
	    goto exit;
	}
    } else {
	printf("NetUltraInit: unable to send info command to adapter\n");
	goto exit;
    }
    /*
     * Send an extended diagnostics command to the adapter.
     */
    buffer = (char *) malloc(NET_ULTRA_EXT_DIAG_BUF_SIZE);
    status = NetUltraExtDiag(statePtr, FALSE, buffer, &extDiagCmd);
    free(buffer);
    if (status == SUCCESS) {
	if (extDiagCmd.error == 0) {
	    printf("Ultranet adapter passed all extended diagnostics.\n");
	} else {
	    int	tmp = extDiagCmd.error;
	    int i;
	    printf(
	    "Ultranet adapter failed extended diagnostics (0x%x).\n", tmp);
	    printf("The following tests failed:\n");
	    for (i = 0; i < sizeof(int) * 8; i++) {
		if (tmp & 1) {
		    printf("%2d: %s\n", i+1, extDiagNames[i]);
		}
		tmp >>= 1;
	    }
	    status = FAILURE;
	    goto exit;
	} 
    } else {
	printf(
    "NetUltraInit: unable to send extended diagnostics command to adapter\n");
	goto exit;
    }
    interPtr->ctrlAddr = (Address) ctrlAddr;
    statePtr->queuesInit = FALSE;
    InitQueues(statePtr);
    statePtr->priority = NET_ULTRA_INTERRUPT_PRIORITY;
    statePtr->requestLevel = NET_ULTRA_VME_REQUEST_LEVEL;
    statePtr->addressSpace = NET_ULTRA_VME_ADDRESS_SPACE;
    Mach_SetHandler(interPtr->vector, Net_Intr, (ClientData) interPtr);
    interPtr->init 	= NetUltraInit;
    interPtr->intr	= NetUltraIntr;
    interPtr->ioctl	= NetUltraIOControl;
    interPtr->reset 	= Net_UltraReset;
    interPtr->output 	= NetUltraOutput;
    interPtr->netType	= NET_NETWORK_ULTRA;
    interPtr->maxBytes	= NET_ULTRA_MAX_BYTES;
    interPtr->minBytes	= NET_ULTRA_MIN_BYTES;
    if (sizeof(NetUltraXRB) != 132) {
	printf("NetUltraInit: WARNING: NetUltraXRB is not 132 bytes!!!\n");
    }
exit:
    if (status != SUCCESS && statePtr != (NetUltraState *) NIL) {
	free((char *) statePtr);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraHardReset --
 *
 *	Does a hard reset of the Ultranet adapter. This means the adapter
 *	board goes back to its state right after power-up -- no 
 *	micro-code or anything.
 *
 * Results:
 *	SUCCESS if a reply was received properly, FAILURE otherwise
 *
 * Side effects:
 *	The ultranet adapter is initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraHardReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    NetUltraState	*statePtr;	/* State of the adapter. */
    ReturnStatus	status = SUCCESS;
    int			zero = 0;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    printf("Ultra: adapter reset.\n");
    status = Mach_Probe(sizeof(int), (char *) &zero, 
		(char *) statePtr->resetReg);
    if (status != SUCCESS) {
	/*
	 * Adapter is no longer responding.
	 */
	printf("NetUltraHardReset: adapter did not respond to reset!!\n");
	statePtr->flags = 0;
	return FAILURE;
    }
    MACH_DELAY(NET_ULTRA_RESET_DELAY);
    InitQueues(statePtr);
    /*
     * After a reset the adapter is in the EPROM mode.
     * This allows several additional commands (load, go, etc.) to be
     * sent to the adapter.
     */
    statePtr->flags = NET_ULTRA_STATE_EXIST | NET_ULTRA_STATE_EPROM;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraInitAdapter --
 *
 *	Allocate the various queues and send the initialization command
 *	to the adapter.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	The adapter board is initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraInitAdapter(statePtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
{
    ReturnStatus	status;
    NetUltraInitCommand	cmd;		/* Init command block. */
    Net_Interface	*interPtr;

    interPtr = statePtr->interPtr;
    MASTER_LOCK(&interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_GO)) {
	if (netUltraDebug) {
	    printf("NetUltraInitAdapter: device is not started.\n");
	}
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &cmd, sizeof(cmd));
    cmd.opcode = NET_ULTRA_INIT_OPCODE;
    cmd.toAdapterAddr = (int) DVMA_TO_VME(statePtr->firstToAdapterPtr, 
				    statePtr);
    cmd.toAdapterNum = statePtr->lastToAdapterPtr - 
			statePtr->firstToAdapterPtr + 1;
    cmd.toHostAddr = (int) DVMA_TO_VME(statePtr->firstToHostPtr, statePtr);
    cmd.toHostNum = statePtr->lastToHostPtr - statePtr->firstToHostPtr + 1;
    cmd.XRBSize = sizeof(NetUltraXRB);
    cmd.priority = (char) statePtr->priority;
    cmd.vector = (char) interPtr->vector;
    cmd.requestLevel = (char) statePtr->requestLevel;
    cmd.addressSpace = (char) statePtr->addressSpace;
    status = NetUltraSendCmd(statePtr, NET_ULTRA_INIT_OK, sizeof(cmd), 
		    (Address) &cmd);
    if (status != SUCCESS) {
	if (cmd.reply == NET_ULTRA_BAD_SIZE) {
	    printf("NetUltraInitAdapter: adapter disagrees on size of XRB\n");
	}
	goto exit;
    }
    statePtr->flags &= ~NET_ULTRA_STATE_GO;
    statePtr->flags |= NET_ULTRA_STATE_INIT;
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return status;
} 

/*
 *----------------------------------------------------------------------
 *
 * NetUltraStart--
 *
 *	Send a start request to the board.
 *	Assumes the interface mutex is held.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	A start request is sent to the board.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraStart(statePtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
{
    NetUltraRequest		request;
    NetUltraStartRequest	*startPtr;
    NetUltraRequestHdr		*hdrPtr;
    Net_Interface		*interPtr;
    int				i;
    int				power = 0;
    int				tmp;
    ReturnStatus		status = SUCCESS;
    static int			sequence = 0;
    Sync_Condition		startComplete;

    interPtr = statePtr->interPtr;
    startPtr = &request.start;
    hdrPtr = &startPtr->hdr;
    if (!(NET_ADDRESS_COMPARE(interPtr->netAddress[NET_PROTO_RAW],
		netZeroAddress))){
	printf(
"NetUltraStart: can't send start command - adapter address not set\n");
	status = FAILURE;
	goto exit;
    }
    if (statePtr->flags & NET_ULTRA_STATE_START) {
	printf("NetUltraStart: adapter already started.\n");
	status = FAILURE;
	goto exit;
    }
    if (!(statePtr->flags & NET_ULTRA_STATE_INIT)) {
	printf("NetUltraStart: adapter not initialized.\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    hdrPtr->cmd = NET_ULTRA_NEW_START_REQ;
    startPtr->sequence = sequence;
    sequence++;
    startPtr->hostType = NET_ULTRA_START_HOSTTYPE_SUN;
    /*
     * Determine which power of 2 equals the maximum packet size. 
     */
    tmp = interPtr->maxBytes;
    for (i = 0; i < sizeof(int) * 8; i++) {
	if (tmp & 1) {
	    power = i;
	}
	tmp >>= 1;
    }
    if (1 << power > interPtr->maxBytes) {
	power++;
    }
    startPtr->hostMaxBytes = power;
    /* 
     * This part is kind of goofy.  The adapter expects the network
     * address to be 7 bytes long (the first byte is missing). We have
     * to bcopy the netAddress minus its first byte.
     */
    startPtr->netAddressSize = 7;
    bcopy((char *) &interPtr->netAddress[NET_PROTO_RAW].generic.data[1],
	(char *) &startPtr->netAddressBuf, sizeof(Net_Address)-1);
    startPtr->netAddressBuf[0] = 0x49;
    startPtr->netAddressBuf[5] = 0xfe;
    status = NetUltraSendReq(statePtr, StandardDone, 
		(ClientData) &startComplete, FALSE, 
		0, (Net_ScatterGather *) NIL, sizeof(NetUltraStartRequest), 
		&request);
    if (status != SUCCESS) {
	printf("NetUltraStart: unable to send start request\n");
	goto exit;
    }
    Sync_MasterWait(&(startComplete), &(interPtr->mutex), FALSE);
    if (hdrPtr->status & NET_ULTRA_STATUS_OK) {
	if (netUltraDebug) {
	    printf("NetUltraStart: hdrPtr->status = %d : %s\n",
		hdrPtr->status, GetStatusString(hdrPtr->status));
	}
	if (netUltraDebug) {
	    printf("Ultranet adapter started\n");
	    printf("Ucode %d\n", startPtr->ucodeRel);
	    printf("Adapter type %d, adapter hw %d\n", startPtr->adapterType,
		startPtr->adapterHW);
	    printf("Max VC = %d\n", startPtr->maxVC);
	    printf("Max DRCV = %d\n", startPtr->maxDRCV);
	    printf("Max DSND = %d\n", startPtr->maxDSND);
	    printf("Slot = %d\n", startPtr->slot);
	    printf("Speed = %d\n", startPtr->speed);
	    printf("Max bytes = %d\n", startPtr->adapterMaxBytes);
	}
	if (startPtr->maxDRCV < statePtr->maxReadPending) {
	    printf("NetUltraStart: WARNING: max pending reads reset to %d\n",
		startPtr->maxDRCV);
	    statePtr->maxReadPending = startPtr->maxDRCV;
	}
	if (startPtr->maxDSND < statePtr->maxWritePending) {
	    printf("NetUltraStart: WARNING: max pending writes reset to %d\n",
		startPtr->maxDSND);
	    statePtr->maxWritePending = startPtr->maxDSND;
	}
	if (startPtr->adapterMaxBytes < power) {
	    interPtr->maxBytes = 1 << power;
	    printf("NetUltraStart: WARNING: max bytes reset to %d\n",
		interPtr->maxBytes);
	}
	statePtr->flags = (NET_ULTRA_STATE_EXIST | NET_ULTRA_STATE_START |
			    NET_ULTRA_STATE_NORMAL);
	/*
	 * Now queue up pending reads. 
	 */
	for (i = 0; i < statePtr->maxReadPending; i++) {
	    List_Links		*itemPtr;
	    Address	buffer;
	    itemPtr = List_First(statePtr->freeBufferList);
	    if (itemPtr == statePtr->freeBufferList) {
		panic("NetUltraOutput: list screwup\n");
	    }
	    List_Remove(itemPtr);
	    buffer = BUFFER_TO_DVMA(itemPtr, statePtr);
	    status = NetUltraPendingRead(interPtr, statePtr->bufferSize,
			buffer);
	    if (status != SUCCESS) {
		printf("NetUltraStart: could not queue pending read.\n");
	    }
	}
    } else {
	printf("NetUltraStart: start command failed <0x%x> : %s\n",
	    hdrPtr->status, GetStatusString(hdrPtr->status));
    }
exit:
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * StandardDone --
 *
 *	This is a callback routine that is called from the interrupt
 *	handler when "standard" commands complete. It copies the request
 *	from the XRB in the queue to the request structure pointed
 *	to by the XRB.  This allows the XRB to be used again.
 *	If the data field in the info structure is not NIL then it
 *	is assumed to be a pointer to a condition variable and a
 *	broadcast is done on that variable.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *	
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
StandardDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    bcopy((char *) &infoPtr->xrbPtr->request, (char *) infoPtr->requestPtr,
	infoPtr->requestSize);
    if (infoPtr->doneData != (ClientData) NIL) {
	Sync_Broadcast((Sync_Condition *) infoPtr->doneData);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraStop--
 *
 *	Send a stop request to the board.
 *	Assumes the interface mutex is held.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	The adapter abandons what it is doing.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraStop(statePtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
{
    NetUltraRequest		request;
    NetUltraStopRequest		*stopPtr;
    NetUltraRequestHdr		*hdrPtr;
    Net_Interface		*interPtr;
    ReturnStatus		status = SUCCESS;
    Sync_Condition		stopComplete;

    interPtr = statePtr->interPtr;
    stopPtr = &request.stop;
    hdrPtr = &stopPtr->hdr;
    if (!(statePtr->flags & NET_ULTRA_STATE_START)) {
	printf("NetUltraStop: adapter not started.\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    hdrPtr->cmd = NET_ULTRA_STOP_REQ;
    status = NetUltraSendReq(statePtr, StandardDone, 
		(ClientData) &stopComplete, FALSE,
		0, (Net_ScatterGather *) NIL, sizeof(NetUltraStopRequest), 
		&request);
    if (status != SUCCESS) {
	printf("NetUltraStop: unable to send stop request\n");
	goto exit;
    }
    Sync_MasterWait(&(stopComplete), &(interPtr->mutex), FALSE);
    if (hdrPtr->status & NET_ULTRA_STATUS_OK) {
	if (netUltraDebug) {
	    printf("NetUltraStop: hdrPtr->status = %d : %s\n",
		hdrPtr->status, GetStatusString(hdrPtr->status));
	}
    } else {
	printf("NetUltraStop: stop command failed <0x%x> : %s\n",
	    hdrPtr->status, GetStatusString(hdrPtr->status));
    }
exit:
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraReset --
 *
 *	Resets the board by sending it a stop command followed by a
 *	start command.  The XRB queues are also initialized.
 *	Assumes the interface mutex is held.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    ReturnStatus	status = SUCCESS;
    NetUltraState	*statePtr;	/* State of the adapter. */

    statePtr = (NetUltraState *) interPtr->interfaceData;
    status = NetUltraStop(statePtr);
    if (status != NULL) {
	printf("NetUltraReset: stop failed\n");
	return status;
    }
    InitQueues(statePtr);
    status = NetUltraStart(statePtr);
    if (status != NULL) {
	printf("NetUltraReset: start failed\n");
	return status;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_UltraReset --
 *
 *	This is a version of the reset routine that can be called
 * 	from outside the module since it locks the mutex.
 *
 * Results:
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Net_UltraReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    MASTER_LOCK(&interPtr->mutex);
    (void) NetUltraReset(interPtr);
    MASTER_UNLOCK(&interPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_UltraHardReset --
 *
 *	This is a version of the hard reset routine that can be called from
 * 	outside the net module because it does not assume that a
 *	lock is held (or interrupts are disabled) when it is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The adapter is reset.
 *
 *----------------------------------------------------------------------
 */

void
Net_UltraHardReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    MASTER_LOCK(&interPtr->mutex);

    NetUltraHardReset(interPtr);

    MASTER_UNLOCK(&interPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * NetUltraDiag --
 *
 *	Send a diagnostic command to the adapter.
 *
 * Results:
 *	SUCCESS if the adapter processed the command correctly,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	The adapter is sent a diagnostic command.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraDiag(statePtr, cmdPtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    NetUltraDiagCommand	*cmdPtr;	/* Diagnostic command block. */
{
    ReturnStatus		status = SUCCESS;
    NetUltraDiagCommand		tmpCmd;	
    int				inSize;
    int				outSize;
    int				fmtStatus;

    MASTER_LOCK(&statePtr->interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_EPROM)) {
	if (netUltraDebug) {
	    printf("NetUltraDiag: device is not in EPROM state.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (netUltraDebug) {
	printf("Sending diagnostic command to adapter.\n");
    }
    bzero((char *) &tmpCmd, sizeof(tmpCmd));
    tmpCmd.opcode = NET_ULTRA_DIAG_OPCODE;
    status = NetUltraSendCmd(statePtr, NET_ULTRA_DIAG_OK, 
		    sizeof(tmpCmd), (Address) &tmpCmd);
    if (status != SUCCESS) {
	goto exit;
    }
    cmdPtr->reply = tmpCmd.reply;
    cmdPtr->version = tmpCmd.version;
    cmdPtr->error = tmpCmd.error;
    /*
     * The hardware related fields are returned in Intel 386 byte-order.
     */
    inSize = 5 * sizeof(int);
    outSize = inSize;
    fmtStatus = Fmt_Convert("w5", NET_ULTRA_FORMAT, &inSize, 
			(char *) &(tmpCmd.hwModel), mach_Format, &outSize, 
			(char *) &(cmdPtr->hwModel));
    if (fmtStatus != 0) {
	printf("NetUltraInfo: Format returned %d\n");
	status = FAILURE;
	goto exit;
    }
exit:
    MASTER_UNLOCK(&statePtr->interPtr->mutex)
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraInfo --
 *
 *	Send an info command to the adapter. The results are the same
 * 	as the diagnostic command, except the diagnostics are not 
 *	actually run.  Presumably we are fetching the results of the
 *	last time diagnostics were run, probably due to a reset.
 *
 * Results:
 *	SUCCESS if the adapter processed the command correctly,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	The adapter is sent an info command.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraInfo(statePtr, cmdPtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    NetUltraInfoCommand	*cmdPtr;	/* Info command block. */
{
    ReturnStatus		status = SUCCESS;
    int				fmtStatus;
    NetUltraInfoCommand		tmpCmd;
    int				inSize;
    int				outSize;

    MASTER_LOCK(&statePtr->interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_EPROM)) {
	if (netUltraDebug) {
	    printf("NetUltraInfo: device is not in EPROM state.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (netUltraDebug) {
	printf("Sending Info command to adapter.\n");
    }
    bzero((char *) &tmpCmd, sizeof(NetUltraInfoCommand));
    tmpCmd.opcode = NET_ULTRA_INFO_OPCODE;
    status = NetUltraSendCmd(statePtr, NET_ULTRA_INFO_OK, 
		    sizeof(NetUltraInfoCommand), (Address) &tmpCmd);
    if (status != SUCCESS) {
	goto exit;
    }
    cmdPtr->reply = tmpCmd.reply;
    cmdPtr->version = tmpCmd.version;
    cmdPtr->error = tmpCmd.error;
    /*
     * The hardware related fields are returned in Intel 386 byte-order.
     */
    inSize = 5 * sizeof(int);
    outSize = inSize;
    fmtStatus = Fmt_Convert("w5", NET_ULTRA_FORMAT, &inSize, 
			(char *) &(tmpCmd.hwModel), mach_Format, &outSize, 
			(char *) &(cmdPtr->hwModel));
    if (fmtStatus != 0) {
	printf("NetUltraInfo: Format returned %d\n");
	status = FAILURE;
    }
exit:
    MASTER_UNLOCK(&statePtr->interPtr->mutex)
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraExtDiag --
 *
 *	Send an extended diagnostic command to the adapter.
 *
 * Results:
 *	SUCCESS if the adapter processed the command correctly,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	The adapter is sent an extended diagnostic command.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraExtDiag(statePtr, external, buffer, cmdPtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    Boolean		external;	/* TRUE => use external loopback
					 * adapter. */
    char		buffer[]; 	/* Used for VME tests. */
    NetUltraExtDiagCommand	*cmdPtr;	/* Extended diagnostic command
						 * block. */
{
    ReturnStatus		status = SUCCESS;
    Address			bufAddress;

    MASTER_LOCK(&statePtr->interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_EPROM)) {
	if (netUltraDebug) {
	    printf("NetUltraExtDiag: device is not in EPROM state.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (netUltraDebug) {
	printf("Sending extended diagnostic command to adapter.\n");
    }
    bufAddress = (Address) VmMach_DMAAlloc(NET_ULTRA_EXT_DIAG_BUF_SIZE, buffer);
    if (bufAddress == (Address) NIL) {
	printf("NetUltraExtDiag: unable to allocate buffer in DMA space.\n");
	status = FAILURE;
    }
    bzero((char *) cmdPtr, sizeof(*cmdPtr));
    cmdPtr->opcode = NET_ULTRA_EXT_DIAG_OPCODE;
    cmdPtr->externalLoopback = (external == TRUE) ? 1 : 0;
    cmdPtr->bufferAddress = (int) DVMA_TO_VME(bufAddress, statePtr);
    status = NetUltraSendCmd(statePtr, NET_ULTRA_EXT_DIAG_OK, sizeof(*cmdPtr), 
		(Address) cmdPtr);
    VmMach_DMAFree(NET_ULTRA_EXT_DIAG_BUF_SIZE, bufAddress);
exit:
    MASTER_UNLOCK(&statePtr->interPtr->mutex);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraLoad --
 *
 *	Download a block of ucode onto the adapter.
 *
 * Results:
 *	SUCCESS if the command was processed properly
 *
 * Side effects:
 *	Data is loaded into the adapter ram.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraLoad(statePtr, address, length, ucodePtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    int			address;	/* Load address of ucode. */
    int			length;		/* Size of ucode. */
    Address		ucodePtr;	/* Ucode buffer. */
{
    NetUltraLoadCommand		cmd;
    int				inSize;
    int				outSize;
    int				fmtStatus;
    Address			dmaAddress;
    ReturnStatus		status = SUCCESS;

    MASTER_LOCK(&statePtr->interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_EPROM)) {
	if (netUltraDebug) {
	    printf("NetUltraExtDiag: device is not in EPROM state.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (netUltraDebug) {
	printf("Sending load command to adapter.\n");
	printf("Address = 0x%x, length = %d\n", address, length);
    }
    /*
     * Convert the address and length to Intel format.
     */
    inSize = sizeof(int);
    outSize = sizeof(int);
    fmtStatus = Fmt_Convert("w", mach_Format, &inSize,
		    (char *) &address, NET_ULTRA_FORMAT, &outSize,
		    (char *) &(cmd.loadAddress));
    if (fmtStatus != 0) {
	printf("NetUltraLoad: Fmt_Convert returned %d\n",
	    fmtStatus);
	status = FAILURE;
	goto exit;
    }
    inSize = sizeof(int);
    outSize = sizeof(int);
    fmtStatus = Fmt_Convert("w", mach_Format, &inSize,
		    (char *) &length, NET_ULTRA_FORMAT, &outSize,
		    (char *) &(cmd.length));
    if (fmtStatus != 0) {
	printf("NetUltraLoad: Fmt_Convert returned %d\n",
	    fmtStatus);
	status = FAILURE;
	goto exit;
    }
    cmd.opcode = NET_ULTRA_LOAD_OPCODE;
    cmd.reply = 0;
    /*
     * Map the ucode into DVMA space.
     */
    dmaAddress = (Address) VmMach_DMAAlloc(length, ucodePtr); 
    cmd.dataAddress = (unsigned long) DVMA_TO_VME(dmaAddress, statePtr);
    status = NetUltraSendCmd(statePtr, NET_ULTRA_LOAD_OK, sizeof(cmd), 
		(Address) &cmd);
    VmMach_DMAFree(length, dmaAddress);
    if (status != SUCCESS) {
	goto exit;
    }
    statePtr->flags |= NET_ULTRA_STATE_LOAD;
exit:
    MASTER_UNLOCK(&statePtr->interPtr->mutex)
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraGo --
 *
 *	Send the go command to the adapter to start its processor
 *	running the ucode.
 *
 * Results:
 *	SUCCESS if it worked, FAILURE otherwise
 *
 * Side effects:
 *	The adapter cpu begins executing.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraGo(statePtr, address)
    NetUltraState	*statePtr;	/* State of the adapter. */
    int			address;	/* Start address. */
{
    NetUltraGoCommand		cmd;
    int				inSize;
    int				outSize;
    int				fmtStatus;
    ReturnStatus		status = SUCCESS;

    MASTER_LOCK(&statePtr->interPtr->mutex);
    if (!(statePtr->flags & NET_ULTRA_STATE_EPROM)) {
	if (netUltraDebug) {
	    printf("NetUltraExtDiag: device is not in EPROM state.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (!(statePtr->flags & NET_ULTRA_STATE_LOAD)) {
	if (netUltraDebug) {
	    printf("NetUltraGo: ucode not downloaded.\n");
	}
	status = FAILURE;
	goto exit;
    }
    if (netUltraDebug) {
	printf("Sending go command to adapter.\n");
    }
    /*
     * Convert the address into Intel format.
     */
    inSize = sizeof(int);
    outSize = sizeof(int);
    fmtStatus = Fmt_Convert("w", mach_Format, &inSize,
		    (char *) &address, NET_ULTRA_FORMAT, &outSize,
		    (char *) &(cmd.startAddress));
    if (fmtStatus != 0) {
	printf("NetUltraGo: Fmt_Convert returned %d\n",
	    fmtStatus);
	status = FAILURE;
	goto exit;
    }
    cmd.opcode = NET_ULTRA_GO_OPCODE;
    cmd.reply = 0;
    status = NetUltraSendCmd(statePtr, NET_ULTRA_GO_OK, sizeof(cmd), 
		(Address) &cmd);
    if (status != SUCCESS) {
	goto exit;
    }
    statePtr->flags &= ~NET_ULTRA_STATE_LOAD;
    statePtr->flags |= NET_ULTRA_STATE_GO;
    statePtr->flags &= ~NET_ULTRA_STATE_EPROM;
exit:
    MASTER_UNLOCK(&statePtr->interPtr->mutex)
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * NetUltraSendCmd --
 *
 *	Send a command to the adapter.
 *
 * Results:
 *	SUCCESS if the adapter responded to the command,
 *	DEV_TIMEOUT if the adapter did not respond
 *
 * Side effects:
 *	A command is sent to the adapter.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraSendCmd(statePtr, ok,size, cmdPtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    int			ok;		/* Value of reply field if command
					 * was successful. */
    int			size;		/* Size of command block. */
    Address		cmdPtr;		/* The command block. */
{
    struct StdCmd {
	volatile int		opcode;
	volatile int		reply;
    } *stdCmdPtr;
    ReturnStatus	status = SUCCESS;

    stdCmdPtr = (struct StdCmd *) cmdPtr;
    if (netUltraDebug) {
	printf("NetUltraSendCmd: sending command %d to adapter\n",
	    stdCmdPtr->opcode);
	printf("NetUltraSendCmd: cmdPtr = 0x%x\n", cmdPtr);
    }
    if ((int) cmdPtr & 0x3) {
	panic("NetUltraSendCmd: command not aligned on a word boundary\n");
    }
    stdCmdPtr->reply = 0;
    cmdPtr = VmMach_DMAAlloc(size, cmdPtr);
    if (cmdPtr == (Address) NIL) {
	panic("NetUltraSendCmd: can't allocate DMA space for command\n");
    }
    if (netUltraDebug) {
	printf("NetUltraSendCmd: cmdPtr mapped into DMA space\n");
	printf("NetUltraSendCmd: cmdPtr = 0x%x\n", cmdPtr);
    }
    *statePtr->intrReg = 
	(int) DVMA_TO_VME(cmdPtr,statePtr);
    WAIT_FOR_REPLY(&stdCmdPtr->reply, NET_ULTRA_DELAY);
    VmMach_DMAFree(size, cmdPtr);
    if (stdCmdPtr->reply == 0) {
	printf("NetUltraSendCmd: adapter timed out\n");
	status = DEV_TIMEOUT;
    } else if (stdCmdPtr->reply == ok) {
	status = SUCCESS;
    } else {
	printf("NetUltraSendCmd: adapter returned %d\n", stdCmdPtr->reply);
	status = FAILURE;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraIntr --
 *
 *	Handle an interrupt from the Ultranet adapter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
NetUltraIntr(interPtr, polling)
    Net_Interface	*interPtr;	/* Interface to process. */
    Boolean		polling;	/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    NetUltraXRB		*xrbPtr;
    NetUltraXRB		*nextXRBPtr;
    NetUltraState	*statePtr;
    NetUltraDMAInfo	*dmaPtr;
    NetUltraRequestHdr  *hdrPtr;
    NetUltraXRBInfo	*infoPtr;
    int			processed;
    NetUltraTraceInfo	*tracePtr;
    Address		buffer;

    MASTER_LOCK(&interPtr->mutex);
#ifndef CLEAN
    if (netUltraDebug) {
	printf("Received an Ultranet interrupt.\n");
    }
#endif
    statePtr = (NetUltraState *) interPtr->interfaceData;
    xrbPtr = statePtr->nextToHostPtr;
#ifndef CLEAN
    if (netUltraTrace) {
	NEXT_TRACE(statePtr, &tracePtr);
	tracePtr->event = INTERRUPT;
	Timer_GetCurrentTicks(&tracePtr->ticks);
    }
#endif
    processed = 0;
    while(xrbPtr->filled != 0) {
	/*
	 * Compute the next xrb to the host.
	 */
	if (xrbPtr == statePtr->lastToHostPtr) {
	    nextXRBPtr = statePtr->firstToHostPtr;
	} else {
	    nextXRBPtr = xrbPtr + 1;
	}
	statePtr->nextToHostPtr = nextXRBPtr;
	dmaPtr = &xrbPtr->dma;
	if (!(dmaPtr->cmd & NET_ULTRA_DMA_CMD_FROM_ADAPTER)) {
	    printf("NetUltraIntr: cmd (0x%x) not from adapter?\n", dmaPtr->cmd);
	    goto endLoop;
	}
	if ((dmaPtr->cmd & NET_ULTRA_DMA_CMD_MASK) != NET_ULTRA_DMA_CMD_XRB) {
	    printf("NetUltraIntr: dmaPtr->cmd = 0x%x\n", dmaPtr->cmd);
	    goto endLoop;
	}
	hdrPtr = (NetUltraRequestHdr *) &xrbPtr->request;
	infoPtr = hdrPtr->infoPtr;
	if (infoPtr->flags & NET_ULTRA_INFO_PENDING) {
#ifndef CLEAN
	    if (netUltraDebug) {
		printf("NetUltraIntr: processing 0x%x\n", infoPtr);
	    }
	    if (netUltraTrace) {
		NEXT_TRACE(statePtr, &tracePtr);
		tracePtr->event = PROCESS_XRB;
		tracePtr->index = xrbPtr - statePtr->firstToHostPtr;
		tracePtr->infoPtr = infoPtr;
		Timer_GetCurrentTicks(&tracePtr->ticks);
	    }
#endif
	    infoPtr->xrbPtr = xrbPtr;
	    if (statePtr->flags & NET_ULTRA_STATE_STATS) {
		if (hdrPtr->cmd == NET_ULTRA_DGRAM_RECV_REQ) {
		    statePtr->stats.packetsReceived++;
		    statePtr->stats.bytesReceived += hdrPtr->size;
		    statePtr->stats.receivedHistogram[hdrPtr->size >> 10]++;
		}
	    }
	    /*
	     * Mark the info as not pending.
	     * Clearing the pending bit ensures
	     * that this packet does not get processed twice, since the
	     * master lock around the interface will get released before
	     * calling the RPC system, so that the RPC system can
	     * output a packet. Do not
	     * clear the filled bit since we are using the contents of
	     * the xrb and we don't want the adapter to overwrite it
	     * yet. 
	     */
	    infoPtr->flags &= ~NET_ULTRA_INFO_PENDING;
	    if (infoPtr->doneProc != NILPROC) {
		(infoPtr->doneProc)(interPtr, infoPtr);
	    }
	    if (infoPtr->flags & NET_ULTRA_INFO_STD_BUFFER) {
		if (infoPtr->flags & NET_ULTRA_INFO_REMAP) {
		    VmMach_DMAFree(hdrPtr->size, 
			VME_TO_DVMA(hdrPtr->buffer, statePtr));
		    buffer = infoPtr->buffer;
		} else {
		    buffer = VME_TO_BUFFER(hdrPtr->buffer, statePtr);
		}
		List_InitElement((List_Links *) buffer);
		List_Insert((List_Links *) buffer,
		       LIST_ATREAR(statePtr->freeBufferList));
	    }
	    List_Remove((List_Links *) infoPtr);
	    List_Insert((List_Links *) infoPtr, 
		LIST_ATREAR(statePtr->freeXRBInfoList));
	    processed++;
	} else {
#ifndef CLEAN
	    if (netUltraTrace) {
		NEXT_TRACE(statePtr, &tracePtr);
		tracePtr->event = INFO_NOT_PENDING;
		tracePtr->index = xrbPtr - statePtr->firstToHostPtr;
		tracePtr->infoPtr = infoPtr;
	    }
#endif
	}
endLoop: 
	xrbPtr->filled = 0;
	xrbPtr = nextXRBPtr;
	/*
	 * Check and see if there is a free xrb in the "to adapter"
	 * queue.  Presumably the appearance of one in the "to host"
	 * queue implies one freed up in the "to adapter" queue.
	 */
	if ((statePtr->nextToAdapterPtr->filled != 0) && 
	    (statePtr->toAdapterAvail.waiting == TRUE)) {
	    Sync_Broadcast(&statePtr->toAdapterAvail);
	}
    }
#ifndef CLEAN
    if (processed == 0) {
	if (netUltraDebug) {
	    printf("NetUltraIntr: didn't process any packets.\n");
	}
    }
#endif
    MASTER_UNLOCK(&interPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraIOControl --
 *
 *	Handle ioctls for the Ultranet adapter..
 *
 * Results:
 *	SUCCESS if the ioctl was performed successfully, a standard
 *	Sprite error code otherwise.
 *
 * Side effects:
 *	Commands may be sent to the adapter and the adapter state
 *	may change.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
NetUltraIOControl(interPtr, ioctlPtr, replyPtr)
    Net_Interface *interPtr;	/* Interface on which to perform ioctl. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ReturnStatus status = SUCCESS;
    int	fmtStatus;
    int inSize;
    int outSize;
    NetUltraState	*statePtr = (NetUltraState *) interPtr->interfaceData;

    if (netUltraDebug) {
	printf("NetUltraIOControl: command = %d\n", ioctlPtr->command);
    }
    if ((ioctlPtr->command & ~0xffff) != IOC_ULTRA) {
	return DEV_INVALID_ARG;
    }
    switch(ioctlPtr->command) {
	case IOC_ULTRA_SET_FLAGS:
	case IOC_ULTRA_RESET_FLAGS:
	case IOC_ULTRA_GET_FLAGS:
	    return GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_ULTRA_CLR:
	case IOC_ULTRA_INT:
	case IOC_ULTRA_WFI:
	    return DEV_INVALID_ARG;
	    break;
	case IOC_ULTRA_DUMP:
	    return GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_ULTRA_DEBUG: {
	    int	value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_ULTRA_DEBUG parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    netUltraDebug = value;
	    break;
	}
	case IOC_ULTRA_TRACE: {
	    int	value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_ULTRA_TRACE parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    netUltraTrace = value;
	    break;
	}
	case IOC_ULTRA_RESET: 
	    Net_UltraReset(interPtr);
	    break;
	case IOC_ULTRA_HARD_RESET:
	    Net_UltraHardReset(interPtr);
	    break;
	case IOC_ULTRA_GET_ADAP_INFO: {
	    NetUltraInfoCommand		cmd;
	    Dev_UltraAdapterInfo	info;

	    status = NetUltraInfo(statePtr, &cmd);
	    if (status != SUCCESS) {
		return status;
	    }
	    info.version = cmd.version;
	    info.error = cmd.error;
	    info.hwModel = cmd.hwModel;
	    info.hwVersion = cmd.hwVersion;
	    info.hwRevision = cmd.hwRevision;
	    info.hwOption = cmd.hwOption;
	    info.hwSerial = cmd.hwSerial;
	    inSize = sizeof(info);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{w7}", mach_Format, &inSize,
		(Address) &info, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}
	case IOC_ULTRA_DIAG: {
	    NetUltraDiagCommand		cmd;
	    Dev_UltraDiag		diag;

	    status = NetUltraDiag(statePtr, &cmd);
	    if (status != SUCCESS) {
		return status;
	    }
	    diag.version = cmd.version;
	    diag.error = cmd.error;
	    diag.hwModel = cmd.hwModel;
	    diag.hwVersion = cmd.hwVersion;
	    diag.hwRevision = cmd.hwRevision;
	    diag.hwOption = cmd.hwOption;
	    diag.hwSerial = cmd.hwSerial;
	    inSize = sizeof(diag);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{w7}", mach_Format, &inSize,
		(Address) &diag, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}
	case IOC_ULTRA_EXTENDED_DIAG: {
	    NetUltraExtDiagCommand	cmd;
	    Dev_UltraExtendedDiag	extDiag;
	    char			*buffer;

	    buffer = (char *) malloc(NET_ULTRA_EXT_DIAG_BUF_SIZE);
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(extDiag);
	    fmtStatus = Fmt_Convert("{w3}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &extDiag);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    status = NetUltraExtDiag(statePtr, extDiag.externalLoopback, 
			    buffer,&cmd);
	    free(buffer);
	    if (status != SUCCESS) {
		return status;
	    }
	    extDiag.version = cmd.version;
	    extDiag.error = cmd.error;
	    inSize = sizeof(extDiag);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{w3}", mach_Format, &inSize,
		(Address) &extDiag, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}
	case IOC_ULTRA_LOAD: {
	    Dev_UltraLoad		load;
	    Address			ucodePtr;
	    inSize = 2 * sizeof(int);
	    outSize = sizeof(load);
	    fmtStatus = Fmt_Convert("w2", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &load);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    /*
	     * This assumes that there isn't any padding between the
	     * two integers in NetUltraLoadCommand and the data buffer.
	     */
	    ucodePtr = (Address) ioctlPtr->inBuffer + inSize;
	    status = NetUltraLoad(statePtr, load.address, load.length,
			    ucodePtr);
	    break;
	}
	case IOC_ULTRA_GO: {
	    Dev_UltraGo			go;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(go);
	    fmtStatus = Fmt_Convert("{w1}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &go.address);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    status = NetUltraGo(statePtr, go.address);
	    break;
	}
	case IOC_ULTRA_INIT: {
	    status = NetUltraInitAdapter(statePtr);
	    break;
	}
	case IOC_ULTRA_START: {
	    MASTER_LOCK(&interPtr->mutex);
	    status = NetUltraStart(statePtr);
	    MASTER_UNLOCK(&interPtr->mutex);
	    break;
	}
	case IOC_ULTRA_ADDRESS: {
	    /* 
	     * Set the adapter address.  This is not the normal way to
	     * do this (usually happens during route installation) but
	     * it is useful for debugging. 
	     */
	    Net_UltraAddress	address;
	    int			group;
	    int			unit;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(Net_UltraAddress);
	    fmtStatus = Fmt_Convert("{b8}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &address);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    Net_UltraAddressGet(&address, &group, &unit);
	    printf("Setting ultra address to %d/%d\n", group, unit); 
	    MASTER_LOCK(&interPtr->mutex);
	    interPtr->netAddress[NET_PROTO_RAW].ultra = address;
	    MASTER_UNLOCK(&interPtr->mutex);
	    break;
	}
	case IOC_ULTRA_SEND_DGRAM: {
	    Dev_UltraSendDgram		dgram;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(dgram);
	    fmtStatus = Fmt_Convert("{w2{w2}wb*}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &dgram);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    status = NetUltraSendDgram(interPtr, &dgram.address, dgram.count,
			dgram.size, 
			(dgram.useBuffer == TRUE) ? (Address) dgram.buffer :
			(Address) NIL, &dgram.time);
	    inSize = sizeof(dgram);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{w2{w2}wb*}", mach_Format, &inSize,
		(Address) &dgram, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}
	case IOC_ULTRA_ECHO: {
	    Dev_UltraEcho		echo;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(echo);
	    fmtStatus = Fmt_Convert("{w}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &echo);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    if (echo.echo == TRUE) {
		statePtr->flags |= NET_ULTRA_STATE_ECHO;
		statePtr->flags &= ~NET_ULTRA_STATE_NORMAL;
	    } else {
		statePtr->flags &= ~NET_ULTRA_STATE_ECHO;
		statePtr->flags |= NET_ULTRA_STATE_NORMAL;
	    }
	    break;
	}
	case IOC_ULTRA_SOURCE: {
	    Dev_UltraSendDgram		dgram;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(dgram);
	    fmtStatus = Fmt_Convert("{w2{w2}wb*}", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &dgram);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    status = NetUltraSource(interPtr, &dgram.address, dgram.count,
			dgram.size, 
			(dgram.useBuffer == TRUE) ? (Address) dgram.buffer :
			(Address) NIL, &dgram.time);
	    inSize = sizeof(dgram);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{w2{w2}wb*}", mach_Format, &inSize,
		(Address) &dgram, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}
	case IOC_ULTRA_SINK: {
	    Dev_UltraSink		sink;
	    int				value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_ULTRA_TRACE parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    outSize = ioctlPtr->outBufSize;
	    sink.packets = packetsSunk;
	    Timer_SubtractTicks(sinkEndTime, sinkStartTime, &sink.time);
	    Timer_TicksToTime(sink.time, &sink.time);
	    inSize = sizeof(sink);
	    fmtStatus = Fmt_Convert("w{w2}", mach_Format, &inSize,
			    (Address) &sink, ioctlPtr->format, &outSize,
			    ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		printf("Format of IOC_ULTRA_TRACE parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    if (value > 0) {
		statePtr->flags |= NET_ULTRA_STATE_SINK;
		statePtr->flags &= ~NET_ULTRA_STATE_NORMAL;
		packetsSunk = 0;
	    } else {
		statePtr->flags &= ~NET_ULTRA_STATE_SINK;
		statePtr->flags |= NET_ULTRA_STATE_NORMAL;
	    }
	    break;
	}
	case IOC_ULTRA_COLLECT_STATS: {
	    int		value;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(int);
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetUltraIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    if (value == TRUE) {
		statePtr->flags |= NET_ULTRA_STATE_STATS;
	    } else {
		statePtr->flags &= ~NET_ULTRA_STATE_STATS;
	    }
	    break;
	}
	case IOC_ULTRA_CLEAR_STATS: {
	    bzero((char *) &statePtr->stats, sizeof(statePtr->stats));
	    break;
	}
	case IOC_ULTRA_GET_STATS: {
	    outSize = ioctlPtr->outBufSize;
	    inSize = sizeof(Dev_UltraStats);
	    fmtStatus = Fmt_Convert("w*", mach_Format, &inSize,
			    (Address) &statePtr->stats, 
			    ioctlPtr->format, &outSize,
			    ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		printf("Format of IOC_ULTRA_GET_STATS parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    break;
	}
	case IOC_ULTRA_MAP_THRESHOLD: {
	    int				value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf(
		"Format of IOC_ULTRA_MAP_THRESHOLD parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    netUltraMapThreshold = value;
	    break;
	}
	default: {
	    printf("NetUltraIOControl: unknown ioctl 0x%x\n",
		ioctlPtr->command);
	}

    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraSendReq --
 *
 *	Queue up a request for the adapter board. If there isn't
 *	any room in the queue then we block until there is.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	The adapter board is notified of the addition to the queue.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraSendReq(statePtr, doneProc, data, rpc, scatterLength, scatterPtr, 
	requestSize, requestPtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
    void		(*doneProc)();	/* Procedure to call when
					 * XRB is done. */
    ClientData		data;		/* Data used by doneProc. */
    Boolean		rpc;		/* Is this an RPC packet? */
    int			scatterLength;	/* Size of scatter/gather array. */
    Net_ScatterGather	*scatterPtr;	/* The scatter/gather array. */
    int			requestSize;	/* Size of the request. */
    NetUltraRequest	*requestPtr;	/* Request to be sent. */
{
    NetUltraXRB			*xrbPtr;
    NetUltraRequestHdr		*hdrPtr;
    NetUltraDMAInfo		*dmaPtr;
    ReturnStatus		status = SUCCESS;
    int				signal;
    NetUltraXRBInfo		*infoPtr;
    NetUltraTraceInfo		*tracePtr;
    int				size;
    int				i;

    if (netUltraDebug) {
	printf("NetUltraSendReq: sending a request\n");
    }
    xrbPtr = statePtr->nextToAdapterPtr;
#ifndef CLEAN
    if (netUltraTrace) {
	NEXT_TRACE(statePtr, &tracePtr);
	tracePtr->event = SEND_REQ;
	tracePtr->index = xrbPtr - statePtr->firstToAdapterPtr;
	Timer_GetCurrentTicks(&tracePtr->ticks);
    }
#endif
    if (statePtr->nextToAdapterPtr->filled) {
#ifndef CLEAN
	if (netUltraTrace) {
	    NEXT_TRACE(statePtr, &tracePtr);
	    tracePtr->event = TO_ADAPTER_FILLED;
	    tracePtr->index = statePtr->nextToAdapterPtr - 
		statePtr->firstToAdapterPtr;
	}
#endif
	do {
	    if (netUltraDebug) {
		printf("NetUltraSendReq: no XRB free, waiting\n");
	    }
	    statePtr->toAdapterAvail.waiting = TRUE;
	    signal = Sync_SlowMasterWait(
		    (unsigned int) &statePtr->toAdapterAvail,
		    &statePtr->interPtr->mutex, TRUE);
	    if (signal) {
		return GEN_ABORTED_BY_SIGNAL;
	    }
	} while(statePtr->nextToAdapterPtr->filled);
    }
    xrbPtr = statePtr->nextToAdapterPtr;
    hdrPtr = (NetUltraRequestHdr *) requestPtr;
    infoPtr = (NetUltraXRBInfo *) List_First(statePtr->freeXRBInfoList);
    if ((List_Links *) infoPtr == statePtr->freeXRBInfoList) {
	panic("NetUltraSendReq: no available XRBInfo structures!");
    }
    List_Remove((List_Links *) infoPtr);
    List_Insert((List_Links *) infoPtr, 
	LIST_ATFRONT(statePtr->pendingXRBInfoList));
    infoPtr->doneProc = doneProc;
    infoPtr->doneData = data;
    infoPtr->xrbPtr = xrbPtr;
    infoPtr->scatterPtr = scatterPtr;
    infoPtr->scatterLength = scatterLength;
    infoPtr->requestSize = requestSize;
    infoPtr->requestPtr = (union NetUltraRequest *) requestPtr;
    infoPtr->flags = NET_ULTRA_INFO_PENDING;
#ifndef CLEAN
    if (netUltraDebug) {
	printf("NetUltraSendReq: using 0x%x\n", infoPtr);
    }
#endif
#ifndef CLEAN
    if (netUltraTrace) {
	NEXT_TRACE(statePtr, &tracePtr);
	tracePtr->event = SEND_REQ_INFO;
	tracePtr->index = xrbPtr - statePtr->firstToAdapterPtr;
	tracePtr->infoPtr = infoPtr;
    }
#endif
    hdrPtr->infoPtr = infoPtr;
    hdrPtr->status = 0;
    hdrPtr->reference = 0;
    if (scatterLength > 0) {
	Address		buffer;
	List_Links	*itemPtr;
	/*
	 * If the buffer is not in DVMA space then get one that is.
	 */
	if (! DVMA_ADDRESS(scatterPtr[0].bufAddr, statePtr)) {
	    if (netUltraDebug) {
		printf("Data is not in DVMA space.\n");
	    }
	    while(List_IsEmpty(statePtr->freeBufferList)) {
		int		signal;
		statePtr->bufferAvail.waiting = TRUE;
		signal = Sync_SlowMasterWait(
			(unsigned int) &statePtr->bufferAvail,
			&statePtr->interPtr->mutex, TRUE);
		if (signal) {
		    status = GEN_ABORTED_BY_SIGNAL;
		    goto exit;
		}
	    }
	    itemPtr = List_First(statePtr->freeBufferList);
	    if (itemPtr == statePtr->freeBufferList) {
		panic("NetUltraOutput: list screwup\n");
	    }
	    List_Remove(itemPtr);
	    buffer = (Address) itemPtr;
	    infoPtr->flags |= NET_ULTRA_INFO_STD_BUFFER;
	    if (rpc) {
		int	lastIndex;
		/*
		 * This is a standard RPC packet with 4 parts --
		 * packet header, RPC header, RPC params, and data. 
		 * Copy the first three into the DVMA buffer.  If the data
		 * is below a threshold then copy it also.  Otherwise
		 * map the data into DVMA space and remap the first three parts
		 * so that they precede the data.
		 */
		size = 0;
		lastIndex = scatterLength - 1;
		for (i = 0; i < scatterLength; i++) {
		    size += scatterPtr[i].length;
		}
		if ((scatterPtr[lastIndex].length > 0) &&
		    (((unsigned int) scatterPtr[lastIndex].bufAddr & 
		    VMMACH_OFFSET_MASK_INT) + 
		    scatterPtr[lastIndex].length > netUltraMapThreshold)) {
		    Address			adjBuffer;
		    Net_ScatterGather	tmpScatter[2];
		    Net_ScatterGather	newScatter[2];
		    int			junk;
		    int			tmpSize;
		    RpcHdrNew		*rpcHdrPtr;

		    if (netUltraDebug) {
			printf("Mapping data into DVMA space.\n");
		    }
		    infoPtr->buffer = buffer;
		    tmpSize = size - scatterPtr[lastIndex].length;
		    adjBuffer = (Address) ((((unsigned int) (buffer + tmpSize) 
			& ~VMMACH_OFFSET_MASK_INT) + VMMACH_PAGE_SIZE_INT)
			- tmpSize);
		    Net_GatherCopy(scatterPtr, lastIndex, adjBuffer);
		    tmpScatter[0].bufAddr = adjBuffer;
		    tmpScatter[0].length = tmpSize;
		    tmpScatter[1] = scatterPtr[lastIndex];
		    VmMach_DMAAllocContiguous(tmpScatter, 2, newScatter);
		    buffer = newScatter[0].bufAddr;
		    tmpSize = newScatter[1].bufAddr + newScatter[1].length - 
				buffer;
		    if (tmpSize > size + VMMACH_PAGE_SIZE_INT) {
			panic("Contiguous DVMA mapping failed.\n");
		    }
		    size = tmpSize;
		    junk = (int) newScatter[1].bufAddr & VMMACH_OFFSET_MASK_INT;
		    rpcHdrPtr = (RpcHdrNew *) buffer;
		    rpcHdrPtr->dataStart += junk;
		    infoPtr->flags |= NET_ULTRA_INFO_REMAP;
		} else {
		    Net_GatherCopy(scatterPtr, scatterLength, buffer);
		}
	    } else {

		if (netUltraDebug) {
		    printf("Copying data into DVMA space.\n");
		}
		Net_GatherCopy(scatterPtr, 1, buffer);
		size = scatterPtr[0].length;
	    }
	} else {
	    /* 
	     * The data is already in DVMA space.  It had better be contiguous!
	     */
	    if (netUltraDebug) {
		printf("Data is already in DVMA space.\n");
	    }
	    buffer = scatterPtr[0].bufAddr;
	    size = scatterPtr[0].length;
	}
	if (! DVMA_ADDRESS(buffer, statePtr)) {
	    buffer = BUFFER_TO_DVMA(buffer, statePtr);
	}
	hdrPtr->buffer = DVMA_TO_VME(buffer, statePtr);
	hdrPtr->size = size;
    } else {
	hdrPtr->size = 0;
    }
    if (netUltraDebug) {
	printf("NetUltraSendReq: hdrPtr->buffer = 0x%x, size = %d\n",
	    hdrPtr->buffer, hdrPtr->size);
    }
#ifndef CLEAN
    if (statePtr->flags & NET_ULTRA_STATE_STATS) {
	if (hdrPtr->cmd == NET_ULTRA_DGRAM_SEND_REQ) {
	    statePtr->stats.packetsSent++;
	    statePtr->stats.bytesSent += hdrPtr->size;
	    statePtr->stats.sentHistogram[hdrPtr->size >> 10]++;
	}
    }
#endif
    bcopy((char *) requestPtr, (char *) &xrbPtr->request, requestSize);
    dmaPtr = &xrbPtr->dma;
    if (hdrPtr->cmd == NET_ULTRA_DGRAM_SEND_REQ) {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB_DATA;
	statePtr->numWritePending++;
#ifndef CLEAN
	if (netUltraDebug) {
	    printf("NetUltraSendReq: number of pending writes = %d.\n",
		statePtr->numWritePending);
	}
#endif
	if (statePtr->numWritePending > statePtr->maxWritePending) {
	    panic("NetUltraSendReq: too many writes.\n");
	}
    } else if (hdrPtr->cmd == NET_ULTRA_DGRAM_RECV_REQ) {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB;
	statePtr->numReadPending++;
#ifndef CLEAN
	if (netUltraDebug) {
	    printf("NetUltraSendReq: number of pending reads = %d.\n",
		statePtr->numReadPending);
	}
#endif
	if (statePtr->numReadPending > statePtr->maxReadPending) {
	    panic("NetUltraSendReq: too many reads.\n");
	}
    } else {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB;
    }
    dmaPtr->id = 0;
    dmaPtr->reference = 0;
    dmaPtr->offset = 0;
    dmaPtr->infoPtr = (NetUltraXRBInfo *) NIL;
    dmaPtr->length = requestSize;
    /*
     * Set the filled field so the adapter will process the XRB.
     */
    if (netUltraDebug) {
	printf("NetUltraSendReq: xrbPtr = 0x%x\n", xrbPtr);
    }
    xrbPtr->filled = 1;
    if (statePtr->nextToAdapterPtr == statePtr->lastToAdapterPtr) {
	statePtr->nextToAdapterPtr = statePtr->firstToAdapterPtr;
    } else {
	statePtr->nextToAdapterPtr++;
    }
    /*
     * Poke the adapter by setting the address register to 0. This
     * tells it to look in the queue.
     */
    *statePtr->intrReg = 0;
#ifndef CLEAN
    if (netUltraDebug) {
	printf("NetUltraSendReq: returning\n");
    }
#endif
exit:
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * GetStatusString --
 *
 *	Returns a string corresponding to the given status (status
 *	is a field in a NetUltraRequestHdr.
 *
 * Results:
 *	Ptr to string describing the status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetStatusString(status)
    unsigned char	status;
{
    static char *statusMsg[] = {
	"Unknown",				/* 0 */
	"OK",					/* 1 */
	"FAIL: invalid request",		/* 2 */
	"OK: EOM",				/* 3 */
	"FAIL: no resources",			/* 4 */
	"OK: decide",				/* 5 */
	"FAIL: unknown reference",		/* 6 */
	"OK: closed",				/* 7 */
	"FAIL: buffer too small",		/* 8 */
	"OK; withdrawn",			/* 9 */
	"FAIL: buffer too large",		/* 10 */
	"OK: reject connection",		/* 11 */
	"FAIL: illegal request",		/* 12 */
	"OK: connection request",		/* 13 */
	"FAIL: REM abort",			/* 14 */
	"OK: closing",				/* 15 */
	"FAIL: LOC timeout",			/* 16 */
	"OK: timed-out",			/* 17 */
	"FAIL: unknown connection class", 	/* 18 */
	"OK: out of sequence",			/* 19 */
	"FAIL: duplicate request",		/* 20 */
	"Unknown",				/* 21 */
	"FAIL: connection rejected",		/* 22 */
	"Unknown",				/* 23 */
	"FAIL: negotiation failed",		/* 24 */
	"Unknown",				/* 25 */
	"FAIL: illegal address",		/* 26 */
	"Unknown",				/* 27 */
	"FAIL: network error",			/* 28 */
	"Unknown",				/* 29 */
	"FAIL: protocol error",			/* 30 */
	"Unknown",				/* 31 */
	"FAIL: illegal RB length",		/* 32 */
	"Unknown",				/* 33 */
	"FAIL: unknown SAP id",			/* 34 */
	"Unknown",				/* 35 */
	"FAIL: zero RB id",			/* 36 */
	"Unknown",				/* 37 */
	"FAIL: adapter down",			/* 38 */
    };
    static int numStatusMsg = sizeof(statusMsg) / sizeof(char *);

    if (status >= numStatusMsg) {
	return "Unknown";
    } else {
	return statusMsg[status];
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InitQueues --
 *
 *	Initializes the XRB queues.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated and mapped into DMA space if necessary.
 *
 *----------------------------------------------------------------------
 */

static void
InitQueues(statePtr)
    NetUltraState	*statePtr;	/* State of the adapter. */
{
    int		size;
    Address	addr;
    List_Links	*itemPtr;
    int		i;

    if (!(statePtr->queuesInit)) {
	/*
	 * Allocate XRBs that go from the adapter to the host. 
	 */
	size = sizeof(NetUltraXRB) * NET_ULTRA_NUM_TO_HOST;
	addr = (Address) malloc(size);
	addr = VmMach_DMAAlloc(size, (Address) addr);
	if (addr == (Address) NIL) {
	    panic("NetUltraInit: unable to allocate DMA space.\n");
	}
	statePtr->firstToHostPtr = (NetUltraXRB *) addr;
	/* 
	 * Allocate XRBs that go from the host to the adapter. 
	 */
	size = sizeof(NetUltraXRB) * NET_ULTRA_NUM_TO_ADAPTER;
	addr = (Address) malloc(size);
	addr = VmMach_DMAAlloc(size, (Address) addr);
	if (addr == (Address) NIL) {
	    panic("NetUltraInit: unable to allocate DMA space.\n");
	}
	statePtr->firstToAdapterPtr = (NetUltraXRB *) addr;
	statePtr->pendingXRBInfoList = &statePtr->pendingXRBInfoListHdr;
	statePtr->freeXRBInfoList = &statePtr->freeXRBInfoListHdr;
	List_Init(statePtr->pendingXRBInfoList);
	List_Init(statePtr->freeXRBInfoList);
	for (i = 0; i < NET_ULTRA_NUM_TO_ADAPTER + NET_ULTRA_NUM_TO_HOST; i++) {
	    itemPtr = (List_Links *) malloc(sizeof(NetUltraXRBInfo));
	    List_InitElement(itemPtr);
	    List_Insert(itemPtr, LIST_ATREAR(statePtr->freeXRBInfoList));
	}
	/*
	 * Allocate buffers in DVMA space for pending reads and writes. 
	 */
	statePtr->maxReadPending = NET_ULTRA_PENDING_READS;
	statePtr->numReadPending = 0;
	statePtr->maxWritePending = NET_ULTRA_PENDING_WRITES;
	statePtr->numWritePending = 0;
	statePtr->numBuffers = statePtr->maxReadPending + 
	    statePtr->maxWritePending;
	statePtr->bufferSize = NET_ULTRA_MAX_BYTES;
	size = statePtr->numBuffers * statePtr->bufferSize;
	addr = (Address) malloc(size);
	statePtr->buffers = addr;
	addr = VmMach_DMAAlloc(size, (Address) addr);
	statePtr->buffersDVMA = addr;
	statePtr->freeBufferList = &statePtr->freeBufferListHdr;

	statePtr->queuesInit = TRUE;
    }
    statePtr->lastToHostPtr = statePtr->firstToHostPtr + 
	    NET_ULTRA_NUM_TO_HOST - 1;
    statePtr->nextToHostPtr = statePtr->firstToHostPtr;
    statePtr->lastToAdapterPtr = statePtr->firstToAdapterPtr + 
	    NET_ULTRA_NUM_TO_ADAPTER - 1;
    statePtr->nextToAdapterPtr = statePtr->firstToAdapterPtr;
    size = sizeof(NetUltraXRB) * NET_ULTRA_NUM_TO_HOST;
    bzero((char *) statePtr->firstToHostPtr, size);
    size = sizeof(NetUltraXRB) * NET_ULTRA_NUM_TO_ADAPTER;
    bzero((char *) statePtr->firstToAdapterPtr, size);
    LIST_FORALL(statePtr->pendingXRBInfoList, itemPtr) {
	List_Remove(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->freeXRBInfoList));
    }
    bzero((char *) statePtr->buffers, statePtr->numBuffers * 
	statePtr->bufferSize);
    List_Init(statePtr->freeBufferList);
    for (i = 0; i < statePtr->numBuffers; i++) {
	itemPtr = (List_Links *) 
		(statePtr->buffers + (i * statePtr->bufferSize));
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->freeBufferList));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraPendingRead --
 *
 *	Send a pending read to the adapter.
 *
 * Results:
 *	Standard Sprite return status.
 *
 * Side effects:
 *	A "read datagram" XRB is sent to the adapter.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetUltraPendingRead(interPtr, size, buffer)
    Net_Interface	*interPtr;	/* Interface to read from. */
    int			size;		/* Size of the data buffer. */
    Address		buffer;		/* Address of the buffer in DMA space */
{
    NetUltraState		*statePtr;
    NetUltraRequest		request;
    NetUltraDatagramRequest	*dgramReqPtr;
    NetUltraRequestHdr		*hdrPtr;
    ReturnStatus		status = SUCCESS;
    Net_ScatterGather		scatter;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(statePtr->flags & NET_ULTRA_STATE_START)) {
	printf("NetUltraPendingRead: adapter not started!\n");
	return FAILURE;
    }
#ifndef CLEAN
    if (netUltraDebug) {
	printf("NetUltraPendingRead: queuing a pending read.\n");
	printf("Buffer size = %d, buffer = 0x%x\n", size, buffer);
    }
#endif
    statePtr = (NetUltraState *) interPtr->interfaceData;
    bzero((char *) &request, sizeof(request));
    dgramReqPtr = &request.dgram;
    hdrPtr = &dgramReqPtr->hdr;
    dgramReqPtr->remoteAddress = wildcardAddress;
    dgramReqPtr->localAddress = wildcardAddress;
    hdrPtr->cmd = NET_ULTRA_DGRAM_RECV_REQ;
    /*
     * Save room at the beginning of the buffer for the datagram request
     * block.  The higher-level software expects the packet header to
     * proceed the packet (this should be fixed sometime).
     */
    buffer += sizeof(NetUltraDatagramRequest);
    size -= sizeof(NetUltraDatagramRequest);
    scatter.bufAddr = buffer;
    scatter.length = size;
    status = NetUltraSendReq(statePtr, ReadDone, (ClientData) 0, FALSE,  
	1, &scatter, sizeof(NetUltraDatagramRequest), &request);
    if (status != SUCCESS) {
	printf("NetUltraPendingRead: could not send request to adapter\n");
    }
#ifndef CLEAN
    if (netUltraDebug) {
	printf("NetUltraPendingRead: returning.\n");
    }
#endif
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadDone --
 *
 *	This routine is called from the interrupt handler when a read
 *	request completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ReadDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    NetUltraXRB			*xrbPtr;
    NetUltraDatagramRequest	*reqPtr;
    NetUltraRequestHdr		*hdrPtr;
    NetUltraState		*statePtr;
    int				size;
    Address			buffer = (Address) NIL;
    ReturnStatus		status = SUCCESS;

    xrbPtr = infoPtr->xrbPtr;
    reqPtr = &xrbPtr->request.dgram;
    hdrPtr = &reqPtr->hdr;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(hdrPtr->status & NET_ULTRA_STATUS_OK)) {
	panic("ReadDone: read failed 0x%x (continuable)\n", hdrPtr->status);
    } else {
#ifndef CLEAN
	if (netUltraDebug) {
	    char	local[32];
	    char	remote[32];
	    printf("ReadDone: received a packet.\n");
	    *local = '\0';
	    *remote = '\0';
	    (void) Net_AddrToString((Net_Address *) 
		    &reqPtr->localAddress.address, 
		    NET_PROTO_RAW, NET_NETWORK_ULTRA, local);
	    (void) Net_AddrToString((Net_Address *) 
		    &reqPtr->remoteAddress.address, 
		    NET_PROTO_RAW, NET_NETWORK_ULTRA, remote);
	    printf("Local address: %s\n", local);
	    printf("Remote address: %s\n", remote);
	    printf("Data size = %d\n", hdrPtr->size);
	    if (hdrPtr->size > 0) {
		printf("Data: %s\n", hdrPtr->buffer + VMMACH_DMA_START_ADDR);
	    }
	}
#endif
	/*
	 * Adjust size and start of buffer to account for the space
	 * for the request block.
	 */
	buffer = VME_TO_BUFFER(hdrPtr->buffer, statePtr) -
		    sizeof(NetUltraDatagramRequest);
	size = hdrPtr->size + sizeof(NetUltraDatagramRequest);
	if (statePtr->flags & NET_ULTRA_STATE_NORMAL) {
	    /* 
	     * Copy the request block (packet header) to the start of the
	     * packet. 
	     */

	    *((NetUltraDatagramRequest *) buffer) = *reqPtr;
	    /*
	     * Release the lock around interface so that the RPC system
	     * can output a packet in response to this one.  The XRB is
	     * still marked as filled, so the adapter won't overwrite it.
	     * Also the XRBinfo is not marked as pending, so the interrupt
	     * routine won't re-process the packet should it loop around
	     * the queue before Net_Input returns.
	     */
	    MASTER_UNLOCK(&interPtr->mutex);
	    Net_Input(interPtr, buffer, size);
	    MASTER_LOCK(&interPtr->mutex);
	} else if (statePtr->flags & NET_ULTRA_STATE_ECHO) {
	    Net_ScatterGather		tmpScatter;
	    if (netUltraDebug) {
		printf("ReadDone: returning datagram to sender.\n");
	    }
	    hdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
	    hdrPtr->status = 0;
	    tmpScatter.bufAddr = BUFFER_TO_DVMA(buffer, statePtr);
	    tmpScatter.length = hdrPtr->size;
	    status = NetUltraSendReq(statePtr, EchoDone, (ClientData) NIL, 
			FALSE, 1, &tmpScatter,sizeof(NetUltraDatagramRequest),
			(NetUltraRequest *) reqPtr);
	    if (status != SUCCESS) {
		panic("ReadDone: unable to return datagram to sender.\n");
	    }
	} else if (statePtr->flags & NET_ULTRA_STATE_DSND_TEST) {
	    Net_ScatterGather		tmpScatter;
	    dsndCount--;
	    if (dsndCount > 0) {
		if (netUltraDebug) {
		    printf("ReadDone: returning datagram to sender.\n");
		}
		hdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
		hdrPtr->status = 0;
		tmpScatter.bufAddr = BUFFER_TO_DVMA(buffer, statePtr);
		tmpScatter.length = hdrPtr->size;
		status = NetUltraSendReq(statePtr, EchoDone, (ClientData) NIL, 
			    FALSE, 1, &tmpScatter,
			    sizeof(NetUltraDatagramRequest),
			    (NetUltraRequest *) reqPtr);
		if (status != SUCCESS) {
		    panic("ReadDone: unable to return datagram to sender.\n");
		}
	    } else {

		/*
		 * Notify the waiting process that the last datagram has
		 * arrived.
		 */
		Sync_Broadcast(&dsndTestDone);
	    }
	} else if (statePtr->flags & NET_ULTRA_STATE_SINK) {
	    packetsSunk++;
	}
    }
    statePtr->numReadPending--;
    if (statePtr->numReadPending < 0) {
	panic("ReadDone: number of pending reads < 0.\n");
    }
    /* 
     * This may introduce a deadlock if the queues are of size 1, 
     * since the XRB and info structures are
     * not freed until this routine (ReadDone) returns, and 
     * NetUltraPendingRead needs to use these structures.  If the queues
     * are greater than 1 then NetUltraPendingRead may have to
     * wait for an XRB to free up, but that should be ok.
     */
    status = NetUltraPendingRead(interPtr, statePtr->bufferSize, 
		BUFFER_TO_DVMA(buffer,statePtr));
    if (status != SUCCESS) {
	printf("ReadDone: could not queue next pending read.\n");
    }
#ifndef CLEAN
    if (netUltraDebug) {
	printf("ReadDone: returning.\n");
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * EchoDone --
 *
 *	This routine is called when the write of a datagram that is
 *	being echoed back to the sender completes.  All it does is
 *	decrement the number of pending writes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
EchoDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    NetUltraState	*statePtr;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    statePtr->numWritePending--;
}


/*
 *----------------------------------------------------------------------
 *
 * NetUltraSendDgram --
 *
 *	This routine will send a datagram to the specified host.  It
 *	is intended for debugging purposes.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A write is queued to the adapter.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
NetUltraSendDgram(interPtr, netAddressPtr, count, bufSize, buffer, timePtr)
    Net_Interface	*interPtr;		/* Interface to send on. */
    Net_Address		*netAddressPtr;		/* Host to send to. */
    int			count;			/* Number of times to send
						 * datagram. */
    int			bufSize;		/* Size of data buffer. */
    Address		buffer;			/* Data to send. */
    Time		*timePtr;		/* Place to store total 
						 * time to send datagrams. */
{
    NetUltraState		*statePtr;
    NetUltraRequest		request;
    NetUltraDatagramRequest	*dgramReqPtr;
    NetUltraRequestHdr		*hdrPtr;
    ReturnStatus		status;
    Net_Address			*addressPtr;
    Timer_Ticks 		startTime;
    Timer_Ticks 		endTime;
    Timer_Ticks 		curTime;
    NetUltraTraceInfo		*tracePtr;
    Net_ScatterGather		scatter;

    MASTER_LOCK(&interPtr->mutex);
#ifndef CLEAN
    if (netUltraDebug) {
	char	address[100];
	(void) Net_AddrToString(netAddressPtr, NET_PROTO_RAW, NET_NETWORK_ULTRA,
		    address);
	printf("NetUltraSendDgram: sending to %s\n", address);
    }
#endif
    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(statePtr->flags & NET_ULTRA_STATE_START)) {
	printf("NetUltraSendDgram: adapter not started!\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    dgramReqPtr = &request.dgram;
    hdrPtr = &dgramReqPtr->hdr;
    dgramReqPtr->remoteAddress = wildcardAddress;
    dgramReqPtr->remoteAddress.tsapSize = 2;
    dgramReqPtr->remoteAddress.tsap[1] = 1;
    dgramReqPtr->remoteAddress.address = netAddressPtr->ultra;
    addressPtr = (Net_Address *) &dgramReqPtr->remoteAddress.address;
    addressPtr->generic.data[1] = 0x49;
    addressPtr->generic.data[6] = 0xfe;
    dgramReqPtr->localAddress = wildcardAddress;
    dgramReqPtr->localAddress.tsapSize = 2;
    dgramReqPtr->localAddress.tsap[1] = 1;
    dgramReqPtr->localAddress.address = 
	interPtr->netAddress[NET_PROTO_RAW].ultra;
    addressPtr = (Net_Address *) &dgramReqPtr->localAddress.address;
    addressPtr->generic.data[1] = 0x49;
    addressPtr->generic.data[6] = 0xfe;
    hdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
    scatter.bufAddr = buffer;
    scatter.length = bufSize;
    statePtr->flags |= NET_ULTRA_STATE_DSND_TEST;
    statePtr->flags &= ~NET_ULTRA_STATE_NORMAL;
    dsndCount = count;
#ifndef CLEAN
    if (netUltraTrace) {
	NEXT_TRACE(statePtr, &tracePtr);
	tracePtr->event = DSND;
	Timer_GetCurrentTicks(&curTime);
	tracePtr->ticks = curTime;
    }
#endif
    Timer_GetCurrentTicks(&startTime);
    status = NetUltraSendReq(statePtr, DgramSendDone, (ClientData) NIL,
		FALSE, 1, &scatter, sizeof(NetUltraDatagramRequest), &request);
    Sync_MasterWait(&(dsndTestDone), &(interPtr->mutex), FALSE);
    Timer_GetCurrentTicks(&endTime);
#ifndef CLEAN
    if (netUltraDebug) {
	printf("NetUltraSendDgram: test done.\n");
    } 
#endif
    statePtr->flags &= ~NET_ULTRA_STATE_DSND_TEST;
    statePtr->flags |= NET_ULTRA_STATE_NORMAL;
    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, timePtr);
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DgramSendDone --
 *
 *	Called by the interrupt handler when the datagram sent by 
 * 	NetUltraSendDgram is actually sent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process waiting for the datagram to be sent is notified.
 *
 *----------------------------------------------------------------------
 */

static void
DgramSendDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    NetUltraXRB			*xrbPtr;
    NetUltraDatagramRequest	*reqPtr;
    NetUltraRequestHdr		*hdrPtr;

    xrbPtr = infoPtr->xrbPtr;
    reqPtr = &xrbPtr->request.dgram;
    hdrPtr = &reqPtr->hdr;

    if (!(hdrPtr->status & NET_ULTRA_STATUS_OK)) {
	panic("DgramSendDone: dgram send failed 0x%x\n", hdrPtr->status);
    } else {
#ifndef CLEAN
	if (netUltraDebug) {
	    printf("DgramSendDone: datagram sent ok\n");
	}
#endif
    }
    ((NetUltraState *) interPtr->interfaceData)->numWritePending--;
    if (((NetUltraState *) interPtr->interfaceData)->numWritePending < 0) {
	panic("DgramSendDone: number of pending writes < 0.\n");
    }
    if (infoPtr->doneData != (ClientData) NIL) {
	Sync_Broadcast((Sync_Condition *) infoPtr->doneData);
    }
#ifndef CLEAN
    if (netUltraDebug) {
	printf("DgramSendDone: returning.\n");
    }
#endif

}


/*
 *----------------------------------------------------------------------
 *
 * NetUltraOutput --
 *
 *	Puts the outgoing packet into the queue to the adapter.
 *	Since the ultranet adapter does not to scatter/gather
 *	we have to get a free write buffer and copy the data
 *	into the buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The packet is put into the queue and the adapter is notified.
 *
 *----------------------------------------------------------------------
 */

void
NetUltraOutput(interPtr, hdrPtr, scatterGatherPtr, scatterGatherLength)
    Net_Interface		*interPtr;	/* The interface to use. */
    Address			hdrPtr;		/* Packet header. */
    Net_ScatterGather		*scatterGatherPtr; /* Scatter/gather elements.*/
    int				scatterGatherLength; /* Number of elements in
						      * scatter/gather list. */
{
    NetUltraState		*statePtr;
    ReturnStatus		status = SUCCESS;
    Net_UltraHeader		*ultraHdrPtr = (Net_UltraHeader *) hdrPtr;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    MASTER_LOCK(&interPtr->mutex);
    if (netUltraDebug) {
	char	address[100];
	(void) Net_AddrToString((Net_Address *) 
		&ultraHdrPtr->remoteAddress.address, 
		NET_PROTO_RAW, NET_NETWORK_ULTRA,
		    address);
	printf("NetUltraOutput: sending to %s\n", address);
    }
    status = NetUltraSendReq(statePtr, OutputDone, 
		(ClientData) NIL, TRUE, scatterGatherLength, scatterGatherPtr, 
		sizeof(Net_UltraHeader), 
		(NetUltraRequest *) ultraHdrPtr);
    if (status != SUCCESS) {
	printf("NetUltraOutput: packet not sent\n");
    }
    MASTER_UNLOCK(&interPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * OutputDone --
 *
 *	This routine is called by the interrupt handler when a packet
 *	sent by NetUltraOutput completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The write buffer associated with the command is added to
 * 	the list of free write buffers.
 *
 *----------------------------------------------------------------------
 */

static void
OutputDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    NetUltraRequestHdr		*hdrPtr;
    NetUltraState		*statePtr;

    hdrPtr = &(infoPtr->xrbPtr->request.dgram.hdr);

    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(hdrPtr->status & NET_ULTRA_STATUS_OK)) {
	panic("OutputDone: write failed 0x%x (continuable)\n", hdrPtr->status);
    } else {
#ifndef CLEAN
	if (netUltraDebug) {
	    printf("OutputDone: packet sent\n");
	}
#endif
	statePtr->numWritePending--;
	if (statePtr->numWritePending < 0) {
	    panic("OutputDone: number of pending writes < 0.\n");
	}
	/*
	 * Wakeup any process waiting for the packet to be sent.
	 */
	infoPtr->scatterPtr->done = TRUE;
	if (infoPtr->scatterPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    NetOutputWakeup(infoPtr->scatterPtr->mutexPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraSource --
 *
 *	This routine will send a stream of datagrams to the specified host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A write is queued to the adapter.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
NetUltraSource(interPtr, netAddressPtr, count, bufSize, buffer, timePtr)
    Net_Interface	*interPtr;		/* Interface to send on. */
    Net_Address		*netAddressPtr;		/* Host to send to. */
    int			count;			/* Number of times to send
						 * datagram. */
    int			bufSize;		/* Size of data buffer. */
    Address		buffer;			/* Data to send. */
    Time		*timePtr;		/* Place to store total 
						 * time to send datagrams. */
{
    NetUltraState		*statePtr;
    NetUltraRequest		request;
    NetUltraDatagramRequest	*dgramReqPtr;
    NetUltraRequestHdr		*hdrPtr;
    ReturnStatus		status = SUCCESS;
    List_Links			*itemPtr;
    Net_Address			*addressPtr;
    Timer_Ticks 		startTime;
    Timer_Ticks 		endTime;
    Timer_Ticks 		curTime;
    NetUltraTraceInfo		*tracePtr;
    Net_ScatterGather		scatter;

    MASTER_LOCK(&interPtr->mutex);
#ifndef CLEAN
    if (netUltraDebug) {
	char	address[100];
	(void) Net_AddrToString(netAddressPtr, NET_PROTO_RAW, NET_NETWORK_ULTRA,
		    address);
	printf("NetUltraSendDgram: sending to %s\n", address);
    }
#endif
    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(statePtr->flags & NET_ULTRA_STATE_START)) {
	printf("NetUltraSendDgram: adapter not started!\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    dgramReqPtr = &request.dgram;
    hdrPtr = &dgramReqPtr->hdr;
    dgramReqPtr->remoteAddress = wildcardAddress;
    dgramReqPtr->remoteAddress.tsapSize = 2;
    dgramReqPtr->remoteAddress.tsap[1] = 1;
    dgramReqPtr->remoteAddress.address = netAddressPtr->ultra;
    addressPtr = (Net_Address *) &dgramReqPtr->remoteAddress.address;
    addressPtr->generic.data[1] = 0x49;
    addressPtr->generic.data[6] = 0xfe;
    dgramReqPtr->localAddress = wildcardAddress;
    dgramReqPtr->localAddress.tsapSize = 2;
    dgramReqPtr->localAddress.tsap[1] = 1;
    dgramReqPtr->localAddress.address = 
	interPtr->netAddress[NET_PROTO_RAW].ultra;
    addressPtr = (Net_Address *) &dgramReqPtr->localAddress.address;
    addressPtr->generic.data[1] = 0x49;
    addressPtr->generic.data[6] = 0xfe;
    hdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
    Timer_GetCurrentTicks(&startTime);
    while(count > 0) {
	count--;
	while(List_IsEmpty(statePtr->freeBufferList)) {
	    int		signal;
	    statePtr->bufferAvail.waiting = TRUE;
	    signal = Sync_SlowMasterWait((unsigned int) &statePtr->bufferAvail,
		    &interPtr->mutex, TRUE);
	    if (signal) {
		status = GEN_ABORTED_BY_SIGNAL;
		goto exit;
	    }
	}
	itemPtr = List_First(statePtr->freeBufferList);
	if (itemPtr == statePtr->freeBufferList) {
	    panic("NetUltraSendDgram: list screwup\n");
	}
	List_Remove(itemPtr);
	if (buffer != (Address) NIL) {
	    bcopy((char *) buffer, (char *) itemPtr, bufSize);
	}
	dsndCount = count;
#ifndef CLEAN
	if (netUltraTrace) {
	    NEXT_TRACE(statePtr, &tracePtr);
	    tracePtr->event = DSND_STREAM;
	    Timer_GetCurrentTicks(&curTime);
	    tracePtr->ticks = curTime;
	}
#endif
	scatter.bufAddr = (Address) itemPtr;
	scatter.length = bufSize;
	status = NetUltraSendReq(statePtr, SourceDone, 
		    (ClientData) NIL, FALSE, 1, &scatter,
		    sizeof(NetUltraDatagramRequest), &request);
    }
    Timer_GetCurrentTicks(&endTime);
    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, timePtr);
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * SourceDone --
 *
 *	This routine is called by the interrupt handler when a packet
 *	sent by  NetUltraSource completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The write buffer associated with the command is added to
 * 	the list of free write buffers.
 *
 *----------------------------------------------------------------------
 */

static void
SourceDone(interPtr, infoPtr)
    Net_Interface	*interPtr;	/* Interface. */
    NetUltraXRBInfo	*infoPtr;	/* Info about XRB that completed. */
{
    NetUltraRequestHdr		*hdrPtr;
    NetUltraState		*statePtr;
    List_Links			*itemPtr;

    hdrPtr = &(infoPtr->xrbPtr->request.dgram.hdr);

    statePtr = (NetUltraState *) interPtr->interfaceData;
    if (!(hdrPtr->status & NET_ULTRA_STATUS_OK)) {
	panic("SourceDone: write failed 0x%x (continuable)\n", hdrPtr->status);
    } else {
#ifndef CLEAN
	if (netUltraDebug) {
	    printf("SourceDone: packet sent\n");
	}
#endif
	statePtr->numWritePending--;
	if (statePtr->numWritePending < 0) {
	    panic("SourceDone: number of pending writes < 0.\n");
	}
	/*
	 * Free up the write buffer. 
	 */
	itemPtr = (List_Links *) VME_TO_BUFFER(hdrPtr->buffer,statePtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->freeBufferList));
	if (statePtr->bufferAvail.waiting == TRUE) {
	    Sync_Broadcast(&statePtr->bufferAvail);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetUltraGetStats --
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
NetUltraGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    NetUltraState	*statePtr;

    statePtr = (NetUltraState *) interPtr->interfaceData;
    MASTER_LOCK(&interPtr->mutex);
    statPtr->ultra = statePtr->stats;
    MASTER_UNLOCK(&interPtr->mutex);
    return SUCCESS;
}

