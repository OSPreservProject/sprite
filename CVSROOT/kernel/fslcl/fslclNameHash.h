/* fsNameHash.h --
 *
 *	Definitions for the filesystem name hash table.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 */


#ifndef	_FSNAMEHASH
#define	_FSNAMEHASH

#include "list.h"

/* 
 * The hash table includes an array of bucket list headers,
 * and some administrative information that affects the hash
 * function and comparison functions.  The table is contrained to
 * have a power of two number of entries to make the hashing go faster.
 */
typedef struct FsHashTable {
    struct FsHashBucket *table;	/* Pointer to array of List_Links. */
    List_Links		lruList;	/* The header of the LRU list */
    int 		size;		/* Actual size of array. */
    int 		numEntries;	/* Number of entries in the table. */
    int 		downShift;	/* Shift count, used in hashing 
					 * function. */
    int 		mask;		/* Used to select bits for hashing. */
} FsHashTable;

/*
 * Default size of the name hash table.
 */
extern fsNameHashSize;
#define FS_NAME_HASH_SIZE	512

/*
 * The bucket header is just a list header.
 */

typedef struct FsHashBucket {
    List_Links	list;
} FsHashBucket;

/* 
 * The following defines one entry in the hash table. 
 */

typedef struct FsHashEntry {
    List_Links	links;		/* For the list starting at the bucket header */
    FsHashBucket *bucketPtr;	/* Pointer to hash bucket for this entry */
    struct FsLruList {
	List_Links links;	/* Links for the LRU list */
	struct FsHashEntry *entryPtr;	/* Back pointer needed to get entry */
    } lru;
    FsHandleHeader *hdrPtr;	/* Pointer to handle of named component. */
    FsHandleHeader *keyHdrPtr;	/* Pointer to handle of parent directory. */
    char 	keyName[4];	/* Text name of this entry.  Note: the
				 * actual size may be longer if necessary
				 * to hold the whole string. This MUST be
				 * the last entry in the structure!!! */
} FsHashEntry;

typedef struct FsLruEntry {
    List_Links	lruList;	/* This record is used to map from the LRU */
    FsHashBucket *entryPtr;	/* List back to the hash entry */
} FsLruEntry;

/*
 * The following procedure declarations and macros
 * are the only things that should be needed outside
 * the implementation code.
 */

extern FsHashTable	*fsNameTablePtr;
extern Boolean		fsNameCaching;

extern void 		FsNameHashInit();
extern void		Fs_NameHashStats();
extern void		FsHashKill();
extern FsHashEntry 	*FsHashFind();
extern FsHashEntry 	*FsHashLookOnly();
extern void 		FsHashDelete();
extern void 		FsHashStartSearch();
extern FsHashEntry	*FsHashInsert();
extern FsHashEntry	*FsHashNext();

#define FS_HASH_LOOK_ONLY(table, string, keyHandle) \
    (fsNameCaching ? \
	FsHashLookOnly(table, string, (FsHandleHeader *)keyHandle) : \
	(FsHashEntry *)NIL)

#define FS_HASH_INSERT(table, string, keyHandle, handle) \
    (fsNameCaching ? \
	FsHashInsert(table, string, (FsHandleHeader *)keyHandle, \
	(FsHandleHeader *)handle) : (FsHashEntry *)NIL)

#define FS_HASH_DELETE(table, string, keyHandle) \
    if (fsNameCaching) { \
	FsHashDelete(table, string, (FsHandleHeader *)keyHandle); \
    }

#endif _FSNAMEHASH
