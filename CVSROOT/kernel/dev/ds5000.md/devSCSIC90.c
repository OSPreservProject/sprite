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
#include "devInt.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bstring.h"
#include "devSCSIC90.h"
#include "vm.h"

#include "dbg.h"

/*
 *
 *      Definitions for Sun's third(?) variant on the SCSI device interface.
 *	The reference for the 53C90 is the NCR Advanced SCSI Processor
 *	User's Guide, Micro-electronics Division, Revision 2.1, Aug. 1989.
 */

/*
 * The control registers have different definitions depending on whether
 * you are reading or writing them.  The fields require byte accesses, even
 * though they are full-word aligned.
 */
typedef struct ReadRegs {
    unsigned char xCntLo;	/* LSB of transfer counter. */
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    unsigned char xCntHi;	/* MSB of transfer counter. */
    unsigned char pad5;
    unsigned char pad6;
    unsigned char pad7;
    unsigned char FIFO;		/* FIFO. */
    unsigned char pad9;
    unsigned char pad10;
    unsigned char pad11;
    unsigned char command;	/* Command register. */
    unsigned char pad13;
    unsigned char pad14;
    unsigned char pad15;
    unsigned char status;	/* Status register. */
    unsigned char pad17;
    unsigned char pad18;
    unsigned char pad19;
    unsigned char interrupt;	/* Interrupt register. */
    unsigned char pad21;
    unsigned char pad22;
    unsigned char pad23;
    unsigned char sequence;	/* Sequnce step register. */
    unsigned char pad25;
    unsigned char pad26;
    unsigned char pad27;
    unsigned char FIFOFlags;	/* FIFO flags. */
    unsigned char pad29;
    unsigned char pad30;
    unsigned char pad31;
    unsigned char config1;	/* Configuration 1. */
    unsigned char pad33;
    unsigned char pad34;
    unsigned char pad35;
    unsigned char reserved1;	/* NCR reserved. */
    unsigned char pad37;
    unsigned char pad38;
    unsigned char pad39;
    unsigned char reserved2;	/* NCR reserved. */
    unsigned char pad41;
    unsigned char pad42;
    unsigned char pad43;
    unsigned char config2;	/* Configuration 2. */
    unsigned char pad45;
    unsigned char pad46;
    unsigned char pad47;
    unsigned char config3;	/* Configuration 3. */
    unsigned char pad49;
    unsigned char pad50;
    unsigned char pad51;
    unsigned char reserved3;	/* NCR reserved. */
} ReadRegs;

/*
 * The following format applies when writing the registers.
 */
typedef struct WriteRegs {
    unsigned char xCntLo;	/* LSB of transfer counter. */
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    unsigned char xCntHi;	/* MSB of transfer counter. */
    unsigned char pad5;
    unsigned char pad6;
    unsigned char pad7;
    unsigned char FIFO;		/* FIFO. */
    unsigned char pad9;
    unsigned char pad10;
    unsigned char pad11;
    unsigned char command;	/* Command register. */
    unsigned char pad13;
    unsigned char pad14;
    unsigned char pad15;
    unsigned char destID;	/* Destination ID. */
    unsigned char pad17;
    unsigned char pad18;
    unsigned char pad19;
    unsigned char timeout;	/* (Re)selection timeout. */
    unsigned char pad21;
    unsigned char pad22;
    unsigned char pad23;
    unsigned char synchPer;	/* Synchronous period. */
    unsigned char pad25;
    unsigned char pad26;
    unsigned char pad27;
    unsigned char synchOffset;	/* Synchronous offset. */
    unsigned char pad29;
    unsigned char pad30;
    unsigned char pad31;
    unsigned char config1;	/* Configuration 1. */
    unsigned char pad33;
    unsigned char pad34;
    unsigned char pad35;
    unsigned char clockConv;	/* Clock conversion factor. */
    unsigned char pad37;
    unsigned char pad38;
    unsigned char pad39;
    unsigned char test;		/* Test mode. */
    unsigned char pad41;
    unsigned char pad42;
    unsigned char pad43;
    unsigned char config2;	/* Configuration 2. */
    unsigned char pad45;
    unsigned char pad46;
    unsigned char pad47;
    unsigned char config3;	/* Configuration 3. */
    unsigned char pad49;
    unsigned char pad50;
    unsigned char pad51;
    unsigned char resFIFO;	/* Reserve FIFO byte. */
} WriteRegs;

/*
 * The control register layout.
 */
typedef	struct	CtrlRegs {
    union {
	struct	ReadRegs	read;		/* scsi bus ctrl, read reg. */
	struct	WriteRegs	write;		/* scsi bus crl, write reg. */
    } scsi_ctrl;
} CtrlRegs;


/*
 * Control bits in the 53C90 Command Register.
 * The command register is a two deep, 8-bit read/write register.
 * Up to 2 commands may be stacked in the command register.
 * Reset chip, reset SCSI bus and target stop DMA execute immediately,
 * all others wait for the previous command to complete.  Reading the
 * command register has no effect on its contenets.
 */
#define	CR_DMA		0x80	/* If set, command is a DMA instruction. */
#define CR_CMD		0x7F	/* The command code bits [6:0]. */

