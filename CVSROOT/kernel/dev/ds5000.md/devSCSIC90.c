/* 
 * devSCSIC90.c --
 *
 *	Routines specific to the SCSI NCR 53C9X Host Adaptor.  This adaptor is
 *	based on the NCR 53C90 chip.
 *	The 53C90 supports connect/dis-connect.
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
#endif /* not lint */

#include "sprite.h"
#include "scsiC90.h"
#include "mach.h"
#include "dev.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bstring.h"
#include "devSCSIC90.h"
#include "devSCSIC90Int.h"

/*
 * Forward declarations.  
 */

static void		PrintMsg _ARGS_ ((unsigned int msg));
static void		PrintPhase _ARGS_ ((unsigned int phase));
static void		PrintLastPhase _ARGS_ ((unsigned int phase));
static ReturnStatus     SendCommand _ARGS_ ((Device *devPtr,
                                             ScsiCmd *scsiCmdPtr));
static void             PrintRegs _ARGS_((volatile CtrlRegs *regsPtr));
static void             PerformCmdDone _ARGS_((Controller *ctrlPtr,
					       ReturnStatus status));
static ReturnStatus     PerformSelect _ARGS_((Controller *ctrlPtr,
					      unsigned int interruptReg,
					      unsigned int sequenceReg));
static ReturnStatus     PerformDataXfer _ARGS_((Controller *ctrlPtr,
					      unsigned int interruptReg,
					      unsigned int statusReg));
static ReturnStatus     PerformStatus _ARGS_((Controller *ctrlPtr,
					      unsigned int interruptReg));
static ReturnStatus     PerformMsgIn _ARGS_(( Controller *ctrlPtr));
static ReturnStatus     PerformReselect _ARGS_(( Controller *ctrlPtr,
					      unsigned int interruptReg));
static ReturnStatus     PerformExtendedMsgIn _ARGS_(( Controller *ctrlPtr,
					      unsigned int message));
static int              SpecialSenseProc _ARGS_((void));
static void             PutCircBuf _ARGS_((int type, char *object));

/*
 * devSCSIC90Debug - debugging level
 *	2 - normal level
 *	4 - one print per command in the normal case
 *	5 - traces interrupts
 */
int devSCSIC90Debug = 2;
Controller *Controllers[MAX_SCSIC90_CTRLS];

char        circBuf[CIRCBUFLEN] = {""};
int         circHead = 0;
static int         lastDiscon[8] = {
    -1,-1,-1,-1,-1,-1,-1,-1
    };
static int         lastNegot[8] = {
    -1,-1,-1,-1,-1,-1,-1,-1
    };
static int         lastReject[8] = {
    -1,-1,-1,-1,-1,-1,-1,-1
    };
static int         lastConflict[8] = {
    -1,-1,-1,-1,-1,-1,-1,-1
    };
static int         lastSense[8] = {
    -1,-1,-1,-1,-1,-1,-1,-1
    };
