/* 
 * devXylogics.c --
 *
 *	Driver the for Xylogics 450 controller.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "devXylogics.h"
#include "devMultibus.h"
#include "devDiskLabel.h"
#include "dbg.h"
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "proc.h"	/* for Mach_SetJump */
#include "fs.h"
#include "mem.h"
#include "user/byte.h"
#include "vmMach.h"

/*
 * State for each Xylogics controller.
 */
static DevXylogicsController *xylogics[XYLOGICS_MAX_CONTROLLERS];

/*
 * State for each disk.  The state for all the disks are kept
 * together so that the driver can easily find the disk and partition
 * that correspond to a filesystem unitNumber.
 */

DevXylogicsDisk *xyDisk[XYLOGICS_MAX_DISKS];
static int xyDiskIndex = -1;
static Boolean xyDiskInit = FALSE;

/*
 * SetJump stuff needed when probing for the existence of a device.
 */
static Mach_SetJumpState setJumpState;

/*
 * DevXyCommand() takes a Boolean that indicates whether it should cause
 * an interupt when the command is complete or whether it should busy wait
 * until the command finishes.  These defines make the calls clearer.
 */
#define INTERRUPT	TRUE
#define WAIT		FALSE

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
 * Enable chaining of control blocks (IOPB) by setting this boolean.
 */
Boolean xyDoChaining = FALSE;

/*
 * Forward declarations.
 */

void		DevXylogicsReset();
ReturnStatus	DevXylogicsTest();
ReturnStatus	DevXylogicsDoLabel();
void		DevXylogicsSetupIOPB();
ReturnStatus	DevXylogicsCommand();
ReturnStatus	DevXylogicsStatus();
ReturnStatus	DevXylogicsWait();


