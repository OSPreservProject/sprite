/* 
 * devSCSI0.c --
 *
 *	Driver routines specific to the original Sun Host Adaptor.
 *	This lives either on the Multibus or the VME.  It does
 *	not support dis-connect/connect.
 * 	This information is derived from Sun's "Manual for Sun SCSI
 *	Programmers", which details the layout of the this implementation
 *	of the Host Adaptor, the device that interfaces to the SCSI Bus.
 *
 * Copyright 1986 Regents of the University of California
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
#include "mach.h"
#include "scsi0.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "devMultibus.h"
#include "sync.h"
#include "stdlib.h"

/*
 * The device registers for the original Sun SCSI Host Adaptor.
 * Through these registers the SCSI bus is controlled.  There are the
 * usual status and control bits, and there are also registers through
 * which command blocks and status blocks are transmitted.  This format
 * is defined on Page 10. of Sun's SCSI Programmers' Manual.
 */
typedef struct CtrlRegs {
    unsigned char data;		/* Data register.  Contains the ID of the
				 * SCSI "target", or controller, for the 
				 * SELECT phase. Also, leftover odd bytes
				 * are left here after a read. */
    unsigned char pad1;		/* The other half of the data register which
				 * is never used by us */
    unsigned char commandStatus;/* Command and status blocks are passed
				 * in and out through this */
    unsigned char pad2;		/* The other half of the commandStatus register
				 * which never contains useful information */
    unsigned short control;	/* The SCSI interface control register.
				 * Bits are defined below */
    unsigned short pad3;
    unsigned int dmaAddress;	/* Target address for DMA */
    short 	dmaCount;	/* Number of bytes for DMA.  Initialize this
				 * this to minus the byte count minus 1 more,
				 * and the device increments to -1. If this
				 * is 0 or 1 after a transfer then there was
				 * a DMA overrun. */
    unsigned char pad4;
    unsigned char intrVector;	/* For VME, Index into autovector */
} CtrlRegs;

/*
 * Control bits in the SCSI Host Interface control register.
 *
 *	SCSI_PARITY_ERROR There was a parity error on the SCSI bus.
 *	SCSI_BUS_ERROR	There was a bus error on the SCSI bus.
 *	SCSI_ODD_LENGTH An odd byte is left over in the data register after
 *			a read or write.
 *	SCSI_INTERRUPT_REQUEST bit checked by polling routine.  If a command
 *			block is sent and the SCSI_INTERRUPT_ENABLE bit is
 *			NOT set, then the appropriate thing to do is to
 *			wait around (poll) until this bit is set.
 *	SCSI_REQUEST	Set by controller to start byte passing handshake.
 *	SCSI_MESSAGE	Set by a controller during message phase.
 *	SCSI_COMMAND	Set during the command, status, and messages phase.
 *	SCSI_INPUT	If set means data (or commandStatus) set by device.
 *	SCSI_PARITY	Used to test the parity checking hardware.
 *	SCSI_BUSY	Set by controller after it has been selected.
 *  The following bits can be set by the CPU.
 *	SCSI_SELECT	Set by the host when it want to select a controller.
 *	SCSI_RESET	Set by the host when it want to reset the SCSI bus.
 *	SCSI_PARITY_ENABLE	Enable parity checking on transfer
 *	SCSI_WORD_MODE		Send 2 bytes at a time
 *	SCSI_DMA_ENABLE		Do DMA, always used.
 *	SCSI_INTERRUPT_ENABLE	Interrupt upon completion.
 */
#define SCSI_PARITY_ERROR		0x8000
#define SCSI_BUS_ERROR			0x4000
#define SCSI_ODD_LENGTH			0x2000
#define SCSI_INTERRUPT_REQUEST		0x1000
#define SCSI_REQUEST			0x0800
#define SCSI_MESSAGE			0x0400
#define SCSI_COMMAND			0x0200
#define SCSI_INPUT			0x0100
#define SCSI_PARITY			0x0080
#define SCSI_BUSY			0x0040
#define SCSI_SELECT			0x0020
#define SCSI_RESET			0x0010
#define SCSI_PARITY_ENABLE		0x0008
#define SCSI_WORD_MODE			0x0004
#define SCSI_DMA_ENABLE			0x0002
#define SCSI_INTERRUPT_ENABLE		0x0001

