/*
 * vmInt.h --
 *
 *	Machine independent virtual memory data structures and procedure
 *	headers used internally by the virtual memory module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMINT
#define _VMINT

#ifdef KERNEL
#include "vmMach.h"
#include "fs.h"
#include "list.h"
#include "sync.h"
#include "proc.h"
#include "status.h"
#else
#include <kernel/vmMach.h>
#include <kernel/fs.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <status.h>
#include <list.h>
#endif

/*
 * KERNEL VIRTUAL ADDRESS SPACE
 *
 * The kernel's virtual address space is divided up in the following way:
 *
 * -------------------------------------- mach_KernStart
 * |					|
 * | Machine dependent stuff		|
 * |					|
 * -------------------------------------- mach_StackBottom
 * |					|
 * | Stack for the main process which	|
 * | is mach_KernStackSize bytes long.	|
 * |					|
 * -------------------------------------- mach_CodeStart
 * |					|
 * | Kernel code + data.  Current end	|
 * | is vmMemEnd which is incremented	|
 * | each time Vm_RawAlloc is called.	|
 * | The absolute end is		|
 * | mach_CodeStart + vmKernMemSize.	|			
 * |					|
 * -------------------------------------- mach_CodeStart + vmKernMemSize
 * |					|
 * | Kernel stacks.  There are		|
 * | vmMaxProcesses worth of stacks.	|
 * |					|
 * -------------------------------------- vmStackEndAddr and vmMapBaseAddr
 * |					|
 * | Place to map pages into the	|
 * | kernel's VAS.  There are at most	|
 * | vmNumMappedPages mapped at once.	|
 * |					|
 * -------------------------------------- vmMapEndAddr
 * |					|
 * | Machine dependent part of VAS	|
 * |					|
 * -------------------------------------- vmBlockCacheBaseAddr
 * |					|
 * | File system block cache pages.	|
 * |					|
 * -------------------------------------- vmBlockCacheEndAddr and mach_KernEnd
 *
 *
 * USER VIRTUAL ADDRESS SPACE
 *
 * A users virtual address space is divided into three segments:
 * code, heap and stack.  The code is in the lowest part of the
 * VAS and is followed by the heap segment which grows towards the stack.
 * The stack is at the top of the virtual address and grows towards the
 * heap.  The two fields offset and numPages in each segment table entry
 * define the bounds of a segment.  offset is the virtual page number that
 * maps to page table entry zero.  Thus the index into the page table for any
 * page is the page number minus the offset.  The offset for the code and
 * heap segment is fixed and the offset for the stack segment will change
 * as the page table grows.  numPages is the number of pages that can be
 * accessed for a segment.  Thus for code and heap segments, offset is the
 * lowest accessible page and (numPages + offset - 1) is the highest
 * accessible page.  For a stack segment the highest accessible page is fixed
 * at mach_LastUserStackPage and the lowest accessible page is
 * (mach_LastUserStackPage - numPages + 1).
 *
 * The heap and stack grow in chunks that contain a multiple of
 * vmPageTableInc pages.  Thus the page table may contain more page table
 * entries then there are valid pages for a segment.  Two heap and stack
 * segments overlap if the page tables overlap.
 *
 * -------------------------------------- codeSeg.offset * vm_PageSize
 * |					|
 * | Code for the process.  There are	|
 * | codeSeg.numPages worth of pages	|
 * | in the segment.			|
 * |					|
 * -------------------------------------- heapSeg.offset * vm_PageSize
 * |					|
 * | Heap for the process.  There are	|
 * | heapSeg.numPages worth of virtual 	|
 * | pages in the segment.  However, The|
 * | actual end corresponds to the size |<- Last addr in heap segment =
 * | of the page table.			|   (heapSeg.offset + heapSeg.numPages)
 * |					|    			* vm_PageSize
 * |					|
 * -------------------------------------- (heapSeg.offset + heapSeg.ptSize) *
 *		    |			   			vm_PageSize
 *		    V			
 *
 *		    A
 *		    |
 * -------------------------------------- stackSeg.offset * vm_PageSize
 * |					|
 * | Stack for the process.  There are	|
 * | stackSeg.numPages worth of virtual |<- First addr in stack segment =
 * | pages for the segment.  However,   |   (mach_LastUserStackPage -
 * | like the heap segment the actual	|    stackSeg.numPages + 1) *
 * | size corresponds to the size of the|	vm_PageSize
 * | page table.			|
 * |					|
 * -------------------------------------- mach_MaxUserStackAddr
 *
 * SYNCHRONIZATION
 *
 * There are four types of synchronization in virtual memory:
 *
 *	1) The monitor lock.
 *	2) Per virtual page lock.
 *	3) Per page table entry lock.
 *	4) Reader and writer locks on a page table.
 *	5) Copy-on-write lock.
 *
 * The monitor lock is used when updating and accessing internal virtual
 * memory structures such as the core map and the segment table.  It is
 * also used to synchronize access to page table entries as will be explained
 * below.
 *
 * Each page managed by VM has a lock count on the page.  As long as the lock
 * count is greater than zero the page will not be stolen from its owner.
 * This is used when a;  page needs to be wired down in memory.
 *
 * Each page table entry has a page-fault-in-progress bit which is set whenever
 * a page is being faulted in.  Whenever this bit is set all other page faults
 * will wait until the fault completes.
 *
 * The page tables have two levels of locking.  First there is a count of
 * the number of users of a page table.  There can be multiple users of
 * the page table at once.  When the user count is greater than zero,
 * the page tables are guaranteed not to be expanded.  Thus while the count
 * is greater than zero a pointer to the page table is guaranteed to be
 * good; that is, noone is going to move the page table.  The second level
 * of locking is an exclusive lock.  When this lock is grabbed there can
 * only be one user of the page table.  This lock cannot be grabbed as long
 * as the user count is greater than zero.  Once this lock is grabbed the
 * page tables can be expanded, moved around or whatever.  The first lock
 * is used when handling page faults or forking segments - operations that
 * require access to the page table outside of the monitor lock.  The
 * second level lock is used when adding or deleting virtual addresses
 * from a segment - operations that require the page table to be reallocated
 * and copied, some of which must be done outside of the monitor lock.
 *
 * The page table locking is only used for heap segments.  Code segments don't
 * need to do the locking because they never expand.  Stack segments don't
 * need it because they can't be shared so the calling process doesn't have to
 * worry about someone else mucking with its page tables.
 *
 * Actual updating of page table entries must be done inside of the monitor.
 * This is because the page allocation code may decide at any time to steal
 * a page from a segment and it does not pay attention to either of the two
 * levels of locking for page tables.  Thus although most parts of a page
 * table entry can be changed at non-monitor level, the resident bit must
 * be examined inside of the monitor.  Also copying and expanding of page
 * tables must also be done inside of the monitor.
 *
 * The last form of synchronization is for copy-on-write.  See the file
 * vmCOW.c for details.
 */


