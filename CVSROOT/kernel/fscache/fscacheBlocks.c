/* 
 * fscacheBlocks.c --
 *
 *	Routines to manage the <file-id, block #> cache.
 *
 * Copyright 1987 Regents of the University of California.
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include	<sprite.h>
#include	<fs.h>
#include	<fsutil.h>
#include	<fscache.h>
#include	<fscacheBlocks.h>
#include	<fsStat.h>
#include	<fsNameOps.h>
#include	<fsdm.h>
#include	<fsio.h>
#include	<fsutilTrace.h>
#include	<hash.h>
#include	<vm.h>
#include 	<vmMach.h>
#include	<proc.h>
#include	<sys.h>
#include	<rpc.h>


/*
 * There are numerous points of synchronization in this module.  The CacheInfo
 * struct in the handle, the block info struct and several global conditions
 * and variables are used.
 *
 *     1) Synchronization between a block being fetched and a block having
 *	  good data in it.  The FSCACHE_IO_IN_PROGRESS flag is used for this.
 *	  If this flag is set in the call to Fscache_FetchBlock then the fetch
 *	  will block until the block becomes unreferenced.  Thus if the block
 *	  is already being used the user won't get their data stomped on.
 *        Whenever Fscache_FetchBlock returns a block it sets the 
 *	  FSCACHE_IO_IN_PROGRESS flag in the block.  This flag will be cleared
 *	  whenever the block is released with Fscache_UnlockBlock or the
 *	  function Fscache_IODone is called.  While this flag is set in the
 *	  block all future fetches will block until the flag is cleared.
 *
 *     2) Synchronization for changing where a block lives on disk.  When a
 *	  block cleaner is writing out a block and Fscache_UnlockBlock is
 *	  called with a new location for the block, it blocks until the
 *	  block cleaner finishes.  The flag FSCACHE_BLOCK_BEING_WRITTEN in the
 *	  cache block struct indicates this.
 *
 *     3) Waiting for blocks from a file to be written back.  This is done by
 *	  waiting for the dirty list for the file to go empty.
 *
 *     4) Waiting for blocks from the whole cache to be written back.  We
 *	  keep track of the number of cache backends starts and wake the
 *	  waiting process when this number goes to zero.
 *
 *     5) Synchronizing informing the server when there are no longer any
 *	  dirty blocks in the cache for a file.  When the last dirty block
 *	  for a file is written the write is tagged saying that it is the
 *	  last dirty block.  However the server is also told when a file is
 *	  closed if no dirty blocks are left.  Since all writes out of the
 *	  cache are unsynchronized there is a race between the close and the
 *	  delayed write back.  This is solved by using the following 
 *	  synchronization.  When a file is closed the function 
 *	  Fscache_PreventWriteBacks is called.  This function will not return
 *	  until there are no block cleaners active on the file.  When it
 *	  returns it sets the FSCACHE_CLOSE_IN_PROGRESS flag in the cacheInfo
 *	  struct in the file handle and it returns the number of dirty blocks.
 *	  All subsequent block writes are blocked until the function
 *	  Fscache_AllowWriteBacks is called.  Thus the number of dirty blocks
 *	  in the cache for the file is accurate on close because no dirty
 *	  blocks can be written out while the file is being closed.
 *	  Likewise when a block cleaner writes out the last dirty block for
 *	  a file and it tells the server on the write that its the last
 *	  dirty block the server knows that it can believe the block cleaner
 *	  if the file is closed.  This is because if its closed then it must
 *	  have been closed when the  block cleaner did the write
 *	  (all closes are prohibited during the write) and thus there
 *	  is no way that more dirty blocks can be put into the cache.
 *	  If its open then the server ignores what the block cleaner
 *	  says because it will get told again when the file is closed.
 */

/*
 * Monitor lock.
 */
static Sync_Lock	cacheLock = Sync_LockInitStatic("Fs:blockCacheLock");
#define	LOCKPTR	&cacheLock

/*
 * Condition variables.
 */
Sync_Condition	cleanBlockCondition;	/* Condition that block 
						 * allocator waits on when all 
						 * blocks are dirty. */
Sync_Condition	writeBackComplete; 	/* Condition to wait on when 
						 * are waiting for the write 
						 * back of blocks in the cache 
						 * to complete. */
Sync_Condition	closeCondition;		/* Condition to wait on when
						 * are waiting for the block
						 * cleaner to finish to write
						 * out blocks for this file. */
static unsigned int filewriteBackTime;		/* Write back all blocks in
						 * the cache file 
						 * descriptors that were
						 * dirtied before this 
						 * time. */
static int	numBackendsActive;		/* Number of backend write back
					         * processes currently active.
				                 */
/*
 * Pointer to LRU list that is used for block allocation.
 */
static	List_Links	lruListHdr;
#define	lruList		(&lruListHdr)

/*
 * There are two free lists.  The first contains blocks that are in pages that
 * only contain free blocks.  The second contains blocks that are in pages that
 * contain non-free blocks.  When the physical pages size <= the block size then
 * the second list will always be empty.
 */
static	List_Links	totFreeListHdr;
#define	totFreeList	(&totFreeListHdr)
static	List_Links	partFreeListHdr;
#define	partFreeList	(&partFreeListHdr)

/*
 * Pointer to list of unmapped blocks.
 */
static	List_Links	unmappedListHdr;
#define	unmappedList	(&unmappedListHdr)

/*
 * List of Fscache_Backend's that could have file in the cache.
 * This list is kept so CacheWriteBack has a list of all the
 * backends that could have files in the cache. 
 */
static	List_Links	backendListHdr;
#define	backendList 	(&backendListHdr)

/*
 * Writes and reads can block on a full cache.  This list records the
 * processes that are blocked on this condition.
 */
List_Links fscacheFullWaitListHdr;
List_Links *fscacheFullWaitList = &fscacheFullWaitListHdr;

/*
 * Hash tables for blocks.
 */
static	Hash_Table	blockHashTableStruct;
static	Hash_Table	*blockHashTable = &blockHashTableStruct;

/*
 * The key to use for the block hash and a macro to set it.  The fact
 * that the key includes a pointer into the I/O handle for the block
 * means that this handle has to be kept around until there are no
 * blocks left in the cache.
 */
typedef	struct {
    Fscache_FileInfo *cacheInfoPtr;
    int		blockNumber;
} BlockHashKey;
#define	SET_BLOCK_HASH_KEY(blockHashKey, ZcacheInfoPtr, fileBlock) \
    (blockHashKey).cacheInfoPtr = ZcacheInfoPtr; \
    (blockHashKey).blockNumber = fileBlock;

/*
 * Miscellaneous variables.
 */
static	Address	blockCacheStart;	/* The address of the beginning of the
				   	   block cache. */
static	Address	blockCacheEnd;		/* The maximum virtual address for the 
					 * cache.*/
static	int	pageSize;		/* The size of a physical page. */
static	int	blocksPerPage;		/* Number of blocks in a page. */

static  int	numAvailBlocks;		/* Number of cache block available for 
					 * use without waiting. */
static  int	minNumAvailBlocks;	/* Minimum number of cache blocks to
					 * keep available. */

/*
 * Macros for large page sizes.
 *
 *	GET_OTHER_BLOCK		Return a pointer to the other block in
 *				the page given a pointer to one of the blocks.
 *	PAGE_IS_8K		Return true if the VM page size is 8K.
 */
#define GET_OTHER_BLOCK(blockPtr) \
    (((int) blockPtr->blockAddr & (pageSize - 1)) != 0) ?  \
		blockPtr - 1 : blockPtr + 1

#define	PAGE_IS_8K	(pageSize == 8192)

/*
 * External symbols.
 */

int	fscache_MaxBlockCleaners = FSCACHE_MAX_CLEANER_PROCS;

/*
 * Internal functions.
 */
static void CacheFileInvalidate _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
			int firstBlock, int lastBlock));
static void DeleteBlockFromDirtyList _ARGS_((Fscache_Block *blockPtr));
static void StartFileSync _ARGS_((Fscache_FileInfo *cacheInfoPtr));
static void CacheWriteBack _ARGS_((unsigned int writeBackTime, 
			int *blocksSkippedPtr, Boolean writeTmpFiles));
static Boolean CreateBlock _ARGS_((Boolean retBlock,
			Fscache_Block **blockPtrPtr));
static Boolean DestroyBlock _ARGS_((Boolean retOnePage, int *pageNumPtr));
#ifdef SOSP91
static Fscache_Block *FetchBlock _ARGS_((Boolean canWait, Boolean cantBlock,
			unsigned int flags));
#else
static Fscache_Block *FetchBlock _ARGS_((Boolean canWait, Boolean cantBlock));
#endif /* SOSP91 */
static void StartBackendWriteback _ARGS_((Fscache_Backend *backendPtr));
static void PutOnFreeList _ARGS_((Fscache_Block *blockPtr));
static void PutFileOnDirtyList _ARGS_((Fscache_FileInfo *cacheInfoPtr,
			int oldestDirtyBlockTime));
static void PutBlockOnDirtyList _ARGS_((Fscache_Block *blockPtr, 
			Boolean onFront));
static Hash_Entry *GetUnlockedBlock _ARGS_((BlockHashKey *blockHashKeyPtr, 
			int blockNum));
static void DeleteBlock _ARGS_((Fscache_Block *blockPtr));


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_Init --
 *
 * 	Initialize the cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash tables initialized and memory allocated for the cache.
 *
 * ----------------------------------------------------------------------------
 */
void
Fscache_Init(blockHashSize)
    int	blockHashSize;	/* The number of hash table entries to put in the
			   block hash table for starters. */
{
    register	Address		blockAddr;
    Address			listStart;
    register	Fscache_Block	*blockPtr;
    register	int		i;

    Vm_FsCacheSize(&blockCacheStart, &blockCacheEnd);
    pageSize = Vm_GetPageSize();
    blocksPerPage = pageSize / FS_BLOCK_SIZE;
    /*
     * Currently the cache code only handles the case for 
     * (pageSize != FS_BLOCK_SIZE) when the block size is 4K and 
     * the page size 8K.
     */
    if (!((pageSize == FS_BLOCK_SIZE) ||
	  ((FS_BLOCK_SIZE == 4096) && PAGE_IS_8K))) {
	panic("Bad pagesize (%d) for file cache code\n", pageSize);
    }
    fs_Stats.blockCache.maxNumBlocks = 
			(blockCacheEnd - blockCacheStart + 1) / FS_BLOCK_SIZE;

    fs_Stats.blockCache.minCacheBlocks = FSCACHE_MIN_BLOCKS;
    fs_Stats.blockCache.maxCacheBlocks = fs_Stats.blockCache.maxNumBlocks;

    /*
     * Allocate space for the cache block list.
     */
    listStart = Vm_RawAlloc((int)fs_Stats.blockCache.maxNumBlocks *
			    sizeof(Fscache_Block));
    blockPtr = (Fscache_Block *) listStart;

    /*
     * Initialize the hash table.
     */
    Hash_Init(blockHashTable, blockHashSize, Hash_Size(sizeof(BlockHashKey)));

    /*
     * Initialize all lists.
     */
    List_Init(lruList);
    List_Init(totFreeList);
    List_Init(partFreeList);
    List_Init(backendList);
    List_Init(unmappedList);
    List_Init(fscacheFullWaitList);

    for (i = 0, blockAddr = blockCacheStart, 
		blockPtr = (Fscache_Block *)listStart;
	 i < fs_Stats.blockCache.maxNumBlocks; 
	 i++, blockPtr++, blockAddr += FS_BLOCK_SIZE) {
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	blockPtr->blockAddr = blockAddr;
	blockPtr->refCount = 0;
	List_Insert(&blockPtr->useLinks, LIST_ATREAR(unmappedList));
    }
#ifdef sun4
    {
	/*
	 * Hack that allows VM to be backward compat with old block cache
	 * code.  This should have been removed by the time you see it.
	 */
	extern Boolean vmMachCanStealFileCachePmegs;
	vmMachCanStealFileCachePmegs = TRUE;
    }
#endif
    /*
     * Give enough blocks memory so that the minimum cache size requirement
     * is met.
     */
    fs_Stats.blockCache.numCacheBlocks = 0;
    while (fs_Stats.blockCache.numCacheBlocks < 
					fs_Stats.blockCache.minCacheBlocks) {
	if (!CreateBlock(FALSE, (Fscache_Block **) NIL)) {
	    printf("Fscacahe_Init: Couldn't create block\n");
	    fs_Stats.blockCache.minCacheBlocks = 
					fs_Stats.blockCache.numCacheBlocks;
	}
    }
    minNumAvailBlocks = fs_Stats.blockCache.minCacheBlocks/2;
    printf("FS Cache has %d %d-Kbyte blocks (%d max)\n",
	    fs_Stats.blockCache.minCacheBlocks, FS_BLOCK_SIZE / 1024,
	    fs_Stats.blockCache.maxNumBlocks);

}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_FileInfoInit --
 *
 * 	Initialize the cache information for a file.  Called when setting
 *	up a handle for a file that uses the block cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the fields of the Fscache_FileInfo struct. First file may
 *	cause the backend to be initialized.
 *	
 * ----------------------------------------------------------------------------
 */
