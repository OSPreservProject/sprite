/* 
 * devSCSI3.c --
 *
 *	Routines specific to the SCSI-3 Host Adaptor.  This adaptor is
 *	based on the NCR 5380 chip.  There are two variants, one is
 *	"onboard" the main CPU board (3/50, 3/60, 4/110) and uses a
 *	Universal DMA Controller chip, the AMD 9516 UDC.  The other is
 *	on the VME and has a much simpler DMA interface.  Both are
 *	supported here.  The 5380 supports connect/dis-connect.
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
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devDiskLabel.h"
#include "sys.h"
#include "sync.h"
#include "proc.h"	/* for Mach_SetJump */
#include "sched.h"

int devSCSI3Debug = 5;

/*
 * Number of times to try things like target selection.
 */
#define SBC_NUM_RETRIES 3

/*
 * Registers are passed into the wait routine as Addresses, and their
 * size is passed in as a separate argument to determine type coercion.
 */
typedef enum {
    REG_BYTE,
    REG_SHORT
} RegType;

/*
 * For waiting, there are several possibilities:
 *  ACTIVE_HIGH - wait for any bits in mask to be 1
 *  ACTIVE_ALL  - wait for all bits in mask to be 1
 *  ACTIVE_LOW  - wait for any bits in mask to be 0.
 *  ACTIVE_NONE - wait for all bits in mask to be 0.
 */
typedef enum {
    ACTIVE_HIGH,
    ACTIVE_ALL,
    ACTIVE_LOW,
    ACTIVE_NONE,
} BitSelection;	    

/*
 * Forward declarations.  FIXME: separate file-local from dev-global routines.
 */

