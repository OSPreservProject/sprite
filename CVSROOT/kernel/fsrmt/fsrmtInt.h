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

#include <fsNameOps.h>
#include <proc.h>
#include <fsrmt.h>

#include <stdio.h>

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
extern ReturnStatus FsrmtImport _ARGS_((char *prefix, int serverID, 
		Fs_UserIDs *idPtr, int *domainTypePtr,
		Fs_HandleHeader **hdrPtrPtr));
extern ReturnStatus FsrmtOpen _ARGS_((Fs_HandleHeader *prefixHandle,
		char *relativeName, Address argsPtr, Address resultsPtr,
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtReopen _ARGS_((Fs_HandleHeader *hdrPtr, int inSize,
		Address inData, int *outSizePtr, Address outData));
extern ReturnStatus FsrmtGetAttrPath _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr,
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtSetAttrPath _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtMakeDevice _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtMakeDir _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtRemove _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtRemoveDir _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FsrmtRename _ARGS_((Fs_HandleHeader *prefixHandle1, 
		char *relativeName1, Fs_HandleHeader *prefixHandle2, 
		char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr, Boolean *name1ErrorPtr));
extern ReturnStatus FsrmtHardLink _ARGS_((Fs_HandleHeader *prefixHandle1,
		char *relativeName1, Fs_HandleHeader *prefixHandle2, 
		char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr, Boolean *name1ErrorPtr));


/*
 * Sprite Domain functions called via the fsAttrOpsTable switch.
 * These are called with a fileID.
 */
#ifdef SOSP91
extern ReturnStatus FsrmtGetAttr _ARGS_((Fs_FileID *fileIDPtr, int clientID, 
		Fs_Attributes *attrPtr, int hostID, int userID));
extern ReturnStatus FsrmtSetAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, Fs_UserIDs *idPtr, int flags, int
		clientID, int hostID, int userID));
#else
extern ReturnStatus FsrmtGetAttr _ARGS_((Fs_FileID *fileIDPtr, int clientID, 
		Fs_Attributes *attrPtr));
extern ReturnStatus FsrmtSetAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, Fs_UserIDs *idPtr, int flags));
#endif

extern ReturnStatus FsrmtDeviceIoOpen _ARGS_((Fs_FileID *ioFileIDPtr, 
		int *flagsPtr, int clientID, ClientData streamData,
		char *name, Fs_HandleHeader **ioHandlePtrPtr));
extern Fs_HandleHeader *FsrmtDeviceVerify _ARGS_((Fs_FileID *fileIDPtr, 
		int clientID, int *domainTypePtr));
extern ReturnStatus FsrmtDeviceMigrate _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int dstClientID, int *flagsPtr, int *offsetPtr, int *sizePtr, 
		Address *dataPtr));
extern ReturnStatus FsrmtDeviceReopen _ARGS_((Fs_HandleHeader *hdrPtr, 
		int clientID, ClientData inData, int *outSizePtr, 
		ClientData *outDataPtr));

extern Fs_HandleHeader *FsrmtPipeVerify _ARGS_((Fs_FileID *fileIDPtr,
		int clientID, int *domainTypePtr));
extern ReturnStatus FsrmtPipeReopen _ARGS_((Fs_HandleHeader *hdrPtr, 
		int clientID, ClientData inData, int *outSizePtr, 
		ClientData *outDataPtr));


extern ReturnStatus FsrmtFileIoOpen _ARGS_((Fs_FileID *ioFileIDPtr, 
		int *flagsPtr, int clientID, ClientData streamData, char *name,
		Fs_HandleHeader **ioHandlePtrPtr));
extern ReturnStatus FsrmtFileReopen _ARGS_((Fs_HandleHeader *hdrPtr, 
		int clientID, ClientData inData, int *outSizePtr, 
		ClientData *outDataPtr));
#ifdef SOSP91
extern ReturnStatus FsrmtFileClose _ARGS_((Fs_Stream *streamPtr, int clientID, 
		Proc_PID procID, int flags, int dataSize, 
		ClientData closeData, int *offsetPtr, int *rwFlagsPtr));
#else
extern ReturnStatus FsrmtFileClose _ARGS_((Fs_Stream *streamPtr, int clientID, 
		Proc_PID procID, int flags, int dataSize, ClientData closeData));
#endif
extern Boolean FsrmtFileScavenge _ARGS_((Fs_HandleHeader *hdrPtr));
extern Fs_HandleHeader *FsrmtFileVerify _ARGS_((Fs_FileID *fileIDPtr, 
		int clientID, int *domainTypePtr));
extern ReturnStatus FsrmtFileMigClose _ARGS_((Fs_HandleHeader *hdrPtr, 
		int flags));
extern ReturnStatus FsrmtFileMigOpen _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int size, ClientData data, Fs_HandleHeader **hdrPtrPtr));
extern ReturnStatus FsrmtFileMigrate _ARGS_((Fsio_MigInfo *migInfoPtr, 
		int dstClientID, int *flagsPtr, int *offsetPtr, int *sizePtr, 
		Address *dataPtr));
extern ReturnStatus FsrmtFileRead _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *readPtr, Sync_RemoteWaiter *remoteWaitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus FsrmtFileWrite _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *writePtr, Sync_RemoteWaiter *remoteWaitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus FsrmtFilePageRead _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *readPtr, Sync_RemoteWaiter *remoteWaitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus FsrmtFilePageWrite _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOParam *writePtr, Sync_RemoteWaiter *remoteWaitPtr, 
		Fs_IOReply *replyPtr));
extern ReturnStatus FsrmtFileIOControl _ARGS_((Fs_Stream *streamPtr, 
		Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus FsrmtFileGetIOAttr _ARGS_((Fs_FileID *fileIDPtr, 
		int clientID, Fs_Attributes *attrPtr));
extern ReturnStatus FsrmtFileSetIOAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, int flags));


#endif _FSSPRITEDOMAIN
