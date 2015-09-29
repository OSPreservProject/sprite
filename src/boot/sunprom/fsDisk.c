/* 
 * fsDisk.c --
 *
 *	Routines related to managing local disks.  Each partition of a local
 *	disk (partitions are defined by a table on the disk header) is
 *	called a ``domain''.  FsAttachDisk attaches a domain into the file
 *	system, and FsDeattachDisk removes it.  A domain is given
 *	a number the first time it is ever attached.  This is recorded on
 *	the disk so it doesn't change between boots.  The domain number is
 *	used to identify disks, and a domain number plus a file number is
 *	used to identify files.  Fsdm_DomainFetch is used to get the state
 *	associated with a disk, and Fsdm_DomainRelease releases the reference
 *	on the state.  FsDetachDisk checks the references on domains in
 *	the normal (non-forced) case so that active disks aren't detached.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/sunprom/RCS/fsDisk.c,v 1.12 90/11/27 11:17:33 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"

#include "fsBoot.h"
#include "devDiskLabel.h"
#include "dev.h"
#include "devFsOpTable.h"
#include "machMon.h"
#include "ofs.h"
/*
 * fsDevice is copied into all Fsio_FileIOHandles.  It is used by the drivers
 * to get to the partition and geometry information for the disk.
 */
Fs_Device fsDevice;

/*
 * fsDomainPtr and fsRootHandlePtr are used by Fs_Open.
 */
static Fsdm_Domain fsDomain;
Fsdm_Domain *fsDomainPtr = &fsDomain;
static Fsio_FileIOHandle fsRootHandle;
Fsio_FileIOHandle *fsRootHandlePtr = &fsRootHandle;

/*
 * Forward declarations.
 */
static int	InstallLocalDomain();
void		AddDomainFlags();
static Boolean	IsSunLabel();
static Boolean	IsSpriteLabel();