char numTab[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
     


/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Send a command to a SCSI controller.
 *	NOTE: The caller is assumed to have the master lock of the controller
 *	to which the device is attached held.
 *      
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
    volatile CtrlRegs	*regsPtr; /* Host Adaptor registers */
    char	*charPtr;
    Controller	*ctrlPtr;
    int i;
    int size;				/* Number of bytes to transfer */
    char tempChar;

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    regsPtr = ctrlPtr->regsPtr;

    if (ctrlPtr->devPtr != (Device *)NIL) {
	panic("SCSIC90Command: can't send: host is busy\n");
    }

    SET_CTRL_BUSY(ctrlPtr,devPtr);
    SET_DEV_BUSY(devPtr, scsiCmdPtr);

    devPtr->savedDataPtr = scsiCmdPtr->buffer;
    devPtr->savedDataLen = scsiCmdPtr->bufferLen;
    devPtr->scsiCmdPtr   = scsiCmdPtr;
    size = scsiCmdPtr->bufferLen;
    if (size == 0) {
	devPtr->dmaState = DMA_INACTIVE;
    } else {
	devPtr->dmaState = (scsiCmdPtr->dataToDevice) ? DMA_SEND :
							DMA_RECEIVE;
    }

    PUTCIRCBUF(CSTR, "send: targ ");
    PUTCIRCBUF(CBYTE, (char *)devPtr->targetID);
    PUTCIRCBUF(CSTR, ";cmd ");
    PUTCIRCBUF(CINT, (char *)scsiCmdPtr);
    PUTCIRCBUF(CSTR,";buf ");
    PUTCIRCBUF(CINT, (char *)scsiCmdPtr->buffer);
    PUTCIRCBUF(CSTR, ";len ");
    PUTCIRCBUF(CINT, (char *)size);
    PUTCIRCBUF(CSTR,";op ");
    PUTCIRCBUF(CBYTE, (char *)scsiCmdPtr->commandBlock[0]);
    PUTCIRCNULL;

    /*
     * SCSI SELECTION.
     */

    /*
     * Set phase to selection and command phase so that we know what's happened
     * in the next phase.
     */
    devPtr->lastPhase = PHASE_SELECTION;	/* Selection & command phase.*/
    devPtr->residual = size;			/* No bytes transfered yet. */
    devPtr->commandStatus = 0;			/* No status yet. */
    /* Load select/reselect bus ID register with target ID. */
    regsPtr->scsi_ctrl.write.destID = devPtr->targetID;
    /* Load select/reselect timeout period. */
    regsPtr->scsi_ctrl.write.timeout = SELECT_TIMEOUT;
    /* Zero value for asynchronous transfer. */
    regsPtr->scsi_ctrl.write.synchOffset = devPtr->synchOffset;
    if (devPtr->synchOffset != 0) {
	regsPtr->scsi_ctrl.write.synchPer = devPtr->synchPeriod;
    }
    /* Set the clock conversion register. */
    regsPtr->scsi_ctrl.write.clockConv = CLOCKCONV;

    EMPTY_BUFFER();

    /* 
     * There are 3 selection possibilities:
     * 1) Without Attention (NATN) which says we don't do disconnect/reselect.
     *    No need to use this anymore.
     * 2) With Attention (ATN) which goes through arbitration, selection
     *    and command phases, announcing that we do disconnect/reselect.
     *    The usual mode.
     * 3) With attention and stop (ATNS) which just goes through 
     *    arbitration and selection phases and stops.  This gives
     *    us a chance to send another message after the IDENTIFY msg
     *    before sending the command.  For use in sending the 1st
     *    message of the synchronous data xfer negotiation.
     */

    if (scsiCmdPtr->commandBlockLen != 6 &&
	scsiCmdPtr->commandBlockLen != 10 &&
	scsiCmdPtr->commandBlockLen != 12) {
	printf("%s: Command is wrong length.\n");
	PUTCIRCBUF(CSTR,"send: bad cmd len\n");
	PUTCIRCNULL;
	return DEV_INVALID_ARG; 	/* Is this the correct error? */
    }


    regsPtr->scsi_ctrl.write.FIFO = SCSI_DIS_REC_IDENTIFY | devPtr->handle.LUN;

    if ((devPtr->synchPeriod < MIN_SYNCH_PERIOD) &&
	(devPtr->msgFlag & ENABLEEXTENDEDMSG)) {
	devPtr->msgFlag |= REQEXTENDEDMSG;
	lastNegot[devPtr->targetID] = circHead;
	EMPTY_BUFFER();
	regsPtr->scsi_ctrl.write.command = CR_SLCT_ATNS;
	PUTCIRCBUF(CSTR,"send ATNS; per ");
    } else {
	/* Load FIFO with 6, 10, or 12 byte scsi command. */
	charPtr = scsiCmdPtr->commandBlock;
	for (i = 0; i < scsiCmdPtr->commandBlockLen; i++) {
	    regsPtr->scsi_ctrl.write.FIFO = *charPtr;
	    charPtr++;
	}
	PUTCIRCBUF(CSTR,"send ATN; per ");
	EMPTY_BUFFER();
	regsPtr->scsi_ctrl.write.command = CR_SLCT_ATN;
    }
    PUTCIRCBUF(CBYTE,(char *)(devPtr->synchPeriod));
    PUTCIRCBUF(CSTR,"; off ");
    PUTCIRCBUF(CBYTE,(char *)(devPtr->synchOffset));
    PUTCIRCBUF(CSTR,"; cmd ");
    PUTCIRCBUF(CBYTE,(char *)(*scsiCmdPtr->commandBlock));
    PUTCIRCNULL;

    return SUCCESS;
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

    SET_CTRL_FREE(ctrlPtr);
    PUTCIRCBUF(CSTR,"done: targ ");
    PUTCIRCBUF(CBYTE, (char *)devPtr->targetID);
    PUTCIRCBUF(CSTR,";rc ");
    PUTCIRCBUF(CBYTE, (char *)status);
    PUTCIRCBUF(CSTR,";stat ");
    PUTCIRCBUF(CBYTE, (char *)scsiStatusByte);
    PUTCIRCBUF(CSTR,";cnt ");
    PUTCIRCBUF(CBYTE, (char *)amountTransferred);
    PUTCIRCNULL;


    /*
     * First check to see if this is the reponse of a HBA-driver generated 
     * REQUEST SENSE command.  If this is the case, we can process
     * the callback of the frozen command for this device and
     * allow the flow of command to the device to be resummed.
     */
    if (scsiCmdPtr->doneProc == SpecialSenseProc) {
        MASTER_UNLOCK(&(ctrlPtr->mutex));
	PUTCIRCBUF(CSTR,"sense data:");
	for (i=0; i<amountTransferred; i++) {
	    circBuf[circHead] = ' ';
	    circHead = (circHead + 1) % CIRCBUFLEN;
	    PUTCIRCBUF(CBYTE, (char *)(devPtr->senseBuffer[i]));
	}
	PUTCIRCNULL;
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
		SUCCESS,
		devPtr->frozen.statusByte, 
		devPtr->frozen.amountTransferred,
		amountTransferred,
		devPtr->senseBuffer);
         MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_DEV_FREE(devPtr);
	 return;
    }
    /*
     * This must be an outside request finishing. If the request 
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) ||
	(scsiStatusByte != SCSI_STATUS_CHECK)) { 
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
	PUTCIRCBUF(CSTR,"done: callback before...");
			       amountTransferred, 0, (char *) 0);
	MASTER_LOCK(&(ctrlPtr->mutex));
	SET_DEV_FREE(devPtr);
	PUTCIRCBUF(CSTR,"after");
	PUTCIRCNULL;
	return;
    } 
    /*
     * If we got here than the SCSI command came back from the device
     * with the CHECK bit set in the status byte.
     * Need to perform a REQUEST SENSE.  Move the current request 
     * into the frozen state and issue a REQUEST SENSE. 
     */

    devPtr->synchPeriod = 0;
    PUTCIRCBUF(CSTR,"done: issue sense");
    PUTCIRCNULL;
    lastSense[devPtr->targetID] = circHead;
    devPtr->synchOffset = 0;
    devPtr->frozen.scsiCmdPtr = scsiCmdPtr;
    devPtr->frozen.statusByte = scsiStatusByte;
    devPtr->frozen.amountTransferred = amountTransferred;
    DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES, 
		    devPtr->senseBuffer, &(devPtr->SenseCmd));
    devPtr->SenseCmd.doneProc = SpecialSenseProc;
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
	SET_DEV_FREE(devPtr);
    }

}

