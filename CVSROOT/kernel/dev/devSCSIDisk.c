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

	return (DEV_INVALID_ARG);
    } else {
	return(SUCCESS);
    }
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

