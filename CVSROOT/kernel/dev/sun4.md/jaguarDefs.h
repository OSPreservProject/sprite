/*
 * jaguarDefs.h --
 *
 *	Declarations of data structures and constants for the Interphase
 *	V/SCSI 4210 Jaguar SCSI host bus adapter (HBA). This definitions 
 *	in this file come from the "V/SCSI 4210 Jaguar System Interface
 *	User's Guide", document number UG-0770-000-X0F. The document was
 *	obtained from:
 *		Interphase Corporation
 *		Application Engineering Department
 *		2925 Merrell Road
 *		Dallas, Tx 75229
 *
 * Note that this file should only contain information for the Jaguar 
 * HBA as a generic VME bus device.  This means that information about 
 * the Jaguar's interface to particle machines should not be included.
 *
 * The structures in this file are set up so that most C compilers will generate
 * layouts of the structures that match the Jaguar vision of the data 
 * structures. These structures should only need to be changed if the
 * Jaguar firmware changes.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _JAGUAR_HBA_INT
#define _JAGUAR_HBA_INT

/* constants */

/* data structures */
/*
 * The Jaguar communicates with the host computer using 2K bytes of dual 
 * ported memory mapped into the Short I/0 space of the VMEbus.  To 
 * communicate with the Jaguar, the host builds command blocks in the
 * shared memory and points the Jaguar at them.  The following type
 * declarations defined the memory image of these command blocks. The
 * acronyms (ie MCSB, MCE, etc) are taken from the Jaguar User's guide
 * mentioned above. The referend page numbers are also from this guide.
 *
 * Note that the Jaguar's processor is a big endian hence these structures
 * probably will be incorrect if the host is little endian.
 */

/*
 * In order to simplify some of the computations in this header file, we
 * set the maximum size IOPB to be 64 bytes.  This allows us to handle 
 * SCSI command blocks upto (64-32) bytes with out using extented pass-thru
 * mode.  It also makes indexing into arrays of IOPBs fast.
 */

#define	JAGUAR_MAX_IOBP_SIZE	64
#define	JAGUAR_MAX_SCSI_CMDSIZE	(JAGUAR_MAX_IOBP_SIZE - 32)

 /*
  ************************************************************************
  *			MCSB
  ***********************************************************************
  * The master control/status block (MCSB) is used by the Jaguar to pass
  * and receive information about the overall operation of the controller.
  * Page 8
  * Note that the MCSB must reside the the short I/O space of the VME bus. 
  * Because of this the host processor must only do 16 bit stores to the
  * fields in the MCSB.  
  */

typedef struct JaguarMCSB {
    unsigned short status;	/* The master status register (MSR). 
				 * Used to report board level status.
				 * Read only to the host. Defined below.
				 */
    unsigned short control;	/* The master control register (MCR). 
				 * Used to control board. Defined below. 
				 * The Jaguar never chanages any of the 
				 * bits in this register.
				 */
    unsigned short queueAvail;  /* The interrupt on queue available register.
				 * (IQAR). Controls the issue of interrupts
				 * when queue entries become available.
				 */
    unsigned short queueHead;	/* The queue head pointer. This location is
				 * for use by the host software and not used
				 * by the Jaguar.
				 */
    unsigned short thawQueue;	/* The thaw work queue register. This register
				 * is used to restart a work queue after an
				 * error. Defined below.
				 */
    unsigned short reserved[3];	

} JaguarMCSB;

/*
 * JAGUAR_MCSB_SIZE - The size in bytes of the MCSB according to 
 * the Jaguar.  This should equal sizeof(JaguarMCSB) or all bets are off.
 * Page 5
 */

#define	JAGUAR_MCSB_SIZE	16

/*
 * The master status register (MSR). Bits 15 thru 3 are reserved. 
 * Pages 8-9
 * NOT_AVAILABLE  - Bit indicating controller available for commands.
 * BOARD_OK	  - Bit indicating power-up diagnostics passed. Not valid for
 *		    100 microseconds after reset.
 * FLUSH_COMPLETE - Bit indicating the completion of a Queue flush operation.
 *
 */