void
Fscache_FileInfoInit(cacheInfoPtr, hdrPtr, version, cacheable, attrPtr, 
		backendPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* Information to initialize. */
    Fs_HandleHeader	     *hdrPtr;		/* Back pointer to handle */
    int			     version;		/* Used to check consistency */
    Boolean		     cacheable;		/* TRUE if server says we can
						 * cache */
    Fscache_Attributes	     *attrPtr;		/* File attributes */
    Fscache_Backend	     *backendPtr;	/* Cache backend for
						 * this file. */
{
    List_InitElement(&cacheInfoPtr->links);
    List_Init(&cacheInfoPtr->dirtyList);
    List_Init(&cacheInfoPtr->blockList);
    List_Init(&cacheInfoPtr->indList);
    cacheInfoPtr->flags = (cacheable ? 0 : FSCACHE_FILE_NOT_CACHEABLE);
    cacheInfoPtr->version = version;
    cacheInfoPtr->hdrPtr = hdrPtr;
    cacheInfoPtr->blocksWritten = 0;
    cacheInfoPtr->noDirtyBlocks.waiting = 0;
    cacheInfoPtr->blocksInCache = 0;
    cacheInfoPtr->numDirtyBlocks = 0;
    cacheInfoPtr->lastTimeTried = 0;
    cacheInfoPtr->oldestDirtyBlockTime = Fsutil_TimeInSeconds();
    cacheInfoPtr->attr = *attrPtr;
    cacheInfoPtr->backendPtr = backendPtr;
    Sync_LockInitDynamic(&cacheInfoPtr->lock, "Fs:perFileCacheLock");
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_InfoSyncLockCleanup --
 *
 * 	Clean up Sync_Lock tracing for the cache lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the fields of the Fscache_FileInfo struct.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fscache_InfoSyncLockCleanup(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    Sync_LockClear(&cacheInfoPtr->lock);
}



/*
 *----------------------------------------------------------------------
 *
 * Fscache_RegisterBackend --
 *
 *	Allocate and initialize the Fscache_Backend data structure for a 
 *	cache backend.
 *
 * Results:
 *	The malloc'ed cache backend.
 *
 * Side effects:
 *	The backend is added to the backendList.
 *
 *----------------------------------------------------------------------
 */

Fscache_Backend *
Fscache_RegisterBackend(ioProcsPtr, clientData, flags)
    Fscache_BackendRoutines   *ioProcsPtr;
    ClientData	clientData;
    int	     flags;	/* Backend flags. */
{
    Fscache_Backend	*backendPtr;

    LOCK_MONITOR;

    backendPtr = (Fscache_Backend *) malloc(sizeof(Fscache_Backend));
    bzero((char *) backendPtr, sizeof(Fscache_Backend));

    List_Init((List_Links *)backendPtr);
    backendPtr->clientData = clientData;
    backendPtr->flags = flags;
    List_Init((List_Links *)&(backendPtr->dirtyListHdr));
    backendPtr->ioProcs = *ioProcsPtr;

    List_Insert((List_Links *)backendPtr, LIST_ATREAR(backendList));
    UNLOCK_MONITOR;
    return backendPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_UnregisterBackend --
 *
 *	Deallocate a Fscache_Backend data structure allocated with
 *	Fscache_RegisterBackend.
 *
 * Results:
 *
 * Side effects:
 *	Backend is removed from the list of active backends and its memory
 *	is freed.
 *
 *----------------------------------------------------------------------
 */

void
Fscache_UnregisterBackend(backendPtr)
    Fscache_Backend	*backendPtr; /* Backend to deallocate. */
{

    LOCK_MONITOR;

    if (!List_IsEmpty(&(backendPtr->dirtyListHdr))) {
	UNLOCK_MONITOR;
	panic("Fscache_UnregisterBackend: Backend has dirty files.\n");
	return;
    }
    List_Remove((List_Links *)backendPtr);
#ifndef CLEAN
    /*
     * NIL out these fields in hope in catching any bad code that
     * trys to use them.
     */
    backendPtr->clientData = (ClientData) NIL;
    backendPtr->ioProcs.allocate = (ReturnStatus (*)()) NIL;
    backendPtr->ioProcs.blockRead = (ReturnStatus (*)()) NIL;
    backendPtr->ioProcs.blockWrite = (ReturnStatus (*)()) NIL;
    backendPtr->ioProcs.reallocBlock = (void (*)()) NIL;
    backendPtr->ioProcs.startWriteBack = (ReturnStatus (*)()) NIL;
#endif /* not CLEAN */

    free((char *) backendPtr);

    UNLOCK_MONITOR;
    return;
}



/*
 * ----------------------------------------------------------------------------
 *
 *	Functions for external block access.  Includes functions to fetch,
 *	release and truncate cache blocks.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_FetchBlock --
 *
 *	Return in *blockPtrPtr a pointer to a block in the 
 *	cache that corresponds to virtual block blockNum in the file 
 *	identified by *cacheInfoPtr.  If the block for the file is not in the 
 *	cache then *foundPtr is set to FALSE and if allocate is 
 *	TRUE, then a clean block is returned.  Otherwise a pointer to the 
 *	actual data block is returned and *foundPtr is set to TRUE.
 *	The block that is returned is locked down in the cache (i.e. it cannot
 *	be replaced) until it is unlocked by Fscache_UnlockBlock.  If the block
 *	isn't found or the FSCACHE_IO_IN_PROGRESS flag is given then the block
 *	is marked as IO in progress and must be either unlocked by 
 *	Fscache_UnlockBlock or marked as IO done by Fscache_IODone.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New block may be allocated out of the block cache, block that 
 *	is returned is locked, and the block may be set as IO in progress.
 *
 * ----------------------------------------------------------------------------
 *
 */
static Fscache_Block *lostBlockPtr;

ENTRY void
Fscache_FetchBlock(cacheInfoPtr, blockNum, flags, blockPtrPtr, foundPtr)
    register Fscache_FileInfo *cacheInfoPtr; /* Pointer to the cache state 
				   * for the file. */
    int		 blockNum;	/* Virtual block number in the file. */
    int		 flags;		/* FSCACHE_DONT_BLOCK |
				 * FSCACHE_CANT_BLOCK |
				 * FSCACHE_READ_AHEAD_BLOCK |
				 * FSCACHE_IO_IN_PROGRESS
				 * plus the type of block */
    Fscache_Block **blockPtrPtr; /* Where pointer to cache block information
				 * structure is returned. The structure
				 * contains the virtual address of the 
				 * actual cache block. */
    Boolean	*foundPtr;	/* TRUE if the block is present in the
				 * cache, FALSE if not. */
{
    BlockHashKey		blockHashKey;
    register	Hash_Entry	*hashEntryPtr;
    register	Fscache_Block	*blockPtr;
    Fscache_Block		*otherBlockPtr;
    Fscache_Block		*newBlockPtr;
    int				refTime;
    Boolean		cantBlock = (flags & FSCACHE_CANT_BLOCK);
    Boolean		dontBlock = (flags & FSCACHE_DONT_BLOCK);

    LOCK_MONITOR;

    *blockPtrPtr = (Fscache_Block *)NIL;
    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, blockNum);

    do {
	/*
	 * Keep re-hashing until we get a block.  If we ever have to
	 * wait in this loop then the hash table can change out from
	 * under us, so we always rehash.
	 */
	hashEntryPtr = Hash_Find(blockHashTable, (Address) &blockHashKey);
	blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);
	if (blockPtr != (Fscache_Block *) NIL) {
	    Boolean	blockBusy;
	    *foundPtr = TRUE;
	    if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
		UNLOCK_MONITOR;
		panic("Fscache_FetchBlock hashing error\n");
		*foundPtr = FALSE;
		return;
	    }
	    blockBusy = ((blockPtr->refCount > 0) || 
		    (blockPtr->flags & (FSCACHE_IO_IN_PROGRESS|
					FSCACHE_BLOCK_BEING_WRITTEN)));
	    if ( ((flags & FSCACHE_IO_IN_PROGRESS) && blockBusy) ||
		  (blockPtr->flags & FSCACHE_IO_IN_PROGRESS)) {
		if (!dontBlock) {
		    /*
		     * Wait until it becomes unlocked, or return
		     * found = TRUE and block = NIL if caller can't block.
		     */
		    (void)Sync_Wait(&blockPtr->ioDone, FALSE);
		}
		blockPtr = (Fscache_Block *)NIL;
	    } else {
		blockPtr->refCount++;
		if (blockPtr->refCount == 1) {
		    VmMach_LockCachePage(blockPtr->blockAddr);
		    if (!(blockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
			numAvailBlocks--;
			if (numAvailBlocks < 0) {
			    panic("Fscache_FetchBlock numAvailBlocks < 0\n");
			}
		    }
		}
		if (flags & FSCACHE_IO_IN_PROGRESS) {
		    blockPtr->flags |= FSCACHE_IO_IN_PROGRESS;
		}
	    }
	} else {
	    /*
	     * Have to allocate a block.  If there is a free block use it, or
	     * take a block off of the lru list, or make a new one.
	     */
	    *foundPtr = FALSE;
	    if ((numAvailBlocks > minNumAvailBlocks) || cantBlock) {
		/*
		 * If we have enought blocks available take a free one.
		 */
		if (!List_IsEmpty(partFreeList)) {
		    /*
		     * Use partially free blocks first.
		     */
		    fs_Stats.blockCache.numFreeBlocks--;
		    fs_Stats.blockCache.partFree++;
		    blockPtr = USE_LINKS_TO_BLOCK(List_First(partFreeList));
		    List_Remove(&blockPtr->useLinks);
		} else if (!List_IsEmpty(totFreeList)) {
		    /*
		     * Can't find a partially free block so use a totally free
		     * block.
		     */
		    fs_Stats.blockCache.numFreeBlocks--;
		    fs_Stats.blockCache.totFree++;
		    blockPtr = USE_LINKS_TO_BLOCK(List_First(totFreeList));
		    List_Remove(&blockPtr->useLinks);
		    if (PAGE_IS_8K) {
			otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
			List_Move(&otherBlockPtr->useLinks,
				      LIST_ATREAR(partFreeList));
		    }
		}
	    }
	    if (blockPtr == (Fscache_Block *) NIL) {
		/*
		 * Can't find any free blocks so have to use one of our blocks
		 * or create new ones.
		 */
		if (fs_Stats.blockCache.numCacheBlocks >= 
				    fs_Stats.blockCache.maxCacheBlocks) {
		    /*
		     * We can't have anymore blocks so reuse one of our own.
		     */
#ifdef SOSP91
		    blockPtr = FetchBlock(!dontBlock, cantBlock,
			    FSCACHE_BLOCK_LRU);
#else
		    blockPtr = FetchBlock(!dontBlock, cantBlock);
#endif /* SOSP91 */
		    if ((blockPtr == (Fscache_Block *) NIL) && cantBlock) {
			goto getBlock;
		    }
		} else {
		    /*
		     * Grow the cache if VM has an older page than we have.
		     */
		    refTime = Vm_GetRefTime();
		    blockPtr = USE_LINKS_TO_BLOCK(List_First(lruList));
		    if (blockPtr->timeReferenced > refTime) {
		getBlock:
			if (!CreateBlock(TRUE, &newBlockPtr)) {
#ifdef SOSP91
			    blockPtr = FetchBlock(!dontBlock, cantBlock,
				    FSCACHE_BLOCK_LRU);
#else
			    blockPtr = FetchBlock(!dontBlock, cantBlock);
#endif /* SOSP91 */
			} else {
			    fs_Stats.blockCache.unmapped++;
			    blockPtr = newBlockPtr;
			}
		    } else {
			/*
			 * We have an older block than VM's oldest page so reuse
			 * the block.
			 */
#ifdef SOSP91
			blockPtr = FetchBlock(!dontBlock, cantBlock,
				FSCACHE_BLOCK_LRU);
#else
			blockPtr = FetchBlock(!dontBlock, cantBlock);
#endif /* SOSP91 */
		    }
		}
	    }
	    /*
	     * If blockPtr is NIL we waited for room in the cache or
	     * for a busy cache block.  Now we'll retry all the various
	     * ploys to get a free block.
	     */
	}
    } while ((blockPtr == (Fscache_Block *)NIL) && !dontBlock);

    if ((*foundPtr == FALSE) && (blockPtr != (Fscache_Block *)NIL)) {
	cacheInfoPtr->blocksInCache++;
	blockPtr->cacheInfoPtr = cacheInfoPtr;
	blockPtr->refCount = 1;
	VmMach_LockCachePage(blockPtr->blockAddr);
	blockPtr->flags = flags & (FSCACHE_DATA_BLOCK | FSCACHE_IND_BLOCK |
				   FSCACHE_DESC_BLOCK | FSCACHE_DIR_BLOCK |
				   FSCACHE_READ_AHEAD_BLOCK);
	blockPtr->flags |= FSCACHE_IO_IN_PROGRESS;
	blockPtr->fileNum = cacheInfoPtr->hdrPtr->fileID.minor;
	blockPtr->blockNum = blockNum;
	blockPtr->blockSize = -1;
	blockPtr->timeDirtied = 0;
	blockPtr->timeReferenced = Fsutil_TimeInSeconds();
	*blockPtrPtr = blockPtr;
	if (Hash_GetValue(hashEntryPtr) != (char *)NIL) {
	    lostBlockPtr = (Fscache_Block *)Hash_GetValue(hashEntryPtr);
	    UNLOCK_MONITOR;
	    panic("Fscache_FetchBlock: hashEntryPtr->value changed\n");
	    LOCK_MONITOR;
	}
	Hash_SetValue(hashEntryPtr, blockPtr);
	List_Insert(&(blockPtr->useLinks), LIST_ATREAR(lruList));
	List_InitElement(&blockPtr->fileLinks);
	if (flags & FSCACHE_IND_BLOCK) {
	    List_Insert(&blockPtr->fileLinks, LIST_ATREAR(&cacheInfoPtr->indList));
	} else {
	    List_Insert(&blockPtr->fileLinks,LIST_ATREAR(&cacheInfoPtr->blockList));
	}
	numAvailBlocks--;
	if (numAvailBlocks < 0) {
	    panic("Fscache_FetchBlock numAvailBlocks < 0.\n");
	}
    }
    *blockPtrPtr = blockPtr;
    UNLOCK_MONITOR;
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_IODone --
 *
 *	Remove the IO-in-progress flag from the cache block flags field.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO-in-progress flag is removed from the cache block flags field.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_IODone(blockPtr)
    Fscache_Block *blockPtr;	/* Pointer to block information for block.*/
{
    LOCK_MONITOR;

    Sync_Broadcast(&blockPtr->ioDone);
    blockPtr->flags &= ~FSCACHE_IO_IN_PROGRESS;

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UnlockBlock --
 *
 *	Release the lock on the cache block pointed to by blockPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock count of the block is decremented.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_UnlockBlock(blockPtr, timeDirtied, diskBlock, blockSize, flags)
    Fscache_Block *blockPtr;	/* Pointer to block information for block
				   that is to be released. */
    unsigned int timeDirtied;	/* Time in seconds that the block was 
				   dirtied. */
    int		 diskBlock;	/* If not -1 is the block on disk where this
				   block resides.  For remote blocks this 
				   should be the same as blockPtr->blockNum.*/
    int		 blockSize;	/* The number of valid bytes in this block. */
    int		 flags;		/* FSCACHE_DELETE_BLOCK | FSCACHE_CLEAR_READ_AHEAD |
				 * FSCACHE_BLOCK_UNNEEDED | FSCACHE_DONT_WRITE_THRU
				 * FSCACHE_WRITE_TO_DISK | FSCACHE_BLOCK_BEING_CLEANED*/
{
    LOCK_MONITOR;

    if (blockPtr->flags & FSCACHE_BLOCK_FREE) {
	panic("Checking in free block\n");
    }

    if (blockPtr->flags & FSCACHE_IO_IN_PROGRESS) {
	Sync_Broadcast(&blockPtr->ioDone);
	blockPtr->flags &= ~FSCACHE_IO_IN_PROGRESS;
    }

    if (flags & FSCACHE_DELETE_BLOCK) {
	/*
	 * The caller is deleting this block from the cache.  Decrement the
	 * lock count and then invalidate the block.
	 */
	blockPtr->refCount--;
	if (blockPtr->refCount == 0) { 
	    VmMach_UnlockCachePage(blockPtr->blockAddr);
	    if (!(blockPtr->flags & 
			(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN))) {
		numAvailBlocks++;
		if (! List_IsEmpty(fscacheFullWaitList)) {
		    Fsutil_WaitListNotify(fscacheFullWaitList);
		}
		Sync_Broadcast(&cleanBlockCondition);
	    }
	}
	CacheFileInvalidate(blockPtr->cacheInfoPtr, blockPtr->blockNum, 
			    blockPtr->blockNum);
	UNLOCK_MONITOR;
	return;
    }

    if (flags & FSCACHE_CLEAR_READ_AHEAD) {
	blockPtr->flags &= ~FSCACHE_READ_AHEAD_BLOCK;
    }
    if (timeDirtied != 0) {
	if (flags & FSCACHE_BLOCK_BEING_CLEANED) {
	    blockPtr->cacheInfoPtr->flags |= FSCACHE_FILE_BEING_CLEANED;
	    blockPtr->flags |= FSCACHE_BLOCK_BEING_CLEANED;
	}
	if (!(blockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
	    /*
	     * Increment the count of dirty blocks if the block isn't marked
	     * as dirty.  The block cleaner will decrement the count 
	     * after it cleans a block.
	     */
	    blockPtr->flags |= FSCACHE_BLOCK_DIRTY;
	    blockPtr->timeDirtied = timeDirtied;
	    if (!(blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN)) { 
		blockPtr->cacheInfoPtr->numDirtyBlocks++;
		PutBlockOnDirtyList(blockPtr, FALSE);
	    }
	}
    }
    if (diskBlock != -1) {
	blockPtr->diskBlock = diskBlock;
	if (blockPtr->blockSize != blockSize) {
	    blockPtr->blockSize = blockSize;
	}
    } else if (blockPtr->blockSize == -1 && blockSize > 0) {
	/*
	 * Patch up the block size so our internal fragmentation
	 * calculation is correct.  The size of a read-only block
	 * is not used for anything else.
	 */
	blockPtr->blockSize = blockSize;
    }

    blockPtr->refCount--;
    if (blockPtr->refCount == 0) {
	/*
	 * Wake up anybody waiting for the block to become unlocked.
	 */
	Sync_Broadcast(&blockPtr->ioDone);
	VmMach_UnlockCachePage(blockPtr->blockAddr);
	if (!(blockPtr->flags & 
		(FSCACHE_BLOCK_DIRTY | FSCACHE_BLOCK_BEING_WRITTEN))) {
	    numAvailBlocks++;
	    if (! List_IsEmpty(fscacheFullWaitList)) {
		Fsutil_WaitListNotify(fscacheFullWaitList);
	    }
	    Sync_Broadcast(&cleanBlockCondition);
	}
	if (blockPtr->flags & FSCACHE_BLOCK_CLEANER_WAITING) {
	    StartBackendWriteback(blockPtr->cacheInfoPtr->backendPtr);
	    blockPtr->flags &= ~FSCACHE_BLOCK_CLEANER_WAITING;
	}
	if (flags & FSCACHE_BLOCK_UNNEEDED) {
	    /*
	     * This block is unneeded so move it to the front of the LRU list
	     * and set its time referenced to zero so that it will be taken
	     * at the next convenience.
	     */
	    if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
		blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	    } else {
		List_Move(&blockPtr->useLinks, LIST_ATFRONT(lruList));
	    }
	    fs_Stats.blockCache.blocksPitched++;
	    blockPtr->timeReferenced = 0;
	} else {
	    /*
	     * Move it to the end of the lru list, mark it as being referenced. 
	     */
	    blockPtr->timeReferenced = Fsutil_TimeInSeconds();
	    blockPtr->flags &= ~FSCACHE_MOVE_TO_FRONT;
	    List_Move( &blockPtr->useLinks, LIST_ATREAR(lruList));
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_BlockTrunc --
 *
 * 	Truncate the given cache block.  Used to set the blockSize in the
 *	cache block to reflect the actual amount of data in the block after
 *	a truncate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	blockSize in block is changed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_BlockTrunc(cacheInfoPtr, blockNum, newBlockSize)
    Fscache_FileInfo *cacheInfoPtr;	/* Cache state of file. */
    int		blockNum;		/* Block to truncate. */
    int		newBlockSize;		/* New block size. */
{
    register Hash_Entry	     *hashEntryPtr;
    register Fscache_Block    *blockPtr;
    BlockHashKey	     blockHashKey;

    LOCK_MONITOR;

    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

    hashEntryPtr = GetUnlockedBlock(&blockHashKey, blockNum);
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    panic( "CacheBlockTrunc, hashing error\n");
	} else {
	    blockPtr->blockSize = newBlockSize;
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to perform some action on a file.  This includes write-back
 *	and invalidation.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_FileInvalidate --
 *
 * 	This function removes from the cache all blocks for the file 
 *	identified by *filePtr in the range firstBlock to lastBlock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All blocks in the cache for the file are removed.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_FileInvalidate(cacheInfoPtr, firstBlock, lastBlock)
    Fscache_FileInfo *cacheInfoPtr;	/* Cache state of file to invalidate. */
    int		firstBlock;	/* First block to invalidate. Starts at zero. */
    int		lastBlock;	/* Last block to invalidate.  FSCACHE_LAST_BLOCK
				 * can be used if the caller doesn't know
				 * the exact last block of the file. */
{
    LOCK_MONITOR;

    if (lastBlock == FSCACHE_LAST_BLOCK) {
	if (cacheInfoPtr->attr.lastByte > 0) {
	    lastBlock = cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE;
	} else {
	    lastBlock = 0;
	}
    }
    CacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);

    UNLOCK_MONITOR;
}



/*
 * ----------------------------------------------------------------------------
 *
 * CacheFileInvalidate --
 *
 * 	This function removes from the cache all blocks for the given file 
 *	identified in the range firstBlock to lastBlock.  If any blocks are 
 *	being written to disk, it will block until they have finished being
 *	written.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All blocks in the cache for the file are removed.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
CacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock)
    Fscache_FileInfo	*cacheInfoPtr;	/* File to invalidate. */
    int			firstBlock;	/* First block to invalidate. */
    int			lastBlock;	/* Last block to invalidate. */
{
    register Hash_Entry	     *hashEntryPtr;
    register Fscache_Block    *blockPtr;
    BlockHashKey	     blockHashKey;
    int			     i;


    if (cacheInfoPtr->blocksInCache > 0) {
	SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

	for (i = firstBlock; i <= lastBlock; i++) {
	    hashEntryPtr = GetUnlockedBlock(&blockHashKey, i);
	    if (hashEntryPtr == (Hash_Entry *) NIL) {
		continue;
	    }
	    blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);
	    if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
		panic( "CacheFileInvalidate, hashing error\n");
		continue;
	    }

	    /*
	     * Remove it from the hash table.
	     */
	    cacheInfoPtr->blocksInCache--;
	    List_Remove(&blockPtr->fileLinks);
	    Hash_Delete(blockHashTable, hashEntryPtr);

	    /*
	     * Invalidate the block, including removing it from dirty list
	     * if necessary.
	     */
	    if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
		cacheInfoPtr->numDirtyBlocks--;
		DeleteBlockFromDirtyList(blockPtr);
		numAvailBlocks++;
	    }
	    List_Remove(&blockPtr->useLinks);
	    PutOnFreeList(blockPtr);
	}
    }
    if (cacheInfoPtr->blocksInCache == 0) {
	cacheInfoPtr->flags &=
			~(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE |
			  FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * DeleteBlockFromDirtyList --
 *
 * 	Delete the given block from the dirty list.  This is done when
 *	the file is being deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If this is the last dirty block of a file then the file is
 *	removed from the file dirty list.  Also, if someone was waiting
 *	on the block the global writeBackComplete is notified.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
DeleteBlockFromDirtyList(blockPtr)
    register	Fscache_Block	*blockPtr;
{
    register	Fscache_FileInfo	*cacheInfoPtr;

    cacheInfoPtr = blockPtr->cacheInfoPtr;
    List_Remove(&blockPtr->dirtyLinks);
    if ((cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) &&
        (cacheInfoPtr->numDirtyBlocks == 0) &&
	!(cacheInfoPtr->flags & FSCACHE_FILE_DESC_DIRTY)) {
	/*
	 * No more dirty blocks for this file.  Remove the file from the dirty
	 * list and wakeup anyone waiting for the file's list to become
	 * empty.
	 */
	List_Remove((List_Links *)cacheInfoPtr);
	cacheInfoPtr->flags &= ~(FSCACHE_FILE_ON_DIRTY_LIST|FSCACHE_FILE_FSYNC|FSCACHE_FILE_BEING_CLEANED);
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
    }

}

/*
 *----------------------------------------------------------------------
 *
 * StartFileSync --
 *
 *	Start the process of syncing from the cache to the backend.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Backend may be started. File may be moved up the dirty list.
 *
 *----------------------------------------------------------------------
 */
static void
StartFileSync(cacheInfoPtr)
    Fscache_FileInfo	*cacheInfoPtr;
{
    register List_Links	*linkPtr;
    List_Links	*dirtyList, *place;

    dirtyList = &cacheInfoPtr->backendPtr->dirtyListHdr;

    /*
     * If the file has not already been synced, move the file up the
     * dirty list to the front or right behide the last synced file.
     */
    if (!(cacheInfoPtr->flags & FSCACHE_FILE_FSYNC)) {
	/*
	 * Move down the list until we reach the file or the first non
	 * synced file. 
	 */
	place = (List_Links *) cacheInfoPtr;
	LIST_FORALL(dirtyList, linkPtr) {
	    if ((linkPtr == (List_Links *) cacheInfoPtr) ||
		!(((Fscache_FileInfo *) linkPtr)->flags & FSCACHE_FILE_FSYNC)) {
		place = linkPtr;
		break;
	    }
	}
	cacheInfoPtr->flags |= FSCACHE_FILE_FSYNC;
	if (place != (List_Links *) cacheInfoPtr) {
	    List_Move((List_Links *)cacheInfoPtr, LIST_BEFORE(place));
	}
    }
    StartBackendWriteback(cacheInfoPtr->backendPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_FileWriteBack --
 *
 * 	This function forces all blocks for the file identified by 
 *	*hdrPtr in the range firstBlock to lastBlock to disk (or 
 *	the server).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All dirty blocks in the cache for the file are written out.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY ReturnStatus
Fscache_FileWriteBack(cacheInfoPtr, firstBlock, lastBlock, flags,
	blocksSkippedPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* State to force out. */
    int		firstBlock;	/* First block to write back. */
    int		lastBlock;	/* Last block to write back. */
    int		flags;		/* FSCACHE_FILE_WB_WAIT | FSCACHE_WRITE_BACK_INDIRECT |
				 * FSCACHE_WRITE_BACK_AND_INVALIDATE. 
				 * FSCACHE_WB_MIGRATION. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
{
    register Hash_Entry	     *hashEntryPtr;
    register Fscache_Block    *blockPtr;
    BlockHashKey	     blockHashKey;
    int			     i;
    ReturnStatus	     status;
    Boolean		     fsyncFile;
    enum  {ENTIRE_FILE, SINGLE_BLOCK, MULTI_BLOCK_RANGE} rangeType;

    LOCK_MONITOR;

    *blocksSkippedPtr = 0;

    fsyncFile = FALSE;
    /* 
     * First classify the type of writeback being requests as either involving
     * a single file block, the entire file, or a multiple-block range.
     */
    if ((firstBlock == lastBlock) && (firstBlock >= 0)) {
	rangeType = SINGLE_BLOCK;
    } else if ((firstBlock == 0) && 
	        ((lastBlock == FSCACHE_LAST_BLOCK) || 
	        (lastBlock == cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE))) {
	rangeType = ENTIRE_FILE;
    } else {
	rangeType = MULTI_BLOCK_RANGE;
    }

    /*
     * Clear out the host down and no disk space flags so we can retry
     * for this file.
     */
    cacheInfoPtr->flags &= 
		    ~(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE | 
		      FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR);

    if ((cacheInfoPtr->blocksInCache == 0) ||
	(flags & FSCACHE_WRITE_BACK_DESC_ONLY)) {
	/*
	 * If file is on dirty list and has no blocks then it 
	 * descriptor is dirty.  Force the descriptor out 
	 * waiting if the user specified to.
	 */
	if ((cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) &&
	    (flags & FSCACHE_WRITE_BACK_DESC_ONLY)) {
	    StartFileSync(cacheInfoPtr);
	    if (flags & FSCACHE_FILE_WB_WAIT) {
		while (cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) {
			(void) Sync_Wait(&cacheInfoPtr->noDirtyBlocks, FALSE);
		}
	    }
	}
	UNLOCK_MONITOR;
	return(SUCCESS);
    }

    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

    if (lastBlock == FSCACHE_LAST_BLOCK) {
	if (cacheInfoPtr->attr.lastByte > 0) {
	    lastBlock = cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE;
	} else {
	    lastBlock = 0;
	}
    }
    blockPtr = (Fscache_Block *) NIL;
    for (i = firstBlock; i <= lastBlock; i++) {
	/*
	 * See if block is in the hash table.
	 */

	blockHashKey.blockNumber = i;
again:
	hashEntryPtr = Hash_LookOnly(blockHashTable, (Address) &blockHashKey);
	if (hashEntryPtr == (Hash_Entry *) NIL) {
	    continue;
	}

	blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    panic( "Fscache_FileWriteBack, hashing error\n");
	    UNLOCK_MONITOR;
	    return(FAILURE);
	}

	if (flags & (FSCACHE_WRITE_BACK_AND_INVALIDATE | FSCACHE_FILE_WB_WAIT)) {
	    /*
	     * Wait for the block to become unlocked.  If have to wait then
	     * must start over because the block could have been freed while
	     * we were sleeping.
	     */
	    if (blockPtr->refCount > 0) {
		(void) Sync_Wait(&blockPtr->ioDone, FALSE);
		if (sys_ShuttingDown) {
		    UNLOCK_MONITOR;
		    return(SUCCESS);
		}
		goto again;
	    }
	} else if (blockPtr->refCount > 0) {
	    /* 
	     * If the block is locked and we are not invalidating it, 
	     * skip the block because it might be being modified.
	     */
	    (*blocksSkippedPtr)++;
	    continue;
	}

	if (blockPtr->flags & (FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) {
	    fsyncFile = TRUE;
	}
	if (flags & FSCACHE_WRITE_BACK_AND_INVALIDATE) {
	    if (blockPtr->flags & 
			(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) {
		/*
		 * Mark the block to be release by the block cleaner. We
		 * delete it from the hash table and remove it from the LRU
		 * list for the block cleaner. 
		 */
		if (!(blockPtr->flags & FSCACHE_BLOCK_DELETED)) { 
		    blockPtr->flags |= FSCACHE_BLOCK_DELETED;
		    Hash_Delete(blockHashTable, hashEntryPtr);
		    List_Remove( &blockPtr->useLinks);
		}
	    } else {
		/*
		 * The block is clean and not being written, We remove it
		 * from cache.
		 */
		cacheInfoPtr->blocksInCache--;
		List_Remove(&blockPtr->fileLinks);
		Hash_Delete(blockHashTable, hashEntryPtr);
		List_Remove(&blockPtr->useLinks);
		PutOnFreeList(blockPtr);
	    }
	} 
    }
    if ((cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) && fsyncFile) {
	StartFileSync(cacheInfoPtr);
    }

    /*
     * Wait until all blocks are written back.
     */
    if (flags & FSCACHE_FILE_WB_WAIT) {
	 if ((rangeType == SINGLE_BLOCK) && fsyncFile) {
	    while ((blockPtr->flags & 
		(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) && 
		   !(cacheInfoPtr->flags & 
			(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE |
			 FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR)) &&
		   !sys_ShuttingDown) {
		(void) Sync_Wait(&blockPtr->ioDone, FALSE);
	    }
	 } else { 
	    while ((cacheInfoPtr->flags & 
		    (FSCACHE_FILE_ON_DIRTY_LIST|FSCACHE_FILE_BEING_WRITTEN)) && 
		   !(cacheInfoPtr->flags & 
			(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE |
			 FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR)) &&
		   !sys_ShuttingDown) {
		(void) Sync_Wait(&cacheInfoPtr->noDirtyBlocks, FALSE);
	    }
	}
    }

    switch (cacheInfoPtr->flags&(FSCACHE_SERVER_DOWN|FSCACHE_NO_DISK_SPACE|
				 FSCACHE_DOMAIN_DOWN|FSCACHE_GENERIC_ERROR)) {
	case FSCACHE_SERVER_DOWN:
	    status = RPC_TIMEOUT;
	    break;
	case FSCACHE_NO_DISK_SPACE:
	    status = FS_NO_DISK_SPACE;
	    break;
	case FSCACHE_DOMAIN_DOWN:
	    status = FS_DOMAIN_UNAVAILABLE;
	    break;
	case FSCACHE_GENERIC_ERROR:
	    status = FS_INVALID_ARG;
	    break;
	default:
	    status = SUCCESS;
	    break;
    }

    UNLOCK_MONITOR;

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_BlocksUnneeded --
 *
 *	This function moves the blocks that span the given range of bytes to
 *	the front of the LRU list and marks them as not referenced.  This
 *	function is called by virtual memory after it has read in an object
 *	file block that it will cache in a sticky segment.  If we are the
 *	file server and are being called for an object file, then don't do 
 *	anything because other clients might need the blocks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None here, see FscacheBlocksUnneeded.
 *	All blocks that span the given range of bytes are moved to the front of
 *	the LRU list and marked as not-referenced.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
Fscache_BlocksUnneeded(streamPtr, offset, numBytes, objectFile)
    register Fs_Stream	*streamPtr;	/* File for which blocks are unneeded.*/
    int			offset;		/* First byte which is unneeded. */
    int			numBytes;	/* Number of bytes that are unneeded. */
    Boolean		objectFile;	/* TRUE if this is for an object 
					 * file.*/
{
    register Fscache_FileInfo *cacheInfoPtr;

    switch (streamPtr->ioHandlePtr->fileID.type) {
	case FSIO_LCL_FILE_STREAM: {
	    register Fsio_FileIOHandle *localHandlePtr;
	    if (objectFile) {
		/*
		 * Keep the blocks cached for remote clients.
		 */
		return;
	    }
	    localHandlePtr = (Fsio_FileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &localHandlePtr->cacheInfo;
	    break;
	}
	case FSIO_RMT_FILE_STREAM: {
	    register Fsrmt_FileIOHandle *rmtHandlePtr;
	    rmtHandlePtr = (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &rmtHandlePtr->cacheInfo;
	    break;
	}
	default:
	    panic( "Fscache_BlocksUnneeded, bad stream type %d\n",
		streamPtr->ioHandlePtr->fileID.type);
	    return;
    }
    FscacheBlocksUnneeded(cacheInfoPtr, offset, numBytes);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FscacheBlocksUnneeded --
 *
 *	This function moves the blocks that span the given range of bytes to
 *	the front of the LRU list and marks them as not referenced.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All blocks that span the given range of bytes are moved to the front of
 *	the LRU list and marked as not-referenced.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FscacheBlocksUnneeded(cacheInfoPtr, offset, numBytes)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state. */
    int			offset;		/* First byte which is unneeded. */
    int			numBytes;	/* Number of bytes that are unneeded. */
{
    register Hash_Entry		*hashEntryPtr;
    register Fscache_Block    	*blockPtr;
    BlockHashKey	     	blockHashKey;
    int			     	i;
    int				firstBlock;
    int				lastBlock;

    LOCK_MONITOR;

    if (cacheInfoPtr->blocksInCache == 0) {
	UNLOCK_MONITOR;
	return;
    }

    firstBlock = offset / FS_BLOCK_SIZE;
    lastBlock = (offset + numBytes - 1) / FS_BLOCK_SIZE;
    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

    for (i = firstBlock; i <= lastBlock; i++) {
	/*
	 * See if block is in the hash table.
	 */
	blockHashKey.blockNumber = i;
	hashEntryPtr = Hash_LookOnly(blockHashTable, (Address) &blockHashKey);
	if (hashEntryPtr == (Hash_Entry *) NIL) {
	    continue;
	}

	blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    panic( "CacheBlocksUnneeded, hashing error\n");
	    continue;
	}

	if (blockPtr->refCount > 0) {
	    /*
	     * The block is locked.  This means someone is doing something with
	     * it so we just skip it.
	     */
	    continue;
	}
	if (blockPtr->flags & 
			(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) {
	    /*
	     * Block is being cleaned.  Set the flag so that it will be 
	     * moved to the front after it has been cleaned.
	     */
	    blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	} else {
	    /*
	     * Move the block to the front of the LRU list.
	     */
	    List_Move(&blockPtr->useLinks, LIST_ATFRONT(lruList));
	}
	fs_Stats.blockCache.blocksPitched++;
	/*
	 * Set time referenced to zero so this block will be taken as soon
	 * as needed.
	 */
	blockPtr->timeReferenced = 0;
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to perform an action on the entire cache. 
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_WriteBack --
 *
 *	Force all dirty blocks in the cache that were dirtied before
 *	writeBackTime to disk (or the server).  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All dirty blocks in the cache are written out.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_WriteBack(writeBackTime, blocksSkippedPtr, writeBackAll)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	writeBackAll;	   /* Write back all files. */
{
    LOCK_MONITOR;
    CacheWriteBack(writeBackTime, blocksSkippedPtr, writeBackAll);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_Empty --
 *
 *	Write back and invalidate all unlocked blocks from the cache.
 *
 * Results:
 *	The number of locked blocks is returned in the argument.
 *
 * Side effects:
 *	All unlocked blocks are written back, if necessary, and invalidated.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_Empty(numLockedBlocksPtr)
    int *numLockedBlocksPtr;
{
    int 			blocksSkipped;
    register	Fscache_Block	*blockPtr;
    register	List_Links	*nextPtr, *listPtr;

    LOCK_MONITOR;

    *numLockedBlocksPtr = 0;
    CacheWriteBack(-1, &blocksSkipped, TRUE);
    listPtr = lruList;
    nextPtr = List_First(listPtr);
    while (!List_IsAtEnd(listPtr, nextPtr)) {
	blockPtr = USE_LINKS_TO_BLOCK(nextPtr);
	nextPtr = List_Next(nextPtr);
	if (blockPtr->refCount > 0 || 
	    (blockPtr->flags & 
		(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN))) {
	    /* 
	     * Skip locked or dirty blocks.
	     */
	    (*numLockedBlocksPtr)++;
	} else {
	    List_Remove(&blockPtr->useLinks);
	    DeleteBlock(blockPtr);
	    PutOnFreeList(blockPtr);
	}
    }
#ifdef SOSP91
    /* XXX Darn - I'll miss blocks quicked out of the cache due to sync. */
#endif SOSP91

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * CacheWriteBack --
 *
 *	Force all dirty blocks in the cache that were dirtied before
 *	writeBackTime to disk (or the server).  If writeBackTime equals 
 *	-1 then all blocks are written back.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All dirty blocks in the cache are written out.
 *
 * ----------------------------------------------------------------------------
 *
 */
static INTERNAL void
CacheWriteBack(writeBackTime, blocksSkippedPtr, writeTmpFiles)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	writeTmpFiles;	   /* TRUE => write-back tmp files even though
				    *         they are marked as not being
				    *         written back. */
{
    register Fscache_FileInfo	*cacheInfoPtr;
    int				currentTime;
    Fscache_Backend		*backendPtr;

    currentTime = Fsutil_TimeInSeconds();

    *blocksSkippedPtr = 0;
    /*
     * Look thru all the cache backend`s dirty list for files to 
     * writeback.
     */
    filewriteBackTime = writeBackTime;
    LIST_FORALL(backendList, (List_Links *) backendPtr) {
	LIST_FORALL(&backendPtr->dirtyListHdr, (List_Links *) cacheInfoPtr) { 
	    /*
	     * Check to see if we should start a block cleaner for this
	     * backend.
	     */
	    if (cacheInfoPtr->flags & (FSCACHE_SERVER_DOWN|FSCACHE_FILE_GONE)) {
		/*
		 * Don't bother to write-back files for which the server is
		 * down.  These will be written back during recovery.
		 */
		continue;
	    }
	    if (cacheInfoPtr->flags &
		    (FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
		     FSCACHE_GENERIC_ERROR)) {
		/*
		 * Retry for these types of errors.
		 */
		if (cacheInfoPtr->lastTimeTried < currentTime) {
		    cacheInfoPtr->flags &=
			~(FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
				  FSCACHE_GENERIC_ERROR);
		    StartBackendWriteback(cacheInfoPtr->backendPtr);
		    break;
		} else {
		    continue;
		}
	    }
	    if ((numAvailBlocks < minNumAvailBlocks + FSCACHE_MIN_BLOCKS) ||
		(cacheInfoPtr->oldestDirtyBlockTime < writeBackTime) ||
		(cacheInfoPtr->flags & FSCACHE_FILE_FSYNC)) {
		StartBackendWriteback(cacheInfoPtr->backendPtr);
		break;
	    }
	}
    }
    /*
     * Wait for all block cleaners to go idea before returning.
     */
    while ((numBackendsActive > 0) && !sys_ShuttingDown) {
	(void) Sync_Wait(&writeBackComplete, FALSE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to create, destroy and allocate cache blocks.  These 
 *	functions are used to provide the variable sized cache and the LRU
 *	algorithm for managing cache blocks.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_SetMinSize --
 *
 * 	Set the minimum size of the block cache.  This will entail mapping
 *	enough blocks so that the number of physical pages in use is greater
 *	than or equal to the minimum number.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	More blocks get memory put behind them.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_SetMinSize(minBlocks)
    int	minBlocks;	/* The minimum number of blocks in the cache. */
{
    LOCK_MONITOR;



    if (minBlocks > fs_Stats.blockCache.maxNumBlocks) {
	minBlocks = fs_Stats.blockCache.maxNumBlocks;
	printf( "Fscache_SetMinSize: Only raising min cache size to %d blocks\n", 
				minBlocks);
    }
    fs_Stats.blockCache.minCacheBlocks = minBlocks;
    if (fs_Stats.blockCache.minCacheBlocks <= 
				    fs_Stats.blockCache.numCacheBlocks) {
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Give enough blocks memory so that the minimum cache size requirement
     * is met.
     */
    while (fs_Stats.blockCache.numCacheBlocks < 
					fs_Stats.blockCache.minCacheBlocks) {
	if (!CreateBlock(FALSE, (Fscache_Block **) NIL)) {
	    printf("Fscache_SetMinSize: lowered min cache size to %d blocks\n",
		       fs_Stats.blockCache.numCacheBlocks);
	    fs_Stats.blockCache.minCacheBlocks = 
				    fs_Stats.blockCache.numCacheBlocks;
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_SetMaxSize --
 *
 * 	Set the maximum size of the block cache.  This entails freeing
 *	enough main memory pages so that the number of cache pages is
 *	less than the maximum allowed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cache blocks have memory removed from behind them and are moved to
 *	the unmapped list. 
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_SetMaxSize(maxBlocks)
    int	maxBlocks;	/* The minimum number of pages in the cache. */
{
    Boolean			giveUp;
    int				pageNum;

    LOCK_MONITOR;

    if (maxBlocks > fs_Stats.blockCache.maxNumBlocks) {
	maxBlocks = fs_Stats.blockCache.maxNumBlocks;
	printf("Fscache_SetMaxSize: Only raising max cache size to %d blocks\n",
		maxBlocks);
    }

    fs_Stats.blockCache.maxCacheBlocks = maxBlocks;
    if (fs_Stats.blockCache.maxCacheBlocks >= 
				    fs_Stats.blockCache.numCacheBlocks) {
	UNLOCK_MONITOR;
	return;
    }
    
    /*
     * Free enough pages to get down to maximum size.
     */
    giveUp = FALSE;
    while (fs_Stats.blockCache.numCacheBlocks > 
				fs_Stats.blockCache.maxCacheBlocks && !giveUp) {
	giveUp = !DestroyBlock(FALSE, &pageNum);
    }


    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_GetPageFromFS --
 *
 * 	Compare LRU time of the caller to time of block in LRU list and
 *	if caller has newer pages unmap a block and return a page.
 *
 * Results:
 *	Physical page number if unmap a block.
 *
 * Side effects:
 *	Blocks may be unmapped.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_GetPageFromFS(timeLastAccessed, pageNumPtr)
    int	timeLastAccessed;
    int	*pageNumPtr;
{
    register	Fscache_Block	*blockPtr;

    LOCK_MONITOR;

    fs_Stats.blockCache.vmRequests++;
    *pageNumPtr = -1;
    if (fs_Stats.blockCache.numCacheBlocks > 
		fs_Stats.blockCache.minCacheBlocks && !List_IsEmpty(lruList)) {
	fs_Stats.blockCache.triedToGiveToVM++;
	blockPtr = (Fscache_Block *) USE_LINKS_TO_BLOCK(List_First(lruList));
	if (blockPtr->timeReferenced < timeLastAccessed) {
	    fs_Stats.blockCache.vmGotPage++;
	    (void) DestroyBlock(TRUE, pageNumPtr);
	}
    }

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CreateBlock --
 *
 * 	Add a new block to the list of free blocks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Block removed from the unmapped list and put onto the free list.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static Boolean
CreateBlock(retBlock, blockPtrPtr)
    Boolean		retBlock;	/* TRUE => return a pointer to one of 
					 * the newly created blocks in 
					 * *blockPtrPtr. */
    Fscache_Block	**blockPtrPtr;	/* Where to return pointer to block.
					 * NIL if caller isn't interested. */
{
    register	Fscache_Block	*blockPtr;
    int				newCachePages;

    if (List_IsEmpty(unmappedList)) {
	printf( "CreateBlock: No unmapped blocks\n");
	return(FALSE);
    }
    blockPtr = USE_LINKS_TO_BLOCK(List_First(unmappedList));
    /*
     * Put memory behind the first available unmapped cache block.
     */
    newCachePages = Vm_MapBlock(blockPtr->blockAddr);
    if (newCachePages == 0) {
	return(FALSE);
    }
    numAvailBlocks += newCachePages * blocksPerPage;
    fs_Stats.blockCache.numCacheBlocks += newCachePages * blocksPerPage;
    /*
     * If we are told to return a block then take it off of the list of
     * unmapped blocks and let the caller put it onto the appropriate list.
     * Otherwise put it onto the free list.
     */
    if (retBlock) {
	List_Remove(&blockPtr->useLinks);
	*blockPtrPtr = blockPtr;
    } else {
	fs_Stats.blockCache.numFreeBlocks++;
	blockPtr->flags = FSCACHE_BLOCK_FREE;
	List_Move(&blockPtr->useLinks, LIST_ATREAR(totFreeList));
    }
    if (PAGE_IS_8K) {
	/*
	 * Put the other block in the page onto the appropriate free list.
	 */
	blockPtr = GET_OTHER_BLOCK(blockPtr);
	blockPtr->flags = FSCACHE_BLOCK_FREE;
	fs_Stats.blockCache.numFreeBlocks++;
	if (retBlock) {
	    List_Move(&blockPtr->useLinks, LIST_ATREAR(partFreeList));
	} else {
	    List_Move(&blockPtr->useLinks, LIST_ATREAR(totFreeList));
	}
    }
    if (! List_IsEmpty(fscacheFullWaitList)) {
	Fsutil_WaitListNotify(fscacheFullWaitList);
    }
    Sync_Broadcast(&cleanBlockCondition);

    return(TRUE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DestroyBlock --
 *
 * 	Destroy one physical page worth of blocks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cache blocks have memory removed from behind them and are moved to
 *	the unmapped list. 
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static Boolean
DestroyBlock(retOnePage, pageNumPtr)
    Boolean	retOnePage;
    int		*pageNumPtr;
{
    register	Fscache_Block	*blockPtr;
    register	Fscache_Block	*otherBlockPtr;
    int		pages;

    if ((numAvailBlocks-blocksPerPage <  minNumAvailBlocks) ||
        (fs_Stats.blockCache.numCacheBlocks - FSCACHE_MIN_BLOCKS - 
			blocksPerPage < minNumAvailBlocks) ) {
	return FALSE;
    }
    /*
     * First try the list of totally free pages.
     */
    if (!List_IsEmpty(totFreeList)) {
	blockPtr = USE_LINKS_TO_BLOCK(List_First(totFreeList));
	pages = Vm_UnmapBlock(blockPtr->blockAddr, retOnePage,
				  (unsigned int *)pageNumPtr);
	fs_Stats.blockCache.numCacheBlocks -= pages * blocksPerPage;
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	List_Move(&blockPtr->useLinks, LIST_ATREAR(unmappedList));
	fs_Stats.blockCache.numFreeBlocks--;
	if (PAGE_IS_8K) {
	    /*
	     * Unmap the other block.  The block address can point to either
	     * of the two blocks.
	     */
	    blockPtr = GET_OTHER_BLOCK(blockPtr);
	    blockPtr->flags = FSCACHE_NOT_MAPPED;
	    List_Move(&blockPtr->useLinks, LIST_ATREAR(unmappedList));
	    fs_Stats.blockCache.numFreeBlocks--;
	}
	numAvailBlocks -= pages * blocksPerPage;
	if (numAvailBlocks < 0) {
	    panic("DestroyBlock numAvailBlocks < 0.\n");
	}
	return(TRUE);
    }

    /*
     * Now take blocks from the LRU list until we get one that we can use.
     */
    while (TRUE) {
#ifdef SOSP91
	unsigned int	flags;

	if (retOnePage) {
	    flags = FSCACHE_BLOCK_VM;
	} else {
	    flags = FSCACHE_BLOCK_SHRINK;
	}
	blockPtr = FetchBlock(FALSE, FALSE, flags);
#else
	blockPtr = FetchBlock(FALSE, FALSE);
#endif /* SOSP91 */
	if (blockPtr == (Fscache_Block *) NIL) {
	    /*
	     * There are no clean blocks left so give up.
	     */
	    return(FALSE);
	}
	if (PAGE_IS_8K) {
	    /*
	     * We have to deal with the other block.  If it is in use, then
	     * we can't take this page.  Otherwise delete the block from
	     * the cache and put it onto the unmapped list.
#ifdef SOSP91
	     * If we end up not using the block 'cause the other's in
	     * use, this block still counts as being LRU'd since we end up
	     * putting it on the free list and then continuing.
#endif SOSP91
	     */
	    otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
	    if (otherBlockPtr->refCount > 0 ||
		(otherBlockPtr->flags & 
		     (FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN))) {
		PutOnFreeList(blockPtr);
		continue;
	    }
	    /*
	     * The other block is cached but not in use.  Delete it.
	     */
	    if (!(otherBlockPtr->flags & FSCACHE_BLOCK_FREE)) {
		DeleteBlock(otherBlockPtr);
#ifdef SOSP91
		/* Count this block as whatever the other one was. */
		Fscache_AddCleanStats(flags, otherBlockPtr);
#endif SOSP91
	    }
	    otherBlockPtr->flags = FSCACHE_NOT_MAPPED;
	    List_Move(&otherBlockPtr->useLinks, LIST_ATREAR(unmappedList));
	}
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	List_Insert(&blockPtr->useLinks, LIST_ATREAR(unmappedList));
	pages = Vm_UnmapBlock(blockPtr->blockAddr, 
				retOnePage, (unsigned int *)pageNumPtr);
	fs_Stats.blockCache.numCacheBlocks -= pages * blocksPerPage;
	numAvailBlocks -= pages * blocksPerPage;
	if (numAvailBlocks < 0) {
	    panic("DestroyBlock numAvailBlocks < 0.\n");
	}
	return(TRUE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * FetchBlock --
 *
 *	Return a pointer to the oldest available block on the lru list.
 *	If had to sleep because all of memory is dirty then return NIL.
 *	In this cause our caller has to retry various free lists.
 *
 * Results:
 *	Pointer to oldest available block, NIL if had to wait.  
 *
 * Side effects:
 *	Block deleted.
 *
 * ----------------------------------------------------------------------------
 */
#ifdef SOSP91
static INTERNAL Fscache_Block *
FetchBlock(canWait, cantBlock, flags)
    Boolean	canWait;	/* TRUE implies can sleep if all of memory is 
				 * dirty. */
    Boolean	cantBlock;	/* TRUE if we can't block. */
    unsigned	int	flags;
#else
static INTERNAL Fscache_Block *
FetchBlock(canWait, cantBlock)
    Boolean	canWait;	/* TRUE implies can sleep if all of memory is 
				 * dirty. */
    Boolean	cantBlock;	/* TRUE if we can't block. */
#endif /* SOSP91 */
{
    register	Fscache_Block	*blockPtr;
    register	List_Links	*listPtr;

    if (List_IsEmpty(lruList)) {
	printf("FetchBlock: LRU list is empty\n");
	return((Fscache_Block *) NIL);
    }

    if ((numAvailBlocks > minNumAvailBlocks) || cantBlock)  {
	/*
	 * Scan list for unlocked, clean block.
	 */
	LIST_FORALL(lruList, listPtr) {
	    blockPtr = USE_LINKS_TO_BLOCK(listPtr);
	    if (blockPtr->refCount > 0) {
		/*
		 * Block is locked.
		 */
	    } else if (blockPtr->flags & 
			    (FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) {
		/*
		 * Block is dirty or being cleaned.  Mark it so that it will be
		 * freed after it has been cleaned.
		 */
		blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
		if (!(blockPtr->cacheInfoPtr->flags & 
					FSCACHE_FILE_BEING_WRITTEN)) {
#ifdef SOSP91
		    if (flags & FSCACHE_BLOCK_VM) {
			blockPtr->cacheInfoPtr->flags |= FSCACHE_VM;
		    } else if (flags & FSCACHE_BLOCK_SHRINK) {
			blockPtr->cacheInfoPtr->flags |= FSCACHE_SHRINK;
		    } else {
			blockPtr->cacheInfoPtr->flags |= FSCACHE_LRU;
		    }
#endif SOSP91
		    StartFileSync(blockPtr->cacheInfoPtr);
		}
	    } else if (blockPtr->flags & FSCACHE_BLOCK_DELETED) {
		printf( "FetchBlock: deleted block %d of file %d in LRU list\n",
		    blockPtr->blockNum, blockPtr->fileNum);
	    } else  {
		/*
		 * This block is clean and unlocked.  Delete it from the
		 * hash table and use it.
		 */
#ifdef SOSP91
		Fscache_AddCleanStats(flags, blockPtr);
#endif SOSP91
		fs_Stats.blockCache.lru++;
		List_Remove(&blockPtr->useLinks);
		DeleteBlock(blockPtr);
		return(blockPtr);
	    }
	}
    } else if (numAvailBlocks <= minNumAvailBlocks) {
	 Fscache_Backend	*backendPtr;
         LIST_FORALL(backendList, (List_Links *) backendPtr) {
	    if (!List_IsEmpty(&backendPtr->dirtyListHdr)) { 
#ifdef SOSP91
	    /*
	     * XXX Darn - I can't put FSCACHE_SPACE flag into the various
	     * file's cacheInfo structs here, but I'd like to.  This means
	     * that "unknown" may imply "space" in the results.
	     */
#endif SOSP91
		StartBackendWriteback(backendPtr);
	    }
         }
  }
    /*
     * We have looked at every block but we couldn't use any.
     * If possible wait until the block cleaner cleans a block for us.
     */
    if (canWait && !cantBlock) {
	(void) Sync_Wait(&cleanBlockCondition, FALSE);
    }
    return((Fscache_Block *) NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * StartBackendWriteback --
 *
 *	Start a backend writeback process for the specified backend.
 *	This routine keeps track the number of backend writebacks 
 *	active.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
StartBackendWriteback(backendPtr)
    Fscache_Backend *backendPtr;
{
    Boolean	started;

    started = backendPtr->ioProcs.startWriteBack(backendPtr);
    if (started) {
	numBackendsActive++;
    }
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 *  Fscache_GetDirtyBlock --
 *
 *     	Take the blocks off of a file's dirty list and return a pointer
 *	to it.  The synchronization between closing a file and writing
 *	back its blocks is done here.  If there are no more dirty blocks
 *	and the file is being closed we poke the closing process.  If
 *	there are still dirty blocks and someone is closing the file
 *	we put the file back onto the file dirty list and don't return a block.
 *
 * Results:
 *	The block returned. NIL if no blocks are ready.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Fscache_Block *
Fscache_GetDirtyBlock(cacheInfoPtr, blockMatchProc, clientData,
			lastDirtyBlockPtr)
    Fscache_FileInfo	*cacheInfoPtr;
    Boolean		(*blockMatchProc)();
    ClientData		clientData;
    int			*lastDirtyBlockPtr;
{
    register	List_Links	*dirtyPtr;
    register	Fscache_Block	*blockPtr;

    LOCK_MONITOR;

    *lastDirtyBlockPtr = 0;

    if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
	/*
	 * We can't do any write-backs until the file is closed.
	 * Return zero blocks in hope that the backend will return
	 * the file to the cache.
	 */
	UNLOCK_MONITOR;
	return (Fscache_Block *) NIL;
    }
    if (cacheInfoPtr->flags & (FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE | 
			      FSCACHE_GENERIC_ERROR|FSCACHE_FILE_GONE)) {
	UNLOCK_MONITOR;
	return (Fscache_Block *) NIL;
    }

    LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
	blockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    UNLOCK_MONITOR;
	    panic( "GetDirtyBlock, bad block\n");
	    LOCK_MONITOR;
	    continue;
	}
	if (blockPtr->refCount > 0) {
	    /*
	     * Being actively used.  Wait until it is not in use anymore in
	     * case the user is writing it for example.
	     */
	    blockPtr->flags |= FSCACHE_BLOCK_CLEANER_WAITING;
	    continue;
	}
	if (!blockMatchProc(blockPtr, clientData)) {
		continue;
	}
	/*
	 * Mark the block as being written out and clear the dirty flag in case
	 * someone modifies it while we are writing it out.
	 */
	List_Remove(dirtyPtr);
	blockPtr->flags &= ~(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_CLEANED);
	blockPtr->flags |= FSCACHE_BLOCK_BEING_WRITTEN;
	blockPtr->refCount++;
	if (blockPtr->refCount == 1) { 
	    VmMach_LockCachePage(blockPtr->blockAddr);
	}
	/*
	 * Gather statistics.
	 */
	fs_Stats.blockCache.blocksWrittenThru++;
	switch (blockPtr->flags &
		(FSCACHE_DATA_BLOCK | FSCACHE_IND_BLOCK |
		 FSCACHE_DESC_BLOCK | FSCACHE_DIR_BLOCK)) {
	    case FSCACHE_DATA_BLOCK:
		fs_Stats.blockCache.dataBlocksWrittenThru++;
		break;
	    case FSCACHE_IND_BLOCK:
		fs_Stats.blockCache.indBlocksWrittenThru++;
		break;
	    case FSCACHE_DESC_BLOCK:
		fs_Stats.blockCache.descBlocksWrittenThru++;
		break;
	    case FSCACHE_DIR_BLOCK:
		fs_Stats.blockCache.dirBlocksWrittenThru++;
		break;
	    default:
		printf( "Ofs_CleanBlocks: Unknown block type\n");
	}
	if (blockPtr->blockSize < 0) {
	    panic( "Fscache_GetDirtyBlock: uninitialized block size\n");
	}
	*lastDirtyBlockPtr = List_IsEmpty(&cacheInfoPtr->dirtyList);
	UNLOCK_MONITOR;
	return blockPtr;
    }
    *lastDirtyBlockPtr = List_IsEmpty(&cacheInfoPtr->dirtyList);
    UNLOCK_MONITOR;
    return (Fscache_Block *) NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_ReturnDirtyBlock --
 *
 *     	This routine will process the newly cleaned blocks.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The block may be moved on the allocate list and also possibly freed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_ReturnDirtyBlock(blockPtr, status)
    register  Fscache_Block	*blockPtr; /*  blocks to  return. */
    ReturnStatus		status;
{
    register	Fscache_FileInfo	*cacheInfoPtr;

    LOCK_MONITOR;

    cacheInfoPtr = blockPtr->cacheInfoPtr;

    blockPtr->flags &= ~FSCACHE_BLOCK_BEING_WRITTEN;
    blockPtr->refCount--;
    if (blockPtr->refCount == 0) {
	VmMach_UnlockCachePage(blockPtr->blockAddr);
    }
    Sync_Broadcast(&blockPtr->ioDone);
    if (status == GEN_EINTR) {
	blockPtr->flags |= FSCACHE_BLOCK_DIRTY;
    } else if (status != SUCCESS) {
	/*
	 * This block could not be written out.
	 */
	Boolean		printErrorMsg;

	printErrorMsg = FALSE;
	switch (status) {
	    case RPC_TIMEOUT:
	    case FS_STALE_HANDLE:
	    case RPC_SERVICE_DISABLED:	
		if (!(cacheInfoPtr->flags & FSCACHE_SERVER_DOWN)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FSCACHE_SERVER_DOWN;
		}
		/*
		 * Mark the handle as needing recovery.  Then invoke a
		 * background process to attempt the recovery now.
		 */
		(void) Fsutil_WantRecovery(cacheInfoPtr->hdrPtr);
		if (status == FS_STALE_HANDLE) {
		    Proc_CallFunc(Fsutil_AttemptRecovery,
			      (ClientData)cacheInfoPtr->hdrPtr, 0);
		}
		break;
	    case FS_NO_DISK_SPACE:
		if (!(cacheInfoPtr->flags & FSCACHE_NO_DISK_SPACE)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FSCACHE_NO_DISK_SPACE;
		}
		break;
	    case FS_DOMAIN_UNAVAILABLE:
		if (!(cacheInfoPtr->flags & FSCACHE_DOMAIN_DOWN)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FSCACHE_DOMAIN_DOWN;
		}
		break;
	    case DEV_RETRY_ERROR:
	    case DEV_HARD_ERROR:
		/*
		 * Schedule a background process to allocate new space for
		 * this block.  Inc the ref count so the block won't go away
		 * and won't be written again, and mark it as being written so
		 * noone will attempt to change where the block is on disk.
		 */
		blockPtr->refCount++;
		if (blockPtr->refCount == 1) { 
		    VmMach_LockCachePage(blockPtr->blockAddr);
		}
		blockPtr->flags |= FSCACHE_BLOCK_BEING_WRITTEN;
		Proc_CallFunc(
		    cacheInfoPtr->backendPtr->ioProcs.reallocBlock, 
				    (ClientData)blockPtr, 0);
		printErrorMsg = TRUE;
		printf("File blk %d phys blk %d: ",
			    blockPtr->blockNum, blockPtr->diskBlock);
		cacheInfoPtr->flags |= FSCACHE_GENERIC_ERROR;
		break;
	    default: 
		printErrorMsg = TRUE;
		cacheInfoPtr->flags |= FSCACHE_GENERIC_ERROR;
		break;
	}
	if (printErrorMsg) {
	    Fsutil_FileError(cacheInfoPtr->hdrPtr, 
		    "Write-back failed", status);
	}
	cacheInfoPtr->lastTimeTried = Fsutil_TimeInSeconds();
	PutBlockOnDirtyList(blockPtr, TRUE);
	UNLOCK_MONITOR;
	return;
    }
    /*
     * Successfully wrote the block.
     */
    cacheInfoPtr->flags &= 
			~(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE | 
			  FSCACHE_GENERIC_ERROR);
    if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) { 
	PutBlockOnDirtyList(blockPtr, TRUE);
    } else { 
	if (blockPtr->refCount == 0) {
	    numAvailBlocks++; 
	}
	/* 
	 * Now see if we are supposed to take any special action with this
	 * block once we are done.
	 */
	if (blockPtr->flags & FSCACHE_BLOCK_DELETED) {
	    cacheInfoPtr->blocksInCache--;
	    List_Remove(&blockPtr->fileLinks);
	    PutOnFreeList(blockPtr);
	} else if (blockPtr->flags & FSCACHE_MOVE_TO_FRONT) {
	    List_Move(&blockPtr->useLinks, LIST_ATFRONT(lruList));
	    blockPtr->flags &= ~FSCACHE_MOVE_TO_FRONT;
	}

	cacheInfoPtr->numDirtyBlocks--;
	/*
	 * Wakeup the block allocator which may be waiting for us to clean
	 * a block
	 */
	if (! List_IsEmpty(fscacheFullWaitList)) {
	    Fsutil_WaitListNotify(fscacheFullWaitList);
	}
	Sync_Broadcast(&cleanBlockCondition);
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 *  Fscache_GetDirtyFile --
 *
 *     	Take a dirty file off of the dirty file list for the
 *	specified backend. Return a pointer to the cache state for 
 *	the file return. This routine is used by the backend to
 *	walk through the dirty list.
 *
 * Results:
 *	A pointer to the cache state for the first file on the dirty list.
 *	If there are no files then a NIL pointer is returned.  
 *
 * Side effects:
 *      An element is removed from the dirty file list.  The fact that
 *	the dirty list links are at the beginning of the cacheInfo struct
 *	is well known and relied on to map from the dirty list to cacheInfoPtr.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Fscache_FileInfo *
Fscache_GetDirtyFile(backendPtr, fsyncOnly, fileMatchProc, clientData)
    Fscache_Backend	*backendPtr;	/* Cache backend to take dirty files
					 * from. */
    Boolean		fsyncOnly;	/* TRUE if we should return only 
					 * fsynced files. */
    Boolean		(*fileMatchProc)();	/* File match procedure. */
    ClientData		clientData;	/* ClientData for match procedure. */
{
    register Fscache_FileInfo *cacheInfoPtr;

    LOCK_MONITOR;

    /*
     * If the dirty list is empty return NIL.
     */
    if (List_IsEmpty(&backendPtr->dirtyListHdr)) {
	UNLOCK_MONITOR;
	return (Fscache_FileInfo *) NIL;
    }

    /*
     * Otherwise, search the the dirtyList picking off the file to return.
     */
    LIST_FORALL(&(backendPtr->dirtyListHdr), (List_Links *)cacheInfoPtr) {
	 if (cacheInfoPtr->flags & 
			(FSCACHE_CLOSE_IN_PROGRESS|FSCACHE_SERVER_DOWN)) {
	    /*
	     * Close in progress on the file the block lives in so we aren't
	     * allowed to write any more blocks.
	     */
	    continue;
	} else if (cacheInfoPtr->flags & FSCACHE_FILE_GONE) {
	    /*
	     * The file is being deleted.
	     */
	    printf("FscacheGetDirtyFile skipping deleted file <%d,%d> \"%s\"\n",
		cacheInfoPtr->hdrPtr->fileID.major,
		cacheInfoPtr->hdrPtr->fileID.minor,
		Fsutil_HandleName(cacheInfoPtr->hdrPtr));
	    continue;
	} else if (cacheInfoPtr->flags & 
		       (FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
		        FSCACHE_GENERIC_ERROR)) {
	    if (Fsutil_TimeInSeconds() - cacheInfoPtr->lastTimeTried <
			FSUTIL_WRITE_RETRY_INTERVAL) {
		continue;
	    }
	    cacheInfoPtr->flags &= 
			    ~(FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
			      FSCACHE_GENERIC_ERROR);
	} 

	if (!fileMatchProc(cacheInfoPtr, clientData)) {
		continue;
	}
	if ((numAvailBlocks > minNumAvailBlocks) && fsyncOnly && 
		!( (cacheInfoPtr->flags & FSCACHE_FILE_FSYNC) ||
		    (cacheInfoPtr->oldestDirtyBlockTime < filewriteBackTime))) {
	    break;
	}
	/*
	 * Check to make sure that if dirty blocks exist then at least one 
	 * of the blocks is available to write.
	 */
	if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) { 
	    register	List_Links	*dirtyPtr;
	    register	Fscache_Block	*blockPtr;
	    Boolean	found = FALSE;
	    LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
		blockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
		if (blockPtr->refCount == 0) {
		    found = TRUE;
		    break;
		}
	    }
	    if (!found) {
		continue;
	    }
        }

	cacheInfoPtr->flags |= FSCACHE_FILE_BEING_WRITTEN;
	cacheInfoPtr->flags &= ~FSCACHE_FILE_ON_DIRTY_LIST;
#ifdef SOSP91
	if (fsyncOnly) {
	    cacheInfoPtr->flags |= FSCACHE_TIME;
	}
	if (numAvailBlocks <= minNumAvailBlocks) {
	    cacheInfoPtr->flags |= FSCACHE_SPACE;
	}
#endif SOSP91
	List_Remove((List_Links *)cacheInfoPtr);
	UNLOCK_MONITOR;
	return cacheInfoPtr;
    }
    UNLOCK_MONITOR;
    return (Fscache_FileInfo *) NIL;
}

/*
 * ----------------------------------------------------------------------------
 *
 *  Fscache_ReturnDirtyFile --
 *
 *     	Return a file checked-out using get dirty file. 
 *
 * Results:
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Fscache_ReturnDirtyFile(cacheInfoPtr, onFront)
    Fscache_FileInfo	*cacheInfoPtr;	/* File to return. */
    Boolean 		onFront;	/* Put it on the front the of list. */
{

    LOCK_MONITOR;

    /*
     * Move the from file being written state (and list) to the
     * dirty list or no list at all.
     */
    cacheInfoPtr->flags &= ~FSCACHE_FILE_BEING_WRITTEN;
    if (cacheInfoPtr->numDirtyBlocks == 0) { 
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
	if (!(cacheInfoPtr->flags & FSCACHE_FILE_DESC_DIRTY)) {
	    cacheInfoPtr->flags &= ~(FSCACHE_FILE_FSYNC |
	                             FSCACHE_FILE_BEING_CLEANED);
	}
    }

    if (((cacheInfoPtr->numDirtyBlocks > 0) || 
         (cacheInfoPtr->flags & FSCACHE_FILE_DESC_DIRTY)) &&
	 !(cacheInfoPtr->flags & FSCACHE_FILE_GONE)) {
	PutFileOnDirtyList(cacheInfoPtr, 
			    cacheInfoPtr->oldestDirtyBlockTime);
    }
    if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
	/*
	 * Wake up anyone waiting for us to finish so that they can 
	 * close their file. 
	 */
	Sync_Broadcast(&closeCondition);
    }

    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FscacheBackendIdle --
 *
 *	Inform the cache that a backend write-back has finished..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
FscacheBackendIdle(backendPtr)
    Fscache_Backend *backendPtr;
{
    LOCK_MONITOR;
    numBackendsActive--;
    if (numBackendsActive == 0) {
	Sync_Broadcast(&writeBackComplete);
    }
    UNLOCK_MONITOR;
    return;
}



/*
 * ----------------------------------------------------------------------------
 *
 * FscacheFinishRealloc --
 *
 *	After reallocating new space for a block, finish things up.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
FscacheFinishRealloc(blockPtr, diskBlock)
    Fscache_Block	*blockPtr;
    int			diskBlock;
{
    LOCK_MONITOR;

    blockPtr->refCount--;
    if (blockPtr->refCount == 0) { 
	VmMach_UnlockCachePage(blockPtr->blockAddr);
    }
    blockPtr->flags &= ~FSCACHE_BLOCK_BEING_WRITTEN;
    Sync_Broadcast(&blockPtr->ioDone);
    if (diskBlock != -1) {
	blockPtr->diskBlock = diskBlock;
	blockPtr->cacheInfoPtr->flags &= 
			    ~(FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
			      FSCACHE_GENERIC_ERROR);
	PutFileOnDirtyList(blockPtr->cacheInfoPtr, 
			blockPtr->cacheInfoPtr->oldestDirtyBlockTime);
	StartBackendWriteback(blockPtr->cacheInfoPtr->backendPtr);
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_PreventWriteBacks --
 *
 *	Mark this file as a close in progress.  This routine will not
 *	return until all dirty block cleaners are done writing out blocks
 *	for this file.  This is called before doing a close on a file
 *	and is needed to synchronize write-backs and closes so the
 *	file server knows when it has all the dirty blocks of a file.
 *
 * Results:
 *	The number of dirty blocks in the cache for this file.
 *	-1 is returned if the file is not cacheable.
 *
 * Side effects:
 *	FSCACHE_CLOSE_IN_PROGRESS flags set in the cacheInfo for this file.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY int
Fscache_PreventWriteBacks(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    int	numDirtyBlocks;

    LOCK_MONITOR;

    cacheInfoPtr->flags |= FSCACHE_CLOSE_IN_PROGRESS;
    while (cacheInfoPtr->flags & FSCACHE_FILE_BEING_WRITTEN) {
	(void)Sync_Wait(&closeCondition, FALSE);
    }
    if (cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) {
	numDirtyBlocks = -1;
    } else {
	numDirtyBlocks = cacheInfoPtr->numDirtyBlocks;
    }

    UNLOCK_MONITOR;

    return(numDirtyBlocks);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_AllowWriteBacks --
 *
 *	The close that was in progress on this file is now done.  We
 *	can continue to write back blocks now.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	FSCACHE_CLOSE_IN_PROGRESS flag cleared from the handle for this file.
 *	Also if the block cleaner is waiting for us then wake it up.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_AllowWriteBacks(cacheInfoPtr)
    register	Fscache_FileInfo *cacheInfoPtr;
{
    LOCK_MONITOR;

    cacheInfoPtr->flags &= ~FSCACHE_CLOSE_IN_PROGRESS;
    if ((cacheInfoPtr->numDirtyBlocks > 0) || 
        (cacheInfoPtr->flags & FSCACHE_FILE_DESC_DIRTY)) { 
	PutFileOnDirtyList(cacheInfoPtr, cacheInfoPtr->oldestDirtyBlockTime);
	if (cacheInfoPtr->flags & FSCACHE_FILE_FSYNC) {
	    StartBackendWriteback(cacheInfoPtr->backendPtr);
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_PutFileOnDirtyList --
 *
 *	Put the specified file on the dirty list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fscache_PutFileOnDirtyList(cacheInfoPtr, flags)
    register	Fscache_FileInfo *cacheInfoPtr;
    int		flags;
{
    LOCK_MONITOR;
    cacheInfoPtr->flags |= flags;

    PutFileOnDirtyList(cacheInfoPtr, Fsutil_TimeInSeconds());

    UNLOCK_MONITOR;
    return (SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_RemoveFileFromDirtyList --
 *
 *	Remove the specified file on the dirty list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fscache_RemoveFileFromDirtyList(cacheInfoPtr)
    register	Fscache_FileInfo *cacheInfoPtr;
{
    int blocksInCache;
    LOCK_MONITOR;
    while (cacheInfoPtr->flags & FSCACHE_FILE_BEING_WRITTEN) {
	Sync_Wait(&cacheInfoPtr->noDirtyBlocks, FALSE);
    }
    if (cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) {
	cacheInfoPtr->flags &= ~(FSCACHE_FILE_ON_DIRTY_LIST|FSCACHE_FILE_FSYNC|
			FSCACHE_FILE_BEING_CLEANED);
	List_Remove((List_Links *)cacheInfoPtr);
    }
    blocksInCache = cacheInfoPtr->blocksInCache;

    UNLOCK_MONITOR;

    if (blocksInCache != 0) {
	panic("Fscache_RemoveFileFromDirtyList blocks in cache\n");
    }
    return SUCCESS;
}



/*
 * ----------------------------------------------------------------------------
 *
 *	Miscellaneous functions.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * FscacheAllBlocksInCache --
 *
 * 	Return true if all of this files blocks are in the cache.  This
 *	is used to optimize out read ahead.
 *
 * Results:
 *	TRUE if all blocks for the file are in the cache.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Boolean
FscacheAllBlocksInCache(cacheInfoPtr)
    register	Fscache_FileInfo *cacheInfoPtr;
{
    Boolean	result;
    int		numBlocks;

    LOCK_MONITOR;

    fs_Stats.blockCache.allInCacheCalls++;
    if (cacheInfoPtr->attr.lastByte == -1) {
	result = TRUE;
    } else {
	if (cacheInfoPtr->attr.firstByte == -1) {
	    cacheInfoPtr->attr.firstByte = 0;
	}
	numBlocks = (cacheInfoPtr->attr.lastByte/FS_BLOCK_SIZE) -
		    (cacheInfoPtr->attr.firstByte/FS_BLOCK_SIZE) + 1;
	result = (numBlocks == cacheInfoPtr->blocksInCache);
	if (result) {
	    fs_Stats.blockCache.allInCacheTrue++;
	}
    }

    UNLOCK_MONITOR;

    return(result);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_ReserveBlocks --
 *
 *   Insure that at least the specified number of cache blocks will be 
 *   available to Fetch_Block() calls the the FSCACHE_CANT_BLOCK flag.
 *
 * Results:
 *	The number of blocks made available.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
int
Fscache_ReserveBlocks(backendPtr, numResBlocks, numNonResBlocks)
    Fscache_Backend	*backendPtr;
    int			numResBlocks;
    int			numNonResBlocks;
{
    int	numBlocks = numResBlocks + numNonResBlocks;

    LOCK_MONITOR;
    if (numBlocks > fs_Stats.blockCache.maxNumBlocks) {
	numBlocks = fs_Stats.blockCache.maxNumBlocks - minNumAvailBlocks;
    }

    while (fs_Stats.blockCache.numCacheBlocks < minNumAvailBlocks + numBlocks) { 
	if (!CreateBlock(FALSE, (Fscache_Block **) NIL)) {
		break;
	} 
    }
    if (minNumAvailBlocks + numResBlocks > 
		fs_Stats.blockCache.numCacheBlocks - numNonResBlocks) {
	numResBlocks = fs_Stats.blockCache.numCacheBlocks - numNonResBlocks -
			minNumAvailBlocks;
	if (numResBlocks< 0) {
	    numResBlocks = 0;
	}
    }
    minNumAvailBlocks += numResBlocks;
    while (minNumAvailBlocks > numAvailBlocks) {
	(void) Sync_Wait(&cleanBlockCondition, FALSE);
    }
    UNLOCK_MONITOR;
    return numResBlocks;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_ReleaseReserveBlocks
 *
 *   Release blocks that were Reserved.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
void
Fscache_ReleaseReserveBlocks(backendPtr, numBlocks)
    Fscache_Backend	*backendPtr;
    int			numBlocks;
{
    LOCK_MONITOR;

    minNumAvailBlocks  -= numBlocks;
    Sync_Broadcast(&cleanBlockCondition);

    UNLOCK_MONITOR;
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FscacheBlockOkToScavenge --
 *
 *	Decide if it is safe to scavenge the file handle.  This is
 *	called from Fscache_OkToScavenge which
 *	has already grabbed the per-file cache lock.
 *
 * Results:
 *	TRUE if there are no blocks (clean or dirty) in the cache for this file.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY int
FscacheBlockOkToScavenge(cacheInfoPtr)
    register Fscache_FileInfo	*cacheInfoPtr;	/* Cache state to check. */
{
    register int numBlocks;
    register int numBlocksCheck = 0;
    register Boolean ok;
    register Fscache_Block *blockPtr;
    List_Links		*linkPtr;

    LOCK_MONITOR;
    numBlocks = cacheInfoPtr->blocksInCache;
    LIST_FORALL(&cacheInfoPtr->blockList, (List_Links *)linkPtr) {
	blockPtr = FILE_LINKS_TO_BLOCK(linkPtr);
	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    UNLOCK_MONITOR;
	    panic( "FsCacheFileBlocks, bad block\n");
	    return(FALSE);
	}
	numBlocksCheck++;
    }
    LIST_FORALL(&cacheInfoPtr->indList, (List_Links *)linkPtr) {
	numBlocksCheck++;
    }
    if (numBlocksCheck != numBlocks) {
	UNLOCK_MONITOR;
	panic( "FsCacheFileBlocks, wrong block count\n");
	return(FALSE);
    }
    ok = (numBlocks == 0) &&
	((cacheInfoPtr->flags & 
		(FSCACHE_FILE_ON_DIRTY_LIST|FSCACHE_FILE_BEING_WRITTEN)) == 0);
    UNLOCK_MONITOR;
    return(ok);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_FileInfoSyncLockCleanup --
 *
 * 	Clean up Sync_Lock tracing for the cache lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the fields of the Fscache_FileInfo struct.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fscache_FileInfoSyncLockCleanup(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    Sync_LockClear(&cacheInfoPtr->lock);
}


/*
 * ----------------------------------------------------------------------------
 * 
 *	List Functions
 *
 * Functions to put objects onto the free and dirty lists.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * PutOnFreeList --
 *
 * 	Put the given block onto one of the two free lists.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Block either added to the partially free list or the totally free list.
 *	The list of processes waiting on the full cache is notified if
 *	is non-empty.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnFreeList(blockPtr)
    register	Fscache_Block	*blockPtr;
{
    register	Fscache_Block	*otherBlockPtr;

    blockPtr->flags = FSCACHE_BLOCK_FREE;
    fs_Stats.blockCache.numFreeBlocks++;
    if (PAGE_IS_8K) {
	/*
	 * If all blocks in the page are free then put this block onto the
	 * totally free list.  Otherwise it goes onto the partially free list.
	 */
	otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
	if (otherBlockPtr->flags & FSCACHE_BLOCK_FREE) {
	    List_Insert(&blockPtr->useLinks, LIST_ATFRONT(totFreeList));
	    List_Move(&otherBlockPtr->useLinks, LIST_ATFRONT(totFreeList));
	} else {
	    List_Insert(&blockPtr->useLinks, LIST_ATFRONT(partFreeList));
	}
    } else {
	List_Insert(&blockPtr->useLinks, LIST_ATFRONT(totFreeList));
    }
    if (! List_IsEmpty(fscacheFullWaitList)) {
	Fsutil_WaitListNotify(fscacheFullWaitList);
    }
    Sync_Broadcast(&cleanBlockCondition);
}	


/*
 * ----------------------------------------------------------------------------
 *
 * PutFileOnDirtyList --
 *
 * 	Put the given file onto it's backend's dirty list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File's backend's dirty list is modified.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutFileOnDirtyList(cacheInfoPtr, oldestDirtyBlockTime)
    register Fscache_FileInfo	*cacheInfoPtr;	/* Cache info for a file */
    int		oldestDirtyBlockTime;
{
    register List_Links	*linkPtr;
    List_Links	*dirtyList, *place;

    dirtyList = &cacheInfoPtr->backendPtr->dirtyListHdr;
    if (!(cacheInfoPtr->flags & 
	(FSCACHE_FILE_ON_DIRTY_LIST|FSCACHE_FILE_BEING_WRITTEN))) {
        List_Insert((List_Links *)cacheInfoPtr, LIST_ATREAR(dirtyList));
	if (cacheInfoPtr->flags & FSCACHE_FILE_FSYNC) {
	    /*
	     * Move down the list until we reach the file or the first non
	     * synced file. 
	     */
	    place = (List_Links *) cacheInfoPtr;
	    LIST_FORALL(dirtyList, linkPtr) {
		if ((linkPtr == (List_Links *) cacheInfoPtr) ||
		    !(((Fscache_FileInfo *) linkPtr)->flags & 
					FSCACHE_FILE_FSYNC)) {
		    place = linkPtr;
		    break;
		}
	    }
	    if (place != (List_Links *) cacheInfoPtr) {
		List_Move((List_Links *)cacheInfoPtr, LIST_BEFORE(place));
	    }
	}
	cacheInfoPtr->oldestDirtyBlockTime = oldestDirtyBlockTime;
	cacheInfoPtr->flags |= FSCACHE_FILE_ON_DIRTY_LIST;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutBlockOnDirtyList --
 *
 * 	Put the given block onto the dirty list for the file, and make
 *	sure that the file is on the list of dirty files.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The block goes onto the dirty list of the file.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutBlockOnDirtyList(blockPtr, onFront)
    register	Fscache_Block	*blockPtr;	/* Block to put on list. */
    Boolean	onFront;			/* Put block on front not
						 * rear of list. */
{
    register Fscache_FileInfo *cacheInfoPtr = blockPtr->cacheInfoPtr;

    blockPtr->flags |= FSCACHE_BLOCK_DIRTY;
    List_Insert(&blockPtr->dirtyLinks, 
		onFront ? LIST_ATFRONT(&cacheInfoPtr->dirtyList) :
			  LIST_ATREAR(&cacheInfoPtr->dirtyList));

    PutFileOnDirtyList(cacheInfoPtr, (int)(blockPtr->timeDirtied));
}




/*
 * ----------------------------------------------------------------------------
 *
 * GetUnlockedBlock --
 *
 *	Retrieve a block from the hash table.  This routine will not return
 *	until the block is unlocked and is not being written.
 *
 * Results:
 *	Pointer to hash table entry for the block.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static Hash_Entry *
GetUnlockedBlock(blockHashKeyPtr, blockNum)
    register	BlockHashKey	*blockHashKeyPtr;
    int				blockNum;
{
    register	Fscache_Block	*blockPtr;
    register	Hash_Entry	*hashEntryPtr;

    /*
     * See if block is in the hash table.
     */
    blockHashKeyPtr->blockNumber = blockNum;
again:
    hashEntryPtr = Hash_LookOnly(blockHashTable, (Address)blockHashKeyPtr);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	return((Hash_Entry *) NIL);
    }

    blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);
    /*
     * Wait until the block is unlocked.  Once wake up start over because
     * the block could have been freed while we were asleep.
     */
    if (blockPtr->refCount > 0 || 
	(blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN)) {
	(void) Sync_Wait(&blockPtr->ioDone, FALSE);
	if (sys_ShuttingDown) {
	    return((Hash_Entry *) NIL);
	}
	goto again;
    }
    return(hashEntryPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DeleteBlock --
 *
 *	Remove the block from the hash table.
 *
 * Results:
 *	None.	
 *
 * Side effects:
 *	Decrement count of blocks in cache for file and block deleted from
 *	hash table.
 *
 * ----------------------------------------------------------------------------
 *
 */
static Fscache_Block *deletedBlockPtr;

INTERNAL static void
DeleteBlock(blockPtr)
    register	Fscache_Block	*blockPtr;
{
    BlockHashKey	blockHashKey;
    register Hash_Entry	*hashEntryPtr;

    SET_BLOCK_HASH_KEY(blockHashKey, blockPtr->cacheInfoPtr,
				     blockPtr->blockNum);
    hashEntryPtr = Hash_LookOnly(blockHashTable, (Address) &blockHashKey);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	UNLOCK_MONITOR;
	deletedBlockPtr = blockPtr;
	panic("DeleteBlock: Block in LRU list is not in the hash table.\n");
	LOCK_MONITOR;
	return;
    }
    Hash_Delete(blockHashTable, hashEntryPtr);
    blockPtr->cacheInfoPtr->blocksInCache--;
    List_Remove(&blockPtr->fileLinks);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_DumpStats --
 *
 *	Print out the cache statistics.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
void
Fscache_DumpStats(dummy)
    ClientData dummy;		/* unused; see dump.c:eventTable */
{
    register Fs_BlockCacheStats *block;

    block = &fs_Stats.blockCache;

    printf("\n");
    printf("READ  %d dirty hits %d clean hits %d zero fill %d\n",
		block->readAccesses,
		block->readHitsOnDirtyBlock,
		block->readHitsOnCleanBlock,
		block->readZeroFills);
    printf("WRITE %d p-hits %d p-misses %d thru %d zero %d/%d app %d over %d\n",
		block->writeAccesses,
		block->partialWriteHits,
		block->partialWriteMisses,
		block->blocksWrittenThru,
		block->writeZeroFills1, block->writeZeroFills2,
		block->appendWrites,
		block->overWrites);
    if (block->fragAccesses != 0) {
	printf("FRAG upgrades %d hits %d zero fills\n",
		    block->fragAccesses,
		    block->fragHits,
		    block->fragZeroFills);
    }
    if (block->fileDescReads != 0) {
	printf("FILE DESC reads %d hits %d writes %d hits %d\n", 
		    block->fileDescReads, block->fileDescReadHits,
		    block->fileDescWrites, block->fileDescWriteHits);
    }
    if (block->indBlockAccesses != 0) {
	printf("INDIRECT BLOCKS Accesses %d hits %d\n", 
		    block->indBlockAccesses, block->indBlockHits);
    }
    printf("VM asked %d, we tried %d, gave up %d\n",
		block->vmRequests, block->triedToGiveToVM, block->vmGotPage);
    printf("BLOCK free %d new %d lru %d part free %d\n",
		block->totFree, block->unmapped,
		block->lru, block->partFree);
    printf("SIZES Blocks min %d num %d max %d, Blocks max %d free %d pitched %d\n",
		block->minCacheBlocks, block->numCacheBlocks,
		block->maxCacheBlocks, block->maxNumBlocks,
		block->numFreeBlocks, block->blocksPitched);

    printf("OBJECTS stream %d (clt %d) file %d dir %d rmtFile %d pipe %d\n",
	    fs_Stats.object.streams, fs_Stats.object.streamClients,
	    fs_Stats.object.files, fs_Stats.object.directory,
	    fs_Stats.object.rmtFiles, fs_Stats.object.pipes);
    printf("OBJECTS dev %d pdevControl %d pdev %d remote %d Total %d\n",
	    fs_Stats.object.devices, fs_Stats.object.controls,
	    fs_Stats.object.pseudoStreams, fs_Stats.object.remote,
	    fs_Stats.object.streams + fs_Stats.object.files +
	    fs_Stats.object.rmtFiles + fs_Stats.object.pipes +
	    fs_Stats.object.devices + fs_Stats.object.controls +
	    fs_Stats.object.directory +
	    2 * fs_Stats.object.pseudoStreams + fs_Stats.object.remote);
    printf("HANDLES max %d exist %d. In %d scans replaced %d of %d (dirs %d)\n",
	    fs_Stats.handle.maxNumber, fs_Stats.handle.exists,
	    fs_Stats.handle.lruScans, fs_Stats.handle.lruHits,
	    fs_Stats.handle.lruChecks, fs_Stats.object.dirFlushed);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_CheckFragmentation --
 *
 *	Scan through the cache determining the number of bytes wasted
 *	compared to a fully variable cache and a cache with 1024 byte blocks.
 *
 * Results:
 *	The number of blocks in the cache, number of bytes wasted.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_CheckFragmentation(numBlocksPtr, totalBytesWastedPtr, fragBytesWastedPtr)
    int	*numBlocksPtr;		/* Return number of blocks in the cache. */
    int	*totalBytesWastedPtr;	/* Return the total number of bytes wasted in
				 * the cache. */
    int	*fragBytesWastedPtr;	/* Return the number of bytes wasted when cache
				 * is caches 1024 byte fragments. */
{
    register Fscache_Block 	*blockPtr;
    register List_Links	  	*listPtr, *lPtr;
    register int		numBlocks = 0;
    register int		totalBytesWasted = 0;
    register int		fragBytesWasted = 0;
    register int		bytesInBlock;
    int				numFrags;

    LOCK_MONITOR;

    listPtr = lruList;
    LIST_FORALL(listPtr, lPtr) {
	blockPtr = USE_LINKS_TO_BLOCK(lPtr);
	if ((blockPtr->refCount > 0) || (blockPtr->blockSize < 0)) {
	    /* 
	     * Skip locked blocks because they might be in the process of
	     * being modified.
	     */
	    continue;
	}
	numBlocks++;
	bytesInBlock = blockPtr->blockSize;
	if (bytesInBlock < FS_BLOCK_SIZE) {
	    totalBytesWasted += FS_BLOCK_SIZE - bytesInBlock;
	    if (blockPtr->blockNum < FSDM_NUM_DIRECT_BLOCKS) {
		numFrags = (bytesInBlock - 1) / FS_FRAGMENT_SIZE + 1; 
		fragBytesWasted += FS_BLOCK_SIZE - numFrags * FS_FRAGMENT_SIZE;
	    }
	}
    }

    *numBlocksPtr = numBlocks;
    *totalBytesWastedPtr = totalBytesWasted;
    *fragBytesWastedPtr = fragBytesWasted;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_CountBlocks --
 *
 *	Count the number of clean and dirty blocks under the specified 
 *	domain.
 *
 * Results:
 *	
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_CountBlocks(serverID, majorNumber, numBlocksPtr, numDirtyBlocksPtr)
    int		serverID; /* ServerID of file. */
    int		majorNumber;  /* Major number of domain */
    int		*numBlocksPtr; /* OUT: Number of blocks in the cache. */
    int		*numDirtyBlocksPtr;  /* OUT: Number of dirty blocks in cache.*/

{
    register	Fscache_Block	*blockPtr;
    register	List_Links	*listPtr;

    LOCK_MONITOR;

    *numBlocksPtr = *numDirtyBlocksPtr = 0;
    LIST_FORALL(lruList, listPtr) {
	blockPtr = USE_LINKS_TO_BLOCK(listPtr);
	if ((blockPtr->cacheInfoPtr->hdrPtr->fileID.major != majorNumber) ||
	    (blockPtr->cacheInfoPtr->hdrPtr->fileID.serverID != serverID)) {
	    continue;
	}
	(*numBlocksPtr)++;
	if (blockPtr->flags & 
			(FSCACHE_BLOCK_DIRTY|FSCACHE_BLOCK_BEING_WRITTEN)) {
	    (*numDirtyBlocksPtr)++;
	} 
    }

    UNLOCK_MONITOR;
}

#ifdef SOSP91

Fscache_ExtraStats	fscache_ExtraStats;


/*
 *----------------------------------------------------------------------
 *
 * Fscache_AddBlockToStats --
 *
 *	Count another block as being written out from the cache.
 *	Record the reason why from its flags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stats updated.
 *
 *----------------------------------------------------------------------
 */
void
Fscache_AddBlockToStats(cacheInfoPtr, blockPtr)
    Fscache_FileInfo	*cacheInfoPtr;
    Fscache_Block	*blockPtr;
{
    int		now;
    int		dirtyLifeTime;

    now = Fsutil_TimeInSeconds();
    dirtyLifeTime = now - blockPtr->timeDirtied;

    if ((cacheInfoPtr->flags & FSCACHE_REASON_FLAGS) == 0) {
	fscache_ExtraStats.unknown++;
	fscache_ExtraStats.unDLife += dirtyLifeTime;
	return;
    }
    if (cacheInfoPtr->flags & FSCACHE_CONSIST_WB) {
	fscache_ExtraStats.consistWB++;
	fscache_ExtraStats.cwbDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_CONSIST_WBINV) {
	fscache_ExtraStats.consistWBInv++;
	fscache_ExtraStats.cwbiDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_SYNC) {
	fscache_ExtraStats.sync++;
	fscache_ExtraStats.syncDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_VM) {
	fscache_ExtraStats.vm++;
	fscache_ExtraStats.vmDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_SHRINK) {
	fscache_ExtraStats.shrink++;
	fscache_ExtraStats.shrinkDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_REOPEN) {
	fscache_ExtraStats.reopen++;
	fscache_ExtraStats.reDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_DETACH) {
	fscache_ExtraStats.detach++;
	fscache_ExtraStats.detDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_TIME) {
	fscache_ExtraStats.time++;
	fscache_ExtraStats.timeDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_SPACE) {
	fscache_ExtraStats.space++;
	fscache_ExtraStats.spaceDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_DESC) {
	fscache_ExtraStats.desc++;
	fscache_ExtraStats.descDLife += dirtyLifeTime;
    } else if (cacheInfoPtr->flags & FSCACHE_LRU) {
	fscache_ExtraStats.lru++;
	fscache_ExtraStats.lruDLife += dirtyLifeTime;
    }
    return;

}



/*
 *----------------------------------------------------------------------
 *
 * Fscache_AddCleanStats --
 *
 *	Add extra sosp stats about clean blocks pulled from the lru list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stats updated.
 *
 *----------------------------------------------------------------------
 */
void
Fscache_AddCleanStats(flags, blockPtr)
    unsigned int	flags;
    Fscache_Block	*blockPtr;
{
    int		now;
    int		lifeTime = 0;
    Boolean	unrefed = FALSE;

    now = Fsutil_TimeInSeconds();
    if (blockPtr->timeReferenced <= 0) {
	unrefed = TRUE;
    } else {
	lifeTime = now - blockPtr->timeReferenced;
    }
    if (lifeTime < 0) {
	unrefed = TRUE;
    }

    if (flags & FSCACHE_BLOCK_VM) {
	if (unrefed) {
	    fscache_ExtraStats.unRcleanVm++;
	} else {
	    fscache_ExtraStats.cleanVm++;
	    fscache_ExtraStats.cVmLife += lifeTime;
	}
    } else if (flags & FSCACHE_BLOCK_SHRINK) {
	if (unrefed) {
	    fscache_ExtraStats.unRcleanShrink++;
	} else {
	    fscache_ExtraStats.cleanShrink++;
	    fscache_ExtraStats.cShrinkLife += lifeTime;
	}
    } else {
	if (unrefed) {
	    fscache_ExtraStats.unRcleanLru++;
	} else {
	    fscache_ExtraStats.cleanLru++;
	    fscache_ExtraStats.cLruLife += lifeTime;
	}
    }
    return;
}
#endif SOSP91
