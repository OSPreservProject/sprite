/*
 * fsdm.h --
 *
 *	Definitions related to the storage of a filesystem on a disk.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSDM
#define _FSDM

#include "dev.h"
#include "fslcl.h"
#include "fsioFile.h"

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
 * FSDM_NUM_DOMAIN_SECTORS is the standard number of sectors taken
 * up by the domain header.
 */
#define FSDM_NUM_DOMAIN_SECTORS	((sizeof(Fsdm_DomainHeader)-1) / DEV_BYTES_PER_SECTOR + 1)


/*
 * ONE sector of summary information is kept on disk.  This records things
 * like the number of free blocks and free file descriptors.  This info
 * is located just before the domain header.
 */
#define FSDM_SUMMARY_PREFIX_LENGTH	64
typedef struct Fsdm_SummaryInfo {
    int		numFreeKbytes;		/* Free space in kbytes, not blocks */
    int		numFreeFileDesc;	/* Number of free file descriptors */
    int		state;			/* Unused. */
    char	domainPrefix[FSDM_SUMMARY_PREFIX_LENGTH];
					/* Last prefix used for the domain */
    int		domainNumber;		/* The domain number of the domain
					 * under which this file system was
					 * last mounted. */
    int		flags;			/* Flags defined below. */
    int		attachSeconds;		/* Time the disk was attached */
    int		detachSeconds;		/* Time the disk was off-lined.  This
					 * is the fsutil_TimeInSeconds that the
					 * system was shutdown or the disk
					 * was detached.  If the domain is
					 * marked NOT_SAFE then this field
					 * is undefined, but attachTime is ok
					 * as long as TIMES_VALID is set. */
    int		fixCount;		/* Number of consecutive times that 
					 * fscheck has found an error in this
					 * domain. Used to prevent infinite
					 * looping.
					 */
    char pad[DEV_BYTES_PER_SECTOR -
	     (8 * sizeof(int) + FSDM_SUMMARY_PREFIX_LENGTH)];
} Fsdm_SummaryInfo;

/*
 * Flags for summary info structure.
 *	FSDM_DOMAIN_NOT_SAFE	Set during normal operation. This is unset
 *		when we know we	are shutting down cleanly and the data
 *		structures on the disk partition (domain) are ok.
 *	FSDM_DOMAIN_ATTACHED_CLEAN	Set if the initial attach found the
 *		disk marked 'safe'
 *	FSDM_DOMAIN_TIMES_VALID	If set then the attach/detachSeconds fields
 *		are valid.
 *	FSDM_DOMAIN_JUST_CHECKED Set if the disk was just checked by fscheck
 *		and doesn't need to be rechecked upon reboot.  This is only
 *		useful for the root partition, since that is the only one
 *		that requires a reboot.  In theory only the 
 *		FSDM_DOMAIN_NOT_SAFE should be needed, but we don't yet
 *		trust the filesystem to shut down cleanly.
 */
#define	FSDM_DOMAIN_NOT_SAFE		0x1
#define FSDM_DOMAIN_ATTACHED_CLEAN	0x2
#define	FSDM_DOMAIN_TIMES_VALID		0x4
#define FSDM_DOMAIN_JUST_CHECKED	0x8


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
 *	FSDM_FD_RESERVED	The file descriptor is reserved and not for use.
 *	FSDM_FD_DIRTY	The file descriptor has been modified since the
 *			last time that it was written to disk.
 */
#define FSDM_FD_MAGIC	(unsigned short)0xF1D0
#define FSDM_FD_FREE	0x1
#define FSDM_FD_ALLOC	0x2
#define FSDM_FD_RESERVED	0x4
#define FSDM_FD_DIRTY	0x8

/*
 * The special index value FSDM_NIL_INDEX for direct[] and indirect[]
 * means there is no block allocated for that index.
 */ 
#define FSDM_NIL_INDEX	-1


/*
 * Stuff for block allocation 
 */

#define	FSDM_NUM_FRAG_SIZES	3

/*
 * The bad block file, the root directory of a domain and the lost and found 
 * directory have well known file numbers.
 */
#define FSDM_BAD_BLOCK_FILE_NUMBER	1
#define FSDM_ROOT_FILE_NUMBER		2
#define FSDM_LOST_FOUND_FILE_NUMBER	3

/*
 * The lost and found directory is preallocated and is of a fixed size. Define
 * its size in 4K blocks here.
 */
#define	FSDM_NUM_LOST_FOUND_BLOCKS	2

/*
 * Structure to keep statistics about each cylinder.
 */

