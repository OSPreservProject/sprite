/* 
 * fsDisk.c --
 *
 *	Routines related to managing local disks.  Each partition of a local
 *	disk (partitions are defined by a table on the disk header) is
 *	called a ``domain''.  Fsdm_AttachDisk attaches a domain into the file
 *	system, and FsDeattachDisk removes it.  A domain is given
 *	a number the first time it is ever attached.  This is recorded on
 *	the disk so it doesn't change between boots.  The domain number is
 *	used to identify disks, and a domain number plus a file number is
 *	used to identify files.  Fsdm_DomainFetch is used to get the state
 *	associated with a disk, and Fsdm_DomainRelease releases the reference
 *	on the state.  Fsdm_DetachDisk checks the references on domains in
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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"

#include "fs.h"
#include "fsutil.h"
#include "fsdm.h"
#include "fslcl.h"
#include "fsNameOps.h"
#include "fsio.h"
#include "fsprefix.h"
#include "fsconsist.h"
#include "devDiskLabel.h"
#include "dev.h"
#include "devFsOpTable.h"
#include "sync.h"
#include "rpc.h"
#include "fsioDevice.h"
#include "fsdmInt.h"

/*
 * A table of domains indexed by domain number.  For use by a server
 * to map from domain number to info about the domain.
 */
Fsdm_Domain *fsdmDomainTable[FSDM_MAX_LOCAL_DOMAINS];
static int domainTableIndex = 0;

Sync_Lock	domainTableLock = Sync_LockInitStatic("Fs:domainTableLock");
#define LOCKPTR (&domainTableLock)


static Fscache_IOProcs  physicalIOProcs = {
    Fsdm_BlockAllocate, 
    Fsio_FileBlockRead, 
    Fsio_FileBlockWrite,
    Fsio_FileBlockCopy
};
/*
 * Forward declarations.
 */
