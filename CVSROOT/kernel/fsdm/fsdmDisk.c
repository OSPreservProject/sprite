/* 
 * fsdmDisk.c --
 *
 *	Routines related to managing local disks domains.  Each partition of
 *	a local	disk (partitions are defined by a table on the disk header) is
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
#include "fsconsist.h"
#include "fsdm.h"
#include "fslcl.h"
#include "fsNameOps.h"
#include "fsio.h"
#include "fsprefix.h"
#include "devDiskLabel.h"
#include "dev.h"
#include "devFsOpTable.h"
#include "sync.h"
#include "rpc.h"
#include "fsioDevice.h"
#include "fsdmInt.h"
#include "string.h"
#include "fscache.h"


#include "ofs.h"
#include "lfs.h"


/*
 * A table of domains indexed by domain number.  For use by a server
 * to map from domain number to info about the domain.
 */
Fsdm_Domain *fsdmDomainTable[FSDM_MAX_LOCAL_DOMAINS];
static int domainTableIndex = 0;

Sync_Lock	domainTableLock = Sync_LockInitStatic("Fs:domainTableLock");
#define LOCKPTR (&domainTableLock)

/*
 * Data structure will list of registered disk storage managers and their
 * attach procedures.  This list is stored as an array and used in the
 * Fsdm_AttachDisk procedure.
 */

typedef struct StorageManagerList {
    char	*typeName;	/* Name of storage manager type. */
    ReturnStatus (*attachProc) _ARGS_((Fs_Device *devicePtr, 
			char *localName, int flags, int *domainNumPtr)); 
				/* Disk attach procedure. */
} StorageManagerList;

#define	MAX_STORAGE_MANAGER_TYPES 4
static StorageManagerList storageManagers[MAX_STORAGE_MANAGER_TYPES];

static int numStorageManagers = 0;	/* Number of storage managers. */
/*
 * Forward declarations.
 */
static Boolean OkToDetach _ARGS_((Fsdm_Domain *domainPtr));
static void MarkDomainDown _ARGS_((Fsdm_Domain *domainPtr));



/*
 *----------------------------------------------------------------------
 *
 * Fsdm_RegisterDiskManager --
 *
 *	Register the attach procedure of a disk storage manager. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Attach procedure added to list.
 *
 *----------------------------------------------------------------------
 */