typedef struct Fsdm_Cylinder {
    int	blocksFree;	/* Number of blocks free in this cylinder. */
} Fsdm_Cylinder;



/*
 * The geometry of a disk is a parameter to the disk block allocation routine.
 * It is stored in disk in the Domain header so that different configurations
 * on the same disk can be tried out and compared.
 *
 * The following parameters define array sizes in the Fsdm_Geometry struct.
 *
 * FSDM_MAX_ROT_POSITIONS defines how many different rotational positions are
 * possible for a filesystem block.  An Eagle Drive, for example, has 23
 * rotational positions.  There are 46 sectors per track.  That means that
 * 5 4K filesystem blocks fit on a track and the 6th spills over onto the
 * next track.  This offsets all the 4K blocks on the next track by 1K.
 * This continues for 4 tracks after which 23 whole blocks have been
 * packed in.  The set of 23 blocks is called a ``rotational set''.
 * Also, because the Eagle has 20 heads, and each rotational set occupies
 * 4 tracks, there are 5 rotational sets per cylinder.
 *
 * FSDM_MAX_TRACKS_PER_SET defines how many tracks a rotational set can
 * take up.
 */
#define FSDM_MAX_ROT_POSITIONS	32
#define FSDM_MAX_TRACKS_PER_SET	10

/*
 * There is a new mapping available for SCSI disks.  In this mapping we
 * ignore rotational sets altogether and pack as many blocks as possible
 * into a cylinder. The field rotSetsPerCyl is overloaded to allow us
 * to be backwards compatible. If rotSetsPerCyl == FSDM_SCSI_MAPPING
 * then we are using the new mapping.  The other fields relating to
 * rotational sets and block offsets are ignored.
 */

#define FSDM_SCSI_MAPPING -1

typedef struct Fsdm_Geometry {
    /*
     * Fundamental disk geometry that cannot be varied.
     */
    int		sectorsPerTrack;/* The number of sectors per track */
    int		numHeads;	/* The number of surfaces on the disk */
    /*
     * Filesystem blocks take up several disk sectors, and filesystem
     * blocks on different surfaces may be skewed relative to each other
     * because a whole number of filesystem blocks generally does not fit
     * on one track.  (This property is exploited when looking for blocks
     * that are rotationaly optimal with respect to each other.)  The
     * skewing pattern is repeated after a certain number of filesystem
     * blocks.  The pattern is contrained to fit on a whole number of
     * tracks, and a whole number of the patterns has to fit in one
     * cylinder.  This means that there may be unused sectors at the end
     * of each set of filesystem blocks.
     */
    int		blocksPerRotSet;	/* The number of distinct rotational
					 * positions for filesystem blocks */
    int		tracksPerRotSet;	/* The number of disk tracks taken
					 * up by a rotational set.  Used to
					 * bounce from one set to another
					 * and to map from filesystem block
					 * indexes to disk sectors */
    int		rotSetsPerCyl;		/* The number of rotational sets in a
					 * cylinder.  There are numRotPositions
					 * filesystem blocks in each set */
    int		blocksPerCylinder;	/* This is the product of blocksPerSet
					 * and numRotationSets */
    /*
     * The following diagram shows a rotational set for a disk with 46
     * 512 byte sectors per track.  8 disk sectors are needed for each
     * filesystem block, and this allows 5 3/4 blocks per trask.
     * This means that 23 filesystem blocks make up a rotational set
     * that occupies 4 tracks.
	----------------------------------------------------
	|..1.....|..2.....|..3.....|..4.....|..5.....|..6...	track 1
	----------------------------------------------------
	..|..7.....|..8.....|..9.....|..10....|..11....|..12	track 2
	----------------------------------------------------
	....|..13....|..14....|..15....|..16....|..17....|..	track 3
	----------------------------------------------------
	.18...|..19....|..20....|..21....|..22....|..23....|	track 4
	----------------------------------------------------
     * In order to facilitate rotationally optimal allocation the
     * rotational positions are sorted by increasing offset.  In the
     * above example, the sorted ordering is (1, 7, 13, 19, 2, 8...)
     */
    int		blockOffset[FSDM_MAX_ROT_POSITIONS];	/* This keeps the
					 * starting sector number for each
					 * rotational position.  This table
					 * is computed by the makeFilesystem
					 * user program */
    int		sortedOffsets[FSDM_MAX_ROT_POSITIONS];	/* An ordered set of
					 * the rotational positions */
    /*
     * Add more data after here so we have to reformat the disk less often.
     */
} Fsdm_Geometry;


/*
 * A disk is partitioned into areas that each store a domain.  Each domain
 * contains a DomainHeader that describes how the blocks of the domain are
 * used.  The layout information takes into account the blocks that are
 * reserved for the copy of the Disk Header and the boot program.
 */
