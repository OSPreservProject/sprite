/*
 * fsStat.h --
 *
 *	Declarations for the file system statistics.
 *
 *	Note: since all variables are counters, they are unsigned
 *	in order to make the high-order bit useable.
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
    unsigned int physBytesRead;		/* Number of physical (ie descriptors)
				 	 * disk bytes read. */
    unsigned int fileBytesRead;		/* Number of file bytes read from the
					 * disk */
    unsigned int fileReadOverflow;	/* Extra */
    unsigned int remoteBytesRead;	/* Number of bytes read from
					 * un-cachable remote files */
    unsigned int remoteReadOverflow;	/* Extra */
    unsigned int deviceBytesRead;	/* Number of bytes read from devices */

    unsigned int physBytesWritten;	/* Number of physical disk bytes
					 * written */
    unsigned int fileBytesWritten;	/* Number of file bytes written to
					 * the disk */
    unsigned int fileWriteOverflow;	/* Extra */
    unsigned int remoteBytesWritten;	/* Number of bytes written to
					 * un-cachable remote files */
    unsigned int remoteWriteOverflow;	/* Extra */
    unsigned int deviceBytesWritten;	/* Number of bytes written to devices */
    unsigned int fileBytesDeleted;	/* Number of file bytes deleted
					 * (total) */
    unsigned int fileDeleteOverflow;	/* Extra */
    unsigned int numReadOpens;		/* Number of calls to open files for
					 * reading */
    unsigned int numWriteOpens;		/* Number of calls to open files for
					 * writing */
    unsigned int numReadWriteOpens;	/* Number of calls to open files for
					 * reading  and writing */
    unsigned int numSetAttrs;		/* Number of calls to set attributes */
} FsGeneralStats;

/*
 * Cache statistics.
 */
typedef struct {
    /*
     * Read statistics.
     */
    unsigned int readAccesses;		/* Read fetches on the cache. */
    unsigned int bytesRead;		/* Total bytes read from the cache */
    unsigned int bytesReadOverflow;	/* Extra */
    unsigned int readHitsOnDirtyBlock;	/* Fetches that hit on a block that had
				 	 * dirty data in it. */
    unsigned int readHitsOnCleanBlock;	/* Fetches that hit on a block that had
				 	 * clean data in it. */
    unsigned int readZeroFills;		/* Blocks that are zero filled
					 * because read did not fill the
					 * whole block. */
    unsigned int domainReadFails;	/* Number of domain reads from
					 * FsCacheWrite and FsCacheRead that
					 * were unsuccessful. */
    unsigned int readAheads;		/* Number of read ahead processes
					 * that were started up. */
    unsigned int readAheadHits;		/* Number of reads that hit on a
					 * block that was read ahead. */
    unsigned int allInCacheCalls;	/* Number of time FsAllInCache routine
					 * was called. */
    unsigned int allInCacheTrue;        /* Number of times that FsAllInCache
					 * returned TRUE. */
    /*
     * Write statistics.
     */
    unsigned int writeAccesses;		/* Number of write fetches on the
					 * cache. */
    unsigned int bytesWritten;		/* Total bytes written to the cache */
    unsigned int bytesWrittenOverflow;	/* Extra */
    unsigned int appendWrites;		/* Blocks written in append mode. */
    unsigned int overWrites;		/* Cache blocks that were
					 * overwritten. */
    unsigned int writeZeroFills1;	/* Blocks that are zero filled because
					 * read of old block did not fill
					 * whole block. */
    unsigned int writeZeroFills2;	/* Blocks that are zero filled because
					 * user is only doing a partial write
					 * to a new block. */
    unsigned int partialWriteHits;	/* Read hits in the cache when are
					 * writing to the middle of a block
					 * and have to read in the data that
					 * is already there. */
    unsigned int partialWriteMisses;	/* Misses for above case. */
    unsigned int blocksWrittenThru;	/* Number of dirty blocks that were
					 * written thru. */
    unsigned int dataBlocksWrittenThru;	/* Number of data block that were
					 * written thru. */
    unsigned int indBlocksWrittenThru;	/* Number of indirect blocks that
					 * were written thru. */
    unsigned int descBlocksWrittenThru;	/* Number of descriptor blocks that
					 * were written thru. */
    unsigned int dirBlocksWrittenThru;	/* Number of directory blocks that
					 * were writtenw thru. */
    /*
     * Fragment statistics.
     */
    unsigned int fragAccesses;		/* Cache blocks that were fetched in
					 * order to upgrade fragments. */
    unsigned int fragHits;		/* Hits on fragAccesses. */
    unsigned int fragZeroFills;		/* Cache blocks that had to be zero
					 * filled because of frag upgrades. */
    /*
     * File descriptor accesses.
     */
    unsigned int fileDescReads;		/* File descriptor reads. */
    unsigned int fileDescReadHits;	/* File descriptor hits in the cache. */
    unsigned int fileDescWrites;	/* File descriptor writes. */
    unsigned int fileDescWriteHits;	/* File descriptor write hits in the
					 * cache. */
    /*
     * Indirect block accesses.
     */
    unsigned int indBlockAccesses;	/* Indirect block reads. */
    unsigned int indBlockHits;		/* Access hits in the cache. */
    unsigned int indBlockWrites;	/* Indirect blocks written. */
    /*
     * Directory block accesses.
     */
    unsigned int dirBlockAccesses;	/* Directory block reads. */
    unsigned int dirBlockHits;		/* Directory block hits. */
    unsigned int dirBlockWrites;	/* Directory block writes. */
    unsigned int dirBytesRead;		/* Bytes read from directories */
    unsigned int dirBytesWritten;	/* Bytes written to directories */
    /*
     * Variable size cache statistics.
     */
    unsigned int vmRequests;		/* Number of times virtual memory
					 * requested memory from us. */
    unsigned int triedToGiveToVM;	/* Number of vmRequests that we
					 * actually tried to satisfy. */
    unsigned int vmGotPage;		/* Number of vmRequests that we
					 * actually satisfied. */
    unsigned int gavePageToVM;		/* Blocks that we voluntarily gave
					 * back to to virtual memory when a
					 * block was freed.  */
    /*
     * Block allocation statistics.
     */
    unsigned int partFree;		/* Got the block off of the partially
					 * free list. */
    unsigned int totFree;		/* Got the block off of the totally
					 * free list. */
    unsigned int unmapped;		/* Created a new block. */
    unsigned int lru;	    		/* Recycled a block. */
    /*
     * Cache size numbers.
     */
    unsigned int minCacheBlocks;	/* The minimum number of blocks that
				 	 * can be in the cache. */
    unsigned int maxCacheBlocks;	/* The maximum number of blocks that
				 	 * can be in the cache. */
    unsigned int maxNumBlocks;		/* The maximum number of blocks that
					 * can ever be in the cache. */
    unsigned int numCacheBlocks;	/* The actual number of blocks that
					 * are in the cache. */
    unsigned int numFreeBlocks;		/* The number of cache blocks that
					 * aren't being used. */
    /*
     * Miscellaneous.
     */
    unsigned int blocksPitched;		/* The number of blocks that were
					 * thrown out at the command of
					 * virtual memory. */
} FsBlockCacheStats;

