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
#endif /* not lint */

#include "sprite.h"
#include "scsi3.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdlib.h"

#include "dbg.h"

/*
 *
 *      Definitions for Sun's second variant on the SCSI device interface.
 *	This interface is  found on 32-bit VME versions, i.e. some plug-in
 *	controllers, and the 3/{56}0.  The associated paper reference is
 *	"Hardware Reference Manual for the Sun-3 SCSI Board".  This explains
 *	general behavior of the VME version of the SCSI-3 interface.
 *	The reference for the 5380 is the NCR Standard Products Data Book,
 *	Micro-electronics Division.  Page number references refer to the
 *	4/88 (April 88) edition.  The UDC (Universal DMA Controller) chip
 *	is the AMD 9516 and the AMD reference manual can be consulted for chip
 *	specifics.
 *
 *
 *	The 5380 has 8 general registers.  They have different functions
 *	when read and written.  See pp. 80-85 for register descriptions.
 *	This chip allows direct control over the SCSI bus by the CPU,
 *	so many bits correspond directly to SCSI bus signals.
 */
typedef struct ReadRegs {
    unsigned char data;		/* Data register.  A direct connection to
				 * the SCSI data bus.  This is read during
				 * programmed I/O to get msgs, and during
				 * arbitration. */
    unsigned char initCmd;	/* Initiator command register */
    unsigned char mode;		/* Mode register */
    unsigned char trgtCmd;	/* Target command register */
    unsigned char curStatus;	/* All SCSI signals except ATN and ACK */
    unsigned char status;	/* ATN, ACK, plus DMA and interrupt signals */
    unsigned char inData;	/* Input data register.  Used for "latched"
				 * data on the SCSI bus during DMA.  This
				 * is not accessed directly by the driver. */
    unsigned char clear;	/* Read this to clear the following bits
				 * in the status register: parity error,
				 * interrupt request, busy failure. */
} ReadRegs;

/*
 * The following format applies when writing the registers.
 */
typedef struct WriteRegs {
    unsigned char data;		/* Data register.  Contains the ID of the
				 * SCSI "target", or controller, for the 
				 * SELECT phase. Also, leftover odd bytes
				 * are left here after a read. */
    unsigned char initCmd;	/* Initiator command register */
    unsigned char mode;		/* Mode register */
    unsigned char trgtCmd;	/* Target command register */
    unsigned char select;	/* Select/reselect enable register */
    /*
     * DMA is initiated by writing to these registers.  The TARGET mode
     * bit should be set right, i.e. cleared before writing to
     * the send or initRecv registers, and the DMA mode bit should be set.
     */
    unsigned char send;		/* Start DMA from memory to SCSI bus */
    unsigned char trgtRecv;	/* Start DMA from SCSI bus to target */
    unsigned char initRecv;	/* Start DMA from SCSI bus to initiator */
} WriteRegs;

/*
 * Control bits in the 5380 Initiator Command Register.
 * RST, ACK, BSY, SEL, ATN are direct connections to SCSI control lines.
 * Setting or clearing the bit raises or lowers the SCSI signal.
 * Reading these bits indicates the current value of the control signal.
 */
#define	SBC_ICR_RST	0x80	/* (r/w) SCSI RST (reset) signal */
#define SBC_ICR_AIP	0x40	/* (r)   arbitration in progress */
#define SBC_ICR_TEST	0x40	/* (w)   test mode, disables output */
#define SBC_ICR_LA	0x20	/* (r)   lost arbitration */
#define SBC_ICR_DE	0x20	/* (w)   differential enable (5381 only) */
#define SBC_ICR_ACK	0x10	/* (r/w) SCSI ACK (acknowledge) signal */
#define SBC_ICR_BUSY	0x08	/* (r/w) SCSI BSY (busy) signal */
#define SBC_ICR_SEL	0x04	/* (r/w) SCSI SEL (select) signal */
#define SBC_ICR_ATN	0x02	/* (r/w) SCSI ATN (attention) signal */
#define SBC_ICR_DATA	0x01	/* (r/w) assert data bus.  Enables the outData
				 * contents to be output on the SCSi data lines.
				 * This should be set during DMA send. */

/*
 * Bits in the 5380 Mode Register (same on read or write).
 * "This is used to control the operation of the chip."
 * The mode controls DMA, target/initiator roles, parity, and interrupts.
 */
#define SBC_MR_BDMA	0x80	/* Enable block mode dma */
#define SBC_MR_TRG	0x40	/* Target mode when set, else Initiator */
#define SBC_MR_EPC	0x20	/* Enable parity check */
#define SBC_MR_EPI	0x10	/* Enable parity interrupt */
#define SBC_MR_EEI	0x08	/* Enable eop (end-of-process, dma) interrupt */
#define SBC_MR_MBSY	0x04	/* Enable monitoring of BSY (busy) signal */
#define SBC_MR_DMA	0x02	/* Enable DMA.  Used with other DMA regs. */
#define SBC_MR_ARB	0x01	/* Set during SCSI bus arbitration */

/*
 * Bits in the 5380 Target Command Register.
 * As an Initator, which we always are, this register must be set to
 * match the current phase that's on the SCSI bus before sending data.
 */
#define SBC_TCR_REQ	0x08	/* assert request.  Only for targets. */
#define SBC_TCR_MSG	0x04	/* message phase, if set */
#define SBC_TCR_CD	0x02	/* command phase if set, else data phase */
#define SBC_TCR_IO	0x01	/* input phase if set, else output */

/*
 * Combinations for different phases as represented in the target cmd. reg.
 */
#define TCR_COMMAND	(SBC_TCR_CD)
#define TCR_STATUS	(SBC_TCR_CD | SBC_TCR_IO)
#define TCR_MSG_OUT	(SBC_TCR_MSG | SBC_TCR_CD)
#define TCR_MSG_IN	(SBC_TCR_MSG | SBC_TCR_CD | SBC_TCR_IO)
#define TCR_DATA_OUT	0
#define TCR_DATA_IN	(SBC_TCR_IO)
#define TCR_UNSPECIFIED	(SBC_TCR_MSG)

/*
 * Bits in the 5380 Current SCSI Bus Status register (read only).
 * This register is used to monitor the current state of all of
 * the SCSI bus lines except ATN (attention) and ACK (acknowledge).
 */
#define SBC_CBSR_RST	0x80	/* reset */
#define SBC_CBSR_BSY	0x40	/* busy */
#define SBC_CBSR_REQ	0x20	/* request */
#define SBC_CBSR_MSG	0x10	/* message */
#define SBC_CBSR_CD	0x08	/* command/data */
#define SBC_CBSR_IO	0x04	/* input/output */
#define SBC_CBSR_SEL	0x02	/* select */
#define SBC_CBSR_DBP	0x01	/* data bus parity */

/*
 * Combinations for different phases as represented on the SCSI bus.
 * COMMAND phase is used to send a command block to a Target.
 * STATUS phase is used to get status bytes from a Target.
 * MSG_OUT phase is used to send message bytes to a Target.
 * MSG_IN phase is used to get a message from a Target.
 * DATA_OUT phase is used to send data to a Target.
 * DATA_IN phase is used when receiving data from a Target.
 */
#define CBSR_PHASE_BITS	(SBC_CBSR_CD | SBC_CBSR_MSG | SBC_CBSR_IO)
#define PHASE_COMMAND	(SBC_CBSR_CD)
#define PHASE_STATUS	(SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_MSG_OUT	(SBC_CBSR_MSG | SBC_CBSR_CD)
#define PHASE_MSG_IN	(SBC_CBSR_MSG | SBC_CBSR_CD | SBC_CBSR_IO)
#define PHASE_DATA_OUT	0
#define PHASE_DATA_IN	(SBC_CBSR_IO)

/*
 * Bits in the 5380 Bus and Status register.  This has the ATN and ACK
 * SCSI lines, plus other status bits.
 */