#define	JAGUAR_MSR_NOT_AVAILABLE	0x1
#define	JAGUAR_MSR_BOARD_OK	0x2
#define	JAGUAR_MSR_FLUSH_COMPLETE	0x4

/*
 * The master control register MCR.  Bit 1, 3-10, 14-15 are reserved and must
 * be set to zero.
 * Page 10-12.
 * START_QUEUES  - 	Bit used to enable queue mode.
 * FLUSH_QUEUES_RPT  	Bit used to flush all work queues with report 
 *		        given for each command flushed.
 * FLUSH_QUEUES	 -	Bit used to flush all command queues.
 * RESET	 -	Reset the controller and all SCSI busses.  Must leave
 *			set for 50 microseconds.
 * SYSFAIL_ENA 	- 	Enables the Jaguar to assert the VME Sysfail signal
 *			if it fails diagnostics.
 */

#define	JAGUAR_MCR_START_QUEUES		   0x1
#define	JAGUAR_MCR_FLUSH_QUEUES_RPT	   0x4
#define	JAGUAR_MCR_FLUSH_QUEUES		 0x800
#define	JAGUAR_MCR_RESET			0x1000
#define	JAGUAR_MCR_SYSFAIL_ENA		0x2000

/*
 * The interrupt of queue available registers IQAR. 
 * Page 12-14.
 * INTR_VECTOR	- Macro to convert an VME interupt level and vector into the
 *		  value to be stored into the IQAR and CIB.
 * HALF_EMPTY_INTR - Bit indicating interrupt when queue is half ready.
 * INTR_ENABLE	   - Bit indicating queue avail interrupt enable.
 */

#define	JAGUAR_INTR_VECTOR(level, vector)	(((level)<<8)|(vector))
#define	JAGUAR_IQAR_HALF_EMPTY_INTR	0x4000
#define	JAGUAR_IQAR_INTR_ENABLE		0x8000

/*
 * The thaw work queue register.  Bits 1-7 are reserved must be zero.
 * Page 14-15
 * THAW_WORK_QUEUE_BIT - Bit indicating thawing of the specified work queue.
 * THAW_WORK_QUEUE     - Macro to compute the value to store in the 
 *			 thaw work queue register to thaw a queue. Thaw
 *			 is complete when Jaguar resets THAW_WORK_QUEUE_BIT.
 *
 */

#define	THAW_WORK_QUEUE_BIT	0x1
#define	THAW_WORK_QUEUE(workQueue)	(THAW_WORK_QUEUE_BIT|((workQueue)<<8))

 /*
  ************************************************************************
  *			CQE
  ************************************************************************
  * The command CQE provides the Jaguar with all the info needed to find and
  * execute commands.
  * Page 16.
  */

typedef struct JaguarCQE {
    unsigned short	controlReg;	/* Queue entry control register. 
					 * (QECR). Used to kick off command
					 * execution. Defined below.
					 */
    unsigned short	iopbOffset;	/* Offset into the Jaguar's memory
					 * of the IOPB for this command.
					 */
    unsigned short	commandTag[2];	/* Four bytes for the host software. */
    unsigned char	iopbLength;	/* The length of the IOPB for this
					 * command. (In 32 bit words!!!)
					 */
    unsigned char	workQueue;	/* What work queue is this command
					 * destine.
					 */
    unsigned short	reserved;	/* Zero me. */
} JaguarCQE;


/*
 * JAGUAR_CQE_SIZE - The size in bytes of the CQE according to 
 * the Jaguar.  This should equal sizeof(JaguarCQE) or all bets are off.
 * Page 16
 */

#define	JAGUAR_CQE_SIZE	12

/*
 * The queue entry control register (QECR). Bits 3-7, 12-15 are reserved and
 * must be zero. Bits 8-11 are the IOPB type and since the Jaguar only 
 * supports type zero IOBBs, bits 8-11 are also zero.
 * Page 17-18
 * GO_BUSY	- Start the command.  This bit set means the CQE is busy.
 * ABORT_ACK	- Stop aborting commands.
 * HIGH_PRI	- This is a high priority command. Put it in the front of the
 *		  work queue.
 */