void		DevSCSI3Reset();
ReturnStatus	DevSCSI3Command();
ReturnStatus	DevSCSI3Status();
Boolean		DevSCSI3Intr();
ReturnStatus	DevSCSI3Wait();
ReturnStatus	DevSCSI3WaitReg();
static ReturnStatus GetByte();
static void 	PrintRegs();
static void 	StartDMA();


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3ProbeOnBoard --
 *
 *	Probe memory for the onboard SCSI-3 interface.
 *
 * Results:
 *	TRUE if the host adaptor was found.
 *
 * Side effects:
 *	Sets controller type, register address, and dmaState.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevSCSI3ProbeOnBoard(address, scsiPtr)
    int address;			/* Alledged controller address */
    register DevSCSIController *scsiPtr;	/* Controller state */
{
    register DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)address;
    int x;

    if (Mach_SetJump(&scsiSetJumpState) == SUCCESS) {
	/*
	 * Touch the device's UDC read data register.
	 */
	x = regsPtr->udcRdata;
#ifdef lint
	regsPtr->udcRdata = x;
#endif
    } else {
	/*
	 * Got a bus error trying to access the UDC data register.
	 */
	Mach_UnsetJump();
	return(FALSE);
    }
    Mach_UnsetJump();
    scsiPtr->onBoard = TRUE;
    scsiPtr->type = SCSI3;
    scsiPtr->udcDmaTable = (DevUDCDMAtable *) malloc(sizeof(DevUDCDMAtable));
    scsiPtr->resetProc = DevSCSI3Reset;
    scsiPtr->commandProc = DevSCSI3Command;
    scsiPtr->intrProc = DevSCSI3Intr;
    if (devSCSI3Debug > 4) {
	printf("Onboard SCSI3 found\n");
    }
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3ProbeVME --
 *
 *	Probe memory for the new-style VME SCSI interface.  This occupies
 *	2K of VME space and it looks very much like the old-style adaptor,
 *	so we should be called after DevSCSI0Probe.
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
DevSCSI3ProbeVME(address, scsiPtr, vector)
    int address;			/* Alledged controller address */
    register DevSCSIController *scsiPtr;	/* Controller state */
    int vector;		/* Vector number for VME vectored interrupts */
{
    volatile DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)address;
    Boolean found = FALSE;

    if (Mach_SetJump(&scsiSetJumpState) == SUCCESS) {
	/*
	 * Touch the device. The dmaCount register should hold more
	 * than 16 bits, which is all the old host adaptor's dmaCount can hold.
	 */
	regsPtr->dmaCount = 0x4AABCC;
	if (regsPtr->dmaCount != 0x4AABCC) {
	    printf("ProbeSCSI-3 read back problem %x not %x\n",
		regsPtr->dmaCount, 0x4AABCC);
	} else {
	    found = TRUE;
	    /*
	     * Set the address modifier in the interrupt vector.
	     */
	    regsPtr->addrIntr = vector | VME_SUPV_DATA_24;

	    scsiPtr->type = SCSI3;
	    scsiPtr->onBoard = FALSE;
	    scsiPtr->udcDmaTable = (DevUDCDMAtable *)NIL;
	    scsiPtr->resetProc = DevSCSI3Reset;
	    scsiPtr->commandProc = DevSCSI3Command;
	    scsiPtr->intrProc = DevSCSI3Intr;
	    if (devSCSI3Debug > 4) {
		printf("VME SCSI3 found\n");
	    }
	}
    }
    Mach_UnsetJump();
    return(found);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3Reset --
 *
 *	Reset a SCSI bus controlled by the SCSI-3 Sun Host Adaptor.
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
DevSCSI3Reset(scsiPtr)
    DevSCSIController *scsiPtr;
{
    volatile DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    unsigned char clear;

    regsPtr->fifoCount = 0;
    regsPtr->control = 0;
    MACH_DELAY(100);
    regsPtr->control = SI_CSR_SCSI_RES | SI_CSR_FIFO_RES;

    if (!scsiPtr->onBoard) {
	regsPtr->dmaCount = 0;
	regsPtr->dmaAddress = 0;
    }

    regsPtr->sbc.write.initCmd = SBC_ICR_RST;
    MACH_DELAY(1000);
    regsPtr->sbc.write.initCmd = 0;
    
    clear = regsPtr->sbc.read.clear;
#ifdef lint
    regsPtr->sbc.read.clear = clear;
#endif
    regsPtr->control |= SI_CSR_INTR_EN;
    regsPtr->sbc.write.mode = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3Command --
 *
 *      Send a command to a SCSI controller via the SCSI-3 Host Adaptor.
 *	The control block needs to have
 *      been set up previously with DevSCSISetupCommand.  If the interrupt
 *      argument is WAIT (FALSE) then this waits around for the command to
 *      complete and checks the status results.  Otherwise DevSCSI3Intr
 *      will be invoked later to check completion status.
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
DevSCSI3Command(targetID, scsiPtr, size, addr, interrupt)
    int targetID;			/* Id of the SCSI device to select */
    DevSCSIController *scsiPtr;		/* The SCSI controller that will be
					 * doing the command. The control block
					 * within this specifies the unit
					 * number and device address of the
					 * transfer */
    int size;				/* Number of bytes to transfer */
    Address addr;			/* Kernel address of transfer */
    int interrupt;			/* WAIT or INTERRUPT.  If INTERRUPT
					 * then this procedure returns
					 * after nitiating the command
					 * and the device interrupts
					 * later.  If WAIT this polls
					 * the SCSI interface register
					 * until the command completes. */
{
    register ReturnStatus status;
    register DevSCSI3Regs *regsPtr;	/* Host Adaptor registers */
    char *charPtr;			/* Used to put the control block
					 * into the commandStatus register */
    int i;
    unsigned char initCmd;		/* holder for initCmd value during
					   wait */
    unsigned char *initCmdPtr;		/* pointer to initCmd register */
    unsigned char *modePtr;		/* pointer to mode register */
    char command;			/* holder for command field of
					 * command block, when checking
					 * for send/receive of data */
    unsigned char phase;

    /*
     * Save some state needed by the interrupt handler to check errors.
     */
    command = scsiPtr->controlBlock.command;
    scsiPtr->command = command;

    switch(command) {
	case SCSI_READ:
	case SCSI_REQUEST_SENSE:
	case SCSI_INQUIRY:
	case SCSI_MODE_SENSE:
	case SCSI_READ_BLOCK_LIMITS:
	case SCSI_READ_CAPACITY:
	    scsiPtr->devPtr->dmaState = SBC_DMA_RECEIVE;
	    break;
	case SCSI_WRITE:
	case SCSI_MODE_SELECT:
	case SCSI_SEND_DIAGNOSTIC:
	    scsiPtr->devPtr->dmaState = SBC_DMA_SEND;
	    break;
	default:
	    scsiPtr->devPtr->dmaState = SBC_DMA_INACTIVE;
	    break;
    }
    if (devSCSI3Debug > 3) {
	printf("SCSI3Command 0x%x targetID %d addr %x size %d dma %s\n",
	    command, targetID, addr, size,
	    (scsiPtr->devPtr->dmaState == SBC_DMA_INACTIVE) ? "not active" :
		((scsiPtr->devPtr->dmaState == SBC_DMA_SEND) ? "send" :
							      "receive"));
    }

    regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    initCmdPtr = &regsPtr->sbc.write.initCmd;
    modePtr = &regsPtr->sbc.write.mode;
    if (!scsiPtr->onBoard) {
	/*
	 * For VME interface dis-allow DMA interrupts from reconnect attempts.
	 */
	regsPtr->control &= ~SI_CSR_DMA_EN;
	regsPtr->control |= SI_CSR_BPCON;	/* word byte packing */
    }
    regsPtr->sbc.write.select = 0;
    regsPtr->sbc.write.trgtCmd = 0;
    *modePtr &= ~SBC_MR_DMA;

    /*
     * Check against a continuously busy bus.  This stupid condition would
     * fool the code below that tries to select a device.
     *
     * FIXME: change to use WaitReg routine.
     */
    for (i=0 ; i < SCSI_WAIT_LENGTH ; i++) {
	if ((regsPtr->sbc.read.curStatus & SBC_CBSR_BSY) == 0) {
	    break;
	} else {
	    MACH_DELAY(10);
	}
    }
    if (i == SCSI_WAIT_LENGTH) {
	if (devSCSI3Debug > 0) {
	    PrintRegs(regsPtr);
	    panic("SCSI3Command: bus stuck busy\n");
	} else {
	    printf("SCSI3Command: bus stuck busy\n");
	}
	DevSCSI3Reset(scsiPtr);
	return(FAILURE);
    }

    /*
     * check for target attempting a reselection -- which shouldn't happen!
     */
    if ((regsPtr->sbc.read.curStatus & SBC_CBSR_SEL) && 
	(regsPtr->sbc.read.curStatus & SBC_CBSR_IO) &&
	(regsPtr->sbc.read.data & SI_HOST_ID)) {
	panic("SCSI3Command: someone attempted to reselect.\n");
    }
    
    /*
     * Select the device.  We output the ID of the host, then then ID
     * of the target (unlike the "standard" SCSI interface).
     * In each case, the ID is put in the data register,
     * the SELECT bit is set, and we wait until the device responds
     * by setting the BUSY bit.
     * The outer loop is to keep turning on the SBC_MR_ARB bit; don't
     * yet know if that's important.
     */
    regsPtr->sbc.write.data = SI_HOST_ID;

    for (i = 0; i < SBC_NUM_RETRIES; i++) {
	*modePtr |= SBC_MR_ARB;
	status = DevSCSI3WaitReg(regsPtr,
			       (Address) &regsPtr->sbc.read.initCmd,
			       REG_BYTE, SBC_ICR_AIP, NO_RESET, ACTIVE_HIGH);
	if (status == DEV_TIMEOUT) {
	    continue;
	}
	if (status != SUCCESS) {
	    regsPtr->sbc.write.data = 0;
	    if (command != SCSI_TEST_UNIT_READY) {
		printf("SCSI-%d: can't select slave %d\n", 
				     scsiPtr->number, targetID);
	    }
	    return(status);
	}
	/*
	 * Confirm that we "won" arbitration.
	 */
	MACH_DELAY(SI_ARBITRATION_DELAY);
	if (((regsPtr->sbc.read.initCmd & SBC_ICR_LA) == 0) &&
	    ((regsPtr->sbc.read.data & ~SI_HOST_ID)  < SI_HOST_ID)) {
	    initCmd = regsPtr->sbc.read.initCmd & ~SBC_ICR_AIP;
	    regsPtr->sbc.write.initCmd = initCmd | SBC_ICR_ATN;
	    initCmd = regsPtr->sbc.read.initCmd & ~SBC_ICR_AIP;
	    regsPtr->sbc.write.initCmd = initCmd | SBC_ICR_SEL;
	    MACH_DELAY(SI_BUS_CLEAR_DELAY + SI_BUS_SETTLE_DELAY);
	    break;
	}
	/*
	 * Lost arbitration.  (Should this ever happen??)
	 */
	printf("SCSI3Command: lost arbitration");
	*modePtr &= ~SBC_MR_ARB;
    }
    if (i == SBC_NUM_RETRIES) {
	DevSCSI3Reset(scsiPtr);
	printf("SCSI-%d: unable to select slave\n", targetID);
	return(FAILURE);	
    }

    /*
     * Select the target by putting its ID plus our own on the bus
     * and waiting for the busy signal.
     */
    regsPtr->sbc.write.data = (1 << targetID) | SI_HOST_ID;
    initCmd = regsPtr->sbc.read.initCmd   & ~SBC_ICR_AIP;
    MACH_DELAY(1);
    regsPtr->sbc.write.initCmd = initCmd | SBC_ICR_DATA | SBC_ICR_BUSY;
    *modePtr &= ~SBC_MR_ARB;

#ifdef notdef
	regsPtr->sbc.write.initCmd &= ~SBC_ICR_BUSY;
#else notdef
        *initCmdPtr  &= ~SBC_ICR_BUSY;
#endif notdef	
    status = DevSCSI3WaitReg(regsPtr,
			   (Address) &regsPtr->sbc.read.curStatus,
			   REG_BYTE, SBC_CBSR_BSY, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	if (command != SCSI_TEST_UNIT_READY) {
	    printf("SCSI3Comand can't select slave %d on SCSI bus %d\n", 
				 targetID, scsiPtr->number);
	}
	regsPtr->sbc.write.data = 0;
	return(status);
    }
        
    regsPtr->sbc.write.initCmd &= ~(SBC_ICR_SEL | SBC_ICR_DATA);

    /*
     * Wait for the target to REQUEST a command.
     */

    regsPtr->sbc.write.trgtCmd = PHASE_MSG_OUT;
    status = DevSCSI3WaitPhase(regsPtr, PHASE_MSG_OUT, RESET);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 3) {
	    printf("SCSI3Command: wait on PHASE_MSG_OUT failed.\n");
	}
	return(status);
    }
    regsPtr->sbc.write.trgtCmd = TCR_MSG_OUT;
    status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    printf("SCSI3: wait on request prior to ID failed.\n");
	}
	return(status);
    }

    regsPtr->sbc.write.data = SCSI_IDENTIFY | scsiPtr->devPtr->LUN;
    regsPtr->sbc.write.initCmd = SBC_ICR_DATA;
    regsPtr->sbc.write.initCmd |= SBC_ICR_ACK;
    status =  DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    printf("SCSI3: wait on REQ line to go low failed.\n");
	}
	return(status);
    }	

    regsPtr->sbc.write.initCmd = 0;
    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;


    if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	if (devSCSI3Debug > 0) {
	    panic("SCSI3Command: bus error");
	} else {
	    printf("SCSI3Command: bus error\n");
	}
	DevSCSI3Reset(scsiPtr);
	return(DEV_DMA_FAULT);
    }
    if (scsiPtr->devPtr->dmaState != SBC_DMA_INACTIVE) {
	/*
	 * If the DMA is still active we have to reset it before
	 * mucking about.  Otherwise the DMA master will complain
	 * if we touch the wrong registers.
	 */
	if (scsiPtr->onBoard) {
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_COMMAND;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_CMD_RESET;
	    MACH_DELAY(SI_UDC_WAIT);
	} 

	regsPtr->control &= ~SI_CSR_FIFO_RES;    
	regsPtr->control |= SI_CSR_FIFO_RES;    
	if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
	    regsPtr->control &= ~SI_CSR_SEND;
	} else {
	    regsPtr->control |= SI_CSR_SEND;
	}
