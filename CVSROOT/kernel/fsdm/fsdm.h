/*
 * fsDisk.h --
 *
 *	Definitions related to the storage of a filesystem on a disk.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSDISK
#define _FSDISK

#include "dev.h"

/*
 * A disk is partitioned into domains that are managed separately.
 * Each domain takes up an even number of cylinders.
 * An array of FsDiskPartition's is kept on the disk to define how the
 * disk is divided into domains.
 *
 * FS_NUM_DISK_PARTS defines how many different domains there could be
 *	on a disk.  Generally, not all the domains are defined.  
 */
#define FS_NUM_DISK_PARTS	8

typedef struct FsDiskPartition {
    int firstCylinder;	/* The first cylinder in the partition. */
    int numCylinders;	/* The number of cylinders in the partition.  Set
			 * this to zero for unused partitions. */
} FsDiskPartition;

/*
 * The first few blocks of each domain are reserved.  They contain a copy
 * of the Disk Header, a copy of the boot program, and finally Domain
 * Header information.  The Disk Header defines the division of the disk's
 * cylinders into domains,  and the layout of the rest of the reserved
 * blocks.  The Disk Header is replicated on the zero'th sector of each
 * domain. For the Sun implementation, the boot program is expected to
 * start in sector #1 and contain at most 15 sectors.  The boot program on
 * the zero'th cylinder of the disk is used automatically, although other
 * boot program locations can be specified manually.
 *
 *      NOTE: we are temporarily using Sun's format of the Disk Header,
 *      not the following typedef.  Sun's label is defined in
 *      "../sun/sunDiskLabel.h".  We assume that sector zero contains a
 *      Sun format label, sectors 1 through 16 contain a boot program.
 *      Sector SUN_SUMMARY_SECTOR contains an FsSummaryInfo structure.
 *      Sector SUN_DOMAIN_SECTOR contains an FsDomainHeader structure.
 */

typedef struct FsDiskHeader {
    char asciiLabel[128];	/* Human readable string used for manufacturer's
				 * model number and redudant geometry info */
    /*
     * Padding is used to shove the following data down so the check sum
     * occurs at the end of the sector.  The 10 in the declaration indicates
     * how may integer fields there are in this struct.
     */
    char pad[DEV_BYTES_PER_SECTOR - 128 -
	     (12 + FS_NUM_DISK_PARTS * 2) * sizeof(int)];
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
    FsDiskPartition map[FS_NUM_DISK_PARTS];	/* Partition map */
    int checkSum;		/* Checksum such that an XOR of all the ints
				 * in the sector results in the same thing
				 * as the magic number */
} FsDiskHeader;

#define FS_DISK_MAGIC	(unsigned int)0xD15CFEBA	/* 'disk fever' */



/*
 * The geometry of a disk is a parameter to the disk block allocation routine.
 * It is stored in disk in the Domain header so that different configurations
 * on the same disk can be tried out and compared.
 *
 * The following parameters define array sizes in the FsGeometry struct.
 *
 * FS_MAX_ROT_POSITIONS defines how many different rotational positions are
 * possible for a filesystem block.  An Eagle Drive, for example, has 23
 * rotational positions.  There are 46 sectors per track.  That means that
 * 5 4K filesystem blocks fit on a track and the 6th spills over onto the
 * next track.  This offsets all the 4K blocks on the next track by 1K.
 * This continues for 4 tracks after which 23 whole blocks have been
 * packed in.  The set of 23 blocks is called a ``rotational set''.
 * Also, because the Eagle has 20 heads, and each rotational set occupies
 * 4 tracks, there are 5 rotational sets per cylinder.
 *
 * FS_MAX_TRACKS_PER_SET defines how many tracks a rotational set can
 * take up.
 */
#define FS_MAX_ROT_POSITIONS	32
#define FS_MAX_TRACKS_PER_SET	10

typedef struct FsGeometry {
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
    int		blockOffset[FS_MAX_ROT_POSITIONS];	/* This keeps the
					 * starting sector number for each
					 * rotational position.  This table
					 * is computed by the makeFilesystem
					 * user program */
    int		sortedOffsets[FS_MAX_ROT_POSITIONS];	/* An ordered set of
					 * the rotational positions */
    /*
     * Add more data after here so we have to reformat the disk less often.
     */
} FsGeometry;

/*
 * A disk is partitioned into areas that each store a domain.  Each domain
 * contains a DomainHeader that describes how the blocks of the domain are
 * used.  The layout information takes into account the blocks that are
 * reserved for the copy of the Disk Header and the boot program.
 */
typedef struct FsDomainHeader {
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
    FsGeometry	geometry;	/* Used by the allocation routines and
				 * by the block IO routines */
} FsDomainHeader;

#define FS_DOMAIN_MAGIC	(unsigned int)0xF8E7D6C5

/*
 * FS_NUM_DOMAIN_SECTORS is the standard number of sectors taken
 * up by the domain header.
 */
#define FS_NUM_DOMAIN_SECTORS	((sizeof(FsDomainHeader)-1) / DEV_BYTES_PER_SECTOR + 1)


/*
 * ONE sector of summary information is kept on disk.  This records things
 * like the number of free blocks and free file descriptors.  This info
 * is located just before the domain header.
 */