#define SBC_BSR_EDMA	0x80	/* End of dma, almost, see p. 84 */
#define SBC_BSR_RDMA	0x40	/* DRQ (dma request) signal, set during DMA */
#define SBC_BSR_PERR	0x20	/* Parity error */
#define SBC_BSR_INTR	0x10	/* IRQ (interrupt request) */
#define SBC_BSR_PMTCH	0x08	/* Phase match indicates if trgtCmd is ok */
#define SBC_BSR_BERR	0x04	/* Busy error set when BSY goes away */
#define SBC_BSR_ATN	0x02	/* SCSI ATN (attention) signal */
#define SBC_BSR_ACK	0x01	/* SCSI ACK (acknowledge)_signal */

/*
 * AMD 9516 UDC (Universal DMA Controller) Registers.
 * Sun3/50 and Sun3/60.
 */

/* addresses of the udc registers accessed directly by driver */
#define UDC_ADR_MODE		0x38	/* master mode register */
#define UDC_ADR_COMMAND		0x2e	/* command register (write only) */
#define UDC_ADR_STATUS		0x2e	/* status register (read only) */
#define UDC_ADR_CAR_HIGH	0x26	/* chain addr reg, high word */
#define UDC_ADR_CAR_LOW		0x22	/* chain addr reg, low word */
#define UDC_ADR_CARA_HIGH	0x1a	/* cur addr reg A, high word */
#define UDC_ADR_CARA_LOW	0x0a	/* cur addr reg A, low word */
#define UDC_ADR_CARB_HIGH	0x12	/* cur addr reg B, high word */
#define UDC_ADR_CARB_LOW	0x02	/* cur addr reg B, low word */
#define UDC_ADR_CMR_HIGH	0x56	/* channel mode reg, high word */
#define UDC_ADR_CMR_LOW		0x52	/* channel mode reg, low word */
#define UDC_ADR_COUNT		0x32	/* number of words to transfer */

/* 
 * For a dma transfer, the appropriate udc registers are loaded from a 
 * table in memory pointed to by the chain address register.
 */
typedef struct UDCDMAtable {
	unsigned short	rsel;	/* tells udc which regs to load */
	unsigned short	haddr;	/* high word of main mem dma address */
	unsigned short	laddr;	/* low word of main mem dma address */
	unsigned short	count;	/* num words to transfer */
	unsigned short	hcmr;	/* high word of channel mode reg */
	unsigned short	lcmr;	/* low word of channel mode reg */
} UDCDMAtable;

/* indicates which udc registers are to be set based on info in above table */
#define UDC_RSEL_RECV		0x0182
#define UDC_RSEL_SEND		0x0282

/* setting of chain mode reg: selects how the dma op is to be executed */
#define UDC_CMR_HIGH		0x0040	/* high word of channel mode reg */
#define UDC_CMR_LSEND		0x00c2	/* low word of cmr when send */
#define UDC_CMR_LRECV		0x00d2	/* low word of cmr when receiving */

/* setting for the master mode register */
#define UDC_MODE		0xd	/* enables udc chip */

/* setting for the low byte in the high word of an address */
#define UDC_ADDR_INFO		0x40	/* inc addr after each word is dma'd */

/* udc commands */
#define UDC_CMD_STRT_CHN	0xa0	/* start chaining */
#define UDC_CMD_CIE		0x32	/* channel 1 interrupt enable */
#define UDC_CMD_RESET		0x00	/* reset udc, same as hdw reset */

/* bits in the udc status register */
#define UDC_SR_CIE		0x8000	/* channel interrupt enable */
#define UDC_SR_IP		0x2000	/* interrupt pending */
#define UDC_SR_CA		0x1000	/* channel abort */
#define UDC_SR_NAC		0x0800	/* no auto reload or chaining*/
#define UDC_SR_WFB		0x0400	/* waiting for bus */
#define UDC_SR_SIP		0x0200	/* second interrupt pending */
#define UDC_SR_HM		0x0040	/* hardware mask */
#define UDC_SR_HRQ		0x0020	/* hardware request */
#define UDC_SR_MCH		0x0010	/* match on upper comparator byte */
#define UDC_SR_MCL		0x0008	/* match on lower comparator byte */
#define UDC_SR_MC		0x0004	/* match condition ended dma */
#define UDC_SR_EOP		0x0002	/* eop condition ended dma */
#define UDC_SR_TC		0x0001	/* termination of count ended dma */

/*
 * Misc defines 
 */
/*
 * Values for the reset argument of WaitReg.
 */
#define RESET		TRUE
#define NO_RESET	FALSE

/* arbitrary retry count */
#define SI_NUM_RETRIES		2
/*
 * WAIT_LENGTH - the number of microseconds that the host waits for
 *	various control lines to be set on the SCSI bus.  The largest wait
 *	time is when a controller is being selected.  This delay is
 *	called the Bus Abort delay and is about 250 milliseconds.
 */

#define	WAIT_LENGTH		250000

/* scsi timer values, all in microseconds */
#define SI_ARBITRATION_DELAY	3
#define SI_BUS_CLEAR_DELAY	1
#define SI_BUS_SETTLE_DELAY	1
#define SI_UDC_WAIT		1
#define	SI_WAIT_COUNT		250000

/* directions for dma transfers */
#define SI_RECV_DATA		0
#define SI_SEND_DATA		1
#define SI_NO_DATA		2

/* initiator's scsi device id */
#define	SI_HOST_ID		0x80
/*
 * INTR_ADDR(vector) - Compute the correct interruptAddr modifier
 * for the VME version of this interface. Vector is the VME interrupt
 * to use.  The high order 8 bits is the address space modiifer.
 * The correct value 0x3d - 24 bit address Supervisor data space. 
 * We shift it right 8 bits to leave room the the interrupt vector
 * we or in.
 */
#define INTR_ADDR(vector)	((0x3d<<8)|(vector))

/*
 * Maximum data transfer size for the onBoard HBA appears to be
 * limited by the 16 bit dma register. Since this counter is in 
 * words we can send up to 127 K but not 128 K. The DMA counter
 * on the VME version is 24 bits.  As it turns out both this values are
 * limited by the size of the mapped DMA buffer DEV_MAX_TRANSFER_SIZE.
 */

#define	MAX_ONBOARD_TRANSFER_SIZE	(64*1024)

#define	MAX_VME_TRANSFER_SIZE		(64*1024)

/* 
 * Register layout for the SCSI control logic interface.
 * Some of these registers apply to only one interface and some
 * apply to both. The registers which apply to the Sun3/50 onboard 
 * version only are udc_rdata and udc_raddr. The registers which
 * apply to the Sun3 vme version only are dma_addr, dma_count, bpr,
 * iv_am, and bcrh. Thus, the sbc registers, fifo_data, bcr, and csr 
 * apply to both interfaces.
 * One other feature of the vme interface: a write to the dma count 
 * register also causes a write to the fifo byte count register and
 * vice-versa.
 */
typedef struct CtrlRegs {
	union {
		struct ReadRegs	read;	/* scsi bus ctrl, read reg */
		struct WriteRegs	write;	/* scsi bus ctrl, write reg */
	} sbc;					/* SBC 5380 registers, 8 bytes*/
	unsigned short		dmaAddressHigh;	/* dma address register High */
	unsigned short		dmaAddressLow;	/* dma address register Low */
	unsigned short		dmaCountHigh;	/* dma count register High */
	unsigned short		dmaCountLow;	/* dma count register low*/
	unsigned short		udcRdata;	/* UDC, reg data */
	unsigned short		udcRaddr;	/* UDC, reg addr */
	unsigned short		fifoData;	/* fifo data register */
						/* holds extra byte on odd */
						/* byte dma read */
	unsigned short		fifoCountLow;	/* fifo byte count reg */
	unsigned short		control;	/* control/status register */
	unsigned short		bytePackHigh;	/* Byte 0 and Byte 1 */
	unsigned short		bytePackLow;	/* Byte 2 and Byte 3 */
	unsigned short		addrIntr;	/* bits 0-7: addr modifier */
						/* bits 8-13: intr vector */
						/* bits 14-15: unused */
	unsigned short		fifoCountHigh;	/* high portion of fifoCount */
} CtrlRegs;