#define	JAGUAR_CQE_GO_BUSY	0x1
#define	JAGUAR_CQE_ABORT_ACK	0x2
#define	JAGUAR_CQE_HIGH_PRI	0x4

 /*
  ************************************************************************
  *			CCSB
  ************************************************************************
  * The controller configuation status block is used by the Jaguar to report
  * the hardware and firmware configuration of the controller.
  * Page 25-28
  */

typedef struct JaguarCCSB {
    char	reserved1[3];
    char	code[3];	/* Product code - 3 ASCII chars. */
    char	reserved2[3];
    char	variation;	/* Product variation. */
    char	reserved3[3];
    char	firmwareLevel[3];/* Firmware revison level - 2 ASCII chars. */
    char	reserved4[2];
    char	firmwareDate[8]; /* Firmware release data. Format MMDDYYYY. */
    char	reserved5[2];
    unsigned short bufferRAMsize; /* The amount of buffer RAM on board. Units
				   * is kilobytes.
				   */
    char	reserved6[2];
    unsigned char primaryID;	/* The target ID of the HBA on the primary bus.
				 * Encoded in binary.
				 */
    unsigned char secondaryID;	/* The target ID of the HBA on the secondary
				 * bus. Encoded in binary.
				 */

    char	reserved[86];

} JaguarCCSB;

/*
 * JAGUAR_CCSB_SIZE - The size in bytes of the CCSB according to 
 * the Jaguar.  This should equal sizeof(JaguarCCSB) or all bets are off.
 * Page 25
 */

#define	JAGUAR_CCSB_SIZE	120
 /*
  ************************************************************************
  *			CIB
  ************************************************************************
  * The controller initialization block is not initialize the Jaguar controller.
  * To is used by the initialize controller command 0x41 and must reside in
  * the dual ported memory.
  * Page 58-60
  */

typedef struct JaguarCIB {
    unsigned short numQueueSlots;	/* Number of slots in each command Q. */
    unsigned short dmaBurstCount;	/* Number of DMA transfers the
					 * jaguar will perform before releasing
					 * the VME bus.
					 */
    unsigned short normalIntrVector;	/* Normal completion interrupt vector.*/
    unsigned short errorIntrVector;	/* Error completion interrupt vector.*/
    unsigned short priTargetID;		/* Primary bus SCSI ID. */
    unsigned short secTargetID;		/* Secondary bus SCSI ID. */
    unsigned short offsetCRB;		/* Offset into dual ported memory of the
					 * command reponse block. 
					 */
    unsigned short scsiSelTimeout[2];	/* SCSI selection time out value in 
					 * milliseconds.  0 -> infinite.
					 */
    unsigned short scsiReselTimeout[2];	/* SCSI re-selection time out value in 
					 * (32 milliseconds) ticks. 
					 * 0 -> infinite.
					 */
    unsigned short  vmeTimeout[2];	/* VME timeout value. 32 milliseconds
					 * ticks.  0 -> 100 milliseconds. 
					 */
    unsigned short reserved[3];
} JaguarCIB;


#define	JAGUAR_CIB_SIZE 32
/*
 * For secTargetID and priTargetID, BUS ID directing the board to read
 * default from DIP switches.
 */
#define	JAGUAR_DEFAULT_BUS_ID	0x8

 /*
  ************************************************************************
  *			IOPB
  ************************************************************************
  * The i/O parameter block (IOPB) is used to send commands to the Jaguar.
  * Page 31-
  *
  * The Format of the IOPB control block depends on the command being
  * executed. 
  */

