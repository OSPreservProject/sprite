/* 
 * devSCSI.c --
 *
 *	SCSI = Small Computer System Interface.
 *	Device driver for the SCSI disk and tape interface.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSCSIWorm.h"

#include "multibus.h"
#include "sunMon.h"
#include "sunDiskLabel.h"
#include "dbg.h"
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "proc.h"	/* for Sys_SetJump */
#include "fs.h"
#include "mem.h"
#include "user/byte.h"

/*
 * State for each SCSI controller.
 */
static DevSCSIController *scsi[SCSI_MAX_CONTROLLERS];

/*
 * State for each SCSI disk.  The state for all SCSI disks are kept
 * together so that the driver can easily find the disk and partition
 * that correspond to a filesystem unit number.
 */

DevSCSIDevice *scsiDisk[SCSI_MAX_DISKS];
static int scsiDiskIndex = -1;

/*
 * State for each SCSI tape drive.  This used to map from unit numbers
 * back to the controller for the drive.
 */
static int scsiTapeIndex = -1;
DevSCSIDevice *scsiTape[SCSI_MAX_TAPES];

/*
 * State for each SCSI worm drive.  This used to map from unit numbers
 * back to the controller for the drive.
 */
static int scsiWormIndex = -1;
DevSCSIDevice *scsiWorm[SCSI_MAX_WORMS];

/*
 * SetJump stuff needed when probing for the existence of a device.
 */
static Sys_SetJumpState setJumpState;

/*
 * DevSCSICommand() takes a Boolean that indicates whether it should cause
 * an interupt when the command is complete or whether it should busy wait
 * until the command finishes.  These defines make the calls clearer.
 */
#define INTERRUPT	TRUE
#define WAIT		FALSE

/*
 * DevSCSIWait() takes a Boolean that indicates whether it should reset
 * the SCSI bus if the condition being waited on never occurs.
 */
#define RESET		TRUE
#define NO_RESET	FALSE

/*
 * SECTORS_PER_BLOCK
 */
#define SECTORS_PER_BLOCK	(FS_BLOCK_SIZE / DEV_BYTES_PER_SECTOR)
#define SECTORS_PER_WORM_BLOCK	(FS_BLOCK_SIZE / DEV_BYTES_PER_WORM_SECTOR)

/*
 * Define the maximum number of sectors that may be transferred to the
 * RXT in one shot.  Since we allocate space from the kernel's address
 * space statically, we don't want to make it too much even though
 * the drive can handle 256 blocks in a shot.  Besides, the drive only
 * transfers 32 sectors at a time.
 */
#define MAX_WORM_SECTORS_IO 32

/*
 * This utility macro should probably be defined in some global header file.
 */
#define max(a,b) (((a) >= (b)) ? (a) : (b))

/*
 * The error codes for class 0-6 sense data are class specific.
 * The follow arrays of strings are used to print error messages.
 */
char *scsiClass0Errors[] = {
    "No sense data",
    "No index signal",
    "No seek complete",
    "Write fault",
    "Drive notready",
    "Drive not selected",
    "No Track 00",
    "Multiple drives selected",
    "No address acknowledged",
    "Media not loaded",
    "Insufficient capacity",
};
char *scsiClass1Errors[] = {
    "ID CRC error",
    "Unrecoverable data error",
    "ID address mark not found",
    "Data address mark not found",
    "Record not found",
    "Seek error",
    "DMA timeout error",
    "Write protected",
    "Correctable data check",
    "Bad block found",
    "Interleave error",
    "Data transfer incomplete",
    "Unformatted or bad format on drive",
    "Self test failed",
    "Defective track (media errors)",
};
char *scsiClass2Errors[] = {
    "Invalid command",
    "Illegal block address",
    "Aborted",
    "Volume overflow",
};
int scsiNumErrors[] = {
    sizeof(scsiClass0Errors) / sizeof (char *),
    sizeof(scsiClass1Errors) / sizeof (char *),
    sizeof(scsiClass2Errors) / sizeof (char *),
    0, 0, 0, 0,
};
char **scsiErrors[] = {
    scsiClass0Errors,
    scsiClass1Errors,
    scsiClass2Errors,
};

int devSCSIDebug = FALSE;

/*
 * Forward declarations.
 */

void		DevSCSIReset();
ReturnStatus	DevSCSITest();
void		DevSCSIDoLabel();
void		DevSCSISetupCommand();
ReturnStatus	DevSCSICommand();
ReturnStatus	DevSCSIStatus();
ReturnStatus	DevSCSIWait();

extern ReturnStatus DevSCSIDiskError();
extern ReturnStatus DevSCSITapeError();
extern ReturnStatus DevSCSIWormError();