static int	InstallLocalDomain();
static void	MarkDomainDown();
static Boolean	OkToDetach();
static Boolean	IsSunLabel();
static Boolean	IsSpriteLabel();

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_AttachDiskByHandle --
 *
 *	Make a particular open Handle correspond to a prefix. Calls 
 *	Fsdm_AttachDisk to do real work.
 *
 * Results:
 *	The SUCCESS if disk attach otherwise a Sprite return status.
 *
 * Side effects:
 *	Many - Those of Fsdm_AttachDisk.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsdm_AttachDiskByHandle(ioHandlePtr, localName, flags)
    Fs_HandleHeader *ioHandlePtr; /* Open device handle of domain. */
    char *localName;		/* The local prefix for the domain */
    int flags;			/* FS_ATTACH_READ_ONLY or FS_ATTACH_LOCAL */
{
    Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *) ioHandlePtr;
    return Fsdm_AttachDisk(&devHandlePtr->device, localName, flags);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_AttachDisk --
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
 *	Sets up the Fsutil_DomainInfo for the domain.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_AttachDisk(devicePtr, localName, flags)
    register Fs_Device *devicePtr;	/* Device info from I/O handle */
    char *localName;			/* The local prefix for the domain */
    int flags;			/* FS_ATTACH_READ_ONLY or FS_ATTACH_LOCAL */
{
    ReturnStatus status;		/* Error code */
    register Address buffer;		/* Read buffer */
    int headerSector;			/* Starting sector of domain header */
    int numHeaderSectors;		/* Number of sectors in domain header */
    int summarySector;			/* Sector of summary information. */
    Fsdm_SummaryInfo *summaryInfoPtr;	/* Pointer to summary info. */
    register Fsdm_Domain *domainPtr;	/* Top level info for the domain stored
					 * on the device */
    Fsio_FileIOHandle	*handlePtr;	/* Reference to file handle for root */
    Fs_FileID	fileID;			/* ID for root directory of domain */
    int		prefixFlags;		/* For installing the prefix */
    int		partition;		/* Partition number from the disk. */
    Fs_IOParam	io;			/* I/O Parameter block */
    Fs_IOReply	reply;			/* Results of I/O */
    int		devFlags;		/* Device flags. */
    int 	useFlags;		/* Use flags. */

    /*
     * Open the raw disk device so we can grub around in the header info.
     */
    useFlags = (flags | FS_ATTACH_READ_ONLY) ? FS_READ : (FS_READ|FS_WRITE);
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].open)
	    (devicePtr, useFlags, (Fs_NotifyToken) NIL, &devFlags);
    if (status != SUCCESS) {
	return(status);
    }

    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    /*
     * Read the zero'th sector of the partition.  It has a copy of the
     * zero'th sector of the whole disk which describes how the rest of the
     * domain's zero'th cylinder is layed out.
     */
    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR * FSDM_NUM_DOMAIN_SECTORS);
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
    if (IsSunLabel(buffer)) {
	Fsdm_DomainHeader		*domainHeaderPtr = (Fsdm_DomainHeader *) buffer;
	int			i;
	/*
	 * For Sun formatted disks we put the domain header well past
	 * the disk label and the boot program.
	 */
	numHeaderSectors = FSDM_NUM_DOMAIN_SECTORS;
	io.length = DEV_BYTES_PER_SECTOR * FSDM_NUM_DOMAIN_SECTORS;
	for (i = 2; i < FSDM_MAX_BOOT_SECTORS + 3; i+= FSDM_BOOT_SECTOR_INC) {
	    io.offset = i * DEV_BYTES_PER_SECTOR;
	    io.length = DEV_BYTES_PER_SECTOR * FSDM_NUM_DOMAIN_SECTORS;
	    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
			(devicePtr, &io, &reply);
	    if (status != SUCCESS) {
		free(buffer);
		return(status);
	    }
	    if (domainHeaderPtr->magic == FSDM_DOMAIN_MAGIC) {
		headerSector = i;
		summarySector = i - 1;
	        break;
	    }
	}
	if (i >= FSDM_MAX_BOOT_SECTORS + 3) {
	    printf("Fsdm_AttachDisk: Can't find domain header.\n");
	    free(buffer);
	    return(FAILURE);
	}
    } else if (IsSpriteLabel(buffer)) {
	register Fsdm_DiskHeader *diskHeaderPtr;
	diskHeaderPtr = (Fsdm_DiskHeader *)buffer;
	headerSector = diskHeaderPtr->domainSector;
	numHeaderSectors = diskHeaderPtr->numDomainSectors;
	summarySector = diskHeaderPtr->summarySector;
    } else {
	printf("Fsdm_AttachDisk: No disk header\n");
	free(buffer);
	return(FAILURE);
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
    summaryInfoPtr = (Fsdm_SummaryInfo *) buffer;
    (void)strcpy(summaryInfoPtr->domainPrefix, localName);

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
	free(buffer);
	return(status);
    } else if (((Fsdm_DomainHeader *)buffer)->magic != FSDM_DOMAIN_MAGIC) {
	printf("Fsdm_AttachDisk: Bad magic # on partition header <%x>\n",
				  ((Fsdm_DomainHeader *)buffer)->magic);
	free(buffer);
	return(FAILURE);
    }

    /*
     * Attempt to attach the disk under the same domain number each time.
     * This is required if clients are to be able to re-open files.
     */

    if (summaryInfoPtr->domainNumber != -1) {
	domainPtr = fsdmDomainTable[summaryInfoPtr->domainNumber];
	if (domainPtr != (Fsdm_Domain *)NIL) {
	    if (!(domainPtr->flags & FSDM_DOMAIN_DOWN)) {
		printf("Fsdm_AttachDisk: domain already attached?\n");
		free(buffer);
		return(FS_DOMAIN_UNAVAILABLE);
	    }
	}
    } else {
	domainPtr = (Fsdm_Domain *)NIL;
    }
    if (domainPtr == (Fsdm_Domain *)NIL) {
	domainPtr = (Fsdm_Domain *)malloc(sizeof(Fsdm_Domain));
	domainPtr->flags = 0;
	domainPtr->refCount = 0;
    }
    domainPtr->headerPtr = (Fsdm_DomainHeader *) buffer;
    domainPtr->summaryInfoPtr = summaryInfoPtr;
    domainPtr->summarySector = summarySector;

    if (rpc_SpriteID == 0) {
	/*
	 * Find the spriteID on the disk if we don't know it by now.
	 * This is the last resort.  Usually reverse arp or the hook
	 * in the RPC protocol have established our ID.  If there are
	 * no other Sprite hosts running on the network , however,
	 * then this code will execute.
	 */
	printf("Fsdm_AttachDisk: setting rpc_SpriteID to 0x%x from disk header\n",
		    domainPtr->headerPtr->device.serverID);
	if (domainPtr->headerPtr->device.serverID <= 0) {
	    panic("Bad sprite ID\n");
	}
	rpc_SpriteID = domainPtr->headerPtr->device.serverID;
    }
    /*
     * Set up the ClientData part of *devicePtr to reference the
     * Fsdm_Geometry part of the domain header.  This is used by the
     * block I/O routines.
     */
    ((DevBlockDeviceHandle *) (devicePtr->data))->clientData = 
			(ClientData)&domainPtr->headerPtr->geometry;
    /* 
     * Verify the device specification by checking the partition
     * number kept in the domain header.  We may map to a different
     * partition here, allowing us to attach at boot time other than
     * the zero'th ('a') partition.  The 'c' partition, for example,
     * also starts at the zero'th sector, but traditionally takes
     * up the whole disk.
     */
    partition = domainPtr->headerPtr->device.unit;
    if (partition >= 0 && partition < FSDM_NUM_DISK_PARTS) {
	if ((devicePtr->unit % FSDM_NUM_DISK_PARTS) != partition) {
	    if (devicePtr->unit % FSDM_NUM_DISK_PARTS) {
		/*
		 * Only allow automatic corrections with partition 'a' -> 'c'.
		 */
		printf("Fsdm_AttachDisk: partition mis-match, arg %d disk %d\n",
			  devicePtr->unit, partition);
	    } else {
		devicePtr->unit += partition;
	    }
	}
    }
    /*
     * Fix up the device information in the domain header
     * as this is used by the block I/O routines.
     */
    domainPtr->headerPtr->device.unit = devicePtr->unit;
    domainPtr->headerPtr->device.type = devicePtr->type;
    domainPtr->headerPtr->device.data = devicePtr->data;

    /*
     * After reading the low level header information from the disk we
     * install the domain into the set of active domains and initialize
     * things for block I/O.
     */
    fileID.type = FSIO_LCL_FILE_STREAM;
    fileID.serverID = rpc_SpriteID;
    fileID.major = InstallLocalDomain(domainPtr);
    summaryInfoPtr->domainNumber = fileID.major;
    fileID.minor = FSDM_ROOT_FILE_NUMBER;

    status = Fsdm_IOInit(domainPtr, fileID.major);
    if (status != SUCCESS) {
	printf( "Fsdm_AttachDisk: can't initialize block I/O %x\n",
		status);
	domainPtr->flags |= FSDM_DOMAIN_DOWN;
	return(status);
    }

    /*
     * Now that the block I/O is set up we can read the file descriptor
     * of the root directory of the domain.
     */
    status = Fsio_LocalFileHandleInit(&fileID, localName, &handlePtr);
    if (status != SUCCESS) {
	printf( "Fsdm_AttachDisk: can't get root file handle %x\n",
		status);
	domainPtr->flags |= FSDM_DOMAIN_DOWN;
	return(status);
    }
    Fsutil_HandleUnlock(handlePtr);
    /*
     * Install a prefix for the domain.  We always import it so that
     * we can get to the disk locally.  Then we either keep the domain
     * private or export it depending on the flags argument.
     */
    prefixFlags = FSPREFIX_IMPORTED;
    if (flags & FS_ATTACH_LOCAL) {
	prefixFlags |= FSPREFIX_LOCAL;
    } else {
	prefixFlags |= FSPREFIX_EXPORTED;
    }
    (void)Fsprefix_Install(localName, (Fs_HandleHeader *)handlePtr,
			  FS_LOCAL_DOMAIN,  prefixFlags);

    /*
     * Finally mark the domain to indicate that if we go down hard,
     * clean recovery of this domain is impossible.
     */
    if ((domainPtr->summaryInfoPtr->flags & FSDM_DOMAIN_NOT_SAFE) == 0) {
	domainPtr->summaryInfoPtr->flags |= FSDM_DOMAIN_ATTACHED_CLEAN;
    } else {
	domainPtr->summaryInfoPtr->flags &= ~FSDM_DOMAIN_ATTACHED_CLEAN;
	printf("Warning: Fsdm_AttachDisk: \"%s\" not clean\n", localName);
    }
    domainPtr->summaryInfoPtr->flags |= FSDM_DOMAIN_NOT_SAFE |
					FSDM_DOMAIN_TIMES_VALID;
    domainPtr->summaryInfoPtr->attachSeconds = fsutil_TimeInSeconds;
    if (flags & FS_ATTACH_READ_ONLY == 0) {
	status = FsdmWriteBackSummaryInfo(domainPtr);
	if (status != SUCCESS) {
	    panic( "Fsdm_AttachDisk: Summary write failed, status %x\n", status);
	}
    }
    domainPtr->flags = 0;

    /*
     * Make sure a name hash table exists now that we have a disk attached.
     */
    Fslcl_NameHashInit();
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_IOInit --
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
Fsdm_IOInit(domainPtr, domainNumber)
    register	Fsdm_Domain	*domainPtr;
    int				domainNumber;
{
    ReturnStatus		status;
    Fscache_Attributes		attr;

    /*
     * Initialize the file handle used for raw I/O, i.e. for file descriptors,
     * the bitmaps, and indirect blocks.
     */

    bzero((Address)&domainPtr->physHandle, sizeof(domainPtr->physHandle));
    domainPtr->physHandle.hdr.fileID.major = domainNumber;
    domainPtr->physHandle.hdr.fileID.minor = 0;
    domainPtr->physHandle.hdr.fileID.type = FSIO_LCL_FILE_STREAM;
    domainPtr->physHandle.descPtr = (Fsdm_FileDescriptor *)NIL;

    bzero((Address)&attr, sizeof(attr));
    attr.lastByte = 0x7fffffff;
    Fscache_InfoInit(&domainPtr->physHandle.cacheInfo,
		    (Fs_HandleHeader *) &domainPtr->physHandle,
		    0, TRUE, &attr, &physicalIOProcs);

    status = FsdmBlockAllocInit(domainPtr);
    if (status != SUCCESS) {
	printf( "Block Alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }

    status = FsdmFileDescAllocInit(domainPtr);
    if (status != SUCCESS) {
	printf( "File Desc alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DetachDisk --
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
Fsdm_DetachDisk(prefixName)
    char	*prefixName;	/* Name that the disk is attached under. */
{
    Fs_HandleHeader	*hdrPtr;
    char		*lookupName;
    int			domainType;
    Fsprefix		*prefixPtr;
    Fs_FileID		rootID;
    int			serverID;
    int			domain;
    register Fsdm_Domain	*domainPtr;
    int			i;
    ReturnStatus	status;
    int			blocksLeft;

    /*
     * Find the domain to detach.
     */
    status = Fsprefix_Lookup(prefixName, 
		   FSPREFIX_EXACT | FSPREFIX_EXPORTED | FSPREFIX_LOCAL,
		   rpc_SpriteID, &hdrPtr, &rootID, &lookupName,
		   &serverID, &domainType, &prefixPtr);
    if (status != SUCCESS) {
	return(status);
    } else if (hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	return(GEN_INVALID_ARG);
    }
    domain = hdrPtr->fileID.major;
    domainPtr = Fsdm_DomainFetch(domain, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    /*
     * Recall dirty blocks from remote clients, and copy dirty file descriptors
     * into their cache blocks.  Once done, don't allow any more dirty
     * blocks to enter the cache.
     */
    Fsconsist_GetAllDirtyBlocks(domain, TRUE);	
    status = Fsutil_HandleDescWriteBack(FALSE, -1);
    if (status != SUCCESS) {
	printf( "Fsdm_DetachDisk: handle write-back failed <%x>.\n",
	    status);
    }
    /*
     * Mark the domain and wait for other users of the domain to leave.
     * The user can interrupt this wait, at which point we bail out.
     */
    if (!OkToDetach(domainPtr)) {
	Fsdm_DomainRelease(domain);
	return(FS_FILE_BUSY);
    }
    /*
     * Nuke the prefix table entry.  Actually, this closes the handle
     * and leaves the prefix entry with no handle.
     */
    Fsprefix_HandleClose(prefixPtr, FSPREFIX_EXPORTED);
    /*
     * Write all dirty blocks, bitmaps, etc. to disk and take the
     * domain down.
     */
    Fscache_WriteBack(-1, &blocksLeft, FALSE);	/* Write back the cache. */
    Fsdm_DomainWriteBack(domain, FALSE, TRUE);/* Write back domain info. */
    MarkDomainDown(domainPtr);
    /*
     * We successfully brought down the disk, so mark it as OK.
     * The detach time is noted in order to track how long disks are available.
     */
    domainPtr->summaryInfoPtr->flags &= ~FSDM_DOMAIN_NOT_SAFE;
    domainPtr->summaryInfoPtr->detachSeconds = fsutil_TimeInSeconds;
    status = FsdmWriteBackSummaryInfo(domainPtr);
    if (status != SUCCESS) {
	panic( "Fsdm_DetachDisk: Summary write failed, status %x\n",
		    status);
    }

    /*
     * Free up resources for the domain.
     */
    Sync_LockClear(&domainPtr->dataBlockLock);
    Sync_LockClear(&domainPtr->fileDescLock);
    free((Address)domainPtr->headerPtr);
    free((Address)domainPtr->summaryInfoPtr);
    free((Address)domainPtr->dataBlockBitmap);
    free((Address)domainPtr->cylinders);
    for (i = 0; i < FSDM_NUM_FRAG_SIZES; i++) {
	List_Links	*fragList;
	FsdmFragment	*fragPtr;
	fragList = domainPtr->fragLists[i];
	while (!List_IsEmpty(fragList)) {
	    fragPtr = (FsdmFragment *)List_First(fragList);
	    List_Remove((List_Links *)fragPtr);
	    free((Address)fragPtr);
	}
	free((Address)fragList);
    }
    free((Address)domainPtr->fileDescBitmap);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DomainWriteBack --
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
void
Fsdm_DomainWriteBack(domain, shutdown, detach)
    int		domain;		/* Domain number, -1 means all domains */
    Boolean	shutdown;	/* TRUE if are syncing to shutdown the system.*/
    Boolean	detach;		/* TRUE if are writing back as part of 
				 * detaching the domain.  This is used to force 
				 * the domain fetch to work even if it is
				 * marked as down. */
{
    register	Fsdm_Domain	*domainPtr;
    ReturnStatus		status1, status2;
    int				firstDomain;
    register int 		lastDomain;
    register int 		i;

    if (domain >= 0) {
	/*
	 * Write back a particular domain.
	 */
	firstDomain = domain;
	lastDomain = domain;
    } else {
	/*
	 * Write back all domains.
	 */
	firstDomain = 0;
	lastDomain = FSDM_MAX_LOCAL_DOMAINS - 1;
	detach = FALSE;
    }
    for (i = firstDomain; i <= lastDomain; i++) {
	domainPtr = Fsdm_DomainFetch(i, detach);
	if (domainPtr != (Fsdm_Domain *) NIL) {
	    status1 = FsdmWriteBackDataBlockBitmap(domainPtr);
	    status2 = FsdmWriteBackFileDescBitmap(domainPtr);
	    if (shutdown && status1 == SUCCESS && status2 == SUCCESS) {
		domainPtr->summaryInfoPtr->flags &= ~FSDM_DOMAIN_NOT_SAFE;
		domainPtr->summaryInfoPtr->detachSeconds = fsutil_TimeInSeconds;
	    } else {
		domainPtr->summaryInfoPtr->flags |= FSDM_DOMAIN_NOT_SAFE;
	    }
	    (void)FsdmWriteBackSummaryInfo(domainPtr);
	    Fsdm_DomainRelease(i);
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * InstallLocalDomain --
 *
 *	Take the information about a newly attached local domain and
 *	save it in the domain table.  The index into this table is
 *	returned and passed around as the identifier for the domain.
 *
 * Results:
 *	The domain number.
 *
 * Side effects:
 *	Save the info in fsVolumeTable
 *
 *----------------------------------------------------------------------
 */
static ENTRY int
InstallLocalDomain(domainPtr)
    Fsdm_Domain *domainPtr;
{
    int domainNumber;

    LOCK_MONITOR;


    if (domainPtr->summaryInfoPtr->domainNumber == -1) {
	while (fsdmDomainTable[domainTableIndex] != (Fsdm_Domain *)NIL) {
	    domainTableIndex++;
	}
	fsdmDomainTable[domainTableIndex] = domainPtr;
	domainNumber = domainTableIndex;
	domainTableIndex++;
    } else {
	domainNumber = domainPtr->summaryInfoPtr->domainNumber;
	fsdmDomainTable[domainNumber] = domainPtr;
    }

    UNLOCK_MONITOR;
    return(domainNumber);
}


/*
 *----------------------------------------------------------------------
 *
 * OkToDetach --
 *
 *	Wait for activity in a domain to end and then mark the
 *	domain as being down.  This prints a warning message and
 *	waits in an interruptable state if the domain is in use.
 *	Our caller should back out if we return FALSE.
 *
 * Results:
 *	TRUE if it is ok to detach the domain.
 *
 * Side effects:
 *	The FSDM_DOMAIN_GOING_DOWN flag is set in the domain.
 *
 *----------------------------------------------------------------------
 */
static ENTRY Boolean
OkToDetach(domainPtr)
    Fsdm_Domain	*domainPtr;
{
    LOCK_MONITOR;

    domainPtr->flags |= FSDM_DOMAIN_GOING_DOWN;
    /*
     * Wait until we are the only user of the domain because 
     * noone else but the cache block cleaner or us should be using this
     * domain once we set this flag.
     */
    while (domainPtr->refCount > 1) {
	printf("Waiting for busy domain \"%s\"\n",
	    domainPtr->summaryInfoPtr->domainPrefix);
	if (Sync_Wait(&domainPtr->condition, TRUE)) {
	    domainPtr->flags &= ~FSDM_DOMAIN_GOING_DOWN;
	    UNLOCK_MONITOR;
	    return(FALSE);	/* Interrupted while waiting, domain busy */
	}
    }

    UNLOCK_MONITOR;
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * MarkDomainDown --
 *
 *	Mark the domain as being down.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The FSDM_DOMAIN_DOWN flag is set.
 *
 *----------------------------------------------------------------------
 */
static ENTRY void
MarkDomainDown(domainPtr)
    Fsdm_Domain	*domainPtr;
{
    LOCK_MONITOR;

    domainPtr->flags |= FSDM_DOMAIN_DOWN;
    if (domainPtr->refCount > 1) {
	printf("DomainDown: Refcount > 1\n");
    }
    domainPtr->refCount = 0;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DomainFetch --
 *
 *	Return a pointer to the given domain if it is available.
 *
 * Results:
 *	Pointer to the domain.
 *
 * Side effects:
 *	Reference count in the domain incremented.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fsdm_Domain *
Fsdm_DomainFetch(domain, dontStop)
    int		domain;		/* Domain to fetch. */
    Boolean	dontStop;	/* Fetch this domain unless it has been
				 * totally detached. */
{
    register	Fsdm_Domain	*domainPtr;

    LOCK_MONITOR;

    if (domain < 0 || domain >= FSDM_MAX_LOCAL_DOMAINS) {
	printf( "Fsdm_DomainFetch, bad domain number <%d>\n",
	    domain);
	domainPtr = (Fsdm_Domain *)NIL;
    } else {
	domainPtr = fsdmDomainTable[domain];
    }
    if (domainPtr != (Fsdm_Domain *) NIL) {
	if (domainPtr->flags & FSDM_DOMAIN_DOWN) {
	    domainPtr = (Fsdm_Domain *) NIL;
	} else if (dontStop || !(domainPtr->flags & FSDM_DOMAIN_GOING_DOWN)) {
	    domainPtr->refCount++;
	} else {
	    domainPtr = (Fsdm_Domain *) NIL;
	}
    }

    UNLOCK_MONITOR;
    return(domainPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DomainRelease --
 *
 *	Release access to the given domain using a domain number.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reference count for the domain decremented.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsdm_DomainRelease(domainNum)
    int	domainNum;
{
    register	Fsdm_Domain	*domainPtr;

    LOCK_MONITOR;

    domainPtr = fsdmDomainTable[domainNum];
    if (domainPtr == (Fsdm_Domain *)NIL) {
	panic( "Fsdm_DomainRelease: NIL domain pointer\n");
    }

    domainPtr->refCount--;
    if (domainPtr->refCount < 0) {
	panic( "Fsdm_DomainRelease: Negative ref count on domain\n");
    }
    if (domainPtr->refCount == 0) {
	Sync_Broadcast(&domainPtr->condition);
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * IsSunLabel --
 *
 *	Poke around in the input buffer and see if it looks like
 *	a Sun format disk label.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
IsSunLabel(buffer)
    Address buffer;	/* Buffer containing zero'th sector */
{
    register Sun_DiskLabel *sunLabelPtr;

    sunLabelPtr = (Sun_DiskLabel *)buffer;
    if (sunLabelPtr->magic == SUN_DISK_MAGIC) {
	/*
	 * Should check checkSum...
	 */
	return(TRUE);
    } else {
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * IsSpriteLabel --
 *
 *	Poke around in the input buffer and see if it looks like
 *	a Sprite format disk header.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
IsSpriteLabel(buffer)
    Address buffer;	/* Buffer containing zero'th sector */
{
    register Fsdm_DiskHeader	*diskHeaderPtr;
    register int 		index;
    register int 		checkSum;
    int				*headerPtr;

    diskHeaderPtr = (Fsdm_DiskHeader *)buffer;
    if (diskHeaderPtr->magic == FSDM_DISK_MAGIC) {
	/*
	 * Check the checkSum which set so that an XOR of all the
	 * ints in the disk header comes out to FSDM_DISK_MAGIC also.
	 */
	checkSum = 0;
	for (index = 0, headerPtr = (int *)buffer;
	     index < DEV_BYTES_PER_SECTOR;
	     index += sizeof(int), headerPtr++) {
	    checkSum ^= *headerPtr;
	}
	if (checkSum == FSDM_DISK_MAGIC) {
	    return(TRUE);
	} else {
	    printf("IsSpriteLabel: checksum mismatch <%x>\n", checkSum);
	}
    }
    return(FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_Init --
 *
 *	Initialized the disk management routines. This means initializing
 *	the domain table to NIL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Fsdm_Init()
{
    register int index;
    for (index = 0; index < FSDM_MAX_LOCAL_DOMAINS; index++) {
        fsdmDomainTable[index] = (Fsdm_Domain *) NIL;
    }

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
 * Fsdm_BlocksToSectors --
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
Fsdm_BlocksToSectors(fragNumber, data)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
{
    register Fsdm_Geometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (Fsdm_Geometry *)data;
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
    } else if (geoPtr->rotSetsPerCyl == FSDM_SCSI_MAPPING) {
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
    }
    return(sectorNumber);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_BlocksToDiskAddr --
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
Fsdm_BlocksToDiskAddr(fragNumber, data, diskAddrPtr)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
    Dev_DiskAddr *diskAddrPtr;
{
    register Fsdm_Geometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (Fsdm_Geometry *)data;
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
 * Fsdm_SectorsToRawDiskAddr --
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
Fsdm_SectorsToRawDiskAddr(sector, numSectors, numHeads, diskAddrPtr)
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


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_RereadSummaryInfo --
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
Fsdm_RereadSummaryInfo(prefixName)
   char	*prefixName;	/* Name that the disk is attached under. */
{
    Fs_HandleHeader	*hdrPtr;
    char		*lookupName;
    int			domainType;
    Fsprefix		*prefixPtr;
    Fs_FileID		rootID;
    int			serverID;
    int			domain;
    register Fsdm_Domain	*domainPtr;
    ReturnStatus	status;
    Fs_Device		*devicePtr;
    Fs_IOParam		io;
    Fs_IOReply		reply;
    char		buffer[DEV_BYTES_PER_SECTOR];

    /*
     * Find the correct domain.
     */
    status = Fsprefix_Lookup(prefixName, 
		   FSPREFIX_EXACT | FSPREFIX_EXPORTED | FSPREFIX_LOCAL,
		   rpc_SpriteID, &hdrPtr, &rootID, &lookupName, &serverID,
		   &domainType, &prefixPtr);
    if (status != SUCCESS) {
	return(status);
    } else if (hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	return(GEN_INVALID_ARG);
    }
    domain = hdrPtr->fileID.major;
    domainPtr = Fsdm_DomainFetch(domain, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    /*
     * Read the summary sector.
     */
    bzero((Address)&io, sizeof(io));
    bzero((Address)&reply, sizeof(reply));
    devicePtr = &(domainPtr->headerPtr->device);
    io.buffer = buffer;
    io.length = DEV_BYTES_PER_SECTOR;
    io.offset = domainPtr->summarySector * DEV_BYTES_PER_SECTOR;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)
		(devicePtr, &io, &reply); 
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Copy information from the buffer.
     */
    bcopy(buffer, domainPtr->summaryInfoPtr, reply.length);
    return SUCCESS;
}

