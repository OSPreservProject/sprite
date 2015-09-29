/* 
 * Hash_CreateEntry.c --
 *
 *	Source code for the Hash_CreateEntry library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/hash/RCS/Hash_CreateEntry.c,v 1.3 88/07/28 17:57:23 ouster Exp Locker: mgbaker $ SPRITE (Berkeley)";
#endif not lint

#include <hash.h>
#include <list.h>
#include <stdlib.h>
#include <string.h>

/*
 * Utility procedures defined in other files:
 */

extern Hash_Entry *	HashChainSearch();
extern int		Hash();

/* 
 * The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
 */

static rebuildLimit = 3;

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

static void
RebuildTable(tablePtr)
    Hash_Table 	*tablePtr;		/* Table to be enlarged. */
{
    int 		 oldSize;
    int 		 bucket;
    List_Links		 *oldBucketPtr;
    register Hash_Entry  *hashEntryPtr;
    register List_Links	 *oldHashList;

    oldBucketPtr = tablePtr->bucketPtr;
    oldSize = tablePtr->size;

    /* 
     * Build a new table 4 times as large as the old one. 
     */

    Hash_InitTable(tablePtr, tablePtr->size * 4, tablePtr->keyType);

    for (oldHashList = oldBucketPtr; oldSize > 0; oldSize--, oldHashList++) {
	while (!List_IsEmpty(oldHashList)) {
	    hashEntryPtr = (Hash_Entry *) List_First(oldHashList);
	    List_Remove((List_Links *) hashEntryPtr);
	    switch (tablePtr->keyType) {
		case HASH_STRING_KEYS:
		    bucket = Hash(tablePtr, (Address) hashEntryPtr->key.name);
		    break;
		case HASH_ONE_WORD_KEYS:
		    bucket = Hash(tablePtr, (Address) hashEntryPtr->key.ptr);
		    break;
		default:
		    bucket = Hash(tablePtr, (Address) hashEntryPtr->key.words);
		    break;
	    }
	    List_Insert((List_Links *) hashEntryPtr, 
		LIST_ATFRONT(&(tablePtr->bucketPtr[bucket])));
	    tablePtr->numEntries++;
	}
    }

    free((Address) oldBucketPtr);
}

/*
 *---------------------------------------------------------
 *
 * Hash_CreateEntry --
 *
 *	Searches a hash table for an entry corresponding to
 *	key.  If no entry is found, then one is created.
 *
 * Results:
 *	The return value is a pointer to the entry.  If *newPtr
 *	isn't NULL, then *newPtr is filled in with TRUE if a
 *	new entry was created, and FALSE if an entry already existed
 *	with the given key.
 *
 * Side Effects:
 *	Memory may be allocated, and the hash buckets may be modified.
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_CreateEntry(tablePtr, key, newPtr)
    register Hash_Table *tablePtr;	/* Hash table to search. */
    Address key;			/* A hash key. */
    Boolean *newPtr;			/* Filled in with TRUE if new entry
    					 * created, FALSE otherwise. */
{
    register Hash_Entry *hashEntryPtr;
    register int 	*hashKeyPtr;
    register int 	*keyPtr;
    List_Links 		*hashList;

    keyPtr = (int *) key;

    hashList = &(tablePtr->bucketPtr[Hash(tablePtr, (Address) keyPtr)]);
    hashEntryPtr = HashChainSearch(tablePtr, (Address) keyPtr, hashList);

    if (hashEntryPtr != (Hash_Entry *) NULL) {
	if (newPtr != NULL) {
	    *newPtr = FALSE;
	}
    	return hashEntryPtr;
    }

    /* 
     * The desired entry isn't there.  Before allocating a new entry,
     * see if we're overloading the buckets.  If so, then make a
     * bigger table.
     */

    if (tablePtr->numEntries >= rebuildLimit * tablePtr->size) {
	RebuildTable(tablePtr);
	hashList = &(tablePtr->bucketPtr[Hash(tablePtr, (Address) keyPtr)]);
    }
    tablePtr->numEntries += 1;

    /*
     * Not there, we have to allocate.  If the string is longer
     * than 3 bytes, then we have to allocate extra space in the
     * entry.
     */

    switch (tablePtr->keyType) {
	case HASH_STRING_KEYS:
	    hashEntryPtr = (Hash_Entry *) malloc((unsigned)
		    (sizeof(Hash_Entry) + strlen((char *) keyPtr) - 3));
	    strcpy(hashEntryPtr->key.name, (char *) keyPtr);
	    break;
	case HASH_ONE_WORD_KEYS:
	    hashEntryPtr = (Hash_Entry *) malloc(sizeof(Hash_Entry));
	    hashEntryPtr->key.ptr = (Address) keyPtr;
	    break;
	case 2:
	    hashEntryPtr = 
		(Hash_Entry *) malloc(sizeof(Hash_Entry) + sizeof(int));
	    hashKeyPtr = hashEntryPtr->key.words;
	    *hashKeyPtr++ = *keyPtr++;
	    *hashKeyPtr = *keyPtr;
	    break;
	default: {
	    register 	n;

	    n = tablePtr->keyType;
	    hashEntryPtr = (Hash_Entry *) 
		    malloc((unsigned) (sizeof(Hash_Entry)
		    + (n - 1) * sizeof(int)));
	    hashKeyPtr = hashEntryPtr->key.words;
	    do { 
		*hashKeyPtr++ = *keyPtr++; 
	    } while (--n);
	    break;
	}
    }

    hashEntryPtr->clientData = (ClientData) NULL;
    List_Insert((List_Links *) hashEntryPtr, LIST_ATFRONT(hashList));

    if (newPtr != NULL) {
	*newPtr = TRUE;
    }
    return hashEntryPtr;
}
