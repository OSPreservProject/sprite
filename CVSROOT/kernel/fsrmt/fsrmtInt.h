/*
 * fsrmtInt.h --
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

#ifndef _FSSPRITEDOMAIN
#define _FSSPRITEDOMAIN

#include "fsNameOps.h"
#include "proc.h"
#include "fsrmt.h"
/*
 * Parameters for the read and write RPCs.
 */

typedef struct FsrmtIOParam {
    Fs_FileID	fileID;			/* Identifies file to read from */
    Fs_FileID	streamID;		/* Identifies stream (for offset) */
    Sync_RemoteWaiter waiter;		/* Process info for remote waiting */
    Fs_IOParam	io;			/* I/O parameter block */
} FsrmtIOParam;

/*
 * Parameters for the iocontrol RPC
 */

typedef struct FsrmtIOCParam {
    Fs_FileID	fileID;		/* File to manipulate. */
    Fs_FileID	streamID;	/* Stream to the file, needed for locking */
    Proc_PID	procID;		/* ID of invoking process */
    Proc_PID	familyID;	/* Family of invoking process */
    int		command;	/* I/O Control to perform. */
    int		inBufSize;	/* Size of input params to ioc. */
    int		outBufSize;	/* Size of results from ioc. */
    Fmt_Format	format;		/* Defines client's byte order/alignment 
				 * format. */
    int		uid;		/* Effective User ID */
} FsrmtIOCParam;

/*
 * Parameters for the I/O Control RPC.  (These aren't used, oops,
 * someday they should be used.)
 */

typedef struct FsrmtIOCParamNew {
    Fs_FileID	fileID;		/* File to manipulate. */
    Fs_FileID	streamID;	/* Stream to the file, needed for locking */
    Fs_IOCParam	ioc;		/* IOControl parameter block */
} FsrmtIOCParamNew;

/*
 * Parameters for the block copy RPC.
 */
typedef struct FsrmtBlockCopyParam {
    Fs_FileID	srcFileID;	/* File to copy from. */
    Fs_FileID	destFileID;	/* File to copy to. */
    int		blockNum;	/* Block to copy to. */
} FsrmtBlockCopyParam;


/*
 * RPC debugging.
 */
#ifndef CLEAN
#define FSRMT_RPC_DEBUG_PRINT(string) \
	if (fsrmt_RpcDebug) {\
	    printf(string);\
	}
#define FSRMT_RPC_DEBUG_PRINT1(string, arg1) \
	if (fsrmt_RpcDebug) {\
	    printf(string, arg1);\
	}
#define FSRMT_RPC_DEBUG_PRINT2(string, arg1, arg2) \
	if (fsrmt_RpcDebug) {\
	    printf(string, arg1, arg2);\
	}
#define FSRMT_RPC_DEBUG_PRINT3(string, arg1, arg2, arg3) \
	if (fsrmt_RpcDebug) {\
	    printf(string, arg1, arg2, arg3);\
	}
#define FSRMT_RPC_DEBUG_PRINT4(string, arg1, arg2, arg3, arg4) \
	if (fsrmt_RpcDebug) {\
	    printf(string, arg1, arg2, arg3, arg4);\
	}
#else
#define FSRMT_RPC_DEBUG_PRINT(string)
#define FSRMT_RPC_DEBUG_PRINT1(string, arg1)
#define FSRMT_RPC_DEBUG_PRINT2(string, arg1, arg2)
#define FSRMT_RPC_DEBUG_PRINT3(string, arg1, arg2, arg3)
#define FSRMT_RPC_DEBUG_PRINT4(string, arg1, arg2, arg3, arg4)
#endif not CLEAN


 /*
 * Sprite Domain functions called via Fsprefix_LookupOperation.
 * These are called with a pathname.
 */
extern	ReturnStatus	FsrmtImport();
extern	ReturnStatus	FsrmtOpen();
extern	ReturnStatus	FsrmtReopen();
extern	ReturnStatus	FsrmtDevOpen();
extern	ReturnStatus	FsrmtDevClose();
extern	ReturnStatus	FsrmtGetAttrPath();
extern	ReturnStatus	FsrmtSetAttrPath();
extern	ReturnStatus	FsrmtMakeDevice();
extern	ReturnStatus	FsrmtMakeDir();
extern	ReturnStatus	FsrmtRemove();
extern	ReturnStatus	FsrmtRemoveDir();
extern	ReturnStatus	FsrmtRename();
extern	ReturnStatus	FsrmtHardLink();


/*
 * Sprite Domain functions called via the fsAttrOpsTable switch.
 * These are called with a fileID.
 */
extern	ReturnStatus	FsrmtGetAttr();
extern	ReturnStatus	FsrmtSetAttr();

extern ReturnStatus FsrmtDeviceIoOpen();
extern Fs_HandleHeader *FsrmtDeviceVerify();
extern ReturnStatus FsrmtDeviceMigrate();
extern ReturnStatus FsrmtDeviceReopen();

extern Fs_HandleHeader *FsrmtPipeVerify();
extern ReturnStatus FsrmtPipeMigrate();
extern ReturnStatus FsrmtPipeReopen();
extern ReturnStatus FsrmtPipeClose();


extern ReturnStatus	FsrmtFileIoOpen();
extern Fs_HandleHeader	*FsrmtFileVerify();
extern ReturnStatus	FsrmtFileRead();
extern ReturnStatus	FsrmtFileWrite();
extern ReturnStatus	FsrmtFilePageRead();
extern ReturnStatus	FsrmtFilePageWrite();
extern ReturnStatus	FsrmtFileIOControl();
extern ReturnStatus	FsrmtFileSelect();
extern ReturnStatus	FsrmtFileGetIOAttr();
extern ReturnStatus	FsrmtFileSetIOAttr();
extern ReturnStatus	FsrmtFileMigClose();
extern ReturnStatus	FsrmtFileMigOpen();
extern ReturnStatus	FsrmtFileMigrate();
extern ReturnStatus	FsrmtFileReopen();
extern ReturnStatus     FsrmtFileBlockAllocate();
extern ReturnStatus     FsrmtFileBlockRead();
extern ReturnStatus     FsrmtFileBlockWrite();
extern ReturnStatus     FsrmtFileBlockCopy();
extern Boolean		FsrmtFileScavenge();
extern ReturnStatus	FsrmtFileClose();


#endif _FSSPRITEDOMAIN
