/*
 * vmTrace.h --
 *
 *     Virtual memory data structures and procedure headers for tracing.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMTRACE
#define _VMTRACE

#include "vmMachTrace.h"

/*
 * Definition for the trace buffer.  The trace buffer is a circular buffer.
 * The indices into the buffer are managed as if the buffer was infinite in
 * size.  Thus when one of the indices is used it has to be done as
 * (index & (VM_TRACE_BUFFER_SIZE - 1)).  vmTraceFirstByte contains the
 * index of the first valid byte in the buffer and vmTraceNextByte contains
 * the index of where the next byte is to be put.  In no case can
 * vmTraceNextByte - vmTraceFirstByte exceed VM_TRACE_BUFFER_SIZE.
 */
#define	VM_TRACE_BUFFER_SIZE	(32 * 1024)
#define	VM_TRACE_BUF_MASK	(~(VM_TRACE_BUFFER_SIZE - 1))
extern	char		*vmTraceBuffer;
extern	int		vmTraceFirstByte;
extern	int		vmTraceNextByte;

/*
 * Macro to go from an infinite buffer index (i.e. vmTraceNextByte) to
 * an index that fits in the buffer.
 */
#define	VM_GET_BUFFER_INDEX(index) (index & (VM_TRACE_BUFFER_SIZE - 1))

/*
 * Macro to get a pointer to the next trace record.
 */
#define	VM_GET_TRACE_BUFFER_PTR(type) \
    (type *)(vmTraceBuffer + VM_GET_BUFFER_INDEX(vmTraceNextByte))

/*
 * The different record types.  The first short of each record determines its
 * type.  If it is not one of the three types below then it is a page 
 * reference, modify record type since these types never have the sign bit set.
 *
 *	VM_TRACE_TIME_REC	A record of type Vm_TraceTimeStamp
 *	VM_TRACE_SEG		A record of type Vm_TraceSeg
 *	VM_TRACE_SKIP_REC	This type means that should skip bytes up
 *				until hit a multiple of VM_TRACE_BUFFER_SIZE.
 *	VM_TRACE_STEAL_PMEG_REC	The following segment record relates to a
 *				stolen PMEG.
 *	VM_TRACE_START_REC	A record of type Vm_TraceStart
 */
#define	VM_TRACE_TIME_REC	-1
#define	VM_TRACE_SEG_REC	-2
#define	VM_TRACE_SKIP_REC	-3
#define	VM_TRACE_STEAL_PMEG_REC	-4
#define	VM_TRACE_START_REC	-5

/*
 * Trace begin time stamp record.
 */
typedef struct {
    short		recType;	/* Always equals VM_TRACE_TIME_REC. */
    Time		time;
} Vm_TraceTimeStamp;

/*
 * Segment trace begin record.
 */
typedef struct {
    short		recType;	/* Always equals VM_TRACE_SEG_REC. */
    unsigned	short	hardSegNum;	/* 128Kbyte segment number. */
    unsigned	short	softSegNum;	/* Which of the 256 software segments.*/
    unsigned	char	segType;	/* One of VM_SYSTEM, VM_CODE, VM_HEAP,
					 * VM_STACK. */
    unsigned	char	refCount;	/* Number of processes that are using
					 * the segment. */
} Vm_TraceSeg;

/*
 * Page reference, modified trace record.  The lower 4 bits contain which
 * page within the hardware segment that this page lies and the bytes contains
 * info as to whether the page was referenced of modified.
 */
#define	VM_TRACE_REFERENCED	0x100
#define	VM_TRACE_MODIFIED	0x200

typedef unsigned short	Vm_TracePage;

/*
 * Start trace record.
 */
typedef struct {
    short	recType;
    int		hostID;
    int		pageSize;
    int		numPages;
    Address	codeStartAddr;
    Address	dataStartAddr;
    Address	stackStartAddr;
    Address	mapStartAddr;
    Address	cacheStartAddr;
    Address	cacheEndAddr;
} Vm_TraceStart;

/*
 * Variable to indicate which trace iteration that this is.  Is incremented
 * every time a trace is taken.
 */
extern	int		vmTraceTime;

extern	Boolean		vmTracing;
extern	int		vmTracesPerClock;
extern	int		vmTracesToGo;
extern	Fs_Stream	*vmTraceFilePtr;
extern	char		*vmTraceFileName;
extern	Boolean		vmTraceDumpStarted;

/*
 * Trace stats.
 */
typedef struct {
    int			traceDumps;
    int			traceDrops;
    VmMach_TraceStats	machStats;
} VmTraceStats;
extern	VmTraceStats	vmTraceStats;

/*
 * The name of the trace file is the following followed by the host on
 * which the trace is occuring.
 */
#define	VM_TRACE_FILE_NAME	"/sprite/vmtrace/tfile."

/*
 * Trace dump file and function to do the tracing.
 */
extern	Fs_Stream	*vmTraceFilePtr;
extern	char		*vmTraceFileName;
extern	Boolean		vmTraceDumpStarted;
extern	void		VmTraceDump();
#endif _VMTRACE
