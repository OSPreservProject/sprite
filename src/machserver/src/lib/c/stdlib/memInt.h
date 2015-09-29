/*
 * memInt.h --
 *
 *	Defines things that are shared across the procedures that
 *	do dynamic storage allocation (malloc, free, etc) but aren't
 *	visible to users of the storage allocator.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /r3/kupfer/spriteserver/src/lib/c/stdlib/RCS/memInt.h,v 1.2 91/12/12 21:56:10 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _MEMINT
#define _MEMINT

#include <sync.h>
#include <stdlib.h>

#include <cfuncproto.h>

/*
 * If the MEM_TRACE flag is defined, extra code and data structures will
 * be compiled to allow programs to trace malloc and free calls. 
#define MEM_TRACE
*/

/*
 * WARNING: several of the macros in this file expect both integers and
 * pointers to be 32 bits (4 bytes) long.
 */

/*
 * This storage allocator consists of two independent allocators,
 * one used for binned objects and the other used for large objects.
 * Objects <= SMALL_SIZE are automatically binned, and all objects
 * larger than BIN_SIZE are allocated with the large object
 * allocator.  Objects in between these two sizes are normally
 * allocated with the large allocator, but may be binned by calling
 * Mem_Bin.
 *
 * Both allocators deal with "blocks" of storage, which correspond
 * roughly to the areas that users request and free.  Each block of
 * storage consists of an administrative area (usually a single word)
 * followed by the useable area.  All allocations are done in even
 * numbers of words, which means that clients may occasionally get more
 * bytes than they asked for.  Malloc returns the address of the word
 * just after the administrative area.  The first word of the administrative
 * area is called the administrative word;  it allows us to keep track of
 * which blocks are in use and which are free, and also to keep track of
 * the sizes of blocks so clients don't have to know how large a block is
 * when they free it.  The low-order bit of the administrative word
 * indicates whether or not the block is free.  The next-to-low-order bit
 * of the administrative word is used to indicate that this is a "dummy
 * block":  for blocks on the un-binned list, this means that the block
 * marks an area not belonging to the allocator;  for blocks returned to
 * the "free" procedure, this means that the block must go back to the
 * binned allocator.  The rest of the word tells how large the block is
 * (in bytes including the administrative area).  Using the low-order bits
 * for flags works because all blocks that we allocate are always multiples
 * of 4 bytes in length.  There is one exception to this usage of the
 * administrative word, which occurs for free binned objects.  In this
 * case the word is a link to the next free binned object instead of a
 * size.  If tracing is enabled, then the administrative area also contains
 * a couple of other fields in addition to the administrative word.  The
 * macros below define the format of the administrative area and how to
 * access it.
 */

#ifdef MEM_TRACE