/*
 * The dmaAddress, dmaCount, and fifoCount are 32 or 24 bit registers with only
 * a 16 bit path to them. Because of this we have the following macros
 * for setting and reading them. 
 * SET_FIFO_COUNT()	- Set the FIFO count register to the provided value.
 * READ_FIFO_COUNT()	- Set the FIFO count register.
 * SET_DMA_COUNT()	- Set the DMA count register to the provided value.
 * READ_DMA_COUNT()	- Read the DMA count register.
 * SET_DMA_ADDR()	- Set the DMA address register to the provided value.
 * READ_DMA_ADDR()	- Read the DMA address register.
 */
#ifdef bigio_works
#define	SET_FIFO_COUNT(regsPtr, value)	{\
	    (regsPtr)->fifoCountLow = ((unsigned) (value) & 0xffff);	      \
	    (regsPtr)->fifoCountHigh = (((unsigned)(value) >> 16) & 0xff);    \
	}
#define	READ_FIFO_COUNT(regsPtr)	\
	   ((((regsPtr)->fifoCountHigh&0xff) << 16) | ((regsPtr)->fifoCountLow))

#define	SET_DMA_COUNT(regsPtr, value)	{\
	    (regsPtr)->dmaCountLow = ((unsigned) (value) & 0xffff);	      \
	    (regsPtr)->dmaCountHigh = (((unsigned)(value) >> 16) & 0xff);    \
	    }
#define	READ_DMA_COUNT(regsPtr)	\
	    ((((regsPtr)->dmaCountHigh&0xff) << 16) | ((regsPtr)->dmaCountLow))
#else
#define	SET_FIFO_COUNT(regsPtr, value)	{\
	    (regsPtr)->fifoCountLow = ((unsigned) (value) & 0xffff);	      \
	    (regsPtr)->fifoCountHigh = (((unsigned)(value) >> 16) & 0x0);    \
	}
#define	READ_FIFO_COUNT(regsPtr)	\
	   ((((regsPtr)->fifoCountHigh&0x0) << 16) | ((regsPtr)->fifoCountLow))

#define	SET_DMA_COUNT(regsPtr, value)	{\
	    (regsPtr)->dmaCountLow = ((unsigned) (value) & 0xffff);	      \
	    (regsPtr)->dmaCountHigh = (((unsigned)(value) >> 16) & 0x0);    \
	    }
#define	READ_DMA_COUNT(regsPtr)	\
	    ((((regsPtr)->dmaCountHigh&0x0) << 16) | ((regsPtr)->dmaCountLow))
#endif
#define	SET_DMA_ADDR(regsPtr, value)	{\
	    (regsPtr)->dmaAddressLow = ((unsigned) (value) & 0xffff);	      \
	    (regsPtr)->dmaAddressHigh = (((unsigned)(value) >> 16) & 0xffff); \
	}
#define	READ_DMA_ADDR(regsPtr)	\
	    (((regsPtr)->dmaAddressHigh << 16) | ((regsPtr)->dmaAddressLow))

/*
 * Status Register.
 * Note:
 *	(r)	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 *	(v)	vme host adaptor interface only.
 *	(o)	sun3/50 onboard host adaptor interface only.
 *	(b)	both vme and sun3/50 host adaptor interfaces.
 */
#define SI_CSR_DMA_ACTIVE	0x8000	/* (r,o) dma transfer active */
#define SI_CSR_DMA_CONFLICT	0x4000	/* (r,b) reg accessed while dmaing */
#define SI_CSR_DMA_BUS_ERR	0x2000	/* (r,b) bus error during dma */
#define SI_CSR_ID		0x1000	/* (r,b) 0 for 3/50, 1 for SCSI-3, */
					/* 0 if SCSI-3 unmodified */
#define SI_CSR_FIFO_FULL	0x0800	/* (r,b) fifo full */
#define SI_CSR_FIFO_EMPTY	0x0400	/* (r,b) fifo empty */
#define SI_CSR_SBC_IP		0x0200	/* (r,b) sbc interrupt pending */
#define SI_CSR_DMA_IP		0x0100	/* (r,b) dma interrupt pending */
#define SI_CSR_LOB		0x00c0	/* (r,v) number of leftover bytes */
#define SI_CSR_LOB_THREE	0x00c0	/* (r,v) three leftover bytes */
#define SI_CSR_LOB_TWO		0x0080	/* (r,v) two leftover bytes */
#define SI_CSR_LOB_ONE		0x0040	/* (r,v) one leftover byte */
#define SI_CSR_BPCON		0x0020	/* (rw,v) byte packing control */
					/* dma is in 0=longwords, 1=words */
#define SI_CSR_DMA_EN		0x0010	/* (rw,v) dma enable */
#define SI_CSR_SEND		0x0008	/* (rw,b) dma dir, 1=to device */
#define SI_CSR_INTR_EN		0x0004	/* (rw,b) interrupts enable */
#define SI_CSR_FIFO_RES		0x0002	/* (rw,b) inits fifo, 0=reset */
#define SI_CSR_SCSI_RES		0x0001	/* (rw,b) reset sbc and udc, 0=reset */


/*
 * devSCSI3Debug - debugging level
 *	2 - normal level
 *	4 - one print per command in the normal case
 *	5 - traces interrupts
 */
int devSCSI3Debug = 2;

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
			 * the LUN is store in the device handle.  */
    Controller *ctrlPtr;	/* Controller to which device is attached. */
		   /*
		    * The following part of this structure is 
		    * used to handle SCSI commands that return 
		    * CHECK status. To handle the REQUEST SENSE
		    * command we must: 1) Save the state of the current
		    * command into the "struct FrozenCommand". 2) Submit
		    * a request sense command formatted in SenseCmd
		    * to the device.  */
    struct FrozenCommand {		       
	ScsiCmd	*scsiCmdPtr;	   /* The frozen command. */
	unsigned char statusByte; /* It's SCSI status byte, Will always have
				   * the check bit set.
				   */
	int amountTransferred;    /* Number of bytes transferred by this 
				   * command.
				   */
    } frozen;	
    char senseBuffer[DEV_MAX_SENSE_BYTES]; /* Data buffer for request sense */
    ScsiCmd		SenseCmd;  	   /* Request sense command buffer. */
} Device;

/*
 * Controller - The Data structure describing a sun SCSI3 controller. One
 * of these structures exists for each active SCSI3 HBA on the system. Each
 * controller may have from zero to 56 (7 targets each with 8 logical units)
 * devices attached to it. 
 */
