/* 
 * Hash_PrintStats.c --
 *
 *	Source code for the Hash_PrintStats library procedure.
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
static char rcsid[] = "$Header: Hash_PrintStats.c,v 1.2 88/07/25 10:53:41 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <hash.h>
#include <list.h>
#include <stdio.h>


/*
 *---------------------------------------------------------
 *
 * Hash_PrintStats --
 *
 *	This routine calls a caller-supplied procedure to print
 *	statistics about the current bucket situation.
 *
 * Results:	
 *	None.
 *
 * Side Effects:	
 *	Proc gets called (potentially many times) to output information
 *	about the hash table. It must have the following calling sequence:
 *
 *	void
 *	proc(clientData, string)
 *	    ClientData clientData;
 *	    char *string;
 *	{
 *	}
 *
 *	In each call, clientData is the same as the clientData argument
 *	to this procedure, and string is a null-terminated string of
 *	characters to output.
 *
 *---------------------------------------------------------
 */

void
Hash_PrintStats(tablePtr, proc, clientData)
    Hash_Table *tablePtr;		/* Table for which to print info. */
    void (*proc)();			/* Procedure to call to do actual
    					 * I/O. */
{
    int count[10], overflow, i, j;
    char msg[100];
    Hash_Entry 	*hashEntryPtr;
    List_Links	*hashList;

    for (i=0; i<10; i++) {
	count[i] = 0;
    }
    overflow = 0;
    for (i = 0; i < tablePtr->size; i++) {
	j = 0;
	hashList = &(tablePtr->bucketPtr[i]);
	LIST_FORALL(hashList, (List_Links *) hashEntryPtr) {
	    j++;
	}
	if (j < 10) {
	    count[j]++;
	} else {
	    overflow++;
	}
    }

    sprintf(msg, "Entries in table %d number of buckets %d\n", 
		tablePtr->numEntries, tablePtr->size);
    (*proc)(clientData, msg);
    for (i = 0;  i < 10; i++) {
	sprintf(msg, "Number of buckets with %d entries: %d.\n",
		i, count[i]);
	(*proc)(clientData, msg);
    }
    sprintf(msg, "Number of buckets with > 9 entries: %d.\n",
	    overflow);
    (*proc)(clientData, msg);
}
