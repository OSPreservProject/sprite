/* 
 * fsdm.h --
 *
 *	Disk management module -- Definitions related to the storage of 
 *	filesystems on a disk.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSDM
#define _FSDM

#include <dev.h>
#include <fslcl.h>
#include <fsioFile.h>
#include <fscache.h>

/*
 * A disk is partitioned into domains that are managed separately.
 * Each domain takes up an even number of cylinders.
 * An array of Fsdm_DiskPartition's is kept on the disk to define how the
 * disk is divided into domains.
 *
 * FSDM_NUM_DISK_PARTS defines how many different domains there could be
 *	on a disk.  Generally, not all the domains are defined.  
 */
#define FSDM_NUM_DISK_PARTS	8

typedef struct Fsdm_DiskPartition {
    int firstCylinder;	/* The first cylinder in the partition. */
    int numCylinders;	/* The number of cylinders in the partition.  Set
			 * this to zero for unused partitions. */
} Fsdm_DiskPartition;

/*
 * The first few blocks of each domain are reserved.  They contain a copy
 * of the Disk Header, a copy of the boot program, and finally Domain
 * Header information.  The Disk Header defines the division of the disk's
 * cylinders into domains,  and the layout of the rest of the reserved
 * blocks.  The Disk Header is replicated on the zero'th sector of each
 * domain. For the Sun implementation, the boot program is expected to
 * start in sector #1. The boot program on
 * the zero'th cylinder of the disk is used automatically, although other
 * boot program locations can be specified manually.
 *
 */

#define FSDM_MAX_BOOT_SECTORS	128
#define FSDM_BOOT_SECTOR_INC	16

typedef struct Fsdm_DiskHeader {
    char asciiLabel[128];	/* Human readable string used for manufacturer's
				 * model number and redudant geometry info */
    /*
     * Padding is used to shove the following data down so the check sum
     * occurs at the end of the sector.  The 10 in the declaration indicates
     * how may integer fields there are in this struct.
     */
    char pad[DEV_BYTES_PER_SECTOR - 128 -
	     (12 + FSDM_NUM_DISK_PARTS * 2) * sizeof(int)];
    unsigned int magic;		/* Magic number used for consistency check */
    int numCylinders;		/* The number of cylinders on the disk */
    int numAltCylinders;	/* # of alternates used for bad blocks */
    int numHeads;		/* # of surfaces on the disk */
    int numSectors;		/* # of sectors per track */
    int bootSector;		/* The starting sector of the boot program.
				 * This is usually 1. */
    int numBootSectors;		/* The number of sectors in the boot program.
				 * This is usually 15. */
    int summarySector;		/* Index of sector used for summary info */
    int domainSector;		/* The sector where the domain header starts.*/
    int numDomainSectors;	/* The number of sectors taken up by the
				 * domain header */
    int partition;		/* Index of the partition that this copy of
				 * the Disk Header is on.  Each partition has
				 * a copy of the Disk Header */
    Fsdm_DiskPartition map[FSDM_NUM_DISK_PARTS];	/* Partition map */
    int checkSum;		/* Checksum such that an XOR of all the ints
				 * in the sector results in the same thing
				 * as the magic number */
} Fsdm_DiskHeader;

#define FSDM_DISK_MAGIC	(unsigned int)0xD15CFEBA	/* 'disk fever' */




/*
 * A File Descriptor is kept on disk for every file in a domain.  It
 * contains administrative information and also the indexing structure
 * used to access the file's data blocks.
 */

#define FSDM_NUM_DIRECT_BLOCKS	10
#define FSDM_NUM_INDIRECT_BLOCKS	3
#define	FSDM_INDICES_PER_BLOCK	1024

#define FSDM_MAX_FILE_DESC_SIZE	128
#define FSDM_FILE_DESC_PER_BLOCK	(FS_BLOCK_SIZE / FSDM_MAX_FILE_DESC_SIZE)