#ifdef notdef
	regsPtr->control |= SI_CSR_BPCON;
#endif
	if (scsiPtr->onBoard) {
	    register DevUDCDMAtable *udct = scsiPtr->udcDmaTable;
	    if (devSCSI3Debug > 4) {
		printf("SCSI DMA addr = 0x%x size = %d\n",addr,size);
	    }
	    regsPtr->fifoCount = size;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_COMMAND;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_CMD_RESET;
	    MACH_DELAY(SI_UDC_WAIT);

	    regsPtr->control &= ~SI_CSR_FIFO_RES;    
	    regsPtr->control |= SI_CSR_FIFO_RES;    
	    udct->haddr = (((unsigned) addr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	    udct->laddr = (unsigned)addr & 0xffff;
	    udct->hcmr = UDC_CMR_HIGH;
	    udct->count = size / 2; /* #bytes -> #words */

	    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
		    udct->rsel = UDC_RSEL_RECV;
		    udct->lcmr = UDC_CMR_LRECV;
	    } else {
		    udct->rsel = UDC_RSEL_SEND;
		    udct->lcmr = UDC_CMR_LSEND;
		    if (size & 1) {
			    udct->count++;
		    }
	    }

	    /* initialize udc chain address register */
	    regsPtr->udcRaddr = UDC_ADR_CAR_HIGH;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = ((int)udct & 0xff0000) >> 8;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_CAR_LOW;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = (int)udct & 0xffff;
    
    
	    /* initialize udc master mode register */
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_MODE;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_MODE;
    
	    /* issue channel interrupt enable command, in case of error, 
	     * to udc */
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_COMMAND;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_CMD_CIE;

	} else {
	    regsPtr->fifoCount = 0;
	    regsPtr->dmaCount = 0;
	    regsPtr->fifoCountHigh = 0;
	    /*
	     * reset again??
	     */
	    regsPtr->control &= ~SI_CSR_FIFO_RES;    
	    regsPtr->control |= SI_CSR_FIFO_RES;
	    if (scsiPtr->devPtr->dmaState != SBC_DMA_INACTIVE) {
		regsPtr->dmaAddress = (int)(addr - VMMACH_DMA_START_ADDR);
	    } else {
		regsPtr->dmaAddress = 0;
	    }
	}
    } else {
	regsPtr->fifoCount = 0;
    }
    if (interrupt == INTERRUPT) {
	 regsPtr->control |= SI_CSR_INTR_EN;
    } else {
	regsPtr->control &= ~SI_CSR_INTR_EN;
    }

    if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	if (devSCSI3Debug > 0) {
	    panic("SCSI3Command: bus error\n");
	} else {
	    printf("SCSI3Command: bus error\n");
	}
	regsPtr->sbc.write.select = SI_HOST_ID;
	DevSCSI3Reset(scsiPtr);
	return(DEV_DMA_FAULT);
    }

    status = DevSCSI3WaitPhase(regsPtr, PHASE_COMMAND, RESET);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 0) {
	    printf("SCSI3: wait on PHASE_COMMAND failed.\n");
	}
	return(status);
    }
    /*
     * Stuff the control block through the commandStatus register.
     * The handshake on the SCSI bus is visible here:  we have to
     * wait for the Request line on the SCSI bus to be raised before
     * we can send the next command byte to the controller.  All commands
     * are of "group 0" which means they are 6 bytes long.
     */

    
    regsPtr->sbc.write.trgtCmd = TCR_COMMAND;
    charPtr = (char *)&scsiPtr->controlBlock;
    for (i=0 ; i<sizeof(DevSCSIControlBlock) ; i++) {
	status = DevSCSI3WaitReg(regsPtr,
			       (Address) &regsPtr->sbc.read.curStatus,
			       REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
	if (status != SUCCESS) {
	    printf("SCSI3Command: bus-%d: couldn't send command block (i=%d)\n",
				 scsiPtr->number, i);
	    return(status);
	}
	regsPtr->sbc.write.data = *charPtr;
	regsPtr->sbc.write.initCmd = SBC_ICR_DATA;

	if (! (regsPtr->sbc.read.status & SBC_BSR_PMTCH)) {
	    PrintRegs(regsPtr);
	    panic("DevSCSI3Command: Phase mismatch.\n");
	    DevSCSI3Reset(scsiPtr);
	    return(FAILURE);
	}
	
	regsPtr->sbc.write.initCmd |= SBC_ICR_ACK;
	status = DevSCSI3WaitReg(regsPtr,
			       (Address) &regsPtr->sbc.read.curStatus,
			       REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
	if (status != SUCCESS) {
	    printf("SCSI3Command: bus-%d: request line didn't go low.\n",
				 scsiPtr->number);
	    return(status);
	}
	charPtr++;
	if (i < sizeof(DevSCSIControlBlock) - 1) {
	    regsPtr->sbc.write.initCmd = 0;
	}
    }

    i = regsPtr->sbc.read.clear;
    if (interrupt == INTERRUPT) {
	regsPtr->sbc.write.select = SI_HOST_ID;
	regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
	*modePtr |= SBC_MR_DMA;
	regsPtr->sbc.write.initCmd = 0;
	if (!scsiPtr->onBoard) {
	    regsPtr->control |= SI_CSR_DMA_EN;
	}
    } else {
	regsPtr->sbc.write.initCmd = 0;
    }
    /*
     * Initialize the residual field to the number of bytes to be
     * transferred, since StartDMA uses this when called from here or
     * from the interrupt handler.
     */
    scsiPtr->residual = size;
    if (interrupt == WAIT) {
	/*
	 * A synchronous command.  Wait here for the command to complete.
	 */
	if (scsiPtr->devPtr->dmaState != SBC_DMA_INACTIVE) {
	    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
		phase = PHASE_DATA_IN;
	    } else {
		phase = PHASE_DATA_OUT;
	    }
	    status = DevSCSI3WaitPhase(regsPtr, phase, NO_RESET);
	    if (status != SUCCESS) {
		printf("Warning: %s",
		      "SCSI3Command: wait on PHASE_DATA_{IN,OUT} failed.\n");
		DevSCSI3Reset(scsiPtr);
		return(status);
	    }
	    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
	    StartDMA(scsiPtr);
	}

	status = DevSCSI3CommandWait(scsiPtr);
	if (status == SUCCESS) {
	    if (scsiPtr->onBoard) {
		scsiPtr->residual = regsPtr->fifoCount;
	    } else {
		scsiPtr->residual = regsPtr->dmaCount;
	    }
	    status = DevSCSI3Status(scsiPtr);
	} else {
	    printf("SCSI3Command bus-%d: couldn't wait for command\n",
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
 * StartDMA --
 *
 *	Issue the sequence of commands to the controller to start DMA.
 *	This can be called by DevSCSI3Command if the DMA is to be done
 *	using polling, or by Dev_SCSI3Intr in response to a DATA_{IN,OUT}
 *	phase message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DMA is enabled.  No registers other than the control register are
 *	to be accessed until DMA is disabled again.
 *
 *----------------------------------------------------------------------
 */
static void
StartDMA(scsiPtr)
    DevSCSIController *scsiPtr;
{
    register DevSCSI3Regs *regsPtr;
    unsigned char junk;

    if (devSCSI3Debug > 4) {
	printf("SCSI3: StartDMA %s called.\n",
	    (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) ? "receive" :
		((scsiPtr->devPtr->dmaState == SBC_DMA_SEND) ? "send" :
							"not-active!"));
    }
    regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    if (scsiPtr->onBoard) {
	MACH_DELAY(SI_UDC_WAIT);
	regsPtr->udcRdata = UDC_CMD_STRT_CHN;
    } else {
        regsPtr->dmaCount = scsiPtr->residual;
    }

    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
	regsPtr->sbc.write.trgtCmd = TCR_DATA_IN;
	junk = regsPtr->sbc.read.clear;
	regsPtr->sbc.write.mode |= SBC_MR_DMA;
	regsPtr->sbc.write.initRecv = 0;
    } else {
	regsPtr->sbc.write.trgtCmd = TCR_DATA_OUT;
	junk = regsPtr->sbc.read.clear;
#ifdef lint
	regsPtr->sbc.read.clear = junk;
#endif
	regsPtr->sbc.write.initCmd = SBC_ICR_DATA;
	regsPtr->sbc.write.mode |= SBC_MR_DMA;
	regsPtr->sbc.write.send = 0;
    }
    if (!scsiPtr->onBoard) {
	regsPtr->control |= SI_CSR_DMA_EN;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3Status --
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
DevSCSI3Status(scsiPtr)
    DevSCSIController *scsiPtr;
{
    register ReturnStatus status;
    register DevSCSI3Regs *regsPtr;
    char message;
    char statusByte;
    char *statusBytePtr;
    int numStatusBytes = 0;

    if (devSCSI3Debug > 3) {
	printf("SCSI3: DevSCSI3Status called.\n");
    }
    regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    statusBytePtr = (char *)&scsiPtr->statusBlock;
    bzero((Address)statusBytePtr,sizeof(DevSCSIStatusBlock));

    /*
     * The SCSI-3 interface is more explicit (and more complicated)
     * than the old SCSI interface.  It requires that we wait on the
     * STATUS phase, then keep waiting for STATUS phases to get data,
     * and finally get the MSG_IN phase and SCSI_COMMAND_COMPLETE message
     * to complete the transfer.
     */
    status = DevSCSI3WaitPhase(regsPtr, PHASE_STATUS, RESET);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 3) {
	    printf("SCSI3: wait on PHASE_STATUS failed.\n");
	}
	return(status);
    }
    for ( ; ; ) {
	status = GetByte(scsiPtr, PHASE_STATUS, &statusByte);
	if (status != SUCCESS) {
	    if (devSCSI3Debug > 4) {
	        printf("SCSI3-%d: got error %x after %d status bytes\n",
				 scsiPtr->number, status, numStatusBytes);
	    }
	    break;
	}
	if (numStatusBytes < sizeof(DevSCSIStatusBlock)) {
	    *statusBytePtr = statusByte;
	    statusBytePtr++;
	}
	numStatusBytes++;
    }
    status = DevSCSI3WaitPhase(regsPtr, PHASE_MSG_IN, RESET);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    printf("SCSI3: GetStatus: wait on PHASE_MSG_IN failed.\n");
	}
	return(status);
    } else {
	status = GetByte(scsiPtr, PHASE_MSG_IN, &message);
	if (status != SUCCESS) {
	    if (devSCSI3Debug > 1) {
	        printf("SCSI3-%d: got error %x getting message.\n",
				 scsiPtr->number, status);
	    }
	    return(status);
	}
	if (message != SCSI_COMMAND_COMPLETE) {
	    panic("Message from SCSI3 is not command complete.\n");
	    return(FAILURE);
	}
	regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
	/*
	 * Other status information may be available.  It is obtained by
	 * another SCSI3 command that uses DMA to transfer the sense data.
	 */
	if (scsiPtr->statusBlock.check) {
	    if (devSCSI3Debug > 7) {
		PrintRegs(regsPtr);
		panic("Entering breakpoint prior to DevSCSIRequestSense.\n");
	    }
	    status = DevSCSIRequestSense(scsiPtr, scsiPtr->devPtr);
	}
	if (scsiPtr->statusBlock.error) {
	    printf("SCSI3-%d: host adaptor error bit set\n",
				 scsiPtr->number);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3CommandWait --
 *
 *	Wait for a command to complete.  This requires waiting for any
 *	DMA to finish, then checking the interrupt and error condition
 *	bits.
 *
 * Results:
 *	SUCCESS if the command completed without error within a threshold
 *	time limit, DEV_TIMEOUT or another error condition (such as
 *	bus error) otherwise.
 *
 * Side effects:
 *	This resets the SCSI bus if an error occurs.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSI3CommandWait(scsiPtr)
    DevSCSIController *scsiPtr;
{
    DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    ReturnStatus status;
    unsigned int offset;
    int foo;

    if (devSCSI3Debug > 4) {
	printf("DevSCSI3CommandWait: waiting for command 0x%x to complete.\n",
	    scsiPtr->command);
    }
    if (scsiPtr->devPtr->dmaState == SBC_DMA_INACTIVE) {
	return(SUCCESS);
    }
    status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->control, REG_SHORT,
			   SI_CSR_DMA_ACTIVE, RESET, ACTIVE_LOW);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    PrintRegs(regsPtr);
	    panic("DevSCSI3CommandWait: DMA did not complete.\n");
	}
	DevSCSI3Reset(scsiPtr);
	return(status);
    }
    status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->control, REG_SHORT,
			   SI_CSR_DMA_IP | SI_CSR_SBC_IP,
			   RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    PrintRegs(regsPtr);
	    panic("DevSCSI3CommandWait: DMA did not interrupt.\n");
	}
	DevSCSI3Reset(scsiPtr);
	return(status);
    }
    /*
     * Disable DMA and then check condition bits.
     */
    if (!scsiPtr->onBoard) {
	regsPtr->control &= ~SI_CSR_DMA_EN;
    }
