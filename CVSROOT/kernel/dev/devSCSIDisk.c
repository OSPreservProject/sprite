/* 
 * devSCSIDisk.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      are defined here for the SCSI disk.  This ``raw'' access to the
 *      disk is used during formatting and disk recovery.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSBC.h"		/* FIXME: need to merge this */
#include "devSCSIDisk.h"

#include "dbg.h"
int SCSIdebug = FALSE;

#define SECTORS_PER_FRAGMENT	(FS_FRAGMENT_SIZE / DEV_BYTES_PER_SECTOR)

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskOpen --
 *
 *	Open a partition of an SCSI disk as a file.  This merely
 *	checks to see if the disk is on-line before succeeding.
 *
 * Results:
 *	SUCCESS if the disk is on-line.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_SCSIDiskOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData token;		/* Call-back token for input, unused here */
{
    ReturnStatus error;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    DevSCSIDevice *diskPtr;	/* State of the disk */

    disk = devicePtr->unit / DEV_NUM_DISK_PARTS;
    if (disk >= SCSI_MAX_DISKS) {
	return(DEV_INVALID_UNIT);
    }
    diskPtr = scsiDisk[disk];
    if ((diskPtr == (DevSCSIDevice *)NIL) || (diskPtr == (DevSCSIDevice *)0)) {
	return(DEV_INVALID_UNIT);
    }

    error = DevSCSITest(diskPtr);
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskRead --
 *
 *	Read from a raw SCSI Disk.  This understands about sectors and
 *	may break the read up to account for user offsets into the disk
 *	file that don't correspond to the start of a sector.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process will sleep waiting for the I/O to complete.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_SCSIDiskRead(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle for raw SCSI disk device */
    int offset;			/* Indicates starting point for read.  This
				 * is rounded down to the start of a sector */
    int bufSize;		/* Number of bytes to read,  rounded down
				 * do a multiple of the sector size */
    char *buffer;		/* Buffer for the read */
    int *lenPtr;		/* How many bytes actually read */
{
    ReturnStatus error;	/* Error code */
    int firstSector;	/* The first sector to read */
    int numSectors;	/* The number of sectors to read */

    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to read is also rounded down to a whole number of
     * sectors.  This should break misaligned reads up so the first and
     * last sectors are read into an extra buffer and copied into the
     * user's buffer.
     */
    if (SCSIdebug) {
	DBG_CALL;
    }
    offset &= ~(DEV_BYTES_PER_SECTOR-1);
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    firstSector = offset / DEV_BYTES_PER_SECTOR;
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSIDiskIO(SCSI_READ, devicePtr->unit, buffer,
			firstSector, &numSectors);
    *lenPtr = numSectors * DEV_BYTES_PER_SECTOR;
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskWrite --
 *
 *	Write to a raw SCSI disk.
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
Dev_SCSIDiskWrite(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle of raw disk device */
    int offset;			/* Indicates the starting point of the write.
				 * Rounded down to the start of a sector */
    int bufSize;		/* Number of bytes to write.  Rounded down
				 * to a multiple of the sector size */
    char *buffer;		/* Write buffer */
    int *lenPtr;		/* How much was actually written */
{
    ReturnStatus error;
    int firstSector;
    int numSectors;

    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to write is also rounded down to a whole number of
     * sectors.  For misaligned writes we need to first read in the sector
     * and then overwrite part of it.
     */
    if (SCSIdebug) {
	DBG_CALL;
    }
    offset &= ~(DEV_BYTES_PER_SECTOR-1);
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    firstSector = offset / DEV_BYTES_PER_SECTOR;
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSIDiskIO(SCSI_WRITE, devicePtr->unit, buffer,
			firstSector, &numSectors);
    *lenPtr = numSectors * DEV_BYTES_PER_SECTOR;

    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskIOControl --
 *
 *	Do a special operation on a raw SCSI Disk.
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
ReturnStatus
Dev_SCSIDiskIOControl(devicePtr, command, inBufSize, inBuffer,
				 outBufSize, outBuffer)
    Fs_Device *devicePtr;
    int command;
    int inBufSize;
    char *inBuffer;
    int outBufSize;
    char *outBuffer;
{
#ifdef not_used_yet
    ReturnStatus error;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    int part;		/* Partition of disk that corresponds to unit number */
    DevSCSIDisk *diskPtr;	/* State of the disk */
#endif
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
 * Dev_SCSIDiskClose --
 *
 *	Close a raw SCSI disk file.
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
ReturnStatus
Dev_SCSIDiskClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int 	useFlags;
    int		openCount;
    int		writerCount;
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDiskError --
 *
 *	Handle errors indicated by the sense data returned from the disk.
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
DevSCSIDiskError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status = SUCCESS;
    DevSCSIDisk *diskPtr = (DevSCSIDisk *)devPtr->data;
    int command = devPtr->scsiPtr->command;

    switch (diskPtr->type) {
	/*
	 * The shoebox apparently just returns the unextended sense
	 * format.
	 */
	case SCSI_SHOEBOX_DISK: {
	    if (sensePtr->error != SCSI_NO_SENSE_DATA) {	    
		register int class = (sensePtr->error & 0x70) >> 4;
		register int code = sensePtr->error & 0xF;
		register int addr;
		addr = (sensePtr->highAddr << 16) |
			(sensePtr->midAddr << 8) |
			sensePtr->lowAddr;
		Sys_Printf("SCSI-%d: Sense error (%d-%d) at <%x> ",
				 devPtr->scsiPtr->number, class, code, addr);
		if (scsiNumErrors[class] > code) {
		    Sys_Printf("%s", scsiErrors[class][code]);
		}
		Sys_Printf("\n");
		status = DEV_HARD_ERROR;
	    }
	    break;
	}
	/*
	 * The SCSIBOX Emulex drives will transfer in extended sense format.
	 * Note: "gs/emulux/emulex/" will follow.
	 *
	 * Unit attention, at least in one case, is potentially ignorable.
	 * Also, the Emulex SCSIBOX drive doesn't set an extra error code,
	 * it just sets the key to SCSI_UNIT_ATTN_KEY.
	 *
	 * FIXME: the relationship between SCSI and SBC is pretty confused.
	 */
	case SCSI_EMULEX_DISK: {
	    register DevSBCSense *sbcSensePtr;
	    register DevSCSIExtendedSense *extSensePtr;
	    extern Boolean devSBCDebug;
	    DevSBCDevice *sbcDevPtr;
	    DevSBCController *sbcPtr;

	    sbcSensePtr = (DevSBCSense *)sensePtr;
	    extSensePtr = (DevSCSIExtendedSense *)sensePtr;
	    sbcDevPtr = (DevSBCDevice *) devPtr;
	    sbcPtr = sbcDevPtr->sbcPtr;
	    
	    switch (extSensePtr->key) {
		case SCSI_NO_SENSE:
		    if (devSBCDebug) {
			Sys_Panic(SYS_WARNING,
				  "SCSI-%d drive %d, no sense?\n",
				  devPtr->scsiPtr->number, devPtr->slaveID);
		    }
		    break;
		case SCSI_RECOVERABLE:
		    /*
		     * The drive recovered from an error.
		     */
		    if (devSBCDebug > 2) {
			Sys_Panic(SYS_WARNING,
				  "SCSI-%d drive %d, recoverable error, code %x\n",
				  devPtr->scsiPtr->number, devPtr->slaveID,
				  sbcSensePtr->code2);
			Sys_Printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
				   extSensePtr->info1 & 0xff,
				   extSensePtr->info2 & 0xff,
				   extSensePtr->info3 & 0xff,
				   extSensePtr->info4 & 0xff);
		    }
		    sbcPtr->stats.numRecoverableErrors++;
		    status = DEV_RETRY_ERROR;
		    break;
		case SCSI_NOT_READY_KEY:
		    status = DEV_OFFLINE;
		    break;
		case SCSI_ILLEGAL_REQUEST:
		    /*
		     * Probably a programming error.
		     */
		    Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, illegal request %d\n",
				devPtr->scsiPtr->number, devPtr->slaveID,
				command);
		    status = DEV_INVALID_ARG;
		    break;
		case SCSI_MEDIA_ERROR:
		case SCSI_HARDWARE_ERROR:
		    Sys_Panic((devSBCDebug > 2) ? SYS_FATAL : SYS_WARNING,
			      "SCSI-%d drive %d, hard class7 error %x code %x\n",
			      devPtr->scsiPtr->number, devPtr->slaveID,
			      extSensePtr->key, sbcSensePtr->code2);
		    Sys_Printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
			extSensePtr->info1 & 0xff,
			extSensePtr->info2 & 0xff,
			extSensePtr->info3 & 0xff,
			extSensePtr->info4 & 0xff);
		    status = DEV_HARD_ERROR;
		    sbcPtr->stats.numHardErrors++;
		    break;
		case SCSI_UNIT_ATTN_KEY:
		    /*
		     * This is an error that occurs after the drive is reset.
		     * It can probably be ignored.
		     */
		    Sys_Panic(SYS_WARNING,
			    "SCSI-%d drive %d, unit attention\n",
			    devPtr->scsiPtr->number, devPtr->slaveID);
		    sbcPtr->stats.numUnitAttns++;
		    break;
		case SCSI_WRITE_PROTECT_KEY:
		    if (command == SCSI_WRITE ||
			command == SCSI_WRITE_EOF ||
			command == SCSI_ERASE_TAPE) {
			status = FS_NO_ACCESS;
		    }
		    break;
		case SCSI_BLANK_CHECK:
		    Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, \"blank check\"\n",
			devPtr->scsiPtr->number, devPtr->slaveID);
		    Sys_Printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
			extSensePtr->info1 & 0xff,
			extSensePtr->info2 & 0xff,
			extSensePtr->info3 & 0xff,
			extSensePtr->info4 & 0xff);
		    break;
		case SCSI_VENDOR:
		case SCSI_POWER_UP_FAILURE:
		case SCSI_ABORT:
		case SCSI_EQUAL:
		case SCSI_OVERFLOW:
		    Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, unsupported class7 error %d\n",
			devPtr->scsiPtr->number, devPtr->slaveID,
			extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
		default:
		    Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, can't handle error %d\n",
			devPtr->scsiPtr->number, devPtr->slaveID,
			extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
	    }
#ifdef notdef
	    if (emuluxSensePtr->extSense.key != SCSI_UNIT_ATTN_KEY) {
		switch (emuluxSensePtr->error) {
		    case SCSI_NOT_READY:
			status = DEV_OFFLINE;
			break;
		    case SCSI_INSUF_CAPACITY:
			Sys_Panic(SYS_WARNING,
				    "Emulex: Insufficient disk capacity");
			/* fall thru */
		    case SCSI_END_OF_MEDIA:
			status = DEV_END_OF_TAPE;  /* ??? */
			break;
		    case SCSI_HARD_DATA_ERROR:
			status = DEV_HARD_ERROR;
			break;
		    case SCSI_WRITE_PROTECT:
			if (command == SCSI_WRITE) {
			    status = FS_NO_ACCESS;
			}
			break;
		    case SCSI_CORRECTABLE_ERROR:
			Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, correctable error",
				devPtr->scsiPtr->number, devPtr->slaveID);
			break;
		    case SCSI_INVALID_COMMAND:
			Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, invalid command 0x%x",
				devPtr->scsiPtr->number, devPtr->slaveID,
				command);
			break;

		    default:
			Sys_Panic(SYS_FATAL,
				"SCSI-%d drive %d, unknown error %x\n",
				  devPtr->scsiPtr->number, devPtr->slaveID,
				  emuluxSensePtr->error);
			status = DEV_NO_MEDIA;
			break;
		}
	    } else {
	      /*
	       * The drive has been reset sinse the last command.
	       * Looks like we get this at startup.
	       */
		Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, unit attention\n",
			devPtr->scsiPtr->number, devPtr->slaveID);
	    }