typedef struct Fsdm_FileDescriptor {
    unsigned short magic;/* FSDM_FD_MAGIC, for disk consistency check */
    short flags;	/* FSDM_FD_FREE, FSDM_FD_ALLOC, FSDM_FD_RESERVED */
    short fileType;	/* FS_REGULAR, FS_DIRECTORY, FS_PIPE, FS_DEVICE,
			 * FS_SYMLINK, FS_RMTLINK */
    short permissions;	/* 9 permission bits plus flags for set user ID
			 * upon execution */
    int uid;		/* ID of owner */
    int gid;		/* Group ID of owner */
    int lastByte;	/* The number of bytes in the file */
    int lastByteXtra;	/* (Some day we may have 64 bit sizes?) */
    int firstByte;	/* For named pipes, offset of the first valid byte */
    int userType;	/* Information about what sort of user file it is. */
    int numLinks;	/* Number of directory references to the file */
    int devServerID;	/* ID of the host that controls the device */
    short devType;	/* For devices, their type.  For others this is the
			 * type of disk the file is stored on */
    unsigned short devUnit;	/* For devices, their unit number.  For others,
			 * the unit indicates the disk partition */
    /*
     * All times in seconds since Jan 1 1970, Greenwich time.
     */

    int createTime;	/* Time the file was created. */
    int accessTime;	/* Time of last access.  This is not updated by
			 * directory traversals. */
    int descModifyTime;	/* Time of last modification to the file descriptor */
    int dataModifyTime;	/* Time of last modification to the file data */

    /*
     * Pointers to the data blocks of the file.   The pointers are really
     * indexes into the array of blocks stored in the data block section
     * of a partition.  The direct array contains the indexes of the first
     * several blocks of the files.  The indirect indexes are interpreted
     * as follows.  The first indirect index is the index of a block that
     * contains 1 K indexes of data blocks.  This is called a singly-indirect
     * block.  The second indirect index is the index of a block that
     * contains 1 K indexes of singly-indirect blocks.  This is called
     * a doubly-indirect block.  Finally, the third indirect index is the index
     * of a block that contains 1 K indexes of doubly-indirect blocks.
     * Each data block contains 4Kbytes, so this indexing scheme supports
     * files up to 40K + 4Meg + 4Gig + 4Pig bytes.
     *
     * The values of the direct and indirect indexes are indexes of
     * fragments, ie. 1k pieces.  All the but the last index point to the
     * beginning of a filesystem block, ie. 4K.  The last valid direct
     * block may point to a fragment, and fragments can start on 1K
     * boundaries.
     */

    int direct[FSDM_NUM_DIRECT_BLOCKS];
    int indirect[FSDM_NUM_INDIRECT_BLOCKS];
    int numKbytes;	/* The number of KiloBytes acutally allocated towards
			 * the file on disk.  This accounts for fragments
			 * and indirect blocks. */
    int version;	/* Version number of the handle for the file.  Needed
			 * on disk for recovery purposes (client re-open */
} Fsdm_FileDescriptor;

/*
 * Magic number and flag definitions for file descriptors.
 *	FSDM_FD_FREE	The file descriptor is unused
 *	FSDM_FD_ALLOC	The file descriptor is used for a file.
 *	FSDM_FD_RESERVED The file descriptor is reserved and not for use.
 *	FSDM_FD_DIRTY	The file descriptor has been modified since the
 *			last time that it was written to disk.
 *  	FSDM_FD_PERMISSIONS_DIRTY  Premissions field (permissions) have be
 *				    changed since the descriptor last written.
 *  	FSDM_FD_SIZE_DIRTY         Size fields (lastByte lastByteXtra,
 *				   firstByte, numBlocks) have been changed
 *				   size the descriptor was last written.
 *  	FSDM_FD_USERTYPE_DIRTY     The user type field has changed since the
 *				   descriptor was last written.
 *  	FSDM_FD_LINKS_DIRTY  	   The number of links has changed since the
 *				   descriptor was written.
 *	FSDM_FD_ACCESSTIME_DIRTY   The access time has changed since the
 *				   descriptor was written.
 *	FSDM_FD_MODTIME_DIRTY	   The modify time has changed since the
 *				   descriptor was written.
 *	FSDM_FD_INDEX_DIRTY	   The file index fields (direct, indirect) 
 *				   have changed since the descriptor was
 *				   written.
 *	FSDM_FD_VERSION_DIRTY	   The file version number changed since the
 *				   descriptor was written.
 *	FSDM_FD_OTHERS_DIRTY	   Some other (not listed above) field has
 *				   changed since the descriptor was written.
 *
 */