#ifdef notdef
    if ((regsPtr->control & (SI_CSR_SBC_IP) == 0) {
	if (devSCSI3Debug > 0) {
	    PrintRegs(regsPtr);
	}
	if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	    if (devSCSI3Debug > 0) {
		panic("SCSI3CommandWait: bus error\n");
	    } else {
		printf("SCSI3CommandWait: bus error\n");
	    }
	    status = DEV_DMA_FAULT;
	} else if (regsPtr->control & SI_CSR_DMA_CONFLICT) {
	    if (devSCSI3Debug > 0) {
		panic("SCSI3: DMA register conflict\n");
	    } else {
		printf("SCSI3: DMA register conflict\n");
	    }
	    status = DEV_DMA_FAULT;
	} else {
	    if (devSCSI3Debug > 0) {
		panic("DevSCSI3CommandWait: didn't get interrupt.\n");
	    } else {
		printf("DevSCSI3CommandWait: didn't get interrupt.\n");
	    }
	    status = FAILURE;
	}
	DevSCSI3Reset(scsiPtr);
	return(status);
    }
#endif
    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
	if (!scsiPtr->onBoard) { 
	    if ((regsPtr->control & SI_CSR_LOB) != 0) {
	    /*
	     * On a read the last odd byte is left in the byte pack
	     * register.  Note: this assumes "wordmode" transfers rather than
	     * longwords.
	     * FIXME?
	     * Without documentation it's not clear which byte this is!
	     * The scsi driver  just writes to the dmaAddress, but is this
	     * incremented on the scsi-3?
	     */ 
	    *(char *) (regsPtr->dmaAddress + VMMACH_DMA_START_ADDR) = 
		    (regsPtr->bytePack & 0x0000ff00) >> 8;
	    }
	} else {
	    regsPtr->udcRaddr = UDC_ADR_COUNT;

	    /* wait for the fifo to empty */
	   status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->control, 
			REG_SHORT,SI_CSR_FIFO_EMPTY, RESET, ACTIVE_HIGH);
	   if (status != SUCCESS) {
		panic("DevSCSI3CommandWait:  fifo never emptied\n");
	   }
