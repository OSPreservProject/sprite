/*
 * fsSpriteDomain.h --
 *
 *	Definitions of the parameters required for Sprite Domain operations
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSSPRITEDOMAIN
#define _FSSPRITEDOMAIN

#include "fsNameOps.h"
#include "proc.h"

/*
 * Parameters for the read RPC.
 */

typedef struct FsRemoteReadParams {
    Fs_FileID	fileID;			/* Identifies file to read from */
    Fs_FileID	streamID;		/* Identifies stream (for offset) */
    int		offset;			/* Byte offset at which to read */
    int		length;			/* Byte amount to read */
    int		flags;			/* FS_CLIENT_CACHE_READ if file is
					 * being cached. FS_RMT_SHARED if
					 * the server's offset is to be used */
    Sync_RemoteWaiter waiter;		/* Process info for remote waiting */
    Proc_PID	pid, familyID;		/* Process ID and Family ID of this
					 * process used to read from
					 * a remote device */
} FsRemoteReadParams;

/*
 * Parameters for the write RPC.
 */

typedef struct FsRemoteWriteParams {
    Fs_FileID	fileID;			/* File to write to */
    Fs_FileID	streamID;		/* Stream to write to (for offset) */
    int		offset;			/* Byte offset at which to write */
    int		length;			/* Byte amout to write */
    int		flags;			/* FS_APPEND | FS_CLIENT_CACHE_WRITE
					 * FS_LAST_DIRTY_BLOCK | FS_RMT_SHARED*/
    Sync_RemoteWaiter waiter;		/* Process info for remote waiting */
} FsRemoteWriteParams;

/*
 * Parameters for the Device Open RPC.
 */

typedef struct FsSpriteDevOpenParams {
    Fs_FileID	fileID;		/* File ID from the name server used by the
				 * I/O server to construct its own file ID */
    Fs_Device	device;		/* Specifies device server, type, unit */
    int		flags;		/* FS_MIGRATING_HANDLE. */
    int		streamType;	/* Type of stream being opened, either a
				 * domain stream, a cacheable stream for
				 * named pipes, or a pdev stream to
				 * open up the master's req/res pipes */
} FsSpriteDevOpenParams;

typedef struct FsSpriteDevOpenResults {
    Fs_FileID	fileID;		/* File ID that identifies the handle on the
				 * I/O server. */
} FsSpriteDevOpenResults;

/*
 * Parameters for the iocontrol RPC
 */

typedef struct FsSpriteIOCParams {
    Fs_FileID	fileID;		/* File to manipulate. */
    Fs_FileID	streamID;	/* Stream to the file, needed for locking */
    Proc_PID	procID;		/* ID of invoking process */
    Proc_PID	familyID;	/* Family of invoking process */
    int		command;	/* I/O Control to perform. */
    int		inBufSize;	/* Size of input params to ioc. */
    int		outBufSize;	/* Size of results from ioc. */
    int		byteOrder;	/* Defines client's byte ordering */
    int		reserved;	/* Extra */
} FsSpriteIOCParams;

/*
 * Parameters for the block copy RPC.
 */
typedef struct FsRemoteBlockCopyParams {
    Fs_FileID	srcFileID;	/* File to copy from. */
    Fs_FileID	destFileID;	/* File to copy to. */
    int		blockNum;	/* Block to copy to. */
} FsRemoteBlockCopyParams;

/*
 * Sprite Domain functions called via FsLookupOperation.
 * These are called with a pathname.
 */
extern	ReturnStatus	FsSpriteImport();
extern	ReturnStatus	FsSpriteOpen();
extern	ReturnStatus	FsSpriteReopen();
extern	ReturnStatus	FsSpriteDevOpen();
extern	ReturnStatus	FsSpriteDevClose();
extern	ReturnStatus	FsRemoteGetAttrPath();
extern	ReturnStatus	FsRemoteSetAttrPath();
extern	ReturnStatus	FsSpriteMakeDevice();
extern	ReturnStatus	FsSpriteMakeDir();
extern	ReturnStatus	FsSpriteRemove();
extern	ReturnStatus	FsSpriteRemoveDir();
extern	ReturnStatus	FsSpriteRename();
extern	ReturnStatus	FsSpriteHardLink();

/*
 * Sprite Domain functions called via the fsAttrOpsTable switch.
 * These are called with a fileID.
 */
extern	ReturnStatus	FsRemoteGetAttr();
extern	ReturnStatus	FsRemoteSetAttr();

/*
 * General purpose remote stubs shared by remote files, devices, pipes, etc.
 */
extern	ReturnStatus	FsRemoteRead();
extern	ReturnStatus	FsRemoteWrite();
extern	ReturnStatus	FsRemoteSelect();
extern	ReturnStatus	FsRemoteIOControl();
extern	ReturnStatus	FsRemoteClose();
extern	ReturnStatus	FsRemoteGetIOAttr();
extern	ReturnStatus	FsRemoteSetIOAttr();
extern	ReturnStatus	FsRemoteBlockCopy();
extern	ReturnStatus	FsRemoteDomainInfo();

#endif _FSSPRITEDOMAIN
