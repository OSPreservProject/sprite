/*
 * ofs.h --
 *
 *	Definitions related to the storage of the original Sprite filesystem
 *	on a disk.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _OFS
#define _OFS

#ifdef KERNEL
#include <dev.h>
#include <fslcl.h>
#include <fsioFile.h>
#include <fsdm.h>
#include <devBlockDevice.h>
#else
#include <kernel/dev.h>
#include <kernel/fslcl.h>
#include <kernel/fsioFile.h>
#include <kernel/fsdm.h>
#include <kernel/devBlockDevice.h>
#endif

/*
 * ONE sector of summary information is kept on disk.  This records things
 * like the number of free blocks and free file descriptors.  This info
 * is located just before the domain header.
 */
#define OFS_SUMMARY_PREFIX_LENGTH	64
typedef struct Ofs_SummaryInfo {
    int		numFreeKbytes;		/* Free space in kbytes, not blocks */
    int		numFreeFileDesc;	/* Number of free file descriptors */
    int		state;			/* Unused. */
    char	domainPrefix[OFS_SUMMARY_PREFIX_LENGTH];
					/* Last prefix used for the domain */
    int		domainNumber;		/* The domain number of the domain
					 * under which this file system was
					 * last mounted. */
    int		flags;			/* Flags defined below. */
    int		attachSeconds;		/* Time the disk was attached */
    int		detachSeconds;		/* Time the disk was off-lined.  This
					 * is the Fsutil_TimeInSeconds that the
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
	     (8 * sizeof(int) + OFS_SUMMARY_PREFIX_LENGTH)];
} Ofs_SummaryInfo;

/*
 * Flags for summary info structure.
 *	OFS_DOMAIN_NOT_SAFE	Set during normal operation. This is unset
 *		when we know we	are shutting down cleanly and the data
 *		structures on the disk partition (domain) are ok.
 *	OFS_DOMAIN_ATTACHED_CLEAN	Set if the initial attach found the
 *		disk marked 'safe'
 *	OFS_DOMAIN_TIMES_VALID	If set then the attach/detachSeconds fields
 *		are valid.
 *	OFS_DOMAIN_JUST_CHECKED Set if the disk was just checked by fscheck
 *		and doesn't need to be rechecked upon reboot.  This is only
 *		useful for the root partition, since that is the only one
 *		that requires a reboot.  In theory only the 
 *		OFS_DOMAIN_NOT_SAFE should be needed, but we don't yet
 *		trust the filesystem to shut down cleanly.
 */
#define	OFS_DOMAIN_NOT_SAFE		0x1
#define OFS_DOMAIN_ATTACHED_CLEAN	0x2
#define	OFS_DOMAIN_TIMES_VALID		0x4
#define OFS_DOMAIN_JUST_CHECKED	0x8

/*
 * Stuff for block allocation 
 */

#define	OFS_NUM_FRAG_SIZES	3

/*
 * The bad block file, the root directory of a domain and the lost and found 
 * directory have well known file numbers.
 */
#define OFS_BAD_BLOCK_FILE_NUMBER	FSDM_BAD_BLOCK_FILE_NUMBER
#define OFS_ROOT_FILE_NUMBER		FSDM_ROOT_FILE_NUMBER
#define OFS_LOST_FOUND_FILE_NUMBER	FSDM_LOST_FOUND_FILE_NUMBER

/*
 * The lost and found directory is preallocated and is of a fixed size. Define
 * its size in 4K blocks here.
 */
#define	OFS_NUM_LOST_FOUND_BLOCKS	2

/*
 * Structure to keep statistics about each cylinder.
 */

typedef struct OfsCylinder {
    int	blocksFree;	/* Number of blocks free in this cylinder. */
} OfsCylinder;



/*
 * The geometry of a disk is a parameter to the disk block allocation routine.
 * It is stored in disk in the Domain header so that different configurations
 * on the same disk can be tried out and compared.
 *
 * The following parameters define array sizes in the Ofs_Geometry struct.
 *
 * OFS_MAX_ROT_POSITIONS defines how many different rotational positions are
 * possible for a filesystem block.  An Eagle Drive, for example, has 23
 * rotational positions.  There are 46 sectors per track.  That means that
 * 5 4K filesystem blocks fit on a track and the 6th spills over onto the
 * next track.  This offsets all the 4K blocks on the next track by 1K.
 * This continues for 4 tracks after which 23 whole blocks have been
 * packed in.  The set of 23 blocks is called a ``rotational set''.
 * Also, because the Eagle has 20 heads, and each rotational set occupies
 * 4 tracks, there are 5 rotational sets per cylinder.
 *
 * OFS_MAX_TRACKS_PER_SET defines how many tracks a rotational set can
 * take up.
 */
#define OFS_MAX_ROT_POSITIONS	32
#define OFS_MAX_TRACKS_PER_SET	10

/*
 * There is a new mapping available for SCSI disks.  In this mapping we
 * ignore rotational sets altogether and pack as many blocks as possible
 * into a cylinder. The field rotSetsPerCyl is overloaded to allow us
 * to be backwards compatible. If rotSetsPerCyl == OFS_SCSI_MAPPING
 * then we are using the new mapping.  The other fields relating to
 * rotational sets and block offsets are ignored.
 */

#define OFS_SCSI_MAPPING -1

typedef struct Ofs_Geometry {
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
    int		blockOffset[OFS_MAX_ROT_POSITIONS];	/* This keeps the
					 * starting sector number for each
					 * rotational position.  This table
					 * is computed by the makeFilesystem
					 * user program */
    int		sortedOffsets[OFS_MAX_ROT_POSITIONS];	/* An ordered set of
					 * the rotational positions */
    /*
     * Add more data after here so we have to reformat the disk less often.
     */
} Ofs_Geometry;


/*
 * A disk is partitioned into areas that each store a domain.  Each domain
 * contains a DomainHeader that describes how the blocks of the domain are
 * used.  The layout information takes into account the blocks that are
 * reserved for the copy of the Disk Header and the boot program.
 */
typedef struct Ofs_DomainHeader {
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
    Ofs_Geometry	geometry;	/* Used by the allocation routines and
					 * by the block IO routines */
} Ofs_DomainHeader;

#define OFS_DOMAIN_MAGIC	(unsigned int)0xF8E7D6C5

/*
 * OFS_NUM_DOMAIN_SECTORS is the standard number of sectors taken
 * up by the domain header.
 */
#define OFS_NUM_DOMAIN_SECTORS	((sizeof(Ofs_DomainHeader)-1) / DEV_BYTES_PER_SECTOR + 1)

/*
 * Structure for each domain.
 */

typedef struct Ofs_Domain {
    Fsio_FileIOHandle	physHandle;	/* Handle to use to read and write
					 * physical blocks. */
    Ofs_DomainHeader *headerPtr; 	/* Disk information for the domain. */
    /*
     * Disk summary information.
     */
    Ofs_SummaryInfo *summaryInfoPtr;
    int		  summarySector;
    /*
     * Data block allocation.
     */
    unsigned char *dataBlockBitmap;	/* The per domain data block bit map.*/
    int		bytesPerCylinder;	/* The number of bytes in the bit map
					 * for each cylinder. */
    OfsCylinder	*cylinders;		/* Pointer to array of cylinder
					 * information. */
    List_Links	*fragLists[OFS_NUM_FRAG_SIZES];	/* Lists of fragments. */
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
    Fsdm_Domain *domainPtr;		/* Domain of this file system. */
    DevBlockDeviceHandle *blockDevHandlePtr; /* Block device containing file
					      * system. */
} Ofs_Domain;


/*
 * Types of indexing.  Order is important here because the indirect and
 * double indirect types can be used to index into the indirect block 
 * pointers in the file descriptor.
 */

#define	OFS_INDIRECT		FSDM_INDIRECT
#define	OFS_DBL_INDIRECT	FSDM_DBL_INDIRECT
#define	OFS_DIRECT		FSDM_DIRECT

typedef	int	OfsBlockIndexType;

/*
 * Structure to keep information about the indirect and doubly indirect
 * blocks used in indexing.
 */

typedef struct OfsIndirectInfo {
    	Fscache_Block 	*blockPtr;	/* Pointer to indirect block. */
    	int		index;		/* An index into the indirect block. */
    	Boolean	 	blockDirty;	/* TRUE if the block has been
					   modified. */
    	int	 	deleteBlock;	/* FSCACHE_DELETE_BLOCK bit set if should 
					   delete the block when are
					   done with it. */
} OfsIndirectInfo;

/*
 * Structure used when going through the indexing structure of a file.
 */

typedef struct OfsBlockIndexInfo {
    OfsBlockIndexType	 indexType;	/* Whether chasing direct, indirect,
					   or doubly indirect blocks. */
    int		blockNum;		/* Block that is being read, written,
					   or allocated. */
    int		lastDiskBlock;		/* The disk block for the last file
					   block. */
    int		*blockAddrPtr;		/* Pointer to pointer to block. */
    int		directIndex;		/* Index into direct block pointers. */
    OfsIndirectInfo indInfo[2];	/* Used to keep track of the two 
					   indirect blocks. */
    int		 flags;			/* Flags defined below. */
    Ofs_Domain	*ofsPtr;		/* Domain that the file is in. */
} OfsBlockIndexInfo;

/*
 * Structure to keep information about each fragment.
 */

typedef struct OfsFragment {
    List_Links  links;          /* Links to put in list of free fragments of
                                   this size. */
    int         blockNum;       /* Block that this fragment comes from. */
} OfsFragment;

/*
 * Flags for the index structure.
 *
 *     OFS_ALLOC_INDIRECT_BLOCKS		If an indirect is not allocated then
 *					allocate it.
 *     OFS_DELETE_INDIRECT_BLOCKS	After are finished with an indirect
 *					block if it is empty delete it.
 *     OFS_DELETING_FROM_FRONT		Are deleting blocks from the front
 *					of the file.
 *     OFS_DELETE_EVERYTHING		The file is being truncated to length
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

#define	OFS_ALLOC_INDIRECT_BLOCKS	0x01
#define	OFS_DELETE_INDIRECT_BLOCKS	0x02
#define	OFS_DELETING_FROM_FRONT	0x04
#define	OFS_DELETE_EVERYTHING		0x08
/*resrv FSCACHE_DONT_BLOCK		0x40000 */

/*
 * OFS_PTR_FROM_DOMAIN retrieve the Ofs_Domain pointer from the Fsdm_Domain.
 */
#define	OFS_PTR_FROM_DOMAIN(domainPtr)	((Ofs_Domain *)(domainPtr->clientData))

/*
 * Declarations for the file descriptor allocation routines.
 */

extern ReturnStatus Ofs_FileDescInit _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber, int type, int permissions, int uid, int gid, 
		Fsdm_FileDescriptor *fileDescPtr));