typedef struct JaguarIOPB {
    unsigned short	command;	/* Command to issue. Defined below. */
    unsigned short 	options;	/* Command options to used. 
					 * Defined below. 
					 */
    unsigned short	returnStatus;	/* Return status of the command. */
    unsigned short 	reserved;
    unsigned short	intrVector;	/* Normal and error error vectors. */
    unsigned short	intrLevel;	/* VME interrupt level. */
    unsigned short 	reserved2;
    unsigned short	addrModifier;	/* Controls the data transfer on the
					 * VME bus. Defined below. 
					 */
    unsigned short	bufferAddr[2];	/* Address of data buffer for cmd. */
    unsigned short	maxXferLen[2];	/* Maximum number of bytes to xfer. */
    unsigned short	reserved3[2];	
    /*
     * The data following the reserved3 field is command depended. 
     */
    union {
	struct {
	    /*
	     * PASS_THRU, PASS_THRU_EXT, and RESET commands.
	     */
	    unsigned short length;	  /* The length of the SCSI command 
					   * block. Used only for the 
					   * PASS_THRU_EXT command. This 
					   * is also the SCSI Bus ID for
					   * RESET commands. 
					   */
	    unsigned short unitAddress;	/* SCSI bus and target ID. */
	    unsigned char  cmd[JAGUAR_MAX_SCSI_CMDSIZE];
					/* Scsi Command Block. */
	} scsiArg;
	struct {
	    /*
	     * DIAG_CMD command.
	     */
	    unsigned short romTest;	/* ROM test results. */
	    unsigned short scrRamTest;	/* Scratch pad RAM test results. */
	    unsigned short bufRamTest;	/* Buffer RAM test results. */
	    unsigned short eventRamTest;/* Evant RAM test results. */
	    unsigned short priPort;	/* Primary SCSI port test results. */
	    unsigned short secPort;	/* Primary SCSI port test results. */
	} diagArg;
	struct {
	    /*
	     * INIT_WORK_QUEUE_CMD, DUMP_WORK_QUEUE_CMD, FLUSH_WORK_QUEUE_CMD
	     */
	     unsigned short number;	/* Work number queue to initialize. */
	     unsigned short options;	/* Work queue options. Defined below. */
	     unsigned short slots;	/* Number of slots in work queue. */
	     unsigned short priority;	/* Priority level of queue. */
	} workQueueArg;
    } cmd;
}  JaguarIOPB;


/*
 * IOPB commands. 
 *
 * SCSI IOPBs:
 * PASS_THRU_CMD - Send a SCSI command to the specified target. 
 * PASS_THRU_EXT_CMD - Send a command to the specified target. Command doesn't
 *		       fit in IOPB.
 * RESET_CMD	- Reset the SCSI bus.
 *
 * Control IOBs:
 * DIAG_CMD	- Perform diagnostics.
 * INIT_HBA_CMD - Initialized the controller.
 * INIT_WORK_QUEUE_CMD - Initialize a work queue.
 * DUMP_HBA_PARAMS_CMD - Return the initialization parameters.
 * DUMP_WORK_QUEUE_CMD	- Return the parameters of a work queue.
 * FLUSH_WORK_QUEUE_CMD - Flush a work queue. 
 */

#define	JAGUAR_PASS_THRU_CMD		0x20
#define	JAGUAR_PASS_THRU_EXT_CMD	0x21
#define	JAGUAR_RESET_CMD		0x22

#define	JAGUAR_DIAG_CMD			0x40
#define	JAGUAR_INIT_HBA_CMD		0x41
#define	JAGUAR_INIT_WORK_QUEUE_CMD	0x42
#define	JAGUAR_DUMP_HBA_PARAMS_CMD	0x43
#define	JAGUAR_DUMP_WORK_QUEUE_CMD	0x44
#define	JAGUAR_FLUSH_WORK_QUEUE_CMD 	0x49



#define	JAGUAR_CMD_NAMES { \
	{ JAGUAR_PASS_THRU_CMD, "SCSI Pass-Through"} , \
	{ JAGUAR_PASS_THRU_EXT_CMD, "SCSI Pass-Through Extended"}, \
	{ JAGUAR_RESET_CMD, "SCSI Reset"}, \
	{ JAGUAR_DIAG_CMD, "Perform Diagnostics" }, \
	{ JAGUAR_INIT_HBA_CMD, "Initalize Controller" }, \
	{ JAGUAR_INIT_WORK_QUEUE_CMD, "Initalize Work Queue" }, \
	{ JAGUAR_DUMP_HBA_PARAMS_CMD, "Dump Initialization Parameters" }, \
	{ JAGUAR_DUMP_WORK_QUEUE_CMD, "Dump Work Queue Parameters" }, \
	{ JAGUAR_FLUSH_WORK_QUEUE_CMD, "Flush Work Queue" } }

