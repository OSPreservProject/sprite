/*
 * lfsInt.h --
 *
 *	Type and data uses internally to the LFS module.
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

#ifndef _LFSINT
#define _LFSINT

#include <sprite.h>
#include <fs.h>
#include <fsconsist.h>
#include <user/fs.h>
#include <lfs.h>
#include <lfsDesc.h>
#include <lfsDescMapInt.h>
#include <lfsDescInt.h>
#include <lfsSuperBlock.h>
#include <lfsSegUsageInt.h>
#include <lfsFileLayoutInt.h>
#include <lfsDirLogInt.h>
#include <lfsMemInt.h>
#include <lfsStats.h>
#include <lfsTypes.h>

#include <fsdm.h>

/*
 * So we can use printf and bzero, bcopy in the lfs module.
 */
#include <stdio.h> 
#include <bstring.h>

/* constants */
/*
 * Flags for checkpoint callback.
 * LFS_CHECKPOINT_DETACH - This checkpoint is part of a file system detach.
 *	                   Any data structures malloc'ed for this file
 *		           system during attach should be freed.
 * LFS_CHECKPOINT_NOSEG_WAIT - This checkpoint shouldn't wait for clean
 *				segments because it is a checkpoint after
 *				a cleaning.
 * LFS_CHECKPOINT_WRITEBACK - This checkpoint is being done for a domain
 *			      writeback operation.
 * LFS_CHECKPOINT_TIMER - This checkpoint is part of the regular callback.
 * LFS_CHECKPOINT_CLEANER - This checkpoint is part of the cleaner.
 * 
 */
#define	LFS_CHECKPOINT_DETACH		 0x1
#define	LFS_CHECKPOINT_NOSEG_WAIT	 0x2
#define	LFS_CHECKPOINT_WRITEBACK	 0x4
#define	LFS_CHECKPOINT_TIMER		 0x8
#define	LFS_CHECKPOINT_CLEANER		 0x10

/*
 * Possible values for activeFlags:
 * LFS_CLEANER_ACTIVE	  - A segment cleaner process is active on this file 
 *			    system.
 * LFS_WRITE_ACTIVE	  - Someone is actively writing to the log.
 * LFS_CHECKPOINT_ACTIVE  - A checkpoint is active on this file system.
 * LFS_SHUTDOWN_ACTIVE    - The file system is about to be shutdown.
 * LFS_CHECKPOINTWAIT_ACTIVE - Someone is waiting for a checkpoint to be
 *			       performed.
 * LFS_CLEANER_CHECKPOINT_ACTIVE - A segment cleaner is doing a checkpoint.
 * LFS_SYNC_CHECKPOINT_ACTIVE - A segment cleaner is doing a checkpoint.
 * LFS_CLEANSEGWAIT_ACTIVE - Someone is waiting for clean segments to be
 *			     generated.
 */

#define	LFS_WRITE_ACTIVE	  0x1
#define	LFS_CLEANER_ACTIVE	 0x10
#define	LFS_SHUTDOWN_ACTIVE	 0x40
#define	LFS_CHECKPOINTWAIT_ACTIVE 0x80
#define	LFS_SYNC_CHECKPOINT_ACTIVE 0x100
#define	LFS_CLEANER_CHECKPOINT_ACTIVE 0x200
#define	LFS_CHECKPOINT_ACTIVE 0x300
#define	LFS_CLEANSEGWAIT_ACTIVE	0x400

extern int lfsMinNumberToClean;

/*
 * This is for ASPLOS stats only.  Remove when that's done.  -Mary 2/16/92.
 */
extern Boolean Lfs_DoASPLOSStats;

/* Useful macros for LFS.
 *
 * LfsFromDomainPtr(domainPtr) - Return the Lfs data stucture for a Fsdm_domain.
 *
 * LfsSegSize(lfsPtr)	- Return the segment size in bytes.
 * LfsSegSizeInBlocks(lfsPtr) - Return the segment size in blocks.
 * LfsBlockSize(lfsPtr)       - Return the block size.
 * LfsBytesToBlocks(lfsPtr, bytes) - Convert bytes into the number of blocks
 *				     it would take to contain the bytes.
 * LfsBlocksToBytes(lfsPtr, blocks) - Convert from blocks into bytes.
 * LfsSegNumToDiskAddress(lfsPtr, segNum) - Convert a segment number into
 *					    a disk address.
 * LfsBlockToSegmentNum(lfsPtr, diskAdress)  - Compute the segment number 
 *					 of a disk  address .
 * LfsIsCleanerProcess(lfsPtr) - Return TRUE if current process is a cleaner.
 *
 * LfsGetCurrentTimestamp(lfsPtr) - Return the current file system timestamp
 */

#define	LfsFromDomainPtr(domainPtr) ((Lfs *) ((domainPtr)->clientData))