struct Controller {
    volatile CtrlRegs *regsPtr; /* Pointer to the registers of
                                    this controller. */
    Boolean  onBoard;	/* TRUE if this is a on board version of the 
			 * controller such as in the Sun 3/50, 3/60. FALSE
			 * if it is the VME version.
			 */
    UDCDMAtable	*udcDmaTable; /* Table for the onBoard's DMA chip. */
    int	    dmaState;	/* DMA state for this controller, defined below. */
    Boolean dmaSetup;	/* TRUE if the DMA register have been setup. Only used
			 * for the VME version. */
    char    *name;	/* String for error message for this controller.  */
    DevCtrlQueues devQueues;    /* Device queues for devices attached to this
				 * controller.	 */
    Sync_Semaphore mutex; /* Lock protecting controller's data structures. */
			  /* Until disconnect/reconnect is added we can have
			   * only one current active device and scsi command.*/
    Device     *devPtr;	   /* Current active command. */
    ScsiCmd   *scsiCmdPtr; /* Current active command. */
    Address    dmaBuffer; /* DMA buffer allocated for this address. */
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
 * MAX_SCSI3_CTRLS - Maximum number of SCSI3 controllers attached to the
 *		     system. We set this to the maximum number of VME slots
 *		     in any Sun system currently available.
 */
#define	MAX_SCSI3_CTRLS	16
static Controller *Controllers[MAX_SCSI3_CTRLS];
/*
 * Highest number controller we have probed for.
 */
static int numSCSI3Controllers = 0;

/*
 * Forward declarations.  
 */

static void		Reset();
static ReturnStatus	SendCommand();
static ReturnStatus	GetStatusByte();
static ReturnStatus	WaitPhase();
static ReturnStatus	WaitReg();
static ReturnStatus GetByte();
static ReturnStatus PutByte();
static void 	PrintRegs();
static void 	StartDMA();



/*
 *----------------------------------------------------------------------
 *
 * ProbeOnBoard --
 *
 *	Test of the existance for the onboard SCSI-3 interface.
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
static ProbeOnBoard(address)
    int address;			/* Alledged controller address */
{
    ReturnStatus	status;
    register volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)address;
    int x;

    /*
     * Touch the device's UDC read data register.
     */
    status = Mach_Probe(sizeof(regsPtr->udcRdata),(char *)&(regsPtr->udcRdata),
			(char *)&x);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 3) {
	    printf("Onboard SCSI3 not found at address 0x%x\n",address);
	}
        return (FALSE);
    }
    if (devSCSI3Debug > 3) {
	printf("Onboard SCSI3 found\n");
    }
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * ProbeVME --
 *
 *	Probe memory for the new-style VME SCSI interface.  This occupies
 *	2K of VME space.
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
ProbeVME(address)
    int address;			/* Alledged controller address */
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)address;
    ReturnStatus	status;

    /*
     * Touch the device. The dmaCount register should hold more
     * than 16 bits, which is all the old host adaptor's dmaCount can hold.
     */
    { 
	unsigned short value = 0xABCC;
	status = Mach_Probe(sizeof(regsPtr->dmaCountLow), (char *) &value,
			(char *) &(regsPtr->dmaCountLow));
	value = 0x4A;
	status = Mach_Probe(sizeof(regsPtr->dmaCountHigh),
			   (char *) &value, (char *) &(regsPtr->dmaCountHigh));
  }
    if (status != SUCCESS) {
	return (FALSE);
    }
    if (regsPtr->dmaCountLow != 0xABCC) {
	 printf("Warning: ProbeSCSI-3 read back problem %x not %x\n",
		READ_DMA_COUNT(regsPtr), 0xABCC);
	return (FALSE);
    } 

    if (devSCSI3Debug > 3) {
	printf("VME SCSI3 found\n");
    }
    return(TRUE);
}


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
    unsigned char clear;

    SET_FIFO_COUNT(regsPtr,0);
    regsPtr->control = 0;
    MACH_DELAY(100);
    regsPtr->control = SI_CSR_SCSI_RES | SI_CSR_FIFO_RES;

    if (!ctrlPtr->onBoard) {
	SET_DMA_COUNT(regsPtr,0);
	SET_DMA_ADDR(regsPtr,0);
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
 * SendCommand --
 *
 *      Send a command to a SCSI controller via the SCSI-3 Host Adaptor.
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
    ReturnStatus status;
    register volatile CtrlRegs *regsPtr; /* Host Adaptor registers */
    register volatile unsigned char *initCmdPtr; /* pointer to initCmd reg */
    register volatile unsigned char *modePtr; /* pointer to mode register */
    register char *charPtr;
    Controller	*ctrlPtr;
    int i;
    int size;				/* Number of bytes to transfer */
    Address addr;			/* Kernel address of transfer */

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr);
    ctrlPtr->dmaBuffer = (Address) NIL;
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
    ctrlPtr->dmaSetup = FALSE;
    if (devSCSI3Debug > 3) {
	printf("SCSI3Command: %s addr %x size %d dma %s\n",
	    devPtr->handle.locationName, addr, size,
	    (ctrlPtr->dmaState == DMA_INACTIVE) ? "not active" :
		((ctrlPtr->dmaState == DMA_SEND) ? "send" :
						      "receive"));
    }

    regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    initCmdPtr = &regsPtr->sbc.write.initCmd;
    modePtr = &regsPtr->sbc.write.mode;
    /*
     * Clear all control lines.
     */
    if (!ctrlPtr->onBoard) {
	/*
	 * For VME interface dis-allow DMA interrupts from reconnect attempts.
	 */
	regsPtr->control &= ~SI_CSR_DMA_EN;
	regsPtr->control |= SI_CSR_BPCON;	/* word byte packing */
    }
    regsPtr->sbc.write.select = 0;
    regsPtr->sbc.write.trgtCmd = 0;
    regsPtr->sbc.write.initCmd = 0;
    *modePtr &= ~SBC_MR_DMA;

    /*
     * SCSI ARBITRATION.
     *
     * Arbitrate for the SCSI bus by putting our SCSI ID on the data
     * bus and asserting the BUSY signal.  After an arbitration delay
     * we look for other, higher priority IDs on the bus. (We won't find
     * any because the Host Adaptor is wired in to be the highest.)
     * Arbitration is completed by asserting the SELECT line.
     */

    regsPtr->sbc.write.data = SI_HOST_ID;
    for (i = 0; i < SBC_NUM_RETRIES; i++) {
	/*
	 * Wait for the bus to go to BUS FREE - busy line not held.
	 */
	for (i=0 ; i < WAIT_LENGTH ; i++) {
	    if ((regsPtr->sbc.read.curStatus & SBC_CBSR_BSY) == 0) {
		break;
	    } else {
		MACH_DELAY(10);
	    }
	}
	if (i == WAIT_LENGTH) {
	    /*
	     * Probably a higher level synchronization error.  The
	     * SCSI bus is probably busy with another transaction.
	     */
	    printf("Warning: %s SCSI bus stuck busy\n",ctrlPtr->name);
	    Reset(ctrlPtr);
	    return(FAILURE);
	}
	/*
	 * Enter Arbitration mode on the chip.
	 */
	*modePtr |= SBC_MR_ARB;
	status = WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.initCmd,
			       REG_BYTE, SBC_ICR_AIP, NO_RESET, ACTIVE_HIGH);
	if (status == DEV_TIMEOUT) {
	    continue;
	}
	if (status != SUCCESS) {
	    regsPtr->sbc.write.data = 0;
	    *modePtr &= ~SBC_MR_ARB;
	    printf("Warning: %s arbitration failed on %s\n", 
			    ctrlPtr->name, devPtr->handle.locationName);
	    return(status);
	}
	MACH_DELAY(SI_ARBITRATION_DELAY);
	if (((regsPtr->sbc.read.initCmd & SBC_ICR_LA) == 0) &&
	    ((regsPtr->sbc.read.data & ~SI_HOST_ID)  < SI_HOST_ID)) {
	    break;
	}
	/*
	 * Lost arbitration due to reselection attempt by a target.
	 */
	*modePtr &= ~SBC_MR_ARB;
	printf("Warning: %s lost arbitration\n",ctrlPtr->name);
	/*
	 * A target may have tried to select us during arbitration phase.
	 * At this point we should save the current command and
	 * respond to the reconnection interrupt.
	 */
	if ((regsPtr->sbc.read.curStatus & SBC_CBSR_SEL) && 
	    (regsPtr->sbc.read.curStatus & SBC_CBSR_IO) &&
	    (regsPtr->sbc.read.data & SI_HOST_ID)) {
	    printf("Warning: %s someone attempted to reselect.\n",ctrlPtr->name);
	    return(FAILURE);
	}
    }
    if (i == SBC_NUM_RETRIES) {
	Reset(ctrlPtr);
	printf("Warning: %s unable to select target %s\n", ctrlPtr->name,
				devPtr->handle.locationName);
	return(FAILURE);	
    }

    /*
     * Arbitration complete.  Confirm by setting SELECT and BUSY.
     * The ATN (attention) line would be set here if we want allow
     * disconnection by the target.
     */
    *initCmdPtr = SBC_ICR_SEL | SBC_ICR_BUSY;
    *modePtr &= ~SBC_MR_ARB;
    MACH_DELAY(SI_BUS_CLEAR_DELAY + SI_BUS_SETTLE_DELAY);
    /*
     * SCSI SELECTION.
     *
     * Select the target by putting its ID plus our own on the bus
     * and waiting for the target to assert the BUSY signal.  We
     * drop SEL and DATA after the target responds.
     */
    regsPtr->sbc.write.data = (1 << devPtr->targetID) | SI_HOST_ID;
    *initCmdPtr = SBC_ICR_SEL | SBC_ICR_DATA | SBC_ICR_BUSY;
    MACH_DELAY(1);
    *initCmdPtr  &= ~SBC_ICR_BUSY;
    status = WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.curStatus,
			   REG_BYTE, SBC_CBSR_BSY, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	printf("Warning: %s can't select %s\n", 
			ctrlPtr->name, devPtr->handle.locationName);
	regsPtr->sbc.write.data = 0;
	return(status);
    }
    *initCmdPtr &= ~(SBC_ICR_SEL | SBC_ICR_DATA);
    /*
     * Clear selection and DMA interrupts.
     */
    regsPtr->sbc.write.select = 0;
    *modePtr &= ~SBC_MR_DMA;

