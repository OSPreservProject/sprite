/*
 * fscache.h --
 *
 *	Declarations of interface to the Sprite file system cache. 
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

#ifndef _FSCACHE
#define _FSCACHE

#include "sync.h"
#include "list.h"
#include "fs.h" /* For Handle declarations. */

/* data structures */	


/*
 * Cache information for each file.
 */

typedef struct Fscache_Attributes {
    int		firstByte;	/* Cached version of desc. firstByte */
    int		lastByte;	/* Cached version of desc. lastByte */
    int		accessTime;	/* Cached version of access time */
    int		modifyTime;	/* Cached version of modify time */
    int		createTime;	/* Create time (won't change, but passed
				 * to clients for use in
				 * statistics-gathering) */
    int		userType;	/* user advisory file type, defined in
				 * user/fs.h */
    /*
     * The following fields are needed by Proc_Exec.
     */
    int		permissions;	/* File permissions */
    int		uid;		/* User ID of owner */
    int		gid;		/* Group Owner ID */
} Fscache_Attributes;

/*
 * Block cache IO operation routines. 
 */
typedef struct Fscache_IOProcs {
    /*
     *	FooAllocate(hdrPtr, offset, bytes, flags, blockAddrPtr, newBlockPtr)
     *		Fs_HandleHeader *hdrPtr;			(File handle)
     *		int		offset;			(Byte offset)
     *		int		bytes;			(Bytes to allocate)
     *		int		flags;			(FSCACHE_DONT_BLOCK)
     *		int		*blockAddrPtr;		(Returned block number)
     *		Boolean		*newBlockPtr;		(TRUE if new block)
     *	FooBlockRead(hdrPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
     *		Fs_HandleHeader *hdrPtr;			(File handle)
     *		int		flags;		(For compatibility with .read)
     *		Address		buffer;			(Target of read)
     *		int		*offsetPtr;		(Byte offset)
     *		int		*lenPtr;		(Byte count)
     *		Sync_RemoteWaiter *waitPtr;		(For remote waiting)
     *	FooBlockWrite(hdrPtr, blockPtr, lastDirtyBlock)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		Fscache_Block	*blockPtr;		(Cache block to write)
     *		Boolean		lastDirtyBlock;		(Indicates last block)
     *	FooBlockCopy(srcHdrPtr, dstHdrPtr, blockNumber)
     *		Fs_HandleHeader	*srcHdrPtr;		(Source file handle)
     *		Fs_HandleHeader	*dstHdrPtr;		(Destination handle)
     *		int		blockNumber;		(Block to copy)
     */
    ReturnStatus (*allocate)();
    ReturnStatus (*blockRead)();
    ReturnStatus (*blockWrite)();
    ReturnStatus (*blockCopy)();
} Fscache_IOProcs;



/*
 * Structure to represent a cache block in the fileservers cache block 
 * list and the core map list.
 */
typedef struct Fscache_FileInfo {
    List_Links	   links;	   /* Links for the list of dirty files.
				      THIS MUST BE FIRST in the struct */
    List_Links	   dirtyList;	   /* List of dirty blocks for this file.
				    * THIS MUST BE SECOND, see the macro
				    * in fsBlockCache.c that depends on it. */
    List_Links	   blockList;      /* List of blocks for the file */
    List_Links	   indList;	   /* List of indirect blocks for the file */
    Sync_Lock	   lock;	   /* This is used to serialize cache access */
    int		   flags;	   /* Flags to indicate the state of the
				      file, defined below. */
    int		   version;	   /* Used to verify validity of cached data */
    struct Fs_HandleHeader *hdrPtr; /* Back pointer to I/O handle */
    int		   blocksInCache;  /* The number of blocks that this file has
				      in the cache. */
    int		   blocksWritten;  /* The number of blocks that have been
				    * written in a row without requiring a 
				    * sync of the servers cache. */
    int		   numDirtyBlocks; /* The number of dirty blocks in the cache.*/
    Sync_Condition noDirtyBlocks;  /* Notified when all write backs done. */
    int		   lastTimeTried;  /* Time that last tried to see if disk was
				    * available for this block. */
    Fscache_Attributes attr;	   /* Local version of descriptor attributes. */
    Fscache_IOProcs   *ioProcsPtr;  /* Routines for read/write/allocate/copy. */
} Fscache_FileInfo;

