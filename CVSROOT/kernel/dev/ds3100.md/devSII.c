/* 
 *  devSII.c --
 *
 *	The driver for the SII chip.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "sii.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "scsi.h"
#include "sync.h"
#include "stdlib.h"
#include "user/fs.h"

#define RESET TRUE

/* 
 * MACROS for timing out spin loops.
 *
 *	Waits while expression is true.
 *
 *	args:	expr 		- expression to spin on
 *		spincount 	- amount of time to wait in microseconds
 *		retval		- return value of this macro
 */
#define	SII_WAIT_WHILE(expr,spincount,retval) {				\
		for (retval = 0; ((retval < 100) && (expr)); retval++);	\
		while ((expr) && ( retval < spincount)) {		\
			MACH_DELAY(100);				\
			retval++;					\
		}							\
	}

/*
 * 	Waits unitl expression is true.
 */
#define SII_WAIT_UNTIL(expr,spincount,retval) {				\
		SII_WAIT_WHILE(!(expr),spincount,retval);		\
	}

/*
 * The different phases.
 */
#define MSG_IN_PHASE    0x7
#define MSG_OUT_PHASE	0x6
#define STATUS_PHASE	0x3
#define CMD_PHASE	0x2
#define DATA_IN_PHASE	0x1
#define DATA_OUT_PHASE	0x0

/*
 * Forward declaration. 
 */
typedef struct Controller Controller;

/*
 * Device - The data structure containing information about a device. One of
 * these structure is kept for each attached device. Note that is structure
 * is casted into a ScsiDevice and returned to higher level software.
 * This implies that the ScsiDevice must be the first field in this
 * structure.
 */

typedef struct Device {
    ScsiDevice	handle;	/* Scsi Device handle. This is the only part
			 * of this structure visible to higher 
			 * level software. MUST BE FIRST FIELD IN STRUCTURE. */
    int		targetID;/* SCSI Target ID of this device. Note that
			  * the LUN is store in the device handle. */
    Controller	*ctrlPtr;/* Controller to which device is attached. */
		   /*
		    * The following part of this structure is 
		    * used to handle SCSI commands that return 
		    * CHECK status. To handle the REQUEST SENSE
		    * command we must: 1) Save the state of the current
		    * command into the "struct FrozenCommand". 2) Submit
		    * a request sense command formatted in SenseCmd
		    * to the device.
		    */
    unsigned	buffOffset;
    struct FrozenCommand {		       
	ScsiCmd	*scsiCmdPtr;	   /* The frozen command. */
	unsigned char statusByte; /* It's SCSI status byte, Will always have
				   * the check bit set.
				   */
	int amountTransferred;    /* Number of bytes transferred by this 
				   * command.
				   */
	int status;		  /* Status of frozen command. */
    } frozen;	
    char senseBuffer[DEV_MAX_SENSE_BYTES]; /* Data buffer for request sense */
    ScsiCmd		SenseCmd;  	   /* Request sense command buffer. */
} Device;

/*
 * Controller - The Data structure describing an SII controller.
 */
struct Controller {
    volatile SIIRegs *regsPtr; /* Pointer to the registers of this controller.*/
    int	    dmaState;	/* DMA state for this controller, defined below. */
    Boolean dmaStarted; /* TRUE => dma was started. */
    char    *name;	/* String for error message for this controller.  */
    DevCtrlQueues devQueues;    /* Device queues for devices attached to this
				 * controller.	 */
    Address	ramBuff;	/* DMA memory. */
    Sync_Semaphore mutex; /* Lock protecting controller's data structures. */
			  /* Until disconnect/reconnect is added we can have
			   * only one current active device and scsi command.*/
    Device     *devPtr;	   /* Current active command. */
    ScsiCmd   *scsiCmdPtr; /* Current active command. */
    Device  *devicePtr[8][8]; /* Pointers to the device attached to the 
			       * controller index by [targetID][LUN].
			       * NIL if device not attached yet. Zero if
			       * device conflicts with HBA address.  */
};

/*
 * Possible values for the dmaState state field of a controller.
 *
 * DMA_RECEIVE  - data is being received from the device, such as on
 *	a read, inquiry, or request sense.
 * DMA_SEND     - data is being send to the device, such as on a write.
 * DMA_INACTIVE - no data needs to be transferred.
 */

#define DMA_RECEIVE 0
#define	DMA_SEND 1
#define	DMA_INACTIVE 2


/*
 * MAX_SII_CTRLS - Maximum number of SII controllers attached to the
 *		     system.
 */
#define	MAX_SII_CTRLS	1
static Controller *controllers[MAX_SII_CTRLS];

/*
 * Highest number controller we have probed for.
 */
static int numSIIControllers = 0;

int devSIIDebug = 1;

/*
 * Forward declarations.  
 */

