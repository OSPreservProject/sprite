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
#include "mach.h"
#include "list.h"
#include "mem.h"
#include "proc.h"
#include "sched.h"
#include "sync.h"
#include "sys.h"
#include "byte.h"
#include "dbg.h"

/*
 * Macros to translate from a virtual page to a physical page and back.
 */
#define	VirtToPhysPage(pfNum) (MACH_PAGE_SLOT_MASK | ((pfNum) + MACH_FIRST_PHYS_PAGE))
#define	PhysToVirtPage(pfNum) (((pfNum)-MACH_FIRST_PHYS_PAGE) & ~MACH_PAGE_SLOT_MASK)

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
		numKernPagesPtr)
    int	*pageSizePtr;
    int	*pageShiftPtr;
    int	*pageTableIncPtr;
    int	*kernMemSizePtr;
    int	*numKernPagesPtr;
{
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
    *numKernPagesPtr = GetNumPages();
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
    return(MACH_NUM_PHYS_PAGES);
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
    VmMachPTE		pte;
    int 		i;
    int 		firstFreeSegment;
    Address		virtAddr;
    int			pfNum;
    VmMach_SegData	*machPtr;

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
	Vm_ReservePage(PhysToVirtPage(pfNum));
    }

    /*
     * Map enough of the kernel page tables to cover the page tables for
     * the maximum amount of the kernel VAS that can be in use.
     */
    for (ptePtr = kernPT2Ptr, i = 0;
	 i <= machPtr->lastPTPage;
	 ptePtr++, i++) {
	pfNum = GetPageFrame(*ptePtr);
	Vm_ReservePage(PhysToVirtPage(pfNum));
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
    Byte_Zero((VMMACH_NUM_SEGMENTS - 1) * VMMACH_SEG_PT2_SIZE,
	      ((Address)kernPT2Ptr) + VMMACH_SEG_PT2_SIZE);

    /*
     * Clear out the rest of kernel page tables starting with the first
     * free page.
     */
    for (ptePtr = basePTPtr + firstFreePage, i = firstFreePage;
         i < (machPtr->lastPTPage + 1) * VMMACH_PTES_IN_PAGE; 
         ptePtr++, i++) {
	*ptePtr = (VmMachPTE) 0;
    }
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
	    Byte_Zero(VMMACH_PAGE_SIZE, ptAddr);
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
	segDataPtr = (VmMach_SegData *)Mem_Alloc(sizeof(VmMach_SegData));
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
    AllocPageTable(segPtr, segOffset, numPages);
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
    AllocPageTable(segPtr, firstPage - segPtr->type * VMMACH_PAGES_PER_SEG,
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
    register	int		ptPage;
    register	int		firstPage;

    segDataPtr = segPtr->machPtr;
    ptPage = (unsigned int)(segDataPtr->ptBasePtr +
		       (segDataPtr->firstPTPage << VMMACH_SEG_PT2_SHIFT)) >>
			    VMMACH_PAGE_SHIFT;
    for (ptePtr = Get2ndPageTablePtr(segDataPtr, segDataPtr->firstPTPage),
	    firstPage = segDataPtr->firstPTPage;
	 firstPage <= segDataPtr->lastPTPage;
	 firstPage++, ptePtr++, ptPage++) {
	if (*ptePtr & VMMACH_RESIDENT_BIT) {
	    Vm_KernPageFree(PhysToVirtPage(GetPageFrame(*ptePtr)));
	    *ptePtr &= ~VMMACH_RESIDENT_BIT;
	    VmMachFlushPage(vm_SysSegPtr, ptPage);
	}
     }
    VmMachFlushSegment(segPtr);
/*
    VmMachFlushBytes(vm_SysSegPtr, segDataPtr->pt2BasePtr, VMMACH_SEG_PT2_SIZE);
*/
    segPtr->machPtr = (VmMach_SegData *)NIL;
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
    if (((unsigned)virtAddr & VMMACH_SEG_REG_MASK) == procDataPtr->segRegMask) {
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
	vmPtr->machPtr = (VmMach_ProcData *)Mem_Alloc(sizeof(VmMach_ProcData));
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
    if (((unsigned)toAddr >> VMMACH_SEG_REG_SHIFT) == VM_STACK) {
	segToUse = VM_HEAP;
    } else {
	segToUse = VM_STACK;
    }

    /*
     * Map the source segment in.
     */
    segNums = machPtr->segNums;
    RPTPMs = machPtr->RPTPMs;
    savedSegReg = segNums[segToUse];
    savedRPTPM = RPTPMs[segToUse];
    segNums[segToUse] = (unsigned)fromAddr >> VMMACH_SEG_REG_SHIFT;
    RPTPMs[segToUse] = segNums[segToUse] / 4 + rootPTPageNum;
    VmMachSetSegRegisters(segNums, RPTPMs);
    fromAddr = (Address) ((unsigned int)fromAddr & ~VMMACH_SEG_REG_MASK);
    fromAddr = (Address) ((unsigned int)fromAddr | (segToUse << VMMACH_SEG_REG_SHIFT));
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
    int				segOffset;
    int				bytesToCopy;
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
    if (((unsigned)fromAddr >> VMMACH_SEG_REG_SHIFT) == VM_STACK) {
	segToUse = VM_HEAP;
    } else {
	segToUse = VM_STACK;
    }

    /*
     * Map the source segment in.
     */
    segNums = machPtr->segNums;
    RPTPMs = machPtr->RPTPMs;
    savedSegReg = segNums[segToUse];
    savedRPTPM = RPTPMs[segToUse];
    segNums[segToUse] = (unsigned)toAddr >> VMMACH_SEG_REG_SHIFT;
    RPTPMs[segToUse] = segNums[segToUse] / 4 + rootPTPageNum;
    VmMachSetSegRegisters(segNums, RPTPMs);
    toAddr = (Address) ((unsigned)toAddr & ~VMMACH_SEG_REG_MASK);
    toAddr = (Address) ((unsigned)toAddr | (segToUse << VMMACH_SEG_REG_SHIFT));
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
 * for us because we made be called directly by the routines in the mach
 * module.
 *
 *----------------------------------------------------------------------
 */

Sync_Lock		ptLock;
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

    LOCK_MONITOR;

    segDataPtr = segPtr->machPtr;
    firstPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    lastPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, firstPage);
    for (; firstPage <= lastPage; firstPage++, ptePtr++) {
	if (*ptePtr & VMMACH_RESIDENT_BIT) {
	    *ptePtr &= ~VMMACH_PROTECTION_FIELD;
	    *ptePtr |= 
		makeWriteable ? VMMACH_KRW_URW_PROT : VMMACH_KRW_URO_PROT;
	}
    }
    VmMachFlushSegment(segPtr);

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
    int				page;

    LOCK_MONITOR;

    segPtr = virtAddrPtr->segPtr;
    segDataPtr = segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr &= ~VMMACH_PROTECTION_FIELD;
    *ptePtr |= (softPTE & (VM_COW_BIT | VM_READ_ONLY_PROT)) ? 
				VMMACH_KRW_URO_PROT : VMMACH_KRW_URW_PROT;
    VmMachFlushPage(segPtr, page);

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
    *refPtr = *ptePtr & VMMACH_REFERENCED_BIT;
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
ENTRY void
VmMach_ClearRefBit(virtAddrPtr, virtFrameNum)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtFrameNum;
{
    register VmMach_SegData	*segDataPtr;
    VmMachPTE			*ptePtr;
    int				page;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr &= ~VMMACH_REFERENCED_BIT;
    VmMachFlushPage(virtAddrPtr->segPtr, page);

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

    LOCK_MONITOR;

    segPtr = VmGetSegPtr((unsigned int)addr >> VMMACH_SEG_REG_SHIFT);
    segDataPtr = segPtr->machPtr;
    page = ((unsigned int)(addr) & ~VMMACH_SEG_REG_MASK) >> VMMACH_PAGE_SHIFT;
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr |= VMMACH_REFERENCED_BIT;

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
ENTRY void
VmMach_ClearModBit(virtAddrPtr, virtFrameNum)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
{
    register VmMach_SegData	*segDataPtr;
    int				page;
    VmMachPTE			*ptePtr;

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr &= ~VMMACH_MODIFIED_BIT;
    VmMachFlushPage(virtAddrPtr->segPtr, page);

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

    LOCK_MONITOR;

    segPtr = VmGetSegPtr((unsigned int)addr >> VMMACH_SEG_REG_SHIFT);
    segDataPtr = segPtr->machPtr;
    page = ((unsigned int)(addr) & ~VMMACH_SEG_REG_MASK) >> VMMACH_PAGE_SHIFT;
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr |= VMMACH_MODIFIED_BIT;

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

    LOCK_MONITOR;

    segDataPtr = virtAddrPtr->segPtr->machPtr;
    page = virtAddrPtr->page & ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    ptePtr = GetPageTablePtr(segDataPtr, page);
    *ptePtr = VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT |
              SetPageFrame(VirtToPhysPage(Vm_GetPageFrame(pte)));
    if (virtAddrPtr->segPtr == vm_SysSegPtr) {
	*ptePtr |= VMMACH_KRW_UNA_PROT | VMMACH_REFERENCED_BIT | 
		   VMMACH_MODIFIED_BIT;
    } else {
	if (pte & (VM_COW_BIT | VM_READ_ONLY_PROT)) {
	    *ptePtr |= VMMACH_KRW_URO_PROT;
	} else {
	    *ptePtr |= VMMACH_KRW_URW_PROT;
	}
    }

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
    if (!segDeletion) {
	VmMachFlushPage(virtAddrPtr->segPtr, page);
    }
    *ptePtr = 0;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_PinUserPage --
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
VmMach_PinUserPage(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
{
    int	*intPtr;
    int	i;

    /*
     * Read out the value and write it back to ensure that the reference
     * and modify bits are set and we don't accidently change the value.
     */
    intPtr = (int *) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
    i = *intPtr;
    *intPtr = i;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_UnpinUserPage --
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
/* ARGSUSED */
void
VmMach_UnpinUserPage(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
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
	    Byte_Zero(VMMACH_PAGE_SIZE, (Address)ptAddr);
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
    register int *sPtr;
    register int *dPtr;
    int	    	  extra;
    
    /*
     * If the destination is below the source or doesn't overlap
     * the source, then it is safe to copy in the forward direction.
     * Otherwise, we must start at the top and work down, again optimizing
     * for large transfers.
     */

    if (destPtr < sourcePtr || (destPtr > sourcePtr + numBytes)) {
	/*
	 * If both the sourcePtr and the destPtr point to aligned
	 * addresses then copy as much as we can in large transfers.  Once
	 * we have less than 4 bytes to copy then it must be done by
	 * byte transfers.  Furthermore, use an expanded loop to avoid
	 * the overhead of continually testing loop variables.
	 */
	
	/*
	 * If the pointers aren't aligned, but they're both misaligned by the
	 * same amount, we can copy a few bytes to align them.
	 */
	extra = (int) sourcePtr & WORDMASK;
	if (extra && extra == ((int)destPtr&WORDMASK)) {
	    extra = 4 - extra;
	    if (extra < numBytes) {
		while (extra > 0) {
		    *destPtr++ = *sourcePtr++;
		    extra--;
		    numBytes--;
		}
	    }
	}

	sPtr = (int *)sourcePtr;
	dPtr = (int *)destPtr;
	    
	if (!((int) sPtr & WORDMASK) && !((int) dPtr & WORDMASK)) {
	    while (numBytes >= 32) {
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		*dPtr++ = *sPtr++;
		numBytes -= 32;
	    }
	    while (numBytes >= 4) {
		*dPtr++ = *sPtr++;
		numBytes -= 4;
	    }
	    sourcePtr = (char *) sPtr;
	    destPtr = (char *) dPtr;
	}
	
	/*
	 * Copy the remaining bytes.
	 */
	
	while (numBytes > 0) {
	    *destPtr++ = *sourcePtr++;
	    numBytes--;
	}
    } else {
	destPtr += numBytes;
	sourcePtr += numBytes;
	extra = (int)sourcePtr & WORDMASK;

	if (extra && extra == ((int)destPtr & WORDMASK) && (extra < numBytes)){
	    while (extra > 0) {
		*--destPtr = *--sourcePtr;
		extra--;
		numBytes--;
	    }
	}

	sPtr = (int *)sourcePtr;
	dPtr = (int *)destPtr;
	
	if (!((int) sPtr & WORDMASK) && !((int) dPtr & WORDMASK)) {
	    while (numBytes >= 32) {
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		*--dPtr = *--sPtr;
		numBytes -= 32;
	    }
	    while (numBytes >= 4) {
		*--dPtr = *--sPtr;
		numBytes -= 4;
	    }
	    sourcePtr = (char *) sPtr;
	    destPtr = (char *) dPtr;
	}
	
	/*
	 * Copy the remaining bytes.
	 */
	
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
    return(VmMachDoCopy(numBytes, sourcePtr, destPtr));
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
    if (destPtr < mach_FirstUserAddr || destPtr > mach_LastUserAddr) {
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
Vm_StringNCopy(n, src, dst)
    register int n;		/* How many characters to place at dst. */
    register char *src;		/* Source string. */
    char *dst;			/* Destination area. */
{
    register char *copy = dst;

    if (n == 0) {
	return(SUCCESS);
    }
    do {
	if ((*copy++ = *src) != 0) {
	    src += 1;
	}
    } while (--n > 0);
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
 * VmMachFlushPage --
 *
 *	Flush the given page from all caches.  Note the page is relative
 *	to the beginning of the segment.
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
VmMachFlushPage(segPtr, pageNum)
    Vm_Segment	*segPtr;	/* Segment to flush. */
    int		pageNum;	/* Page within segment to flush. */
{
    register	Address	pageAddr;
    register	Address	addr;
    VmMachPTE		*ptePtr;
    Boolean		setResBit = FALSE;
    VmMach_SegData	*segDataPtr;

    segDataPtr = segPtr->machPtr;

    ptePtr = GetPageTablePtr(segDataPtr, pageNum);
    if (*ptePtr & VMMACH_RESIDENT_BIT) {
	*ptePtr &= ~VMMACH_RESIDENT_BIT;
	setResBit = TRUE;
    }
    pageAddr = (Address) ((segPtr->segNum << VMMACH_SEG_REG_SHIFT) + 
			  (pageNum << VMMACH_PAGE_SHIFT));
    for (addr = pageAddr; 
         addr < pageAddr + VMMACH_PAGE_SIZE;
	 addr = addr + VMMACH_CACHE_BLOCK_SIZE) {
	VmMachReadAnyways(addr);
	VmMachFlushBlock(addr);
    }
    if (setResBit) {
	*ptePtr |= VMMACH_RESIDENT_BIT;
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
    int		firstPage;
    int		lastPage;

    LOCK_MONITOR;

    firstPage = (unsigned int)startAddr >> VMMACH_PAGE_SHIFT;
    firstPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    lastPage = (unsigned int)(startAddr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    lastPage &= ~(VMMACH_SEG_REG_MASK >> VMMACH_PAGE_SHIFT);
    for (; firstPage <= lastPage; firstPage++) {
	VmMachFlushPage(segPtr, firstPage);
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushSegment --
 *
 *	Flush the given page from all caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given page is flushed from the caches.
 *
 *----------------------------------------------------------------------
 */
void
VmMachFlushSegment(segPtr)
    Vm_Segment	*segPtr;
{
    int		i;

    LOCK_MONITOR;

    /*
     * Uni-processor solution.  Flush the entire cache on this machine.
     */
    for (i = 0; i < VMMACH_CACHE_SIZE; i += VMMACH_CACHE_BLOCK_SIZE) {
	VmMachFlushBlock(i);
    }

    UNLOCK_MONITOR;
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
	if (!(*ptePtr & VMMACH_RESIDENT_BIT) ||
	    (*ptePtr & VMMACH_PROTECTION_FIELD) == 0) {
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
 * Dummy function which will turn out to be the function that the debugger
 * prints out on a backtrace after a trap.  The debugger gets confused
 * because trap stacks originate from assembly language stacks.  I decided
 * to make a dummy procedure because it was to confusing seeing the
 * previous procedure (VmMach_MapKernelIntoUser) on every backtrace.
 */
VmMachTrap()
{
}
