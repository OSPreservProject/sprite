/* vmSubr.c -
 *
 *     This file contains miscellaneous virtual memory routines.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vm.h"
#include "vmInt.h"
#include "vmTrace.h"
#include "lock.h"
#include "sync.h"
#include "sys.h"
#include "list.h"
#include "byte.h"
#include "dbg.h"
#include "mem.h"

/*
 * Declarations of external variables
 */

Vm_Stat		vmStat;
int             vmFirstFreePage;  
Address		vmMemEnd;
Sync_Lock 	vmMonitorLock = {0, 0};
int		vm_PageSize;
int		vmPageShift;
int		vmPageTableInc;
int		vmKernMemSize;
int		vmMaxProcesses = 80;
Address		vmBlockCacheBaseAddr;
Address 	vmBlockCacheEndAddr;
int		vmMaxMachSegs;

Boolean		vmDebugLargeAllocs = FALSE;

/*
 * The maximum amount that a stack is allowed to grow.  We have to make it
 * real big because of the current configuration of SPUR.  This can be made
 * smaller once the exec stuff has changed.
 */
#define	MAX_STACK_GROWTH_SIZE	(1024 * 1024 * 2)
int		vmMaxStackPagesGrowth;

/*
 * ----------------------------------------------------------------------------
 *
 * Vm_Init --
 *
 *     Initialize all virtual memory data structures.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All virtual memory linked lists and arrays are initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_Init()
{
    register	Vm_PTE	*ptePtr;
    int			i;
#ifdef notdef
    unsigned int	virtPage;
#endif
    /*
     * Set up the maximum number of pages that a stack can grow.
     */
    vmMaxStackPagesGrowth = MAX_STACK_GROWTH_SIZE / vm_PageSize;
    /*
     * Partition up the kernel virtual address space.
     */
    vmStackBaseAddr = (Address) (mach_KernStart + vmKernMemSize);
    vmStackEndAddr = vmStackBaseAddr + mach_KernStackSize * vmMaxProcesses;
    vmMapBaseAddr = vmStackEndAddr;
    vmMapBasePage = (unsigned int)vmMapBaseAddr / vm_PageSize;
    vmMapEndAddr = vmMapBaseAddr + vmNumMappedPages * vm_PageSize;
    vmMapEndPage = vmMapBasePage + vmNumMappedPages;
    vmBlockCacheBaseAddr = VmMach_AllocKernSpace(vmMapEndAddr);
    vmBlockCacheEndAddr = (Address)mach_KernEnd;
    /*
     * Allocate the segment table and core map.
     */
    VmSegTableAlloc();
    VmCoreMapAlloc();
    /*
     * Initialize the structure for kernel stacks.
     */
    VmStackInit();
    /*
     * Allocate and initialize the kernel page table.
     */
    vm_SysSegPtr->ptSize = (mach_KernEnd - mach_KernStart) / vm_PageSize;
    vm_SysSegPtr->ptPtr =
		(Vm_PTE *)Vm_BootAlloc(sizeof(Vm_PTE) * vm_SysSegPtr->ptSize);
    Byte_Zero(sizeof(Vm_PTE) * vm_SysSegPtr->ptSize,
	      (Address)vm_SysSegPtr->ptPtr);
    /*
     * Can no longer use Vm_BootAlloc
     */
    vmNoBootAlloc = TRUE;
    /* 
     * Determine how many physical pages that we have used.
     */
    vmFirstFreePage = 
	(unsigned int)(vmMemEnd - mach_KernStart - 1) / vm_PageSize + 1;

    for (i = 0, ptePtr = vm_SysSegPtr->ptPtr;
	 i < vmFirstFreePage;
	 i++, ptePtr++) {
	*ptePtr = VM_VIRT_RES_BIT | VM_PHYS_RES_BIT | i;
    }
    /*
     * Initialize the segment table and core map.
     */
    VmSegTableInit();
    VmCoreMapInit();