/*
 * Values for flags field in the Fscache_FileInfo struct.
 *
 *   FSCACHE_CLOSE_IN_PROGRESS	There is a close being done on this file so
 *				no more delayed writes are allowed.
 *   FSCACHE_SERVER_DOWN	The host that this file belongs is down.
 *   FSCACHE_NO_DISK_SPACE	The domain that this file lives in has no
 *				disk space.
 *   FSCACHE_DOMAIN_DOWN	The domain to write to is not available.
 *   FSCACHE_GENERIC_ERROR	An error occured for which we just hang onto
 *				file blocks until we can write them out.
 *   FSCACHE_SYNC_DONE		The server has been told to force all blocks
 *				for this file to disk.
 *   FSCACHE_FILE_BEING_WRITTEN	There already is a block cleaner working on
 *				this process.
 *   FSCACHE_FILE_ON_DIRTY_LIST	This file is on the dirty list.
 *   FSCACHE_FILE_IS_WRITE_THRU	(unused?) Means that there is no delayed write.
 *   FSCACHE_FILE_NOT_CACHEABLE	This is set when files served by remote hosts
 *				are no longer caching because of write sharing
 *   FSCACHE_LARGE_FILE_MODE		This file is large enough such that we limit it
 *				to only a few blocks in the cache.
 *   FSCACHE_FILE_GONE		The file has been removed and any delayed
 *				writes should be discarded.
 *   FSCACHE_WB_ON_LDB		Force this file to be written back to disk
 *				on the last dirty block.
 *   FSCACHE_ALLOC_FAILED	Allocated failed due to disk full.  This
 *				is used to throttle error messages.
 */
#define	FSCACHE_CLOSE_IN_PROGRESS		0x0001
#define	FSCACHE_SERVER_DOWN		0x0002
#define	FSCACHE_NO_DISK_SPACE		0x0004
#define FSCACHE_DOMAIN_DOWN		0x0008
#define FSCACHE_GENERIC_ERROR		0x0010
#define	FSCACHE_SYNC_DONE		0x0020
#define FSCACHE_FILE_BEING_WRITTEN		0x0040
#define	FSCACHE_FILE_ON_DIRTY_LIST		0x0080
#define FSCACHE_FILE_IS_WRITE_THRU		0x0100
#define FSCACHE_FILE_NOT_CACHEABLE		0x0200
#define	FSCACHE_LARGE_FILE_MODE		0x0400
#define FSCACHE_FILE_GONE			0x0800
#define	FSCACHE_WB_ON_LDB		0x1000
#define FSCACHE_ALLOC_FAILED		0x2000

/*
 * Structure to represent a cache block in the fileservers cache block 
 * list and the core map list.
 */
typedef struct Fscache_Block {
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
    Fscache_FileInfo *cacheInfoPtr;	/* Reference to file's cache info. */
    int		fileNum;	/* For consistency checks */
    int		blockNum;	/* The number of this block in the file. */
    int		diskBlock;	/* The block number on disk for this block. 
				   For remote blocks this equals blockNum. */
    int		blockSize;	/* The number of valid bytes in this block. */
    int		refCount;	/* Number of times that the block is referenced.
				   0 means is unreferenced. */
    Sync_Condition ioDone;	/* Notified when block is unlocked after I/O */
    int		flags;		/* Flags to indicate state of block. */
} Fscache_Block;

/* 
 * Flags for a Fscache_Block: 
 * 
 *   FSCACHE_BLOCK_FREE			The block is not being used.
 *   FSCACHE_BLOCK_ON_DIRTY_LIST	The block is on the dirty list.
 *   FSCACHE_BLOCK_BEING_WRITTEN	The block is in the process of being
 *					written to disk.
 *   FSCACHE_BLOCK_DIRTY		The block contains dirty data.
 *   FSCACHE_BLOCK_DELETED		This block has been deleted.  This
 *					flag is set when a block is to be
 *					invalidated after it has been cleaned.
 *   FSCACHE_MOVE_TO_FRONT		After this block has finished being
 *					cleaned move it to the front of the
 *					LRU list.
 *   FSCACHE_WRITE_BACK_WAIT		This block is being written out by 
 *					FsCacheWriteBack which is waiting
 *					for all such blocks to be written out.
 *   FSCACHE_BLOCK_WRITE_LOCKED		This block is being modified.
 *   FSCACHE_BLOCK_NEW			This block was just created.
 *   FSCACHE_BLOCK_CLEANER_WAITING	The block cleaner is waiting for this
 *					block to become unlocked in order to
 *					write it out.
 *   FSCACHE_NOT_MAPPED			This cache block does not have
 *					physical memory behind it.
 *   FSCACHE_IND_BLOCK			This block is an indirect block.
 *   FSCACHE_DESC_BLOCK			This block is a file descriptor block.
 *   FSCACHE_DIR_BLOCK			This is a directory block.
 *   FSCACHE_DATA_BLOCK			This is a data block.
 *   FSCACHE_READ_AHEAD_BLOCK		This block was read ahead.
 *   FSCACHE_IO_IN_PROGRESS		IO is in progress on this block.
 *   FSCACHE_DONT_BLOCK			Don't block if the cache block is
 *					already locked.	
 *   FSCACHE_PIPE_BLOCK			This is a block that is permanently
 *					locked so that it can serve as the
 *					data area for a pipe. (NOT USED)
 *   FSCACHE_WRITE_THRU_BLOCK		This block is being written through by
 *					the caller to Fscache_UnlockBlock.
 */