#ifdef notdef
	    /*
	     * Didn't transfer any data.
	     * "Just say no" and leave, rather than
	     * erroneously executing left over byte code.
	     * The bcr + 1 above wards against 5380 prefetch.
	     */
	    if ((regsPtr->fifoCount == size)  ||
		(regsPtr->fifoCount + 1 == size) {
		    goto out;
	    }
	    /* handle odd byte */
	    offset = regsPtr->dmaAddress + (size - regsPtr->fifoCount);
	    if ((size - regsPtr->fifoCount) & 1) {
		    DVMA[offset - 1] = (regsPtr->fifoData & 0xff00) >> 8;

	    /*
	     * The udc may not dma the last word from the fifo_data
	     * register into memory due to how the hardware turns
	     * off the udc at the end of the dma operation.
	     */
	    } else if (((regsPtr->udcRdata*2) - regsPtr->fifoCount) == 2) {
		    DVMA[offset - 2] = (regsPtr->fifoData & 0xff00) >> 8;
		    DVMA[offset - 1] = regsPtr->fifoData & 0x00ff;
	    }
#endif
	}
    }
    /*
     * Reset the state of the transfer.  This can (and should) be put in
     * a separate routine if it is going to be used elsewhere.
     * (FIXME)
     */
    foo = regsPtr->sbc.read.clear;