typedef struct FsSummaryInfo {
    int		numFreeKbytes;		/* Free space in kbytes, not blocks */
    int		numFreeFileDesc;	/* Number of free file descriptors */
    int		state;			/* Unused. */
    char	domainPrefix[64];	/* Last prefix used for the domain */
    int		domainNumber;		/* The domain number of the domain
					 * under which this file system was
					 * last mounted. */
    int		flags;			/* Flags defined below. */
    int		attachSeconds;		/* Time the disk was attached */
    int		detachSeconds;		/* Time the disk was off-lined.  This
					 * is the fsTimeInSeconds that the
					 * system was shutdown or the disk
					 * was detached.  If the domain is
					 * marked NOT_SAFE then this field
					 * is undefined, but attachTime is ok
					 * as long as TIMES_VALID is set. */
} FsSummaryInfo;

/*
 * Flags for summary info structure.
 *	FS_DOMAIN_NOT_SAFE	Set during normal operation. This is unset
 *		when we know we	are shutting down cleanly and the data
 *		structures on the disk partition (domain) are ok.
 *	FS_DOMAIN_ATTACHED_CLEAN	Set if the initial attach found the
 *		disk marked 'safe'
 *	FS_DOMAIN_TIMES_VALID	If set then the attach/detachSeconds fields
 *		are valid.
 */
#define	FS_DOMAIN_NOT_SAFE		0x1
#define FS_DOMAIN_ATTACHED_CLEAN	0x2
#define	FS_DOMAIN_TIMES_VALID		0x4


/*
 * A File Descriptor is kept on disk for every file in a domain.  It
 * contains administrative information and also the indexing structure
 * used to access the file's data blocks.
 */

#define FS_NUM_DIRECT_BLOCKS	10
#define FS_NUM_INDIRECT_BLOCKS	3
#define	FS_INDICES_PER_BLOCK	1024

#define FS_MAX_FILE_DESC_SIZE	128
#define FS_FILE_DESC_PER_BLOCK	(FS_BLOCK_SIZE / FS_MAX_FILE_DESC_SIZE)

typedef struct FsFileDescriptor {
    unsigned short magic;/* FS_FD_MAGIC, for disk consistency check */
    short flags;	/* FS_FD_FREE, FS_FD_ALLOC, FS_FD_RESERVED */
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

    int direct[FS_NUM_DIRECT_BLOCKS];
    int indirect[FS_NUM_INDIRECT_BLOCKS];
    int numKbytes;	/* The number of KiloBytes acutally allocated towards
			 * the file on disk.  This accounts for fragments
			 * and indirect blocks. */
    int version;	/* Version number of the handle for the file.  Needed
			 * on disk for recovery purposes (client re-open */
} FsFileDescriptor;

/*
 * Magic number and flag definitions for file descriptors.
 *	FS_FD_FREE	The file descriptor is unused
 *	FS_FD_ALLOC	The file descriptor is used for a file.
 *	FS_FD_RESERVED	The file descriptor is reserved and not for use.
 *	FS_FD_DIRTY	The file descriptor has been modified since the
 *			last time that it was written to disk.
 */
#define FS_FD_MAGIC	(unsigned short)0xF1D0
#define FS_FD_FREE	0x1
#define FS_FD_ALLOC	0x2
#define FS_FD_RESERVED	0x4
#define FS_FD_DIRTY	0x8

/*
 * The special index value FS_NIL_INDEX for direct[] and indirect[]
 * means there is no block allocated for that index.
 */ 
#define FS_NIL_INDEX	-1
/*
 * The bad block file, the root directory of a domain and the lost and found 
 * directory have well known file numbers.
 */
#define FS_BAD_BLOCK_FILE_NUMBER	1
#define FS_ROOT_FILE_NUMBER		2
#define FS_LOST_FOUND_FILE_NUMBER	3
/*
 * The lost and found directory is preallocated and is of a fixed size. Define
 * its size in 4K blocks here.
 */
#define	FS_NUM_LOST_FOUND_BLOCKS	2

/*
 * A directory entry:
 */
typedef struct FsDirEntry {
    int fileNumber;		/* Index of the file descriptor for the file. */
    short recordLength;		/* How many bytes this directory entry is */
    short nameLength;		/* The length of the name in bytes */
    char fileName[FS_MAX_NAME_LENGTH+1];	/* The name itself */
} FsDirEntry;
/*
 *	FS_DIR_BLOCK_SIZE	Directory's grow in multiples of this constant,
 *		and records within a directory don't cross directory blocks.
 *	FS_DIR_ENTRY_HEADER	The size of the header of a FsDirEntry;
 *	FS_REC_LEN_GRAIN	The number of bytes in a directory record
 *				are rounded up to a multiple of this constant.
 */
#define FS_DIR_BLOCK_SIZE	512
#define FS_DIR_ENTRY_HEADER	(sizeof(int) + 2 * sizeof(short))
#define FS_REC_LEN_GRAIN	4

/*
 * FsDirRecLength --
 *	This computes the number of bytes needed for a directory entry.
 *	The argument should be the return of the String_Length function,
 *	ie, not include the terminating null in the count.
 */
#define FsDirRecLength(stringLength) \
    (FS_DIR_ENTRY_HEADER + \
    ((stringLength / FS_REC_LEN_GRAIN) + 1) * FS_REC_LEN_GRAIN)

#endif _FSDISK
