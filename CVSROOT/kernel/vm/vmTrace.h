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

#ifdef KERNEL
#include <vmMachTrace.h>
#include <vmStat.h>
#include <spriteTime.h>
#else
#include <kernel/vmMachTrace.h>
#include <vmStat.h>
#include <spriteTime.h>
#endif

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
 * Trace stats.
 */
typedef struct {
    int			traceDumps;
    int			traceDrops;
    int			numTraces;
    VmMach_TraceStats	machStats;
} Vm_TraceStats;
extern	Vm_TraceStats	vmTraceStats;

/*
 * The different record types.  The first short of each record determines its
 * type.  If it is not one of the three types below then it is a page
 * reference, modify record type since these types never have the sign bit set.
 *
 *	VM_TRACE_START_REC	A record of type Vm_TraceStart.
 *	VM_TRACE_END_REC	A record of type Vm_TraceEnd.
 *	VM_TRACE_TIME_REC	A record of type Vm_TraceTimeStamp
 *	VM_TRACE_SEG		A record of type Vm_TraceSeg
 *	VM_TRACE_SKIP_REC	This type means that should skip bytes up
 *				until hit a multiple of VM_TRACE_BUFFER_SIZE.
 *	VM_TRACE_STEAL_PMEG_REC	The following segment record relates to a
 *				stolen PMEG.
 *	VM_TRACE_SEG_CREATE_REC	A record of type Vm_TraceSegCreate.
 *	VM_TRACE_SEG_DESTROY_REC A record of type Vm_TraceSegDestroy.
 *	VM_TRACE_PTE_CHANGE_REC	A record of type Vm_TracePTEChangeRec.
 *	VM_TRACE_CLEAR_COW_REC	A record of type Vm_TraceClearCOW
 *	VM_TRACE_PAGE_FAULT_REC	A record of type Vm_TracePageFault
 *	VM_TRACE_END_INIT_REC	Trace initialization has completed.
 */
#define	VM_TRACE_START_REC		-1
#define	VM_TRACE_END_REC		-2
#define	VM_TRACE_TIME_REC		-3
#define	VM_TRACE_SEG_REC		-4
#define	VM_TRACE_SKIP_REC		-5
#define	VM_TRACE_STEAL_PMEG_REC		-6
#define	VM_TRACE_SEG_CREATE_REC		-7
#define	VM_TRACE_SEG_DESTROY_REC	-8
#define	VM_TRACE_PTE_CHANGE_REC		-9
#define	VM_TRACE_CLEAR_COW_REC		-10
#define	VM_TRACE_PAGE_FAULT_REC		-11
#define	VM_TRACE_END_INIT_REC		-12
#define	VM_TRACE_MIN_REC_TYPE		-12

/*
 * Start trace record.
 */
typedef struct {
    short	recType;		/* Always equals VM_TRACE_START_REC. */
    int		hostID;			/* Sprite host number. */
    int		pageSize;		/* The page size. */
    int		numPages;		/* The number of physical pages. */
    Address	codeStartAddr;		/* The starting address of the kernel's
					 * code (runs up to dataStartAddr). */
    Address	dataStartAddr;		/* The starting address of the kernel's
					 * data (runs up to stackStartAddr). */
    Address	stackStartAddr;		/* The start of the range of virtual
					 * addresses that are used for kernel
					 * stacks (runs up to mapStartAddr). */
    Address	mapStartAddr;		/* The start of kernel virtual
					 * addresses used for mapping stuff
					 * (runs up to cacheStartAddr). */
    Address	cacheStartAddr;		/* The start of the FS cache. */
    Address	cacheEndAddr;		/* The end of the FS cache. */
    Vm_Stat	startStats;		/* Stats at the start of the trace. */
    int		tracesPerSecond;	/* The number of traces per second. */
} Vm_TraceStart;

/*
 * End trace record.
 */
typedef struct {
    short		recType;	/* Always equals VM_TRACE_END_REC. */
    Vm_Stat		endStats;	/* Stats at the end of the trace. */
    Vm_TraceStats	traceStats;	/* Trace stats. */
} Vm_TraceEnd;

/*
 * Trace time stamp record.
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
    unsigned	short	hardSegNum;	/* Which hardware segment.  Hardware
					 * segments are a multiple of 32K on
					 * Sun-2s and 128K on Sun3's. */
    unsigned	short	softSegNum;	/* Which of the 256 software segments.*/
    unsigned	char	segType;	/* One of VM_SYSTEM, VM_CODE, VM_HEAP,
					 * VM_STACK. */
    unsigned	char	refCount;	/* Number of processes that are using
					 * the segment. */
} Vm_TraceSeg;

/*
 * Page reference, modified trace record.  The lower 4 bits contain which
 * page within the hardware segment that this page is and the high order byte
 * contains info as to whether the page was referenced or modified.
 */
#define	VM_TRACE_REFERENCED	0x100
#define	VM_TRACE_MODIFIED	0x200

typedef unsigned short	Vm_TracePage;

/*
 * Segment creation record.
 */
