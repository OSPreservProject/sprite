/*
 * vmTypes.h --
 *
 *	Type declarations for user VM interface.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/vmTypes.h,v 1.1 91/03/01 22:12:25 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMTYPESUSER
#define _VMTYPESUSER

#include <sprite.h>


/*
 * The different commands to give to Vm_Cmd.  For more details on these
 * options look at the man page for vmcmd.
 *
 *    VM_SET_CLOCK_PAGES		Set the number of pages to check on
 *					each iteration of the clock algorithm
 *    VM_SET_CLOCK_INTERVAL		Set how many seconds between iterations
 *					of the clock algorithm.
 *    VM_SET_MAX_DIRTY_PAGES		Set the maximum number of dirty pages
 *					to check before waiting for clean 
 *					memory.
 *    VM_DO_COPY_IN			Time Vm_CopyIn.
 *    VM_DO_COPY_OUT			Time Vm_CopyOut.
 *    VM_DO_MAKE_ACCESS_IN		Time Vm_MakeAccessible.
 *    VM_DO_MAKE_ACCESS_OUT		Time Vm_MakeAccessible.
 *    VM_SET_COPY_SIZE			Set the number of bytes to move when
 *					using the previous 4 commands.
 *    VM_GET_STATS			Return vm stats.
 *    VM_SET_PAGEOUT_PROCS		Set the number of processes used 
 *					to clean memory.
 *    VM_SET_COW			Set the flags which determines whether
 *					to use copy-on-write or not.
 *    VM_COUNT_DIRTY_PAGES		Determine how many pages in memory
 *					are dirty.
 *    VM_SET_FREE_WHEN_CLEAN		Set the flag which determines whether
 *					to free memory after it is cleaned.
 *    VM_FLUSH_SEGMENT			Flush a segment from memory.
 *    VM_SET_FS_PENALTY			Set the file system penalty .
 *    VM_SET_NUM_PAGE_GROUPS		Set the number of pages groups to 
 *					divide memory into when calculating
 *					the penalty.
 *    VM_SET_ALWAYS_REFUSE		Always refuse file system requests for
 *					memory.
 *    VM_SET_ALWAYS_SAY_YES		Always satisfy file system request for
 *					memory if possible.
 *    VM_RESET_FS_STATS			Clear out the min and max size of
 *					the file system cache kept in the
 *					vm stat structure.
 *    VM_SET_COR_READ_ONLY		Set the flag which determines if pages
 *					that are copied because of copy-on-ref
 *					faults are marked read-only.
 *    VM_SET_PREFETCH			Turn prefetch on or off.
 *    VM_SET_USE_FS_READ_AHEAD		Say whether to use the file systems
 *					read-ahead to help out vm paging.
 *    VM_START_TRACING			Start virtual memory tracing.
 *    VM_END_TRACING			Stop virtual memory tracing.
 *    VM_SET_WRITEABLE_PAGEOUT		Set flag which forces all writeable
 *					pages to be written out to swap when
 *					they are recycled whether they are
 *					dirty or not.
 *    VM_SET_WRITEABLE_REF_PAGEOUT	Set flag which forces all writeable
 *					pages that have been referenced to be
 *					written out to swap when they are 
 *					recycled whether they are dirty or not.
 */
#define VM_SET_CLOCK_PAGES		0
#define VM_SET_CLOCK_INTERVAL		1
#define VM_SET_MAX_DIRTY_PAGES		4
#define VM_DO_COPY_IN			5
#define VM_DO_COPY_OUT			6
#define VM_DO_MAKE_ACCESS_IN		7
#define VM_DO_MAKE_ACCESS_OUT		8
#define VM_SET_COPY_SIZE		9
#define VM_GET_STATS			10
#define VM_SET_PAGEOUT_PROCS		11
#define VM_FORCE_SWAP			12
#define VM_SET_COW			13
#define VM_COUNT_DIRTY_PAGES		14
#define VM_SET_FREE_WHEN_CLEAN		15
#define VM_FLUSH_SEGMENT		16
#define VM_SET_FS_PENALTY		17
#define VM_SET_NUM_PAGE_GROUPS		18
#define VM_SET_ALWAYS_REFUSE		19
#define VM_SET_ALWAYS_SAY_YES		20
#define VM_RESET_FS_STATS		21
#define VM_SET_COR_READ_ONLY		22
#define VM_SET_PREFETCH			23
#define VM_SET_USE_FS_READ_AHEAD	24
#define VM_START_TRACING		25
#define VM_END_TRACING			26
#define	VM_SET_WRITEABLE_PAGEOUT	27
#define	VM_SET_WRITEABLE_REF_PAGEOUT	28

/*
 * The first allowable machine dependent command.
 */
#define	VM_FIRST_MACH_CMD		50

/*
 * Number of segments
 */

#define	VM_NUM_SEGMENTS		4

/*
 * The type of segment.
 */
#define VM_SYSTEM	0
#define VM_CODE		1
#define VM_HEAP		2
#define VM_STACK	3
#define VM_SHARED	4


/*
 * Length of the object file name that is embedded in each segment table
 * entry.
 */
#define	VM_OBJ_FILE_NAME_LENGTH	50

/*
 * Implementation independent definition of segment ids.
 */

typedef int Vm_SegmentID;

/*
 * Segment information. Add any new fields to the end of the structure.
 */

typedef struct Vm_SegmentInfo {
    int			segNum;		/* The number of this segment. */
    int 		refCount;	/* Number of processes using this 
					 * segment */
				        /* Name of object file for code 
					 * segments. */
    char		objFileName[VM_OBJ_FILE_NAME_LENGTH];
    int           	type;		/* CODE, STACK, HEAP, or SYSTEM */
    int			numPages;	/* Explained in vmInt.h. */
    int			ptSize;		/* Number of pages in the page table */
    int			resPages;	/* Number of pages in physical memory
					 * for this segment. */
    int			flags;		/* Flags to give information about the
					 * segment table entry. */
    int			ptUserCount;	/* The number of current users of this
					 * page table. */
    int			numCOWPages;	/* Number of copy-on-write pages that
					 * this segment references. */
    int			numCORPages;	/* Number of copy-on-ref pages that
					 * this segment references. */
    Address		minAddr;	/* Minimum address that the segment
					 * can ever have. */
    Address		maxAddr;	/* Maximium address that the segment
					 * can ever have. */
    int			traceTime;	/* The last trace interval that this
					 * segment was active. */
} Vm_SegmentInfo;


#endif /* _VMTYPESUSER */
