/*
 *  memory.c --
 *
 *	This file implements a dynamic memory allocation system.
 *	It provides procedures to allocate and release storage,
 *	and also includes monitoring and tracing features.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "stdlib.h"
#include "memInt.h"
#include "sprite.h"
#include "sync.h"
#undef free

/*
 * If the MEM_TRACE flag is defined, extra code will be compiled to allow
 * programs to trace Mem_Alloc and Mem_Free calls.
#define MEM_TRACE
 */

/*
 * WARNING: several of the macros in this file expect both integers and
 * pointers to be 32 bits (4 bytes) long.
 */

/*
 * This storage allocator consists of two independent allocators,
 * one used for binned objects (<= BIN_SIZE bytes) and the other
 * used for large objects.
 *
 * Both allocators deal with "blocks" of storage, which correspond
 * roughly to the areas that users request and free.  Each block of
 * storage consists of one administrative word followed by the
 * useable area.  All allocations are done in even numbers of words,
 * which means that clients may occasionally get more bytes than
 * they asked for.  Mem_Alloc returns the address of the word just
 * after the administrative word.  The administrative word allows us
 * to keep track of which blocks are in use and which are free, and also
 * keeps track of the sizes of blocks so clients don't have to know how
 * large a block is when they free it.  The low-order bit of the
 * administrative word indicates whether or not the block is free, and
 * the rest of the word tells how large the block is (in bytes
 * including the administrative word).  The next-to-low-order bit
 * of each block is used to indicate that this is a "dummy block"(see
 * the comments for the large-object allocator).  Using the low-order
 * bits for flags works because all blocks that we allocate are always
 * multiples of 4 bytes in length.  There is one exception to this
 * usage of the administrative word, which occurs for free binned objects.
 * In this case the word is a link to the next free binned object instead
 * of a size.  The macros below define the format of the administrative
 * words and how to access them.
 *
 * All objects <= SMALL_SIZE are automatically binned.  In order to bin
 * objects between SMALL_SIZE and BIN_SIZE bytes the routine Mem_Bin
 * should be called.  Note that Mem_Bin must be called before any objects
 * of the size to be binned have been allocated.
 */

#ifdef MEM_TRACE

typedef struct {
    int		admin;		/* Administrative word. */
    Address	pc;		/* PC of call to Mem_Alloc. */
    int		origSize;	/* Original size given to Mem_Alloc. */
    int         padding;	/* To make it double word aligned */
} AdminInfo;

#define GET_ADMIN(blockPtr)	(((AdminInfo *) (blockPtr))->admin)
#define SET_ADMIN(blockPtr, value)  \
			((AdminInfo *) (blockPtr))->admin = (int) (value)

#define GET_PC(blockPtr)	(((AdminInfo *) (blockPtr))->pc)
#define SET_PC(blockPtr)	((AdminInfo *) (blockPtr))->pc = Mem_CallerPC()

#define GET_ORIG_SIZE(blockPtr)	(((AdminInfo *) (blockPtr))->origSize)
#define SET_ORIG_SIZE(blockPtr,size)	\
			((AdminInfo *) (blockPtr))->origSize = size

#else

typedef double AdminInfo;

#define GET_ADMIN(blockPtr)	(*(int *)(AdminInfo *) (blockPtr))
#define SET_ADMIN(blockPtr, value) *((int *)(AdminInfo *) (blockPtr)) = (int) (value)

#endif /* MEM_TRACE */

#define USE_BIT		1
#define DUMMY_BITS	3
#define ADMIN_BITS(admin)	((admin) & DUMMY_BITS)
#define IS_IN_USE(admin)	((admin) & USE_BIT)
#define IS_DUMMY(admin)		(((admin) & DUMMY_BITS) == DUMMY_BITS)
#define MARK_DUMMY(admin)	((admin) | DUMMY_BITS)
#define MARK_IN_USE(admin)	((admin) | USE_BIT)
#define MARK_FREE(admin)	((admin) & ~DUMMY_BITS)
#define SIZE(admin)		((admin) & ~DUMMY_BITS)

/*
 * The memory manager is a monitor.  The following definition is
 * for its monitor lock.
 */

static Sync_Lock memMonitorLock = Sync_LockInitStatic("memMonitorLock");
#define LOCKPTR (&memMonitorLock)

/*
 * Global variables that can be set to control thresholds for printing
 * statistics.
 */

int     mem_SmallMinNum = 1;            /* There must be at least this many
                                         * binned objects of a size before info
                                         * about its size gets printed. */
int     mem_LargeMinNum  = 1;           /* There must be at least this many
                                         * non-binned objects of a size before
                                         * info about the size gets printed. */
int     mem_LargeMaxSize = 10000;       /* Info is printed for non-binned
                                         * objects larger than this regardless
                                         * of how many of them there are. */

void Mem_PrintStatsSubrInt();
void Mem_PrintConfigSubr();

