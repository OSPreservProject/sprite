/* 
 * ofsDisk.c --
 *
 *	Routines related to managing local disks under the orginial sprite
 *	file system. 
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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>

#include <fs.h>
#include <fsutil.h>
#include <fsdm.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsprefix.h>
#include <fsconsist.h>
#include <devDiskLabel.h>
#include <dev.h>
#include <devFsOpTable.h>
#include <sync.h>
#include <rpc.h>
#include <fsioDevice.h>
#include <ofs.h>

#include <stdio.h>

static Fsdm_DomainOps ofsDomainOps = {
	Ofs_AttachDisk,
	Ofs_DetachDisk,
	Ofs_DomainWriteBack,
	Ofs_RereadSummaryInfo,
	Ofs_DomainInfo,
	Ofs_BlockAllocate,
	Ofs_GetNewFileNumber,
	Ofs_FreeFileNumber,
	Ofs_FileDescInit,
	Ofs_FileDescFetch,
	Ofs_FileDescStore,
	Ofs_FileBlockRead,
	Ofs_FileBlockWrite,
	Ofs_FileTrunc,
	Ofs_DirOpStart,
	Ofs_DirOpEnd
};


static Fscache_BackendRoutines  ofsBackendRoutines = {
	    Fsdm_BlockAllocate,
	    Fsdm_FileTrunc,
	    Fsdm_FileBlockRead,
	    Fsdm_FileBlockWrite,
	    Ofs_ReallocBlock,
	    Ofs_StartWriteBack,

};
static 	Fscache_Backend *cacheBackendPtr = (Fscache_Backend *) NIL;



/*
 *----------------------------------------------------------------------
 *
 * Ofs_Init --
 *
 *	Initialized code of the OFS disk storage manager module.
 *
 * Results:
 *
 * Side effects:
 *	Registers OFS with Fsdm module.
 *
 *----------------------------------------------------------------------
 */
void
Ofs_Init()
{
    Fsdm_RegisterDiskManager("OFS", Ofs_AttachDisk);
}

/*
 *----------------------------------------------------------------------
 *
 * Ofs_AttachDisk --
 *
 *	Try to attach a OFS disk from the specified disk.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Ofs_AttachDisk(devicePtr, localName, flags, domainNumPtr)
    Fs_Device	*devicePtr;	/* Device containing file system. */
    char *localName;		/* The local prefix for the domain */
    int  flags;			/* Attach flags. */
    int *domainNumPtr;		/* OUT: Domain number allocated. */