#define FSDM_FD_MAGIC	(unsigned short)0xF1D0
#define FSDM_FD_FREE	0x1
#define FSDM_FD_ALLOC	0x2
#define FSDM_FD_RESERVED	0x4
#define FSDM_FD_DIRTY		    0xFF8
#define	FSDM_FD_PERMISSIONS_DIRTY   0x010	
#define	FSDM_FD_SIZE_DIRTY	    0x020
#define	FSDM_FD_USERTYPE_DIRTY	    0x040
#define	FSDM_FD_LINKS_DIRTY	    0x080
#define	FSDM_FD_ACCESSTIME_DIRTY    0x100
#define	FSDM_FD_MODTIME_DIRTY	    0x200
#define	FSDM_FD_INDEX_DIRTY	    0x400
#define	FSDM_FD_VERSION_DIRTY	    0x800
#define	FSDM_FD_OTHERS_DIRTY	    0x008

/*
 * The special index value FSDM_NIL_INDEX for direct[] and indirect[]
 * means there is no block allocated for that index.
 */ 
#define FSDM_NIL_INDEX	-1

/*
 * Structure for each domain.
 */

typedef struct Fsdm_Domain {
    char	*domainPrefix;	  /* Prefix of this domain. */
    int		domainNumber;	   /* Number of this domain. */
    int		flags;		  /* Flags defined below. */		
    int		refCount;	  /* Number of active users of the domain. */
    Sync_Condition condition;	  /* Condition to wait on. */
    Fscache_Backend *backendPtr;  /* Cache backend for this domain. */
    struct Fsdm_DomainOps *domainOpsPtr;
				  /* Domain specific data and routines. */
    ClientData	clientData;	  /* Domain specific info. */
} Fsdm_Domain;
/*
 * Structure defining the domain specific routines and data.
 */
typedef struct Fsdm_DomainOps {
    ReturnStatus (*attachDisk) _ARGS_((Fs_Device *devicePtr, 
					char *localName, int flags, 
					int *domainNumPtr));
				     /* Attach and install the domain from 
				      * the specified disk partition. */
    ReturnStatus (*detachDisk) _ARGS_((Fsdm_Domain *domainPtr));
				     /* Detach  the domain from 
				      * the specified disk partition. */
    ReturnStatus (*writeBack) _ARGS_((Fsdm_Domain *domainPtr, 
				      Boolean shutdown));   
				     /* Writeback the internal data structures
				      * for the specified domain. */
    ReturnStatus (*rereadSummary) _ARGS_((Fsdm_Domain *domainPtr));
				    /* Reread the domain info from disk. */

    ReturnStatus (*domainInfo) _ARGS_ ((Fsdm_Domain *domainPtr, 
				Fs_DomainInfo *domainInfoPtr));
				     /* Return a Fs_DomainInfo for the 
				      * specified domain.  */
    ReturnStatus (*blockAlloc) _ARGS_((Fsdm_Domain *domainPtr, 
				      Fsio_FileIOHandle *handlePtr, int offset,
				      int numBytes, int flags, 
				      int *blockAddrPtr, Boolean *newBlockPtr));
					/* Allocate a block for a file. */
    ReturnStatus (*getNewFileNumber) _ARGS_((Fsdm_Domain *domainPtr, 
					    int dirFileNum, 
					    int *fileNumberPtr));

					/* Allocate a file number of a new
					 * file.  */
    ReturnStatus (*freeFileNumber)  _ARGS_((Fsdm_Domain *domainPtr, 
					    int fileNumber));
					/* Deallocate a file number. */

    ReturnStatus (*fileDescInit) _ARGS_((Fsdm_Domain *domainPtr, 
					int fileNumber, int type, 
					int permissions, int uid, int gid, 
					Fsdm_FileDescriptor *fileDescPtr));
				     /* Initialize a new file descriptor. */

    ReturnStatus (*fileDescFetch)  _ARGS_((Fsdm_Domain *domainPtr, 
					   int fileNumber, 
					   Fsdm_FileDescriptor *fileDescPtr));
				     /* Fetch a file descriptor from disk. */

    ReturnStatus (*fileDescStore)  _ARGS_((Fsdm_Domain *domainPtr, 
					   Fsio_FileIOHandle *handlePtr, 
					   int fileNumber, 
					   Fsdm_FileDescriptor *fileDescPtr,
					   Boolean forceOut));
				     /* Store a file descriptor back into it's
				      * disk block. */
    ReturnStatus (*fileBlockRead) _ARGS_((Fsdm_Domain *domainPtr, 
					 Fsio_FileIOHandle *handlePtr, 
					 Fscache_Block *blockPtr));

    ReturnStatus (*fileBlockWrite) _ARGS_((Fsdm_Domain *domainPtr, 
					   Fsio_FileIOHandle *handlePtr,
					   Fscache_Block *blockPtr));

    ReturnStatus (*fileTrunc)  _ARGS_((Fsdm_Domain *domainPtr, 
					Fsio_FileIOHandle *handlePtr, 
					int size, Boolean delete));
				     /* Truncate the file to the specifed 
				      * length. */
    ClientData	(*dirOpStart) _ARGS_((Fsdm_Domain *domainPtr, int opFlags,
				char *name, int nameLen, int fileNumber, 
				Fsdm_FileDescriptor *fileDescPtr, 
				int dirFileNumber, int dirOffset, 
				Fsdm_FileDescriptor *dirFileDescPtr));
				     /* Directory operation start. */	

    void	(*dirOpEnd) _ARGS_((Fsdm_Domain *domainPtr,
				    ClientData clientData, ReturnStatus status,
				    int opFlags, char *name, int nameLen,
				    int fileNumber, 
				    Fsdm_FileDescriptor *fileDescPtr, 
				    int dirFileNumber,	int dirOffset, 
				    Fsdm_FileDescriptor *dirFileDescPtr));   
				     /* Directory operation end. */	

} Fsdm_DomainOps;



