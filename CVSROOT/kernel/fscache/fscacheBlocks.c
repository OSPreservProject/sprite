/* 
 * fsBlockCache.c --
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

#include	"sprite.h"
#include	"fs.h"
#include	"fsInt.h"
#include	"fsBlockCache.h"
#include	"fsDebug.h"
#include	"fsStat.h"
#include	"fsOpTable.h"
#include	"fsDisk.h"
#include	"fsFile.h"
#include	"fsTrace.h"
#include	"hash.h"
#include	"vm.h"
#include	"proc.h"
#include	"sys.h"
#include	"rpc.h"

/*
 * There are numerous points of synchronization in this module.  The CacheInfo
 * struct in the handle, the block info struct and several global conditions
 * and variables are used.
 *
 *     1) Synchronization between a block being fetched and a block having
 *	  good data in it.  The FS_IO_IN_PROGRESS flag is used for this.
 *	  If this flag is set in the call to FsCacheFetchBlock then the fetch
 *	  will block until the block becomes unreferenced.  Thus if the block
 *	  is already being used the user won't get their data stomped on.
 *        Whenever FsCacheFetchBlock returns a block it sets the 
 *	  FS_IO_IN_PROGRESS flag in the block.  This flag will be cleared
 *	  whenever the block is released with FsCacheUnlockBlock or the
 *	  function FsCacheIODone is called.  While this flag is set in the block
 *	  all future fetches will block until the flag is cleared.
 *
 *     2) Synchronization for changing where a block lives on disk.  When a
 *	  block cleaner is writing out a block and FsCacheUnlockBlock is
 *	  called with a new location for the block, it blocks until the
 *	  block cleaner finishes.  The flag FS_BLOCK_BEING_WRITTEN in the
 *	  cache block struct indicates this.
 *
 *     3) Waiting for blocks from a file to be written back.  This is done by
 *	  waiting for the dirty list for the file to go empty.
 *
 *     4) Waiting for blocks from the whole cache to be written back.  This
 *	  is done just like (3) except that a global count of the number
 *	  of blocks being written back is kept and the FS_WRITE_BACK_WAIT flag
 *	  is set in the block instead.
 *
 *     5) Synchronizing informing the server when there are no longer any
 *	  dirty blocks in the cache for a file.  When the last dirty block
 *	  for a file is written the write is tagged saying that it is the
 *	  last dirty block.  However the server is also told when a file is
 *	  closed if no dirty blocks are left.  Since all writes out of the
 *	  cache are unsynchronized there is a race between the close and the
 *	  delayed write back.  This is solved by using the following 
 *	  synchronization.  When a file is closed the function 
 *	  FsPreventWriteBacks is called.  This function will not return until
 *	  there are no block cleaners active on the file.  When it returns it 
 *	  sets the FS_CLOSE_IN_PROGRESS flag in the cacheInfo struct in the file *	  handle and it returns the number of dirty blocks.  All subsequent
 *	  block writes are blocked until the function FsAllowWriteBacks is
 *	  called.  Thus the number of dirty blocks in the cache for the
 *	  file is accurate on close because no dirty blocks can be written
 *	  out while the file is being closed.  Likewise when a block cleaner
 *	  writes out the last dirty block for a file and it tells the server
 *	  on the write that its the last dirty block the server knows that
 *	  it can believe the block cleaner if the file is closed.  This
 *	  is because if its closed then it must have been closed when the
 *	  block cleaner did the write (all closes are prohibited during the
 *	  write) and thus there is no way that more dirty blocks can be
 *	  put into the cache.  If its open then the server ignores what the
 *	  block cleaner says because it will get told again when the file is
 *	  closed.
 */

/*
 * Monitor lock.
 */
static Sync_Lock	cacheLock = {0, 0};
#define	LOCKPTR	&cacheLock

/*
 * Condition variables.
 */
static	Sync_Condition	cleanBlockCondition;	/* Condition that block 
						 * allocator waits on when all 
						 * blocks are dirty. */
static	Sync_Condition	writeBackComplete; 	/* Condition to wait on when 
						 * are waiting for the write 
						 * back of blocks in the cache 
						 * to complete. */
static	Sync_Condition	closeCondition;		/* Condition to wait on when
						 * are waiting for the block
						 * cleaner to finish to write
						 * out blocks for this file. */

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
 * Pointer to list of files that have dirty blocks being written to disk.
 */
static	List_Links	dirtyListHdr;
static	List_Links	*dirtyList = &dirtyListHdr;

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
    FsCacheFileInfo *cacheInfoPtr;
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
static	int	numWriteBackBlocks = 0;	/* The number of blocks that are being 
					 * forced back to disk by 
					 * Fs_CacheWriteBack. */
static	int	numBlockCleaners;	/* Number of block cleaner processes
					 * currently in action. */
int		fsMaxBlockCleaners = 3;	/* The maximum number of block cleaners
					 * that there can be. */
static	int	blocksPerPage;		/* Number of blocks in a page. */


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
 * Debugging stuff.
 */
#ifndef CLEAN
/*
 * This macro has to be called with a double set of parenthesis.
 * This lets you pass a variable number of arguments through to Sys_Printf:
 *	DEBUG_PRINT( ("foo %d\n", 17) );
 */
#define DEBUG_PRINT( format ) \
    if (cacheDebug) {\
	Sys_Printf format ; \
    }
#else
#define	DEBUG_PRINT(format)
#endif not CLEAN

static	Boolean	traceDirtyBlocks = FALSE;
static	Boolean	cacheDebug = FALSE;

/*
 * Internal functions.
 */
void		PutOnFreeList();
void		PutFileOnDirtyList();
void		PutBlockOnDirtyList();
Boolean		CreateBlock();
Boolean		DestroyBlock();
FsCacheBlock	*FetchBlock();
void		CacheFileInvalidate();
void		CacheWriteBack();
void		StartBlockCleaner();
void		GetDirtyFile();
void		GetDirtyBlock();
void		GetDirtyBlockInt();
void		ProcessCleanBlock();
void		ReallocBlock();
void		FinishRealloc();
Hash_Entry	*GetUnlockedBlock();
void		DeleteBlock();