/*
 * IOPB options:
 *
 * JAGUAR_IOPB_INTR_ENA	- Interrupt upon command completion.
 * JAGUAR_IOPB_SCAT_GATH	- Enable scatter/gather.
 * JAGUAR_IOPB_TO_HBA	- Data is read from the VME Bus to HBA.
 *
 *  These work on only the work queue:
 * JAGUAR_IOPB_RPT_FLUSH	- Report after each command is flushed.
 * JAGUAR_IOPB_RESET_IP    - Reset SCSI Bus when an In procgress command
 *				  is flushed.
 */

#define	JAGUAR_IOPB_INTR_ENA	0x1
#define	JAGUAR_IOPB_SCAT_GATH	0x2
#define	JAGUAR_IOPB_TO_HBA		0x100

#define	JAGUAR_IOPB_RPT_FLUSH	0x100
#define	JAGUAR_IOPB_RESET_IP	0x200

/*
 * IOPB interrupt vector (intrVector).
 *
 * JAGUAR_IOPB_INTR_VECTOR()	Set the intrVector field on of IOPB
 *				 	to interrupt with the specified error
 *					and normal  vectors.
 */

#define	JAGUAR_IOPB_INTR_VECTOR(normalVector, errorVector) \
			(((normalVector)<<8)|(errorVector))
/*
 * IOPB address modifier. The address modifier controls how the Jaguar reads and
 * writes data to and from the VME bus. Three components make up the SCSI 
 * addr modifier: the VME address modifier, the memory type, and transfer type.
 * The low byte of the transfer type is the VME address modifier.
 *
 */

/*
 * Memory types:
 * JAGUAR_16BIT_MEM_TYPE	- Transfer data 16 bits at a time.
 * JAGUAR_32BIT_MEM_TYPE	- Transfer data 32 bits at a time.
 * JAGUAR_BOARD_MEM_TYPE	- Transfer data to or from Jaguar dual 
 *				  port memory.
 *
 * Transfer Type:
 *
 * JAGUAR_NORMAL_MODE_XFER	- Normal transfer mode.
 * JAGUAR_BLOCK_MODE_XFER	- Block transfer mode.
 * JAGUAR_NO_INC_MODE_XFER - Diable incrementing addresses transfer mode.
 */

#define	JAGUAR_16BIT_MEM_TYPE	0x100
#define	JAGUAR_32BIT_MEM_TYPE	0x200
#define	JAGUAR_BOARD_MEM_TYPE	0x300

#define	JAGUAR_NORMAL_MODE_XFER	0x000
#define JAGUAR_BLOCK_MODE_XFER	0x400
#define	JAGUAR_NO_INC_MODE_XFER	0x800

/*
 * UnitAddress parameter for scsiCmd type parameters.
 * JAGUAR_UNIT_ADDRESS() - Form an SCSI unit address from the Bus,
 * targetID and logical unit number. This macro can't handle extended 
 * addressing.
 */

#define	JAGUAR_UNIT_ADDRESS(bus, targetID, lun) \
		((targetID) | ((lun)<<3) | ((bus)<<6))

/*
 * Work Queue options.  This constants define the options field for
 * the INIT_WORK_QUEUE_CMD type commands.
 * Page 64-65
 *
 * ABORT_ENABLE	- Abort all commands in work queue afther error.
 * FREEZE_QUEUE - Freeze the work queue on any error.
 * PARITY_ENABLE - Enable SCSI bus parity checking.
 * INIT_QUEUE	 - Initialize queue if it is already initialize.
 */

#define	JAGUAR_WQ_ABORT_ENABLE	0x1
#define	JAGUAR_WQ_FREEZE_QUEUE  0x4
#define	JAGUAR_WQ_PARITY_ENABLE	0x8
#define	JAGUAR_WQ_INIT_QUEUE	0x8000

/*
 * Controller error code names:
 */
