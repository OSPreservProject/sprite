/*
 * lfsDescInt.h --
 *
 *	Declarations of data structures for file descriptors internal to 
 *	a LFS file system.
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

#ifndef _LFSDESCINT
#define _LFSDESCINT

#include <fsioFile.h>

/* constants */

/* data structures */

/*
 * LfsDescCache - Data structure defining the cache of file descriptor blocks
 *		  maintained by a LFS file system.  The current implementation
 *		  caches descriptor map blocks in files the file cache.
 */

typedef struct LfsDescCache {
    Fsio_FileIOHandle handle; /* File handle use to cache descriptor
			       * block under. */
} LfsDescCache;
/* procedures */

extern void LfsDescCacheInit _ARGS_((struct Lfs *lfsPtr));
extern ClientData LfsDescCacheBlockInit _ARGS_((struct Lfs *lfsPtr, 
		LfsDiskAddr diskBlockAddr, 
		Boolean cantBlock, char **blockStartPtr));
extern void LfsDescCacheBlockRelease _ARGS_((struct Lfs *lfsPtr, 
		ClientData clientData, Boolean deleteBlock));

#endif /* _LFSDESCINT */

