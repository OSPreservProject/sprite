/* hash.c --
 *
 * 	This module contains routines to manipulate a hash table.
 * 	See hash.h for a definition of the structure of the hash
 * 	table.  Hash tables grow automatically as the amount of
 * 	information increases.
 *
 * Copyright (C) 1983 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif  not lint

#include "sprite.h"
#include "hash.h"
#include "mem.h"
#include "string.h"
#include "byte.h"
#include "list.h"
#include "sys.h"

void RebuildTable();

/* 
 * The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
 */

static rebuildLimit = 3;


/*
 *---------------------------------------------------------
 * 
 * Hash_Init --
 *
 *	This routine just sets up the hash table.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Memory is allocated for the initial bucket area.
 *
 *---------------------------------------------------------
 */

void
Hash_Init(table, numBuckets, ptrKeys)
    register Hash_Table	*table;
    int			numBuckets;	/* How many buckets to create for 
					 * starters. This number is rounded up 
					 * to a power of two. */
    int 	        ptrKeys;	/* 0 means that key values passed to 
					 * HashFind will be strings, passed via
					 * a (char *) pointer.  1 means that 
					 * key values will be any one-word 
					 * value passed as Address.  > 1 means 
					 * that key values will be multi-word 
					 * values whose address is passed as 
					 * Address.  In this last case, ptrKeys 
					 * is the number of words in the key, 
					 * not the number of bytes. */
{
    register	int 		i;
    register	Hash_Bucket 	*tablePtr;

    /* 
     * Round up the size to a power of two. 
     */

    if (numBuckets < 0) {
	numBuckets = -numBuckets;
    }
    table->numEntries = 0;
    table->version = 0;
    table->ptrKeys = ptrKeys;
    table->size = 2;
    table->mask = 1;
    table->downShift = 29;
    while (table->size < numBuckets) {
	table->size <<= 1;
	table->mask = (table->mask << 1) + 1;
	table->downShift--;
    }

    table->table = (Hash_Bucket *) Mem_Alloc(sizeof(Hash_Bucket) * table->size);
    for (i=0, tablePtr = table->table; i < table->size; i++, tablePtr++) {
	List_Init(&(tablePtr->list));
	tablePtr->version = 0;
    }
}


/*
 *---------------------------------------------------------
 *
 * Hash --
 *	This is a local procedure to compute a hash table
 *	bucket address based on a string value.
 *
 * Results:
 *	The return value is an integer between 0 and size - 1.
 *
 * Side Effects:	
 *	None.
 *
 * Design:
 *	It is expected that most keys are decimal numbers,
 *	so the algorithm behaves accordingly.  The randomizing
 *	code is stolen straight from the rand library routine.
 *
 *---------------------------------------------------------
 */