/* Forward declaration. */
typedef struct Controller Controller;

/*
 * Device - The data structure containing information about a device. One of
 * these structure is kept for each attached device. Note that is structure
 * is casted into a ScsiDevice and returned to higher level software.
 * This implies that the ScsiDevice must be the first field in this
 * structure.
 */

typedef struct Device {
    ScsiDevice handle;	/* Scsi Device handle. This is the only part
			 * of this structure visible to higher 
			 * level software. MUST BE FIRST FIELD IN STRUCTURE.
			 */
    int	targetID;	/* SCSI Target ID of this device. Note that
			 * the LUN is store in the device handle.
			 */
    Controller *ctrlPtr;	/* Controller to which device is attached. */
		   /*
		    * The following part of this structure is 
		    * used to handle SCSI commands that return 
		    * CHECK status. To handle the REQUEST SENSE
		    * command we must: 1) Save the state of the current
		    * command into the "struct FrozenCommand". 2) Submit
		    * a request sense command formatted in SenseCmd
		    * to the device.
		    */
    struct FrozenCommand {		       
	ScsiCmd	*scsiCmdPtr;	  /* The frozen command. */
	unsigned char statusByte; /* It's SCSI status byte, Will always have
				   * the check bit set.  */
	int amountTransferred;    /* Number of bytes transferred by this 
				   * command.  */
    } frozen;	
    char senseBuffer[DEV_MAX_SENSE_BYTES]; /* Data buffer for request sense */
    ScsiCmd		SenseCmd;  	   /* Request sense command buffer. */
} Device;

/*
 * Controller - The Data structure describing a sun SCSI0 controller. One
 * of these structures exists for each active SCSI0 HBA on the system. Each
 * controller may have from zero to 56 (7 targets each with 8 logical units)
 * devices attached to it. 
 */
struct Controller {
    volatile CtrlRegs *regsPtr;	/* Pointer to the registers
                                    of this controller. */
    int	    dmaState;	/* DMA state for this controller, defined below. */
    char    *name;	/* String for error message for this controller.  */
    DevCtrlQueues devQueues;    /* Device queues for devices attached to this
				 * controller.	 */
    Sync_Semaphore mutex; /* Lock protecting controller's data structures. */
    Device     *devPtr;	   /* Current active command. */
    ScsiCmd   *scsiCmdPtr; /* Current active command. */
    Address   dmaBuffer;  /* dma buffer allocated for request. */
    Device  *devicePtr[8][8]; /* Pointers to the device
                               * attached to the
			       * controller index by [targetID][LUN].
			       * NIL if device not attached yet. Zero if
			       * device conflicts with HBA address.  */

};

/* 
 * SCSI_WAIT_LENGTH - the number of microseconds that the host waits for
 *	various control lines to be set on the SCSI bus.  The largest wait
 *	time is when a controller is being selected.  This delay is
 *	called the Bus Abort delay and is about 250 milliseconds.
 */
#define SCSI_WAIT_LENGTH		250000

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
 * Test, mark, and unmark the controller as busy.
 */
#define	IS_CTRL_BUSY(ctrlPtr)	((ctrlPtr)->scsiCmdPtr != (ScsiCmd *) NIL)
#define	SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr) \
		((ctrlPtr)->scsiCmdPtr = (scsiCmdPtr))
#define	SET_CTRL_FREE(ctrlPtr)	((ctrlPtr)->scsiCmdPtr = (ScsiCmd *) NIL)

/*
 * MAX_SCSI0_CTRLS - Maximum number of SCSI0 controllers attached to the
 *		     system. We set this to the maximum number of VME slots
 *		     in any Sun2 system currently available.
 */
