/*
 * fsrmt.h --
 *
 *	Definitions of the parameters required for Sprite Domain operations
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSRMT
#define _FSRMT

#include <fsNameOps.h>
#include <proc.h>

#include <fsutil.h>
#include <fscache.h>


/* 
 * The I/O descriptor for remote streams.  This is all that is needed for
 *	remote devices, remote pipes, and named pipes that are not cached
 *	on the local machine.  This structure is also embedded into the
 *	I/O descriptor for remote files.  These stream types share some
 *	common remote procedure stubs, and this structure provides
 *	a common interface.
 *	FSIO_RMT_DEVICE_STREAM, FSIO_RMT_PIPE_STREAM, FS_RMT_NAMED_PIPE_STREAM,
 *	FSIO_RMT_PSEUDO_STREAM, FSIO_RMT_PFS_STREAM
 */

typedef struct Fsrmt_IOHandle {
    Fs_HandleHeader	hdr;		/* Standard handle header.  The server
					 * ID field in the hdr is used to
					 * forward the I/O operation. */
    Fsutil_RecoveryInfo	recovery;	/* For I/O server recovery */
} Fsrmt_IOHandle;


/*
 * The I/O descriptor for a remote file.  Used with FSIO_RMT_FILE_STREAM.
 */

typedef struct Fsrmt_FileIOHandle {
    Fsrmt_IOHandle	rmt;		/* Remote I/O handle used for RPCs. */
    Fscache_FileInfo	cacheInfo;	/* Used to access block cache. */
    Fscache_ReadAheadInfo	readAhead;	/* Read ahead info used to synchronize
					 * with other I/O and closes/deletes. */
    int			openTimeStamp;	/* Returned on open from the server
					 * and used to catch races with cache
					 * consistency msgs due to other opens*/
    int			flags;		/* FS_SWAP */
    struct Vm_Segment	*segPtr;	/* Reference to code segment needed
					 * to flush VM cache. */
} Fsrmt_FileIOHandle;			/* 216 BYTES  (264 with traced locks)*/

/*
 * RPC debugging.
 */
extern	Boolean	fsrmt_RpcDebug;


extern void Fsrmt_IOHandleInit _ARGS_((Fs_FileID *ioFileIDPtr, int useFlags,
		char *name, Fs_HandleHeader **newHandlePtrPtr));

/*
 * General purpose remote stubs shared by remote files, devices, pipes, etc.
 */
extern ReturnStatus Fsrmt_Read _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *readPtr, Sync_RemoteWaiter *waitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus Fsrmt_Write _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *writePtr, Sync_RemoteWaiter *waitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus Fsrmt_Select _ARGS_((Fs_HandleHeader *hdrPtr, 
		Sync_RemoteWaiter *waitPtr, int *readPtr, int *writePtr, 
		int *exceptPtr));
extern ReturnStatus Fsrmt_IOControl _ARGS_((Fs_Stream *streamPtr,
		Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Fsrmt_Close _ARGS_((Fs_Stream *streamPtr, int clientID, 
		Proc_PID procID, int flags, int dataSize, 
		ClientData closeData));
extern ReturnStatus Fsrmt_GetIOAttr _ARGS_((Fs_FileID *fileIDPtr, int clientID,
		Fs_Attributes *attrPtr));
extern ReturnStatus Fsrmt_SetIOAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, int flags));
extern ReturnStatus Fsrmt_BlockCopy _ARGS_((Fs_HandleHeader *srcHdrPtr, 
		Fs_HandleHeader *dstHdrPtr, int blockNum));
extern ReturnStatus Fsrmt_DomainInfo _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_DomainInfo *domainInfoPtr));
extern ReturnStatus Fsrmt_IOMigClose _ARGS_((Fs_HandleHeader *hdrPtr, 
		int flags));
extern ReturnStatus Fsrmt_IOMigOpen _ARGS_((Fsio_MigInfo *migInfoPtr, int size,
		ClientData data, Fs_HandleHeader **hdrPtrPtr));
extern ReturnStatus Fsrmt_IOClose _ARGS_((Fs_Stream *streamPtr, int clientID,
		Proc_PID procID, int flags, int dataSize, 
		ClientData closeData));
extern ReturnStatus Fsrmt_DeviceOpen _ARGS_((Fs_FileID *ioFileIDPtr, 
		int useFlags, int inSize, ClientData inBuffer));
extern ReturnStatus Fsrmt_DeviceReopen _ARGS_((Fs_HandleHeader *hdrPtr,
		int clientID, ClientData inData, int *outSizePtr, 
		ClientData *outDataPtr));
extern ReturnStatus FsrmtDeviceMigrate _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int dstClientID, int *flagsPtr, int *offsetPtr, int *sizePtr, 
		Address *dataPtr));

extern ReturnStatus Fsrmt_NotifyOfMigration _ARGS_((Fsio_MigInfo *migInfoPtr,
		int *flagsPtr, int *offsetPtr, int outSize, Address outData));

extern ReturnStatus FsrmtReopen _ARGS_((Fs_HandleHeader *hdrPtr, int inSize,
		Address inData, int *outSizePtr, Address outData));
extern ReturnStatus FsrmtFileMigrate _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int dstClientID, int *flagsPtr, int *offsetPtr, int *sizePtr, 
		Address *dataPtr));
extern ReturnStatus FsrmtPipeMigrate _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int dstClientID, int *flagsPtr, int *offsetPtr, int *sizePtr, 
		Address *dataPtr));

extern void Fsrmt_InitializeOps _ARGS_((void));
extern void Fsrmt_Bin _ARGS_((void));

/*
 * Recovery testing operations.
 */
extern	int	Fsrmt_FileRecovTestNumCacheBlocks();
extern	int	Fsrmt_FileRecovTestNumDirtyCacheBlocks();

#endif _FSRMT