static int
Hash(table, key)
    register Hash_Table *table;
    register char 	*key;
{
    register int 	i = 0;
    register int 	j;
    register unsigned 	*intPtr;

    switch (table->ptrKeys) {
	case 0:
	    while (*key != 0) {
		i = (i * 10) + (*key++ - '0');
	    }
	    break;
	case 1:
	    i = (int) key;
	    break;
	case 2:
	    i = ((unsigned *) key)[0] + ((unsigned *) key)[1];
	    break;
	default:
	    j = table->ptrKeys;
	    intPtr = (unsigned *) key;
	    do { 
		i += *intPtr++; 
		j--;
	    } while (j > 0);
	    break;
    }


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

static Hash_Entry *
ChainSearch(table, key, hashList)
    Hash_Table 		*table;	/* Hash table to search. */
    register Address	key;	/* A hash key. */
    register List_Links *hashList;
{
    register Hash_Entry *hashEntryPtr;
    register unsigned 	*hashKeyPtr;
    unsigned 		*keyPtr;
    register int	numKeys;

    numKeys = table->ptrKeys;
    LIST_FORALL(hashList, (List_Links *) hashEntryPtr) {
	switch (numKeys) {
	    case 0:
		if (String_Compare(hashEntryPtr->key.name, key) == 0) {
		    return(hashEntryPtr);
		}
		break;
	    case 1:
		if (hashEntryPtr->key.ptr == key) {
		    return(hashEntryPtr);
		}
		break;
	    case 2:
		hashKeyPtr = hashEntryPtr->key.words;
		keyPtr = (unsigned *) key;
		if (hashKeyPtr[0] == keyPtr[0] && hashKeyPtr[1] == keyPtr[1]) {
		    return(hashEntryPtr);
		}
		break;
	    default:
		if (Byte_Compare(numKeys * sizeof(int), 
			    (Address) hashEntryPtr->key.words,
			    (Address) key)) {
		    return(hashEntryPtr);
		}
		break;
	}
    }

    /* 
     * The desired entry isn't there 
     */

    return ((Hash_Entry *) NIL);
}



/*
 *---------------------------------------------------------
 *
 * Hash_LookOnly --
 *
 * 	Searches a hash table for an entry corresponding to string.
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

Hash_Entry *
Hash_LookOnly(table, key)
    register Hash_Table *table;	/* Hash table to search. */
    Address 		key;	/* A hash key. */
{
    return(ChainSearch(table, key, &(table->table[Hash(table, key)].list)));
}


/*
 *---------------------------------------------------------
 *
 * Hash_Find --
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

Hash_Entry *
Hash_Find(table, key)
    register	Hash_Table *table;	/* Hash table to search. */
    Address 		   key;		/* A hash key. */
{
    register 	unsigned 	*hashKeyPtr;
    register 	unsigned 	*keyPtr;
    register 	Hash_Bucket 	*bucketPtr;
    Hash_Entry			*hashEntryPtr;

    keyPtr = (unsigned *) key;

    bucketPtr = &(table->table[Hash(table, (Address) keyPtr)]);
    hashEntryPtr = ChainSearch(table, (Address) keyPtr, &(bucketPtr->list));

    if (hashEntryPtr != (Hash_Entry *) NIL) {
	return(hashEntryPtr);
    }

    /* 
     * The desired entry isn't there.  Before allocating a new entry,
     * see if we're overloading the buckets.  If so, then make a
     * bigger table.
     */

    if (table->numEntries >= rebuildLimit * table->size) {
	RebuildTable(table);
	bucketPtr = &(table->table[Hash(table, (Address) keyPtr)]);
    }
    table->numEntries += 1;

    /*
     * Not there, we have to allocate.  If the string is longer
     * than 3 bytes, then we have to allocate extra space in the
     * entry.
     */

    switch (table->ptrKeys) {
	case 0:
	    hashEntryPtr = (Hash_Entry *) Mem_Alloc(sizeof(Hash_Entry) + 
				String_Length((Address) keyPtr) - 3);
	    (void)String_Copy((char *) keyPtr, (char *) hashEntryPtr->key.name);
	    break;
	case 1:
	    hashEntryPtr = (Hash_Entry *) Mem_Alloc(sizeof(Hash_Entry));
	    hashEntryPtr->key.ptr = (Address) keyPtr;
	    break;
	case 2:
	    hashEntryPtr = 
		(Hash_Entry *) Mem_Alloc(sizeof(Hash_Entry) + sizeof(unsigned));
	    hashKeyPtr = hashEntryPtr->key.words;
	    *hashKeyPtr++ = *keyPtr++;
	    *hashKeyPtr = *keyPtr;
	    break;
	default: {
	    register 	n;

	    n = table->ptrKeys;
	    hashEntryPtr = (Hash_Entry *) 
		    Mem_Alloc(sizeof(Hash_Entry) + (n - 1) * sizeof(unsigned));
	    hashKeyPtr = hashEntryPtr->key.words;
	    do { 
		*hashKeyPtr++ = *keyPtr++; 
	    } while (--n);
	    break;
	}
    }

    hashEntryPtr->value = (Address) NIL;
    hashEntryPtr->bucketPtr = bucketPtr;
    List_Insert((List_Links *) hashEntryPtr, LIST_ATFRONT(&(bucketPtr->list)));
    bucketPtr->version++;

    return(hashEntryPtr);
}



/*
 *---------------------------------------------------------
 *
 * Hash_Delete --
 *
 * 	Delete the given hash table entry and free memory associated with
 *	it.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Hash chain that entry lives in is modified and memory is freed.
 *
 *---------------------------------------------------------
 */

void
Hash_Delete(table, hashEntryPtr)
    Hash_Table			*table;
    register	Hash_Entry	*hashEntryPtr;
{
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	List_Remove(hashEntryPtr);
	hashEntryPtr->bucketPtr->version++;
	Mem_Free(hashEntryPtr);
	table->numEntries--;
    }
}


/*
 *---------------------------------------------------------
 *
 * RebuildTable --
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

void
RebuildTable(table)
    register	Hash_Table 	*table;		/* Table to be enlarged. */
{
    register	Hash_Bucket	*oldTable;
    register	Hash_Entry  	*hashEntryPtr;
    register	int 		oldSize;
    int 		 	bucket;
    Hash_Bucket		 	*saveTable;
    Hash_Bucket		 	*bucketPtr;
    int			 	version;

    saveTable = table->table;
    oldSize = table->size;
    version = table->version + 1;

    /* 
     * Build a new table 4 times as large as the old one. 
     */

    Hash_Init(table, table->size * 4, table->ptrKeys);
    table->version = version;

    for (oldTable = saveTable; oldSize > 0; oldSize--, oldTable++) {
	while (!List_IsEmpty(&(oldTable->list))) {
	    hashEntryPtr = (Hash_Entry *) List_First(&(oldTable->list));
	    List_Remove((List_Links *) hashEntryPtr);
	    switch (table->ptrKeys) {
		case 0:
		    bucket = Hash(table, (Address) hashEntryPtr->key.name);
		    break;
		case 1:
		    bucket = Hash(table, (Address) hashEntryPtr->key.ptr);
		    break;
		default:
		    bucket = Hash(table, (Address) hashEntryPtr->key.words);
		    break;
	    }
	    bucketPtr = &(table->table[bucket]);
	    List_Insert((List_Links *) hashEntryPtr, 
		LIST_ATFRONT(&(bucketPtr->list)));
	    hashEntryPtr->bucketPtr = bucketPtr;
	    table->numEntries++;
	}
    }

    Mem_Free((Address) saveTable);
}


