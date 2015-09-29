/* 
 * hash.c --
 *
 *	Hash package for Jaquith system
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/hash.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#define AVAIL    0
#define DELETED -1

static Hash_Node *FindHashNode   _ARGS_ ((Hash_Handle *hashPtr,
					  Hash_Key key,
					  int keyLen));
static Hash_Node *FindFreeNode   _ARGS_ ((Hash_Handle *hashPtr,
					  Hash_Key key, int keyLen));
static void       CreateHashNode _ARGS_ ((Hash_Handle *hashPtr,
					  Hash_Key key, int keyLen, 
					  Hash_ClientData datum));
static void       DeleteHashNode _ARGS_ ((Hash_Node *nodePtr,
					  int freeFlag));
static void       GrowHashTab    _ARGS_ ((Hash_Handle *hashPtr));
static void       GrowStringTab  _ARGS_ ((Hash_Handle *hashPtr));


/*
 *----------------------------------------------------------------------
 *
 * Hash_Create --
 *
 *	Initialize a hash table of specified size
 *
 * Results:
 *	ptr to hash table handle.
 *
 * Side effects:
 *	Allocates internal table space.
 *
 * Note:
 *      size of table must be odd.
 *
 *----------------------------------------------------------------------
 */

Hash_Handle *
Hash_Create(name, size, hashFunc, freeData)
    char *name;               /* name of new hash table */
    int size;                 /* number of entries */
    int (*hashFunc)();        /* hashing function to use */
    int freeData;             /* pass datum to Free when deallocating */
{
    Hash_Handle *hashPtr;

    if ((size < 0) || (size > MAXHASHSIZE) || ((size & 0x1) == 0)) {
	return NULL;
    }

    hashPtr = (Hash_Handle *)MEM_ALLOC("Hash_Create", sizeof(Hash_Handle));
    hashPtr->name = Str_Dup(name);
    hashPtr->tabSize = size;
    hashPtr->tab = (Hash_Node *)
	MEM_ALLOC("Hash_Create", size*sizeof(Hash_Node));
    /* string space: guess 8 chars/string; hash table 1/2 full */
    hashPtr->stringSize = 4*size;
    hashPtr->stringTab = (char *)
	MEM_ALLOC("Hash_Create", hashPtr->stringSize*sizeof(char));
    hashPtr->freeData = freeData;
    hashPtr->stringUsed = 0;
    hashPtr->hashFunc = hashFunc;
    hashPtr->tabGrowCnt = 0;
    hashPtr->tabFill = 0.0;
    hashPtr->stringGrowCnt = 0;
    hashPtr->stringFill = 0.0;
    hashPtr->insertCnt = 0;
    hashPtr->deleteCnt = 0;
    hashPtr->lookupCnt = 0;
    hashPtr->updateCnt = 0;
    hashPtr->probeCnt = 0;

    while (--size >= 0) {
	hashPtr->tab[size].keyLen = AVAIL;
    }

    return hashPtr;
}    


/*
 *----------------------------------------------------------------------
 *
 * Hash_Destroy --
 *
 *	Free a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases table space.
 *
 *----------------------------------------------------------------------
 */

void
Hash_Destroy(hashPtr)
    Hash_Handle *hashPtr;     /* hash table handle */
{
    int i;

    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Destroy: Null hash ptr.\n", BAIL_PRINT);
    }

    if (hashPtr->freeData) {
	for (i=0; i<hashPtr->tabSize;i++) {
	    if (hashPtr->tab[i].keyLen != AVAIL) {
		MEM_FREE("Hash_Destroy", (char *)hashPtr->tab[i].datum);
	    }
	}
    }
    MEM_FREE("Hash_Destroy", (char *)hashPtr->name);
    MEM_FREE("Hash_Destroy", (char *)hashPtr->tab);
    MEM_FREE("Hash_Destroy", (char *)hashPtr->stringTab);
    MEM_FREE("Hash_Destroy", (char *)hashPtr);

}