#define	LfsSegSize(lfsPtr) ((lfsPtr)->usageArray.params.segmentSize)

#define	LfsSegSizeInBlocks(lfsPtr) \
			(LfsSegSize(lfsPtr)>>(lfsPtr)->blockSizeShift)

#define	LfsBlockSize(lfsPtr) ((lfsPtr)->superBlock.hdr.blockSize)

#define	LfsBytesToBlocks(lfsPtr, bytes)	\
	 (((bytes) + (LfsBlockSize(lfsPtr)-1))>>(lfsPtr)->blockSizeShift)

#define	LfsBlocksToBytes(lfsPtr, blocks) ((blocks)<<(lfsPtr)->blockSizeShift)


#define LfsValidSegmentNum(lfsPtr, segNum) (((segNum) >= 0) && \
		((segNum) < (lfsPtr)->usageArray.params.numberSegments))


#define LfsSegNumToDiskAddress(lfsPtr, segNum, diskAddrPtr) \
		LfsOffsetToDiskAddr(  \
		     ((lfsPtr)->superBlock.hdr.logStartOffset + \
		(LfsSegSizeInBlocks((lfsPtr)) * (segNum))), diskAddrPtr)

#define LfsDiskAddrToSegmentNum(lfsPtr, diskAddress) \
		((LfsDiskAddrToOffset(diskAddress) - \
				(lfsPtr)->superBlock.hdr.logStartOffset) / \
					 LfsSegSizeInBlocks((lfsPtr)))

#define	LfsGetCurrentTimestamp(lfsPtr)	(++((lfsPtr)->checkPoint.timestamp))

#define	LfsIsCleanerProcess(lfsPtr) \
		(Proc_GetCurrentProc() == (lfsPtr)->cleanerProcPtr)

/*
 * Attach detach routines. 
 */
extern ReturnStatus LfsLoadFileSystem _ARGS_((Lfs *lfsPtr, int flags));
extern ReturnStatus LfsDetachFileSystem _ARGS_((Lfs *lfsPtr));
extern ReturnStatus LfsCheckPointFileSystem _ARGS_((Lfs *lfsPtr, int flags));



/*
 * Utility  routines.
 */
extern int LfsLogBase2 _ARGS_((unsigned int val));
extern void LfsError _ARGS_((Lfs *lfsPtr, ReturnStatus status, char *message));
extern void LfsSegCleanStart _ARGS_((Lfs *lfsPtr));
extern void LfsWaitForCheckPoint _ARGS_((Lfs *lfsPtr));
extern void LfsSegmentWriteProc _ARGS_((ClientData clientData,
				Proc_CallInfo *callInfoPtr));
extern void LfsWaitForCleanSegments _ARGS_((Lfs *lfsPtr));

extern void Lfs_ReallocBlock _ARGS_((ClientData data, 
				Proc_CallInfo *callInfoPtr));
/*
 * Second parameter below is for ASPLOS measurements and can be removed
 * after that's all over.  Mary 2/14/92.
 */
extern Boolean Lfs_StartWriteBack _ARGS_((Fscache_Backend *backendPtr, Boolean fileFsynced));
extern void LfsStopWriteBack _ARGS_((Lfs *lfsPtr));
extern Boolean LfsMoreToWriteBack _ARGS_((Lfs *lfsPtr));
extern Fscache_Backend *LfsCacheBackendInit _ARGS_((Lfs *lfsPtr));
/*
 * I/o routines. 
 */
extern ReturnStatus LfsReadBytes _ARGS_((Lfs *lfsPtr, LfsDiskAddr diskAddress, 
			int numBytes, char *bufferPtr));
extern ReturnStatus LfsWriteBytes _ARGS_((Lfs *lfsPtr, LfsDiskAddr diskAddress, 
			int numBytes, char *bufferPtr));
extern void LfsCheckRead _ARGS_((Lfs *lfsPtr, LfsDiskAddr diskAddress, 
				int numBytes));

/*
 * File index routines. 
 */
extern ReturnStatus LfsFile_GetIndex _ARGS_((Fsio_FileIOHandle *handlePtr,
			int blockNum, int cacheFlags, 
			LfsDiskAddr *diskAddressPtr));
extern ReturnStatus LfsFile_SetIndex _ARGS_((Fsio_FileIOHandle *handlePtr, 
			int blockNum, int blockSize, int cacheFlags, 
			LfsDiskAddr diskAddress));

extern ReturnStatus LfsFile_TruncIndex _ARGS_((struct Lfs *lfsPtr, 
			Fsio_FileIOHandle *handlePtr, 
			int length));

extern ReturnStatus LfsFile_GrowBlock _ARGS_((Lfs *lfsPtr, 
			Fsio_FileIOHandle *handlePtr,
			int offset, int numBytes));

#endif /* _LFSINT */