#define	JAGUAR_ERROR_CODES { \
	{ 0x00, "Good Status"} , /* MACSI/controller error codes. 0x00-0x0a */ \
	{ 0x01, "Queue Full Error"} , \
	{ 0x02, "Work Queue Initialization Error"}, \
	{ 0x03, "First Command Error"}, \
	{ 0x04, "Command Code Error"}, \
	{ 0x05, "Queue Number Error"}, \
	{ 0x06, "Queue Already Initialized"}, \
	{ 0x07, "Queue Un-Initialized"}, \
	{ 0x08, "Queue Mode Not Ready"}, \
	{ 0x09, "Command Unavailable"}, \
	{ 0x0a, "Priority Error"}, \
	{ 0x10, "Reserved Field Error"}, /* General error code information. */ \
	{ 0x11, "Reset Bus Status"}, \
	{ 0x12, "Port 2 Unavailable"}, \
	{ 0x13, "SCSI ID Error"}, \
	{ 0x14, "SCSI Bus Reset Status"}, \
	{ 0x15, "Command Aborted by Reset"}, \
	{ 0x20, "VME Bus Error"}, 	/* VME Errors. */ \
	{ 0x21, "VME Timeout Error"}, \
	{ 0x23, "VME Illegal Address"}, \
	{ 0x24, "VME Illegal Memory Type"}, \
	{ 0x25, "VME Illegal Count Specified"}, \
	{ 0x30, "SCSI Selection Timeout Error"}, 	/* SCSI Errors. */ \
	{ 0x31, "SCSI Discounnect Timeout Error"}, \
	{ 0x32, "SCSI Error"}, \
	{ 0x30, "SCSI Transfer Count Exception"}, \
	{ 0x80, "Flush on error in progress"}, \
	{ 0x81, "Flush work queue status"}}


/*
 * Scatter/gather element list Format. 
 */
typedef struct JaguarSG {
    unsigned short	byteCount;	/* Number of bytes in element. */
    unsigned short	bufferAddr[2];	/* Address of data buffer. */
    unsigned short	addressModifier; /* Address modifier of data buffer. */
} JaguarSG;

#define	JAGUAR_SG_SIZE	8
 /*
  ************************************************************************
  *			CRB
  ************************************************************************
  * The command response block is used by the Jaguar to post command 
  * completion status and data.
  * Page 22.
  */

typedef struct JaguarCRB {
    unsigned short status;	/* Command response status word (CRSW). 
				 * Used for handshake and respose type.
				 * Defined below.
				 */
    unsigned short reserved;	/* zero. */
    unsigned short commandTag[2]; /* Command tag from CQE. */
    unsigned char  iopbLength;	/* IOBP length from CQE. */
    unsigned char  workQueue;	/* Work queue number from CQE. */ 
    unsigned short reserved2;	/* zero. */
    JaguarIOPB	   iopb;	/* IOPB of command. */
} JaguarCRB;

/*
 * JAGUAR_CRB_SIZE - The size in bytes of the CRB according to 
 * the Jaguar.  This should equal sizeof(JaguarCRB)-sizeof(JaguarIOPB)
 * or all bets are off.
 * Page 22
 */

#define	JAGUAR_CRB_SIZE	12

/*
 * The Command response status word (CRSW).  Bits 7-15 reserved.
 *
 * BLOCK_VALID		 - The CRB is valid.  
 * CLEAR_INTERRUPT 	 - Used by the host to ack the CRB.
 * COMMAND_COMPLETE	 - The CRB is for a command completion and not a
 *			   queue available interrupt.
 * ERROR		 - The command completed with an error.
 * EXCEPTION		 - The command completed with a SCSI exception.
 * ABORTED		 - The command was aborted.
 * QUEUE_START		 - The command was a queue start.
 * QUEUE_AVAILABLE	 - The interrupt was due to a queue available condition.
 * 
 */


/* procedures */

#define	JAGUAR_CRB_BLOCK_VALID		  0x1
#define	JAGUAR_CRB_CLEAR_INTERRUPT	  JAGUAR_CRB_BLOCK_VALID 
#define	JAGUAR_CRB_COMMAND_COMPLETE	  0x2
#define	JAGUAR_CRB_ERROR		  0x4
#define	JAGUAR_CRB_EXCEPTION		  0x8
#define	JAGUAR_CRB_ABORTED		 0x10
#define	JAGUAR_CRB_QUEUE_START		 0x20
#define	JAGUAR_CRB_QUEUE_AVAILABLE	 0x40


#endif /* __JAGUAR_HBA_INT */

