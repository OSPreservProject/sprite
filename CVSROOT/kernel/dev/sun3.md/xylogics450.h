/*
 * devXylogics.h --
 *
 *	Declarations for the Xylogics 450 controller.  The technical
 *	manual to refer to is "XYLOGICS 450 Disk Controller User's Manual".
 *	(Date Aug 1983, Rev. B)
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVXYLOGICS
#define _DEVXYLOGICS

#include "sync.h"

Boolean Dev_XylogicsInitController();
Boolean Dev_XylogicsInitDevice();
Boolean	Dev_XylogicsIntr();		/* C routine */
int	Dev_XylogicsIntrStub();		/* Autovector stub to call C routine */
void	Dev_XylogicsIdleCheck();

extern int xylogicsPrints;		/* Set to TRUE for debug prints */

/*
 * RANDOM NOTES:
 *
 * Interrupt vector number 72, hex 0x48
 *
 * The following two notes are tackled in Dev_Config()
 *	The controller lives at 0xee40 in VME A16D16 space
 *	which is really VME address 0xFFFFee40.  A page of kernel
 *	virtual space needs to be mapped to that address.
 *
 *	The DVMA space on a Sun 3 is in the high Megabyte of kernel virtual,
 *		x0FF00000 to 0x0FFFFFFF
 *	Buffers for DVMA must be allocated there,
 *	but one must only pass low order bits in control blocks, ie.
 *	mask with 0xFFFFF
 *
 * The Xylogics board does crazed address relocation, either to 20-bit
 * or 24-bit addresses.  I expect only 24-bit mode with the sun3's,
 * which means the top half of the DVMA addresses has to be cropped
 * off and put in the relocation register part of the IOPB
 *
 * The IOPB format must be byte swapped with respect to the documentation
 * because of disagreement between the multibus/8086 ordering and the
 * sun3/Motorola ordering.
 */

/*
 * The I/O Registers of the 450 are used to initiate commands and to
 * specify where parameter blocks (IOPB) are.  The bytes are swapped
 * because the controller thinks it's on a multibus.
 */
typedef struct DevXylogicsRegs {
    char relocHigh;	/* Byte 1 - IOPB Relocation Register High Byte */
    char relocLow;	/* Byte 0 - IOPB Relocation Register Low Byte */
    char addrHigh;	/* Byte 3 - IOPB Address Register High Byte */
    char addrLow;	/* Byte 2 - IOPB Address Register Low Byte */
    char resetUpdate;	/* Byte 5 - Controller Reset/Update Register */
    char status;	/* Byte 4 - Controller Status Register (CSR) */
} DevXylogicsRegs;

/*
 * Definitions for the bits in the status register
 *	XY_GO_BUSY	- set by driver to start command, remains set
 *			  until the command completes.
 *	XY_ERROR	- set by the controller upon error
 *			  Do error reset by setting this bit to 1
 *	XY_DBL_ERROR	- set by controller upon error DMA'ing status bytes.
 *	XY_INTR_PENDING	- set by controller, must be unset by handler
 *			  by writing a 1 to it.
 *	XY_20_OR_24	- If zero, the controller does 20 bit relocation,
 *			  otherwise it uses the relocation bytes as the
 *			  top 16 bits of the DMA address.
 *	XY_ATTN_REQ	- Set when you want to add more IOPB's to the chain,
 *			  clear this when work on the chain is complete.
 *	XY_ATTN_ACK	- Set by the controller when it's safe to add/delete
 *			  IOPB's to the command chain.
 *	XY_READY	- "Indicates the Ready-On Cylinder status of the
 *			  last drive selected.
 */
#define	XY_GO_BUSY	0x80
#define XY_ERROR	0x40
#define XY_DBL_ERROR	0x20
#define XY_INTR_PENDING	0x10
#define	XY_20_OR_24	0x08
#define XY_ATTN_REQ	0x04
#define XY_ATTN_ACK	0x02
#define XY_READY	0x01

