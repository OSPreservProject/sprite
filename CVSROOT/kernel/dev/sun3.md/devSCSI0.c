/* 
 * devSCSI0.c --
 *
 *	Driver routines specific to the original Sun Host Adaptor.
 *	This lives either on the Multibus or the VME.  It does
 *	not support dis-connect/connect.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
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
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "devSCSI.h"
#include "devMultibus.h"
#include "devDiskLabel.h"
#include "vm.h"
#include "sync.h"
#include "proc.h"	/* for Mach_SetJump */
#include "sched.h"

void DevSCSI0Reset();
void DevSCSI0Command();
void DevSCSI0Intr();


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Probe --
 *
 *	Probe memory for the old-style VME SCSI interface.  We rely
 *	on the fact that this occupies 4K of address space.  This should
 *	be called before ProbeNewAdaptor because the new adaptor looks
 *	very much like the old one.
 *
 * Results:
 *	TRUE if the host adaptor was found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevSCSI0Probe(address, scsiPtr)
    Address address;	/* Alledged controller address */
    register DevSCSIController *scsiPtr;	/* Controller state */
{
    volatile DevSCSIRegs *regsPtr = (DevSCSIRegs *)address;

    if (Mach_SetJump(&setJumpState) == SUCCESS) {
	/*
	 * Touch the device. If it exists it occupies 4K.
	 */
	regsPtr->dmaCount = 0x4BCC;
	regsPtr = (DevSCSIRegs *)((int)address + 0x800);
	regsPtr->dmaCount = 0x5BCC;
    } else {
	/*
	 * Got a bus error trying to access the dma count register.
	 */
	Mach_UnsetJump();
	return(FALSE);
    }
    Mach_UnsetJump();
    scsiPtr->type = SCSI0;
    scsiPtr->onBoard = FALSE;
    scsiPtr->udcDmaTable = (DevUDCDMAtable *)NIL;
    scsiPtr->resetProc = DevSCSI0Reset;
    scsiPtr->commandProc = DevSCSI0Command;
    scsiPtr->intrProc = DevSCSI0Intr;
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Reset --
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
void
DevSCSI0Reset(scsiPtr)
    DevSCSIController *scsiPtr;
{
    volatile DevSCSIRegs *regsPtr = (DevSCSIRegs *)scsiPtr->regsPtr;

    regsPtr->control = SCSI_RESET;
    MACH_DELAY(100);
    regsPtr->control = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Command --
 *
 *      Send a command to a controller on the old-style SCSI Host Adaptor
 *      indicated by scsiPtr.  The control block needs to have
 *      been set up previously with DevSCSISetupCommand.  If the interrupt
 *      argument is WAIT (FALSE) then this waits around for the command to
 *      complete and checks the status results.  Otherwise Dev_SCSIIntr
 *      will be invoked later to check completion status.
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
ReturnStatus
DevSCSI0Command(slaveID, scsiPtr, size, addr, interrupt)
    int slaveID;			/* Id of the SCSI device to select */
    DevSCSIController *scsiPtr;		/* The SCSI controller that will be
					 * doing the command. The control block
					 * within this specifies the unit
					 * number and device address of the
					 * transfer */
    int size;				/* Number of bytes to transfer */
    Address addr;			/* Kernel address of transfer */
    int interrupt;			/* WAIT or INTERRUPT.  If INTERRUPT
					 * then this procedure returns
					 * after initiating the command
					 * and the device interrupts
					 * later.  If WAIT this polls
					 * the SCSI interface register
					 * until the command completes. */
{
    register ReturnStatus status;
    register DevSCSIRegs *regsPtr;	/* Host Adaptor registers */
    char *charPtr;			/* Used to put the control block
					 * into the commandStatus register */
    int i;
    int bits = 0;			/* variable bits to OR into control */
    Boolean checkMsg = FALSE;		/* have DevSCSI0Wait check for
					   a premature message */

    /*
     * Save some state needed by the interrupt handler to check errors.
     */
    scsiPtr->command = scsiPtr->controlBlock.command;

    regsPtr = scsiPtr->regsPtr;
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
	DevSCSIReset(regsPtr);
	printf("SCSI bus stuck busy\n");
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
    regsPtr->data = (1 << slaveID);
    regsPtr->control = SCSI_SELECT;
    status = DevSCSI0Wait(regsPtr, SCSI_BUSY, NO_RESET, FALSE);
    if (status != SUCCESS) {
	regsPtr->data = 0;
	regsPtr->control = 0;
	if (scsiPtr->controlBlock.command != SCSI_TEST_UNIT_READY) {
	    printf("SCSI-%d: can't select slave %d\n", 
				 scsiPtr->number, slaveID);
	}
	return(status);
    }
    /*
     * Set up the interface's registers for the transfer.  The DMA address
     * is relative to the multibus memory so the kernel's base address
     * for multibus memory is subtracted from 'addr'. The host adaptor
     * increments the dmaCount register until it reaches -1, hence the
     * funny initialization. See page 4 of Sun's SCSI Prog. Manual.
     */
    regsPtr->dmaAddress = (int)(addr - DEV_MULTIBUS_BASE);
    regsPtr->dmaCount = -size - 1;
    bits = SCSI_WORD_MODE | SCSI_DMA_ENABLE;
    if (interrupt == INTERRUPT) {
	bits |= SCSI_INTERRUPT_ENABLE;
    } 
    regsPtr->control = bits;

    /*
     * Stuff the control block through the commandStatus register.
     * The handshake on the SCSI bus is visible here:  we have to
     * wait for the Request line on the SCSI bus to be raised before
     * we can send the next command byte to the controller.  All commands
     * are of "group 0" which means they are 6 bytes long.
     */
    charPtr = (char *)&scsiPtr->controlBlock;
    if (scsiPtr->devPtr->type == SCSI_WORM) {
	checkMsg = TRUE;
    }
    for (i=0 ; i<sizeof(DevSCSIControlBlock) ; i++) {
	status = DevSCSI0Wait(regsPtr, SCSI_REQUEST, RESET, checkMsg);
/*
 * This is just a guess.
 */
	if (status == DEV_EARLY_CMD_COMPLETION) {
	    return(SUCCESS);
	}
	if (status != SUCCESS) {
	    printf("SCSI-%d: couldn't send command block (i=%d)\n",
				 scsiPtr->number, i);
	    return(status);
	}
	/*
	 * The device keeps the Control/Data line set while it
	 * is accepting control block bytes.
	 */
	if ((regsPtr->control & SCSI_COMMAND) == 0) {
	    DevSCSIReset(regsPtr);
	    printf("SCSI-%d: device dropped command line\n",
				 scsiPtr->number);
	    return(DEV_HANDSHAKE_ERROR);
	}
	regsPtr->commandStatus = *charPtr;
	charPtr++;
    }
    if (interrupt == WAIT) {
	/*
	 * A synchronous command.  Wait here for the command to complete.
	 */
	status = DevSCSI0Wait(regsPtr, SCSI_INTERRUPT_REQUEST, RESET, FALSE);
	if (status == SUCCESS) {
	    scsiPtr->residual = -regsPtr->dmaCount -1;
	    status = DevSCSIStatus(scsiPtr);
	} else {
	    printf("SCSI-%d: couldn't wait for command to complete\n",
				 scsiPtr->number);
	}
    } else {
	status = SUCCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Status --
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
ReturnStatus
DevSCSI0Status(scsiPtr)
    DevSCSIController *scsiPtr;
{
    register ReturnStatus status;
    register DevSCSIRegs *regsPtr;
    short message;
    char statusByte;
    char *statusBytePtr;
    int numStatusBytes = 0;

    regsPtr = scsiPtr->regsPtr;
    statusBytePtr = (char *)&scsiPtr->statusBlock;
    bzero((Address)statusBytePtr, sizeof(DevSCSIStatusBlock));
    for ( ; ; ) {
	/*
	 * Could probably wait either on the INTERUPT_REQUEST bit or the
	 * REQUEST bit.  Reading the byte out of the commandStatus
	 * register acknowledges the REQUEST and clears these bits.  Here
	 * we grab bytes until the MESSAGE bit indicates that all the
	 * status bytes have been received and that the byte in the
	 * commandStatus register is the message byte.
	 */
	status = DevSCSI0Wait(regsPtr, SCSI_REQUEST, RESET, FALSE);
	if (status != SUCCESS) {
	    printf("SCSI-%d: wait error after %d status bytes\n",
				 scsiPtr->number, numStatusBytes);
	    break;
	}
	if (regsPtr->control & SCSI_MESSAGE) {
	    message = regsPtr->commandStatus & 0xff;
	    if (message != SCSI_COMMAND_COMPLETE) {
		printf("SCSI-%d: Unexpected message 0x%x\n",
				     scsiPtr->number, message);
	    }
	    break;
	}  else {
	    /*
	     * This is another status byte.  Place the first few status
	     * bytes into the status block.
	     */
	    statusByte = regsPtr->commandStatus;
	    if (numStatusBytes < sizeof(DevSCSIStatusBlock)) {
		*statusBytePtr = statusByte;
		statusBytePtr++;
	    }
	    numStatusBytes++;
	}
    }
    if (status == SUCCESS) {
	/*
	 * The status may indicate that further ``sense'' data is
	 * available.  This is obtained by another SCSI command
	 * that uses DMA to transfer the sense data.
	 */
	if (scsiPtr->statusBlock.check) {
	    status = DevSCSIRequestSense(scsiPtr, scsiPtr->devPtr);
	}
	if (scsiPtr->statusBlock.error) {
	    printf("SCSI-%d: host adaptor error bit set\n",
				 scsiPtr->number);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Wait --
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
ReturnStatus
DevSCSI0Wait(regsPtr, condition, reset, checkMsg)
    DevSCSIRegs *regsPtr;
    int condition;
    Boolean reset;
    Boolean checkMsg;
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register int control;

    if (devSCSIDebug && checkMsg) {
	printf("DevSCSI0Wait: checking for message.\n");
    }
    for (i=0 ; i<SCSI_WAIT_LENGTH ; i++) {
	control = regsPtr->control;
        /*
	 * For debugging of WORM: 
	 *  .. using printf because using kdbx causes different behavior.
	 */
	if (devSCSIDebug && i < 5) {
	    printf("%d/%x ", i, control);
	}
/* this is just a guess too. */
	if (checkMsg) {
	    register int mask = SCSI_REQUEST | SCSI_INPUT | SCSI_MESSAGE | SCSI_COMMAND;
	    if ((control & mask) == mask) {
		register int msg;
	    
		msg = regsPtr->commandStatus & 0xff;
		printf("DevSCSI0Wait: Unexpected message 0x%x\n", msg);
		if (msg == SCSI_COMMAND_COMPLETE) {
		    return(DEV_EARLY_CMD_COMPLETION);
		} else {
		    return(DEV_HANDSHAKE_ERROR);
		}
	    }
	}
	if (control & condition) {
	    return(SUCCESS);
	}
	if (control & SCSI_BUS_ERROR) {
	    printf("SCSI: bus error\n");
	    status = DEV_DMA_FAULT;
	    break;
	} else if (control & SCSI_PARITY_ERROR) {
	    printf("SCSI: parity error\n");
	    status = DEV_DMA_FAULT;
	    break;
	}
	MACH_DELAY(10);
    }
    if (devSCSIDebug) {
	printf("DevSCSI0Wait: timed out, control = %x.\n", control);
    }
    if (reset) {
	DevSCSIReset(regsPtr);
    }
    return(status);
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
DevSCSI0Intr(scsiPtr)
    register DevSCSIController *scsiPtr;
{
    volatile DevSCSIRegs *regsPtr = (DevSCSIRegs *)scsiPtr->regsPtr;

    if (regsPtr->control & SCSI_INTERRUPT_REQUEST) {
	if (regsPtr->control & SCSI_BUS_ERROR) {
	    if (regsPtr->dmaCount >= 0) {
		/*
		 * A DMA overrun.  Unlikely with a disk but could
		 * happen while reading a large tape block.  Consider
		 * the I/O complete with no residual bytes
		 * un-transferred.
		scsiPtr->residual = 0;
		scsiPtr->flags |= SCSI_IO_COMPLETE;
	    } else {
		/*
		 * A real Bus Error.  Complete the I/O but flag an error.
		 * The residual is computed because the Bus Error could
		 * have occurred after a number of sectors.
		 */
		scsiPtr->residual = -regsPtr->dmaCount -1;
		scsiPtr->flags |= SCSI_IO_COMPLETE;
	    }
	    /*
	     * The board needs to be reset to clear the Bus Error
	     * condition so no status bytes are grabbed.
	     */
	    DevSCSIReset(scsiPtr->regsPtr);
	    scsiPtr->status = DEV_DMA_FAULT;
	    Sync_MasterBroadcast(&scsiPtr->IOComplete);
	    return(TRUE);
	} else {
	    /*
	     * Normal command completion.  Compute the residual,
	     * the number of bytes not transferred, check for
	     * odd transfer sizes, and finally get the completion
	     * status from the device.
	     */
	    scsiPtr->residual = -regsPtr->dmaCount -1;
	    if (regsPtr->control & SCSI_ODD_LENGTH) {
		/*
		 * On a read the last odd byte is left in the data
		 * register.  On both reads and writes the number
		 * of bytes transferred as determined from dmaCount
		 * is off by one.  See Page 8 of Sun's SCSI
		 * Programmers' Manual.
		 */
		if (scsiPtr->controlBlock.command == SCSI_READ) {
		    *(char *)(DEV_MULTIBUS_BASE + regsPtr->dmaAddress) =
			regsPtr->data;
		    scsiPtr->residual--;
		} else {
		    scsiPtr->residual++;
		}
	    }
	    scsiPtr->status = DevSCSIStatus(scsiPtr);
	    scsiPtr->flags |= SCSI_IO_COMPLETE;
	    Sync_MasterBroadcast(&scsiPtr->IOComplete);
	    return(TRUE);
	}
    } else {
	return(FALSE);
    }
}