typedef struct Fsdm_DomainHeader {
    unsigned int magic;		/* magic number for consistency check */
    int		firstCylinder;	/* Disk relative number of the first cylinder
				 * in the domain.  This is redundant with
				 * respect to the complete partition map
				 * kept in the Disk Header */
    int		numCylinders;	/* The number of cylinders in the domain.
				 * Also redundant with Disk Header */
    Fs_Device	device;		/* Device identifier of the domain passed to
				 * the device driver block IO routines.
				 * Two fields are valid on disk: the serverID
				 * records the rpc_SpriteID of the server,
				 * and the unit number indicates which partition
				 * in the superblock this header corresponds
				 * to.  This is needed because more than one
				 * partition can start at the same spot on
				 * disk (of course, only one is valid to use).
				 * The device type on disk is ignored */
    /*
     * An array of FsDescriptors is kept on the disk and an accompanying
     * bitmap is used to keep track of free and allocated file descriptors.
     */
    int		fdBitmapOffset;	/* The block offset of the bitmap used to
				 * keep track of free FileDescriptors */
    int		fdBitmapBlocks;	/* The number of blocks in the bitmap */
    int		fileDescOffset;	/* The block offset of the array of fds.
				 * The number of blocks in the array comes
				 * from numFileDesc */
    int		numFileDesc;	/* The number of FsDescriptors in the domain.
				 * This is an upper limit on the number of
				 * files that be kept in the domain */ 
    /*
     * A large bitmap is used to record the status of all the data blocks
     * in the domain.
     */
    int		bitmapOffset;	/* The block number of the start of bitmap */
    int		bitmapBlocks;	/* Number of blocks used to store bitmap */
    /*
     * The data blocks take up the rest of the domain.
     */
    int		dataOffset;	/* The block number of the start of data */
    int		dataBlocks;	/* Number of data blocks */
    int		dataCylinders;	/* Number of cylinders containing data blocks */
    /*
     * Disk geometery parameters are used map from block indexes to
     * disk sectors, and also to optimally allocate blocks.
     */
    Fsdm_Geometry	geometry;	/* Used by the allocation routines and
				 * by the block IO routines */
} Fsdm_DomainHeader;

#define FSDM_DOMAIN_MAGIC	(unsigned int)0xF8E7D6C5

/*
 * Structure for each domain.
 */

typedef struct Fsdm_Domain {
    Fsio_FileIOHandle	physHandle;	/* Handle to use to read and write
					 * physical blocks. */
    Fsdm_DomainHeader *headerPtr; 		/* Disk information for the domain. */
    /*
     * Disk summary information.
     */
    Fsdm_SummaryInfo *summaryInfoPtr;
    int		  summarySector;
    /*
     * Data block allocation.
     */
    unsigned char *dataBlockBitmap;	/* The per domain data block bit map.*/
    int		bytesPerCylinder;	/* The number of bytes in the bit map
					 * for each cylinder. */
    Fsdm_Cylinder	*cylinders;		/* Pointer to array of cylinder
					 * information. */
    List_Links	*fragLists[FSDM_NUM_FRAG_SIZES];	/* Lists of fragments. */
    Sync_Lock	dataBlockLock;		/* Lock for data block allocation. */
    int		minKFree;		/* The minimum number of kbytes that 
					 * must be free at all times. */
    /*
     * File descriptor allocation.
     */
    unsigned char *fileDescBitmap;	/* The per domain file descriptor bit
					 * map.*/
    Sync_Lock	fileDescLock;		/* Lock for file descriptor
					 * allocation. */
    int		flags;		/* Flags defined below. */		
    int		refCount;	/* Number of active users of the domain. */
    Sync_Condition condition;	/* Condition to wait on. */
} Fsdm_Domain;

/*
 * Domain flags used for two stage process of detaching a domain:
 *
 *    FSDM_DOMAIN_GOING_DOWN	This domain is being detached.
 *    FSDM_DOMAIN_DOWN		The domain is detached.
 */
#define	FSDM_DOMAIN_GOING_DOWN	0x1
#define	FSDM_DOMAIN_DOWN 		0x2

/*
 * Types of indexing.  Order is important here because the indirect and
 * double indirect types can be used to index into the indirect block 
 * pointers in the file descriptor.
 */

#define	FSDM_INDIRECT		0 
#define	FSDM_DBL_INDIRECT		1
#define	FSDM_DIRECT		2

typedef	int	Fsdm_BlockIndexType;