/*
 * ----------------------------------------------------------------------------
 *
 *	Binned-object allocator: it keeps a separate linked free list
 *	for each size of object less than SMALL_SIZE bytes in size and
 *	a free list for all objects less than BIN_SIZE that have been
 *	specified as being binned by a call to Mem_Bin.
 *	The blocks on each list are linked together via their
 *	administrative words, terminated by a NIL pointer.  Allocation
 *	and freeing are done in a stack-like manner from the front of
 *	the free lists.
 *
 *	Definitions (these are all related:  if you change one, you'll
 *	have to change several):
 *
 *	GRANULARITY: The difference in size between succesive buckets.  This
 *		is determined by data alignment restrictions and should be
 *		a power of 2 to allow for shifting to map from size to bucket.
 *	SIZE_SHIFT: The shift distance to convert between size and bucket.
 *	SMALL_SIZE: largest blocksize (total including administrative word)
 *		managed by default by the binned-object allocator.  This
 *		should be 1 less than a multiple of the granularity.
 *	BIN_SIZE: largest possible that can be binned.  Should be one less
 *		than a multiple of the granularity.
 *
 *	These can be computed from the above constants.
 *
 *	SMALL_BUCKETS: default number of separate free lists.
 *	BIN_BUCKETS: number of binned free lists.
 *	BYTES_TO_BLOCKSIZE: how many bytes a block must contain (total
 *		including admin. word) to satisfy a user request in bytes.
 *	BLOCKSIZE_TO_INDEX: which free list to use for blocks of a given
 *		size (total including administrative word).
 *	INDEX_TO_BLOCKSIZE: the (maximum) size corresponding to a bucket.
 *		This will include the size of the administrative word.
 *
 *	These constants determine how many new blocks are allocated to
 *		a bin at one time.
 *
 *	MIN_BLOCKS_AT_ONCE: The amount allocate the first time, and the amount
 *		to increment the number of blocks each successive time.
 *	MIN_SHIFT: The log2 of MIN_BLOCKS_AT_ONCE.
 *	MAX_BLOCKS_AT_ONCE: the most number of blocks to be allocated at once.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * 8 byte granularity to handle SPUR and SPARC.
 */
#define GRANULARITY			8
#define SIZE_SHIFT			3
/*
 * This SMALL_SIZE determined by examination of memory tracing, 1/89.
 * The BIN_SIZE allows us to bin an FS_BLOCK_SIZE object.
 */
#define SMALL_SIZE			271
#define	BIN_SIZE			4199

#define SMALL_BUCKETS			((SMALL_SIZE + 1) / GRANULARITY)
#define BIN_BUCKETS			((BIN_SIZE + 1) / GRANULARITY)
#define MASK				(GRANULARITY - 1)
#define BYTES_TO_BLOCKSIZE(bytes) ((bytes+MASK+sizeof(AdminInfo)) & (~MASK))
#define BLOCKSIZE_TO_INDEX(size)  ((size) >> SIZE_SHIFT)
#define INDEX_TO_BLOCKSIZE(index) ((index) << SIZE_SHIFT)
/*
 * These are set up to allocate 4, 8, 12, and then 16 blocks at a time.
 */
#define MIN_BLOCKS_AT_ONCE		4
#define MIN_SHIFT			2
#define MAX_BLOCKS_AT_ONCE 		16

/*
 * The following array holds pointers to the first free blocks of each size.
 * Bucket i holds blocks whose total length is INDEX_TO_BLOCKSIZE(i) bytes
 * (including the administrative info).  Blocks from bucket i are used to
 * satisfy user requests ranging from INDEX_TO_BLOCKSIZE(i)-sizeof(AdminInfo)
 * bytes up to INDEX_TO_BLOCKSIZE(i)-1 bytes.
 * Buckets 0 and 1 are never used, since they correspond to blocks too small
 * to hold any information for the user.
 */

static Address freeLists[BIN_BUCKETS];

/*
 * Used to gather statistics about allocations:
 */

static int numBlocks[BIN_BUCKETS];	/* Total number of existing blocks,
					 * both allocated and free, for each
					 * small size.  */
static int numAllocs[BIN_BUCKETS];	/* Total number of allocation requests
					 * made for blocks of each small size.
					 */

/*
 * ----------------------------------------------------------------------------
 *	Large-object allocator:  used for blocks that aren't binned.
 *	We expect there to be relatively few allocations
 *	made by the large-object allocator.  Since all the blocks it
 *	allocates are relatively large, fragmentation should be less
 *	than it would be if it allocated small blocks too. All of the
 *	blocks, in use or not, are kept in a single linked list using
 *	their administrative words.  The address of the next block in
 *	the list is found by adding the size of the current block to the
 *	address of its administrative word.  Things are complicated
 *	slightly because the storage area managed by this allocator
 *	may be discontiguous:  there could be several large regions
 *	that it manages, separated by gaps that are managed by some other
 *	storage allocator (e.g. the binned-object allocator).  In this
 *	case, the gaps between regions are handled by making the gaps
 *	look like allocated blocks, with an administrative word at the
 *	end of one region that causes a skip to the beginning of the
 *	next region.  These blocks are called "dummy blocks", and have
 *	special flag bits in their administrative words (see definitions
 *	below).  All the blocks within a region are linked together
 *	in increasing order of address.  However, the different regions
 *	may appear in any order on the list.  Two dummy blocks mark the
 *	beginning and end of the list.
 * ----------------------------------------------------------------------------
 */

static char *first, *last;
static char _first[sizeof(AdminInfo) + 4], _last[sizeof(AdminInfo) + 4];
				/* Dummy blocks marking beginning and
				 * end of free list.  Last has a size of
				 * zero.  */
static Address currentPtr;	/* Next block to consider allocating.
				 * Rotates circularly through free list. */
int largestFree;		/* This holds the size of the largest
				 * free block that's been seen on the
				 * free list.  It's updated as the
				 * list is traversed.  */

/*
 * Minimum size of new regions requested by large-object allocator
 * when storage runs out:
 */

#define MIN_REGION_SIZE 2048

static int numLargeBytes;	/* Total amount of storage managed by
				 * the large allocator.  */
static int numLargeAllocs;	/* Total number of allocation requests
				 * handled by the large allocator.  */
static int numLargeLoops;	/* Number of iterations through the
				 * inner block-checking loop of the
				 * large allocator.  */

/*
 * ----------------------------------------------------------------------------
 *	Miscellaneous declarations.
 * ----------------------------------------------------------------------------
 */

/*
 * Flag to make sure Init gets called once.
 */

static void	Init();
static int	initialized = FALSE;


#ifdef MEM_TRACE

/*
 * When Mem_Alloc or Mem_Free is called, a trace record will be printed and/or
 * the number of blocks being used by the caller will be stored in the trace
 * array if the block size is in sizeTraceArray. The array is defined with
 * Mem_SetTraceSizes(). PrintTrace calls a default printing routine; it can
 * be changed with Mem_SetPrintProc().
 */
