/*
 * lfsMemInt.h --
 *
 *	Declarations of Lfs memory resource management routines and
 *	data structures.
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

#ifndef _LFSMEMINT
#define _LFSMEMINT


/* data structures */

/*
 * LfsMem - Per LFS file system resource list.
 */
typedef struct LfsMem {
    int	cacheBlocksReserved; /* Number of cache blocks reserved for this file
			      * system. */
} LfsMem;

/* procedures */

extern void LfsMemInit _ARGS_((struct Lfs *lfsPtr));
extern void LfsMemDetach _ARGS_((struct Lfs *lfsPtr));
extern void LfsMemReserve _ARGS_((struct Lfs *lfsPtr, int *cacheBlocksPtr,
				char **memPtrPtr));
extern void LfsMemRelease _ARGS_((struct Lfs *lfsPtr, int cacheBlocks, 
				char *memPtr));

#endif /* _LFSMEMINT */

