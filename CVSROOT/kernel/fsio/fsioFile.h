/*
 * fsFile.h --
 *
 *	Declarations for regular file access, local and remote.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSFILE
#define _FSFILE

#include "fsRecovery.h"
#include "fsReadAhead.h"

/*
 * When a regular file is opened state is packaged up on the server
 * and used on the client to set up the I/O handle for the file.
 * This is the 'streamData' generated by FsFileSrvOpen and passed to
 * FsFileCltOpen
 */
typedef struct FsFileState {
    Boolean	cacheable;	/* TRUE if the client can cache data blocks */
    int		version;	/* Version number for data block cache */
    int		openTimeStamp;	/* Time stamp used to catch races between
				 * open replies and cache consistency msgs */
    FsCachedAttributes attr;	/* A copy of some file attributes */
    int		newUseFlags;	/* The server may modify the stream use flags.
				 * In particular, the execute bit is stripped
				 * off when directories are opened. */
} FsFileState;

/*
 * When a client re-opens a file it sends the following state to the server.
 */
typedef struct FsFileReopenParams {
    Fs_FileID	fileID;		/* File ID of file to reopen. MUST BE FIRST */
    Fs_FileID	prefixFileID;	/* File ID for the prefix of this file. */
    FsUseCounts	use;		/* Reference counts */
    Boolean	flags;		/* FS_HAVE_BLOCKS | FS_SWAP */
    int		version;	/* Expected version number for the file. */
} FsFileReopenParams;

/*
 * File reopen flags
 *	FS_HAVE_BLOCKS	Set when the client has dirty blocks in its cache.
 *		This implies that it ought to be able to continue caching.
 *		A race exists in that another client could open for writing
 *		first, and thus invalidate the first client's data, or another
 *		client could open for reading and possibly see stale data.
 *	FS_SWAP	This stream flag is passed along so the server doesn't
 *		erroneously grant cacheability to swap files.
 *
 */
#define FS_HAVE_BLOCKS		0x1
/*resrv FS_SWAP			0x4000 */

/*
 * The I/O descriptor for a local file.  Used with FS_LCL_FILE_STREAM.
 */

typedef struct FsLocalFileIOHandle {
    FsHandleHeader	hdr;		/* Standard handle header.  The
					 * 'major' field of the fileID
					 * is the domain number.  The
					 * 'minor' field is the file num. */
    FsUseCounts		use;		/* Open, writer, and exec counts.
					 * Used for consistency checks. This
					 * is a summary of all uses of a file */
    int			flags;		/* FS_FILE_DELETED */
    struct FsFileDescriptor *descPtr;	/* Reference to disk info, this
					 * has attritutes, plus disk map. */
    FsCacheFileInfo	cacheInfo;	/* Used to access block cache. */
    FsConsistInfo	consist;	/* Client use info needed to enforce
					 * network cache consistency */
    FsLockState		lock;		/* User level locking state. */
    FsReadAheadInfo	readAhead;	/* Read ahead info used to synchronize
					 * with other I/O and closes/deletes. */
    struct Vm_Segment	*segPtr;	/* Reference to code segment needed
					 * to flush VM cache. */
} FsLocalFileIOHandle;			/* 268 BYTES (316 with traced locks) */

/*
 * Flags for local I/O handles.
 *	FS_FILE_DELETED		Set when all names of a file have been
 *				removed.  This marks the handle for removal.
 */
#define FS_FILE_DELETED		0x1

/*
 * The I/O descriptor for a remote file.  Used with FS_RMT_FILE_STREAM.
 */

typedef struct FsRmtFileIOHandle {
    FsRemoteIOHandle	rmt;		/* Remote I/O handle used for RPCs. */
    FsCacheFileInfo	cacheInfo;	/* Used to access block cache. */
    FsReadAheadInfo	readAhead;	/* Read ahead info used to synchronize
					 * with other I/O and closes/deletes. */
    int			openTimeStamp;	/* Returned on open from the server
					 * and used to catch races with cache
					 * consistency msgs due to other opens*/
    int			flags;		/* FS_SWAP */
    struct Vm_Segment	*segPtr;	/* Reference to code segment needed
					 * to flush VM cache. */
} FsRmtFileIOHandle;			/* 216 BYTES  (264 with traced locks)*/


/*
 * Open operations.
 */
extern ReturnStatus	FsFileSrvOpen();

/*
 * Stream operations.
 */
extern ReturnStatus	FsFileCltOpen();
extern ReturnStatus	FsFileRead();
extern ReturnStatus	FsFileWrite();
extern ReturnStatus	FsFileIOControl();
extern ReturnStatus	FsFileSelect();
extern ReturnStatus	FsFileRelease();
extern ReturnStatus	FsFileMigEnd();
extern ReturnStatus	FsFileMigrate();
extern ReturnStatus	FsFileReopen();
extern ReturnStatus	FsFileBlockAllocate();
extern ReturnStatus	FsFileBlockRead();
extern ReturnStatus	FsFileBlockWrite();
extern ReturnStatus	FsFileBlockCopy();
extern Boolean		FsFileScavenge();
extern void		FsFileClientKill();
extern ReturnStatus	FsFileClose();
extern ReturnStatus	FsFileCloseInt();

extern ReturnStatus	FsRmtFileCltOpen();
extern FsHandleHeader	*FsRmtFileVerify();
extern ReturnStatus	FsRmtFileRead();
extern ReturnStatus	FsRmtFileWrite();
extern ReturnStatus	FsRmtFileIOControl();
extern ReturnStatus	FsRmtFileSelect();
extern ReturnStatus	FsRmtFileGetIOAttr();
extern ReturnStatus	FsRmtFileSetIOAttr();
extern ReturnStatus	FsRmtFileRelease();
extern ReturnStatus	FsRmtFileMigEnd();
extern ReturnStatus	FsRmtFileMigrate();
extern ReturnStatus	FsRmtFileReopen();
extern ReturnStatus	FsRmtFileAllocate();
extern ReturnStatus	FsRmtFileBlockRead();
extern ReturnStatus	FsRmtFileBlockWrite();
extern ReturnStatus	FsRmtFileBlockCopy();
extern Boolean		FsRmtFileScavenge();
extern ReturnStatus	FsRmtFileClose();

extern void		FsFileSyncLockCleanup();
#endif /* _FSFILE */