/*
 * Value returned for a page frame when none are available.
 */
#define VM_NO_MEM_VAL	0x7fffffff

extern	int	vmFirstFreePage;	/* The first page frame that is not
					 * owned by the kernel. */
extern	Boolean	vmNoBootAlloc;		/* TRUE implies can no longer use
					 * Vm_BootAlloc. */
extern	Fs_Stream *vmSwapStreamPtr;	/* Swap directory stream. */
extern	int	vmPageShift;		/* Log base 2 of vm_PageSize. */
extern	int	vmPageTableInc;		/* The size in which page tables can
					 * grow. */
extern	Address	vmMemEnd;		/* Current end of kernel code + data. */
extern	int	vmKernMemSize;		/* Amount of code + data available for
					 * the kernel. */
extern	int	vmMaxProcesses;		/* The maximum number of processes that
					 * are supported by virtual memory. */
extern	int	vmNumMappedPages;	/* The maximum number of pages that
					 * can be mapped in by the kernel at
					 * one time. */
extern	Address	vmStackBaseAddr;	/* Base of where kernel stacks are. */
extern	Address	vmStackEndAddr;		/* End of where kernel stacks are. */
extern	Address	vmMapBaseAddr;		/* Base of where to map pages. */
extern	Address	vmMapEndAddr;		/* End of where to map pages. */
extern	int	vmMapBasePage;		/* First page to use for mapping. */
extern	int	vmMapEndPage;		/* Last page to use for mapping. */
extern	Address	vmBlockCacheBaseAddr;	/* Base of the file system cache. */
extern	Address	vmBlockCacheEndAddr;	/* End of the file system cache. */
extern	int	vmMaxMachSegs;		/* Maximum number of machine segments
					 * that the hardware will allow. */
