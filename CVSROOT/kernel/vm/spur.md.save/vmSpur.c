/* vmSpur.c --
 *
 *     	This file contains all hardware dependent routines for SPUR.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vmMach.h"
#include "vmMachInt.h"
#include "vm.h"
#include "vmInt.h"
#include "user/vm.h"
#include "user/spur.md/vmMach.h"
#include "machConfig.h"
#include "mach.h"
#include "list.h"
#include "stdlib.h"
#include "proc.h"
#include "sched.h"
#include "sync.h"
#include "sys.h"
#include "dbg.h"
/*
 * Number of page frames in system.
 */
static int	NumPageFrames;

/*
 * Structure for mapping virtual page frame numbers to physical page frame
 * numbers and back for each memory board.
 */
typedef struct {
    unsigned int	endVirPfNum;	/* Ending virtual page frame number
					 * on board. */
    unsigned int	startVirPfNum;  /* Starting virtual page frame number
					 * on board. */
    unsigned int	physStartAddr;	/* Physical address of page frame. */
    unsigned int	physEndAddr;	/* End Physical address of page frame.*/
} Memory_Board;

/*
 * Pointer to last configured Mboard structure.
 */
static Memory_Board   *LastMboard;
/*
 * Memory_Board structures for each board in system. This array is sorted
 * by endVirPfNum.
 */
static Memory_Board Mboards[16];
/*
 * Macros to translate from a virtual page to a physical page and back.
 */

#define	VirtToPhysPage(pfNum) ({\
	register Memory_Board 	*mb; \
	for (mb = Mboards; mb < LastMboard; mb++) { \
	    if (pfNum < mb->endVirPfNum) { \
		break; \
	    } \
	} \
	(mb->physStartAddr + pfNum - mb->startVirPfNum); })

#define	PhysToVirtPage(pfNum) ({\
	register Memory_Board 	*mb; \
	for (mb = Mboards; mb < LastMboard; mb++) { \
	    if (pfNum >= mb->physStartAddr && \
		pfNum < mb->physEndAddr) { \
		break; \
	    } \
	} \
	(pfNum - mb->physStartAddr + mb->startVirPfNum); })

/*
 * Macro to go from a virtual page number within a segment to the page
 * table entry for the page.
 */
#define	GetPageTablePtr(segDataPtr, virtPage) \
			    (segDataPtr->ptBasePtr + virtPage)

/*
 * Macro to go from a virtual page number within a segment page table to 
 * the page table entry which maps the page table.
 */
#define Get2ndPageTablePtr(segDataPtr, virtPage) \
			    (segDataPtr->pt2BasePtr + virtPage)

int	vmMachKernMemSize = 4096 * 1024;

VmMachPTE	*basePTPtr;	/* Pointer to base of the segment page
				 * tables. */
VmMachPTE	*kernPTPtr;	/* Pointer to kernel's page table. */
VmMachPTE	*kernPT2Ptr;	/* Pointer to page table that maps the kernel's
				 * page table. */
VmMachPTE	*kernPT3Ptr;	/* Pointer to page table that maps the page
				 * table that maps the kernel's page table. */
static int	rootPTPageNum;	/* The first physical page behind the root
				 * page table. */

/*
 * The kernel segments machine dependent data.
 */
VmMach_SegData	kernSegData;

/*
 * Machine dependent flags for the flags field in the Vm_VirtAddr struct.
 * We are only allowed to use the second byte of the flags.
 *
 *	USING_MAPPED_SEG		The parsed virtual address falls into
 *					the mapping segment.
 */
#define	USING_MAPPED_SEG	0x100

/*
 * Variable to tell whether have initialized VM or not.
 */
static Boolean initialized = FALSE;

/*
 * Variable to control whether we should use the simple or complex validation
 * of virtual addresses.  The simple one will only allow us to look at the
 * system segment.
 */
static Boolean useSimpleValidation = FALSE;

#ifdef lint
#ifndef volatile
#define volatile
#endif
#endif

/*
 * Variable to control reference bit usage.
 */
static Boolean flushOnRefBitClear = FALSE;
static Boolean useHardRefBit = TRUE;

/*
 * Variable to control coherency bit usage in page table entries.  If
 * TRUE all stack and heap segments have the VMMACH_COHERENCY_BIT bit set
 * and all reads become read for ownership.
 */
static Boolean ownStackAndHeap = TRUE;


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_BootInit --
 *
 *      Do hardware dependent boot time initialization.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Pointers to kernel page tables set up.  Also the various size
 * 	fields are filled in.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_BootInit(pageSizePtr, pageShiftPtr, pageTableIncPtr, kernMemSizePtr,
		numKernPagesPtr, maxSegsPtr, maxProcessesPtr)
    int	*pageSizePtr;
    int	*pageShiftPtr;
    int	*pageTableIncPtr;
    int	*kernMemSizePtr;
    int	*numKernPagesPtr;
    int	*maxSegsPtr;
    int *maxProcessesPtr;
{
    ReturnStatus	status;
    int			boardNum;
    Mach_Board		board;
    int			nextVframeNum, numFrames;
    /*
     * Initailize the memory boards.
     */
    LastMboard = Mboards;
    nextVframeNum = 0;
    for (boardNum = 0; boardNum < 16; boardNum++) {
	status = Mach_FindBoardDescription(MACH_CONFIG_MEMORY_MASK, boardNum,
					   TRUE, &board);
	if (status != SUCCESS) {
		break;
	} 
	numFrames = Mach_ConfigMemSize(board) / VMMACH_PAGE_SIZE;
	LastMboard->startVirPfNum = nextVframeNum;
	LastMboard->endVirPfNum = nextVframeNum + numFrames;
	nextVframeNum += numFrames;
	LastMboard->physStartAddr = (Mach_ConfigInitMem(board) >> 
						VMMACH_PAGE_SHIFT);
	LastMboard->physEndAddr = LastMboard->physStartAddr + numFrames;
	LastMboard++;
    }
    if (LastMboard == Mboards) {
	panic("No memory boards in system configuration.");
    }
    NumPageFrames = nextVframeNum;
    basePTPtr = (VmMachPTE *)VMMACH_KERN_PT_BASE;
    kernPTPtr = basePTPtr;
    kernPT2Ptr = (VmMachPTE *)VMMACH_KERN_PT2_BASE;
    kernPT3Ptr = (VmMachPTE *)((Address)kernPT2Ptr + VMMACH_SEG_PT2_SIZE / 4 * VMMACH_KERN_PT_QUAD);
    /*
     * Return lots of sizes to the machine independent module who called us.
     */
    *pageSizePtr = VMMACH_PAGE_SIZE;
    *pageShiftPtr = VMMACH_PAGE_SHIFT;
    *pageTableIncPtr = VMMACH_PAGE_TABLE_INCREMENT;
    *kernMemSizePtr = vmMachKernMemSize;
    *maxProcessesPtr = 128;
    *numKernPagesPtr = GetNumPages();
    /*
     * The number of useable segments by our caller is the maximum minus 1
     * because the last segment is intentionally left invalid to catch 
     * kernel errors.
     */
    *maxSegsPtr = VMMACH_NUM_SEGMENTS - 1;
}


/*
 * ----------------------------------------------------------------------------
 *
 * GetNumPages --
 *
 *     Determine how many pages of physical memory there are.
 *
 * Results:
 *     The number of physical pages.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
int
GetNumPages()
{
    return(NumPageFrames);
}


/*
 * ---------------------------------------------------------------------------
 *
 * VmMach_AllocKernSpace --
 *
 *     Allocate memory for machine dependent stuff in the kernels VAS.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ---------------------------------------------------------------------------
 */
