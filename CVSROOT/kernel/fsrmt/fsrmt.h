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

#include "fsNameOps.h"
#include "proc.h"

#include "fsutil.h"
#include "fscache.h"


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

extern void Fsrmt_IOHandleInit();

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


/*
 * General purpose remote stubs shared by remote files, devices, pipes, etc.
 */
extern	ReturnStatus	Fsrmt_Read();
extern	ReturnStatus	Fsrmt_Write();
extern	ReturnStatus	Fsrmt_Select();
extern	ReturnStatus	Fsrmt_IOControl();
extern	ReturnStatus	Fsrmt_Close();
extern	ReturnStatus	Fsrmt_GetIOAttr();
extern	ReturnStatus	Fsrmt_SetIOAttr();
extern	ReturnStatus	Fsrmt_BlockCopy();
extern	ReturnStatus	Fsrmt_DomainInfo();
extern  ReturnStatus    Fsrmt_IOMigClose();
extern  ReturnStatus    Fsrmt_IOMigOpen();
extern  ReturnStatus    Fsrmt_IOClose();


extern ReturnStatus	Fsrmt_DeviceOpen();
extern ReturnStatus	Fsrmt_DeviceReopen();

extern ReturnStatus	Fsrmt_NotifyOfMigration();

extern void	Fsrmt_InitializeOps();
extern void	Fsrmt_Bin();

#endif _FSRMT
