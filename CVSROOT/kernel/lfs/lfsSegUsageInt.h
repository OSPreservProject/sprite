/*
 * lfsSegUsageInt.h --
 *
 *	Declarations of LFS segment ussage routines and data structures
 *	private to the Lfs module.
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

#ifndef _LFSSEGUSAGEINT
#define _LFSSEGUSAGEINT

#include <lfsUsageArray.h>

/* constants */

/* data structures */

typedef struct LfsSegUsage {
    LfsStableMem	stableMem;/* Stable memory supporting the map. */
    LfsSegUsageParams	params;	  /* Map parameters taken from super block. */
    LfsSegUsageCheckPoint checkPoint; /* Desc map data written at checkpoint. */
} LfsSegUsage;

typedef struct LfsSegList {
    int	segNumber;	/* Segment number of segment. */
    int activeBytes;	/* Active bytes from the seg usage array. */
} LfsSegList;

/* procedures */

extern void LfsSegUsageInit _ARGS_((void));

extern void LfsSetSegUsage _ARGS_((struct Lfs *lfsPtr, int segNumber, 
			int activeBytes));
extern ReturnStatus LfsGetLogTail _ARGS_((struct Lfs *lfsPtr, Boolean cantWait,
			LfsSegLogRange *logRangePtr, int *startBlockPtr ));

extern void LfsSetLogTail _ARGS_((struct Lfs *lfsPtr, 
			LfsSegLogRange *logRangePtr, int startBlock, 
			int activeBytes));

extern void LfsMarkSegClean _ARGS_((struct Lfs *lfsPtr, int segNumber));
extern void LfsSetDirtyLevel _ARGS_((struct Lfs *lfsPtr, int dirtyActiveBytes));

extern int LfsGetSegsToClean _ARGS_((struct Lfs *lfsPtr, 
			int maxSegArrayLen, LfsSegList *segArrayPtr));

extern ReturnStatus LfsSegUsageFreeBlocks _ARGS_((struct Lfs *lfsPtr, 
			int blockSize, int blockArrayLen, 
			LfsDiskAddr *blockArrayPtr));
extern ReturnStatus LfsSegUsageAllocateBytes _ARGS_((struct Lfs *lfsPtr,				 int numBytes));
extern ReturnStatus LfsSegUsageFreeBytes _ARGS_((struct Lfs *lfsPtr, 
			int numBytes));

extern void LfsSegUsageCheckpointUpdate _ARGS_((struct Lfs *lfsPtr,
			char *checkPointPtr, int size));

#endif /* _LFSSEGUSAGEINT */