/*
 *---------------------------------------------------------
 *
 * HashStats --
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
Hash_Stats(table)
    Hash_Table *table;
{
    int count[10], overflow, i, j;
    Hash_Entry 	*hashEntryPtr;
    List_Links	*hashList;

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

    Sys_Printf("Entries in table %d number of buckets %d\n", 
		table->numEntries, table->size);
    for (i = 0;  i < 10; i++) {
	Sys_Printf("Number of buckets with %d entries: %d.\n", i, count[i]);
    }
    Sys_Printf("Number of buckets with > 9 entries: %d.\n", overflow);
}


/*
 *---------------------------------------------------------
 *
 * Hash_StartSearch --
 *
 *	This procedure sets things up for a complete search
 *	of all entries recorded in the hash table.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	The version number of the table is set to -1 so that the structure
 *	will be initialized in the first Hash_Next.
 *
 *---------------------------------------------------------
 */

void
Hash_StartSearch(hashSearchPtr)
    register Hash_Search *hashSearchPtr; /* Area in which to keep state 
					    about search.*/
{
    hashSearchPtr->tableVersion = -1;
}

/*
 *---------------------------------------------------------
 *
 * Hash_Next --
 *
 *    This procedure returns successive entries in the hash table.
 *
 * Results:
 *    The return value is a pointer to the next HashEntry
 *    in the table, or NIL when the end of the table is
 *    reached.
 *
 * Side Effects:
 *    The information in hashSearchPtr is modified to advance to the
 *    next entry.
 *
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_Next(table, hashSearchPtr)
    register Hash_Table  *table;	 /* Table to be searched. */
    register Hash_Search *hashSearchPtr; /* Area used to keep state about 
					    search. */
{
    register List_Links *hashList;
    register Hash_Entry *hashEntryPtr;
    Hash_Bucket		*bucketPtr;

    /*
     * Check version number of the hash table.
     */
    if (hashSearchPtr->tableVersion < table->version) {
	hashSearchPtr->nextIndex = 0;
	hashSearchPtr->hashEntryPtr = (Hash_Entry *) NIL;
	hashSearchPtr->tableVersion = table->version;
    }
    hashEntryPtr = hashSearchPtr->hashEntryPtr;

    /* 
     * Now check version number of the hash bucket.
     */
    if (hashEntryPtr != (Hash_Entry *) NIL &&
	!List_IsAtEnd(hashSearchPtr->hashList, (List_Links *) hashEntryPtr) &&
	hashEntryPtr->bucketPtr->version > hashSearchPtr->bucketVersion) {
	hashEntryPtr = (Hash_Entry *) NIL;
	hashSearchPtr->nextIndex--;
    }

    while (hashEntryPtr == (Hash_Entry *) NIL ||
	   List_IsAtEnd(hashSearchPtr->hashList, (List_Links *) hashEntryPtr)) {
	if (hashSearchPtr->nextIndex >= table->size) {
	    return((Hash_Entry *) NIL);
	}
	bucketPtr = &(table->table[hashSearchPtr->nextIndex]);
	hashList = &(bucketPtr->list);
	hashSearchPtr->bucketVersion = bucketPtr->version;
	hashSearchPtr->nextIndex++;
	if (!List_IsEmpty(hashList)) {
	    hashEntryPtr = (Hash_Entry *) List_First(hashList);
	    hashSearchPtr->hashList = hashList;
	    break;
	}
    }

    hashSearchPtr->hashEntryPtr = 
		(Hash_Entry *) List_Next((List_Links *) hashEntryPtr);

    return(hashEntryPtr);
}


/*
 *---------------------------------------------------------
 *
 * Hash_Kill --
 *
 *	This routine removes everything from a hash table
 *	and frees up the memory space it occupied.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Lots of memory is freed up.
 *
 *---------------------------------------------------------
 */

void
Hash_Kill(table)
    Hash_Table *table;	/* Hash table whose space is to be freed */
{
    register	Hash_Bucket	*hashTableEnd;
    register	Hash_Entry	*hashEntryPtr;
    register	Hash_Bucket	*tablePtr;

    tablePtr = table->table;
    hashTableEnd = &(tablePtr[table->size]);
    for (; tablePtr < hashTableEnd; tablePtr++) {
	while (!List_IsEmpty(&(tablePtr->list))) {
	    hashEntryPtr = (Hash_Entry *) List_First(&(tablePtr->list));
	    List_Remove((List_Links *) hashEntryPtr);
	    Mem_Free((Address) hashEntryPtr);
	}
    }
    Mem_Free((Address) table->table);

    /*
     * Set up the hash table to cause memory faults on any future
     * access attempts until re-initialization.
     */

    table->table = (Hash_Bucket *) NIL;
}