void
Fsdm_RegisterDiskManager(typeName, attachProc)
    char	*typeName;	/* Storage manger type name. */
    ReturnStatus (*attachProc)  _ARGS_((Fs_Device *devicePtr, 
			char *localName, int flags, int *domainNumPtr));
			/* Disk attach procedure. */
{
    LOCK_MONITOR;

    if (numStorageManagers >= MAX_STORAGE_MANAGER_TYPES) {
	UNLOCK_MONITOR;
	panic("Fsdm_RegisterDiskManager: Can't register %s, %s (%d)\n",
		typeName, "Too many disk storage managers registered", 
			numStorageManagers);
	return;
    }
    storageManagers[numStorageManagers].typeName = typeName;
    storageManagers[numStorageManagers].attachProc = attachProc;
    numStorageManagers++;
    UNLOCK_MONITOR;
}

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
    int flags;			/* FS_ATTACH_READ_ONLY|FS_ATTACH_LOCAL 
				 * or FS_DEFAULT_DOMAIN */
{
    ReturnStatus status;		/* Error code */
    register Fsdm_Domain *domainPtr;	/* Top level info for the domain stored
					 * on the device */
    Fsio_FileIOHandle	*handlePtr;	/* Reference to file handle for root */
    Fs_FileID		fileID;
    int		domainNum;		/* Domain number. */
    int		prefixFlags;		/* For installing the prefix */
    int		devFlags;		/* Device flags. */
    int 	useFlags;		/* Use flags. */
    int		i;

    /*
     * Open the raw disk device so we can grub around in the header info.
     */
    useFlags = (flags | FS_ATTACH_READ_ONLY) ? FS_READ : (FS_READ|FS_WRITE);
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].open)
	    (devicePtr, useFlags, (Fs_NotifyToken) NIL, &devFlags);
    if (status != SUCCESS) {
	return(status);
    }


    /*
     * Attempt to attach the disk under the same domain number each time.
     * This is required if clients are to be able to re-open files.
     */
    domainNum = -1;
    for (i = 0; i < numStorageManagers; i++) {
	status = storageManagers[i].attachProc(devicePtr, localName, flags, 
		&domainNum);
#ifdef lint
	status = Ofs_AttachDisk(devicePtr, localName, flags, &domainNum);
	status = Lfs_AttachDisk(devicePtr, localName, flags, &domainNum);
#endif /* lint */
	if (domainNum >= 0) {
	    break;
	}
    }
    if (status != SUCCESS) {
	return status;
    }
    domainPtr = Fsdm_DomainFetch(domainNum, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FAILURE);
    }

    fileID.type = FSIO_LCL_FILE_STREAM;
    fileID.serverID = rpc_SpriteID;
    fileID.major = domainNum;
    fileID.minor = FSDM_ROOT_FILE_NUMBER;
    /*
     * Now that the block I/O is set up we can read the file descriptor
     * of the root directory of the domain.
     */
    status = Fsio_LocalFileHandleInit(&fileID, localName, 
				(Fsdm_FileDescriptor *) NIL, FALSE, &handlePtr);
    if (status != SUCCESS) {
	printf( "Fsdm_AttachDisk: %s - can't get root file handle (%x)\n",
		localName, status);
	domainPtr->flags |= FSDM_DOMAIN_DOWN;
	return(status);
    }
    Fsutil_HandleUnlock(handlePtr);
    /*
     * The attach will succeed from this point, so print out info..
     */
    printf("%s: devType %#x devUnit %#x\n", localName,
	    devicePtr->type, devicePtr->unit);
    /*
     * Install a prefix for the domain.  We always import it so that
     * we can get to the disk locally.  Then we either keep the domain
     * private or export it depending on the flags argument.
     */
    prefixFlags = FSPREFIX_IMPORTED;
    if (flags & FS_ATTACH_LOCAL) {
	prefixFlags |= FSPREFIX_LOCAL;
	printf("local ");
    } else {
	prefixFlags |= FSPREFIX_EXPORTED;
	printf("exported ");
    }
    (void)Fsprefix_Install(localName, (Fs_HandleHeader *) handlePtr,
			  FS_LOCAL_DOMAIN,  prefixFlags);

    if (flags & FS_ATTACH_READ_ONLY) {
	printf("read only\n");
    } else {
	printf("\n");
    }

    /*
     * Make sure a name hash table exists now that we have a disk attached.
     */
    Fslcl_NameHashInit();
    Fsdm_DomainRelease(domainNum);
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
    ReturnStatus	status;

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
	printf( "Fsdm_DetachDisk: %s - handle write-back failed <%x>.\n",
	    domainPtr->domainPrefix, status);
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
    status = domainPtr->domainOpsPtr->detachDisk(domainPtr);
    MarkDomainDown(domainPtr);
    return(status);
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
	    (void ) domainPtr->domainOpsPtr->writeBack(domainPtr, shutdown);
#ifdef lint
	    (void) Ofs_DomainWriteBack(domainPtr, shutdown);
	    (void) Lfs_DomainWriteBack(domainPtr, shutdown);
