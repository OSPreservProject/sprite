/* 
 * netHppi.c --
 *
 *	Routines for handling the TMC 29K cards.
 *
 * Copyright 1992 Regents of the University of California
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
#include <netHppi.h>
#include <netHppiInt.h>
#include <dev/hppi.h>
#include <fmt.h>
#include <sync.h>
#include <dbg.h>
#include <rpcPacket.h>
#include <assert.h>
#include <list.h>

/*
 * "Borrow" the Vax format for the format of the Ultranet adapter (it
 * is really a 386.  We need to swap some of the words in the
 * diagnostic and information structures.
 */

#define NET_ULTRA_FORMAT FMT_VAX_FORMAT

/*
 * The transmit and receive queues are in the dual port RAM on the
 * link board.
 */
#define QUEUES_IN_DUALPORT_RAM

Boolean		netHppiDebug = FALSE;
Boolean		netHppiTrace = FALSE;
int		netHppiMapThreshold = NET_ULTRA_MAP_THRESHOLD;
static unsigned char netHppiConnPar[] = {0xc6, 0x10, 0x0c, 0x00,
					     0xff, 0xff, 0xff, 0xff,
					     0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00};
static int	numHppiInterfaces = 0;
static NetHppiState* hppiState[NET_HPPI_MAX_INTERFACES];
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

#define WAIT_FOR_BIT_CLEAR(ptr, count, mask) {		\
    int i;						\
    for (i = (count); i > 0; i -= 100) {		\
	if ((*((volatile int *)ptr) & (mask)) == 0) {	\
	    break;					\
	}						\
	MACH_DELAY (100);				\
    }							\
}


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

static char 		*GetStatusString _ARGS_ ((int status));
static Sync_Condition	dsndTestDone;
static Sync_Condition	drcvTestDone;
static Sync_Condition	iopOutputDone;
static int		dsndCount;

static void		InitQueues _ARGS_((NetHppiState *statePtr));
static void		StandardDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static ReturnStatus	NetHppiSendDgram _ARGS_((Net_Interface *interPtr,
				Net_Address *netAddressPtr, int count,
				int bufSize, Address buffer, Time *timePtr));
static void		DgramSendDone _ARGS_((Dev_HppiSendDSND* dgramCmdPtr));
static void		NetHppiRecvDgram _ARGS_((Net_Interface *interPtr));
static void		DgramRecvDone _ARGS_((Dev_HppiSendDRCV* dgramCmdPtr));
static void		SourceDone _ARGS_((Net_Interface *interPtr, 
				NetUltraXRBInfo *infoPtr));
static ReturnStatus	NetHppiSource _ARGS_((Net_Interface *interPtr,
				Net_Address *netAddressPtr, int count,
				int bufSize, Address buffer, Time *timePtr));
static void		NetHppiResetCallback _ARGS_((ClientData data,
				Proc_CallInfo *infoPtr));
static void		NetHppiMsgCallback _ARGS_((ClientData data,
			        Proc_CallInfo *infoPtr));
static void		NetHppiCmdCallback _ARGS_((ClientData data,
						   Proc_CallInfo *infoPtr));
static ReturnStatus	NetHppiSetupBoard _ARGS_((NetHppiState *interPtr));
static ReturnStatus	NetHppiStart _ARGS_((NetHppiState *statePtr));
static ReturnStatus	NetHppiStop _ARGS_((NetHppiState *statePtr));
static ReturnStatus	NetHppiSendReq _ARGS_((NetHppiState *statePtr,
				void (*doneProc)(), ClientData data,
				Boolean rpc, int scatterLength,
				Net_ScatterGather *scatterPtr,
				int requestSize, NetUltraRequest *requestPtr));
static ReturnStatus	NetHppiSendCmd _ARGS_((NetHppiState *statePtr,
					       int size, Address cmdPtr,
					       int flags));
static void		NetHppiConnectionAccepted _ARGS_((Dev_HppiSendALSN*
							  connBlockPtr));
static void		NetHppiXferCallback _ARGS_((Dev_HppiSendXfer* cmdPtr));
static void		NetHppiOpenCallback _ARGS_((Dev_HppiSendOPEN* cmdPtr));
static void		NetHppiConnectionCleanup _ARGS_((Dev_HppiSendRLSE*
							 cmdPtr));
static NetHppiOutputStub _ARGS_((Net_Interface* interPtr, Address hdrPtr,
				 Net_ScatterGather* scatterGatherPtr,
				 int scatterGatherLength, Boolean rpc,
				 ReturnStatus* statusPtr));
static void iocConnCallback _ARGS_ ((Net_HppiConnection* connPtr));
static void iocXferCallback _ARGS_ ((Net_HppiDataRequest* dataReqPtr));
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
    ((Address) ((((unsigned int)(addr)) & 0x00ffffff) + 	\
    ((unsigned int) VMMACH_DMA_START_ADDR)))

#define BUFFER_TO_VME(addr, statePtr) \
    ((Address) (DVMA_TO_VME(BUFFER_TO_DVMA(addr,statePtr), (statePtr))))

#define VME_TO_BUFFER(addr, statePtr) \
    ((Address) (DVMA_TO_BUFFER(VME_TO_DVMA((addr),statePtr),(statePtr))))

#define DVMA_ADDRESS(addr, statePtr) \
    ((unsigned int)(addr) < ((unsigned int)VMMACH_DMA_START_ADDR)? FALSE :TRUE)


/*
 *----------------------------------------------------------------------
 *
 * NetHppiInit --
 *
 *	Initialize the TMC VME boards.
 *
 * Results:
 *	SUCCESS if the TMC cards were found and initialized.
 *	FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetHppiInit(interPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
{
    unsigned int		ctrlAddr;
    ReturnStatus		status = SUCCESS;
    NetHppiState		*statePtr;
    unsigned long		initialTsap = 0x07d00000;

    /*
     * First, see if the controller is present.  We will *always* map it
     * in, since we need to map in two different controllers--the HPPI-S
     * and HPPI-D have different addresses.  The controller address
     * specified will be the setting of bits 23:16 on the boards (see
     * their documentation for specifics)
     */
    ctrlAddr = (unsigned int) interPtr->ctrlAddr;

    printf ("NetHppiInit: Trying to locate HPPI boards at VME24D32 0x%x.\n",
	    ctrlAddr);
    if (numHppiInterfaces == 0) {
	int i;
	for (i = 0; i < NET_HPPI_MAX_INTERFACES; i++) {
	    hppiState[i] = NULL;
	}
    } else if (numHppiInterfaces > NET_HPPI_MAX_INTERFACES) {
	printf ("NetHppiInit: too many HPPI interfaces\n");
	status = FAILURE;
	goto initFailed;
    }
    statePtr = (NetHppiState *)malloc (sizeof (NetHppiState));

    if (statePtr == NULL) {
	panic ("NetHppiInit: unable to allocate state area\n");
    }
    hppiState[numHppiInterfaces] = statePtr;

    /*
     * Map in boards and attempt to reset them.  If the reset fails, then
     * the boards probably aren't actually there.  NOTE:  the last
     * parameter to VmMach_MapInDevice is the address space.  3 corresponds
     * to VME A24D32.
     */
    statePtr->hppisReg = (NetHppiSrcReg *)
	VmMach_MapInDevice((Address) (ctrlAddr + NET_HPPI_SRC_CTRL_OFFSET), 3);
    statePtr->hppidReg = (NetHppiDestReg *)
	VmMach_MapInDevice((Address) (ctrlAddr + NET_HPPI_DST_CTRL_OFFSET), 3);
    statePtr->iopReg = (NetHppiIopReg *)
	VmMach_MapInDevice((Address) (ctrlAddr + NET_HPPI_IOP_CTRL_OFFSET), 3);

    if ((statePtr->hppidReg == NULL) || (statePtr->hppisReg == NULL)) {
	printf ("NetHppiInit: Unable to get map space for boards.\n");
	status = FAILURE;
	goto initFailed;
    } else {
	printf ("NetHppiInit: SRC at virtual 0x%x; DST at virtual 0x%x \n",
		statePtr->hppisReg, statePtr->hppidReg);
    }

    /*
     * Reset both boards...
     */
    statePtr->magic = NET_HPPI_STATE_MAGIC;
    statePtr->interPtr = interPtr;
    interPtr->interfaceData = (ClientData) statePtr;
    statePtr->tracePtr = statePtr->traceBuffer;
    statePtr->traceSequence = 0;
    statePtr->queuesInit = FALSE;
    statePtr->priority = NET_HPPI_INTERRUPT_PRIORITY;
    statePtr->requestLevel = NET_HPPI_VME_REQUEST_LEVEL;
    statePtr->addressSpace = NET_HPPI_VME_ADDRESS_SPACE;
    statePtr->hppiNum = numHppiInterfaces;
    statePtr->boardFlags = 0;
    bcopy (&initialTsap, statePtr->curTsap, sizeof (statePtr->curTsap));
    Mach_SetHandler (interPtr->vector, Net_Intr, (ClientData) interPtr);
    interPtr->init 	= NetHppiInit;
    interPtr->intr	= NetHppiIntr;
    interPtr->ioctl	= NetHppiIOControl;
    interPtr->reset 	= Net_HppiReset;
    interPtr->output 	= NetHppiOutputStub;
    interPtr->netType	= NET_NETWORK_ULTRA;
    interPtr->maxBytes	= NET_HPPI_MAX_BYTES;
    interPtr->minBytes	= NET_HPPI_MIN_BYTES;
    interPtr->ctrlAddr = (Address) ctrlAddr;
    status = NetHppiHardReset (interPtr);
    if (status != SUCCESS) {
	printf ("NetHppiInit: board reset failed (boards not present?)\n");
	goto initFailed;
    }
#if 0
    status = NetHppiInfo (statePtr);
    if (status == SUCCESS) {
	printf ("Hppi boards successfully got info.\n");
    } else {
	printf ("NetHppiInit: unable to get info from board.\n");
	status = FAILURE;
	goto initFailed;
    }
#endif


  initFailed:
    numHppiInterfaces += 1;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiHardReset --
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
NetHppiHardReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    NetHppiState	*statePtr;	/* State of the adapter. */
    ReturnStatus	status = SUCCESS;
    int			resetVal;
    int			srcStatus = SUCCESS, dstStatus = SUCCESS;

    if (netHppiDebug) {
	printf ("NetHppiHardReset: entering....\n");
    }
    resetVal = NET_HPPI_RESET_BOARD | NET_HPPI_RESET_CPU;
    statePtr = (NetHppiState *) interPtr->interfaceData;
    printf("Hppi: hard reset boards.\n");

    srcStatus = Mach_Probe(sizeof(int), (char *) &resetVal,
		(char *) &(statePtr->hppisReg->reset));

    dstStatus = Mach_Probe(sizeof(int), (char *) &resetVal,
		(char *) &(statePtr->hppidReg->reset));

    if (srcStatus != SUCCESS) {
	/*
	 * Board is no longer responding.
	 */
	printf("NetHppiHardReset: HPPI-S did not respond to reset!! 0x%x\n",
	       srcStatus);
	statePtr->flags = 0;
	status = FAILURE;
    }

    if (dstStatus != SUCCESS) {
	/*
	 * Board is no longer responding.
	 */
	printf("NetHppiHardReset: HPPI-D did not respond to reset!! 0x%x\n",
	       dstStatus);
	statePtr->flags = 0;
	status = FAILURE;
    }

    if (status != SUCCESS) {
	return status;
    }

    MACH_DELAY(NET_HPPI_RESET_DELAY);

    statePtr->hppisReg->reset = 0;
    statePtr->hppidReg->reset = 0;
    statePtr->hppisReg->config = NET_HPPI_SRC_CONFIG_VALUE;
    statePtr->hppidReg->config = NET_HPPI_DST_CONFIG_VALUE;
    statePtr->outputCallback = NULL;
    InitQueues(statePtr);
    /*
     * After a reset the adapter is in the EPROM mode.
     * This allows several additional commands (load, go, etc.) to be
     * sent to the adapter.
     */
    statePtr->flags = (NET_HPPI_STATE_EXIST | NET_HPPI_STATE_SRC_EPROM |
		       NET_HPPI_STATE_DST_EPROM);

    if (netHppiDebug) {
	printf ("NetHppiHardReset: exiting....\n");
    }

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiReset --
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
NetHppiReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    ReturnStatus	status = SUCCESS;
    NetHppiState	*statePtr;	/* State of the adapter. */
    Dev_HppiReset	resetCmd;	/* reset command for both src & dst */
    int which = 0;			/* reset SRC, DST, or both */

    statePtr = (NetHppiState *) interPtr->interfaceData;
    if (which == 0) {
	which = NET_HPPI_SRC_CMD | NET_HPPI_DST_CMD;
    }
    if (netHppiDebug) {
	if (which & NET_HPPI_SRC_CMD) {
	    printf ("NetHppiReset: resetting SRC board.\n");
	}
	if (which & NET_HPPI_DST_CMD) {
	    printf ("NetHppiReset: resetting DST board.\n");
	}
    }

    resetCmd.hdr.opcode = DEV_HPPI_RESET;
    if (which & NET_HPPI_SRC_CMD) {
	resetCmd.hdr.magic = DEV_HPPI_SRC_MAGIC;
	status = NetHppiSendCmd (statePtr, sizeof (resetCmd),
				 (Address)&resetCmd, NET_HPPI_SRC_CMD);
	if (status != SUCCESS) {
	    printf ("NetHppiReset: soft reset of SRC board failed.\n");
	    return (status);
	}
    }

    if (which & NET_HPPI_DST_CMD) {
	resetCmd.hdr.magic = DEV_HPPI_DEST_MAGIC;
	status = NetHppiSendCmd (statePtr, sizeof (resetCmd),
				 (Address)&resetCmd, 0);
	if (status != SUCCESS) {
	    printf ("NetHppiReset: soft reset of DST board failed.\n");
	}
    }

    InitQueues(statePtr);

    statePtr->flags = NET_HPPI_STATE_EXIST;

    if (which & NET_HPPI_RESET_RESTART) {
	status = NetHppiSetupBoard (statePtr);

	status = NetHppiStop(statePtr);
	if (status != NULL) {
	    printf("NetHppiReset: stop failed\n");
	    return status;
	}

	status = NetHppiStart(statePtr);
	if (status != NULL) {
	    printf("NetHppiReset: start failed\n");
	    return status;
	}
    }
    if (netHppiDebug) {
	printf ("NetHppiReset: reset done...returning.\n");
    }

    return (status);
}