/* Miscellaneous group. */
#define	CR_NOP		0x00	/* NOP. */
#define	CR_FLSH_FIFO	0x01	/* Flush FIFO. */
#define	CR_RESET_CHIP	0x02	/* Reset chip. */
#define	CR_RESET_BUS	0x03	/* Reset SCSI bus. */
/* Disconnected state group. */
#define	CR_RESLCT_SEQ	0x40	/* Reselect sequence. */
#define	CR_SLCT_NATN	0x41	/* Select without ATN sequence. */
#define	CR_SLCT_ATN	0x42	/* Select with ATN sequence. */
#define	CR_SLCT_ATNS	0x43	/* Select with ATN and stop sequence. */
#define	CR_EN_SLCT	0x44	/* Enable selection/reselection. */
#define	CR_DIS_SLCT	0x45	/* Disable selection/reselection. */
#define	CR_SLCT_ATN3	0x46	/* Select ATN3. */
/* Target state group. */
#define	CR_SEND_MSG	0x20	/* Send message. */
#define	CR_SEND_STATUS	0x21	/* Send status. */
#define	CR_SEND_DATA	0x22	/* Send data. */
#define	CR_DIS_SEQ	0x23	/* Disconnect sequence. */
#define	CR_TERM_SEQ	0x24	/* Terminate sequence. */
#define	CR_TARG_COMP	0x25	/* Target command complete sequence. */
#define	CR_DISCONNECT	0x27	/* Disconnect. */
#define	CR_REC_MSG	0x28	/* Receive message. */
#define	CR_REC_CMD	0x29	/* Receive command sequence. */
#define	CR_REC_DATA	0x2A	/* Receive data. */
#define	CR_TARG_ABRT	0x06	/* Target abort sequence. */
/* #define	CR_REC_CMD	0x2B	Another receive command sequence? */
/* Initiator state group */
#define	CR_XFER_INFO	0x10	/* Transfer information. */
#define	CR_INIT_COMP	0x11	/* Initiator command complete sequence. */
#define	CR_MSG_ACCPT	0x12	/* Message accepted. */
#define	CR_XFER_PAD	0x18	/* Transfer pad. */
#define	CR_SET_ATN	0x1A	/* Set ATN. */
#define	CR_RESET_ATN	0x1B	/* Reset ATN. */

#define C1_SLOW		0x80	/* Slow cable mode. */
#define	C1_REPORT	0x40	/* Disable reporting of interrupts from the
				 * scsi bus reset command. */
#define C1_PARITY_TEST	0x20	/* Enable parity test feature. */
#define C1_PARITY	0x10	/* Enable parity checking. */
#define C1_TEST		0x08	/* Enable chip test mode. */
#define C1_BUS_ID	0x03	/* Bus ID. */

#define C2_RESERVE_FIFO 0x80	/* Reserve fifo byte. */
#define C2_PHASE_LATCH	0x40	/* Enable phase latch. */
#define C2_BYTE_CONTROL	0x20	/* Enable byte control. */
#define C2_DREQ_HIGH	0x10	/* Set the DREQ output to high impedence. */
#define C2_SCSI2	0x08	/* Enable SCSI2 stuff. */
#define C2_BAD_PARITY	0x04	/* Abort if bad parity detected. */
#define C2_REG_PARITY	0x02	/* No idea. (see the documentation). */
#define C2_DMA_PARITY	0x01	/* Enable parity on DMA transfers. */

#define C3_RESIDUAL	0x04	/* Save residual byte. */
#define C3_ALT_DMA	0x02	/* Alternate DMA mode. */
#define C3_THRESHOLD8	0x01	/* Don't DMA until there are 8 bytes. */

/*
 * Bits in the 53C90 Status register (read only).
 * This register contains flags that indicate certain events have occurred.
 * Bits 7-3 are latched until the interrupt register is read.  The phase
 * bits are not normally latched.  The interrupt bit (SR_INT) may be polled.
 * Hardware reset or software reset or a read from the interrupt register
 * will release an active INT signal and also clear this bit.
 * See pages 15-16 for which bits cause interrupts and for how to clear them.
 */
#define	SR_INT		0x80	/* ASC interrupting processor. */
#define	SR_GE		0x40	/* Gross error. */
#define	SR_PE		0x20	/* Parity error. */
#define	SR_TC		0x10	/* Terminal count. */
#define	SR_VGC		0x08	/* Valid group code. */
#define	SR_PHASE	0x07	/* Phase bits. */
#define	SR_DATA_OUT	0x00	/* Data out w.r.t. initiator. */
#define	SR_DATA_IN	0x01	/* Data in. */
#define	SR_COMMAND	0x02	/* Command. */
#define	SR_STATUS	0x03	/* Status. */
#define	SR_MSG_OUT	0x06	/* Message out w.r.t. initiator. */
#define	SR_MSG_IN	0x07	/* Message in. */

/*
 * Program/human-level description of phases.  The controller combines
 * the arbitration, selection and command phases into one big "selection"
 * phase.  Likewise, the status and msg-in phases are combined, but you can
 * have one by itself.  The bus free phase isn't represented by phase bits
 * in this controller, but rather by a disconnect after the MSG_IN phase.
 * We use these defines to be the "last phase" we were in, and the phases
 * above (SR_MSG_OUT, etc) to be the phase we are currently in.  Because
 * of the combined phases, this makes the state machine easier if these
 * aren't symmetrical.
 */
