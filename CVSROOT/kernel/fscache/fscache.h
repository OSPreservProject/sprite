/*
 * fsBlockCache.h --
 *
 *	Declarations for the file systems block cache.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSBLOCKCACHE
#define _FSBLOCKCACHE

#include "sync.h"
#include "list.h"
#include "fs.h"

/*
 * Minimum number of cache blocks required.  The theoretical limit
 * is about 3, enough for indirect blocks and data blocks, but
 * that is a bit extreme.  The maximum number of cache blocks is
 * a function of the physical memory size and is computed at boot time.
 */
#define FS_MIN_CACHE_BLOCKS	32

/*
 * Values for flags field in the FsCacheFileInfo struct defined in fsInt.h
 *
 *   FS_CLOSE_IN_PROGRESS	There is a close being done on this file so
 *				no more delayed writes are allowed.
 *   FS_CACHE_SERVER_DOWN	The host that this file belongs is down.
 *   FS_CACHE_NO_DISK_SPACE	The domain that this file lives in has no
 *				disk space.
 *   FS_CACHE_DOMAIN_DOWN	The domain to write to is not available.
 *   FS_CACHE_GENERIC_ERROR	An error occured for which we just hang onto
 *				file blocks until we can write them out.
 *   FS_CACHE_SYNC_DONE		The server has been told to force all blocks
 *				for this file to disk.
 *   FS_FILE_BEING_WRITTEN	There already is a block cleaner working on
 *				this process.
 *   FS_FILE_ON_DIRTY_LIST	This file is on the dirty list.
 *   FS_FILE_IS_WRITE_THRU	(unused?) Means that there is no delayed write.
 *   FS_FILE_NOT_CACHEABLE	This is set when files served by remote hosts
 *				are no longer caching because of write sharing
 *   FS_LARGE_FILE_MODE		This file is large enough such that we limit it
 *				to only a few blocks in the cache.
 *   FS_FILE_GONE		The file has been removed and any delayed
 *				writes should be discarded.
 *   FS_CACHE_WB_ON_LDB		Force this file to be written back to disk
 *				on the last dirty block.
 */
#define	FS_CLOSE_IN_PROGRESS		0x0001
#define	FS_CACHE_SERVER_DOWN		0x0002
#define	FS_CACHE_NO_DISK_SPACE		0x0004
#define FS_CACHE_DOMAIN_DOWN		0x0008
#define FS_CACHE_GENERIC_ERROR		0x0010
#define	FS_CACHE_SYNC_DONE		0x0020
#define FS_FILE_BEING_WRITTEN		0x0040
#define	FS_FILE_ON_DIRTY_LIST		0x0080
#define FS_FILE_IS_WRITE_THRU		0x0100
#define FS_FILE_NOT_CACHEABLE		0x0200
#define	FS_LARGE_FILE_MODE		0x0400
#define FS_FILE_GONE			0x0800
#define	FS_CACHE_WB_ON_LDB		0x1000


/*
 * Structure to represent a cache block in the fileservers cache block
 * list and the core map list.
 */

typedef struct FsCacheBlock {
    List_Links	cacheLinks;	/* Links to put block into list of unused
				   cache blocks or LRU list of cache blocks.
				   THIS MUST BE FIRST in the struct. */
    List_Links	dirtyLinks;	/* Links to put block into list of dirty
				 * blocks for the file.  THIS MUST BE 2ND */
    List_Links	fileLinks;	/* Links to put block into list of blocks
				 * for the file.  There are two lists, either
				 * regular or for indirect blocks. */
    unsigned int timeDirtied;	/* Time in seconds that block was
				   dirtied if at all. */
    unsigned int timeReferenced;/* Time in seconds that this block was
				 * last referenced. */
    Address	blockAddr;	/* Kernel virtual address where data for
				   cache block is at. */
    FsCacheFileInfo *cacheInfoPtr;	/* Reference to file's cache info. */
    int		fileNum;	/* For consistency checks */
    int		blockNum;	/* The number of this block in the file. */
    int		diskBlock;	/* The block number on disk for this block.
				   For remote blocks this equals blockNum. */
    int		blockSize;	/* The number of valid bytes in this block. */
    int		refCount;	/* Number of times that the block is referenced.
				   0 means is unreferenced. */
    Sync_Condition ioDone;	/* Notified when block is unlocked after I/O */
    int		flags;		/* Flags to indicate state of block. */
} FsCacheBlock;

/*
 * Macros to get from the dirtyLinks of a cache block to the cache block itself.
 */
#define DIRTY_LINKS_TO_BLOCK(ptr) \
		((FsCacheBlock *) ((int) (ptr) - sizeof(List_Links)))

#define FILE_LINKS_TO_BLOCK(ptr) \
		((FsCacheBlock *) ((int) (ptr) - 2 * sizeof(List_Links)))
