/*
 * lfsUsageArray.h --
 *
 *	Declarations defining the disk resident format of the LFS 
 *	segment usage array. The main purpose of the segment usage array 
 *	is to aid LFS in making intelligent choices for segment to clean
 *	and segments to write. The segment usage array also keeps track of
 *	the disk space usage for the file system.
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
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/lfs/lfsUsageArray.h,v 1.5 90/10/19 17:23:12 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _LFSUSAGEARRAY
#define _LFSUSAGEARRAY

#ifdef KERNEL
#include <lfsStableMem.h>
#else
#include <kernel/lfsStableMem.h>
#endif
/*
 * The segment usage array layout on disk is described by the following 
 * super block resident structure. 
 * It must be LFS_USAGE_ARRAY_PARAM_SIZE (currently 64 bytes) in size. 
 */
#define	LFS_USAGE_ARRAY_PARAM_SIZE	64

typedef struct LfsSegUsageParams {
    int segmentSize;  	   /* The number of bytes in each of segment. 
			    * Must be a multiple of the file system
			    * block size. */
    int numberSegments;	  /* The number of segments in the system. */
    int   minNumClean;    /* The min number of clean segment we allow the
			   * system to reach before starting clean. */
    int   minFreeBlocks;  /* The min number of free blocks we allow the
			   * system to reach. */
    int	  wasteBlocks;	  /* The number of blocks we are willing to wasted at
			   * the end of a segment. */
    int	  numSegsToClean;  /* Number of segment to clean at a time. */
    char  padding[LFS_USAGE_ARRAY_PARAM_SIZE - sizeof(LfsStableMemParams)-6*4];
    LfsStableMemParams	 stableMem; /* Memory for locating the array. */
} LfsSegUsageParams;


/*
 * The checkpoint data of segment usage array is described by a structure
 * LfsSegUsageCheckPoint. The disk resident structure of a checkpoint
 * contains a LfsSegUsageCheckPoint followed by a LfsStableMemCheckPoint.
 */
typedef struct LfsSegUsageCheckPoint {
    int	freeBlocks;	/* Number of free blocks available. */ 
    int numClean;	/* Number of clean segments. */
    int numDirty;	/* Number of dirty segments. */
    int dirtyActiveBytes; /* Number of known active bytes below which a 
			   * segment is considered dirty. */
    int	currentSegment;	/* Current segment being written. */
    int	currentBlockOffset; /* Current block offset into last segment being
			     * written. -1 means segment filled. */
    int curSegActiveBytes; /* Active bytes of last segment written. */
    int	previousSegment;   /* Previous segment written. */
    int cleanSegList;	/* First segment in the list of clean segemnts. */
} LfsSegUsageCheckPoint;

/*
 * For each segment in a LFS, the segment usage are keeped an 
 * entry of type LfsSegUsageEntry. LfsSegUsageEntry are packed into blocks
 * to form an array index by segment number. 
 */
typedef struct LfsSegUsageEntry {
    int  activeBytes;     	    /* An estimate of the number of active
				     * bytes in this segment. If the segment
				     * is clean (LFS_SEG_USAGE_CLEAN is set
				     * in flags), then activeBytes contains
				     * the index of the next clean segment. */
    int	 timeOfLastWrite;	    /* File system time of last write 
				     * in seconds. */
    int  flags;     		    /* Flags described below. */
} LfsSegUsageEntry;

/*
 * Values for the flags field of the LfsSegUsageEntry.
 *
 * LFS_SEG_USAGE_CLEAN	- The segment has been cleaned and contains no 
 *			  live data. 
 * LFS_SEG_USAGE_DIRTY  - The segment is neither full or dirty.
 * LFS_SEG_USAGE_CHAIN	- The segment is a member of checkpoint chain that
 *			  hasn't been terminated. 
 */
#define	LFS_SEG_USAGE_CLEAN 0x0001
#define	LFS_SEG_USAGE_DIRTY 0x0002
#define	LFS_SEG_USAGE_CHAIN 0x0004


#endif /* _LFSUSAGEARRAY */