static void		Reset();
static ReturnStatus	SendCommand();
static ReturnStatus	GetStatusByte();
static ReturnStatus	WaitPhase();
static ReturnStatus 	RecvBytes();
static void 		PrintRegs();
static ReturnStatus 	StartDMA();
static char		*PhaseName();


/*
 *----------------------------------------------------------------------
 *
 * Reset --
 *
 *	Reset a SCSI bus controlled by the SII.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the controller and SCSI bus.
 *
 *----------------------------------------------------------------------
 */
static void
Reset(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile SIIRegs *regsPtr = ctrlPtr->regsPtr;

    /*
     * Reset the SII chip.
     */
    regsPtr->comm = SII_CHRESET;
    /*
     * Set arbitrated bus mode.
     */
    regsPtr->csr = SII_HPM;
    /*
     * SII is always ID 7.
     */
    regsPtr->id = SII_ID_IO | 7;
    /*
     * Enable SII to drive SCSI bus and turn off synchronous commands.
     */
    regsPtr->dictrl = SII_PRE;
    regsPtr->dmctrl = 0;
    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. SII_RST is self clearing.
     */
    regsPtr->comm = SII_RST;
    MACH_DELAY(25);
    /*
     * Clear any pending interrupts from the reset.
     */
    regsPtr->cstat = regsPtr->cstat;
    regsPtr->dstat = regsPtr->dstat;
    /*
     * Set up SII for arbitrated bus mode, SCSI parity checking,
     * Select Enable, Reselect Enable, and Interrupt Enable.
     */
    regsPtr->csr = (SII_HPM | SII_RSE | SII_SLE | SII_PCE | SII_IE);
    MACH_DELAY(5 * 1000000);
}