/*
 *----------------------------------------------------------------------
 *
 * Hash_Insert --
 *
 *	Use key to add an entry to table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Hash_Insert(hashPtr, key, keyLen, datum)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* key for insertion */
    int keyLen;               /* size of key */
    Hash_ClientData datum;    /* value of interest */
{
    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Insert: Null hash ptr.\n", BAIL_PRINT);
    }

    if (keyLen < 1) {
	Utils_Bailout("Hash_Insert: Bad key length.\n", BAIL_PRINT);
    }

    if (FindHashNode(hashPtr, key, keyLen) != (Hash_Node *)NULL) {
	return T_FAILURE;
    }

    CreateHashNode(hashPtr, key, keyLen, datum);
    hashPtr->insertCnt++;
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Hash_Update --
 *
 *	Use key to modify an entry in table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Hash_Update(hashPtr, key, keyLen, datum)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* key for insertion */
    int keyLen;               /* size of key */
    Hash_ClientData datum;    /* value of interest */
{
    Hash_Node *nodePtr;

    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Update: Null hash ptr.\n", BAIL_PRINT);
    }
    
    if (keyLen < 1) {
	Utils_Bailout("Hash_Update: Bad key length.\n", BAIL_PRINT);
    }

    if ((nodePtr=FindHashNode(hashPtr, key, keyLen)) ==	(Hash_Node *)NULL) {
	return T_FAILURE;
    }

    nodePtr->datum = datum;
    hashPtr->updateCnt++;
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Hash_Delete --
 *
 *	Remove an item from the table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Hash_Delete(hashPtr, key, keyLen)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* key for item to be removed */
    int keyLen;               /* size of key */
{
    Hash_Node *nodePtr;

    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Delete: Null hash ptr.\n", BAIL_PRINT);
    }
    
    if (keyLen < 1) {
	Utils_Bailout("Hash_Delete: Bad key length.\n", BAIL_PRINT);
    }

    if ((nodePtr=FindHashNode(hashPtr, key, keyLen)) ==	(Hash_Node *)NULL) {
	return T_FAILURE;
    }

    DeleteHashNode(nodePtr, hashPtr->freeData);
    hashPtr->deleteCnt++;
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Hash_Lookup --
 *
 *	Find an item by key in the table
 *
 * Results:
 *	Corresponding client datum and a return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Hash_Lookup(hashPtr, key, keyLen, datumPtr)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* key for item to be lookup up */
    int keyLen;               /* size of key */
    Hash_ClientData *datumPtr;/* return value */
{
    Hash_Node *nodePtr;

    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Lookup: Null hash ptr.\n", BAIL_PRINT);
    }
    
    if (keyLen < 1) {
	Utils_Bailout("Hash_Lookup: Bad key length.\n", BAIL_PRINT);
    }

    if ((nodePtr=FindHashNode(hashPtr, key, keyLen)) ==	(Hash_Node *)NULL) {
	return T_FAILURE;
    }

    *datumPtr = nodePtr->datum;
    hashPtr->lookupCnt++;
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Hash_Iterate --
 *
 *	Invoke callback function for each item in table
 *
 * Results:
 *	
 *
 * Side effects:
 *	Calls user function.
 *
 * The callback function is provided with:                   
 *   hash ptr  : so it knows what table it's operating on        
 *   key    : for processing                                  
 *   keyLen : for processing                                  
 *   datum  : for processing                                  
 *   callVal: Arbitray ptr provided by Hash_Iterate caller.
 *                                                           
 * Callback routine must not call Hash_*  for this table.
 *                                                           
 * We respond to the return code as follows:                 
 *    = HASH_ITER_REMOVE_STOP : remove item from table and stop
 *    = HASH_ITER_REMOVE_CONT : remove item from table and continue 
 *    = HASH_ITER_STOP : stop
 *    = HASH_ITER_CONTINUE  : do nothing to the item and continue
 *
 *----------------------------------------------------------------------
 */

void
Hash_Iterate(hashPtr, func, callVal) 
    Hash_Handle *hashPtr;        /* hash table handle */
    int (*func)();            /* callback function */
    int *callVal;             /* arbitrary value passed to func */
{
    int i;
    int retCode;
    Hash_Node *nodePtr;
    
    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Iterate: Null hash ptr.\n", BAIL_PRINT);
    }

    for (i=0,nodePtr=hashPtr->tab; i<hashPtr->tabSize; i++,nodePtr++) {
	if ((nodePtr->keyLen != DELETED) && (nodePtr->keyLen != AVAIL)) {
	    retCode = (*func)(hashPtr, nodePtr->key,
		    nodePtr->keyLen, nodePtr->datum, callVal);
	    if (retCode == HASH_ITER_STOP) {
		break;
	    } else if (retCode == HASH_ITER_CONTINUE) {
		/* do nothing */
	    } else if (retCode == HASH_ITER_REMOVE_CONT) {
		DeleteHashNode(nodePtr, hashPtr->freeData);
	    } else if (retCode == HASH_ITER_REMOVE_STOP) {
		DeleteHashNode(nodePtr, hashPtr->freeData);
		break;
	    } else {
		Utils_Bailout("Hash_Iterate: Unknown return code from callback function.\n", BAIL_PRINT);
	    }
	}
    }

}