/*----------------------------------------------------------------------
 *
 * Net_HppiReset --
 *
 *	Reset the HPPI boards.  This can be called
 * 	from outside the module since it locks the mutex.
 *
 * Results:
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Net_HppiReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    MASTER_LOCK(&interPtr->mutex);
    /*
     * If we are at interrupt level we have to do a callback to reset
     * the adapter since we can't wait for the response from
     * the adapter (there may not be a current process and we can't
     * get the interrupt).
     */
    if (Mach_AtInterruptLevel()) {
	Proc_CallFunc(NetHppiResetCallback, (ClientData) interPtr, 0);
    } else {
	(void) NetHppiReset(interPtr);
    }
    MASTER_UNLOCK(&interPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiResetCallback --
 *
 *	This routine is called by the Proc_ServerProc during the
 *	callback to reset the adapter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The adapter is reset.
 *
 *----------------------------------------------------------------------
 */

static void
NetHppiResetCallback(data, infoPtr)
    ClientData		data;		/* Ptr to the interface to reset. */
    Proc_CallInfo	*infoPtr;	/* Unused. */
{
    Net_HppiReset((Net_Interface *) data);
}


/*
 *----------------------------------------------------------------------
 *
 * Net_HppiHardReset --
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
Net_HppiHardReset(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    MASTER_LOCK(&interPtr->mutex);

    NetHppiHardReset(interPtr);

    MASTER_UNLOCK(&interPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * getSgTag --
 *
 *	Get a free tag for the scatter-gather array.  This assumes that
 *	the mutex is held by the calling routine.
 *
 * Results:
 *	The tag to use (or -1 if none available).
 * Side effects:
 *	A tag is allocated.
 *
 *----------------------------------------------------------------------
 */
static
int
getSgTag (statePtr)
NetHppiState*	statePtr;
{
    int		i;
    int		curTag;

    for (i = 0, curTag = statePtr->curSgTag; statePtr->tags[curTag] != 0;
	 curTag ++, i++) {
	if (curTag == NET_HPPI_MAX_TAGS) {
	    curTag = 1;
	}
	if (i == NET_HPPI_MAX_TAGS - 1) {
	    printf ("getSgTag: no free tags!\n");
	    return (-1);
	}
    }
    statePtr->curSgTag = curTag;
    statePtr->tags[curTag] = 1;

    return (curTag);
}

static
void
freeSgTag (statePtr, tag)
NetHppiState*	statePtr;
char		tag;
{
    if (netHppiDebug) {
	printf ("freeing hppi tag %d\n", tag);
    }
    statePtr->tags[tag] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiCopyFromFifo --
 *
 *	Copy a command to the appropriate FIFO (its address is passed).
 *	This is similar to bcopy, except all the words in the source
 *	address are copied to the (single) address corresponding to
 *	a HPPI board FIFO.
 *
 * Results:
 *	SUCCESS if the adapter responded to the command,
 *	DEV_TIMEOUT if the adapter did not respond
 *
 * Side effects:
 *	The data is copied to the appropriate FIFO.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiCopyFromFifo (addr, size, statePtr, which)
    uint32	*addr;		/* the command to be sent */
    int		size;		/* size of the command (in bytes) */
    NetHppiState *statePtr;
    int		which;		/* copy from SRC or dst fifo? */
{
    volatile uint32 *fifoAddr;	/* address of the FIFO to copy to */
    volatile uint32 *stateAddr;	/* address of state register */
    register uint32 *kaddr = addr;
    ReturnStatus status = SUCCESS;
    int i;

    if (which == NET_HPPI_SRC_CMD) {
	stateAddr = &(statePtr->hppisReg->status);
	fifoAddr = &(statePtr->hppisReg->outputFifo);
    } else {
	stateAddr = &(statePtr->hppidReg->status);
	fifoAddr = &(statePtr->hppidReg->outputFifo);
    }

    if (netHppiDebug) {
	printf ("NetHppiCopyFromFifo: trying to copy %d bytes from fifo.\n",
		size);
    }
    for (i = 0; i < size; i += 4) {
	WAIT_FOR_BIT_CLEAR (stateAddr, NET_HPPI_DELAY, DEV_HPPI_OFIFO_EMPTY);
	if ((*stateAddr) & DEV_HPPI_OFIFO_EMPTY) {
	    if (netHppiDebug) {
		printf ("\nNetHppiCopyFromFifo: copy timed out.\n");
	    }
	    return (DEV_TIMEOUT);
	}
	*addr = *fifoAddr;
	if (netHppiDebug) {
	    printf ("0x%08x ", *addr);
	    if ((i % 32) == 31) {
		printf ("\n");
	    }
	}
	addr++;
    }

    if (netHppiDebug) {
	printf ("\nNetHppiCopyFromFifo: copied %d bytes from fifo to 0x%x\n",
		size, kaddr);
    }
    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiCopyToFifo --
 *
 *	Copy a command to the appropriate FIFO (its address is passed).
 *	This is similar to bcopy, except all the words in the source
 *	address are copied to the (single) address corresponding to
 *	a HPPI board FIFO.
 *
 * Results:
 *	SUCCESS if the adapter responded to the command,
 *	DEV_TIMEOUT if the adapter did not respond
 *
 * Side effects:
 *	The data is copied to the appropriate FIFO.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiCopyToFifo (addr, size, statePtr, which)
    uint32	*addr;		/* the command to be sent */
    int		size;		/* size of the command (in bytes) */
    NetHppiState *statePtr;	/* ptr to HPPI state */
    int		which;		/* copy to src or dest fifo? */
{
    volatile uint32 *fifoAddr;	/* address of the FIFO to copy to */
    volatile uint32 *stateAddr;	/* address of state register */
    register uint32 *kaddr = addr;
    register int cnt, xferCnt, i;
    ReturnStatus status = SUCCESS;

    if (which == NET_HPPI_SRC_CMD) {
	stateAddr = &(statePtr->hppisReg->status);
	fifoAddr = &(statePtr->hppisReg->inputFifo);
    } else {
	stateAddr = &(statePtr->hppidReg->status);
	fifoAddr = &(statePtr->hppidReg->inputFifo);
    }

    cnt = size;
    while (cnt > 0) {
	xferCnt = (cnt > (DEV_HPPI_IFIFO_DEPTH/2 - 16)) ?
		   (DEV_HPPI_IFIFO_DEPTH/2 - 16) : cnt;
	for (i = 0; i < xferCnt; i += 4, cnt -= 4) {
	    *fifoAddr = *(addr++);
	}
	if (cnt > 0) {
	    WAIT_FOR_BIT_CLEAR (stateAddr, NET_HPPI_DELAY, DEV_HPPI_IFIFO_HF);
	    if ((*stateAddr) & DEV_HPPI_IFIFO_HF) {
#ifndef CLEAN
		if (netHppiDebug) {
		    printf ("NetHppiCopyToFifo: timeout after %d bytes\n",
			    size - i);
		}
#endif
		return (DEV_TIMEOUT);
	    }
	}
    }

    if (netHppiDebug) {
	printf ("NetHppiCopyToFifo: copied %d bytes from 0x%x to fifo\n",
		size, kaddr);
    }
    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiReleaseSrcFifo
 *
 *	Release the HPPI-S command FIFO.  It must have been previously
 *	acquired by NetHppiAcquireSrcFifo.
 *
 * Results:
 *	SUCCESS if the adapter responded to the command,
 *
 * Side effects:
 *	The server releases control of the HPPI-S command FIFO.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiReleaseSrcFifo (statePtr)
    NetHppiState	*statePtr;	/* state of the adapter */
{
    ReturnStatus status = SUCCESS;

    if (!(statePtr->flags & NET_HPPI_OWN_SRC_FIFO)) {
	printf ("NetHppiReleaseSrcFifo: don't own SRC FIFO.\n");
    }

#if 0
    statePtr->hppidReg->commandData = 0x0;
    statePtr->flags &= ~NET_HPPI_OWN_SRC_FIFO;
#endif

    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiAcquireSrcFifo
 *
 *	Arbitrate with the HPPI-D board to get access to the HPPI-S
 *	FIFO.  This is done so the HPPI-D and the server don't
 *	interleave requests in the HPPI-S command FIFO.
 *
 * Results:
 *	SUCCESS if the adapter responded to the command,
 *	DEV_TIMEOUT if the adapter did not respond within a specified time.
 *
 * Side effects:
 *	The server gets control of the HPPI-S command FIFO.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiAcquireSrcFifo (statePtr, timeOutVal)
    NetHppiState	*statePtr;	/* state of the adapter */
    int			timeOutVal;	/* maximum time to wait */
{
    ReturnStatus status = SUCCESS;
#if 0
    volatile NetHppiDestReg *destReg = statePtr->hppidReg;
#endif

#if 0
    /*
     * Notify the HPPI-D that we want to talk to the HPPI-S.
     */
    destReg->commandData = 0x10000;
    destReg->command |= 0x1000000;

    /*
     * Wait for the HPPI-D to grant our request;
     */
    while (!((destReg->responseData == 0x0) &&
	     (!(destReg->command & 0xff000000)) &&
	   (timeOutVal > 0)) {
	timeOutVal -= 1;
    }
#endif

    statePtr->flags |= NET_HPPI_OWN_SRC_FIFO;

    if (timeOutVal == 0) {
	status = NetHppiReleaseSrcFifo (statePtr);
	if (status != SUCCESS) {
	    panic ("NetHppiAcquireSrcFifo: timeout and FIFO release failed.\n");
	}
	status = DEV_TIMEOUT;
    }

    return (status);
}

/*------------------------------------------------------------
 *
 * getFreeBuffer
 *
 *	This routine removes a buffer from the list of free buffers.
 *
 *------------------------------------------------------------
 */
static
Address
getFreeBuffer (statePtr)
    NetHppiState *statePtr;
{
    List_Links *itemPtr;
    int		signal;

    while(List_IsEmpty(statePtr->freeBufferList)) {
	statePtr->bufferAvail.waiting = TRUE;
	signal = Sync_SlowMasterWait((unsigned int) &statePtr->bufferAvail,
				     &(statePtr->interPtr->mutex), TRUE);
	if (signal) {
	    return ((Address)NIL);
	}
    }
    itemPtr = List_First(statePtr->freeBufferList);
    if (itemPtr == statePtr->freeBufferList) {
	panic("NetHppi: getFreeBuffer list screwup\n");
    }
    List_Remove(itemPtr);
    
    return ((Address)itemPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiSendCmd --
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
NetHppiSendCmd(statePtr, size, cmdPtr, flags)
    NetHppiState	*statePtr;	/* State of the adapter. */
    int			size;		/* Size of command block. */
    Address		cmdPtr;		/* The command block. */
    int			flags;		/* flags pertaining to this command */
{
    ReturnStatus	status = SUCCESS;
    Dev_HppiCmdHdr	*cmdHdr = (Dev_HppiCmdHdr *)cmdPtr;

    /*
     * We must copy the command to the adapter ourselves, so there's
     * no point in mapping the command into DMA memory.  If the command
     * goes to the destination, we just copy the command into the
     * destination's FIFO.  If it goes into the source, we must
     * arbitrate for control of the FIFO.  We can't wait for results
     * since incoming packet information could arrive first.
     */
    if (netHppiDebug) {
	printf("NetHppiSendCmd: sending command %d to adapter\n",
	    cmdHdr->opcode);
	printf("NetHppiSendCmd: size = %d, cmdPtr = 0x%x\n", size, cmdPtr);
    }
    if ((int) cmdPtr & 0x3) {
	panic("NetHppiSendCmd: command not aligned on a word boundary\n");
    }

    if (flags & NET_HPPI_SRC_CMD) {
	if (flags & NET_HPPI_STATE_SRC_EPROM) {
	    printf ("NetHppiSendCmd: SRC not running\n");
	    status = FAILURE;
	    goto exit;
	}
	cmdHdr->magic = DEV_HPPI_SRC_MAGIC;
	if (!(statePtr->flags & NET_HPPI_OWN_SRC_FIFO)) {
	    status = NetHppiAcquireSrcFifo (statePtr, 1000000);
	}
	if (status == SUCCESS) {
	    status = NetHppiCopyToFifo (cmdPtr, size, statePtr,
					NET_HPPI_SRC_CMD);
	}
	if (!(flags & NET_HPPI_KEEP_SRC_FIFO)) {
	    NetHppiReleaseSrcFifo (statePtr);
	}
    } else {
	if (flags & NET_HPPI_STATE_DST_EPROM) {
	    printf ("NetHppiSendCmd: DST not running\n");
	    status = FAILURE;
	    goto exit;
	}
	cmdHdr->magic = DEV_HPPI_DEST_MAGIC;
	status = NetHppiCopyToFifo (cmdPtr, size, statePtr, 0);
    }

    if (netHppiDebug) {
	printf ("NetHppiSendCmd: command sent (status = %d).\n", status);
    }

exit:
    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiMsgCallback
 *
 *	This is the routine called back by a ServerProc to handle a
 *	message from the HPPI boards.  The message can be one of three
 *	types: an error message from either HPPI-S or HPPI-D, or a
 *	request from the HPPI-D to send data to the HPPI-S.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The message is retrieved from the HPPI board(s) and appropriate
 *	actions are taken.
 *
 *----------------------------------------------------------------------
 */
static
void
NetHppiMsgCallback (data, infoPtr)
    ClientData		data;
    Proc_CallInfo	*infoPtr;
{
    Net_Interface	*interPtr = (Net_Interface *)data;
    NetHppiState	*statePtr = (NetHppiState *)interPtr->interfaceData;
    Dev_HppiErrorMsg	errMsg;
    int			size;
    int			numElements;
    int			i;
    int			status = SUCCESS;

    if (netHppiDebug) {
	printf ("NetHppiMsgCallback: entering with data = 0x%x\n", data);
    }
    if (!(statePtr->flags &
	  (NET_HPPI_STATE_SRC_ERROR | NET_HPPI_STATE_DST_ERROR))) {
	Dev_HppiCopyDataMsg	copyDataMsg;
	Dev_HppiOutput		outputCmd;

	size = sizeof (copyDataMsg.size) + sizeof (copyDataMsg.magic) +
	    sizeof (copyDataMsg.dmaWord);

	/*
	 * Copy the entire message from the destination board.  Since
	 * we can only get one type of message, we don't need to check
	 * message type except for verification.
	 */
	NetHppiCopyFromFifo (&copyDataMsg, size, statePtr, 0);
	if (copyDataMsg.magic != DEV_HPPI_COPY_MAGIC) {
	    printf ("NetHppiMsgCallback: bad copyToSource magic number\n");
	    goto exit;
	}
	NetHppiCopyFromFifo (&(copyDataMsg.element[0]), copyDataMsg.size,
			     statePtr, 0);
	if (netHppiTrace) {
	    outputCmd.hdr.opcode = DEV_HPPI_OUTPUT_TRACE;
	} else {
	    outputCmd.hdr.opcode = DEV_HPPI_OUTPUT;
	}

	numElements = copyDataMsg.size / sizeof (copyDataMsg.element[0]);
	for (i = 0, size = sizeof (copyDataMsg.dmaWord);
	     i < numElements; i++) {
	    size += copyDataMsg.element[i].size;
	}
	copyDataMsg.dmaWord.cmd &= ~NET_ULTRA_DMA_CMD_FROM_ADAPTER;
	copyDataMsg.dmaWord.cmd |= NET_ULTRA_DMA_CMD_DMA_DATA;
	outputCmd.hdr.magic = DEV_HPPI_SRC_MAGIC;
	outputCmd.fifoDataSize = size;
	outputCmd.iopDataSize = 0;
	NetHppiSendCmd (statePtr, sizeof (outputCmd), (Address)&outputCmd,
			NET_HPPI_SRC_CMD | NET_HPPI_KEEP_SRC_FIFO);
	NetHppiCopyToFifo (&(copyDataMsg.dmaWord),
			   sizeof (copyDataMsg.dmaWord), statePtr,
			   NET_HPPI_SRC_CMD);
	/*
	 * Copy scatter-gather buffers to input FIFO.
	 */
	for (i = 0; i < numElements; i++) {
	    NetHppiCopyToFifo (VME_TO_BUFFER (copyDataMsg.element[i].address,
					      statePtr),
			       copyDataMsg.element[i].size, statePtr,
			       NET_HPPI_SRC_CMD);
	}
	NetHppiReleaseSrcFifo (statePtr);
    } else {
	if (statePtr->flags & NET_HPPI_STATE_SRC_ERROR) {
	    status = NetHppiCopyFromFifo (&errMsg, sizeof (errMsg),
					  statePtr, NET_HPPI_SRC_CMD);
	    if (errMsg.magic != DEV_HPPI_ERR_MAGIC) {
		printf ("NetHppiMsgCallback: bad SRC error magic number\n");
		goto exit;
	    }
	    if (netHppiDebug) {
		printf ("NetHppiMsgCallback: SRC error (%d words)\n",
			errMsg.errorInfoLength);
	    }
	    status = NetHppiCopyFromFifo (&(statePtr->srcErrorBuffer),
					  (errMsg.errorInfoLength << 2),
					  statePtr, NET_HPPI_SRC_CMD);
	}

	if (statePtr->flags & NET_HPPI_STATE_DST_ERROR) {
	    status = NetHppiCopyFromFifo (&errMsg, sizeof (errMsg), statePtr,
					  NET_HPPI_SRC_CMD);
	    if (errMsg.magic != DEV_HPPI_ERR_MAGIC) {
		printf ("NetHppiMsgCallback: bad DST error magic number\n");
		goto exit;
	    }
	    if (netHppiDebug) {
		printf ("NetHppiMsgCallback: DST error (%d words)\n",
			errMsg.errorInfoLength);
	    }
	    status = NetHppiCopyFromFifo (&(statePtr->dstErrorBuffer),
					  (errMsg.errorInfoLength << 2),
					  statePtr, 0);
	}
    }

  exit:
    if (status != SUCCESS) {
	printf ("NetHppiMsgCallback: error 0x%x occurred\n", status);
    }
    if (netHppiDebug) {
	printf ("NetHppiMsgCallback: returning....\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiLoopback
 *
 *	Set up the HIPPI boards for loopback testing.  This can either
 *	mean a full loopback from xbus to hipppi and back, or a partial
 *	loop from SRC -> DST -> XBUS.
 *
 * Returns:
 *	none
 * Side effects:
 *	Sends commands to the HIPPI boards.
 *
 *----------------------------------------------------------------------
 */
static
void
NetHppiLoopback (statePtr, size, callbackProc, clientData, srcPat)
NetHppiState*	statePtr;
int	size;		/* size in BYTES */
void	(*callbackProc)();
ClientData clientData;
int	srcPat;
{
    Dev_HppiOutput	outputCmd;
    Dev_HppiOutputPattern patternCmd;

    if (statePtr->outputCallback != NULL) {
	(callbackProc)(clientData, FAILURE);
    } else {
	statePtr->outputCallback = callbackProc;
	statePtr->outputClientData = clientData;
	outputCmd.fifoDataSize = 0;
	outputCmd.iopDataSize = size;
	outputCmd.hdr.opcode = DEV_HPPI_OUTPUT_TO_IOP;
	NetHppiSendCmd (statePtr, sizeof (outputCmd), (Address)&outputCmd,
			NET_HPPI_DST_CMD);
	if (srcPat == 0) {
	    outputCmd.hdr.opcode = DEV_HPPI_OUTPUT;
	    NetHppiSendCmd (statePtr, sizeof (outputCmd), (Address)&outputCmd,
			    NET_HPPI_SRC_CMD);
	} else if (srcPat == 1) {
	    patternCmd.hdr.opcode = DEV_HPPI_OUTPUT_PATTERN;
	    patternCmd.size = size;
	    patternCmd.start = 0;
	    patternCmd.increment = 0x00010001;
	    NetHppiSendCmd (statePtr, sizeof (patternCmd),(Address)&patternCmd,
			    NET_HPPI_SRC_CMD);
	}
    }
}

static
void
iopOutputCallback (statePtr, status)
NetHppiState*	statePtr;
ReturnStatus	status;
{
    MASTER_LOCK (&(statePtr->interPtr->mutex));
    if (!(statePtr->flags & NET_HPPI_STATE_IOP_OUTPUT_TEST)) {
	printf ("Hppi iopOutputCallback: not testing iop?\n");
    }
    statePtr->flags &= ~NET_HPPI_STATE_IOP_OUTPUT_TEST;
    Sync_MasterBroadcast (&iopOutputDone);
    MASTER_UNLOCK (&(statePtr->interPtr->mutex));
}

/*
 *----------------------------------------------------------------------
 *
 * Net_HppiLoopback
 *
 *	This routine is intended ONLY for debugging.  It calls NetHppiLoopback
 *	to send data from IOP bus -> HIPPI-S -> HIPPI-D -> IOP bus.
 *	The supplied callback routine will be called when the
 *	transfer completes.  Note that the callback may occur at interrupt
 *	level.  Also, only one of these transfers may be outstanding at any
 *	time.  If this is called while there is still an outstanding output
 *	command, the callback will immediately be called with FAILURE status.
 *
 * Returns:
 *	none
 * Side effects:
 *	Commands are sent to the HIPPI-D and HIPPI-S boards.
 *
 *----------------------------------------------------------------------
 */
void
Net_HppiLoopback (hippiNum, size, callbackProc, clientData)
int	hippiNum;
int	size;		/* size in BYTES */
void	(*callbackProc)();
ClientData clientData;
{
    NetHppiState*	statePtr;

    if (netHppiDebug) {
	printf ("Net_HppiLoopback: called for unit %d.\n", hippiNum);
    }
    if ((hippiNum >= NET_HPPI_MAX_INTERFACES) ||
	(statePtr = hppiState[hippiNum]) == NULL) {
	(callbackProc)(clientData, DEV_INVALID_UNIT);
	return;
    }
    MASTER_LOCK (&(statePtr->interPtr->mutex));
    NetHppiLoopback (statePtr, size, callbackProc, clientData, 0);
    MASTER_UNLOCK (&(statePtr->interPtr->mutex));
}

/*
 *----------------------------------------------------------------------
 *
 * Net_HppiSrcPattern
 *
 *	This routine is almost identical to Net_HppiLoopback, except
 *	that the HIPPI source board is set up to provide a fixed
 *	pattern rather than use data from the IOP bus.
 *
 * Returns:
 *	none
 * Side effects:
 *	Sets up HIPPI boards.
 *
 *----------------------------------------------------------------------
 */
void
Net_HppiSrcPattern (hippiNum, size, callbackProc, clientData)
int	hippiNum;
int	size;		/* size in BYTES */
void	(*callbackProc)();
ClientData clientData;
{
    NetHppiState*	statePtr;

    if (netHppiDebug) {
	printf ("Net_HppiSrcPattern: called for unit %d.\n", hippiNum);
    }
    if ((hippiNum >= NET_HPPI_MAX_INTERFACES) ||
	(statePtr = hppiState[hippiNum]) == NULL) {
	(callbackProc)(clientData, DEV_INVALID_UNIT);
	return;
    }
    MASTER_LOCK (&(statePtr->interPtr->mutex));
    NetHppiLoopback (statePtr, size, callbackProc, clientData, 1);
    MASTER_UNLOCK (&(statePtr->interPtr->mutex));
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiIntr --
 *
 *	Handle an interrupt from the HPPI boards.
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
NetHppiIntr(interPtr, polling)
    Net_Interface	*interPtr;	/* Interface to process. */
    Boolean		polling;	/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    NetUltraXRB		*xrbPtr;
    NetUltraXRB		*nextXRBPtr;
    NetHppiState	*statePtr;
    volatile NetUltraDMAInfo *dmaPtr;
    NetUltraRequestHdr  *hdrPtr;
    NetUltraXRBInfo	*infoPtr;
    int			processed;
    NetUltraTraceInfo	*tracePtr;
    Address		buffer;
    int			hppiStatus;
    unsigned int	response;

    statePtr = (NetHppiState *) interPtr->interfaceData;
    response = statePtr->hppidReg->responseData;
    /*
     * Acknowledge the interrupts by setting(!) the interrupt bits in the
     * status registers.
     */
    statePtr->hppisReg->status = NET_HPPI_SRC_STATUS_INTR;
    statePtr->hppidReg->status = NET_HPPI_DST_STATUS_INTR;
    MASTER_LOCK(&interPtr->mutex);
    if (response == DEV_HPPI_INTR_MAGIC) {
	void (*cb)();
	if ((cb = statePtr->outputCallback) != NULL) {
	    if (netHppiDebug) {
		printf ("Received a HPPI testing interrupt.\n");
	    }
	    statePtr->outputCallback = NULL;
	    MASTER_UNLOCK (&interPtr->mutex);
	    (cb)(statePtr->outputClientData, SUCCESS);
	    MASTER_LOCK (&interPtr->mutex);
	} else {
	    printf ("Received a HPPI testing interrupt with no callback.\n");
	}
	goto intrExit;
    }
#ifndef CLEAN
    if (netHppiDebug) {
	printf("Received an HPPI interrupt.\n");
    }
#endif
    xrbPtr = statePtr->nextToHostPtr;
#ifndef CLEAN
    if (netHppiTrace) {
	NEXT_TRACE(statePtr, &tracePtr);
	tracePtr->event = INTERRUPT;
	Timer_GetCurrentTicks(&tracePtr->ticks);
    }
    if (netHppiDebug) {
	printf ("NetHppiIntr: xrbPtr = 0x%x\n", xrbPtr);
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
#ifndef CLEAN
	if (netHppiDebug) {
	    printf ("NetHppiIntr: xrbPtr = 0x%x\n", xrbPtr);
	}
#endif
	if (xrbPtr->filled == DEV_HPPI_FILLED_CMD) {
	    Dev_HppiCmdHdr*	hdr;
	    statePtr->nextToHostPtr = nextXRBPtr;
#ifndef CLEAN
	    if (netHppiDebug) {
		hdr = (Dev_HppiCmdHdr*)&(xrbPtr->request);
		printf ("NetHppiIntr: processing command 0x%x\n", hdr->opcode);
	    }
#endif
	    /*
	     * This could get complicated, and will always require callbacks
	     * anyway, so just schedule callback to process the return.  The
	     * command will always have an xrbId and xrbBufId which identify
	     * the originator of the command.  Don't clear the filled field;
	     * that will be done by the callback.
	     */
	    Proc_CallFunc (NetHppiCmdCallback, (ClientData)xrbPtr, 0);
	    processed += 1;
	} else {
	    hdrPtr = (NetUltraRequestHdr *) &xrbPtr->request;
	    infoPtr = statePtr->tagToXRBInfo[(int)hdrPtr->infoPtr];
	    statePtr->nextToHostPtr = nextXRBPtr;
	    dmaPtr = &xrbPtr->dma;
	    if (!(dmaPtr->cmd & NET_ULTRA_DMA_CMD_FROM_ADAPTER)) {
		printf("NetHppiIntr: cmd (0x%x) not from adapter?\n", dmaPtr->cmd);
		goto endLoop;
	    }
	    if ((dmaPtr->cmd & NET_ULTRA_DMA_CMD_MASK) != NET_ULTRA_DMA_CMD_XRB) {
		printf("NetHppiIntr: dmaPtr->cmd = 0x%x\n", dmaPtr->cmd);
		goto endLoop;
	    }
	    if (infoPtr->flags & NET_ULTRA_INFO_PENDING) {
#ifndef CLEAN
		if (netHppiDebug) {
		    printf("NetHppiIntr: processing 0x%x\n", infoPtr);
		}
		if (netHppiTrace) {
		    NEXT_TRACE(statePtr, &tracePtr);
		    tracePtr->event = PROCESS_XRB;
		    tracePtr->index = xrbPtr - statePtr->firstToHostPtr;
		    tracePtr->infoPtr = infoPtr;
		    Timer_GetCurrentTicks(&tracePtr->ticks);
		}
#endif
		infoPtr->xrbPtr = xrbPtr;
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
		if (netHppiTrace) {
		    NEXT_TRACE(statePtr, &tracePtr);
		    tracePtr->event = INFO_NOT_PENDING;
		    tracePtr->index = xrbPtr - statePtr->firstToHostPtr;
		    tracePtr->infoPtr = infoPtr;
		}
#endif
	    }
	}
endLoop: 
	xrbPtr->filled = 0;
	xrbPtr = nextXRBPtr;
    }


    /*
     * Check to see if the HPPI-D board has a message for us.
     */
    hppiStatus = statePtr->hppidReg->status;
    if ((hppiStatus & NET_HPPI_DST_STATUS_ALIVE_MASK) ==
	NET_HPPI_DST_STATUS_ERROR) {
	statePtr->flags |= NET_HPPI_STATE_DST_ERROR;
	Proc_CallFunc (NetHppiMsgCallback, (ClientData)interPtr, 0);
    } else if (!(hppiStatus & DEV_HPPI_OFIFO_EMPTY)) {
	Proc_CallFunc (NetHppiMsgCallback, (ClientData)interPtr, 0);
    }
    
    /*
     * Check to see if the HPPI-S board has an error message for us.
     */
    hppiStatus = statePtr->hppisReg->status;
    if ((hppiStatus & NET_HPPI_SRC_STATUS_ALIVE_MASK) ==
	NET_HPPI_SRC_STATUS_ERROR) {
	statePtr->flags |= NET_HPPI_STATE_SRC_ERROR;
	Proc_CallFunc (NetHppiMsgCallback, (ClientData)interPtr, 0);
    }

#ifndef CLEAN
    if (processed == 0) {
	if (netHppiDebug) {
	    printf("NetHppiIntr: didn't process any packets.\n");
	}
    }
#endif
  intrExit:
    MASTER_UNLOCK(&interPtr->mutex);

}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiSetupBoard
 *
 *	Send a setup command to the HPPI-D board.  This should only be done
 *	immediately after code is downloaded and run on the HPPI-D board.
 *
 * Results:
 *	SUCCESS if command went OK
 *	various failure codes otherwise
 *
 * Side effects:
 *	A setup command is sent to the adapter.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiSetupBoard (statePtr)
    NetHppiState	*statePtr;
{
    Dev_HppiSetup	setupCmd;
    ReturnStatus	status = SUCCESS;

    /*
     * set up the configuration registers appropriately, in case
     * they were cleared when the link board was reset.
     *
     */
    statePtr->hppisReg->config = NET_HPPI_SRC_CONFIG_VALUE;
    statePtr->hppidReg->config = NET_HPPI_DST_CONFIG_VALUE;

    setupCmd.hdr.opcode = DEV_HPPI_SETUP;
    setupCmd.hdr.magic = DEV_HPPI_DEST_MAGIC;
    setupCmd.reqBlockSize = sizeof (NetUltraXRB);
    setupCmd.queueAddress = (uint32)(statePtr->firstToHostVME);
    setupCmd.queueSize = NET_HPPI_NUM_TO_HOST;
    status = NetHppiSendCmd (statePtr, sizeof (setupCmd),
			     (Address)&setupCmd, 0);

    return (status);
}

/*----------------------------------------------------------------------
 *
 * NetHppiRomCmd --
 *
 *	Send a command to the ROM monitor of one of the boards.  After
 *	the command is sent, the routine busy-waits for a reply.
 *
 * Results:
 *	SUCCESS if the command was sent OK.
 *	DEV_TIMEOUT if the device didn't complete the command
 *
 * Side effects:
 *	A command is sent to one of the HPPI boards.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiRomCmd (interPtr, cmdBufPtr, cmdBufSize, responsePtr, responseSizePtr,
	       flags)
    Net_Interface	*interPtr;
    int			*cmdBufPtr;
    int			cmdBufSize;
    int			*responsePtr;
    int			*responseSizePtr;
    int			flags;
{
    NetHppiState	*statePtr = (NetHppiState *)interPtr->interfaceData;
    ReturnStatus	status = SUCCESS;
    volatile uint32	*boardStatus;
    int			which;
    int			responseSize;
    int			garbage;
    int			excess = 0;

    if (netHppiDebug) {
	printf ("NetHppiRomCmd: starting cmd.\n");
    }

    if (flags & NET_HPPI_SRC_CMD) {
	if (!(statePtr->flags & NET_HPPI_STATE_SRC_EPROM)) {
	    printf ("NetHppiRomCmd: SRC board not running EPROM code\n");
	    return (FAILURE);
	}
	which = NET_HPPI_SRC_CMD;
	boardStatus = &(statePtr->hppisReg->status);
    } else {
	if (!(statePtr->flags & NET_HPPI_STATE_DST_EPROM)) {
	    printf ("NetHppiRomCmd: DST board not running EPROM code\n");
	    return (FAILURE);
	}
	which = 0;
	boardStatus = &(statePtr->hppidReg->status);
    }

    responseSize = 0;
    MASTER_LOCK (&interPtr->mutex);

    NetHppiCopyToFifo (cmdBufPtr, cmdBufSize, statePtr, which);

    WAIT_FOR_BIT_CLEAR (boardStatus, NET_HPPI_DELAY, DEV_HPPI_OFIFO_EMPTY);

    status = NetHppiCopyFromFifo (&responseSize, sizeof (responseSize),
				  statePtr, which);
    if (status != SUCCESS) {
	printf ("NetHppiRomCmd: command timed out.\n");
	status = DEV_TIMEOUT;
	goto exit;
    }
    *(responsePtr++) = responseSize;
    responseSize -= 4;
    if (responseSize > (*responseSizePtr)) {
	excess = responseSize - *responseSizePtr;
	responseSize = *responseSizePtr;
    }

    status = NetHppiCopyFromFifo (responsePtr, responseSize, statePtr, which);
    while (excess > 0) {
	(void)NetHppiCopyFromFifo (&garbage, sizeof(garbage), statePtr, which);
	excess -= sizeof (garbage);
    }

    /*
     * Correct so the size includes the first (size) word
     */
    *responseSizePtr = responseSize + 4;

exit:	
    MASTER_UNLOCK (&interPtr->mutex);

    if (netHppiDebug) {
	printf ("NetHppiRomCmd: returning...\n");
    }
    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiIOControl --
 *
 *	Handle ioctls for the HPPI boards.
 *
 * Results:
 *	SUCCESS if the ioctl was performed successfully, a standard
 *	Sprite error code otherwise.
 *
 * Side effects:
 *	Commands may be sent to the adapter and/or the either HPPI board,
 *	and the adapter state and board states may change.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
NetHppiIOControl(interPtr, ioctlPtr, replyPtr)
    Net_Interface *interPtr;	/* Interface on which to perform ioctl. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ReturnStatus status = SUCCESS;
    int	fmtStatus;
    int inSize;
    int outSize;
    NetHppiState	*statePtr = (NetHppiState *) interPtr->interfaceData;

    if (netHppiDebug) {
	printf("NetHppiIOControl: command = 0x%x\n", ioctlPtr->command);
    }
    if ((ioctlPtr->command & ~0xffff) != IOC_HPPI) {
	return DEV_INVALID_ARG;
    }
    switch(ioctlPtr->command) {
	case IOC_HPPI_SET_FLAGS:
	case IOC_HPPI_RESET_FLAGS:
	case IOC_HPPI_GET_FLAGS:
	    return GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_HPPI_DEBUG: {
	    int	value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_HPPI_DEBUG parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    netHppiDebug = value;
	    break;
	}

	case IOC_HPPI_TRACE: {
	    int	value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_HPPI_TRACE parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    netHppiTrace = value;
	    break;
	}
	case IOC_HPPI_RESET: 
	    Net_HppiReset(interPtr);
	    break;
	case IOC_HPPI_HARD_RESET:
	    Net_HppiHardReset(interPtr);
	    break;
#if 0
	case IOC_HPPI_SRC_RESET:
	    break;
	case IOC_HPPI_DST_RESET:
	    break;
#endif
	case IOC_HPPI_START: {
	    MASTER_LOCK(&interPtr->mutex);
	    status = NetHppiStart(statePtr);
	    MASTER_UNLOCK(&interPtr->mutex);
	    break;
	}
	case IOC_HPPI_GET_ADAP_INFO:
	    return GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_HPPI_DIAG:
	case IOC_HPPI_EXTENDED_DIAG:
	    return GEN_NOT_IMPLEMENTED;
	    break;

	case IOC_HPPI_WRITE_REG: {
	    Dev_HppiRegCmd writeCmd;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (writeCmd);
	    fmtStatus = Fmt_Convert ("www", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&writeCmd);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of HPPI_WRITE_REG parameter failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    if (writeCmd.board == DEV_HPPI_SRC_BOARD) {
		if (writeCmd.offset >= sizeof (NetHppiSrcReg)) {
		    printf ("HPPI_WRITE_REG: bad SRC register offset 0x%x\n",
			    writeCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof (int), (char *)&(writeCmd.value),
			(char *)(statePtr->hppisReg) + writeCmd.offset);
	    } else if (writeCmd.board == DEV_HPPI_DST_BOARD) {
		if (writeCmd.offset >= sizeof (NetHppiDestReg)) {
		    printf ("HPPI_WRITE_REG: bad DST register offset 0x%x\n",
			    writeCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof (int), (char *)&(writeCmd.value),
			(char *)(statePtr->hppidReg) + writeCmd.offset);
	    } else if (writeCmd.board == DEV_HPPI_IOP_BOARD) {
		if (writeCmd.offset >= sizeof (NetHppiIopReg)) {
		    printf ("HPPI_WRITE_REG: bad IOP register offset 0x%x\n",
			    writeCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof (int), (char *)&(writeCmd.value),
			(char *)(statePtr->iopReg) + writeCmd.offset);
	    } else {
		return GEN_INVALID_ARG;
	    }
	    break;
	}

	case IOC_HPPI_READ_REG: {
	    Dev_HppiRegCmd readCmd;
	    uint32 regValue;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (readCmd);
	    fmtStatus = Fmt_Convert ("www", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&readCmd);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of HPPI_READ_REG parameter failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    if (readCmd.board == DEV_HPPI_SRC_BOARD) {
		if (readCmd.offset >= sizeof (NetHppiSrcReg)) {
		    printf ("HPPI_READ_REG: bad SRC register offset 0x%x\n",
			    readCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof(int),(char *)(statePtr->hppisReg) +
				     readCmd.offset, (char *)&regValue);
	    } else if (readCmd.board == DEV_HPPI_DST_BOARD) {
		if (readCmd.offset >= sizeof (NetHppiDestReg)) {
		    printf ("HPPI_READ_REG: bad DST register offset 0x%x\n",
			    readCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof(int),(char *)(statePtr->hppidReg) +
				     readCmd.offset, (char *)&regValue);
	    } else if (readCmd.board == DEV_HPPI_IOP_BOARD) {
		if (readCmd.offset >= sizeof (NetHppiIopReg)) {
		    printf ("HPPI_READ_REG: bad IOP register offset 0x%x\n",
			    readCmd.offset);
		    return GEN_INVALID_ARG;
		}
		status = Mach_Probe (sizeof(int), (char *)(statePtr->iopReg) +
				     readCmd.offset, (char *)&regValue);
	    } else {
		return GEN_INVALID_ARG;
	    }
	    readCmd.value = regValue;

	    if (ioctlPtr->outBufSize < sizeof (readCmd)) {
		printf ("HPPI_READ_REG: return buffer too small\n");
		return GEN_INVALID_ARG;
	    }
	    inSize = outSize = sizeof (readCmd);
	    fmtStatus = Fmt_Convert ("www", mach_Format, &inSize,
				     (Address)&readCmd, ioctlPtr->format,
				     &outSize, (Address)ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of HPPI_READ_REG output failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    break;
	}

	case IOC_HPPI_GO: {
	    Dev_HppiGo goCmd;
	    int flags;
	    Dev_HppiRomGo hppiCmd;
	    Dev_HppiRomGoReply hppiReply;
	    int replySize;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (goCmd);
	    fmtStatus = Fmt_Convert ("w2", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&goCmd);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of HPPI GO parameter failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    /*
	     * Format the GO command
	     */
	    hppiCmd.hdr.length = sizeof (hppiCmd);
	    hppiCmd.hdr.commandId = DEV_HPPI_START_CODES;
	    hppiCmd.hdr.callerPid = 0;
	    hppiCmd.startAddress = goCmd.startAddress;

	    if ((goCmd.board != DEV_HPPI_SRC_BOARD) &&
		(goCmd.board != DEV_HPPI_DST_BOARD)) {
		printf ("IOC_HPPI_GO: bad board id (0x%x)\n", goCmd.board);
		return GEN_INVALID_ARG;
	    }
	    flags = (goCmd.board == DEV_HPPI_SRC_BOARD) ? NET_HPPI_SRC_CMD : 0;
	    replySize = sizeof (hppiReply);
	    status = NetHppiRomCmd (interPtr, &hppiCmd, sizeof (hppiCmd),
				    &hppiReply, &replySize, flags);

	    if (status != SUCCESS) {
		printf ("IOC_HPPI_GO: couldn't start code (0x%x)\n", status);
		return FAILURE;
	    }
	    if ((hppiReply.hdr.responseId != DEV_HPPI_START_CODES) ||
		(hppiReply.hdr.status != 0) ||
		(hppiReply.startAddress != goCmd.startAddress)) {
		printf ("IOC_HPPI_GO: response was wrong (r%d s%d a0x%x)\n",
			hppiReply.hdr.responseId, hppiReply.hdr.status,
			hppiReply.startAddress);
		return FAILURE;
	    }
	    if (goCmd.board == DEV_HPPI_SRC_BOARD) {
		statePtr->flags &= ~NET_HPPI_STATE_SRC_EPROM;
	    } else {
		statePtr->flags &= ~NET_HPPI_STATE_DST_EPROM;
	    }

	    if (!(statePtr->flags & (NET_HPPI_STATE_SRC_EPROM |
				     NET_HPPI_STATE_DST_EPROM))) {
		statePtr->flags |= NET_HPPI_STATE_NORMAL;
	    }
	    break;
	}

	case IOC_HPPI_LOAD: {
	    Dev_HppiLoadHdr loadCmdHdr;
	    Dev_HppiRomWrite *loadPtr;
	    Dev_HppiRomWriteReply reply;
	    int replySize = sizeof (reply);
	    int flags;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (loadCmdHdr); 
	    fmtStatus = Fmt_Convert ("w3", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&loadCmdHdr);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_HPPI_LOAD parameter failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    inSize  = outSize = loadCmdHdr.size;
	    if (inSize >= DEV_HPPI_MAX_ROM_CMD_SIZE) {
		printf ("IOC_HPPI_LOAD: load data too big (0x%x)\n", inSize);
		return GEN_INVALID_ARG;
	    }
	    loadPtr = (Dev_HppiRomWrite *)
		malloc (sizeof (Dev_HppiRomWrite) + loadCmdHdr.size);
	    if (loadPtr == NULL) {
		printf ("IOC_HPPI_LOAD: unable to allocate space\n");
		return FAILURE;
	    }
	    fmtStatus = Fmt_Convert ("w*", ioctlPtr->format, &inSize,
		(char *)ioctlPtr->inBuffer + sizeof (loadCmdHdr),
		mach_Format, &outSize, (Address)(loadPtr->data));
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_HPPI_LOAD data failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto exitLoad;
	    }
	    loadPtr->hdr.commandId = DEV_HPPI_PUT_MEMORY;
	    loadPtr->hdr.length = (sizeof (Dev_HppiRomWrite) - sizeof (int)) +
		loadCmdHdr.size;
	    loadPtr->hdr.callerPid = 0;
	    loadPtr->writeAddress = loadCmdHdr.startAddress;
	    loadPtr->writeWords = loadCmdHdr.size / sizeof(int);
	    flags =
		(loadCmdHdr.board == DEV_HPPI_SRC_BOARD) ? NET_HPPI_SRC_CMD :0;
	    status = NetHppiRomCmd (interPtr, loadPtr, loadPtr->hdr.length,
				    &reply, &replySize, flags);
	    if ((status == SUCCESS) && (
		(reply.hdr.responseId != DEV_HPPI_PUT_MEMORY) ||
		(reply.hdr.status != 0) ||
		(reply.wordsWritten != loadPtr->writeWords))) {
		printf ("IOC_HPPI_LOAD: load failed (0x%x words)\n",
			reply.wordsWritten);
		status = FAILURE;
	    }
	  exitLoad:
	    free ((Address)loadPtr);
	    break;
	}

	case IOC_HPPI_SRC_ROM_CMD:
	case IOC_HPPI_DST_ROM_CMD: {
	    int *cmdBufPtr;
	    static int replyBuf[NET_HPPI_ROM_MAX_REPLY];
	    int replySize;
	    int flags;

	    inSize = ioctlPtr->inBufSize;
	    cmdBufPtr = (int *)malloc (inSize);
	    outSize = inSize;
	    fmtStatus = Fmt_Convert ("w*", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)cmdBufPtr);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of HPPI ROM parameter failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto exitRomCmd;
	    }
	    flags = (ioctlPtr->command == IOC_HPPI_SRC_ROM_CMD) ?
		NET_HPPI_SRC_CMD : 0;
	    status = NetHppiRomCmd (interPtr, cmdBufPtr, inSize, replyBuf,
				    &replySize, flags);
	    if (status == SUCCESS) {
		if (replySize > ioctlPtr->outBufSize) {
		    printf ("IOC_HPPI_ROM_CMD: reply too long\n");
		    goto exitRomCmd;
		}
		inSize = replySize;
		fmtStatus = Fmt_Convert("w*", mach_Format, &replySize,
					(Address) replyBuf,
					ioctlPtr->format, &outSize,
					ioctlPtr->outBuffer);
		if (fmtStatus != 0) {
		    printf("Format of HPPI ROM response failed, 0x%x\n",
			   fmtStatus);
		    status = GEN_INVALID_ARG;
		}
	    } else {
		printf ("IOC_HPPI_ROM_CMD: command failed 0x%x\n", status);
	    }
	  exitRomCmd:
	    free ((Address)cmdBufPtr);
	    break;
	}
	case IOC_HPPI_SETUP: {
	    MASTER_LOCK (&interPtr->mutex);
	    status = NetHppiSetupBoard (statePtr);
	    MASTER_UNLOCK (&interPtr->mutex);
	    break;
	}

	case IOC_HPPI_SEND_DGRAM: {
	    Dev_UltraSendDgram		dgram;
	    int *tmpBuf = (int *)NIL;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(dgram);
	    fmtStatus = Fmt_Convert("{{b8}w{w2}w2b*}",
				    ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &dgram);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    if (dgram.useBuffer != TRUE) {
		int i;
		tmpBuf = (int *)malloc (dgram.size);
		for (i = 0; i < dgram.size/4; i++) {
		    *(tmpBuf + i) = i;
		}
	    } 
	    status = NetHppiSendDgram(interPtr, &dgram.address, dgram.count,
			dgram.size, 
			(dgram.useBuffer == TRUE) ? (Address) dgram.buffer :
			(Address) tmpBuf, &dgram.time);
	    free ((char *)tmpBuf);
	    inSize = sizeof(dgram);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{{b8}w{w2}w2b*}", mach_Format, &inSize,
		(Address) &dgram, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}

	case IOC_HPPI_RECV_DGRAM: {
	    if (statePtr->flags & NET_HPPI_STATE_SINK) {
		if (netHppiDebug) {
		    printf ("NetHppiIOControl: already sinking dgrams.\n");
		}
		return FAILURE;
	    }
	    statePtr->flags |= NET_HPPI_STATE_SINK;
	    statePtr->flags &= ~NET_HPPI_STATE_NORMAL;
	    Proc_CallFunc (NetHppiRecvDgram, (ClientData)interPtr, 0);
	    break;
	}

	case IOC_HPPI_ECHO: {
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
		    printf("NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    if (echo.echo == TRUE) {
		statePtr->flags |= NET_HPPI_STATE_ECHO;
		statePtr->flags &= ~NET_HPPI_STATE_NORMAL;
	    } else {
		statePtr->flags &= ~NET_HPPI_STATE_ECHO;
		statePtr->flags |= NET_HPPI_STATE_NORMAL;
	    }
	    break;
	}

	case IOC_HPPI_SOURCE: {
	    Dev_UltraSendDgram		dgram;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof(dgram);
	    fmtStatus = Fmt_Convert("{{b8}w{w2}w2b*}",
				    ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &dgram);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_INPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf("NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    status = NetHppiSource(interPtr, &dgram.address, dgram.count,
			dgram.size, 
			(dgram.useBuffer == TRUE) ? (Address) dgram.buffer :
			(Address) NIL, &dgram.time);
	    inSize = sizeof(dgram);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert("{{b8}w{w2}w2b*}", mach_Format, &inSize,
		(Address) &dgram, ioctlPtr->format, &outSize,
		(Address) ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		if (fmtStatus == FMT_OUTPUT_TOO_SMALL) {
		    return GEN_INVALID_ARG;
		} else {
		    printf(
			"NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    break;
	}

	case IOC_HPPI_SINK: {
	    Dev_UltraSink		sink;
	    int				value;
	    outSize = sizeof(int);
	    inSize = ioctlPtr->inBufSize;
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
			    ioctlPtr->inBuffer, mach_Format, &outSize,
			    (Address) &value);
	    if (fmtStatus != 0) {
		printf("Format of IOC_HPPI_SINK parameter failed, 0x%x\n",
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
		printf("Format of IOC_HPPI_SINK parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    if (value > 0) {
		statePtr->flags |= NET_HPPI_STATE_SINK;
		statePtr->flags &= ~NET_HPPI_STATE_NORMAL;
		packetsSunk = 0;
	    } else {
		statePtr->flags &= ~NET_HPPI_STATE_SINK;
		statePtr->flags |= NET_HPPI_STATE_NORMAL;
	    }
	    break;
	}

	case IOC_HPPI_ADDRESS: {
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
	    status = Net_SetAddress(NET_ADDRESS_ULTRA,
			(Address) &address, 
			&interPtr->netAddress[NET_PROTO_RAW]);
	    if (status != SUCCESS) {
		panic("NetHppiIOControl: Net_SetAddress failed\n");
	    }
	    MASTER_UNLOCK(&interPtr->mutex);
	    break;
	}

	case IOC_HPPI_COLLECT_STATS: {
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
		    printf("NetHppiIOControl: Fmt_Convert returned %d\n",
			fmtStatus);
		    return FAILURE;
		}
	    }
	    if (value == TRUE) {
		statePtr->flags |= NET_HPPI_STATE_STATS;
	    } else {
		statePtr->flags &= ~NET_HPPI_STATE_STATS;
	    }
	    break;
	}
	case IOC_HPPI_CLEAR_STATS: {
	    bzero((char *) &statePtr->stats, sizeof(statePtr->stats));
	    break;
	}
	case IOC_HPPI_GET_STATS: {
	    outSize = ioctlPtr->outBufSize;
	    inSize = sizeof(Dev_UltraStats);
	    fmtStatus = Fmt_Convert("w*", mach_Format, &inSize,
			    (Address) &statePtr->stats, 
			    ioctlPtr->format, &outSize,
			    ioctlPtr->outBuffer);
	    if (fmtStatus != 0) {
		printf("Format of IOC_HPPI_GET_STATS parameter failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    break;
	}

	case IOC_HPPI_SET_TSAP: {
	    char newTsap[4];

	    inSize = outSize = sizeof (int);
	    fmtStatus = Fmt_Convert("bbbb", ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format, &outSize,
				    (Address)&newTsap);
	    if (fmtStatus != FMT_OK) {
		printf("Format of HPPI_SET_TSAP parm failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    if (netHppiDebug) {
		printf ("NetHppiIOControl: setting TSAP to %02x%02x%02x%02x\n",
			(unsigned char)newTsap[0], (unsigned char)newTsap[1],
			(unsigned char)newTsap[2], (unsigned char)newTsap[3]);
	    }
	    bcopy (newTsap, statePtr->curTsap, sizeof (statePtr->curTsap));
	    break;
	}
	case IOC_HPPI_SET_BOARD_FLAGS:
	case IOC_HPPI_SET_SRC_BOARD_FLAGS:
	case IOC_HPPI_SET_DST_BOARD_FLAGS: {
	    Dev_HppiSetBoardFlags flagCmd;
	    inSize = outSize = sizeof (int);
	    fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format, &outSize,
				    (Address)&(flagCmd.flags));
	    if (fmtStatus != FMT_OK) {
		printf("Format of HPPI_SET_BOARD_FLAGS parm failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    statePtr->boardFlags = flagCmd.flags;
	    flagCmd.hdr.opcode = DEV_HPPI_SET_BOARD_FLAGS;
	    if ((ioctlPtr->command == IOC_HPPI_SET_BOARD_FLAGS) ||
		(ioctlPtr->command == IOC_HPPI_SET_SRC_BOARD_FLAGS)) {
		status = NetHppiSendCmd (statePtr, sizeof (flagCmd),
					 (Address)&flagCmd, NET_HPPI_SRC_CMD);
		if (status != SUCCESS) {
		    printf ("HPPI_SET_BOARD_FLAGS: SRC cmd failed\n");
		    return (status);
		}
	    }
	    if ((ioctlPtr->command == IOC_HPPI_SET_BOARD_FLAGS) ||
		(ioctlPtr->command == IOC_HPPI_SET_DST_BOARD_FLAGS)) {
		status = NetHppiSendCmd (statePtr, sizeof (flagCmd),
					 (Address)&flagCmd, 0);
		if (status != SUCCESS) {
		    printf ("HPPI_SET_BOARD_FLAGS: DST cmd failed\n");
		    return (status);
		}
	    }
	    break;
	}

	/*
	 * NOTE: there is a memory leak in the following IOC call.  This
	 * call is to be used for testing purposes ONLY!
	 */
	case IOC_HPPI_CONNECTION_OPEN: {
	    Net_HppiConnection*	connPtr;
	    Sync_Condition	gotOpen;

	    connPtr = (Net_HppiConnection*)malloc(sizeof (Net_HppiConnection));
	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format,
				    &outSize, (Address)connPtr);
	    if (fmtStatus != FMT_OK) {
		printf("Format HPPI_CONNECTION_OPEN failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    connPtr->userData = (ClientData)&gotOpen;
	    connPtr->interPtr = interPtr;
	    connPtr->callbackProc = iocConnCallback;
	    status = Net_HppiConnectionOpen (connPtr);
	    if (status != SUCCESS) {
		printf ("HPPI_CONNECTION_OPEN: couldn't open 0x%x\n",
			status);
		return (FAILURE);
	    }
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_OPEN: waiting...\n");
	    }
	    MASTER_LOCK (&(interPtr->mutex));
	    Sync_MasterWait(&gotOpen, &(interPtr->mutex), FALSE);
	    MASTER_UNLOCK (&(interPtr->mutex));
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_OPEN: returned from wait.\n");
	    }
	    connPtr->userData = (ClientData)connPtr;
	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", mach_Format, &inSize,
				    (Address)connPtr, ioctlPtr->format,
				    &outSize, ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf("Return format HPPI_CONNECTION_OPEN failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    status = connPtr->status;
	    break;
	}

	case IOC_HPPI_CONNECTION_LISTEN: {
	    Net_HppiConnection	conn, *connPtr;
	    Sync_Condition	gotListen;

	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format,
				    &outSize, (Address)&conn);
	    if (fmtStatus != FMT_OK) {
		printf("Return format HPPI_CONNECTION_LISTEN failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    connPtr = (Net_HppiConnection*)conn.userData;
	    connPtr->callbackProc = iocConnCallback;
	    connPtr->interPtr = interPtr;
	    connPtr->userData = (ClientData)&gotListen;
	    status = Net_HppiConnectionListen (connPtr);
	    if (status != SUCCESS) {
		printf ("HPPI_CONNECTION_LISTEN: couldn't listen 0x%x\n",
			status);
		return (FAILURE);
	    }
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_LISTEN: waiting...\n");
	    }
	    MASTER_LOCK (&(interPtr->mutex));
	    Sync_MasterWait(&gotListen, &(interPtr->mutex), FALSE);
	    MASTER_UNLOCK (&(interPtr->mutex));
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_LISTEN: returned from wait.\n");
	    }
	    conn.status = connPtr->status;
	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", mach_Format, &inSize,
				    (Address)&conn, ioctlPtr->format,
				    &outSize, ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf("Return format HPPI_CONNECTION_LISTEN failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    break;
	}

	case IOC_HPPI_CONNECTION_SEND:
	case IOC_HPPI_CONNECTION_RECV:
	{
	    Net_HppiConnection	conn, *connPtr;
	    Net_HppiDataRequest	dataReq;
	    Sync_Condition	dataDone;

	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", ioctlPtr->format, &inSize,
				    ioctlPtr->inBuffer, mach_Format,
				    &outSize, (Address)&conn);
	    if (fmtStatus != FMT_OK) {
		printf("Return format HPPI_CONNECTION_xfer failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    connPtr = (Net_HppiConnection*)conn.userData;
	    connPtr->interPtr = interPtr;
	    dataReq.interPtr = interPtr;
	    dataReq.connectionId = connPtr->connectionId;
	    dataReq.userData = (ClientData)&dataDone;
	    dataReq.callbackProc = iocXferCallback;
	    dataReq.xferSize = 100;
	    dataReq.xferOffset = 0;
	    dataReq.sgSize = 1;
	    dataReq.sg[0].addr = (Address)0xff905000;
	    dataReq.sg[0].length = 100;
	    if (ioctlPtr->command == IOC_HPPI_CONNECTION_SEND) {
		status = Net_HppiSendData (connPtr, &dataReq);
	    } else {
		status = Net_HppiReceiveData (connPtr, &dataReq);
	    }
	    if (status != SUCCESS) {
		printf ("HPPI_CONNECTION_xfer: couldn't transfer 0x%x\n",
			status);
		return (FAILURE);
	    }
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_xfer: waiting...\n");
	    }
	    MASTER_LOCK (&(interPtr->mutex));
	    Sync_MasterWait(&dataDone, &(interPtr->mutex), FALSE);
	    MASTER_UNLOCK (&(interPtr->mutex));
	    if (netHppiDebug) {
		printf ("HPPI_CONNECTION_xfer: returned from wait.\n");
	    }
	    conn.status = connPtr->status;
	    inSize = outSize = sizeof (Net_HppiConnection);
	    fmtStatus = Fmt_Convert("w9", mach_Format, &inSize,
				    (Address)&conn, ioctlPtr->format,
				    &outSize, ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf("Return format HPPI_CONNECTION_xfer failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    status = dataReq.status;
	    break;
	}
	case IOC_HPPI_IOP_OUTPUT: {
	    int ioSize;

	    inSize = outSize = sizeof (ioSize);
	    fmtStatus = Fmt_Convert ("w", ioctlPtr->format, &inSize,
				     (Address)ioctlPtr->inBuffer,
				     mach_Format, &outSize, (Address)&ioSize);
	    if (fmtStatus != FMT_OK) {
		printf("Format of IOC_HPPI_IOP_OUTPUT size failed, 0x%x\n",
		    fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    MASTER_LOCK (&interPtr->mutex);
	    statePtr->flags |= NET_HPPI_STATE_IOP_OUTPUT_TEST;
	    NetHppiLoopback (statePtr, ioSize, iopOutputCallback,
			     (ClientData)statePtr, 0);
	    Sync_MasterWait (&iopOutputDone, &(interPtr->mutex), TRUE);
	    MASTER_UNLOCK (&interPtr->mutex);
	}
	default: {
	    printf("NetHppiIOControl: unknown ioctl 0x%x\n",
		ioctlPtr->command);
	}

    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiSendReq --
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
NetHppiSendReq(statePtr, doneProc, data, rpc, scatterLength, scatterPtr, 
	requestSize, requestPtr)
    NetHppiState	*statePtr;	/* State of the adapter. */
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
    Dev_HppiScatterGather	sgList;
    Dev_HppiOutput		outputCommand;

    if (netHppiDebug) {
	printf("NetHppiSendReq: sending a request\n");
    }
    xrbPtr = statePtr->nextToAdapterPtr;

#ifndef CLEAN
    if (netHppiTrace) {
	NEXT_TRACE (statePtr, &tracePtr);
	tracePtr->event = SEND_REQ;
	tracePtr->index = xrbPtr - statePtr->firstToAdapterPtr;
	Timer_GetCurrentTicks (&tracePtr->ticks);
    }

#endif

    /*
     * For the HPPI boards, the next xrb should never be filled, but
     * we'll leave the code in case we can DMA request blocks to the
     * adapter in the future.
     */
    if (statePtr->nextToAdapterPtr->filled) {
	do {
	    if (netHppiDebug) {
		printf("NetHppiSendReq: no XRB free, waiting\n");
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
	panic("NetHppiSendReq: no available XRBInfo structures!");
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

#if 0
    hdrPtr->infoPtr = infoPtr;
#else
    hdrPtr->infoPtr = (NetUltraXRBInfo *)infoPtr->requestTag;
    statePtr->tagToXRBInfo[infoPtr->requestTag] = infoPtr;
#endif
    hdrPtr->status = 0;
    hdrPtr->reference = 0;

    if (scatterLength > 0) {
	Address		buffer;
	List_Links	*itemPtr;
	/*
	 * If the buffer is not in DVMA space then get one that is.
	 */
	if (! DVMA_ADDRESS(scatterPtr[0].bufAddr, statePtr)) {
	    if (netHppiDebug) {
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
		panic("NetHppiSendReq: list screwup\n");
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
		Net_GatherCopy(scatterPtr, scatterLength, buffer);
	    } else {

		if (netHppiDebug) {
		    printf("Copying data into DVMA space.\n");
		}
		Net_GatherCopy(scatterPtr, 1, buffer);
		size = scatterPtr[0].length;
	    }
	} else {
	    /* 
	     * The data is already in DVMA space.  It had better be contiguous!
	     */
	    if (netHppiDebug) {
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
	sgList.size = 1;
	sgList.element[0].address = (uint32)DVMA_TO_VME(buffer, statePtr);
	sgList.element[0].size = size;
    } else {
	hdrPtr->size = 0;
	sgList.size = 0;
    }

    if ((statePtr->flags & NET_HPPI_STATE_STATS) &&
	(hdrPtr->cmd == NET_ULTRA_DGRAM_SEND_REQ)) {
	statePtr->stats.packetsSent += 1;
	statePtr->stats.bytesSent += hdrPtr->size;
	statePtr->stats.sentHistogram[hdrPtr->size >> 10] += 1;
    }

    dmaPtr = &xrbPtr->dma;

    if (hdrPtr->cmd == NET_ULTRA_DGRAM_SEND_REQ) {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB_DATA;
	statePtr->numWritePending++;
	if (statePtr->numWritePending > statePtr->maxWritePending) {
	    panic("NetHppiSendReq: too many writes.\n");
	}
    } else if (hdrPtr->cmd == NET_ULTRA_DGRAM_RECV_REQ) {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB;
	statePtr->numReadPending++;
	if (statePtr->numReadPending > statePtr->maxReadPending) {
	    panic("NetHppiSendReq: too many reads.\n");
	}
    } else {
	dmaPtr->cmd = NET_ULTRA_DMA_CMD_XRB;
    }

#if 0
    bcopy((char *) requestPtr, (char *) &xrbPtr->request, requestSize);
#endif
    /*
     * The channel-based adapters require the command to also be
     * stuffed into the transaction ID in the DMA word.
     */
    dmaPtr->id = hdrPtr->cmd;
    dmaPtr->reference = 0;
    dmaPtr->length = requestSize;
    sgList.tag = (int)hdrPtr->infoPtr;
    dmaPtr->infoPtr = hdrPtr->infoPtr;
    if (netHppiDebug) {
	printf ("NetHppiSendReq: tag is 0x%x\n", sgList.tag);
    }
    dmaPtr->offset = 0;
    /*
     * Set the filled field so the adapter will process the XRB.
     */
    xrbPtr->filled = 1;
    if (statePtr->nextToAdapterPtr == statePtr->lastToAdapterPtr) {
	statePtr->nextToAdapterPtr = statePtr->firstToAdapterPtr;
    } else {
	statePtr->nextToAdapterPtr++;
    }

    /*
     * Send the command to the adapter.  First, send the scatter-gather
     * list to the destination board.  Next, send the request to the
     * source board.
     */
    if (netHppiTrace) {
	sgList.hdr.opcode = DEV_HPPI_SCATTER_GATHER_TRACE;
	outputCommand.hdr.opcode = DEV_HPPI_OUTPUT_TRACE;
    } else {
	sgList.hdr.opcode = DEV_HPPI_SCATTER_GATHER;
	outputCommand.hdr.opcode = DEV_HPPI_OUTPUT;
    }
    outputCommand.hdr.magic = DEV_HPPI_SRC_MAGIC;
    outputCommand.fifoDataSize = dmaPtr->length + sizeof (*dmaPtr);
    outputCommand.iopDataSize = 0;
    outputCommand.hdr.magic = DEV_HPPI_SRC_MAGIC;
    if (netHppiDebug) {
	printf("NetHppiSendReq: xrbPtr = 0x%x, sending %d bytes\n", xrbPtr,
	       outputCommand.fifoDataSize);
    }
    if (scatterLength > 0) {
	size = sizeof (sgList.hdr)+sizeof (sgList.size)+sizeof (sgList.tag) +
	    sizeof (Dev_HppiScatterGatherElement) * sgList.size;
	sgList.hdr.magic = DEV_HPPI_DEST_MAGIC;
	NetHppiSendCmd (statePtr, size, (Address)&sgList, 0);
    }

    status = NetHppiSendCmd (statePtr, sizeof (outputCommand),
			     (Address)&outputCommand,
			     NET_HPPI_KEEP_SRC_FIFO | NET_HPPI_SRC_CMD);
    if (status != SUCCESS) {
	goto exit;
    }
    status = NetHppiCopyToFifo (dmaPtr, sizeof (*dmaPtr), statePtr,
				NET_HPPI_SRC_CMD);
    if (status != SUCCESS) {
	goto exit;
    }
    status = NetHppiCopyToFifo (requestPtr, requestSize, statePtr,
				NET_HPPI_SRC_CMD);
    if (status != SUCCESS) {
	goto exit;
    }
    if (hdrPtr->cmd == NET_ULTRA_DGRAM_SEND_REQ) {
	outputCommand.fifoDataSize = hdrPtr->size;
	status = NetHppiSendCmd (statePtr, sizeof (outputCommand),
				 (Address)&outputCommand,
				 NET_HPPI_KEEP_SRC_FIFO | NET_HPPI_SRC_CMD);
	if (status != SUCCESS) {
	    goto exit;
	}
	for (i = 0; i < sgList.size; i++) {
	    status=NetHppiCopyToFifo (VME_TO_BUFFER(sgList.element[i].address,
					     statePtr), sgList.element[i].size,
			       statePtr, NET_HPPI_SRC_CMD);
	    if (status != SUCCESS) {
		goto exit;
	    }
	}
    }

exit:
    /*
     * Since we just sent the XRB to the HPPI boards, it is no longer
     * filled.  The code still acts like it is DMAed out (with filled
     * flags, etc.) so that it can be easily modified in the future if
     * the HPPI boards DMA requests out of memory at some future date.
     */
    xrbPtr->filled = 0;
    NetHppiReleaseSrcFifo (statePtr);
    return status;
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
    NetHppiState	*statePtr;	/* State of the adapter. */
{
    int		size;
    Address	addr;
    List_Links	*itemPtr;
    int		i;

    if (!(statePtr->queuesInit)) {
	/*
	 * Allocate XRBs that go from the adapter to the host. 
	 */
	size = sizeof(NetUltraXRB) * NET_HPPI_NUM_TO_HOST;
#ifdef QUEUES_IN_DUALPORT_RAM
	statePtr->firstToHostVME = (Address)NET_HPPI_DUAL_PORT_RAM;
	statePtr->firstToAdapterVME = (Address) NET_HPPI_DUAL_PORT_RAM + size;
	addr = VmMach_MapInDevice (statePtr->firstToHostVME, 3);
#else
	addr = (Address) malloc(size);
	addr = VmMach_DMAAlloc(size, (Address) addr);
	if (addr == (Address) NIL) {
	    panic("NetUltraInit: unable to allocate DMA space.\n");
	}
	statePtr->firstToHostVME = DVMA_TO_VME (addr, statePtr);
#endif
	statePtr->firstToHostPtr = (NetUltraXRB *) addr;
	/* 
	 * Allocate XRBs that go from the host to the adapter. 
	 */
	size = sizeof(NetUltraXRB) * NET_HPPI_NUM_TO_ADAPTER;

#ifdef QUEUES_IN_DUALPORT_RAM
	addr = VmMach_MapInDevice (statePtr->firstToAdapterVME, 3);
	if (Mach_Probe (sizeof(int), addr + size, (Address)&i) == FAILURE) {
	    panic ("NetHppiInit: circular queues too big.\n");
	}
#else
	addr = (Address) malloc(size);
	addr = VmMach_DMAAlloc(size, (Address) addr);
	if (addr == (Address) NIL) {
	    panic("NetUltraInit: unable to allocate DMA space.\n");
	}
	statePtr->firstToAdapterVME = DVMA_TO_VME (addr, statePtr);
#endif
	statePtr->firstToAdapterPtr = (NetUltraXRB *) addr;
	statePtr->pendingXRBInfoList = &statePtr->pendingXRBInfoListHdr;
	statePtr->freeXRBInfoList = &statePtr->freeXRBInfoListHdr;
	List_Init(statePtr->pendingXRBInfoList);
	List_Init(statePtr->freeXRBInfoList);
	for (i = 0; i < NET_HPPI_NUM_TO_ADAPTER + NET_HPPI_NUM_TO_HOST; i++) {
	    itemPtr = (List_Links *) malloc(sizeof(NetUltraXRBInfo));
	    ((NetUltraXRBInfo *)itemPtr)->requestTag = i + 1;
	    List_InitElement(itemPtr);
	    List_Insert(itemPtr, LIST_ATREAR(statePtr->freeXRBInfoList));
	}
	/*
	 * Allocate buffers in DVMA space for pending reads and writes. 
	 */
	statePtr->maxReadPending = NET_HPPI_PENDING_READS;
	statePtr->numReadPending = 0;
	statePtr->maxWritePending = NET_HPPI_PENDING_WRITES;
	statePtr->numWritePending = 0;
	statePtr->numBuffers = statePtr->maxReadPending + 
	    statePtr->maxWritePending;
	statePtr->bufferSize = NET_HPPI_MAX_BYTES;
	size = statePtr->numBuffers * statePtr->bufferSize;
#if 1
	addr = (Address) malloc(size);
	statePtr->buffers = addr;
	addr = VmMach_DMAAlloc(size, (Address) addr);
	statePtr->buffersDVMA = addr;
#else
	addr = NET_HPPI_DUAL_PORT_RAM + 0x4000;
#endif

	statePtr->freeBufferList = &statePtr->freeBufferListHdr;

	statePtr->queuesInit = TRUE;
    }
    statePtr->lastToHostPtr = statePtr->firstToHostPtr + 
	    NET_HPPI_NUM_TO_HOST - 1;
    statePtr->nextToHostPtr = statePtr->firstToHostPtr;
    statePtr->lastToAdapterPtr = statePtr->firstToAdapterPtr + 
	    NET_HPPI_NUM_TO_ADAPTER - 1;
    statePtr->nextToAdapterPtr = statePtr->firstToAdapterPtr;
    size = sizeof(NetUltraXRB) * NET_HPPI_NUM_TO_HOST;
    bzero((char *) statePtr->firstToHostPtr, size);
    size = sizeof(NetUltraXRB) * NET_HPPI_NUM_TO_ADAPTER;
    bzero((char *) statePtr->firstToAdapterPtr, size);
    while (!List_IsEmpty(statePtr->pendingXRBInfoList)) {
	itemPtr = List_First(statePtr->pendingXRBInfoList);
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
    for (i = 0; i < NET_HPPI_MAX_TAGS; i++) {
	statePtr->tags[i] = 0;
    }
    statePtr->curSgTag = 1;
    for (i = 0; i < NET_HPPI_MAX_CONNECTIONS; i++) {
	statePtr->connection[i].flags = 0;
	statePtr->connection[i].statePtr = statePtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiStart--
 *
 *	Send a start request to the adapter.
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
NetHppiStart(statePtr)
    NetHppiState	*statePtr;	/* State of the adapter. */
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
    if (!Net_AddrCmp(&interPtr->netAddress[NET_PROTO_RAW], &netZeroAddress)) {
	printf("NetHppiStart: can't send start cmd-adapter address not set\n");
	status = FAILURE;
	goto exit;
    }
    if (statePtr->flags & NET_HPPI_STATE_START) {
	printf("NetHppiStart: adapter already started.\n");
	status = FAILURE;
	goto exit;
    } else if (statePtr->flags &
	       (NET_HPPI_STATE_SRC_EPROM | NET_HPPI_STATE_DST_EPROM)) {
	printf ("NetHppiStart: boards not yet set up.\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    hdrPtr->cmd = NET_ULTRA_NEW_START_REQ;
    startPtr->sequence = sequence;
    sequence++;
    startPtr->hostType = NET_ULTRA_START_HOSTTYPE_SUN;
    startPtr->adapterType = NET_ULTRA_START_ADAPTYPE_HSC;
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
    if (netHppiDebug) {
	printf ("NetHppiStart: power = %d\n", power);
    }
    /* 
     * This part is kind of goofy.  The adapter expects the network
     * address to be 7 bytes long (the first byte is missing). We have
     * to bcopy the netAddress minus its first byte.
     */
    startPtr->netAddressSize = 7;
#if 0
    bcopy(&(((char *) &interPtr->netAddress[NET_PROTO_RAW])[1]),
	(char *) &startPtr->netAddressBuf, sizeof(Net_UltraAddress)-1);
#endif
    bcopy ((char *)&(interPtr->netAddress[NET_PROTO_RAW].address.ultra) + 1,
	   startPtr->netAddressBuf, startPtr->netAddressSize);
    startPtr->netAddressBuf[0] = 0x49;
    startPtr->netAddressBuf[5] = 0xfe;
    status = NetHppiSendReq(statePtr, StandardDone, 
		(ClientData) &startComplete, FALSE, 
		0, (Net_ScatterGather *) NIL, sizeof(NetUltraStartRequest), 
		&request);
    if (status != SUCCESS) {
	printf("NetHppiStart: unable to send start request\n");
	goto exit;
    }
    Sync_MasterWait(&(startComplete), &(interPtr->mutex), FALSE);
    if (netHppiDebug) {
	printf ("NetHppiStart: returned from wait\n");
    }
    if (hdrPtr->status & NET_ULTRA_STATUS_OK) {
	if (netHppiDebug) {
	    printf("NetHppiStart: hdrPtr->status = %d : %s\n",
		hdrPtr->status, GetStatusString(hdrPtr->status));
	}
	if (netHppiDebug) {
	    printf("Ultranet hub started\n");
	    printf("Ucode %d\n", startPtr->ucodeRel);
	    printf("Adapter type %d, adapter hw %d\n", startPtr->adapterType,
		startPtr->adapterHW);
	    printf("Max VC = %d\n", startPtr->maxVC);
	    printf("Max DRCV = %d\n", startPtr->maxDRCV);
	    printf("Max DSND = %d\n", startPtr->maxDSND);
	    printf("Slot = %d\n", startPtr->slot);
	    printf("Speed = %d\n", startPtr->speed);
	    printf("Max bytes = %d\n", (1 << startPtr->adapterMaxBytes));
	}
	if (startPtr->maxDRCV < statePtr->maxReadPending) {
	    printf("NetHppiStart: WARNING: max pending reads reset to %d\n",
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
	statePtr->maxBytesPower = startPtr->adapterMaxBytes;
	statePtr->flags |= NET_HPPI_STATE_START;
    } else {
	printf("NetHppiStart: start command failed <0x%x> : %s\n",
	    hdrPtr->status, GetStatusString(hdrPtr->status));
    }

exit:
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiStop--
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
NetHppiStop(statePtr)
    NetHppiState	*statePtr;	/* State of the adapter. */
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
    if (!(statePtr->flags & NET_HPPI_STATE_START)) {
	printf("NetHppiStop: adapter not started.\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &request, sizeof(request));
    hdrPtr->cmd = NET_ULTRA_STOP_REQ;
    status = NetHppiSendReq(statePtr, StandardDone, 
		(ClientData) &stopComplete, FALSE,
		0, (Net_ScatterGather *) NIL, sizeof(NetUltraStopRequest), 
		&request);
    if (status != SUCCESS) {
	printf("NetHppiStop: unable to send stop request\n");
	goto exit;
    }
    Sync_MasterWait(&(stopComplete), &(interPtr->mutex), FALSE);
    if (hdrPtr->status & NET_ULTRA_STATUS_OK) {
	if (netHppiDebug) {
	    printf("NetHppiStop: hdrPtr->status = %d : %s\n",
		hdrPtr->status, GetStatusString(hdrPtr->status));
	}
	statePtr->flags &= ~NET_HPPI_STATE_START;
    } else {
	printf("NetHppiStop: stop command failed <0x%x> : %s\n",
	    hdrPtr->status, GetStatusString(hdrPtr->status));
    }
exit:
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiSendDgram --
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
NetHppiSendDgram(interPtr, netAddressPtr, count, bufSize, buffer, timePtr)
    Net_Interface	*interPtr;		/* Interface to send on. */
    Net_Address		*netAddressPtr;		/* Host to send to. */
    int			count;			/* Number of times to send
						 * datagram. */
    int			bufSize;		/* Size of data buffer. */
    Address		buffer;			/* Data to send. */
    Time		*timePtr;		/* Place to store total 
						 * time to send datagrams. */
{
    NetHppiState		*statePtr;
    ReturnStatus		status;
    Timer_Ticks 		startTime;
    Timer_Ticks 		endTime;
    Dev_HppiSendDSND		dgramCmd;

    MASTER_LOCK(&interPtr->mutex);
    if (netHppiDebug) {
	char	address[100];
	(void) Net_AddrToString(netAddressPtr, address);
	printf("NetHppiSendDgram: sending %d bytes to %s\n", bufSize, address);
    }
    statePtr = (NetHppiState *) interPtr->interfaceData;
    if (!(statePtr->flags & NET_HPPI_STATE_START)) {
	printf("NetHppiSendDgram: adapter not started!\n");
	status = FAILURE;
	goto exit;
    }
    bzero((char *) &dgramCmd, sizeof(dgramCmd));
    dgramCmd.hdr.opcode = DEV_HPPI_SEND_DSND;
    dgramCmd.xrbHdr.xrbId = (uint32)interPtr;
    dgramCmd.xrbHdr.xrbBufId = (uint32)&dgramCmd;
    bcopy ((char*)&(netAddressPtr->address.ultra)+1, &(dgramCmd.xrbRemTA),
	   sizeof (dgramCmd.xrbRemTA));
    dgramCmd.xrbRemTA[0] = 0x49;
    dgramCmd.xrbRemTA[5] = 0xfe;
    bcopy ((char *)&(interPtr->netAddress[NET_PROTO_RAW].address.ultra) + 1,
	   &(dgramCmd.xrbLocTA), sizeof (dgramCmd.xrbLocTA));
    dgramCmd.xrbLocTA[0] = 0x49;
    dgramCmd.xrbLocTA[5] = 0xfe;
    bcopy (statePtr->curTsap, dgramCmd.tsap, sizeof (dgramCmd.tsap));

    if (buffer == (Address)NIL) {
	buffer = statePtr->buffers;
    }
    dgramCmd.fifoDataSize = bufSize;
    dgramCmd.iopDataSize = 0;
    dgramCmd.xrbHdr.tag = 1;
    statePtr->flags |= NET_HPPI_STATE_DSND_TEST;
    statePtr->flags &= ~NET_HPPI_STATE_NORMAL;
    dsndCount = count;
    Timer_GetCurrentTicks(&startTime);

    status = NetHppiSendCmd (statePtr, sizeof (dgramCmd), (Address)&dgramCmd,
			     NET_HPPI_SRC_CMD);
    if (status != SUCCESS) {
	if (netHppiDebug) {
	    printf ("NetHppiSendDgram: command failed 0x%x\n", status);
	}
	goto exit;
    }
    status = NetHppiCopyToFifo (buffer, bufSize, statePtr, NET_HPPI_SRC_CMD);
    if (status != SUCCESS) {
	if (netHppiDebug) {
	    printf ("NetHppiSendDgram: data copy failed 0x%x\n", status);
	}
	goto exit;
    }

    if (netHppiDebug) {
	printf ("NetHppiSendDgram: waiting for dgram to return.\n");
    }
    Sync_MasterWait(&dsndTestDone, &(interPtr->mutex), FALSE);
    Timer_GetCurrentTicks(&endTime);
#ifndef CLEAN
    if (netHppiDebug) {
	printf("NetHppiSendDgram: test done.\n");
    } 
#endif
    statePtr->flags &= ~NET_HPPI_STATE_DSND_TEST;
    statePtr->flags |= NET_HPPI_STATE_NORMAL;
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
DgramSendDone(dgramCmdPtr)
Dev_HppiSendDSND*	dgramCmdPtr;
{
    if (!(dgramCmdPtr->xrbHdr.status & NET_ULTRA_STATUS_OK)) {
	printf ("DgramSendDone: dgram send failed 0x%x\n",
		dgramCmdPtr->xrbHdr.status);
    } else {
#ifndef CLEAN
	if (netHppiDebug) {
	    printf("DgramSendDone: datagram sent ok\n");
	}
#endif
    }

    Sync_MasterBroadcast ((Sync_Condition *) &dsndTestDone);

#ifndef CLEAN
    if (netHppiDebug) {
	printf("DgramSendDone: returning.\n");
    }
#endif

}


/*
 *----------------------------------------------------------------------
 *
 * NetHppiRecvDgram --
 *
 *	This routine will receive a datagram.  The receive address is
 *	ignored, as is the buffer address (the link board memory will
 *	be used).  This is intended for debugging purposes, and is pretty
 *	slow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A read is queued to the adapter.
 *
 *----------------------------------------------------------------------
 */

static
void
NetHppiRecvDgram(interPtr)
Net_Interface	*interPtr;		/* Interface to send on. */
{
    int			bufSize;		/* Size of data buffer. */
    Address		buffer;			/* Data to send. */
    NetHppiState	*statePtr = (NetHppiState*)interPtr->interfaceData;
    ReturnStatus		status = SUCCESS;
    Timer_Ticks 		startTime;
    Timer_Ticks 		endTime;
    Dev_HppiSendDRCV		dgramCmd;
    Dev_HppiScatterGather	sgCmd;
    int				sgCmdSize;

    MASTER_LOCK(&interPtr->mutex);
    while ((statePtr->flags & NET_HPPI_STATE_SINK) && (status == SUCCESS)) {
	bufSize = 0x8000;
	buffer = (Address)NET_HPPI_DUAL_PORT_RAM + 0x8000;
#if 0
	if (netHppiDebug) {
	    char	address[100];
	    (void) Net_AddrToString(netAddressPtr, address);
	    printf("NetHppiRecvDgram: receiving %d bytes\n", bufSize);
	}
#endif
	statePtr = (NetHppiState *) interPtr->interfaceData;
	if (!(statePtr->flags & NET_HPPI_STATE_START)) {
	    printf("NetHppiRecvDgram: adapter not started!\n");
	    status = FAILURE;
	    goto exit;
	}
	/*
	 * Send the scatter-gather array.  Use the link board memory to store
	 * the return data.
	 */
	sgCmd.size = 1;
	sgCmd.offset = 0;
	if ((sgCmd.tag = getSgTag (statePtr)) == -1) {
	    printf ("NetHppiRecvDgram: couldn't get SG tag.\n");
	    status = FAILURE;
	    goto exit;
	}
	sgCmd.element[0].address = (uint32)buffer;
	sgCmd.element[0].size = (uint32)bufSize;
	sgCmd.hdr.opcode = DEV_HPPI_SCATTER_GATHER;
	sgCmdSize = (sizeof (sgCmd) - sizeof (sgCmd.element)) +
	    (sgCmd.size * sizeof (sgCmd.element[0]));
	status = NetHppiSendCmd (statePtr, sgCmdSize, (Address)&sgCmd,
				 NET_HPPI_DST_CMD);
	if (status != SUCCESS) {
	    printf ("NetHppiDgramRecv: SG command failed 0x%x\n", status);
	    goto exit;
	}
	
	bzero((char *) &dgramCmd, sizeof(dgramCmd));
	dgramCmd.hdr.opcode = DEV_HPPI_SEND_DRCV;
	dgramCmd.xrbHdr.xrbId = (uint32)interPtr;
	dgramCmd.xrbHdr.xrbBufId = (uint32)&dgramCmd;
	dgramCmd.xrbHdr.tag = sgCmd.tag;
	bzero (&dgramCmd.xrbRemTA, sizeof (dgramCmd.xrbRemTA));
	bzero (&dgramCmd.xrbLocTA, sizeof (dgramCmd.xrbLocTA));
	bcopy (statePtr->curTsap, dgramCmd.tsap, sizeof (dgramCmd.tsap));
#if 0
	dgramCmd.xrbRemTA[1] = 0x49;
	dgramCmd.xrbRemTA[6] = 0xfe;
	dgramCmd.xrbLocTA[1] = 0x49;
	dgramCmd.xrbLocTA[6] = 0xfe;
#endif
	dgramCmd.cnt = bufSize;
	
	if (buffer == (Address)NIL) {
	    buffer = statePtr->buffers;
	}
	Timer_GetCurrentTicks(&startTime);
	
	status = NetHppiSendCmd (statePtr, sizeof (dgramCmd),
				 (Address)&dgramCmd, NET_HPPI_SRC_CMD);
	if (status != SUCCESS) {
	    if (netHppiDebug) {
		printf ("NetHppiRecvDgram: command failed 0x%x\n", status);
	    }
	    goto exit;
	}
	
	if (netHppiDebug) {
	    printf ("NetHppiRecvDgram: waiting to receive dgram.\n");
	}
	Sync_MasterWait(&drcvTestDone, &(interPtr->mutex), FALSE);
	Timer_GetCurrentTicks(&endTime);
#ifndef CLEAN
	if (netHppiDebug) {
	    printf("NetHppiRecvDgram: test done.\n");
	} 
#endif
	Timer_SubtractTicks(endTime, startTime, &endTime);
#if 0
	Timer_TicksToTime(endTime, timePtr);
#endif
      exit:
	;
    }
    statePtr->flags &= ~NET_HPPI_STATE_SINK;
    MASTER_UNLOCK(&interPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * DgramRecvDone --
 *
 *	Called by the interrupt handler when the datagram requested by
 * 	NetUltraRecvDgram is actually received.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process waiting for the datagram to be received is notified.
 *
 *----------------------------------------------------------------------
 */

static void
DgramRecvDone(dgramCmdPtr)
Dev_HppiSendDRCV*	dgramCmdPtr;
{
    Net_Interface*	interPtr;
    NetHppiState*	statePtr;

    interPtr = (Net_Interface*)(dgramCmdPtr->xrbHdr.xrbId);
    statePtr = (NetHppiState*)(interPtr->interfaceData);
    if (!(dgramCmdPtr->xrbHdr.status & NET_ULTRA_STATUS_OK)) {
	printf ("DgramRecvDone: dgram recv failed (status=0x%x)\n",
		dgramCmdPtr->xrbHdr.status);
	printf ("DgramRecvDone: terminating datagram sinking...\n");
	MASTER_LOCK (&interPtr->mutex);
	statePtr->flags &= ~NET_HPPI_STATE_SINK;
	statePtr->flags |= NET_HPPI_STATE_NORMAL;
	MASTER_UNLOCK (&interPtr->mutex);
    } else {
#ifndef CLEAN
	if (netHppiDebug) {
	    printf("DgramRecvDone: got datagram of size 0x%x\n",
		   dgramCmdPtr->cnt);
	}
#endif
    }

    Sync_MasterBroadcast ((Sync_Condition *) &drcvTestDone);

#ifndef CLEAN
    if (netHppiDebug) {
	printf("DgramRecvDone: returning.\n");
    }
#endif

}


/*
 *----------------------------------------------------------------------
 *
 * NetHppiOutputStub --
 *
 *	Return FAILURE -- the network shouldn't be trying to output
 *	RPC packets.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetHppiOutputStub(interPtr, hdrPtr, scatterGatherPtr, scatterGatherLength,
		  rpc, statusPtr)
    Net_Interface		*interPtr;	/* The interface to use. */
    Address			hdrPtr;		/* Packet header. */
    Net_ScatterGather		*scatterGatherPtr; /* Scatter/gather elems.*/
    int				scatterGatherLength; /* Number of elements in
						      * scatter/gather list. */
    Boolean			rpc;		/* Is this an RPC packet? */
    ReturnStatus		*statusPtr;	/* Place to store status. */
{
    printf ("NetHppiOutputStub: shouldn't call HPPI for normal output.\n");
    return FAILURE;
}

/*
 *----------------------------------------------------------------------
 *
 * NetHppiSource --
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
NetHppiSource(interPtr, netAddressPtr, count, bufSize, buffer, timePtr)
    Net_Interface	*interPtr;		/* Interface to send on. */
    Net_Address		*netAddressPtr;		/* Host to send to. */
    int			count;			/* Number of times to send
						 * datagram. */
    int			bufSize;		/* Size of data buffer. */
    Address		buffer;			/* Data to send. */
    Time		*timePtr;		/* Place to store total 
						 * time to send datagrams. */
{
    NetHppiState		*statePtr;
    NetUltraRequest		request;
    NetUltraDatagramRequest	*dgramReqPtr;
    NetUltraRequestHdr		*hdrPtr;
    ReturnStatus		status = SUCCESS;
    Address			itemPtr;
    char			*ptr;
    Timer_Ticks 		startTime;
    Timer_Ticks 		endTime;
    Timer_Ticks 		curTime;
    NetUltraTraceInfo		*tracePtr;
    Net_ScatterGather		scatter;

    MASTER_LOCK(&interPtr->mutex);
#ifndef CLEAN
    if (netHppiDebug) {
	char	address[100];
	(void) Net_AddrToString(netAddressPtr, address);
	printf("NetHppiSendDgram: sending to %s\n", address);
    }
#endif
    statePtr = (NetHppiState *) interPtr->interfaceData;
    bzero((char *) &request, sizeof(request));
    dgramReqPtr = &request.dgram;
    hdrPtr = &dgramReqPtr->hdr;
    dgramReqPtr->remoteAddress = wildcardAddress;
    dgramReqPtr->remoteAddress.tsapSize = 2;
    dgramReqPtr->remoteAddress.tsap[1] = 1;
    dgramReqPtr->remoteAddress.address = netAddressPtr->address.ultra;
    ptr = (char *) &dgramReqPtr->remoteAddress.address;
    ptr[1] = 0x49;
    ptr[6] = 0xfe;
    dgramReqPtr->localAddress = wildcardAddress;
    dgramReqPtr->localAddress.tsapSize = 2;
    dgramReqPtr->localAddress.tsap[1] = 1;
    dgramReqPtr->localAddress.address = 
	interPtr->netAddress[NET_PROTO_RAW].address.ultra;
    ptr = (char *) &dgramReqPtr->localAddress.address;
    ptr[1] = 0x49;
    ptr[6] = 0xfe;
    hdrPtr->cmd = NET_ULTRA_DGRAM_SEND_REQ;
    Timer_GetCurrentTicks(&startTime);
    while(count > 0) {
	count--;
	itemPtr = getFreeBuffer (statePtr);
	if (itemPtr == (Address)NIL) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    goto exit;
	}
	if (buffer != (Address) NIL) {
	    bcopy((char *) buffer, itemPtr, bufSize);
	}
	dsndCount = count;
#ifndef CLEAN
	if (netHppiTrace) {
	    NEXT_TRACE(statePtr, &tracePtr);
	    tracePtr->event = DSND_STREAM;
	    Timer_GetCurrentTicks(&curTime);
	    tracePtr->ticks = curTime;
	}
#endif
	scatter.bufAddr = BUFFER_TO_DVMA(itemPtr, statePtr);
	scatter.length = bufSize;
	status = NetHppiSendReq(statePtr, SourceDone, 
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
    NetHppiState		*statePtr;
    List_Links			*itemPtr;

    hdrPtr = &(infoPtr->xrbPtr->request.dgram.hdr);

    statePtr = (NetHppiState *) interPtr->interfaceData;
    if (!(hdrPtr->status & NET_ULTRA_STATUS_OK)) {
	panic("SourceDone: write failed 0x%x (continuable)\n", hdrPtr->status);
    } else {
#ifndef CLEAN
	if (netHppiDebug) {
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
 * NetHppiGetStats --
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
NetHppiGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    NetHppiState	*statePtr;

    statePtr = (NetHppiState *) interPtr->interfaceData;
    MASTER_LOCK(&interPtr->mutex);
    statPtr->hppi = statePtr->stats;
    MASTER_UNLOCK(&interPtr->mutex);
    return SUCCESS;
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


/*----------------------------------------------------------------------
 *
 * setLocalAddress --
 *
 *	Copy the current interface's network address into the passed
 *	location.
 *
 * Results:
 *	None.
 * Side effects:
 *	Copies a network address.
 *
 *----------------------------------------------------------------------
 */
static
void
setLocalAddress (interPtr, netAddrPtr)
Net_Interface*	interPtr;
char*		netAddrPtr;
{
    bcopy ((char *)&(interPtr->netAddress[NET_PROTO_RAW].address.ultra) + 1,
	   netAddrPtr, 7);
    netAddrPtr[0] = 0x49;
    netAddrPtr[5] = 0xfe;

}

/*----------------------------------------------------------------------
 *
 * NetHppiCmdCallback --
 *
 *	This is the routine scheduled when a completed command is sent
 *	into the ring buffer.  It is called once for each command, and
 *	is passed the address of the buffer to deal with.  It simply
 *	calls the appropriate handler for the type of message.
 *
 * Results:
 *	none
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */
void
NetHppiCmdCallback (data, callInfoPtr)
ClientData	data;
Proc_CallInfo*	callInfoPtr;
{
    Dev_HppiSendXfer*	cmdPtr;
    NetUltraXRB*	xrbPtr = (NetUltraXRB*)data;
    NetHppiConnState*	connStatePtr;

    cmdPtr = (Dev_HppiSendXfer*)&(xrbPtr->request);
    if (cmdPtr->hdr.magic != DEV_HPPI_DEST_MAGIC) {
	panic ("NetHppiCmdCallback: bad magic number in command (0x%x)\n",
	       cmdPtr->hdr.magic);
    }
    connStatePtr = (NetHppiConnState*)cmdPtr->xrbHdr.xrbId;
    switch (cmdPtr->hdr.opcode) {
      case DEV_HPPI_SEND_ALSN:
	NetHppiConnectionAccepted ((Dev_HppiSendALSN*)cmdPtr);
	break;
      case DEV_HPPI_SEND_OPEN:
	NetHppiOpenCallback ((Dev_HppiSendOPEN*)cmdPtr);
	break;
      case DEV_HPPI_SEND_CONN:
	break;
      case DEV_HPPI_SEND_CLSE:
	/*
	 * The ISO reason code will be garbage, but the cleanup code
	 * will ignore it in case of an abrupt (CLSE) close.
	 */
	NetHppiConnectionCleanup ((Dev_HppiSendRLSE*)cmdPtr);
	break;
      case DEV_HPPI_SEND_RLSE:
	NetHppiConnectionCleanup ((Dev_HppiSendRLSE*)cmdPtr);
	break;
      case DEV_HPPI_SEND_SEND:
      case DEV_HPPI_SEND_SEOM:
      case DEV_HPPI_SEND_RECV:
	NetHppiXferCallback ((Dev_HppiSendXfer*)cmdPtr);
	break;
      case DEV_HPPI_SEND_DRCV:
	DgramRecvDone ((Dev_HppiSendDRCV*)cmdPtr);
	break;
      case DEV_HPPI_SEND_DSND:
	DgramSendDone ((Dev_HppiSendDSND*)cmdPtr);
	break;
      default:
	printf ("NetHppiCmdCallback: unknown command (0x%x)\n",
		cmdPtr->hdr.opcode);
	break;
    }
    /*
     * Now that we're done with the packet, we can declare it empty.
     */
    xrbPtr->filled = 0;
}


/*----------------------------------------------------------------------
 *
 * NetHppiOpenCallback --
 *
 *	This procedure is called back when an open request returns.
 *	It fills in the appropriate fields in the connection state
 *	information, and then calls back the user-specified callback
 *	routine.
 *
 * Returns:
 *	none
 * Side effects:
 *	Fills in connection state data structure.
 *
 *----------------------------------------------------------------------
 */
void
NetHppiOpenCallback (openBlockPtr)
Dev_HppiSendOPEN*	openBlockPtr;
{
    Net_Interface*	interPtr;
    NetHppiConnState*	connStatePtr;
    Net_HppiConnection*	connPtr;
    Boolean		doCallback = TRUE;

    connStatePtr = (NetHppiConnState*)openBlockPtr->xrbHdr.xrbId;
    connPtr = (Net_HppiConnection*)openBlockPtr->xrbHdr.xrbBufId;
    interPtr = connPtr->interPtr;
    MASTER_LOCK (&interPtr->mutex);
    if (connStatePtr->flags & NET_HPPI_CONN_STATE_OPEN) {
	doCallback = FALSE;
	printf ("NetHppiOpenCallback: connection already open!\n");
	goto exit;
    } else if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_INUSE)) {
	doCallback = FALSE;
	printf ("NetHppiOpenCallback: connection not in use?!\n");
	goto exit;
    }
    if (openBlockPtr->xrbHdr.status != NET_ULTRA_STATUS_OK) {
	connStatePtr->flags &= ~NET_HPPI_CONN_STATE_INUSE;
	connPtr->status = NET_NOT_CONNECTED;
    } else {
	connStatePtr->flags |= NET_HPPI_CONN_STATE_OPEN;
	connStatePtr->xrbRef = openBlockPtr->xrbHdr.xrbRef;
	connPtr->status = SUCCESS;
    }

  exit:
    MASTER_UNLOCK (&interPtr->mutex);
    if (doCallback) {
	(*connPtr->callbackProc)(connPtr);
    }
}

/*----------------------------------------------------------------------
 *
 * Net_HppiConnectionOpen --
 *
 *	Open a virtual circuit.  This must be done before attempting
 *	to listen on a virtual circuit.  The calling semantics are
 *	similar to Net_HppiConnectionListen.
 *
 * Returns:
 *	Standard Sprite return status.
 * Side effects:
 *	Sends a HPPI open request.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_HppiConnectionOpen (connStatusPtr)
Net_HppiConnection*	connStatusPtr;
{
    ReturnStatus	status = SUCCESS;
    NetHppiState*	statePtr;
    NetHppiConnState*	connStatePtr;
    Dev_HppiSendOPEN	adapOpen;
    Net_Interface*	interPtr;
    int			connId;

    interPtr = connStatusPtr->interPtr;
    statePtr = (NetHppiState*)(interPtr->interfaceData);
    MASTER_LOCK (&interPtr->mutex);

    /*
     * Find a free connection ID.
     */
    for (connId = 0; connId < NET_HPPI_MAX_CONNECTIONS; connId += 1) {
	if (!(statePtr->connection[connId].flags & NET_HPPI_CONN_STATE_INUSE)){
	    break;
	}
    }
    if (connId == NET_HPPI_MAX_CONNECTIONS) {
	printf ("Net_HppiConnectionListen: no connections available.\n");
	status = NET_NO_CONNECTS;
	goto exit;
    }
    connStatusPtr->connectionId = connId;
    connStatePtr = &(statePtr->connection[connId]);
    connStatePtr->userConnInfo = connStatusPtr;
    connStatePtr = &(statePtr->connection[connId]);
    connStatePtr->flags = NET_HPPI_CONN_STATE_INUSE;
    adapOpen.xrbHdr.xrbId = (uint32)connStatePtr;
    adapOpen.xrbHdr.xrbBufId = (uint32)connStatusPtr;
    adapOpen.xrbHdr.xrbRef = 0;
    adapOpen.xrbHdr.tag = 1;
    adapOpen.hdr.opcode = DEV_HPPI_SEND_OPEN;
    NetHppiSendCmd (statePtr, sizeof (adapOpen), (Address)&adapOpen,
		    NET_HPPI_SRC_CMD);
  exit:
    MASTER_UNLOCK (&interPtr->mutex);
    return (status);
}

/*----------------------------------------------------------------------
 *
 * NetHppiConnectionAccepted --
 *
 *	This is called via callback when a request block accepting a
 *	connection comes in.  Since it's always called via a Proc_Callfunc,
 *	the connection callback is called directly at the end of the
 *	routine.
 *
 * Results:
 *	none
 * Side effects:
 *	Calls a callback routine specified when the connection was requested.
 *
 *----------------------------------------------------------------------
 */
void
NetHppiConnectionAccepted (connBlockPtr)
Dev_HppiSendALSN*	connBlockPtr;
{
    NetHppiState*	statePtr;
    NetHppiConnState*	connStatePtr;
    Net_HppiConnection*	connPtr;
    Net_Interface*	interPtr;
    Boolean		doCallback = TRUE;

    connStatePtr = (NetHppiConnState*)connBlockPtr->xrbHdr.xrbId;
    connPtr = connStatePtr->userConnInfo;
    interPtr = connPtr->interPtr;
    statePtr = (NetHppiState*)interPtr->interfaceData;

    MASTER_LOCK (&interPtr->mutex);
    if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_LISTEN)) {
	MASTER_UNLOCK (&interPtr->mutex);
	printf ("NetHppiConnectionEstablish: connection %d not listening?\n",
		connPtr->connectionId);
	return;
    }

    if (connBlockPtr->xrbHdr.status == NET_HPPI_STATUS_CONN_REQ) {
	if (netHppiDebug) {
	    printf ("NetHppiConnectionAccepted: connection requested.\n");
	}
	doCallback = FALSE;
    } else if (connBlockPtr->xrbHdr.status != NET_ULTRA_STATUS_OK) {
	if (netHppiDebug) {
	    printf ("NetHppiConnectionAccepted: bad ultra status 0x%x\n",
		    connBlockPtr->xrbHdr.status);
	}
	connStatePtr->flags &= ~NET_HPPI_CONN_STATE_INUSE;
	connPtr->status = NET_NOT_CONNECTED;
    } else {
	/*
	 * Connection established, so set up shop.
	 */
	connStatePtr->flags |= NET_HPPI_CONN_STATE_CONNECTED;
	connStatePtr->flags &= ~NET_HPPI_CONN_STATE_LISTEN;
	connStatePtr->numOutstandingRecv = 0;
	connStatePtr->numOutstandingSend = 0;
	List_Init (&connStatePtr->sendHdr);
	List_Init (&connStatePtr->recvHdr);
	connPtr->status = SUCCESS;
    }
    MASTER_UNLOCK (&interPtr->mutex);
    if (doCallback) {
	if (netHppiDebug) {
	    printf ("NetHppiConnectionAccepted: doing user callback.\n");
	}
	(*connPtr->callbackProc)(connPtr);
    }
}

/*----------------------------------------------------------------------
 *
 * Net_HppiConnectionListen --
 *
 *	Send an ALSN command to the HIPPI boards, and wait for a
 *	connection request.  When one is received, the callback
 *	routine will be called.
 *
 * Results:
 *	Standard Sprite return status.
 * Side effects:
 *	Sends ALSN command to Ultra hub.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_HppiConnectionListen (connStatusPtr)
Net_HppiConnection*	connStatusPtr;
{
    ReturnStatus	status = SUCCESS;
    NetHppiState	*statePtr;
    NetHppiConnState	*connStatePtr;
    Dev_HppiSendALSN	adapListen;
    Net_Interface*	interPtr;

    interPtr = connStatusPtr->interPtr;
    statePtr = (NetHppiState*)(interPtr->interfaceData);
    connStatePtr = &(statePtr->connection[connStatusPtr->connectionId]);
    MASTER_LOCK (&interPtr->mutex);
    if (!(statePtr->flags & NET_HPPI_STATE_START)) {
	printf ("Net_HppiConnectionListen: adapter not started!\n");
	status = FAILURE;
	goto exit;
    }
    if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_OPEN)) {
	printf ("Net_HppiConnectionListen: channel not open!\n");
	status = FAILURE;
	goto exit;
    }

    if (netHppiDebug) {
	printf ("Net_HppiConnectionListen: using connection %d\n",
		connStatusPtr->connectionId);
    }
    connStatePtr->flags |= NET_HPPI_CONN_STATE_LISTEN;
    adapListen.xrbHdr.xrbRef = connStatePtr->xrbRef;
    adapListen.xrbHdr.xrbId = (uint32)connStatePtr;
    adapListen.xrbHdr.xrbBufId = (uint32)connStatusPtr;
    adapListen.xrbHdr.tag = 1;
    adapListen.hdr.opcode = DEV_HPPI_SEND_ALSN;
    bcopy (&netHppiConnPar,&adapListen.xrbConPar,sizeof(adapListen.xrbConPar));
    adapListen.xrbConPar[1] = statePtr->maxBytesPower;
    /*
     * Set remote address to wildcard.  Set local to our address.
     */
    bzero (&adapListen.xrbRemTA, sizeof (adapListen.xrbRemTA));
    setLocalAddress (interPtr, &(adapListen.xrbLocTA));
    bcopy (statePtr->curTsap, adapListen.tsap, sizeof (adapListen.tsap));
    NetHppiSendCmd (statePtr, sizeof (adapListen), (Address)&adapListen,
		    NET_HPPI_SRC_CMD);

  exit:
    MASTER_UNLOCK (&interPtr->mutex);
    return status;
}

/*----------------------------------------------------------------------
 *
 * NetHppiConnectionCleanup --
 *
 *	Tear down the state associated with a connection.  This aborts
 *	all outstanding requests and frees the connection to be reused.
 *	It is called when a request block indicating a closed connection
 *	is received.  This could be the result of either end wanting
 *	to close the connection.
 *
 * Results:
 *	Standard Sprite return status.
 * Side effects:
 *	Many callbacks are scheduled.
 *
 *----------------------------------------------------------------------
 */
static
void
NetHppiConnectionCleanup (cmdPtr)
Dev_HppiSendRLSE*	cmdPtr;
{
    NetHppiState*	statePtr;
    NetHppiConnState*	connStatePtr;
    Net_Interface*	interPtr;
    Net_HppiConnection*	connPtr;
    List_Links		freeUpList;
    Net_HppiDataRequest* reqPtr;

    connStatePtr = (NetHppiConnState*)cmdPtr->xrbHdr.xrbId;
    connPtr = (Net_HppiConnection*)cmdPtr->xrbHdr.xrbBufId;
    statePtr = connStatePtr->statePtr;
    interPtr = statePtr->interPtr;

    /*
     * At this point, we can free up any remaining data requests,
     * as the connection has been officially closed.
     */
    List_Init (&freeUpList);
    MASTER_LOCK (&interPtr->mutex);
    List_ListInsert (&connStatePtr->recvHdr, LIST_ATFRONT (&freeUpList));
    List_ListInsert (&connStatePtr->sendHdr, LIST_ATFRONT (&freeUpList));
    /*
     * Now that we've pulled the lists off the state pointer, the connection
     * may be reused (even before we've made all the callbacks for the
     * old connection), so mark the connection as free.
     */
    connStatePtr->flags = 0;
    MASTER_UNLOCK (&interPtr->mutex);

    /*
     * Send a callback for each of the still-outstanding requests.
     */
    while (!List_IsEmpty (&freeUpList)) {
	reqPtr = (Net_HppiDataRequest*)List_First (&freeUpList);
	List_Remove ((List_Links*)reqPtr);
	freeSgTag (statePtr, reqPtr->status);
	reqPtr->status = GEN_ABORTED_BY_SIGNAL;
	(*reqPtr->callbackProc)(reqPtr);
    }
    /*
     * Callback to indicate that the connection has finished closing.
     */
    (*connPtr->callbackProc)(connPtr);
}

/*----------------------------------------------------------------------
 *
 * Net_HppiConnectionClose --
 *
 *	Close a connection that was already open.  This routine will
 *	call the routine to actually send a close packet to the adapter.
 *	The connection state won't be nuked until the close packet
 *	successfully returns.
 *
 * Results:
 *	Sprite return status.
 * Side effects:
 *	A HIPPI connection is closed.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_HppiConnectionClose (connPtr)
Net_HppiConnection*	connPtr;
{
    ReturnStatus	status = SUCCESS;
    Dev_HppiSendRLSE	rlseCmd;
    Net_Interface*	interPtr;
    NetHppiState*	statePtr;
    NetHppiConnState*	connStatePtr;

    if ((connPtr->connectionId < 0) ||
	(connPtr->connectionId > NET_HPPI_MAX_CONNECTIONS)) {
	return (FAILURE);
    }
    interPtr = connPtr->interPtr;
    statePtr = (NetHppiState*)interPtr->interfaceData;
    connStatePtr = &(statePtr->connection[connPtr->connectionId]);
    MASTER_LOCK (&interPtr->mutex);
    if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_CONNECTED)) {
	if (netHppiDebug) {
	    printf ("Net_HppiConnectionClose: connection %d not connected.\n",
		    connPtr->connectionId);
	}
	status = NET_NOT_CONNECTED;
	goto exit;
    }
    connStatePtr->flags &= ~NET_HPPI_CONN_STATE_CONNECTED;
    rlseCmd.xrbHdr.xrbId = (uint32)connStatePtr;
    rlseCmd.xrbHdr.xrbBufId = (uint32)connPtr;
    rlseCmd.xrbHdr.xrbRef = connStatePtr->xrbRef;
    rlseCmd.xrbHdr.tag = 1;
    rlseCmd.hdr.opcode = DEV_HPPI_SEND_RLSE;
    status =  NetHppiSendCmd (statePtr, sizeof (rlseCmd), (Address)&rlseCmd,
			      NET_HPPI_SRC_CMD);
  exit:
    MASTER_UNLOCK (&interPtr->mutex);
    if (status != SUCCESS) {
	printf ("Net_HppiConnectionClose: couldn't send RLSE command.\n");
    }

    return status;
}

/*----------------------------------------------------------------------
 *
 * NetHppiXferCallback --
 *
 *	The callback for data being sent or received.  Translate the Ultra
 *	status into some kind of return code, and fill in the field in
 *	the transfer request block.  Unhook the block from the connection
 *	state, and then callback the upper-level routine.
 *
 *----------------------------------------------------------------------
 */
void
NetHppiXferCallback (cmdPtr)
Dev_HppiSendXfer*	cmdPtr;
{
    Net_HppiDataRequest*	dataReqPtr;
    NetHppiConnState*		connStatePtr;
    NetHppiState*		statePtr;

    dataReqPtr = (Net_HppiDataRequest*)cmdPtr->xrbHdr.xrbBufId;
    connStatePtr = (NetHppiConnState*)cmdPtr->xrbHdr.xrbId;
    statePtr = connStatePtr->statePtr;
    if (netHppiDebug) {
	printf ("NetHppiXferCallback: returning call for request at 0x%x\n",
		dataReqPtr);
    }

    MASTER_LOCK (&statePtr->interPtr->mutex);
    if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_CONNECTED)) {
	if (netHppiDebug) {
	    printf ("NetHppiXferCallback: connection no longer valid.\n");
	}
	MASTER_UNLOCK (&statePtr->interPtr->mutex);
	return;
    }
    freeSgTag (statePtr, dataReqPtr->status);
    if (cmdPtr->xrbHdr.status & NET_ULTRA_STATUS_OK) {
	dataReqPtr->status = SUCCESS;
    } else {
	if (netHppiDebug) {
	    printf ("NetHppiXferCallback: got bad status 0x%x\n",
		    cmdPtr->xrbHdr.status);
	}
	dataReqPtr->status = FAILURE;
    }

    List_Remove ((List_Links*)dataReqPtr);
    if (cmdPtr->hdr.opcode == DEV_HPPI_SEND_RECV) {
	connStatePtr->numOutstandingRecv -= 1;
    } else {
	connStatePtr->numOutstandingSend -= 1;
    }
    /*
     * Copy the actual transfer size back to the data request block.
     */
    dataReqPtr->xferSize = cmdPtr->cnt;
    MASTER_UNLOCK (&statePtr->interPtr->mutex);

    /*
     * Unmap buffers here if necessary (not yet implemented).
     */
    (*dataReqPtr->callbackProc)(dataReqPtr);
}

/*----------------------------------------------------------------------
 *
 * NetHppiXferData --
 *
 *	Common code for sending or receiving data on an established
 *	connection.  The data is mapped into VME space if necessary.
 *	Next, the appropriate scatter-gather array is sent to the
 *	DST board.  Finally, the request block is sent out to the
 *	SRC board.
 *
 * Returns:
 *	Standard Sprite return status.
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
NetHppiXferData (connPtr, dataReqPtr, which)
Net_HppiConnection*	connPtr;	/* -> connection id block */
Net_HppiDataRequest*	dataReqPtr;	/* -> data request */
int			which;		/* send or receive? */
{
    NetHppiState*	statePtr;
    NetHppiConnState*	connStatePtr;
    Dev_HppiSendXfer	sendCmd;
    Dev_HppiScatterGather sgCmd;
    Net_Interface*	interPtr = connPtr->interPtr;
    ReturnStatus	status = SUCCESS;
    int			sgCmdSize;
    int			i;

    statePtr = (NetHppiState *)connPtr->interPtr->interfaceData;
    if ((connPtr->connectionId < 0) ||
	(connPtr->connectionId > NET_HPPI_MAX_CONNECTIONS)) {
	if (netHppiDebug) {
	    printf ("NetHppiXferData: bad connection id (%d)\n",
		    connPtr->connectionId);
	}
	return (FAILURE);
    }
    connStatePtr = &(statePtr->connection[connPtr->connectionId]);

    MASTER_LOCK (&interPtr->mutex);
    if (!(connStatePtr->flags & NET_HPPI_CONN_STATE_CONNECTED)) {
	status = NET_NOT_CONNECTED;
	goto exit;
    }
    /*
     * Set up the scatter-gather command.  Map in buffers if necessary.
     */
    for (i = 0; i < dataReqPtr->sgSize; i++) {
	sgCmd.element[i].size = dataReqPtr->sg[i].length;
	/*
	 * Must map buffer into VME if necessary.  This isn't done yet.
	 */
	sgCmd.element[i].address = (uint32)dataReqPtr->sg[i].addr;
    }
    sgCmd.size = dataReqPtr->sgSize;
    sgCmd.offset = dataReqPtr->xferOffset;
    sgCmd.hdr.opcode = DEV_HPPI_SCATTER_GATHER;
    sgCmdSize = sizeof (sgCmd) - sizeof (sgCmd.element) +
	(sgCmd.size * sizeof (sgCmd.element[0]));

    if ((sgCmd.tag = getSgTag (statePtr)) == -1) {
	status = FAILURE;
	goto exit;
    }

    dataReqPtr->status = sendCmd.xrbHdr.tag = sgCmd.tag;
    if (netHppiDebug) {
	printf ("NetHppiXferData: setting SG tag to %d\n", dataReqPtr->status);
    }
    /*
     * Add the request to the chain associated with the connection.
     */
    if (which == NET_HPPI_SEND) {
	if (connStatePtr->numOutstandingSend >= connPtr->outstandingSends) {
	    status = GEN_ENOENT;
	    goto exit;
	}
	connStatePtr->numOutstandingSend += 1;
	List_Insert ((List_Links *)dataReqPtr, &(connStatePtr->sendHdr));
    } else {
	if (connStatePtr->numOutstandingRecv >= connPtr->outstandingRecvs) {
	    status = GEN_ENOENT;
	    goto exit;
	}
	connStatePtr->numOutstandingRecv += 1;
	List_Insert ((List_Links *)dataReqPtr, &(connStatePtr->recvHdr));
    }
    /*
     * Send the scatter-gather command to the DST board.
     */
    status = NetHppiSendCmd (statePtr, sgCmdSize, (Address)&sgCmd,
			     NET_HPPI_DST_CMD);
    if (status != SUCCESS) {
	goto exit;
    }

    /*
     * Build the data request command and send it to the SRC board.
     */
    sendCmd.xrbHdr.tag = sgCmd.tag;
    sendCmd.xrbHdr.xrbRef = connStatePtr->xrbRef;
    sendCmd.xrbHdr.xrbId = (uint32)connStatePtr;
    sendCmd.xrbHdr.xrbBufId = (uint32)dataReqPtr;
    sendCmd.cnt = dataReqPtr->xferSize;
    if (which == NET_HPPI_SEND) {
	sendCmd.hdr.opcode = DEV_HPPI_SEND_SEND;
    } else {
	sendCmd.hdr.opcode = DEV_HPPI_SEND_RECV;
    }
    status = NetHppiSendCmd (statePtr, sizeof (sendCmd), (Address)&sendCmd,
			     NET_HPPI_SRC_CMD);
  exit:
    MASTER_UNLOCK (&interPtr->mutex);
    return status;
}

/*----------------------------------------------------------------------
 *
 * Net_HppiSendData --
 *
 *	Send data over an established connection.
 *
 * Results:
 *	Standard Sprite return status.
 * Side effects:
 *	Data sent over the HIPPI boards.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_HppiSendData (connPtr, dataReqPtr)
Net_HppiConnection*	connPtr;
Net_HppiDataRequest*	dataReqPtr;
{
    return (NetHppiXferData (connPtr, dataReqPtr, NET_HPPI_SEND));
}

/*----------------------------------------------------------------------
 *
 * Net_HppiReceiveData --
 *
 *	Queue up a request to receive data over an established connection.
 *	The callback routine will be called when all of the data is
 *	received.  The data must be contiguous within the connection (ie,
 *	only one start offset and length), but may be placed according
 *	to the request's scatter-gather array.
 *
 * Results:
 *	Standard Sprite return status.
 * Side effects:
 *	Requests that data be received.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Net_HppiReceiveData (connPtr, dataReqPtr)
Net_HppiConnection*	connPtr;
Net_HppiDataRequest*	dataReqPtr;
{
    return (NetHppiXferData (connPtr, dataReqPtr, NET_HPPI_RECV));
}

/*----------------------------------------------------------------------
 *
 * iocConnCallback
 *
 *	Callback for the IOC connection call.  This wakes up the waiting
 *	IOC.
 *
 *----------------------------------------------------------------------
 */
static
void
iocConnCallback (connPtr)
Net_HppiConnection*	connPtr;
{
    Sync_Broadcast ((Sync_Condition *) connPtr->userData);
}

static
void
iocXferCallback (dataReqPtr)
Net_HppiDataRequest*	dataReqPtr;
{
    Sync_Broadcast ((Sync_Condition *) dataReqPtr->userData);
}
