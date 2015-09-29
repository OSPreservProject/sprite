/* 
 * Hash_EnumFirst.c --
 *
 *	Source code for the Hash_EnumFirst library procedure.
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
static char rcsid[] = "$Header: Hash_EnumFirst.c,v 1.1 88/06/20 09:30:23 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "hash.h"
#include "list.h"

/*
 *---------------------------------------------------------
 *
 * Hash_EnumFirst --
 *	This procedure sets things up for a complete search
 *	of all entries recorded in the hash table.
 *
 * Results:	
 *	The return value is the address of the first entry in
 *	the hash table, or NULL if the table is empty.
 *
 * Side Effects:
 *	The information in hashSearchPtr is initialized so that successive
 *	calls to Hash_Next will return successive HashEntry's
 *	from the table.
 *
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_EnumFirst(tablePtr, hashSearchPtr)
    Hash_Table *tablePtr;			/* Table to be searched. */
    register Hash_Search *hashSearchPtr;	/* Area in which to keep state 
						 * about search.*/
{
    hashSearchPtr->tablePtr = tablePtr;
    hashSearchPtr->nextIndex = 0;
    hashSearchPtr->hashEntryPtr = (Hash_Entry *) NULL;
    return Hash_EnumNext(hashSearchPtr);
}