/*
 *----------------------------------------------------------------------
 *
 * DevEntryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for this
 *	controller. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for this controller. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *	NOTE: This routine is also called from DevSCSIC90Intr to start the
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

Boolean
DevEntryAvailProc(clientData, newRequestPtr) 
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links *newRequestPtr;	/* The new SCSI request. */
{
    Device		*devPtr; 
    Controller 		*ctrlPtr;
    ScsiCmd		*scsiCmdPtr = (ScsiCmd *)newRequestPtr;
    ReturnStatus	status;

    devPtr = (Device *) clientData;
    ctrlPtr = devPtr->ctrlPtr;
    /*
     * If we are busy (have an active request) just return. Otherwise 
     * start the request.
     */
    
    if ((!IS_CTRL_FREE(ctrlPtr)) ||  (!IS_DEV_FREE(devPtr))) {
    if ((IS_CTRL_FREE(ctrlPtr)) &&  (IS_DEV_FREE(devPtr))) {
	PUTCIRCBUF(CSTR,"EAP: exec targ ");
	PUTCIRCBUF(CBYTE, (char *)(devPtr->targetID));
	PUTCIRCBUF(CSTR,"; mask ");
	PUTCIRCBUF(CBYTE, (char *)(ctrlPtr->devQueuesMask));
	PUTCIRCBUF(CSTR,"; cmd ptr ");
	PUTCIRCBUF(CINT, (char *)newRequestPtr);
	PUTCIRCNULL;
    } else {
	PUTCIRCBUF(CSTR,"EAP: NQ targ ");
	PUTCIRCBUF(CBYTE, (char *)(devPtr->targetID));
	PUTCIRCBUF(CSTR,"; mask ");
	PUTCIRCBUF(CBYTE, (char *)(ctrlPtr->devQueuesMask));
	PUTCIRCBUF(CSTR,"; cmd ptr ");
	PUTCIRCBUF(CINT,(char *)newRequestPtr);
	PUTCIRCBUF(CSTR,"; intDev ");
	PUTCIRCBUF(CINT,(char *)(ctrlPtr->interruptDevPtr));
	if (ctrlPtr->interruptDevPtr != (Device *)NIL) {
	    PUTCIRCBUF(CSTR,"; intDevCmd ");
	    PUTCIRCBUF(CINT,(char *)(ctrlPtr->interruptDevPtr->scsiCmdPtr));
	}
	PUTCIRCNULL;
    }

again:
    scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    devPtr = (Device *) clientData;
    status = SendCommand(devPtr, scsiCmdPtr);

    /*	
     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
	 PUTCIRCBUF(CSTR,"eap: send fail");
	 PUTCIRCNULL;
	 newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
						 ctrlPtr->devQueuesMask,
						 &clientData);
	if (newRequestPtr != (List_Links *) NIL) { 
	    goto again;
	    PUTCIRCBUF(CSTR,"eap: no cmd");
	    PUTCIRCNULL;
	}
	} else {
	    PUTCIRCBUF(CSTR,"eap: cmd");
	    PUTCIRCBUF(CINT,(char *)newRequestPtr);
	    PUTCIRCNULL;
    }

    return TRUE;

}   


/*
 *----------------------------------------------------------------------
 *
 * DevSCSIC90Intr --
 *
 * Handle interrupts from the SCSI controller.
 *
 * Results:
 *	TRUE if an SCSIC90 controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *      Extreme headaches from trying to follow this absurd code.
 * 
 * Note:
 *      Cannot use printf for debugging in this routine since 
 *      printf re-enables interrupts.
 *
 *----------------------------------------------------------------------
 */
Boolean 
DevSCSIC90Intr(clientDataArg)
    ClientData	clientDataArg;
{
    Controller		*ctrlPtr;
    volatile CtrlRegs	*regsPtr;
    Device		*devPtr;
    unsigned char	phase;
    ReturnStatus	status = SUCCESS;
    unsigned char	interruptReg;
    unsigned char	statusReg;
    unsigned char	sequenceReg;
    int                 i;
    char	        *charPtr;

    char	        tempChar;
    ctrlPtr = (Controller *) clientDataArg;
    regsPtr = ctrlPtr->regsPtr;
    devPtr = ctrlPtr->devPtr;

    MASTER_LOCK(&(ctrlPtr->mutex));

    /* Read registers.
     * reading the interrupt register clears the status and sequence
     * registers so it must be read last.
     */
    statusReg = regsPtr->scsi_ctrl.read.status;
    sequenceReg = regsPtr->scsi_ctrl.read.sequence;
    interruptReg = regsPtr->scsi_ctrl.read.interrupt;
    phase = statusReg & SR_PHASE;
    sequenceReg &= SEQ_MASK;

    /* Check for errors. */
    PUTCIRCBUF(CSTR,"intr: dev ");
    PUTCIRCBUF(CINT,(char *)devPtr);
    PUTCIRCBUF(CSTR,";int ");
    PUTCIRCBUF(CBYTE, (char *)interruptReg);
    PUTCIRCBUF(CSTR,";stat ");
    PUTCIRCBUF(CBYTE, (char *)statusReg);
    PUTCIRCBUF(CSTR,";seq ");
    PUTCIRCBUF(CBYTE, (char *)sequenceReg);
    if (devPtr != (Device *)NIL) {
	PUTCIRCBUF(CSTR,";last ");
	PUTCIRCBUF(CBYTE, (char *)devPtr->lastPhase);
    }
    PUTCIRCNULL;

/*    printf("interruptReg 0x%02x, statusReg 0x%02x, sequenceReg 0x%02x\n",
		interruptReg, statusReg, sequenceReg);
*/
    if ((IS_CTRL_FREE(ctrlPtr)) && !(interruptReg & IR_RESLCT)) {
	panic("SCSIC90: Got int. but ctrl is free\n");
    }

    if (statusReg & SR_GE) {
	panic("gross error 1\n");
	printf("%s: some gross error happened.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }
    if (statusReg & SR_PE) {
	printf("%s: a parity error happened.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }

    if (interruptReg & IR_SCSI_RST) {
	printf("%s: SCSI reset detected.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }
    if (interruptReg & IR_ILL_CMD) {
	if (ctrlPtr->interruptDevPtr != (Device *)NIL) {
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    PUTCIRCBUF(CSTR,"ignoring illegal cmd interrupt");
	    PUTCIRCNULL;
	    return TRUE;
	} else {
	    printf("%s: illegal command.\n",
		   devPtr->handle.locationName);
	    status = FAILURE;
	}
    }
    if (interruptReg & IR_SLCT_ATN) {
    if (interruptReg & IR_RESLCT) {
	PerformReselect(ctrlPtr, interruptReg);
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
    }
	printf("%s: scsi controller selected with ATN which we don't allow.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }
    if (interruptReg & IR_SLCT) {
	printf("%s: scsi controller selected as target which we don't allow.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }
    if (interruptReg & IR_RESLCT) {
    /* Where did we come from? */
    switch (devPtr->lastPhase) {
    case PHASE_COMMAND:
	break;
    case PHASE_BUS_FREE:
	/* nothing happening yet. */
	break;
    case PHASE_SELECTION:
	status = PerformSelect(ctrlPtr, interruptReg, sequenceReg);
	break;
    case PHASE_DATA_IN:
#ifdef sun4c
	/* Drain remaining bytes in pack register to memory. */
	dmaRegsPtr->ctrl |= DMA_DRAIN;
#endif
	status = PerformDataXfer(ctrlPtr, interruptReg, statusReg);
	break;
    case PHASE_DATA_OUT:
	status = PerformDataXfer(ctrlPtr, interruptReg, statusReg);
	break;
    case PHASE_STATUS:
	status = PerformStatus(ctrlPtr, interruptReg);
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
	break;
    case PHASE_MSG_OUT:
	break;
    case PHASE_MSG_IN:
	status = PerformMsgIn(ctrlPtr);
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
	break;
    case PHASE_STAT_MSG_IN:
	if (!(interruptReg & IR_DISCNCT)) {
	    printf("SCSIC90: Should have seen end of I/O.\n");
	    status = FAILURE;
	}
	break;
    case PHASE_RDY_DISCON:
	if (interruptReg & IR_DISCNCT) {
	    devPtr->lastPhase = PHASE_BUS_FREE;
	    lastDiscon[devPtr->targetID] = circHead;
	    PUTCIRCBUF(CSTR,"intr: targ ");
	    PUTCIRCBUF(CBYTE, (char *)devPtr->targetID);
	    PUTCIRCBUF(CSTR," discon.");
	    PUTCIRCNULL;
	    SET_CTRL_FREE(ctrlPtr);
	    PerformCmdDone(ctrlPtr,status);
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return TRUE;
	} else {
	    printf("SCSIC90: expecting disconnect signal.\n");
	    status = FAILURE;
	}
	break;
    default:
	/* We set this field, so this shouldn't happen. */
	printf("SCSIC90Intr: We came from an unknown phase.\n");
	status = FAILURE;
	break;
    }

    if ((status != SUCCESS) || (interruptReg & IR_DISCNCT)) {
	RequestDone(devPtr,
        PUTCIRCBUF(CSTR,"intr: exit stat ");
        PUTCIRCBUF(CBYTE,(char *)status);
        PUTCIRCNULL;
		    devPtr->scsiCmdPtr,
		    status,
		    devPtr->commandStatus,
		    devPtr->scsiCmdPtr->bufferLen - devPtr->activeBufLen);
		    devPtr->scsiCmdPtr->bufferLen - devPtr->residual);
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
    }

    switch (phase) {

    case SR_DATA_OUT:
    case SR_DATA_IN:
	devPtr->lastPhase = (phase == SR_DATA_OUT ? PHASE_DATA_OUT :
			     PHASE_DATA_IN);
	/*
	 * It should be possible to do multiple blocks of DMA without
	 * returning to the higher level, if we set the max transfer size
	 * larger, but we don't handle that yet. XXX
	 */
#ifdef sun4c
	dmaControllerActive++;	/* Resetting controller not allowed. */
#endif
	DevStartDMA(ctrlPtr);
	break;
    case SR_COMMAND:
	charPtr = devPtr->scsiCmdPtr->commandBlock;
	for (i = 0; i < devPtr->scsiCmdPtr->commandBlockLen; i++) {
	PUTCIRCBUF(CSTR,"cmd:");
	    regsPtr->scsi_ctrl.write.FIFO = *charPtr;
	    circBuf[circHead] = ' ';
	    circHead = (circHead + 1) % CIRCBUFLEN;
	    PUTCIRCBUF(CBYTE, (char *)*charPtr);
	    charPtr++;
	}
	devPtr->lastPhase = PHASE_COMMAND;
	PUTCIRCNULL;
	regsPtr->scsi_ctrl.write.synchPer = devPtr->synchPeriod;
	regsPtr->scsi_ctrl.write.synchOffset = devPtr->synchOffset;
	regsPtr->scsi_ctrl.write.command = CR_XFER_INFO;
	break;
    case SR_STATUS:
	/*
	 * We're in status phase.  If all goes right, the next interrupt
	 * will be after we've handled the message phase as well, although
	 * the phase will say we're in message phase.
	 */
	devPtr->lastPhase = PHASE_STATUS;
	regsPtr->scsi_ctrl.write.command = CR_INIT_COMP;
	break;
    case SR_MSG_OUT:
	for (i=0; i<devPtr->messageBufLen; i++) {
	i = regsPtr->scsi_ctrl.read.FIFOFlags & FIFO_BYTES_MASK;
	if (i > 0) {
	    PUTCIRCBUF(CSTR,"msg-out: fifo: ");
	    while (i-- > 0) {
		circBuf[circHead] = ' ';
		circHead = (circHead + 1) % CIRCBUFLEN;
		tempChar = regsPtr->scsi_ctrl.read.FIFO;
		PUTCIRCBUF(CBYTE,(char*)(tempChar));
	    }
	    PUTCIRCNULL;
	}
	PUTCIRCBUF(CSTR,"msg-out: msg: ");
	    regsPtr->scsi_ctrl.write.FIFO = devPtr->messageBuf[i];
	    circBuf[circHead] = ' ';
	    circHead = (circHead + 1) % CIRCBUFLEN;
	    PUTCIRCBUF(CBYTE, (char *)devPtr->messageBuf[i]);
	    devPtr->messageBuf[i] = '\0';
	}
	regsPtr->scsi_ctrl.write.command = CR_XFER_INFO;
	PUTCIRCNULL;
	devPtr->lastPhase = PHASE_MSG_OUT;
	devPtr->messageBufLen = 0;
	status = regsPtr->scsi_ctrl.read.status;
	if (status & SR_GE) {
	    panic("gross error 2");
	}
	break;
    case SR_MSG_IN:
	/* request incoming message xfer */
	regsPtr->scsi_ctrl.write.command = CR_XFER_INFO;
	devPtr->lastPhase = PHASE_MSG_IN;
	break;
    default:
	printf("unknown scsi phase type: 0x%02x\n", (int)phase);
	status = FAILURE;
	break;
    }

    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * PerformCmdDone
 *
 *	Do cleanup after final phase of command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls RequestDone routine to invoke callback
 *      and then gets next item of queue set.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
PerformCmdDone(ctrlPtr,status)
    Controller   *ctrlPtr;
Controller   *ctrlPtr;
ReturnStatus status;
    List_Links		*newRequestPtr;
    ClientData		clientData;

    ctrlPtr->regsPtr->scsi_ctrl.write.command = CR_EN_SLCT;
    if (IS_CTRL_FREE(ctrlPtr)) {
	if (ctrlPtr->interruptDevPtr == (Device *)NIL) {
	    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
						ctrlPtr->devQueuesMask,
						&clientData);
	} else {
	    PUTCIRCBUF(CSTR,"cdone: cmd ");
	    PUTCIRCBUF(CINT,(char *)newRequestPtr);
	    PUTCIRCNULL;
	    clientData = (ClientData)(ctrlPtr->interruptDevPtr);
	    newRequestPtr=(List_Links *)(ctrlPtr->interruptDevPtr->scsiCmdPtr);
	    SET_DEV_FREE(ctrlPtr->interruptDevPtr);
	    ctrlPtr->interruptDevPtr = (Device *)NIL;
	    ctrlPtr->interruptDevPtr = (Device *)NIL;
	    PUTCIRCBUF(CSTR,"cdone: resend dev ");
	    PUTCIRCBUF(CINT, (char *)clientData);
	    PUTCIRCBUF(CSTR," cmd ");
	    PUTCIRCBUF(CINT,(char *)newRequestPtr);
	    PUTCIRCNULL;
	}
	if (newRequestPtr != (List_Links *) NIL) {
	    (void) DevEntryAvailProc(clientData, newRequestPtr);
	}
    } else {
	PUTCIRCBUF(CSTR,"cdone: busy ");
	PUTCIRCNULL;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * PerformSelect
 *
 *	Interpret select phase
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformSelect(ctrlPtr, interruptReg, sequenceReg)
Controller      *ctrlPtr;
unsigned int	interruptReg;
unsigned int	sequenceReg;
{
    Device   *devPtr = ctrlPtr->devPtr;
    static char *errMsg[] = {
	"No error",
	"target timed out",
	"no message-out phase",
	"no command phase",
	"command phase incomplete",
	"unknown sequence error"
	};
    int msgNum;
    ReturnStatus status = SUCCESS;

    switch(sequenceReg) {
    case SEQ_COMPLETE:
	msgNum = 0;
	break;
    case SEQ_NO_SEL:
	if (interruptReg & IR_DISCNCT) {
	    msgNum = 1;
	} else if (!(devPtr->msgFlag & REQEXTENDEDMSG)) {
	    msgNum = 2;
	} else {
	    msgNum = 0;
	    devPtr->msgFlag &= ~REQEXTENDEDMSG;
	    devPtr->messageBuf[0] = SCSI_EXTENDED_MESSAGE;
	    devPtr->messageBuf[1] = 3;
	    devPtr->messageBuf[2] = SCSI_EXTENDED_MSG_SYNC;
	    devPtr->messageBuf[3] = NCR_TO_SCSI(MIN_SYNCH_PERIOD);
	    devPtr->messageBuf[4] = MAX_SYNCH_OFFSET;
	    devPtr->messageBufLen = 5;
	}
	break;
    case SEQ_NO_CMD:
	msgNum = 3;
	break;
    case SEQ_CMD_INCOMPLETE:
	msgNum = 4;
	break;
    default:
	msgNum = 5;
	break;
    }
    
    if (msgNum) {
	ctrlPtr->regsPtr->scsi_ctrl.write.command = CR_FLSH_FIFO;
	status = FAILURE;
	printf("%s: selection failed: %s\n",
	       ctrlPtr->devPtr->handle.locationName,
	       errMsg[msgNum]);
	if (devPtr->targetID == 1) {
	    panic("selection failed\n");
	}
    }
	
    return status;

} 

/*
 *----------------------------------------------------------------------
 *
 * PerformDataXfer
 *
 *	Interpret data-in, data-out condition
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformDataXfer(ctrlPtr, interruptReg, statusReg)
Controller      *ctrlPtr;
unsigned int	interruptReg;
unsigned int	statusReg;
{
    ReturnStatus status = SUCCESS;
    volatile CtrlRegs	*regsPtr = ctrlPtr->regsPtr;
    Device   *devPtr = ctrlPtr->devPtr;

    if (interruptReg & IR_DISCNCT) {
	printf("%s disconnected or timed out during data xfer.\n",
	       devPtr->handle.locationName);
	return DEV_TIMEOUT;
    }

#ifdef sun4c
    dmaControllerActive--;
    MACH_DELAY(100);
#endif
    devPtr->residual = regsPtr->scsi_ctrl.read.xCntLo;
#ifdef sun4c
    MACH_DELAY(100);
#endif
    devPtr->residual += (regsPtr->scsi_ctrl.read.xCntHi << 8);
    /*
     * If the transfer was the maximum, 16K bytes, a 0 in the counter
     * may mean that nothing was transfered...  What should I do? XXX
     */
    if (devPtr->residual != 0) {
	PUTCIRCBUF(CSTR,"DMA xfer didn't finish: bytes left: ");
	PUTCIRCBUF(CINT,(char *)(devPtr->residual));
	PUTCIRCNULL;
    }
    /*
     * Flush the cache on data in, since the dma put it into memory
     * but didn't go through the cache.  We don't have to worry about this
     * on writes, since the sparcstation has a write-through cache.
     */
    if (devPtr->lastPhase == PHASE_DATA_IN) {
	int		amountXfered;
	
	amountXfered = devPtr->scsiCmdPtr->bufferLen - devPtr->residual;
	FLUSH_BYTES((char *) ctrlPtr->buffer, devPtr->scsiCmdPtr->buffer, 
		    amountXfered);
    }
    if (! (statusReg & SR_TC)) {
	/* Transfer count didn't go to zero, or this bit would be set. */
	PUTCIRCBUF(CSTR,"DMA: xfer cnt bit is 0");
	PUTCIRCNULL;
    }
    if (! (interruptReg & IR_BUS_SERV)) {
	/* Target didn't request information transfer phase. */
	printf("Didn't receive bus service signal after DMA xfer.\n");
	status = FAILURE;
    }

    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * PerformStatus
 *
 *	Interpret status condition
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformStatus(ctrlPtr, interruptReg)
Controller      *ctrlPtr;
unsigned int	interruptReg;
{
    volatile CtrlRegs	*regsPtr = ctrlPtr->regsPtr;
    Device              *devPtr = ctrlPtr->devPtr;
    int			numBytes;
    unsigned            char message;

    devPtr->lastPhase = PHASE_STAT_MSG_IN;
    /* Read bytes from FIFO. */
    numBytes = regsPtr->scsi_ctrl.read.FIFOFlags & FIFO_BYTES_MASK;
    if (numBytes != 2) {
	/* We didn't get both phases. */
	printf("SCSIC90: Missing message byte after status phase byte.\n");
	return(FAILURE);
    }
    devPtr->commandStatus = regsPtr->scsi_ctrl.read.FIFO;
    devPtr->commandStatus &= SCSI_STATUS_MASK;
    message = regsPtr->scsi_ctrl.read.FIFO;
    if (! (interruptReg & IR_FUNC_COMP)) {
	printf("SCSIC90: Command didn't complete.\n");
	return(FAILURE);
    }

    if (message != SCSI_COMMAND_COMPLETE) {
	printf("SCSIC90: Expecting cmd_complete msg from %s, got ",
	       ctrlPtr->devPtr->handle.locationName);
	PrintMsg((unsigned int) message);
	return(FAILURE);
    }
    regsPtr->scsi_ctrl.write.command = CR_MSG_ACCPT;
    
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * PerformMsgIn
 *
 *	Interpret msg_in condition
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformMsgIn(ctrlPtr)
Controller *ctrlPtr;
{
    ReturnStatus status = SUCCESS;
    Device *devPtr = ctrlPtr->devPtr;
    volatile CtrlRegs *regsPtr = ctrlPtr->regsPtr;
    int residual;
    unsigned char message;

    message = regsPtr->scsi_ctrl.read.FIFO;

    PUTCIRCBUF(CSTR,"msg in: ");
    PUTCIRCBUF(CBYTE, (char *)message);
    PUTCIRCNULL;

    if (devPtr->msgFlag & STARTEXTENDEDMSG) { 
	status = PerformExtendedMsgIn(ctrlPtr,(unsigned int)message);
	return status;
    }

    switch(message) {
    case SCSI_COMMAND_COMPLETE:
	/* this is handled in that stat_msg_in phase */
	printf("not expecting command_complete msg.\n");
	status = FAILURE;
	break;
    case SCSI_DISCONNECT:
	devPtr->lastPhase = PHASE_RDY_DISCON;
	break;
    case SCSI_SAVE_DATA_POINTER:
	devPtr->lastPhase = PHASE_BUS_FREE;
	residual = regsPtr->scsi_ctrl.read.xCntLo;
	residual += (regsPtr->scsi_ctrl.read.xCntHi << 8);
	if (residual) {
	    devPtr->savedDataPtr = devPtr->scsiCmdPtr->buffer +
		                   devPtr->scsiCmdPtr->bufferLen -
				   residual;
	    devPtr->savedDataLen = residual;
	}
	break;
    case SCSI_RESTORE_POINTERS:
	/* an implicit restore_ptrs is done by reselect proc too */
	devPtr->lastPhase = PHASE_BUS_FREE;
	devPtr->scsiCmdPtr->buffer = devPtr->savedDataPtr;
	devPtr->scsiCmdPtr->bufferLen = devPtr->savedDataLen;
	break;
    case SCSI_IDENTIFY:
	devPtr->lastPhase = PHASE_BUS_FREE;
	break;
    case SCSI_EXTENDED_MESSAGE:
	devPtr->msgFlag |= STARTEXTENDEDMSG;
	status = PerformExtendedMsgIn(ctrlPtr,(unsigned int)message);
	return status;
	break;
    case SCSI_MESSAGE_REJECT:
	lastReject[devPtr->targetID] = circHead;
	devPtr->synchPeriod = MIN_SYNCH_PERIOD;
	devPtr->synchOffset = 0;
	devPtr->lastPhase = PHASE_BUS_FREE;
	devPtr->msgFlag &= ~STARTEXTENDEDMSG;
	break;
    default:
	PUTCIRCBUF(CSTR,"Msg-in: unknown msg");
	PUTCIRCNULL;
	printf("SCSIC90: Couldn't handle msg type: 0x%02x\n",message);
	regsPtr->scsi_ctrl.write.command = CR_SET_ATN;
	devPtr->lastPhase = PHASE_BUS_FREE;
	break;
    }

    regsPtr->scsi_ctrl.write.command = CR_MSG_ACCPT;

    return status;

} /* PerformMsgIn */

/*
 *----------------------------------------------------------------------
 *
 * PerformExtendedMsgIn
 *
 *	Interpret extended msg_in condition
 *
 * Results:
 *	status.
 *
 * Side effects:
 *      Sets the values for synchronous xfer in the device structure.
 *
 *	We get the bytes of the extended msg 1 at a time.
 *      Format: extended msg indicator : 0x01 
 *                       msg length    : 0x?? 
 *                       msg code      : 0x03 (we only do 1 type) 
 *                       msg param1    : 0x?? (should be synch_period)
 *                       msg param2    : 0x?? (should be synch_offset)
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformExtendedMsgIn(ctrlPtr, message)
Controller *ctrlPtr;
unsigned int message;
{
    ReturnStatus status = SUCCESS;
    Device *devPtr      = ctrlPtr->devPtr;
    volatile CtrlRegs *regsPtr = ctrlPtr->regsPtr;
    int len             = devPtr->messageBufLen;
    unsigned char periodInClks;
    unsigned char period;
    unsigned char offset;
    int i;

    devPtr->messageBuf[len] = message;
    devPtr->messageBufLen++;

    switch(len) {
    case 0: /* extended msg code 0x01 */
	devPtr->lastPhase = PHASE_BUS_FREE;
	break;
    case 1: /* msg length. i.e. # bytes to follow this one */
	devPtr->lastPhase = PHASE_BUS_FREE;
	break;
    case 2: /* extended msg code */
	if ((message != SCSI_EXTENDED_MSG_SYNC) ||
	    (devPtr->messageBuf[1] != 3)) {
	    PUTCIRCBUF(CSTR,"Msg-in: bad xtend msg: ");
	    PUTCIRCBUF(CBYTE,(char *)(message));
	    PUTCIRCBUF(CSTR,"; len ");
	    PUTCIRCBUF(CBYTE,(char *)(devPtr->messageBuf[1]));
	    PUTCIRCNULL;
	    panic("bad xtend msg");
	    devPtr->messageBuf[0] = SCSI_MESSAGE_REJECT;
	    devPtr->messageBufLen = 1;
	    regsPtr->scsi_ctrl.write.command = CR_SET_ATN;
	    devPtr->lastPhase = PHASE_MSG_OUT;
	    devPtr->msgFlag &= ~STARTEXTENDEDMSG;
	} else {
	    devPtr->lastPhase = PHASE_BUS_FREE;
	}
	break;
    case 3: /* synch_period */
	devPtr->lastPhase = PHASE_BUS_FREE;
	break;
    case 4: /* synch_offset */
	/* 
	 * Now we have both the period and the offset.
	 */
	period = devPtr->messageBuf[3];
	offset = devPtr->messageBuf[4];
	periodInClks = SCSI_TO_NCR(period);
	/* if values are acceptable, install them else negotiate */
	if ((periodInClks >= MIN_SYNCH_PERIOD) &&
	    (offset <= MAX_SYNCH_OFFSET)) {
	    PUTCIRCBUF(CSTR,"accept per: ");
	    PUTCIRCBUF(CBYTE,(char *)period);
	    PUTCIRCBUF(CSTR,"; per clk ");
	    PUTCIRCBUF(CBYTE,(char *)periodInClks);
	    PUTCIRCBUF(CSTR,"; off ");
	    PUTCIRCBUF(CBYTE,(char *)offset);
	    PUTCIRCNULL;
	    devPtr->synchPeriod = periodInClks; 
	    devPtr->synchOffset = offset;
	    devPtr->lastPhase = PHASE_BUS_FREE;
	    devPtr->msgFlag &= ~STARTEXTENDEDMSG;
	} else {
	    if (periodInClks < devPtr->synchPeriod) {
		devPtr->messageBuf[3] = NCR_TO_SCSI(MIN_SYNCH_PERIOD);
	    }
	    if (offset > devPtr->synchOffset) {
		devPtr->messageBuf[4] = MAX_SYNCH_OFFSET;
	    }
	    PUTCIRCBUF(CSTR,"negotiate per: ");
	    PUTCIRCBUF(CBYTE,(char *)(devPtr->messageBuf[3]));
	    PUTCIRCBUF(CSTR,"; off ");
	    PUTCIRCBUF(CBYTE,(char *)(devPtr->messageBuf[4]));
	    PUTCIRCNULL;
	    regsPtr->scsi_ctrl.write.command = CR_SET_ATN;
	    devPtr->lastPhase = PHASE_MSG_OUT;
	    devPtr->msgFlag &= ~STARTEXTENDEDMSG;
	}
	break;
    default:
	printf("SCSIC90: xmsg case error\n");
	devPtr->msgFlag &= ~STARTEXTENDEDMSG;
	status = FALSE;
	break;
    }
    PUTCIRCBUF(CSTR,"Xmsg-in: accept msg");
    len = regsPtr->scsi_ctrl.read.FIFOFlags & FIFO_BYTES_MASK;
    PUTCIRCBUF(CSTR,"; FIFO:");
    for(i=0;i<len;i++) {
	circBuf[circHead] = ' ';
	circHead = (circHead + 1) % CIRCBUFLEN;
	offset = regsPtr->scsi_ctrl.read.FIFO;
	PUTCIRCBUF(CBYTE, (char *)offset);
    }
    PUTCIRCBUF(CSTR,"; mbuf:");
    for(i=0;i<devPtr->messageBufLen;i++) {
	circBuf[circHead] = ' ';
	circHead = (circHead + 1) % CIRCBUFLEN;
	PUTCIRCBUF(CBYTE, (char *)(devPtr->messageBuf[i]));
    }
    PUTCIRCNULL;
    regsPtr->scsi_ctrl.write.command = CR_MSG_ACCPT;
    return status;
} 

/*
 *----------------------------------------------------------------------
 *
 * PerformReselect
 *
 *	Reconnect a logical unit
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PerformReselect(ctrlPtr, interruptReg)
Controller      *ctrlPtr;
unsigned int	interruptReg;
{
    volatile CtrlRegs *regsPtr = ctrlPtr->regsPtr;
    Device            *devPtr;
    unsigned char     target;
    unsigned char     ourID;
    unsigned char     message;
    int               fifoCnt,i;

    /* The ID of the target which is requesting reselection comes over 
     * the bus encoded, not 0-7.  The encoding is the logical OR of 
     * two bytes: one with the i'th bit set (for target #i) and
     * one with the host's bit set (usually #7). Unfortunately,
     * the host's ID is kindly provided to us in binary so we perform
     * a little transformation...
     */
    target = regsPtr->scsi_ctrl.read.FIFO;
    ourID = regsPtr->scsi_ctrl.read.config1 & C1_BUS_ID;
    ourID = ~(1 << ourID);

    switch(target & ourID) {
    case 0x01:
	target = 0;
	break;
    case 0x02:
	target = 1;
	break;
    case 0x04:
	target = 2;
	break;
    case 0x08:
	target = 3;
	break;
    case 0x10:
	target = 4;
	break;
    case 0x20:
	target = 5;
	break;
    case 0x40:
	target = 6;
	break;
    case 0x80:
	target = 7;
	break;
    default:
	printf("SCSIC90: couldn't decode target ID 0x%02x\n", target);
	return FAILURE;
    }

    message = regsPtr->scsi_ctrl.read.FIFO;    

    if (!(message & SCSI_IDENTIFY)) {
	printf("SCSIC90: Expected Identify msg after reselect, got 0x%02x.\n",
	       message);
	return FAILURE;
    }
    message &= SCSI_IDENT_LUN_MASK;

    devPtr = ctrlPtr->devicePtr[target][message];

    fifoCnt = regsPtr->scsi_ctrl.read.FIFOFlags & FIFO_BYTES_MASK;
    PUTCIRCBUF(CSTR,"resel: targ ");
    PUTCIRCBUF(CBYTE, (char *)target);
    PUTCIRCBUF(CSTR,"; lun ");
    PUTCIRCBUF(CBYTE, (char *)message);
    ourID = regsPtr->scsi_ctrl.read.command;
    PUTCIRCBUF(CSTR,"; cmdreg:");
    PUTCIRCBUF(CBYTE,(char *)ourID);
    PUTCIRCBUF(CSTR,"; FIFO:");
    for(i=0;i<fifoCnt;i++) {
	circBuf[circHead] = ' ';
	circHead = (circHead + 1) % CIRCBUFLEN;
	ourID = regsPtr->scsi_ctrl.read.FIFO;
	PUTCIRCBUF(CBYTE, (char *)ourID);
    }
    PUTCIRCNULL;

    if (IS_DEV_FREE(devPtr)) {
	panic("resel: device is free.\n");
    }

    /*
     * It's possible for the reselection interrupt to occur just
     * when we were sending a new command.  Three cases:
     *  1) Interrupt happened before loading any cmd bytes.
     *     Only indication is that controller is busy.
     *  2) Interrupt happened before the send; part of the
     *     cmd in the fifo.
     *  3) Interrupt happened during the arbitration/selection
     *     phases of the send; the bus_serv bit will be set.
     * Note that the 53C90 doesn't handle this situation quite
     * the same as the later 53C94, 53C95 chips. The older chip
     * accepts cmd bytes after it shouldn't so there may be bytes
     * in the FIFO now that should be here. See the manual.
     */


    /* do an implied restore data pointer */
    devPtr->scsiCmdPtr->buffer = devPtr->savedDataPtr;
    devPtr->scsiCmdPtr->bufferLen = devPtr->savedDataLen;
    regsPtr->scsi_ctrl.write.command = CR_MSG_ACCPT;

    if (!(IS_CTRL_FREE(ctrlPtr)) ||
	(interruptReg & IR_BUS_SERV) ||
	(fifoCnt > 0)) {
	lastConflict[ctrlPtr->devPtr->targetID] = circHead;
	PUTCIRCBUF(CSTR,"resel: save dev ");
	PUTCIRCBUF(CINT, (char *)ctrlPtr->devPtr);
	PUTCIRCBUF(CSTR," cmd ");
	PUTCIRCBUF(CINT, (char *)ctrlPtr->devPtr->scsiCmdPtr);
	PUTCIRCNULL;
	ctrlPtr->interruptDevPtr = ctrlPtr->devPtr;
	regsPtr->scsi_ctrl.write.command = CR_FLSH_FIFO;
    }

    SET_CTRL_BUSY(ctrlPtr,devPtr);

    return SUCCESS;
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
 * DevSCSIC90AttachDevice --
 *
 *	Attach a SCSI device using the Sun SCSIC90 HBA. 
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
DevSCSIC90AttachDevice(devicePtr, insertProc)
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
     * First find the SCSIC90 controller this device is on.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr);
    if ((ctrlNum > MAX_SCSIC90_CTRLS) ||
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
					      (1<<targetID),
					      insertProc,
					      (ClientData) devPtr);
    devPtr->handle.locationName = "Unknown";
    devPtr->handle.LUN = lun;
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.maxTransferSize = MAX_TRANSFER_SIZE;
    devPtr->targetID = targetID;
    devPtr->ctrlPtr = ctrlPtr;
    devPtr->synchPeriod = 0;
    devPtr->synchOffset = 0;
    devPtr->msgFlag = 0;
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
    SET_DEV_FREE(devPtr);
    ctrlPtr->devicePtr[targetID][lun] = devPtr;
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    (void) sprintf(tmpBuffer, "%s Target %d LUN %d", ctrlPtr->name, 
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    if (devSCSIC90Debug > 3) {
	printf("devSCSIC90Attach: attached device %s.\n", tmpBuffer);
    }

    return (ScsiDevice *) devPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintMsg --
 *
 *	Print out the asci string for a scsi msg.
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
PrintMsg(msg)
    unsigned int msg;
{

    if (msg & SCSI_IDENTIFY) {
	if (msg & SCSI_DIS_REC_IDENTIFY) {
	    printf("dis_rec_identify (LUN 0x%02x) msg.\n",
		   (int)(msg & SCSI_IDENT_LUN_MASK));
	} else {
	    printf("identify (LUN 0x%02x) msg.\n",
		   (int)(msg & SCSI_IDENT_LUN_MASK));
	}
	return;
    } 

    switch (msg) {
    case SCSI_COMMAND_COMPLETE:
	printf("command_complete msg.\n");
	break;
    case SCSI_SAVE_DATA_POINTER:
	printf("save_data_ptr msg.\n");
	break;
    case SCSI_RESTORE_POINTERS:
	printf("restore_ptrs msg.\n");
	break;
    case SCSI_DISCONNECT:
	printf("disconnect msg.\n");
	break;
    case SCSI_ABORT:
	printf("abort msg.\n");
	break;
    case SCSI_MESSAGE_REJECT:
	printf("msg_reject msg.\n");
	break;
    case SCSI_NO_OP:
	printf("no_op msg.\n");
	break;
    case SCSI_MESSAGE_PARITY_ERROR:
	printf("msg_parity msg.\n");
	break;
    case SCSI_BUS_RESET:
	printf("bus_reset msg.\n");
	break;
    default:
	printf("unknown msg %d.\n", msg);
	break;
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintPhase --
 *
 *	Print out the asci string for a scsi phase.
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
PrintPhase(phase)
    unsigned int	phase;
{
    
    switch (phase) {
    case SR_DATA_OUT:
	printf("data out phase.\n");
	break;
    case SR_DATA_IN:
	printf("data in phase.\n");
	break;
    case SR_COMMAND:
	printf("command phase.\n");
	break;
    case SR_STATUS:
	printf("status phase.\n");
	break;
    case SR_MSG_OUT:
	printf("msg out phase.\n");
	break;
    case SR_MSG_IN:
	printf("msg in phase.\n");
	break;
    default:
	printf("unknown phase %d.\n", phase);
	break;
    }

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintLastPhase --
 *
 *	Print out the asci string for the last scsi phase we were in.
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
PrintLastPhase(phase)
    unsigned int	phase;
{
    switch (phase) {
    case PHASE_BUS_FREE:
	printf("bus free phase.\n");
	break;
    case PHASE_SELECTION:
	printf("selection phase.\n");
	break;
    case PHASE_DATA_OUT:
	printf("data out phase.\n");
	break;
    case PHASE_DATA_IN:
	printf("data in phase.\n");
	break;
    case PHASE_STATUS:
	printf("status phase.\n");
	break;
    case PHASE_MSG_IN:
	printf("msg in phase.\n");
	break;
    case PHASE_STAT_MSG_IN:
	printf("stat_msg in phase.\n");
	break;
    case PHASE_RDY_DISCON:
	printf("rdy_discon phase.\n");
	break;
    default:
	printf("unknown phase %d.\n", phase);
	break;
    }

    return;
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
static void
PrintRegs(regsPtr)
    register volatile CtrlRegs *regsPtr;
{

    printf("Won't print interrupt register since that would clear it.\n");
    printf("xCntLow: 0x%x, xCntHi: 0x%x, FIFO: 0x%x, command: 0x%x,\n",
	    regsPtr->scsi_ctrl.read.xCntLo,
	    regsPtr->scsi_ctrl.read.xCntHi,
	    regsPtr->scsi_ctrl.read.FIFO,
	    regsPtr->scsi_ctrl.read.command);
    printf("status: 0x%x, sequence: 0x%x, FIFOFlags: 0x%x, config1: 0x%x,\n",
	    regsPtr->scsi_ctrl.read.status,
	    regsPtr->scsi_ctrl.read.sequence,
	    regsPtr->scsi_ctrl.read.FIFOFlags,
	    regsPtr->scsi_ctrl.read.config1);
    printf("config2: 0x%x, config3: 0x%x\n",
	    regsPtr->scsi_ctrl.read.config2,
	    regsPtr->scsi_ctrl.read.config3);

    return;
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
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_ChangeScsiDebugLevel --
 *
 *	Change the level of debugging info for this scsi driver.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A larger or lesser number of messages will be printed out.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ChangeScsiDebugLevel(level)
    int	level;
{
    
    printf("Changing scsi debug level: was %d, ", devSCSIC90Debug);
    devSCSIC90Debug = level;
    printf("and is now %d.\n", devSCSIC90Debug);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * PutCircBuf
 *
 *	Stuff data into the circular log buffer
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void
PutCircBuf(type, object)
int type;
char *object;
{
    int num = (int)object;

    switch(type) {
    case CSTR:
	while(*object) {
	    circBuf[circHead] = *object++;
	    circHead = (circHead + 1) % CIRCBUFLEN;
	}
	break;
    case CBYTE:
	circBuf[circHead] = CVTHEX(num,4);
	circHead = (circHead + 1) % CIRCBUFLEN;
	circBuf[circHead] = CVTHEX(num,0);
	circHead = (circHead + 1) % CIRCBUFLEN;
	break;
    case CINT:
	circBuf[circHead] = CVTHEX(num,28); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,24); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,20); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,16); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,12); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,8); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,4); 
	circHead = (circHead + 1) % CIRCBUFLEN; 
	circBuf[circHead] = CVTHEX(num,0); 
	circHead = (circHead + 1) % CIRCBUFLEN;
	break;
    default:
	panic("PutCircBuf: unknown type\n");
	break;
    }
}