/*
 * Block allocation statistics.
 */
typedef struct {
    unsigned int blocksAllocated;	/* Full blocks allocated. */
    unsigned int blocksFreed;		/* Full blocks freed. */
    unsigned int cylsSearched;		/* Cylinders searched to find a good
					 * cylinder.*/
    unsigned int cylHashes;		/* Hashes done to find a starting
					 * cylinder. */
    unsigned int cylBitmapSearches;	/* Cylinder bitmap entries searched. */
    unsigned int fragsAllocated;	/* Fragments allocated. */
    unsigned int fragsFreed;		/* Fragments freed. */
    unsigned int fragToBlock;		/* Fragments that when freed made a
					 * full block free. */
    unsigned int fragUpgrades;		/* Fragments that were attempted to
					 * be extended.*/
    unsigned int fragsUpgraded;		/* Number of fragUpgrades that
					 * were successful. */
    unsigned int badFragList;		/* Fragment list entries that didn't
					 * really have fragments of the
					 * desired size.*/
    unsigned int fullBlockFrags;	/* Full blocks that had to
					 * be fragmented. */
} FsAllocStats;

/*
 * Name cache statistics.
 */
typedef struct {
    unsigned int accesses;		/* Number of times something was
					 * looked for */
    unsigned int hits;			/* Number of times it was found */
    unsigned int replacements;		/* Number of entries recycled via LRU */
    unsigned int size;			/* Number of entries total */
} FsNameCacheStats;

/*
 * Handle statistics.
 */
typedef struct {
    unsigned int exists;	/* Handles currently in existence. */
    unsigned int created;	/* Handles that have been created. */
    unsigned int updateCalls;	/* Number of calls to HandleUpdate */
    unsigned int installCalls;	/* Calls to FsHandleInstall. */
    unsigned int installHits;	/* Number of installs in which handle was found. */
    unsigned int versionMismatch; /* Version mismatch on file. */
    unsigned int cacheFlushes;	/* Cache flushed because of version mismatch
				 * of not cacheable. */
    unsigned int maxNumber;	/* Current limit on table size. */
    unsigned int fetchCalls;	/* Calls to FsHandleFetch. */
    unsigned int fetchHits;	/* Number of fetches in which handle was
				 * found. */
    unsigned int lockCalls;	/* Calls to FsHandleLock. */
    unsigned int locks;		/* Number of times a handle was locked. */
    unsigned int lockWaits;	/* Number of times had to wait to lock a
				 * handle. */
    unsigned int releaseCalls;	/* Calls to FsHandleRelease. */
    unsigned int segmentFetches; /* Calls by VM to see if there is indeed
				  * already a segment with the code file. */
    unsigned int segmentHits;	/* Segment fetches that return non-nil
				 * segment. */
} FsHandleStats;

/*
 * Prefix table statistics
 */
