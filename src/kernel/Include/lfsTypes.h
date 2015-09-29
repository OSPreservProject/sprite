/*
 * lfsTypes.h --
 *
 *	Type definitions for the LFS module.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/kernel/Cvsroot/kernel/lfs/lfsTypes.h,v 1.2 92/09/03 18:13:32 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _LFSTYPES
#define _LFSTYPES

#include <lfsDesc.h>
#include <lfsDescMap.h>
#include <lfsDirOpLog.h>
#include <lfsUsageArray.h>
#include <lfsFileLayout.h>
#include <lfsSuperBlock.h>
#include <lfsStats.h>

/* Types from LfsDescInt.h */
/*
 * LfsDescCache - Data structure defining the cache of file descriptor blocks
 *                maintained by a LFS file system.  The current implementation
 *                caches descriptor map blocks in files the file cache.
 */

typedef struct LfsDescCache {
    Fsio_FileIOHandle handle; /* File handle use to cache descriptor
                               * block under. */
} LfsDescCache;

/* Types from LfsDirLogInt.h */
typedef struct LfsDirLog {
    int                 nextLogSeqNum;  /* The next log sequence number to be
                                         * allocated. */
    LfsDirOpLogBlockHdr *curBlockHdrPtr; /* The log block header of the block
                                         * current being filled in. */
    char                *nextBytePtr;   /* The next available byte in the
                                         * block being filled in. */
    int                 bytesLeftInBlock;/* Number of bytes left in the
                                          * block being filled in. */
    List_Links          activeListHdr;   /* List cache blocks of log blocks. */
    List_Links          writingListHdr;   /* List cache blocks of log blocks
                                           * being written. */
    Fsio_FileIOHandle   handle;          /* File handle used to cache blocks
                                          * under. */
    int                 leastCachedSeqNum; /* The least log sequence number in
                                            * the in memory log. */
    Boolean             paused;         /* Log traffic is currently paused. */
    Sync_Condition      logPausedWait;  /* Wait for paused to become false. */
} LfsDirLog; 

/* Types from LfsStableMem.h */
typedef struct LfsStableMem {
    struct Lfs        *lfsPtr;          /* File system for stable memory. */
    Fsio_FileIOHandle dataHandle;       /* Handle used to store blocks in
                                         * cache under. */
    LfsDiskAddr *blockIndexPtr;         /* Index of current disk addresses. */
    int         numCacheBlocksOut;      /* The number of cache blocks currently
                                         * fetched by the backend. */
    LfsStableMemCheckPoint checkPoint; /* Data to be checkpoint. */
    LfsStableMemParams params;  /* A copy of the parameters of the index. */
} LfsStableMem;

typedef struct LfsStableMemEntry {
    Address     addr;                   /* Memory address of entry. */
    Boolean     modified;               /* TRUE if the entry has been
                                         * modified. */
    int         blockNum;               /* Block number of entry. */
    ClientData  clientData;             /* Clientdata maintained by
                                         * StableMem code. */
} LfsStableMemEntry;

/* Types from LfsDescMapInt.h */
typedef struct LfsDescMap {
    LfsStableMem        stableMem;/* Stable memory supporting the map. */
    LfsDescMapParams    params;   /* Map parameters taken from super block. */
    LfsDescMapCheckPoint checkPoint; /* Desc map data written at checkpoint. */
} LfsDescMap;

/* Types from lfsSegUsageInt.h */
typedef struct LfsSegUsage {
    LfsStableMem        stableMem;/* Stable memory supporting the map. */
    LfsSegUsageParams   params;   /* Map parameters taken from super block. */
    LfsSegUsageCheckPoint checkPoint; /* Desc map data written at checkpoint. */
    int                 timeOfLastWrite; /* Time of last write of current
                                          * segment. */
} LfsSegUsage;

typedef struct LfsSegList {
    int segNumber;      /* Segment number of segment. */
    int activeBytes;    /* Active bytes from the seg usage array. */
    unsigned int priority;      /* Priority for the space-time sorting. */
} LfsSegList;

/* Types from LfsFileLayoutInt.h */
typedef struct LfsFileLayout {
    LfsFileLayoutParams  params;        /* File layout description. */
} LfsFileLayout;

/* Types from LfsMemInt.h */
/*
 * LfsMem - Per LFS file system resource list.
 */
typedef struct LfsMem {
    int cacheBlocksReserved; /* Number of cache blocks reserved for this file
                              * system. */
} LfsMem;

/* Types from LfsInt.h */

/*
 * LfsCheckPoint contains the info and memory needed to perform checkpoints.
 * The file system timestamp and the next checkpoint area to write
 * indicator are kept here. 
 */