#define	PHASE_BUS_FREE		0x0
#define	PHASE_SELECTION		0x1
#define	PHASE_DATA_OUT		0x2
#define	PHASE_DATA_IN		0x3
#define	PHASE_STATUS		0x4
#define	PHASE_MSG_IN		0x5

/*
 * Fifo flags register.
 */
#define	FIFO_BYTES_MASK	0x1F

/*
 * Destination ID register for write.destID(write-only).
 */
#define	DEST_ID_MASK	0x07	/* Lowest 3 bits give binary-encoded id. */

/*
 * Timeout period to load into write.timeout field:  the usual value
 * is 250ms to meet ANSI standards.  To get the register value, we do
 * (timeout period) * (CLK frequency) / 8192 * (clock conversion factor).
 * The clock conversion factor is defined in the write.clockConv register.
 * The values are
 *	2	for	10MHz
 *	3	for	15MHz
 *	4	for	20MHz
 *	5	for	25MHz		153 is reg val
 */
#define	SELECT_TIMEOUT	153

/*
 * Interrupt register. (read-only).  Used in conjuction with the status
 * register and sequence step register to determine the cause of an interrupt.
 * Reading this register when the interrupt output is true will clear all
 * three registers.
 */
#define	IR_SCSI_RST	0x80	/* SCSI reset detected. */
#define	IR_ILL_CMD	0x40	/* Illegal command. */
#define	IR_DISCNCT	0x20	/* Target disconnected or time-out. */
#define	IR_BUS_SERV	0x10	/* Another device wants bus service. */
#define	IR_FUNC_COMP	0x08	/* Function complete. */
#define	IR_RESLCT	0x04	/* Reselected. */
#define	IR_SLCT_ATN	0x02	/* Selected with ATN. */
#define	IR_SLCT		0x01	/* Selected. */

/*
 * The address register for DMA transfers.
 */
typedef int DMARegister;

/*
 * If this bit is set in the DMA Register then the transfer is a write.
 */
#define DMA_WRITE	0x80000000

/*
 * The transfer size is limited to 16 bits since the scsi ctrl transfer
 * counter is only 2 bytes.  A 0 value means the biggest transfer size
 * (2 ** 16) == 64k.
 */
#define MAX_TRANSFER_SIZE	(64 * 1024)


/*
 * Misc defines 
 */

/*
 * devSCSIC90Debug - debugging level
 *	2 - normal level
 *	4 - one print per command in the normal case
 *	5 - traces interrupts
 */
int devSCSIC90Debug = 2;

int	dmaControllerActive = 0;

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
    ScsiDevice handle;			/* Scsi Device handle. This is the only
   					 * part of this structure visible to
					 * higher-level software. MUST BE FIRST
					 * FIELD IN STRUCTURE. */
    int	targetID;			/* SCSI Target ID of this device. Note
					 * that the LUN is stored in the device
					 * handle. */
    Controller *ctrlPtr;		/* Controller to which device is
					 * attached. */
					/* The following part of this structure
					 * is used to handle SCSI commands that
					 * return CHECK status. To handle the
					 * REQUEST SENSE command we must:
					 * 1) Save the state of the current
					 * command into the "struct
					 * FrozenCommand".
					 * 2) Submit a request sense command
					 * formatted in SenseCmd to the device.
					 */
    struct FrozenCommand {		       
	ScsiCmd	*scsiCmdPtr;	   	/* The frozen command. */
	unsigned char statusByte; 	/* It's SCSI status byte, Will always
					 * have the check bit set. */
	int amountTransferred;    	/* Number of bytes transferred by this 
				         * command. */
    } frozen;	
    char senseBuffer[DEV_MAX_SENSE_BYTES]; /* Data buffer for request sense */
    ScsiCmd		SenseCmd;  	   /* Request sense command buffer. */
} Device;

/*
 * Controller - The Data structure describing a sun SCSIC90 controller. One
 * of these structures exists for each active SCSIC90 HBA on the system. Each
 * controller may have from zero to 56 (7 targets each with 8 logical units)
 * devices attached to it. 
 */
struct Controller {
    volatile CtrlRegs	*regsPtr;	/* Pointer to the registers of
					 * this controller. */
    volatile DMARegister *dmaRegPtr;	/* Pointer to DMA register. */
    int		dmaState;		/* DMA state for this controller,
					 * defined below. */
    char	*name;			/* String for error message for this
					 * controller. */
    DevCtrlQueues	devQueues;	/* Device queues for devices attached
					 * to this controller. */
    Sync_Semaphore	mutex;		/* Lock protecting controller's data
					 * structures. */
	/*
	 * Until disconnect/reconnect is added we can have only one current
	 * active device and scsi command.
	 */
    Device	*devPtr;	   	/* Current active command. */
    ScsiCmd	*scsiCmdPtr;		/* Current active command. */
    int		residual;		/* Residual bytes in xfer counter. */
    int		lastPhase;		/* The scsi phase we were last in. */
    unsigned char	commandStatus;	/* Status received from device. */
    Device  *devicePtr[8][8];		/* Pointers to the device attached to
					 * the controller index by
					 * [targetID][LUN].  NIL if device not
					 * attached yet. Zero if device
					 * conflicts with HBA address. */
    char	*buffer;		/* SCSI buffer address. */
    int		slot;			/* Slot that this controller is in. */
};

