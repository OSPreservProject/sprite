/*
 * lfs.h --
 *
 *	Declarations of data structures and routines of the LFS file system
 *	exported to the rest of the Sprite operating system. 
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

#ifndef _LFS
#define _LFS

#include "sprite.h"

/* constants */

/* data structures */

/* Descriptor management routines. */

extern ReturnStatus Lfs_GetNewFileNumber();
extern ReturnStatus Lfs_FreeFileNumber();
extern Boolean Lfs_FileDescStore();
extern ReturnStatus Lfs_FileDescFetch();
extern ReturnStatus Lfs_FileDescTrunc();
/*
 * Startup and shutdown routines. 
 */
extern ReturnStatus Lfs_AttachDisk();
extern ReturnStatus Lfs_DetachDisk();
/*
 * File I/O and allocate routines. 
 */
extern ReturnStatus Lfs_FileBlockRead();
extern ReturnStatus Lfs_FileBlockWrite();
extern ReturnStatus Lfs_FileBlockAllocate();



#endif /* _LFS */