/*
 *----------------------------------------------------------------------
 *
 * Dev_XylogicsInitController --
 *
 *	Initialize a Xylogics controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Map the controller into kernel virtual space.
 *	Allocate buffer space associated with the controller.
 *	Do a hardware reset of the controller.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_XylogicsInitController(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    DevXylogicsController *xyPtr;	/* Xylogics specific state */
    register DevXylogicsRegs *regsPtr;	/* Control registers for Xylogics */
    int x;				/* Used when probing the controller */

    /*
     * Allocate and initialize the controller state info.
     */
    xyPtr = (DevXylogicsController *)Mem_Alloc(sizeof(DevXylogicsController));
    xylogics[cntrlrPtr->controllerID] = xyPtr;
    xyPtr->magic = XY_CNTRLR_STATE_MAGIC;
    xyPtr->number = cntrlrPtr->controllerID;
    xyPtr->regsPtr = (DevXylogicsRegs *)cntrlrPtr->address;

    /*
     * Reset the array of disk information.
     */
    if (!xyDiskInit) {
	for (x = 0 ; x < XYLOGICS_MAX_DISKS ; x++) {
	    xyDisk[x] = (DevXylogicsDisk *)NIL;
	}
	xyDiskInit = TRUE;
    }
    /*
     * Poke at the controller's registers to see if it works
     * or we get a bus error.
     */
    regsPtr = xyPtr->regsPtr;
    if (Mach_SetJump(&setJumpState) == SUCCESS) {
	x = regsPtr->resetUpdate;
	regsPtr->addrLow = 'x';
	if (regsPtr->addrLow != 'x') {
	    Mach_UnsetJump();
	    Mem_Free((Address) xyPtr);
	    xylogics[cntrlrPtr->controllerID] = (DevXylogicsController *)NIL;
	    return(FALSE);
	}
    } else {
	Mach_UnsetJump();
	/*
	 * Got a bus error. Zap the info about the non-existent controller.
	 */
	Mem_Free((Address) xyPtr);
	xylogics[cntrlrPtr->controllerID] = (DevXylogicsController *)NIL;
	return(FALSE);
    }
    Mach_UnsetJump();

    DevXylogicsReset(regsPtr);

    /*
     * Allocate the mapped DMA memory to buffers:  a one sector buffer for
     * the label, and one buffer for reading and writing filesystem
     * blocks.  A physical page is obtained for the IOPB and the label.
     * The label buffer has 8 extra bytes for reading low-level sector info.
     * The general buffer gets mapped just before a read or write.  Note
     * that it has to be twice as large as a filesystem block so that an
     * unaligned block can be mapped into it.
     */
    xyPtr->IOPBPtr = (DevXylogicsIOPB *)VmMach_DevBufferAlloc(&devIOBuffer,
					       sizeof(DevXylogicsIOPB));
    VmMach_GetDevicePage((int)xyPtr->IOPBPtr);

    xyPtr->labelBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
					     DEV_BYTES_PER_SECTOR + 8);
    VmMach_GetDevicePage((int)xyPtr->labelBuffer);

    xyPtr->IOBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
					  2 * FS_BLOCK_SIZE);
    
    /*
     * Initialize synchronization variables and set the controllers
     * state to alive and not busy.
     */
    xyPtr->mutex = 0;
    xyPtr->IOComplete.waiting = 0;
    xyPtr->readyForIO.waiting = 0;
    xyPtr->flags = XYLOGICS_CNTRLR_ALIVE;

    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_XylogicsInitDevice --
 *
 *      Initialize a device hanging off an Xylogics controller.  This
 *      keeps track of how many times it is called so that it can properly
 *      correlate filesystem unit numbers to particular devices.
 *
 * Results:
 *	TRUE if the disk exists and the label is ok.
 *
 * Side effects:
 *	Disks:  The label sector is read and the partitioning of
 *	the disk is set up.  The partitions correspond to device
 *	files of the same type but with different unit number.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_XylogicsInitDevice(devPtr)
    DevConfigDevice *devPtr;	/* Config info about the device */
{
    ReturnStatus error;
    DevXylogicsController *xyPtr;	/* Xylogics specific controller state */
    register DevXylogicsDisk *diskPtr;

    /*
     * Increment this before checking for the controller so the
     * unit numbers match up right.  ie. each controller accounts
     * for DEV_NUM_DISK_PARTS unit numbers, and the unit number is
     * used to index the devDisk array (div DEV_NUM_DISK_PARTS).
     */
    xyDiskIndex++;

    xyPtr = xylogics[devPtr->controllerID];
    if (xyPtr == (DevXylogicsController *)NIL ||
	xyPtr == (DevXylogicsController *)0 ||
	xyPtr->magic != XY_CNTRLR_STATE_MAGIC) {
	xyDisk[xyDiskIndex] = (DevXylogicsDisk *)NIL;
	return(FALSE);
    }

    /*
     * Set up a slot in the disk list. See above about xyDiskIndex.
     */
    if (xyDiskIndex >= XYLOGICS_MAX_DISKS) {
	Sys_Printf("Xylogics: To many disks configured\n");
	return(FALSE);
    }
    diskPtr = (DevXylogicsDisk *) Mem_Alloc(sizeof(DevXylogicsDisk));
    diskPtr->magic = XY_DISK_STATE_MAGIC;
    diskPtr->xyPtr = xyPtr;
    diskPtr->slaveID = devPtr->slaveID;
    diskPtr->xyDriveType = 0;
    /*
     * See if the disk is really there.
     */
    error = DevXylogicsTest(xyPtr, diskPtr);
    if (error != SUCCESS) {
	Mem_Free(diskPtr);
	return(FALSE);
    }
    /*
     * Look at the zero'th sector for disk information.  This also
     * sets the drive type with the controller.
     */
    if (DevXylogicsDoLabel(xyPtr, diskPtr) != SUCCESS) {
	Mem_Free(diskPtr);
	return(FALSE);
    } else {
	xyDisk[xyDiskIndex] = diskPtr;
	return(TRUE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsReset --
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
void
DevXylogicsReset(regsPtr)
    DevXylogicsRegs *regsPtr;
{
    char x;
    x = regsPtr->resetUpdate;
#ifdef lint
    regsPtr->resetUpdate = x;
#endif
    MACH_DELAY(100);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsTest --
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
ReturnStatus
DevXylogicsTest(xyPtr, diskPtr)
    DevXylogicsController *xyPtr;
    DevXylogicsDisk *diskPtr;
{

    (void) DevXylogicsCommand(xyPtr, XY_READ_STATUS, diskPtr,
		   (Dev_DiskAddr *)NIL, 0, (Address)0, WAIT);

#ifdef notdef
    Sys_Printf("DevXylogicsTest:\n");
    Sys_Printf("Xylogics-%d disk %d\n", xyPtr->number, diskPtr->slaveID);
    Sys_Printf("Drive Status Byte %x\n", xyPtr->IOPBPtr->numSectLow);
    Sys_Printf("Drive Type %d: Cyls %d Heads %d Sectors %d\n",
		     diskPtr->xyDriveType,
		     xyPtr->IOPBPtr->cylHigh << 8 | xyPtr->IOPBPtr->cylLow,
		     xyPtr->IOPBPtr->head, xyPtr->IOPBPtr->sector);
    Sys_Printf("Bytes Per Sector %d, Num sectors %d\n",
		    xyPtr->IOPBPtr->dataAddrHigh << 8 |
		    xyPtr->IOPBPtr->dataAddrLow,
		    xyPtr->IOPBPtr->relocLow);
    MACH_DELAY(1000000);
#endif notdef
    /*
     * If all the status bits are low then the drive is ok.
     */
    if (xyPtr->IOPBPtr->numSectLow == 0) {
	return(SUCCESS);
    } else if (xyPtr->IOPBPtr->numSectLow == 0x20) {
	Sys_Panic(SYS_WARNING, "Xylogics-%d disk %d write protected\n",
				xyPtr->number, diskPtr->slaveID);
	return(SUCCESS);
    } else {
	return(DEV_OFFLINE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsDoLabel --
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
ReturnStatus
DevXylogicsDoLabel(xyPtr, diskPtr)
    DevXylogicsController *xyPtr;
    DevXylogicsDisk *diskPtr;
{
    register ReturnStatus error ;
    Sun_DiskLabel *diskLabelPtr;
    Dev_DiskAddr diskAddr;
    int part;

    /*
     * Do a low level read that includes sector header info and ecc codes.
     * This is done so we can learn the drive type needed in other commands.
     */
    diskAddr.head = 0;
    diskAddr.sector = 0;
    diskAddr.cylinder = 0;

    error = DevXylogicsCommand(xyPtr, XY_RAW_READ, diskPtr,
			   &diskAddr, 1, xyPtr->labelBuffer, WAIT);
    if (error != SUCCESS) {
	Sys_Printf("Xylogics-%d: disk%d, couldn't read the label\n",
			     xyPtr->number, diskPtr->slaveID);
	return(error);
    }
    diskPtr->xyDriveType = (xyPtr->labelBuffer[3] & 0xC0) >> 6;
    diskLabelPtr = (Sun_DiskLabel *)(&xyPtr->labelBuffer[4]);
#ifdef notdef
    Sys_Printf("Header Bytes: ");
    for (part=0 ; part<4 ; part++) {
	Sys_Printf("%x ", xyPtr->labelBuffer[part] & 0xff);
    } 
    Sys_Printf("\n");
    Sys_Printf("Label magic <%x>\n", diskLabelPtr->magic);
    Sys_Printf("Drive type byte (%x) => type %x\n",
		      xyPtr->labelBuffer[3] & 0xff, diskPtr->xyDriveType);
    MACH_DELAY(1000000);
#endif notdef
    if (diskLabelPtr->magic == SUN_DISK_MAGIC) {
	Sys_Printf("Xylogics-%d disk%d: %s\n", xyPtr->number, diskPtr->slaveID,
				diskLabelPtr->asciiLabel);
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

	error = DevXylogicsCommand(xyPtr, XY_SET_DRIVE_SIZE, diskPtr,
			       &diskAddr, 0, (Address)0, WAIT);
	if (error != SUCCESS) {
	    Sys_Printf("Xylogics-%d: disk%d, couldn't set drive size\n",
				 xyPtr->number, diskPtr->slaveID);
	    return(error);
	}

	return(SUCCESS);
    } else {
	Sys_Printf("Xylogics-%d Disk %d, Unsupported label, magic = <%x>\n",
			       xyPtr->number, diskPtr->slaveID,
			       diskLabelPtr->magic);
	return(FAILURE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsDiskIO --
 *
 *      Read or Write (to/from) a raw Xylogics disk file.  The deviceUnit
 *      number is mapped to a particular partition on a particular disk.
 *      The starting coordinate, diskAddrPtr, is relative to the
 *      corresponding disk partition.  This routine relocates it using the
 *      disk parition info. The transfer is checked against the partition
 *      size to make sure that the I/O doesn't cross a disk partition.
 *      The number of sectors to read could be more than one block.
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
DevXylogicsDiskIO(command, deviceUnit, buffer, diskAddrPtr, numSectorsPtr)
    int command;			/* XY_READ or XY_WRITE */
    int deviceUnit;			/* Unit from the filesystem that
					 * indicates a disk and partition */
    char *buffer;			/* Target buffer */
    Dev_DiskAddr *diskAddrPtr;		/* Partition relative disk address of
					 * the first sector to transfer */
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors actually transferred. */
{
    ReturnStatus error;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    int part;		/* Partition of disk that corresponds to unit number */
    DevXylogicsDisk *diskPtr;	/* State of the disk */
    int totalSectors;	/* The total number of sectors to transfer */
    int numSectors;	/* The number of sectors to transfer at
			 * one time, up to a blocks worth. */
    int lastSector;	/* Last sector of the partition */
    int totalRead;	/* The total number of sectors actually transferred */
    int startSector;	/* The first sector of the transfer */

    int loopCount;	/* For debugging */
    char *bufferOrig;	/* For debugging */

    bufferOrig = buffer;

    disk = deviceUnit / DEV_NUM_DISK_PARTS;
    part = deviceUnit % DEV_NUM_DISK_PARTS;
    diskPtr = xyDisk[disk];
    if (diskPtr->magic != XY_DISK_STATE_MAGIC) {
	Sys_Panic(SYS_WARNING, "DevXylogicsDiskIO: bad disk state info\n");
    }

    /*
     * Do bounds checking to keep the I/O within the partition.
     * sectorZero is the sector number of the first sector in the partition,
     * lastSector is the sector number of the last sector in the partition.
     * (These sector numbers are relative to the start of the partition.)
     */
    lastSector = diskPtr->map[part].numCylinders *
		 (diskPtr->numHeads * diskPtr->numSectors) - 1;
    totalSectors = *numSectorsPtr;

    startSector = diskAddrPtr->cylinder *
		     (diskPtr->numHeads * diskPtr->numSectors) +
		  diskAddrPtr->head * diskPtr->numSectors +
		  diskAddrPtr->sector;
    if (startSector > lastSector) {
	/*
	 * The offset is past the end of the partition.
	 */
	*numSectorsPtr = 0;
	Sys_Panic(SYS_WARNING, "DevXylogicsDiskIO: Past end of partition %d\n",
				part);
	return(SUCCESS);
    } else if ((startSector + totalSectors - 1) > lastSector) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	totalSectors = lastSector - startSector + 1;
	Sys_Panic(SYS_WARNING, "DevXylogicsDiskIO: Overrun partition %d\n",
				part);
    }
    /*
     * Relocate the disk address to be relative to this partition.
     */
    diskAddrPtr->cylinder += diskPtr->map[part].firstCylinder;
    if (diskAddrPtr->cylinder > diskPtr->numCylinders) {
	Sys_Panic(SYS_FATAL, "Xylogics, bad cylinder # %d\n",
	    diskAddrPtr->cylinder);
    }

    /*
     * Chop up the IO into blocksize pieces.  This is due to a size limit
     * on the pre-allocated disk block buffer. (The filesystem cache doesn't
     * call us for more than one fs block, but the raw device interface might.)
     */
    totalRead = 0;
    loopCount = 0;
    do {
	loopCount++;
	if (loopCount > 1) {
	    Sys_Printf("bufferOrig = %x, buffer = %x\n", bufferOrig, buffer);
	    Sys_Panic(SYS_WARNING, "DevXylogicsDiskIO transferring >1 block");
	}
	if (totalSectors > SECTORS_PER_BLOCK) {
	    numSectors = SECTORS_PER_BLOCK;
	} else {
	    numSectors = totalSectors;
	}
	error = DevXylogicsSectorIO(command, diskPtr, diskAddrPtr,
				&numSectors, buffer);
	if (error == SUCCESS) {
	    if (numSectors != totalSectors && numSectors != SECTORS_PER_BLOCK) {
		Sys_Panic(SYS_FATAL, "SectorIO: numSectors corrupted");
	    }
	    totalRead += numSectors;
	    totalSectors -= numSectors;
	    if (totalSectors > 0) {
		buffer += numSectors * DEV_BYTES_PER_SECTOR;
		diskAddrPtr->sector += numSectors;
		if (diskAddrPtr->sector >= diskPtr->numSectors) {
		    diskAddrPtr->sector -= diskPtr->numSectors;
		    diskAddrPtr->head++;
		    if (diskAddrPtr->head >= diskPtr->numHeads) {
			diskAddrPtr->cylinder++;
		    }
		}
	    }
	}
    } while (error == SUCCESS && totalSectors > 0);
    *numSectorsPtr = totalRead;
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsSectorIO --
 *
 *      Lower level routine to read or write an Xylogics device.  Use this
 *      to transfer a contiguous set of sectors.  This routine is the
 *      point of synchronization over the controller.  The interface here
 *      is in terms of a particular Xylogics disk and the number of
 *      sectors to transfer.  This routine takes care of mapping its
 *      buffer into the special multibus memory area that is set up for
 *      Sun DMA.
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
DevXylogicsSectorIO(command, diskPtr, diskAddrPtr, numSectorsPtr, buffer)
    int command;			/* XY_READ or XY_WRITE */
    DevXylogicsDisk *diskPtr;		/* Which disk to do I/O with */
    Dev_DiskAddr *diskAddrPtr;		/* The disk address at which the
					 * transfer begins. */
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors transferred. */
    char *buffer;			/* Target buffer */
{
    ReturnStatus error;
    register DevXylogicsController *xyPtr; /* Controller for the disk */
    int retries = 0;
#ifdef mouse_trap
    if (command == XY_WRITE) {
	int		i, j;
	register char	*bufPtr;
	register int	*intPtr;
	/*
	 * Check all sectors for sectors full of zeroes.
	 */
	for (i = 0, bufPtr = buffer; i < *numSectorsPtr; i++, bufPtr += 512) {
	    intPtr = (int *)bufPtr;
	    if (*intPtr == 0 && *(intPtr + 127) == 0) {
		Boolean	allZero = TRUE;
		for (j = 0; j < 128; j++, intPtr++) {
		    if (*intPtr != 0) {
			allZero = FALSE;
			break;
		    }
		}
		if (allZero) {
		    Sys_Panic(SYS_FATAL,
				"DevXylogicsSectorIO: Writing all 0's\n");
		}
	    }
	}
    }
#endif mouse_trap
    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     */
    xyPtr = diskPtr->xyPtr;
    MASTER_LOCK(xyPtr->mutex);

    /*
     * Here we are using a condition variable and the scheduler to
     * synchronize access to the controller.  An alternative would be to
     * have a command queue associated with the controller.  Note that We
     * can't rely on the mutex variable because that is relinquished later
     * when the process using the controller waits for the I/O to complete.
     */
    while (xyPtr->flags & XYLOGICS_CNTRLR_BUSY) {
	Sync_MasterWait(&xyPtr->readyForIO, &xyPtr->mutex, FALSE);
    }
    xyPtr->flags |= XYLOGICS_CNTRLR_BUSY;
    xyPtr->flags &= ~XYLOGICS_IO_COMPLETE;

    /*
     * Map the buffer into the special area of multibus memory that
     * the device can DMA into.
     */
    buffer = VmMach_DevBufferMap(*numSectorsPtr * DEV_BYTES_PER_SECTOR,
			     buffer, xyPtr->IOBuffer);
retry:
    error = DevXylogicsCommand(xyPtr, command, diskPtr, diskAddrPtr,
			  *numSectorsPtr, buffer, INTERRUPT);
    /*
     * Wait for the command to complete.  The interrupt handler checks
     * for I/O errors and notifies us.
     */
    if (error == SUCCESS) {
	while((xyPtr->flags & XYLOGICS_IO_COMPLETE) == 0) {
	    Sync_MasterWait(&xyPtr->IOComplete, &xyPtr->mutex, FALSE);
	}
    }
    if (xyPtr->flags & XYLOGICS_RETRY) {
	retries++;
	xyPtr->flags &= ~(XYLOGICS_RETRY|XYLOGICS_IO_COMPLETE);
	if (command == XY_READ || command == XY_WRITE) {
	    Sys_Printf("(%s)", (command == XY_READ) ? "Read" : "Write");
	} else {
	    Sys_Printf("(%d)", command);
	}
	if (retries < 3) {
	    Sys_Printf("\n");
	    goto retry;
	} else {
	    Sys_Panic(SYS_WARNING, "Xylogics retry at <%d,%d,%d> FAILED\n",
		diskAddrPtr->cylinder, diskAddrPtr->head, diskAddrPtr->sector);
	    error = DEV_RETRY_ERROR;
	}
    }
    *numSectorsPtr -= (xyPtr->residual / DEV_BYTES_PER_SECTOR);
    if (xyPtr->flags & XYLOGICS_IO_ERROR) {
	xyPtr->flags &= ~XYLOGICS_IO_ERROR;
	error = DEV_DMA_FAULT;
    }
    xyPtr->flags &= ~XYLOGICS_CNTRLR_BUSY;
    Sync_MasterBroadcast(&xyPtr->readyForIO);
    MASTER_UNLOCK(xyPtr->mutex);
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsSetupIOPB --
 *
 *      Setup a IOPB for a command.  The IOPB can then
 *      be passed to DevXylogicsCommand.  It specifies everything the
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
void
DevXylogicsSetupIOPB(command, diskPtr, diskAddrPtr, numSectors, address,
		     interrupt, IOPBPtr)
    char command;			/* One of the Xylogics commands */
    DevXylogicsDisk *diskPtr;		/* Spefifies unit, drive type, etc */
    register Dev_DiskAddr *diskAddrPtr;	/* Head, sector, cylinder */
    int numSectors;			/* Number of sectors to transfer */
    register Address address;		/* Main memory address of the buffer */
    Boolean interrupt;			/* If TRUE use interupts, else poll */
    register DevXylogicsIOPB *IOPBPtr;	/* I/O Parameter Block  */
{
    Byte_Zero(sizeof(DevXylogicsIOPB), (Address)IOPBPtr);

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

    if ((int)address != 0 && (int)address != NIL) {
	if ((unsigned)address < VMMACH_DMA_START_ADDR) {
	    Sys_Printf("%x: ", address);
	    Sys_Panic(SYS_FATAL, "Xylogics data address not in DMA space\n");
	}
	address = (Address)( (int)address - VMMACH_DMA_START_ADDR );
	IOPBPtr->relocHigh	= ((int)address & 0xff000000) >> 24;
	IOPBPtr->relocLow	= ((int)address & 0x00ff0000) >> 16;
	IOPBPtr->dataAddrHigh	= ((int)address & 0x0000ff00) >> 8;
	IOPBPtr->dataAddrLow	= ((int)address & 0x000000ff);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsCommand --
 *
 *	Set the controller working on a command specified by an IOPB.
 *	This routine will poll for the I/O to finish if the interrupt
 *	argument is FALSE.  Otherwise it will return and the caller
 *	should wait on the controlles IOComplete condition.
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
DevXylogicsCommand(xyPtr, command, diskPtr, diskAddrPtr, numSectors, address,
		     interrupt)
    DevXylogicsController *xyPtr;	/* The Xylogics controller that will be
					 * doing the command. The control block
					 * within this specifies the unit
					 * number and device address of the
					 * transfer */
    char command;		/* One of the standard Xylogics commands */
    DevXylogicsDisk *diskPtr;	/* Unit, type, etc, of disk */
    Dev_DiskAddr *diskAddrPtr;	/* Head, sector, cylinder */
    int numSectors;		/* Number of sectors to transfer */
    Address address;		/* Main memory address of the buffer */
    Boolean interrupt;		/* If TRUE use interupts, else poll */
{
    register ReturnStatus error;
    register DevXylogicsRegs *regsPtr;	/* I/O registers */
    register int IOPBAddr;
    int retries = 0;

    /*
     * Without chaining the controller should always be idle at this
     * point.  (Raw device access can conflict.  IMPLEMENT QUEUEING!)
     */
    regsPtr = xyPtr->regsPtr;
    if (regsPtr->status & XY_GO_BUSY) {
	Sys_Panic(SYS_WARNING, "Xylogics waiting for busy controller\n");
	DevXylogicsWait(regsPtr, XY_GO_BUSY);
    }
    /*
     * Set up the I/O registers for the transfer.  All addresses given to
     * the controller must be relocated to the low megabyte so that the Sun
     * MMU will recognize them and map them back into the high megabyte of
     * the kernel's virtual address space.  (As circular as this sounds,
     * the level of indirection means the system can use any physical page
     * for an I/O buffer.)
     */
    if ((int)xyPtr->IOPBPtr < VMMACH_DMA_START_ADDR) {
	Sys_Printf("%x: ", xyPtr->IOPBPtr);
	Sys_Panic(SYS_WARNING, "Xylogics IOPB not in DMA space\n");
	return(FAILURE);
    }
    IOPBAddr = (int)xyPtr->IOPBPtr - VMMACH_DMA_START_ADDR;
    regsPtr->relocHigh = (IOPBAddr & 0xFF000000) >> 24;
    regsPtr->relocLow  = (IOPBAddr & 0x00FF0000) >> 16;
    regsPtr->addrHigh  = (IOPBAddr & 0x0000FF00) >>  8;
    regsPtr->addrLow   = (IOPBAddr & 0x000000FF);

retry:
    DevXylogicsSetupIOPB(command, diskPtr, diskAddrPtr, numSectors, address,
		     interrupt, xyPtr->IOPBPtr);
#ifdef notdef
    if (xylogicsPrints) {
	Sys_Printf("IOBP bytes\n");
	address = (char *)xyPtr->IOPBPtr;
	for (i=0 ; i<24 ; i++) {
	    Sys_Printf("%x ", address[i] & 0xff);
	}
	Sys_Printf("\n");
    }
#endif notdef

    /*
     * Start up the controller
     */
    regsPtr->status = XY_GO_BUSY;

    if (interrupt == WAIT) {
	/*
	 * A synchronous command.  Wait here for the command to complete.
	 */
	error = DevXylogicsWait(regsPtr, XY_GO_BUSY);
	if (error != SUCCESS) {
	    Sys_Printf("Xylogics-%d: couldn't wait for command to complete\n",
				 xyPtr->number);
	} else {
	    /*
	     * Check for error status from the operation itself.
	     */
	    error = DevXylogicsStatus(xyPtr);
	    if (error == DEV_RETRY_ERROR && retries < 3) {
		retries++;
#ifdef notdef
		Sys_Panic(SYS_WARNING, "Xylogics Retrying...\n");
#endif
		goto retry;
	    }
	}
    } else {
	xyPtr->flags |= XYLOGICS_WANT_INTERRUPT;
	error = SUCCESS;
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXylogicsStatus --
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
ReturnStatus
DevXylogicsStatus(xyPtr)
    DevXylogicsController *xyPtr;
{
    register ReturnStatus error = SUCCESS;
    register DevXylogicsRegs *regsPtr;
    register DevXylogicsIOPB *IOPBPtr;

    regsPtr = xyPtr->regsPtr;
    IOPBPtr = xyPtr->IOPBPtr;
    if ((regsPtr->status & XY_ERROR) || IOPBPtr->error) {
	if (regsPtr->status & XY_DBL_ERROR) {
	    Sys_Printf("Xylogics-%d double error %x\n", xyPtr->number,
				    IOPBPtr->errorCode);
	    error = DEV_HARD_ERROR;
	} else {
	    switch (IOPBPtr->errorCode) {
		case XY_NO_ERROR:
		    error = SUCCESS;
		    break;
		case XY_ERR_BAD_CYLINDER:
		    Sys_Printf("Xylogics bad cylinder # %d\n",
				 IOPBPtr->cylHigh << 8 | IOPBPtr->cylLow);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_BAD_SECTOR:
		    Sys_Printf("Xylogics bad sector # %d\n", IOPBPtr->sector);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_BAD_HEAD:
		    Sys_Printf("Xylogics bad head # %d\n", IOPBPtr->head);
		    error = DEV_HARD_ERROR;
		    break;
		case XY_ERR_ZERO_COUNT:
		    Sys_Printf("Xylogics zero count\n");
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
		    Sys_Panic(SYS_FATAL, "Stupid Xylogics error: 0x%x\n",
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
		    Sys_Panic(SYS_WARNING, "Retryable Xylogics error: 0x%x\n",
				IOPBPtr->errorCode);
		    break;
		case XY_WRITE_PROTECT_ON:
		    Sys_Printf("Xylogics-%d: ", xyPtr->number);
		    Sys_Panic(SYS_WARNING, "Write protected\n");
		    error = DEV_HARD_ERROR;
		    break;
		case XY_SOFT_ECC_RECOVERED:
		    Sys_Printf("Xylogics-%d: ", xyPtr->number);
		    Sys_Panic(SYS_WARNING, "Soft ECC error recovered\n");
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
 * DevXylogicsWait --
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
ReturnStatus
DevXylogicsWait(regsPtr, condition)
    DevXylogicsRegs *regsPtr;
    int condition;
{
    register int i;
    ReturnStatus status = DEV_TIMEOUT;

    for (i=0 ; i<XYLOGICS_WAIT_LENGTH ; i++) {
	if ((regsPtr->status & condition) == 0) {
	    return(SUCCESS);
	} else if (regsPtr->status & XY_ERROR) {
	    /*
	     * Let XylogicsStatus() figure out what happened
	     */
	    return(SUCCESS);
	}
	MACH_DELAY(10);
    }
    Sys_Panic(SYS_WARNING, "Xylogics reset");
    DevXylogicsReset(regsPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_XylogicsIntr --
 *
 *	Handle interrupts from the Xylogics controller.  This has to poll
 *	through the possible Xylogics controllers to find the one generating
 *	the interrupt.  The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
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
Dev_XylogicsIntr()
{
    ReturnStatus error = SUCCESS;
    int index;
    register DevXylogicsController *xyPtr;
    register DevXylogicsRegs *regsPtr;
    register DevXylogicsIOPB *IOPBPtr;

    for (index = 0; index < XYLOGICS_MAX_CONTROLLERS ; index++) {
	xyPtr = xylogics[index];
	if (xyPtr == (DevXylogicsController *)NIL ||
	    xyPtr == (DevXylogicsController *)0 ||
	    xyPtr->magic != XY_CNTRLR_STATE_MAGIC) {
	    continue;
	}
	regsPtr = xyPtr->regsPtr;
	if (regsPtr->status & XY_INTR_PENDING) {
	    if (xyPtr->flags & XYLOGICS_WANT_INTERRUPT) {
		IOPBPtr = xyPtr->IOPBPtr;
		if ((regsPtr->status & XY_ERROR) || IOPBPtr->error) {
		    error = DevXylogicsStatus(xyPtr);
		}
		xyPtr->flags |= XYLOGICS_IO_COMPLETE;
		if (error == DEV_RETRY_ERROR) {
		    xyPtr->flags |= XYLOGICS_RETRY;
		} else if (error != SUCCESS) {
		    xyPtr->flags |= XYLOGICS_IO_ERROR;
		}
		/*
		 * For now there are no occasions where only part
		 * of an I/O can complete.
		 */
		xyPtr->residual = 0;
		/*
		 * Reset the pending interrupt by writing a 1 to the
		 * INTR_PENDING bit of the status register.
		 */
		regsPtr->status = XY_INTR_PENDING;
		xyPtr->flags &= ~XYLOGICS_WANT_INTERRUPT;
		Sync_MasterBroadcast(&xyPtr->IOComplete);
		return(TRUE);
	    } else {
		Sys_Printf("Xylogics spurious interrupt\n");
		regsPtr->status = XY_INTR_PENDING;
		return(TRUE);
	    }
	}
    }
    return(FALSE);
}
