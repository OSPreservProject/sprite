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

#include <sync.h>
#include <list.h>
#include <fs.h>

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
    int		   oldestDirtyBlockTime;
    Fscache_Attributes attr;	   /* Local version of descriptor attributes. */
    struct Fscache_Backend   *backendPtr;  
			/* Routines for read/write/allocate/copy. */
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
 *   FSCACHE_WRITE_TO_DISK	Want file forced to disk.
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
 *   FSCACHE_FILE_BEING_CLEANED
 */
#define	FSCACHE_CLOSE_IN_PROGRESS	0x0001
#define	FSCACHE_SERVER_DOWN		0x0002
#define	FSCACHE_NO_DISK_SPACE		0x0004
#define FSCACHE_DOMAIN_DOWN		0x0008
#define FSCACHE_GENERIC_ERROR		0x0010
#define	FSCACHE_SYNC_DONE		0x0020
#define FSCACHE_FILE_BEING_WRITTEN	0x0040
#define	FSCACHE_FILE_ON_DIRTY_LIST	0x0080
#define FSCACHE_WRITE_TO_DISK		0x0100
#define FSCACHE_FILE_NOT_CACHEABLE	0x0200
#define	FSCACHE_LARGE_FILE_MODE		0x0400
#define FSCACHE_FILE_GONE		0x0800
#define	FSCACHE_WB_ON_LDB		0x1000
#define FSCACHE_ALLOC_FAILED		0x2000
#define	FSCACHE_FILE_FSYNC		0x4000
#define	FSCACHE_FILE_DESC_DIRTY		0x8000
#define	FSCACHE_FILE_BEING_CLEANED     0x10000

/*
 * Structure to represent a cache block in the fileservers cache block 
 * list and the core map list.
 */
typedef struct Fscache_Block {
    List_Links	dirtyLinks;	/* Links to put block into list of dirty
				 * blocks for the file.  THIS MUST BE FIRST 
				 * in the struct. It may be used by the
				 * cache backend's after a Fetch_DirtyBlock
				 * call.  */
    List_Links	useLinks;	/* Links to put block into list of unused 
				   cache blocks or LRU list of cache blocks.
				   THIS MUST BE SECOND in the struct. */
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
 *   FSCACHE_CANT_BLOCK
 *   FSCACHE_DONT_BLOCK			Don't block if the cache block is
 *					already locked.	
 *   FSCACHE_PIPE_BLOCK			This is a block that is permanently
 *					locked so that it can serve as the
 *					data area for a pipe. (NOT USED)
 *   FSCACHE_WRITE_THRU_BLOCK		This block is being written through by
 *					the caller to Fscache_UnlockBlock.
 *   FSCACHE_BLOCK_BEING_CLEANED        The block is being cleaned.
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
#define	FSCACHE_CANT_BLOCK			0x200000
#define	FSCACHE_BLOCK_BEING_CLEANED		0x400000


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
 *    FSCACHE_WRITE_BACK_DESC_ONLY 	Only writeback the descriptor.
 */
#define FSCACHE_FILE_WB_WAIT		0x1
#define	FSCACHE_WRITE_BACK_INDIRECT	0x2
#define	FSCACHE_WRITE_BACK_AND_INVALIDATE	0x4
#define	FSCACHE_WB_MIGRATION	0x8
#define	FSCACHE_WRITE_BACK_DESC_ONLY	0x10

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
 *	FSCACHE_TRUNC_DELETE	Truncate because the file is deleted.  This
 *				is used to prevent delayed writes during the
 *				truncation of the file.
 */
#define FSCACHE_TRUNC_DELETE		0x1

typedef struct Fscache_BackendRoutines {
    /*
     *	FooAllocate(hdrPtr, offset, bytes, flags, blockAddrPtr, newBlockPtr)
     *		Fs_HandleHeader *hdrPtr;			(File handle)
     *		int		offset;			(Byte offset)
     *		int		bytes;			(Bytes to allocate)
     *		int		flags;			(FSCACHE_DONT_BLOCK)
     *		int		*blockAddrPtr;		(Returned block number)
     *		Boolean		*newBlockPtr;		(TRUE if new block)
     *	FooTruncate(hdrPtr, size, delete)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		int		size;			(New size)
     *		Boolean		delete;			(TRUE if file being 
     *							 removed)
     *	FooBlockRead(hdrPtr, blockPtr,remoteWaitPtr)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		Fscache_Block	*blockPtr;		(Cache block to read)
     *		Sync_RemoteWaiter *remoteWaitPtr;	(For remote waiting)
     *	FooBlockWrite(hdrPtr, blockPtr, lastDirtyBlock)
     *		Boolean		lastDirtyBlock;		(Indicates last block)
     *	FooReallocBlock(data, callInfoPtr)
     *		ClientData	data = 	blockPtr;  (Cache block to realloc)
     *		Proc_CallInfo	*callInfoPtr;	
     *	FooStartWriteBack(backendPtr)
     *        Fscache_Backend *backendPtr;	(Backend to start writeback.)
     */ 
    ReturnStatus (*allocate) _ARGS_((Fs_HandleHeader *hdrPtr, int offset,
				    int numBytes, int flags, int *blockAddrPtr,
				    Boolean *newBlockPtr));
    ReturnStatus (*truncate) _ARGS_((Fs_HandleHeader *hdrPtr, int size, 
				     Boolean delete));
    ReturnStatus (*blockRead) _ARGS_((Fs_HandleHeader *hdrPtr, 
				      Fscache_Block *blockPtr, 
				      Sync_RemoteWaiter *remoteWaitPtr));
    ReturnStatus (*blockWrite) _ARGS_((Fs_HandleHeader *hdrPtr, 
				       Fscache_Block *blockPtr, int flags));
    void	 (*reallocBlock) _ARGS_((ClientData data, 
					 Proc_CallInfo *callInfoPtr));

    ReturnStatus (*startWriteBack) _ARGS_((struct Fscache_Backend *backendPtr));
} Fscache_BackendRoutines;


/*
 * Routines and data structures defining a backend to the cache. 
 */
typedef struct Fscache_Backend {
    List_Links	cacheLinks;	/* Used by fscacheBlocks.c to link backends
				 * onto a list. Must be first in structure! */
    int		flags;		/* See below. */
    ClientData	clientData;	/* ClientData for the backend. */
    List_Links	dirtyListHdr;   /* List of dirty files for this backend. */

    Fscache_BackendRoutines ioProcs; /* Routines for backend. */
} Fscache_Backend;


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

#define	FSCACHE_NUM_DOMAIN_TYPES	2

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
extern void Fscache_WriteBack _ARGS_((unsigned int writeBackTime,
			int *blocksSkippedPtr, Boolean writeBackAll));
extern ReturnStatus Fscache_FileWriteBack _ARGS_((
			Fscache_FileInfo *cacheInfoPtr, int firstBlock, 
			int lastBlock, int flags, int *blocksSkippedPtr));
extern void Fscache_FileInvalidate _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
			int firstBlock, int lastBlock));
