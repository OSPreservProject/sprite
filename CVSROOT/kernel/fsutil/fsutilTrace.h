/*
 * fsutilTrace.h --
 *
 *	Definitions for the filesystem trace record.  This includes the
 *	trace record types, and externs for the trace record itself.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * feg is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef _FSTRACE
#define _FSTRACE

#include "trace.h"

#define FSUTIL_TRACE_0		0
#define FSUTIL_TRACE_OPEN_START	1
#define FSUTIL_TRACE_LOOKUP_START	2
#define FSUTIL_TRACE_LOOKUP_DONE	3
#define FSUTIL_TRACE_DEL_LAST_WR	4
#define FSUTIL_TRACE_OPEN_DONE	5
#define FSUTIL_TRACE_BLOCK_WAIT	6
#define FSUTIL_TRACE_BLOCK_HIT	7
#define FSUTIL_TRACE_DELETE		8
#define FSUTIL_TRACE_NO_BLOCK	9
#define FSUTIL_TRACE_OPEN_DONE_2	10
#define FSUTIL_TRACE_OPEN_DONE_3	11
#define FSUTIL_TRACE_INSTALL_NEW	12
#define FSUTIL_TRACE_INSTALL_HIT	13
#define FSUTIL_TRACE_RELEASE_FREE	14
#define FSUTIL_TRACE_RELEASE_LEAVE	15
#define FSUTIL_TRACE_REMOVE_FREE	16
#define FSUTIL_TRACE_REMOVE_LEAVE	17
#define FSUTIL_TRACE_SRV_WRITE_1	18
#define FSUTIL_TRACE_SRV_WRITE_2	19
#define FSUTIL_TRACE_SRV_GET_ATTR_1	20
#define FSUTIL_TRACE_SRV_GET_ATTR_2	21
#define FSUTIL_TRACE_OPEN		22
#define FSUTIL_TRACE_READ		23
#define FSUTIL_TRACE_WRITE		24
#define FSUTIL_TRACE_CLOSE		25
#define	FSUTIL_TRACE_RA_SCHED	26
#define	FSUTIL_TRACE_RA_BEGIN	27
#define	FSUTIL_TRACE_RA_END		28
#define FSUTIL_TRACE_DEL_BLOCK	29
#define FSUTIL_TRACE_BLOCK_WRITE	30
#define FSUTIL_TRACE_GET_NEXT_FREE	31
#define FSUTIL_TRACE_LRU_FREE	32
#define FSUTIL_TRACE_LRU_DONE_FREE	33
#define FSUTIL_TRACE_34
#define FSUTIL_TRACE_35

extern Trace_Header *fsutil_TraceHdrPtr;
extern int fsutil_TraceLength;
extern Boolean fsutil_Tracing;
extern int fsutil_MaxTraceDataSize;
extern int fscache_RATracing;

/*
 * The following types and macros are used to take filesystem trace data.
 * Each struct has to be smaller than a Fsutil_TraceRecord - see the call to
 * Trace_Init in fsInit.c - as the trace module pre-allocates storage.
 */
typedef enum { FST_NIL, FST_IO, FST_NAME,
		FST_HANDLE, FST_RA, FST_BLOCK } Fsutil_TraceRecType ;

typedef struct Fsutil_TraceIORec {
    Fs_FileID	fileID;
    int		offset;
    int		numBytes;
} Fsutil_TraceIORec;

typedef struct Fsutil_TraceHdrRec {
    Fs_FileID	fileID;
    int		refCount;
    int		numBlocks;
} Fsutil_TraceHdrRec;

typedef struct Fsutil_TraceBlockRec {
    Fs_FileID	fileID;
    int		blockNum;
    int		flags;
} Fsutil_TraceBlockRec;

typedef struct Fsutil_TraceRecord {
    union {
	Fs_FileID	fileID;
	Fsutil_TraceHdrRec	hdrRec;
	Fsutil_TraceIORec	ioRec;
	Fsutil_TraceBlockRec	blockRec;
	char		name[40];
    } un;
} Fsutil_TraceRecord;

extern int fsutil_TracedFile;

#ifndef CLEAN

#define FSUTIL_TRACE(event) \
    if (fsutil_Tracing) {						\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)NIL);	\
    }

#define FSUTIL_TRACE_IO(event, zfileID, zoffset, znumBytes) \
    if (fsutil_Tracing &&						\
	(fsutil_TracedFile < 0 || fsutil_TracedFile == zfileID.minor)) {	\
	Fsutil_TraceIORec ioRec;					\
	ioRec.fileID = zfileID;					\
	ioRec.offset = zoffset;					\
	ioRec.numBytes = znumBytes;				\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)&ioRec);	\
    }

#ifdef notdef
#define FSUTIL_TRACE_NAME(event, pathName) \
    if (fsutil_Tracing) {							\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)pathName);	\
    }
#endif notdef
#define FSUTIL_TRACE_NAME(event, pathName)

#define FSUTIL_TRACE_HANDLE(event, hdrPtr) \
    if (fsutil_Tracing &&							\
	(fsutil_TracedFile < 0 || fsutil_TracedFile == hdrPtr->fileID.minor)) {	\
	Fsutil_TraceHdrRec hdrRec;						\
	hdrRec.fileID = hdrPtr->fileID;					\
	hdrRec.refCount = hdrPtr->refCount;				\
	if (hdrPtr->fileID.type == FSIO_LCL_FILE_STREAM) {		\
	    hdrRec.numBlocks = ((Fsio_FileIOHandle *)hdrPtr)->cacheInfo.blocksInCache; \
	} else {							\
	    hdrRec.numBlocks = -1;					\
	}								\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)&hdrRec);\
    }

#define FSUTIL_TRACE_BLOCK(event, blockPtr) \
    if (fsutil_Tracing &&							\
	(fsutil_TracedFile < 0 ||						\
	 fsutil_TracedFile == (blockPtr)->cacheInfoPtr->hdrPtr->fileID.minor)) { \
	Fsutil_TraceBlockRec blockRec;					\
	blockRec.fileID = (blockPtr)->cacheInfoPtr->hdrPtr->fileID;	\
	blockRec.blockNum = (blockPtr)->blockNum;			\
	blockRec.flags	= (blockPtr)->flags;				\
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)&blockRec);\
    }

#define	FSUTIL_TRACE_READ_AHEAD(event, blockNum) \
    if (fsutil_Tracing || fscache_RATracing) { \
	Trace_Insert(fsutil_TraceHdrPtr, event, (ClientData)blockNum); \
    }

#else
/*
 * Compiling with -DCLEAN will zap the if statement and procedure
 * call defined by the above macros
 */

#define FSUTIL_TRACE(event)
#define FSUTIL_TRACE_IO(event, zfileID, zoffset, znumBytes)
#define FSUTIL_TRACE_NAME(event, pathName)
#define FSUTIL_TRACE_HANDLE(event, handlePtr)
#define	FSUTIL_TRACE_READ_AHEAD(event, blockNum)
#define FSUTIL_TRACE_BLOCK(event, blockPtr)

#endif not CLEAN

#endif _FSTRACE