/*
 * Flags for a FsCacheBlock:
 *
 *   FS_BLOCK_FREE		The block is not being used.
 *   FS_BLOCK_ON_DIRTY_LIST	The block is on the dirty list.
 *   FS_BLOCK_BEING_WRITTEN	The block is in the process of being written to
 *				disk.
 *   FS_BLOCK_DIRTY		The block contains dirty data.
 *   FS_BLOCK_DELETED		This block has been deleted.  This flag is set
 *				when a block is to be invalidated after it has
 *				been cleaned.
 *   FS_MOVE_TO_FRONT		After this block has finished being cleaned
 *				move it to the front of the LRU list.
 *   FS_WRITE_BACK_WAIT		This block is being written out by
 *				FsCacheWriteBack which is waiting for all
 *				such blocks to be written out.
 *   FS_BLOCK_WRITE_LOCKED	This block is being modified.
 *   FS_BLOCK_NEW		This block was just created.
 *   FS_BLOCK_CLEANER_WAITING	The block cleaner is waiting for this
 *				block to become unlocked in order to write
 *				it out.
 *   FS_NOT_MAPPED		This cache block does not have physical memory
 *				behind it.
 *   FS_IND_CACHE_BLOCK		This block is an indirect block.
 *   FS_DESC_CACHE_BLOCK	This block is a file descriptor block.
 *   FS_DIR_CACHE_BLOCK		This is a directory block.
 *   FS_DATA_CACHE_BLOCK	This is a data block.
 *   FS_READ_AHEAD_BLOCK	This block was read ahead.
 *   FS_IO_IN_PROGRESS		IO is in progress on this block.
 *   FS_CACHE_DONT_BLOCK	Don't block if the cache block is already
 *				locked.
 *   FS_PIPE_BLOCK		This is a block that is permanently locked
 *				so that it can serve as the data area for
 *				a pipe.
 *   FS_WRITE_THRU_BLOCK	This block is being written through by the
 *				caller to FsCacheUnlockBlock.
 */
#define	FS_BLOCK_FREE			0x000001
#define	FS_BLOCK_ON_DIRTY_LIST		0x000002
#define	FS_BLOCK_BEING_WRITTEN		0x000004
#define	FS_BLOCK_DIRTY			0x000008
#define	FS_BLOCK_DELETED		0x000010
#define	FS_MOVE_TO_FRONT		0x000020
#define	FS_WRITE_BACK_WAIT		0x000040
#define	FS_BLOCK_WRITE_LOCKED		0x000100
#define	FS_BLOCK_NEW			0x000200
#define	FS_BLOCK_CLEANER_WAITING	0x000400
#define	FS_NOT_MAPPED			0x000800
#define	FS_IND_CACHE_BLOCK		0x001000
#define	FS_DESC_CACHE_BLOCK		0x002000
#define	FS_DIR_CACHE_BLOCK		0x004000
#define	FS_DATA_CACHE_BLOCK		0x008000
#define	FS_READ_AHEAD_BLOCK		0x010000
#define	FS_IO_IN_PROGRESS		0x020000
#define FS_CACHE_DONT_BLOCK		0x040000
#define FS_PIPE_BLOCK			0x080000
#define	FS_WRITE_THRU_BLOCK		0x100000

/*
 * Macro to get the block address field of the FsCacheBlock struct.
 */

#define	FsCacheBlockAddress(cacheBlockPtr) ((cacheBlockPtr)->blockAddr)

/*
 * Constant to pass to FsCacheFileWriteBack, which takes block numbers as
 * arguments.
 */
#define FS_LAST_BLOCK	-1

/*
 * Constants to pass as flags to FsCacheFileWriteBack.
 *
 *    FS_FILE_WB_WAIT		Wait for blocks to be written back.
 *    FS_FILE_WB_INDIRECT	Write back indirect blocks.
 *    FS_FILE_WB_INVALIDATE	Invalidate after writing back.
 */
#define FS_FILE_WB_WAIT		0x1
#define	FS_FILE_WB_INDIRECT	0x2
#define	FS_FILE_WB_INVALIDATE	0x4

/*
 * Constants to pass as flags to FsUnlockCacheBlock.
 *
 *    FS_DELETE_BLOCK	    The block should be deleted when it is unlocked.
 *    FS_CLEAR_READ_AHEAD   Clear the read ahead flag from the block.
 *    FS_BLOCK_UNNEEDED     This block is not needed anymore.  Throw it away
 *			    as soon as possible.
 *    FS_DONT_WRITE_THRU    Don't write this block through to disk.
 *
 * Also can pass one of the 4 block types defined above (0x1000 - 0x8000).
 */
#define	FS_DELETE_BLOCK			0x0001
#define	FS_CLEAR_READ_AHEAD		0x0002
#define FS_BLOCK_UNNEEDED		0x0004
#define	FS_DONT_WRITE_THRU		0x0008

/*
 * Global cache variables.
 */
extern	int	fsCacheDebug;		/* Debug flag */
extern	int	fsNumCacheBlocks;	/* Number of blocks in the cache */
extern	Boolean	fsLargeFileMode;	/* TRUE => are in mode where large
					 * files cannot occupy too large a
					 * portion of the cache. */
extern	int	fsMaxFilePortion;	/* Number to divide maximum number of
					 * cache blocks by determine size of
					 * file that puts it into large file
					 * mode. */
extern	int	fsMaxFileSize;		/* Maximum size of a file before
					 * changing to large file mode. */
/*
 * Cache routines.
 */
extern	void		Fs_CacheWriteBack();
extern	ReturnStatus	FsCacheFileWriteBack();
extern	void		FsCacheFileInvalidate();
extern	void		Fs_CacheEmpty();
extern	void		Fs_CheckFragmentation();
extern	void		Fs_BlockCleaner();

extern	void		FsCacheInfoInit();
extern	void		FsCacheInfoSyncLockCleanup();
extern	void		FsCacheFetchBlock();
extern	void		FsCacheUnlockBlock();
extern	void		FsCacheBlockTrunc();
extern	void		FsCacheIODone();
extern	void		FsCacheBlocksUnneeded();
extern	void		FsCleanBlocks();
extern	void		FsBlockCacheInit();
extern	int		FsPreventWriteBacks();
extern	void		FsAllowWriteBacks();

#endif /* _FSBLOCKCACHE */
