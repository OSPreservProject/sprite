/*
 * fsStat.h --
 *
 *	Declarations for the file system statistics.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSSTAT
#define _FSSTAT

typedef struct FsGeneralStats {
    int physBytesRead;		/* Number of physical (ie descriptors) disk
				 * bytes read. */
    int fileBytesRead;		/* Number of file bytes read from the disk */
    int fileReadOverflow;	/* Extra */
    int remoteBytesRead;	/* Number of bytes read from un-cachable
   				 * remote files */
    int remoteReadOverflow;	/* Extra */
    int deviceBytesRead;	/* Number of bytes read from devices */

    int physBytesWritten;	/* Number of physical disk bytes written */
    int fileBytesWritten;	/* Number of file bytes written to the disk */
    int fileWriteOverflow;	/* Extra */
    int remoteBytesWritten;	/* Number of bytes written to un-cachable
   				 * remote files */
    int remoteWriteOverflow;	/* Extra */
    int deviceBytesWritten;	/* Number of bytes written to devices */
} FsGeneralStats;

/*
 * Cache statistics.
 */
typedef struct {
    /*
     * Read statistics.
     */
    int	readAccesses;		/* Read fetches on the cache. */
    int bytesRead;		/* Total bytes read from the cache */
    int bytesReadOverflow;	/* Extra */
    int	readHitsOnDirtyBlock;	/* Fetches that hit on a block that had
				 * dirty data in it. */
    int	readHitsOnCleanBlock;	/* Fetches that hit on a block that had
				 * clean data in it. */
    int	readZeroFills;		/* Blocks that are zero filled because read
				 * did not fill the whole block. */
    int	domainReadFails;	/* Number of domain reads from FsCacheWrite
				 * and FsCacheRead that were unsuccessful. */
    int	readAheads;		/* Number of read ahead processes that were
				 * started up. */
    int	readAheadHits;		/* Number of reads that hit on a block that
				 * was read ahead. */
    int	allInCacheCalls;	/* Number of time FsAllInCache routine was
				 * called. */
    int	allInCacheTrue;		/* Number of times that FsAllInCache returned
				 * TRUE. */
    /*
     * Write statistics.
     */
    int	writeAccesses;		/* Number of write fetches on the cache. */
    int bytesWritten;		/* Total bytes written to the cache */
    int bytesWrittenOverflow;	/* Extra */
    int	appendWrites;		/* Blocks written in append mode. */
    int	overWrites;		/* Cache blocks that were overwritten. */
    int	writeZeroFills1;	/* Blocks that are zero filled because read
				 * of old block did not fill whole block. */
    int	writeZeroFills2;	/* Blocks that are zero filled because user
				 * is only doing a partial write to a new 
				 * block. */
    int	partialWriteHits;	/* Read hits in the cache when are writing to 
				 * the middle of a block and have to read in 
				 * the data that is already there. */
    int	partialWriteMisses;	/* Misses for above case. */
    int	blocksWrittenThru;	/* Number of dirty blocks that were written
				 * thru. */
    int	dataBlocksWrittenThru;	/* Number of data block that were written
				 * thru. */
    int	indBlocksWrittenThru;	/* Number of indirect blocks that were written
				 * thru. */
    int	descBlocksWrittenThru;	/* Number of descriptor blocks that were written
				 * thru. */
    int	dirBlocksWrittenThru;	/* Number of directory blocks that were written
				 * thru. */
    /*
     * Fragment statistics.
     */
    int	fragAccesses;		/* Cache blocks that were fetched in order
				 * to upgrade fragments. */
    int	fragHits;		/* Hits on fragAccesses. */
    int	fragZeroFills;		/* Cache blocks that had to be zero filled
				 * because of frag upgrades. */
    /*
     * File descriptor accesses.
     */
    int	fileDescReads;		/* File descriptor reads. */
    int	fileDescReadHits;	/* File descriptor hits in the cache. */
    int	fileDescWrites;		/* File descriptor writes. */
    int	fileDescWriteHits;	/* File descriptor write hits in the cache. */
    /*
     * Indirect block accesses.
     */
    int	indBlockAccesses;	/* Indirect block reads. */
    int	indBlockHits;		/* Access hits in the cache. */
    int	indBlockWrites;		/* Indirect blocks written. */
    /*
     * Directory block accesses.
     */
    int	dirBlockAccesses;	/* Directory block reads. */
    int	dirBlockHits;		/* Directory block hits. */
    int	dirBlockWrites;		/* Directory block writes. */
    int dirBytesRead;		/* Bytes read from directories */
    int dirBytesWritten;	/* Bytes written to directories */
    /*
     * Variable size cache statistics.
     */
    int	vmRequests;		/* Number of times virtual memory requested
				 * memory from us. */
    int	triedToGiveToVM;	/* Number of vmRequests that we actually tried
				 * to satisfy. */
    int	vmGotPage;		/* Number of vmRequests that we actually 
				 * satisfied. */
    int	gavePageToVM;		/* Blocks that we voluntarily gave back to
				 * to virtual memory when a block was freed.  */
    /*
     * Block allocation statistics.
     */
    int	partFree;		/* Got the block off of the partially free 
				 * list. */
    int	totFree;		/* Got the block off of the totally free list.*/
    int	unmapped;		/* Created a new block. */
    int	lru;	    		/* Recycled a block. */
    /*
     * Cache size numbers.
     */
    int	minCacheBlocks;		/* The minimum number of blocks that
				 * can be in the cache. */
    int	maxCacheBlocks;		/* The maximum number of blocks that
				 * can be in the cache. */
    int	maxNumBlocks;		/* The maximum number of blocks that can ever
				 * be in the cache. */
    int	numCacheBlocks;		/* The actual number of blocks that are in
				 * the cache. */
    int	numFreeBlocks;		/* The number of cache blocks that aren't being
				 * used. */
    /*
     * Miscellaneous.
     */
    int	blocksPitched;		/* The number of blocks that were thrown out
				 * at the command of virtual memory. */
} FsBlockCacheStats;