{
    ReturnStatus status;		/* Error code */
    register Address buffer;		/* Read buffer */
    int headerSector;			/* Starting sector of domain header */
    int numHeaderSectors;		/* Number of sectors in domain header */
    int summarySector;			/* Sector of summary information. */
    Ofs_SummaryInfo *summaryInfoPtr;	/* Pointer to summary info. */
    Fs_IOParam	io;			/* I/O Parameter block */
    Fs_IOReply	reply;			/* Results of I/O */
    int		partition;
    Fsdm_Domain	*domainPtr;	
    Ofs_Domain	*ofsPtr;


    headerSector = summarySector = 0;

    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    /*
     * Read the zero'th sector of the partition.  It has a copy of the
     * zero'th sector of the whole disk which describes how the rest of the
     * domain's zero'th cylinder is layed out.
     */
    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR * OFS_NUM_DOMAIN_SECTORS);
    io.buffer = buffer;
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = 0;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)(devicePtr,
		&io, &reply);
    if (status != SUCCESS) {
	free(buffer);
	return(status);
    }
    /*
     * Check for different disk formats, and figure out how the rest
     * of the zero'th cylinder is layed out.
     */
    if (Fsdm_IsSunLabel(buffer)) {
	Ofs_DomainHeader	*domainHeaderPtr = (Ofs_DomainHeader *) buffer;
	int			i;
	/*
	 * For Sun formatted disks we put the domain header well past
	 * the disk label and the boot program.
	 */
	numHeaderSectors = OFS_NUM_DOMAIN_SECTORS;
	io.length = DEV_BYTES_PER_SECTOR * OFS_NUM_DOMAIN_SECTORS;
	for (i = 2; i < FSDM_MAX_BOOT_SECTORS + 3; i+= FSDM_BOOT_SECTOR_INC) {
	    io.offset = i * DEV_BYTES_PER_SECTOR;
	    io.length = DEV_BYTES_PER_SECTOR * OFS_NUM_DOMAIN_SECTORS;
	    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
			(devicePtr, &io, &reply);
	    if (status != SUCCESS) {
		free(buffer);
		return(status);
	    }
	    if (domainHeaderPtr->magic == OFS_DOMAIN_MAGIC) {
		headerSector = i;
		summarySector = i - 1;
	        break;
	    }
	}
	if (i >= FSDM_MAX_BOOT_SECTORS + 3) {
	    free(buffer);
	    return(FAILURE);
	}
    } else if (Fsdm_IsSpriteLabel(buffer)) {
	register Fsdm_DiskHeader *diskHeaderPtr;
	diskHeaderPtr = (Fsdm_DiskHeader *)buffer;
	headerSector = diskHeaderPtr->domainSector;
	numHeaderSectors = diskHeaderPtr->numDomainSectors;
	summarySector = diskHeaderPtr->summarySector;
    } else {
	io.buffer = buffer;
	io.length = DEV_BYTES_PER_SECTOR;
	io.offset = DEC_LABEL_SECTOR * DEV_BYTES_PER_SECTOR;
	status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
		(devicePtr, &io, &reply);
	if (status != SUCCESS) {
	    free(buffer);
	    return(status);
	}
	if (Fsdm_IsDecLabel(buffer)){
	    register Dec_DiskLabel *decLabelPtr;
	    decLabelPtr = (Dec_DiskLabel *)buffer;
	    headerSector = decLabelPtr->domainSector;
	    numHeaderSectors = decLabelPtr->numDomainSectors;
	    summarySector = decLabelPtr->summarySector;
	} else {
	    printf("Ofs_AttachDisk: No disk header\n");
	    free(buffer);
	    return(FAILURE);
	}
    }
    /*
     * Read in summary information.
     */
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = summarySector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
		(devicePtr, &io, &reply); 
    if (status != SUCCESS) {
	free(buffer);
	return(status);
    }
    summaryInfoPtr = (Ofs_SummaryInfo *) buffer;

    /*
     * Read the domain header and save it with the domain state.
     */
    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR * numHeaderSectors);
    io.buffer = buffer;
    io.length = numHeaderSectors * DEV_BYTES_PER_SECTOR;
    io.offset = headerSector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)(devicePtr,
		&io, &reply);
    if (status != SUCCESS) {
	free((char *) summaryInfoPtr);
	free(buffer);
	return(status);
    } else if (((Ofs_DomainHeader *)buffer)->magic != OFS_DOMAIN_MAGIC) {
	free((char *) summaryInfoPtr);
	free(buffer);
	return(FAILURE);
    }
    /*
     * Make the domain flag positive to inform caller that this is an
     * OFS file system even if the mount fails.
     */
    *domainNumPtr = 1;
    /* 
     * Verify the device specification by checking the partition
     * number kept in the domain header. 
     */
    partition = ((Ofs_DomainHeader *)buffer)->device.unit;
    if (partition >= 0 && partition < FSDM_NUM_DISK_PARTS) {
	if ((devicePtr->unit % FSDM_NUM_DISK_PARTS) != partition) {
	    printf("Ofs_AttachDisk: partition mis-match, arg %d disk %d\n",
		      devicePtr->unit, partition);
	    free((char *) summaryInfoPtr);
	    free(buffer);
	    return(FAILURE);
	}
    }
    status = Fsdm_InstallDomain(summaryInfoPtr->domainNumber,
		((Ofs_DomainHeader *)buffer)->device.serverID, 
		localName, flags, &domainPtr);
    if (status != SUCCESS) {
	if (status == FS_FILE_BUSY) {
	    if (domainPtr->flags & FSDM_DOMAIN_ATTACH_BOOT) {
		domainPtr->flags &= ~FSDM_DOMAIN_ATTACH_BOOT;
		summaryInfoPtr->flags &=  ~OFS_DOMAIN_JUST_CHECKED;
		printf("Ofs_AttachDisk: clearing just-checked bit\n");
		if ((flags & FS_ATTACH_READ_ONLY) == 0) {
		    io.buffer = (char *) summaryInfoPtr;
		    io.length = DEV_BYTES_PER_SECTOR;
		    io.offset = summarySector * DEV_BYTES_PER_SECTOR;
		    status = 
		     (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].write)
					(devicePtr, &io, &reply);
		    if (status != SUCCESS) {
			panic( 
	    "Ofs_AttachDisk: Summary write failed, status %x\n", status);
		    }
		}
	    }
	    status = FS_DOMAIN_UNAVAILABLE;
	}
	free((char *) summaryInfoPtr);
	free(buffer);
	return status;
    }
    if (cacheBackendPtr == (Fscache_Backend *) NIL) {
	cacheBackendPtr = 
		Fscache_RegisterBackend(&ofsBackendRoutines,(ClientData) 0, 0);
    }

    ofsPtr = (Ofs_Domain *) malloc(sizeof(Ofs_Domain));
    bzero((char *) ofsPtr, sizeof(Ofs_Domain));

    domainPtr->backendPtr = cacheBackendPtr;
    domainPtr->domainOpsPtr = &ofsDomainOps;
    domainPtr->clientData = (ClientData) ofsPtr;

    ofsPtr->domainPtr = domainPtr;
    ofsPtr->headerPtr = (Ofs_DomainHeader *) buffer;
    ofsPtr->summaryInfoPtr = summaryInfoPtr;
    ofsPtr->summarySector = summarySector;
    ofsPtr->blockDevHandlePtr = (DevBlockDeviceHandle *) (devicePtr->data);
    /*
     * Fix up the device information in the domain header
     * as this is used by the block I/O routines.
     */
    ofsPtr->headerPtr->device.unit = devicePtr->unit;
    ofsPtr->headerPtr->device.type = devicePtr->type;
    ofsPtr->headerPtr->device.data = devicePtr->data;

    /*
     * After reading the low level header information from the disk we
     * install the domain into the set of active domains and initialize
     * things for block I/O.
     */

    status = OfsIOInit(domainPtr);
    if (status != SUCCESS) {
	printf( "Ofs_InitializeDomain: can't initialize block I/O %x\n",
		status);
	domainPtr->flags |= FSDM_DOMAIN_DOWN;
	return(status);
    }
    *domainNumPtr = domainPtr->domainNumber;
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * OfsIOInit --
 *
 *	Initialize a particular local domain for I/O.
 *
 * Results:
 *	Error code returned from initializing data block and file
 *	descriptor allocation.
 *
 * Side effects:
 *	Physical file handle for the domain is initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
