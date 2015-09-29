/* 
 * Hash_FindEntry.c --
 *
 *	Source code for the Hash_FindEntry library procedure.
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
static char rcsid[] = "$Header: Hash_FindEntry.c,v 1.1 88/06/20 09:30:24 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "hash.h"
#include "list.h"

/*
 * Utility procedures defined in other files:
 */

extern Hash_Entry *	HashChainSearch();
extern int		Hash();

/*
 *---------------------------------------------------------
 *
 * Hash_FindEntry --
 *
 * 	Searches a hash table for an entry corresponding to key.
 *
 * Results:
 *	The return value is a pointer to the entry for key,
 *	if key was present in the table.  If key was not
 *	present, NULL is returned.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_FindEntry(tablePtr, key)
    Hash_Table *tablePtr;	/* Hash table to search. */
    Address key;		/* A hash key. */
{
    return(HashChainSearch(tablePtr, key,
	    &(tablePtr->bucketPtr[Hash(tablePtr, key)])));
}