extern	Boolean	vmFreeWhenClean;	/* TRUE if pages should be freed after
					 * they have been cleaned. */
extern	Boolean	vmAlwaysRefuse;		/* TRUE if VM should always refuse the
					 * file systems requests for memory. */
extern	Boolean	vmAlwaysSayYes;		/* TRUE if VM should always satisfy
					 * file system requests for memory. */
extern	int	vmMaxDirtyPages;	/* Maximum number of dirty pages
					 * before waiting for a page to be
					 * cleaned. */
extern	int		vmPagesToCheck;	/* Number of pages to check each time
					 * that the clock is run. */
extern	unsigned int	vmClockSleep;	/* Number of seconds to sleep between
					 * iterations of the clock. */
#define VM_MAX_PAGE_OUT_PROCS	3
extern	int		vmMaxPageOutProcs; /* Maximum number of page out procs
					    * at any given time. */
extern	Boolean		vmCORReadOnly;	/* After a cor fault the page is marked
					 * as read only so that it can be
					 * determined if it gets modified. */
extern	Boolean		vmPrefetch;	/* Whether to do prefetch or not. */
extern	Boolean		vmUseFSReadAhead;/* Should have FS do read ahead on
					  * object files. */

/*
 * Variables to control negotiations between the file system and the virtual
 * memory system.  Each time that FS asks for a page its reference time is
 * penalized depending on how many pages that it has allocated to it.  The
 * penalty is enforced by subtracting vmCurPenalty seconds from its access time
 * or adding vmCurPenalty to the VM access time.  This is done in the
 * following way.  Let vmPagesPerGroup = total-available-pages /
 * vmNumPageGroups,  vmCurPenalty = 0 and vmBoundary = vmPagesPerGroup.
 * Whenever the number of pages allocated to FS exceeds vmBoundary, vmBoundary
 * is incremented by vmPagesPerGroup and vmCurPenalty is incremented by
 * vmFSPenalty.  Whenever the number of pages allocated to FS goes under
 * vmBoundary, vmBoundary is decremented by vmPagesPerGroup and vmCurPenalty is
 * decremented by vmFSPenalty.
 */
extern	int	vmFSPenalty;	/* Number of seconds FS is penalized when it
				 * asks for page. */
extern	int	vmNumPageGroups;/* The number of groups to divide memory up
				 * into. */
extern	int	vmPagesPerGroup;/* The number of pages in each group. */
extern	int	vmCurPenalty;	/* The number of seconds that FS is currently
				 * penalized by. */
extern	int	vmBoundary;	/* The current number of pages that must be
				 * exceeded or gone under before changing the
				 * penalty. */

/*
 * Variables to control use of modify and reference bits.
 */
extern	Boolean	vmWriteablePageout;	/* Page out all pages that are
					 * writeable before recycling them
					 * whether they have been modified
					 * or not. */
extern	Boolean	vmWriteableRefPageout;	/* Page out all pages that have been
					 * referenced and are writeable
					 * before recycling them whether they
					 * have been modified or not. */

/*
 * Flags for VmPageAllocate and VmPageAllocateInt:
 *
 *  VM_CAN_BLOCK	Can block if no clean memory is available.
 *  VM_ABORT_WHEN_DIRTY	Abrot even if VM_CAN_BLOCK is set if have exceeded
 *			the maximum number of dirty pages on the dirty list.
 */
