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

/*
 * Note, both client and server Naming operations are kept.
 */
typedef struct Fs_NameOpStats {
    unsigned int numReadOpens;		/* open(O_RDONLY) on client.  Count
					 * of all server open calls */
    unsigned int numWriteOpens;		/* open(O_WRONLY) */
    unsigned int numReadWriteOpens;	/* open(O_RDWR) */
    unsigned int chdirs;		/* Number Fs_ChangeDir */
    unsigned int makeDevices;		/* Number Fs_MakeDevice */
    unsigned int makeDirs;		/* Number Fs_MakeDirectory */
    unsigned int removes;		/* Number Fs_Remove */
    unsigned int removeDirs;		/* Number Fs_RemoveDirs */
    unsigned int renames;		/* Number Fs_Rename */
    unsigned int hardLinks;		/* Number Fs_HardLink */
    unsigned int symLinks;		/* Number Fs_SymLink */
    unsigned int getAttrs;		/* Number get attrs from name */
    unsigned int setAttrs;		/* Number set attrs by name */
    unsigned int getAttrIDs;		/* Number set attrs by open stream */
    unsigned int setAttrIDs;		/* Number get attrs from open stream */
    unsigned int getIOAttrs;		/* Number get attr from I/O server */
    unsigned int setIOAttrs;		/* Number set attr to I/O server */
} Fs_NameOpStats;

typedef struct Fs_GeneralStats {
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
} Fs_GeneralStats;

/*
 * Cache statistics.
 */
typedef struct Fs_BlockCacheStats {
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
					 * Fscache_Write and Fscache_Read that
					 * were unsuccessful. */
    unsigned int readAheads;		/* Number of read ahead processes
					 * that were started up. */
    unsigned int readAheadHits;		/* Number of reads that hit on a
					 * block that was read ahead. */
    unsigned int allInCacheCalls;	/* Number of time FscacheAllBlocksInCache routine
					 * was called. */
    unsigned int allInCacheTrue;        /* Number of times that FscacheAllBlocksInCache
					 * returned TRUE. */
    /*
     * Write statistics.  See also WriteBackStats.
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
					 * tried to satisfy because
					 * we had free blocks */
    unsigned int vmGotPage;		/* Number of vmRequests that we
					 * satisfied after checking LRU times */
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
    int blocksFlushed;			/* The number of blocks written back
					   due to consistency. */
    int migBlocksFlushed;		/* The number of blocks written back
					   due to consistency for migrated
					   files. */
} Fs_BlockCacheStats;

/*
 * Block allocation statistics.
 */
typedef struct Fs_AllocStats {
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
} Fs_AllocStats;

/*
 * Name cache statistics.
 */
typedef struct Fs_NameCacheStats {
    unsigned int accesses;		/* Number of times something was
					 * looked for */
    unsigned int hits;			/* Number of times it was found */
    unsigned int replacements;		/* Number of entries recycled via LRU */
    unsigned int size;			/* Number of entries total */
} Fs_NameCacheStats;

/*
 * Handle statistics.
 */
typedef struct Fs_HandleStats {
    unsigned int maxNumber;	/* Current limit on table size. */
    unsigned int exists;	/* Number of handles currently in existence. */
    unsigned int installCalls;	/* Calls to Fsutil_HandleInstall. */
    unsigned int installHits;	/* installs in which handle was found. */
    unsigned int fetchCalls;	/* Calls to Fsutil_HandleFetch. */
    unsigned int fetchHits;	/* fetches in which handle was found. */
    unsigned int release;	/* Calls to Fsutil_HandleRelease. */
    unsigned int locks;		/* Number of times a handle was locked. */
    unsigned int lockWaits;	/* Number of times had to wait on a lock */
    unsigned int unlocks;	/* Number of times a handle was unlocked. */
    unsigned int created;	/* Handles that have been created. */
    unsigned int lruScans;	/* Number of LRU replacement scans */
    unsigned int lruChecks;	/* Number of handles checked for reclaimation */
    unsigned int lruHits;	/* Number of handles actually reclaimed */
    unsigned int lruEntries;	/* Number of handles in LRU list. */
    unsigned int limbo;		/* Number of handles marked for removal */
    /*
     * The following are specific to regular files.
     */
    unsigned int versionMismatch; /* Version mismatch on file. */
    unsigned int cacheFlushes;	/* Cache flushed because of version mismatch
				 * of not cacheable. */
    unsigned int segmentFetches; /* Calls by VM to see if there is indeed
				  * already a segment with the code file. */
    unsigned int segmentHits;	/* Segment fetches that return non-nil
				 * segment. */
} Fs_HandleStats;