/*
 * Domain flags used for two stage process of detaching a domain:
 *
 *    FSDM_DOMAIN_GOING_DOWN	This domain is being detached.
 *    FSDM_DOMAIN_DOWN		The domain is detached.
 *    FSDM_DOMAIN_ATTACH_BOOT   The domain was attached by the kernel
 *				during boot.
 */
#define	FSDM_DOMAIN_GOING_DOWN	0x1
#define	FSDM_DOMAIN_DOWN 		0x2
#define FSDM_DOMAIN_ATTACH_BOOT		0x4

/*
 * Types of indexing.  Order is important here because the indirect and
 * double indirect types can be used to index into the indirect block 
 * pointers in the file descriptor.
 */

#define	FSDM_INDIRECT		0 
#define	FSDM_DBL_INDIRECT		1
#define	FSDM_DIRECT		2


/*
 * Whether or not to keep information about file I/O by user file type.
 */
extern Boolean fsdmKeepTypeInfo;

/*
 * The bad block file, the root directory of a domain and the lost and found 
 * directory have well known file numbers.
 */
#define FSDM_BAD_BLOCK_FILE_NUMBER	1
#define FSDM_ROOT_FILE_NUMBER		2
#define FSDM_LOST_FOUND_FILE_NUMBER	3

/*
 * Directry change log operations and flags.
 *	Operations:
 * FSDM_LOG_CREATE		Creating a new object in a directory.	
 * FSDM_LOG_UNLINK		Unlinking an object from a directory.	
 * FSDM_LOG_LINK		Linking to an existing object.	
 * FSDM_LOG_RENAME_DELETE	Deleting an object as part of a rename.	
 * FSDM_LOG_RENAME_LINK		Linking to an object as part of a rename.
 * FSDM_LOG_RENAME_UNLINK	Unlinking an object as part of a rename.
 * FSDM_LOG_OP_MASK		Mask out the log operation.
 * 
 * Flags:
 *
 * FSDM_LOG_START_ENTRY		Start of change entry
 * FSDM_LOG_END_ENTRY		End of change entry
 * FSDM_LOG_STILL_OPEN		File is still open after last unlink.
 */

#define	FSDM_LOG_CREATE		1
#define	FSDM_LOG_UNLINK		2
#define	FSDM_LOG_LINK		3
#define	FSDM_LOG_RENAME_DELETE	4
#define	FSDM_LOG_RENAME_LINK	5
#define	FSDM_LOG_RENAME_UNLINK	6
#define	FSDM_LOG_OP_MASK	0xff

#define	FSDM_LOG_START_ENTRY	0x100
#define	FSDM_LOG_END_ENTRY	0x200
#define	FSDM_LOG_STILL_OPEN	0x1000


/*
 * Declarations for the file descriptor allocation routines.
 */
extern ReturnStatus Fsdm_FileDescInit _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber, int type, int permissions, int uid, int gid, 
		Fsdm_FileDescriptor *fileDescPtr));
