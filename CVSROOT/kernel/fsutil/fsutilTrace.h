/*
 * fsTrace.h --
 *
 *	Definitions for the filesystem trace record.  This includes the
 *	trace record types, and externs for the trace record itself.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSTRACE
#define _FSTRACE

#include "trace.h"

#define FS_TRACE_0		0
#define FS_TRACE_OPEN_START	1
#define FS_TRACE_LOOKUP_START	2
#define FS_TRACE_LOOKUP_DONE	3
#define FS_TRACE_4		4
#define FS_TRACE_OPEN_DONE	5
#define FS_TRACE_BLOCK_WAIT	6
#define FS_TRACE_BLOCK_HIT	7
#define FS_TRACE_DELETE		8
#define FS_TRACE_NO_BLOCK	9
#define FS_TRACE_OPEN_DONE_2	10
#define FS_TRACE_OPEN_DONE_3	11
#define FS_TRACE_INSTALL_NEW	12
#define FS_TRACE_INSTALL_HIT	13
#define FS_TRACE_RELEASE_FREE	14
#define FS_TRACE_RELEASE_LEAVE	15
#define FS_TRACE_REMOVE_FREE	16
#define FS_TRACE_REMOVE_LEAVE	17
#define FS_TRACE_SRV_WRITE_1	18
#define FS_TRACE_SRV_WRITE_2	19
#define FS_TRACE_SRV_GET_ATTR_1	20
#define FS_TRACE_SRV_GET_ATTR_2	21
#define FS_TRACE_OPEN	22
#define FS_TRACE_READ	23
#define FS_TRACE_WRITE	24
#define FS_TRACE_CLOSE	25
#define	FS_TRACE_RA_SCHED	26
#define	FS_TRACE_RA_BEGIN	27
#define	FS_TRACE_RA_END		28

extern Trace_Header *fsTraceHdrPtr;
extern int fsTraceLength;
extern Boolean fsTracing;
extern int fsMaxTraceDataSize;
extern int fsRATracing;

/*
 * The following types and macros are used to take filesystem trace data.
 * Each struct has to be smaller than a FsTraceRecord - see the call to
 * Trace_Init in fsInit.c - as the trace module pre-allocates storage.
 */
typedef enum { FST_NIL, FST_IO, FST_NAME,
		FST_HANDLE, FST_RA, FST_BLOCK } FsTraceRecType ;

typedef struct FsTraceIORec {
    FsFileID	fileID;
    int		offset;
    int		numBytes;
} FsTraceIORec;

typedef struct FsTraceHdrRec {
    FsFileID	fileID;
    int		refCount;
    int		numBlocks;
} FsTraceHdrRec;

typedef struct FsTraceBlockRec {
    FsFileID	fileID;
    int		blockNum;
    int		flags;
} FsTraceBlockRec;

typedef struct FsTraceRecord {
    union {
	FsFileID	fileID;
	FsTraceHdrRec	hdrRec;
	FsTraceIORec	ioRec;
	FsTraceBlockRec	blockRec;
	char		name[40];
    } un;
} FsTraceRecord;

#ifndef CLEAN

#define FS_TRACE(event) \
    if (fsTracing) {						\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)NIL);	\
    }

#define FS_TRACE_IO(event, zfileID, zoffset, znumBytes) \
    if (fsTracing) {						\
	FsTraceIORec ioRec;					\
	ioRec.fileID = zfileID;					\
	ioRec.offset = zoffset;					\
	ioRec.numBytes = znumBytes;				\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)&ioRec);	\
    }

#ifdef notdef
#define FS_TRACE_NAME(event, pathName) \
    if (fsTracing) {							\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)pathName);	\
    }
#endif notdef
#define FS_TRACE_NAME(event, pathName)

#define FS_TRACE_HANDLE(event, hdrPtr) \
    if (fsTracing) {							\
	FsTraceHdrRec hdrRec;						\
	hdrRec.fileID = hdrPtr->fileID;					\
	hdrRec.refCount = hdrPtr->refCount;				\
	if (hdrPtr->fileID.type == FS_LCL_FILE_STREAM) {		\
	    hdrRec.numBlocks = ((FsLocalFileIOHandle *)hdrPtr)->cacheInfo.blocksInCache; \
	} else {							\
	    hdrRec.numBlocks = -1;					\
	}								\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)&hdrRec);\
    }

#define FS_TRACE_BLOCK(event, blockPtr) \
    if (fsTracing) {							\
	FsTraceBlockRec blockRec;					\
	blockRec.fileID = (blockPtr)->cacheInfoPtr->hdrPtr->fileID;	\
	blockRec.blockNum = (blockPtr)->blockNum;			\
	blockRec.flags	= (blockPtr)->flags;				\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)&blockRec);\
    }

#define	FS_TRACE_READ_AHEAD(event, blockNum) \
    if (fsTracing || fsRATracing) { \
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)blockNum); \
    }

#else
/*
 * Compiling with -DCLEAN will zap the if statement and procedure
 * call defined by the above macros
 */

#define FS_TRACE(event)
#define FS_TRACE_IO(event, zfileID, zoffset, znumBytes)
#define FS_TRACE_NAME(event, pathName)
#define FS_TRACE_HANDLE(event, handlePtr)
#define	FS_TRACE_READ_AHEAD(event, blockNum)
#define FS_TRACE_BLOCK(event, blockPtr)

#endif not CLEAN

#endif _FSTRACE