#define MAX_NUM_TRACE_SIZES	20
#define	MAX_CALLERS_TO_TRACE	20
static	struct TraceElement {
    Mem_TraceInfo	traceInfo;
    struct {
	Address	callerPC;
	int	numBlocks;
    } allocInfo[MAX_CALLERS_TO_TRACE];
} sizeTraceArray[MAX_NUM_TRACE_SIZES];

static	int	numSizesToTrace = 0;
static	void	DoTrace();
static	void	PrintTrace();

int		mem_PrintLargeAllocPC = 0;

#endif /* MEM_TRACE */



/*
 * ----------------------------------------------------------------------------
 *
 * Init --
 *
 *      Initializes the dynamic storage allocator.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage allocation structures are initialized.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static void
Init()
{
    int i;

    /*
     * Clear out all of the bins.
     */
    for (i = BIN_BUCKETS - 1; i > SMALL_BUCKETS - 1; i--) {
	numBlocks[i] = -1;
	freeLists[i] = (Address) NIL;
	numAllocs[i] = 0;
    }

    /*
     * Clear out the small-object free lists.
     */
    for (; i >= 0; i--) {
	freeLists[i] = (Address) NIL;
	numBlocks[i] = 0;
	numAllocs[i] = 0;
    }

    /*
     * Initialize the large-object free list with two dummy blocks that
     * mark its beginning and end. These blocks are declared to be arrays
     * of characters. In order for malloc to work correctly they have to
     * be aligned on word boundaries.
     */
#if 0
    if (((int) first) & 0x3) {
	panic("Mem: 'first' is not word-aligned");
    }
    if (((int) last) & 0x3) {
	panic("Mem: 'last' is not word-aligned");
    }
#else
    first = (char *) (((long)_first + 3) & ~3);
    last = (char *) (((long)_last + 3) & ~3);