/*
 * Possible values for the dmaState state field of a controller.
 *
 * DMA_RECEIVE  - data is being received from the device, such as on
 *	a read, inquiry, or request sense.
 * DMA_SEND     - data is being send to the device, such as on a write.
 * DMA_INACTIVE - no data needs to be transferred.
 */

#define DMA_RECEIVE  0x0
#define	DMA_SEND     0x2
#define	DMA_INACTIVE 0x4

/*
 * Test, mark, and unmark the controller as busy.
 */
#define	IS_CTRL_BUSY(ctrlPtr)	((ctrlPtr)->scsiCmdPtr != (ScsiCmd *) NIL)
#define	SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr) \
		((ctrlPtr)->scsiCmdPtr = (scsiCmdPtr))
#define	SET_CTRL_FREE(ctrlPtr)	((ctrlPtr)->scsiCmdPtr = (ScsiCmd *) NIL)

/*
 * MAX_SCSIC90_CTRLS - Maximum number of SCSIC90 controllers attached to the
 *		     system. 
 */
#define	MAX_SCSIC90_CTRLS	4

static Controller *Controllers[MAX_SCSIC90_CTRLS];
/*
 * Highest number controller we have probed for.
 */
static int numSCSIC90Controllers = 0;


#define REG_OFFSET	0
#define	DMA_OFFSET	0x40000
#define BUFFER_OFFSET	0x80000
#define ROM_OFFSET	0xc0000
/*
 * Forward declarations.  
 */

static void		PrintPhase _ARGS_ ((unsigned int phase));
static void		PrintLastPhase _ARGS_ ((unsigned int phase));
static Boolean          ProbeOnBoard _ARGS_ ((int address));
static Boolean          ProbeSBus _ARGS_ ((int address));
static void             Reset _ARGS_ ((Controller *ctrlPtr));
static ReturnStatus     SendCommand _ARGS_ ((Device *devPtr,
                                             ScsiCmd *scsiCmdPtr));
static void             PrintRegs _ARGS_((volatile CtrlRegs *regsPtr));
static void             StartDMA _ARGS_ ((Controller *ctrlPtr));

/*
 * This is global for now, so that we can see what the last value of
 * the register was while debugging.  Reading the real register clears
 * 3 registers, so that isn't a good thing to do.
 */
unsigned char	interruptReg;



