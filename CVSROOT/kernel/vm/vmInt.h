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

#include "vmMachInt.h"
#include "fs.h"
#include "list.h"
#include "sync.h"
#include "proc.h"
#include "status.h"

/*
 * The number of the first page frame in physical memory that is not owned 
 * by the kernel.
 */

extern	int	vmFirstFreePage;

/*
 * The pointer to memory allocated at boot time.
 */

extern	Address	vmMemEnd;

/*
 * Flag that is set once we can no longer allocate memory using the boot
 * allocation routine.
 */

extern	Boolean	vmNoBootAlloc;

/*
 * Structure to represent a translated virtual address
 */

typedef struct {
    struct	Vm_Segment	*segPtr;
    int 			page;
    int 			offset;
} VmVirtAddr;

/*
 * Structure that contains file information that needs to be freed.
 */

typedef struct {
    Fs_Stream	*objStreamPtr;
    Fs_Stream	*swapStreamPtr;
    char	*swapFileName;
    int		segNum;
} VmFileInfo;


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
 * The two fields offset and numPages in each segment table entry define the 
 * bounds of a segment.  offset is the virtual page number that maps to 
 * page table entry zero.  Thus the index into the page table for any 
 * page is the page number minus the offset.  The offset for the code and 
 * heap segment is fixed and the offset for the stack segment will change 
 * as the page table grows.  
 *
 * numPages is the number of pages that can be accessed for a segment.  Thus
 * for code and heap segments, offset is the lowest accessible page and 
 * (numPages + offset - 1) is the highest accessible page.  For a stack segment
 * the highest accessible page is fixed at MACH_LAST_USER_STACK_PAGE and 
 * the lowest accessible page is (MACH_LAST_USER_STACK_PAGE - numPages + 1).
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
    VmMachData		*machData;	/* Pointer to machine dependent data */
    VmProcLink		*procLinkPtr;	/* Pointer to proc list element. */
} VmSpace;

/*
 * VmSegmentDeleteInt returns different status values depending on the
 * reference count and segment type.  These status values indicate what
 * should be done with the segment after the procedure returns.
 *
 *	DELETE_SEG	-	The segment should be deleted.
 *	CLOSE_OBJ_FILE	-	Don't delete the segment, but close the file
 *				containing the code for the segment.
 *	DELETE_NOTHING	-	Don't do anything.
 */

typedef enum {
    VM_DELETE_SEG,
    VM_CLOSE_OBJ_FILE,
    VM_DELETE_NOTHING,
} VmDeleteStatus;

/*
 * Pointer to system segment.
 */

extern 	Vm_Segment	*vmSysSegPtr;

/*
 * Segment table flags:
 *
 *   VM_SEG_FREE		The segment is currently on the free list.
 *   VM_DONT_EXPAND		This segment cannot be expanded right now.
 *   VM_SEG_INACTIVE		The segment is currently on the inactive
 *				list.
 *   VM_OLD_CODE_SEG		This code segment corresponds to an object file
 *				that has been modified.
 *   VM_SWAP_FILE_OPENED	A swap file has been opened for this segment.
 *   VM_SWAP_FILE_LOCKED	The swap file for this segment is being
 *				opened or written to.
 *   VM_SEG_DEAD		This segment is in the process of being
 *				deleted.
 */

#define	VM_SEG_FREE			0x01
#define	VM_DONT_EXPAND			0x02
#define	VM_SEG_INACTIVE			0x04
#define	VM_OLD_CODE_SEG			0x08
#define	VM_SWAP_FILE_OPENED		0x10
#define	VM_SWAP_FILE_LOCKED		0x20
#define	VM_SEG_DEAD			0x40


/*---------------------------------------------------------------------------*/

/*
 * 			Core map structure.  
 *
 * The core map contains one entry for each page in physical memory.
 * There are two list that run through the core map: the allocate list 
 * and the dirty list.  All pages, except those that are on the dirty list, 
 * or are owned by the kernel are on the allocate list.  The allocate
 * list is used to keep track of which pages are the best candidates to
 * use when a new page is needed.  All pages that are not attached to
 * any segment are at the front of the allocate list and the rest of the
 * pages on the allocate list are kept in LRU order.  The dirty list is
 * a list of pages that are being written to disk.  
 *
 * See vmPage.c for the actual use of the core map and the dirty and
 * allocate lists.
 */

typedef struct VmCore {
    List_Links	links;		/* Links for allocate and dirty lists */

    VmVirtAddr	virtPage;	/* The virtual page information for this page */

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


/*----------------------------------------------------------------------------*/

/* 
 * Initialization routines.
 */

extern	void	VmSegTableAlloc();
extern	void	VmSegTableInit();
extern	void	VmSwapFileInit();
extern	void	VmCoreMapAlloc();
extern	void	VmCoreMapInit();
/*
 * Internal virtual memory procedures.
 */

extern	int		VmPageAllocate();
extern	int		VmPageAllocateInt();
extern	int		VmGetReservePage();
extern	void		VmPageFree();
extern	void		VmPutOnFreeSegList();
extern	void		VmPutOnFreePageList();
extern	void		VmPageFreeInt();
extern	void		VmLockPageInt();
extern	void		VmUnlockPage();
extern	ReturnStatus	VmDoPageIn();
extern	void		VmDecExpandCount();
extern	ReturnStatus 	VmAddToSeg();
extern	void 		VmDeleteFromSeg();
extern	void 		VmCleanSeg();
extern	void 		VmMakeSwapName();
extern	ReturnStatus	VmOpenSwapFile();
extern	void 		VmSwapFileLock();
extern	void 		VmSwapFileUnlock();
extern  VmDeleteStatus 	VmSegmentDeleteInt();

/*
 * Procedures for remote page access.
 */

extern	ReturnStatus	VmCopySwapSpace();
extern	ReturnStatus	VmPageServerRead();
extern	ReturnStatus	VmPageServerWrite();
extern	ReturnStatus	VmFileServerRead();

/*
 * Procedures for process migration.
 */

extern	ReturnStatus	VmOpenSwapByName();
extern  ENTRY void	VmPutOnDirtyList();

#endif _VMINT