#ifdef notdef
    /*
     * Take away the page at the bottom of the kernel stack.
     */
    virtPage = (mach_StackBottom - mach_KernStart) >> vmPageShift;
    vm_SysSegPtr->ptPtr[virtPage] = 0;
    VmPutOnFreePageList(virtPage);
#endif
    /*
     * Now call the hardware dependent initialization routine.
     */
    VmMach_Init(vmFirstFreePage);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_ProcInit --
 *
 *     Initialize VM info for this process.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Virtual memory information for the given process is initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_ProcInit(procPtr)
    Proc_ControlBlock	*procPtr;
{
    int				i;
    register	Vm_ProcInfo	*vmPtr;

    if (procPtr->vmPtr == (Vm_ProcInfo *)NIL) {
	vmPtr = (Vm_ProcInfo *)Mem_Alloc(sizeof(Vm_ProcInfo));
	vmPtr->machPtr = (VmMach_ProcData *)NIL;
	procPtr->vmPtr = vmPtr;
    } else {
	vmPtr = procPtr->vmPtr;
    }
    for (i = 0; i < VM_NUM_SEGMENTS; i++) {
	vmPtr->segPtrArray[i] = (Vm_Segment *)NIL;
    }
    vmPtr->vmFlags = 0;
    vmPtr->numMakeAcc = 0;
    VmMach_ProcInit(vmPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_RawAlloc --
 *
 *	Allocate bytes of memory.
 *
 * Results:
 *	Pointer to beginning of memory allocated..
 *
 * Side effects:
 *	Variable that indicates the end of kernel memory is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY Address
Vm_RawAlloc(numBytes)
{
    Address 		retAddr;
    int 		maxAddr;
    int 		lastPage;
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    register Vm_Segment	*segPtr;

    LOCK_MONITOR;

    /*
     * We return the current end of memory as our new address.
     */
    if (numBytes > 100 * 1024) {
	Sys_Printf("\nvmMemEnd = 0x%x - ", vmMemEnd);
	Sys_Panic(SYS_WARNING, "VmRawAlloc asked for >100K\n");
	if (vmDebugLargeAllocs) {
	    Sig_SendProc(Proc_GetEffectiveProc(), SIG_DEBUG, 0);
	}
    }
    retAddr = vmMemEnd;

    /*
     * Bump the end of memory by the number of bytes that we just
     * allocated making sure that it is four byte aligned.
     */
#ifdef SPUR
    retAddr = (Address) (((unsigned)retAddr + 7) & ~7);
    vmMemEnd += (numBytes + 7) & ~7;	/* eight byte aligned for SPUR. */
#else
    vmMemEnd += (numBytes + 3) & ~3;
#endif

    /*
     * Panic if we just ran off the end of memory.
     */
    if (vmMemEnd > (Address) ( mach_KernStart + vmKernMemSize)) {
	Sys_Printf("vmMemEnd = 0x%x - ", vmMemEnd);
	Sys_Panic(SYS_FATAL, "Vm_RawAlloc: Out of memory.\n");
    }

    segPtr = vm_SysSegPtr;
    virtAddr.segPtr = segPtr;
    lastPage = segPtr->numPages + segPtr->offset - 1;
    maxAddr = (lastPage + 1) * vm_PageSize - 1;
    ptePtr = VmGetPTEPtr(segPtr, lastPage);

    /*
     * Add new pages to the virtual address space until we have added
     * enough to handle this memory request.  Note that we don't allow
     * VmPageAllocateInt to block if it encounters lots of dirty pages.
     */
    while ((int) (vmMemEnd) - 1 > maxAddr) {
	int	page;

	maxAddr += vm_PageSize;
	lastPage++;
	VmIncPTEPtr(ptePtr, 1);
	virtAddr.page = lastPage;
	virtAddr.offset = 0;
	page = VmPageAllocateInt(&virtAddr, 0);
	if (page == VM_NO_MEM_VAL) {
	    /*
	     * The normal page allocation mechanism failed so go to the
	     * list of pages that are held in reserve for just such an
	     * occasion.
	     */
	    page = VmGetReservePage(&virtAddr);
	    if (page == VM_NO_MEM_VAL) {
		Sys_Panic(SYS_FATAL, "VmRawAlloc: No memory available\n");
	    }
	}
	*ptePtr |= page;
	VmPageValidateInt(&virtAddr, ptePtr);
	segPtr->numPages++;
    }

    UNLOCK_MONITOR;

    return(retAddr);
}

void ChangeCodeProt();


/*
 *----------------------------------------------------------------------
 *
 * Vm_ChangeCodeProt --
 *
 *	Change the protection of the code segment for the given process.  If
 *	the process still has the shared code segment then make a new 
 *	copy.
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
Vm_ChangeCodeProt(procPtr, startAddr, numBytes, makeWriteable)
    Proc_ControlBlock		*procPtr;	/* Process to change code
						 * protection for. */
    Address		       	startAddr;	/* Beginning address of range
						 * of bytes to change
						 * protection.*/
    int			       	numBytes;	/* Number of bytes to change
						 * protection for. */
    Boolean			makeWriteable;	/* TRUE => make the pages 
					         *	 writable.
					         * FALSE => make readable only*/
{
    register	Vm_Segment	*segPtr;
    Vm_Segment	*newSegPtr;
    Fs_Stream			*codeFilePtr;

    segPtr = procPtr->vmPtr->segPtrArray[VM_CODE];
    if (!(segPtr->flags & VM_DEBUGGED_SEG)) {
	/*
	 * This process still has a hold of the original shared code 
	 * segment.  Make a new segment for the process.
	 */
	(void)Fs_StreamCopy(segPtr->filePtr, &codeFilePtr);
	newSegPtr = Vm_SegmentNew(VM_CODE, codeFilePtr, segPtr->fileAddr, 
				  segPtr->numPages, segPtr->offset, procPtr);
	Vm_ValidatePages(newSegPtr, newSegPtr->offset, 
			 newSegPtr->offset + newSegPtr->numPages - 1,
			 FALSE, TRUE);
    } else {
	newSegPtr = (Vm_Segment *)NIL;
    }
    ChangeCodeProt(procPtr, &newSegPtr, startAddr, numBytes, makeWriteable);
    if (newSegPtr != (Vm_Segment *)NIL) {
	Vm_SegmentDelete(newSegPtr, procPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ChangeCodeProt --
 *
 *	Change the protection of the code segment for the given process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ChangeCodeProt(procPtr, segPtrPtr, startAddr, numBytes, makeWriteable)
    Proc_ControlBlock		*procPtr;	/* Process to change protection
						 * for. */
    Vm_Segment			**segPtrPtr;	/* IN:  New duplicated segment
						 * OUT: Segment to free if 
						 *      non-NIL. */
    Address		       	startAddr;	/* Beginning address of range
						 * of bytes to change
						 * protection.*/
    int			       	numBytes;	/* Number of bytes to change
						 * protection for. */
    Boolean			makeWriteable;	/* TRUE => make the pages 
					         *	 writable.
					         * FALSE => make readable only*/
{
    int				firstPage;
    int				lastPage;
    int				i;
    register	Vm_PTE		*ptePtr;
    register	Vm_Segment	*segPtr;

    LOCK_MONITOR;

    segPtr = procPtr->vmPtr->segPtrArray[VM_CODE];
    if (!(segPtr->flags & VM_DEBUGGED_SEG)) {
	/*
	 * This process is currently using the shared code segment.  Use the
	 * private copy that our caller allocated for us and return the 
	 * original segment so our caller can release its reference to it.
	 */
	segPtr = *segPtrPtr;
	segPtr->flags |= VM_DEBUGGED_SEG;
	*segPtrPtr = procPtr->vmPtr->segPtrArray[VM_CODE];
	procPtr->vmPtr->segPtrArray[VM_CODE] = segPtr;
	/*
	 * Free up the hardware context for this process.  When it starts
	 * running again new context will be setup which will have
	 * the new code segment in it.
	 */
	VmMach_FreeContext(procPtr);
    }

    firstPage = (unsigned int) startAddr >> vmPageShift;
    lastPage = ((unsigned int) (startAddr) + numBytes - 1) >> vmPageShift;
    /* 
     * Make sure that the range of addresses falls into the code segment's 
     * page table.  If not don't do anything.
     */
    if (firstPage >= segPtr->offset &&
	lastPage < segPtr->offset + segPtr->ptSize) {
	for (i = lastPage - firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	     i >= 0;
	     i--, VmIncPTEPtr(ptePtr, 1)) {
	    if (makeWriteable) {
		*ptePtr &= ~VM_READ_ONLY_PROT;
	    } else {
		*ptePtr |= VM_READ_ONLY_PROT;
	    }
	}
	VmMach_SetSegProt(segPtr, firstPage, lastPage, makeWriteable);
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_ValidatePages --
 *
 *	Initialize the page table for the given segment.  This involves
 *	going through the software page table in the range of pages given.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table modified for the given segment.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Vm_ValidatePages(segPtr, firstPage, lastPage, zeroFill, clobber)
    Vm_Segment 	*segPtr;	/* The segment whose pages are being 
				 * made valid. */
    int		firstPage;	/* The first page to mark valid. */
    int		lastPage;	/* The last page to mark valid. */
    Boolean	zeroFill;	/* Should mark pages zero fill. */
    Boolean	clobber;	/* TRUE -> overwrite the pte no matter what.
				 * FALSE -> only overwrite if the pte is not
				 *	    marked as valid in this segment's
				 *	    virtual address space. */
{
    LOCK_MONITOR;

    VmValidatePagesInt(segPtr, firstPage, lastPage, zeroFill, clobber);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmValidatePagesInt --
 *
 *	Mark as virtually resident the range of pages in the page table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table modified for the given segment.
 *
 *----------------------------------------------------------------------
 */
INTERNAL void
VmValidatePagesInt(segPtr,  firstPage, lastPage, zeroFill, clobber)
    Vm_Segment 	*segPtr;	/* The segment whose page table is being 
				 * initialized. */
    int		firstPage;	/* The first pte to be initialized */
    int		lastPage;	/* The last pte to be initialized */
    Boolean	zeroFill;	/* TRUE => Mark the page as zero fill. */
    Boolean	clobber;	/* TRUE -> overwrite the pte no matter what.
				 * FALSE -> only overwrite if the pte is not
				 *	    marked as valid in this segment's
				 *	    virtual address space. */
{
    register	int	i;
    register	Vm_PTE	pte;
    register	Vm_PTE	*ptePtr;

    if (vm_Tracing && !(segPtr->flags & VM_SEG_CREATE_TRACED)) {
	Vm_TraceSegCreate	segCreate;

	segCreate.segNum = segPtr->segNum;
	segCreate.parSegNum = -1;
	segCreate.segType = segPtr->type;
	segCreate.cor = FALSE;
	VmStoreTraceRec(VM_TRACE_SEG_CREATE_REC, sizeof(segCreate),
			(Address)&segCreate, TRUE);
	segPtr->flags |= VM_SEG_CREATE_TRACED;
    }

    pte = VM_VIRT_RES_BIT;
    if (segPtr->type == VM_CODE) {
	pte |= VM_READ_ONLY_PROT;
    } else if (zeroFill) {
	pte |= VM_ZERO_FILL_BIT;
    }
    for (i = firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 i <= lastPage;
	 i++, ptePtr++) {
	if (clobber || !(*ptePtr & VM_VIRT_RES_BIT)) {
	    *ptePtr = pte;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmZeroPage --
 *
 *	External routine to fill the entire given page frame with zeroes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The page is filled with zeroes.
 *
 *----------------------------------------------------------------------
 */
void
VmZeroPage(pfNum)
    unsigned int	pfNum;
{
    register	int	mappedAddr;

    mappedAddr = (int) VmMapPage(pfNum);
    Byte_Zero(vm_PageSize, (Address) mappedAddr);
    VmUnmapPage((Address) mappedAddr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmVirtAddrParse --
 *
 *	Take the given virtual address and fill in a virtual address struct
 *	with the segment, page, and offset for this address.  If it is 
 *	determined in this routine that the address does not fall in any 
 *	segment then the segment that is returned is NIL.
 *
 * Results:
 *	The translated virtual address.
 *
 * Side effects:
 *	If the virtual address falls into a stack or heap segment then the
 *	heap segment for the process is prevented from being expanded.  This
 *	is to prevent another process that is sharing the heap segment from
 *	changing its size and making the parsed virtual address wrong.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmVirtAddrParse(procPtr, virtAddr, transVirtAddrPtr)
    Proc_ControlBlock		*procPtr;
    Address			virtAddr;
    register	Vm_VirtAddr	*transVirtAddrPtr;
{
    register	Vm_Segment		*seg1Ptr;
    register	Vm_Segment		*seg2Ptr;
    register	int			page;

    LOCK_MONITOR;

    seg1Ptr = procPtr->vmPtr->segPtrArray[VM_HEAP];
    while (seg1Ptr->flags & VM_PT_EXCL_ACC) {
	Vm_Segment	*tSegPtr;
	/*
	 * Wait while someone has exclusive access to the page tables.
	 */
	tSegPtr = seg1Ptr;
	(void)Sync_Wait(&tSegPtr->condition, FALSE);
    }
    transVirtAddrPtr->flags = 0;
    if (VmMach_VirtAddrParse(procPtr, virtAddr, transVirtAddrPtr)) {
	/*
	 * The hardware routine was able to translate it for us.
	 */
	UNLOCK_MONITOR;
	return;
    }

    transVirtAddrPtr->offset = (unsigned int)virtAddr & (vm_PageSize - 1);

    page = (unsigned int) (virtAddr) >> vmPageShift;
    transVirtAddrPtr->page = page;

    /*
     * See if the address is too large to fit into the user's virtual
     * address space.
     */
    if (page > mach_LastUserStackPage) {
	transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
	UNLOCK_MONITOR;
	return;
    }
    seg2Ptr = procPtr->vmPtr->segPtrArray[VM_STACK];
    /*
     * Check the stack segment.  Anything past the end of the heap segment 
     * falls into the stack segment.  Since page tables are not allowed to
     * overlap, the end of the heap segment is defined to be the end of
     * the heap page table.  If it falls in the stack segment then prevent
     * this process's heap segment from being expanded by incrementing the
     * in use count on the page table.
     */
    if (page > seg1Ptr->ptSize + seg1Ptr->offset) {
	if (page < seg2Ptr->offset) {
	    int	newPTSize;
	    newPTSize = ((mach_LastUserStackPage - page)/vmPageTableInc + 1) * 
							    vmPageTableInc;
	    /* 
	     * We are going to have to grow the stack to cover this so
	     * make sure that the heap and stack segments don't overlap and
	     * we aren't trying to grow too much.
	     */
	    if ((Address) (page << vmPageShift) < seg2Ptr->minAddr ||
	        seg2Ptr->offset - page > vmMaxStackPagesGrowth ||
	        seg1Ptr->offset + seg1Ptr->ptSize >=
		     mach_LastUserStackPage - newPTSize + 1) {
		transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
		UNLOCK_MONITOR;
		return;
	    }
	}
	transVirtAddrPtr->segPtr = seg2Ptr;
	transVirtAddrPtr->flags = VM_HEAP_PT_IN_USE;
	seg1Ptr->ptUserCount++;
	UNLOCK_MONITOR;
	return;
    }
    /* 
     * Check the heap segment.  If it falls in the heap segment then prevent
     * the segment from being expanded.
     */
    if (page >= seg1Ptr->offset && 
	page < (seg1Ptr->offset + seg1Ptr->numPages)) {
	transVirtAddrPtr->segPtr = seg1Ptr;
	transVirtAddrPtr->flags = VM_HEAP_PT_IN_USE;
	seg1Ptr->ptUserCount++;
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Check the code segment.
     */
    seg1Ptr = procPtr->vmPtr->segPtrArray[VM_CODE];
    if (page >= seg1Ptr->offset &&
        page < (seg1Ptr->offset + seg1Ptr->numPages)) {
	transVirtAddrPtr->segPtr = seg1Ptr;
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Doesn't fall in any segment so return NIL.
     */
    transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmCheckBounds --
 *
 *	See if the given virtual address falls within the bounds of the
 *	segment.  It is assumed that this segment is prevented from being
 *	expanded.
 *
 * Results:
 *	TRUE if the virtual address is in bounds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
VmCheckBounds(virtAddrPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
{
    register	Vm_Segment	*segPtr;

    segPtr = virtAddrPtr->segPtr;
    if (segPtr == (Vm_Segment *) NIL) {
	return(FALSE);
    }
    if (segPtr->type == VM_STACK) {
	return(virtAddrPtr->page > mach_LastUserStackPage - segPtr->numPages);
    } else {
	return (segPtr->ptPtr[virtAddrPtr->page - segPtr->offset] & 
							    VM_VIRT_RES_BIT);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyInProc --
 *
 *	Copy from another processes address space into the current address
 *	space. It assumed that this routine is called with the source 
 *	process locked such that its VM will not go away while we are doing
 *	this copy.
 *
 * Results:
 *	SUCCESS if the copy succeeded, SYS_ARG_NOACCESS if fromAddr is invalid.
 *
 * Side effects:
 *	What toAddr points to is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_CopyInProc(numBytes, fromProcPtr, fromAddr, toAddr, toKernel)
    int 	numBytes;		/* The maximum number of bytes to 
					   copy in. */
    Proc_ControlBlock	*fromProcPtr;	/* Which process to copy from.*/
    Address	fromAddr;		/* The address to copy from */
    Address	toAddr;			/* The address to copy to */
    Boolean	toKernel;		/* This copy is happening to the
					 * kernel's address space. */
{
    ReturnStatus		status = SUCCESS;
    Vm_VirtAddr			transVirtAddr;
    int				lastPage;

    if (fromProcPtr->genFlags & PROC_NO_VM) {
	/*
	 * The process that we are copying from has already deleted its VM.
	 */
	return(SYS_ARG_NOACCESS);
    }
    if (!toKernel && (toAddr < mach_FirstUserAddr ||
                      toAddr > mach_LastUserAddr ||
		      toAddr + numBytes - 1 > mach_LastUserAddr)) {
	/*
	 * The dest address is definitely not in this user process's address
	 * space.
	 */
	return(SYS_ARG_NOACCESS);
    }
    /*
     * Determine which segment the address falls into.
     */
    VmVirtAddrParse(fromProcPtr, fromAddr, &transVirtAddr);
    if (transVirtAddr.segPtr == (Vm_Segment *)NIL) {
	return(SYS_ARG_NOACCESS);
    }
    /*
     * We now have the segment that the first address falls into, now make
     * sure that the end address falls in there as well.
     */
    lastPage = ((unsigned int)fromAddr + numBytes - 1) / vm_PageSize;
    if (transVirtAddr.segPtr->type == VM_STACK) {
	if (lastPage > mach_LastUserStackPage) {
	    status = SYS_ARG_NOACCESS;
	    goto exit;
	}
    } else {
	if (lastPage >= 
		transVirtAddr.segPtr->offset + transVirtAddr.segPtr->numPages) {
	    status = SYS_ARG_NOACCESS;
	    goto exit;
	}
    }
    /*
     * Call the hardware dependent routine to do the copy.
     */
    status = VmMach_CopyInProc(numBytes, fromProcPtr, fromAddr,
                               &transVirtAddr, toAddr, toKernel);

exit:
    /*
     * If the source segment was a stack or heap segment then the heap
     * segment was prevented from being expanded.  Let it be expanded now.
     */
    if (transVirtAddr.flags & VM_HEAP_PT_IN_USE) {
	VmDecPTUserCount(fromProcPtr->vmPtr->segPtrArray[VM_HEAP]);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyOutProc --
 *
 *	Copy from the current VAS to another processes VAS.  It assumed that
 *	this routine is called with the dest process locked such that its 
 *	VM will not go away while we are doing the copy.
 *
 * Results:
 *	SUCCESS if the copy succeeded, SYS_ARG_NOACCESS if fromAddr is invalid.
 *
 * Side effects:
 *	What toAddr points to is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_CopyOutProc(numBytes, fromAddr, fromKernel, toProcPtr, toAddr)
    int 	numBytes;		/* The maximum number of bytes to 
					 * copy in. */
    Address	fromAddr;		/* The address to copy from */
    Boolean	fromKernel;		/* This copy is happening to the
					 * kernel's address space. */
    Proc_ControlBlock	*toProcPtr;	/* Which process to copy from.*/
    Address	toAddr;			/* The address to copy to */
{
    ReturnStatus		status = SUCCESS;
    Vm_VirtAddr			transVirtAddr;
    int				lastPage;
    register	Vm_Segment	*segPtr;

    if (toProcPtr->genFlags & PROC_NO_VM) {
	/*
	 * The process that we are copying from has already deleted its VM.
	 */
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Determine which segment the address falls into.
     */
    VmVirtAddrParse(toProcPtr, toAddr, &transVirtAddr);
    if (transVirtAddr.segPtr == (Vm_Segment *)NIL) {
	return(SYS_ARG_NOACCESS);
    }
    segPtr = transVirtAddr.segPtr;
    /*
     * We now have the segment that the first address falls into, now make
     * sure that the end address falls in there as well.
     */
    lastPage = ((unsigned int)toAddr + numBytes - 1) / vm_PageSize;
    if (segPtr->type == VM_STACK) {
	if (lastPage > mach_LastUserStackPage) {
	    status = SYS_ARG_NOACCESS;
	    goto exit;
	}
    } else {
	if (lastPage >= segPtr->offset + segPtr->numPages) {
	    status = SYS_ARG_NOACCESS;
	    goto exit;
	}
    }
    status = VmMach_CopyOutProc(numBytes, fromAddr, fromKernel, toProcPtr, 
				toAddr, &transVirtAddr);

exit:
    /*
     * If the dest segment was a stack or heap segment then the heap
     * segment was prevented from being expanded.  Let it be expanded now.
     */
    if (transVirtAddr.flags & VM_HEAP_PT_IN_USE) {
	VmDecPTUserCount(toProcPtr->vmPtr->segPtrArray[VM_HEAP]);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetKernPageFrame --
 *
 *	Return the kernel virtual page frame that is valid at the given virtual
 *	page number.  Intended to be used by the hardware specific module.
 *
 * Results:
 *	Kernel page from the page table entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
unsigned int
Vm_GetKernPageFrame(pageFrame)
    int	pageFrame;
{
    Vm_PTE	*ptePtr;
    ptePtr = VmGetPTEPtr(vm_SysSegPtr, pageFrame);
    return(Vm_GetPageFrame(*ptePtr));
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_KernPageAllocate --
 *
 *	Return a physical page frame.  Intended to be used by the hardware
 *	specific module.
 *
 * Results:
 *	Virtual page frame.
 *
 * Side effects:
 *	Page is taken out of the page pool.
 *
 *----------------------------------------------------------------------
 */
unsigned int
Vm_KernPageAllocate()
{
    Vm_VirtAddr	virtAddr;

    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = 0;
    return(VmPageAllocate(&virtAddr, VM_CAN_BLOCK));
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_KernPageFree --
 *
 *	Free the page frame that was returned from Vm_KernPageAlloc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page freed.
 *
 *----------------------------------------------------------------------
 */
void
Vm_KernPageFree(pfNum)
    unsigned	int	pfNum;
{
    VmPageFree(pfNum);
}