/*
 *----------------------------------------------------------------------
 *
 * Reset --
 *
 *	Reset a SCSI bus controlled by the SCSI-3 Sun Host Adaptor.
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
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;

    if (devSCSIC90Debug > 3) {
	printf("Reset\n");
    }
    /* Reset scsi controller. */
    regsPtr->scsi_ctrl.write.command = CR_RESET_CHIP;
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_NOP;
    ctrlPtr->dmaState = DMA_INACTIVE;
    /*
     * Don't interrupt when the SCSI bus is reset. Set our bus ID to 7.
     */
    regsPtr->scsi_ctrl.write.config1 |= C1_REPORT | 0x7;
    regsPtr->scsi_ctrl.write.command = CR_RESET_BUS;
    /*
     * We initialize configuration, clock conv, synch offset, etc, in
     * SendCommand.
     * Parity is disabled by hardware reset or software.
     * We never send ID with ATN, so reselection shouldn't ever happen?
     */

    return;
}


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

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    regsPtr = ctrlPtr->regsPtr;
    SET_CTRL_BUSY(ctrlPtr, scsiCmdPtr);
    ctrlPtr->devPtr = devPtr;
    size = scsiCmdPtr->bufferLen;
    if (size == 0) {
	ctrlPtr->dmaState = DMA_INACTIVE;
    } else {
	ctrlPtr->dmaState = (scsiCmdPtr->dataToDevice) ? DMA_SEND :
							DMA_RECEIVE;
    }
    if (devSCSIC90Debug > 3) {
	printf("SCSIC90Command: %s addr %x size %d dma %s\n",
		devPtr->handle.locationName, scsiCmdPtr->buffer, size,
		(ctrlPtr->dmaState == DMA_INACTIVE) ? "not active" :
		((ctrlPtr->dmaState == DMA_SEND) ? "send" : "receive"));
	printf("TargetID = %d\n", devPtr->targetID);
    }

    /*
     * SCSI SELECTION.
     */

    /*
     * Set phase to selection and command phase so that we know what's happened
     * in the next phase.
     */
    ctrlPtr->lastPhase = PHASE_SELECTION;	/* Selection & command phase. */
    ctrlPtr->residual = size;			/* No bytes transfered yet. */
    ctrlPtr->commandStatus = 0;			/* No status yet. */
    /* Load select/reselect bus ID register with target ID. */
    regsPtr->scsi_ctrl.write.destID = 0x7;
    regsPtr->scsi_ctrl.write.destID = devPtr->targetID;
    /* Load select/reselect timeout period. */
    regsPtr->scsi_ctrl.write.timeout = SELECT_TIMEOUT;
    /* Zero value for asynchronous transfer. */
    regsPtr->scsi_ctrl.write.synchOffset = 0;
    /* Set the clock conversion register. */
    regsPtr->scsi_ctrl.write.clockConv = 5;
    /* Synchronous transfer period register not used for async xfer. */

    Mach_EmptyWriteBuffer();
    /*
     * Selection without attention since we don't do disconnect/reconnect yet.
     */
    
    /* Load FIFO with 6, 10, or 12 byte scsi command. */
    if (devSCSIC90Debug > 5) {
	printf("SCSIC90Command: %s stuffing command of %d bytes.\n", 
		devPtr->handle.locationName, scsiCmdPtr->commandBlockLen);
    }

    charPtr = scsiCmdPtr->commandBlock;
    if (scsiCmdPtr->commandBlockLen != 6 && scsiCmdPtr->commandBlockLen != 10
	    && scsiCmdPtr->commandBlockLen != 12) {
	printf("Command is wrong length.\n");
	return DEV_INVALID_ARG; 	/* Is this the correct error? */
    }
    for (i = 0; i < scsiCmdPtr->commandBlockLen; i++) {
	regsPtr->scsi_ctrl.write.FIFO = *charPtr;
	charPtr++;
    }
    Mach_EmptyWriteBuffer();
    /* Issue selection without attention command. */
    regsPtr->scsi_ctrl.write.command = CR_SLCT_NATN;

    if (devSCSIC90Debug > 5) {
	printf("\n");
    }

    /*
     * If all goes well, the chip will go through the arbitration,
     * selection and command phases for us.
     */
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * StartDMA --
 *
 *	Issue the sequence of commands to the controller to start DMA.
 *	This can be called by Dev_SCSIC90Intr in response to a DATA_{IN,OUT}
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
StartDMA(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile CtrlRegs	*regsPtr;
    int			size;

    size = ctrlPtr->scsiCmdPtr->bufferLen;

    if (devSCSIC90Debug > 4) {
	printf("StartDMA called for %s, dma %s, size = %d.\n", ctrlPtr->name,
	    (ctrlPtr->dmaState == DMA_RECEIVE) ? "receive" :
		((ctrlPtr->dmaState == DMA_SEND) ? "send" :
						  "not-active!"), size);
    }
    if (ctrlPtr->dmaState == DMA_INACTIVE) {
	printf("Returning, since DMA state isn't active.\n");
	return;
    }
    regsPtr = ctrlPtr->regsPtr;
    if (ctrlPtr->scsiCmdPtr->buffer == (Address) NIL) {
	panic("DMA buffer was NIL before dma.\n");
    }
    if (ctrlPtr->dmaState == DMA_RECEIVE) {
	*ctrlPtr->dmaRegPtr = 0;
    } else {
	bcopy((char *) ctrlPtr->scsiCmdPtr->buffer, ctrlPtr->buffer, size);
	*ctrlPtr->dmaRegPtr = (unsigned int) DMA_WRITE;
    }
    /*
     * Put transfer size in counter.  If this is 16k (max size), this puts
     * a 0 in the counter, which is the correct thing to do.
     */
    /* High byte of size. */
    regsPtr->scsi_ctrl.write.xCntHi = (unsigned char) ((size & 0xff00) >> 8);
    /* Low byte of size. */
    regsPtr->scsi_ctrl.write.xCntLo = (unsigned char) (size & 0x00ff);
    /* Load count into counter by writing a DMA NOP command on C90 only */
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_NOP;
    /* Start scsi command. */
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_XFER_INFO;

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

    printf("Won't print interrupt register since that would clear it,\n");
    printf(" but the old interrupt register is 0x%x.\n", interruptReg);
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

    /*
    if (devSCSIC90Debug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
     * First check to see if this is the reponse of a HBA-driver generated 
     * REQUEST SENSE command.  If this is the case, we can process
     * the callback of the frozen command for this device and
     * allow the flow of command to the device to be resummed.
     */
    if (scsiCmdPtr->doneProc == SpecialSenseProc) {
        MASTER_UNLOCK(&(ctrlPtr->mutex));
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
	if (devSCSIC90Debug > 3) {
	    printf("Calling special sense proc for frozen command.\n");
	}
		SUCCESS,
		devPtr->frozen.statusByte, 
		devPtr->frozen.amountTransferred,
		amountTransferred,
		devPtr->senseBuffer);
         MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_DEV_FREE(devPtr);
	 SET_CTRL_FREE(ctrlPtr);
    }
    /*
     * This must be an outside request finishing. If the request 
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) ||
    if ((status != SUCCESS) || !SCSI_CHECK_STATUS(scsiStatusByte)) {
        MASTER_UNLOCK(&(ctrlPtr->mutex));
	if (devSCSIC90Debug > 3) {
	    printf("Calling doneProc for regular command.\n");
	}
			       amountTransferred, 0, (char *) 0);
				   amountTransferred, 0, (char *) 0);
	SET_DEV_FREE(devPtr);
	SET_CTRL_FREE(ctrlPtr);
    } 
    /*
     * If we got here than the SCSI command came back from the device
     * with the CHECK bit set in the status byte.
     * Need to perform a REQUEST SENSE.  Move the current request 
     * into the frozen state and issue a REQUEST SENSE. 
     */

    if (devSCSIC90Debug > 3) {
	printf("Check bit set, performing Request Sense.\n");
    }
    devPtr->frozen.statusByte = scsiStatusByte;
    devPtr->frozen.amountTransferred = amountTransferred;
    DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES, 
		    devPtr->senseBuffer, &(devPtr->SenseCmd));
	    devPtr->senseBuffer, &(devPtr->SenseCmd));
    devPtr->SenseCmd.doneProc = SpecialSenseProc,
    
    /*
     * command with the SUCCESS status but zero sense bytes returned.
     */
    if (senseStatus != SUCCESS) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
	if (devSCSIC90Debug > 3) {
	    printf("Request sense failed.\n");
	}
				   amountTransferred, 0, (char *) 0);
        MASTER_LOCK(&(ctrlPtr->mutex));
	SET_DEV_FREE(devPtr);
	SET_CTRL_FREE(ctrlPtr);

}