#ifdef reselection
    /*
     * After target selection there is an optional message phase where
     * we send an IDENTIFY message to indicate dis-connect capability.
     */
    data = SCSI_IDENDIFY | devPtr->handle.LUN;
    status = PutByte(ctrlPtr, &data);
    if (status != SUCCESS) {
	return(status);
    }
#endif reselection
#ifdef lint
    status = PutByte(ctrlPtr, (char *) 0);
#endif
    if (ctrlPtr->dmaState != DMA_INACTIVE) {
	if ((unsigned) scsiCmdPtr->buffer < (unsigned) VMMACH_DMA_START_ADDR) {
	    ctrlPtr->dmaBuffer = addr = 
			VmMach_DMAAlloc(size,scsiCmdPtr->buffer);
	} else {
	    /*
	     * Already mapped into DMA space.
	     */
	    addr = scsiCmdPtr->buffer;
	}
	if (devSCSI3Debug > 5) {
	    printf("SCSI3Command: selected %s setup DMA addr 0x%x size %d\n",
		devPtr->handle.locationName, addr, size);
	}
	if (addr == (Address) NIL) {
	    panic("%s can't allocate DMA buffer of %d bytes\n", 
			devPtr->handle.locationName, size);
	}
	/*
	 * DMA SETUP.
	 *
	 * First reset the DMA controllers so they
	 * don't complain with a DMA_CONFLICT interrupt.
	 */
	if (ctrlPtr->onBoard) {
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_COMMAND;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_CMD_RESET;
	    MACH_DELAY(SI_UDC_WAIT);
	} 

	regsPtr->control &= ~SI_CSR_FIFO_RES;    
	regsPtr->control |= SI_CSR_FIFO_RES;    
	if (ctrlPtr->dmaState == DMA_RECEIVE) {
	    regsPtr->control &= ~SI_CSR_SEND;
	} else {
	    regsPtr->control |= SI_CSR_SEND;
	}
	if (ctrlPtr->onBoard) {
	    register UDCDMAtable *udct = ctrlPtr->udcDmaTable;
	    if (devSCSI3Debug > 4) {
		printf("SCSI DMA addr = 0x%x size = %d\n",addr,size);
	    }
	    /*
	     * Set fifoCount which is also wired to dmaCount, thus
	     * both registers are set.  The onboard DMA controller requires
	     * that these counts be set before entering the DATA PHASE.
	     */
	     regsPtr->fifoCountLow = size;
	    /*
	     * The onboard DMA controller expects a control block (!)
	     * that describes the DMA transfer.
	     */
	    udct->haddr = (((unsigned) addr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	    udct->laddr = (unsigned)addr & 0xffff;
	    udct->hcmr = UDC_CMR_HIGH;
	    udct->count = size / 2; /* #bytes -> #words */

	    if (ctrlPtr->dmaState == DMA_RECEIVE) {
		    udct->rsel = UDC_RSEL_RECV;
		    udct->lcmr = UDC_CMR_LRECV;
	    } else {
		    udct->rsel = UDC_RSEL_SEND;
		    udct->lcmr = UDC_CMR_LSEND;
		    if (size & 1) {
			    udct->count++;
		    }
	    }
	    /*
	     * Now we tell the DMA chip where the control block is
	     * by setting the Chain Address Register (CAR).
	     */
	    regsPtr->udcRaddr = UDC_ADR_CAR_HIGH;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = ((int)udct & 0xff0000) >> 8;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_CAR_LOW;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = (int)udct & 0xffff;
	    /*
	     * Tell the chip to be a DMA master.
	     */
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_MODE;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_MODE;
	    /*
	     * Tell the chip to interrupt on error.
	     */
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRaddr = UDC_ADR_COMMAND;
	    MACH_DELAY(SI_UDC_WAIT);
	    regsPtr->udcRdata = UDC_CMD_CIE;

	} else {
	    /*
	     * Clear thing now, and set size later in StartDMA.
	     */
	    SET_FIFO_COUNT(regsPtr,0); 
	    SET_DMA_COUNT(regsPtr,0);
	    if (ctrlPtr->dmaState != DMA_INACTIVE) {
		SET_DMA_ADDR(regsPtr,(unsigned)(addr - VMMACH_DMA_START_ADDR));
	    } else {
		SET_DMA_ADDR(regsPtr,0);
	    }
	}
    } else {
	/*
	 * fifoCount register is wired to dmaCount so both are set.
	 */
	SET_FIFO_COUNT(regsPtr,0);
    }
    regsPtr->control |= SI_CSR_INTR_EN;

    if (devSCSI3Debug > 5) {
	printf("SCSI3Command: %s waiting for command phase.\n",
	    devPtr->handle.locationName);
    }
    status = WaitPhase(ctrlPtr, PHASE_COMMAND, RESET);
    if (status != SUCCESS) {
	/*
	 * After we implement reselection it is at this point that
	 * we have to handle messages from targets.
	 */
	if (devSCSI3Debug > 0) {
	    printf("SCSI3: wait on PHASE_COMMAND failed.\n");
	}
	return(status);
    }
    /*
     * Stuff the control block through the commandStatus register.
     * The handshake on the SCSI bus is visible here:  we have to
     * wait for the Request line on the SCSI bus to be raised before
     * we can send the next command byte to the controller.  Then we
     * have to set the ACK line after putting out the data, and finnaly
     * wait for the REQ line to drop again.
     */
    if (devSCSI3Debug > 5) {
	printf("SCSI3Command: %s stuffing command of %d bytes.\n", 
		devPtr->handle.locationName, scsiCmdPtr->commandBlockLen);
    }
    regsPtr->sbc.write.trgtCmd = TCR_COMMAND;
    charPtr = scsiCmdPtr->commandBlock;
    for (i=0 ; i< scsiCmdPtr->commandBlockLen; i++) {
	/*
	 * SCSI DATA TRANSFER HANDSHAKE.
	 *
	 * Wait for Target to request data byte.
	 */
	status = WaitReg(ctrlPtr, (Address)&regsPtr->sbc.read.curStatus,
			       REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
	if (status != SUCCESS) {
	    printf("Warning: %s couldn't send SCSI command block byte %d\n",
				 ctrlPtr->name, i);
	    return(status);
	}
	/*
	 * Gate data onto SCSI bus and then set ACK.
	 */
	regsPtr->sbc.write.data = *charPtr;
	regsPtr->sbc.write.initCmd = SBC_ICR_DATA;
	regsPtr->sbc.write.initCmd |= SBC_ICR_ACK;
	/*
	 * Wait for Target to take byte and drop REQ.
	 */
	status = WaitReg(ctrlPtr, (Address)&regsPtr->sbc.read.curStatus,
			       REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
	if (status != SUCCESS) {
	    printf("Warning: %s: request line didn't go low.\n",
				 ctrlPtr->name);
	    return(status);
	}
	if (devSCSI3Debug > 5) {
	    printf("0x%x ", *charPtr);
	}
	charPtr++;
	if (i < scsiCmdPtr->commandBlockLen - 1) {
	    /*
	     * Finally we drop the ACK line.
	     */
	    regsPtr->sbc.write.initCmd = 0;
	}
    }
    if (devSCSI3Debug > 5) {
	printf("\n");
    }

    i = regsPtr->sbc.read.clear;
    regsPtr->sbc.write.select = SI_HOST_ID;
    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
    *modePtr |= SBC_MR_DMA;
    regsPtr->sbc.write.initCmd = 0;
    if (!ctrlPtr->onBoard) {
	regsPtr->control |= SI_CSR_DMA_EN;
    }
    status = SUCCESS;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * StartDMA --
 *
 *	Issue the sequence of commands to the controller to start DMA.
 *	This can be called by Dev_SCSI3Intr in response to a DATA_{IN,OUT}
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
    register Controller *ctrlPtr;
{
    register volatile CtrlRegs *regsPtr;
    unsigned char junk;
    int	size;

    size = ctrlPtr->scsiCmdPtr->bufferLen;

    if (devSCSI3Debug > 4) {
	printf("%s: StartDMA %s called size = %d.\n", ctrlPtr->name,
	    (ctrlPtr->dmaState == DMA_RECEIVE) ? "receive" :
		((ctrlPtr->dmaState == DMA_SEND) ? "send" :
						  "not-active!"), size);
    }
    regsPtr = ctrlPtr->regsPtr;
    if (ctrlPtr->onBoard) {
	/*
	 * The DMA control block has already been set up.  We just say "go".
	 */
	MACH_DELAY(SI_UDC_WAIT);
	regsPtr->udcRdata = UDC_CMD_STRT_CHN;
    } else {
        SET_DMA_COUNT(regsPtr,size);
	ctrlPtr->dmaSetup = TRUE;
    }

    if (ctrlPtr->dmaState == DMA_RECEIVE) {
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
    if (!ctrlPtr->onBoard) {
	regsPtr->control |= SI_CSR_DMA_EN;
    }
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
    register volatile CtrlRegs *regsPtr;
    ReturnStatus status;
    char message;

    if (devSCSI3Debug > 4) {
	printf("GetStatusByte called ");
    }
    regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    *statusBytePtr = 0;

    /*
     * After the DATA_IN/OUT phase we enter the STATUS phase for
     * 1 byte (usually) of status.  This is followed by the MESSAGE phase
     */
    status = WaitPhase(ctrlPtr, PHASE_STATUS, RESET);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 3) {
	    printf("Warning: %s wait on PHASE_STATUS failed.\n",ctrlPtr->name);
	}
	return(status);
    }
    /*
     * Get one status byte.
     */
    status = GetByte(ctrlPtr, PHASE_STATUS, (char *) statusBytePtr);
    if (status != SUCCESS) {
	printf("Warning: %s error 0x%x getting status byte\n", 
		ctrlPtr->name, status);
	return (status);
    }
#ifdef notdef
    /* 
     * From the way the code was originally written it looks like some
     * devices return more that one byte of status info. Since we don't
     * want these bytes drop them on the floor.
     */
    for (; ; ) {
	    status = GetByte(ctrlPtr, PHASE_STATUS, (char *) statusBytePtr);
	    if (devSCSI3Debug > 4 && (numStatusBytes == 0)) {
	        printf("SCSI3-%d: got error %x after %d status bytes\n",
				 ctrlPtr->number, status, numStatusBytes);
	    }
	    break;
	}
	    *statusBytePtr = statusByte;
	    statusBytePtr++;
	}
	numStatusBytes++;
    }
#endif
    if (devSCSI3Debug > 4) {
	printf("got 0x%x\n", *statusBytePtr);
    }
    /*
     * Wait for the message in phase and grap the COMMAND COMPLETE message
     * off the bus.
     */
    status = WaitPhase(ctrlPtr, PHASE_MSG_IN, RESET);
    if (status != SUCCESS) {
        printf("Warning: %s wait on PHASE_MSG_IN after status failed.\n",
		ctrlPtr->name);
	return(status);
    } 
    status = GetByte(ctrlPtr, PHASE_MSG_IN, &message);
    if (status != SUCCESS) {
	printf("Warning: %s got error 0x%x getting message and status.\n",
			     ctrlPtr->name, status);
	return(status);
    }
    if (message != SCSI_COMMAND_COMPLETE) {
	printf("Warning: %s message %d after status is not command complete.\n",		ctrlPtr->name, message);
	return(FAILURE);
    }
    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
    if (devSCSI3Debug > 4) {
	printf("Got message 0x%x\n", message);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * WaitReg --
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
static ReturnStatus
WaitReg(ctrlPtr, thisRegPtr, type, conditions, reset, bitSel)
    Controller *ctrlPtr;	/* Controller state */
    Address thisRegPtr;		/* pointer to register to check */
    RegType type;		/* "type" of the register */
    unsigned int conditions;	/* one or more bits to check */
    Boolean reset;		/* whether to reset the bus on error */
    BitSelection bitSel;	/* check for all or some bits going to 1/0 */
{
    volatile register CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register unsigned int thisReg;

    for (i=0 ; i<WAIT_LENGTH ; i++) {
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
	    
	if (devSCSI3Debug > 10 && i < 5) {
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
	    if (devSCSI3Debug > 5) {
	        panic("SCSI3WaitReg: bus error\n");
	    } else {
		printf("SCSI3WaitRes: bus error\n");
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
    if (devSCSI3Debug > 2) {
	printf("WaitReg: timed out.\n");
	PrintRegs(regsPtr);
	printf("WaitReg: was checking %x for condition(s) %x to go ",
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
    if (reset) {
	Reset(ctrlPtr);
    }
    return(status);
}

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
    volatile register CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register unsigned char thisReg;

    for (i=0 ; i<WAIT_LENGTH ; i++) {
	thisReg = regsPtr->sbc.read.curStatus;
	if (devSCSI3Debug > 10 && i < 5) {
	    printf("%d/%x ", i, thisReg);
	}
	if ((thisReg & CBSR_PHASE_BITS) == phase) {
	    return(SUCCESS);
	}
	if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	    if (devSCSI3Debug > 5) {
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
    if (devSCSI3Debug > 5) {
	printf("WaitPhase: timed out.\n");
	PrintRegs(regsPtr);
	printf("WaitPhase: was checking for phase %x.\n",
		   (int) phase);
    }
    if (reset) {
	Reset(ctrlPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * PutByte --
 *
 *	Put a byte onto the SCSI bus.  This always goes into the MSG_OUT
 *	phase.  This handles the standard REQ/ACK handshake to put
 *	the bytes on the SCSI bus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Yanks control lines in order to put a byte on the bus.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
PutByte(ctrlPtr, dataPtr)
    Controller *ctrlPtr;
    char *dataPtr;    
{
    volatile register CtrlRegs *regsPtr = (volatile CtrlRegs *) ctrlPtr->regsPtr;
    volatile unsigned char *initCmdPtr = &regsPtr->sbc.write.initCmd;
    register ReturnStatus status;
    unsigned char junk;

    /*
     * Enter MESSAGE OUT phase and wait for REQ to be set by the target.
     */
    regsPtr->sbc.write.trgtCmd = TCR_MSG_OUT;
    *initCmdPtr = 0;
    status = WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    printf("PutByte couldn't wait for REQ.\n");
	}
	return(status);
    }
    /*
     * Put the data on and then ACK the target's REQ.
     */
    regsPtr->sbc.write.data = *dataPtr;
    regsPtr->sbc.write.initCmd = SBC_ICR_DATA;
    regsPtr->sbc.write.initCmd |= SBC_ICR_ACK;
    status =  WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
    if (status != SUCCESS) {
	if (devSCSI3Debug > 1) {
	    printf("Warning: %s wait on REQ line to go low failed.\n",
		   ctrlPtr->name);
	}
	return(status);
    }	

    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
    junk = regsPtr->sbc.read.clear;
#ifdef lint
    regsPtr->sbc.read.clear = junk;
#endif
    regsPtr->sbc.write.initCmd = 0;
    return(SUCCESS);
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
GetByte(ctrlPtr, phase, charPtr)
    Controller *ctrlPtr;
    unsigned short phase;
    char *charPtr;    
{
    register volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    ReturnStatus status;
    
    status = WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_HIGH);
    if (status != SUCCESS) {
	return(status);
    }
    if ((regsPtr->sbc.read.curStatus & CBSR_PHASE_BITS) != phase) {
	if (devSCSI3Debug > 5) {
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
    status = WaitReg(ctrlPtr, (Address) &regsPtr->sbc.read.curStatus,
			    REG_BYTE, SBC_CBSR_REQ, RESET, ACTIVE_LOW);
    if (status != SUCCESS) {
	panic("SCSI3: GetByte: request line didn't go low.\n");
	return(status);
    }
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
    register volatile CtrlRegs *regsPtr;
{
    printf("ctl %x addr %x%x dmaCount %x%x fifoCount %x%x data %x\n\tinitCmd %x mode %x target %x curStatus %x status %x\n", 
	       regsPtr->control,
	       regsPtr->dmaAddressHigh,
	       regsPtr->dmaAddressLow,
	       regsPtr->dmaCountHigh,
	       regsPtr->dmaCountLow,
	       regsPtr->fifoCountHigh,
	       regsPtr->fifoCountLow,
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


    if (devSCSI3Debug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
    /*
     * Unallocated any DMA allocated for this request. 
     */
    if (ctrlPtr->dmaBuffer != (Address) NIL) {
	VmMach_DMAFree(scsiCmdPtr->bufferLen, ctrlPtr->dmaBuffer);
	ctrlPtr->dmaBuffer = (Address) NIL;
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
 * entryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for this
 *	controller. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for this controller. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *	NOTE: This routine is also called from DevSCSI3Intr to start the
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
    status = SendCommand(devPtr, scsiCmdPtr);
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
 * DevSCSI3Intr --
 *
 * Handle interrupts from the SCSI-3 controller.
 * The follow cases cause interrupts on the 5380: => SBC_IP
 *	1. Reselection attempt by a target (not implemented yet)
 *	2. EOP during DMA.  This indicates DMA has completed.
 *	3. SCSI bus reset.  (Only applies to targets, not us.)
 *	4. Parity error during data transfer.  (Parity checking is disabled.)
 *	5. Bus phase mismatch.  trgtCmd must be correct for data transfers.
 *	6. SCSI bus disconnect by a target (not implmented yet)
 * In addition the SCSI-3 Host Adaptor will generate interrupts when:
 *	7. Registers other than control are touched during DMA => DMA_CONFLICT
 *	8. An error occurs during DMA. => DMA_IP
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
DevSCSI3Intr(clientDataArg)
    ClientData	clientDataArg;
{
    Controller *ctrlPtr;
    register volatile CtrlRegs *regsPtr;
    Device	*devPtr;
    unsigned char statusByte;
    ReturnStatus status;
    int byteCount;
    register int i;
    unsigned char phase;
    unsigned char foo;
    int	 residual;
    List_Links	*newRequestPtr;
    ClientData	clientData;

    if (devSCSI3Debug > 4) {
	printf("DevSCSI3Intr: ");
    }
    ctrlPtr = (Controller *) clientDataArg;
    regsPtr = ctrlPtr->regsPtr;
    devPtr = ctrlPtr->devPtr;
    if ((regsPtr->control & (SI_CSR_SBC_IP |	/* 5380 interrupt */
			     SI_CSR_DMA_IP |		/* or DMA error */
			     SI_CSR_DMA_CONFLICT))	/* or register goof */
			     == 0) {
	if (devSCSI3Debug > 4 ) {
	    printf("spurious\n");
	}
	return FALSE;
    }
    MASTER_LOCK(&(ctrlPtr->mutex));
    if (ctrlPtr->scsiCmdPtr != (ScsiCmd *) NIL) {
	residual = ctrlPtr->scsiCmdPtr->bufferLen;
    }
   /*
     * First, disable DMA or else we'll get register conflicts.
     */
    if (!ctrlPtr->onBoard) {
	regsPtr->control &= ~SI_CSR_DMA_EN;
	byteCount = ctrlPtr->dmaSetup ? READ_FIFO_COUNT(regsPtr) : residual;
    } else { 
	byteCount = READ_FIFO_COUNT(regsPtr);
    }
    regsPtr->sbc.write.trgtCmd = TCR_UNSPECIFIED;
    /*
     * The follow cases cause interrupts on the 5380:
     *	1. Selection or Reselection (only reselection applies to the CPU)
     *	2. EOP during DMA.  This indicates DMA has completed.
     *	3. SCSI bus reset.  This probably shouldn't happen; we do the resetting
     *	4. Parity error during data transfer.  Parity checking is disabled.
     *	5. Bus phase mismatch.  trgtCmd must be correct for data transfers.
     *	6. SCSI bus disconnect.  A target is disconnecting.
     * In addition the Host Adaptor will generate interrupts when:
     *	7. Registers other than control are touched during DMA => DMA_CONFLICT
     *	8. An error occurs during DMA. => DMA_IP
     */
    if (regsPtr->control & (SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) {
	/*
	 * DMA Error.  DMA_IP means a bus error or
	 *			"send & fifo-empty & dmaCount == 0"
	 *	DMA_CONFLICT means we touched a non-control reg during DMA.
	 */
	if (regsPtr->control & SI_CSR_DMA_BUS_ERR) {
	    /*
	     * A Bus Error.  Complete the I/O but flag an error.
	     * The residual is computed because the Bus Error could
	     * have occurred after a number of sectors.
	     */
	    residual = byteCount;
	    printf("Warning: %s DMA bus error\n",ctrlPtr->name);
	} else if (regsPtr->control & SI_CSR_DMA_CONFLICT) {
	    printf("Warning: %s DMA register conflict goof\n",ctrlPtr->name);
	} else {
	    printf("Warning: %s DMA programming error\n",ctrlPtr->name);
	}
	status = DEV_DMA_FAULT;
	goto rtnHardErrorAndGetNext;	/* Return the hard error message
					 * to the call and get the next 
					 * entry in the devQueue. */
    }
    /*
     * 5380 generated interrupt.
     * Interrupt processing is described on pages 86-89.
     *	Parity is turned off, so SBC_BSR_PERR can be ignored.
     *	Busy monitoring mode is not set, so SBC_BSR_BERR can be ignored.
     */
    if (regsPtr->sbc.read.curStatus & SBC_CBSR_SEL) {
	/*
	 * Reselection attempt by a target.  Unimplementned.
	 */
	printf("Warning: %s reselection attempt!\n",ctrlPtr->name);
	foo = regsPtr->sbc.read.clear;
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return(TRUE);
    }
    /*
     * SBC_BSR_EDMA may be set to indicate that DMA has completed,
     * or the SBC_BSR_PMTCH bit is 0 (this has been verified).
     * We fall through and test the REQ line to see if the target
     * is trying to send additional data bytes, or we are just
     * getting a standard phase change interrupt.
     */
    for (i=0 ; i<30 ; i++) {
	if (regsPtr->sbc.read.curStatus & SBC_CBSR_REQ) {
	    break;
	}
	MACH_DELAY(10);
    }
    foo = regsPtr->sbc.read.clear;
#ifdef lint		
    regsPtr->sbc.read.clear = foo;
#endif
    if (i == 30) {
	/*
	 * Apparently spurious interrupt, cause as yet unknown.
	 */
	if (devSCSI3Debug > 4) {
	    printf("REQ not set: CBSR %x BSR %x\n",
		regsPtr->sbc.read.curStatus, regsPtr->sbc.read.status);
	}
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return(TRUE);
    }
    phase = regsPtr->sbc.read.curStatus & CBSR_PHASE_BITS;
    switch (phase) {
	case PHASE_DATA_IN:
	case PHASE_DATA_OUT: {
	    if (devSCSI3Debug > 4) {
		printf("Data Phase Interrupt\n");
	    }
	    regsPtr->sbc.write.mode &= ~SBC_MR_DMA;
	    StartDMA(ctrlPtr);
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	}
	case PHASE_MSG_IN: {
	    char	message;
	    status = GetByte(ctrlPtr, PHASE_MSG_IN, (char *)&message);
	    if (devSCSI3Debug > 4) {
		printf("Msg Phase Interrupt\n");
	    }
	    if (status != SUCCESS) {
		printf("Warning: %s couldn't get message.\n",ctrlPtr->name);
		if (!ctrlPtr->onBoard) {
		    regsPtr->control |= SI_CSR_DMA_EN;
		}
		MASTER_UNLOCK(&(ctrlPtr->mutex));
		return(TRUE);
	    }
	    if (message != SCSI_COMMAND_COMPLETE) {
		printf("Warning: %s couldn't handle message 0x%x from %s.\n",
			ctrlPtr->name, message, 
			ctrlPtr->devPtr->handle.locationName);
		if (!ctrlPtr->onBoard) {
		    regsPtr->control |= SI_CSR_DMA_EN;
		}
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	}
	case PHASE_STATUS: {
	    if (ctrlPtr->onBoard) {
		residual = byteCount;
	    } else {
		residual = ctrlPtr->dmaSetup ? READ_DMA_COUNT(regsPtr) :
						byteCount;
	    }
	    if (devSCSI3Debug > 4) {
		printf("Status Phase Interrupt, residual = %d\n",residual);
	    }
	    if (ctrlPtr->dmaState == DMA_RECEIVE) {
		if (!ctrlPtr->onBoard) { 
		    if ((regsPtr->control & SI_CSR_LOB) != 0) {
		    /*
		     * On a read the last odd byte is left in the byte pack
		     * register. We use wordmode (not 32-bit longwords)
		     * so there will only be one byte left.  I assume it
		     * is Byte 0 in the byte pack reg, with is the first
		     * byte in the high-half.
		     */ 
		    *(char *) (READ_DMA_ADDR(regsPtr) + VMMACH_DMA_START_ADDR) = 
			    (regsPtr->bytePackHigh & 0xff00) >> 8;
		    }
		} else {
		    regsPtr->udcRaddr = UDC_ADR_COUNT;
		    /*
		     * wait for the fifo to empty
		     */
		   status = WaitReg(ctrlPtr, (Address)&regsPtr->control,
			    REG_SHORT, SI_CSR_FIFO_EMPTY, RESET, ACTIVE_HIGH);
		   if (status != SUCCESS) {
			printf("Warning: %s fifo wait failed\n",ctrlPtr->name);
		   }
#ifdef notsure		    
		    if ((READ_FIFO_COUNT(regsPtr) == size)  ||
			(READ_FIFO_COUNT(regsPtr) + 1 == size) {
			    goto out;
			/*
			 * Didn't transfer any data.
			 * The fifoCount + 1 above wards against 5380 prefetch.
			 */
			    goto out;
		     }
		    /*
		     * Transfer left over byte or shortword by hand
		     */
		    if ((size - READ_FIFO_COUNT(regsPtr)) & 1) {
		    *(char *)(READ_DMA_ADDR(regsPtr) - 1 + VMMACH_DMA_START_ADDR) =
			    (regsPtr->fifoData & 0xff00) >> 8;
		    } else if (((regsPtr->udcRdata*2) - READ_FIFO_COUNT(regsPtr)) == 2) {
		    *(char *)(READ_DMA_ADDR(regsPtr) - 2 + VMMACH_DMA_START_ADDR) =
			    (regsPtr->fifoData & 0xff00) >> 8;
		    *(char *)(READ_DMA_ADDR(regsPtr->dmaAddress) + VMMACH_DMA_START_ADDR) =
			    (regsPtr->fifoData & 0xff);
		    }
#endif
		}
	    }

	    status =  GetStatusByte(ctrlPtr, &statusByte);
	    if (status != SUCCESS) {
					/* Return the hard error message
					 * to the call and get the next 
					 * entry in the devQueue. */
		goto rtnHardErrorAndGetNext;
	    }
	    RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, statusByte,
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
	default: {
	    printf("Warning: %s couldn't handle phase %x... ignoring.\n",
		       ctrlPtr->name, phase);
	    if (devSCSI3Debug > 0) {
		PrintRegs(regsPtr);
	    }
	    if (!ctrlPtr->onBoard) {
		regsPtr->control |= SI_CSR_DMA_EN;
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	}
    }
    /*
     * Jump here to return an error and reset the HBA.
     */
rtnHardErrorAndGetNext:
    if (ctrlPtr->scsiCmdPtr != (ScsiCmd *) NIL) { 
	printf("Warning: %s reset and current command terminated.\n",
	       devPtr->handle.locationName);
	RequestDone(devPtr,ctrlPtr->scsiCmdPtr,status,0,residual);
    }
    Reset(ctrlPtr);
    /*
     * Use the queue entryAvailProc to start the next request for this device.
     */
    newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
    if (newRequestPtr != (List_Links *) NIL) { 
	(void) entryAvailProc(clientData,newRequestPtr);
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
 * DevSCSI3Init --
 *
 *	Check for the existant of the Sun SCSI3 HBA controller. If it
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
DevSCSI3Init(ctrlLocPtr)
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
    found =  (ctrlLocPtr->space == DEV_OBIO) ? 
		  ProbeOnBoard(ctrlLocPtr->address)  :
		  ProbeVME(ctrlLocPtr->address);
    if (!found) {
	return DEV_NO_CONTROLLER;
    }
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    if (ctrlNum+1 > numSCSI3Controllers) {
	numSCSI3Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (ctrlLocPtr->address);
    if (ctrlLocPtr->space == DEV_OBIO) {
	ctrlPtr->onBoard = TRUE;
	ctrlPtr->udcDmaTable = (UDCDMAtable *) 
	    VmMach_DMAAlloc(sizeof(UDCDMAtable), malloc(sizeof(UDCDMAtable)));
    } else {
	ctrlPtr->onBoard = FALSE;
	/*
	 * Set the address modifier in the interrupt vector.
	 */
	ctrlPtr->regsPtr->addrIntr = INTR_ADDR(ctrlLocPtr->vectorNumber);
    }
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
 * DevSCSI3AttachDevice --
 *
 *	Attach a SCSI device using the Sun SCSI3 HBA. 
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
DevSCSI3AttachDevice(devicePtr, insertProc)
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
     * First find the SCSI3 controller this device is on.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr);
    if ((ctrlNum > MAX_SCSI3_CTRLS) ||
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
    devPtr->handle.maxTransferSize = (ctrlPtr->onBoard) ? 
					MAX_ONBOARD_TRANSFER_SIZE :
					MAX_VME_TRANSFER_SIZE;

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
    (void) sprintf(tmpBuffer, "%s#%d Target %d LUN %d", ctrlPtr->name, ctrlNum,
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    return (ScsiDevice *) devPtr;
}