typedef struct LfsCheckPoint {
    int	  timestamp;	/* Current file system timestamp. */
    int	  nextArea;	/* Next checkpoint area to write. Must be 0 or 1. */
    char  *buffer;	/* Memory buffer to place checkpoint. */
    int	  maxSize;	/* Maximum size of the buffer. */
} LfsCheckPoint;

/*
 * LfsSegCache - Data structure describing the in memory cache of segments.  
 * With the current implementation, this cache contains the last segment
 * read during cleaning and it takes any hits during cleaning.
 */
typedef struct LfsSegCache {
    Boolean valid;	        /* TRUE if the cache contains valid data. */
    int	    segNum;		/* The segment number being cached. */
    LfsDiskAddr  startDiskAddress;  /* The starting and ending disk address */
    LfsDiskAddr  endDiskAddress;    /* of the cached segment.  */
    char    *memPtr;		/* Memory location of segment. */
} LfsSegCache;

/*
 * Lfs - The main data structure describing an LFS file system.
 */
typedef struct Lfs {
		/*
		 * Fields that are set at attach time and then read-only
		 * until detach.
		 */
    Fs_Device	  *devicePtr;	/* Device containing file system. */
    char	  *name;	/* Name used for error messages. */
    int		  controlFlags;	/* Flags controlling file system operating. 
				 * see lfs.h for definitions.  */
    Fsdm_Domain	  *domainPtr;	/* Domain this file system belongs. */
    /*
     * Routine for cache backend. 
     */
    Sync_Lock	  cacheBackendLock; /* Lock for cache backend use. */
    Boolean	   writeBackActive; /* TRUE if cache backend is active. */
    Boolean 	   writeBackMoreWork; /* TRUE if more work is available for
				       * the cache backend. */
    Boolean	   shutDownActive;   /* TRUE if the file system is being 
				      * shutdown. */
    int		  cacheBlocksReserved; /* Number of file cache blocks
					* reserved for file system. */
    int	   	  attachFlags;	/* Flags from Lfs_AttachDisk() call.  */
    int	      	  blockSizeShift;   /* Log base 2 of blockSize. Used by
				     * Blocks<->Bytes macros below to 
				     * use fast shifts rather than costly 
				     * multiplies and divides. */
    int	*checkpointIntervalPtr; /* A pointer to the interval to call
				* the checkpoint processor on. A 
				* value of zero will cause the 
				* checkpoint process to stop. */
		/*
		 * Fields modified after boot that require locking.
		 */
    Sync_Lock      lock;	/* Lock protecting the below data structures. */
    int		activeFlags;	/* Flags specifing what processes are active
				 * on file system. See below for values. */
    Proc_ControlBlock *cleanerProcPtr; /* Process Control block of cleaner
					* process. NIL if cleaner is not
					* active. */
    Sync_Condition writeWait; /* Condition to wait for the file system 
			       * write to complete. */
    Sync_Condition cleanSegmentsWait; /* Condition to wait for clean
				       * segments to be generated. */
    Sync_Condition checkPointWait; /* Condition to wait for checkpoint
				    * completing or starting. */
    int		dirModsActive;	/* Number of processes inside directory 
				 * modification code. */
    int		numDirtyBlocks; /* Estimate of the number of dirty blocks
				 * in the file cache. */
    LfsSegCache   segCache;	  /* Cache of recently read segments. */
    LfsDescCache  descCache;	  /* Cache of file desciptors. */
    Sync_Lock     logLock;	/* Lock protecting the directory log. */
    LfsDirLog	  dirLog;	 /* Directory change log data structures. */
    Boolean	  segMemInUse;	/* TRUE if segment memory is being used. */
    LfsDescMap	  descMap;	/* Descriptor map data. */
    LfsSegUsage   usageArray;   /* Segment usage array data. */
    Sync_Lock     checkPointLock; /* Lock protecting the checkpoint data. */
    LfsCheckPoint checkPoint;   /* Checkpoint data. */
    LfsFileLayout fileLayout;	/* File layout data structures. */
    LfsSuperBlock superBlock;	/* Copy of the file system's super block 
				 * read at attach time. */
    Lfs_Stats	stats;		/* Stats on the file system.  */
    /*
     * Segment data structures. Currently three segments are 
     * preallocated: one for writing, one for cleaning, and one for the
     * checkpoint processes.
     */
#define	LFS_NUM_PREALLOC_SEGS	3

    int		segsInUse;
    struct LfsSeg *segs;
    char   *writeBuffers[2];    /* Buffers used to speed segment writes. */
    LfsMem	mem;		/* Memory resources allocated to file system. */
} Lfs;

#endif /* _LFSTYPES */

