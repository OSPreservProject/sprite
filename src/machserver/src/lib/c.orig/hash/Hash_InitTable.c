/* 
 * Hash_InitTable.c --
 *
 *	Source code for the Hash_InitTable library procedure.
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
static char rcsid[] = "$Header: Hash_InitTable.c,v 1.3 88/07/28 17:57:28 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <hash.h>
#include <list.h>
#include <stdlib.h>

/*
 *---------------------------------------------------------
 * 
 * Hash_InitTable --
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
Hash_InitTable(tablePtr, numBuckets, keyType)
    register Hash_Table *tablePtr;	/* Structure to use to hold table. */
    int 	        numBuckets;	/* How many buckets to create for
					 * starters. This number is rounded
					 * up to a power of two.   If <= 0,
					 * a reasonable default is chosen.
					 * The table will grow in size later
					 * as needed. */
    int 	        keyType;	/* HASH_STRING_KEYS means that key
    					 * values passed to HashFind will be
					 * strings, passed via a (char *)
					 * pointer.  HASH_ONE_WORD_KEYS means
					 * that key values will be any
					 * one-word value passed as Address.
			 		 * > 1 means that key values will be 
				 	 * multi-word values whose address is
					 * passed as Address.  In this last
					 * case, keyType is the number of
					 * words in the key, not the number
					 * of bytes. */
{
    register	int 		i;
    register	List_Links 	*bucketPtr;

    /* 
     * Round up the size to a power of two. 
     */

    if (numBuckets <= 0) {
	numBuckets = 16;
    }
    tablePtr->numEntries = 0;
    tablePtr->keyType = keyType;
    tablePtr->size = 2;
    tablePtr->mask = 1;
    tablePtr->downShift = 29;
    while (tablePtr->size < numBuckets) {
	tablePtr->size <<= 1;
	tablePtr->mask = (tablePtr->mask << 1) + 1;
	tablePtr->downShift--;
    }

    tablePtr->bucketPtr = (List_Links *) malloc((unsigned) (sizeof(List_Links)
	    * tablePtr->size));
    for (i=0, bucketPtr = tablePtr->bucketPtr; i < tablePtr->size;
	    i++, bucketPtr++) {
	List_Init(bucketPtr);
    }
}
