/* 
 * devDebugMem.c --
 *
 *	Initially, this module was created to detect bugs associated with
 *	memory allocation/deallocation (i.e. memory leakage, freeing of
 *	unallocated memory locations etc...).
 *	Since then, it has been expanded to deal with the problem that
 *	'free' can not be called from an interrupt routine.
 *	The 'Free' procedure defined in this module, places the mem block to
 *	be freed on a to-be-freed list rather than calling 'free' directly.
 *	All mem blocks on the to-be-freed list are freed the next time
 *	'Malloc' is called.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include "hash.h"

extern char *malloc();

#define FREE_LIST_SIZE	4096
#define LOCK_TABLE_SIZE	4096

#ifdef TESTING
static Hash_Table _debugMemTable;
static _debugMemCount = 0;
#endif TESTING
static Sync_Semaphore _debugMemMutex =
	Sync_SemInitStatic("devRaidLock.c: DebugMem Lock Table");
static char *freeList[FREE_LIST_SIZE];
static int freeListIndex = 0;


/*
 *----------------------------------------------------------------------
 *
 * InitDebugMem --
 *
 *	This procedure should be the first procedure to be called from this
 *	module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes data structures.
 *
 *----------------------------------------------------------------------
 */

void
InitDebugMem()
{
#ifdef TESTING
    static int	initialized = 0;

    MASTER_LOCK(&_debugMemMutex);
    if (initialized == 0) {
	initialized = 1;
        MASTER_UNLOCK(&_debugMemMutex);
        Hash_Init(&_debugMemTable, 1000, 1);
    } else {
	MASTER_UNLOCK(&_debugMemMutex);
    }
#endif TESTING
}


/*
 *----------------------------------------------------------------------
 *
 * Free --
 *
 *	Replacement procedure for 'free' which can be called from interrupt
 *	routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Places mem block to be freed on a global to-be-freed list.
 *
 *----------------------------------------------------------------------
 */

void
Free(memPtr)
    char *memPtr;
{
    MASTER_LOCK(&_debugMemMutex);
    if (freeListIndex == FREE_LIST_SIZE) {
        MASTER_UNLOCK(&_debugMemMutex);
	printf("Error: Free: free list overflow\n");
    } else {
    	freeList[freeListIndex] = memPtr;
	freeListIndex++;
        MASTER_UNLOCK(&_debugMemMutex);
    }
#ifdef TESTING
{
    Hash_Entry		*hashEntryPtr;

    MASTER_LOCK(&_debugMemMutex);
    if (--_debugMemCount == 0) {
        MASTER_UNLOCK(&_debugMemMutex);
	printf("Msg: Free: _debugMemCount == 0\n");
    } else {
        MASTER_UNLOCK(&_debugMemMutex);
    }
    MASTER_LOCK(&_debugMemMutex);
    hashEntryPtr = Hash_Find(&_debugMemTable, (Address) memPtr);
    if ( Hash_GetValue(hashEntryPtr) == (char *) NIL ) {
        MASTER_UNLOCK(&_debugMemMutex);
	printf("Error: Free: unallocated mem=%x freed\n", memPtr);
    } else {
        Hash_Delete(&_debugMemTable, hashEntryPtr);
        MASTER_UNLOCK(&_debugMemMutex);
    }
}
#endif TESTING
}


/*
 *----------------------------------------------------------------------
 *
 * Malloc --
 *
 *	Replacement procedure for 'malloc'.
 *	Should be used with 'Free'.
 *
 * Results:
 *	Newly allocated memory block.
 *
 * Side effects:
 *	Frees mem blocks on global to-be-freed list
 *	Calls 'malloc' to allocate memory.
 *
 *----------------------------------------------------------------------
 */

char *
Malloc(size)
    unsigned size;
{
    char *memPtr;

    MASTER_LOCK(&_debugMemMutex);
    for (; freeListIndex > 0;) {
	freeListIndex--;
	free(freeList[freeListIndex]);
    }
    MASTER_UNLOCK(&_debugMemMutex);

    memPtr = malloc(size);

#ifdef TESTING
{
    Hash_Entry		*hashEntryPtr;

    MASTER_LOCK(&_debugMemMutex);
    if (_debugMemCount++ == 0) {
        MASTER_UNLOCK(&_debugMemMutex);
	printf("Msg: Malloc: _debugMemCount == 0\n");
    } else {
        MASTER_UNLOCK(&_debugMemMutex);
    }
    MASTER_LOCK(&_debugMemMutex);
    hashEntryPtr = Hash_Find(&_debugMemTable, memPtr);
    if ( Hash_GetValue(hashEntryPtr) != (char *) NIL ) {
        MASTER_UNLOCK(&_debugMemMutex);
	printf("Error: Malloc: allocated mem=%x reallocated\n", memPtr);
    } else {
        MASTER_UNLOCK(&_debugMemMutex);
    }
    Hash_SetValue(hashEntryPtr, memPtr);
}
#endif TESTING
    return(memPtr);
}