/*
 *----------------------------------------------------------------------
 *
 * DevEntryAvailProc --
 * entryAvailProc --
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
static Boolean
entryAvailProc(clientData, newRequestPtr) 
   List_Links *newRequestPtr;	/* The new SCSI request. */
{
    Device		*devPtr; 
    Controller 		*ctrlPtr;
    ScsiCmd		*scsiCmdPtr = (ScsiCmd *)newRequestPtr;
    ScsiCmd		*scsiCmdPtr;

    devPtr = (Device *) clientData;
    ctrlPtr = devPtr->ctrlPtr;
    /*
     * If we are busy (have an active request) just return. Otherwise 
     * start the request.
     */
    

    if (IS_CTRL_BUSY(ctrlPtr)) { 
    }

    scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    devPtr = (Device *) clientData;
    status = SendCommand(devPtr, scsiCmdPtr);

     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
	 newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
    }
    if (!IS_CTRL_BUSY(ctrlPtr)) { 
        newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
	    goto again;
	}
    }


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
    List_Links		*newRequestPtr;
    ClientData		clientData;
    char		statusReg;
    int                 i;
    int			numBytes;
    unsigned char	message = 0;
    ReturnStatus	status = SUCCESS;
    ctrlPtr = (Controller *) clientDataArg;
    if (devSCSIC90Debug > 4) {
	printf("DevSCSIC90Intr: ");
    }
    regsPtr = ctrlPtr->regsPtr;
    devPtr = ctrlPtr->devPtr;

    MASTER_LOCK(&(ctrlPtr->mutex));

    /* Read registers.
    /* Read registers */
    sequenceReg = regsPtr->scsi_ctrl.read.sequence;
    interruptReg = regsPtr->scsi_ctrl.read.interrupt;
    phase = statusReg & SR_PHASE;
    sequenceReg &= SEQ_MASK;

#ifdef NOTDEF
    /*
     * In order to have multiple controllers at this interrupt level on
     * the scsi bus, we need to have some way of checking if we should be in
     * this routine, unfortunately things like the status register
     * interrupt bit don't seem to be set when we get here, so we just hope
     * it's okay.  XXXX Is this still true?
     */
if ((statusReg & SR_INT) == 0 &&
    (dmaReg & (DMA_INT_PEND | DMA_ERR_PEND)) == 0) {
	printf("Is this spurious? interruptReg 0x%x, statusReg 0x%x, dmaReg 0x%x.\n",
		interruptReg, statusReg, dmaReg);

}
    /* Check if we should we be in this routine. */
    if ((statusReg & SR_INT) == 0 &&
	    (dmaReg & (DMA_INT_PEND | DMA_ERR_PEND)) == 0) {
	printf("spurious: interruptReg 0x%x, statusReg 0x%x, dmaCtrl 0x%x.\n",
		interruptReg, statusReg, dmaReg);
    /* Check for errors. */
	return FALSE;
    }
