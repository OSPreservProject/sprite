/* 
 * devXylogics450.c --
 *
 *	Driver the for Xylogics 450 SMD controller.
 *	The technical
 *	manual to refer to is "XYLOGICS 450 Disk Controller User's Manual".
 *	(Date Aug 1983, Rev. B)
 *
 * Copyright 1989 Regents of the University of California
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
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "devDiskLabel.h"
#include "dbg.h"
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "fs.h"
#include "vmMach.h"
#include "devQueue.h"
#include "devBlockDevice.h"
#include "xylogics450.h"
#include "devDiskStats.h"
#include "stdlib.h"

/*
 * RANDOM NOTES:
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
typedef struct XylogicsRegs {
    char relocHigh;	/* Byte 1 - IOPB Relocation Register High Byte */
    char relocLow;	/* Byte 0 - IOPB Relocation Register Low Byte */
    char addrHigh;	/* Byte 3 - IOPB Address Register High Byte */
    char addrLow;	/* Byte 2 - IOPB Address Register Low Byte */
    char resetUpdate;	/* Byte 5 - Controller Reset/Update Register */
    char status;	/* Byte 4 - Controller Status Register (CSR) */
} XylogicsRegs;

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

typedef struct XylogicsIOPB {
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
} XylogicsIOPB;

#define XYLOGICS_MAX_CONTROLLERS	2
#define XYLOGICS_MAX_DISKS		4

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

typedef struct XylogicsDisk XylogicsDisk;
typedef struct Request	Request;

typedef struct XylogicsController {
    int			magic;		/* To catch bad pointers */
    Boolean		busy;		/* TRUE if the controller is busy. */
    volatile XylogicsRegs *regsPtr;	/* Pointer to Controller's registers */
    int			number;		/* Controller number, 0, 1 ... */
    Request		*requestPtr;	/* Current active request. */
    Address		dmaBuffer;	/* Address of the DMA buffer
					 * for reads/writes */
    int			dmaBufferSize;	/* Size of the dmaBuffer mapped. */
    volatile XylogicsIOPB *IOPBPtr;	/* Ref to IOPB */
    Sync_Semaphore	mutex;		/* Mutex for queue access */
    Sync_Condition	specialCmdWait; /* Condition to wait of for special
					 * commands liked test unit ready and
					 * reading labels. */
    int			numSpecialWaiting; /* Number of processes waiting to 
					    * get access to the controller for
					    * a sync command. */
    DevCtrlQueues	ctrlQueues;	/* Queues of disk attached to the 
					 * controller. */
    XylogicsDisk	*disks[XYLOGICS_MAX_DISKS]; /* Disk attached to the
						     * controller. */
} XylogicsController;

#define XY_CNTRLR_STATE_MAGIC	0xf5e4d3c2


struct XylogicsDisk {
    int				magic;		/* Check against bad pointers */
    XylogicsController	*xyPtr;	/* Back pointer to controller state */
    int				xyDriveType;	/* Xylogics code for disk */
    int				slaveID;	/* Drive number */
    int				numCylinders;	/* ... on the disk */
    int				numHeads;	/* ... per cylinder */
    int				numSectors;	/* ... on each track */
    DevQueue			queue;
    DevDiskMap			map[DEV_NUM_DISK_PARTS];/* partitions */
    Sys_DiskStats		*diskStatsPtr;
};

#define XY_DISK_STATE_MAGIC	0xa1b2c3d4

/*
 * The interface to XylogicsDisk the outside world views the disk as a 
 * partitioned disk.  
 */
typedef struct PartitionDisk {
    DevBlockDeviceHandle handle; /* Must be FIRST field. */
    int	partition;		 /* Partition number on disk. */
    XylogicsDisk	*diskPtr; /* Real disk. */
} PartitionDisk;

/*
 * Format of request queued for a Xylogics disk. This request is 
 * built in the ctrlData area of the DevBlockDeviceRequest.
 */

struct Request {
    List_Links	queueLinks;	/* For the dev queue modole. */
    int		command;	/* XY_READ or XY_WRITE. */
    XylogicsDisk *diskPtr;	/* Target disk for request. */
    Dev_DiskAddr diskAddress;	/* Starting address of request. */
    int		numSectors;	/* Number of sectors to transfer. */
    Address	buffer;		/* Memory to transfer to/from. */
    int		retries;	/* Number of retries on the command. */
    DevBlockDeviceRequest *requestPtr; /* Block device generating this 
					* request. */
};

/*
 * State for each Xylogics controller.
 */
static XylogicsController *xylogics[XYLOGICS_MAX_CONTROLLERS];

/*
 * This controlls the time spent busy waiting for the transfer completion
 * when not in interrupt mode.
 */
#define XYLOGICS_WAIT_LENGTH	250000

/*
 * SECTORS_PER_BLOCK
 */
#define SECTORS_PER_BLOCK	(FS_BLOCK_SIZE / DEV_BYTES_PER_SECTOR)

/*
 * Forward declarations.
 */

static void		ResetController();
static ReturnStatus	TestDisk();
static ReturnStatus	ReadDiskLabel();
static void		SetupIOPB();
static ReturnStatus	SendCommand();
static ReturnStatus	GetStatus();
static ReturnStatus	WaitForCondition();
static void 		RequestDone();
static void 		StartNextRequest();
static void		FillInDiskTransfer();