extern void Fscache_Empty _ARGS_((int *numLockedBlocksPtr));
extern void Fscache_CheckFragmentation _ARGS_((int *numBlocksPtr, 
			int *totalBytesWastedPtr, int *fragBytesWastedPtr));

extern ReturnStatus Fscache_CheckVersion _ARGS_((
			Fscache_FileInfo *cacheInfoPtr, int version, 
			int clientID));
extern ReturnStatus Fscache_Consist _ARGS_((
			Fscache_FileInfo *cacheInfoPtr, int flags, 
			Fscache_Attributes *cachedAttrPtr));

extern void Fscache_SetMinSize _ARGS_((int minBlocks));
extern void Fscache_SetMaxSize _ARGS_((int maxBlocks));
extern void Fscache_BlocksUnneeded _ARGS_((Fs_Stream *streamPtr,
				int offset, int numBytes, Boolean objectFile));
extern void Fscache_DumpStats _ARGS_((void));
extern void Fscache_GetPageFromFS _ARGS_((int timeLastAccessed, 
				int *pageNumPtr));

extern void Fscache_FileInfoInit _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		Fs_HandleHeader *hdrPtr, int version, Boolean cacheable,
		Fscache_Attributes *attrPtr, Fscache_Backend *backendPtr));
extern void Fscache_InfoSyncLockCleanup _ARGS_((Fscache_FileInfo *cacheInfoPtr));
extern Fscache_Backend *Fscache_RegisterBackend _ARGS_((
		Fscache_BackendRoutines *ioProcsPtr, ClientData clientData, 
		int flags));
extern void Fscache_UnregisterBackend _ARGS_((Fscache_Backend *backendPtr));

extern void Fscache_FetchBlock _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		int blockNum, int flags, Fscache_Block **blockPtrPtr, 
		Boolean *foundPtr));
extern void Fscache_IODone _ARGS_((Fscache_Block *blockPtr));
extern void Fscache_UnlockBlock _ARGS_((Fscache_Block *blockPtr, 
		unsigned int timeDirtied, int diskBlock, int blockSize, 
		int flags));
