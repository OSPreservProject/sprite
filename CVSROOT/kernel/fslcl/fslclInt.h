/*
 * fsLocalDomain.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLOCALDOMAIN
#define _FSLOCALDOMAIN

#include "fsDisk.h"
#include "fsBlockCache.h"
#include "fsFile.h"

/*
 * Stuff for block allocation 
 */

#define	FS_NUM_FRAG_SIZES	3

/*
 * Image of a new directory.
 */
extern char *fsEmptyDirBlock;

/*
 * Structure to keep statistics about each cylinder.
 */

typedef struct FsCylinder {
    int	blocksFree;	/* Number of blocks free in this cylinder. */
} FsCylinder;

/*
 * Structure to keep information about each fragment.
 */

typedef struct FsFragment {
    List_Links	links;		/* Links to put in list of free fragments of 
				   this size. */
    int		blockNum;	/* Block that this fragment comes from. */
} FsFragment;

/*
 * Structure for each domain.
 */

typedef struct FsDomain {
    FsLocalFileIOHandle	physHandle;	/* Handle to use to read and write
					 * physical blocks. */
    FsDomainHeader *headerPtr; 		/* Disk information for the domain. */
    /*
     * Disk summary information.
     */
    FsSummaryInfo *summaryInfoPtr;
    int		  summarySector;
    /*
     * Data block allocation.
     */
    unsigned char *dataBlockBitmap;	/* The per domain data block bit map.*/
    int		bytesPerCylinder;	/* The number of bytes in the bit map
					 * for each cylinder. */
    FsCylinder	*cylinders;		/* Pointer to array of cylinder
					 * information. */
    List_Links	*fragLists[FS_NUM_FRAG_SIZES];	/* Lists of fragments. */
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
} FsDomain;

/*
 * Domain flags used for two stage process of detaching a domain:
 *
 *    FS_DOMAIN_GOING_DOWN	This domain is being detached.
 *    FS_DOMAIN_DOWN		The domain is detached.
 */
#define	FS_DOMAIN_GOING_DOWN	0x1
#define	FS_DOMAIN_DOWN 		0x2

/*
 * A table of domains.  This is used to go from domain number
 * to the state for the domain.
 *
 * FS_MAX_LOCAL_DOMAINS defines how many local domains a server can keep
 *      track of.
 */
#define FS_MAX_LOCAL_DOMAINS    10
extern FsDomain *fsDomainTable[];

/*
 * Types of indexing.  Order is important here because the indirect and
 * double indirect types can be used to index into the indirect block 
 * pointers in the file descriptor.
 */

#define	FS_INDIRECT		0 
#define	FS_DBL_INDIRECT		1
#define	FS_DIRECT		2

typedef	int	FsBlockIndexType;

/*
 * Structure to keep information about the indirect and doubly indirect
 * blocks used in indexing.
 */

typedef struct {
    	FsCacheBlock 	*blockPtr;	/* Pointer to indirect block. */
    	int		index;		/* An index into the indirect block. */
    	Boolean	 	blockDirty;	/* TRUE if the block has been
					   modified. */
    	int	 	deleteBlock;	/* FS_DELETE_BLOCK bit set if should 
					   delete the block when are
					   done with it. */
} FsIndirectInfo;

/*
 * Structure used when going through the indexing structure of a file.
 */

typedef struct FsBlockIndexInfo {
    FsBlockIndexType	 indexType;	/* Whether chasing direct, indirect,
					   or doubly indirect blocks. */
    int		blockNum;		/* Block that is being read, written,
					   or allocated. */
    int		lastDiskBlock;		/* The disk block for the last file
					   block. */
    int		*blockAddrPtr;		/* Pointer to pointer to block. */
    int		directIndex;		/* Index into direct block pointers. */
    FsIndirectInfo indInfo[2];		/* Used to keep track of the two 
					   indirect blocks. */
    int		 flags;			/* Flags defined below. */
    FsDomain	*domainPtr;		/* Domain that the file is in. */
} FsBlockIndexInfo;

/*
 * Flags for the index structure.
 *
 *     FS_ALLOC_INDIRECT_BLOCKS		If an indirect is not allocated then
 *					allocate it.
 *     FS_DELETE_INDIRECT_BLOCKS	After are finished with an indirect
 *					block if it is empty delete it.
 *     FS_DELETING_FROM_FRONT		Are deleting blocks from the front
 *					of the file.
 *     FS_DELETE_EVERYTHING		The file is being truncated to length
 *					0 so delete all blocks and indirect
 *					blocks.
 */

#define	FS_ALLOC_INDIRECT_BLOCKS	0x01
#define	FS_DELETE_INDIRECT_BLOCKS	0x02
#define	FS_DELETING_FROM_FRONT		0x04
#define	FS_DELETE_EVERYTHING		0x08


/*
 * Declarations for the file descriptor allocation routines.
 */

extern ReturnStatus	FsFileDescAllocInit();
extern ReturnStatus	FsWriteBackFileDescBitmap();
extern ReturnStatus	FsGetNewFileDesc();
extern ReturnStatus	FsFetchFileDesc();
extern ReturnStatus	FsStoreFileDesc();
extern ReturnStatus	FsFreeFileDesc();

/*
 * Declarations for the local Domain data block allocation routines and 
 * indexing routines.
 */

extern	ReturnStatus	FsLocalBlockAllocInit();
extern	ReturnStatus	FsWriteBackDataBlockBitmap();
extern	void		FsLocalBlockAllocate();
extern	ReturnStatus	FsLocalTruncateFile();
extern	int		FsSelectCylinder();
extern	void		FsFreeCylinderBlock();
extern	void		FsFindBlock();
extern	void		FsFreeBlock();
extern	void		FsFindFrag();
extern	void		FsFreeFrag();
extern	ReturnStatus	FsGetFirstIndex();
extern	ReturnStatus	FsGetNextIndex();
extern	void		FsEndIndex();

/*
 * Declarations for the Local Domain lookup operations called via
 * the switch in FsLookupOperation.  These are called with a pathname.
 */
ReturnStatus FsLocalExport();
ReturnStatus FsLocalOpen();
ReturnStatus FsLocalLookup();
ReturnStatus FsLocalGetAttrPath();
ReturnStatus FsLocalSetAttrPath();
ReturnStatus FsLocalMakeDevice();
ReturnStatus FsLocalMakeDir();
ReturnStatus FsLocalRemove();
ReturnStatus FsLocalRemoveDir();
ReturnStatus FsLocalRename();
ReturnStatus FsLocalHardLink();

/*
 * Declarations for the Local Domain attribute operations called via
 * the fsAttrOpsTable switch.  These are called with a fileID.
 */
ReturnStatus FsLocalGetAttr();
ReturnStatus FsLocalSetAttr();

/*
 * Misc. routines.
 */
ReturnStatus FsLocalInitIO();
void	     FsLocalDomainInit();
void 	     FsLocalDomainWriteBack();
ReturnStatus FsLocalSetFirstByte();

/*
 * Routines to manipulate domains.
 */
extern	FsDomain	*FsDomainFetch();
extern	void		FsDomainRelease();

#endif _FSLOCALDOMAIN