#ifdef lint
    regsPtr->sbc.read.clear = foo;
#endif
    regsPtr->dmaCount = 0;
    regsPtr->dmaAddress = 0;
    regsPtr->control &= ~SI_CSR_FIFO_RES;
    regsPtr->control |= SI_CSR_FIFO_RES;
    if (!scsiPtr->onBoard) {
	regsPtr->control &= ~SI_CSR_DMA_EN;
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3WaitReg --
 *
 *	Wait for any of a set of bits to be enabled 
 *	in the specified register.  The generic regsPtr pointer is
 *	passed in so this routine can check for bus and parity errors.
 *	A pointer to the register to check, and an indicator of its type
 *	(its size) are passed in as well.  Finally, the conditions
 * 	can be awaited to become 1 or 0.
 *
 * Results:
 *	SUCCESS if the condition occurred before a threshold time limit,
 *	DEV_TIMEOUT otherwise.
 *
 * Side effects:
 *	This resets the SCSI bus if the reset parameter is true and
 *	the condition bits are not set by the controller before timeout.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSI3WaitReg(regsPtr, thisRegPtr, type, conditions, reset, bitSel)
    DevSCSI3Regs *regsPtr;	/* pointer to controller registers */
    Address thisRegPtr;		/* pointer to register to check */
    RegType type;		/* "type" of the register */
    unsigned int conditions;	/* one or more bits to check */
    Boolean reset;		/* whether to reset the bus on error */
    BitSelection bitSel;	/* check for all or some bits going to 1/0 */
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register unsigned int thisReg;

    for (i=0 ; i<SCSI_WAIT_LENGTH ; i++) {
	switch (type) {
	    case REG_BYTE: {
		unsigned char *charPtr = (unsigned char *) thisRegPtr;
		thisReg = (unsigned int) *charPtr;
		break;
	    }
	    case REG_SHORT: {
		unsigned short *shortPtr = (unsigned short *) thisRegPtr;
		thisReg = (unsigned int) *shortPtr;
		break;
	    }
	    default: {
		panic("SCSI3: GetByte: unknown type.\n");
		break;
	    }
	}
	    
	if (devSCSI3Debug > 5 && i < 5) {
	    printf("%d/%x ", i, thisReg);
	}
	switch(bitSel) {
	    case ACTIVE_HIGH: {
		if ((thisReg & conditions) != 0) {
		    return(SUCCESS);
		}
		break;
	    }
	    case ACTIVE_ALL: {
		if ((thisReg & conditions) == conditions) {
		    return(SUCCESS);
		}
		break;
	    }
	    case ACTIVE_LOW: {
		if ((thisReg & conditions) != conditions) {
		    return(SUCCESS);
		}
		break;
	    }
	    case ACTIVE_NONE: {
		if ((thisReg & conditions) == 0) {
		    return(SUCCESS);
		}
		break;
	    }
	    default: {
		panic("SCSI3: bit selector: unknown type: %d.\n",
			  (int) bitSel);
		break;
	    }
	} 
	if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	    if (devSCSI3Debug > 0) {
	        panic("SCSI3Wait: bus error\n");
	    } else {
		printf("SCSI3Wait: bus error\n");
	    }
	    status = DEV_DMA_FAULT;
	    break;
#ifdef notdef
	} else if (regsPtr->sbc.read.status & SBC_BSR_PERR) {
	    if (devSCSI3Debug > 0) {
	        panic("SCSI3: parity error\n");
	    } else {
		printf("SCSI3: parity error\n");
	    }
	    status = DEV_DMA_FAULT;
	    break;
#endif
	}
	MACH_DELAY(10);
    }
    if (devSCSI3Debug > 1) {
	printf("DevSCSI3WaitReg: timed out.\n");
	PrintRegs(regsPtr);
	printf("DevSCSI3WaitReg: was checking %x for condition(s) %x to go ",
		   (int) thisRegPtr, (int) conditions);
	switch(bitSel) {
	    case ACTIVE_HIGH: {
		printf("ACTIVE_HIGH.\n");
		break;
	    }
	    case ACTIVE_ALL: {
		printf("ACTIVE_ALL.\n");
		break;
	    }
	    case ACTIVE_LOW: {
		printf("ACTIVE_LOW.\n");
		break;
	    }
	    case ACTIVE_NONE: {
		printf("ACTIVE_NONE.\n");
		break;
	    }
	} 
	
    }
#ifdef notdef
    if (reset) {
	DevSCSI3Reset(scsiPtr);
    }