#endif NOTDEF

    if (devSCSIC90Debug > 4) {
	printf(
	"interruptReg 0x%x, statusReg 0x%x, sequenceReg 0x%x\n",
		interruptReg, statusReg, sequenceReg);
    }
    if (devSCSIC90Debug > 3) {
	printf("LastPhase was ");
	PrintLastPhase(ctrlPtr->lastPhase);
    }

    if (statusReg & SR_GE) {
	panic("gross error 1\n");
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
	printf("%s: illegal command.\n",
		devPtr->handle.locationName);
	status = FAILURE;
    }
    if ((interruptReg & IR_DISCNCT) && ctrlPtr->lastPhase != PHASE_MSG_IN) {
	printf("%s disconnected or timed out.\n",
		devPtr->handle.locationName);
	status = DEV_TIMEOUT;
    if (interruptReg & IR_SLCT_ATN) {
    if (interruptReg & IR_RESLCT) {
	printf("%s: target asked for reselection, which we aren't doing yet.\n",
		devPtr->handle.locationName);
	status = FAILURE;
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
    switch (ctrlPtr->lastPhase) {
	status = PerformSelect(ctrlPtr, interruptReg, sequenceReg);
	if (! (interruptReg & IR_BUS_SERV)) {
	    if (devSCSIC90Debug > 4) {
		printf("We came from selection phase, but didn't finish.\n");
	    }
	    /*
	     * Save timeout error from above.
	     */
	    if (status != DEV_TIMEOUT) {
		status = FAILURE;
	    }
	}
    case PHASE_DATA_IN:
#ifdef sun4c
	status = PerformDataXfer(ctrlPtr, interruptReg, statusReg);
	ctrlPtr->residual = regsPtr->scsi_ctrl.read.xCntLo;
	ctrlPtr->residual += (regsPtr->scsi_ctrl.read.xCntHi << 8);
	/*
	 * If the transfer was the maximum, 16K bytes, a 0 in the counter
	 * may mean that nothing was transfered...  What should I do? XXX
	 */
	if (ctrlPtr->residual != 0 && devSCSIC90Debug > 3) {
	    printf("DMA transfer didn't finish. %d bytes left, xfered %d\n",
		    ctrlPtr->residual,
		    ctrlPtr->scsiCmdPtr->bufferLen - ctrlPtr->residual);
	}
	/*
	 * Flush the cache on data in, since the dma put it into memory
	 * but didn't go through the cache.  We don't have to worry about this
	 * on writes, since the sparcstation has a write-through cache.
	 */
	if (ctrlPtr->lastPhase == PHASE_DATA_IN) {
	    int		amountXfered;

	    amountXfered = ctrlPtr->scsiCmdPtr->bufferLen - ctrlPtr->residual;
	    bcopy((char *) ctrlPtr->buffer, ctrlPtr->scsiCmdPtr->buffer,
		amountXfered);
	}
	if (! (statusReg & SR_TC)) {
	    /* Transfer count didn't go to zero, or this bit would be set. */
	    if (devSCSIC90Debug > 3) {
		printf("After DMA, transfer count didn't go to zero.\n");
	    }
	}
	if (interruptReg & IR_DISCNCT) {
	    /* Target released BSY/ before count reached zero. */
	    printf("Target disconnected during DMA transfer.\n");
	    status = FAILURE;
	}
	if (! (interruptReg & IR_BUS_SERV)) {
	    /* Target didn't request information transfer phase. */
	    printf("Didn't receive bus service signal after DMA xfer.\n");
	    status = FAILURE;
	}
    case PHASE_STATUS:
	status = PerformStatus(ctrlPtr, interruptReg);
	/* Read bytes from FIFO. */
	numBytes = regsPtr->scsi_ctrl.read.FIFOFlags & FIFO_BYTES_MASK;
	if (numBytes != 2) {
	    /* We didn't get both phases. */
	    printf("Missing message in phase after status phase.\n");
	}
	    break;
	break;
	ctrlPtr->commandStatus = regsPtr->scsi_ctrl.read.FIFO;
	message = regsPtr->scsi_ctrl.read.FIFO;
	/* Would check status reg for parity here. */
	if (phase != SR_MSG_IN || (interruptReg & IR_BUS_SERV)) {
	    printf("Target wanted other phase than MSG_IN after status.\n");
	    status = FAILURE;
	    break;
	}
	    devPtr->lastPhase = PHASE_BUS_FREE;
	    printf("Target released BSY/ signal before MSG_IN.\n");
	}
	    break;
	break;
	if (! (interruptReg & IR_FUNC_COMP)) {
	    printf("Command didn't complete.\n");
	    status = FAILURE;
	    break;
	}
	if (message != SCSI_COMMAND_COMPLETE) {
	    printf("Warning: %s couldn't handle message 0x%x from %s.\n",
		    ctrlPtr->name, message,
		    ctrlPtr->devPtr->handle.locationName);
	}
    default:
    case PHASE_MSG_IN:
	if (interruptReg & IR_BUS_SERV) {
	    /* Request for another transfer. */
	    printf("Another transfer requested after MSG_IN.\n");
	    printf("We can't do this yet.\n");
	    status = FAILURE;
	    break;
	}
	if (!(interruptReg & IR_DISCNCT)) {
	    printf("Should have seen end of I/O.\n");
	    status = FAILURE;
	    break;
	}
	/* End of I/O. */
	if (devSCSIC90Debug > 3) {
	    printf("End of I/O.\n");
	}
	ctrlPtr->lastPhase = PHASE_BUS_FREE;
	RequestDone(devPtr, ctrlPtr->scsiCmdPtr, SUCCESS,
		ctrlPtr->commandStatus,
		ctrlPtr->scsiCmdPtr->bufferLen - ctrlPtr->residual);

	if (! IS_CTRL_BUSY(ctrlPtr)) {
	    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
		    DEV_QUEUE_ANY_QUEUE_MASK, &clientData);
	    if (newRequestPtr != (List_Links *) NIL) {
		(void) entryAvailProc(clientData, newRequestPtr);
	    }
	}
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
	/* We set this field, so this shouldn't happen. */
	printf("SCSIC90Intr: We came from an unknown phase.\n");
	printf("We came from an unknown phase.\n");
	break;
    }

    if (status != SUCCESS) {
	goto HandleHardErrorAndGetNext;
    }
    if ((status != SUCCESS) || (interruptReg & IR_DISCNCT)) {
    /* No error so far. */

    if (devSCSIC90Debug > 4 ) {
	PrintPhase((unsigned int) phase);

    switch (phase) {

    case SR_DATA_IN:
	devPtr->lastPhase = (phase == SR_DATA_OUT ? PHASE_DATA_OUT :
	ctrlPtr->lastPhase = (phase == SR_DATA_OUT ? PHASE_DATA_OUT :
		PHASE_DATA_IN);
	 * It should be possible to do multiple blocks of DMA without
	 * returning to the higher level, if we set the max transfer size
	 * larger, but we don't handle that yet. XXX
	 */
#ifdef sun4c
	StartDMA(ctrlPtr);
    case SR_COMMAND:
	charPtr = devPtr->scsiCmdPtr->commandBlock;
	printf("We shouldn't ever enter this phase.\n");
	status = FAILURE;
    case SR_STATUS:
	/*
	 * We're in status phase.  If all goes right, the next interrupt
	 * will be after we've handled the message phase as well, although
	 * the phase will say we're in message phase.
	 */
	devPtr->lastPhase = PHASE_STATUS;
	ctrlPtr->lastPhase = PHASE_STATUS;
	break;
    case SR_MSG_OUT:
    case SR_MSG_IN:
	ctrlPtr->lastPhase = PHASE_MSG_IN;
	/* Accept message */
	if (devSCSIC90Debug > 3) {
	    printf("Accepting message while in msg in phase.\n");
	break;
	regsPtr->scsi_ctrl.write.command = CR_MSG_ACCPT;
    case SR_MSG_IN:
    case SR_MSG_OUT:
	printf("MSG_OUT phase currently unused.  Why are we in it?!\n");
	status = FAILURE;
    default:
	printf("unknown scsi phase type: 0x%02x\n", (int)phase);
	break;
    }

    MASTER_UNLOCK(&(ctrlPtr->mutex));
    if (status == SUCCESS) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return TRUE;
    }

HandleHardErrorAndGetNext:
    if (devSCSIC90Debug > 3) {
	PrintPhase((unsigned int) phase);
    }
	
    if (ctrlPtr->scsiCmdPtr != (ScsiCmd *) NIL) { 
	if (devSCSIC90Debug > 3) {
	    printf("Warning: %s reset and current command terminated.\n",
		   devPtr->handle.locationName);
	}
	RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, 0,
		ctrlPtr->scsiCmdPtr->bufferLen - ctrlPtr->residual);
    }
    Reset(ctrlPtr);
    /*
     * Use the queue entryAvailProc to start the next request for this device.
     */
    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
    if (newRequestPtr != (List_Links *) NIL) { 
	(void) entryAvailProc(clientData, newRequestPtr);
    }
    MASTER_UNLOCK(&(ctrlPtr->mutex));
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
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSIC90Init --
 *
 *	Check for the existant of the Sun SCSIC90 HBA controller. If it
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
DevSCSIC90Init(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int			ctrlNum;
    Boolean		found;
    Controller 		*ctrlPtr;
    int			i,j;
    Mach_SlotInfo	slotInfo;
    char		*slotAddr;
    static char		*vendor = "DEC";
    static char		*module = "PMAZ-AA";
    ReturnStatus	status;

    slotAddr = (char *) MACH_IO_SLOT_ADDR(ctrlLocPtr->slot);

    status = Mach_GetSlotInfo(slotAddr + ROM_OFFSET, &slotInfo);
    if (status != SUCCESS) {
	return DEV_NO_CONTROLLER;
    }
    if (strcmp(slotInfo.vendor, vendor) || strcmp(slotInfo.module, module)) {
	return DEV_NO_CONTROLLER;
    }
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    ctrlNum = ctrlLocPtr->controllerID;
    if (ctrlNum >= MAX_SCSIC90_CTRLS) {
	printf("DevSCSIC90Init: too many controllers\n");
	return DEV_NO_CONTROLLER;
    }
    if (ctrlNum+1 > numSCSIC90Controllers) {
	numSCSIC90Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (slotAddr + REG_OFFSET);
    ctrlPtr->dmaRegPtr = (volatile DMARegister *) (slotAddr + DMA_OFFSET);
    ctrlPtr->buffer = slotAddr + BUFFER_OFFSET;
    ctrlPtr->name = ctrlLocPtr->name;
    ctrlPtr->slot = ctrlLocPtr->slot;
    Sync_SemInitDynamic(&(ctrlPtr->mutex), ctrlPtr->name);
    printf("SCSI controller \"%s\" in slot %d (%s %s %s %s)\n",
	ctrlPtr->name, ctrlPtr->slot, slotInfo.module, slotInfo.vendor, 
	slotInfo.revision, slotInfo.type);
    /* 
     * Initialized the name, device queue header, and the master lock.
     * The controller comes up with no devices active and no devices
     * attached.  Reserved the devices associated with the 
     * targetID of the controller (7).
     */
    ctrlPtr->devQueues = Dev_CtrlQueuesCreate(&(ctrlPtr->mutex),
	    entryAvailProc);
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    ctrlPtr->devicePtr[i][j] = (i == 7) ? (Device *) 0 : (Device *) NIL;
	}
    }
    ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
    Controllers[ctrlNum] = ctrlPtr;
    Mach_SetIOHandler(ctrlPtr->slot, (void (*)()) DevSCSIC90Intr, 
	(ClientData) ctrlPtr);
    Reset(ctrlPtr);

    return (ClientData) ctrlPtr;
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
				1, insertProc, (ClientData) devPtr);
    devPtr->handle.locationName = "Unknown";
    devPtr->handle.LUN = lun;
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.maxTransferSize = MAX_TRANSFER_SIZE;
    devPtr->targetID = targetID;
    devPtr->ctrlPtr = ctrlPtr;
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
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    (void) sprintf(tmpBuffer, "%s Target %d LUN %d", ctrlPtr->name, 
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    return (ScsiDevice *) devPtr;
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
    default:
	printf("unknown phase %d.\n", phase);
	break;
    }

    return;
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
