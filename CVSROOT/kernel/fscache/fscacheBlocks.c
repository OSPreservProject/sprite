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
#include	"fsutil.h"
#include	"fscache.h"
#include	"fsBlockCache.h"
#include	"fsStat.h"
#include	"fsNameOps.h"
#include	"fsdm.h"
#include	"fsio.h"
#include	"fsutilTrace.h"
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
 *     4) Waiting for blocks from the whole cache to be written back.  This
 *	  is done just like (3) except that a global count of the number
 *	  of blocks being written back is kept and the FSCACHE_WRITE_BACK_WAIT
 *	  flag is set in the block instead.
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
 * Special trace for indirect block bug
 */
#undef FSUTIL_TRACE_BLOCK
#define FSUTIL_TRACE_BLOCK(event, blockPtr) \
    if (((blockPtr)->flags & FSCACHE_IND_BLOCK) && \
	(event != BLOCK_FETCH_HIT)) { \
	Fsutil_TraceBlockRec blockRec;					\
	blockRec.fileID = (blockPtr)->cacheInfoPtr->hdrPtr->fileID;	\
	blockRec.blockNum = (blockPtr)->blockNum;			\
	blockRec.flags	= (blockPtr)->flags;				\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)&blockRec);\
    }

/*
 * FETCH_INIT	Block newly initialized by Fscache_FetchBlock
 * FETCH_HIT	Block found by Fscache_FetchBlock or GetUnlockedBlock
 * DELETE	Block being removed, usually from FetchBlock
 * FETCH_WAIT	Fscache_FetchBlock had to wait for the block
 * INVALIDATE	CacheFileInvalidate removed the block
 * WRITE_INVALIDATE CacheFileWriteBack removed the block
 * WRITE	Block cleaner wrote the block
 * ITER_WAIT	GetUnlockedBlock had to wait for the block
 * ITER_WAIT_HIT	GetUnlockedBlock got the block after waiting for it.
 * DIRTY	Block becomming dirty for the first time
 * DELETE_UNLOCK	UnlockBlock discarding the block via CacheFileInvalidate
 * DELETE_TO_FRONT	UnlockBlock moving block to front of LRU list
 * DELETE_TO_REAR	UnlockBlock moving block to rear of LRU list
 */
#define BLOCK_FETCH_INIT	1
#define BLOCK_FETCH_HIT		2
#define BLOCK_DELETE		3
#define BLOCK_FETCH_WAIT	4
#define BLOCK_INVALIDATE	5
#define BLOCK_WRITE_INVALIDATE	6
#define BLOCK_WRITE		7
#define BLOCK_ITER_WAIT		8
#define BLOCK_ITER_WAIT_HIT	9
#define BLOCK_DIRTY		10
#define BLOCK_DELETE_UNLOCK	11
#define BLOCK_TO_FRONT		12
#define BLOCK_TO_REAR		13
#define BLOCK_TO_DIRTY_LIST	14

/*
 * Monitor lock.
 */
static Sync_Lock	cacheLock = Sync_LockInitStatic("Fs:blockCacheLock");
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
static	int	numWriteBackBlocks = 0;	/* The number of blocks that are being 
					 * forced back to disk by 
					 * Fscache_WriteBack. */
static	int	blocksPerPage;		/* Number of blocks in a page. */

static	int	numBlockCleaners;	/* Number of block cleaner processes
					 * currently in action. */
int	fscache_MaxBlockCleaners = FSCACHE_MAX_CLEANER_PROCS;

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
 * This lets you pass a variable number of arguments through to printf:
 *	DEBUG_PRINT( ("foo %d\n", 17) );
 */
#define DEBUG_PRINT( format ) \
    if (cacheDebug) {\
	printf format ; \
    }
#else
#define	DEBUG_PRINT(format)
#endif not CLEAN

static	Boolean	traceDirtyBlocks = FALSE;
static	Boolean	cacheDebug = FALSE;

/*
 * Internal functions.
 */
static void		PutOnFreeList();
static void		PutFileOnDirtyList();
static void		PutBlockOnDirtyList();
static Boolean		CreateBlock();
static Boolean		DestroyBlock();
static Fscache_Block	*FetchBlock();
static void		CacheWriteBack();
static void		StartBlockCleaner();
static void		ProcessCleanBlock();
static void		CacheFileInvalidate();
static void		GetDirtyFile();
static void		GetDirtyBlock();
static void		GetDirtyBlockInt();
static void		ReallocBlock();
static void		FinishRealloc();
static Hash_Entry	*GetUnlockedBlock();
static void		DeleteBlock();


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_InfoInit --
 *
 * 	Initialize the cache information for a file.  Called when setting
 *	up a handle for a file that uses the block cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the fields of the Fscache_FileInfo struct.
 *
 * ----------------------------------------------------------------------------
 */
void
Fscache_InfoInit(cacheInfoPtr, hdrPtr, version, cacheable, attrPtr, ioProcsPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* Information to initialize. */
    Fs_HandleHeader	     *hdrPtr;		/* Back pointer to handle */
    int			     version;		/* Used to check consistency */
    Boolean		     cacheable;		/* TRUE if server says we can
						 * cache */
    Fscache_Attributes	     *attrPtr;		/* File attributes */
    Fscache_IOProcs	     *ioProcsPtr;	/* IO routines. */
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
    cacheInfoPtr->attr = *attrPtr;
    cacheInfoPtr->ioProcsPtr = ioProcsPtr;
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
void
Fscache_InfoSyncLockCleanup(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    Sync_LockClear(&cacheInfoPtr->lock);
}


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
    List_Init(dirtyList);
    List_Init(unmappedList);
    List_Init(fscacheFullWaitList);

    for (i = 0,blockAddr = blockCacheStart,blockPtr = (Fscache_Block *)listStart;
	 i < fs_Stats.blockCache.maxNumBlocks; 
	 i++, blockPtr++, blockAddr += FS_BLOCK_SIZE) {
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	blockPtr->blockAddr = blockAddr;
	blockPtr->refCount = 0;
	List_Insert((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
    }

    /*
     * Give enough blocks memory so that the minimum cache size requirement
     * is met.
     */
    fs_Stats.blockCache.numCacheBlocks = 0;
    while (fs_Stats.blockCache.numCacheBlocks < 
					fs_Stats.blockCache.minCacheBlocks) {
	if (!CreateBlock(FALSE, (Fscache_Block **) NIL)) {
	    printf("Fscache_Init: Couldn't create block\n");
	    fs_Stats.blockCache.minCacheBlocks = 
					fs_Stats.blockCache.numCacheBlocks;
	}
    }
    printf("FS Cache has %d %d-Kbyte blocks (%d max)\n",
	    fs_Stats.blockCache.minCacheBlocks, FS_BLOCK_SIZE / 1024,
	    fs_Stats.blockCache.maxNumBlocks);

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
	    List_Insert((List_Links *) blockPtr, LIST_ATFRONT(totFreeList));
	    List_Move((List_Links *) otherBlockPtr, LIST_ATFRONT(totFreeList));
	} else {
	    List_Insert((List_Links *) blockPtr, LIST_ATFRONT(partFreeList));
	}
    } else {
	List_Insert((List_Links *) blockPtr, LIST_ATFRONT(totFreeList));
    }
    if (! List_IsEmpty(fscacheFullWaitList)) {
	Fsutil_WaitListNotify(fscacheFullWaitList);
    }
}	