#define	MAX_SCSI0_CTRLS	4
static Controller *Controllers[MAX_SCSI0_CTRLS];
/*
 * Highest number controller we have probed for.
 */
static int numSCSI0Controllers = 0;

int devSCSI0Debug = 0;

static void RequestDone();
static ReturnStatus Wait();


/*
 *----------------------------------------------------------------------
 *
 * Probe --
 *
 *	Probe memory for the old-style VME SCSI interface.  We rely
 *	on the fact that this occupies 4K of address space. 
 *
 * Results:
 *	TRUE if the host adaptor was found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
Probe(address)
    int address;			/* Alledged controller address */
{
    ReturnStatus	status;
    register volatile CtrlRegs *regsPtr = (CtrlRegs *)address;
    short value;

    /*
     * Touch the device. If it exists it occupies 4K.
     */
    value = 0x4BCC;
    status = Mach_Probe(sizeof(regsPtr->dmaCount), (char *) &value,
			(char *) &(regsPtr->dmaCount));
    if (status == SUCCESS) {
	value = 0x5BCC;
	status = Mach_Probe(sizeof(regsPtr->dmaCount),(char *) &value,
			  ((char *) &(regsPtr->dmaCount)) + 0x800);
    }
    return(status == SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Reset --
 *
 *	Reset a SCSI bus controlled by the orignial Sun Host Adaptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the controller.
 *
 *----------------------------------------------------------------------
 */
static void
Reset(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;

    regsPtr->control = SCSI_RESET;
    MACH_DELAY(100);
    regsPtr->control = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Send a command to a controller on the old-style SCSI Host Adaptor
 *      indicated by devPtr.  
 *
 *	Note: the ID of the controller is never placed on the bus
 *	(contrary to standard protocol, but necessary for the early Sun
 *	SCSI interface).
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
    Device	*devPtr;		/* Device to sent to. */
    ScsiCmd	*scsiCmdPtr;		/* Command to send. */
{
    register ReturnStatus status;
    register volatile CtrlRegs *regsPtr;/* Host Adaptor registers */
    char *charPtr;			/* Used to put the control block
					 * into the commandStatus register */
    int bits = 0;			/* variable bits to OR into control */
    int targetID;			/* Id of the SCSI device to select */
    int size;				/* Number of bytes to transfer */
    Address addr;			/* Kernel address of transfer */
    Controller	*ctrlPtr;		/* HBA of device. */
    int	i;

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr);
    ctrlPtr->dmaBuffer = (Address) NIL;
    ctrlPtr->devPtr = devPtr;
    size = scsiCmdPtr->bufferLen;
    addr = scsiCmdPtr->buffer;
    targetID = devPtr->targetID;
    regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    if (size == 0) {
	ctrlPtr->dmaState = DMA_INACTIVE;
    } else {
	ctrlPtr->dmaState = (scsiCmdPtr->dataToDevice) ? DMA_SEND :
							 DMA_RECEIVE;
    }
   /*
     * Check against a continuously busy bus.  This stupid condition would
     * fool the code below that tries to select a device.
     */
    for (i=0 ; i < SCSI_WAIT_LENGTH ; i++) {
	if ((regsPtr->control & SCSI_BUSY) == 0) {
	    break;
	} else {
	    MACH_DELAY(10);
	}
    }
    if (i == SCSI_WAIT_LENGTH) {
	Reset(ctrlPtr);
	printf("Warning: %s SCSI bus stuck busy\n", ctrlPtr->name);
	return(FAILURE);
    }
    /*
     * Select the device.  Sun's SCSI Programmer's Manual recommends
     * resetting the SCSI_WORD_MODE bit so that the byte packing hardware
     * is reset and the data byte that has the target ID gets transfered
     * correctly.  After this, the target's ID is put in the data register,
     * the SELECT bit is set, and we wait until the device responds
     * by setting the BUSY bit.  The ID bit of the host adaptor is not
     * put in the data word because of problems with Sun's Host Adaptor.
     */
    regsPtr->control = 0;
    regsPtr->data = (1 << targetID);
    regsPtr->control = SCSI_SELECT;
    status = Wait(ctrlPtr, SCSI_BUSY, FALSE);
    if (status != SUCCESS) {
	regsPtr->data = 0;
	regsPtr->control = 0;
        printf("Warning: %s: can't select device at %s\n", 
				 ctrlPtr->name, devPtr->handle.locationName);
	return(status);
    }
    /*
     * Set up the interface's registers for the transfer.  The DMA address
     * is relative to the multibus memory so the kernel's base address
     * for multibus memory is subtracted from 'addr'. The host adaptor
     * increments the dmaCount register until it reaches -1, hence the
     * funny initialization. See page 4 of Sun's SCSI Prog. Manual.
     */
    if (ctrlPtr->dmaState != DMA_INACTIVE) {
	ctrlPtr->dmaBuffer = addr = VmMach_DMAAlloc(size,scsiCmdPtr->buffer);
    }
    if (addr == (Address) NIL) {
	panic("%s can't allocate DMA buffer of %d bytes\n", 
			devPtr->handle.locationName, size);
    }
    regsPtr->dmaAddress = (int)(addr - VMMACH_DMA_START_ADDR);
    regsPtr->dmaCount = -size - 1;
    bits = SCSI_WORD_MODE | SCSI_DMA_ENABLE | SCSI_INTERRUPT_ENABLE;
    regsPtr->control = bits;

    /*
     * Stuff the control block through the commandStatus register.
     * The handshake on the SCSI bus is visible here:  we have to
     * wait for the Request line on the SCSI bus to be raised before
     * we can send the next command byte to the controller.  All commands
     * are of "group 0" which means they are 6 bytes long.
     */
    charPtr = scsiCmdPtr->commandBlock;
    for (i=0 ; i < scsiCmdPtr->commandBlockLen ; i++) {
	status = Wait(ctrlPtr, SCSI_REQUEST, TRUE);
	if (status != SUCCESS) {
	    printf("Warning: %s couldn't send command block byte %d\n",
				 ctrlPtr->name, i);
	    return(status);
	}
	/*
	 * The device keeps the Control/Data line set while it
	 * is accepting control block bytes.
	 */
	if ((regsPtr->control & SCSI_COMMAND) == 0) {
	    Reset(ctrlPtr);
	    printf("Warning: %s: device %s dropped command line\n",
				ctrlPtr->name, devPtr->handle.locationName);
	    return(DEV_HANDSHAKE_ERROR);
	}
        regsPtr->commandStatus = *charPtr;
	charPtr++;
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * GetStatusByte --
 *
 *	Complete an SCSI command by getting the status bytes from
 *	the device and waiting for the ``command complete''
 *	message that follows the status bytes.  If the command has
 *	additional ``sense data'' then this routine issues the
 *	SCSI_REQUEST_SENSE command to get the sense data.
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
GetStatusByte(ctrlPtr, statusBytePtr)
    Controller *ctrlPtr;
    unsigned char *statusBytePtr;
{
    register ReturnStatus status;
    register volatile CtrlRegs *regsPtr;
    short message;
    char statusByte;
    int numStatusBytes = 0;

    regsPtr = ctrlPtr->regsPtr;
    *statusBytePtr = 0;
    for (;;) {
	/*
	 * Could probably wait either on the INTERUPT_REQUEST bit or the
	 * REQUEST bit.  Reading the byte out of the commandStatus
	 * register acknowledges the REQUEST and clears these bits.  Here
	 * we grab bytes until the MESSAGE bit indicates that all the
	 * status bytes have been received and that the byte in the
	 * commandStatus register is the message byte.
	 */
	status = Wait(ctrlPtr, SCSI_REQUEST, TRUE);
	if (status != SUCCESS) {
	    printf("Warning: %s: wait error after %d status bytes\n",
				 ctrlPtr->name, numStatusBytes);
	    break;
	}
	if (regsPtr->control & SCSI_MESSAGE) {
	    message = regsPtr->commandStatus & 0xff;
	    if (message != SCSI_COMMAND_COMPLETE) {
		printf("Warning %s: Unexpected message 0x%x\n",
				     ctrlPtr->name, message);
	    }
	    break;
	} else {
	    /*
	     * This is another status byte.  Place the first status
	     * bytes into the status block.
	     */
	    statusByte = regsPtr->commandStatus;
	    if (numStatusBytes < 1) {
		*statusBytePtr = statusByte;
	    }
	    numStatusBytes++;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Wait --
 *
 *	Wait for a condition in the SCSI controller.
 *
 * Results:
 *	SUCCESS if the condition occurred before a threashold time limit,
 *	DEV_TIMEOUT otherwise.
 *
 * Side effects:
 *	This resets the SCSI bus if the reset parameter is true and
 *	the condition bits are not set by the controller before timeout..
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
Wait(ctrlPtr, condition, reset)
    Controller *ctrlPtr;
    int condition;
    Boolean reset;
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register int control;

    for (i=0 ; i < SCSI_WAIT_LENGTH ; i++) {
	control = regsPtr->control;
	if (devSCSI0Debug && i < 5) {
	    printf("%d/%x ", i, control);
	}
	if (control & condition) {
	    return(SUCCESS);
	}
	if (control & SCSI_BUS_ERROR) {
	    printf("Warning: %s : SCSI bus error\n",ctrlPtr->name);
	    status = DEV_DMA_FAULT;
	    break;
	} else if (control & SCSI_PARITY_ERROR) {
	    printf("Warning: %s: parity error\n",ctrlPtr->name);
	    status = DEV_DMA_FAULT;
	    break;
	}
	MACH_DELAY(10);
    }
    if (devSCSI0Debug) {
	printf("DevSCSI0Wait: timed out, control = %x.\n", control);
    }
    if (reset) {
	Reset(ctrlPtr);
    }
    return(status);
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
 *	NOTE: This routine is also called from DevSCSI0Intr to start the
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
   List_Links *newRequestPtr;	/* The new SCSI request. */
{
    register Device *devPtr; 
    register Controller *ctrlPtr;
    register ScsiCmd	*scsiCmdPtr;
    ReturnStatus	status;

    devPtr = (Device *) clientData;
    ctrlPtr = devPtr->ctrlPtr;
    /*
     * If we are busy (have an active request) just return. Otherwise 
     * start the request.
     */

    if (IS_CTRL_BUSY(ctrlPtr)) { 
	return FALSE;
    }
again:
    scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    devPtr = (Device *) clientData;
    status = SendCommand( devPtr, scsiCmdPtr);
    /*	
     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
    }
    if (!IS_CTRL_BUSY(ctrlPtr)) { 
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

static void
RequestDone(devPtr,scsiCmdPtr,status,scsiStatusByte,amountTransferred)
    Device	*devPtr;	/* Device for request. */
    ScsiCmd	*scsiCmdPtr;	/* Request that finished. */
    ReturnStatus status;	/* Status returned. */
    unsigned char scsiStatusByte;	/* SCSI Status Byte. */
    int		amountTransferred; /* Amount transferred by command. */
{
    ReturnStatus	senseStatus;
    Controller	        *ctrlPtr = devPtr->ctrlPtr;


    if (devSCSI0Debug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
    if (ctrlPtr->dmaState != DMA_INACTIVE) {
	VmMach_DMAFree(scsiCmdPtr->bufferLen,ctrlPtr->dmaBuffer);
	ctrlPtr->dmaState = DMA_INACTIVE;
    }
    /*
     * First check to see if this is the reponse of a HBA generated 
     * REQUEST SENSE command.  If this is the case, we can process
     * the callback of the frozen command for this device and
     * allow the flow of command to the device to be resummed.
     */
    if (scsiCmdPtr->doneProc == SpecialSenseProc) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
			SUCCESS,
			devPtr->frozen.statusByte, 
			devPtr->frozen.amountTransferred,
			amountTransferred,
			devPtr->senseBuffer);
	 MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_CTRL_FREE(ctrlPtr);
	 return;
    }
    /*
     * This must be a outside request finishing. If the request 
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) || !SCSI_CHECK_STATUS(scsiStatusByte)) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	 MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_CTRL_FREE(ctrlPtr);
	 return;
   } 
   /*
    * If we got here than the SCSI command came back from the device
    * with the CHECK bit set in the status byte.
    * Need to perform a REQUEST SENSE. Move the current request 
    * into the frozen state and issue a REQUEST SENSE. 
    */
   devPtr->frozen.scsiCmdPtr = scsiCmdPtr;
   devPtr->frozen.statusByte = scsiStatusByte;
   devPtr->frozen.amountTransferred = amountTransferred;
   DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES, 
		   devPtr->senseBuffer, &(devPtr->SenseCmd));
   devPtr->SenseCmd.doneProc = SpecialSenseProc,
   senseStatus = SendCommand(devPtr, &(devPtr->SenseCmd));
   /*
    * If we got an HBA error on the REQUEST SENSE we end the outside 
    * command with the SUCCESS status but zero sense bytes returned.
    */
   if (senseStatus != SUCCESS) {
        MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	MASTER_LOCK(&(ctrlPtr->mutex));
	SET_CTRL_FREE(ctrlPtr);
   }

}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Intr --
 *
 *	Handle interrupts from the SCSI controller.  This has to poll
 *	through the possible SCSI controllers to find the one generating
 *	the interrupt.  The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
 *
 * Results:
 *	TRUE if the SCSI controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevSCSI0Intr(clientDataArg)
    ClientData	clientDataArg;
{
    register Controller *ctrlPtr;
    Device	*devPtr;
    volatile CtrlRegs *regsPtr;
    int		residual;
    ReturnStatus	status;
    List_Links	*newRequestPtr;
    unsigned char statusByte;
    ClientData	clientData;

    ctrlPtr = (Controller *) clientDataArg;
    regsPtr = ctrlPtr->regsPtr;
    devPtr = ctrlPtr->devPtr;
    MASTER_LOCK(&(ctrlPtr->mutex));
    if (regsPtr->control & SCSI_INTERRUPT_REQUEST) {
	if (regsPtr->control & SCSI_BUS_ERROR) {
	    if (regsPtr->dmaCount >= 0) {
		/*
		 * A DMA overrun.  Unlikely with a disk but could
		 * happen while reading a large tape block.  Consider
		 * the I/O complete with no residual bytes
		 * un-transferred.
		residual = 0;
	    } else {
		/*
		 * A real Bus Error.  Complete the I/O but flag an error.
		 * The residual is computed because the Bus Error could
		 * have occurred after a number of sectors.
		 */
		residual = -regsPtr->dmaCount -1;
	    }
	    /*
	     * The board needs to be reset to clear the Bus Error
	     * condition so no status bytes are grabbed.
	     */
	    Reset(ctrlPtr);
	    status = DEV_DMA_FAULT;
	    RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, 0,
			ctrlPtr->scsiCmdPtr->bufferLen - residual);
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
		if (newRequestPtr != (List_Links *) NIL) { 
		    (void) entryAvailProc(clientData,newRequestPtr);
		}
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	} else {
	    /*
	     * Normal command completion.  Compute the residual,
	     * the number of bytes not transferred, check for
	     * odd transfer sizes, and finally get the completion
	     * status from the device.
	     */
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		printf("Warning: Spurious interrupt from SCSI0\n");
		Reset(ctrlPtr);
		MASTER_UNLOCK(&(ctrlPtr->mutex));
		return(TRUE);
	    }
	    residual = -regsPtr->dmaCount -1;
	    if (regsPtr->control & SCSI_ODD_LENGTH) {
		/*
		 * On a read the last odd byte is left in the data
		 * register.  On both reads and writes the number
		 * of bytes transferred as determined from dmaCount
		 * is off by one.  See Page 8 of Sun's SCSI
		 * Programmers' Manual.
		 */
		if (!ctrlPtr->scsiCmdPtr->dataToDevice) {
		  *(volatile char *)(DEV_MULTIBUS_BASE + regsPtr->dmaAddress) =
			regsPtr->data;
		    residual--;
		} else {
		    residual++;
		}
	    }
	    status = GetStatusByte(ctrlPtr,&statusByte);
	    RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, 
			statusByte,
			ctrlPtr->scsiCmdPtr->bufferLen - residual);
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
		if (newRequestPtr != (List_Links *) NIL) { 
		    (void) entryAvailProc(clientData,newRequestPtr);
		}
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	}
    }
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return (FALSE);
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
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Init --
 *
 *	Check for the existant of the Sun SCSI0 HBA controller. If it
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
DevSCSI0Init(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int	ctrlNum;
    Boolean	found;
    Controller *ctrlPtr;
    int	i,j;

    /*
     * See if the controller is there. 
     */
    ctrlNum = ctrlLocPtr->controllerID;
    found =  Probe(ctrlLocPtr->address);
    if (!found) {
	return DEV_NO_CONTROLLER;
    }
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    if (ctrlNum+1 > numSCSI0Controllers) {
	numSCSI0Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (ctrlLocPtr->address);
    ctrlPtr->regsPtr->intrVector = ctrlLocPtr->vectorNumber;
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
    Controllers[ctrlNum] = ctrlPtr;
    Reset(ctrlPtr);
    return (ClientData) ctrlPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0ttachDevice --
 *
 *	Attach a SCSI device using the Sun SCSI0 HBA. 
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
DevSCSI0AttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	 /* Device to attach. */
    void	(*insertProc)(); /* Queue insert procedure. */
{
    Device *devPtr;
    Controller	*ctrlPtr;
    char   tmpBuffer[512];
    int	   length;
    int	   ctrlNum;
    int	   targetID, lun;

    /*
     * First find the SCSI0 controller this device is on.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr);
    if ((ctrlNum > MAX_SCSI0_CTRLS) ||
	(Controllers[ctrlNum] == (Controller *) 0)) { 
	return (ScsiDevice  *) NIL;
    } 
    ctrlPtr = Controllers[ctrlNum];
    targetID = SCSI_TARGET_ID(devicePtr);
    lun = SCSI_LUN(devicePtr);
    /*
     * Allocate a device structure for the device and fill in the
     * handle part. This must be created before we grap the MASTER_LOCK.
     */
    devPtr = (Device *) malloc(sizeof(Device)); 
    bzero((char *) devPtr, sizeof(Device));
    devPtr->handle.devQueue = Dev_QueueCreate(ctrlPtr->devQueues,
				1, insertProc, (ClientData) devPtr);
    devPtr->handle.locationName = "Unknown";
    devPtr->handle.LUN = lun;
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.maxTransferSize = 63*1024;
    /*
     * See if the device is already present.
     */
    MASTER_LOCK(&(ctrlPtr->mutex));
    /*
     * A device pointer of zero means that targetID/LUN 
     * conflicts with that of the HBA. A NIL means the
     * device hasn't been attached yet.
     */
    if (ctrlPtr->devicePtr[targetID][lun] == (Device *) 0) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(void) Dev_QueueDestroy(devPtr->handle.devQueue);
	free((char *) devPtr);
	return (ScsiDevice *) NIL;
    }
    if (ctrlPtr->devicePtr[targetID][lun] != (Device *) NIL) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(void) Dev_QueueDestroy(devPtr->handle.devQueue);
	free((char *) devPtr);
	return (ScsiDevice *) (ctrlPtr->devicePtr[targetID][lun]);
    }

    ctrlPtr->devicePtr[targetID][lun] = devPtr;
    devPtr->targetID = targetID;
    devPtr->ctrlPtr = ctrlPtr;
    MASTER_UNLOCK(&(ctrlPtr->mutex));

    (void) sprintf(tmpBuffer, "%s#%d Target %d LUN %d", ctrlPtr->name, ctrlNum,
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    return (ScsiDevice *) devPtr;
}

