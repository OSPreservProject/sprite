/* 
 * fsOpTable.c --
 *
 *	The operation tables for the file system.  They are encountered
 *	by the system in roughly the order they are presented here.  First
 *	the Domain Lookup routines are used for name operations.  They
 *	are used by Fsprefix_LookupOperation and Fsprefix_TwoNameOperation which use
 *	the prefix table to choose a server.  If a stream is to be made
 *	then the Open operations are used.  Next comes the Stream operations.
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
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsio.h"
#include "fsutil.h"

#define	FS_NUM_FILE_TYPE	(FS_XTRA_FILE+1)

/*
 * File type server open routine table:
 *
 * THIS ARRAY INDEXED BY FILE TYPE. 
 */
Fsio_OpenOps fsio_OpenOpTable[FS_NUM_FILE_TYPE];

/*
 * Stream type specific routine table.  See fsOpTable.h for an explaination
 *	of the calling sequence for each routine.
 *
 * THIS ARRAY INDEXED BY STREAM TYPE.  Do not arbitrarily insert entries.
 */
Fsio_StreamTypeOps fsio_StreamOpTable[FSIO_NUM_STREAM_TYPES];


/*
 * Simple arrays are used to map between local and remote types.
 */
int fsio_RmtToLclType[FSIO_NUM_STREAM_TYPES] = {
    FSIO_STREAM, 			/* FSIO_STREAM */
    FSIO_LCL_FILE_STREAM,		/* FSIO_LCL_FILE_STREAM */
    FSIO_LCL_FILE_STREAM,		/* FSIO_RMT_FILE_STREAM */
    FSIO_LCL_DEVICE_STREAM,	/* FSIO_LCL_DEVICE_STREAM */
    FSIO_LCL_DEVICE_STREAM,	/* FSIO_RMT_DEVICE_STREAM */
    FSIO_LCL_PIPE_STREAM,		/* FSIO_LCL_PIPE_STREAM */
    FSIO_LCL_PIPE_STREAM,		/* FSIO_RMT_PIPE_STREAM */
    FSIO_CONTROL_STREAM,		/* FSIO_CONTROL_STREAM */
    FSIO_SERVER_STREAM,		/* FSIO_SERVER_STREAM */
    FSIO_LCL_PSEUDO_STREAM,	/* FSIO_LCL_PSEUDO_STREAM */
    FSIO_LCL_PSEUDO_STREAM,	/* FSIO_RMT_PSEUDO_STREAM */
    FSIO_PFS_CONTROL_STREAM,	/* FSIO_PFS_CONTROL_STREAM */
    FSIO_LCL_PSEUDO_STREAM,	/* FSIO_PFS_NAMING_STREAM */
    FSIO_LCL_PFS_STREAM,		/* FSIO_LCL_PFS_STREAM */
    FSIO_LCL_PFS_STREAM,		/* FSIO_RMT_PFS_STREAM */
    FSIO_CONTROL_STREAM,		/* FSIO_RMT_CONTROL_STREAM */
    FSIO_PASSING_STREAM,		/* FSIO_PASSING_STREAM */
#ifdef INET
    FSIO_RAW_IP_STREAM,		/* FSIO_RAW_IP_STREAM */
    FSIO_UDP_STREAM,		/* FSIO_UDP_STREAM */
    FSIO_TCP_STREAM,		/* FSIO_TCP_STREAM */
#endif /* INET */

};

int fsio_LclToRmtType[FSIO_NUM_STREAM_TYPES] = {
    FSIO_STREAM, 			/* FSIO_STREAM */
    FSIO_RMT_FILE_STREAM,		/* FSIO_LCL_FILE_STREAM */
    FSIO_RMT_FILE_STREAM,		/* FSIO_RMT_FILE_STREAM */
    FSIO_RMT_DEVICE_STREAM,	/* FSIO_LCL_DEVICE_STREAM */
    FSIO_RMT_DEVICE_STREAM,	/* FSIO_RMT_DEVICE_STREAM */
    FSIO_RMT_PIPE_STREAM,		/* FSIO_LCL_PIPE_STREAM */
    FSIO_RMT_PIPE_STREAM,		/* FSIO_RMT_PIPE_STREAM */
    FSIO_CONTROL_STREAM,		/* FSIO_CONTROL_STREAM */
    -1,				/* FSIO_SERVER_STREAM */
    FSIO_RMT_PSEUDO_STREAM,	/* FSIO_LCL_PSEUDO_STREAM */
    FSIO_RMT_PSEUDO_STREAM,	/* FSIO_RMT_PSEUDO_STREAM */
    FSIO_PFS_CONTROL_STREAM,	/* FSIO_PFS_CONTROL_STREAM */
    FSIO_PFS_NAMING_STREAM,	/* FSIO_PFS_NAMING_STREAM */
    FSIO_RMT_PFS_STREAM,		/* FSIO_LCL_PFS_STREAM */
    FSIO_RMT_PFS_STREAM,		/* FSIO_RMT_PFS_STREAM */
    FSIO_RMT_CONTROL_STREAM,	/* FSIO_RMT_CONTROL_STREAM */
    FSIO_PASSING_STREAM,		/* FSIO_PASSING_STREAM */
#ifdef INET
    FSIO_RAW_IP_STREAM,		/* FSIO_RAW_IP_STREAM */
    FSIO_UDP_STREAM,		/* FSIO_UDP_STREAM */
    FSIO_TCP_STREAM,		/* FSIO_TCP_STREAM */
#endif /* INET */
};


