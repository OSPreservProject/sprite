 /* 
 * devSCSIC90Int.h --
 *
 *	Def'ns specific to the SCSI NCR 53C9X Host Adaptor.  This adaptor is
 *	based on the NCR 53C90 chip.
 *	The 53C90 supports connect/dis-connect.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *$Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIC90INT
#define _DEVSCSIC90INT

#include "devSCSIC90Mach.h"


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
    unsigned char xCntLo;	/* LSB of transfer count2er. */
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
#define C1_BUS_ID	0x07	/* Bus ID. */

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
#define	PHASE_MSG_OUT		0x6
#define	PHASE_STAT_MSG_IN	0x7
#define	PHASE_RDY_DISCON        0x8
#define	PHASE_COMMAND           0xa
#define	PHASE_SYNCH             0xb

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
 * sequence register. (read-only). Used in conjunction with the status
 * register and interrupt registers to determine partial completion
 * of last action. In initiator mode, which is all we do, the only
 * action which will set the sequence bits is the selection phase.
 */

#define SEQ_MASK        0x04    /* mask to isolate sequence bits */

/*
 * These are interpreted with the interrupt reg bits. See p.40-41
 * of the NCR 53C90 Enhanced SCSI Processor Data Manual Rev 3.0
 */

#define SEQ_NO_SEL           0x00    /* when IR == 0x20        */
#define SEQ_NO_MSG           0x00    /* when IR == 0x18        */
#define SEQ_NO_CMD           0x02    /* when IR == 0x18        */
#define SEQ_CMD_INCOMPLETE   0x03
#define SEQ_COMPLETE         0x04

/*
 * The transfer size is limited to 16 bits since the scsi ctrl transfer
 * counter is only 2 bytes.  A 0 value means the biggest transfer size
 * (2 ** 16) == 64k.
 */
#define MAX_TRANSFER_SIZE	(64 * 1024)


/*
 * Misc defines 
 */

extern int devSCSIC90Debug;

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
    ScsiCmd		*scsiCmdPtr;  	/* The active command */ 
    Address             activeBufPtr;   /* working copies of scsiCmd.buffer */
    int                 activeBufLen;   /* scsiCmd.bufferLen */
    unsigned char	commandStatus;	/* Status received from device. */
    int		lastPhase;		/* The scsi phase we were last in. */
    int		dmaState;		/* DMA state for this device
					 * defined below. */
    unsigned char       messageBuf[5];  /* msg byte buffer */
    unsigned char       messageBufLen;
    unsigned char       synchOffset;    /* Max # of REQs w/o an ACK */
    unsigned char       synchPeriod;    /* Min # cycles between REQs */
    unsigned char       msgFlag;  
} Device;

/* 
 * msgFlag definitions
 */
#define REQEXTENDEDMSG       0x01       /* request an extended msg */
#define STARTEXTENDEDMSG     0x02       /* processing xtended msg */
#define ENABLEEXTENDEDMSG    0x04       /* enable extended msgs */

/*
 * Controller - The Data structure describing a sun SCSIC90 controller. One
 * of these structures exists for each active SCSIC90 HBA on the system. Each
 * controller may have from zero to 56 (7 targets each with 8 logical units)
 * devices attached to it. 
 */