typedef struct DevXylogicsIOPB {
    /*
     * Byte 1 - Interrupt Mode
     */
    unsigned char		:1;
    unsigned char intrIOPB	:1;	/* Interrupt upon completion of IOPB */
    unsigned char intrError	:1;	/* Interrupt upon error (440 only) */
    unsigned char holdDualPort	:1;	/* Don't release dual port drive */
    unsigned char autoSeekRetry	:1;	/* Enables re-calibration on error */
    unsigned char enableExtras	:1;	/* Enables commands 3 and 4 */
    unsigned char eccMode	:2;	/* ECC Correction Mode, set to 2 */
    /*
     * Byte 0 - Command Byte
     */
    unsigned char autoUpdate	:1;	/* Update IOPB after completion */
    unsigned char relocation	:1;	/* Enables relocation on data addrs */
    unsigned char doChaining	:1;	/* Enables chaining of IOPBs */
    unsigned char interrupt	:1;	/* When set interupts upon completion */
    unsigned char command	:4;	/* Commands defined below */
    /*
     * Byte 3 - Status Byte 2
     */
    unsigned char errorCode;	/* Error codes, 0 means success, the rest
				 * of the codes are explained in the manual */
    /*
     * Byte 2 - Status Byte 1
     */
    unsigned char error		:1;	/* Indicates an error occurred */
    unsigned char 		:2;
    unsigned char cntrlType	:3;	/* Controller type, 1 = 450 */
    unsigned char		:1;
    unsigned char done		:1;	/* Command is complete */
    /*
     * Byte 5 - Drive Type, Unit Select
     */
    unsigned char driveType	:2;	/* 2 => Fujitsu 2351 (Eagle) */
    unsigned char		:4;
    unsigned char unit		:2;	/* Up to 4 drives per controller */
    /*
     * Byte 4 - Throttle
     */
    unsigned char transferMode	:1;	/* == 0 for words, 1 for bytes */
    unsigned char interleave	:4;	/* == 0 for 1:1 interleave */
    unsigned char throttle	:3;	/* 4 => 32 words per DMA burst */

    unsigned char sector;	/* Byte 7 - Sector Byte */
    unsigned char head;		/* Byte 6 - Head Byte */
    unsigned char cylHigh;	/* Byte 9 - High byte of cylinder address */
    unsigned char cylLow;	/* Byte 8 - Low byte of cylinder address */
    unsigned char numSectHigh;	/* Byte B - High byte of sector count */
    unsigned char numSectLow;	/* Byte A - Low byte of sector count.
				 * This byte is also used to return status
				 * with the Read Drive Status command */
    /*
     * Don't byteswap the data address and relocation offset.  All the
     * byte-swapped device is going to do is turn around and put these
     * addresses back onto the bus, so don't have to worry about ordering.
     * (The relocation register is scary, but the Sun MMU puts all the
     * DMA buffer space into low physical memory addresses, so the relocation
     * register is probably zero anyway.)
     */
    unsigned char dataAddrHigh;	/* Byte D - High byte of data address */
    unsigned char dataAddrLow;	/* Byte C - Low byte of data address */
    unsigned char relocHigh;	/* Byte F - High byte of relocation value */
    unsigned char relocLow;	/* Byte E - Low byte of relocation value */
    /*
     * Back to byte-swapping
     */
    unsigned char reserved1;	/* Byte 11 */
    unsigned char headOffset;	/* Byte 10 */
    unsigned char nextHigh;	/* Byte 13 - High byte of next IOPB address */
    unsigned char nextLow;	/* Byte 12 - Low byte of next IOPB address */
    unsigned char eccByte15;	/* Byte 15 - ECC Pattern byte 15 */
    unsigned char eccByte14;	/* Byte 14 - ECC Pattern byte 14 */
    unsigned char eccAddrHigh;	/* Byte 17 - High byte of sector bit address */
    unsigned char eccAddrLow;	/* Byte 16 - Low byte of sector bit address */
} DevXylogicsIOPB;

/*
 * Defines for the command field of Byte 0. These are explained in
 * pages 25 to 58 of the manual.  The code here uses READ and WRITE, of course,
 * and also XY_RAW_READ to learn the proper drive type, and XY_READ_STATUS
 * to see if a drive exists.
 */
#define XY_NO_OP		0x0
#define XY_WRITE		0x1
#define XY_READ			0x2
#define XY_WRITE_HEADER		0x3
#define XY_READ_HEADER		0x4
#define XY_SEEK			0x5
#define XY_DRIVE_RESET		0x6
#define XY_WRITE_FORMAT		0x7
#define XY_RAW_READ		0x8
#define XY_READ_STATUS		0x9
#define XY_RAW_WRITE		0xA
#define XY_SET_DRIVE_SIZE	0xB
#define XY_SELF_TEST		0xC
/*      reserved  		0xD */
#define XY_BUFFER_LOAD		0xE
#define XY_BUFFER_DUMP		0xF

/*
 * Defines for error code values.  They are explained fully in the manual.
 */
#define XY_NO_ERROR		0x00
/*
 * Programming errors
 */
