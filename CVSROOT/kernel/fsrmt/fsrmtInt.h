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

typedef struct FsSpriteReadParams {
    FsFileID	fileID;			/* Identifies file to read from */
    FsFileID	streamID;		/* Identifies stream (for offset) */
    int		offset;			/* Byte offset at which to read */
    int		length;			/* Byte amount to read */
    int		flags;			/* FS_CLIENT_CACHE_READ if file is
					 * being cached. FS_RMT_SHARED if
					 * the server's offset is to be used */
    Sync_RemoteWaiter waiter;		/* Process info for remote waiting */
    Proc_PID	pid, familyID;		/* Process ID and Family ID of this
					 * process used to read from
					 * a remote device */
} FsSpriteReadParams;

/*
 * Parameters for the write RPC.
 */

typedef struct FsSpriteWriteParams {
    FsFileID	fileID;			/* File to write to */
    FsFileID	streamID;		/* Stream to write to (for offset) */
    int		offset;			/* Byte offset at which to write */
    int		length;			/* Byte amout to write */
    int		flags;			/* FS_APPEND | FS_CLIENT_CACHE_WRITE
					 * FS_LAST_DIRTY_BLOCK | FS_RMT_SHARED*/
    Sync_RemoteWaiter waiter;		/* Process info for remote waiting */
} FsSpriteWriteParams;

/*
 * Parameters for the Device Open RPC.
 */

typedef struct FsSpriteDevOpenParams {
    FsFileID	fileID;		/* File ID from the name server used by the
				 * I/O server to construct its own file ID */
    Fs_Device	device;		/* Specifies device server, type, unit */
    int		flags;		/* FS_MIGRATING_HANDLE. */
    int		streamType;	/* Type of stream being opened, either a
				 * domain stream, a cacheable stream for
				 * named pipes, or a proc-file stream to
				 * open up the master's req/res pipes */
} FsSpriteDevOpenParams;

typedef struct FsSpriteDevOpenResults {
    FsFileID	fileID;		/* File ID that identifies the handle on the
				 * I/O server. */
} FsSpriteDevOpenResults;
/*
 * Parameters for the file lock RPC.
 */

typedef struct FsSpriteLockParams {
    FsFileID	fileID;		/* File to be re-opened */
    int		operation;	/* Operation argument to Fs_Lock */
    Sync_RemoteWaiter waiter;	/* Process info for remote waiting */
    int		streamType;	/* This is used to generate an Fs_Stream
				 * on the server. */
} FsSpriteLockParams;

/*
 * Parameters for the iocontrol RPC
 */

typedef struct FsSpriteIOCParams {
    FsFileID	fileID;		/* File for iocontrol. */
    int		command;	/* Iocontrol to perform. */
    int		inBufSize;	/* Size of input params to ioc. */
    int		outBufSize;	/* Size of results from ioc. */
} FsSpriteIOCParams;

/*
 * Parameters for the block copy RPC.
 */
typedef struct FsSpriteBlockCopyParams {
    FsFileID	srcFileID;	/* File to copy from. */
    FsFileID	destFileID;	/* File to copy to. */
    int		blockNum;	/* Block to copy to. */
} FsSpriteBlockCopyParams;

/*
 * Parameters for the two path name rpcs (link and rename).
 */

typedef struct FsSprite2PathParams {
    FsLookupArgs	lookupArgs;	/* Includes first prefixID */
    FsFileID		prefixID2;
} FsSprite2PathParams;

typedef struct FsSprite2PathData {
    char		path1[FS_MAX_PATH_NAME_LENGTH];
    char		path2[FS_MAX_PATH_NAME_LENGTH];
} FsSprite2PathData;

typedef struct FsSprite2PathReplyParams {
    int		prefixLength;
    Boolean	name1RedirectP;
} FsSprite2PathReplyParams;

/*
 * Sprite Domain functions called via FsLookupOperation.
 * These are called with a pathname.
 */
extern	ReturnStatus	FsSpritePrefix();
extern	ReturnStatus	FsSpriteOpen();
extern	ReturnStatus	FsSpriteReopen();
extern	ReturnStatus	FsSpriteDevOpen();
extern	ReturnStatus	FsSpriteDevClose();
extern	ReturnStatus	FsSpriteGetAttrPath();
extern	ReturnStatus	FsSpriteSetAttrPath();
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
extern	ReturnStatus	FsSpriteGetAttr();
extern	ReturnStatus	FsSpriteSetAttr();

/*
 * General purpose remote stubs shared by remote files, devices, pipes, etc.
 */
extern	ReturnStatus	FsSpriteRead();
extern	ReturnStatus	FsSpriteWrite();
extern	ReturnStatus	FsSpriteBlockWrite();
extern	ReturnStatus	FsSpriteSelect();
extern	ReturnStatus	FsRemoteIOControl();
extern	ReturnStatus	FsSpriteClose();
extern	ReturnStatus	FsSpriteGetIOAttr();
extern	ReturnStatus	FsSpriteSetIOAttr();
extern	ReturnStatus	FsSpriteBlockCopy();
extern	ReturnStatus	FsSpriteDomainInfo();

#endif _FSSPRITEDOMAIN
