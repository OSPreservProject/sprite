/*
 * lfsInt.h --
 *
 *	Type and data uses internally to the LFS module.
 *
 * Copyright 1989 Regents of the University of California
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

#ifndef _LFSINT
#define _LFSINT

#include "user/fs.h"
#include "lfsDescMapInt.h"
#include "lfsSuperBlock.h"
#include "lfsSegUsageInt.h"
#include "lfsFileLayoutInt.h"
#include "lfsStats.h"

#include "fsioFile.h"
#include "fsdm.h"

/* constants */
/*
 * Flags for checkpoint callback.
 * LFS_DETACH - This checkpoint is part of a file system detach.
 *	            Any data structures malloc'ed for this file
 *		    system during attach should be freed.
 */
#define	LFS_DETACH	 0x1

/* data structures */

/*
 * LfsSegLogRange describes the position in the log of the segment. 
 */
typedef struct LfsSegLogRange {
    int		current;	/* Current segment being written. */
    int		prevSeg;	/* Previous segment that was written. */
    int		nextSeg;	/* Next segment to be written. */
} LfsSegLogRange;

typedef struct LfsCheckPoint {
    int	  timestamp;	/* Current file system timestamp. */
    char  *buffer;	/* Memory buffer to place checkpoint. */
    int	  nextArea;	/* Next checkpoint area to write. Must be 0 or 1. */
} LfsCheckPoint;

typedef struct LfsSegCache {
    Boolean valid;	
    int	    segNum;
    int     startDiskAddress;
    int     endDiskAddress;
    char    *memPtr;
} LfsSegCache;

typedef struct Lfs {
    Fs_Device	  *devicePtr;	/* Device containing file system. 
				 * bytes and back. */
    char	  *name;	/* Name used for error messages. */
    Fsdm_Domain	  *domainPtr;	/* Domain this file system belongs. */
    Boolean   	  readOnly;	/* TRUE if the file system is readonly. */
    Boolean	  dirty;	/* TRUE if the file system has been modified
				 * since the last checkpoint. */
    Sync_Lock      lfsLock;	/* Lock for file system. */
    Sync_Condition lfsUnlock;	/* Condition to wait for unlock on */
    Sync_Condition cleanSegments; /* Condition to wait for clean
				   * segments to be generated. */
    Boolean	   locked;	/* File system is locked. */
    int	      blockSizeShift;   /* Log base 2 of blockSize. */
    LfsSegLogRange activeLogRange; /* The current segment being written. */
    int	      activeBlockOffset; /* The block offst in tp the current segment
				  * to be written. -1 means activeLogRange
				  * is not valid. */
    Boolean	writeBackActive; /* TRUE if an writeback process active on this
				  * file system. */
    Boolean  	checkForMoreWork; /* TRUE if writeback should check for more
				   * file to be written before exiting. 
				   */
    Boolean	cleanActive;	/* TRUE if an cleaner is active on this
				 * file system. */
    int		cleanBlocks;	/* Maximum number of blocks to clean. */
    Fsio_FileIOHandle descCacheHandle; /* File handle use to cache descriptor
					* block under. */
    LfsSegCache   segCache;	  /* Cache of recently read segments. */
    char	  *segMemoryPtr; /* Memory to be used for segment writing. */
    char	  *cleaningMemPtr; /* Memory to be used for cleaning. */
    Boolean	  segMemInUse;	/* TRUE if segment memory is being used. */
    LfsDescMap	  descMap;	/* Descriptor map data. */
    LfsSegUsage   usageArray;   /* Segment usage array data. */
    LfsCheckPoint checkPoint;   /* Checkpoint data. */
    LfsFileLayout fileLayout;	/* File layout data structures. */
    LfsSuperBlock superBlock;	/* Copy of the file system's super block 
				 * read at attach time. */
    Lfs_Stats	stats;		/* Stats on the file system.  */
} Lfs;

/* Useful macros for LFS.
 *
 * LfsSegSize(lfsPtr)	- Return the segment size in bytes.
 * LfsSegSizeInBlocks(lfsPtr) - Return the segment size in blocks.
 * LfsBlockSize(lfsPtr)       - Return the block size.
 * LfsBytesToBlocks(lfsPtr, bytes) - Convert bytes into number of blocks.
 * LfsBlocksToBytes(lfsPtr, blocks) - Convert from blocks into bytes.
 * LfsSegNumToDiskAddress(lfsPtr, segNum) - Convert a segment number into
 *					    a disk address.
 * LfsBlockToSegmentNum(lfsPtr, block)  - Number the segment of a disk 
 *					  address .
 */

#define	LfsSegSize(lfsPtr) ((lfsPtr)->usageArray.params.segmentSize)

#define	LfsSegSizeInBlocks(lfsPtr) \
			(LfsSegSize(lfsPtr)>>(lfsPtr)->blockSizeShift)

#define	LfsBlockSize(lfsPtr) ((lfsPtr)->superBlock.hdr.blockSize)

#define	LfsBytesToBlocks(lfsPtr, bytes)	\
	 (((bytes) + (LfsBlockSize(lfsPtr)-1))>>(lfsPtr)->blockSizeShift)

#define	LfsBlocksToBytes(lfsPtr, blocks) ((blocks)<<(lfsPtr)->blockSizeShift)


#define LfsValidSegmentNum(lfsPtr, segNum) (((segNum) >= 0) && \
		((segNum) < (lfsPtr)->usageArray.params.numberSegments))


#define LfsSegNumToDiskAddress(lfsPtr, segNum) \
		((int)(lfsPtr)->superBlock.hdr.logStartOffset + \
				 (LfsSegSizeInBlocks((lfsPtr)) * (segNum)))

#define LfsBlockToSegmentNum(lfsPtr, blockNum) \
		((blockNum - (lfsPtr)->superBlock.hdr.logStartOffset) / \
					 LfsSegSizeInBlocks((lfsPtr)))

#define	LfsGetCurrentTimestamp(lfsPtr)	(++((lfsPtr)->checkPoint.timestamp))

#define	LfsFromDomainPtr(domainPtr) ((Lfs *) ((domainPtr)->clientData))


extern void LfsError();
extern void LfsSegCleanStart();

extern void LfsDescCacheInit();
extern void LfsDescMapInit();
extern void LfsSegUsageInit();
extern void LfsFileLayoutInit();

#endif /* _LFSINT */