#define	VM_CAN_BLOCK		0x1
#define	VM_ABORT_WHEN_DIRTY	0x2


/*---------------------------------------------------------------------------*/

/*
 *                     Segment Table Structure
 *
 * There is one segment table for the entire system that contains
 * one entry for each segment. Associated with each segment table entry
 * is a reference count of processes that are using the segment.  If
 * the reference count is non-zero then the segment is actively being used.
 * If the reference count is zero then the segment table entry is either in the
 * inactive segment list or the free segment list.  The inactive segment list
 * is a list of segment table entries that are not currently in use by
 * any process but contain code segments that can be reused if a new process
 * needs the code segment.  The free segment list is a list of segment
 * table entries that are not being used by any process and do not contain code
 * segments that could be used by future processes.
 *
 * All processes that are sharing segments are linked together.  This is
 * done by having each segment table entry contain a pointer to a list of
 * pointers to proc table entries.
 *
 * See vmSeg.c for the actual use of the segment table and the free and
 * inactive lists.
 */

/*
 * An element of a linked list of processes sharing a segment.  Each
 * element of the linked list points to the proc table entry of the process
 * that is sharing the segment.
 */
typedef struct {
    List_Links	links;
    Proc_ControlBlock	*procPtr;
} VmProcLink;

/*
 * Memory space that has to be allocated for each segment.
 */
typedef	struct {
    Boolean		spaceToFree;	/* TRUE if this structure contains
					   space that has to be deallocated.*/
    Vm_PTE		*ptPtr;		/* Pointer to page table */
    int			ptSize;		/* Size of page table. */
    VmProcLink		*procLinkPtr;	/* Pointer to proc list element. */
} VmSpace;

/*
 * VmSegmentDeleteInt returns different status values depending on the
 * reference count and segment type.  These status values indicate what
 * should be done with the segment after the procedure returns.
 *
 *	VM_DELETE_SEG -		The segment should be deleted.
 *	VM_CLOSE_OBJ_FILE -	Don't delete the segment, but close the file
 *				containing the code for the segment.
 *	VM_DELETE_NOTHING -	Don't do anything.
 */
typedef enum {
    VM_DELETE_SEG,
    VM_CLOSE_OBJ_FILE,
    VM_DELETE_NOTHING,
} VmDeleteStatus;

/*
 * System segment number.
 */
#define VM_SYSTEM_SEGMENT       0

/*
 * Segment table flags:
 *
 *   VM_SEG_FREE		The segment is currently on the free list.
 *   VM_SEG_INACTIVE		The segment is currently on the inactive
 *				list.
 *   VM_SWAP_FILE_OPENED	A swap file has been opened for this segment.
 *   VM_SWAP_FILE_LOCKED	The swap file for this segment is being
 *				opened or written to.
 *   VM_SEG_DEAD		This segment is in the process of being
 *				deleted.
 *   VM_PT_EXCL_ACC		Someone has grabbed exclusive access to the
 *				the page tables.
 *   VM_DEBUGGED_SEG		This is a special code segment that is being
 *				written by the debugger.
 *   VM_SEG_CREATE_TRACED	The segment creation has been traced already.
 *   VM_SEG_CANT_COW		This segment cannot be forked copy-on-write.
 *   VM_SEG_COW_IN_PROGRESS	This segment is being actively copied at
 *				fork time.
 *   VM_SEG_IO_ERROR		An I/O error has occurred while paging to
 *				or from this segment.
 */

#define	VM_SEG_FREE			0x001
#define	VM_SEG_INACTIVE			0x002
#define	VM_SWAP_FILE_OPENED		0x004
#define	VM_SWAP_FILE_LOCKED		0x008
#define	VM_SEG_DEAD			0x010
#define	VM_PT_EXCL_ACC			0x020
#define	VM_DEBUGGED_SEG			0x040
#define	VM_SEG_CREATE_TRACED		0x080
#define	VM_SEG_CANT_COW			0x100
#define	VM_SEG_COW_IN_PROGRESS		0x200
#define VM_SEG_IO_ERROR		        0x400


