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

#define FS_TRACE_0	0
#define FS_TRACE_1	1
#define FS_TRACE_2	2
#define FS_TRACE_3	3
#define FS_TRACE_4	4
#define FS_TRACE_5	5
#define FS_TRACE_6	6
#define FS_TRACE_7	7
#define FS_TRACE_8	8
#define FS_TRACE_9	9
#define FS_TRACE_10	10
#define FS_TRACE_11	11
#define FS_TRACE_SRV_OPEN_1	12
#define FS_TRACE_SRV_OPEN_2	13
#define FS_TRACE_SRV_CLOSE_1	14
#define FS_TRACE_SRV_CLOSE_2	15
#define FS_TRACE_SRV_READ_1	16
#define FS_TRACE_SRV_READ_2	17
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
typedef enum { FST_NIL, FST_IO, FST_NAME, FST_HANDLE, FST_RA } FsTraceRecType ;

typedef struct FsTraceIORec {
    FsFileID	fileID;
    int		offset;
    int		numBytes;
} FsTraceIORec;

typedef struct FsTraceRecord {
    union {
	FsFileID	fileID;
	FsTraceIORec	ioRec;
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

#define FS_TRACE_NAME(event, pathName) \
    if (fsTracing) {							\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)pathName);	\
    }

#define FS_TRACE_HANDLE(event, handlePtr) \
    if (fsTracing) {							\
	Trace_Insert(fsTraceHdrPtr, event, (ClientData)&handlePtr->rec.fileID);\
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
#define	fS_TRACE_READ_AHEAD(event, blockNum)

#endif not CLEAN

#endif _FSTRACE