/*
 * This array contains type-specific functions for the recovery test
 * statistics syscall.
 */
extern	int	Fsrmt_FileRecovTestNumCacheBlocks();
extern	int	Fsrmt_FileRecovTestNumDirtyCacheBlocks();
Fsio_RecovTestInfo	fsio_StreamRecovTestFuncs[FSIO_NUM_STREAM_TYPES] = {
    /* FSIO_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_LCL_FILE_STREAM */
    { Fsio_FileRecovTestUseCount, Fsio_FileRecovTestNumCacheBlocks,
      Fsio_FileRecovTestNumDirtyCacheBlocks },
    /* FSIO_RMT_FILE_STREAM */
    { (int (*)()) NIL, Fsrmt_FileRecovTestNumCacheBlocks,
      Fsrmt_FileRecovTestNumDirtyCacheBlocks},
    /* FSIO_LCL_DEVICE_STREAM */
    { Fsio_DeviceRecovTestUseCount, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_RMT_DEVICE_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_LCL_PIPE_STREAM */
    { Fsio_PipeRecovTestUseCount, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_RMT_PIPE_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_CONTROL_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_SERVER_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_LCL_PSEUDO_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_RMT_PSEUDO_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_PFS_CONTROL_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_PFS_NAMING_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_LCL_PFS_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_RMT_PFS_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_RMT_CONTROL_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_PASSING_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
#ifdef INET
    /* FSIO_RAW_IP_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_UDP_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
    /* FSIO_TCP_STREAM */
    { (int (*)()) NIL, (int (*)()) NIL, (int (*)()) NIL },
#endif /* INET */
};

/*
 *----------------------------------------------------------------------
 *
 * Fsio_InstallStreamOps --
 *
 *	Install the stream operation routines for a specified stream type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	fsio_StreamOpTable modified.
 *
 *----------------------------------------------------------------------
 */
void
Fsio_InstallStreamOps(streamType, streamOpsPtr)
    int		streamType; 	/* Stream type to install operations for. */
    Fsio_StreamTypeOps *streamOpsPtr; /* Operations for stream. */
{
    fsio_StreamOpTable[streamType] = *streamOpsPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_InstallSrvOpenOp --
 *
 *	Install file server open procedure for a specified file type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	fsio_OpenOpTable modified.
 *
 *----------------------------------------------------------------------
 */

void
Fsio_InstallSrvOpenOp(fileType, openOpsPtr)
    int		fileType; 	/* File type to install operations for. */
    Fsio_OpenOps  *openOpsPtr;     /* Operations for file type. */

{
    fsio_OpenOpTable[fileType] = *openOpsPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_NullProc --
 *
 *	Null procedure for entries in Fsio_StreamTypeOps table.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsio_NullProc()
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_NoProc --
 *
 *    No procedure for entries in Fsio_StreamTypeOps table. This is always
 *    returns failure.
 *
 * Results:
 *	FAILURE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_NoProc()
{
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 *  Fsio_NullClientKill --
 *
 *  	Minimum procedure for client kill stream operation.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Handle unlocked.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Fsio_NullClientKill(hdrPtr, clientID)
    Fs_HandleHeader *hdrPtr;
    int clientID;
{
    Fsutil_HandleUnlock(hdrPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_NoHandle --
 *
 *	Stub for a stream ops client verify routine that will always fail.
 *
 * Results:
 *	A NILL FsHanldeHeader.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
Fsio_NoHandle()
{
    return((Fs_HandleHeader *)NIL);
}