/*---------------------------------------------------------------------------*/

/*
 * 			Core map structure.
 *
 * The core map contains one entry for each page in physical memory.
 * There are four lists that run through the core map: the allocate list,
 * the dirty list, the free list and the reserve list.  All pages that aren't
 * be used by any segment are on the free list.  All pages that are being
 * used by users processes are on the allocate list or the dirty list.
 * The allocate list is used to keep track of which pages are the best
 * candidates to use when a new page is needed.  All pages that are not
 * attached to any segment are at the front of the allocate list and the
 * rest of the pages on the allocate list are kept in LRU order.  The dirty
 * list is a list of pages that are being written to disk.  The reserve
 * list is a list of pages that are kept in case the kernel needs memory
 * and no clean pages are available.
 *
 * See vmPage.c for the actual use of the core map and the lists.
 * allocate lists.
 */

typedef struct VmCore {
    List_Links	links;		/* Links for allocate, free, dirty and reserver
				 * lists */
    Vm_VirtAddr	virtPage;	/* The virtual page information for this page */
    int		wireCount;	/* The number of times that the page has bee
				 * wired down by users. */
    int		lockCount;	/* The number of times that this page has been
				   locked down (i.e. made unpageable). */
    int 	flags;		/* Flags that indicate the state of the page
				   as defined below. */
    int		lastRef;	/* Time in seconds that pages reference bit
				 * last cleared by clock. */
} VmCore;

/*
 * The following defines the state of the page:
 *
 * VM_FREE_PAGE    		The page is not attached to any segment.
 * VM_DIRTY_PAGE   		The page is on the dirty list.
 * VM_SEG_PAGEOUT_WAIT		A segment is waiting for this page to be
 *				cleaned.
 * VM_PAGE_BEING_CLEANED	The page is actually being cleaned.
 * VM_DONT_FREE_UNTIL_CLEAN	This page cannot be freed until it has
 *				been written out.
 */
#define VM_FREE_PAGE 			0x01
#define VM_DIRTY_PAGE 			0x02
#define VM_SEG_PAGEOUT_WAIT 		0x04
#define VM_PAGE_BEING_CLEANED		0x08
#define	VM_DONT_FREE_UNTIL_CLEAN	0x10

/*
 * Copy-on-write info struct.
 */
typedef struct VmCOWInfo {
    List_Links		cowList;
    int			numSegs;
    Sync_Condition	condition;
    Boolean		copyInProgress;
} VmCOWInfo;

/*
 * Shared memory.
 */
extern int vmShmDebug;
/*
 * Debugging printf.
 */
#ifdef lint
#define dprintf printf
#else
#define dprintf if (vmShmDebug) printf
#endif

/*
 * Macros to get a pointer to a page table entry.
 */
#ifdef CLEAN2
#define	VmGetPTEPtr(segPtr, page) \
    (&((segPtr)->ptPtr[(page) - (segPtr)->offset]))
#else /* CLEAN */
#define	VmGetPTEPtr(segPtr, page) \
    (((((page) - (segPtr)->offset) > (segPtr)->ptSize)) ? \
	panic("Page number outside bounds of page table"), (Vm_PTE *) NIL : \
	(&((segPtr)->ptPtr[(page) - (segPtr)->offset])))
#endif /* CLEAN */

#ifdef CLEAN
#define	VmGetAddrPTEPtr(virtAddrPtr, page) \
    (&((virtAddrPtr)->segPtr->ptPtr[(page) - segOffset(virtAddrPtr)]))
#else /* CLEAN */
#define	VmGetAddrPTEPtr(virtAddrPtr, page) \
    (((((page) - segOffset(virtAddrPtr)) < 0) || \
    (((page) - segOffset(virtAddrPtr)) > (virtAddrPtr)->segPtr->ptSize) ) ? \
	panic("Page number outside bounds of page table"), (Vm_PTE *) NIL : \
	(&((virtAddrPtr)->segPtr->ptPtr[(page) - segOffset(virtAddrPtr)])))
#endif /* CLEAN */

/*
 * Macro to increment a page table pointer.
 */
