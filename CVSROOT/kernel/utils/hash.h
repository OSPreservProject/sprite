/* hash.h --
 *
 * 	This file contains definitions used by the hash module,
 * 	which maintains hash tables.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 */


#ifndef	_HASH
#define	_HASH

#include "list.h"

/*
 * Each hash bucket consists of a version number for the hash chain and
 * a hash chain list header.
 */

typedef struct Hash_Bucket {
    int		version;
    List_Links	list;
} Hash_Bucket;

/* 
 * The following defines one entry in the hash table. 
 */

typedef struct Hash_Entry {
    List_Links	links;
    Address	value;		/* Pointer to anything. */
    Hash_Bucket	*bucketPtr;	/* Pointer to hash bucket that this entry is
				 * in. */
    union {
	Address	 ptr;		/* One-word key value to identify entry. */
	unsigned words[1];	/* N-word key value.  Note: the actual
				 * size may be longer if necessary to hold
				 * the entire key.
				 */
	char 	 name[4];	/* Text name of this entry.  Note: the
				 * actual size may be longer if necessary
				 * to hold the whole string. This MUST be
				 * the last entry in the structure!!!
				 */
    } key;
} Hash_Entry;

/* 
 * A hash table consists of an array of pointers to hash
 * lists.  Tables can be organized in either of three ways, depending
 * on the type of comparison keys:
 *
 *	Strings:	  these are NULL-terminated; their address
 *			  is passed to HashFind as a (char *).
 *	Single-word keys: these may be anything, but must be passed
 *			  to Hash_Find as an Address.
 *	Multi-word keys:  these may also be anything; their address
 *			  is passed to HashFind as an Address.
 *
 *	Single-word keys are fastest, but most restrictive.
 */
 
#define HASH_STRING_KEYS	0
#define HASH_ONE_WORD_KEYS	1

typedef struct Hash_Table {
    Hash_Bucket 	*table;		/* Pointer to array of List_Links. */
    int 		size;		/* Actual size of array. */
    int			version;	/* Version of the hash table.  Goes up
					 * every time hash table grows. */
    int 		numEntries;	/* Number of entries in the table. */
    int 		downShift;	/* Shift count, used in hashing 
					 * function. */
    int 		mask;		/* Used to select bits for hashing. */
    int 		ptrKeys;	/* 1 means that keys (h_names) are 
					 * 1-word values (char *'s).  0 means 
					 * keys are strings.  >1 means keys 
					 * are ht_ptrKeys-word values.  */
} Hash_Table;

/* 
 * The following structure is used by the searching routines
 * to record where we are in the search.
 */

typedef struct Hash_Search {
    int 	nextIndex;	/* Next bucket to check (after current). */
    int		bucketVersion;	/* Version of the bucket currently being 
				 * searched.*/
    int		tableVersion;	/* The version of the table being searched. */
    Hash_Entry 	*hashEntryPtr;	/* Next entry to check in current bucket. */
    List_Links	*hashList;	/* Hash chain that are currently checking. */
} Hash_Search;

/*
 * Macros.
 */

/*
 * char * Hash_GetValue(h) 
 *     HashEntry *h; 
 */

#define Hash_GetValue(h) ((h)->value)

/* 
 * Hash_SetValue(h, val); 
 *     HashEntry *h; 
 *     char *val; 
 */

#define Hash_SetValue(h, val) ((h)->value = (Address) (val))

/* 
 * Hash_Size(n) returns the number of words in an object of n bytes 
 */

#define	Hash_Size(n)	(((n) + sizeof (unsigned) - 1) / sizeof (unsigned))

/*
 * The following procedure declarations and macros
 * are the only things that should be needed outside
 * the implementation code.
 */

extern void 		Hash_Init();
extern void		Hash_Stats();
extern void		Hash_Kill();
extern Hash_Entry 	*Hash_Find();
extern Hash_Entry 	*Hash_LookOnly();
extern void 		Hash_Delete();
extern void 		Hash_StartSearch();
extern Hash_Entry	*Hash_Next();

#endif _HASH
