/* fsNameHash.c --
 *
 *      This is a modified version of the hashing utilities used for the
 *      filesystem name cache.  The main difference is that there is a two
 *      part key, the string and a file identifier. Also, entries are kept
 *      in LRU order (in another list) for replacement when the table is
 *      full.
 *
 * Copyright (C) 1983 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif  not lint

#include "sprite.h"
#include "fs.h"
#include "fsutil.h"
#include "fsNameHash.h"
#include "fslcl.h"
#include "fsStat.h"
#include "string.h"
#include "list.h"
#include "sys.h"

static	Sync_Lock nameHashLock = Sync_LockInitStatic("Fs:nameHashLock");
#define	LOCKPTR	&nameHashLock



/*
 *---------------------------------------------------------
 * 
 * HashInit --
 *
 *	This routine just sets up the hash table with the given size.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Memory is allocated for the initial bucket area.
 *
 *---------------------------------------------------------
 */

static void
HashInit(table, numBuckets)
    register FslclHashTable	*table;
    int			numBuckets;	/* How many buckets to create for 
					 * starters. This number is rounded up 
					 * to a power of two. */
{
    register	int 		i;
    register	FslclHashBucket 	*tablePtr;

    /* 
     * Round up the size to a power of two, and compute a shift and mask
     * used to index into the hash header table.
     */

    if (numBuckets < 0) {
	numBuckets = -numBuckets;
    }
    table->numEntries = 0;
    table->size = 2;
    table->mask = 1;
    table->downShift = 29;
    while (table->size < numBuckets) {
	table->size <<= 1;
	table->mask = (table->mask << 1) + 1;
	table->downShift--;
    }

    fs_Stats.nameCache.size = table->size;

    List_Init(&(table->lruList));
    table->table =
	(FslclHashBucket *) malloc(sizeof(FslclHashBucket) * table->size);
    for (i=0, tablePtr = table->table; i < table->size; i++, tablePtr++) {
	List_Init(&(tablePtr->list));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fslcl_NameHashInit --
 *
 *	Make sure the local name hash table is initialized.  Called
 *	when disks are attached so that diskless clients don't allocate
 *	space for the name hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	HashInit is called which allocates memory for the initial bucket area.
 *	
 *
 *----------------------------------------------------------------------
 */

void
Fslcl_NameHashInit()
{
    if (fslclNameTablePtr == (FslclHashTable *)NIL) {
	fslclNameTablePtr = &fslclNameTable;
	HashInit(fslclNameTablePtr, fslclNameHashSize);
    }
}


/*
 *---------------------------------------------------------
 *
 * Hash --
 *	This is a local procedure to compute a hash table
 *	bucket address based on a pair <string, fileHdrPtr>.
 *	The file information needs to be used to randomize the same
 *	names found in different directories.
 *
 * Results:
 *	The return value is an integer between 0 and size - 1.
 *
 * Side Effects:	
 *	None.
 *
 * Note:
 *	The randomizing code is stolen straight from the rand library routine.
 *
 *---------------------------------------------------------
 */

INTERNAL static int
Hash(table, string, keyHdrPtr)
    register FslclHashTable *table;	/* The hash table (per domain?) */
    register char 	*string;	/* Name of the component */
    Fs_HandleHeader	*keyHdrPtr;	/* Handle of the parent directory */
{
    register int 	i = 0;

    while (*string != 0) {
	i = i * 10 + *string++;
    }
    i += (int) keyHdrPtr;
    return(((i*1103515245 + 12345) >> table->downShift) & table->mask);
}


/*
 *---------------------------------------------------------
 *
 * ChainSearch --
 *
 * 	Search the hash table for the entry in the hash chain.
 *
 * Results:
 *	Pointer to entry in hash chain, NIL if none found.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

INTERNAL static FslclHashEntry *
ChainSearch(table, string, keyHdrPtr, hashList)
    FslclHashTable 		*table;		/* Hash table to search. */
    register char		*string;	/* Hash key, part 1 */
    register Fs_HandleHeader	*keyHdrPtr;	/* Hash key, part 2 */
    register List_Links 	*hashList;	/* Bucket list indexed by Hash*/
{
    register FslclHashEntry *hashEntryPtr;

    LIST_FORALL(hashList, (List_Links *) hashEntryPtr) {
	if ((strcmp(hashEntryPtr->keyName, string) == 0) &&
		(hashEntryPtr->keyHdrPtr == keyHdrPtr)) {
	    /*
	     * Move the entry to the front of the LRU list.
	     */
	    List_Move(&(hashEntryPtr->lru.links),
			 LIST_ATFRONT(&(table->lruList)));
	    return(hashEntryPtr);
	}
    }
    /* 
     * The desired entry isn't there 
     */

    return ((FslclHashEntry *) NIL);
}

/*
 *---------------------------------------------------------
 *
 * FslclHashLookOnly --
 *
 * 	Searches a hash table for an entry corresponding to the string
 *	and parent file.
 *
 * Results:
 *	The return value is a pointer to the entry for string,
 *	if string was present in the table.  If string was not
 *	present, NIL is returned.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

ENTRY FslclHashEntry *
FslclHashLookOnly(table, string, keyHdrPtr)
    register FslclHashTable *table;		/* Hash table to search. */
    char		*string;		/* Hash key, part 1. */
    Fs_HandleHeader	*keyHdrPtr;		/* Hash key, part 2. */
{
    FslclHashEntry *hashEntryPtr;

    LOCK_MONITOR;
    fs_Stats.nameCache.accesses++;
    hashEntryPtr = ChainSearch(table, string, keyHdrPtr,
		  &(table->table[Hash(table, string, keyHdrPtr)].list));
    if (hashEntryPtr != (FslclHashEntry  *) NIL) {
	fs_Stats.nameCache.hits++;
    }

    UNLOCK_MONITOR;
    return(hashEntryPtr);
}


/*
 *---------------------------------------------------------
 *
 * FsHashFind --
 *
 *	Searches a hash table for an entry corresponding to
 *	key.  If no entry is found, then one is created.
 *
 * Results:
 *	The return value is a pointer to the entry for string.
 *	If the entry is a new one, then the pointer field is
 *	zero.
 *
 *	Side Effects:
 *	Memory is allocated, and the hash buckets may be modified.
 *---------------------------------------------------------
 */

ENTRY FslclHashEntry *
FslclHashInsert(table, string, keyHdrPtr, hdrPtr)
    register FslclHashTable	*table;		/* Hash table to search. */
    register	char		*string;	/* Hash key, part 1 */
    Fs_HandleHeader		*keyHdrPtr;	/* Hash key, part 2 */
    Fs_HandleHeader		*hdrPtr;	/* Value */
{
    register 	FslclHashBucket 	*bucketPtr;
    register 	FslclHashEntry	*hashEntryPtr;
    register	List_Links	*lruLinkPtr;

    LOCK_MONITOR;

    bucketPtr = &(table->table[Hash(table, string, keyHdrPtr)]);
    hashEntryPtr = ChainSearch(table, string, keyHdrPtr,
					&(bucketPtr->list));

    if (hashEntryPtr != (FslclHashEntry *) NIL) {
	UNLOCK_MONITOR;
	return(hashEntryPtr);
    }

    /* 
     * See if we have to do LRU replacement before adding another entry.
     */

    if (table->numEntries >= table->size) {
	fs_Stats.nameCache.replacements++;
	lruLinkPtr = LIST_ATREAR(&(table->lruList));
	hashEntryPtr = ((struct FsLruList *)lruLinkPtr)->entryPtr;
	Fsutil_HandleDecRefCount(hashEntryPtr->hdrPtr);
	Fsutil_HandleDecRefCount(hashEntryPtr->keyHdrPtr);
	List_Remove((List_Links *)hashEntryPtr);
	List_Remove(&(hashEntryPtr->lru.links));
	free((Address)hashEntryPtr);
    } else {
	table->numEntries += 1;
    }

    /*
     * Not there, we have to allocate.  If the string is longer than 3
     * bytes, then we have to allocate extra space in the entry.
     */

    hashEntryPtr = (FslclHashEntry *) malloc(sizeof(FslclHashEntry) + 
			strlen(string) - 3);
    (void)strcpy(hashEntryPtr->keyName, string);
    hashEntryPtr->keyHdrPtr = keyHdrPtr;
    hashEntryPtr->hdrPtr = hdrPtr;
    hashEntryPtr->bucketPtr = bucketPtr;
    List_Insert((List_Links *) hashEntryPtr, LIST_ATFRONT(&(bucketPtr->list)));
    hashEntryPtr->lru.entryPtr = hashEntryPtr;
    List_Insert(&(hashEntryPtr->lru.links), LIST_ATFRONT(&(table->lruList)));
    /*
     * Increment the reference count on the handle since we now have it
     * in the name cache.
     */
    Fsutil_HandleIncRefCount(hdrPtr, 1);
    Fsutil_HandleIncRefCount(keyHdrPtr, 1);
    UNLOCK_MONITOR;

    return(hashEntryPtr);
}

/*
 *---------------------------------------------------------
 *
 * FslclHashDelete --
 *
 * 	Search the hash table for an entry corresponding to the string
 *	and parent file and then delete it if it is there.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Hash chain that entry lives is modified and memory is freed.
 *
 *---------------------------------------------------------
 */

void
FslclHashDelete(table, string, keyHdrPtr)
    register FslclHashTable *table;	/* Hash table to search. */
    char		 *string;		/* Hash key, part 1. */
    Fs_HandleHeader	 *keyHdrPtr;		/* Hash key, part 2. */
{
    FslclHashEntry *hashEntryPtr;

    LOCK_MONITOR;

    fs_Stats.nameCache.accesses++;
    hashEntryPtr = ChainSearch(table, string, keyHdrPtr,
		  &(table->table[Hash(table, string, keyHdrPtr)].list));
    if (hashEntryPtr != (FslclHashEntry  *) NIL) {
	/*
	 * Release the two handles referenced by the name cache entry.
	 * This is called when deleting the file, at which point both
	 * the parent (keyHdrPtr) and the file itself (hdrPtr) are locked.
	 */
	Fsutil_HandleDecRefCount(hashEntryPtr->hdrPtr);
	Fsutil_HandleDecRefCount(hashEntryPtr->keyHdrPtr);
	List_Remove((List_Links *)hashEntryPtr);
	List_Remove(&(hashEntryPtr->lru.links));
	free((Address)hashEntryPtr);
	table->numEntries--;
    }

    UNLOCK_MONITOR;
}


/*
 *---------------------------------------------------------
 *
 * FsRebuildTable --
 *	This local routine makes a new hash table that
 *	is larger than the old one.
 *
 * Results:	
 * 	None.
 *
 * Side Effects:
 *	The entire hash table is moved, so any bucket numbers
 *	from the old table are invalid.
 *
 *---------------------------------------------------------
 */
#ifdef notdef
static void
FsRebuildTable(table)
    register	FslclHashTable 	*table;		/* Table to be enlarged. */
{
    register	FslclHashBucket	*oldTable;
    register	FslclHashEntry  	*hashEntryPtr;
    register	int 		oldSize;
    int 		 	bucket;
    FslclHashBucket		*saveTable;
    FslclHashBucket		*bucketPtr;
    Fs_HandleHeader		*keyHdrPtr;
    int			 	version;

    LOCK_MONITOR;

    saveTable = table->table;
    oldSize = table->size;

    /* 
     * Build a new table 4 times as large as the old one. 
     */

    HashInit(table, table->size * 4);

    for (oldTable = saveTable; oldSize > 0; oldSize--, oldTable++) {
	while (!List_IsEmpty(&(oldTable->list))) {
	    hashEntryPtr = (FslclHashEntry *) List_First(&(oldTable->list));
	    List_Remove((List_Links *) hashEntryPtr);
	    List_Remove(&(hashEntryPtr->lru.links));
	    keyHdrPtr = hashEntryPtr->keyHdrPtr;
	    bucket = Hash(table, hashEntryPtr->keyName, keyHdrPtr);
	    bucketPtr = &(table->table[bucket]);
	    List_Insert((List_Links *) hashEntryPtr, 
		LIST_ATFRONT(&(bucketPtr->list)));
	    List_Insert(&(hashEntryPtr->lru.links),
		LIST_ATFRONT(&(table->lruList)));
	    hashEntryPtr->bucketPtr = bucketPtr;
	    table->numEntries++;
	}
    }

    free((Address) saveTable);

    UNLOCK_MONITOR;
}
#endif notdef

/*
 *---------------------------------------------------------
 *
 * FsHashStats --
 *	This routine merely prints statistics about the
 *	current bucket situation.
 *
 * Results:	
 *	None.
 *
 * Side Effects:	
 *	Junk gets printed.
 *
 *---------------------------------------------------------
 */

void
FslclNameHashStats()
{
    FslclHashTable *table = fslclNameTablePtr;
    int count[10], overflow, i, j;
    FslclHashEntry 	*hashEntryPtr;
    List_Links	*hashList;

    if (table == (FslclHashTable *)NULL || table == (FslclHashTable *)NIL) {
	return;
    }
    for (i=0; i<10; i++) {
	count[i] = 0;
    }
    overflow = 0;
    for (i = 0; i < table->size; i++) {
	j = 0;
	hashList = &(table->table[i].list);
	LIST_FORALL(hashList, (List_Links *) hashEntryPtr) {
	    j++;
	}
	if (j < 10) {
	    count[j]++;
	} else {
	    overflow++;
	}
    }

    printf("FS Name Hash Table, %d entries in %d buckets\n", 
		table->numEntries, table->size);
    for (i = 0;  i < 10; i++) {
	printf("%d buckets with %d entries\n", count[i], i);
    }
    printf("%d buckets with > 9 entries\n", overflow);
}