/*
 *----------------------------------------------------------------------
 *
 * SelectTarget --
 *
 *      Select a target.
 *
 * Results:
 *	SUCCESS if could select, FAILURE if couldn't.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
SelectTarget(devPtr)
    Device	*devPtr;	/* Device we want to select target for. */
{
    volatile register SIIRegs *regsPtr;	
    int retval, i;

    regsPtr = devPtr->ctrlPtr->regsPtr;

    /* Loop till retries exhausted */
    for(i=0; i < 2; i++) {
        regsPtr->slcsr = devPtr->targetID;
        regsPtr->comm = SII_SELECT;
	Mach_EmptyWriteBuffer();

        /* 
	 * Start timer to wait for a select to occur
	 */
        SII_WAIT_UNTIL(regsPtr->cstat & SII_SCH, SII_WAIT_COUNT/4, retval);

	/* 
	 * If a state change did occur then make sure we are connected
	 */
	if((regsPtr->cstat & SII_SCH) && !(regsPtr->cstat & SII_CON)) {
            SII_WAIT_UNTIL((regsPtr->cstat & SII_CON),SII_WAIT_COUNT,retval);
	}

	if(retval >= SII_WAIT_COUNT || !(regsPtr->cstat & SII_CON)) {
	    regsPtr->cstat = SII_SCH;
	    Mach_EmptyWriteBuffer();
	    continue;
        }

	regsPtr->cstat = SII_SCH;
	Mach_EmptyWriteBuffer();
	return(SUCCESS);
    }
    /* 
     * Selection failed, clear all bus signals
     */
    if (devSIIDebug > 0) {
	printf("SelectTarget: Couldn't select target %d\n", devPtr->targetID);
    }
    if (devSIIDebug > 4) {
	printf("Cstat = 0x%x\n", regsPtr->cstat);
    }
    regsPtr->cstat = SII_SCH;
    regsPtr->comm = SII_DISCON;
    Mach_EmptyWriteBuffer();
    SII_WAIT_UNTIL((regsPtr->cstat & SII_SCH), SII_WAIT_COUNT, retval);
    regsPtr->cstat = 0xffff;
    regsPtr->dstat = 0xffff;
    regsPtr->comm = 0;
    Mach_EmptyWriteBuffer();
    return(FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Send a command to a SCSI controller via the SII.
 *
 *	NOTE: The caller is assumed to have the master lock of the controller
 *	to which the device is attached held.
 *
 *	NOTE2: We are always called with the master lock down so we don't
 *	       have to worry about interrupts jumping in at the wrong time.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Those of the command (Read, write etc.)
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SendCommand(devPtr, scsiCmdPtr)
    Device	 *devPtr;		/* Device to sent to. */
    ScsiCmd	*scsiCmdPtr;		/* Command to send. */
{
    ReturnStatus		status;
    volatile register SIIRegs	*regsPtr;
    register char		*charPtr;
    Controller			*ctrlPtr;
    int				size;
    Address 			addr;	
    unsigned short		tmpPhase;
    unsigned short		tmpState;
    unsigned			retval;

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    ctrlPtr->scsiCmdPtr = scsiCmdPtr;
    ctrlPtr->devPtr = devPtr;
    size = scsiCmdPtr->bufferLen;
    addr = scsiCmdPtr->buffer;
    /*
     * Determine the DMA state the size and direction of data transfer.
     */
    if (size == 0) {
	ctrlPtr->dmaState = DMA_INACTIVE;
    } else {
	ctrlPtr->dmaState = (scsiCmdPtr->dataToDevice) ? DMA_SEND :
							 DMA_RECEIVE;
    }
    ctrlPtr->dmaStarted = FALSE;

    if (devSIIDebug > 3) {
	printf("SIICommand: %s cmd 0x%x addr %x size %d dma %s\n",
	    devPtr->handle.locationName, scsiCmdPtr->commandBlock[0], addr,
	    size, (ctrlPtr->dmaState == DMA_INACTIVE) ? "not active" :
		((ctrlPtr->dmaState == DMA_SEND) ? "send" :
						      "receive"));
    }

    regsPtr = (SIIRegs *)ctrlPtr->regsPtr;
    status = SelectTarget(devPtr);
    if (status != SUCCESS) { 
	return(status);
    }

    if (devSIIDebug > 5) {
	printf("SIICommand: %s waiting for command phase.\n",
	    devPtr->handle.locationName);
    }
    status = WaitPhase(ctrlPtr, CMD_PHASE, RESET);
    if (status != SUCCESS) {
	if (devSIIDebug > 0) {
	    printf("SII: wait on CMD_PHASE failed.\n");
	}
	return(status);
    }
    /*
     * Stuff the control block through the commandStatus register.
     */
    if (devSIIDebug > 5) {
	printf("SCSI3Command: %s stuffing command of %d bytes.\n", 
		devPtr->handle.locationName, scsiCmdPtr->commandBlockLen);
    }
    charPtr = scsiCmdPtr->commandBlock;

    if((unsigned char)charPtr & 0x1) {
	panic("SendCommand: Misaligned scsi command\n");
    }
    status = StartDMA(ctrlPtr, DMA_SEND, TRUE, scsiCmdPtr->commandBlockLen, 
		    charPtr);
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * StartDMA --
 *
 *	Issue the sequence of commands to the controller to start DMA.
 *	If the wait parameter is TRUE then this procedure will wait 
 *	until the DMA is finished and copy the results into the
 *	buffer. Otherwise it will just return and whoever handles the
 *	DMA interrupt will have to copy the data.
 *
 *	NOTE: the data buffer should be word-aligned for DMA out.
 *
 * Results:
 *	SUCCESS if DMA was started (and possibly finished) correctly.
 *	DEV_TIMEOUT if we timed out waiting for the DMA to finish.
 *
 * Side effects:
 *	DMA is enabled.  No registers other than the control register are
 *	to be accessed until DMA is disabled again.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
StartDMA(ctrlPtr, direction, wait, size, buffer)
    register Controller *ctrlPtr;	/* Controller */
    Boolean		wait;		/* TRUE => wait for DMA to complete. */
    int			direction;	/* DMA_SEND or DMA_RECEIVE. */
    int			size;		/* # of bytes to transfer. */
    char		*buffer;	/* Data buffer. */
{
    volatile register SIIRegs	*regsPtr;
    register Device		*devPtr;
    unsigned short		tmpPhase;
    unsigned short		tmpState;

    devPtr = ctrlPtr->devPtr;

    if (devSIIDebug > 4) {
	printf("%s: StartDMA %s called size = %d.\n", ctrlPtr->name,
	    (direction == DMA_RECEIVE) ? "receive" :
		((direction == DMA_SEND) ? "send" :
						  "not-active!"), size);
    }
    regsPtr = ctrlPtr->regsPtr;
    regsPtr->comm &= ~(SII_INXFER | SII_DMA);
    regsPtr->dstat = SII_DNE;
    Mach_EmptyWriteBuffer();
    if (direction == DMA_SEND) {
	CopyToBuffer((unsigned short *)buffer, 
		     (unsigned short *)(ctrlPtr->ramBuff + devPtr->buffOffset),
		     size);
	Mach_EmptyWriteBuffer();
    }
    regsPtr->dmaddrl = devPtr->buffOffset & 0x00ffffff;
    regsPtr->dmaddrh = (devPtr->buffOffset & 0x00ffffff) >> 16;
    regsPtr->dmlotc = size;
    Mach_EmptyWriteBuffer();
    tmpPhase = regsPtr->dstat & SII_PHA_MSK;
    tmpState = regsPtr->cstat & SII_STATE_MSK;
    if (devSIIDebug > 4) {
	printf("StartDMA: dstat is 0x%x.\n", regsPtr->dstat);
	printf("StartDMA: cstat is 0x%x.\n", regsPtr->cstat);
	printf("StartDMA: setting comm to 0x%x.\n", 
	    SII_DMA | SII_INXFER | tmpState | tmpPhase);
    }
    regsPtr->comm = SII_DMA | SII_INXFER | tmpState | tmpPhase;
    Mach_EmptyWriteBuffer();

    if (wait) {
	int	retval;

	/*
	 * Wait for the DMA to complete.
	 */
	SII_WAIT_UNTIL((regsPtr->dstat & SII_DNE), SII_WAIT_COUNT, retval);
	regsPtr->comm &= ~(SII_INXFER | SII_DMA);
	if (!(regsPtr->dstat & SII_DNE)) {
	    printf("StartDMA: DMA failed\n");
	}
	if (retval >= SII_WAIT_COUNT) {
	    return (DEV_TIMEOUT);
	}
	regsPtr->dstat = SII_DNE;
	regsPtr->dmlotc = 0;
	Mach_EmptyWriteBuffer();
	if (direction == DMA_RECEIVE) {
	    CopyFromBuffer(
		(unsigned short *)(ctrlPtr->ramBuff+devPtr->buffOffset),
	        (unsigned char *) buffer, size);
	}
    } else {
	ctrlPtr->dmaStarted = TRUE;
    }
    return (SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * GetStatusByte --
 *
 *	Complete an SCSI command by getting the status bytes from
 *	the device and waiting for the ``command complete''
 *	message that follows the status bytes.  
 *
 * Results:
 *	An error code if the status didn't come through or it
 *	indicated an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetStatusByte(ctrlPtr,statusBytePtr)
    Controller *ctrlPtr;		/* Controller to get byte from. */
    unsigned char *statusBytePtr;	/* Where to put the status byte. */
{
    ReturnStatus		status;
    char			message;

    if (devSIIDebug > 4) {
	printf("GetStatusByte called ");
    }
    *statusBytePtr = 0;

    /*
     * After the DATA_IN/OUT phase we enter the STATUS phase for
     * 1 byte (usually) of status.  This is followed by the MESSAGE phase
     */
    status = WaitPhase(ctrlPtr, STATUS_PHASE, RESET);
    if (status != SUCCESS) {
	if (devSIIDebug > 3) {
	    printf("Warning: %s wait on PHASE_STATUS failed.\n",ctrlPtr->name);
	}
	return(status);
    }
    /*
     * Get one status byte.
     */
    status = StartDMA(ctrlPtr, DMA_RECEIVE, TRUE, 1, (char *)statusBytePtr);
    if (status != SUCCESS) {
	printf("Warning: %s error 0x%x getting status byte\n", 
		ctrlPtr->name, status);
	return (status);
    }
    if (devSIIDebug > 4) {
	printf("got 0x%x\n", *statusBytePtr);
    }
    /*
     * Wait for the message in phase and grab the COMMAND COMPLETE message
     * off the bus.
     */
    status = WaitPhase(ctrlPtr, MSG_IN_PHASE, RESET);
    if (status != SUCCESS) {
        printf("Warning: %s wait on PHASE_MSG_IN after status failed.\n",
		ctrlPtr->name);
	return(status);
    } 
    status = StartDMA(ctrlPtr, DMA_RECEIVE, TRUE, 1, &message);
    if (status != SUCCESS) {
	printf("Warning: %s got error 0x%x getting message and status.\n",
			     ctrlPtr->name, status);
	return(status);
    }
    if (message != SCSI_COMMAND_COMPLETE) {
	printf("Warning: %s message %d after status is not command complete.\n",		ctrlPtr->name, message);
	return(FAILURE);
    }
    if (devSIIDebug > 4) {
	printf("Got message 0x%x\n", message);
    }
    return(SUCCESS);
}


#define	WAIT_LENGTH		500000


/*
 *----------------------------------------------------------------------
 *
 * WaitPhase --
 *
 *	Wait for a phase to be signalled in the controller registers.
 * 	This is a specialized version of WaitReg, which compares
 * 	all the phase bits to make sure the phase is exactly what is
 *	requested and not something that matches only in some bits.
 *
 * Results:
 *	SUCCESS if the condition occurred before a threshold time limit,
 *	DEV_TIMEOUT otherwise.
 *
 * Side effects:
 *	This resets the SCSI bus if the condition bits are not set by
 *	the controller before timeout.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WaitPhase(ctrlPtr, phase, reset)
    Controller *ctrlPtr;	/* Controller state */
    unsigned char phase;	/* phase to check */
    Boolean reset;		/* whether to reset the bus on error */
{
    volatile register SIIRegs	*regsPtr = (SIIRegs *)ctrlPtr->regsPtr;
    register int		i;
    ReturnStatus		status = DEV_TIMEOUT;
    register unsigned short	dstatReg;

    for (i=0 ; i < WAIT_LENGTH ; i++) {
	dstatReg = regsPtr->dstat;
	if (devSIIDebug > 10 && i < 5) {
	    printf("%d/%x ", i, dstatReg);
	}
	if ((dstatReg & SII_PHA_MSK) == phase) {
	    return(SUCCESS);
	}
    }
    if (devSIIDebug > 5) {
	printf("WaitPhase: timed out.\n");
	PrintRegs(regsPtr);
	printf("WaitPhase: was checking for phase 0x%x.\n",
		   (int) phase);
	printf("WaitPhase: dstat is 0x%x.\n", dstatReg);
    }
    if (reset) {
	Reset(ctrlPtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRegs --
 *
 *	Print out the interesting registers.  This could be a macro but
 *	then it couldn't be called from kdbx.  This routine is necessary
 *	because kdbx doesn't print all the character values properly.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data is displayed on the console or to the debugger.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
PrintRegs(regsPtr)
    volatile	SIIRegs *regsPtr;
{
}


/*
 *----------------------------------------------------------------------
 *
 *  SpecialSenseProc --
 *
 *	Special function used for HBA generated REQUEST SENSE. A SCSI
 *	command request with this function as a call back proc will
 *	be processed by routine RequestDone as a result of a 
 *	REQUEST SENSE. This routine is never called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SpecialSenseProc()
{
}


/*
 *----------------------------------------------------------------------
 *
 * RequestDone --
 *
 *	Process a request that has finished. Unless a SCSI check condition
 *	bit is present in the status returned, the request call back
 *	function is called.  If check condition is set we fire off a
 *	SCSI REQUEST SENSE to get the error sense bytes from the device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The call back function may be called.
 *
 *----------------------------------------------------------------------
 */

void
RequestDone(devPtr, scsiCmdPtr, status, scsiStatusByte, amountTransferred)
    Device		*devPtr;		/* Device for request. */
    ScsiCmd		*scsiCmdPtr;		/* Request that finished. */
    ReturnStatus	status;			/* Status returned. */
    unsigned char	scsiStatusByte;		/* SCSI Status Byte. */
    int			amountTransferred;	/* Amount transferred by
						 * command. */
{
    ReturnStatus	senseStatus;
    Controller	        *ctrlPtr = devPtr->ctrlPtr;


    if (devSIIDebug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
    /*
     * First check to see if this is the reponse of a HBA generated 
     * REQUEST SENSE command.  If this is the case, we can process
     * the callback of the frozen command for this device and
     * allow the flow of command to the device to be resummed.
     */
    if (scsiCmdPtr->doneProc == SpecialSenseProc) {
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
			devPtr->frozen.status,
			devPtr->frozen.statusByte, 
			devPtr->frozen.amountTransferred,
			amountTransferred,
			devPtr->senseBuffer);
	 ctrlPtr->scsiCmdPtr = (ScsiCmd *)NIL;
	 return;
    }
    /*
     * This must be an outside request finishing. If the request 
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) || !SCSI_CHECK_STATUS(scsiStatusByte)) {
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	 ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
	 return;
   } 
   if (status != SUCCESS) {
       amountTransferred = 0;
   }
   /*
    * If we got here than the SCSI command came back from the device
    * with the CHECK bit set in the status byte.
    * Need to perform a REQUEST SENSE. Move the current request 
    * into the frozen state and issue a REQUEST SENSE. 
    */
   MASTER_LOCK(&(ctrlPtr->mutex));
   devPtr->frozen.scsiCmdPtr = scsiCmdPtr;
   devPtr->frozen.statusByte = scsiStatusByte;
   devPtr->frozen.amountTransferred = amountTransferred;
   devPtr->frozen.status = status;
   DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES, 
		   devPtr->senseBuffer, &(devPtr->SenseCmd));
   devPtr->SenseCmd.doneProc = SpecialSenseProc,
   senseStatus = SendCommand(devPtr, &(devPtr->SenseCmd));
   MASTER_UNLOCK(&(ctrlPtr->mutex));
   /*
    * If we got an HBA error on the REQUEST SENSE we end the outside 
    * command with the SUCCESS status but zero sense bytes returned.
    */
   if (senseStatus != SUCCESS) {
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
   }

}

/*
 *----------------------------------------------------------------------
 *
 * entryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for this
 *	controller. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for this controller. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *	NOTE: This routine is also called from Dev_SIIIntr to start the
 *	next request after the previously one finishes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Request may be dequeue and submitted to the device. Request callback
 *	function may be called.
 *
 *----------------------------------------------------------------------
 */

static Boolean
entryAvailProc(clientData, newRequestPtr) 
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links	*newRequestPtr;	/* The new SCSI request. */
{
    register Device	 *devPtr;
    register Controller	*ctrlPtr;
    register ScsiCmd	*scsiCmdPtr;
    ReturnStatus	status;

    static Address	notifications[100];
    static Address	sent[100];
    static Address	bailed[100];
    static Address	cause[100];
    static Address	failed[100];
    static int		nctr = 0;
    static int		sctr = 0;
    static int		bctr = 0;
    static int		fctr = 0;
#define INC(a) { (a) = ((a) + 1) % 100; }

    devPtr = (Device *) clientData;
    ctrlPtr = devPtr->ctrlPtr;
    /*
     * If we are busy (have an active request) just return. Otherwise 
     * start the request.
     */

    notifications[nctr] = (Address) newRequestPtr;
    INC(nctr);
    if (ctrlPtr->scsiCmdPtr != (ScsiCmd *) NIL) {
	bailed[bctr] = (Address) newRequestPtr;
	cause[bctr] = (Address) ctrlPtr->scsiCmdPtr;
	INC(bctr);
	return FALSE;
    }
again:
    scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    devPtr = (Device *) clientData;
    sent[sctr] = (Address) scsiCmdPtr;
    INC(sctr);
    status = SendCommand((Device *) devPtr, scsiCmdPtr);
    /*	
     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	failed[fctr] = (Address) scsiCmdPtr;
	INC(fctr);
	 MASTER_UNLOCK(&(ctrlPtr->mutex));
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
	 MASTER_LOCK(&(ctrlPtr->mutex));
    }
    if (ctrlPtr->scsiCmdPtr == (ScsiCmd *) NIL) {
        newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
                                DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
        if (newRequestPtr != (List_Links *) NIL) {
            goto again;
        }
    }
    return TRUE;

}   


/*
 *----------------------------------------------------------------------
 *
 * Dev_SIIIntr --
 *
 *	Handle interrupts from the SII controller.
 *
 * Results:
 *	TRUE if an SII controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SIIIntr()
{
    Controller			*ctrlPtr;
    volatile register SIIRegs	*regsPtr;
    Device			*devPtr;
    unsigned char		statusByte = 0;
    ReturnStatus		status;
    register int		i;
    Boolean			found;
    List_Links			*newRequestPtr;
    ClientData			clientData;
    unsigned short		cstat;
    unsigned short		dstat;

    if (devSIIDebug > 4) {
	printf("DevSIIIntr: ");
    }
    found = FALSE;
    /*
     * Find which controller caused the interrupt.
     */
    for (i = 0; i < numSIIControllers; i++) {
	ctrlPtr = controllers[i];
	if (ctrlPtr == (Controller *) 0) {
	    continue;
	}
	regsPtr = (SIIRegs *)ctrlPtr->regsPtr;
	devPtr = ctrlPtr->devPtr;
	cstat = regsPtr->cstat;
	dstat = regsPtr->dstat;
	if (cstat & (SII_CI | SII_DI)) {
	    found = TRUE;
	    break;
	}
    }
    if (!found) {
	if (devSIIDebug > 4 ) {
	    printf("spurious\n");
	}
	return FALSE;
    }

    /*
     * Acknowledge everything.
     */
    regsPtr->cstat = cstat;
    regsPtr->dstat = dstat;
    Mach_EmptyWriteBuffer();

    /*
     * Check for a BUS ERROR
     */
    if(cstat & SII_BER) {
	if (devSIIDebug > 4) {
	    printf("Bus error");
	}
    }

    /*
     * Check for a PARITY ERROR
     */
    if(dstat & SII_IPE) {
	printf("Dev_SIIIntr: Parity error!!\n");
	goto rtnHardErrorAndGetNext;
    }

    /* 
     * Check for a BUS RESET
     */
    if(cstat & SII_RST_ONBUS) {
	printf("Dev_SIIIntr: Bus reset!!\n");
	goto rtnHardErrorAndGetNext;
    }

    /*
     * Check for a state change.
     */
    if (cstat & SII_SCH) {
	if (devSIIDebug > 4) {
	    printf("State change ");
	}
    }

    /*
     * Check for DMA completion.
     */
    if (dstat & SII_DNE) {
	if (devSIIDebug > 4) {
	    printf("DMA complete ");
	}
    }

    /*
     * Check for phase change.
     */
    if (dstat & SII_MIS) {
	if (devSIIDebug > 4) {
	    printf("Phase change ");
	}
	switch (dstat & SII_PHA_MSK) {
	    case DATA_IN_PHASE:
	    case DATA_OUT_PHASE: {
		if (devSIIDebug > 4) {
		    printf("Data Phase Interrupt\n");
		}
		status = StartDMA(ctrlPtr, ctrlPtr->dmaState, FALSE, 
		    ctrlPtr->scsiCmdPtr->bufferLen, 
		    ctrlPtr->scsiCmdPtr->buffer);
		if (status != SUCCESS) {
		    printf("Warning: couldn't start dma.\n");
		}
		return(TRUE);
	    }
	    case MSG_IN_PHASE: {
		char	message;
		status = StartDMA(ctrlPtr, DMA_RECEIVE, TRUE, 1, &message);
		if (devSIIDebug > 4) {
		    printf("Msg Phase Interrupt\n");
		}
		if (status != SUCCESS) {
		    printf("Warning: %s couldn't get message.\n",ctrlPtr->name);
		    return(TRUE);
		}
		if (message != SCSI_COMMAND_COMPLETE) {
		    printf("Warning: %s couldn't handle message 0x%x from %s.\n",
			    ctrlPtr->name, message, 
			    ctrlPtr->devPtr->handle.locationName);
		}
		return(TRUE);
	    }
	    case STATUS_PHASE: {
		if (devSIIDebug > 4) {
		    printf("Status Phase Interrupt\n");
		}
		if ((ctrlPtr->dmaStarted == TRUE) && 
		    (ctrlPtr->dmaState == DMA_RECEIVE)) {
		    /*
		     * We just transitioned from the data phase so copy
		     * in the data.
		     */
		    CopyFromBuffer((unsigned short *)(ctrlPtr->ramBuff + 
						      devPtr->buffOffset),
			       (unsigned char *)ctrlPtr->scsiCmdPtr->buffer,
		   (int)(ctrlPtr->scsiCmdPtr->bufferLen - regsPtr->dmlotc));
		   ctrlPtr->dmaStarted = FALSE;
		}
    
		status =  GetStatusByte(ctrlPtr, &statusByte);
		if (status != SUCCESS) {
					    /* Return the hard error message
					     * to the call and get the next 
					     * entry in the devQueue. */
		    goto rtnHardErrorAndGetNext;
		}
		RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, statusByte,
		    (int)(ctrlPtr->scsiCmdPtr->bufferLen - regsPtr->dmlotc));
		if (ctrlPtr->scsiCmdPtr == (ScsiCmd *)NIL) {
		    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				    DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
		    if (newRequestPtr != (List_Links *) NIL) { 
			MASTER_LOCK(&(ctrlPtr->mutex));
			entryAvailProc(clientData,newRequestPtr);
			MASTER_UNLOCK(&(ctrlPtr->mutex));
		    }
		}
		return(TRUE);
	    }
	    default: {
		printf("Warning: %s couldn't handle phase %x... ignoring.\n",
			   ctrlPtr->name, dstat & SII_PHA_MSK);
		if (devSIIDebug > 0) {
		    PrintRegs(regsPtr);
		}
		return(TRUE);
	    }
	}
    }
    if (devSIIDebug > 4) {
	printf("\n");
    }

    return(TRUE);

    /*
     * Jump here to return an error and reset the HBA.
     */
rtnHardErrorAndGetNext:
    printf("Warning: %s reset and current command terminated.\n",
	   devPtr->handle.locationName);
    if (ctrlPtr->scsiCmdPtr != (ScsiCmd *) NIL) {
	RequestDone(devPtr,ctrlPtr->scsiCmdPtr,status,statusByte,0);
    }
    Reset(ctrlPtr);
    /*
     * Use the queue entryAvailProc to start the next request for this device.
     */
    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
    if (newRequestPtr != (List_Links *) NIL) { 
	MASTER_LOCK(&(ctrlPtr->mutex));
	entryAvailProc(clientData,newRequestPtr);
	MASTER_UNLOCK(&(ctrlPtr->mutex));
    }
    return (TRUE);

}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *
 *	Device release proc for controller.
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
static ReturnStatus
ReleaseProc(scsiDevicePtr)
    ScsiDevice	*scsiDevicePtr;
{

}


/*
 *----------------------------------------------------------------------
 *
 * DevSIIInit --
 *
 *	Check for the existant of the SII controller. If it
 *	exists allocate data stuctures for it.
 *
 * Results:
 *	TRUE if the controller exists, FALSE otherwise.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevSIIInit(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int		ctrlNum;
    Controller *ctrlPtr;
    int		i,j;

    ctrlNum = ctrlLocPtr->controllerID;
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    if (ctrlNum+1 > numSIIControllers) {
	numSIIControllers = ctrlNum + 1;
    }
    controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (SIIRegs *) (ctrlLocPtr->address);
    ctrlPtr->ramBuff = SII_BUF_BASE;
    ctrlPtr->name = ctrlLocPtr->name;
    Sync_SemInitDynamic(&(ctrlPtr->mutex),ctrlPtr->name);
    /* 
     * Initialized the name, device queue header, and the master lock.
     * The controller comes up with no devices active and no devices
     * attached.  Reserved the devices associated with the 
     * targetID of the controller (7).
     */
    ctrlPtr->devQueues = Dev_CtrlQueuesCreate(&(ctrlPtr->mutex),entryAvailProc);
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    ctrlPtr->devicePtr[i][j] = (i == 7) ? (Device *) 0 : (Device *) NIL;
	}
    }
    ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
    Reset(ctrlPtr);
    return((ClientData) ctrlPtr);
}

/*
 * Offset into DMA buffer. We partition the DMA buffer into seperate 
 * regions for each device.  We have to have seperate regions if we
 * ever want disconnect/reconnect.
 *
 * IMPORTANT!!!
 * For some reason DMA doesn't work if this offset is anything other than
 * 0. Do not increment it when attaching devices.
 */

static unsigned ramBuffOffset = 0;


/*
 *----------------------------------------------------------------------
 *
 * DevSIIDevice --
 *
 *	Attach a SCSI device using the SII HBA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ScsiDevice   *
DevSIIAttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	 /* Device to attach. */
    void	(*insertProc)(); /* Queue insert procedure. */
{
    Device		*devPtr;
    Controller		*ctrlPtr;
    char   		tmpBuffer[512];
    int	   		length;
    int	   		ctrlNum;
    int	   		targetID, lun;
    ScsiInquiryData	inqData;
    ScsiCmd		scsiCmd;
    ReturnStatus	status;
    unsigned char	statusByte;
    extern char		*strcpy();

    /*
     * First find the SII controller this device is on.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr);
    if ((ctrlNum > MAX_SII_CTRLS) ||
	(controllers[ctrlNum] == (Controller *) 0)) { 
	return (ScsiDevice  *) NIL;
    } 
    ctrlPtr = controllers[ctrlNum];
    /*
     * See if the device is already present.
     */
    targetID = SCSI_TARGET_ID(devicePtr);
    lun = SCSI_LUN(devicePtr);
    MASTER_LOCK(&(ctrlPtr->mutex));

    /*
     * A device pointer of zero means that targetID/LUN 
     * conflicts with that of the HBA. A NIL means the
     * device hasn't been attached yet.
     */
    if (ctrlPtr->devicePtr[targetID][lun] == (Device *) 0) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return (ScsiDevice *) NIL;
    }
    if (ctrlPtr->devicePtr[targetID][lun] != (Device *) NIL) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return (ScsiDevice *) (ctrlPtr->devicePtr[targetID][lun]);
    }

    /*
     * Initialize the device struct.
     */
    devPtr = (Device *) malloc(sizeof(Device));
    bzero((char *) devPtr, sizeof(Device));
    devPtr->targetID = targetID;
    devPtr->ctrlPtr = ctrlPtr;
    devPtr->buffOffset = ramBuffOffset;
    ctrlPtr->devPtr = (Device *)NIL;
    ctrlPtr->scsiCmdPtr = (ScsiCmd *)NIL;
    ctrlPtr->devicePtr[targetID][lun] = devPtr;
    /*
     * Until we support disconnect/reconnect all device queues are
     * stored under the same queueBit
     */
    devPtr->handle.devQueue = Dev_QueueCreate(ctrlPtr->devQueues,
				1, insertProc, (ClientData) devPtr);
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.LUN = lun;
    devPtr->handle.maxTransferSize = SII_MAX_DMA_XFER_LENGTH;
    (void) sprintf(tmpBuffer, "%s#%d Target %d LUN %d", ctrlPtr->name, ctrlNum,
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    /*
     * IMPORTANT: the following statement is ifdef'd out for a reason.
     * See comment above.
     */
#if 0
    ramBuffOffset += SII_MAX_DMA_XFER_LENGTH * 2;
#endif

    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return (ScsiDevice *) devPtr;

error:
    ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    free((char *)devPtr);
    return((ScsiDevice *) NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * CopyToBuffer --
 *
 *	Copy data to the dma buffer. The dma buffer can only be written
 *	one word at a time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
CopyToBuffer(src, dst, length)
    register volatile unsigned short	*src;
    register volatile unsigned short	*dst;
    register int 			length;
{
    while(length > 0) {
	*dst++ = *src++;
	dst++;
	length -= 2;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CopyFromBuffer --
 *
 *	Copy data from the dma buffer. The dma buffer can only be read
 *	one word at a time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
CopyFromBuffer(src, dst, length)
    volatile register unsigned short	*src;
    volatile register unsigned char	*dst;
    register int			length;
{
    if((int)dst & 0x01) {
	while(length > 1) {
	    *dst++ = *src & 0xff;
	    *dst++ = (*src >> 8) & 0xff;
	    src += 2;
	    length -= 2;
	}
	if (length == 1) {
	    /*
	     * Handle an odd length.  Shove the last byte out.
	     */
	    *dst = *src & 0xff;
	}
    } else {
	register unsigned short *wdst = (unsigned short *)dst;
	register int		i;

	for(i = 0; i<length-16; i+=16) {
	    *wdst = *src;
	    *(wdst+1) = *(src+2);
	    *(wdst+2) = *(src+4);
	    *(wdst+3) = *(src+6);

	    *(wdst+4) = *(src+8);
	    *(wdst+5) = *(src+10);
	    *(wdst+6) = *(src+12);
	    *(wdst+7) = *(src+14);
	    src += 16;
	    wdst += 8;
	}
	while(i < length - 1) {
	    *wdst++ = *src;
	    src += 2;
	    i += 2;
	}
	if (i == length - 1) {
	    /*
	     * Handle an odd length.  Shove the last byte out.
	     */
	    *dst = *src & 0xff;
	}
    }
}