#endif notdef
	    break;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskBlockIOInit --
 *
 *	Initialization routine for the Block I/O interface to the SCSI disk.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Saves a pointer to the geometry information for the ClientData
 *	field of the Fs_Device object passed in.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_SCSIDiskBlockIOInit(devicePtr)
    Fs_Device *devicePtr;	/* Use the unit number to specify partition */
{
    return(SUCCESS);		/* NOT CALLED */
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskBlockIO --
 *
 *	Do block I/O on the SCSI disk.  This uses the disk geometry
 *	information to map from filesystem fragments indexes to filesystem
 *	block indexes, and finally to disk sectors.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	The disk write, if readWrite == FS_WRITE.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Dev_SCSIDiskBlockIO(readWrite, devicePtr, fragNumber, numFrags, buffer)
    int readWrite;		/* FS_READ or FS_WRITE */
    Fs_Device *devicePtr;	/* Use the unit number to specify partition */
    int fragNumber;		/* Index of first fragment to transfer*/
    int numFrags;		/* Number of fragments to transfer */
    Address buffer;		/* I/O buffer */
{
    ReturnStatus status;	/* General return code */
    int firstSector;		/* Starting sector of transfer */
    int numSectors;		/* Number of sectors to transfer */
    int scsiCommand;		/* SCSI_READ or SCSI_WRITE */

    scsiCommand = (readWrite == FS_READ) ? SCSI_READ : SCSI_WRITE;
    if ((fragNumber % FS_FRAGMENTS_PER_BLOCK) != 0) {
	/*
	 * The I/O doesn't start on a block boundary.  Transfer the
	 * first few extra fragments to get things going on a block boundary.
	 */
	register int extraFrags;

	extraFrags = FS_FRAGMENTS_PER_BLOCK -
		    (fragNumber % FS_FRAGMENTS_PER_BLOCK);
	if (extraFrags > numFrags) {
	    extraFrags = numFrags;
	}
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = extraFrags * SECTORS_PER_FRAGMENT;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
	extraFrags = numSectors / SECTORS_PER_FRAGMENT;
	fragNumber += extraFrags;
	buffer += extraFrags * FS_FRAGMENT_SIZE;
	numFrags -= extraFrags;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    while (numFrags >= FS_FRAGMENTS_PER_BLOCK) {
	/*
	 * Transfer whole blocks.
	 */
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = SECTORS_PER_FRAGMENT * FS_FRAGMENTS_PER_BLOCK;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
	fragNumber += FS_FRAGMENTS_PER_BLOCK;
	buffer += FS_BLOCK_SIZE;
	numFrags -= FS_FRAGMENTS_PER_BLOCK;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (numFrags > 0) {
	/*
	 * Transfer the left over fragments.
	 */
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = numFrags * SECTORS_PER_FRAGMENT;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
    }
    return(status);
}