/*
 *----------------------------------------------------------------------
 *
 * Hash_Stats --
 *
 *	Return some mildly interesting numbers about performance
 *
 * Results:
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Hash_Stats(hashPtr, statPtr)
    Hash_Handle *hashPtr;        /* hash table handle */
    Hash_Stat *statPtr;       /* receiving data structure */
{

    if (hashPtr == (Hash_Handle *)NULL) {
	Utils_Bailout("Hash_Stats: Null hash ptr.\n", BAIL_PRINT);
    }

    statPtr->tabSize = hashPtr->tabSize;
    statPtr->tabGrowCnt = hashPtr->tabGrowCnt;
    if (hashPtr->tabGrowCnt == 0) {
	statPtr->avgTabFill = 0.0;
    } else {
	statPtr->avgTabFill =
	    hashPtr->tabFill / (float)hashPtr->tabGrowCnt;
    }

    statPtr->stringSize = hashPtr->stringSize;
    statPtr->stringGrowCnt = hashPtr->stringGrowCnt;
    if (hashPtr->stringGrowCnt == 0) {
	statPtr->avgStringFill = 0.0;
    } else {
	statPtr->avgStringFill =
	    hashPtr->stringFill / (float)hashPtr->stringGrowCnt;
    }

    statPtr->insertCnt = hashPtr->insertCnt;
    statPtr->deleteCnt = hashPtr->deleteCnt;
    statPtr->lookupCnt = hashPtr->lookupCnt;
    statPtr->updateCnt = hashPtr->updateCnt;
    statPtr->avgProbeCnt = (hashPtr->insertCnt+
			    hashPtr->deleteCnt+
			    hashPtr->lookupCnt) / (float)hashPtr->probeCnt;
}


/*
 *----------------------------------------------------------------------
 *
 * FindHashNode --
 *
 *	Internal location routine
 *
 * Results:
 *	returns a pointer to a Hash_Node.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Stupid algorithm. Probe function is just a linear search,
 *      skipping every other location.
 *
 *----------------------------------------------------------------------
 */

static Hash_Node *
FindHashNode(hashPtr, key, keyLen)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* lookup key */
    int keyLen;               /* size of key */
{
    int loc;
    int startLoc;
    char *nodeString;
    int nodeStringLen;

    loc = startLoc = (*hashPtr->hashFunc)(key, keyLen, hashPtr->tabSize);
/*
    fprintf(stderr,"hash: lookup key %s %d at loc %d...",  key, keyLen, loc);
*/
    do {
	nodeString = hashPtr->tab[loc].key;
	nodeStringLen = hashPtr->tab[loc].keyLen;
	if ((hashPtr->tab[loc].keyLen == keyLen) &&
	    (bcmp(nodeString, key, keyLen) == 0)) {
/*
	    fprintf(stderr,"found at %d, val %x\n",
		    loc, hashPtr->tab[loc].datum);
*/
	    return &hashPtr->tab[loc];
	}
	hashPtr->probeCnt++;
	loc = (loc+2) % hashPtr->tabSize;
    } while ((loc != startLoc) && (nodeStringLen != AVAIL));
/*
    fprintf(stderr,"not found\n");
*/
    return NULL;

}


/*
 *----------------------------------------------------------------------
 *
 * FindFreeNode --
 *
 *	Internal location routine
 *
 * Results:
 *	returns a pointer to available Hash_Node.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Stupid algorithm. Probe function is just a linear search,
 *      skipping every other location.
 *
 *----------------------------------------------------------------------
 */

static Hash_Node *
FindFreeNode(hashPtr, key, keyLen)
    Hash_Handle *hashPtr;        /* hash table handle */
    Hash_Key key;          /* lookup key */
    int keyLen;               /* size of key */
{
    int loc;
    int startLoc;
    int nodeStringLen;

    loc = startLoc =
	(*hashPtr->hashFunc)(key, keyLen, hashPtr->tabSize);

    do {
	nodeStringLen = hashPtr->tab[loc].keyLen;
	if ((nodeStringLen == AVAIL) || (nodeStringLen == DELETED)) {
	    return &hashPtr->tab[loc];
	}
	loc = (loc+2) % hashPtr->tabSize;
    } while (loc != startLoc);

    return NULL;

}


/*
 *----------------------------------------------------------------------
 *
 * CreateHashNode --
 *
 *	Internal allocation routine
 *
 * Results:
 *	returns a pointer to a new filled in Hash_Node
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Stupid algorithm.
 *
 *----------------------------------------------------------------------
 */

