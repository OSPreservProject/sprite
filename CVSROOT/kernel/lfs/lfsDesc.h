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
#define _LFSDESC
#ifdef KERNEL
#include <fsdm.h>
#else
#include <kernel/fsdm.h>
#endif

/*
 * Format of disk address.
 */

#ifndef LFS_USE_DISK_ADDR_STRUCT
typedef int LfsDiskAddr;

#define	LfsSetNilDiskAddr(diskAddrPtr) ((*(diskAddrPtr)) = FSDM_NIL_INDEX)
#define	LfsIsNilDiskAddr(diskAddr) ((diskAddr) == FSDM_NIL_INDEX)
#define	LfsIsNullDiskAddr(diskAddr) ((diskAddr) == 0)
#define	LfsSameDiskAddr(diskAddrA, diskAddrB) ((diskAddrA) == (diskAddrB))
#define	LfsDiskAddrToOffset(diskAddr)	(diskAddr)
#define	LfsDiskAddrPlusOffset(diskAddr,offset, newDiskAddrPtr) \
		((*(newDiskAddrPtr)) = (diskAddr) + (offset))
#define	LfsOffsetToDiskAddr(offset, diskAddrPtr) ((*(diskAddrPtr)) = (offset))
#define	LfsDiskAddrInRange(diskAddr, size, startAddr, endAddr) \
	(((diskAddr) >= (startAddr)) && ((diskAddr) + (size) < (endAddr)))
#define LfsDiskAddrOffset(diskAddrA, diskAddrB) ((diskAddrA) - (diskAddrB))
#else /* LFS_USE_DISK_ADDR_STRUCT */
typedef struct LfsDiskAddr {
	int offset;
} LfsDiskAddr;
#define	LfsSetNilDiskAddr(diskAddrPtr) ((diskAddrPtr)->offset = FSDM_NIL_INDEX)
#define	LfsIsNilDiskAddr(diskAddr) ((diskAddr).offset == FSDM_NIL_INDEX)
#define	LfsIsNullDiskAddr(diskAddr) ((diskAddr).offset == 0)
#define	LfsSameDiskAddr(diskAddrA, diskAddrB) \
		((diskAddrA).offset == (diskAddrB).offset)
#define	LfsDiskAddrToOffset(diskAddr)	((diskAddr).offset)
#define	LfsDiskAddrPlusOffset(diskAddr,blockOffset, newDiskAddrPtr) \
		(((newDiskAddrPtr)->offset) = (diskAddr).offset + (blockOffset))
#define	LfsOffsetToDiskAddr(blockOffset, diskAddrPtr) \
		(((diskAddrPtr)->offset) = (blockOffset))
#define	LfsDiskAddrInRange(diskAddr, size, startAddr, endAddr) \
	(((diskAddr).offset >= (startAddr).offset) && \
	    ((diskAddr).offset + (size) < (endAddr).offset))
#define LfsDiskAddrOffset(diskAddrA, diskAddrB) \
	((diskAddrA).offset - (diskAddrB).offset)
#endif /* LFS_USE_DISK_ADDR_STRUCT */
/*
 * The disk resident format an LFS descriptor. 
 */
typedef struct LfsFileDescriptor {
    Fsdm_FileDescriptor	  common;
    int	  fileNumber;	/* File number that is descriptor  belongs too. */
    LfsDiskAddr	  prevDiskAddr; /* The previous disk block address of
			         * this file descriptors. */
} LfsFileDescriptor;

#define	LFS_FILE_DESC_SIZE	128

#endif /* _LFSDESC */

