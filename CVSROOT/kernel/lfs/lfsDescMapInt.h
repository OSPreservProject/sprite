/*
 * lfsDescMapInt.h --
 *
 *	Declarations of LFS descriptor map routines and data structures
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

#ifndef _LFSDESCMAPINT
#define _LFSDESCMAPINT

#include <lfsDescMap.h>
#include <lfsStableMemInt.h>

/* constants */

/* data structures */

typedef struct LfsDescMap {
    LfsStableMem	stableMem;/* Stable memory supporting the map. */
    LfsDescMapParams	params;	  /* Map parameters taken from super block. */
    LfsDescMapCheckPoint checkPoint; /* Desc map data written at checkpoint. */
} LfsDescMap;

/* procedures */
extern void LfsDescMapInit _ARGS_((void));

extern void LfsDescCacheInit _ARGS_((struct Lfs *lfsPtr));
extern ReturnStatus LfsDescMapGetVersion _ARGS_((struct Lfs *lfsPtr,
			int fileNumber, unsigned short *versionNumPtr));
extern ReturnStatus LfsDescMapIncVersion _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, int *versionPtr));
extern ReturnStatus LfsDescMapGetDiskAddr _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, LfsDiskAddr *diskAddrPtr));
extern ReturnStatus LfsDescMapSetDiskAddr _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, LfsDiskAddr diskAddr));
extern ReturnStatus LfsDescMapGetAccessTime _ARGS_((struct Lfs *lfsPtr,
			int fileNumber, int *accessTimePtr));
extern ReturnStatus LfsDescMapSetAccessTime _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, int accessTime));


#endif /* _LFSDESCMAPINT */