/*
 * ----------------------------------------------------------------------------
 *
 * PutFileOnDirtyList --
 *
 * 	Put the given file onto the global dirty list.  This is suppressed
 *	if the file is being deleted.  In this case a concurrent delete
 *	and write-back can end up with a deleted file back on the
 *	dirty list if we are not careful.
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
    register	Fscache_FileInfo	*cacheInfoPtr;	/* Cache info for a file */
{
    if ((cacheInfoPtr->flags & FSCACHE_FILE_GONE) ||
        (cacheInfoPtr->flags & FSCACHE_FILE_BEING_WRITTEN)) {
	/*
	 * Don't put a file on the dirty list if it has been deleted or
	 * it's already being written.
	 */
	return;
    }
    if (!(cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST)) {
	List_Insert((List_Links *)cacheInfoPtr, LIST_ATREAR(dirtyList));
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
 *	The block goes onto the dirty list of the file and the
 *	block cleaner is kicked.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutBlockOnDirtyList(blockPtr, shutdown)
    register	Fscache_Block	*blockPtr;	/* Block to put on list. */
    Boolean			shutdown;	/* TRUE => are shutting
						   down the system so the
						   calling process is going
						   to synchronously sync
						   the cache. */
{
    register Fscache_FileInfo *cacheInfoPtr = blockPtr->cacheInfoPtr;

    blockPtr->flags |= FSCACHE_BLOCK_ON_DIRTY_LIST;
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
    Fscache_Block	**blockPtrPtr;	/* Where to return pointer to block.
					 * NIL if caller isn't interested. */
{
    register	Fscache_Block	*blockPtr;
    int				newCachePages;

    if (List_IsEmpty(unmappedList)) {
	printf( "CreateBlock: No unmapped blocks\n");
	return(FALSE);
    }
    blockPtr = (Fscache_Block *) List_First(unmappedList);
    /*
     * Put memory behind the first available unmapped cache block.
     */
    newCachePages = Vm_MapBlock(blockPtr->blockAddr);
    if (newCachePages == 0) {
	return(FALSE);
    }
    fs_Stats.blockCache.numCacheBlocks += newCachePages * blocksPerPage;
    /*
     * If we are told to return a block then take it off of the list of
     * unmapped blocks and let the caller put it onto the appropriate list.
     * Otherwise put it onto the free list.
     */
    if (retBlock) {
	List_Remove((List_Links *) blockPtr);
	*blockPtrPtr = blockPtr;
    } else {
	fs_Stats.blockCache.numFreeBlocks++;
	blockPtr->flags = FSCACHE_BLOCK_FREE;
	List_Move((List_Links *) blockPtr, LIST_ATREAR(totFreeList));
    }
    if (PAGE_IS_8K) {
	/*
	 * Put the other block in the page onto the appropriate free list.
	 */
	blockPtr = GET_OTHER_BLOCK(blockPtr);
	blockPtr->flags = FSCACHE_BLOCK_FREE;
	fs_Stats.blockCache.numFreeBlocks++;
	if (retBlock) {
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(partFreeList));
	} else {
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(totFreeList));
	}
    }
    if (! List_IsEmpty(fscacheFullWaitList)) {
	Fsutil_WaitListNotify(fscacheFullWaitList);
    }

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

    /*
     * First try the list of totally free pages.
     */
    if (!List_IsEmpty(totFreeList)) {
	DEBUG_PRINT( ("DestroyBlock: Using tot free block to lower size\n") );
	blockPtr = (Fscache_Block *) List_First(totFreeList);
	fs_Stats.blockCache.numCacheBlocks -= 
		    Vm_UnmapBlock(blockPtr->blockAddr, retOnePage,
				  (unsigned int *)pageNumPtr) * blocksPerPage;
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	List_Move((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	fs_Stats.blockCache.numFreeBlocks--;
	if (PAGE_IS_8K) {
	    /*
	     * Unmap the other block.  The block address can point to either
	     * of the two blocks.
	     */
	    blockPtr = GET_OTHER_BLOCK(blockPtr);
	    blockPtr->flags = FSCACHE_NOT_MAPPED;
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	    fs_Stats.blockCache.numFreeBlocks--;
	}
	return(TRUE);
    }

    /*
     * Now take blocks from the LRU list until we get one that we can use.
     */
    while (TRUE) {
	blockPtr = FetchBlock(FALSE);
	if (blockPtr == (Fscache_Block *) NIL) {
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
		(otherBlockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) ||
		(otherBlockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
		DEBUG_PRINT( ("DestroyBlock: Other block in use.\n") );
		PutOnFreeList(blockPtr);
		continue;
	    }
	    /*
	     * The other block is cached but not in use.  Delete it.
	     */
	    if (!(otherBlockPtr->flags & FSCACHE_BLOCK_FREE)) {
		DeleteBlock(otherBlockPtr);
	    }
	    otherBlockPtr->flags = FSCACHE_NOT_MAPPED;
	    List_Move((List_Links *) otherBlockPtr, LIST_ATREAR(unmappedList));
	}
	DEBUG_PRINT( ("DestroyBlock: Using in-use block to lower size\n") );
	blockPtr->flags = FSCACHE_NOT_MAPPED;
	List_Insert((List_Links *) blockPtr, LIST_ATREAR(unmappedList));
	fs_Stats.blockCache.numCacheBlocks -= 
		    Vm_UnmapBlock(blockPtr->blockAddr, 
				retOnePage, (unsigned int *)pageNumPtr)
		    * blocksPerPage;
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
static INTERNAL Fscache_Block *
FetchBlock(canWait)
    Boolean	canWait;	/* TRUE implies can sleep if all of memory is 
				 * dirty. */
{
    register	Fscache_Block	*blockPtr;

    if (List_IsEmpty(lruList)) {
	printf("FetchBlock: LRU list is empty\n");
	return((Fscache_Block *) NIL);
    }

    /* 
     * Scan list for unlocked, clean block.
     */
    LIST_FORALL(lruList, (List_Links *) blockPtr) {
	if (blockPtr->refCount > 0) {
	    /*
	     * Block is locked.
	     */
	} else if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Block is being cleaned.  Mark it so that it will be freed
	     * after it has been cleaned.
	     */
	    blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	} else if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
	    /*
	     * Put a pointer to the block in the dirty list.  
	     * After it is cleaned it will be freed.
	     */
	    FSUTIL_TRACE_BLOCK(BLOCK_TO_DIRTY_LIST, blockPtr);
	    PutBlockOnDirtyList(blockPtr, FALSE);
	    blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	} else if (blockPtr->flags & FSCACHE_BLOCK_DELETED) {
	    printf( "FetchBlock: deleted block %d of file %d in LRU list\n",
		blockPtr->blockNum, blockPtr->fileNum);
	} else if (blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN) {
	    printf( "FetchBlock: block %d of file %d caught being written\n",
		blockPtr->blockNum, blockPtr->fileNum);
	} else {
	    /*
	     * This block is clean and unlocked.  Delete it from the
	     * hash table and use it.
	     */
	    fs_Stats.blockCache.lru++;
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
    return((Fscache_Block *) NIL);
}


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


    DEBUG_PRINT( ("Setting minimum size to %d with current size of %d\n",
		       minBlocks, fs_Stats.blockCache.minCacheBlocks) );

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

#ifndef CLEAN
    if (cacheDebug && giveUp) {
	printf("Fscache_SetMaxSize: Could only lower cache to %d\n", 
					fs_Stats.blockCache.numCacheBlocks);
    }
#endif not CLEAN
    
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
	blockPtr = (Fscache_Block *) List_First(lruList);
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
    register Boolean		dontBlock = (flags & FSCACHE_DONT_BLOCK);

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
	    *foundPtr = TRUE;
	    if (((flags & FSCACHE_IO_IN_PROGRESS) && 
		 blockPtr->refCount > 0) ||
		(blockPtr->flags & FSCACHE_IO_IN_PROGRESS)) {
		if (! dontBlock) {
		    /*
		     * Wait until it becomes unlocked, or return
		     * found = TRUE and block = NIL if caller can't block.
		     */
		    FSUTIL_TRACE_BLOCK(BLOCK_FETCH_WAIT, blockPtr);
		    (void)Sync_Wait(&blockPtr->ioDone, FALSE);
		}
		blockPtr = (Fscache_Block *)NIL;
	    } else {
		blockPtr->refCount++;
		if (flags & FSCACHE_IO_IN_PROGRESS) {
		    blockPtr->flags |= FSCACHE_IO_IN_PROGRESS;
		}
		FSUTIL_TRACE_BLOCK(BLOCK_FETCH_HIT, blockPtr);
	    }
	} else {
	    /*
	     * Have to allocate a block.  If there is a free block use it, or
	     * take a block off of the lru list, or make a new one.
	     */
	    *foundPtr = FALSE;
	    if (!List_IsEmpty(partFreeList)) {
		/*
		 * Use partially free blocks first.
		 */
		fs_Stats.blockCache.numFreeBlocks--;
		fs_Stats.blockCache.partFree++;
		blockPtr = (Fscache_Block *) List_First(partFreeList);
		List_Remove((List_Links *) blockPtr);
	    } else if (!List_IsEmpty(totFreeList)) {
		/*
		 * Can't find a partially free block so use a totally free
		 * block.
		 */
		fs_Stats.blockCache.numFreeBlocks--;
		fs_Stats.blockCache.totFree++;
		blockPtr = (Fscache_Block *) List_First(totFreeList);
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
		if (fs_Stats.blockCache.numCacheBlocks >= 
					    fs_Stats.blockCache.maxCacheBlocks) {
		    /*
		     * We can't have anymore blocks so reuse one of our own.
		     */
		    blockPtr = FetchBlock(!dontBlock);
		} else {
		    /*
		     * Grow the cache if VM has an older page than we have.
		     */
		    refTime = Vm_GetRefTime();
		    blockPtr = (Fscache_Block *) List_First(lruList);
		    DEBUG_PRINT( ("FsCacheBlockFetch: fs=%d vm=%d\n", 
				       blockPtr->timeReferenced, refTime) );
		    if (blockPtr->timeReferenced > refTime) {
			DEBUG_PRINT( ("FsCacheBlockFetch:Creating new block\n" ));
			if (!CreateBlock(TRUE, &newBlockPtr)) {
			    DEBUG_PRINT( ("FsCacheBlockFetch: Couldn't create block\n" ));
			    blockPtr = FetchBlock(!dontBlock);
			} else {
			    fs_Stats.blockCache.unmapped++;
			    blockPtr = newBlockPtr;
			}
		    } else {
			/*
			 * We have an older block than VM's oldest page so reuse
			 * the block.
			 */
			DEBUG_PRINT( ("FsCacheBlockFetch: Recycling block\n") );
			blockPtr = FetchBlock(!dontBlock);
		    }
		}
	    }
	    /*
	     * If blockPtr is NIL we waited for room in the cache or
	     * for a busy cache block.  Now we'll retry all the various
	     * ploys to get a free block. There used to be a bug
	     * where the first hash didn't find the block, FetchBlock
	     * waited for room in the cache, and the block reappeared
	     * in the cache but FetchBlock was called to get a new block
	     * anyway - the hash was not redone so a block could have
	     * been put into the hash table twice.
	     */
	}
    } while ((blockPtr == (Fscache_Block *)NIL) && !dontBlock);

    if ((*foundPtr == FALSE) && (blockPtr != (Fscache_Block *)NIL)) {
	cacheInfoPtr->blocksInCache++;
	blockPtr->cacheInfoPtr = cacheInfoPtr;
	blockPtr->refCount = 1;
	blockPtr->flags = flags & (FSCACHE_DATA_BLOCK | FSCACHE_IND_BLOCK |
				   FSCACHE_DESC_BLOCK | FSCACHE_DIR_BLOCK |
				   FSCACHE_READ_AHEAD_BLOCK);
	blockPtr->flags |= FSCACHE_IO_IN_PROGRESS;
	blockPtr->fileNum = cacheInfoPtr->hdrPtr->fileID.minor;
	blockPtr->blockNum = blockNum;
	blockPtr->blockSize = -1;
	blockPtr->timeDirtied = 0;
	blockPtr->timeReferenced = fsutil_TimeInSeconds;
	*blockPtrPtr = blockPtr;
	FSUTIL_TRACE_BLOCK(BLOCK_FETCH_INIT, blockPtr);
	if (Hash_GetValue(hashEntryPtr) != (char *)NIL) {
	    lostBlockPtr = (Fscache_Block *)Hash_GetValue(hashEntryPtr);
	    UNLOCK_MONITOR;
	    panic("Fscache_FetchBlock: hashEntryPtr->value changed\n");
	    LOCK_MONITOR;
	}
	Hash_SetValue(hashEntryPtr, blockPtr);
	List_Insert((List_Links *) blockPtr, LIST_ATREAR(lruList));
	List_InitElement(&blockPtr->fileLinks);
	if (flags & FSCACHE_IND_BLOCK) {
	    List_Insert(&blockPtr->fileLinks, LIST_ATREAR(&cacheInfoPtr->indList));
	} else {
	    List_Insert(&blockPtr->fileLinks,LIST_ATREAR(&cacheInfoPtr->blockList));
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
 * Fscache_MoveBlock --
 *
 *	Change the disk location of a block.  This has to synchronize
 *	with delayed writes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The diskBlock field of the cache block is modified.  The block is
 *	not marked dirty, however.  That will be done by UnlockBlock.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_MoveBlock(blockPtr, diskBlock, blockSize)
    Fscache_Block *blockPtr;	/* Pointer to block information for block.*/
    int diskBlock;		/* New location for the block */
    int blockSize;		/* New size for the block */
{
    LOCK_MONITOR;

    if (diskBlock != -1) {
	if (blockPtr->diskBlock != diskBlock ||
	    blockPtr->blockSize != blockSize) {
	    while (blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN) {
		(void)Sync_Wait(&blockPtr->ioDone, FALSE);
	    }
	    blockPtr->diskBlock = diskBlock;
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
				 * FSCACHE_BLOCK_UNNEEDED | FSCACHE_DONT_WRITE_THRU */
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
	FSUTIL_TRACE_BLOCK(BLOCK_DELETE_UNLOCK, blockPtr);
	CacheFileInvalidate(blockPtr->cacheInfoPtr, blockPtr->blockNum, 
			    blockPtr->blockNum);
	UNLOCK_MONITOR;
	return;
    }

    if (flags & FSCACHE_CLEAR_READ_AHEAD) {
	blockPtr->flags &= ~FSCACHE_READ_AHEAD_BLOCK;
    }

    if (timeDirtied != 0) {
	if (!(blockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
	    /*
	     * Increment the count of dirty blocks if the block isn't marked
	     * as dirty.  The block cleaner will decrement the count 
	     * after it cleans a block.
	     */
	    blockPtr->cacheInfoPtr->numDirtyBlocks++;
	    if (traceDirtyBlocks) {
		printf("UNL FD=%d Num=%d\n",
			   blockPtr->cacheInfoPtr->hdrPtr->fileID.minor,
			   blockPtr->cacheInfoPtr->numDirtyBlocks);
	    }
	    blockPtr->flags |= FSCACHE_BLOCK_DIRTY;
	    blockPtr->timeDirtied = timeDirtied;
	    FSUTIL_TRACE_BLOCK(BLOCK_DIRTY, blockPtr);
	}
    }

    if (diskBlock != -1) {
	if (blockPtr->diskBlock != diskBlock ||
	    blockPtr->blockSize != blockSize) {
	    /*
	     * The caller is changing where this block lives on disk.  If
	     * so we have to wait until the block has finished being written
	     * before we can allow this to happen.
	     */
	    while (blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN) {
		(void)Sync_Wait(&blockPtr->ioDone, FALSE);
		if (timeDirtied != 0 &&
		    (blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN) == 0) {
		    /*
		     * I assert that the cache block is no longer marked
		     * dirty and may never be written out to its proper
		     * location.
		     */
		    if ((blockPtr->flags & FSCACHE_BLOCK_DIRTY) == 0) {
			printf("Fscache_UnlockBlock: file <%d,%d> block %d disk block %d => %d no longer dirty\n",
			    blockPtr->cacheInfoPtr->hdrPtr->fileID.major,
			    blockPtr->cacheInfoPtr->hdrPtr->fileID.minor,
			   blockPtr->blockNum, blockPtr->diskBlock, diskBlock);
		    }
		}
	    }
	    blockPtr->diskBlock = diskBlock;
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
	if (blockPtr->flags & FSCACHE_BLOCK_CLEANER_WAITING) {
	    StartBlockCleaner(blockPtr->cacheInfoPtr);
	    blockPtr->flags &= ~FSCACHE_BLOCK_CLEANER_WAITING;
	}
	if (flags & FSCACHE_BLOCK_UNNEEDED) {
	    /*
	     * This block is unneeded so move it to the front of the LRU list
	     * and set its time referenced to zero so that it will be taken
	     * at the next convenience.
	     */
	    if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
		blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	    } else {
		FSUTIL_TRACE_BLOCK(BLOCK_TO_FRONT, blockPtr);
		List_Move((List_Links *) blockPtr, LIST_ATFRONT(lruList));
	    }
	    fs_Stats.blockCache.blocksPitched++;
	    blockPtr->timeReferenced = 0;
	} else {
	    /*
	     * Move it to the end of the lru list, mark it as being referenced. 
	     */
	    blockPtr->timeReferenced = fsutil_TimeInSeconds;
	    blockPtr->flags &= ~FSCACHE_MOVE_TO_FRONT;
	    List_Move((List_Links *) blockPtr, LIST_ATREAR(lruList));
	}
	/*
	 * Force the block out if in write-thru or asap mode.
	 */
	if ((blockPtr->flags & (FSCACHE_BLOCK_DIRTY | FSCACHE_BLOCK_BEING_WRITTEN)) &&
	    ((fsutil_WriteThrough || fsutil_WriteBackASAP) &&
	     !(flags & FSCACHE_DONT_WRITE_THRU)) &&
	    (!fsutil_DelayTmpFiles ||
	     Fsdm_FindFileType(blockPtr->cacheInfoPtr) != FSUTIL_FILE_TYPE_TMP)) {
	    /*
	     * Set the write-thru flag for the block so that the block will
	     * keep getting written until it is clean.  This is in case
	     * a block is modified while it is being written to disk.
	     */
	    blockPtr->flags |= FSCACHE_WRITE_THRU_BLOCK;
	    if (fsutil_WriteBackASAP) {
		/*
		 * Force full blocks through.
		 */
		if (blockPtr->blockSize == FS_BLOCK_SIZE ||
		    !(blockPtr->flags & FSCACHE_DATA_BLOCK)) {
		    if (!(blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST)) {
			PutBlockOnDirtyList(blockPtr, FALSE);
		    }
		}
	    } else {
		/* 
		 * Force the block out and then wait for it.
		 */
		if (!(blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST)) {
		    PutBlockOnDirtyList(blockPtr, FALSE);
		}
		do {
		    (void) Sync_Wait(&blockPtr->ioDone, FALSE);
		    if (sys_ShuttingDown) {
			break;
		    }
		} while (blockPtr->flags & 
			    (FSCACHE_BLOCK_DIRTY | FSCACHE_BLOCK_BEING_WRITTEN));
	    }
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
	blockPtr->blockSize = newBlockSize;
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

static void	DeleteBlockFromDirtyList();


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

    FSUTIL_TRACE_IO(FSUTIL_TRACE_SRV_WRITE_1, cacheInfoPtr->hdrPtr->fileID, firstBlock, lastBlock);

    if (cacheInfoPtr->blocksInCache > 0) {
	SET_BLOCK_HASH_KEY(blockHashKey, cacheInfoPtr, 0);

	for (i = firstBlock; i <= lastBlock; i++) {
	    hashEntryPtr = GetUnlockedBlock(&blockHashKey, i);
	    if (hashEntryPtr == (Hash_Entry *) NIL) {
		continue;
	    }
	    blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);

	    /*
	     * Remove it from the hash table.
	     */
	    cacheInfoPtr->blocksInCache--;
	    List_Remove(&blockPtr->fileLinks);
	    Hash_Delete(blockHashTable, hashEntryPtr);
	    FSUTIL_TRACE_BLOCK(BLOCK_INVALIDATE, blockPtr);
    
	    /*
	     * Invalidate the block, including removing it from dirty list
	     * if necessary.
	     */
	    if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
		DeleteBlockFromDirtyList(blockPtr);
	    }
	    if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
		cacheInfoPtr->numDirtyBlocks--;
		if (traceDirtyBlocks) {
		    printf("Inv FD=%d Num=%d\n", 
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
        List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	/*
	 * No more dirty blocks for this file.  Remove the file from the dirty
	 * list and wakeup anyone waiting for the file's list to become
	 * empty.
	 */
	List_Remove((List_Links *)cacheInfoPtr);
	cacheInfoPtr->flags &= ~FSCACHE_FILE_ON_DIRTY_LIST;
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
    }

    if (blockPtr->flags & FSCACHE_WRITE_BACK_WAIT) {
	numWriteBackBlocks--;
	blockPtr->flags &= ~FSCACHE_WRITE_BACK_WAIT;
	if (numWriteBackBlocks == 0) {
	    Sync_Broadcast(&writeBackComplete);
	}
    }
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
				 * FSCACHE_WRITE_BACK_AND_INVALIDATE |
				 * FSCACHE_WB_MIGRATION. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
{
    register Hash_Entry	     *hashEntryPtr;
    register Fscache_Block    *blockPtr;
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
		    ~(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE | 
		      FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR);

    if (cacheInfoPtr->blocksInCache == 0) {
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
	    if (flags & FSCACHE_WRITE_BACK_AND_INVALIDATE) {
		cacheInfoPtr->blocksInCache--;
		List_Remove(&blockPtr->fileLinks);
		Hash_Delete(blockHashTable, hashEntryPtr);
		List_Remove((List_Links *) blockPtr);
		blockPtr->flags |= FSCACHE_BLOCK_DELETED;
		FSUTIL_TRACE_BLOCK(BLOCK_WRITE_INVALIDATE, blockPtr);
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
	if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Blocks already on the dirty list, no need to do anything.
	     */
	} else if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
	    PutBlockOnDirtyList(blockPtr, FALSE);
	    if (flags & FSCACHE_FILE_WB_WAIT) {
#ifdef wait_for_fsStat
		/*
		 * Synchronous invalidation... record statistics.
		 */
		fs_Stats.blockCache.blocksFlushed++;
		if (flags & FSCACHE_WB_MIGRATION) {
		    fs_Stats.blockCache.migBlocksFlushed++;
		}
#endif
	    }
	} else if (flags & FSCACHE_WRITE_BACK_AND_INVALIDATE) {
	    /*
	     * This block is clean.  We need to free it if it is to be
	     * invalidated.
	     */
	    PutOnFreeList(blockPtr);
	}
    }

    /*
     * If required write-back indirect blocks as well.
     */
    if (flags & FSCACHE_WRITE_BACK_INDIRECT) {
	register List_Links	*linkPtr;
	LIST_FORALL(&cacheInfoPtr->indList, linkPtr) {
	    blockPtr = FILE_LINKS_TO_BLOCK(linkPtr);
	    if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
		/*
		 * Blocks already on the dirty list, no need to do anything.
		 */
	    } else if (blockPtr->flags & FSCACHE_BLOCK_DIRTY) {
		PutBlockOnDirtyList(blockPtr, TRUE);
	    }
	}
    }

    if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	StartBlockCleaner(cacheInfoPtr);
    }

    /*
     * Wait until all blocks are written back.
     */
    if (flags & FSCACHE_FILE_WB_WAIT) {
	while (!List_IsEmpty(&cacheInfoPtr->dirtyList) && 
	       !(cacheInfoPtr->flags & 
		    (FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE |
		     FSCACHE_DOMAIN_DOWN | FSCACHE_GENERIC_ERROR)) &&
	       !sys_ShuttingDown) {
	    (void) Sync_Wait(&cacheInfoPtr->noDirtyBlocks, FALSE);
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

	if (blockPtr->refCount > 0) {
	    /*
	     * The block is locked.  This means someone is doing something with
	     * it so we just skip it.
	     */
	    continue;
	}
	if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
	    /*
	     * Block is being cleaned.  Set the flag so that it will be 
	     * moved to the front after it has been cleaned.
	     */
	    blockPtr->flags |= FSCACHE_MOVE_TO_FRONT;
	} else {
	    /*
	     * Move the block to the front of the LRU list.
	     */
	    List_Move((List_Links *) blockPtr, LIST_ATFRONT(lruList));
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
Fscache_WriteBack(writeBackTime, blocksSkippedPtr, shutdown)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	shutdown;	   /* TRUE if the system is being shut down. */
{
    LOCK_MONITOR;

    CacheWriteBack(writeBackTime, blocksSkippedPtr, shutdown, shutdown);

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
    CacheWriteBack(-1, &blocksSkipped, FALSE, TRUE);
    listPtr = lruList;
    nextPtr = List_First(listPtr);
    while (!List_IsAtEnd(listPtr, nextPtr)) {
	blockPtr = (Fscache_Block *)nextPtr;
	nextPtr = List_Next(nextPtr);
	if (blockPtr->refCount > 0 || 
	    (blockPtr->flags & (FSCACHE_BLOCK_DIRTY | FSCACHE_BLOCK_ON_DIRTY_LIST))) {
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
static INTERNAL void
CacheWriteBack(writeBackTime, blocksSkippedPtr, shutdown, writeTmpFiles)
    unsigned int writeBackTime;	   /* Write back all blocks that were 
				      dirtied before this time. */
    int		*blocksSkippedPtr; /* The number of blocks skipped
				      because they were locked. */
    Boolean	shutdown;	   /* TRUE if the system is being shut down. */
    Boolean	writeTmpFiles;	   /* TRUE => write-back tmp files even though
				    *         they are marked as not being
				    *         written back. */
{
    register Fscache_FileInfo	*cacheInfoPtr;
    register Fscache_Block 	*blockPtr;
    register List_Links	  	*listPtr;
    int				currentTime;

    currentTime = fsutil_TimeInSeconds;

    *blocksSkippedPtr = 0;

    listPtr = lruList;
    LIST_FORALL(listPtr, (List_Links *) blockPtr) {
	cacheInfoPtr = blockPtr->cacheInfoPtr;
	if (fsutil_DelayTmpFiles && !writeTmpFiles &&
	    Fsdm_FindFileType(cacheInfoPtr) == FSUTIL_FILE_TYPE_TMP) {
	    continue;
	}
	if (cacheInfoPtr->flags & FSCACHE_SERVER_DOWN) {
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
	if (blockPtr->flags & FSCACHE_WRITE_BACK_WAIT) {
	    /*
	     * Someone is already waiting on this block.  This means that
	     * numWriteBackBlocks has been incremented once already for this
	     * block so no need to increment it again.
	     */
	    continue;
	}
	if (blockPtr->flags & FSCACHE_BLOCK_ON_DIRTY_LIST) {
	    blockPtr->flags |= FSCACHE_WRITE_BACK_WAIT;
	    numWriteBackBlocks++;
	} else if ((blockPtr->flags & FSCACHE_BLOCK_DIRTY) &&
		   (blockPtr->timeDirtied < writeBackTime || shutdown)) {
	    PutBlockOnDirtyList(blockPtr, shutdown);
	    blockPtr->flags |= FSCACHE_WRITE_BACK_WAIT;
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
 * Fscache_CleanBlocks
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
Fscache_CleanBlocks(data, callInfoPtr)
    ClientData		data;		/* Background flag.  If TRUE it means
					 * we are called from a block cleaner
					 * process.  Otherwise we being called
					 * synchrounously during a shutdown */
    Proc_CallInfo	*callInfoPtr;	/* Not Used. */
{
    Boolean			backGround;
    register	Fscache_Block	*blockPtr;
    Fscache_Block		*tBlockPtr;
    ReturnStatus		status;
    int				lastDirtyBlock;
    Fscache_FileInfo		*cacheInfoPtr;
    Boolean			useSameBlock;
    int				numDirtyFiles = 0;
    int				numDirtyBlocks = 0;

    backGround = (Boolean) data;
    GetDirtyFile(backGround, &cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
    blockPtr = tBlockPtr;
    while (cacheInfoPtr != (Fscache_FileInfo *)NIL) {
	numDirtyFiles++;
	while (blockPtr != (Fscache_Block *)NIL) {
	    if (blockPtr->blockSize < 0) {
		panic( "Fscache_CleanBlocks, uninitialized block size\n");
		status = FAILURE;
		break;
	    }
	    /*
	     * Gather statistics.
	     */
	    numDirtyBlocks++;
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
		    printf( "Fscache_CleanBlocks: Unknown block type\n");
	    }

	    /*
	     * Write the block.
	     */
	    FSUTIL_TRACE_BLOCK(BLOCK_WRITE, blockPtr);
	    status = (cacheInfoPtr->ioProcsPtr->blockWrite)
		    (cacheInfoPtr->hdrPtr, blockPtr, lastDirtyBlock);
#ifdef lint
	    status = Fsio_FileBlockWrite(cacheInfoPtr->hdrPtr,
			blockPtr, lastDirtyBlock);
	    status = FsrmtFileBlockWrite(cacheInfoPtr->hdrPtr,
			blockPtr, lastDirtyBlock);
#endif /* lint */
	    ProcessCleanBlock(cacheInfoPtr, blockPtr, status,
			      &useSameBlock, &lastDirtyBlock);
	    if (status != SUCCESS) {
		break;
	    }
	    if (!useSameBlock) {
		GetDirtyBlock(cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
		blockPtr = tBlockPtr;
	    }
	}
	GetDirtyFile(backGround, &cacheInfoPtr, &tBlockPtr, &lastDirtyBlock);
	blockPtr = tBlockPtr;
    }
    fs_Stats.writeBack.passes++;
    fs_Stats.writeBack.files += numDirtyFiles;
    fs_Stats.writeBack.blocks += numDirtyBlocks;
    if (numDirtyBlocks > fs_Stats.writeBack.maxBlocks) {
	fs_Stats.writeBack.maxBlocks = numDirtyBlocks;
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
static INTERNAL void
StartBlockCleaner(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;	/* Cache info for the file. */
{
    if (!(cacheInfoPtr->flags & FSCACHE_FILE_BEING_WRITTEN) &&
	numBlockCleaners < fscache_MaxBlockCleaners) {
	Proc_CallFunc(Fscache_CleanBlocks, (ClientData) TRUE, 0);
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
    Fscache_FileInfo	**cacheInfoPtrPtr;
    Fscache_Block	**blockPtrPtr;
    int			*lastDirtyBlockPtr;
{
    register Fscache_FileInfo *cacheInfoPtr;

    LOCK_MONITOR;

    *cacheInfoPtrPtr = (Fscache_FileInfo *)NIL;

    if (List_IsEmpty(dirtyList)) {
	if (backGround) {
	    numBlockCleaners--;
	}
	UNLOCK_MONITOR;
	return;
    }

    LIST_FORALL(dirtyList, (List_Links *)cacheInfoPtr) {
	if (cacheInfoPtr->flags & FSCACHE_SERVER_DOWN) {
	    /*
	     * The host is down for this file.
	     */
	    continue;
	} else if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
	    /*
	     * Close in progress on the file the block lives in so we aren't
	     * allowed to write any more blocks.
	     */
	    continue;
	} else if (cacheInfoPtr->flags & FSCACHE_FILE_GONE) {
	    /*
	     * The file is being deleted.
	     */
	    printf("FsGetDirtyFile skipping deleted file <%d,%d> \"%s\"\n",
		cacheInfoPtr->hdrPtr->fileID.major,
		cacheInfoPtr->hdrPtr->fileID.minor,
		Fsutil_HandleName(cacheInfoPtr->hdrPtr));
	    continue;
	} else if (cacheInfoPtr->flags & 
		       (FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
		        FSCACHE_GENERIC_ERROR)) {
	    if (fsutil_TimeInSeconds - cacheInfoPtr->lastTimeTried <
			WRITE_RETRY_INTERVAL) {
		continue;
	    }
	    cacheInfoPtr->flags &= 
			    ~(FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
			      FSCACHE_GENERIC_ERROR);
	}
	List_Remove((List_Links *)cacheInfoPtr);
	cacheInfoPtr->flags |= FSCACHE_FILE_BEING_WRITTEN;
	cacheInfoPtr->flags &= ~FSCACHE_FILE_ON_DIRTY_LIST;
	GetDirtyBlockInt(cacheInfoPtr, blockPtrPtr, lastDirtyBlockPtr);
	if (*blockPtrPtr != (Fscache_Block *)NIL) {
	    *cacheInfoPtrPtr = cacheInfoPtr;
	
	    UNLOCK_MONITOR;
	    return;
	}
    }

    FSCACHE_DEBUG_PRINT("GetDirtyFile: All files unusable\n");
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
    Fscache_FileInfo	*cacheInfoPtr;
    Fscache_Block	**blockPtrPtr;
    int			*lastDirtyBlockPtr;
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
    register	Fscache_FileInfo	*cacheInfoPtr;
    Fscache_Block		**blockPtrPtr;
    int				*lastDirtyBlockPtr;
{
    register	List_Links	*dirtyPtr;
    register	Fscache_Block	*blockPtr;

    *blockPtrPtr = (Fscache_Block *) NIL;

    if (List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	cacheInfoPtr->flags &= ~FSCACHE_FILE_BEING_WRITTEN;
	if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
	    /*
	     * Wake up anyone waiting for us to finish so that they can close
	     * their file.
	     */
	    Sync_Broadcast(&closeCondition);
	}
	Sync_Broadcast(&cacheInfoPtr->noDirtyBlocks);
	return;
    } else if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
	/*
	 * We can't do any write-backs until the file is closed.
	 * We put the file back onto the file dirty list so the
	 * block cleaner will find it again.
	 */
	cacheInfoPtr->flags &= ~FSCACHE_FILE_BEING_WRITTEN;
	Sync_Broadcast(&closeCondition);
	PutFileOnDirtyList(cacheInfoPtr);
	return;
    } 

    LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
	blockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
	if (blockPtr->refCount > 0) {
	    /*
	     * Being actively used.  Wait until it is not in use anymore in
	     * case the user is writing it for example.
	     */
	    blockPtr->flags |= FSCACHE_BLOCK_CLEANER_WAITING;
	    continue;
	}
	List_Remove(dirtyPtr);
	/*
	 * Mark the block as being written out and clear the dirty flag in case
	 * someone modifies it while we are writing it out.
	 */
	blockPtr->flags &= ~FSCACHE_BLOCK_DIRTY;
	blockPtr->flags |= FSCACHE_BLOCK_BEING_WRITTEN;
	*blockPtrPtr = blockPtr;
	if (cacheInfoPtr->numDirtyBlocks == 1) {
	    *lastDirtyBlockPtr = FS_LAST_DIRTY_BLOCK;
	    if (cacheInfoPtr->flags & FSCACHE_WB_ON_LDB) {
		*lastDirtyBlockPtr |= FS_WB_ON_LDB;
		cacheInfoPtr->flags &= ~FSCACHE_WB_ON_LDB;
	    }
	} else {
	    *lastDirtyBlockPtr = 0;
	}
	/*
	 * Increment the reference count to make the block unavailable to 
	 * others.
	 */
	blockPtr->refCount++;
	return;
    }

    FSCACHE_DEBUG_PRINT("GetDirtyBlockInt: All blocks unusable\n");
    cacheInfoPtr->flags &= ~FSCACHE_FILE_BEING_WRITTEN;
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
static ENTRY void
ProcessCleanBlock(cacheInfoPtr, blockPtr, status, useSameBlockPtr,
		  lastDirtyBlockPtr) 
    register	Fscache_FileInfo	*cacheInfoPtr;
    register	Fscache_Block	*blockPtr;
    ReturnStatus		status;
    Boolean			*useSameBlockPtr;
    int				*lastDirtyBlockPtr;
{
    LOCK_MONITOR;

    if (status == SUCCESS && 
        (blockPtr->flags & FSCACHE_WRITE_THRU_BLOCK) &&
        (blockPtr->flags & FSCACHE_BLOCK_DIRTY)) {
	/*
	 * We have to keep writing this block until its gets clean, so
	 * rewrite the same block.   (Hmmm.  Does this mean that a
	 * continually modified block will prevent the rest of the file
	 * from being written out?)
	 */
	blockPtr->flags &= ~FSCACHE_BLOCK_DIRTY;
	if (cacheInfoPtr->numDirtyBlocks == 1) {
	    *lastDirtyBlockPtr = FS_LAST_DIRTY_BLOCK;
	    if (cacheInfoPtr->flags & FSCACHE_WB_ON_LDB) {
		*lastDirtyBlockPtr |= FS_WB_ON_LDB;
		cacheInfoPtr->flags &= ~FSCACHE_WB_ON_LDB;
	    }
	} else {
	    *lastDirtyBlockPtr = 0;
	}
	*useSameBlockPtr = TRUE;
	UNLOCK_MONITOR;
	return;
    }
    *useSameBlockPtr = FALSE;
    Sync_Broadcast(&blockPtr->ioDone);

    blockPtr->flags &= ~(FSCACHE_BLOCK_BEING_WRITTEN | FSCACHE_BLOCK_ON_DIRTY_LIST);
    /*
     * Decrement the reference count to make the block available to others.
     */
    blockPtr->refCount--;
    /*
     * Determine if someone is waiting for the block to be written back.  If
     * so and all of the blocks that are being waited for have been written
     * back (or we at least tried but had a timeout or no disk space)
     * wake them up.
     */
    if (blockPtr->flags & FSCACHE_WRITE_BACK_WAIT) {
	numWriteBackBlocks--;
	blockPtr->flags &= ~FSCACHE_WRITE_BACK_WAIT;
	if (numWriteBackBlocks == 0) {
	    Sync_Broadcast(&writeBackComplete);
	}
    }

    if (status != SUCCESS) {
	/*
	 * This file could not be written out.
	 */
	register	List_Links	*dirtyPtr;
	register	Fscache_Block	*newBlockPtr;
	Boolean		printErrorMsg;
	/*
	 * Go down the list of blocks for the file and wake up anyone waiting 
	 * for the blocks to be written out because we aren't going be writing
	 * them anytime soon.
	 */
	LIST_FORALL(&cacheInfoPtr->dirtyList, dirtyPtr) {
	    newBlockPtr = DIRTY_LINKS_TO_BLOCK(dirtyPtr);
	    if (newBlockPtr->flags & FSCACHE_WRITE_BACK_WAIT) {
		numWriteBackBlocks--;
		newBlockPtr->flags &= ~FSCACHE_WRITE_BACK_WAIT;
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
		blockPtr->flags |= FSCACHE_BLOCK_BEING_WRITTEN;
		Proc_CallFunc(ReallocBlock, (ClientData)blockPtr, 0);
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
	    Fsutil_FileError(cacheInfoPtr->hdrPtr, "Write-back failed", status);
	}
	cacheInfoPtr->lastTimeTried = fsutil_TimeInSeconds;
	cacheInfoPtr->flags &= ~FSCACHE_FILE_BEING_WRITTEN;
	PutBlockOnDirtyList(blockPtr, TRUE);
	if (cacheInfoPtr->flags & FSCACHE_CLOSE_IN_PROGRESS) {
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
    /*
     * Successfully wrote the block.
     */
    cacheInfoPtr->flags &= 
			~(FSCACHE_SERVER_DOWN | FSCACHE_NO_DISK_SPACE | 
			  FSCACHE_GENERIC_ERROR);
    if (! List_IsEmpty(fscacheFullWaitList)) {
	Fsutil_WaitListNotify(fscacheFullWaitList);
    }
    /* 
     * Now see if we are supposed to take any special action with this
     * block once we are done.
     */
    if (blockPtr->flags & FSCACHE_BLOCK_DELETED) {
	PutOnFreeList(blockPtr);
    } else if (blockPtr->flags & FSCACHE_MOVE_TO_FRONT) {
	List_Move((List_Links *) blockPtr, LIST_ATFRONT(lruList));
	blockPtr->flags &= ~FSCACHE_MOVE_TO_FRONT;
    }

    cacheInfoPtr->numDirtyBlocks--;
    /*
     * Wakeup the block allocator which may be waiting for us to clean a block
     */
    Sync_Broadcast(&cleanBlockCondition);
    blockPtr->flags &= ~FSCACHE_WRITE_THRU_BLOCK;

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
    Fscache_Block	*blockPtr;
    int			newDiskBlock;

    blockPtr = (Fscache_Block *)data;
    newDiskBlock = FsdmBlockRealloc(blockPtr->cacheInfoPtr->hdrPtr,
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
    Fscache_Block	*blockPtr;
    int			diskBlock;
{
    LOCK_MONITOR;

    blockPtr->refCount--;
    blockPtr->flags &= ~FSCACHE_BLOCK_BEING_WRITTEN;
    Sync_Broadcast(&blockPtr->ioDone);
    if (diskBlock != -1) {
	blockPtr->diskBlock = diskBlock;
	blockPtr->cacheInfoPtr->flags &= 
			    ~(FSCACHE_NO_DISK_SPACE | FSCACHE_DOMAIN_DOWN |
			      FSCACHE_GENERIC_ERROR);
	PutFileOnDirtyList(blockPtr->cacheInfoPtr);
	StartBlockCleaner(blockPtr->cacheInfoPtr);
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
 * FscacheBlockOkToScavenge --
 *
 *	Decide if it is safe to scavenge the file handle.  This is
 *	called from Fscache_OkToScavenge which
 *	has already grabbed the per-file cache lock.
 *
 *	Note:  this has some extra code to check against various bugs.
 *	ideally it should only have to check against blocks in the
 *	cache and being on the dirty list.
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
	/*
	 * Verify that the block is attached to the right file.  Note that
	 * if the file has been invalidated its minor field is negated.
	 */
	if ((blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) &&
	    (blockPtr->fileNum != - cacheInfoPtr->hdrPtr->fileID.minor)) {
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
    if (numBlocks == 0 && (cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST)) {
	printf("FscacheBlockOkToScavenge dirty file with no regular blocks\n");
    }
    ok = (numBlocks == 0) &&
	((cacheInfoPtr->flags & FSCACHE_FILE_ON_DIRTY_LIST) == 0);
    if (ok) {
	if (cacheInfoPtr->flags & FSCACHE_FILE_BEING_WRITTEN) {
	    UNLOCK_MONITOR;
	    panic("Fscache_OkToScavenge: FSCACHE_FILE_BEING_WRITTEN (continuable)\n");
	    LOCK_MONITOR;
	    ok = FALSE;
	}
	/*
	 * Verify that this really isn't on the dirty list.
	 */
	linkPtr = dirtyList->nextPtr;
	while (linkPtr != (List_Links *)NIL && linkPtr != dirtyList) {
	    if (linkPtr == (List_Links *)cacheInfoPtr) {
		UNLOCK_MONITOR;
		panic("Fscache_OkToScavenge: file on dirty list (continuable)\n");
		LOCK_MONITOR;
		ok = FALSE;
	    }
	    linkPtr = linkPtr->nextPtr;
	    if (linkPtr == (List_Links *)NIL) {
		UNLOCK_MONITOR;
		panic("Fscache_OkToScavenge: NIL on dirty list (continuable)\n");
		LOCK_MONITOR;
		ok = FALSE;
	    }
	}
    }
    UNLOCK_MONITOR;
    return(ok);
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
    int event;

    /*
     * See if block is in the hash table.
     */
    blockHashKeyPtr->blockNumber = blockNum;
    event = BLOCK_FETCH_HIT;
again:
    hashEntryPtr = Hash_LookOnly(blockHashTable, (Address)blockHashKeyPtr);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	FSUTIL_TRACE(FSUTIL_TRACE_NO_BLOCK);
	return((Hash_Entry *) NIL);
    }

    blockPtr = (Fscache_Block *) Hash_GetValue(hashEntryPtr);
    /*
     * Wait until the block is unlocked.  Once wake up start over because
     * the block could have been freed while we were asleep.
     */
    if (blockPtr->refCount > 0 || 
	(blockPtr->flags & FSCACHE_BLOCK_BEING_WRITTEN)) {
	FSUTIL_TRACE_BLOCK(BLOCK_ITER_WAIT, blockPtr);
	(void) Sync_Wait(&blockPtr->ioDone, FALSE);
	if (sys_ShuttingDown) {
	    return((Hash_Entry *) NIL);
	}
	event = BLOCK_ITER_WAIT_HIT;
	goto again;
    }
    FSUTIL_TRACE_BLOCK(event, blockPtr);
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
    FSUTIL_TRACE_BLOCK(BLOCK_DELETE, blockPtr);
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
Fscache_DumpStats()
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
    register List_Links	  	*listPtr;
    register int		numBlocks = 0;
    register int		totalBytesWasted = 0;
    register int		fragBytesWasted = 0;
    register int		bytesInBlock;
    int				numFrags;

    LOCK_MONITOR;

    listPtr = lruList;
    LIST_FORALL(listPtr, (List_Links *) blockPtr) {
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