/*
 * Structure to keep information about the indirect and doubly indirect
 * blocks used in indexing.
 */

typedef struct Fsdm_IndirectInfo {
    	Fscache_Block 	*blockPtr;	/* Pointer to indirect block. */
    	int		index;		/* An index into the indirect block. */
    	Boolean	 	blockDirty;	/* TRUE if the block has been
					   modified. */
    	int	 	deleteBlock;	/* FSCACHE_DELETE_BLOCK bit set if should 
					   delete the block when are
					   done with it. */
} Fsdm_IndirectInfo;

/*
 * Structure used when going through the indexing structure of a file.
 */

typedef struct Fsdm_BlockIndexInfo {
    Fsdm_BlockIndexType	 indexType;	/* Whether chasing direct, indirect,
					   or doubly indirect blocks. */
    int		blockNum;		/* Block that is being read, written,
					   or allocated. */
    int		lastDiskBlock;		/* The disk block for the last file
					   block. */
    int		*blockAddrPtr;		/* Pointer to pointer to block. */
    int		directIndex;		/* Index into direct block pointers. */
    Fsdm_IndirectInfo indInfo[2];		/* Used to keep track of the two 
					   indirect blocks. */
    int		 flags;			/* Flags defined below. */
    Fsdm_Domain	*domainPtr;		/* Domain that the file is in. */
} Fsdm_BlockIndexInfo;

/*
 * Flags for the index structure.
 *
 *     FSDM_ALLOC_INDIRECT_BLOCKS		If an indirect is not allocated then
 *					allocate it.
 *     FSDM_DELETE_INDIRECT_BLOCKS	After are finished with an indirect
 *					block if it is empty delete it.
 *     FSDM_DELETING_FROM_FRONT		Are deleting blocks from the front
 *					of the file.
 *     FSDM_DELETE_EVERYTHING		The file is being truncated to length
 *					0 so delete all blocks and indirect
 *					blocks.
 *	FSCACHE_DONT_BLOCK		Don't block on a full cache.  The cache
 *					can get so full of dirty blocks it can
 *					prevent the fetching of needed indirect
 *					blocks.  Our caller can deal with this
 *					if it sets FSCACHE_DONT_BLOCK, otherwise
 *					we'll wait for a free cache block.
 *					(FSCACHE_DONT_BLOCK value is used as
 *					 a convenience - it gets passed to
 *					 Fscache_FetchBlock)
 */

#define	FSDM_ALLOC_INDIRECT_BLOCKS	0x01
#define	FSDM_DELETE_INDIRECT_BLOCKS	0x02
#define	FSDM_DELETING_FROM_FRONT	0x04
#define	FSDM_DELETE_EVERYTHING		0x08
/*resrv FSCACHE_DONT_BLOCK		0x40000 */

/*
 * Whether or not to keep information about file I/O by user file type.
 */
extern Boolean fsdmKeepTypeInfo;


/*
 * Declarations for the file descriptor allocation routines.
 */

extern ReturnStatus	Fsdm_FileDescInit();
extern ReturnStatus	Fsdm_FileDescFetch();
extern ReturnStatus	Fsdm_FileDescStore();
extern ReturnStatus	Fsdm_FileDescFree();
extern ReturnStatus	Fsdm_FileDescTrunc();
extern ReturnStatus 	Fsdm_GetNewFileNumber();
extern ReturnStatus	Fsdm_FileDescWriteBack();

extern ReturnStatus	Fsdm_BlockAllocate();
extern ReturnStatus	Fsdm_FindFileType();
extern ReturnStatus	Fsdm_FreeFileNumber();


/*
 * Declarations for the local Domain data block allocation routines and 
 * indexing routines.
 */

extern	ReturnStatus	Fsdm_GetFirstIndex();
extern	ReturnStatus	Fsdm_GetNextIndex();
extern	void		Fsdm_EndIndex();

/*
 * Routines for attaching/detaching disk.
 */
extern  ReturnStatus	Fsdm_AttachDisk();
extern  ReturnStatus	Fsdm_AttachDiskByHandle();
extern  ReturnStatus	Fsdm_DetachDisk();
/*
 * Routines to manipulate domains.
 */
extern	Fsdm_Domain	*Fsdm_DomainFetch();
extern	void		Fsdm_DomainRelease();
extern  ReturnStatus	Fsdm_DomainInfo();
extern void 	     Fsdm_DomainWriteBack();
extern ReturnStatus	Fsdm_RereadSummaryInfo();
/*
 * Miscellaneous
 */
extern	void		Fsdm_RecordDeletionStats();
extern  void		Fsdm_Init();

#endif _FSDM