extern ReturnStatus Ofs_FileDescFetch _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber, Fsdm_FileDescriptor *fileDescPtr));
extern ReturnStatus Ofs_FileDescStore _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *handlePtr, int fileNumber, 
		Fsdm_FileDescriptor *fileDescPtr, Boolean forceOut));
extern ReturnStatus Ofs_GetNewFileNumber _ARGS_((Fsdm_Domain *domainPtr, 
		int dirFileNum, int *fileNumberPtr));
extern ReturnStatus Ofs_FreeFileNumber _ARGS_((Fsdm_Domain *domainPtr, 
		int fileNumber));
extern ReturnStatus Ofs_FileTrunc _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *handlePtr, int size, Boolean delete));
/*
 * File index routines. 
 */
extern ReturnStatus OfsGetFirstIndex _ARGS_((Ofs_Domain *ofsPtr,
		Fsio_FileIOHandle *handlePtr, int blockNum, 
		OfsBlockIndexInfo *indexInfoPtr, int flags));


extern ReturnStatus OfsGetNextIndex _ARGS_((Fsio_FileIOHandle *handlePtr, 
		OfsBlockIndexInfo *indexInfoPtr, Boolean dirty));
extern void OfsEndIndex _ARGS_((Fsio_FileIOHandle *handlePtr, 
		OfsBlockIndexInfo *indexInfoPtr, Boolean dirty));



