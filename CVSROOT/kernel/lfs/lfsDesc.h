/*
 * lfsDesc.h --
 *
 *	Declarations of disk resident format of the LFS file descriptors.
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

#ifndef _LFSDESC

#ifdef KERNEL
#include "fsdm.h"
#else
#include "kernel/fsdm.h"
#endif
/*
 * The disk resident format an LFS descriptor. 
 */
typedef struct LfsFileDescriptor {
    Fsdm_FileDescriptor	  common;
    int	  fileNumber;	/* File number that is descriptor  belongs too. */
    int	  prevBlockLoc; /* The previous disk block address of
			 * this file descriptors. */
} LfsFileDescriptor;

#define	LFS_FILE_DESC_SIZE	128

#endif /* _LFSDESC */