#define	FSCACHE_BLOCK_FREE			0x000001
#define	FSCACHE_BLOCK_ON_DIRTY_LIST		0x000002
#define	FSCACHE_BLOCK_BEING_WRITTEN		0x000004
#define	FSCACHE_BLOCK_DIRTY			0x000008
#define	FSCACHE_BLOCK_DELETED			0x000010
#define	FSCACHE_MOVE_TO_FRONT			0x000020
#define	FSCACHE_WRITE_BACK_WAIT			0x000040
#define	FSCACHE_BLOCK_WRITE_LOCKED		0x000100
#define	FSCACHE_BLOCK_NEW			0x000200
#define	FSCACHE_BLOCK_CLEANER_WAITING		0x000400
#define	FSCACHE_NOT_MAPPED			0x000800
#define	FSCACHE_IND_BLOCK			0x001000
#define	FSCACHE_DESC_BLOCK			0x002000
#define	FSCACHE_DIR_BLOCK			0x004000
#define	FSCACHE_DATA_BLOCK			0x008000
#define	FSCACHE_READ_AHEAD_BLOCK		0x010000
#define	FSCACHE_IO_IN_PROGRESS			0x020000
#define FSCACHE_DONT_BLOCK			0x040000
#define FSCACHE_PIPE_BLOCK			0x080000
#define	FSCACHE_WRITE_THRU_BLOCK		0x100000

/*
 * Macro to get the block address field of the Fscache_Block struct.
 */

#define	Fscache_BlockAddress(cacheBlockPtr) ((cacheBlockPtr)->blockAddr)

/*
 * Constant to pass to Fscache_FileWriteBack, which takes block numbers as
 * arguments.
 */
#define FSCACHE_LAST_BLOCK	-1

/*
 * Constants to pass as flags to Fscache_FileWriteBack.
 *
 *    FSCACHE_FILE_WB_WAIT		Wait for blocks to be written back.
 *    FSCACHE_WRITE_BACK_INDIRECT	Write back indirect blocks.
 *    FSCACHE_WRITE_BACK_AND_INVALIDATE	Invalidate after writing back.
 *    FSCACHE_WB_MIGRATION		Invalidation due to migration (for
 *					statistics purposes only).
 */
#define FSCACHE_FILE_WB_WAIT		0x1
#define	FSCACHE_WRITE_BACK_INDIRECT	0x2
#define	FSCACHE_WRITE_BACK_AND_INVALIDATE	0x4
#define	FSCACHE_WB_MIGRATION	0x8

/*
 * Constants to pass as flags to FsUnlockCacheBlock.
 *
 *    FSCACHE_DELETE_BLOCK	    The block should be deleted when it is unlocked.
 *    FSCACHE_CLEAR_READ_AHEAD   Clear the read ahead flag from the block.
 *    FSCACHE_BLOCK_UNNEEDED     This block is not needed anymore.  Throw it away
 *			    as soon as possible.
 *    FSCACHE_DONT_WRITE_THRU    Don't write this block through to disk.
 *
 * Also can pass one of the 4 block types defined above (0x1000 - 0x8000).
 */
#define	FSCACHE_DELETE_BLOCK			0x0001
#define	FSCACHE_CLEAR_READ_AHEAD		0x0002
#define FSCACHE_BLOCK_UNNEEDED		0x0004
#define	FSCACHE_DONT_WRITE_THRU		0x0008

/*
 * Flags for Fscache_Trunc
 *	FSCACHE_TRUNC_CONSUME	Truncate a la named pipes, consuming from front
 *	FSCACHE_TRUNC_DELETE		Truncate because the file is deleted.  This
 *				is used to prevent delayed writes during the
 *				truncation of the file.
 */
