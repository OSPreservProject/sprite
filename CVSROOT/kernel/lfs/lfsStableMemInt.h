/*
 * lfsStableMemInt.h --
 *
 *	Declarations of kernel routines implementing the LFS stable memory
 *	abstraction. The stable memory abstraction provides access a
 *	collection of fixed size "entries" numbered consecutively from zero.
 *	
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

#ifndef _LFSSTABLEMEMINT
#define _LFSSTABLEMEMINT

#include <lfsInt.h>
#include <lfsStableMem.h>
#include <fsioFile.h>
#include <lfsSeg.h>

/* constants */

/*
 * Flags for LfsStableMemFetch. 
 * LFS_STABLE_MEM_MAY_DIRTY	- Fetcher may change element.
 * LFS_STABLE_MEM_REL_ENTRY     - The pasted entry should be released.
 */

#define	LFS_STABLE_MEM_MAY_DIRTY 1
#define	LFS_STABLE_MEM_REL_ENTRY 2

/* data structures */
typedef struct LfsStableMem {
    struct Lfs	      *lfsPtr;		/* File system for stable memory. */
    Fsio_FileIOHandle dataHandle;	/* Handle used to store blocks in
					 * cache under. */
    LfsDiskAddr	*blockIndexPtr; 	/* Index of current disk addresses. */
    int		numCacheBlocksOut;	/* The number of cache blocks currently
					 * fetched by the backend. */
    LfsStableMemCheckPoint checkPoint; /* Data to be checkpoint. */
    LfsStableMemParams params;  /* A copy of the parameters of the index. */
} LfsStableMem;

typedef struct LfsStableMemEntry {
    Address	addr;			/* Memory address of entry. */
    Boolean	modified;		/* TRUE if the entry has been 
					 * modified. */
    int		blockNum;		/* Block number of entry. */
    ClientData	clientData;		/* Clientdata maintained by 
					 * StableMem code. */
} LfsStableMemEntry;

/*
 * Macro for accessing elements of LfsStableMemEntry.
 * LfsStableMemEntryAddr - Return the memory address of an entry.
 * LfsStableMemMarkDirty - Mark an entry has modified.
 */

#define	LfsStableMemEntryAddr(entryPtr)	((entryPtr)->addr)
#define	LfsStableMemMarkDirty(entryPtr)	((entryPtr)->modified = TRUE)

/* procedures */

extern ReturnStatus LfsStableMemLoad _ARGS_((struct Lfs *lfsPtr, 
		LfsStableMemParams *smemParamsPtr, int checkPointSize, 
		char *checkPointPtr, LfsStableMem *smemPtr));
extern Boolean LfsStableMemClean _ARGS_((struct LfsSeg *segPtr, int *sizePtr, 
		int *numCacheBlocksPtr, ClientData *clientDataPtr, 
		LfsStableMem *smemPtr));
extern Boolean LfsStableMemCheckpoint _ARGS_((struct LfsSeg *segPtr, 
		char *checkPointPtr, int flags, int *checkPointSizePtr, 
		ClientData *clientDataPtr, LfsStableMem *smemPtr));
extern Boolean LfsStableMemLayout _ARGS_((struct LfsSeg *segPtr, int flags,
		ClientData *clientDataPtr, LfsStableMem *smemPtr));
extern void LfsStableMemWriteDone _ARGS_((struct LfsSeg *segPtr, int flags, 
		ClientData *clientDataPtr, LfsStableMem *smemPtr));
extern ReturnStatus LfsStableMemFetch _ARGS_((LfsStableMem *smemPtr, 
		int entryNumber, int flags, 
		LfsStableMemEntry *entryPtr));
extern void LfsStableMemRelease _ARGS_((LfsStableMem *smemPtr, 
		LfsStableMemEntry *entryPtr, Boolean modified));

extern ReturnStatus LfsStableMemDestory _ARGS_((struct Lfs *lfsPtr, 
						LfsStableMem *smemPtr));

#endif /* _LFSSTABLEMEMINT */