typedef struct {
    int		admin;		/* Administrative word. */
    Address	pc;		/* PC of call to malloc. */
    int		origSize;	/* Original size given to malloc. */
    int		padding;	/* To make it double word aligned. */
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

#endif MEM_TRACE

#define USE_BIT		1
#define DUMMY_BIT	2
#define FLAG_BITS	3
#define ADMIN_BITS(admin)	((admin) & FLAG_BITS)
#define IS_IN_USE(admin)	((admin) & USE_BIT)
#define IS_DUMMY(admin)		(((admin) & FLAG_BITS) == (USE_BIT|DUMMY_BIT))
#define MARK_DUMMY(admin)	((admin) | (USE_BIT|DUMMY_BIT))
#define MARK_IN_USE(admin)	((admin) | USE_BIT)
#define MARK_FREE(admin)	((admin) & ~FLAG_BITS)
#define SIZE(admin)		((admin) & ~FLAG_BITS)

/*
 * The memory manager is a monitor.  The following definition is
 * for its monitor lock.
 */

#if defined(LIBC_USERLOCK) || defined(SPRITED)
extern Sync_Lock memMonitorLock;
#define LOCKPTR (&memMonitorLock)
#endif


/*
 * ----------------------------------------------------------------------------
 *
 *	Binned-object allocator: it keeps a separate linked free list
 *	for each size of object less than SMALL_SIZE bytes in size and
 *	a free list for all objects less than BIN_SIZE that have been 
 *	specified as being binned by a call to Mem_Bin.  The blocks on
 *	each list are linked together via their administrative words
 *	terminated by a NULL pointer.  Allocation and freeing are done in
 *	a stack-like manner from the front of the free lists.
 *
 *	The following definitions are related:  if you change one, you'll
 *	have to change several:
 *
 *	GRANULARITY: The difference in size between succesive buckets.  This
 *		is determined by data alignment restrictions and should be
 *		a power of 2 to allow for shifting to map from size to bucket.
 *	SIZE_SHIFT: The shift distance to convert between size and bucket.
 *	SMALL_SIZE:		largest blocksize (total including
 *				administrative word) managed by default by
 *				the binned-object allocator.
 *	BIN_SIZE:		largest possible that can be binned.
 *
 *	These can be computed from the above constants.
 *
 *	SMALL_BUCKETS:		number of free lists corresponding to
 *				sizes <= SMALL_SIZE.
 *	BIN_BUCKETS:		total number of binned free lists.
 *	BYTES_TO_BLOCKSIZE:	how many bytes a block must contain (total
 *				including admin. word) to satisfy a user
				request in bytes.
 *	BLOCKSIZE_TO_INDEX:	which free list to use for blocks of a given
 *				size (total including administrative word).
 *	INDEX_TO_BLOCKSIZE: the (maximum) size corresponding to a bucket.
 *		This will include the size of the administrative word.
 *
 *	This is independent.
 *
 *	BLOCKS_AT_ONCE:		when allocating a new region to hold blocks of
 *				a given size, how many blocks worth should
 *				be allocated at once.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * 8 byte granularity to handle SPUR and SPARC.
 */
#define GRANULARITY			8
#define SIZE_SHIFT			3
/*
 * These sizes are optimized towards the kernels memory usage, 1/89.
 */
#define SMALL_SIZE			271
#define	BIN_SIZE			4199

#define SMALL_BUCKETS			((SMALL_SIZE+GRANULARITY-1)/GRANULARITY)
#define BIN_BUCKETS			((BIN_SIZE+GRANULARITY-1)/GRANULARITY)
#define MASK				(GRANULARITY-1)
#define BYTES_TO_BLOCKSIZE(bytes) ((bytes+MASK+sizeof(AdminInfo)) & (~MASK))
#define BLOCKSIZE_TO_INDEX(size)  ((size)>>SIZE_SHIFT)
#define INDEX_TO_BLOCKSIZE(index) ((index)<<SIZE_SHIFT)

#define BLOCKS_AT_ONCE			16

/*
 * The following array holds pointers to the first free blocks of each size.
 * Bucket i holds blocks whose total length is 4*i bytes (including the
 * administrative info).  Blocks from bucket i are used to satisfy user
 * requests ranging from (4*i)-sizeof(AdminInfo) bytes up to (4*i)-1 bytes.  
 * Buckets 0 and 1 are never used, since they correspond to blocks too small
 * to hold any information for the user.  If the list head is NOBIN, it
 * means that this size isn't binned:  use the large block allocator.
 */

extern Address	memFreeLists[BIN_BUCKETS];
#define NOBIN	((Address) -1)

/*
 * Used to gather statistics about allocations:
 */

extern int mem_NumBlocks[];		/* Total number of existing blocks,
					 * both allocated and free, for each
					 * small size.  */
extern int mem_NumBinnedAllocs[];	/* Total number of allocation requests
					 * made for blocks of each small size.
					 */

/*
 * ----------------------------------------------------------------------------
 *	Large-object allocator:  used for blocks that aren't binned.
 *	There should be relatively few allocations made by the
 *	large-object allocator.  Since all the blocks it allocates are
 *	relatively large, fragmentation should be less than it would be
 *	if it allocated small blocks too. All of the blocks, in use or
 *	not, are kept in a single linked list using their administrative
 *	words.  The address of the next block in the list is found by
 *	adding the size of the current block to the address of its
 *	administrative word.  Things are complicated slightly because the
 *	storage area managed by this allocator may be discontiguous:  there
 *	could be several large regions that it manages, separated by gaps
 *	that are managed by some other storage allocator (e.g. the
 *	binned-object allocator).  In this case, the gaps between regions
 *	are handled by making the gaps look like allocated blocks, with an
 *	administrative word at the end of one region that causes a skip to
 *	the beginning of the next region.  These blocks are called "dummy
 *	blocks", and have special flag bits in their administrative words.
 *	All the blocks within a region are linked together in increasing
 *	order of address.  However, the different regions may appear in any
 *	order on the list.
 * ----------------------------------------------------------------------------
 */

extern Address memFirst;		/* Pointer to first block of memory
					 * in the un-binned pool. */
extern Address memLast;			/* Points to dummy block at the end
					 * of the memory list, which always
					 * has zero size. */
extern Address memCurrentPtr;		/* Next block to consider allocating.
					 * Rotates circularly through free
					 * list. */
extern int memLargestFree;		/* This holds the size of the largest
					 * free block that's been seen on the
					 * free list.  It's updated as the
					 * list is traversed.  */
extern int memBytesFreed;		/* Sum of unbinned bytes freed since
					 * the last time we started scanning
					 * the memory list. */

/*
 * Minimum size of new regions requested by large-object allocator
 * when storage runs out:
 */

#define MIN_REGION_SIZE 2048

/*
 * Various stats about large-block allocator:  some are only kept when
 * tracing is enabled.
 */

extern int mem_NumLargeBytes;	/* Total amount of storage managed by
				 * the large allocator.  */
extern int mem_NumLargeAllocs;	/* Total number of allocation requests
				 * handled by the large allocator.  */
extern int mem_NumLargeLoops;	/* Number of iterations through the
				 * inner block-checking loop of the
				 * large allocator.  */

/*
 * ----------------------------------------------------------------------------
 *	Miscellaneous declarations.
 * ----------------------------------------------------------------------------
 */

/*
 * Statistics about total calls to malloc and free.
 */

extern int	mem_NumAllocs;
extern int	mem_NumFrees;

/*
 * Flag to make sure MemInit gets called once.
 */

extern int	memInitialized;

/*
 * If tracing is enabled, then when malloc or free is called a
 * trace record will be printed and/or the number of blocks being used
 * by the caller will be stored in the trace array if the block size is
 * in sizeTraceArray. The array is defined with Mem_SetTraceSizes().
 * PrintTrace calls a default printing routine; it can be changed with
 * Mem_SetPrintProc().
 */
#define MAX_NUM_TRACE_SIZES	8
#define	MAX_CALLERS_TO_TRACE	16
typedef struct {
    Mem_TraceInfo	traceInfo;
    struct {
	Address	callerPC;
	int	numBlocks;
    } allocInfo[MAX_CALLERS_TO_TRACE];
} MemTraceElement;

extern MemTraceElement	memTraceArray[];
extern	int		memNumSizesToTrace;

/*
 * Routine to do printing for tracing and status printing.  Can be
 * reset with Mem_SetPrintProc.
 */

extern int		(*memPrintProc)(); /* e.g., fprintf */
extern ClientData	memPrintData;

/*
 * Can free storage be freed a second time?
 */

extern int		memAllowFreeingFree;

/*
 * Various procedures used internally by the allocator.
 */

#if defined(KERNEL) || defined(SPRITED)
extern Address	MemChunkAlloc _ARGS_((int size, Address *addressPtr));
#else
extern Address	MemChunkAlloc _ARGS_((int size));
#endif

extern void	MemDoTrace _ARGS_((Boolean allocated, Address infoPtr,
				   Address curPC, int size));
extern void	MemInit _ARGS_((void));

#endif _MEMINT