struct Controller {
    volatile CtrlRegs	*regsPtr;	/* Pointer to the registers of
					 * this controller. */
    char	        *name;		/* String for error message for this
					 * controller. */
    DevCtrlQueues	devQueues;	/* Device queues for devices attached
					 * to this controller. */
    Sync_Semaphore	mutex;		/* Lock protecting controller's data
					 * structures. */
    Device	        *devPtr;  	/* Current active command. */
    Device            *devicePtr[8][8];	/* Pointers to the device attached to
					 * the controller index by
					 * [targetID][LUN].  NIL if device not
					 * attached yet. Zero if device
					 * conflicts with HBA address. */
    unsigned int        devQueuesMask;  /* Map of device queues.
					 *  1 = dev available; 0 = dev busy. */
    Device            *interruptDevPtr; /* device interrupted by reconnect */
#ifdef ds5000
    volatile int *dmaRegPtr;	/* Pointer to DMA register. */
    char	*buffer;		/* SCSI buffer address. */
    int		slot;			/* Slot that this controller is in. */
#endif
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

/* temporary unti new scsiHBA.h is installed */
#define	SCSI_EXTENDED_MSG_SYNC 0x01

/*
 * Test, mark, and unmark the device as busy.
 */
#define	IS_CTRL_FREE(ctrlPtr) \
                ((ctrlPtr)->devPtr == (Device *)NIL)
#define	SET_CTRL_FREE(ctrlPtr) \
                PUTCIRCBUF(CSTR, "c free "); \
                PUTCIRCNULL; \
                (ctrlPtr)->devPtr = (Device *)NIL;
#define	SET_CTRL_BUSY(ctrlPtr,devPtr) \
                PUTCIRCBUF(CSTR, "c busy "); \
                PUTCIRCNULL; \
                (ctrlPtr)->devPtr = devPtr;
#define	IS_INTERRUPT_DEV(cPtr,dPtr,rPtr) \
                (((cPtr)->interruptDevPtr == (dPtr)) && \
		 ((dPtr)->scsiCmdPtr == (rPtr)))
#define	IS_DEV_FREE(devPtr) \
                ((devPtr)->ctrlPtr->devQueuesMask & (1 << (devPtr)->targetID))
#define	SET_DEV_FREE(devPtr) \
                (devPtr)->ctrlPtr->devQueuesMask |= (1<<(devPtr)->targetID);\
                PUTCIRCBUF(CSTR, "d free "); \
                PUTCIRCBUF(CBYTE, (char *)((devPtr)->targetID)); \
                PUTCIRCBUF(CSTR, "; mask "); \
                PUTCIRCBUF(CBYTE, (char *)((devPtr)->ctrlPtr->devQueuesMask)); \
                PUTCIRCNULL; \
		(devPtr)->scsiCmdPtr = (ScsiCmd *) NIL;
#define	SET_DEV_BUSY(devPtr,cmdPtr)  \
                (devPtr)->ctrlPtr->devQueuesMask &= ~(1<<(devPtr)->targetID);\
                PUTCIRCBUF(CSTR, "d busy "); \
                PUTCIRCBUF(CBYTE, (char *)((devPtr)->targetID)); \
                PUTCIRCBUF(CSTR, "; mask "); \
                PUTCIRCBUF(CBYTE, (char *)((devPtr)->ctrlPtr->devQueuesMask)); \
                PUTCIRCNULL; \
		(devPtr)->scsiCmdPtr = (cmdPtr);

void             DevReset _ARGS_ ((Controller *ctrlPtr));
void             DevStartDMA _ARGS_ ((Controller *ctrlPtr));
Boolean          DevEntryAvailProc _ARGS_ ((ClientData clientData,
					 List_Links *newRequestPtr));

extern Controller *Controllers[MAX_SCSIC90_CTRLS];

/* 
 * Min/max values for synchronous xfer as given in the NCR manual
 */
#define MAX_SYNCH_PERIOD 35
#define MIN_SYNCH_PERIOD 5
#define MAX_SYNCH_OFFSET 15
#define MIN_SYNCH_OFFSET 0

/*
 * These macros convert the synch_period between units of
 * 4ns, for the scsi protocol, and clock units, which is what
 * the NCR chip wants.
 * Each cycle is 1000/(CLOCKCONV*5) ns, so (n*4) ns equals
 * (n*4)/(1000/(CLOCKCONV*5)) = (n*CLOCKCONV/50) cycles.
 */
#define SCSI_TO_NCR(x)  \
    ((x)*CLOCKCONV/50) + \
    (( ((x)*CLOCKCONV/50) * (50/CLOCKCONV) < (x)) ? 1 : 0)
#define NCR_TO_SCSI(x) ((x)*50/CLOCKCONV)

/*
 * debugging junk
 *
 */

#define CIRCBUFLEN (1024*10)

extern int circhead;
extern char circBuf[CIRCBUFLEN];
extern char numTab[16];

#define CSTR 0
#define CBYTE 1
#define CINT 2

#define CVTHEX(num,bits)  numTab[((int)num >> bits) & 0x0f]
#define PUTCIRCBUF(a,b) PutCircBuf(a,b)
#define PUTCIRCNULL \
                circBuf[circHead] = '\0'; \
                circHead = (circHead + 1) % CIRCBUFLEN;
#endif