/*
 *----------------------------------------------------------------------
 *
 * FsAttachDisk --
 *
 *	Make a particular local disk partition correspond to a prefix.
 *	This makes sure the disk is up, reads the domain header,
 *	and calls the initialization routine for the block I/O module
 *	of the disk's driver.  By the time this is called the device
 *	initialization routines have already been called from Dev_Config
 *	so the device driver knows how the disk is partitioned into
 *	domains.  This routine sees if the domain is formatted correctly,
 *	and if so attaches it to the set of domains.
 *
 * Results:
 *	SUCCESS if the disk was readable and had a good domain header.
 *
 * Side effects:
 *	Sets up the Fsdm_DomainInfo for the domain.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsAttachDisk(fsDevicePtr)
    Fs_Device *fsDevicePtr;		/* Global FS device descriptor */
{
    ReturnStatus status;		/* Error code */
    register Address buffer;		/* Read buffer */
    int headerSector;			/* Starting sector of domain header */
    int numHeaderSectors;		/* Number of sectors in domain header */
    int summarySector;			/* Sector of summary information. */
    Ofs_SummaryInfo *summaryInfoPtr;	/* Pointer to summary info. */
    int amountRead;			/* Returned from read call */
    int devType;			/* Device type index */
    Fs_IOParam	io;			/* I/O Parameter block */
    Fs_IOReply	reply;			/* Results of I/O */
    int flags;

    /*
     * Open the raw disk device so we can grub around in the header info.
     */
    devType = DEV_TYPE_INDEX(fsDevicePtr->type);
    status = (*devFsOpTable[devType].open)(&fsDevice, FS_READ, 0, &flags);
    if (status != SUCCESS) {
	return(status);
    }
    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR);

    /*
     * Read the zero'th sector of the partition.  It has a copy of the
     * zero'th sector of the whole disk which describes how the rest of the
     * domain's zero'th cylinder is layed out.
     */
    io.offset = 0;
    io.length = DEV_BYTES_PER_SECTOR;
    io.buffer = buffer;
    status = (*devFsOpTable[devType].read)(&fsDevice, &io, &reply);
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Check for different disk formats, and figure out how the rest
     * of the zero'th cylinder is layed out.
     */
    if (((Sun_DiskLabel *)buffer)->magic == SUN_DISK_MAGIC) {
	Ofs_DomainHeader	*domainHeaderPtr = (Ofs_DomainHeader *) buffer;
	int			i;
	/*
	 * For Sun formatted disks we put the domain header well past
	 * the disk label and the boot program.
	 */
	numHeaderSectors = OFS_NUM_DOMAIN_SECTORS;
	for (i = 2; i < FSDM_MAX_BOOT_SECTORS + 3; i+= FSDM_BOOT_SECTOR_INC) {
	    io.offset = i * DEV_BYTES_PER_SECTOR;
	    io.length = DEV_BYTES_PER_SECTOR * OFS_NUM_DOMAIN_SECTORS;
	    io.buffer = buffer;
	    status = (*devFsOpTable[devType].read)(&fsDevice, &io, &reply);
	    if (status != SUCCESS) {
		return(status);
	    }
	    if (domainHeaderPtr->magic == OFS_DOMAIN_MAGIC) {
		headerSector = i;
		summarySector = i - 1;
	        break;
	    }
	}
	if (i >= FSDM_MAX_BOOT_SECTORS + 3) {
	    printf("Fsdm_AttachDisk: Can't find domain header.\n");
	    return(FAILURE);
	}
    } else {
	printf("Disk label has bad magic number 0x%x\n", 
	    ((Sun_DiskLabel *)buffer)->magic);
	return FAILURE;
    }
    ((Ofs_DomainHeader *) fsDomainPtr->clientData) = 
	(Ofs_DomainHeader *) buffer;

     /*
     * Set up the ClientData part of *devicePtr to reference the
     * Ofs_Geometry part of the domain header.  This is used by the
     * block I/O routines.
     */
    fsDevicePtr->data = (ClientData)
    &((Ofs_DomainHeader *) fsDomainPtr->clientData)->geometry;

    /*
     * Set up a file handle for the root directory.  What is important
     * is the device info (for Block IO) and the file descriptor itself.
     */
    FsInitFileHandle(fsDomainPtr, FSDM_ROOT_FILE_NUMBER, fsRootHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 * The following routines are used by device drivers to map from block
 * and sector numbers to disk addresses.  There are two sets, one for
 * drivers that use logical sector numbers (i.e. SCSI) and the other
 * for <cyl,head,sector> format disk addresses.
 *----------------------------------------------------------------------
 */

/*
 *----------------------------------------------------------------------
 *
 * Fs_BlocksToSectors --
 *
 *	Convert from block indexes (actually, fragment indexes) to
 *	sectors using the geometry information on the disk.  This
 *	is a utility for block device drivers.
 *
 * Results:
 *	The sector number that corresponds to the fragment index.
 *	The caller has to make sure that its I/O doesn't cross a
 *	filesystem block boundary.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define SECTORS_PER_FRAG	(FS_FRAGMENT_SIZE / DEV_BYTES_PER_SECTOR)
#if defined(SCSI_DISK_BOOT) || defined(SUN_PROM_BOOT)
int
Fs_BlocksToSectors(fragNumber, data)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
{
    register Ofs_Geometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (Ofs_Geometry *)data;
    blockNumber		= fragNumber / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    if (geoPtr->rotSetsPerCyl > 0) {
	/*
	 * Do fancy rotational set mapping.
	 */
	blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
	rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
	blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;

	sectorNumber = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		      geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		      rotationalSet +
		      geoPtr->blockOffset[blockNumber];
	sectorNumber += (fragNumber % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;
    } else {
	/*
	 * Do straight-forward mapping.
	 */
	sectorNumber = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		    fragNumber * SECTORS_PER_FRAG - cylinder * 
		    geoPtr->blocksPerCylinder * FS_FRAGMENTS_PER_BLOCK *
		    SECTORS_PER_FRAG;
    }

    return(sectorNumber);
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Fs_BlocksToDiskAddr --
 *
 *	Convert from block indexes (actually, fragment indexes) to
 *	disk address (head, cylinder, sector) using the geometry information
 *	 on the disk.  This is a utility for block device drivers.
 *
 * Results:
 *	The disk address that corresponds to the disk address.
 *	The caller has to make sure that its I/O doesn't cross a
 *	filesystem block boundary.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef XYLOGICS_BOOT
void
Fs_BlocksToDiskAddr(fragNumber, data, diskAddrPtr)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
    Dev_DiskAddr *diskAddrPtr;
{
    register Ofs_Geometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (Ofs_Geometry *)data;
    /*
     * Map to block number because the rotational sets are laid out
     * relative to blocks.  After that the cylinder is easy because we know
     * blocksPerCylinder.  To get the head and sector we first get the
     * rotational set (described in fsDisk.h) of the block and the
     * block's sector offset (relative to the rotational set!).  This complex
     * algorithm crops up because there isn't necessarily an even number
     * of blocks per track.  The 'blockOffset' array in the geometry gives
     * a sector index of each successive block in a rotational set. Finally,
     * we can use the sectorsPerTrack to get the head and sector.
     */
    blockNumber		= fragNumber / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
    diskAddrPtr->cylinder = cylinder;

    rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
    blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;
/*
 * The follow statment had to be broken into two because the compiler used
 * register d2 to do the modulo operation, but wasn't saving its value.
 */
    sectorNumber	= geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
			  rotationalSet + geoPtr->blockOffset[blockNumber];
    sectorNumber	+=
		    (fragNumber % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;

    diskAddrPtr->head	= sectorNumber / geoPtr->sectorsPerTrack;
    diskAddrPtr->sector = sectorNumber -
			  diskAddrPtr->head * geoPtr->sectorsPerTrack;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Fs_SectorsToRawDiskAddr --
 *
 *      Convert from a sector offset to a raw disk address (cyl, head,
 *      sector) using the geometry information on the disk.  This is a
 *      utility for raw device drivers and does not pay attention to the
 *      rotational position of filesystem disk blocks.
 *
 *	This should be moved to Dev
 *
 * Results:
 *	The disk address that corresponds exactly to the byte offset.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef XYLOGICS_BOOT
int
Fs_SectorsToRawDiskAddr(sector, numSectors, numHeads, diskAddrPtr)
    int sector;		/* Sector number (counting from zero 'til the total
			 * number of sectors in the disk) */
    int numSectors;	/* Number of sectors per track */
    int numHeads;	/* Number of heads on the disk */
    Dev_DiskAddr *diskAddrPtr;
{
    register int sectorsPerCyl;	/* The rotational set with cylinder of frag */

    sectorsPerCyl		= numSectors * numHeads;
    diskAddrPtr->cylinder	= sector / sectorsPerCyl;
    sector			-= diskAddrPtr->cylinder * sectorsPerCyl;
    diskAddrPtr->head		= sector / numSectors;
    diskAddrPtr->sector		= sector - numSectors * diskAddrPtr->head;
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * FsDeviceBlockIO --
 *
 *	Map a file system block address to a block device block address 
 *	perform the requested operation.
 *
 * NOTE: This routine is temporary and should be replaced when the file system
 *	 is converted to use the async block io interface.
 *
 * Results:
 *	The return status of the operation.
 *
 * Side effects:
 *	Blocks may be written or read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsDeviceBlockIO(readWriteFlag, devicePtr, fragNumber, numFrags, buffer)
    int readWriteFlag;		/* FS_READ or FS_WRITE */
    Fs_Device *devicePtr;	/* Specifies device type to do I/O with */
    int fragNumber;		/* CAREFUL, fragment index, not block index.
				 * This is relative to start of device. */
    int numFrags;		/* CAREFUL, number of fragments, not blocks */
    Address buffer;		/* I/O buffer */
{
    ReturnStatus status;	/* General return code */
    int firstSector;		/* Starting sector of transfer */
    DevBlockDeviceRequest	request;
    int				transferCount;
    int				devType;
    Fs_IOParam			io;		
    Fs_IOReply			reply;	

    devType = DEV_TYPE_INDEX(devicePtr->type);
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
	io.offset = firstSector * DEV_BYTES_PER_SECTOR;
	io.length = extraFrags * FS_FRAGMENT_SIZE;
	io.buffer = buffer;
	status = (*devFsOpTable[devType].read)(devicePtr, &io, &reply);
	extraFrags = reply.length / FS_FRAGMENT_SIZE;
	fragNumber += extraFrags;
	buffer += reply.length;
	numFrags -= extraFrags;
    }
    if (numFrags > 0) {
	/*
	 * Transfer the left over fragments.
	 */
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	io.offset = firstSector * DEV_BYTES_PER_SECTOR;
	io.length = numFrags * FS_FRAGMENT_SIZE;
	io.buffer = buffer;
	status = (*devFsOpTable[devType].read)(devicePtr, &io, &reply);
    }
    return(status);
}