/*
 *----------------------------------------------------------------------
 *
 * xyEntryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for a
 *	device.. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for a device. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *
 * Results:
 *	TRUE if the request is processed. FALSE if the request should be
 *	enqueued.
 *
 * Side effects:
 *	Request may be dequeue and submitted to the device. Request callback
 *	function may be called.
 *
 *----------------------------------------------------------------------
 */

static Boolean
xyEntryAvailProc(clientData, newRequestPtr) 
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links *newRequestPtr;	/* The new request. */
{
    register XylogicsDisk *diskPtr = (XylogicsDisk *) clientData ;
    XylogicsController	*xyPtr = diskPtr->xyPtr;
    register Request	*requestPtr = (Request *) newRequestPtr;
    ReturnStatus	status;

    if (xyPtr->busy) {
	return FALSE;
    }
    status = SUCCESS;
    if (requestPtr->numSectors > 0) {
	status = SendCommand(diskPtr, requestPtr, FALSE);
    }
    if (status != SUCCESS) {
	RequestDone(requestPtr->diskPtr, requestPtr, status, 0);
    }
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogics450Init --
 *
 *	Initialize a Xylogics controller.
 *
 * Results:
 *	A NIL pointer if the controller does not exists. Otherwise a pointer
 *	the the XylogicsController stucture.
 *
 * Side effects:
 *	Map the controller into kernel virtual space.
 *	Allocate buffer space associated with the controller.
 *	Do a hardware reset of the controller.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevXylogics450Init(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    XylogicsController *xyPtr;	/* Xylogics specific state */
    register volatile XylogicsRegs *regsPtr;/* Control registers for Xylogics */
    char x;				/* Used when probing the controller */
    int	i;
    ReturnStatus status;


    /*
     * Poke at the controller's registers to see if it works
     * or we get a bus error.
     */
    regsPtr = (volatile XylogicsRegs *) cntrlrPtr->address;
    status = Mach_Probe(sizeof(regsPtr->resetUpdate),
			 (char *) &(regsPtr->resetUpdate), (char *) &x);
    if (status != SUCCESS) {
	return DEV_NO_CONTROLLER;
    }
    status = Mach_Probe(sizeof(regsPtr->addrLow), "x",
				(char *) &(regsPtr->addrLow));
    if (status == SUCCESS) {
	status = Mach_Probe(sizeof(regsPtr->addrLow),
			    (char *) &(regsPtr->addrLow), &x);
    }
    if (status != SUCCESS || (x != 'x') ) {
	return DEV_NO_CONTROLLER;
    }

    /*
     * Allocate and initialize the controller state info.
     */
    xyPtr = (XylogicsController *)malloc(sizeof(XylogicsController));
    bzero((char *) xyPtr, sizeof(XylogicsController));
    xylogics[cntrlrPtr->controllerID] = xyPtr;
    xyPtr->magic = XY_CNTRLR_STATE_MAGIC;
    xyPtr->busy = FALSE;
    xyPtr->regsPtr = regsPtr;
    xyPtr->number = cntrlrPtr->controllerID;
    xyPtr->requestPtr = (Request *) NIL;
    /*
     * Allocate the mapped DMA memory for the IOPB. This memory should not
     * be freed unless the controller is not going to be accessed again.
     */
    xyPtr->IOPBPtr = 
        (volatile XylogicsIOPB *)VmMach_DMAAlloc(sizeof(XylogicsIOPB),
						malloc(sizeof(XylogicsIOPB)));

    /*
     * Initialize synchronization variables and set the controllers
     * state to alive and not busy.
     */
    Sync_SemInitDynamic(&xyPtr->mutex,"Dev:xylogics mutex");
    xyPtr->numSpecialWaiting = 0;
    xyPtr->ctrlQueues = Dev_CtrlQueuesCreate(&xyPtr->mutex, xyEntryAvailProc);

    for (i = 0 ; i < XYLOGICS_MAX_DISKS ; i++) {
	 xyPtr->disks[i] =  (XylogicsDisk *) NIL;
    }
    ResetController(regsPtr);
    return( (ClientData) xyPtr);
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
static ReturnStatus
ReleaseProc(handlePtr)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
{
    PartitionDisk	*pdiskPtr = (PartitionDisk *) handlePtr;
    free((char *) pdiskPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * IOControlProc --
 *
 *      Do a special operation on a raw SMD Disk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static ReturnStatus
IOControlProc(handlePtr, command, inBufSize, inBuffer,
                                 outBufSize, outBuffer)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    int command;
    int inBufSize;
    char *inBuffer;
    int outBufSize;
    char *outBuffer;
{

     switch (command) {
	case	IOC_REPOSITION:
	    /*
	     * Reposition is ok
	     */
	    return(SUCCESS);
	    /*
	     * No disk specific bits are set this way.
	     */
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    return(SUCCESS);

	case	IOC_GET_OWNER:
	case	IOC_SET_OWNER:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_TRUNCATE:
	    return(GEN_INVALID_ARG);

	case	IOC_LOCK:
	case	IOC_UNLOCK:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_NUM_READABLE:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_MAP:
	    return(GEN_NOT_IMPLEMENTED);
	    
	default:
	    return(GEN_INVALID_ARG);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * BlockIOProc --
 *
 *	Start a block IO operations on a SMD disk attach to a xylogics 
 *	controller.
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
BlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    DevBlockDeviceRequest *requestPtr; /* IO Request to be performed. */
{
    PartitionDisk	*pdiskPtr = (PartitionDisk *) handlePtr;
    register XylogicsDisk *diskPtr = pdiskPtr->diskPtr;
    register Request	*reqPtr;

    reqPtr = (Request *) requestPtr->ctrlData;
    if (requestPtr->operation == FS_READ) {
	reqPtr->command = XY_READ;
    } else {
	reqPtr->command = XY_WRITE;
    }
    reqPtr->diskPtr = diskPtr;
    FillInDiskTransfer(pdiskPtr, requestPtr->startAddress, 
			(unsigned) requestPtr->bufferLen,
			&(reqPtr->diskAddress), &(reqPtr->numSectors));
    reqPtr->buffer = requestPtr->buffer;
    reqPtr->retries = 0;
    reqPtr->requestPtr = requestPtr;
    Dev_QueueInsert(diskPtr->queue, (List_Links *) reqPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * xyIdleCheck --
 *
 *	Routine for the Disk Stats module to use to determine the idleness
 *	for a disk.
 *
 * Results:
 *	TRUE if the disk pointed to by clientData is idle, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
xyIdleCheck(clientData) 
    ClientData	clientData;
{
    XylogicsDisk *diskPtr = (XylogicsDisk *) clientData;
    return (!diskPtr->xyPtr->busy);
}


/*
 *----------------------------------------------------------------------
 *
 * DevXylogics450DiskAttach --
 *
 *      Initialize a device hanging off an Xylogics controller. 
 *
 * Results:
 *	A DevBlockDeviceHanlde for this disk.
 *
 * Side effects:
 *	Disks:  The label sector is read and the partitioning of
 *	the disk is set up.  The partitions correspond to device
 *	files of the same type but with different unit number.
 *
 *----------------------------------------------------------------------
 */
DevBlockDeviceHandle *
DevXylogics450DiskAttach(devicePtr)
    Fs_Device	    *devicePtr;	/* Device to attach. */
{
    ReturnStatus error;
    XylogicsController *xyPtr;	/* Xylogics specific controller state */
    register XylogicsDisk *diskPtr;
    PartitionDisk	*pdiskPtr;	/* Partitioned disk pointer. */
    int		controllerID;
    int		diskNumber;

    controllerID = XYLOGICS_CTRL_NUM_FROM_DEVUNIT(devicePtr->unit);
    diskNumber = XYLOGICS_DISK_NUM_FROM_DEVUNIT(devicePtr->unit);
    xyPtr = xylogics[controllerID];
    if (xyPtr == (XylogicsController *)NIL ||
	xyPtr == (XylogicsController *)0 ||
	xyPtr->magic != XY_CNTRLR_STATE_MAGIC ||
	diskNumber > XYLOGICS_MAX_DISKS) {
	return ((DevBlockDeviceHandle *) NIL);
    }
    /*
     * Set up a slot in the disk list. We do a malloc before we grap the
     * MASTER_LOCK().
     */
    diskPtr = (XylogicsDisk *) malloc(sizeof(XylogicsDisk));
    bzero((char *) diskPtr, sizeof(XylogicsDisk));
    diskPtr->magic = XY_DISK_STATE_MAGIC;
    diskPtr->xyPtr = xyPtr;
    diskPtr->xyDriveType = 0;
    diskPtr->slaveID = diskNumber;
    diskPtr->queue = Dev_QueueCreate(xyPtr->ctrlQueues, 1, 
				DEV_QUEUE_FIFO_INSERT, (ClientData) diskPtr);

    MASTER_LOCK(&(xyPtr->mutex));
    if (xyPtr->disks[diskNumber] == (XylogicsDisk *) NIL) {
	/*
	 * See if the disk is really there.
	 */
	xyPtr->disks[diskNumber] = diskPtr;
	error = TestDisk(xyPtr, diskPtr);
	if (error == SUCCESS) {
	    /*
	     * Look at the zero'th sector for disk information.  This also
	     * sets the drive type with the controller.
	     */
	    error = ReadDiskLabel(xyPtr, diskPtr);
	}

	if (error != SUCCESS) {
	    xyPtr->disks[diskNumber] =  (XylogicsDisk *) NIL;
	    MASTER_UNLOCK(&(xyPtr->mutex));
	    Dev_QueueDestroy(diskPtr->queue);
	    free((Address)diskPtr);
	    return ((DevBlockDeviceHandle *) NIL);
	} 
	MASTER_UNLOCK(&(xyPtr->mutex));
	/* 
	 * Register the disk with the disk stats module. 
	 */
	{
	    Fs_Device	rawDevice;
	    char	name[128];

	    rawDevice =  *devicePtr;
	    rawDevice.unit = rawDevice.unit & ~0xf;
	    sprintf(name, "xy%d-%d", xyPtr->number, diskPtr->slaveID);
	    diskPtr->diskStatsPtr = DevRegisterDisk(&rawDevice, name, 
				   xyIdleCheck, (ClientData) diskPtr);
	}
    } else {
	/*
	 * The disk already exists. Use it.
	 */
	MASTER_UNLOCK(&(xyPtr->mutex));
	Dev_QueueDestroy(diskPtr->queue);
	free((Address)diskPtr);
	diskPtr = xyPtr->disks[diskNumber];
    }
    pdiskPtr = (PartitionDisk *) malloc(sizeof(*pdiskPtr));
    bzero((char *) pdiskPtr, sizeof(*pdiskPtr));
    pdiskPtr->handle.blockIOProc = BlockIOProc;
    pdiskPtr->handle.IOControlProc = IOControlProc;
    pdiskPtr->handle.releaseProc = ReleaseProc;
    pdiskPtr->handle.minTransferUnit = DEV_BYTES_PER_SECTOR;
    pdiskPtr->handle.maxTransferSize = FS_BLOCK_SIZE;
    pdiskPtr->partition = DISK_IS_PARTITIONED(devicePtr) ? 
					DISK_PARTITION(devicePtr) : 
					WHOLE_DISK_PARTITION;
    pdiskPtr->diskPtr = diskPtr;
    return ((DevBlockDeviceHandle *) pdiskPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ResetController --
 *
 *	Reset the controller.  This is done by reading the reset/update
 *	register of the controller.
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
ResetController(regsPtr)
    volatile XylogicsRegs *regsPtr;
{
    char x;
    x = regsPtr->resetUpdate;
#ifdef lint
    regsPtr->resetUpdate = x;
#endif
    MACH_DELAY(100);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TestDisk --
 *
 *	Get a drive's status to see if it exists.
 *
 * Results:
 *	SUCCESS if the device is ok, DEV_OFFLINE otherwise.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
TestDisk(xyPtr, diskPtr)
    XylogicsController *xyPtr;
    XylogicsDisk *diskPtr;
{
    register ReturnStatus status;
    Request	request;

    bzero((char *) &request, sizeof(request));

    request.command = XY_READ_STATUS;
    request.diskPtr = diskPtr;

    (void) SendCommand(diskPtr, &request, TRUE);

#ifdef notdef
	printf("TestDisk:\n");
	printf("Xylogics-%d disk %d\n", xyPtr->number, diskPtr->slaveID);
	printf("Drive Status Byte %x\n", xyPtr->IOPBPtr->numSectLow);
	printf("Drive Type %d: Cyls %d Heads %d Sectors %d\n",
			 diskPtr->xyDriveType,
			 xyPtr->IOPBPtr->cylHigh << 8 | xyPtr->IOPBPtr->cylLow,
			 xyPtr->IOPBPtr->head, xyPtr->IOPBPtr->sector);
	printf("Bytes Per Sector %d, Num sectors %d\n",
			xyPtr->IOPBPtr->dataAddrHigh << 8 |
			xyPtr->IOPBPtr->dataAddrLow,
			xyPtr->IOPBPtr->relocLow);
	MACH_DELAY(1000000);
#endif notdef
	/*
	 * If all the status bits are low then the drive is ok.
	 */
	if (xyPtr->IOPBPtr->numSectLow == 0) {
	    status = SUCCESS;
	} else if (xyPtr->IOPBPtr->numSectLow == 0x20) {
	    printf("Warning: Xylogics-%d disk %d write protected\n",
				    xyPtr->number, diskPtr->slaveID);
	    status = SUCCESS;
	} else {
	    status = DEV_OFFLINE;
	}

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * ReadDiskLabel --
 *
 *	Read the label of the disk and record the partitioning info.
 *
 * 	This should also check the Drive Type written on sector zero
 *	of cylinder zero.  Use the Read Drive Status command for this.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Define the disk partitions that determine which part of the
 *	disk each different disk device uses.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ReadDiskLabel(xyPtr, diskPtr)
    XylogicsController *xyPtr;
    XylogicsDisk *diskPtr;
{
    register ReturnStatus error ;
    Sun_DiskLabel *diskLabelPtr;
    Dev_DiskAddr diskAddr;
    int part;
    Request	request;
    char labelBuffer[DEV_BYTES_PER_SECTOR + 8]; /* Buffer for reading the
						 * disk label into. The
						 * buffer has 8 extra bytes
						 * reading low-level sector 
						 * info. */


    /*
     * Do a low level read that includes sector header info and ecc codes.
     * This is done so we can learn the drive type needed in other commands.
     */
    diskAddr.head = 0;
    diskAddr.sector = 0;
    diskAddr.cylinder = 0;

    bzero((char *) &request, sizeof(request));
    request.command = XY_RAW_READ;
    request.diskPtr = diskPtr;
    request.diskAddress = diskAddr;
    request.numSectors = 1;
    request.buffer = labelBuffer;

    error = SendCommand(diskPtr, &request, TRUE);
    if (error != SUCCESS) {
	printf("Xylogics-%d: disk%d, couldn't read the label\n",
			     xyPtr->number, diskPtr->slaveID);
    } else {
	diskPtr->xyDriveType = (labelBuffer[3] & 0xC0) >> 6;
	diskLabelPtr = (Sun_DiskLabel *)(&labelBuffer[4]);

	printf("Header Bytes: ");
	for (part=0 ; part<4 ; part++) {
	    printf("%x ", labelBuffer[part] & 0xff);
	} 
	printf("\n");
	printf("Label magic <%x>\n", diskLabelPtr->magic);
	printf("Drive type byte (%x) => type %x\n",
			  labelBuffer[3] & 0xff, diskPtr->xyDriveType);
#ifdef notdef
	MACH_DELAY(1000000);
#endif notdef
	if (diskLabelPtr->magic == SUN_DISK_MAGIC) {
	    printf("Xylogics-%d disk%d: %s\n", xyPtr->number, diskPtr->slaveID,
				    diskLabelPtr->asciiLabel);
	    diskPtr->numCylinders = diskLabelPtr->numCylinders;
	    diskPtr->numHeads = diskLabelPtr->numHeads;
	    diskPtr->numSectors = diskLabelPtr->numSectors;
    
	    printf(" Partitions ");
	    for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
		diskPtr->map[part].firstCylinder =
			diskLabelPtr->map[part].cylinder;
		diskPtr->map[part].numCylinders =
			diskLabelPtr->map[part].numBlocks /
			(diskLabelPtr->numHeads * diskLabelPtr->numSectors) ;
		printf(" (%d,%d)", diskPtr->map[part].firstCylinder,
					   diskPtr->map[part].numCylinders);
	    }
	    printf("\n");
	    /*
	     * Now that we know what the disk is like, we have to make sure
	     * that the controller does also.  The set parameters command
	     * sets the number of heads, sectors, and cylinders for the
	     * drive type.  The minus 1 is required because the controller
	     * numbers from zero and these parameters are upper bounds.
	     */
	    diskAddr.head = diskPtr->numHeads - 1;
	    diskAddr.sector = diskPtr->numSectors - 1;
	    diskAddr.cylinder = diskPtr->numCylinders - 1;

	    bzero((char *) &request, sizeof(request));
	    request.command = XY_SET_DRIVE_SIZE;
	    request.diskPtr = diskPtr;
	    request.diskAddress = diskAddr;

	    error = SendCommand(diskPtr, &request, TRUE);
	    if (error != SUCCESS) {
		printf("Xylogics-%d: disk%d, couldn't set drive size\n",
				     xyPtr->number, diskPtr->slaveID);
	    }
	} else {
	    printf("Xylogics-%d Disk %d, Unsupported label, magic = <%x>\n",
				   xyPtr->number, diskPtr->slaveID,
				   diskLabelPtr->magic);
	    error = FAILURE;
	}
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 *  FillInDiskTransfer
 *	Fill in the disk address and number of sectors of a command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The number of sectors to transfer gets trimmed down if it would
 *	cross into the next partition.
 *
 *----------------------------------------------------------------------
 */
static void
FillInDiskTransfer(pdiskPtr, startAddress, length, diskAddrPtr, numSectorsPtr)
    PartitionDisk	*pdiskPtr;	/* Target Disk Partition. */
    unsigned int	startAddress;	/* Starting offset in bytes.*/
    unsigned int	length;		/* Length in bytes. */
    Dev_DiskAddr *diskAddrPtr;		/* Disk disk address of
					 * the first sector to transfer */
    int *numSectorsPtr;			/* The number
					 * of sectors to transferred. */
{
    XylogicsDisk *diskPtr;	/* State of the disk */
    int totalSectors;	/* The total number of sectors to transfer */
    int lastSector;	/* Last sector of the partition */
    int startSector;	/* The first sector of the transfer */
    int	part;		/* Partition number. */
    int	cylinderSize;	/* Size of a cylinder in sectors. */

    diskPtr = pdiskPtr->diskPtr;
    part = pdiskPtr->partition;
    cylinderSize = diskPtr->numHeads * diskPtr->numSectors;
    /*
     * Do bounds checking to keep the I/O within the partition.
     * sectorZero is the sector number of the first sector in the partition,
     * lastSector is the sector number of the last sector in the partition.
     * (These sector numbers are relative to the start of the partition.)
     */
    lastSector = diskPtr->map[part].numCylinders * cylinderSize - 1;
    totalSectors = length/DEV_BYTES_PER_SECTOR;

    startSector = startAddress / DEV_BYTES_PER_SECTOR;

    if (startSector > lastSector) {
	/*
	 * The offset is past the end of the partition.
	 */
	*numSectorsPtr = 0;
	printf("Warning: XylogicsDiskIO: Past end of partition %d\n",
				part);
	return;
    } else if ((startSector + totalSectors - 1) > lastSector) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	totalSectors = lastSector - startSector + 1;
	printf("Warning: XylogicsDiskIO: Overrun partition %d\n",
				part);
    }
    diskAddrPtr->cylinder = startSector / cylinderSize;
    startSector -= diskAddrPtr->cylinder * cylinderSize;

    diskAddrPtr->head = startSector / diskPtr->numSectors;
    startSector -= diskAddrPtr->head * diskPtr->numSectors;

    diskAddrPtr->sector = startSector;

    diskAddrPtr->cylinder += diskPtr->map[part].firstCylinder;

    if (diskAddrPtr->cylinder > diskPtr->numCylinders) {
	panic("Xylogics, bad cylinder # %d\n",
	    diskAddrPtr->cylinder);
    }
    *numSectorsPtr = totalSectors;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SetupIOPB --
 *
 *      Setup a IOPB for a command.  The IOPB can then
 *      be passed to SendCommand.  It specifies everything the
 *	controller needs to know to do a transfer, and it is updated
 *	with status information upon completion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
static void
SetupIOPB(command, diskPtr, diskAddrPtr, numSectors, address, interrupt,IOPBPtr)
    char command;			/* One of the Xylogics commands */
    XylogicsDisk *diskPtr;		/* Spefifies unit, drive type, etc */
    register Dev_DiskAddr *diskAddrPtr;	/* Head, sector, cylinder */
    int numSectors;			/* Number of sectors to transfer */
    register Address address;		/* Main memory address of the buffer */
    Boolean interrupt;			/* If TRUE use interupts, else poll */
    register volatile XylogicsIOPB *IOPBPtr;	/* I/O Parameter Block  */
{
    bzero((Address)IOPBPtr,sizeof(XylogicsIOPB));

    IOPBPtr->autoUpdate	 	= 1;
    IOPBPtr->relocation	 	= 1;
    IOPBPtr->doChaining	 	= 0;	/* New IOPB's are always end of chain */
    IOPBPtr->interrupt	 	= interrupt;
    IOPBPtr->command	 	= command;

    IOPBPtr->intrIOPB	 	= interrupt;	/* Polling or interrupt mode */
    IOPBPtr->autoSeekRetry	= 1;
    IOPBPtr->enableExtras	= 0;
    IOPBPtr->eccMode		= 2;	/* Correct soft errors for me, please */

    IOPBPtr->transferMode	= 0;	/* For words, not bytes */
    IOPBPtr->interleave		= 0;	/* For non interleaved */
    IOPBPtr->throttle		= 4;	/* max 32 words per burst */

    IOPBPtr->unit		= diskPtr->slaveID;
    IOPBPtr->driveType		= diskPtr->xyDriveType;

    if (diskAddrPtr != (Dev_DiskAddr *)NIL) {
	IOPBPtr->head		= diskAddrPtr->head;
	IOPBPtr->sector		= diskAddrPtr->sector;
	IOPBPtr->cylHigh	= (diskAddrPtr->cylinder & 0x0700) >> 8;
	IOPBPtr->cylLow		= (diskAddrPtr->cylinder & 0x00ff);
    }

    IOPBPtr->numSectHigh	= (numSectors & 0xff00) >> 8;
    IOPBPtr->numSectLow		= (numSectors & 0x00ff);

    if ((unsigned)address != 0 && (unsigned)address != (unsigned) NIL) {
	if ((unsigned)address < VMMACH_DMA_START_ADDR) {
	    printf("%x: ", address);
	    panic("Xylogics data address not in DMA space\n");
	}
	address = (Address)( (unsigned int)address - VMMACH_DMA_START_ADDR );
	IOPBPtr->relocHigh	= ((int)address & 0xff000000) >> 24;
	IOPBPtr->relocLow	= ((int)address & 0x00ff0000) >> 16;
	IOPBPtr->dataAddrHigh	= ((int)address & 0x0000ff00) >> 8;
	IOPBPtr->dataAddrLow	= ((int)address & 0x000000ff);
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Lower level routine to read or write an Xylogics device.  Use this
 *      to transfer a contiguous set of sectors.    The interface here
 *      is in terms of a particular Xylogics disk and the command to be
 *      sent.  This routine takes care of mapping its
 *      buffer into the special multibus memory area that is set up for
 *      Sun DMA.
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
SendCommand(diskPtr, requestPtr, wait)
    XylogicsDisk *diskPtr;	/* Unit, type, etc, of disk to send command. */
    Request	 *requestPtr;	/* Command request block. */
    Boolean 	 wait;		/* Wait for the command to complete. */
{
    XylogicsController *xyPtr;	/* The Xylogics controller doing the command. */
    char command;		/* One of the standard Xylogics commands */
    Dev_DiskAddr *diskAddrPtr;	/* Head, sector, cylinder */
    int numSectors;		/* Number of sectors to transfer */
    Address address;		/* Main memory address of the buffer */
    ReturnStatus error;
    register volatile XylogicsRegs *regsPtr;	/* I/O registers */
    unsigned int IOPBAddr;
    int retries = 0;

    xyPtr = diskPtr->xyPtr;

    if (xyPtr->busy) {
	if (wait) {
	    while (xyPtr->busy) {
		xyPtr->numSpecialWaiting++;
		Sync_MasterWait(&xyPtr->specialCmdWait, &xyPtr->mutex, FALSE);
		xyPtr->numSpecialWaiting--;
	    }
	} else { 
	    panic("Xylogics-%d: Software error, marked busy in SendCommand\n",
		    xyPtr->number);
	}
    }
    xyPtr->busy = TRUE;

    command = requestPtr->command;
    diskAddrPtr = &requestPtr->diskAddress;
    address = requestPtr->buffer;
    numSectors = requestPtr->numSectors;

    /*
     * Without chaining the controller should always be idle at this
     * point.  
     */
    regsPtr = xyPtr->regsPtr;
    if (regsPtr->status & XY_GO_BUSY) {
	printf("Warning: Xylogics waiting for busy controller\n");
	(void)WaitForCondition(regsPtr, XY_GO_BUSY);
    }
    /*
     * Set up the I/O registers for the transfer.  All addresses given to
     * the controller must be relocated to the low megabyte so that the Sun
     * MMU will recognize them and map them back into the high megabyte of
     * the kernel's virtual address space.  (As circular as this sounds,
     * the level of indirection means the system can use any physical page
     * for an I/O buffer.)
     */
    if ((unsigned int)xyPtr->IOPBPtr < VMMACH_DMA_START_ADDR) {
	printf("%x: ", xyPtr->IOPBPtr);
	panic("Warning: Xylogics IOPB not in DMA space\n");
	xyPtr->busy = FALSE;
	return(FAILURE);
    }
    IOPBAddr = (unsigned int)xyPtr->IOPBPtr - VMMACH_DMA_START_ADDR;
    regsPtr->relocHigh = (IOPBAddr & 0xFF000000) >> 24;
    regsPtr->relocLow  = (IOPBAddr & 0x00FF0000) >> 16;
    regsPtr->addrHigh  = (IOPBAddr & 0x0000FF00) >>  8;
    regsPtr->addrLow   = (IOPBAddr & 0x000000FF);

    /*
     * If the command is going to transfer data be relocate the address
     * into DMA space. 
     */
    if ((address != (Address) 0) && (numSectors > 0)) {
	if (command == XY_RAW_READ || command == XY_RAW_WRITE) {
	    xyPtr->dmaBufferSize = (DEV_BYTES_PER_SECTOR + 8) * numSectors;
	} else {
	    xyPtr->dmaBufferSize = DEV_BYTES_PER_SECTOR  * numSectors;
	}
	xyPtr->dmaBuffer =  VmMach_DMAAlloc(xyPtr->dmaBufferSize, address);
    } else {
	xyPtr->dmaBufferSize = 0;
    }

retry:
    SetupIOPB(command, diskPtr, diskAddrPtr, numSectors, xyPtr->dmaBuffer, 
		!wait, xyPtr->IOPBPtr);
#ifdef notdef
    if (TRUE) {
	char *address;
	int	i;
	printf("IOBP bytes: ");
	address = (char *)xyPtr->IOPBPtr;
	for (i=0 ; i<24 ; i++) {
	    printf("%x ", address[i] & 0xff);
	}
	printf("\n");
    }
#endif
    /*
     * Start up the controller
     */
    regsPtr->status = XY_GO_BUSY;

    if (wait) {
        /*
         * A synchronous command.  Wait here for the command to complete.
         */
        error = WaitForCondition(regsPtr, XY_GO_BUSY);
        if (error != SUCCESS) {
            printf("Xylogics-%d: couldn't wait for command to complete\n",
                                 xyPtr->number);
        } else {
            /*
             * Check for error status from the operation itself.
             */
            error = GetStatus(xyPtr);
            if (error == DEV_RETRY_ERROR && retries < 3) {
                retries++;
		printf("Warning: Xylogics Retrying...\n");
                goto retry;
            }
        }
	if (xyPtr->dmaBufferSize > 0) { 
	    VmMach_DMAFree(xyPtr->dmaBufferSize, xyPtr->dmaBuffer);
	    xyPtr->dmaBufferSize = 0;
	}
	xyPtr->busy = FALSE;
	StartNextRequest(xyPtr);
    } else { 
       /*
        * Store request for interrupt handler. 
	*/
       xyPtr->requestPtr = requestPtr;
       error = SUCCESS;
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * GetStatus --
 *
 *	Tidy up after a Xylogics command by looking at status bytes from
 *	the device.
 *
 * Results:
 *	An error code from the recently completed transfer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetStatus(xyPtr)
    XylogicsController *xyPtr;
{
    register ReturnStatus error = SUCCESS;
    register volatile XylogicsRegs *regsPtr;
    register volatile XylogicsIOPB *IOPBPtr;

    regsPtr = xyPtr->regsPtr;
    IOPBPtr = xyPtr->IOPBPtr;
    if ((regsPtr->status & XY_ERROR) || IOPBPtr->error) {
	if (regsPtr->status & XY_DBL_ERROR) {
	    printf("Xylogics-%d double error %x\n", xyPtr->number,
				    IOPBPtr->errorCode);
	    error = DEV_HARD_ERROR;
	} else {
	    switch (IOPBPtr->errorCode) {
		case XY_NO_ERROR:
		    error = SUCCESS;
		    break;
		case XY_ERR_BAD_CYLINDER:
		    printf("Xylogics bad cylinder # %d\n",
				 IOPBPtr->cylHigh << 8 | IOPBPtr->cylLow);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_BAD_SECTOR:
		    printf("Xylogics bad sector # %d\n", IOPBPtr->sector);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_BAD_HEAD:
		    printf("Xylogics bad head # %d\n", IOPBPtr->head);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_ZERO_COUNT:
		    printf("Xylogics zero count\n");
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_INTR_PENDING:
		case XY_ERR_BUSY_CONFLICT:
		case XY_ERR_BAD_COMMAND:
		case XY_ERR_BAD_SECTOR_SIZE:
		case XY_ERR_SELF_TEST_A:
		case XY_ERR_SELF_TEST_B:
		case XY_ERR_SELF_TEST_C:
		case XY_ERR_SLIP_SECTOR:
		case XY_ERR_SLAVE_ACK:
		case XY_FORMAT_ERR_RUNT:
		case XY_FORMAT_ERR_BAD_SIZE:
		case XY_SOFT_ECC:
		    panic("Stupid Xylogics error: 0x%x\n",
					    IOPBPtr->errorCode);
		    error = DEV_HARD_ERROR;
		    break;
		case  XY_SOFT_ERR_TIME_OUT:
		case  XY_SOFT_ERR_BAD_HEADER:
		case  XY_SOFT_ERR_ECC:
		case  XY_SOFT_ERR_NOT_READY:
		case  XY_SOFT_ERR_HEADER:
		case  XY_SOFT_ERR_FAULT:
		case  XY_SOFT_ERR_SEEK:
		    error = DEV_RETRY_ERROR;
		    printf("Warning: Retryable Xylogics error: 0x%x\n",
				IOPBPtr->errorCode);
		    break;
		case XY_WRITE_PROTECT_ON:
		    printf("Xylogics-%d: ", xyPtr->number);
		    printf("Warning: Write protected\n");
		    error = GEN_NO_PERMISSION;
		    break;
		case XY_SOFT_ECC_RECOVERED:
		    printf("Xylogics-%d: ", xyPtr->number);
		    printf("Warning: Soft ECC error recovered\n");
		    error = SUCCESS;
		    break;
		default:
		    error = DEV_HARD_ERROR;
		    break;
	    }
	}
	/*
	 * Reset the error by writing a 1 to the XY_ERROR bit.
	 */
	if (regsPtr->status & XY_ERROR) {
	    regsPtr->status = XY_ERROR;
	}
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForCondition --
 *
 *	Wait for the Xylogics controller to finish.  This is done by
 *	polling its GO_BUSY bit until it reads zero.
 *
 * Results:
 *	SUCCESS if it completed before a threashold time limit,
 *	DEV_TIMEOUT otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WaitForCondition(regsPtr, condition)
    volatile XylogicsRegs *regsPtr;
    int condition;
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;

    for (i=0 ; i<XYLOGICS_WAIT_LENGTH ; i++) {
	if ((regsPtr->status & condition) == 0) {
	    return(SUCCESS);
	} else if (regsPtr->status & XY_ERROR) {
	    /*
	     * Let GetStatus() figure out what happened
	     */
	    return(SUCCESS);
	}
	MACH_DELAY(10);
    }
    printf("Warning: Xylogics reset");
    ResetController(regsPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogics450Intr --
 *
 *	Handle interrupts from the Xylogics controller.   This routine
 *	may start any queued requests.
 *
 * Results:
 *	TRUE if an Xylogics controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevXylogics450Intr(clientData)
    ClientData	clientData;
{
    ReturnStatus error = SUCCESS;
    register XylogicsController *xyPtr;
    register volatile XylogicsRegs *regsPtr;
    register volatile XylogicsIOPB *IOPBPtr;
    Request	*requestPtr;

    xyPtr = (XylogicsController *) clientData;
    regsPtr = xyPtr->regsPtr;
    if (regsPtr->status & XY_INTR_PENDING) {
	MASTER_LOCK(&(xyPtr->mutex));
	/*
	 * Reset the pending interrupt by writing a 1 to the
	 * INTR_PENDING bit of the status register.
	 */
	regsPtr->status = XY_INTR_PENDING;
	IOPBPtr = xyPtr->IOPBPtr;
	requestPtr = xyPtr->requestPtr;
	/*
	 * Release the DMA buffer if command mapped one. 
	 */
	if (xyPtr->dmaBufferSize > 0) {
	    VmMach_DMAFree(xyPtr->dmaBufferSize, xyPtr->dmaBuffer);
	    xyPtr->dmaBufferSize = 0;
	}
	if ((regsPtr->status & XY_ERROR) || IOPBPtr->error) {
	    error = GetStatus(xyPtr);
	    if (error == DEV_RETRY_ERROR) {
		requestPtr->retries++;
		if (requestPtr->retries < 3) {
		    xyPtr->busy = FALSE;
		    error = SendCommand(requestPtr->diskPtr, requestPtr,FALSE);
		    if (error == SUCCESS) {
			MASTER_UNLOCK(&(xyPtr->mutex));
			return (TRUE);
		    }
		}
	    }
	}
	/*
	 * For now there are no occasions where only part
	 * of an I/O can complete.
	 */
	xyPtr->busy = FALSE;
	RequestDone(requestPtr->diskPtr,requestPtr,error,requestPtr->numSectors);
	StartNextRequest(xyPtr);
	MASTER_UNLOCK(&(xyPtr->mutex));
	return(TRUE);
    }
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * RequestDone --
 *
 *	Mark a request as done by calling the request's doneProc. DMA memory
 *	is released.
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
RequestDone(diskPtr, requestPtr, status, numSectors)
    XylogicsDisk *diskPtr;
    Request	*requestPtr;
    ReturnStatus	status;
    int		numSectors;
{
    if (numSectors > 0) {
	if (requestPtr->requestPtr->operation == FS_READ) {
	    diskPtr->diskStatsPtr->diskReads++;
	} else {
	    diskPtr->diskStatsPtr->diskWrites++;
	}
    }
    MASTER_UNLOCK(&diskPtr->xyPtr->mutex);
    (requestPtr->requestPtr->doneProc)(requestPtr->requestPtr, status,
				numSectors*DEV_BYTES_PER_SECTOR);
    MASTER_LOCK(&diskPtr->xyPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * StartNextRequest --
 *
 *	Start the next request of a Xylogics450 controller.
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
StartNextRequest(xyPtr)
    XylogicsController *xyPtr;
{
    ClientData	clientData;
    List_Links *newRequestPtr;
    ReturnStatus	status;

     /*
     * If the controller is busy, just return.
     */
    if (xyPtr->busy) {
	return;
    }
    /*
     * If a SPECIAL command is waiting wake up the process to do the command.
     */
    if (xyPtr->numSpecialWaiting > 0) {
	Sync_MasterBroadcast(&xyPtr->specialCmdWait);
	return;
    }
    /* 
     * Otherwise process requests from the Queue. 
     */
    newRequestPtr = Dev_QueueGetNextFromSet(xyPtr->ctrlQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
    while (newRequestPtr != (List_Links *) NIL) {
	register Request *requestPtr;
	XylogicsDisk	*diskPtr;

	requestPtr = (Request *) newRequestPtr;
	diskPtr = (XylogicsDisk *) clientData;

	status = SendCommand(diskPtr, requestPtr, FALSE);
	if (status != SUCCESS) {
	    RequestDone(requestPtr->diskPtr, requestPtr, status, 0);
	    newRequestPtr = Dev_QueueGetNextFromSet(xyPtr->ctrlQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
	} else {
	    break;
	}
    }
    return;
}