Address
VmMach_AllocKernSpace(baseAddr)
    Address	baseAddr;
{
    return(baseAddr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_Init --
 *
 *     Initialize all virtual memory data structures.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Kernel memory set up.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_Init(firstFreePage)
    int	firstFreePage;	/* Virtual page that is the first free for the 
			 * kernel. */
{
    VmMachPTE		*ptePtr;
    int 		i;
    int			pfNum;
    VmMach_SegData	*machPtr;
    extern	int	etext;
    int			firstCodePage;
    int			lastCodePage;

    /*
     * Unmap pages in low memory that aren't supposed to be accessible:
     *
     *	1) Page 0.
     *	2) The top of the main programs saved window stack.
     *	3) The top of the debuggers saved window stack.
     */
    ptePtr = basePTPtr;
    *ptePtr = 0;
    ptePtr = basePTPtr + ((unsigned)mach_StackBottom >> VMMACH_PAGE_SHIFT) + 1;
    *ptePtr = 0;
    ptePtr = basePTPtr + (MACH_DEBUG_STACK_BOTTOM >> VMMACH_PAGE_SHIFT) + 1;
    *ptePtr = 0;

    machPtr = &kernSegData;
    vm_SysSegPtr->machPtr = machPtr;
    machPtr->ptBasePtr = kernPTPtr;
    machPtr->pt2BasePtr = kernPT2Ptr;
    machPtr->firstPTPage = 0;
    machPtr->lastPTPage  = (((unsigned int)mach_KernEnd - 1) >> 
			   (VMMACH_PAGE_SHIFT + VMMACH_SEG_PT2_SHIFT));

    /*
     * Make sure that the page table that maps the page table is resident.
     */
    rootPTPageNum = -1;
    for (ptePtr = kernPT3Ptr, i = 0;
         i < VMMACH_NUM_PT3_PAGES;
	 ptePtr++, i++) {
	pfNum = GetPageFrame(*ptePtr);
	if (rootPTPageNum == -1) {
	    rootPTPageNum = pfNum;
	    machPtr->RPTPM = rootPTPageNum;
	}
	Vm_ReservePage((unsigned int)PhysToVirtPage(pfNum));
    }

    /*
     * Map enough of the kernel page tables to cover the page tables for
     * the maximum amount of the kernel VAS that can be in use.
     */
    for (ptePtr = kernPT2Ptr, i = 0;
	 i <= machPtr->lastPTPage;
	 ptePtr++, i++) {
	pfNum = GetPageFrame(*ptePtr);
	Vm_ReservePage((unsigned int)PhysToVirtPage(pfNum));
     }

    /*
     * Unmap the rest of the page table pages up to the root page table.
     */
    for (; ptePtr < kernPT3Ptr; ptePtr++, i++) {
	*ptePtr = (VmMachPTE)0;
    }
    ptePtr += VMMACH_NUM_PT3_PAGES;
    i += VMMACH_NUM_PT3_PAGES;
    /*
     * Unmap the rest of the page tables.
     */
    for (; i < VMMACH_NUM_PT_PAGES; ptePtr++, i++) {
	*ptePtr = (VmMachPTE)0;
    }
    bzero(((Address)kernPT2Ptr) + VMMACH_SEG_PT2_SIZE,
	    (VMMACH_NUM_SEGMENTS - 1) * VMMACH_SEG_PT2_SIZE);

    /*
     * Write protect the kernel code.
     */
    firstCodePage = MACH_CODE_START >> VMMACH_PAGE_SHIFT;
    lastCodePage = ((unsigned)&etext - VMMACH_PAGE_SIZE) >> VMMACH_PAGE_SHIFT;
    for (i = firstCodePage, ptePtr = kernPTPtr + firstCodePage;
	 i <= lastCodePage;
	 i++, ptePtr++) {
	VmMachPTE	pte;
	pte = *ptePtr; 
	pte &= ~VMMACH_PROTECTION_FIELD;
	pte |= VMMACH_KRO_UNA_PROT;
	*ptePtr = pte;
    }
         
    /*
     * Clear out the rest of kernel page tables starting with the first
     * free page.
     */
    for (ptePtr = kernPTPtr + firstFreePage, i = firstFreePage;
         i < (machPtr->lastPTPage + 1) * VMMACH_PTES_IN_PAGE; 
         ptePtr++, i++) {
	*ptePtr = (VmMachPTE) 0;
    }
    /*
     * Don't allow copy-on-write on SPUR.
     */
    vm_CanCOW = FALSE;
    /*
     * We have now initialized ourselves.
     */
    initialized = TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * The following routines deal with allocating, deleting and initializing
 * segments.  These are all synchronized by the caller in the machine
 * independent module.  It is up to it to ensure that only one operation
 * on a segment happens at a time.  We can't synchronize ourselves because
 * we have to call into the machine independent module to allocate and
 * free memory for the kernel page tables.
 *
 * ----------------------------------------------------------------------------
 */


/*
 * ----------------------------------------------------------------------------
 *
 * AllocPageTable --
 *
 *	Allocate kernel memory for a page table for the given segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The kernel's page table is modified to map the page table for the
 *	given segment.
 *
 * ----------------------------------------------------------------------------
 */
AllocPageTable(segPtr, offset, numPages)
    register	Vm_Segment	*segPtr;	/* Segment to alloc PT for. */
    unsigned int		offset;		/* Page offset within hardware
						 * segment. */
    int				numPages;	/* Number of pages in segment
						 * from offset. */
{
    register	VmMach_SegData	*segDataPtr;
    register	VmMachPTE	*ptePtr;
    register	int		firstPTPage;
    int				lastPTPage;
    Address			ptAddr;

    segDataPtr = segPtr->machPtr;
    /*
     * Determine first and last page table pages to validate.
     */
    firstPTPage = offset >> VMMACH_SEG_PT2_SHIFT;
    lastPTPage = (offset + numPages - 1) >> VMMACH_SEG_PT2_SHIFT;
    if (firstPTPage < segDataPtr->firstPTPage) {
	segDataPtr->firstPTPage = firstPTPage;
    }
    if (lastPTPage > segDataPtr->lastPTPage) {
	segDataPtr->lastPTPage = lastPTPage;
    }
    /*
     * Validate the appropriate page table pages.
     */
    ptAddr = (Address)(segDataPtr->ptBasePtr + 
			    (firstPTPage << VMMACH_SEG_PT2_SHIFT));
    for (ptePtr = Get2ndPageTablePtr(segDataPtr, firstPTPage);
	 firstPTPage <= lastPTPage;
	 firstPTPage++, ptePtr++, ptAddr += VMMACH_PAGE_SIZE) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT)) {
	    *ptePtr = VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | 
	              VMMACH_KRW_UNA_PROT | VMMACH_REFERENCED_BIT |
		      VMMACH_MODIFIED_BIT |
		      SetPageFrame(VirtToPhysPage(Vm_KernPageAllocate()));
	    bzero(ptAddr, VMMACH_PAGE_SIZE);
	}
     }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SegInit --
 *
 *      Initialize the hardware page table for the segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Machine dependent data struct is allocated and initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SegInit(segPtr)
    register	Vm_Segment	*segPtr;
{
    register	VmMach_SegData	*segDataPtr;
    int			        segOffset;
    int				numPages;

    segPtr->minAddr = (Address)(segPtr->type * VMMACH_SEG_SIZE);
    segPtr->maxAddr = (Address)(segPtr->minAddr + (VMMACH_SEG_SIZE - 1));
    if (segPtr->machPtr == (VmMach_SegData *) NIL) {
	segDataPtr = (VmMach_SegData *)malloc(sizeof(VmMach_SegData));
	segPtr->machPtr = segDataPtr;
	segDataPtr->ptBasePtr = 
		    basePTPtr + segPtr->segNum * (VMMACH_SEG_PT_SIZE / 4);
	segDataPtr->pt2BasePtr =
		    kernPT2Ptr + segPtr->segNum * (VMMACH_SEG_PT2_SIZE / 4); 
	segDataPtr->RPTPM = rootPTPageNum + segPtr->segNum / 4;
    } else {
	segDataPtr = segPtr->machPtr;
    }

    segDataPtr->firstPTPage = 0x7fffffff;
    segDataPtr->lastPTPage = -1;

    segOffset = segPtr->offset - segPtr->type * VMMACH_PAGES_PER_SEG;

    if (segOffset < 0) {
	/*
	 * HACK to allow us to work on SPUR when Proc_KernExec allocates
	 * dummy segments with ridiculous offsets.  The real patch
	 * is in Proc_KernExec not here.
	 */
	segOffset = 0;
    }
    if (segPtr->type == VM_STACK) {
	numPages = mach_LastUserStackPage - segPtr->offset + 1;
    } else {
	numPages = segPtr->numPages;
    }
    AllocPageTable(segPtr, (unsigned int)segOffset, numPages);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SegExpand --
 *
 *	Allocate more space for the machine dependent structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_SegExpand(segPtr, firstPage, lastPage)
    register	Vm_Segment	*segPtr;	/* Segment to expand. */
    int				firstPage;	/* First page to add. */
    int				lastPage;	/* Last page to add. */
{
    AllocPageTable(segPtr, 
	   (unsigned int)(firstPage - segPtr->type * VMMACH_PAGES_PER_SEG),
		   lastPage - firstPage + 1);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SegDelete --
 *
 *      Unmap the page table.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Page table and machine dependent struct for the segment is freed.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SegDelete(segPtr)
    Vm_Segment	*segPtr;    /* Pointer to segment to free. */
{
    register	VmMachPTE	*ptePtr;
    register	VmMach_SegData	*segDataPtr;
    register	int		firstPage;
    Vm_VirtAddr			virtAddr;

    segDataPtr = segPtr->machPtr;
    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = (unsigned int)(segDataPtr->ptBasePtr +
		       (segDataPtr->firstPTPage << VMMACH_SEG_PT2_SHIFT)) >>
			    VMMACH_PAGE_SHIFT;

    for (ptePtr = Get2ndPageTablePtr(segDataPtr, segDataPtr->firstPTPage),
	    firstPage = segDataPtr->firstPTPage;
	 firstPage <= segDataPtr->lastPTPage;
	 firstPage++, ptePtr++, virtAddr.page++) {
	if (*ptePtr & VMMACH_RESIDENT_BIT) {
	    /*
	     * The page table page was resident so free it and flush it out
	     * of the cache.
	     */
	    VmMach_FlushPage(&virtAddr, TRUE);
	    Vm_KernPageFree((unsigned)PhysToVirtPage(GetPageFrame(*ptePtr)));
	    *ptePtr = 0;
	}
     }
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_VirtAddrParse --
 *
 *	Determine which segment the virtual address falls into.
 *
 * Results:
 *	TRUE because can always parse the virtual address.
 *
 * Side effects:
 *	*transVirtAddrPtr is filled in.
 *
 *----------------------------------------------------------------------
 */
Boolean
VmMach_VirtAddrParse(procPtr, virtAddr, transVirtAddrPtr)
    Proc_ControlBlock		*procPtr;
    Address			virtAddr;
    register	Vm_VirtAddr	*transVirtAddrPtr;
{
    register	VmMach_ProcData	*procDataPtr;
    Address			origVirtAddr;

    procDataPtr = procPtr->vmPtr->machPtr;
    if (procDataPtr->segRegMask == (unsigned)-1) {
	return(FALSE);
    }
    if (((unsigned)virtAddr & VMMACH_SEG_REG_MASK) == 
					procDataPtr->mappedSegMask) {
	origVirtAddr = (Address) (((unsigned)virtAddr & ~VMMACH_SEG_REG_MASK) |
				  procDataPtr->segRegMask);
	transVirtAddrPtr->segPtr = procDataPtr->mapSegPtr;
	transVirtAddrPtr->page = (unsigned) (origVirtAddr) >> VMMACH_PAGE_SHIFT;
	transVirtAddrPtr->offset = (unsigned)virtAddr & VMMACH_OFFSET_MASK;
	transVirtAddrPtr->flags |= USING_MAPPED_SEG;
	return(TRUE);
    } else {
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * The following routines work on a per-process state that only be modified
 * by the currently executing process.  Thus no synchronization is required.
 *
 *----------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------
 *
 * VmMach_ProcInit --
 *
 *	Initalize the machine dependent part of the VM proc info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Machine dependent proc info is initialized.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_ProcInit(vmPtr)
    register	Vm_ProcInfo	*vmPtr;
{
    register	VmMach_ProcData	*machPtr;

    if (vmPtr->machPtr == (VmMach_ProcData *)NIL) {
	vmPtr->machPtr = (VmMach_ProcData *)malloc(sizeof(VmMach_ProcData));
    }
    machPtr = vmPtr->machPtr;
    machPtr->segNums[VM_SYSTEM] = VM_SYSTEM_SEGMENT;
    machPtr->segNums[VM_CODE] = -1;
    machPtr->segNums[VM_HEAP] = -1;
    machPtr->segNums[VM_STACK] = -1;
    machPtr->RPTPMs[VM_SYSTEM] = rootPTPageNum;
    machPtr->segRegMask = -1;
}


/*
 * ----------------------------------------------------------------------------
 *
 * SetupContext --
 *
 *      Setup context for the given process that is executing on the 
 *	current processor.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *      Segment register values are modified.
 *
 * ----------------------------------------------------------------------------
 */
void
SetupContext(procPtr) 
    Proc_ControlBlock	*procPtr;
{
    register	Vm_Segment	**segPtrArray;
    register	VmMach_ProcData	*machPtr;
    register	int		*segNums;
    register	int		*RPTPMs;

    machPtr = procPtr->vmPtr->machPtr;
    segNums = machPtr->segNums;
    RPTPMs = machPtr->RPTPMs;
    if (segNums[VM_CODE] == -1) {
	segPtrArray = procPtr->vmPtr->segPtrArray;
	/*
	 * The segment number array has not been initialized yet.
	 */
	if (segPtrArray[VM_CODE] == (Vm_Segment *)NIL) {
	    int	invRPTPM;

	    invRPTPM = VMMACH_INVALID_SEGMENT / 4 + rootPTPageNum;
	    segNums[VM_CODE] = VMMACH_INVALID_SEGMENT;
	    RPTPMs[VM_CODE] = invRPTPM;
	    segNums[VM_HEAP] = VMMACH_INVALID_SEGMENT;
	    RPTPMs[VM_HEAP] =  invRPTPM;
	    segNums[VM_STACK] = VMMACH_INVALID_SEGMENT;
	    RPTPMs[VM_STACK] = invRPTPM;
	} else {
	    segNums[VM_CODE] = segPtrArray[VM_CODE]->segNum;
	    RPTPMs[VM_CODE] = segPtrArray[VM_CODE]->machPtr->RPTPM;
	    segNums[VM_HEAP] = segPtrArray[VM_HEAP]->segNum;
	    RPTPMs[VM_HEAP] = segPtrArray[VM_HEAP]->machPtr->RPTPM;
	    segNums[VM_STACK] = segPtrArray[VM_STACK]->segNum;
	    RPTPMs[VM_STACK] = segPtrArray[VM_STACK]->machPtr->RPTPM;
	}
    }
    VmMachSetSegRegisters(segNums, RPTPMs);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SetupContext --
 *
 *      Setup context for the new process.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *      Context set up for the process.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
void
VmMach_SetupContext(destProcPtr)
    Proc_ControlBlock	*destProcPtr;
{
    SetupContext(destProcPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_FreeContext --
 *
 *      Release the context of the process.  This just entails clearing
 *	out the segment register value for the code segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Segment register value for the CODE segment in the machine dependent
 *	part of the process specific VM info struct is set to -1
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_FreeContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    procPtr->vmPtr->machPtr->segNums[VM_CODE] = -1;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_ReinitContext --
 *
 *      Free the current context and set up another one.  This is called
 *	by routines such as Proc_Exec that add things to the context and
 *	then have to abort or start a process running with a new image.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Context is set up for the currently executing process.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_ReinitContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    VmMach_FreeContext(procPtr);
    SetupContext(procPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_CopyInProc --
 *
 *	Copy from another processes address space into the current address
 *	space.   This is done by mapping the other processes segment into
 *	the current VAS and then doing the copy.  It assumed that this 
 *	routine is called with the source process locked such that its
 *	VM will not go away while we are doing this copy.
 *
 * Results:
 *	SUCCESS if the copy succeeded, SYS_ARG_NOACCESS if fromAddr is invalid.
 *
 * Side effects:
 *	What toAddr points to is modified.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
VmMach_CopyInProc(numBytes, fromProcPtr, fromAddr, virtAddrPtr,
	          toAddr, toKernel)
    int 	numBytes;		/* The maximum number of bytes to 
					   copy in. */
    Proc_ControlBlock	*fromProcPtr;	/* Which process to copy from.*/
    Address		fromAddr;	/* The address to copy from */
    Vm_VirtAddr		*virtAddrPtr;
    Address		toAddr;		/* The address to copy to */
    Boolean		toKernel;	/* This copy is happening to the
					 * kernel's address space. */
{
    register VmMach_ProcData	*machPtr;
    register int		*segNums;
    register int		*RPTPMs;
    Proc_ControlBlock		*toProcPtr;
    ReturnStatus		status = SUCCESS;
    int				segToUse;
    int				savedSegReg;
    int				savedRPTPM;

    toProcPtr = Proc_GetCurrentProc();
    machPtr = toProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->segRegMask = (unsigned int) (fromAddr) & VMMACH_SEG_REG_MASK;
    /*
     * Determine where to map in the source segment.
     */
    fromAddr = (Address) ((unsigned int)fromAddr & ~VMMACH_SEG_REG_MASK);
    if (((unsigned)toAddr & VMMACH_SEG_REG_MASK) == 
	 (unsigned)VMMACH_STACK_SEG_START) {
	fromAddr = (Address) ((unsigned int)fromAddr | VMMACH_HEAP_SEG_START);
	machPtr->mappedSegMask = VMMACH_HEAP_SEG_START;
	segToUse = VM_HEAP;
    } else {
	fromAddr = (Address) ((unsigned int)fromAddr | VMMACH_STACK_SEG_START);
	segToUse = VM_STACK;
	machPtr->mappedSegMask = VMMACH_STACK_SEG_START;
    }

    /*
     * Map the source segment in.
     */
    segNums = machPtr->segNums;
    RPTPMs = machPtr->RPTPMs;
    savedSegReg = segNums[segToUse];
    savedRPTPM = RPTPMs[segToUse];
    segNums[segToUse] = virtAddrPtr->segPtr->segNum;
    RPTPMs[segToUse] = segNums[segToUse] / 4 + rootPTPageNum;
    VmMachSetSegRegisters(segNums, RPTPMs);
    /*
     * Copy the data in.
     */
    status = VmMachDoCopy(numBytes, fromAddr, toAddr);
    /*
     * Clean up.
     */
    segNums[segToUse] = savedSegReg;
    RPTPMs[segToUse] = savedRPTPM;

    VmMachSetSegRegisters(segNums, RPTPMs);

    machPtr->segRegMask = -1;

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_CopyOutProc --
 *
 *	Copy from the current VAS to another processes VAS.  This is done by 
 *	mapping the other processes segment into the current VAS and then 
 *	doing the copy.  It assumed that this routine is called with the dest
 *	process locked such that its VM will not go away while we are doing
 *	the copy.
 *
 * Results:
 *	SUCCESS if the copy succeeded, SYS_ARG_NOACCESS if fromAddr is invalid.
 *
 * Side effects:
 *	What toAddr points to is modified.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
VmMach_CopyOutProc(numBytes, fromAddr, fromKernel, toProcPtr, toAddr,
		   virtAddrPtr)
    int 		numBytes;	/* The maximum number of bytes to 
					   copy in. */
    Address		fromAddr;	/* The address to copy from */
    Boolean		fromKernel;	/* This copy is happening to the
					 * kernel's address space. */
    Proc_ControlBlock	*toProcPtr;	/* Which process to copy from.*/
    Address		toAddr;		/* The address to copy to */
    Vm_VirtAddr		*virtAddrPtr;
{
    register VmMach_ProcData	*machPtr;
    register int		*segNums;
    register int		*RPTPMs;
    Proc_ControlBlock		*fromProcPtr;
    ReturnStatus		status = SUCCESS;
    int				segToUse;
    int				savedSegReg;
    int				savedRPTPM;

    fromProcPtr = Proc_GetCurrentProc();
    machPtr = fromProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->segRegMask = (unsigned int) (toAddr) & VMMACH_SEG_REG_MASK;
    /*
     * Determine where to map in the source segment.
     */
    toAddr = (Address) ((unsigned)toAddr & ~VMMACH_SEG_REG_MASK);
    if (((unsigned)fromAddr & VMMACH_SEG_REG_MASK) == 
	(unsigned)VMMACH_STACK_SEG_START) {
        toAddr = (Address) ((unsigned)toAddr | VMMACH_HEAP_SEG_START);
	machPtr->mappedSegMask = VMMACH_HEAP_SEG_START;
	segToUse = VM_HEAP;
    } else {
        toAddr = (Address) ((unsigned)toAddr | VMMACH_STACK_SEG_START);
	machPtr->mappedSegMask = VMMACH_STACK_SEG_START;
	segToUse = VM_STACK;
    }

    /*
     * Map the source segment in.
     */
    segNums = machPtr->segNums;
    RPTPMs = machPtr->RPTPMs;
    savedSegReg = segNums[segToUse];
    savedRPTPM = RPTPMs[segToUse];
    segNums[segToUse] = virtAddrPtr->segPtr->segNum;
    RPTPMs[segToUse] = segNums[segToUse] / 4 + rootPTPageNum;
    VmMachSetSegRegisters(segNums, RPTPMs);
    /*
     * Copy the data in.
     */
    status = VmMachDoCopy(numBytes, fromAddr, toAddr);
    /*
     * Clean up.
     */
    segNums[segToUse] = savedSegReg;
    RPTPMs[segToUse] = savedRPTPM;
    VmMachSetSegRegisters(segNums, RPTPMs);

    machPtr->segRegMask = -1;

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * The following routines read and write segments page tables. These
 * are underneath a monitor lock in order to synchronize access to these
 * structures.  We don't rely on the machine indepenent module to synchronize
 * for us because we may be called directly by the routines in the mach
 * module.
 *
 *----------------------------------------------------------------------
 */

Sync_Lock		ptLock = SYNC_LOCK_INIT_STATIC();
#define	LOCKPTR		&ptLock


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SetSegProt --
 *
 *	Change the protection in the page table for the given range of bytes
 *	for the given segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table may be modified for the segment.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMach_SetSegProt(segPtr, firstPage, lastPage, makeWriteable)
    register Vm_Segment		*segPtr;    /* Segment to change protection
					       for. */
    register int		firstPage;  /* First page to set protection
					     * for. */
    int				lastPage;   /* Last page to set protection
					     * for. */
    Boolean			makeWriteable;/* TRUE => make the pages 
					       *	 writable.
					       * FALSE => make readable only.*/
{
    register	VmMachPTE	*ptePtr;
    register	VmMach_SegData	*segDataPtr;
    VmMachPTE			machPTE;

    LOCK_MONITOR;

    segDataPtr = segPtr->machPtr;
    firstPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    lastPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, firstPage);
    for (; firstPage <= lastPage; firstPage++, ptePtr++) {
	if (*ptePtr & VMMACH_RESIDENT_BIT) {
	    machPTE = *ptePtr;
	    machPTE &= ~VMMACH_PROTECTION_FIELD;
	    machPTE |= 
		makeWriteable ? VMMACH_KRW_URW_PROT : VMMACH_KRW_URO_PROT;
	    *ptePtr = machPTE;
	    VmMachFlushPage(segPtr, firstPage, FALSE);
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SetPageProt --
 *
 *	Set the protection in hardware and software for the given virtual
 *	page.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table may be modified for the segment.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMach_SetPageProt(virtAddrPtr, softPTE)
    register	Vm_VirtAddr	*virtAddrPtr;	/* The virtual page to set the
						 * protection for.*/
    Vm_PTE			softPTE;	/* Software pte. */
{
    register	VmMach_SegData	*segDataPtr;
    register	VmMachPTE	*ptePtr;
    register	Vm_Segment	*segPtr;
    unsigned int		page;
    VmMachPTE			machPTE;

    LOCK_MONITOR;

    segPtr = virtAddrPtr->segPtr;
    segDataPtr = segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    machPTE = *ptePtr;
    machPTE &= ~VMMACH_PROTECTION_FIELD;
    machPTE |= (softPTE & (VM_COW_BIT | VM_READ_ONLY_PROT)) ? 
				VMMACH_KRW_URO_PROT : VMMACH_KRW_URW_PROT;
    *ptePtr = machPTE;
    VmMachFlushPage(segPtr, page, FALSE);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * VmMach_SetProtForDbg --
 *
 *      Set the protection of the kernel pages for the debugger.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The protection is set for the given range of kernel addresses.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_SetProtForDbg(readWrite, numBytes, addr)
    Boolean     readWrite;      /* TRUE if should make pages writable, FALSE
                                 * if should make read-only. */
    int         numBytes;       /* Number of bytes to change protection for. */
    Address     addr;           /* Address to start changing protection at. */
{
    register	VmMachPTE	*ptePtr;
    register    VmMachPTE       pte;
    register    int             firstPage;
    register    int             lastPage;

    firstPage = (unsigned)addr >> VMMACH_PAGE_SHIFT;
    lastPage = ((unsigned)addr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    for (ptePtr = kernPTPtr + firstPage; 
	 firstPage <= lastPage;
	 firstPage++, ptePtr++) {
        pte = *ptePtr;
        pte &= ~VMMACH_PROTECTION_FIELD;
        pte |= readWrite ? VMMACH_KRW_UNA_PROT : VMMACH_KRO_UNA_PROT;
        *ptePtr = pte;
    }
    /*
     * Flush the range from the cache so new protect will be read.  This
     * code should work before the Vm module is initialized.
     */
    while (numBytes > 0) { 
        VmMachFlushBlock(
		(Address)((unsigned)addr & ~(VMMACH_CACHE_BLOCK_SIZE-1)));
	addr += VMMACH_CACHE_BLOCK_SIZE;
	numBytes -= VMMACH_CACHE_BLOCK_SIZE;
    }
    /*
     * Flush the following cache block in case the range slops over into it.
     * For example addr = 0x1f numBytes = 2.
     */
    VmMachFlushBlock(
		(Address)((unsigned)addr & ~(VMMACH_CACHE_BLOCK_SIZE-1)));
   
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_AllocCheck --
 *
 *      Determine if this page can be reallocated.  A page can be reallocated
 *	if it has not been referenced or modified.
 *  
 * Results:
 *      None.
 *
 * Side effects:
 *      The given page will be invalidated in the hardware if it has not
 *	been referenced and *refPtr and *modPtr will have the hardware 
 *	reference and modify bits or'd in.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
VmMach_AllocCheck(virtAddrPtr, virtFrameNum, refPtr, modPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
    register	Boolean		*refPtr;
    register	Boolean		*modPtr;
{
    register VmMach_SegData	*segDataPtr;
    int				page;
    VmMachPTE			*ptePtr;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    if (useHardRefBit) {
	*refPtr |= *ptePtr & VMMACH_REFERENCED_BIT;
    }
    *modPtr |= *ptePtr & VMMACH_MODIFIED_BIT;
    if (!*refPtr) {
	/*
	 * Invalidate the page so that it will force a fault if it is
	 * referenced.  Since our caller has blocked all faults on this
	 * page, by invalidating it we can guarantee that the reference and
	 * modify information that we are returning will be valid until
	 * our caller reenables faults on this page.
	 */
	VmMachFlushPage(virtAddrPtr->segPtr, page, TRUE);
	*ptePtr = 0;
	if (*modPtr && !(*ptePtr & VMMACH_MODIFIED_BIT)) {
	    /*
	     * This page had the modify bit set in software but not in
	     * hardware.
	     */
	    vmStat.notHardModPages++;
	}
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_GetRefModBits --
 *
 *      Pull the reference and modified bits out of hardware.
 *  
 * Results:
 *      None.
 *
 * Side effects:
 *      
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
VmMach_GetRefModBits(virtAddrPtr, virtFrameNum, refPtr, modPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
    register	Boolean		*refPtr;
    register	Boolean		*modPtr;
{
    register VmMach_SegData	*segDataPtr;
    int				page;
    VmMachPTE			*ptePtr;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    if (useHardRefBit) {
	*refPtr = *ptePtr & VMMACH_REFERENCED_BIT;
    } else {
	*refPtr = FALSE;
    }
    *modPtr = *ptePtr & VMMACH_MODIFIED_BIT;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_ClearRefBit --
 *
 *      Clear the reference bit at the given virtual address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Hardware reference bit cleared.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
VmMach_ClearRefBit(virtAddrPtr, virtFrameNum)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtFrameNum;
{
    register VmMach_SegData	*segDataPtr;
    VmMachPTE			*ptePtr;
    unsigned int		page;

    LOCK_MONITOR;

    if (useHardRefBit) {
	segDataPtr = virtAddrPtr->segPtr->machPtr;
	page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
	ptePtr = GetPageTablePtr(segDataPtr, page);
	*ptePtr &= ~VMMACH_REFERENCED_BIT;
	if (flushOnRefBitClear) {
	    VmMachFlushPage(virtAddrPtr->segPtr, page, FALSE);
	}	
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SetRefBit --
 *
 *      Set the reference bit at the given address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Hardware reference bit set.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_SetRefBit(addr)
    Address	addr;
{
    register VmMach_SegData	*segDataPtr;
    int				page;
    Vm_Segment			*segPtr;
    VmMachPTE			*ptePtr;
    Proc_ControlBlock		*procPtr;
    register	VmMach_ProcData	*procDataPtr;

    LOCK_MONITOR;

    vmStat.machDepStat.refBitFaults++;

    procPtr = Proc_GetCurrentProc();
    procDataPtr = procPtr->vmPtr->machPtr;
    if (procDataPtr->segRegMask != (unsigned)-1 &&
	((unsigned)addr & VMMACH_SEG_REG_MASK) == procDataPtr->mappedSegMask) {
	segPtr = procDataPtr->mapSegPtr;
    } else {
	segPtr = procPtr->vmPtr->segPtrArray[(unsigned int)addr >> 
						VMMACH_SEG_REG_SHIFT];
    }
    if (segPtr != (Vm_Segment *) NIL) {
        segDataPtr = segPtr->machPtr;
        page = ((unsigned int)(addr) & ~VMMACH_SEG_REG_MASK) >> 
							VMMACH_PAGE_SHIFT;
        ptePtr = GetPageTablePtr(segDataPtr, page);
	if (*ptePtr & VMMACH_RESIDENT_BIT) {
	    *ptePtr |= VMMACH_REFERENCED_BIT;
	    if ((vmWriteableRefPageout || vmWriteablePageout) && 
		segPtr->type != VM_CODE) {
		*ptePtr |= VMMACH_MODIFIED_BIT;
	    }
	}
    }

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_ClearModBit --
 *
 *      Clear the modified bit at the given virtual address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Hardware modified bit cleared.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
VmMach_ClearModBit(virtAddrPtr, virtFrameNum)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
{
    register VmMach_SegData	*segDataPtr;
    unsigned int		page;
    VmMachPTE			*ptePtr;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    if ((!vmWriteableRefPageout && !vmWriteablePageout) || 
	!(*ptePtr & VMMACH_REFERENCED_BIT)) {
	*ptePtr &= ~VMMACH_MODIFIED_BIT;
	VmMachFlushPage(virtAddrPtr->segPtr, page, FALSE);
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SetModBit --
 *
 *      Set the modified bit at the given address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Hardware modified bit set.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_SetModBit(addr)
    Address	addr;
{
    register VmMach_SegData	*segDataPtr;
    VmMachPTE			*ptePtr;
    int				page;
    Vm_Segment			*segPtr;
    Proc_ControlBlock		*procPtr;
    register	VmMach_ProcData	*procDataPtr;

    LOCK_MONITOR;

    vmStat.machDepStat.dirtyBitFaults++;
    procPtr = Proc_GetCurrentProc();
    procDataPtr = procPtr->vmPtr->machPtr;
    if (procDataPtr->segRegMask != (unsigned)-1 &&
	((unsigned)addr & VMMACH_SEG_REG_MASK) == procDataPtr->mappedSegMask) {
	segPtr = procDataPtr->mapSegPtr;
    } else {
	segPtr = procPtr->vmPtr->segPtrArray[(unsigned int)addr >> 
						VMMACH_SEG_REG_SHIFT];
    }
    segDataPtr = segPtr->machPtr;
    page = ((unsigned int)(addr) & ~VMMACH_SEG_REG_MASK) >> VMMACH_PAGE_SHIFT;
    ptePtr = GetPageTablePtr(segDataPtr, page);
    if (*ptePtr & VMMACH_RESIDENT_BIT) {
	*ptePtr |= VMMACH_MODIFIED_BIT;
	/*
	 * Flush the block from the cache so that David Wood can get better
	 * cache stats.
	 */
	VmMachFlushBlock(
		    (Address)((unsigned)addr & ~(VMMACH_CACHE_BLOCK_SIZE-1)));
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_PageValidate --
 *
 *      Validate a page for the given virtual address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The page table is modified to validate the page.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_PageValidate(virtAddrPtr, pte) 
    register	Vm_VirtAddr	*virtAddrPtr;
    Vm_PTE			pte;
{
    register  VmMach_SegData	*segDataPtr;
    int				page;
    VmMachPTE			*ptePtr;
    VmMachPTE			machPTE;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    machPTE = VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | 
	      VMMACH_REFERENCED_BIT |
              SetPageFrame(VirtToPhysPage(Vm_GetPageFrame(pte)));
    if (virtAddrPtr->segPtr == vm_SysSegPtr) {
	machPTE |= VMMACH_KRW_URO_PROT | VMMACH_MODIFIED_BIT;
    } else {
	if (pte & (VM_COW_BIT | VM_READ_ONLY_PROT)) {
	    machPTE |= VMMACH_KRW_URO_PROT;
	} else {
	    machPTE |= VMMACH_KRW_URW_PROT;
	}
	if (virtAddrPtr->segPtr->type != VM_CODE) {
	    if (ownStackAndHeap) {
		machPTE |= VMMACH_COHERENCY_BIT;
	    }
	    if (vmWriteablePageout || vmWriteableRefPageout) {
		machPTE |= VMMACH_MODIFIED_BIT;
	    }
	}
    }
    *ptePtr = machPTE;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_PageInvalidate --
 *
 *      Invalidate a page for the given segment.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The page table is modified.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
VmMach_PageInvalidate(virtAddrPtr, virtPage, segDeletion) 
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtPage;
    Boolean			segDeletion;
{
    unsigned 	int	page;
    VmMach_SegData	*segDataPtr;
    VmMachPTE		*ptePtr;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    VmMachFlushPage(virtAddrPtr->segPtr, page, TRUE);
    *ptePtr = 0;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_PinUserPages --
 *
 *	Force a user page to be resident in memory and have its reference
 *	and modify bits set.  Our caller  has already assured that
 *	the page is locked in memory such that its reference and modify
 *	bits won't get cleared once we set them.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_PinUserPages(mapType, virtAddrPtr, lastPage)
    int		mapType;	/* VM_READONLY_ACCESS | VM_READWRITE_ACCESS */
    Vm_VirtAddr	*virtAddrPtr;	/* Pointer to beginning virtual address to
				 * to pin. */
    int		lastPage;	/* Last page to unpin. */
{
    register	Address	addr;
    register	int	i;
    volatile	int	*intPtr;
    int			intVal;

    for (i = lastPage - virtAddrPtr->page + 1, 
		addr = (Address)(virtAddrPtr->page << VMMACH_PAGE_SHIFT);
	 i > 0;
	 i--, addr += VMMACH_PAGE_SIZE) {
	intPtr = (int *)addr;
	intVal = *intPtr;
	if (mapType == VM_READWRITE_ACCESS) {
	    *intPtr = intVal;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_UnpinUserPages --
 *
 *	Allow a page that was pinned to be unpinned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_UnpinUserPages(virtAddrPtr, lastPage)
    Vm_VirtAddr	*virtAddrPtr;
    int		lastPage;
{
}

static	int	nextDevPage = VMMACH_KERN_DEVICE_SPACE / VMMACH_PAGE_SIZE;


/*
 *----------------------------------------------------------------------
 *
 * VmMach_MapInDevice --
 *
 *	Map a device at some physical address into kernel virtual address.
 *	This is for use by the controller initialization routines.  Note
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Address
VmMach_MapInDevice(devPhysAddr, numBytes)
    Address	devPhysAddr;	/* Physical address of the device to map in.*/
    int		numBytes;	/* Number of pages to validate. */
{
    int				numPages;
    int				firstPTPage;
    int				lastPTPage;
    register	VmMachPTE	*ptePtr;
    VmMachPTE			*ptAddr;
    int				i;
    VmMachPTE			pte;
    Address			retAddr;

    numPages = (((unsigned)devPhysAddr + numBytes - 1) >> VMMACH_PAGE_SHIFT) -
	       ((unsigned)devPhysAddr >> VMMACH_PAGE_SHIFT) + 1;
    firstPTPage = nextDevPage >> VMMACH_SEG_PT2_SHIFT;
    lastPTPage = (nextDevPage + numPages - 1) >> VMMACH_SEG_PT2_SHIFT;

    ptePtr = Get2ndPageTablePtr(vm_SysSegPtr->machPtr, firstPTPage);
    ptAddr = (VmMachPTE *)(vm_SysSegPtr->machPtr->ptBasePtr) + 
			    (firstPTPage << VMMACH_SEG_PT2_SHIFT);
    /*
     * Allocate the kernel page table which will map the device pages.
     */
    for (ptePtr = Get2ndPageTablePtr(vm_SysSegPtr->machPtr, firstPTPage);
	 firstPTPage <= lastPTPage;
	 firstPTPage++, ptePtr++, ptAddr += VMMACH_PTES_IN_PAGE) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT)) {
	    *ptePtr = VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | 
		      VMMACH_KRW_URO_PROT | VMMACH_REFERENCED_BIT |
		      VMMACH_MODIFIED_BIT |
		      SetPageFrame(VirtToPhysPage(Vm_KernPageAllocate()));
	    bzero((Address)ptAddr, VMMACH_PAGE_SIZE);
	}
    }
    /*
     * Now initialize the mappings in the page table.
     */
    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_URO_PROT | VMMACH_REFERENCED_BIT | 
		  VMMACH_MODIFIED_BIT | 
	          SetPageFrame((unsigned)devPhysAddr >> VMMACH_PAGE_SHIFT);
    for (i = 0, ptePtr = GetPageTablePtr(vm_SysSegPtr->machPtr, nextDevPage);
         i < numPages;
	 i++, pte = IncPageFrame(pte), ptePtr++) {
	*ptePtr = pte;
    }

    retAddr = (Address) ((nextDevPage << VMMACH_PAGE_SHIFT) + 
                         ((unsigned)devPhysAddr & VMMACH_OFFSET_MASK));
    nextDevPage += numPages;
    return(retAddr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_MapKernelIntoUser --
 *
 *      Map a portion of kernel memory into the user's heap segment.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
VmMach_MapKernelIntoUser(kernelVirtAddr, numBytes, userVirtAddr,
			 realVirtAddrPtr) 
    int	kernelVirtAddr;		/* Kernel virtual address to map in. */
    int	numBytes;		/* Number of bytes to map. */
    int	userVirtAddr;		/* User virtual address to attempt to start 
				   mapping in at. */
    int	realVirtAddrPtr;	/* Where we were able to start mapping at. */
{
}

/* 
 * The following mask is used to detect proper alignment of addresses
 * for doing word operations instead of byte operations.  If none of the
 * following bits are set in an address, then word-based transfers may be
 * used.
 */
#define WORDMASK 0x3


/*
 *----------------------------------------------------------------------
 *
 * VmMachDoCopy --
 *
 *	Copy numBytes from *sourcePtr to *destPtr.  This routine is
 *	optimized to do transfers when sourcePtr and destPtr are both
 *	4-byte aligned and point to large areas.
 *
 * Results:
 *	Contents at sourcePtr copied to contents at destPtr
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmMachDoCopy(numBytes, sourcePtr, destPtr)
    register int numBytes;	/* The number of bytes to copy */
    Address	sourcePtr;	/* Where to copy from */
    Address	destPtr;	/* Where to copy to. */
{
    register int *sPtr = (int *) sourcePtr;
    register int *dPtr = (int *) destPtr;

    /*
     * If the destination is below the source then it is safe to copy
     * in the forward direction.  Otherwise, we must start at the top
     * and work down, again optimizing for large transfers.
     */

    if (dPtr < sPtr) {
	/*
	 * If both the source and the destination point to aligned
	 * addresses then copy as much as we can in large transfers.  Once
	 * we have less than a whole int to copy then it must be done by
	 * byte transfers.  Furthermore, use an expanded loop to avoid
	 * the overhead of continually testing loop variables.
	 */
	
	if (!((((int) sPtr) | (int) dPtr) & WORDMASK)) {
	    while (numBytes >= 16*sizeof(int)) {
		dPtr[0] = sPtr[0];
		dPtr[1] = sPtr[1];
		dPtr[2] = sPtr[2];
		dPtr[3] = sPtr[3];
		dPtr[4] = sPtr[4];
		dPtr[5] = sPtr[5];
		dPtr[6] = sPtr[6];
		dPtr[7] = sPtr[7];
		dPtr[8] = sPtr[8];
		dPtr[9] = sPtr[9];
		dPtr[10] = sPtr[10];
		dPtr[11] = sPtr[11];
		dPtr[12] = sPtr[12];
		dPtr[13] = sPtr[13];
		dPtr[14] = sPtr[14];
		dPtr[15] = sPtr[15];
		sPtr += 16;
		dPtr += 16;
		numBytes -= 16*sizeof(int);
	    }
	    while (numBytes >= sizeof(int)) {
		*dPtr++ = *sPtr++;
		numBytes -= sizeof(int);
	    }
	    if (numBytes == 0) {
		return(SUCCESS);
	    }
	}
	
	/*
	 * Copy the remaining bytes.
	 */
	
	sourcePtr = (char *) sPtr;
	destPtr = (char *) dPtr;
	while (numBytes > 0) {
	    *destPtr++ = *sourcePtr++;
	    numBytes--;
	}
    } else {
	/*
	 * Handle extra bytes at the top that are due to the transfer
	 * length rather than pointer misalignment.
	 */
	while (numBytes & WORDMASK) {
	    numBytes --;
	    destPtr[numBytes] = sourcePtr[numBytes];
	}
	sPtr = (int *) (sourcePtr + numBytes);
	dPtr = (int *) (destPtr + numBytes);

	if (!((((int) sPtr) | (int) dPtr) & WORDMASK)) {
	    while (numBytes >= 16*sizeof(int)) {
		sPtr -= 16;
		dPtr -= 16;
		dPtr[15] = sPtr[15];
		dPtr[14] = sPtr[14];
		dPtr[13] = sPtr[13];
		dPtr[12] = sPtr[12];
		dPtr[11] = sPtr[11];
		dPtr[10] = sPtr[10];
		dPtr[9] = sPtr[9];
		dPtr[8] = sPtr[8];
		dPtr[7] = sPtr[7];
		dPtr[6] = sPtr[6];
		dPtr[5] = sPtr[5];
		dPtr[4] = sPtr[4];
		dPtr[3] = sPtr[3];
		dPtr[2] = sPtr[2];
		dPtr[1] = sPtr[1];
		dPtr[0] = sPtr[0];
		numBytes -= 16*sizeof(int);
	    }
	    while (numBytes >= sizeof(int)) {
		*--dPtr = *--sPtr;
		numBytes -= sizeof(int);
	    }
	    if (numBytes == 0) {
		return(SUCCESS);
	    }
	}
	
	/*
	 * Copy the remaining bytes.
	 */
	
	destPtr = (char *) dPtr;
	sourcePtr = (char *) sPtr;
	while (numBytes > 0) {
	    *--destPtr = *--sourcePtr;
	    numBytes--;
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyIn --
 *
 *	Copy numBytes from *sourcePtr to *destPtr. 
 *
 * Results:
 *	SUCCESS if successfully copied in the string.  SYS_ARG_NOACCESS
 *	otherwise.  SYS_ARG_NOACCESS is returned by the trap handlers if
 *	we hit an error.
 *
 * Side effects:
 *	Contents at sourcePtr copied to contents at destPtr
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_CopyIn(numBytes, sourcePtr, destPtr)
    register int	numBytes;	/* The number of bytes to copy */
    Address		sourcePtr;	/* Where to copy from */
    Address		destPtr;	/* Where to copy to */
{
    if (numBytes == 0) {
	return(SUCCESS);
    } else {
	return(VmMachDoCopy(numBytes, sourcePtr, destPtr));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyOut --
 *
 *	Copy numBytes from *sourcePtr to *destPtr.
 *
 * Results:
 *	SUCCESS if successfully copied in the string.  SYS_ARG_NOACCESS
 *	otherwise.  SYS_ARG_NOACCESS is returned by the trap handlers if
 *	we hit an error.
 *
 * Side effects:
 *	Contents at sourcePtr copied to contents at destPtr.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_CopyOut(numBytes, sourcePtr, destPtr)
    register int	numBytes;	/* The number of bytes to copy */
    Address		sourcePtr;	/* Where to copy from */
    Address		destPtr;	/* Where to copy to */
{
    if (numBytes == 0) {
	return(SUCCESS);
    } else if (destPtr < mach_FirstUserAddr || destPtr > mach_LastUserAddr) {
	return(SYS_ARG_NOACCESS);
    } else {
	return(VmMachDoCopy(numBytes, sourcePtr, destPtr));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_StringNCopy --
 *
 *	Copies exactly n characters from src to dst.  If src doesn't
 *	contain exactly n characters, then the last characters are
 *	ignored (if src is too long) or filled with zeros (if src
 *	is too short).  In the case of truncation, dst may not be
 *	null-terminated.
 *
 * Results:
 *	SUCCESS if successfully copied in the string.  SYS_ARG_NOACCESS
 *	otherwise.  SYS_ARG_NOACCESS is returned by the trap handlers if
 *	we hit an error.
 *
 * Side effects:
 *	Memory at *dst is modified.  The caller must ensure that the
 *	destination is large enough to hold n characters.  Alternatively,
 *	dst may be dynamically allocated.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_StringNCopy(n, src, dst, bytesCopiedPtr)
    register int	n;			/* How many characters to 
						 * place at dst. */
    register char	*src;			/* Source string. */
    char		*dst;			/* Destination area. */
    int			*bytesCopiedPtr;	/* OUT: The number of bytes 
						 * actually copied. */
{
    register char	*copy = dst;
    char		*origSrc = src;

    if (n == 0) {
	*bytesCopiedPtr = 0;
	return(SUCCESS);
    }
#ifndef lint
    do {
	if ((*copy++ = *src) != 0) {
	    src += 1;
	} else {
	    break;
	}
    } while (--n > 0);
#else
    do {
	*copy = *src;
	copy++;
	if (*src != 0) {
	    src += 1;
	} else {
	    break;
	}
	n--;
    } while (n > 0);
#endif
    *bytesCopiedPtr = src - origSrc;

    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * Vm_TouchPages --
 *
 *	Touch each of the given pages.
 *
 * Results:
 *	SUCCESS if successfully touched all pages.  SYS_ARG_NOACCESS
 *	otherwise.  SYS_ARG_NOACCESS is returned by the trap handlers if
 *	we hit an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_TouchPages(firstPage, numPages)
    register	int	firstPage;
    register	int	numPages;
{
    register Address	addr;
    volatile int	*intPtr;
    int			intVal;

    for (addr = (Address)(firstPage << VMMACH_PAGE_SHIFT);
	 numPages > 0;
	 numPages--, addr += VMMACH_PAGE_SIZE) {
	intPtr = (int *)addr;
	intVal = *intPtr;
    }
    return(SUCCESS);
}

/*
 * Routine to mark the end of the copy routines so that the trap handlers
 * will know whether a kernel error happened on a cross-address space
 * copy.
 */
void
VmMachCopyEnd()
{
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_FlushPage --
 *
 *	Flush the page at the given virtual address from all caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given page is flushed from the caches.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMach_FlushPage(virtAddrPtr, invalidate)
    register	Vm_VirtAddr	*virtAddrPtr;	/* Virtual address of page. */
    Boolean			invalidate;	/* Should invalidate the pte
						 * after flushing. */
{
    LOCK_MONITOR;

    VmMachFlushPage(virtAddrPtr->segPtr, 
		    (unsigned int)(virtAddrPtr->page &
			~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT)),
		    invalidate);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushPage --
 *
 *	Flush the given page from all caches.  Note the page is relative
 *	to the beginning of the segment.
 *
 *	NOTE: This code assumes that it will not get involuntarily 
 *	      context switch within it.  If we can get context switched
 *	      then it will have to modify the segment register value
 *	      stored in the machine dependent proc table struct.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given page is flushed from the caches.
 *
 *----------------------------------------------------------------------
 */
INTERNAL void
VmMachFlushPage(segPtr, pageNum, invalidate)
    register	Vm_Segment	*segPtr; /* Segment to flush. */
    unsigned	int		pageNum; /* Page within segment to flush. */
    Boolean			invalidate;	/* Should invalidate the pte
						 * after flushing. */
{
    register	Address		pageAddr;
    register	Address		addr;
    register	VmMachPTE	*ptePtr;
    VmMach_SegData		*segDataPtr;
    int				origSegNum;
    int				refBit;

    segDataPtr = segPtr->machPtr;
    ptePtr = GetPageTablePtr(segDataPtr, pageNum);
    if (!(*ptePtr & VMMACH_RESIDENT_BIT)) {
	return;
    }

    if (segPtr->segNum > 0) {
	origSegNum = VmMachSetSegReg1(segPtr->segNum, 
				      segPtr->segNum / 4 + rootPTPageNum);
	pageAddr = (Address) (VMMACH_CODE_SEG_START |
			      (pageNum << VMMACH_PAGE_SHIFT));
    } else {
	pageAddr = (Address) (pageNum << VMMACH_PAGE_SHIFT);
    }

    *ptePtr &= ~VMMACH_RESIDENT_BIT;
    /*
     * Set the reference bit because the ReadAnyways will cause a
     * reference bit fault.
     */
    refBit = *ptePtr & VMMACH_REFERENCED_BIT;
    *ptePtr |= VMMACH_REFERENCED_BIT;
    for (addr = pageAddr; 
         addr < pageAddr + VMMACH_PAGE_SIZE;
	 addr = addr + VMMACH_CACHE_BLOCK_SIZE) {
	VmMachReadAnyways(addr);
	VmMachFlushBlock(addr);
    }
    *ptePtr &= ~VMMACH_REFERENCED_BIT;
    if (!invalidate) {
	/*
	 * We are not invalidating this page so restore the reference and
	 * modify bits.
	 */
	*ptePtr |= (VMMACH_RESIDENT_BIT | refBit);
    }
    if (segPtr->segNum > 0) {
	(void) VmMachSetSegReg1(origSegNum, origSegNum / 4 + rootPTPageNum);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushBytes --
 *
 *	Flush the given range of bytes from all caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given range of bytes is flushed from the caches.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMachFlushBytes(segPtr, startAddr, numBytes)
    Vm_Segment	*segPtr;	/* Segment to flush bytes in. */
    Address	startAddr;	/* Address to start flushing at. */
    int		numBytes;	/* Number of bytes to flush. */
{
    unsigned	int	firstPage;
    unsigned	int	lastPage;

    LOCK_MONITOR;

    firstPage = (unsigned int)startAddr >> VMMACH_PAGE_SHIFT;
    firstPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    lastPage = (unsigned int)(startAddr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    lastPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    for (; firstPage <= lastPage; firstPage++) {
	VmMachFlushPage(segPtr, firstPage, FALSE);
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * SimpleValidate --
 *
 *	Make sure that the range of bytes are valid in the kernel's address
 *	space.  This works even before virtual memory has been initialized.
 *
 * Results:
 *	TRUE if the range of addresses are valid and FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
SimpleValidate(addr, numBytes)
    Address	addr;
    int		numBytes;
{
    register VmMachPTE	*ptePtr;
    unsigned int	firstPage;
    unsigned int	lastPage;
    unsigned int	firstPTPage;
    unsigned int	lastPTPage;

    if (((unsigned int)addr & VMMACH_SEG_REG_MASK) != 0 ||
	((unsigned int)(addr + numBytes - 1) & VMMACH_SEG_REG_MASK) != 0) {
	return(FALSE);
    }

    firstPage = (unsigned int)addr >> VMMACH_PAGE_SHIFT;
    lastPage = (unsigned int)(addr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    firstPTPage = firstPage >> VMMACH_SEG_PT2_SHIFT;
    lastPTPage = lastPage >> VMMACH_SEG_PT2_SHIFT;

    /*
     * Assume that the root page tables are mapped and check the second
     * level page tables first.  If the root page tables aren't mapped then
     * we won't get this far in the first place.
     */
    for (ptePtr = (VmMachPTE *)(VMMACH_KERN_PT2_BASE) + firstPTPage;
	 firstPTPage <= lastPTPage;
	 firstPTPage++, ptePtr++) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT) ||
	    (*ptePtr & VMMACH_PROTECTION_FIELD) == 0) {
	    return(FALSE);
	}
    }
    /*
     * Now check the first level page table entries.
     */
    for (ptePtr = (VmMachPTE *)(VMMACH_KERN_PT_BASE) + firstPage;
	 firstPage <= lastPage;
	 firstPage++, ptePtr++) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT)) {
	    return(FALSE);
	}
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_ValidateRange --
 *
 *	Make sure that the range of bytes are valid in the kernel's address
 *	space.
 *
 * Results:
 *	TRUE if the range of addresses are valid and FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
Vm_ValidateRange(addr, numBytes)
    Address	addr;
    int		numBytes;
{
    register VmMachPTE	*ptePtr;
    unsigned int	firstPage;
    unsigned int	lastPage;
    unsigned int	firstPTPage;
    unsigned int	lastPTPage;
    VmMach_SegData	*segDataPtr;
    int			hardSegNum;
    Proc_ControlBlock	*procPtr;
    Vm_Segment		*segPtr;
    int			origSegNum;

    if (!initialized || useSimpleValidation) {
	return(SimpleValidate(addr, numBytes));
    }

    hardSegNum = (unsigned int)addr >> VMMACH_SEG_REG_SHIFT;
    if (hardSegNum == 0) {
	segPtr = vm_SysSegPtr;
    } else {
	procPtr = Proc_GetCurrentProc();
	segPtr = procPtr->vmPtr->segPtrArray[hardSegNum];
	if (segPtr == (Vm_Segment *)NIL) {
	    return(FALSE);
	}
	switch (hardSegNum) {
	    case 1:
		origSegNum = VmMachSetSegReg1(segPtr->segNum, 
				      segPtr->segNum / 4 + rootPTPageNum);
		if (origSegNum != segPtr->segNum) {
		    (void)VmMachSetSegReg1(origSegNum,
				      origSegNum / 4 + rootPTPageNum);
		    return(FALSE);
		}
		break;
	    case 2:
		origSegNum = VmMachSetSegReg2(segPtr->segNum, 
				      segPtr->segNum / 4 + rootPTPageNum);
		if (origSegNum != segPtr->segNum) {
		    (void)VmMachSetSegReg2(origSegNum,
				      origSegNum / 4 + rootPTPageNum);
		    return(FALSE);
		}
		break;
	    case 3:
		origSegNum = VmMachSetSegReg3(segPtr->segNum, 
				      segPtr->segNum / 4 + rootPTPageNum);
		if (origSegNum != segPtr->segNum) {
		    (void)VmMachSetSegReg3(origSegNum,
				      origSegNum / 4 + rootPTPageNum);
		    return(FALSE);
		}
		break;
	}
    }
    segDataPtr = segPtr->machPtr;

    addr = (Address) ((unsigned int)addr & ~VMMACH_SEG_REG_MASK);
    firstPage = (unsigned int)addr >> VMMACH_PAGE_SHIFT;
    lastPage = (unsigned int)(addr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    firstPTPage = firstPage >> VMMACH_SEG_PT2_SHIFT;
    lastPTPage = lastPage >> VMMACH_SEG_PT2_SHIFT;

    /*
     * Assume that the root page tables are mapped and check the second
     * level page tables first.  If the root page tables aren't mapped then
     * we won't get this far in the first place.
     */
    for (ptePtr = Get2ndPageTablePtr(segDataPtr, firstPTPage);
	 firstPTPage <= lastPTPage;
	 firstPTPage++, ptePtr++) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT) ||
	    (*ptePtr & VMMACH_PROTECTION_FIELD) == 0) {
	    return(FALSE);
	}
    }
    /*
     * Now check the first level page table entries.
     */
    for (ptePtr = GetPageTablePtr(segDataPtr, firstPage);
	 firstPage <= lastPage;
	 firstPage++, ptePtr++) {
	if (!(*ptePtr & VMMACH_RESIDENT_BIT)) {
	    return(FALSE);
	}
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_Trace --
 *
 *	SPUR VM tracing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_Trace()
{
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_Cmd --
 *
 *	Machine dependent vm command handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmMach_Cmd(command, arg)
    int	command;
    int	arg;
{
    switch (command) {
	case VM_SET_FLUSH_ON_REF_BIT_CLEAR:
	    printf("flushOnRefBitClear val was %d, is %d\n",
		        flushOnRefBitClear, arg);
	    flushOnRefBitClear = arg;
	    return(SUCCESS);
	case VM_SET_USE_HARD_REF_BIT:
	    printf("useHardRefBit val was %d, is %d\n",
		        useHardRefBit, arg);
	    useHardRefBit = arg;
	    return(SUCCESS);
	case VM_SET_COHERENCY_BIT:
	    printf("ownStackAndHeap val was %d, is %d\n",
		        ownStackAndHeap, arg);
	    ownStackAndHeap = (Boolean) arg;
	    return(SUCCESS);
	default:
	    printf("Warning: VmMach_Cmd: Unknown command %d\n", command);
	    return(GEN_INVALID_ARG);
    }
}

/*
 * Dummy function which will turn out to be the function that the debugger
 * prints out on a backtrace after a trap.  The debugger gets confused
 * because trap stacks originate from assembly language stacks.  I decided
 * to make a dummy procedure because it was to confusing seeing the
 * previous procedure (VmMach_MapKernelIntoUser) on every backtrace.
 */
VmMachTrap()
{
}