OfsIOInit(domainPtr)
    Fsdm_Domain	*domainPtr;
{
    register	Ofs_Domain	*ofsPtr;
    ReturnStatus		status;
    Fscache_Attributes		attr;
    int			domainNumber;

    ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    domainNumber = domainPtr->domainNumber;
    /*
     * Initialize the file handle used for raw I/O, i.e. for file descriptors,
     * the bitmaps, and indirect blocks.
     */

    bzero((Address)&ofsPtr->physHandle, sizeof(ofsPtr->physHandle));
    ofsPtr->physHandle.hdr.fileID.major = domainNumber;
    ofsPtr->physHandle.hdr.fileID.minor = 0;
    ofsPtr->physHandle.hdr.fileID.type = FSIO_LCL_FILE_STREAM;
    ofsPtr->physHandle.descPtr = (Fsdm_FileDescriptor *)NIL;

    bzero((Address)&attr, sizeof(attr));
    attr.lastByte = 0x7fffffff;
    Fscache_FileInfoInit(&ofsPtr->physHandle.cacheInfo,
		    (Fs_HandleHeader *) &ofsPtr->physHandle,
		    0, TRUE, &attr, domainPtr->backendPtr);

    status = OfsBlockAllocInit(ofsPtr);
    if (status != SUCCESS) {
	printf( "Block Alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }
    status = OfsFileDescAllocInit(ofsPtr);
    if (status != SUCCESS) {
	printf( "File Desc alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }
    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * Ofs_DetachDisk --
 *
 *	Remove a local domain from the set of accessible domains.
 *
 * Results:
 *	SUCCESS if the domain was already attached and all the outstanding
 *	file handles could be recalled.
 *
 * Side effects:
 *	Clears the prefix table entry for the domain.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Ofs_DetachDisk(domainPtr)
    Fsdm_Domain	*domainPtr;
{
    register Ofs_Domain	*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    Ofs_DomainHeader	*headerPtr;
    int			i;
    ReturnStatus	status;
    int			blocksLeft, blocksSkipped;

    /*
     * Write all dirty blocks, bitmaps, etc. to disk and take the
     * domain down.
     */
    Fscache_WriteBack(-1, &blocksLeft, FALSE);	/* Write back the cache. */
    headerPtr = ofsPtr->headerPtr;
    Fscache_FileWriteBack(&ofsPtr->physHandle.cacheInfo,
	    headerPtr->fileDescOffset,
	    headerPtr->fileDescOffset + 
		(headerPtr->numFileDesc / FSDM_FILE_DESC_PER_BLOCK) + 1,
	    FSCACHE_WRITE_BACK_AND_INVALIDATE, &blocksSkipped);
    Fsdm_DomainWriteBack(ofsPtr->physHandle.hdr.fileID.major, FALSE, TRUE);
	/* Write back domain info. */
    /*
     * We successfully brought down the disk, so mark it as OK.
     * The detach time is noted in order to track how long disks are available.
     */
    ofsPtr->summaryInfoPtr->flags &= ~OFS_DOMAIN_NOT_SAFE;
    ofsPtr->summaryInfoPtr->flags &= ~OFS_DOMAIN_JUST_CHECKED;
    ofsPtr->summaryInfoPtr->detachSeconds = Fsutil_TimeInSeconds();
    status = OfsWriteBackSummaryInfo(ofsPtr);
    if (status != SUCCESS) {
	panic( "Ofs_DetachDisk: Summary write failed, status %x\n",
		    status);
    }

    /*
     * Free up resources for the domain.
     */
    Sync_LockClear(&ofsPtr->dataBlockLock);
    Sync_LockClear(&ofsPtr->fileDescLock);
    free((Address)ofsPtr->headerPtr);
    free((Address)ofsPtr->summaryInfoPtr);
    free((Address)ofsPtr->dataBlockBitmap);
    free((Address)ofsPtr->cylinders);
    for (i = 0; i < OFS_NUM_FRAG_SIZES; i++) {
	List_Links	*fragList;
	OfsFragment	*fragPtr;
	fragList = ofsPtr->fragLists[i];
	while (!List_IsEmpty(fragList)) {
	    fragPtr = (OfsFragment *)List_First(fragList);
	    List_Remove((List_Links *)fragPtr);
	    free((Address)fragPtr);
	}
	free((Address)fragList);
    }
    free((Address)ofsPtr->fileDescBitmap);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Ofs_DomainWriteBack --
 *
 *	Force all domain information to disk.
 *
 * Results:
 *	Error code if the write failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Ofs_DomainWriteBack(domainPtr, shutdown)
    Fsdm_Domain	*domainPtr;	/* Domain to writeback. */
    Boolean	shutdown;	/* TRUE if are syncing to shutdown the system.*/
{
    register	Ofs_Domain	*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    ReturnStatus		status1, status2;

    status1 = OfsWriteBackDataBlockBitmap(ofsPtr);
    status2 = OfsWriteBackFileDescBitmap(ofsPtr);
    if (shutdown && status1 == SUCCESS && status2 == SUCCESS) {
	ofsPtr->summaryInfoPtr->flags &= ~OFS_DOMAIN_NOT_SAFE;
	ofsPtr->summaryInfoPtr->detachSeconds = Fsutil_TimeInSeconds();
    } else {
	ofsPtr->summaryInfoPtr->flags |= OFS_DOMAIN_NOT_SAFE;
    }
    (void)OfsWriteBackSummaryInfo(ofsPtr);
    return (status1 == SUCCESS) ? status2 : status1;
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
 * OfsBlocksToSectors --
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
int
OfsBlocksToSectors(fragNumber, geoPtr)
    int fragNumber;	/* Fragment index to map into block index */
    register Ofs_Geometry *geoPtr;	/* ClientData from the device info */
{
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    blockNumber		= fragNumber / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    /*
     * This is the old way of doing things.  We have to deal with rotational
     * sets and the like.
     */
    if (geoPtr->rotSetsPerCyl > 0) {	
    
	blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
	rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
	blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;
    
	sectorNumber = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		      geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		      rotationalSet +
		      geoPtr->blockOffset[blockNumber];
	sectorNumber += (fragNumber % FS_FRAGMENTS_PER_BLOCK) * 
			SECTORS_PER_FRAG;
    } else if (geoPtr->rotSetsPerCyl == OFS_SCSI_MAPPING) {
	/*
	 * The new way is to map blocks from the start of the disk without
	 * regard for rotational sets.  There is essentially one rotational
	 * set per disk.
	 */
	sectorNumber = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		    fragNumber * SECTORS_PER_FRAG - cylinder * 
		    geoPtr->blocksPerCylinder * FS_FRAGMENTS_PER_BLOCK *
		    SECTORS_PER_FRAG;
    } else {
	panic("Unknown mapping in domain geometry information\n");
	sectorNumber = -1;
    }
    return(sectorNumber);
}

/*
 *----------------------------------------------------------------------
 *
 * Ofs_BlocksToDiskAddr --
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
void
Ofs_BlocksToDiskAddr(fragNumber, data, diskAddrPtr)
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

/*
 *----------------------------------------------------------------------
 *
 * Ofs_SectorsToRawDiskAddr --
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
int
Ofs_SectorsToRawDiskAddr(sector, numSectors, numHeads, diskAddrPtr)
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
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Ofs_RereadSummaryInfo --
 *
 *	Reread the summary sector associated with the prefix and update
 *	the domain information. This should be called if the summary
 *	sector on the disk has been changed since the domain was attached.
 *
 * Results:
 *	SUCCESS if the summary sector was read correctly and the 
 *	information was updated
 *
 * Side effects:
 *	The summary sector information associated with the domain is
 *	updated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ofs_RereadSummaryInfo(domainPtr)
    Fsdm_Domain		*domainPtr;	/* Domain to reread summary for. */
{
    register Ofs_Domain	*ofsPtr = OFS_PTR_FROM_DOMAIN(domainPtr);
    ReturnStatus	status;
    Fs_Device		*devicePtr;
    Fs_IOParam		io;
    Fs_IOReply		reply;
    char		buffer[DEV_BYTES_PER_SECTOR];

    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    devicePtr = &(ofsPtr->headerPtr->device);
    io.buffer = buffer;
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = ofsPtr->summarySector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
		(devicePtr, &io, &reply); 
    /*
     * Copy information from the buffer.
     */
    if (status == SUCCESS) { 
	bcopy(buffer, (char *)ofsPtr->summaryInfoPtr, reply.length);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Ofs_DirOpStart --
 *
 *	Mark the start of a directory operation on an OFS file system.
 *	Since OFS uses fscheck to do crash recovery, this is a noop.
 *
 * Results:
 *	NIL
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Ofs_DirOpStart(domainPtr, opFlags, name, nameLen, fileNumber, fileDescPtr,
		 dirFileNumber, dirOffset, dirFileDescPtr)
    Fsdm_Domain	*domainPtr;	/* Domain containing the object being modified.
				 */
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object being operated on
				       * before operation starts. */
    int		dirFileNumber;	/* File number of directory containing object.*/
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. */
    Fsdm_FileDescriptor *dirFileDescPtr; /* FileDescriptor of directory before
					  * operation starts. */
{
    return (ClientData) -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Ofs_DirOpEnd --
 *
 *	Mark the end of a directory operation on an OFS file system.
 *	Since OFS uses fscheck to do crash recovery, this is a noop.
 *
 * Results:
 *	None
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Ofs_DirOpEnd(domainPtr, clientData, status, opFlags, name, nameLen, 
		fileNumber,  fileDescPtr, dirFileNumber, dirOffset, 
		dirFileDescPtr)
    Fsdm_Domain	*domainPtr;	/* Domain containing the object modified. */
    ClientData	clientData;	/* ClientData as returned by DirOpStart. */
    ReturnStatus status;	/* Return status of the operation, SUCCESS if
				 * operation succeeded. FAILURE otherwise. */
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object after
				      * operation.*/
    int		dirFileNumber;	/* File number of directory containing object.*/
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. */
    Fsdm_FileDescriptor *dirFileDescPtr; /* FileDescriptor of directory after
					  * operation. */
{
    return;
}