#endif
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3WaitPhase --
 *
 *	Wait for a phase to be signalled in the controller registers.
 * 	This is a specialized version of DevSCSI3WaitReg, which compares
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
ReturnStatus
DevSCSI3WaitPhase(regsPtr, phase, reset)
    DevSCSI3Regs *regsPtr;	/* pointer to controller registers */
    unsigned char phase;	/* phase to check */
    Boolean reset;		/* whether to reset the bus on error */
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register unsigned char thisReg;

    for (i=0 ; i<SCSI_WAIT_LENGTH ; i++) {
	thisReg = regsPtr->sbc.read.curStatus;
	if (devSCSI3Debug > 5 && i < 5) {
	    printf("%d/%x ", i, thisReg);
	}
	if ((thisReg & CBSR_PHASE_BITS) == phase) {
	    return(SUCCESS);
	}
	if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	    if (devSCSI3Debug > 0) {
	        panic("SCSI3WaitPhase: bus error\n");
	    } else {
		printf("SCSI3WaitPhase: bus error\n");
	    }
	    status = DEV_DMA_FAULT;
	    break;
#ifdef notdef
	} else if (regsPtr->sbc.read.status & SBC_BSR_PERR) {
	    if (devSCSI3Debug > 0) {
	        panic("SCSI3: parity error\n");
	    } else {
		printf("SCSI3: parity error\n");
	    }
	    status = DEV_DMA_FAULT;
	    break;
#endif
	}
	MACH_DELAY(10);
    }
    if (devSCSI3Debug > 4) {
	printf("DevSCSI3WaitPhase: timed out.\n");
	PrintRegs(regsPtr);
	printf("DevSCSI3WaitPhase: was checking for phase %x.\n",
		   (int) phase);
    }
#ifdef notdef
    if (reset) {
	DevSCSI3Reset(scsiPtr);
    }