extern void Fscache_BlockTrunc _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		int blockNum, int newBlockSize));

extern void Fscache_Init _ARGS_((int blockHashSize));
extern int Fscache_PreventWriteBacks _ARGS_((Fscache_FileInfo *cacheInfoPtr));
extern void Fscache_AllowWriteBacks _ARGS_((Fscache_FileInfo *cacheInfoPtr));
extern Fscache_FileInfo *Fscache_GetDirtyFile _ARGS_((
			Fscache_Backend *backendPtr, Boolean fsyncOnly, 
			Boolean (*fileMatchProc)(), ClientData clientData));
extern void Fscache_ReturnDirtyFile _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		Boolean onFront));
extern Fscache_Block *Fscache_GetDirtyBlock _ARGS_((
		Fscache_FileInfo *cacheInfoPtr, 
		Boolean (*blockMatchProc)(Fscache_Block *blockPtr, 
					  ClientData clientData), 
		ClientData clientData, int *lastDirtyBlockPtr));
extern void Fscache_ReturnDirtyBlock _ARGS_((Fscache_Block *blockPtr,
			ReturnStatus status));
extern ReturnStatus Fscache_PutFileOnDirtyList _ARGS_((
			Fscache_FileInfo *cacheInfoPtr, int flags));
extern ReturnStatus Fscache_RemoveFileFromDirtyList _ARGS_((
			Fscache_FileInfo *cacheInfoPtr));

/*
 * Cache operations.  There are I/O operations, plus routines to deal
 * with the cached I/O attributes like access time, modify time, and size.
 */

extern ReturnStatus Fscache_Read _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		int flags, register Address buffer, int offset, int *lenPtr,
		Sync_RemoteWaiter *remoteWaitPtr));
extern ReturnStatus Fscache_Write _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		int flags,  Address buffer, int offset, int *lenPtr, 
		Sync_RemoteWaiter *remoteWaitPtr));
extern ReturnStatus Fscache_BlockRead _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		int blockNum, Fscache_Block **blockPtrPtr, int *numBytesPtr, 
		int blockType, Boolean allocate));
extern ReturnStatus Fscache_Trunc _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
					int length, int flags));


extern Boolean Fscache_UpdateFile _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		Boolean openForWriting, int version, Boolean cacheable, 
		Fscache_Attributes *attrPtr));
extern void Fscache_UpdateAttrFromClient _ARGS_((int clientID, 
		Fscache_FileInfo *cacheInfoPtr,  Fscache_Attributes *attrPtr));
extern void Fscache_UpdateDirSize _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		int newLastByte));
extern void Fscache_UpdateAttrFromCache _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		Fs_Attributes *attrPtr));
extern void Fscache_GetCachedAttr _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		int *versionPtr, Fscache_Attributes *attrPtr));
extern void Fscache_UpdateCachedAttr _ARGS_((Fscache_FileInfo *cacheInfoPtr,
		Fs_Attributes *attrPtr, int flags));


extern Boolean Fscache_OkToScavenge _ARGS_((Fscache_FileInfo *cacheInfoPtr));


extern int Fscache_ReserveBlocks _ARGS_((Fscache_Backend *backendPtr, 
			int numResBlocks, int numNonResBlocks));

extern void Fscache_ReleaseReserveBlocks _ARGS_((Fscache_Backend *backendPtr,
			int numBlocks));

/*
 * Read ahead routines.
 */

extern void Fscache_ReadAheadInit _ARGS_((Fscache_ReadAheadInfo *readAheadPtr));
extern void Fscache_ReadAheadSyncLockCleanup _ARGS_((
			Fscache_ReadAheadInfo *readAheadPtr));
extern void FscacheReadAhead _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
				int blockNum));

extern void Fscache_WaitForReadAhead _ARGS_((
			Fscache_ReadAheadInfo *readAheadPtr));
extern void Fscache_AllowReadAhead _ARGS_((Fscache_ReadAheadInfo *readAheadPtr));

extern void FscacheReadAhead _ARGS_((Fscache_FileInfo *cacheInfoPtr,
			int blockNum));


extern void FscacheBackendIdle _ARGS_((Fscache_Backend *backendPtr));
extern void FscacheFinishRealloc _ARGS_((Fscache_Block *blockPtr, 
			int diskBlock));

extern void Fscache_CountBlocks _ARGS_((int serverID, int majorNumber,
			int *numBlocksPtr, int *numDirtyBlocksPtr));

#endif /* _FSCACHE */