extern void OfsBlockFind _ARGS_((int hashSeed, Ofs_Domain *ofsPtr, 
		int nearBlock, Boolean allocate, int *blockNumPtr, 
		unsigned char **bitmapPtrPtr));
extern void OfsBlockFree _ARGS_((Ofs_Domain *ofsPtr, int blockNum));
extern void OfsFragFind _ARGS_((int hashSeed, Ofs_Domain *ofsPtr, int numFrags,
		int lastFragBlock, int lastFragOffset, int lastFragSize, 
		int *newFragBlockPtr, int *newFragOffsetPtr));
extern void OfsFragFree _ARGS_((Ofs_Domain *ofsPtr, int numFrags, 
		int fragBlock, int fragOffset));

/*
 * Cache backend routines. 
 */
extern ReturnStatus Ofs_BlockAllocate _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *handlePtr, int offset, int numBytes, 
		int flags, int *blockAddrPtr, Boolean *newBlockPtr));
extern ReturnStatus Ofs_FileBlockWrite _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *handlePtr, Fscache_Block *blockPtr));
extern ReturnStatus Ofs_FileBlockRead _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *handlePtr, Fscache_Block *blockPtr));
extern Boolean Ofs_StartWriteBack _ARGS_((Fscache_Backend *backendPtr));
extern void Ofs_ReallocBlock _ARGS_((ClientData data, 
		Proc_CallInfo *callInfoPtr));


