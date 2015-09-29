/*
 * fsIndex.h --
 *
 *	Definitions to allow moving through indirect blocks.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: fsIndex.h,v 1.3 87/05/08 17:48:40 brent Exp $ SPRITE (Berkeley)
 */

#ifndef _FSINDEX
#define _FSINDEX

/*
 * Index type constants.
 */
#ifndef FS_INDIRECT
#define	FS_INDIRECT	0
#define	FS_DBL_INDIRECT	1
#define	FS_DIRECT	2
#endif

typedef struct BlockIndexInfo {
    int	 	indexType;		/* Whether chasing direct, indirect,
					   or doubly indirect blocks. */
    int		 blockNum;		/* Block that is being read, written,
					   or allocated. */
    int		 *blockAddrPtr;		/* Pointer to block pointer. */
    int		 firstIndex;		/* An index into either the direct
					   block pointers or into an 
					   indirect block. */
    int		 secondIndex;		/* An index into a doubly indirect
					   block. */
    Boolean 	 firstBlockNil;		/* The first block is empty. */
} BlockIndexInfo;

#endif _FSINDEX