#define	VmIncPTEPtr(ptePtr, val) ((ptePtr) += val)

/*
 * Macro to get a virtAddr's offset in the page table.
 */
#define segOffset(virtAddrPtr) (( (virtAddrPtr)->sharedPtr== \
	(Vm_SegProcList *)NIL) ? (virtAddrPtr)->segPtr->offset :\
	   (virtAddrPtr)->sharedPtr->offset)

/*----------------------------------------------------------------------------*/

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

/*
 * Initialization routines.
 */
extern	void		VmSegTableAlloc();
extern	void		VmSegTableInit();
extern	void		VmSwapFileInit();
extern	void		VmStackInit();
extern	void		VmCoreMapAlloc();
extern	void		VmCoreMapInit();
/*
 * Page allocation routines.
 */
extern	unsigned int	VmPageAllocate();
extern	unsigned int	VmPageAllocateInt();
extern	unsigned int	VmGetReservePage();
/*
 * Routine to free pags.
 */
extern	void		VmPageFree();
extern	void		VmPageFreeInt();
/*
 * Routines to put pages on lists.
 */
extern	void		VmPutOnFreeSegList();
extern	void		VmPutOnFreePageList();
/*
 * Routines to lock pages.
 */
extern	void		VmLockPageInt();
extern	void		VmUnlockPage();
extern	void		VmUnlockPageInt();
/*
 * Routine to see if a page is pinned down.
 */
extern	Boolean		VmPagePinned();
/*
 * Routine to handle page faults.
 */
extern	ReturnStatus	VmDoPageIn();
extern	void		VmVirtAddrParse();
extern	Boolean		VmCheckBounds();
extern	void		VmZeroPage();
extern	void		VmKillSharers();
/*
 * Segment handling routines.
 */
extern	ReturnStatus 	VmAddToSeg();
extern  VmDeleteStatus 	VmSegmentDeleteInt();
extern	void		VmDecPTUserCount();
extern	Vm_Segment	*VmGetSegPtr();
extern	void		VmFlushSegment();
extern	Vm_SegProcList	*VmFindSharedSegment();
extern	Boolean		VmCheckSharedSegment();
/*
 * Routines to validate and invalidate pages.
 */
extern	void		VmPageValidate();
extern	void		VmPageValidateInt();
extern	void		VmPageInvalidate();
extern	void		VmPageInvalidateInt();
extern	void		VmValidatePagesInt();
/*
 * VM list routines.  Like normal list routines but do more sanity checks.
 */
extern	void		VmListMove();
extern	void		VmListRemove();
extern	void		VmListInsert();
/*
 * Routines for copy-on-write and copy-on-reference.
 */
extern	Boolean		VmSegCanCOW();
extern	void		VmSegCantCOW();
extern	void		VmSegCOWDone();
extern	void		VmSegFork();
extern	void		VmCOWDeleteFromSeg();
extern	ReturnStatus	VmCOR();
extern	void		VmCOW();
extern	void		VmPageSwitch();
/*
 * Procedures for remote page access.
 */
extern	ReturnStatus	VmCopySwapSpace();
extern	ReturnStatus	VmPageServerRead();
extern	ReturnStatus	VmPageServerWrite();
extern	ReturnStatus	VmFileServerRead();
extern	void 		VmMakeSwapName();
extern	ReturnStatus	VmOpenSwapFile();
extern	ReturnStatus	VmCopySwapPage();
extern	void 		VmSwapFileLock();
extern	void 		VmSwapFileUnlock();
/*
 * Procedures for process migration.
 */
extern	ReturnStatus	VmOpenSwapByName();
extern  ENTRY void	VmPutOnDirtyList();
/*
 * Procedures for mapping.
 */
extern	Address		VmMapPage();
extern	void		VmUnmapPage();
extern	void		VmRemapPage();
/*
 * Prefetch routine.
 */
extern	void		VmPrefetch();
/*
 * Vm tracing.
 */
extern	void		VmTraceSegStart();
extern	void		VmCheckListIntegrity();
#endif _VMINT