/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIInitController --
 *
 *	Initialize an SCSI controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocate buffer space associated with the controller.
 *	Do a hardware reset of the controller.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIInitController(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    DevSCSIController *scsiPtr;		/* SCSI specific state */
    register DevSCSIRegs *regsPtr;	/* Control registers for SCSI */
    int x;				/* Used when probing the controller */

    /*
     * Allocate space for SCSI specific state and
     * initialize the controller itself.
     */
    scsiPtr = (DevSCSIController *)Mem_Alloc(sizeof(DevSCSIController));
    scsi[cntrlrPtr->controllerID] = scsiPtr;
    scsiPtr->number = cntrlrPtr->controllerID;

    /*
     * Poke at the controller's registers to see if it works
     * or we get a bus error.
     */
    scsiPtr->regsPtr = (DevSCSIRegs *)cntrlrPtr->address;
    regsPtr = scsiPtr->regsPtr;
    if (Sys_SetJump(&setJumpState) == SUCCESS) {
	x = regsPtr->dmaCount;
	regsPtr->dmaCount = (short)0xBABE;
	if (regsPtr->dmaCount != (short)0xBABE) {
	    Sys_Printf("SCSI-%d: control register read-back problem\n",
				 scsiPtr->number);
	    Sys_UnsetJump();
	    return(FALSE);
	}
    } else {
	Sys_UnsetJump();
	/*
	 * Got a bus error. Zap the info about the non-existent controller.
	 */
	Mem_Free((Address) scsiPtr);
	scsi[cntrlrPtr->controllerID] = (DevSCSIController *)NIL;
	return(FALSE);
    }
    Sys_UnsetJump();

    DevSCSIReset(regsPtr);

    /*
     * Initialize autovector index for VME based boxes.
     */
    if (cntrlrPtr->vectorNumber > 0) {
	regsPtr->intrVector = cntrlrPtr->vectorNumber;
    }
    /*
     * Allocate the mapped multibus memory to buffers:  one small buffer
     * for sense data, a one sector buffer for the label, and one buffer
     * for reading and writing filesystem blocks.  A physical page is
     * obtained for the sense data and the label.  The general buffer gets
     * mapped just before a read or write.  It has to be twice as large as
     * a filesystem block so that an unaligned block can be mapped into it.
     */
    scsiPtr->senseBuffer =
	    (DevSCSISense *)VmMach_DevBufferAlloc(&devIOBuffer,
					       sizeof(DevSCSISense));
    VmMach_GetDevicePage((int)scsiPtr->senseBuffer);

    scsiPtr->labelBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
					     DEV_BYTES_PER_SECTOR);
    VmMach_GetDevicePage((int)scsiPtr->labelBuffer);

    scsiPtr->IOBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
            2 * max(FS_BLOCK_SIZE,
		    MAX_WORM_SECTORS_IO * DEV_BYTES_PER_WORM_SECTOR));
    
    /*
     * Initialize synchronization variables and set the controllers
     * state to alive and not busy.
     */
    scsiPtr->mutex = 0;
    scsiPtr->IOComplete.waiting = 0;
    scsiPtr->readyForIO.waiting = 0;
    scsiPtr->flags = SCSI_CNTRLR_ALIVE;

    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIInitDevice --
 *
 *	Initialize a device hanging off an SCSI controller.
 *	This keeps track of how many times it is called with
 *	for disks and tapes so that it can properly correlate
 *	filesystem unit numbers to particular devices.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Disks:  The label sector is read and the partitioning of
 *	the disk is set up.  The partitions correspond to device
 *	files of the same type but with different unit number.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIInitDevice(devConfPtr)
    DevConfigDevice *devConfPtr;	/* Config info about the device */
{
    ReturnStatus status;
    DevSCSIController *scsiPtr;	/* SCSI specific controller state */
    DevSCSIDevice *devPtr;	/* Device specific state */

    if (devConfPtr->flags == DEV_SCSI_DISK) {
	/*
	 * Increment this before checking for the controller so the
	 * unit numbers match up right.  ie. each controller accounts
	 * for DEV_NUM_DISK_PARTS unit numbers, and the unit number is
	 * used to index the devDisk array (div DEV_NUM_DISK_PARTS).
	 */
	scsiDiskIndex++;
    } else if (devConfPtr->flags == DEV_SCSI_TAPE) {
	scsiTapeIndex++;
    } else if (devConfPtr->flags == DEV_SCSI_WORM) {
	scsiWormIndex++;
    }

    scsiPtr = scsi[devConfPtr->controllerID];
    if (scsiPtr == (DevSCSIController *)NIL ||
	scsiPtr == (DevSCSIController *)0) {
	return(FALSE);
    }

    devPtr = (DevSCSIDevice *)Mem_Alloc(sizeof(DevSCSIDevice));
    devPtr->scsiPtr = scsiPtr;
    devPtr->subUnitID = 0;
    devPtr->slaveID = devConfPtr->slaveID;
    if (devConfPtr->flags == DEV_SCSI_DISK) {
	register DevSCSIDisk *diskPtr;
	/*
	 * Check that the disk is on-line.  This means we won't find a disk
	 * if its powered down upon boot.
	 */
	devPtr->type = SCSI_DISK;
	devPtr->errorProc = DevSCSIDiskError;
	devPtr->sectorSize = DEV_BYTES_PER_SECTOR;
	status = DevSCSITest(devPtr);
	if (status != SUCCESS) {
	    Mem_Free((Address)devPtr);
	    return(FALSE);
	}
	/*
	 * Set up a slot in the disk list. See above about scsiDiskIndex.
	 */
	if (scsiDiskIndex >= SCSI_MAX_DISKS) {
	    Sys_Printf("SCSI: Too many disks configured\n");
	    Mem_Free((Address)devPtr);
	    return(FALSE);
	}
	diskPtr = (DevSCSIDisk *) Mem_Alloc(sizeof(DevSCSIDisk));
	devPtr->data = (ClientData)diskPtr;
	scsiDisk[scsiDiskIndex] = devPtr;
	DevSCSIDoLabel(devPtr);
    } else if (devConfPtr->flags == DEV_SCSI_TAPE) {
	register DevSCSITape *tapePtr;

	/*
	 * Don't try to talk to the tape drive at boot time.  It may be doing
	 * stuff after the SCSI bus reset like auto load.
	 */
	if (scsiTapeIndex >= SCSI_MAX_TAPES) {
	    Sys_Printf("SCSI: Too many tape drives configured\n");
	    Mem_Free((Address)devPtr);
	    return(FALSE);
	}
	devPtr->type = SCSI_TAPE;
	devPtr->errorProc = DevSCSITapeError;
	devPtr->sectorSize = DEV_BYTES_PER_SECTOR;
	tapePtr = (DevSCSITape *) Mem_Alloc(sizeof(DevSCSITape));
	devPtr->data = (ClientData)tapePtr;
	tapePtr->state = SCSI_TAPE_CLOSED;
	tapePtr->type = SCSI_UNKNOWN;
	scsiTape[scsiTapeIndex] = devPtr;
	Sys_Printf("SCSI-%d tape %d at slave %d\n",
		    scsiPtr->number, scsiTapeIndex, devPtr->slaveID);
    } else if (devConfPtr->flags == DEV_SCSI_WORM) {
	register DevSCSIWorm *wormPtr;
	/*
	 * Check that the worm is on-line.  This means we won't find a disk
	 * if it's powered down upon boot.
	 */
	devPtr->type = SCSI_WORM;
	devPtr->errorProc = DevSCSIWormError;
	devPtr->sectorSize = DEV_BYTES_PER_WORM_SECTOR;
	status = DevSCSITest(devPtr);
	if (status != SUCCESS) {
	    Mem_Free((Address)devPtr);
	    return(FALSE);
	}
	if (scsiWormIndex >= SCSI_MAX_WORMS) {
	    Sys_Printf("SCSI: Too many worm drives configured\n");
	    Mem_Free((Address)devPtr);
	    return(FALSE);
	}
	wormPtr = (DevSCSIWorm *) Mem_Alloc(sizeof(DevSCSIWorm));
	wormPtr->state = SCSI_WORM_CLOSED;
	devPtr->data = (ClientData)wormPtr;
	scsiWorm[scsiWormIndex] = devPtr;
	Sys_Printf("SCSI-%d worm %d at slave %d\n",
		    scsiPtr->number, scsiWormIndex, devPtr->slaveID);
    }
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIReset --
 *
 *	Reset the controller.
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
DevSCSIReset(regsPtr)
    DevSCSIRegs *regsPtr;
{
    regsPtr->control = SCSI_RESET;
    DELAY(100);
    regsPtr->control = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITest --
 *
 *	Test an SCSI device to see if it is ready.
 *
 * Results:
 *	SUCCESS if the device is ok, DEV_OFFLINE otherwise.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITest(devPtr)
    DevSCSIDevice *devPtr;
{
    register ReturnStatus status;

    DevSCSISetupCommand(SCSI_TEST_UNIT_READY, devPtr, 0, 0);

    status = DevSCSICommand(devPtr->slaveID, devPtr->scsiPtr, 0,
				(Address)0, WAIT);
    if (status == DEV_TIMEOUT) {
	status = DEV_OFFLINE;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDoLabel --
 *
 *	Read the label of the disk and record the partitioning info.
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
void
DevSCSIDoLabel(devPtr)
    DevSCSIDevice *devPtr;
{
    register DevSCSIController *scsiPtr = devPtr->scsiPtr;
    DevSCSIDisk *diskPtr;
    register ReturnStatus status;
    Sun_DiskLabel *diskLabelPtr;
    int part;

    DevSCSISetupCommand(SCSI_READ, devPtr, 0, 1);

    status = DevSCSICommand(devPtr->slaveID, scsiPtr, DEV_BYTES_PER_SECTOR,
			    scsiPtr->labelBuffer, WAIT);
    if (status != SUCCESS) {
	Sys_Printf("SCSI-%d: couldn't read the disk%d label\n",
			     scsiPtr->number, devPtr->slaveID);
	return;
    }
    diskLabelPtr = (Sun_DiskLabel *)scsiPtr->labelBuffer;
    Sys_Printf("SCSI-%d disk%d: %s\n", scsiPtr->number, devPtr->slaveID,
			diskLabelPtr->asciiLabel);

    diskPtr = (DevSCSIDisk *)devPtr->data;
    diskPtr->numCylinders = diskLabelPtr->numCylinders;
    diskPtr->numHeads = diskLabelPtr->numHeads;
    diskPtr->numSectors = diskLabelPtr->numSectors;

    Sys_Printf(" Partitions ");
    for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
	diskPtr->map[part].firstCylinder =
		diskLabelPtr->map[part].cylinder;
	diskPtr->map[part].numCylinders =
		diskLabelPtr->map[part].numBlocks /
		(diskLabelPtr->numHeads * diskLabelPtr->numSectors) ;
	Sys_Printf(" (%d,%d)", diskPtr->map[part].firstCylinder,
				   diskPtr->map[part].numCylinders);
    }
    Sys_Printf("\n");
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDiskIO --
 *
 *      Read or Write (to/from) a raw SCSI disk file. The deviceUnit
 *      number is mapped to a particular partition on a particular disk.
 *      The starting coordinate, firstSector,  is relocated to be relative
 *      to the corresponding disk partition.  The transfer is checked
 *      against the partition size to make sure that the I/O doesn't cross
 *      a disk partition.
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
ReturnStatus
DevSCSIDiskIO(command, deviceUnit, buffer, firstSector, numSectorsPtr)
    int command;			/* SCSI_READ or SCSI_WRITE */
    int deviceUnit;			/* Unit from the filesystem that
					 * indicates a disk and partition */
    char *buffer;			/* Target buffer */
    int firstSector;			/* First sector to transfer. Should be
					 * relative to the start of the disk
					 * partition corresponding to the unit*/
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors actually transferred. */
{
    ReturnStatus status;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    int part;		/* Partition of disk that corresponds to unit number */
    DevSCSIDevice *devPtr;		/* Generic SCSI device state */
    register DevSCSIDisk *diskPtr;	/* State of the disk */
    int totalSectors;		/* The total number of sectors to transfer */
    int numSectors;		/* The number of sectors to transfer at
				 * one time, up to a blocks worth. */
    int lastSector;	/* Last sector of the partition */
    int totalRead;	/* The total number of sectors actually transferred */

    disk = deviceUnit / DEV_NUM_DISK_PARTS;
    part = deviceUnit % DEV_NUM_DISK_PARTS;
    devPtr = scsiDisk[disk];
    diskPtr = (DevSCSIDisk *)devPtr->data;

    /*
     * Do bounds checking to keep the I/O within the partition.
     */
    lastSector = diskPtr->map[part].numCylinders *
		 (diskPtr->numHeads * diskPtr->numSectors) - 1;
    totalSectors = *numSectorsPtr;

    if (firstSector > lastSector) {
	/*
	 * The offset is past the end of the partition.
	 */
	*numSectorsPtr = 0;
	return(SUCCESS);
    } else if ((firstSector + totalSectors - 1) > lastSector) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	totalSectors = lastSector - firstSector + 1;
    }
    /*
     * Relocate the disk address to be relative to this partition.
     */
    firstSector += diskPtr->map[part].firstCylinder *
		    (diskPtr->numHeads * diskPtr->numSectors);
    /*
     * Chop up the IO into blocksize pieces.
     */
    totalRead = 0;
    do {
	if (totalSectors > SECTORS_PER_BLOCK) {
	    numSectors = SECTORS_PER_BLOCK;
	} else {
	    numSectors = totalSectors;
	}
	status = DevSCSISectorIO(command, devPtr, firstSector, &numSectors, buffer);
	if (status == SUCCESS) {
	    firstSector += numSectors;
	    totalSectors -= numSectors;
	    buffer += numSectors * DEV_BYTES_PER_SECTOR;
	    totalRead += numSectors;
	}
    } while (status == SUCCESS && totalSectors > 0);
    *numSectorsPtr = totalRead;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSISetupCommand --
 *
 *      Setup a control block for a command.  The control block can then
 *      be passed to DevSCSICommand.  The control block specifies the a
 *      sub-unit to the controller, the command, and the device address of
 *      the transfer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
void
DevSCSISetupCommand(command, devPtr, blockNumber, numSectors)
    char command;	/* One of six standard SCSI commands */
    DevSCSIDevice *devPtr;	/* Device state */
    int blockNumber;	/* The starting block number for the transfer */
    int numSectors;	/* Number of sectors (or bytes!) to transfer */
{
    register DevSCSIControlBlock *controlBlockPtr;

    devPtr->scsiPtr->devPtr = devPtr;
    controlBlockPtr = &devPtr->scsiPtr->controlBlock;
    Byte_Zero(sizeof(DevSCSIControlBlock), (Address)controlBlockPtr);
    controlBlockPtr->command = command;
    controlBlockPtr->unitNumber = devPtr->subUnitID;
    controlBlockPtr->highAddr = (blockNumber & 0x1f0000) >> 16;
    controlBlockPtr->midAddr =  (blockNumber & 0x00ff00) >> 8;
    controlBlockPtr->lowAddr =  (blockNumber & 0x0000ff);
    controlBlockPtr->blockCount =  numSectors;
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSISetupTapeCommand --
 *
 *	A variation on DevSCSISetupCommand that creates a control block
 *	designed for tape drives.  SCSI tape drives read from the current
 *	tape position, so there is only a block count, no offset.  There
 *	is a special code that modifies the command in the tape control
 *	block.  The value of the code is a function of the command and the
 *	type of the tape drive (ugh.)  The correct code is determined here.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the tape control block.
 *
 *----------------------------------------------------------------------
 */
void
DevSCSISetupTapeCommand(command, devPtr, countPtr)
    int command;		/* One of SCSI_* commands */
    DevSCSIDevice *devPtr;	/* Device state */
    int *countPtr;		/* In - Transfer count, blocks or bytes!
				 * Out - The proper dma byte count for caller */
{
    register DevSCSITapeControlBlock	*tapeControlBlockPtr;
    register DevSCSITape		*tapePtr;
    char code = 0;					/* Modifies command */
    int dmaCount = *countPtr;		/* DMA count needed by host interface */
    int count = *countPtr;		/* Count put in the control block */

    devPtr->scsiPtr->devPtr = devPtr;
    tapeControlBlockPtr =
	    (DevSCSITapeControlBlock *)&devPtr->scsiPtr->controlBlock;
    Byte_Zero(sizeof(DevSCSITapeControlBlock), (Address)tapeControlBlockPtr);
    /*
     * Need to mess with the code here for drive specific wierdness.
     * There is a more compact version of this code in the boot
     * source for devSCSI.c - One switch on command with a few
     * special checks against the controller type.
     */
    tapePtr = (DevSCSITape *)devPtr->data;
    switch (tapePtr->type) {
	default:
	case SCSI_SYSGEN: {
	    switch (command) {
		case SCSI_TEST_UNIT_READY:
		    break;
		case SCSI_REWIND:
		    /*
		     * Can also do a tape retension by setting vendor57 bit.
		     */
		    if (tapePtr->state & SCSI_TAPE_RETENSION) {
			tapePtr->state &= ~SCSI_TAPE_RETENSION;
			tapeControlBlockPtr->vendor57 = 1;
		    }
		    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_REQUEST_SENSE:
		    dmaCount = count = sizeof(DevSCSISense);
		    break;
		case SCSI_READ:
		case SCSI_WRITE:
		    dmaCount = count * DEV_BYTES_PER_SECTOR;
		    break;
		case SCSI_WRITE_EOF:
		    dmaCount = 0;
		    count = 1;
		    break;
		case SCSI_SPACE:
		case SCSI_SPACE_FILES:
		    dmaCount = 0;
		    code = 1;
		    command = SCSI_SPACE;
		    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_SPACE_BLOCKS:
		    dmaCount = 0;
		    code = 0;
		    command = SCSI_SPACE;
		    break;
		case SCSI_SPACE_EOT:
		    dmaCount = 0;
		    code = 3;
		    command = SCSI_SPACE;
		    tapePtr->state |= SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_ERASE_TAPE:
		    break;
	    }
	    break;
	}
	case SCSI_EMULUX: {
	    switch (command) {
		case SCSI_TEST_UNIT_READY:
		    break;
		case SCSI_REWIND:
		    /*
		     * Can do tape retension by using SCSI_START_STOP
		     * and setting count to 3 (wild but true)
		     */
		    if (tapePtr->state & SCSI_TAPE_RETENSION) {
			tapePtr->state &= ~SCSI_TAPE_RETENSION;
			command = SCSI_START_STOP;
			dmaCount = 0;
			count = 3;
		    }
		    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_REQUEST_SENSE:
		    dmaCount = count = sizeof(DevEmuluxSense);
		    break;
		case SCSI_MODE_SELECT:
		    break;
		case SCSI_READ:
		case SCSI_WRITE:
		    code = 1;
		    dmaCount = count * DEV_BYTES_PER_SECTOR;
		    break;
		case SCSI_WRITE_EOF:
		    count = 1;
		    dmaCount = 0;
		    break;
		case SCSI_SPACE:
		case SCSI_SPACE_FILES:
		    dmaCount = 0;
		    code = 1;
		    command = SCSI_SPACE;
		    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_SPACE_BLOCKS:
		    dmaCount = 0;
		    code = 0;
		    command = SCSI_SPACE;
		    break;
		case SCSI_SPACE_EOT:
		    dmaCount = 0;
		    code = 3;
		    command = SCSI_SPACE;
		    tapePtr->state |= SCSI_TAPE_AT_EOF;
		    break;
		case SCSI_ERASE_TAPE:
		    code = 1;
		    break;
	    }
	    break;
	}
    }
    tapeControlBlockPtr->command = command & 0xff;
    tapeControlBlockPtr->code = code;
    tapeControlBlockPtr->unitNumber = devPtr->subUnitID;
    tapeControlBlockPtr->highCount = (count & 0x1f0000) >> 16;
    tapeControlBlockPtr->midCount =  (count & 0x00ff00) >> 8;
    tapeControlBlockPtr->lowCount =  (count & 0x0000ff);
    *countPtr = dmaCount;
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSISectorIO --
 *
 *      Lower level routine to read or write an SCSI device.  The
 *      interface here is in terms of a particular SCSI disk and the
 *      number of sectors to transfer.  This routine takes care of mapping
 *      its buffer into the special multibus memory area that is set up
 *      for Sun DMA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSISectorIO(command, devPtr, firstSector, numSectorsPtr, buffer)
    int command;			/* SCSI_READ or SCSI_WRITE */
    DevSCSIDevice *devPtr;		/* Which disk to do I/O with */
    int firstSector;			/* The sector at which the transfer
					 * begins. */
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors transferred. */
    char *buffer;			/* Target buffer */
{
    ReturnStatus status;
    register DevSCSIController *scsiPtr; /* Controller for the disk */

    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     */
    scsiPtr = devPtr->scsiPtr;
    MASTER_LOCK(scsiPtr->mutex);

    /*
     * Here we are using a condition variable and the scheduler to
     * synchronize access to the controller.  An alternative would be
     * to have a command queue associated with the controller.  We can't
     * rely on the mutex variable because that is relinquished later
     * when the process using the controller waits for the I/O to complete.
     */
    while (scsiPtr->flags & SCSI_CNTRLR_BUSY) {
	Sync_MasterWait(&scsiPtr->readyForIO, &scsiPtr->mutex, FALSE);
    }
    scsiPtr->flags |= SCSI_CNTRLR_BUSY;
    scsiPtr->flags &= ~SCSI_IO_COMPLETE;

    /*
     * Map the buffer into the special area of multibus memory that
     * the device can DMA into.
     */
    buffer = VmMach_DevBufferMap(*numSectorsPtr * devPtr->sectorSize,
			     buffer, scsiPtr->IOBuffer);
    DevSCSISetupCommand(command, devPtr, firstSector, *numSectorsPtr);
    status = DevSCSICommand(devPtr->slaveID, scsiPtr,
			     *numSectorsPtr * devPtr->sectorSize,
			     buffer, INTERRUPT);
    /*
     * Wait for the command to complete.  The interrupt handler checks
     * for I/O errors, computes the residual, and notifies us.
     */
    if (status == SUCCESS) {
	while((scsiPtr->flags & SCSI_IO_COMPLETE) == 0) {
	    Sync_MasterWait(&scsiPtr->IOComplete, &scsiPtr->mutex, FALSE);
	}
	status = scsiPtr->status;
    }
    *numSectorsPtr -= (scsiPtr->residual / devPtr->sectorSize);
    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(scsiPtr->mutex);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeIO --
 *
 *      Low level routine to read or write an SCSI tape device.  The
 *      interface here is in terms of a particular SCSI tape and the
 *      number of sectors to transfer.  This routine takes care of mapping
 *      its buffer into the special multibus memory area that is set up
 *      for Sun DMA.  Each IO involves one tape block, and all tape blocks
 *	are multiples of the underlying device block size (DEV_BYTES_PER_SECTOR)
 *
 *	This should be combined with DevSCSISectorIO as it is so similar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeIO(command, devPtr, buffer, countPtr)
    int command;			/* SCSI_READ, SCSI_WRITE, etc. */
    register DevSCSIDevice *devPtr; 	/* State info for the tape */
    char *buffer;			/* Target buffer */
    int *countPtr;			/* Upon entry, the number of sectors to
					 * transfer, or general count for
					 * skipping blocks, etc. Upon return,
					 * the number of sectors transferred. */
{
    ReturnStatus status;
    register DevSCSIController *scsiPtr; /* Controller for the drive */

    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     */
    scsiPtr = devPtr->scsiPtr;
    MASTER_LOCK(scsiPtr->mutex);

    /*
     * Here we are using a condition variable and the scheduler to
     * synchronize access to the controller.  An alternative would be
     * to have a command queue associated with the controller.  We can't
     * rely on the mutex variable because that is relinquished later
     * when the process using the controller waits for the I/O to complete.
     */
    while (scsiPtr->flags & SCSI_CNTRLR_BUSY) {
	Sync_MasterWait(&scsiPtr->readyForIO, &scsiPtr->mutex, FALSE);
    }
    scsiPtr->flags |= SCSI_CNTRLR_BUSY;
    scsiPtr->flags &= ~SCSI_IO_COMPLETE;

    if (command == SCSI_READ || command == SCSI_WRITE) {
	/*
	 * Map the buffer into the special area of multibus memory that
	 * the device can DMA into.  Probably have to worry about
	 * the buffer size.
	 */
	buffer = VmMach_DevBufferMap(*countPtr * DEV_BYTES_PER_SECTOR,
				 buffer, scsiPtr->IOBuffer);
    }
    DevSCSISetupTapeCommand(command, devPtr, countPtr);
    status = DevSCSICommand(devPtr->slaveID, scsiPtr, *countPtr, buffer,
			    INTERRUPT);
    /*
     * Wait for the command to complete.  The interrupt handler checks
     * for I/O errors, computes the residual, and notifies us.
     */
    if (status == SUCCESS) {
	while((scsiPtr->flags & SCSI_IO_COMPLETE) == 0) {
	    Sync_MasterWait(&scsiPtr->IOComplete, &scsiPtr->mutex, FALSE);
	}
	status = scsiPtr->status;
    }
    if (scsiPtr->residual) {
	Sys_Panic(SYS_WARNING, "SCSI residual %d, cmd %x\n", scsiPtr->residual,
			    command);
    }
    *countPtr -= scsiPtr->residual;
    if (command == SCSI_READ || command == SCSI_WRITE) {
	*countPtr /= DEV_BYTES_PER_SECTOR;
    }
    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(scsiPtr->mutex);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSICommand --
 *
 *      Send a command to a controller specified by targetID on the SCSI
 *      bus controlled through scsiPtr.  The control block needs to have
 *      been set up previously with DevSCSISetupCommand.  If the interrupt
 *      argument is WAIT (FALSE) then this waits around for the command to
 *      complete and checks the status results.  Otherwise Dev_SCSIIntr
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
DevSCSICommand(targetID, scsiPtr, size, addr, interrupt)
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
    Boolean checkMsg = FALSE;		/* have DevSCSIWait check for
					   a premature message */
#undef WORM_DEBUG
#ifdef WORM_DEBUG
    Boolean oldDebug;
#endif WORM_DEBUG

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
	    DELAY(10);
	}
    }
    if (i == SCSI_WAIT_LENGTH) {
	Sys_Printf("SCSI bus stuck busy\n");
	DevSCSIReset(regsPtr);
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
    status = DevSCSIWait(regsPtr, SCSI_BUSY, NO_RESET, FALSE);
    if (status != SUCCESS) {
	regsPtr->data = 0;
	regsPtr->control = 0;
	if (scsiPtr->controlBlock.command != SCSI_TEST_UNIT_READY) {
	    Sys_Printf("SCSI-%d: can't select slave %d\n", 
				 scsiPtr->number, targetID);
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
    regsPtr->dmaAddress = (int)(addr - MULTIBUS_BASE);
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
#ifdef WORM_DEBUG
    oldDebug = devSCSIDebug;
    if (scsiPtr->devPtr->type == SCSI_WORM) {
	devSCSIDebug = TRUE;
    }
#endif WORM_DEBUG
    if (scsiPtr->devPtr->type == SCSI_WORM) {
	checkMsg = TRUE;
    }
    for (i=0 ; i<sizeof(DevSCSIControlBlock) ; i++) {
	status = DevSCSIWait(regsPtr, SCSI_REQUEST, RESET, checkMsg);
/*
 * This is just a guess.
 */
	if (status == DEV_EARLY_CMD_COMPLETION) {
#ifdef WORM_DEBUG
	    devSCSIDebug = oldDebug;
#endif WORM_DEBUG
	    return(SUCCESS);
	}
	if (status != SUCCESS) {
	    Sys_Printf("SCSI-%d: couldn't send command block (i=%d)\n",
				 scsiPtr->number, i);
#ifdef WORM_DEBUG
	    devSCSIDebug = oldDebug;
#endif WORM_DEBUG
	    return(status);
	}
	/*
	 * The device keeps the Control/Data line set while it
	 * is accepting control block bytes.
	 */
	if ((regsPtr->control & SCSI_COMMAND) == 0) {
	    DevSCSIReset(regsPtr);
	    Sys_Printf("SCSI-%d: device dropped command line\n",
				 scsiPtr->number);
#ifdef WORM_DEBUG
	    devSCSIDebug = oldDebug;
#endif WORM_DEBUG
	    return(DEV_HANDSHAKE_ERROR);
	}
	regsPtr->commandStatus = *charPtr;
	charPtr++;
    }
#ifdef WORM_DEBUG
    devSCSIDebug = oldDebug;
#endif WORM_DEBUG
    if (interrupt == WAIT) {
	/*
	 * A synchronous command.  Wait here for the command to complete.
	 */
	status = DevSCSIWait(regsPtr, SCSI_INTERRUPT_REQUEST, RESET, FALSE);
	if (status == SUCCESS) {
	    scsiPtr->residual = -regsPtr->dmaCount -1;
	    status = DevSCSIStatus(scsiPtr);
	} else {
	    Sys_Printf("SCSI-%d: couldn't wait for command to complete\n",
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
 * DevSCSIStatus --
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
DevSCSIStatus(scsiPtr)
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
    Byte_Zero(sizeof(DevSCSIStatusBlock), (Address)statusBytePtr);
    for ( ; ; ) {
	/*
	 * Could probably wait either on the INTERUPT_REQUEST bit or the
	 * REQUEST bit.  Reading the byte out of the commandStatus
	 * register acknowledges the REQUEST and clears these bits.  Here
	 * we grab bytes until the MESSAGE bit indicates that all the
	 * status bytes have been received and that the byte in the
	 * commandStatus register is the message byte.
	 */
	status = DevSCSIWait(regsPtr, SCSI_REQUEST, RESET, FALSE);
	if (status != SUCCESS) {
	    Sys_Printf("SCSI-%d: wait error after %d status bytes\n",
				 scsiPtr->number, numStatusBytes);
	    break;
	}
	if (regsPtr->control & SCSI_MESSAGE) {
	    message = regsPtr->commandStatus & 0xff;
	    if (message != SCSI_COMMAND_COMPLETE) {
		Sys_Printf("SCSI-%d: Unexpected message 0x%x\n",
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
	    Sys_Printf("SCSI-%d: host adaptor error bit set\n",
				 scsiPtr->number);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIRequestSense --
 *
 *	Do a request-sense command to obtain the sense data that an
 *	SCSI device returns after some error conditions.  Unfortunately,
 *	the format of the sense data varies with different controllers.
 *	The "sysgen" drive on 2/120's has a format described by
 *	the DevSCSITapeSense type, while the shoebox drives use the
 *	more standard error "class 7" format.
 *
 * Results:
 *	SUCCESS if the sense data is benign.
 *
 * Side effects:
 *	Does an SCSI_REQUEST_SENSE command.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIRequestSense(scsiPtr, devPtr)
    DevSCSIController *scsiPtr;	/* Controller state */
    DevSCSIDevice *devPtr;	/* Device state needed for error checking */
{
    ReturnStatus status = SUCCESS;
    register DevSCSISense *sensePtr = scsiPtr->senseBuffer;
    int command;

    if (scsiPtr->flags & SCSI_GETTING_STATUS) {
	Sys_Panic(SYS_WARNING, "DevSCSIRequestSense recursed");
    } else {
	/*
	 * The regular SetupCommand procedure is used, although the
	 * "numSectors" parameter needs to be a byte count...
	 */
	scsiPtr->flags |= SCSI_GETTING_STATUS;
	command = scsiPtr->command;
	DevSCSISetupCommand(SCSI_REQUEST_SENSE, devPtr, 0,
				sizeof(DevSCSISense));
	status = DevSCSICommand(devPtr->slaveID, scsiPtr, sizeof(DevSCSISense),
				(Address)sensePtr, WAIT);
	scsiPtr->command = command;
	scsiPtr->flags &= ~SCSI_GETTING_STATUS;
	if (devPtr->type == SCSI_TAPE &&
	    ((DevSCSITape *)devPtr->data)->type == SCSI_UNKNOWN) {
	    /*
	     * Heuristically determine the drive type...  We ask for the max
	     * possible sense bytes, and this gets returned by the sysgen
	     * controller.  The emulux controller returns less.  We might
	     * also be able to depend on the class7 sense class below...
	     */
	    Sys_Printf("RequestSense, residual is %d\n", scsiPtr->residual);
	    if (scsiPtr->residual == 0) {
		((DevSCSITape *)devPtr->data)->type = SCSI_SYSGEN;
	    } else {
		((DevSCSITape *)devPtr->data)->type = SCSI_EMULUX;
	    }
	}
	status = (*devPtr->errorProc)(devPtr, sensePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIWait --
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
DevSCSIWait(regsPtr, condition, reset, checkMsg)
    DevSCSIRegs *regsPtr;
    int condition;
    Boolean reset;
    Boolean checkMsg;
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register int control;

    if (devSCSIDebug && checkMsg) {
	Sys_Printf("DevSCSIWait: checking for message.\n");
    }
    for (i=0 ; i<SCSI_WAIT_LENGTH ; i++) {
	control = regsPtr->control;
        /*
	 * For debugging of WORM: 
	 *  .. using printf because using kdbx causes different behavior.
	 */
	if (devSCSIDebug && i < 5) {
	    Sys_Printf("%d/%x ", i, control);
	}
/* this is just a guess too. */
	if (checkMsg) {
	    register int mask = SCSI_REQUEST | SCSI_INPUT | SCSI_MESSAGE | SCSI_COMMAND;
	    if ((control & mask) == mask) {
		register int msg;
	    
		msg = regsPtr->commandStatus & 0xff;
		Sys_Printf("DevSCSIWait: Unexpected message 0x%x\n", msg);
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
	    Sys_Printf("SCSI: bus error\n");
	    status = DEV_DMA_FAULT;
	    break;
	} else if (control & SCSI_PARITY_ERROR) {
	    Sys_Printf("SCSI: parity error\n");
	    status = DEV_DMA_FAULT;
	    break;
	}
	DELAY(10);
    }
    if (devSCSIDebug) {
	Sys_Printf("DevSCSIWait: timed out, control = %x.\n", control);
    }
    if (reset) {
	DevSCSIReset(regsPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIIntr --
 *
 *	Handle interrupts from the SCSI controller.  This has to poll
 *	through the possible SCSI controllers to find the one generating
 *	the interrupt.  The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
 *
 * Results:
 *	TRUE if an SCSI controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIIntr()
{
    int index;
    register DevSCSIController *scsiPtr;
    register DevSCSIRegs *regsPtr;

    for (index = 0; index < SCSI_MAX_CONTROLLERS ; index++) {
	scsiPtr = scsi[index];
	if (scsiPtr == (DevSCSIController *)NIL) {
	    continue;
	}
	regsPtr = scsiPtr->regsPtr;
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
			*(char *)(MULTIBUS_BASE + regsPtr->dmaAddress) =
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
	}
    }
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSIWormIO --
 *
 *      Read or Write (to/from) a raw SCSI worm disk.  This just
 *	maps multiple-sector operations into separate IO's.  They may
 *	be combined for efficiency at a later time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIWormIO(command, deviceUnit, buffer, firstSector, numSectorsPtr)
    int command;			/* SCSI_READ or SCSI_WRITE */
    int deviceUnit;			/* Unit from the filesystem that
					 * indicates a disk */
    char *buffer;			/* Target buffer */
    int firstSector;			/* First sector to transfer. Should be
					 * relative to the start of the disk
					 * partition corresponding to the unit*/
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors actually transferred. */
{
    ReturnStatus status;
    int worm;		/* Number of worm that corresponds to the unit number */
    DevSCSIDevice *devPtr;		/* Generic SCSI device state */
    register DevSCSIWorm *wormPtr;	/* State of the worm */
    int totalSectors;		/* The total number of sectors to transfer */
    int numSectors;		/* The number of sectors to transfer at
				 * one time, up to a blocks worth. */
    int lastSector;	/* Last sector of the partition */
    int totalXfer;	/* The total number of sectors actually transferred */

    worm = deviceUnit;
    devPtr = scsiWorm[worm];
    wormPtr = (DevSCSIWorm *)devPtr->data;
    totalSectors = *numSectorsPtr;

    /*
     * Chop up the IO into pieces of the maximum transfer size.
     * 
     *
     * Flag the stored sense as invalid just before sending the command to
     * the drive.
     */
    totalXfer = 0;
    do {
	if (totalSectors > MAX_WORM_SECTORS_IO) {
	    numSectors = MAX_WORM_SECTORS_IO;
	} else {
	    numSectors = totalSectors;
	}
	wormPtr->state &= ~SCSI_WORM_VALID_SENSE;
	status = DevSCSISectorIO(command, devPtr, firstSector, &numSectors, buffer);
	if (status == SUCCESS) {
	    firstSector += numSectors;
	    totalSectors -= numSectors;
	    buffer += numSectors * DEV_BYTES_PER_WORM_SECTOR;
	    totalXfer += numSectors;
	}
    } while (status == SUCCESS && totalSectors > 0);
    *numSectorsPtr = totalXfer;
    return(status);
}