#endif

    SET_ADMIN(first, MARK_DUMMY(last-first));
    SET_ADMIN(last, MARK_DUMMY(0));
    currentPtr		= first;
    largestFree		= 0;
    numLargeBytes	= 0;
    numLargeAllocs	= 0;
    numLargeLoops	= 0;

    MemPrintInit();
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_Bin --
 *
 *	Make objects of the given size be binned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bin corresponding to blocks of the given size is initialized.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Mem_Bin(numBytes)
    int	numBytes;
{
    int	index;

    LOCK_MONITOR;

    if (!initialized) {
        initialized = TRUE;
	Init();
    }
    if (mem_NumAllocs > 0) {
	MemPanic("Mem_Bin: Mem_Alloc already called.\n");
    }
    numBytes = BYTES_TO_BLOCKSIZE(numBytes);
    if (numBytes > BIN_SIZE) {
	UNLOCK_MONITOR;
	return;
    }
    index = BLOCKSIZE_TO_INDEX(numBytes);
    if (numBlocks[index] >= 0) {
	UNLOCK_MONITOR;
	return;
    }
    numBlocks[index] = 0;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * malloc --
 *
 *	This procedure allocates a chunk of memory.
 *
 * Results:
 *	The return value is a pointer to an area of at least numBytes
 *	bytes of free storage.  If no memory is available, then this
 *	procedure will never return:  the MemChunkAlloc procedure
 *	determines what will happen.
 *
 * Side effects:
 *	The returned block is marked as allocated and will not be
 *	allocated to anyone else until it is returned with a call
 *	to free().
 *
 * ----------------------------------------------------------------------------
 */


ENTRY Address
malloc(numBytes)
    register int numBytes;	/* How many bytes to allocate.  Must be > 0 */
{
    register int size, admin;
    Address	result;
    Address	newRegion;
    int		regionSize;
#ifdef MEM_TRACE
    int		origSize;
#endif /* MEM_TRACE */
    register int index;

    LOCK_MONITOR;

    Sync_LockRegister(LOCKPTR);

    mem_NumAllocs++;

    if (!initialized) {
        initialized = TRUE;
	Init();
    }

#ifdef MEM_TRACE
    origSize = numBytes;
#endif /* MEM_TRACE */

    /*
     * Handle binned objects quickly, if possible.
     */
    numBytes = BYTES_TO_BLOCKSIZE(numBytes);
    index = BLOCKSIZE_TO_INDEX(numBytes);
    if (numBytes <= BIN_SIZE && numBlocks[index] != -1) {
	if (freeLists[index] == (Address) NIL) {
	    register int blocksAtOnce;

	    /*
	     * There aren't currently any free objects of this size.
	     * Call the client's function to obtain another region
	     * from the system.  While we're at it, get a whole bunch
	     * of objects of this size once and put all but one back
	     * on the free list.
	     */
	    if (numBlocks[index] < MAX_BLOCKS_AT_ONCE) {
		blocksAtOnce = ((numBlocks[index] >> MIN_SHIFT) + 1) *
				MIN_BLOCKS_AT_ONCE;
	    } else {
		blocksAtOnce = MAX_BLOCKS_AT_ONCE;
	    }
	    regionSize = MemChunkAlloc(blocksAtOnce * numBytes, &newRegion);

	    while (regionSize >= numBytes) {
		SET_ADMIN(newRegion, freeLists[index]);
		freeLists[index] = newRegion;
		numBlocks[index] += 1;
		newRegion += numBytes;
		regionSize -= numBytes;
	    }

	    /*
	     * If we were given more bytes than we wanted, and there aren't
	     * quite enough left to make a full block, put the leftovers
	     * on the free list for the smallest size.  This may still result
	     * in GRANULARITY bytes leftover that we have to throw away.
	     */

	    while (regionSize >= (sizeof(AdminInfo) + GRANULARITY)) {
		SET_ADMIN(newRegion, freeLists[2]);
		freeLists[2] = newRegion;
		numBlocks[2] += 1;
		newRegion += (sizeof(AdminInfo) + GRANULARITY);
		regionSize -= (sizeof(AdminInfo) + GRANULARITY);
	    }
	}

	result = freeLists[index];
	freeLists[index] = (Address) GET_ADMIN(result);
	SET_ADMIN(result, MARK_IN_USE(numBytes));
	numAllocs[index] += 1;

#ifdef MEM_TRACE
	SET_PC(result);
	SET_ORIG_SIZE(result, origSize);
	DoTrace(TRUE, result, (Address)NULL, numBytes);
#endif /* MEM_TRACE */

	UNLOCK_MONITOR;
	return(result+sizeof(AdminInfo));
    }

    /*
     * This is a large object.  Step circularly through the blocks
     * in the list, looking for one that's large enough to hold
     * what's needed.  Move currentPtr to the next block in the list to
     * make sure that we make forward progress each time Mem_Alloc is called.
     * If we don't then there is an instability in the memory allocator
     * with the following pattern:
     *
     *  while (TRUE) {
     *	    addr = malloc(4096);
     *      free(addr);
     *      malloc(248);
     *  }
     *
     * This pattern causes memory to get heavily fragmented.
     */

    numLargeAllocs += 1;
    admin = GET_ADMIN(currentPtr);
    currentPtr += SIZE(admin);
    while (TRUE) {
	Address nextPtr;

	numLargeLoops += 1;
	admin = GET_ADMIN(currentPtr);
	size = SIZE(admin);
	nextPtr = currentPtr+size;
	if (!IS_IN_USE(admin)) {
	
	    /*
	     * Several blocks in a row could have been freed since the last
	     * time we were here.  If so, merge them together.
	     */

	    while (!IS_IN_USE(GET_ADMIN(nextPtr))) {
		size += SIZE(GET_ADMIN(nextPtr));
		admin = MARK_FREE(size);
		SET_ADMIN(currentPtr, admin);
		nextPtr = currentPtr+size;
	    }
	    if (size >= numBytes) {
		break;
	    }
	    if (size > largestFree) {
		largestFree = size;
	    }
	}

	/*
	 * This block won't do the job.  Go on to the next block.
	 * If the next block is the end of the list, then go back
	 * to the beginning and start again, unless no storage has
	 * been freed since the last time we went back.  In this
	 * case, allocate a new region of storage.
	 */
	
	if (nextPtr != last) {
	    currentPtr = nextPtr;
	    continue;
	}

	if (largestFree > numBytes) {
	    largestFree = 0;
	    currentPtr = first;
	    continue;
	}

	if (numBytes < MIN_REGION_SIZE) {
	    regionSize = MemChunkAlloc(MIN_REGION_SIZE, &newRegion);
	} else {
	    regionSize = MemChunkAlloc(numBytes + sizeof(AdminInfo),
	    			       &newRegion);
	}
	numLargeBytes += regionSize;

	/*
	 * If the new region immediately follows the end of the previous
	 * region, merge the two together (at this point currentPtr always
	 * points to a dummy block whose administrative word points to "last").
	 * If we can't merge, then make currentPtr link to the new area.
	 */

	if (newRegion == (currentPtr+sizeof(AdminInfo))) {
	    newRegion = currentPtr;
	    regionSize += sizeof(AdminInfo);
	} else {
	    SET_ADMIN(currentPtr, MARK_DUMMY(newRegion - currentPtr));
	}

	/*
	 * Create a dummy block at the end of the new region, which links
	 * to "last".
	 */
	
	SET_ADMIN(newRegion, MARK_FREE(regionSize - sizeof(AdminInfo)));
	newRegion += regionSize - sizeof(AdminInfo);
	SET_ADMIN(newRegion, MARK_DUMMY(last - newRegion));

	/*
	 * Continue scanning the list (try currentPtr again in case it
	 * merged with the new region).
	 */
    }

    /*
     * At this point we've got a block that's large enough.  If it's
     * larger than needed for the object, put the rest back on the
     * free list (note: even if the remnant is smaller than the smallest
     * large object, which means it'll be used by itself, we put it back
     * on the list so it can merge with either this block or the next,
     * whichever gets freed first).
     */
    
    if (size == numBytes) {
	SET_ADMIN(currentPtr, MARK_IN_USE(admin));
    } else {
	SET_ADMIN(currentPtr+numBytes, MARK_FREE(size-numBytes));
	SET_ADMIN(currentPtr, MARK_IN_USE(numBytes));
    }

#ifdef MEM_TRACE
    SET_PC(currentPtr);
    SET_ORIG_SIZE(currentPtr, origSize);
    DoTrace(TRUE, currentPtr, (Address)NULL, numBytes);
#endif /* MEM_TRACE */

    UNLOCK_MONITOR;
    return(currentPtr+sizeof(AdminInfo));
}


/*
 * ----------------------------------------------------------------------------
 *
 * _free --
 *
 *      Return a previously-allocated block of storage to the free pool.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage pointed to by blockPtr is marked as free and returned
 *	to the free pool.  Nothing in the bytes pointed to by blockPtr is
 *	modified at this time:  no change will occur until at least the
 *	next call to Mem_Alloc.  This means that callers may use the contents
 *	of a block for a short time after free-ing it (e.g. to read a
 *	"next" pointer).
 *
 * ----------------------------------------------------------------------------
 */

ENTRY int
free(blockPtr) 
    Address blockPtr;
{
    return _free(blockPtr);
}

ENTRY int
_free(blockPtr)
    register Address blockPtr;	/* Pointer to storage to be freed.  Must
				 * have been the return value from Mem_Alloc
				 * at some previous time.  */
{
    register int  admin;
    register int  index;
    register int  size;

    LOCK_MONITOR;

    mem_NumFrees++;

    if (!initialized) {
        MemPanic("Mem_Free: allocator not initialized!\n");
	return 0;		/* should never get here */
    }

    /*
     *  Make sure that this block bears some resemblance to a
     *  well-formed storage block.
     */
    
    blockPtr -= sizeof(AdminInfo);
    admin = GET_ADMIN(blockPtr);
    if (ADMIN_BITS(admin) != USE_BIT) {
	if (!IS_IN_USE(admin)) {
	    if (!memAllowFreeingFree) {
		MemPanic("Mem_Free: storage block already free\n");
	    }
	} else {
	    MemPanic("Mem_Free: storage block is corrupted\n");
	}
	UNLOCK_MONITOR;
	return 0;			/* (should never get here) */
    }

    /* This procedure is easier for large blocks than for small ones.
     * If large, just clear the use bit.  Since this block could merge
     * with its neighbors, we don't really know anymore what's the
     * largest size on the free list, so set largestFree to a very
     * large number.  This guarantees that the allocator will check
     * the whole list again before requesting additional memory.
     */

    size = SIZE(admin);
    index = BLOCKSIZE_TO_INDEX(size);
    if (size > BIN_SIZE || numBlocks[index] == -1) {
	SET_ADMIN(blockPtr, MARK_FREE(admin));
	largestFree = numLargeBytes;
    } else {
	/*
	 * For small blocks, add the block back onto its free list.
	 */

	SET_ADMIN(blockPtr, freeLists[index]);
	freeLists[index] = blockPtr;
    }

#ifdef MEM_TRACE
    DoTrace(FALSE, blockPtr, Mem_CallerPC(), size);
#endif /* MEM_TRACE */

    UNLOCK_MONITOR;
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_Size --
 *
 *      Return the size of a previously-allocated storage block.
 *
 * Results:
 *      The return value is the size of *blockPtr, in bytes.  This is
 *	the total usable size of the block.  It may be slightly greater
 *	than the size actually requested in the Mem_Alloc, since this
 *	module rounds sizes up to convenient boundaries.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY int
Mem_Size(blockPtr)
    Address blockPtr;	/* Pointer to storage to be freed.  Must have been the
			 * return value from Mem_Alloc at some previous time. */
{
    int admin;

    LOCK_MONITOR;

    if (!initialized) {
        MemPanic("Mem_Size: allocator not initialized!\n");
	UNLOCK_MONITOR;
	return(0);			/* should never get here */
    }

    /*
     *  Make sure that this block bears some resemblance to a
     *  well-formed storage block.
     */
    
    blockPtr -= sizeof(AdminInfo);
    admin = GET_ADMIN(blockPtr);
    if (ADMIN_BITS(admin) != USE_BIT) {
	if (!IS_IN_USE(admin)) {
	    MemPanic("Mem_Size: storage block is free\n");
	} else {
	    MemPanic("Mem_Size: storage block is corrupted\n");
	}
	UNLOCK_MONITOR;
	return(0);			/* (should never get here) */
    }

    UNLOCK_MONITOR;
    return(SIZE(admin) - sizeof(AdminInfo));
}


/*
 *----------------------------------------------------------------------
 *
 * Mem_PrintStats --
 *
 *	Print out memory statistics with Mem_PrintStatsSubr using the
 *	default printing routine and default sizes.
 *
 *	See Mem_PrintStatsSubrInt for details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Mem_PrintStats()
{
    LOCK_MONITOR;

    Mem_PrintStatsSubrInt(memPrintProc, memPrintData, mem_SmallMinNum,
			  mem_LargeMinNum, mem_LargeMaxSize);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Mem_PrintStatsInt --
 *
 *	Print out memory statistics with Mem_PrintStatsSubr using the
 *	default printing routine and default sizes.  Same as Mem_PrintStats
 *	except that this routine does not grab the monitor lock.  This is
 *	intended to be used to print out memory stats when the operating
 *	system runs out of memory after being called by Mem_Alloc and hence
 *	the monitor is already down.
 *
 *	See Mem_PrintStatsSubrInt for details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Mem_PrintStatsInt()
{
    Mem_PrintStatsSubrInt(memPrintProc, memPrintData, mem_SmallMinNum,
			  mem_LargeMinNum, mem_LargeMaxSize);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintStatsSubr --
 *
 *      This procedure prints out statistics about memory allocation
 *	so far.  Grabs monitor lock and calls the unmonitored routine
 *	Mem_PrintStatsSubrInt.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Mem_PrintStatsSubr(PrintProc, clientData, smallMinNum, largeMinNum,
	largeMaxSize)
    void 	(*PrintProc)();	/* Procedure to actually print stuff. */
    ClientData	clientData;	/* Argument to pass to PrintProc. */
    int		smallMinNum;	/* Minimum number of elements in a bucket
				 * before will print out stats. */
    int		largeMinNum;	/* Minimum number of large objects of a given
				 * size before will print out stats about
				 * the size. */
    int		largeMaxSize;	/* Statistics will be printed about all blocks
				 * over size regardless of how many there are*/
{
    LOCK_MONITOR;

    Mem_PrintStatsSubrInt(PrintProc, clientData, smallMinNum, largeMinNum,
			  largeMaxSize);

    UNLOCK_MONITOR;
}

#define	MAX_TO_PRINT	256
static struct {
    int		size;		/* Size of the block. */
    int		num;		/* Number of blocks allocated. */
    int		free;		/* Number of blocks freed. */
    int		inUse;		/* Number of blocks still in use. */
    int		dummy;		/* Number of blocks used as dummy blocks. */
} topN[MAX_TO_PRINT + 1];


/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintStatsSubrInt --
 *
 *      This procedure prints out statistics about memory allocation
 *	so far.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The procedure PrintProc is called several times to print out
 *	information.  Its calling sequence is:
 *
 *	void
 *	PrintProc(clientData, format, arg1, arg2, arg3, arg4, arg5)
 *	    ClientData clientData;
 *	    char *format;
 *
 *	This is identical to the calling sequence for the standard I/o
 *	routine Io_Print.  The first argument is the clientData argument
 *	passed to this procedure, which need not necessarily be a
 *	(Io_Stream *).  There will never be more than 5 arguments.
 *	A typical calling sequence is:
 *	"Mem_PrintStatsSubr(Io_PrintStream, (ClientData) io_StdOut,
 *				50, 100, 1000)".
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
Mem_PrintStatsSubrInt(PrintProc, clientData, smallMinNum, largeMinNum,
	largeMaxSize)
    void 	(*PrintProc)();	/* Procedure to actually print stuff. */
    ClientData	clientData;	/* Argument to pass to PrintProc. */
    int		smallMinNum;	/* Minimum number of elements in a bucket
				 * before will print out stats. */
    int		largeMinNum;	/* Minimum number of large objects of a given
				 * size before will print out stats about
				 * the size. */
    int		largeMaxSize;	/* Statistics will be printed about all blocks
				 * over size regardless of how many there are*/
{
    register Address	ptr;
    register int	i;
    int		totalBlocks, totalAllocs, totalFree;
    int		allocBytes = 0;
    int		freeBytes = 0;
    Boolean	firstTime = TRUE;
    Boolean	warnedAboutOverflow;

    if (!initialized) {
	(*PrintProc)(clientData, "Allocator not initialized yet.\n");
	return;
    }

    (*PrintProc)(clientData, "\nTotal allocs = %d, frees = %d\n\n",
		mem_NumAllocs, mem_NumFrees);

    (*PrintProc)(clientData, "Small object allocator:\n");
    totalBlocks = totalAllocs = totalFree = 0;
    for (i = 2; i < BIN_BUCKETS; i++) {
	int	numFree = 0;

	if (numBlocks[i] <= 0) {
	    continue;
	}
	allocBytes += numBlocks[i] * INDEX_TO_BLOCKSIZE(i);
	for (ptr = freeLists[i];
	     ptr != (Address) NIL;
	     ptr = (Address) GET_ADMIN(ptr)) {

	    freeBytes += INDEX_TO_BLOCKSIZE(i);
	    numFree += 1;
	}
	if (numBlocks[i] >= smallMinNum) {
	    if (firstTime) {
		(*PrintProc)(clientData,
				"    Size     Total    Allocs    In Use\n");
		firstTime = FALSE;
	    }
	    (*PrintProc)(clientData, "%8d%10d%10d%10d\n",
			INDEX_TO_BLOCKSIZE(i), numBlocks[i],
			numAllocs[i], numBlocks[i] - numFree);
	}
	totalBlocks += numBlocks[i];
	totalAllocs += numAllocs[i];
	totalFree += numFree;
    }
    (*PrintProc)(clientData, "   Total%10d%10d%10d\n", totalBlocks,
	    totalAllocs, totalBlocks - totalFree);
    (*PrintProc)(clientData, "Bytes allocated = %d, freed = %d\n\n",
				allocBytes, freeBytes);

    /*
     * Initialize the largest N-sizes buffer.
     */
    for (i = 0; i < MAX_TO_PRINT + 1; i++) {
	topN[i].size = -1;
	topN[i].free = 0;
	topN[i].dummy = 0;
    }

    warnedAboutOverflow = FALSE;
    for (ptr = first + SIZE(GET_ADMIN(first));
	 ptr != last;
	 ptr += SIZE(GET_ADMIN(ptr))) {

	int	admin;
	int	size;
	Boolean	found;

	admin = GET_ADMIN(ptr);
	if (!IS_DUMMY(admin) && !IS_IN_USE(admin)) {
	    freeBytes += SIZE(admin);
	}

	size = SIZE(admin);
#ifdef MEM_TRACE
	if (size - GET_ORIG_SIZE(ptr) < 4 &&
	    size - GET_ORIG_SIZE(ptr) >= 0) {
	    size = GET_ORIG_SIZE(ptr);
	}
#endif /* MEM_TRACE */

	found = FALSE;
	for (i = 0; i < MAX_TO_PRINT; i++) {
	    if (size == topN[i].size) {
		found = TRUE;
		topN[i].num++;
		if (IS_DUMMY(admin)) {
		    topN[i].dummy++;
		} else if (IS_IN_USE(admin)) {
		    topN[i].inUse++;
		} else {
		    topN[i].free++;
		}
		break;
	    } else if (topN[i].size == -1) {
		found = TRUE;
		topN[i].size = size;
		topN[i].num = 1;
		if (IS_DUMMY(admin)) {
		    topN[i].dummy = 1;
		} else if (IS_IN_USE(admin)) {
		    topN[i].inUse = 1;
		} else {
		    topN[i].free = 1;
		}
		break;
	    }
	}

	if (!found && !warnedAboutOverflow) {
	    (*PrintProc)(clientData, "Largest-Size buffer overflow: %d\n",size);
	    warnedAboutOverflow = TRUE;
	}
    }
    (*PrintProc)(clientData, "Large object allocator:\n");
    (*PrintProc)(clientData, "   Total bytes managed: %d\n", numLargeBytes);
    (*PrintProc)(clientData, "   Bytes in use:        %d\n",
	    			    numLargeBytes - freeBytes);
    firstTime = TRUE;
    for (i = 0; topN[i].size != -1; i++) {
	if ((topN[i].num >= largeMinNum || topN[i].size >= largeMaxSize) &&
	    (topN[i].num != topN[i].dummy)) {
	    if (firstTime) {
		(*PrintProc)(clientData, "%10s%10s%10s%10s\n",
#ifdef MEM_TRACE
			"Orig. Size", "Num", "Free", "In Use");
#else
			"Size", "Num", "Free", "In Use");
#endif /* MEM_TRACE */
		firstTime = FALSE;
	    }
	    (*PrintProc)(clientData, "%10d%10d%10d%10d\n",
		topN[i].size, topN[i].num, topN[i].free, topN[i].inUse);
	}
    }
    (*PrintProc)(clientData, "\n");
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintConfig --
 *
 *      Prints out the exact configuration of the dynamic memory allocator
 *	using the default printing routine.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mem_PrintConfig()
{
    Mem_PrintConfigSubr(memPrintProc, memPrintData);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintConfigSubr --
 *
 *      Uses a client-supplied procedure to print out the exact
 *	configuration of the dynamic memory allocator.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Stuff gets printed (?) by passing it to PrintProc.  See the
 *	documentation in Mem_PrintStatsSubr for the calling sequence to
 *	PrintProc.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Mem_PrintConfigSubr(PrintProc, clientData)
    void (*PrintProc)();	/* Procedure to actually print stuff. */
    ClientData clientData;	/* Argument to pass to PrintProc. */
{
    register Address ptr;
#ifdef VERBOSE
    int i, j;
#endif /* VERBOSE */

    LOCK_MONITOR;

    if (!initialized) {
	(*PrintProc)(clientData, "Allocator not initialized yet.\n");
	return;
    }

#ifdef VERBOSE
    (*PrintProc)(clientData, "Small object allocator:\n");
    for (i = 2; i < BIN_BUCKETS; i++) {
	if (freeLists[i] == (Address) NIL) {
	    continue;
	}
	(*PrintProc)(clientData, "    %d bytes:", i*GRANULARITY);
	j = 5;
	for (ptr = freeLists[i]; ptr != (Address) NIL;
		ptr = (Address) GET_ADMIN(ptr)) {
	    if (j == 5) {
		(*PrintProc)(clientData, "\n    ");
		j = 0;
	    } else {
		j += 1;
	    }
	    (*PrintProc)(clientData, "%12#x", ptr);
	}
	(*PrintProc)(clientData, "\n");
    }
#endif /* VERBOSE */

    (*PrintProc)(clientData, "Large object allocator:\n");

#ifdef MEM_TRACE
    (*PrintProc)(clientData, "    Location   Orig. Size   State\n");
#else
    (*PrintProc)(clientData, "    Location       Size     State\n");
#endif /* MEM_TRACE */

    for (ptr = first + SIZE(GET_ADMIN(first)); ptr != last;
	    ptr += SIZE(GET_ADMIN(ptr))) {

#ifdef MEM_TRACE
	(*PrintProc)(clientData, "%12#x %10d", ptr, GET_ORIG_SIZE(ptr));
	if (IS_DUMMY(GET_ADMIN(ptr))) {
	    (*PrintProc)(clientData, "     Dummy\n");
	} else if (IS_IN_USE(GET_ADMIN(ptr))) {
	    (*PrintProc)(clientData, "     In use (PC=0x%x)\n", GET_PC(ptr));
#else
	(*PrintProc)(clientData, "%12#x %10d", ptr, SIZE(GET_ADMIN(ptr)));
	if (IS_DUMMY(GET_ADMIN(ptr))) {
	    (*PrintProc)(clientData, "     Dummy\n");
	} else if (IS_IN_USE(GET_ADMIN(ptr))) {
	    (*PrintProc)(clientData, "     In use\n");
#endif /* MEM_TRACE */
	} else {
	    (*PrintProc)(clientData, "     Free\n");
	}
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintInUse --
 *
 *      Uses a default procedure to print out the blocks in allocator
 *	that are still in use. The original size and the PC of the
 *	call to Mem_Alloc are printed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Stuff gets printed (?) by passing it to memPrintProc.  See the
 *	documentation in Mem_PrintStatsSubr for the calling sequence to
 *	memPrintProc.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Mem_PrintInUse()
{
#ifdef MEM_TRACE
    register Address ptr;

    LOCK_MONITOR;

    if (!initialized) {
	(*memPrintProc)(memPrintData, "Allocator not initialized yet.\n");
	return;
    }

    (*memPrintProc)(memPrintData, "Large objects still in use:\n");
    (*memPrintProc)(memPrintData, "    Location   Orig. Size   Alloc.PC\n");

    for (ptr = first + SIZE(GET_ADMIN(first)); ptr != last;
	    ptr += SIZE(GET_ADMIN(ptr))) {

	if (!IS_DUMMY(GET_ADMIN(ptr)) && IS_IN_USE(GET_ADMIN(ptr))) {
	    (*memPrintProc)(memPrintData,"%12#x %10d %12#x\n",
		ptr, GET_ORIG_SIZE(ptr), GET_PC(ptr));
	}
    }
    UNLOCK_MONITOR;
#endif /* MEM_TRACE */
}


/*
 *----------------------------------------------------------------------
 *
 * PrintTrace --
 *
 *	Print out the given trace information about a memory trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef MEM_TRACE
INTERNAL static void
PrintTrace(allocated, infoPtr, curPC)
    Boolean		allocated;	/* If TRUE, we are called by Mem_Alloc,
					 * otherwise by Mem_Free. */
    register Address	infoPtr;	/* Address of admin. info. */
    Address		curPC;		/* If called by Mem_Free, PC of
					 * call to Mem_Free, NULL otherwise. */
{
    if (allocated) {
	(*memPrintProc)(memPrintData,
		"Mem_Alloc: PC=0x%x  addr=0x%x  size=%d\n",
		GET_PC(infoPtr), infoPtr+sizeof(AdminInfo),
		GET_ORIG_SIZE(infoPtr));
    } else {
	(*memPrintProc)(memPrintData,
		"Mem_Free:  PC=0x%x  addr=0x%x  size=%d *\n",
		curPC, infoPtr+sizeof(AdminInfo), GET_ORIG_SIZE(infoPtr));
    }
}
#endif /* MEM_TRACE */


/*
 *----------------------------------------------------------------------
 *
 * Mem_SetTraceSizes --
 *
 *	Defines a list of sizes to trace and causes tracing to start.
 *	If the numSizes is zero or the array ptr is NULL, tracing is
 *	turned off.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Traces of Mem_Alloc and Mem_Free start or end.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
Mem_SetTraceSizes(numSizes, arrayPtr)
    int		  numSizes;		/* # of elements in arrayPtr */
    Mem_TraceInfo *arrayPtr;		/* Array of block sizes to trace. */
{
#ifdef MEM_TRACE
    int	i;

    LOCK_MONITOR;

    if (numSizes <= 0 || arrayPtr == (Mem_TraceInfo *)NIL ||
	arrayPtr == (Mem_TraceInfo *)NULL) {
	if (numSizes == -1) {
	    numSizesToTrace = -1;
	} else {
	    numSizesToTrace = 0;
	}
	UNLOCK_MONITOR;
	return;
    }

    if (numSizes > MAX_NUM_TRACE_SIZES) {
	numSizes = MAX_NUM_TRACE_SIZES;
    }
    numSizesToTrace = numSizes;
    for (i = 0; i < numSizes; i++) {
	sizeTraceArray[i].traceInfo = arrayPtr[i];
	sizeTraceArray[i].traceInfo.flags |= MEM_TRACE_NOT_INIT;
    }
    UNLOCK_MONITOR;

#endif /* MEM_TRACE */
}


/*
 *----------------------------------------------------------------------
 *
 * Mem_SetPrintProc --
 *
 *	Changes the default printing routines
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The default printing routine is changed.
 *
 *----------------------------------------------------------------------
 */

typedef void (*VoidProc)();

ENTRY void
Mem_SetPrintProc(proc, data)
    VoidProc	proc;		/* Address of new print routine. */
    ClientData	data;		/* Data to be passed to proc. */
{
    LOCK_MONITOR;
    if (proc != (VoidProc) NIL && proc != (VoidProc) NULL) {
	memPrintProc = proc;
	memPrintData = data;
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * DoTrace --
 *
 *	Print and/or store a trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef MEM_TRACE
INTERNAL static void
DoTrace(allocated, infoPtr, curPC, size)
    Boolean		allocated;	/* If TRUE, we are called by Mem_Alloc,
					 * otherwise by Mem_Free. */
    register Address	infoPtr;	/* Address of admin. info. */
    Address		curPC;		/* If called by Mem_Free, PC of
					 * call to Mem_Free, NULL otherwise. */
    int			size;		/* Size actually allocated. */
{
    int					i, j;
    int					origSize;
    Address				callerPC;
    register	struct	TraceElement	*trPtr;

    if (numSizesToTrace == -1) {
	PrintTrace(allocated, infoPtr, curPC);
	return;
    }

    callerPC = GET_PC(infoPtr);

    origSize = GET_ORIG_SIZE(infoPtr);

    for (i = 0, trPtr = sizeTraceArray; i < numSizesToTrace; i++, trPtr++) {
	if (trPtr->traceInfo.flags & MEM_DONT_USE_ORIG_SIZE) {
	    if (trPtr->traceInfo.size != size) {
		continue;
	    }
	} else if (trPtr->traceInfo.size != origSize) {
	    continue;
	}
	if (trPtr->traceInfo.flags & MEM_PRINT_TRACE) {
	    PrintTrace(allocated, infoPtr, curPC);
	}
	if (trPtr->traceInfo.flags & MEM_STORE_TRACE) {
	    register int slot;
	    if (trPtr->traceInfo.flags & MEM_TRACE_NOT_INIT) {
		for (j = 0; j < MAX_CALLERS_TO_TRACE; j++) {
		    trPtr->allocInfo[j].numBlocks = 0;
		    trPtr->allocInfo[j].callerPC = 0;
		}
		trPtr->traceInfo.flags &= ~MEM_TRACE_NOT_INIT;
	    }
	    slot = -1;
	    for (j = 0; j < MAX_CALLERS_TO_TRACE; j++) {
		if (trPtr->allocInfo[j].callerPC == callerPC) {
		    if (allocated) {
			trPtr->allocInfo[j].numBlocks++;
		    } else {
			trPtr->allocInfo[j].numBlocks--;
		    }
		    return;
		}
		if ((trPtr->allocInfo[j].numBlocks == 0) && (slot < 0)) {
		    slot = j;
		}
	    }
	    if ((slot >= 0) && allocated) {
		trPtr->allocInfo[slot].callerPC = callerPC;
		trPtr->allocInfo[slot].numBlocks = 1;
	    }
	}
	return;
    }
    if (size > BIN_SIZE && mem_PrintLargeAllocPC) {
	printf("malloc(%d[%d]) from PC 0x%x\n", origSize, size, callerPC);
    }
}
#endif /* MEM_TRACE */



/*
 *----------------------------------------------------------------------
 *
 * Mem_DumpTrace --
 *
 *	Dump the allocation trace records for the given size block.  If
 *	the size is specified as -1 then all trace records for all size
 *	blocks are dumped.
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
void
Mem_DumpTrace(blockSize)
    int	blockSize;
{
#ifdef MEM_TRACE
    register	struct	TraceElement	*trPtr;
    int					i, j;

    for (i = 0, trPtr = sizeTraceArray; i < numSizesToTrace; i++, trPtr++) {
	if (trPtr->traceInfo.size != blockSize && blockSize != -1) {
	    continue;
	}
	if (!(trPtr->traceInfo.flags & MEM_STORE_TRACE) ||
	    (trPtr->traceInfo.flags & MEM_TRACE_NOT_INIT)) {
	    continue;
	}

	(*memPrintProc)(memPrintData, "Trace for size = %d:\n",
				      trPtr->traceInfo.size);
	(*memPrintProc)(memPrintData, "Caller-PC      Num-blocks  \n");
	for (j = 0; j < MAX_CALLERS_TO_TRACE; j++) {
	    if (trPtr->allocInfo[j].numBlocks == 0) {
		break;
	    }
	    (*memPrintProc)(memPrintData, "%8x         %6d\n",
					  trPtr->allocInfo[j].callerPC,
					  trPtr->allocInfo[j].numBlocks);
	}
	if  (blockSize != -1) {
	    break;
	}
    }
#endif
}