/*
 * Block allocation statistics.
 */
typedef struct {
    int	blocksAllocated;	/* Full blocks allocated. */
    int	blocksFreed;		/* Full blocks freed. */
    int	cylsSearched;		/* Cylinders searched to find a good cylinder.*/
    int	cylHashes;		/* Hashes done to find a starting cylinder. */
    int	cylBitmapSearches;	/* Cylinder bitmap entries searched. */
    int	fragsAllocated;		/* Fragments allocated. */
    int	fragsFreed;		/* Fragments freed. */
    int	fragToBlock;		/* Fragments that when freed made a full block
				 * free. */
    int	fragUpgrades;		/* Fragments that were attempted to be 
				 * extended.*/
    int	fragsUpgraded;		/* Number of fragUpgrades that were 
				 * successful. */
    int	badFragList;		/* Fragment list entries that didn't really
				 * have fragments of the desired size.*/
    int	fullBlockFrags;		/* Full blocks that had to be fragmented. */
} FsAllocStats;

/*
 * Name cache statistics.
 */
typedef struct {
    int	accesses;		/* Number of times something was looked for */
    int	hits;			/* Number of times it was found */
    int	replacements;		/* Number of entries recycled via LRU */
    int size;			/* Number of entries total */
} FsNameCacheStats;

/*
 * Handle statistics.
 */
typedef struct {
    int	exists;		/* Handles currently in existence. */
    int	created;	/* Handles that have been created. */
    int updateCalls;	/* Number of calls to HandleUpdate */
    int	installCalls;	/* Calls to FsHandleInstall. */
    int	installHits;	/* Number of installs in which handle was found. */
    int	versionMismatch;/* Version mismatch on file. */
    int	cacheFlushes;	/* Cache flushed because of version mismatch of not
			 * cacheable. */
    int	oldHandles;	/* Handles whose creation date is out of date. */
    int	fetchCalls;	/* Calls to FsHandleFetch. */
    int	fetchHits;	/* Number of fetches in which handle was found. */
    int	lockCalls;	/* Calls to FsHandleLock. */
    int	locks;		/* Number of times a handle was locked. */
    int	lockWaits;	/* Number of times had to wait to lock a handle. */
    int	releaseCalls;	/* Calls to FsHandleRelease. */
    int	segmentFetches;	/* Calls by VM to see if there is indeed already a
			 * segment with the code file. */
    int	segmentHits;	/* Segment fetches that return non-nil segment. */
} FsHandleStats;

/*
 * Prefix table statistics
 */
typedef struct {
    int relative;	/* Number of relative names encountered */
    int absolute;	/* Number of absolute names subject to prefix lookup */
    int redirects;	/* Number of redirects from the server */
    int loops;		/* Number of circular redirects (domain unavailable) */
    int timeouts;	/* Number of times the server was down */
    int stale;		/* Number of times server server rejected a handle */
    int found;		/* Number of times found a new prefix */
} FsPrefixStats;
/*
 * File system statistics.
 */
typedef struct {
    FsBlockCacheStats	blockCache;
    FsNameCacheStats	nameCache;
    FsAllocStats	alloc;
    FsHandleStats	handle;
    FsGeneralStats	gen;
    FsPrefixStats	prefix;
} FsStats;

extern	FsStats	fsStats;

#endif _FSSTAT