#endif
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * GetByte --
 *
 *	Get a byte from the SCSI bus, corresponding to the specified phase.
 * 	This entails waiting for a request, checking the phase, reading
 *	the byte, and sending an acknowledgement.
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
GetByte(scsiPtr, phase, charPtr)
    DevSCSIController *scsiPtr;
    unsigned short phase;
    char *charPtr;    
{
    register DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    ReturnStatus status;
    
    status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	return(status);
    }
    if ((regsPtr->sbc.read.curStatus & CBSR_PHASE_BITS) != phase) {
	if (devSCSI3Debug > 4) {
	    printf("SCSI3: GetByte: wanted phase %x, got phase %x in curStatus %x.\n",
		       phase, regsPtr->sbc.read.curStatus & CBSR_PHASE_BITS,
		       regsPtr->sbc.read.curStatus);
	}
	/*
	 * Use the "handshake error" to signal a new phase.  This should
	 * be propagated into a new DEV status to signal a "condition" that
	 * isn't an "error".
	 */
	return(DEV_HANDSHAKE_ERROR);
    }
    *charPtr = regsPtr->sbc.read.data;
    regsPtr->sbc.write.initCmd = SBC_ICR_ACK;
    status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
    if (status != SUCCESS) {
	panic("SCSI3: GetByte: request line didn't go low.\n");
	return(status);
    }
    /*
     * Here's another case where without documentation, all I can do is
     * mimic you-know-who.  It's not clear why we might have to disable
     * DMA atomically with setting the initCmd register to 0, but *they*
     * do it....
     */
    if ((phase == PHASE_MSG_IN) && (*charPtr == SCSI_COMMAND_COMPLETE)) {
	regsPtr->sbc.write.initCmd = 0;
	regsPtr->sbc.write.mode &= ~SBC_MR_DMA;
    } else {
        regsPtr->sbc.write.initCmd = 0;
    }
    return(SUCCESS);
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
    register DevSCSI3Regs *regsPtr;
{
    printf("ctl %x addr %x dmaCount %x fifoCount %x%x data %x\n\tinitCmd %x mode %x target %x curStatus %x status %x\n", 
	       regsPtr->control,
	       regsPtr->dmaAddress,
	       regsPtr->dmaCount,
	       regsPtr->fifoCountHigh,
	       regsPtr->fifoCount,
	       regsPtr->sbc.read.data,
	       regsPtr->sbc.read.initCmd, 
	       regsPtr->sbc.read.mode,
	       regsPtr->sbc.read.trgtCmd, 
	       regsPtr->sbc.read.curStatus,
	       regsPtr->sbc.read.status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI3Intr --
 *
 *	Handle interrupts from the SCSI3 controller.
 *	The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
 *
 * Results:
 *	TRUE if an SCSI3 controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevSCSI3Intr(scsiPtr)
    DevSCSIController *scsiPtr;
{
    volatile DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
    ReturnStatus status;
    int byteCount;
    unsigned char foo;
    unsigned char phase;
    unsigned char message;

    if (devSCSI3Debug > 6) {
	printf("Entering DevSCSI3Intr.\n");
    }
    
    /*
     * First, disable dma_enable or else we'll get register conflicts.
     */
    if (!scsiPtr->onBoard) {
	regsPtr->control &= ~SI_CSR_DMA_EN;
    }
    byteCount = regsPtr->fifoCount;
    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
    
    if (regsPtr->control & (SI_CSR_SBC_IP | SI_CSR_DMA_IP)) {
	if (regsPtr->control &
	    (SI_CSR_DMA_BUS_ERR | SI_CSR_DMA_CONFLICT)) {
	    if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
		/*
		 * A Bus Error.  Complete the I/O but flag an error.
		 * The residual is computed because the Bus Error could
		 * have occurred after a number of sectors.
		 */
		scsiPtr->residual = byteCount;
		if (devSCSI3Debug > 4) {
		    panic("DevSCSI3Intr: bus error\n");
		} else {
		    printf("DevSCSI3Intr: bus error\n");
		}
	    } else if (regsPtr->control & SI_CSR_DMA_CONFLICT) {
		if (devSCSI3Debug > 0) {
		    panic("SCSI3: DMA register conflict\n");
		} else {
		    printf("SCSI3: DMA register conflict\n");
		}
	    }
	    /*
	     * The board needs to be reset to clear the Bus Error
	     * condition so no status bytes are grabbed.
	     */
	    DevSCSI3Reset(scsiPtr);
	    scsiPtr->status = DEV_DMA_FAULT;
	    scsiPtr->flags |= SCSI_IO_COMPLETE;
	    scsiPtr->devPtr->dmaState = SBC_DMA_INACTIVE;
	    Sync_MasterBroadcast(&scsiPtr->IOComplete);
	    return(TRUE);
	} else if (regsPtr->control & SI_CSR_DMA_IP) {
	    if (devSCSI3Debug > 4) {
		printf("DevSCSI3Intr: DMA Interrupt\n");
	    }
	    foo = regsPtr->sbc.read.clear;
	    scsiPtr->residual = regsPtr->dmaCount;
	    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE &&
		(regsPtr->control & SI_CSR_LOB) != 0) {
		/*
		 * On a read the last odd byte is left in the byte
		 * pack register.  Note: this assumes "wordmode"
		 * transfers rather than longwords.
		 * FIXME?
		 * Without documentation it's not clear which byte
		 * this is!  The scsi driver just writes to the
		 * dmaAddress, but is this incremented on the
		 * scsi-3?
		 */ 
		*(char *) (regsPtr->dmaAddress + VMMACH_DMA_START_ADDR) = 
			(regsPtr->bytePack & 0x0000ff00) >> 8;
	    }
	    scsiPtr->status = DevSCSI3Status(scsiPtr);
	    scsiPtr->flags |= SCSI_IO_COMPLETE;
	    Sync_MasterBroadcast(&scsiPtr->IOComplete);
	    return(TRUE);
	} else {
	    /*
	     * Normal command completion.  Acknowledge the interrupt and
	     * check for the new phase.  
	     */
    
	    foo = regsPtr->sbc.read.clear;
#ifdef lint		
	    regsPtr->sbc.read.clear = foo;
#endif
	    phase = regsPtr->sbc.read.curStatus & CBSR_PHASE_BITS;
	    switch (phase) {
		case PHASE_DATA_IN:
		case PHASE_DATA_OUT: {
		    if (devSCSI3Debug > 4) {
			printf("DevSCSI3Intr: Data Phase Interrupt\n");
		    }
		    regsPtr->sbc.write.mode &= ~SBC_MR_DMA;
		    StartDMA(scsiPtr);
		    return(TRUE);
		}
		case PHASE_MSG_IN: {
		    status = GetByte(scsiPtr, PHASE_MSG_IN,
				     (char *)&message);
		    if (status != SUCCESS) {
			if (devSCSI3Debug > 0) {
			    panic("SCSI3Intr: couldn't get message.\n");
			} else {
			    printf("SCSI3Intr: couldn't get message.\n");
			}
			if (!scsiPtr->onBoard) {
			    regsPtr->control |= SI_CSR_DMA_EN;
			}
			return(TRUE);
		    }
		    if (message != SCSI_COMMAND_COMPLETE) {
			if (devSCSI3Debug > 0) {
			    panic( "SCSI3Intr: couldn't handle message.\n");
			} else {
			    printf("SCSI3Intr: couldn't handle message.\n");
			}
			if (!scsiPtr->onBoard) {
			    regsPtr->control |= SI_CSR_DMA_EN;
			}
		    }
		    return(TRUE);
		}
		case PHASE_STATUS: {
		    if (scsiPtr->onBoard) {
			scsiPtr->residual = regsPtr->fifoCount;
		    } else {
			scsiPtr->residual = regsPtr->dmaCount;
		    }
		    if (scsiPtr->devPtr->dmaState == SBC_DMA_RECEIVE) {
			if (!scsiPtr->onBoard) { 
			    if ((regsPtr->control & SI_CSR_LOB) != 0) {
			    /*
			     * On a read the last odd byte is left in the byte pack
			     * register.  Note: this assumes "wordmode" transfers rather than
			     * longwords.
			     * FIXME?
			     * Without documentation it's not clear which byte this is!
			     * The scsi driver  just writes to the dmaAddress, but is this
			     * incremented on the scsi-3?
			     */ 
			    *(char *) (regsPtr->dmaAddress + VMMACH_DMA_START_ADDR) = 
				    (regsPtr->bytePack & 0x0000ff00) >> 8;
			    }
			} else {
			    regsPtr->udcRaddr = UDC_ADR_COUNT;
		
			    /* wait for the fifo to empty */
			   status = DevSCSI3WaitReg(regsPtr, (Address) &regsPtr->control, 
					REG_SHORT,SI_CSR_FIFO_EMPTY, RESET, ACTIVE_HIGH);
			   if (status != SUCCESS) {
				panic("DevSCSI3CommandWait:  fifo never emptied\n");
			   }
#ifdef notdef		    
			    /*
			     * Didn't transfer any data.
			     * "Just say no" and leave, rather than
			     * erroneously executing left over byte code.
			     * The bcr + 1 above wards against 5380 prefetch.
			     */
			    if ((regsPtr->fifoCount == size)  ||
				(regsPtr->fifoCount + 1 == size)
				    goto out;
    
			    /* handle odd byte */
			    offset = addr + (size - regsPtr->fifoCount);
			    if ((size - regsPtr->fifoCount) & 1) {
				    DVMA[offset - 1] = (regsPtr->fifoData & 0xff00) >> 8;
		
			    /*
			     * The udc may not dma the last word from the fifo_data
			     * register into memory due to how the hardware turns
			     * off the udc at the end of the dma operation.
			     */
			    } else if (((regsPtr->udcRdata*2) - regsPtr->fifoCount) == 2) {
				    DVMA[offset - 2] = (regsPtr->fifoData & 0xff00) >> 8;
				    DVMA[offset - 1] = regsPtr->fifoData & 0x00ff;
			    }
#endif
			}
		    }
    
		    scsiPtr->status = DevSCSI3Status(scsiPtr);
		    scsiPtr->flags |= SCSI_IO_COMPLETE;
		    Sync_MasterBroadcast(&scsiPtr->IOComplete);
		    return(TRUE);
		}
		default: {
		    if (devSCSI3Debug > 0) {
			PrintRegs(regsPtr);
			printf("Warning: %s",
			 "Dev_SCSI3Intr: couldn't handle phase %x... ignoring.\n",
				  phase);
		    }
		    if (!scsiPtr->onBoard) {
			regsPtr->control |= SI_CSR_DMA_EN;
		    }
		    return(TRUE);
		}
	    }
	}
    }
    return(FALSE);
}