#define FSCACHE_TRUNC_CONSUME	0x1
#define FSCACHE_TRUNC_DELETE		0x2

/*
 * Read-ahead is used for both local and remote files that are cached.
 * The following structure is used to synchronize read ahead with other I/O.
 */

typedef struct Fscache_ReadAheadInfo {
    Sync_Lock		lock;		/* Access to this state is monitored */
    int			count;		/* Number of read aheads in progress */
    Boolean		blocked;	/* TRUE if read ahead is blocked */
    Sync_Condition	done;		/* Notified when there are no more read
					 * aheads in progress. */
    Sync_Condition	okToRead;	/* Notified when there are no more
					 * conflicts with read ahead. */
} Fscache_ReadAheadInfo;			/* 24 BYTES */


#ifndef CLEAN
#define FSCACHE_DEBUG_PRINT(string) \
	if (fsconsist_Debug) {\
	    printf(string);\
	}
#define FSCACHE_DEBUG_PRINT1(string, arg1) \
	if (fsconsist_Debug) {\
	    printf(string, arg1);\
	}
#define FSCACHE_DEBUG_PRINT2(string, arg1, arg2) \
	if (fsconsist_Debug) {\
	    printf(string, arg1, arg2);\
	}
#define FSCACHE_DEBUG_PRINT3(string, arg1, arg2, arg3) \
	if (fsconsist_Debug) {\
	    printf(string, arg1, arg2, arg3);\
	}
#else
#define FSCACHE_DEBUG_PRINT(string)
#define FSCACHE_DEBUG_PRINT1(string, arg1)
#define FSCACHE_DEBUG_PRINT2(string, arg1, arg2)
#define FSCACHE_DEBUG_PRINT3(string, arg1, arg2, arg3)
#endif not CLEAN

/*
 * The cache uses a number of Proc_ServerProcs to do write-backs.
 * FSCACHE_MAX_CLEANER_PROCS defines the maximum number, and this
 * is used to configure the right number of Proc_ServerProcs.
 */
#define FSCACHE_MAX_CLEANER_PROCS	3
extern int	fscache_MaxBlockCleaners;

extern Boolean	fscache_RATracing;
extern int	fscache_NumReadAheadBlocks;

extern List_Links *fscacheFullWaitList;

/* procedures */

/*
 * Block Cache routines. 
 */
extern  void            Fscache_WriteBack();
extern  ReturnStatus    Fscache_FileWriteBack();
extern  void            Fscache_FileInvalidate();
extern  void            Fscache_Empty();
extern  void            Fscache_CheckFragmentation();

extern  ReturnStatus	Fscache_CheckVersion();
extern  ReturnStatus    Fscache_Consist();

extern	void		Fscache_SetMaxSize();
extern	void		Fscache_SetMinSize();
extern	void		Fscache_BlocksUnneeded();
extern	void		Fscache_DumpStats();
extern  void		Fscache_GetPageFromFS();

extern  void            Fscache_InfoInit();
extern  void            Fscache_InfoSyncLockCleanup();
extern  void            Fscache_FetchBlock();
extern  void            Fscache_UnlockBlock();
extern  void            Fscache_BlockTrunc();
extern  void            Fscache_IODone();
extern  void            Fscache_CleanBlocks();
extern  void            Fscache_Init();
extern  int             Fscache_PreventWriteBacks();
extern  void            Fscache_AllowWriteBacks();

/*
 * Cache operations.  There are I/O operations, plus routines to deal
 * with the cached I/O attributes like access time, modify time, and size.
 */

extern void             Fscache_Trunc();
extern ReturnStatus     Fscache_Read();
extern ReturnStatus     Fscache_Write();
extern ReturnStatus     Fscache_BlockRead();

extern Boolean          Fscache_UpdateFile();
extern void             Fscache_UpdateAttrFromClient();
extern void             Fscache_UpdateAttrFromCache();
extern void             Fscache_UpdateCachedAttr();
extern void             Fscache_UpdateDirSize();
extern void             Fscache_GetCachedAttr();

extern Boolean		Fscache_AllBlocksInCache();
extern Boolean		Fscache_OkToScavenge();

/*
 * Read ahead routines.
 */
extern void		Fscache_ReadAheadInit();
extern void		Fscache_ReadAheadSyncLockCleanup();
extern void		Fscache_WaitForReadAhead();
extern void		Fscache_AllowReadAhead();

extern void		FscacheReadAhead();

#endif /* _FSCACHE */

