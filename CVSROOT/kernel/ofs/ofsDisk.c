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
 *	used to identify files.  FsDomainFetch is used to get the state
 *	associated with a disk, and FsDomainRelease releases the reference
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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"

#include "fs.h"
#include "fsInt.h"
#include "fsDisk.h"
#include "fsLocalDomain.h"
#include "fsOpTable.h"
#include "fsDevice.h"
#include "fsPrefix.h"
#include "fsConsist.h"
#include "devDiskLabel.h"
#include "dev.h"
#include "sync.h"
#include "rpc.h"

/*
 * A table of domains indexed by domain number.  For use by a server
 * to map from domain number to info about the domain.
 */
FsDomain *fsDomainTable[FS_MAX_LOCAL_DOMAINS];
static int domainTableIndex = 0;

Sync_Lock	domainTableLock;
#define LOCKPTR (&domainTableLock)
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
 *	Sets up the FsDomainInfo for the domain.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsAttachDisk(devicePtr, localName, flags)
    register Fs_Device *devicePtr;	/* Device info from I/O handle */
    char *localName;			/* The local prefix for the domain */
    int flags;			/* FS_ATTACH_READ_ONLY or FS_ATTACH_LOCAL */
{
    ReturnStatus status;		/* Error code */
    register Address buffer;		/* Read buffer */
    int headerSector;			/* Starting sector of domain header */
    int numHeaderSectors;		/* Number of sectors in domain header */
    int summarySector;			/* Sector of summary information. */
    FsSummaryInfo *summaryInfoPtr;	/* Pointer to summary info. */
    int amountRead;			/* Returned from read call */
    register FsDomain *domainPtr;	/* Top level info for the domain stored
					 * on the device */
    FsLocalFileIOHandle	*handlePtr;	/* Reference to file handle for root */
    FsFileID	fileID;			/* ID for root directory of domain */
    int		prefixFlags;		/* For installing the prefix */
    int		partition;		/* Partition number from the disk. */

    /*
     * Open the raw disk device so we can grub around in the header info.
     */
    status = (*fsDeviceOpTable[devicePtr->type].open)(devicePtr);
    if (status != SUCCESS) {
	return(status);
    }
    buffer = (Address)Mem_Alloc(DEV_BYTES_PER_SECTOR);

    /*
     * Read the zero'th sector of the partition.  It has a copy of the
     * zero'th sector of the whole disk which describes how the rest of the
     * domain's zero'th cylinder is layed out.
     */
    status = (*fsDeviceOpTable[devicePtr->type].read)(devicePtr,
		0, DEV_BYTES_PER_SECTOR, buffer, &amountRead);
    if (status != SUCCESS) {
	Mem_Free(buffer);
	return(status);
    }
    /*
     * Check for different disk formats, and figure out how the rest
     * of the zero'th cylinder is layed out.
     */
    if (IsSunLabel(buffer)) {
	/*
	 * For Sun formatted disks we put the domain header well past
	 * the disk label and the boot program.
	 */
	headerSector = SUN_DOMAIN_SECTOR;
	numHeaderSectors = FS_NUM_DOMAIN_SECTORS;
	summarySector = SUN_SUMMARY_SECTOR;
    } else if (IsSpriteLabel(buffer)) {
	register FsDiskHeader *diskHeaderPtr;
	diskHeaderPtr = (FsDiskHeader *)buffer;
	headerSector = diskHeaderPtr->domainSector;
	numHeaderSectors = diskHeaderPtr->numDomainSectors;
	summarySector = diskHeaderPtr->summarySector;
    } else {
	Sys_Printf("No disk header\n");
	Mem_Free(buffer);
	return(FAILURE);
    }
    /*
     * Read in summary information.
     */
    status = (*fsDeviceOpTable[devicePtr->type].read)
		(devicePtr, summarySector * DEV_BYTES_PER_SECTOR,
		    DEV_BYTES_PER_SECTOR,
		    buffer, &amountRead); 
    if (status != SUCCESS) {
	Mem_Free(buffer);
	return(status);
    }
    summaryInfoPtr = (FsSummaryInfo *) buffer;
    String_Copy(localName, summaryInfoPtr->domainPrefix);

    /*
     * Read the domain header and save it with the domain state.
     */
    buffer = (Address)Mem_Alloc(DEV_BYTES_PER_SECTOR * numHeaderSectors);
    status = (*fsDeviceOpTable[devicePtr->type].read)(devicePtr,
		headerSector * DEV_BYTES_PER_SECTOR,
		numHeaderSectors * DEV_BYTES_PER_SECTOR,
		buffer, &amountRead);
    if (status != SUCCESS) {
	Mem_Free(buffer);
	return(status);
    }

    /*
     * Attempt to attach the disk under the same domain number each time.
     * This is required if clients are to be able to re-open files.
     */

    if (summaryInfoPtr->domainNumber != -1) {
	domainPtr = fsDomainTable[summaryInfoPtr->domainNumber];
	if (domainPtr != (FsDomain *)NIL) {
	    if (!(domainPtr->flags & FS_DOMAIN_DOWN)) {
		Mem_Free(buffer);
		return(FS_DOMAIN_UNAVAILABLE);
	    }
	}
    } else {
	domainPtr = (FsDomain *)NIL;
    }
    if (domainPtr == (FsDomain *)NIL) {
	domainPtr = (FsDomain *)Mem_Alloc(sizeof(FsDomain));
	domainPtr->flags = 0;
	domainPtr->refCount = 0;
    }
    domainPtr->headerPtr = (FsDomainHeader *) buffer;
    domainPtr->summaryInfoPtr = summaryInfoPtr;
    domainPtr->summarySector = summarySector;
    if (domainPtr->headerPtr->magic != FS_DOMAIN_MAGIC) {
	Sys_Printf("FsDiskAttach: Bad magic # on domain header <%x>\n",
				  domainPtr->headerPtr->magic);
	Mem_Free(buffer);
	return(FAILURE);
    }
    if (rpc_SpriteID == 0) {
	/*
	 * Find the spriteID on the disk if we don't know it by now.
	 * This is the last resort.  Usually reverse arp or the hook
	 * in the RPC protocol have established our ID.  If there are
	 * no other Sprite hosts running on the network , however,
	 * then this code will execute.
	 */
	Sys_Printf("Setting rpc_SpriteID to 0x%x from disk header\n",
		    domainPtr->headerPtr->device.serverID);
	if (domainPtr->headerPtr->device.serverID <= 0) {
	    Sys_Panic(SYS_FATAL, "Bad sprite ID\n");
	}
	rpc_SpriteID = domainPtr->headerPtr->device.serverID;
    }
    /*
     * Set up the ClientData part of *devicePtr to reference the
     * FsGeometry part of the domain header.  This is used by the
     * block I/O routines.
     */
    devicePtr->data = (ClientData)&domainPtr->headerPtr->geometry;
    /* 
     * Verify the device specification by checking the partition
     * number kept in the domain header.  We may map to a different
     * partition here, allowing us to attach at boot time other than
     * the zero'th ('a') partition.  The 'c' partition, for example,
     * also starts at the zero'th sector, but traditionally takes
     * up the whole disk.
     */
    partition = domainPtr->headerPtr->device.unit;
    if (partition >= 0 && partition < FS_NUM_DISK_PARTS) {
	if ((devicePtr->unit % FS_NUM_DISK_PARTS) != partition) {
	    if (devicePtr->unit % FS_NUM_DISK_PARTS) {
		/*
		 * Only allow automatic corrections with partition 'a'.
		 */
		Sys_Panic(SYS_WARNING,
		    "FsAttachDisk: partition mis-match, arg %d disk %d\n",
			  devicePtr->unit, partition);
	    } else {
		devicePtr->unit += partition;
	    }
	}
    }
    domainPtr->headerPtr->device = *devicePtr;

    /*
     * After reading the low level header information from the disk we
     * install the domain into the set of active domains and initialize
     * things for block I/O.
     */
    fileID.type = FS_LCL_FILE_STREAM;
    fileID.serverID = rpc_SpriteID;
    fileID.major = InstallLocalDomain(domainPtr);
    summaryInfoPtr->domainNumber = fileID.major;
    fileID.minor = FS_ROOT_FILE_NUMBER;

    status = FsLocalIOInit(domainPtr, fileID.major);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsAttachDisk: can't initialize block I/O %x\n",
		status);
	domainPtr->flags |= FS_DOMAIN_DOWN;
	return(status);
    }

    /*
     * Now that the block I/O is set up we can read the file descriptor
     * of the root directory of the domain.
     */
    status = FsLocalFileHandleInit(&fileID, &handlePtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsAttachDisk: can't get root file handle %x\n",
		status);
	domainPtr->flags |= FS_DOMAIN_DOWN;
	return(status);
    }
    FsHandleUnlock(handlePtr);
    /*
     * Install a prefix for the domain.  We always import it so that
     * we can get to the disk locally.  Then we either keep the domain
     * private or export it depending on the flags argument.
     */
    prefixFlags = FS_IMPORTED_PREFIX;
    if (flags & FS_ATTACH_LOCAL) {
	prefixFlags |= FS_LOCAL_PREFIX;
    } else {
	prefixFlags |= FS_EXPORTED_PREFIX;
    }
    (void)FsPrefixInstall(localName, (FsHandleHeader *)handlePtr,
			  FS_LOCAL_DOMAIN,  prefixFlags);

    /*
     * Finally mark the domain to indicate that if we go down hard,
     * clean recovery of this domain is impossible.
     */
    domainPtr->summaryInfoPtr->flags |= FS_DOMAIN_NOT_SAFE;
    status = FsWriteBackSummary(domainPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_FATAL, "FsAttachDisk: Summary write failed, status %x\n",
		    status);
    }
    domainPtr->flags = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalIOInit --
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
FsLocalIOInit(domainPtr, domainNumber)
    register	FsDomain	*domainPtr;
    int				domainNumber;
{
    ReturnStatus		status;
    FsCachedAttributes		attr;

    /*
     * Initialize the file handle used for raw I/O, i.e. for file descriptors,
     * the bitmaps, and indirect blocks.
     */

    Byte_Zero(sizeof(domainPtr->physHandle), (Address)&domainPtr->physHandle);
    domainPtr->physHandle.hdr.fileID.major = domainNumber;
    domainPtr->physHandle.hdr.fileID.minor = 0;
    domainPtr->physHandle.hdr.fileID.type = FS_LCL_FILE_STREAM;
    domainPtr->physHandle.descPtr = (FsFileDescriptor *)NIL;

    Byte_Zero(sizeof(attr), (Address)&attr);
    attr.lastByte = 0x7fffffff;
    FsCacheInfoInit(&domainPtr->physHandle.cacheInfo,
		    (FsHandleHeader *) &domainPtr->physHandle,
		    0, TRUE, &attr);

    status = FsLocalBlockAllocInit(domainPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Block Alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }

    status = FsFileDescAllocInit(domainPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "File Desc alloc init failed for domain %d\n",
		domainNumber);
	return(status);
    }
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * FsDetachDisk --
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
FsDetachDisk(prefixName)
    char	*prefixName;	/* Name that the disk is attached under. */
{
    FsHandleHeader	*hdrPtr;
    char		*lookupName;
    int			domainType;
    FsPrefix		*prefixPtr;
    FsFileID		rootID;
    int			domain;
    register FsDomain	*domainPtr;
    int			i;
    ReturnStatus	status;
    int			blocksLeft;

    /*
     * Find the domain to detach.
     */
    status = FsPrefixLookup(prefixName, 
		   FS_EXACT_PREFIX | FS_EXPORTED_PREFIX | FS_LOCAL_PREFIX,
		   rpc_SpriteID, &hdrPtr, &rootID, &lookupName, &domainType,
		   &prefixPtr);
    if (status != SUCCESS) {
	return(status);
    } else if (hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	return(GEN_INVALID_ARG);
    }
    domain = hdrPtr->fileID.major;
    domainPtr = FsDomainFetch(domain, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    /*
     * Recall dirty blocks from remote clients, and copy dirty file descriptors
     * into their cache blocks.  Once done, don't allow any more dirty
     * blocks to enter the cache.
     */
    FsGetAllDirtyBlocks(domain, TRUE);	
    status = FsHandleDescWriteBack(FALSE, -1);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsDetachDisk: handle write-back failed <%x>.\n",
	    status);
    }
    /*
     * Mark the domain and wait for other users of the domain to leave.
     */
    AddDomainFlags(domainPtr, FS_DOMAIN_GOING_DOWN);
    /*
     * Write all dirty blocks, bitmaps, etc. to disk and take the
     * domain down.
     */
    Fs_CacheWriteBack(-1, &blocksLeft, FALSE);	/* Write back the cache. */
    FsLocalDomainWriteBack(domain, FALSE, TRUE);/* Write back domain info. */
    AddDomainFlags(domainPtr, FS_DOMAIN_DOWN);
    domainPtr->refCount--;
    /*
     * We successfully brought down the disk, so mark it as OK.
     */
    domainPtr->summaryInfoPtr->flags &= ~FS_DOMAIN_NOT_SAFE;
    status = FsWriteBackSummary(domainPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_FATAL, "FsDetachDisk: Summary write failed, status %x\n",
		    status);
    }

    /*
     * Free up resources for the domain.
     */
    Mem_Free((Address)domainPtr->headerPtr);
    Mem_Free((Address)domainPtr->summaryInfoPtr);
    Mem_Free((Address)domainPtr->dataBlockBitmap);
    Mem_Free((Address)domainPtr->cylinders);
    for (i = 0; i < FS_NUM_FRAG_SIZES; i++) {
	List_Links	*fragList;
	FsFragment	*fragPtr;
	fragList = domainPtr->fragLists[i];
	while (!List_IsEmpty(fragList)) {
	    fragPtr = (FsFragment *)List_First(fragList);
	    List_Remove((List_Links *)fragPtr);
	    Mem_Free((Address)fragPtr);
	}
	Mem_Free((Address)fragList);
    }
    Mem_Free((Address)domainPtr->fileDescBitmap);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsLocalDomainWriteBack --
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
FsLocalDomainWriteBack(domain, shutdown, detach)
    int		domain;		/* Domain number, -1 means all domains */
    Boolean	shutdown;	/* TRUE if are syncing to shutdown the system.*/
    Boolean	detach;		/* TRUE if are writing back as part of 
				 * detaching the domain.  This is used to force 
				 * the domain fetch to work even if it is
				 * marked as down. */
{
    register	FsDomain	*domainPtr;
    int		firstDomain;
    register int lastDomain;
    register int i;

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
	lastDomain = FS_MAX_LOCAL_DOMAINS - 1;
	detach = FALSE;
    }
    for (i = firstDomain; i <= lastDomain; i++) {
	domainPtr = FsDomainFetch(i, detach);
	if (domainPtr != (FsDomain *) NIL) {
	    (void)FsWriteBackDataBlockBitmap(domainPtr);
	    (void)FsWriteBackFileDescBitmap(domainPtr);
	    if (shutdown) {
		domainPtr->summaryInfoPtr->flags &= ~FS_DOMAIN_NOT_SAFE;
	    }
	    (void)FsWriteBackSummary(domainPtr);
	    FsDomainRelease(i);
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
    FsDomain *domainPtr;
{
    int domainNumber;

    LOCK_MONITOR;

    if (domainPtr->summaryInfoPtr->domainNumber == -1) {
	while (fsDomainTable[domainTableIndex] != (FsDomain *)NIL) {
	    domainTableIndex++;
	}
	fsDomainTable[domainTableIndex] = domainPtr;
	domainNumber = domainTableIndex;
	domainTableIndex++;
    } else {
	domainNumber = domainPtr->summaryInfoPtr->domainNumber;
	fsDomainTable[domainNumber] = domainPtr;
    }

    UNLOCK_MONITOR;
    return(domainNumber);
}


/*
 *----------------------------------------------------------------------
 *
 * AddDomainFlags --
 *
 *	Add the given flags to the flags in the given domain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The flags in the domain are set.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
AddDomainFlags(domainPtr, flags)
    FsDomain	*domainPtr;
    int		flags;
{
    LOCK_MONITOR;

    domainPtr->flags |= flags;
    if (flags & FS_DOMAIN_GOING_DOWN) {
	/*
	 * Wait until we are the only user of the domain because 
	 * noone else but the cache block cleaner or us should be using this
	 * domain once we set this flag.
	 */
	while (domainPtr->refCount > 1) {
	    (void)Sync_Wait(&domainPtr->condition, FALSE);
	}
    }
    if (flags & FS_DOMAIN_DOWN) {
	if (domainPtr->refCount > 1) {
	    Sys_Panic(SYS_WARNING, "AddDomainFlags: Refcount > 1\n");
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * FsDomainFetch --
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
ENTRY FsDomain *
FsDomainFetch(domain, dontStop)
    int		domain;		/* Domain to fetch. */
    Boolean	dontStop;	/* Fetch this domain unless it has been
				 * totally detached. */
{
    register	FsDomain	*domainPtr;

    LOCK_MONITOR;

    if (domain < 0 || domain >= FS_MAX_LOCAL_DOMAINS) {
	Sys_Panic(SYS_WARNING, "FsDomainFetch, bad domain number <%d>\n",
	    domain);
	domainPtr = (FsDomain *)NIL;
    } else {
	domainPtr = fsDomainTable[domain];
    }
    if (domainPtr != (FsDomain *) NIL) {
	if (domainPtr->flags & FS_DOMAIN_DOWN) {
	    domainPtr = (FsDomain *) NIL;
	} else if (dontStop || !(domainPtr->flags & FS_DOMAIN_GOING_DOWN)) {
	    domainPtr->refCount++;
	} else {
	    domainPtr = (FsDomain *) NIL;
	}
    }

    UNLOCK_MONITOR;
    return(domainPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * FsDomainRelease --
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
FsDomainRelease(domainNum)
    int	domainNum;
{
    register	FsDomain	*domainPtr;

    LOCK_MONITOR;

    domainPtr = fsDomainTable[domainNum];
    if (domainPtr == (FsDomain *)NIL) {
	Sys_Panic(SYS_FATAL, "FsDomainRelease: NIL domain pointer\n");
    }

    domainPtr->refCount--;
    if (domainPtr->refCount < 0) {
	Sys_Panic(SYS_FATAL, "FsDomainRelease: Negative ref count on domain\n");
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
    register FsDiskHeader *diskHeaderPtr;
    register int index;
    register int checkSum;

    diskHeaderPtr = (FsDiskHeader *)buffer;
    if (diskHeaderPtr->magic == FS_DISK_MAGIC) {
	/*
	 * Check the checkSum which set so that an XOR of all the
	 * ints in the disk header comes out to FS_DISK_MAGIC also.
	 */
	checkSum = 0;
	for (index = 0 ; index < DEV_BYTES_PER_SECTOR ; index += sizeof(int)) {
	    checkSum ^= (int)buffer[index];
	}
	if (checkSum == FS_DISK_MAGIC) {
	    return(TRUE);
	} else {
	    Sys_Printf("IsSpriteLabel: checksum mismatch <%x>\n", checkSum);
	}
    }
    return(FALSE);
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
int
Fs_BlocksToSectors(fragNumber, data)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
{
    register FsGeometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (FsGeometry *)data;
    blockNumber		= fragNumber / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
    rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
    blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;

    sectorNumber = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		  geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		  rotationalSet +
		  geoPtr->blockOffset[blockNumber];
    sectorNumber += (fragNumber % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;

    return(sectorNumber);
}

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
void
Fs_BlocksToDiskAddr(fragNumber, data, diskAddrPtr)
    int fragNumber;	/* Fragment index to map into block index */
    ClientData data;	/* ClientData from the device info */
    Dev_DiskAddr *diskAddrPtr;
{
    register FsGeometry *geoPtr;
    register int sectorNumber;	/* The sector corresponding to the fragment */
    register int cylinder;	/* The cylinder number of the fragment */
    register int rotationalSet;	/* The rotational set with cylinder of frag */
    register int blockNumber;	/* The block number within rotational set */

    geoPtr 		= (FsGeometry *)data;
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
    sectorNumber	= geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		  	  rotationalSet + geoPtr->blockOffset[blockNumber] +
		    (fragNumber % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;
    diskAddrPtr->head	= sectorNumber / geoPtr->sectorsPerTrack;
    diskAddrPtr->sector = sectorNumber -
			  diskAddrPtr->head * geoPtr->sectorsPerTrack;
}

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