typedef struct {
    unsigned int relative;	/* Number of relative names encountered */
    unsigned int absolute;	/* Number of absolute names subject to prefix
				 * lookup */
    unsigned int redirects;	/* Number of redirects from the server */
    unsigned int loops;		/* Number of circular redirects (domain
				 * unavailable) */
    unsigned int timeouts;	/* Number of times the server was down */
    unsigned int stale;		/* Number of times server server rejected a
				 * handle */
    unsigned int found;		/* Number of times found a new prefix */
} FsPrefixStats;

/*
 * Counts of various file system objects.
 */
typedef struct {
    int lruScans;		/* Number of LRU replacement scans */
    int scavenges;		/* Number of handles reclaimed */
    int streams;
    int streamClients;
    int files;
    int rmtFiles;
    int pipes;
    int devices;
    int controls;		/* Pdev and Pfs control streams */
    int pseudoStreams;		/* One count for both client/server handles */
    int remote;			/* All the various remote objects but files*/
} FsObjectStats;


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
    FsObjectStats	object;
} FsStats;

/*
 * Keep a histogram of file lifetimes in a three-dimensional matrix,
 * for lifetime, file size, and file type.  Buckets for size are
 * scaled logarithmically.  Buckets for lifetime are determined as
 * follows, where each bucket corresponds to a lifetime less than that
 * value:
 *
 *	seconds: 1, 2, ..., 10, 20, ..., 50
 *	minutes: 1, 2, ..., 10, 20, ..., 50
 *      hours:   1, 2, ..., 10, 15, 20 (<24)
 *	days:    1, 2, ..., 10, 20, .., 60, 90, 120, 180, 240, 300, 360, > 360
 *
 * For example, a file that is 0 seconds old is < 1 second and is in
 * bucket 0; a file that is exactly 15 hours old is in the bucket labeled
 * "20 hours" since that contains everything from 15 hours to
 * (20 hours - 1 second).
 *
 * The divisions are somewhat arbitrary and are subject to change.
 * Define the number of buckets here: 32 size buckets will cover 2 **
 * 32 bytes (max file size); throw in one more bucket for subtotals by
 * time.  For times, define an array containing the number of seconds
 * and number of buckets for each group described above.
 */
#define FS_HIST_SIZE_BUCKETS 	33

typedef struct {
    unsigned int secondsPerBucket;
    unsigned int bucketsPerGroup;
} FsHistGroupInfo;


#define FS_HIST_SECONDS		10
#define FS_HIST_TEN_SECONDS	5
#define FS_HIST_MINUTES		9
#define FS_HIST_TEN_MINUTES	5
#define FS_HIST_HOURS		9
#define FS_HIST_FIVE_HOURS	2
#define FS_HIST_REST_HOURS	1
#define FS_HIST_DAYS		9
#define FS_HIST_TEN_DAYS 	5
#define FS_HIST_THIRTY_DAYS 	2
#define FS_HIST_SIXTY_DAYS 	4
#define FS_HIST_REST_DAYS 	1

#define FS_HIST_TIME_BUCKETS (FS_HIST_SECONDS + \
			      FS_HIST_TEN_SECONDS + \
			      FS_HIST_MINUTES + \
			      FS_HIST_TEN_MINUTES + \
			      FS_HIST_HOURS + \
			      FS_HIST_FIVE_HOURS + \
			      FS_HIST_REST_HOURS + \
			      FS_HIST_DAYS + \
			      FS_HIST_TEN_DAYS + \
			      FS_HIST_THIRTY_DAYS + \
			      FS_HIST_SIXTY_DAYS + \
			      FS_HIST_REST_DAYS)

/*
 * The number of types that we gather statistics for.
 */
#define FS_STAT_NUM_TYPES 5

/*
 * Subscripts for arrays that separate data for read and write.
 */
#define FS_STAT_READ 0
#define FS_STAT_WRITE 1

typedef struct {
    unsigned int diskBytes[2][FS_STAT_NUM_TYPES];
    				/* Number of bytes read/written from/to
				 * different types of files on disk */
    unsigned int cacheBytes[2][FS_STAT_NUM_TYPES];
    				/* Number of bytes read or written through
				 * cache */
    unsigned int bytesDeleted[FS_STAT_NUM_TYPES];
    				/* Number of bytes deleted from files due
				 * to truncation or removal */
    unsigned int deleteHist	/* Histogram of deletions, by type */
	    [FS_HIST_TIME_BUCKETS] [FS_HIST_SIZE_BUCKETS] [FS_STAT_NUM_TYPES];
} FsTypeStats;

FsTypeStats fsTypeStats;

/*
 * Macro to add to a counter, watching for overflow.  We use unsigned
 * integers and wrap around if the high-order bit gets set.  This assumes
 * that the amount to be added each time is
 * relatively small (so we can't miss the overflow bit).
 */

#define FSSTAT_OVERFLOW (1 << (sizeof(unsigned int) * 8 - 1))
#define FsStat_Add(thisCount, counter, overflow) \
    counter += thisCount; \
    if (counter & FSSTAT_OVERFLOW) { \
        overflow += 1; \
        counter &= ~FSSTAT_OVERFLOW; \
    }

extern	FsStats	fsStats;

#endif _FSSTAT
