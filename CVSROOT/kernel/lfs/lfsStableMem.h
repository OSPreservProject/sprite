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

typedef struct LfsStableMemCheckPoint {
    int	numBlocks;	/* Number of block pointers written in this 
			 * checkpoint. */
} LfsStableMemCheckPoint;

typedef struct LfsStableMemParams {
    int blockSize;		/* Block size in bytes for index.  Must be
				 * a multiple of the file system block size. */
    unsigned int maxNumBlocks;	/* Maximum number of blocks supported by this
				 * index. */
} LfsStableMemParams;


typedef struct LfsStableMem {
    char 	*dataPtr;	/* Pointer to metadata. */
    unsigned int *blockIndexPtr; /* Index of current buffer addresses. */
    char	*dirtyBlocksBitMapPtr; /* Bitmap of dirty blocks. */
    int		blockSizeShift;	       /* Log base 2 of params.blockSize. */
    LfsStableMemCheckPoint checkPoint; /* Data to be checkpoint. */
    LfsStableMemParams params;  /* A copy of the parameters of the index. */
} LfsStableMem;

/* procedures */

extern Boolean LfsStableMemClean();
extern Boolean LfsStableMemCheckpoint();
extern void LfsStableMemWriteDone();
extern ReturnStatus LfsStableMemLoad();
extern void LfsStableMemMarkDirty();

#endif /* _LFSSTABLEMEM */