/*
 * ----------------------------------------------------------------------------
 *
 * FsBlockCacheInit --
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
FsBlockCacheInit(blockHashSize)
    int	blockHashSize;	/* The number of hash table entries to put in the
			   block hash table for starters. */
{
    register	Address		blockAddr;
    Address			listStart;
    register	FsCacheBlock	*blockPtr;
    register	int		i;

    fsStats.blockCache.minCacheBlocks = 64;
    fsStats.blockCache.maxCacheBlocks = 64;

    Vm_FsCacheSize(&blockCacheStart, &blockCacheEnd);
    pageSize = Vm_GetPageSize();
    blocksPerPage = pageSize / FS_BLOCK_SIZE;
    fsStats.blockCache.maxNumBlocks = 
			(blockCacheEnd - blockCacheStart + 1) / FS_BLOCK_SIZE;

    /*
     * Allocate space for the cache block list.
     */
    listStart = 
	Vm_RawAlloc(fsStats.blockCache.maxNumBlocks * sizeof(FsCacheBlock));
    blockPtr = (FsCacheBlock *) listStart;

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
    List_Init(dirtyList);
    List_Init(unmappedList);

    for (i = 0,blockAddr = blockCacheStart,blockPtr = (FsCacheBlock *)listStart;
	 i < fsStats.blockCache.maxNumBlocks; 
	 i++, blockPtr++, blockAddr += FS_BLOCK_SIZE) {
	blockPtr->flags = FS_NOT_MAPPED;
	blockPtr->blockAddr = blockAddr;
	blockPtr->refCount = 0;
	List_Insert((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
    }

    /*
     * Give enough blocks memory so that the minimum cache size requirement
     * is met.
     */
    fsStats.blockCache.numCacheBlocks = 0;
    while (fsStats.blockCache.numCacheBlocks < 
					fsStats.blockCache.minCacheBlocks) {
	if (!CreateBlock(FALSE, (FsCacheBlock **) NIL)) {
	    Sys_Printf("FsBlockCacheInit: Couldn't create block\n");
	    fsStats.blockCache.minCacheBlocks = 
					fsStats.blockCache.numCacheBlocks;
	}
    }
    Sys_Printf("FS Cache has %d %d-Kbyte blocks (%d max)\n",
	    fsStats.blockCache.minCacheBlocks, FS_BLOCK_SIZE / 1024,
	    fsStats.blockCache.maxNumBlocks);

}

/*
 * ----------------------------------------------------------------------------
 *
 * FsSetMinSize --
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
FsSetMinSize(minBlocks)
    int	minBlocks;	/* The minimum number of blocks in the cache. */
{
    LOCK_MONITOR;

    DEBUG_PRINT( ("Setting minimum size to %d with current size of %d\n",
		       minBlocks, fsStats.blockCache.minCacheBlocks) );

    if (minBlocks > fsStats.blockCache.maxNumBlocks) {
	minBlocks = fsStats.blockCache.maxNumBlocks;
	Sys_Printf( "FsSetMinSize: Only raising min cache size to %d blocks\n", 
				minBlocks);
    }
    fsStats.blockCache.minCacheBlocks = minBlocks;
    if (fsStats.blockCache.minCacheBlocks <= 
				    fsStats.blockCache.numCacheBlocks) {
	UNLOCK_MONITOR;
	return;
    }
    
    /*
     * Give enough blocks memory so that the minimum cache size requirement
     * is met.
     */
    while (fsStats.blockCache.numCacheBlocks < 
					fsStats.blockCache.minCacheBlocks) {
	if (!CreateBlock(FALSE, (FsCacheBlock **) NIL)) {
	    Sys_Printf("FsSetMinSize: lowered min cache size to %d blocks\n",
		       fsStats.blockCache.numCacheBlocks);
	    fsStats.blockCache.minCacheBlocks = 
				    fsStats.blockCache.numCacheBlocks;
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsSetMaxSize --
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
FsSetMaxSize(maxBlocks)
    int	maxBlocks;	/* The minimum number of pages in the cache. */
{
    Boolean			giveUp;
    int				pageNum;

    LOCK_MONITOR;

    if (maxBlocks > fsStats.blockCache.maxNumBlocks) {
	maxBlocks = fsStats.blockCache.maxNumBlocks;
	Sys_Printf("FsSetMaxSize: Only raising max cache size to %d blocks\n",
		maxBlocks);
    }

    fsStats.blockCache.maxCacheBlocks = maxBlocks;
    if (fsStats.blockCache.maxCacheBlocks >= 
				    fsStats.blockCache.numCacheBlocks) {
	UNLOCK_MONITOR;
	return;
    }
    
    /*
     * Free enough pages to get down to maximum size.
     */
    giveUp = FALSE;
    while (fsStats.blockCache.numCacheBlocks > 
				fsStats.blockCache.maxCacheBlocks && !giveUp) {
	giveUp = !DestroyBlock(FALSE, &pageNum);
    }

#ifndef CLEAN
    if (cacheDebug && giveUp) {
	Sys_Printf("FsSetMaxSize: Could only lower cache to %d\n", 
					fsStats.blockCache.numCacheBlocks);
    }
#endif not CLEAN
    
    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheInfoInit --
 *
 * 	Initialize the cache information for a file.  Called when setting
 *	up a handle for a file that uses the block cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the fields of the FsCacheFileInfo struct.
 *
 * ----------------------------------------------------------------------------
 */
void
FsCacheInfoInit(cacheInfoPtr, hdrPtr, version, cacheable, attrPtr)
    register FsCacheFileInfo *cacheInfoPtr;	/* Information to initialize. */
    FsHandleHeader	     *hdrPtr;		/* Back pointer to handle */
    int			     version;		/* Used to check consistency */
    Boolean		     cacheable;		/* TRUE if server says we can
						 * cache */
    FsCachedAttributes	     *attrPtr;		/* File attributes */
{
    List_InitElement(&cacheInfoPtr->links);
    List_Init(&cacheInfoPtr->dirtyList);
    List_Init(&cacheInfoPtr->blockList);
    List_Init(&cacheInfoPtr->indList);
    cacheInfoPtr->flags = (cacheable ? 0 : FS_FILE_NOT_CACHEABLE);
    cacheInfoPtr->version = version;
    cacheInfoPtr->hdrPtr = hdrPtr;
    cacheInfoPtr->blocksWritten = 0;
    cacheInfoPtr->noDirtyBlocks.waiting = 0;
    cacheInfoPtr->blocksInCache = 0;
    cacheInfoPtr->numDirtyBlocks = 0;
    cacheInfoPtr->lastTimeTried = 0;
    cacheInfoPtr->attr = *attrPtr;
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
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnFreeList(blockPtr)
    register	FsCacheBlock	*blockPtr;
{
    register	FsCacheBlock	*otherBlockPtr;

    blockPtr->flags = FS_BLOCK_FREE;
    fsStats.blockCache.numFreeBlocks++;
    if (PAGE_IS_8K) {
	/*
	 * If all blocks in the page are free then put this block onto the
	 * totally free list.  Otherwise it goes onto the partially free list.
	 */
	otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
	if (otherBlockPtr->flags & FS_BLOCK_FREE) {
	    List_Insert((List_Links *) blockPtr, LIST_ATFRONT(totFreeList));
	    List_Move((List_Links *) otherBlockPtr, LIST_ATFRONT(totFreeList));
	} else {
	    List_Insert((List_Links *) blockPtr, LIST_ATFRONT(partFreeList));
	}
    } else {
	List_Insert((List_Links *) blockPtr, LIST_ATFRONT(totFreeList));
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutFileOnDirtyList --
 *
 * 	Put the given file onto the global dirty list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutFileOnDirtyList(cacheInfoPtr)
    register	FsCacheFileInfo	*cacheInfoPtr;	/* Cache info for a file */
{
    if (!(cacheInfoPtr->flags & FS_FILE_ON_DIRTY_LIST)) {
	List_Insert((List_Links *)cacheInfoPtr, LIST_ATREAR(dirtyList));
	cacheInfoPtr->flags |= FS_FILE_ON_DIRTY_LIST;
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
 *	The block goes onto the dirty list of the file and the
 *	block cleaner is kicked.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutBlockOnDirtyList(blockPtr, shutdown)
    register	FsCacheBlock	*blockPtr;	/* Block to put on list. */
    Boolean			shutdown;	/* TRUE => are shutting
						   down the system so the
						   calling process is going
						   to synchronously sync
						   the cache. */
{
    register FsCacheFileInfo *cacheInfoPtr = blockPtr->cacheInfoPtr;

    blockPtr->flags |= FS_BLOCK_ON_DIRTY_LIST;
    List_Insert(&blockPtr->dirtyLinks, 
		LIST_ATREAR(&cacheInfoPtr->dirtyList));
    PutFileOnDirtyList(cacheInfoPtr);
    if (!shutdown) {
	StartBlockCleaner(cacheInfoPtr);
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
    FsCacheBlock	**blockPtrPtr;	/* Where to return pointer to block.
					 * NIL if caller isn't interested. */
{
    register	FsCacheBlock	*blockPtr;
    int				newCachePages;

    if (List_IsEmpty(unmappedList)) {
	Sys_Panic(SYS_WARNING, "CreateBlock: No unmapped blocks\n");
	return(FALSE);
    }
    blockPtr = (FsCacheBlock *) List_First(unmappedList);
    /*
     * Put memory behind the first available unmapped cache block.
     */
    newCachePages = Vm_MapBlock(blockPtr->blockAddr);
    if (newCachePages == 0) {
	return(FALSE);
    }
    fsStats.blockCache.numCacheBlocks += newCachePages * blocksPerPage;
    /*
     * If we are told to return a block then take it off of the list of
     * unmapped blocks and let the caller put it onto the appropriate list.
     * Otherwise put it onto the free list.
     */
    if (retBlock) {
	List_Remove((List_Links *) blockPtr);
	*blockPtrPtr = blockPtr;
    } else {
	fsStats.blockCache.numFreeBlocks++;
	blockPtr->flags = FS_BLOCK_FREE;
	List_Move((List_Links *) blockPtr, LIST_ATREAR(totFreeList));
    }
    if (PAGE_IS_8K) {
	/*
	 * Put the other block in the page onto the appropriate free list.
	 */
	blockPtr = GET_OTHER_BLOCK(blockPtr);
	blockPtr->flags = FS_BLOCK_FREE;
	fsStats.blockCache.numFreeBlocks++;
	if (retBlock) {
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(partFreeList));
	} else {
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(totFreeList));
	}
    }

    return(TRUE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FetchBlock --
 *
 *	Return a pointer to the oldest available block on the lru list.
 *	If had to sleep because all of memory is dirty then return NIL.
 *
 * Results:
 *	Pointer to oldest available block, NIL if had to wait.
 *
 * Side effects:
 *	Block deleted.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL FsCacheBlock *
FetchBlock(canWait)
    Boolean	canWait;	/* TRUE implies can sleep if all of memory is 
				 * dirty. */
{
    register	FsCacheBlock	*blockPtr;

    if (List_IsEmpty(lruList)) {
	Sys_Panic(SYS_WARNING, "FetchBlock: LRU list empty\n");
	return((FsCacheBlock *) NIL);
    }

    /* 
     * Scan list for unlocked, clean block.
     */
    LIST_FORALL(lruList, (List_Links *) blockPtr) {
	if (blockPtr->refCount > 0) {
	    /*
	     * Block is locked.
	     */
	} else if (blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Block is being cleaned.  Mark it so that it will be freed
	     * after it has been cleaned.
	     */
	    blockPtr->flags |= FS_MOVE_TO_FRONT;
	} else if (blockPtr->flags & FS_BLOCK_DIRTY) {
	    /*
	     * Put a pointer to the block in the dirty list.  
	     * After it is cleaned it will be freed.
	     */
	    PutBlockOnDirtyList(blockPtr, FALSE);
	    blockPtr->flags |= FS_MOVE_TO_FRONT;
	} else if (blockPtr->flags & FS_BLOCK_DELETED) {
	    Sys_Panic(SYS_WARNING, "FetchBlock: deleted block in LRU list\n");
	} else {
	    /*
	     * This block is clean and unlocked.  Delete it from the
	     * hash table and use it.
	     */
	    List_Remove((List_Links *) blockPtr);
	    DeleteBlock(blockPtr);
	    return(blockPtr);
	}
    }

    /*
     * We have looked at every block but we couldn't use any.
     * If possible wait until the block cleaner cleans a block for us.
     */
    DEBUG_PRINT( ("All blocks dirty\n") );
    if (canWait) {
	DEBUG_PRINT( ("Waiting for clean block\n") );
	(void) Sync_Wait(&cleanBlockCondition, FALSE);
    }
    return((FsCacheBlock *) NIL);
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
    register	FsCacheBlock	*blockPtr;
    register	FsCacheBlock	*otherBlockPtr;

    /*
     * First try the list of totally free pages.
     */
    if (!List_IsEmpty(totFreeList)) {
	DEBUG_PRINT( ("DestroyBlock: Using tot free block to lower size\n") );
	blockPtr = (FsCacheBlock *) List_First(totFreeList);
	fsStats.blockCache.numCacheBlocks -= 
		    Vm_UnmapBlock(blockPtr->blockAddr, 
				    retOnePage, pageNumPtr) * blocksPerPage;
	blockPtr->flags = FS_NOT_MAPPED;
	List_Move((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	fsStats.blockCache.numFreeBlocks--;
	if (PAGE_IS_8K) {
	    /*
	     * Unmap the other block.  The block address can point to either
	     * of the two blocks.
	     */
	    blockPtr = GET_OTHER_BLOCK(blockPtr);
	    blockPtr->flags = FS_NOT_MAPPED;
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	    fsStats.blockCache.numFreeBlocks--;
	}
	return(TRUE);
    }

    /*
     * Now take blocks from the LRU list until we get one that we can use.
     */
    while (TRUE) {
	blockPtr = FetchBlock(FALSE);
	if (blockPtr == (FsCacheBlock *) NIL) {
	    /*
	     * There are no clean blocks left so give up.
	     */
	    DEBUG_PRINT( ("DestroyBlock: No clean blocks left (1)\n") );
	    return(FALSE);
	}
	if (PAGE_IS_8K) {
	    /*
	     * We have to deal with the other block.  If it is in use, then
	     * we can't take this page.  Otherwise delete the block from
	     * the cache and put it onto the unmapped list.
	     */
	    otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
	    if (otherBlockPtr->refCount > 0 ||
		(otherBlockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) ||
		(otherBlockPtr->flags & FS_BLOCK_DIRTY)) {
		DEBUG_PRINT( ("DestroyBlock: Other block in use.\n") );
		PutOnFreeList(blockPtr);
		continue;
	    }
	    if (!(otherBlockPtr->flags & FS_BLOCK_FREE)) {
		DeleteBlock(otherBlockPtr);
	    }
	    otherBlockPtr->flags = FS_NOT_MAPPED;
	    List_Move((List_Links *) otherBlockPtr, LIST_ATREAR(unmappedList));
	}
	DEBUG_PRINT( ("DestroyBlock: Using in-use block to lower size\n") );
	blockPtr->flags = FS_NOT_MAPPED;
	List_Insert((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	fsStats.blockCache.numCacheBlocks -= 
		    Vm_UnmapBlock(blockPtr->blockAddr, 
				retOnePage, pageNumPtr) * blocksPerPage;
	return(TRUE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_GetPageFromFS --
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
Fs_GetPageFromFS(timeLastAccessed, pageNumPtr)
    int	timeLastAccessed;
    int	*pageNumPtr;
{
    register	FsCacheBlock	*blockPtr;

    LOCK_MONITOR;

    fsStats.blockCache.vmRequests++;
    *pageNumPtr = -1;
    if (fsStats.blockCache.numCacheBlocks > 
		fsStats.blockCache.minCacheBlocks && !List_IsEmpty(lruList)) {
	fsStats.blockCache.triedToGiveToVM++;
	blockPtr = (FsCacheBlock *) List_First(lruList);
	if (blockPtr->timeReferenced < timeLastAccessed) {
	    fsStats.blockCache.vmGotPage++;
	    (void) DestroyBlock(TRUE, pageNumPtr);
	}
    }

    UNLOCK_MONITOR;
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
 * FsCacheFetchBlock --
 *
 *	Return in *blockPtrPtr a pointer to a block in the 
 *	cache that corresponds to virtual block blockNum in the file 
 *	identified by *cacheInfoPtr.  If the block for the file is not in the 
 *	cache then *foundPtr is set to FALSE and if allocate is 
 *	TRUE, then a clean block is returned.  Otherwise a pointer to the 
 *	actual data block is returned and *foundPtr is set to TRUE.
 *	The block that is returned is locked down in the cache (i.e. it cannot
 *	be replaced) until it is unlocked by FsCacheUnlockBlock.  If the block
 *	isn't found or the FS_IO_IN_PROGRESS flag is given then the block is
 * 	marked as IO in progress and must be either unlocked by 
 *	FsCacheUnlockBlock or marked as IO done by FsCacheIODone.
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
ENTRY void
FsCacheFetchBlock(cacheInfoPtr, blockNum, flags, blockPtrPtr, foundPtr)
    register FsCacheFileInfo *cacheInfoPtr; /* Pointer to the cache state 
				   * for the file. */
    int		 blockNum;	/* Virtual block number in the file. */
    int		 flags;		/* FS_CACHE_DONT_BLOCK |
				 * FS_READ_AHEAD_BLOCK | FS_IO_IN_PROGRESS
				 * plus the type of block */
    FsCacheBlock **blockPtrPtr; /* Where pointer to cache block information
				 * structure is returned. The structure
				 * contains the virtual address of the 
				 * actual cache block. */
    Boolean	*foundPtr;	/* TRUE if the block is present in the
				 * cache, FALSE if not. */
{
    BlockHashKey		blockHashKey;
    register	Hash_Entry	*hashEntryPtr;
    register	FsCacheBlock	*blockPtr;
    FsCacheBlock		*otherBlockPtr;
    FsCacheBlock		*newBlockPtr;
    int				refTime;

    LOCK_MONITOR;

    /*
     * See if the block is in the cache.
     */
    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, blockNum);

again:

    hashEntryPtr = Hash_Find(blockHashTable, (Address) &blockHashKey);
    blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);
    if (blockPtr != (FsCacheBlock *) NIL) {
	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    UNLOCK_MONITOR;
	    Sys_Panic(SYS_FATAL, "CacheFetchBlock hashing error\n");
	    *foundPtr = FALSE;
	    *blockPtrPtr = (FsCacheBlock *) NIL;
	    return;
	}
	if (((flags & FS_IO_IN_PROGRESS) && 
	     blockPtr->refCount > 0) ||
	    (blockPtr->flags & FS_IO_IN_PROGRESS)) {
	    if (flags & FS_CACHE_DONT_BLOCK) {
		*foundPtr = TRUE;
		*blockPtrPtr = (FsCacheBlock *) NIL;
		UNLOCK_MONITOR;
		return;
	    }
	    /*
	     * Wait until becomes unlocked.  Start over when wakeup 
	     * because the block could go away while we are waiting.
	     */
	    FS_TRACE_BLOCK(FS_TRACE_BLOCK_WAIT, blockPtr);
	    (void)Sync_Wait(&blockPtr->ioDone, FALSE);
	    goto again;
	}
	blockPtr->refCount++;
	if (flags & FS_IO_IN_PROGRESS) {
	    blockPtr->flags |= FS_IO_IN_PROGRESS;
	}
	FS_TRACE_BLOCK(FS_TRACE_BLOCK_HIT, blockPtr);
	*foundPtr = TRUE;
	*blockPtrPtr = blockPtr;
	UNLOCK_MONITOR;
	return;
    }
    *foundPtr = FALSE;
	
    /*
     * Have to allocate a block.  If there is a free block use it.  Otherwise
     * either take a block off of the lru list or make a new one.
     */
    blockPtr = (FsCacheBlock *) NIL;
    while (blockPtr == (FsCacheBlock *) NIL) {
	if (!List_IsEmpty(partFreeList)) {
	    /*
	     * Use partially free blocks first.
	     */
	    fsStats.blockCache.numFreeBlocks--;
	    fsStats.blockCache.partFree++;
	    blockPtr = (FsCacheBlock *) List_First(partFreeList);
	    List_Remove((List_Links *) blockPtr);
	} else if (!List_IsEmpty(totFreeList)) {
	    /*
	     * Can't find a partially free block so use a totally free
	     * block.
	     */
	    fsStats.blockCache.numFreeBlocks--;
	    fsStats.blockCache.totFree++;
	    blockPtr = (FsCacheBlock *) List_First(totFreeList);
	    List_Remove((List_Links *) blockPtr);
	    if (PAGE_IS_8K) {
		otherBlockPtr = GET_OTHER_BLOCK(blockPtr);
		List_Move((List_Links *) otherBlockPtr,
			      LIST_ATREAR(partFreeList));
	    }
	} else {
	    /*
	     * Can't find any free blocks so have to use one of our blocks
	     * or create new ones.
	     */
	    if (fsStats.blockCache.numCacheBlocks >= 
					fsStats.blockCache.maxCacheBlocks) {
		/*
		 * We can't have anymore blocks so reuse one of our own.
		 */
		blockPtr = FetchBlock(TRUE);
		fsStats.blockCache.lru++;
	    } else {
		/*
		 * See if VM has an older page than we have.
		 */
		refTime = Vm_GetRefTime();
		blockPtr = (FsCacheBlock *) List_First(lruList);
		DEBUG_PRINT( ("FsCacheBlockFetch: fs=%d vm=%d\n", 
				   blockPtr->timeReferenced, refTime) );
		if (blockPtr->timeReferenced > refTime) {
		    /*
		     * VM has an older page than us so we get to make a new
		     * block.
		     */
		    DEBUG_PRINT( ("FsCacheBlockFetch:Creating new block\n" ));
		    if (!CreateBlock(TRUE, &newBlockPtr)) {
			DEBUG_PRINT( ("FsCacheBlockFetch: Couldn't create block\n" ));
			fsStats.blockCache.lru++;
			blockPtr = FetchBlock(TRUE);
		    } else {
			fsStats.blockCache.unmapped++;
			blockPtr = newBlockPtr;
		    }
		} else {
		    /*
		     * We have an older block than VM's oldest page so reuse
		     * the block.
		     */
		    DEBUG_PRINT( ("FsCacheBlockFetch: Recycling block\n") );
		    fsStats.blockCache.lru++;
		    blockPtr = FetchBlock(TRUE);
		}
	    }
	}
    }

    cacheInfoPtr->blocksInCache++;
    blockPtr->cacheInfoPtr = cacheInfoPtr;
    blockPtr->refCount++;
    blockPtr->flags = flags & (FS_DATA_CACHE_BLOCK | FS_IND_CACHE_BLOCK |
			       FS_DESC_CACHE_BLOCK | FS_DIR_CACHE_BLOCK |
    			       FS_READ_AHEAD_BLOCK);
    blockPtr->flags |= FS_IO_IN_PROGRESS;
    blockPtr->fileNum = cacheInfoPtr->hdrPtr->fileID.minor;
    blockPtr->blockNum = blockNum;
    blockPtr->blockSize = -1;
    blockPtr->timeDirtied = 0;
    *blockPtrPtr = blockPtr;
    FS_TRACE_BLOCK(FS_TRACE_NO_BLOCK, blockPtr);
    Hash_SetValue(hashEntryPtr, blockPtr);
    List_Insert((List_Links *) blockPtr, LIST_ATREAR(lruList));
    List_InitElement(&blockPtr->fileLinks);
    if (flags & FS_IND_CACHE_BLOCK) {
	List_Insert(&blockPtr->fileLinks, LIST_ATREAR(&cacheInfoPtr->indList));
    } else {
	List_Insert(&blockPtr->fileLinks,LIST_ATREAR(&cacheInfoPtr->blockList));
    }

    UNLOCK_MONITOR;
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheIODone --
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
FsCacheIODone(blockPtr)
    FsCacheBlock *blockPtr;	/* Pointer to block information for block.*/
{
    LOCK_MONITOR;

    Sync_Broadcast(&blockPtr->ioDone);
    blockPtr->flags &= ~FS_IO_IN_PROGRESS;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheUnlockBlock --
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
FsCacheUnlockBlock(blockPtr, timeDirtied, diskBlock, blockSize, flags)
    FsCacheBlock *blockPtr;	/* Pointer to block information for block
				   that is to be released. */
    unsigned int timeDirtied;	/* Time in seconds that the block was 
				   dirtied. */
    int		 diskBlock;	/* If not -1 is the block on disk where this
				   block resides.  For remote blocks this 
				   should be the same as blockPtr->blockNum.*/
    int		 blockSize;	/* The number of valid bytes in this block. */
    int		 flags;		/* FS_DELETE_BLOCK | FS_CLEAR_READ_AHEAD */
{
    LOCK_MONITOR;

    if (blockPtr->flags & FS_BLOCK_FREE) {
	Sys_Panic(SYS_FATAL, "Checking free block\n");
    }

    if (blockPtr->flags & FS_IO_IN_PROGRESS) {
	Sync_Broadcast(&blockPtr->ioDone);
	blockPtr->flags &= ~FS_IO_IN_PROGRESS;
    }

    if (flags & FS_DELETE_BLOCK) {
	/*
	 * The caller is deleting this block from the cache.  Decrement the
	 * lock count and then invalidate the block.
	 */
	blockPtr->refCount--;
	CacheFileInvalidate(blockPtr->cacheInfoPtr, blockPtr->blockNum, 
			    blockPtr->blockNum);
	UNLOCK_MONITOR;
	return;
    }

    if (flags & FS_CLEAR_READ_AHEAD) {
	blockPtr->flags &= ~FS_READ_AHEAD_BLOCK;
    }

    if (timeDirtied != 0) {
	if (!(blockPtr->flags & FS_BLOCK_DIRTY)) {
	    /*
	     * Increment the count of dirty blocks if the block isn't marked
	     * as dirty.  The block cleaner will decrement the count 
	     * after it cleans a block.
	     */
	    blockPtr->cacheInfoPtr->numDirtyBlocks++;
	    if (traceDirtyBlocks) {
		Sys_Printf("UNL FD=%d Num=%d\n",
			   blockPtr->cacheInfoPtr->hdrPtr->fileID.minor,
			   blockPtr->cacheInfoPtr->numDirtyBlocks);
	    }
	    blockPtr->flags |= FS_BLOCK_DIRTY;
	}
	blockPtr->timeDirtied = timeDirtied;
    }

    if (diskBlock != -1) {
	if (blockPtr->diskBlock != diskBlock ||
	    blockPtr->blockSize != blockSize) {
	    /*
	     * The caller is changing where this block lives on disk.  If
	     * so we have to wait until the block has finished being written
	     * before we can allow this to happen.
	     */
	    while (blockPtr->flags & FS_BLOCK_BEING_WRITTEN) {
		(void)Sync_Wait(&blockPtr->ioDone, FALSE);
	    }
	    blockPtr->diskBlock = diskBlock;
	    blockPtr->blockSize = blockSize;
	}
    }

    /*
     * If the file is write thru then force the block out.
     */
    if ((blockPtr->cacheInfoPtr->flags & FS_FILE_IS_WRITE_THRU) && 
	(blockPtr->flags & (FS_BLOCK_DIRTY | FS_BLOCK_BEING_WRITTEN))) {
	if (!(blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST)) {
	    PutBlockOnDirtyList(blockPtr, FALSE);
	}
	do {
	    (void) Sync_Wait(&blockPtr->ioDone, FALSE);
	    if (sys_ShuttingDown) {
		break;
	    }
	} while (blockPtr->flags & (FS_BLOCK_DIRTY | FS_BLOCK_BEING_WRITTEN));
    }

    blockPtr->refCount--;

    if (blockPtr->refCount == 0) {
	/*
	 * The block is no longer locked.  Move it to the end of the lru list,
	 * mark it as being referenced, and wake up anybody waiting for the
	 * block to become unlocked.
	 */
	blockPtr->timeReferenced = fsTimeInSeconds;
	blockPtr->flags &= ~FS_MOVE_TO_FRONT;
	List_Move((List_Links *) blockPtr, LIST_ATREAR(lruList));
	Sync_Broadcast(&blockPtr->ioDone);
	if (blockPtr->flags & FS_BLOCK_CLEANER_WAITING) {
	    StartBlockCleaner(blockPtr->cacheInfoPtr);
	    blockPtr->flags &= ~FS_BLOCK_CLEANER_WAITING;
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheBlockTrunc --
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
FsCacheBlockTrunc(cacheInfoPtr, blockNum, newBlockSize)
    FsCacheFileInfo *cacheInfoPtr;	/* Cache state of file. */
    int		blockNum;		/* Block to truncate. */
    int		newBlockSize;		/* New block size. */
{
    register Hash_Entry	     *hashEntryPtr;
    register FsCacheBlock    *blockPtr;
    BlockHashKey	     blockHashKey;

    LOCK_MONITOR;

    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

    hashEntryPtr = GetUnlockedBlock(&blockHashKey, blockNum);
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    Sys_Panic(SYS_FATAL, "CacheBlockTrunc, hashing error\n");
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
 * FsCacheFileInvalidate --
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
FsCacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock)
    FsCacheFileInfo *cacheInfoPtr;	/* Cache state of file to invalidate. */
    int		firstBlock;	/* First block to invalidate. */
    int		lastBlock;	/* Last block to invalidate. */
{
    LOCK_MONITOR;

    CacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);

    UNLOCK_MONITOR;
}

void	DeleteBlockFromDirtyList();


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
    FsCacheFileInfo	*cacheInfoPtr;	/* File to invalidate. */
    int			firstBlock;	/* First block to invalidate. */
    int			lastBlock;	/* Last block to invalidate. */
{
    register Hash_Entry	     *hashEntryPtr;
    register FsCacheBlock    *blockPtr;
    BlockHashKey	     blockHashKey;
    int			     i;

    FS_TRACE_IO(FS_TRACE_SRV_WRITE_1, cacheInfoPtr->hdrPtr->fileID, firstBlock, lastBlock);

    if (cacheInfoPtr->blocksInCache > 0) {
	SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);
	if ((lastBlock == FS_LAST_BLOCK) &&
	    (cacheInfoPtr->attr.lastByte > 0)) {
	    lastBlock = cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE;
	}

	for (i = firstBlock; i <= lastBlock; i++) {
	    hashEntryPtr = GetUnlockedBlock(&blockHashKey, i);
	    if (hashEntryPtr == (Hash_Entry *) NIL) {
		continue;
	    }
	    blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);
	    if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
		Sys_Panic(SYS_FATAL, "CacheFileInvalidate, hashing error\n");
		continue;
	    }

	    /*
	     * Remove it from the hash table.
	     */
	    cacheInfoPtr->blocksInCache--;
	    List_Remove(&blockPtr->fileLinks);
	    Hash_Delete(blockHashTable, hashEntryPtr);
	    FS_TRACE_BLOCK(FS_TRACE_DEL_BLOCK, blockPtr);
    
	    /*
	     * Invalidate the block, including removing it from dirty list
	     * if necessary.
	     */
	    if (blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) {
		DeleteBlockFromDirtyList(blockPtr);
	    }
	    if (blockPtr->flags & FS_BLOCK_DIRTY) {
		cacheInfoPtr->numDirtyBlocks--;
		if (traceDirtyBlocks) {
		    Sys_Printf("Inv FD=%d Num=%d\n", 
			    cacheInfoPtr->hdrPtr->fileID.minor,
			    cacheInfoPtr->numDirtyBlocks);
		}
	    }
	    List_Remove((List_Links *) blockPtr);
	    PutOnFreeList(blockPtr);
	}
    }

    if (cacheInfoPtr->blocksInCache == 0) {
	cacheInfoPtr->flags &=
			~(FS_CACHE_SERVER_DOWN | FS_CACHE_NO_DISK_SPACE |
			  FS_CACHE_DOMAIN_DOWN | FS_CACHE_GENERIC_ERROR);
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
    register	FsCacheBlock	*blockPtr;
{
    register	FsCacheFileInfo	*cacheInfoPtr;

    cacheInfoPtr = blockPtr->cacheInfoPtr;
    List_Remove(&blockPtr->dirtyLinks);
    if ((cacheInfoPtr->flags & FS_FILE_ON_DIRTY_LIST) &&
        List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	/*
	 * No more dirty blocks for this file.  Remove the file from the dirty
	 * list and wakeup anyone waiting for the file's list to become
	 * empty.
	 */
	List_Remove((List_Links *)cacheInfoPtr);
	cacheInfoPtr->flags &= ~FS_FILE_ON_DIRTY_LIST;
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
    }

    if (blockPtr->flags & FS_WRITE_BACK_WAIT) {
	numWriteBackBlocks--;
	blockPtr->flags &= ~FS_WRITE_BACK_WAIT;
	if (numWriteBackBlocks == 0) {
	    Sync_Broadcast(&writeBackComplete);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FileWriteBackStub --
 *
 *      This is the stub for the FsCacheFileWriteBack system call.
 *	The byte arguments are rounded to blocks, and the range of
 *	blocks that covers the byte range is written back out of the cache.
 *
 * Results:
 *	A return status or SUCCESS if successful.
 *
 * Side effects:
 *	Write out the range of blocks in the cache.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_FileWriteBackStub(streamID, firstByte, lastByte, shouldBlock)
    int		streamID;	/* Stream ID of file to write back. */
    int		firstByte;	/* First byte to write back. */
    int		lastByte;	/* Last byte to write back. */
    Boolean	shouldBlock;	/* TRUE if should wait for the blocks to go
				 * to disk. */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr;
    FsCacheFileInfo	*cacheInfoPtr;
    int			blocksSkipped;
    int			flags;

    status = FsGetStreamPtr(Proc_GetEffectiveProc(), 
			    streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    switch(streamPtr->ioHandlePtr->fileID.type) {
	case FS_LCL_FILE_STREAM: {
	    register FsLocalFileIOHandle *localHandlePtr;
	    localHandlePtr = (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &localHandlePtr->cacheInfo;
	    break;
	}
	case FS_RMT_FILE_STREAM: {
	    register FsRmtFileIOHandle *rmtHandlePtr;
	    rmtHandlePtr = (FsRmtFileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &rmtHandlePtr->cacheInfo;
	    break;
	}
	default:
	    return(FS_WRONG_TYPE);
    }
    flags = 0;
    if (shouldBlock) {
	flags |= FS_FILE_WB_WAIT;
    }
    status = FsCacheFileWriteBack(cacheInfoPtr, firstByte / FS_BLOCK_SIZE,
		    lastByte / FS_BLOCK_SIZE, flags, &blocksSkipped);

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheFileWriteBack --
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
FsCacheFileWriteBack(cacheInfoPtr, firstBlock, lastBlock, flags,
	blocksSkippedPtr)
    register FsCacheFileInfo *cacheInfoPtr;	/* State to force out. */
    int		firstBlock;	/* First block to write back. */
    int		lastBlock;	/* Last block to write back. */
    int		flags;		/* FS_FILE_WB_WAIT | FS_FILE_WB_INDIRECT |
				 * FS_FILE_WB_INVALIDATE. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
{
    register Hash_Entry	     *hashEntryPtr;
    register FsCacheBlock    *blockPtr;
    BlockHashKey	     blockHashKey;
    int			     i;
    ReturnStatus	     status;

    LOCK_MONITOR;

    *blocksSkippedPtr = 0;

    /*
     * Clear out the host down and no disk space flags so we can retry
     * for this file.
     */
    cacheInfoPtr->flags &= 
		    ~(FS_CACHE_SERVER_DOWN | FS_CACHE_NO_DISK_SPACE | 
		      FS_CACHE_DOMAIN_DOWN | FS_CACHE_GENERIC_ERROR);

    if (cacheInfoPtr->blocksInCache == 0) {
	UNLOCK_MONITOR;
	return(SUCCESS);
    }

    SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

    if ((lastBlock == FS_LAST_BLOCK) &&
	(cacheInfoPtr->attr.lastByte > 0)) {
	lastBlock = cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE;
    }
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

	blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    Sys_Panic(SYS_FATAL, "CacheWriteBack, hashing error\n");
	    UNLOCK_MONITOR;
	    return(FAILURE);
	}

	if (flags & (FS_FILE_WB_INVALIDATE | FS_FILE_WB_WAIT)) {
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
	    if (flags & FS_FILE_WB_INVALIDATE) {
		cacheInfoPtr->blocksInCache--;
		List_Remove(&blockPtr->fileLinks);
		Hash_Delete(blockHashTable, hashEntryPtr);
		List_Remove((List_Links *) blockPtr);
		blockPtr->flags |= FS_BLOCK_DELETED;
		FS_TRACE_BLOCK(FS_TRACE_DEL_BLOCK, blockPtr);
	    }
	} else if (blockPtr->refCount > 0) {
	    /* 
	     * If the block is locked and we are not invalidating it, 
	     * skip the block because it might be being modified.
	     */
	    (*blocksSkippedPtr)++;
	    continue;
	}

	/*
	 * Write back the block.  If the block is already being waited on then
	 * don't have to do anything special.
	 */
	if (blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Blocks already on the dirty list, no need to do anything.
	     */
	} else if (blockPtr->flags & FS_BLOCK_DIRTY) {
	    PutBlockOnDirtyList(blockPtr, FALSE);
	} else if (flags & FS_FILE_WB_INVALIDATE) {
	    /*
	     * This block is clean.  We need to free it if it is to be
	     * invalidated.
	     */
	    PutOnFreeList(blockPtr);
	}
    }

    if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	StartBlockCleaner(cacheInfoPtr);
    }

    /*
     * Wait until all blocks are written back.
     */
    if (flags & FS_FILE_WB_WAIT) {
	while (!List_IsEmpty(&cacheInfoPtr->dirtyList) && 
	       !(cacheInfoPtr->flags & 
		    (FS_CACHE_SERVER_DOWN | FS_CACHE_NO_DISK_SPACE |
		     FS_CACHE_DOMAIN_DOWN | FS_CACHE_GENERIC_ERROR)) &&
	       !sys_ShuttingDown) {
	    (void) Sync_Wait(&cacheInfoPtr->noDirtyBlocks, FALSE);
	}
    }

    switch (cacheInfoPtr->flags&(FS_CACHE_SERVER_DOWN|FS_CACHE_NO_DISK_SPACE|
				 FS_CACHE_DOMAIN_DOWN|FS_CACHE_GENERIC_ERROR)) {
	case FS_CACHE_SERVER_DOWN:
	    status = RPC_TIMEOUT;
	    break;
	case FS_CACHE_NO_DISK_SPACE:
	    status = FS_NO_DISK_SPACE;
	    break;
	case FS_CACHE_DOMAIN_DOWN:
	    status = FS_DOMAIN_UNAVAILABLE;
	    break;
	case FS_CACHE_GENERIC_ERROR:
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
 * Fs_CacheBlocksUnneeded --
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
 *	None here, see FsCacheBlocksUnneeded.
 *	All blocks that span the given range of bytes are moved to the front of
 *	the LRU list and marked as not-referenced.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
Fs_CacheBlocksUnneeded(streamPtr, offset, numBytes, objectFile)
    register Fs_Stream	*streamPtr;	/* File for which blocks are unneeded.*/
    int			offset;		/* First byte which is unneeded. */
    int			numBytes;	/* Number of bytes that are unneeded. */
    Boolean		objectFile;	/* TRUE if this is for an object 
					 * file.*/
{
    register FsCacheFileInfo *cacheInfoPtr;

    switch (streamPtr->ioHandlePtr->fileID.type) {
	case FS_LCL_FILE_STREAM: {
	    register FsLocalFileIOHandle *localHandlePtr;
	    if (objectFile) {
		/*
		 * Keep the blocks cached for remote clients.
		 */
		return;
	    }
	    localHandlePtr = (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &localHandlePtr->cacheInfo;
	    break;
	}
	case FS_RMT_FILE_STREAM: {
	    register FsRmtFileIOHandle *rmtHandlePtr;
	    rmtHandlePtr = (FsRmtFileIOHandle *)streamPtr->ioHandlePtr;
	    cacheInfoPtr = &rmtHandlePtr->cacheInfo;
	    break;
	}
	default:
	    Sys_Panic(SYS_FATAL, "Fs_CacheBlocksUnneeded, bad stream type %d\n",
		streamPtr->ioHandlePtr->fileID.type);
	    return;
    }
    FsCacheBlocksUnneeded(cacheInfoPtr, offset, numBytes);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheBlocksUnneeded --
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
FsCacheBlocksUnneeded(cacheInfoPtr, offset, numBytes)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state. */
    int			offset;		/* First byte which is unneeded. */
    int			numBytes;	/* Number of bytes that are unneeded. */
{
    register Hash_Entry		*hashEntryPtr;
    register FsCacheBlock    	*blockPtr;
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

	blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);

	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    Sys_Panic(SYS_FATAL, "CacheBlocksUnneeded, hashing error\n");
	    continue;
	}

	if (blockPtr->refCount > 0) {
	    /*
	     * The block is locked.  This means someone is doing something with
	     * it so we just skip it.
	     */
	    continue;
	}
	if (blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Block is being cleaned.  Set the flag so that it will be 
	     * moved to the front after it has been cleaned.
	     */
	    blockPtr->flags |= FS_MOVE_TO_FRONT;
	} else {
	    /*
	     * Move the block to the front of the LRU list.
	     */
	    List_Move((List_Links *) blockPtr, LIST_ATFRONT(lruList));
	}
	fsStats.blockCache.blocksPitched++;
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
 * Fs_CacheWriteBack --
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
ENTRY void
Fs_CacheWriteBack(writeBackTime, blocksSkippedPtr, shutdown)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	shutdown;	   /* TRUE if the system is being shut down. */
{
    LOCK_MONITOR;

    CacheWriteBack(writeBackTime, blocksSkippedPtr, shutdown);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_CacheEmpty --
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
Fs_CacheEmpty(numLockedBlocksPtr)
    int *numLockedBlocksPtr;
{
    int 			blocksSkipped;
    register	FsCacheBlock	*blockPtr;
    register	List_Links	*nextPtr, *listPtr;

    LOCK_MONITOR;

    *numLockedBlocksPtr = 0;
    CacheWriteBack(-1, &blocksSkipped, FALSE);
    listPtr = lruList;
    nextPtr = List_First(listPtr);
    while (!List_IsAtEnd(listPtr, nextPtr)) {
	blockPtr = (FsCacheBlock *)nextPtr;
	nextPtr = List_Next(nextPtr);
	if (blockPtr->refCount > 0 || 
	    (blockPtr->flags & (FS_BLOCK_DIRTY | FS_BLOCK_ON_DIRTY_LIST))) {
	    /* 
	     * Skip locked or dirty blocks.
	     */
	    (*numLockedBlocksPtr)++;
	} else {
	    List_Remove((List_Links *) blockPtr);
	    DeleteBlock(blockPtr);
	    PutOnFreeList(blockPtr);
	}
    }

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
INTERNAL void
CacheWriteBack(writeBackTime, blocksSkippedPtr, shutdown)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	shutdown;	   /* TRUE if the system is being shut down. */
{
    register FsCacheFileInfo	*cacheInfoPtr;
    register FsCacheBlock 	*blockPtr;
    register List_Links	  	*listPtr;
    int				currentTime;

    currentTime = fsTimeInSeconds;

    *blocksSkippedPtr = 0;

    listPtr = lruList;
    LIST_FORALL(listPtr, (List_Links *) blockPtr) {
	cacheInfoPtr = blockPtr->cacheInfoPtr;
	if (cacheInfoPtr->flags & FS_CACHE_SERVER_DOWN) {
	    /*
	     * Don't bother to write-back files for which the server is
	     * down.  These will be written back during recovery.
	     */
	    continue;
	}
	if (cacheInfoPtr->flags &
		(FS_CACHE_NO_DISK_SPACE | FS_CACHE_DOMAIN_DOWN |
		 FS_CACHE_GENERIC_ERROR)) {
	    /*
	     * Retry for these types of errors.
	     */
	    if (cacheInfoPtr->lastTimeTried < currentTime) {
		cacheInfoPtr->flags &=
			    ~(FS_CACHE_NO_DISK_SPACE | FS_CACHE_DOMAIN_DOWN |
			      FS_CACHE_GENERIC_ERROR);
		StartBlockCleaner(cacheInfoPtr);
	    } else {
		continue;
	    }
	}
	if (blockPtr->refCount > 0) {
	    /* 
	     * Skip locked blocks because they might be in the process of
	     * being modified.
	     */
	    (*blocksSkippedPtr)++;
	    continue;
	}
	if (blockPtr->flags & FS_WRITE_BACK_WAIT) {
	    /*
	     * Someone is already waiting on this block.  This means that
	     * numWriteBackBlocks has been incremented once already for this
	     * block so no need to increment it again.
	     */
	    continue;
	}
	if (blockPtr->flags & FS_BLOCK_ON_DIRTY_LIST) {
	    blockPtr->flags |= FS_WRITE_BACK_WAIT;
	    numWriteBackBlocks++;
	} else if ((blockPtr->flags & FS_BLOCK_DIRTY) &&
		   (blockPtr->timeDirtied < writeBackTime || shutdown)) {
	    PutBlockOnDirtyList(blockPtr, shutdown);
	    blockPtr->flags |= FS_WRITE_BACK_WAIT;
	    numWriteBackBlocks++;
	}
    }
    if (!shutdown && numWriteBackBlocks > 0) {
	while (numWriteBackBlocks > 0 && !sys_ShuttingDown) {
	    (void) Sync_Wait(&writeBackComplete, FALSE);
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to clean dirty blocks.
 *
 * ----------------------------------------------------------------------------
 */

#define WRITE_RETRY_INTERVAL	30

/*
 * ----------------------------------------------------------------------------
 *
 * FsCleanBlocks
 *
 *	Write all blocks on the dirty list to disk.  Called either from
 *	a block cleaner process or synchronously during system shutdown.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The dirty list is emptied.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
FsCleanBlocks(data, callInfoPtr)
    ClientData		data;		/* Background flag.  If TRUE it means
					 * we are called from a block cleaner
					 * process.  Otherwise we being called
					 * synchrounously during a shutdown */
    Proc_CallInfo	*callInfoPtr;	/* Not Used. */
{
    Boolean			backGround;
    register	FsCacheBlock	*blockPtr;
    FsCacheBlock		*tBlockPtr;
    ReturnStatus		status;
    Boolean			lastDirtyBlock;
    FsCacheFileInfo		*cacheInfoPtr;

    backGround = (Boolean) data;
    GetDirtyFile(backGround, &cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
    blockPtr = tBlockPtr;
    while (cacheInfoPtr != (FsCacheFileInfo *)NIL) {
	while (blockPtr != (FsCacheBlock *)NIL) {
	    if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
		Sys_Panic(SYS_FATAL, "FsCleanBlocks, bad block\n");
		continue;
	    }
	    /*
	     * Gather statistics.
	     */
	    fsStats.blockCache.blocksWrittenThru++;
	    switch (blockPtr->flags &
		    (FS_DATA_CACHE_BLOCK | FS_IND_CACHE_BLOCK |
		     FS_DESC_CACHE_BLOCK | FS_DIR_CACHE_BLOCK)) {
		case FS_DATA_CACHE_BLOCK:
		    fsStats.blockCache.dataBlocksWrittenThru++;
		    break;
		case FS_IND_CACHE_BLOCK:
		    fsStats.blockCache.indBlocksWrittenThru++;
		    break;
		case FS_DESC_CACHE_BLOCK:
		    fsStats.blockCache.descBlocksWrittenThru++;
		    break;
		case FS_DIR_CACHE_BLOCK:
		    fsStats.blockCache.dirBlocksWrittenThru++;
		    break;
		default:
		    Sys_Panic(SYS_WARNING, "FsCleanBlocks: Unknown block type\n");
	    }

	    /*
	     * Write the block.
	     */
	    if (blockPtr->blockSize < 0) {
		Sys_Panic(SYS_FATAL, "FsCleanBlocks, uninitialized block size\n");
		status = FAILURE;
		break;
	    }
	    FS_TRACE_BLOCK(FS_TRACE_BLOCK_WRITE, blockPtr);
	    status = (*fsStreamOpTable[cacheInfoPtr->hdrPtr->fileID.type].blockWrite)
		    (cacheInfoPtr->hdrPtr, blockPtr->diskBlock,
		     blockPtr->blockSize, blockPtr->blockAddr, lastDirtyBlock);
	    ProcessCleanBlock(cacheInfoPtr, blockPtr, status);
	    if (status != SUCCESS) {
		break;
	    }
	    GetDirtyBlock(cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
	    blockPtr = tBlockPtr;
	}
	GetDirtyFile(backGround, &cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
	blockPtr = tBlockPtr;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * StartBlockCleaner --
 *
 * 	Start a block cleaner process to write out a newly added page to
 *	the given file's dirty list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Number of block cleaner processes may be incremented.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
StartBlockCleaner(cacheInfoPtr)
    FsCacheFileInfo *cacheInfoPtr;	/* Cache info for the file. */
{
    if (!(cacheInfoPtr->flags & FS_FILE_BEING_WRITTEN) &&
	numBlockCleaners < fsMaxBlockCleaners) {
	Proc_CallFunc(FsCleanBlocks, (ClientData) TRUE, 0);
	numBlockCleaners++;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *  GetDirtyFile --
 *
 *     	Take the first dirty file off of the dirty file list and
 *	return a pointer to the cache state for that file.  This
 *	is used by the block cleaner to walk through the dirty list.
 *
 * Results:
 *	A pointer to the cache state for the first file on the dirty list.
 *	If there are no files then a NIL pointer is returned.  Also a
 *	pointer to the first block on the dirty list for the file is returned.
 *
 * Side effects:
 *      An element is removed from the dirty file list.  The fact that
 *	the dirty list links are at the beginning of the cacheInfo struct
 *	is well known and relied on to map from the dirty list to cacheInfoPtr.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
GetDirtyFile(backGround, cacheInfoPtrPtr, blockPtrPtr, lastDirtyBlockPtr)
    Boolean		backGround;
    FsCacheFileInfo	**cacheInfoPtrPtr;
    FsCacheBlock	**blockPtrPtr;
    Boolean		*lastDirtyBlockPtr;
{
    register FsCacheFileInfo *cacheInfoPtr;

    LOCK_MONITOR;

    *cacheInfoPtrPtr = (FsCacheFileInfo *)NIL;

    if (List_IsEmpty(dirtyList)) {
	if (backGround) {
	    numBlockCleaners--;
	}
	UNLOCK_MONITOR;
	return;
    }

    LIST_FORALL(dirtyList, (List_Links *)cacheInfoPtr) {
	if (cacheInfoPtr->flags & FS_CACHE_SERVER_DOWN) {
	    /*
	     * The host is down for this file.
	     */
	    continue;
	} else if (cacheInfoPtr->flags & FS_CLOSE_IN_PROGRESS) {
	    /*
	     * Close in progress on the file the block lives in so we aren't
	     * allowed to write any more blocks.
	     */
	    continue;
	} else if (cacheInfoPtr->flags & 
		       (FS_CACHE_NO_DISK_SPACE | FS_CACHE_DOMAIN_DOWN |
		        FS_CACHE_GENERIC_ERROR)) {
	    if (fsTimeInSeconds - cacheInfoPtr->lastTimeTried <
			WRITE_RETRY_INTERVAL) {
		continue;
	    }
	    cacheInfoPtr->flags &= 
			    ~(FS_CACHE_NO_DISK_SPACE | FS_CACHE_DOMAIN_DOWN |
			      FS_CACHE_GENERIC_ERROR);
	}
	List_Remove((List_Links *)cacheInfoPtr);
	cacheInfoPtr->flags |= FS_FILE_BEING_WRITTEN;
	cacheInfoPtr->flags &= ~FS_FILE_ON_DIRTY_LIST;
	GetDirtyBlockInt(cacheInfoPtr, blockPtrPtr, lastDirtyBlockPtr);
	if (*blockPtrPtr != (FsCacheBlock *)NIL) {
	    *cacheInfoPtrPtr = cacheInfoPtr;
	
	    UNLOCK_MONITOR;
	    return;
	}
    }

    FS_CACHE_DEBUG_PRINT("GetDirtyFile: All files unusable\n");
    if (backGround) {
	numBlockCleaners--;
    }
    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 *  GetDirtyBlock --
 *
 *     	Take the first block off of the dirty list for a file and
 *	return a pointer to it.  This calls GetDirtyBlockInt to do the work.
 *
 * Results:
 *	A pointer to the first block on the file's dirty list,
 *	or NIL if list is empty.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
GetDirtyBlock(cacheInfoPtr, blockPtrPtr, lastDirtyBlockPtr)
    FsCacheFileInfo	*cacheInfoPtr;
    FsCacheBlock	**blockPtrPtr;
    Boolean		*lastDirtyBlockPtr;
{
    LOCK_MONITOR;

    GetDirtyBlockInt(cacheInfoPtr, blockPtrPtr, lastDirtyBlockPtr);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 *  GetDirtyBlockInt --
 *
 *     	Take the first page off of a file's dirty list and return a pointer
 *	to it.  The synchronization between closing a file and writing
 *	back its blocks is done here.  If there are no more dirty blocks
 *	and the file is being closed we poke the closing process.  If
 *	there are still dirty blocks and someone is closing the file
 *	we put the file back onto the file dirty list and don't return a block.
 *
 * Results:
 *     A pointer to the first block on the dirty list (NIL if list is empty).
 *
 * Side effects:
 *     The block's state is changed from ``dirty'' to ``being written''.
 *	If the block's file is being closed this puts the file
 *	back onto the file dirty list and doens't return a block.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
GetDirtyBlockInt(cacheInfoPtr, blockPtrPtr, lastDirtyBlockPtr)
    register	FsCacheFileInfo	*cacheInfoPtr;
    FsCacheBlock		**blockPtrPtr;
    Boolean			*lastDirtyBlockPtr;
{
    register	List_Links	*dirtyPtr;
    register	FsCacheBlock	*blockPtr;

    *blockPtrPtr = (FsCacheBlock *) NIL;

    if (List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	cacheInfoPtr->flags &= ~FS_FILE_BEING_WRITTEN;
	if (cacheInfoPtr->flags & FS_CLOSE_IN_PROGRESS) {
	    /*
	     * Wake up anyone waiting for us to finish so that they can close
	     * their file.
	     */
	    Sync_Broadcast(&closeCondition);
	}
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
	return;
    } else if (cacheInfoPtr->flags & FS_CLOSE_IN_PROGRESS) {
	/*
	 * We can't do any write-backs until the file is closed.
	 * We put the file back onto the file dirty list so the
	 * block cleaner will find it again.
	 */
	cacheInfoPtr->flags &= ~FS_FILE_BEING_WRITTEN;
	Sync_Broadcast(&closeCondition);
	PutFileOnDirtyList(cacheInfoPtr);
	return;
    }

    LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
	blockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
	if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
	    UNLOCK_MONITOR;
	    Sys_Panic(SYS_FATAL, "GetDirtyBlock, bad block\n");
	    LOCK_MONITOR;
	    continue;
	}
	if (blockPtr->refCount > 0) {
	    /*
	     * Being actively used.  Wait until it is not in use anymore in
	     * case the user is writing it for example.
	     */
	    blockPtr->flags |= FS_BLOCK_CLEANER_WAITING;
	    continue;
	}
	List_Remove(dirtyPtr);
	/*
	 * Mark the block as being written out and clear the dirty flag in case
	 * someone modifies it while we are writing it out.
	 */
	blockPtr->flags &= ~FS_BLOCK_DIRTY;
	blockPtr->flags |= FS_BLOCK_BEING_WRITTEN;
	*blockPtrPtr = blockPtr;
	*lastDirtyBlockPtr = (cacheInfoPtr->numDirtyBlocks == 1);

	return;
    }

    FS_CACHE_DEBUG_PRINT("GetDirtyBlockInt: All blocks unusable\n");
    cacheInfoPtr->flags &= ~FS_FILE_BEING_WRITTEN;
    PutFileOnDirtyList(cacheInfoPtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * ProcessCleanBlock --
 *
 *     	This routine will process the newly cleaned block.
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
ProcessCleanBlock(cacheInfoPtr, blockPtr, status) 
    register	FsCacheFileInfo	*cacheInfoPtr;
    register	FsCacheBlock	*blockPtr;
    ReturnStatus		status;
{
    LOCK_MONITOR;

    Sync_Broadcast(&blockPtr->ioDone);

    blockPtr->flags &= ~(FS_BLOCK_BEING_WRITTEN | FS_BLOCK_ON_DIRTY_LIST);

    /*
     * Determine if someone is waiting for the block to be written back.  If
     * so and all of the blocks that are being waited for have been written
     * back (or we at least tried but had a timeout or no disk space)
     * wake them up.
     */
    if (blockPtr->flags & FS_WRITE_BACK_WAIT) {
	numWriteBackBlocks--;
	blockPtr->flags &= ~FS_WRITE_BACK_WAIT;
	if (numWriteBackBlocks == 0) {
	    Sync_Broadcast(&writeBackComplete);
	}
    }

    if (status != SUCCESS) {
	/*
	 * This file could not be written out.
	 */
	register	List_Links	*dirtyPtr;
	register	FsCacheBlock	*newBlockPtr;
	Boolean		printErrorMsg;
	/*
	 * Go down the list of blocks for the file and wake up anyone waiting 
	 * for the blocks to be written out because we aren't going be writing
	 * them anytime soon.
	 */
	LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
	    newBlockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
	    if (newBlockPtr->flags & FS_WRITE_BACK_WAIT) {
		numWriteBackBlocks--;
		newBlockPtr->flags &= ~FS_WRITE_BACK_WAIT;
		if (numWriteBackBlocks == 0) {
		    Sync_Broadcast(&writeBackComplete);
		}
	    }
	}

	printErrorMsg = FALSE;
	switch (status) {
	    case RPC_TIMEOUT:
	    case FS_STALE_HANDLE:
	    case RPC_SERVICE_DISABLED:	
		if (!(cacheInfoPtr->flags & FS_CACHE_SERVER_DOWN)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FS_CACHE_SERVER_DOWN;
		}
		/*
		 * Note, this used to be a non-blocking call to
		 * wait for the I/O server.
		 */
		(void) FsWantRecovery(cacheInfoPtr->hdrPtr);
		break;
	    case FS_NO_DISK_SPACE:
		if (!(cacheInfoPtr->flags & FS_CACHE_NO_DISK_SPACE)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FS_CACHE_NO_DISK_SPACE;
		}
		break;
	    case FS_DOMAIN_UNAVAILABLE:
		if (!(cacheInfoPtr->flags & FS_CACHE_DOMAIN_DOWN)) {
		    printErrorMsg = TRUE;
		    cacheInfoPtr->flags |= FS_CACHE_DOMAIN_DOWN;
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
		blockPtr->flags |= FS_BLOCK_BEING_WRITTEN;
		Proc_CallFunc(ReallocBlock, (ClientData)blockPtr, 0);
		printErrorMsg = TRUE;
		Sys_Printf("File blk %d phys blk %d: ",
			    blockPtr->blockNum, blockPtr->diskBlock);
		cacheInfoPtr->flags |= FS_CACHE_GENERIC_ERROR;
		break;
	    default: 
		printErrorMsg = TRUE;
		cacheInfoPtr->flags |= FS_CACHE_GENERIC_ERROR;
		break;
	}
	if (printErrorMsg) {
	    FsFileError(cacheInfoPtr->hdrPtr, "Write-back failed", status);
	}
	cacheInfoPtr->lastTimeTried = fsTimeInSeconds;
	PutBlockOnDirtyList(blockPtr, TRUE);
	cacheInfoPtr->flags &= ~FS_FILE_BEING_WRITTEN;
	PutFileOnDirtyList(cacheInfoPtr);
	if (cacheInfoPtr->flags & FS_CLOSE_IN_PROGRESS) {
	    /*
	     * Wake up anyone waiting for us to finish so that they can close
	     * their file.
	     */
	    Sync_Broadcast(&closeCondition);
	}
	/*
	 * Wakeup up anyone waiting for this file to be written out.
	 */
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);

	UNLOCK_MONITOR;
	return;
    }

    cacheInfoPtr->flags &= 
			~(FS_CACHE_SERVER_DOWN | FS_CACHE_NO_DISK_SPACE | 
			  FS_CACHE_GENERIC_ERROR);

    /* 
     * Now see if we are supposed to take any spaecial action with this
     * block once we are done.
     */
    if (blockPtr->flags & FS_BLOCK_DELETED) {
	PutOnFreeList(blockPtr);
    } else if (blockPtr->flags & FS_MOVE_TO_FRONT) {
	List_Move((List_Links *) blockPtr, LIST_ATFRONT(lruList));
	blockPtr->flags &= ~FS_MOVE_TO_FRONT;
    }

    cacheInfoPtr->numDirtyBlocks--;
    /*
     * Wakeup the block allocator which may be waiting for us to clean a block
     */
    Sync_Broadcast(&cleanBlockCondition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * ReallocBlock --
 *
 *	Allocate new space for the given cache block.  Called asynchronously
 *	when a write to disk failed because of a disk error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ReallocBlock(data, callInfoPtr)
    ClientData		data;			/* Block to move */
    Proc_CallInfo	*callInfoPtr;		/* Not used. */
{
    FsCacheBlock	*blockPtr;
    int			newDiskBlock;

    blockPtr = (FsCacheBlock *)data;
    newDiskBlock = FsBlockRealloc(blockPtr->cacheInfoPtr->hdrPtr,
				  blockPtr->blockNum, blockPtr->diskBlock);
    FinishRealloc(blockPtr, newDiskBlock);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FinishRealloc --
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
ENTRY static void
FinishRealloc(blockPtr, diskBlock)
    FsCacheBlock	*blockPtr;
    int			diskBlock;
{
    LOCK_MONITOR;

    blockPtr->refCount--;
    blockPtr->flags &= ~FS_BLOCK_BEING_WRITTEN;
    Sync_Broadcast(&blockPtr->ioDone);
    if (diskBlock != -1) {
	blockPtr->diskBlock = diskBlock;
	blockPtr->cacheInfoPtr->flags &= 
			    ~(FS_CACHE_NO_DISK_SPACE | FS_CACHE_DOMAIN_DOWN |
			      FS_CACHE_GENERIC_ERROR);
	PutFileOnDirtyList(blockPtr->cacheInfoPtr);
	StartBlockCleaner(blockPtr->cacheInfoPtr);
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsPreventWriteBacks --
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
 *	FS_CLOSE_IN_PROGRESS flags set in the cacheInfo for this file.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY int
FsPreventWriteBacks(cacheInfoPtr)
    FsCacheFileInfo *cacheInfoPtr;
{
    int	numDirtyBlocks;

    LOCK_MONITOR;

    cacheInfoPtr->flags |= FS_CLOSE_IN_PROGRESS;
    while (cacheInfoPtr->flags & FS_FILE_BEING_WRITTEN) {
	(void)Sync_Wait(&closeCondition, FALSE);
    }
    if (cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) {
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
 * FsAllowWriteBacks --
 *
 *	The close that was in progress on this file is now done.  We
 *	can continue to write back blocks now.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	FS_CLOSE_IN_PROGRESS flag cleared from the handle for this file.
 *	Also if the block cleaner is waiting for us then wake it up.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FsAllowWriteBacks(cacheInfoPtr)
    register	FsCacheFileInfo *cacheInfoPtr;
{
    LOCK_MONITOR;

    cacheInfoPtr->flags &= ~FS_CLOSE_IN_PROGRESS;
    if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	StartBlockCleaner(cacheInfoPtr);
    }

    UNLOCK_MONITOR;
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
 * FsAllInCache --
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
FsAllInCache(cacheInfoPtr)
    register	FsCacheFileInfo *cacheInfoPtr;
{
    Boolean	result;
    int		numBlocks;

    LOCK_MONITOR;

    fsStats.blockCache.allInCacheCalls++;
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
	    fsStats.blockCache.allInCacheTrue++;
	}
    }

    UNLOCK_MONITOR;

    return(result);
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
    register	FsCacheBlock	*blockPtr;
    register	Hash_Entry	*hashEntryPtr;

    /*
     * See if block is in the hash table.
     */
    blockHashKeyPtr->blockNumber = blockNum;
again:
    hashEntryPtr = Hash_LookOnly(blockHashTable, (Address)blockHashKeyPtr);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	FS_TRACE(FS_TRACE_NO_BLOCK);
	return((Hash_Entry *) NIL);
    }

    blockPtr = (FsCacheBlock *) Hash_GetValue(hashEntryPtr);
    /*
     * Wait until the block is unlocked.  Once wake up start over because
     * the block could have been freed while we were asleep.
     */
    if (blockPtr->refCount > 0 || 
	(blockPtr->flags & FS_BLOCK_BEING_WRITTEN)) {
	FS_TRACE_BLOCK(FS_TRACE_BLOCK_WAIT, blockPtr);
	(void) Sync_Wait(&blockPtr->ioDone, FALSE);
	if (sys_ShuttingDown) {
	    return((Hash_Entry *) NIL);
	}
	goto again;
    }
    FS_TRACE_BLOCK(FS_TRACE_BLOCK_HIT, blockPtr);
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
INTERNAL static void
DeleteBlock(blockPtr)
    register	FsCacheBlock	*blockPtr;
{
    BlockHashKey	blockHashKey;
    register Hash_Entry	*hashEntryPtr;

    SET_BLOCK_HASH_KEY(blockHashKey, blockPtr->cacheInfoPtr,
				     blockPtr->blockNum);
    hashEntryPtr = Hash_LookOnly(blockHashTable, (Address) &blockHashKey);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	Sys_Panic(SYS_FATAL,
	    "DeleteBlock: Block in LRU list is not in the hash table.\n");
    }
    FS_TRACE_BLOCK(FS_TRACE_DEL_BLOCK, blockPtr);
    Hash_Delete(blockHashTable, hashEntryPtr);
    blockPtr->cacheInfoPtr->blocksInCache--;
    List_Remove(&blockPtr->fileLinks);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_DumpCacheStats --
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
Fs_DumpCacheStats()
{
    register FsBlockCacheStats *block;

    block = &fsStats.blockCache;

    Sys_Printf("\n");
    Sys_Printf("READ  %d dirty hits %d clean hits %d zero fill %d\n",
		block->readAccesses,
		block->readHitsOnDirtyBlock,
		block->readHitsOnCleanBlock,
		block->readZeroFills);
    Sys_Printf("WRITE %d p-hits %d p-misses %d thru %d zero %d/%d app %d over %d\n",
		block->writeAccesses,
		block->partialWriteHits,
		block->partialWriteMisses,
		block->blocksWrittenThru,
		block->writeZeroFills1, block->writeZeroFills2,
		block->appendWrites,
		block->overWrites);
    Sys_Printf("FRAG upgrades %d hits %d zero fills\n",
		block->fragAccesses,
		block->fragHits,
		block->fragZeroFills);
    Sys_Printf("FILE DESC reads %d hits %d writes %d hits %d\n", 
		block->fileDescReads, block->fileDescReadHits,
		block->fileDescWrites, block->fileDescWriteHits);
    Sys_Printf("INDIRECT BLOCKS Accesses %d hits %d\n", 
		block->indBlockAccesses, block->indBlockHits);
    Sys_Printf("VM requests %d/%d/%d, gave up %d\n",
		block->vmRequests, block->triedToGiveToVM, block->vmGotPage,
		block->gavePageToVM);
    Sys_Printf("BLOCK free %d new %d lru %d part free %d\n",
		block->totFree, block->unmapped,
		block->lru, block->partFree);
    Sys_Printf("SIZES Blocks min %d num %d max %d, Blocks max %d free %d pitched %d\n",
		block->minCacheBlocks, block->numCacheBlocks,
		block->maxCacheBlocks, block->maxNumBlocks,
		block->numFreeBlocks, block->blocksPitched);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_CheckFragmentation --
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
Fs_CheckFragmentation(numBlocksPtr, totalBytesWastedPtr, fragBytesWastedPtr)
    int	*numBlocksPtr;		/* Return number of blocks in the cache. */
    int	*totalBytesWastedPtr;	/* Return the total number of bytes wasted in
				 * the cache. */
    int	*fragBytesWastedPtr;	/* Return the number of bytes wasted when cache
				 * is caches 1024 byte fragments. */
{
    register FsCacheBlock 	*blockPtr;
    register List_Links	  	*listPtr;
    register int		numBlocks = 0;
    register int		totalBytesWasted = 0;
    register int		fragBytesWasted = 0;
    register int		bytesInBlock;
    int				numFrags;

    LOCK_MONITOR;

    listPtr = lruList;
    LIST_FORALL(listPtr, (List_Links *) blockPtr) {
	if (blockPtr->refCount > 0) {
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
	    if (blockPtr->blockNum < FS_NUM_DIRECT_BLOCKS) {
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
