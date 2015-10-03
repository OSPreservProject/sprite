/*
 * lfsDescMap.h --
 *
 *	Declarations defining the disk resident format of the LFS 
 *	descriptor map. The main purpose of the descriptor map is 
 *	to provide a fast lookup of a file descriptor address given
 *	the file descriptor number.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _LFSDESCMAP
#define _LFSDESCMAP

#ifdef KERNEL
#include <lfsStableMem.h>
#else
#include <kernel/lfsStableMem.h>
#endif

/*
 * The descriptor map layout on disk is described by the following 
 * super block resident structure. 
 * It must be LFS_DESC_MAP_PARAM_SIZE (currently 64 bytes) in size. 
 */
#define	LFS_DESC_MAP_PARAM_SIZE	64

typedef struct LfsDescMapParams {
    unsigned int version;  	/* Version number describing the format of
				 * this structure and the descriptor map. */
    int 	maxDesc;	/* The maximum size in descriptor map in 
				 * descriptor. */
    char     padding[LFS_DESC_MAP_PARAM_SIZE - sizeof(LfsStableMemParams)-8];	
				/* Enought padding to make this structure 32
				 * bytes. */
    LfsStableMemParams	 stableMem; /* Index parameters. */

} LfsDescMapParams;

#define LFS_DESC_MAP_VERSION 1

/*
 * Following this structure describes the disk format of LFS descriptor map
 * checkpoint.
 * The total size
 * of the checkpoint should be 
 * sizeof(LfsDescMapCheckPointHdr) + 
 * 		Sizeof(LfsStableMemCheckPoint)
 * The maximum size should be:
 *
 */

#define	LFS_DESC_MAP_MIN_BLOCKS	1

typedef struct LfsDescMapCheckPoint {
    int numAllocDesc;	/* The number of allocated descriptors at this 
			 * checkpoint.  */
} LfsDescMapCheckPoint;

/*
 * For each allocate file number in a LFS, the descriptor map keeps an 
 * entry of type LfsDescMapEntry. LfsDescMapEntry are packed into blocks
 * with 
 */
typedef struct LfsDescMapEntry {
    LfsDiskAddr  blockAddress;	    /* The disk block address of the most
				     * current version of the descriptor. */
    unsigned short truncVersion;    /* A version number increamented each 
				     * time a descriptor is truncated to 
				     * length zero.  See the cleaning code
				     * for its use. */
    unsigned short  flags;  	    /* See flags definition below. */
    int  accessTime;      	    /* The access time of the file as 
				     * return by the stat() system call. */
} LfsDescMapEntry;

/*
 * The following definitions define the uses of the descriptor flags field of
 * the LfsDescMapEntry structure.
 *	
 * LFS_DESC_MAP_ALLOCED	 	- The descriptor map entry has been alloced
 *				  by the file system.
 * LFS_DESC_MAP_UNLINKED	- The file represented by this descMap entry
 *				  has no references to it from directories 
 *				  but has not been freed. This can happen when
 *				  a file is unlinked while open.  
 */

#define	LFS_DESC_MAP_ALLOCED	0x0001
#define	LFS_DESC_MAP_UNLINKED	0x0002


#endif /* _LFSDESCMAP */