#endif /* lint */
	    Fsdm_DomainRelease(i);
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Fsdm_InstallDomain --
 *
 *	Install a Fsdm_Domain structure for a newly attached local domain and
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
ENTRY ReturnStatus
Fsdm_InstallDomain(domainNumber, serverID, prefixName, flags, domainPtrPtr)
    int		domainNumber;	/* Domain nubmer to use. */
    int		serverID;	/* Server ID from disk. */
    char	*prefixName;	/* Prefix of domain. */
    int		flags;		/* Domain flags. */
    Fsdm_Domain **domainPtrPtr;	/* OUT: Return domain pointer. */
{
    Fsdm_Domain *oldDomainPtr, *domainPtr;

    LOCK_MONITOR;

    if (domainNumber == -1) {
	while ((domainTableIndex < FSDM_MAX_LOCAL_DOMAINS) && 
		(fsdmDomainTable[domainTableIndex] != (Fsdm_Domain *)NIL)) {
	    domainTableIndex++;
	}
	if (domainTableIndex == FSDM_MAX_LOCAL_DOMAINS) {
	    printf("Fsdm_InstallDomain: too many local domains.\n");
	    domainNumber = -1;
	    UNLOCK_MONITOR;
	    return(FS_DOMAIN_UNAVAILABLE);
	}
	domainNumber = domainTableIndex;
	domainTableIndex++;
    } 
    if ((domainNumber < 0) || (domainNumber >= FSDM_MAX_LOCAL_DOMAINS)) {
	printf("Fsdm_InstallDomain: domain number (%d) for %s out of range.\n",
		domainNumber, prefixName);
	UNLOCK_MONITOR;
	return(FS_DOMAIN_UNAVAILABLE);
    } 

    oldDomainPtr = fsdmDomainTable[domainNumber];
    if (oldDomainPtr != (Fsdm_Domain *)NIL) {
	if (!(oldDomainPtr->flags & FSDM_DOMAIN_DOWN)) {
	    printf("Fsdm_AttachDisk: %s already attached at domain %d\n",
			oldDomainPtr->domainPrefix, domainNumber);
	    *domainPtrPtr = oldDomainPtr;
	    UNLOCK_MONITOR;
	    return(FS_FILE_BUSY);
	} 
	domainPtr = oldDomainPtr;
    } else {
	domainPtr = (Fsdm_Domain *) malloc(sizeof(Fsdm_Domain));
	bzero((char *) domainPtr, sizeof(Fsdm_Domain));
	fsdmDomainTable[domainNumber] = domainPtr;
    }
    domainPtr->domainPrefix = malloc(strlen(prefixName)+1);
    (void)strcpy(domainPtr->domainPrefix, prefixName);

    domainPtr->domainNumber = domainNumber;
    domainPtr->flags = 0;

    *domainPtrPtr = domainPtr;

     if (rpc_SpriteID == 0) {
	/*
	 * Use the spriteID on the 1st disk if we don't know it by now.
	 * This is the last resort.  Usually reverse arp or the hook
	 * in the RPC protocol have established our ID.  If there are
	 * no other Sprite hosts running on the network , however,
	 * then this code will execute.
	 */
	printf(
	  "Fsdm_InstallDomain: setting rpc_SpriteID to 0x%x from disk header\n",
		    serverID);
	if (serverID <= 0) {
	    panic("Bad sprite ID\n");
	}
	rpc_SpriteID = serverID;
    }

    if (flags & FS_DEFAULT_DOMAIN) {
	domainPtr->flags = FSDM_DOMAIN_ATTACH_BOOT;
    } else {
	domainPtr->flags = 0;
    }
    UNLOCK_MONITOR;

    return SUCCESS;
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
	    domainPtr->domainPrefix);
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
	panic( "Fsdm_DomainRelease: Negative ref count on domain %d\n", 
			domainNum);
    }
    if (domainPtr->refCount == 0) {
	Sync_Broadcast(&domainPtr->condition);
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_IsSunLabel --
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
Boolean
Fsdm_IsSunLabel(buffer)
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
 * Fsdm_IsSpriteLabel --
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
Boolean
Fsdm_IsSpriteLabel(buffer)
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
 * Fsdm_IsDecLabel --
 *
 *	Poke around in the input buffer and see if it looks like
 *	a Dec format disk label.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
Fsdm_IsDecLabel(buffer)
    Address buffer;	/* Buffer containing zero'th sector */
{
    register Dec_DiskLabel *decLabelPtr;

    decLabelPtr = (Dec_DiskLabel *)buffer;
    if (decLabelPtr->magic == DEC_LABEL_MAGIC) {
	if (decLabelPtr->spriteMagic == FSDM_DISK_MAGIC &&
		decLabelPtr->version == DEC_LABEL_VERSION) {
	    return TRUE;
	} else {
	    printf("Dec label version mismatch: %x vs %x\n",
		    decLabelPtr->version, DEC_LABEL_VERSION);
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
    Ofs_Init();
    Lfs_Init();
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
    status = domainPtr->domainOpsPtr->rereadSummary(domainPtr);
#ifdef lint
    status = Lfs_RereadSummaryInfo(domainPtr);
    status = Ofs_RereadSummaryInfo(domainPtr);
#endif /* lint */
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileBlockRead --
 *
 *	Read in a cache block.  
 *
 * Results:
 *	The results of the disk read.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the bufSize parameter.  The blockPtr->blockSize is modified to
 *	reflect how much data was actually read in.  The unused part
 *	of the block is filled with zeroes so that higher levels can
 *	always assume the block has good stuff in all parts of it.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsdm_FileBlockRead(hdrPtr, blockPtr, remoteWaitPtr)
    Fs_HandleHeader	*hdrPtr;	/* Handle on a local file. */
    Fscache_Block	*blockPtr;	/* Cache block to read in.  This assumes
					 * the blockNum, blockAddr (buffer area)
					 * and blockSize are set.  This modifies
					 * blockSize if less bytes were read
					 * because of EOF. */
    Sync_RemoteWaiter *remoteWaitPtr;	/* NOTUSED */
{
    register Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    register	Fsdm_Domain	 *domainPtr;
    ReturnStatus		 status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *) NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    status = domainPtr->domainOpsPtr->fileBlockRead(domainPtr, handlePtr,
				blockPtr);
#ifdef lint
    status = Lfs_FileBlockRead(domainPtr,handlePtr,blockPtr);
    status = Ofs_FileBlockRead(domainPtr,handlePtr,blockPtr);
#endif /* lint */
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileBlockWrite --
 *
 *	Write out a cache block.  
 *
 * Results:
 *	The return code from the driver, or FS_DOMAIN_UNAVAILABLE if
 *	the domain has been un-attached.
 *
 * Side effects:
 *	The device write.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsdm_FileBlockWrite(hdrPtr, blockPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* I/O handle for the file. */
    Fscache_Block *blockPtr;	/* Cache block to write out. */
    int		flags;		/* IGNORED */
{
    register Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    register	Fsdm_Domain	 *domainPtr;
    ReturnStatus		status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = domainPtr->domainOpsPtr->fileBlockWrite(domainPtr, handlePtr, 
		blockPtr);

#ifdef lint
    status = Lfs_FileBlockWrite(domainPtr,handlePtr,blockPtr);
    status = Ofs_FileBlockWrite(domainPtr,handlePtr,blockPtr);
#endif /* lint */
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileTrunc --
 *
 *	Truncate a file.  
 *
 * Results:
 *	The return code from the driver, or FS_DOMAIN_UNAVAILABLE if
 *	the domain has been un-attached.
 *
 * Side effects:
 *	The device write.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsdm_FileTrunc(hdrPtr, size, delete)
    Fs_HandleHeader *hdrPtr;	/* I/O handle for the file. */
    int		    size;	/* Size to truncate to. */
    Boolean	    delete;	/* True if the file is being deleted. */
{
    Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *) hdrPtr;
    register	Fsdm_Domain	 *domainPtr;
    ReturnStatus		status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = domainPtr->domainOpsPtr->fileTrunc(domainPtr, handlePtr, size, 
				delete);

#ifdef lint
    status = Lfs_FileTrunc(domainPtr, handlePtr, size, delete);
    status = Ofs_FileTrunc(domainPtr, handlePtr, size, delete);
#endif /* lint */
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DomainInfo --
 *
 *	Return info about the given domain.
 *
 * Results:
 *	Error  if can't get to the domain.
 *
 * Side effects:
 *	The domain info struct is filled in.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_DomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID		*fileIDPtr;
    Fs_DomainInfo	*domainInfoPtr;
{
    int		domain = fileIDPtr->major;
    ReturnStatus status;
    Fsdm_Domain	*domainPtr;

    if (domain >= FSDM_MAX_LOCAL_DOMAINS) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    domainPtr = Fsdm_DomainFetch(domain, FALSE);
    if (domainPtr == (Fsdm_Domain *) NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = domainPtr->domainOpsPtr->domainInfo(domainPtr, domainInfoPtr);
#ifdef lint
    status = Lfs_DomainInfo(domainPtr, domainInfoPtr);
    status = Ofs_DomainInfo(domainPtr, domainInfoPtr);
#endif /* lint */

    Fsdm_DomainRelease(domain);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_BlockAllocate --
 *
 *	Allocate disk space for the given file.  This routine only allocates
 *	one block beginning at offset and going for numBytes.   If 
 *	offset + numBytes crosses a block boundary then a panic will occur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The file descriptor is modified to contain pointers to the allocated
 *	blocks.  Also *blockAddrPtr is set to the block that was allocated.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_BlockAllocate(hdrPtr, offset, numBytes, flags, blockAddrPtr, newBlockPtr)
    register Fs_HandleHeader *hdrPtr;	/* Local file handle. */
    int 		offset;		/* Offset to allocate at. */
    int 		numBytes;	/* Number of bytes to allocate. */
    int			flags;		/* FSCACHE_DONT_BLOCK */
    int			*blockAddrPtr; 	/* Disk address of block allocated. */
    Boolean		*newBlockPtr;	/* TRUE if there was no block allocated
					 * before. */
{
    Fsdm_Domain		*domainPtr;	/* Domain of file. */
    register Fsio_FileIOHandle *handlePtr;	/* Local file handle. */
    ReturnStatus status;

    handlePtr = (Fsio_FileIOHandle *) hdrPtr;
    if (offset / FS_BLOCK_SIZE != (offset + numBytes - 1) / FS_BLOCK_SIZE) {
	panic("Fsdm_BlockAllocate - ALlocation spans block boundries\n");
	return FAILURE;
    }
    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = domainPtr->domainOpsPtr->blockAlloc(domainPtr, handlePtr,
		offset, numBytes, flags, blockAddrPtr, newBlockPtr);
#ifdef lint
    status = Lfs_BlockAllocate(domainPtr, handlePtr, offset,  numBytes, flags,
				blockAddrPtr, newBlockPtr);
    status = Ofs_BlockAllocate(domainPtr, handlePtr, offset,  numBytes, flags,
				blockAddrPtr, newBlockPtr);
#endif /* lint */

    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FindFileType --
 *
 *	Map from flags in the handle to a constant corresponding to
 *	the file type for the kernel.  
 *
 * Results:
 *	The value corresponding to the file's type is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Fsdm_FindFileType(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;	/* File to determine type of */
{
    switch (cacheInfoPtr->attr.userType) {
	case FS_USER_TYPE_TMP:
	    return(FSUTIL_FILE_TYPE_TMP);
	case FS_USER_TYPE_SWAP:
	    return(FSUTIL_FILE_TYPE_SWAP);
	case FS_USER_TYPE_OBJECT:
	    return(FSUTIL_FILE_TYPE_DERIVED);
	case FS_USER_TYPE_BINARY:
	    return(FSUTIL_FILE_TYPE_BINARY);
	default:
            return(FSUTIL_FILE_TYPE_OTHER);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileDescWriteBack --
 *
 *	Force the file descriptor for the handle to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File descriptor block forced to disk.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsdm_FileDescWriteBack(handlePtr, doWriteBack)
     Fsio_FileIOHandle *handlePtr;	/* Handle that points
					 * to descriptor to write back. */
    Boolean		doWriteBack;	/* Do a cache write back, not only
					 * a store into the cache block. */
{
#ifdef NOTDEF
    Fs_HandleHeader	*hdrPtr = (Fs_HandleHeader *) handlePtr;
#endif NOTDEF
    register Fsdm_FileDescriptor	*descPtr;
    register Fsdm_Domain		*domainPtr;
    register ReturnStatus     	status = SUCCESS;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;
    if (descPtr == (Fsdm_FileDescriptor *)NIL) {
	if ((handlePtr->cacheInfo.flags & FSCACHE_FILE_GONE) == 0) {
	    panic("Fsdm_FileDescWriteBack: no descriptor for \"%s\" (continuable)\n",
		Fsutil_HandleName(handlePtr));
	}
	status = FS_FILE_REMOVED;
	goto exit;
    }
    /*
     * If the handle times differ from the descriptor times then force
     * them out to the descriptor.
     */
    if (descPtr->accessTime < handlePtr->cacheInfo.attr.accessTime) {
	descPtr->accessTime = handlePtr->cacheInfo.attr.accessTime;
#ifdef NOTDEF
	 printf("Fsdm_FileDescWriteBack, access time changed <%d,%d> \"%s\"\n",
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		Fsutil_HandleName(hdrPtr));
#endif
	descPtr->flags |= FSDM_FD_DIRTY;
    }
    if (descPtr->dataModifyTime < handlePtr->cacheInfo.attr.modifyTime) {
	descPtr->dataModifyTime = handlePtr->cacheInfo.attr.modifyTime;
#ifdef NOTDEF
	 printf("Fsdm_FileDescWriteBack, mod time changed <%d,%d> \"%s\"\n",
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		Fsutil_HandleName(hdrPtr));
#endif
	descPtr->flags |= FSDM_FD_DIRTY;
    }
    if (descPtr->dataModifyTime > descPtr->descModifyTime) {
	descPtr->descModifyTime = descPtr->dataModifyTime;
#ifdef NOTDEF
	 printf("Fsdm_FileDescWriteBack, desc time changed <%d,%d> \"%s\"\n",
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		Fsutil_HandleName(hdrPtr));
#endif
	descPtr->flags |= FSDM_FD_DIRTY;
    }
    if (descPtr->flags & FSDM_FD_DIRTY) {
	status =  Fsdm_FileDescStore(handlePtr, doWriteBack);
	if (status != SUCCESS) {
	    printf("Fsdm_FileDescWriteBack: Could not put desc <%d,%d> into cache\n",
		    handlePtr->hdr.fileID.major,
		    handlePtr->hdr.fileID.minor);
	}
    }
exit:
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DirOpStart --
 *
 *	Mark the start of a directory operation.
 *
 * Results:
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Fsdm_DirOpStart(opFlags, dirHandlePtr, dirOffset, name, nameLen, fileNumber,
		type, fileDescPtr)
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    Fsio_FileIOHandle *dirHandlePtr;	/* Handle of directory being operated
					 * on. */
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. -1 if offset
				 * is not known. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    int		type;		/* Type of the object being operated on. */
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object being operated on
				       * before operation starts. */
{
    ClientData			clientData;
    register Fsdm_Domain	*domainPtr;
    domainPtr = Fsdm_DomainFetch(dirHandlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return((ClientData) NIL);
    }
    opFlags |= FSDM_LOG_START_ENTRY;
    clientData = domainPtr->domainOpsPtr->dirOpStart(domainPtr, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);
#ifdef lint
    clientData = Lfs_DirOpStart(domainPtr, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);

    clientData = Ofs_DirOpStart(domainPtr, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);
#endif /* lint */

    Fsdm_DomainRelease(dirHandlePtr->hdr.fileID.major);
    return(clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_DirOpEnd --
 *
 *	Mark the end of a directory operation.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Fsdm_DirOpEnd(opFlags, dirHandlePtr, dirOffset, name, nameLen, fileNumber,
		type, fileDescPtr, clientData, status)
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    Fsio_FileIOHandle *dirHandlePtr;	/* Handle of directory being operated
					 * on. */
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. -1 if offset
				 * is not known. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    int		type;		/* Type of the object being operated on. */
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object being operated on
				       * before operation starts. */
    ClientData	clientData;	/* ClientData as returned by DirOpStart. */
    ReturnStatus status;	/* Return status of the operation, SUCCESS if
				 * operation succeeded. FAILURE otherwise. */
{
    register Fsdm_Domain		*domainPtr;

    domainPtr = Fsdm_DomainFetch(dirHandlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return;
    }
    opFlags |= FSDM_LOG_END_ENTRY;
    domainPtr->domainOpsPtr->dirOpEnd(domainPtr, clientData, status, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);
#ifdef lint
    Lfs_DirOpEnd(domainPtr, clientData, status, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);

    Ofs_DirOpEnd(domainPtr, clientData, status, opFlags, 
		 name, nameLen, fileNumber, fileDescPtr,
		 dirHandlePtr->hdr.fileID.minor, dirOffset, 
		 dirHandlePtr->descPtr);
#endif /* lint */
    Fsdm_DomainRelease(dirHandlePtr->hdr.fileID.major);
    return;
}