/*
 * Prefix table statistics.  These are client-side statistics
 */
typedef struct Fs_PrefixStats {
    unsigned int relative;	/* Number of relative names encountered */
    unsigned int absolute;	/* Number of absolute names subject to prefix
				 * lookup */
    unsigned int redirects;	/* Number of redirects from the server */
    unsigned int loops;		/* Number of circular redirects */
    unsigned int timeouts;	/* Number of times the server was down */
    unsigned int stale;		/* Number of times server server rejected a
				 * handle */
    unsigned int found;		/* Number of times found a new prefix */
} Fs_PrefixStats;

/*
 * Name lookup statistics.  These are server-side statistics.
 */
typedef struct Fs_LookupStats {
    unsigned int number;	/* Number of pathname lookups */
    unsigned int numComponents;	/* Number of pathname components parsed */
    unsigned int numSpecial;	/* Number of $MACHINE names encounted */
    unsigned int forDelete;	/* Number for deletion */
    unsigned int forLink;	/* Number for linking */
    unsigned int forRename;	/* Number for rename */
    unsigned int forCreate;	/* Number for creation */
    unsigned int symlinks;	/* Number of symbolic links encountered */
    unsigned int redirect;	/* Number of redirects due to symbolic links */
    unsigned int remote;	/* Number of redirects due to remote links */
    unsigned int parent;	/* Number of redirects due to ".." */
    unsigned int notFound;	/* Number of FILE_NOT_FOUND lookups */
} Fs_LookupStats;

/*
 * Counts of various file system objects.
 */
typedef struct Fs_ObjectStats {
    int streams;
    int streamClients;		/* Equal to streams, except during migration */
    int files;			/* Local files, not including directories */
    int rmtFiles;
    int pipes;
    int devices;
    int controls;		/* Pdev and Pfs control streams */
    int pseudoStreams;		/* One count for both client/server handles */
    int remote;			/* All the various remote objects but files*/
    int directory;
    int dirFlushed;		/* Directories that were flushed */
    int fileClients;		/* Number of consist.clientList entries */
    int other;			/* For unknown objects */
} Fs_ObjectStats;

/*
 * File system recovery statistics.
 */
typedef struct Fs_RecoveryStats {
    int number;			/* Number of reopens by this client */
    int wants;			/* Calls to Fsutil_WantRecovery */
    int waitOK;			/* Successful RecoveryWaits */
    int waitFailed;		/* Unnsuccessful RecoveryWaits */
    int waitAbort;		/* Interrupted RecoveryWaits */
    int timeout;		/* Re-open's that timed out */
    int failed;			/* Re-open's that failed */
    int deleted;		/* Re-open's of a file that has been deleted */
    int offline;		/* Re-open's of a file that is now offline */
    int succeeded;		/* Re-open's that worked */
    int clientCrashed;		/* Number of clients that crashed */
    int clientRecovered;	/* Number of clients that re-opened files */
} Fs_RecoveryStats;

/*
 * Cache conistency statistics.
 */
typedef struct Fs_ConsistStats {
    int files;			/* The number of times consistency was checked*/
    int clients;		/* The number of clients considered */
    int notCaching;		/* # of other clients that weren't caching */
    int readCachingMyself;	/* # of clients that were read caching */
    int readCachingOther;	/* # of other clients that were read caching */
    int writeCaching;		/* # of lastWriters that re-opened  */
    int writeBack;		/* # of lastWriters forced to write-back */
    int readInvalidate;		/* # of readers forced to stop caching */
    int writeInvalidate;	/* # of writers forced to stop caching */
    int nonFiles;		/* # of directories, links, etc. */
    int swap;			/* # of uncached swap files. */
    int cacheable;		/* # of files that were cacheable */
    int uncacheable;		/* # of files that were not cacheable */
} Fs_ConsistStats;

/*
 * (More) Write-back statistics.
 */
typedef struct Fs_WriteBackStats {
    int passes;			/* Number of times Fs_CleanBlocks called */
    int files;			/* Number of dirty files processed */
    int blocks;			/* Number of dirty blocks processed */
    int maxBlocks;		/* Max blocks processed in one pass */
} Fs_WriteBackStats;

/*
 * Some miscellaneous stats to determine why we read remote bytes.
 */