/*
 * Routines for attaching/detaching disk.
 */
extern ReturnStatus Ofs_AttachDisk _ARGS_((Fs_Device *devicePtr, 
		char *localName, int flags, int *domainNumPtr));
extern ReturnStatus Ofs_DetachDisk _ARGS_((Fsdm_Domain *domainPtr));
/*
 * Routines to manipulate domains.
 */
extern ReturnStatus Ofs_DomainInfo _ARGS_((Fsdm_Domain *domainPtr, 
		Fs_DomainInfo *domainInfoPtr));
extern ReturnStatus Ofs_DomainWriteBack _ARGS_((Fsdm_Domain *domainPtr, 
		Boolean shutdown));
extern ReturnStatus Ofs_RereadSummaryInfo _ARGS_((Fsdm_Domain *domainPtr));
extern ReturnStatus OfsDeviceBlockIO _ARGS_((Ofs_Domain *ofsPtr, 
		int readWriteFlag, int fragNumber, int numFrags, 
		Address buffer));
extern ReturnStatus OfsWriteBackSummaryInfo _ARGS_((Ofs_Domain *ofsPtr));
extern ReturnStatus OfsWriteBackDataBlockBitmap _ARGS_((Ofs_Domain *ofsPtr));
extern ReturnStatus OfsWriteBackFileDescBitmap _ARGS_((Ofs_Domain *ofsPtr));

/*
 * Directory change calls.
 */
extern ClientData Ofs_DirOpStart _ARGS_((Fsdm_Domain *domainPtr, int opFlags,
		char *name, int nameLen, int fileNumber, 
		Fsdm_FileDescriptor *fileDescPtr, int dirFileNumber, 
		int dirOffset, Fsdm_FileDescriptor *dirFileDescPtr));
extern void Ofs_DirOpEnd _ARGS_((Fsdm_Domain *domainPtr, ClientData clientData,
	ReturnStatus status, int opFlags, char *name, int nameLen,
	int fileNumber, Fsdm_FileDescriptor *fileDescPtr, int dirFileNumber,
	int dirOffset, Fsdm_FileDescriptor *dirFileDescPtr));
/*
 * Miscellaneous
 */
extern void Ofs_Init _ARGS_((void));
extern ReturnStatus OfsIOInit _ARGS_((Fsdm_Domain *domainPtr));
extern ReturnStatus OfsFileDescAllocInit _ARGS_((Ofs_Domain *ofsPtr));
extern ReturnStatus OfsBlockAllocInit _ARGS_((Ofs_Domain *ofsPtr));
extern int OfsBlocksToSectors _ARGS_((int fragNumber, Ofs_Geometry *geoPtr));
extern ReturnStatus OfsVerifyBlockWrite _ARGS_((Ofs_Domain *ofsPtr, 
				Fscache_Block *blockPtr));

#endif /* _OFS */
