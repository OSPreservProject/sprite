/*
 * lfsStableMem.h --
 *
 *	Declarations of interface for maintaining the in memory data structures
 *	of LFS.
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

#ifndef _LFSSTABLEMEM
#define _LFSSTABLEMEM

/* constants */

/* data structures */

/*
 * LfsStableMemCheckPoint - data values written at stable memory checkpoints.
 */
typedef struct LfsStableMemCheckPoint {
    int	numBlocks;	/* Number of block pointers written in this 
			 * checkpoint. */
} LfsStableMemCheckPoint;

/*
 * LfsStableMemParams - Configuration parameters for stable memory data
 *			structures
 */
typedef struct LfsStableMemParams {
    int	memType;	/* Stable memory type */
    int blockSize;	/* Block size in bytes for index.  Must be
			 * a multiple of the file system block size. */
    int entrySize;	/* Size of each entry in bytes. */
    int	maxNumEntries;	/* Maximum number of entries supported. */
    int	entriesPerBlock; /* Number of entries per block. */
    int maxNumBlocks;	/* Maximum number of blocks supported by this
			 * index. */
} LfsStableMemParams;


/*
 * An on disk header for each stable memory block.
 */
typedef struct LfsStableMemBlockHdr {
    int	magic;		/* Better be LFS_STABLE_MEM_BLOCK_MAGIC. */
    int	memType;	/* Memory type from params of this block. */
    int	blockNum;	/* Block number in stable memory of this block. */
    int	reserved;	/* Reserved must be zero. */
} LfsStableMemBlockHdr;

#define	LFS_STABLE_MEM_BLOCK_MAGIC	0x1f55da

#endif /* _LFSSTABLEMEM */