static void
CreateHashNode(hashPtr, key, keyLen, datum)
    Hash_Handle *hashPtr;     /* hash table handle */
    Hash_Key key;             /* new key...        */
    Hash_ClientData datum;    /*           and value */
{
    Hash_Node *nodePtr;
    
    if ((nodePtr=FindFreeNode(hashPtr, key, keyLen)) == (Hash_Node *)NULL) {
	GrowHashTab(hashPtr);
	if ((nodePtr=FindFreeNode(hashPtr,key, keyLen)) == (Hash_Node *)NULL) {
	    fprintf(stderr, "CreateHashNode: Couldn't find free node\n");
	    fprintf("die %s\n", 0);
	    exit(-1);
	}
    }

    while (hashPtr->stringUsed+keyLen > hashPtr->stringSize) {
	GrowStringTab(hashPtr);
    }
/*
    fprintf(stderr, "Create: assigned %d to %s, %x\n",
	    nodePtr-hashPtr->tab, key, datum);
*/
    nodePtr->datum = datum;
    nodePtr->key = hashPtr->stringTab+hashPtr->stringUsed;
    nodePtr->keyLen = keyLen;
    bcopy(key, nodePtr->key, keyLen);
    hashPtr->stringUsed += keyLen;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteHashNode --
 *
 *	Internal deallocation routine
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteHashNode(nodePtr, freeData)
    Hash_Node *nodePtr;       /* node to be removed */
    int freeData;
{

    nodePtr->keyLen = DELETED;
    if (freeData) {
	MEM_FREE("DeleteHashNode", (char *)nodePtr->datum);
    }
    nodePtr->datum = (Hash_ClientData) -1;

}


/*
 *----------------------------------------------------------------------
 *
 * GrowHashTab --
 *
 *	Internal hash table expansion routine
 *
 * Results:
 *	Grown table with data copied over.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GrowHashTab(hashPtr)
    Hash_Handle *hashPtr;        /* hash table handle */
{
    int i;
    int used;
    int oldSize = hashPtr->tabSize;
    Hash_Node *oldTab = hashPtr->tab;
    Hash_Node *oldPtr;
    Hash_Node *newPtr;
/*
    fprintf(stderr, "GrowHashTab\n");
*/
    hashPtr->tabSize = 2*oldSize+1;
    hashPtr->tab = 
	(Hash_Node *)MEM_ALLOC("GrowHashTab",hashPtr->tabSize*sizeof(Hash_Node));

    for (i=0,newPtr=hashPtr->tab; i<hashPtr->tabSize; i++,newPtr++) {
	newPtr->keyLen = AVAIL;
    }

    for (i=0,oldPtr=oldTab,used=0; i<oldSize; i++,oldPtr++) {
	if ((oldPtr->keyLen != AVAIL) && (oldPtr->keyLen != DELETED)) {
	    used++;
	    if ((newPtr=FindFreeNode(hashPtr, oldPtr->key, oldPtr->keyLen))
		== (Hash_Node *)NULL) {
		fprintf(stderr, "Error growing hash table: %s\n", hashPtr->name);
	    } else {
		newPtr->key = oldPtr->key;
		newPtr->keyLen = oldPtr->keyLen;
		newPtr->datum = oldPtr->datum;
	    }
	}
    }

    MEM_FREE("GrowHashTab", (char *)oldTab);
    hashPtr->tabGrowCnt++;
    hashPtr->tabFill += used / (float)oldSize;
}


/*
 *----------------------------------------------------------------------
 *
 * GrowStringTab --
 *
 *	Internal string table expansion routine
 *
 * Results:
 *	Grown table with data copied over.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GrowStringTab(hashPtr)
    Hash_Handle *hashPtr;        /* hash table handle */
{
    int oldSize = hashPtr->stringSize;
    int newSize;
    int i;
    int keyLen;
    int used = 0;
    char *curPtr;
    char *oldTab = hashPtr->stringTab;
    Hash_Node *nodePtr;
/*
    fprintf(stderr, "GrowStringTab\n");
*/
    newSize = 2*oldSize;

    hashPtr->stringTab = curPtr =
	(char *)MEM_ALLOC("GrowStringTab",newSize*sizeof(char));

    for (i=0,nodePtr=hashPtr->tab; i<hashPtr->tabSize; i++, nodePtr++) {
	if ((nodePtr->keyLen != AVAIL) && (nodePtr->keyLen != DELETED)) {
	    keyLen = nodePtr->keyLen;
	    bcopy((char *)nodePtr->key, curPtr, keyLen);
	    nodePtr->key = curPtr;
	    used += keyLen;
	    curPtr += keyLen;
	}
    }

    hashPtr->stringUsed = used;
    hashPtr->stringSize = newSize;
    MEM_FREE("GrowStringTab", oldTab);
    hashPtr->stringGrowCnt++;
    hashPtr->stringFill += used / (float)oldSize;

}