#define XY_ERR_INTR_PENDING	0x01
#define XY_ERR_BUSY_CONFLICT	0x03
#define XY_ERR_BAD_CYLINDER	0x07
#define XY_ERR_BAD_SECTOR	0x0A
#define XY_ERR_BAD_COMMAND	0x15
#define XY_ERR_ZERO_COUNT	0x17
#define XY_ERR_BAD_SECTOR_SIZE	0x19
#define XY_ERR_SELF_TEST_A	0x1A
#define XY_ERR_SELF_TEST_B	0x1B
#define XY_ERR_SELF_TEST_C	0x1C
#define XY_ERR_BAD_HEAD		0x20
#define XY_ERR_SLIP_SECTOR	0x09
#define XY_ERR_SLAVE_ACK	0x0E
/*
 * Soft errors that may be recovered by retrying.  Retry at most twice.
 */
#define XY_SOFT_ERR_TIME_OUT	0x04
#define XY_SOFT_ERR_BAD_HEADER	0x05
#define XY_SOFT_ERR_ECC		0x06
#define XY_SOFT_ERR_NOT_READY	0x16
/*
 * These errors cause a drive re-calibration, then you retry the transfer.
 */
#define XY_SOFT_ERR_HEADER	0x12
#define XY_SOFT_ERR_FAULT	0x18
#define XY_SOFT_ERR_SEEK	0x25
/*
 * Errors during formatting.
 */
#define XY_FORMAT_ERR_RUNT	0x0D
#define XY_FORMAT_ERR_BAD_SIZE	0x13
/*
 * Noteworthy errors.
 */
#define XY_WRITE_PROTECT_ON	0x14
#define XY_SOFT_ECC_RECOVERED	0x1F
#define XY_SOFT_ECC		0x1E

/*
 * Bit values for the numSectLow byte used to return Read Drive Status
 *	XY_ON_CYLINDER		== 0 if drive is not seeking
 *	XY_DISK_READY		== 0 if drive is ready
 *	XY_WRITE_PROTECT	== 1 if write protect is on
 *	XY_DUAL_PORT_BUSY	== 1 if dual ported drive is busy
 *	XY_HARD_SEEK_ERROR	== 1 if the drive reports a seek error
 *	XY_DISK_FAULT		== 1 if the drive reports any kind of fault
 */
#define XY_ON_CYLINDER		0x80
#define XY_DISK_READY		0x40
#define XY_WRITE_PROTECT	0x20
#define XY_DUAL_PORT_BUSY	0x10
#define XY_HARD_SEEK_ERROR	0x80
#define XY_DISK_FAULT		0x40

typedef struct DevXylogicsController {
    int			magic;		/* To catch bad pointers */
    int			flags;		/* Defined below */
    DevXylogicsRegs	*regsPtr;	/* Pointer to Controller's registers */
    int			number;		/* Controller number, 0, 1 ... */
    int			slaveID;	/* Current drive number */
    DevXylogicsIOPB	*IOPBPtr;	/* Ref to IOPB */
    int			residual;	/* Bytes left over after a transfer */
    Address		labelBuffer;	/* For copy of sector zero */
    Address		IOBuffer;	/* The buffer for reads/writes */
    int			mutex;		/* Mutex for queue access */
    Sync_Condition	IOComplete;	/* Synchronization stuff... */
    Sync_Condition	readyForIO;
    /*
     * Pointer back to controller config struct which contains the stats
     * for this controller.
     */
    struct DevConfigController *configPtr;
} DevXylogicsController;

#define XY_CNTRLR_STATE_MAGIC	0xf5e4d3c2

/*
 * Definitions for the flags in DevXylogicsController
 */
#define XYLOGICS_CNTRLR_ALIVE	0x1
#define XYLOGICS_CNTRLR_BUSY	0x2
#define XYLOGICS_IO_COMPLETE	0x4
#define XYLOGICS_IO_ERROR	0x8
#define XYLOGICS_RETRY		0x10
#define XYLOGICS_WANT_INTERRUPT	0x20

#define XYLOGICS_MAX_CONTROLLERS	2
#define XYLOGICS_MAX_DISKS		4

typedef struct DevXylogicsDisk {
    int				magic;		/* Check against bad pointers */
    DevXylogicsController	*xyPtr;	/* Back pointer to controller state */
    int				xyDriveType;	/* Xylogics code for disk */
    int				slaveID;	/* Drive number */
    int				numCylinders;	/* ... on the disk */
    int				numHeads;	/* ... per cylinder */
    int				numSectors;	/* ... on each track */
    DevDiskMap			map[DEV_NUM_DISK_PARTS];/* partitions */
} DevXylogicsDisk;

#define XY_DISK_STATE_MAGIC	0xa1b2c3d4

extern DevXylogicsDisk *xyDisk[];
#endif _DEVXYLOGICS