typedef struct {
    short	recType;	/* Always VM_TRACE_SEG_CREATE_REC. */
    short	segNum;		/* The segment being created. */
    short	parSegNum;	/* The parent segment. */
    char	segType;	/* The type of segment. */
    char	cor;		/* TRUE if the segment was created
				 * copy-on-reference. */
} Vm_TraceSegCreate;

/*
 * Segment destruction record.
 */
typedef struct {
    short	recType;	/* Always VM_TRACE_SEG_DESTROY_REC. */
    short	segNum;		/* The segment being destroyed. */
} Vm_TraceSegDestroy;

/*
 * PTE Change record.
 */
typedef struct {
    short		recType;	/* Always VM_TRACE_PTE_CHANGE_REC. */
    char		softPTE;	/* TRUE if is the software page table
					 * entry and FALSE if is the hardware
					 * one. */
    char		changeType;	/* Type	of pte change (defined below)*/
    short		segNum;		/* The segment that the page is in. */
    unsigned short	pageNum;	/* The virtual page number. */
    unsigned int	beforePTE;	/* The PTE before the change. */
    unsigned int	afterPTE;	/* The PTE after the change. */
} Vm_TracePTEChange;

/*
 * Types of PTE changes.
 *
 *	VM_TRACE_CLEAR_REF_BIT		The reference bit was cleared.
 *	VM_TRACE_CLEAR_MOD_BIT		The modified bit was cleared.
 *	VM_TRACE_SET_PAGE_PROT		The page protection was set.
 *	VM_TRACE_SET_SEG_PROT		The protection of the entire segment
 *					was set.
 *	VM_TRACE_VALIDATE_PAGE		A page was validated.
 *	VM_TRACE_INVALIDATE_PAGE	A page was invalidated.
 *	VM_TRACE_LAST_COR		The last copy-on-reference slave
 *					was removed.
 *	VM_TRACE_COW_TO_NORMAL		A page is being changed from COW to
 *					normal protection because of a
 *					copy-on-write fault.
 *	VM_TRACE_GIVEN_FROM_MASTER	The master segment is invalidating
 *					its copy of the page so that it can
 *					give it to a slave.
 *	VM_TRACE_TAKEN_BY_SLAVE		The slave segment took the page from
 *					the master when the master gave it
 *					away with VM_TRACE_GIVEN_FROM_MASTER.
 *	VM_TRACE_COW_COR_CHANGE		A generic copy-on-write change.
 */
#define	VM_TRACE_CLEAR_REF_BIT		1
#define	VM_TRACE_CLEAR_MOD_BIT		2
#define	VM_TRACE_SET_PAGE_PROT		3
#define	VM_TRACE_SET_SEG_PROT		4
#define	VM_TRACE_VALIDATE_PAGE		5
#define	VM_TRACE_INVALIDATE_PAGE	6
#define	VM_TRACE_LAST_COR		7
#define	VM_TRACE_COW_TO_NORMAL		8
#define	VM_TRACE_GIVEN_FROM_MASTER	9
#define	VM_TRACE_TAKEN_BY_SLAVE		10
#define	VM_TRACE_COW_COR_CHANGE		11
#define	VM_TRACE_MAX_PTE_CHANGE_TYPE	11

/*
 * Page fault type record.
 */
typedef struct {
    short		recType;	/* Always VM_TRACE_PAGE_FAULT_REC. */
    short		segNum;		/* The segment that the page is in. */
    unsigned short	pageNum;	/* The virtual page number. */
    short		faultType;	/* One of VM_TRACE_ZERO_FILL,
					 * VM_TRACE_OBJ_FILE,
					 * VM_TRACE_SWAP_FILE. */
} Vm_TracePageFault;

/*
 * Different types of page faults:
 *
 *	VM_TRACE_ZERO_FILL	Page was zero filled.
 *	VM_TRACE_OBJ_FILE	Page was demand loaded from the object file.
 *	VM_TRACE_SWAP_FILE	Page was loaded in from the swap file.
 */
#define	VM_TRACE_ZERO_FILL	1
#define	VM_TRACE_OBJ_FILE	2
#define	VM_TRACE_SWAP_FILE	3

/*
 * Clear copy-on-write record.
 */
typedef struct {
    short		recType;	/* Always VM_TRACE_CLEAR_COW_REC. */
    short		segNum;		/* The segment that is being made to
					 * be no longer copy-on-write. */
} Vm_TraceClearCOW;

/*
 * Variable to indicate which trace iteration that this is.  Is incremented
 * every time a trace is taken.
 */
extern	int		vmTraceTime;
extern	Boolean		vmTraceNeedsInit;
extern	int		vmTracesPerClock;
extern	int		vmTracesToGo;
extern	Fs_Stream	*vmTraceFilePtr;
extern	char		*vmTraceFileName;
extern	Boolean		vmTraceDumpStarted;

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
extern void VmTraceDump _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
extern void VmStoreTraceRec _ARGS_((int recType, int size, Address traceRecAddr, Boolean checkOverflow));
extern void VmCheckTraceOverflow _ARGS_((void));

#endif /* _VMTRACE */