extern ReturnStatus Fsdm_FileDescFetch _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber, Fsdm_FileDescriptor *fileDescPtr));
extern ReturnStatus Fsdm_FileDescStore _ARGS_((Fsio_FileIOHandle *handlePtr,
		Boolean forceOut));

extern ReturnStatus Fsdm_GetNewFileNumber _ARGS_((Fsdm_Domain *domainPtr, 
		int dirFileNum, int *fileNumberPtr));
extern ReturnStatus Fsdm_FreeFileNumber _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber));
extern ReturnStatus Fsdm_FileDescWriteBack _ARGS_((Fsio_FileIOHandle *handlePtr,		Boolean doWriteBack));
extern ReturnStatus Fsdm_FileTrunc _ARGS_((Fs_HandleHeader *hdrPtr, 
		int size, Boolean delete));
extern ReturnStatus Fsdm_BlockAllocate _ARGS_((Fs_HandleHeader *hdrPtr, 
		int offset, int numBytes, int flags, int *blockAddrPtr, 
		Boolean *newBlockPtr));


extern ReturnStatus Fsdm_UpdateDescAttr _ARGS_((Fsio_FileIOHandle *handlePtr, 
		Fscache_Attributes *attrPtr, int dirtyFlags));

/*
 * Routines for attaching/detaching disk.
 */
extern ReturnStatus Fsdm_AttachDiskByHandle _ARGS_((
		Fs_HandleHeader *ioHandlePtr, char *localName, int flags));
extern ReturnStatus Fsdm_AttachDisk _ARGS_((Fs_Device *devicePtr, 
		char *localName, int flags));
extern ReturnStatus Fsdm_DetachDisk _ARGS_((char *prefixName));

/*
 * Routines to manipulate domains.
 */
extern Fsdm_Domain *Fsdm_DomainFetch _ARGS_((int domain, Boolean dontStop));
extern void Fsdm_DomainRelease _ARGS_((int domainNum));
extern ReturnStatus Fsdm_InstallDomain _ARGS_((int domainNumber, 
			int serverID, char *prefixName, int flags, 
			Fsdm_Domain **domainPtrPtr));
extern ReturnStatus Fsdm_DomainInfo _ARGS_((Fs_FileID *fileIDPtr, 
			Fs_DomainInfo *domainInfoPtr));
extern void Fsdm_DomainWriteBack _ARGS_((int domain, Boolean shutdown, 
			Boolean detach));
extern ReturnStatus Fsdm_RereadSummaryInfo _ARGS_((char *prefixName));
extern ReturnStatus Fsdm_FileBlockRead _ARGS_((Fs_HandleHeader *hdrPtr,
			Fscache_Block *blockPtr, 
			Sync_RemoteWaiter *remoteWaitPtr));
extern ReturnStatus Fsdm_FileBlockWrite _ARGS_((Fs_HandleHeader *hdrPtr, 
			Fscache_Block *blockPtr, int flags));

/*
 * Directory log routines.
 */
extern ClientData Fsdm_DirOpStart _ARGS_((int opFlags, 
		Fsio_FileIOHandle *dirHandlePtr, int dirOffset, char *name, 
		int nameLen, int fileNumber, int type, 
		Fsdm_FileDescriptor *fileDescPtr));
extern void Fsdm_DirOpEnd _ARGS_((int opFlags, Fsio_FileIOHandle *dirHandlePtr,
		int dirOffset, char *name, int nameLen, int fileNumber,
		int type, Fsdm_FileDescriptor *fileDescPtr,
		ClientData clientData, ReturnStatus status));

/*
 * Miscellaneous
 */
extern int Fsdm_FindFileType _ARGS_((Fscache_FileInfo *cacheInfoPtr));
extern void Fsdm_Init _ARGS_((void));
extern Boolean Fsdm_IsSunLabel _ARGS_((Address buffer));
extern Boolean Fsdm_IsSpriteLabel _ARGS_((Address buffer));
extern Boolean Fsdm_IsDecLabel _ARGS_((Address buffer));
extern void Fsdm_RegisterDiskManager _ARGS_((char *typeName, 
		ReturnStatus (*attachProc)(Fs_Device *devicePtr,
					   char *localName, int flags, 
					   int *domainNumPtr)));
#endif _FSDM