typedef struct Fs_RemoteIOStats {
    int blocksReadForVM;	/* Blocks read from Fs_PageRead (old) */
    int bytesReadForCache;	/* Bytes read into the cache */
    int bytesWrittenFromCache;	/* Bytes written from the cache */
    int uncacheableBytesRead;	/* Uncacheable bytes read (not counting swap) */
    int uncacheableBytesWritten;/* Uncacheable bytes written */
    int sharedStreamBytesRead;	/* Bytes read from shared uncacheable streams */
    int sharedStreamBytesWritten;/* Bytes written to shared uncached streams */
    int hitsOnVMBlock;		/* Code and Heap pages found in the cache */
    int missesOnVMBlock;	/* Code and Heap pages not found in the cache */
    int bytesReadForVM;		/* Bytes read in RmtFilePageRead */
    int bytesWrittenForVM;	/* Bytes written in RmtFilePageWrite */
} Fs_RemoteIOStats;

/*
 * Statistics relating to migration.
 */
typedef struct Fs_MigStats {
    unsigned int filesEncapsulated; 	/* Total number of files encapsulated
					   by this host */
    unsigned int filesDeencapsulated; 	/* Total number of files deencapsulated
					   by this host */
    unsigned int consistActions; 	/* Total number of files for which
					   this host was the i/o server doing
					   consistency */
    unsigned int readOnlyFiles; 	/* Total number of files deencapsulated
					   read-only */
    unsigned int alreadyThere; 		/* Total number of (writable) files
					   already on target. NOT USED. */
    unsigned int uncacheableFiles; 	/* Total number of files deencapsulated
					   that were uncacheable to begin
					   with, and stayed that way. */
    unsigned int cacheWritableFiles; 	/* Total number of cacheable, writable
					   files that were still cacheable
					   after migration. */
    unsigned int uncacheToCacheFiles; 	/* Total number of uncacheable
					   files that became cacheable after
					   migration. */
    unsigned int cacheToUncacheFiles; 	/* Total number of cacheable
					   files that became uncacheable after
					   migration. */
    unsigned int errorsOnDeencap;	/* Any files that couldn't be
					   deencapsulated due to errors. */
    unsigned int encapSquared; 		/* Sum of squares for
					   filesEncapsulated. */
    unsigned int deencapSquared; 	/* Sum of squares for
					   filesDeencapsulated. */
} Fs_MigStats;

/*
 * File system statistics.
 */
#define FS_STAT_VERSION 2
typedef struct Fs_Stats {
    int			statsVersion;   /* Version number of statistics info */
    Fs_NameOpStats	cltName;	/* Client-side naming operations */
    Fs_NameOpStats	srvName;	/* Server-side naming operations */
    Fs_GeneralStats	gen;		/* General I/O operations */
    Fs_BlockCacheStats	blockCache;	/* Block cache operations */
    Fs_AllocStats	alloc;		/* Disk allocation */
    Fs_HandleStats	handle;		/* Handle management */
    Fs_PrefixStats	prefix;		/* Client-side prefix operations */
    Fs_LookupStats	lookup;		/* Server-side lookup operations */
    Fs_NameCacheStats	nameCache;	/* Server name cache */
    Fs_ObjectStats	object;		/* Counts of various objects */
    Fs_RecoveryStats	recovery;	/* Crash recovery and reopening */
    Fs_ConsistStats	consist;	/* Cache consistency actions */
    Fs_WriteBackStats	writeBack;	/* Cache write-back stats */
    Fs_RemoteIOStats	rmtIO;		/* Remote I/O stats */
    Fs_MigStats		mig;		/* Migration */
} Fs_Stats;

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

typedef struct Fs_HistGroupInfo {
    unsigned int secondsPerBucket;
    unsigned int bucketsPerGroup;
} Fs_HistGroupInfo;


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

typedef struct Fs_TypeStats {
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
} Fs_TypeStats;

Fs_TypeStats fs_TypeStats;

/*
 * Macro to add to a counter, watching for overflow.  We use unsigned
 * integers and wrap around if the high-order bit gets set.  This assumes
 * that the amount to be added each time is
 * relatively small (so we can't miss the overflow bit).
 */

#define FS_STAT_OVERFLOW (1 << (sizeof(unsigned int) * 8 - 1))
#define Fs_StatAdd(thisCount, counter, overflow) \
    counter += thisCount; \
    if (counter & FS_STAT_OVERFLOW) { \
        overflow += 1; \
        counter &= ~FS_STAT_OVERFLOW; \
    }

extern	Fs_Stats	fs_Stats;

#endif _FSSTAT
