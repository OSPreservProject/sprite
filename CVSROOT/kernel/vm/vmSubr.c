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
#include "vmMach.h"
#include "lock.h"
#include "sync.h"
#include "sys.h"
#include "list.h"
#include "dbg.h"
#include "stdlib.h"
#include "fs.h"
#include "fsio.h"
#ifdef sun4
#include "machMon.h"
#endif sun4
/*
 * Declarations of external variables
 */

Vm_Stat		vmStat;
int             vmFirstFreePage;  
Address		vmMemEnd;
Sync_Lock 	vmMonitorLock;
Sync_Lock 	vmShmLock;
int		vmShmLockCnt = 0;
int		vm_PageSize;
int		vmPageShift;
int		vmPageTableInc;
int		vmKernMemSize;
int		vmMaxProcesses = 80;
Address		vmBlockCacheBaseAddr;
Address 	vmBlockCacheEndAddr;
int		vmMaxMachSegs;
extern Vm_SharedSegTable	sharedSegTable;

Boolean		vmDebugLargeAllocs = FALSE;

/*
 * The maximum amount that a stack is allowed to grow.  We have to make it
 * real big because of the current configuration of SPUR.  This can be made
 * smaller once the exec stuff has changed.  Things are worse for the sun4
 * due to the order in which user processes try to flush their register
 * windows to a stack which hasn't been validated yet.
 */
#ifndef sun4
#define	MAX_STACK_GROWTH_SIZE	(1024 * 1024 * 2)
#else
#define	MAX_STACK_GROWTH_SIZE	(1024 * 1024 * 8)
#endif /* not sun4 */
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

    Sync_LockInitDynamic(&vmMonitorLock, "Vm:vmMonitorLock");
    Sync_LockInitDynamic(&vmShmLock, "Vm:vmShmLock");

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

    bzero((Address)vm_SysSegPtr->ptPtr, sizeof(Vm_PTE) * vm_SysSegPtr->ptSize);

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
	vmPtr = (Vm_ProcInfo *)malloc(sizeof(Vm_ProcInfo));
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
    vmPtr->sharedSegs = (List_Links *)NIL;
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
    Address 		maxAddr;
    int 		lastPage;
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    register Vm_Segment	*segPtr;

    LOCK_MONITOR;

    /*
     * We return the current end of memory as our new address.
     */
    if (numBytes > 100 * 1024) {
	printf("\nvmMemEnd = 0x%x - ", vmMemEnd);
	printf("Warning: VmRawAlloc asked for >100K\n");
	if (vmDebugLargeAllocs) {
	    Sig_SendProc(Proc_GetEffectiveProc(), SIG_DEBUG, 0);
	}
    }
    retAddr = vmMemEnd;

    /*
     * Bump the end of memory by the number of bytes that we just
     * allocated making sure that it is four byte aligned.
     */
#if defined(spur) || defined(sun4)
    retAddr = (Address) (((unsigned)retAddr + 7) & ~7);
    vmMemEnd += (numBytes + 7) & ~7;	/* eight byte aligned for SPUR. */
#else
    vmMemEnd += (numBytes + 3) & ~3;
#endif

    /*
     * Panic if we just ran off the end of memory.
     */
    if (vmMemEnd > (Address) ( mach_KernStart + vmKernMemSize)) {
	printf("vmMemEnd = 0x%x - ", vmMemEnd);
	panic("Vm_RawAlloc: Out of memory.\n");
    }

    segPtr = vm_SysSegPtr;
    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    virtAddr.flags = 0;
    lastPage = segPtr->numPages + segPtr->offset - 1;
    maxAddr = (Address) ((lastPage + 1) * vm_PageSize - 1);
    ptePtr = VmGetPTEPtr(segPtr, lastPage);

    /*
     * Add new pages to the virtual address space until we have added
     * enough to handle this memory request.  Note that we don't allow
     * VmPageAllocateInt to block if it encounters lots of dirty pages.
     */
    while (vmMemEnd - 1 > maxAddr) {
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
		panic("VmRawAlloc: No memory available\n");
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
	Fsio_StreamCopy(segPtr->filePtr, &codeFilePtr);
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
    bzero((Address) mappedAddr, vm_PageSize);
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
    Vm_SegProcList			*segProcPtr;
    register	int			page;

    LOCK_SHM_MONITOR;
    LOCK_MONITOR;

#ifdef sun4
    if (!VMMACH_ADDR_CHECK(virtAddr)) {
	transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
	UNLOCK_MONITOR;
	UNLOCK_SHM_MONITOR;
	return;
    }
#endif sun4
    transVirtAddrPtr->flags = 0;
    transVirtAddrPtr->sharedPtr = (Vm_SegProcList *) NIL;
    if (VmMach_VirtAddrParse(procPtr, virtAddr, transVirtAddrPtr)) {
	/*
	 * The hardware routine was able to translate it for us.
	 */
	UNLOCK_MONITOR;
	UNLOCK_SHM_MONITOR;
	return;
    }
	
    seg1Ptr = procPtr->vmPtr->segPtrArray[VM_HEAP];
    while (seg1Ptr->flags & VM_PT_EXCL_ACC) {
	Vm_Segment	*tSegPtr;
	/*
	 * Wait while someone has exclusive access to the page tables.
	 */
	tSegPtr = seg1Ptr;
	(void)Sync_Wait(&tSegPtr->condition, FALSE);
    }
    transVirtAddrPtr->offset = (unsigned int)virtAddr & (vm_PageSize - 1);

    page = (unsigned int) (virtAddr) >> vmPageShift;
    transVirtAddrPtr->page = page;

    if (procPtr->vmPtr->sharedSegs != (List_Links *)NIL &&
	    virtAddr >= procPtr->vmPtr->sharedStart &&
	    virtAddr < procPtr->vmPtr->sharedEnd) {
	dprintf("VmVirtAddrParse: Checking for address %x\n",virtAddr);
	segProcPtr = VmFindSharedSegment(procPtr->vmPtr->sharedSegs,virtAddr);
	if (segProcPtr != (Vm_SegProcList *)NIL) {
	    dprintf("VmVirtAddrParse: found address in seg %x\n",
		    segProcPtr->segTabPtr->segPtr->segNum);
	    transVirtAddrPtr->segPtr = segProcPtr->segTabPtr->segPtr;
	    transVirtAddrPtr->flags |= (segProcPtr->prot & VM_READONLY_SEG);
	    transVirtAddrPtr->sharedPtr = segProcPtr;
	    if (transVirtAddrPtr->flags & VM_READONLY_SEG) {
		dprintf("VmVirtAddrParse: (segment is readonly)\n");
	    }
	    UNLOCK_MONITOR;
	    UNLOCK_SHM_MONITOR;
	    return;
	}
    }

    /*
     * See if the address is too large to fit into the user's virtual
     * address space.
     */
    if (page > mach_LastUserStackPage) {
	transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
	UNLOCK_MONITOR;
	UNLOCK_SHM_MONITOR;
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
		UNLOCK_SHM_MONITOR;
		return;
	    }
	}
	transVirtAddrPtr->segPtr = seg2Ptr;
	transVirtAddrPtr->flags = VM_HEAP_PT_IN_USE;
	seg1Ptr->ptUserCount++;
	UNLOCK_MONITOR;
	UNLOCK_SHM_MONITOR;
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
	UNLOCK_SHM_MONITOR;
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
	UNLOCK_SHM_MONITOR;
	return;
    }

    /*
     * Doesn't fall in any segment so return NIL.
     */
    transVirtAddrPtr->segPtr = (Vm_Segment *) NIL;
    UNLOCK_MONITOR;
    UNLOCK_SHM_MONITOR;
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
	dprintf("VmCheckBounds: NIL failure\n");
	return(FALSE);
    }
    if (segPtr->type == VM_STACK) {
	return(virtAddrPtr->page > mach_LastUserStackPage - segPtr->numPages);
    } else {
	if (!(segPtr->ptPtr[virtAddrPtr->page - segOffset(virtAddrPtr)] &
		VM_VIRT_RES_BIT)) {
	    dprintf("VmCheckBounds: page absent failure\n");
	}
	return (segPtr->ptPtr[virtAddrPtr->page - segOffset(virtAddrPtr)] &
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
    int				numBytes;	/* The maximum number of bytes
						 * to copy in. */
    register Proc_ControlBlock	*fromProcPtr;	/* Which process to copy from.*/
    Address			fromAddr;	/* The address to copy from */
    Address			toAddr;		/* The address to copy to */
    Boolean			toKernel;	/* This copy is happening to 
						 * the kernel's address space.*/
{
    ReturnStatus		status = SUCCESS;
    Vm_VirtAddr			transVirtAddr;
    int				lastPage;
    register Proc_ControlBlock	*toProcPtr;
    register Vm_Segment		**toSegPtrArr;
    register int		genFlags;

    if (fromProcPtr->genFlags & PROC_NO_VM) {
	/*
	 * The process that we are copying from has already deleted its VM.
	 */
	return(SYS_ARG_NOACCESS);
    }
    toProcPtr = Proc_GetCurrentProc();
    if (toProcPtr->genFlags & PROC_KERNEL) {
#ifdef notdef
	if (!toKernel) {
	    panic("Vm_CopyInProc: Kernel process not copying to kernel\n");
	}
#endif

	/*
	 * We are copying to a kernel process (an rpc server process
	 * hopefully).  Since we know that the process that we are copying
	 * from can't exit until we finish this copy we can borrow
	 * its address space and then just do a normal copy in.
	 */
	toSegPtrArr = toProcPtr->vmPtr->segPtrArray;
	toSegPtrArr[VM_CODE] = fromProcPtr->vmPtr->segPtrArray[VM_CODE];
	toSegPtrArr[VM_HEAP] = fromProcPtr->vmPtr->segPtrArray[VM_HEAP];
	toSegPtrArr[VM_STACK] = fromProcPtr->vmPtr->segPtrArray[VM_STACK];
	Proc_Lock(toProcPtr);
	genFlags = toProcPtr->genFlags;
	genFlags &= ~PROC_KERNEL;
	genFlags |= PROC_USER;
	toProcPtr->genFlags = genFlags;
	Proc_Unlock(toProcPtr);
	VmMach_ReinitContext(toProcPtr);
	status = Vm_CopyIn(numBytes, fromAddr, toAddr);
	/*
	 * Change back into a kernel process.
	 */
	Proc_Lock(toProcPtr);
	genFlags = toProcPtr->genFlags;
	genFlags &= ~PROC_USER;
	genFlags |= PROC_KERNEL;
	toProcPtr->genFlags = genFlags;
	Proc_Unlock(toProcPtr);
	toSegPtrArr[VM_CODE] = (Vm_Segment *)NIL;
	toSegPtrArr[VM_HEAP] = (Vm_Segment *)NIL;
	toSegPtrArr[VM_STACK] = (Vm_Segment *)NIL;
	VmMach_ReinitContext(toProcPtr);
	return(status);
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
		segOffset(&transVirtAddr) + transVirtAddr.segPtr->numPages) {
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
    int				numBytes;	/* The maximum number of bytes
						 * to copy in. */
    Address			fromAddr;	/* The address to copy from */
    Boolean			fromKernel;	/* This copy is happening to
						 * the kernel's address space.*/
    register Proc_ControlBlock	*toProcPtr;	/* Which process to copy from.*/
    Address			toAddr;		/* The address to copy to */
{
    ReturnStatus		status = SUCCESS;
    Vm_VirtAddr			transVirtAddr;
    int				lastPage;
    register Vm_Segment		*segPtr;
    register Proc_ControlBlock	*fromProcPtr;
    register Vm_Segment		**fromSegPtrArr;
    register int		genFlags;

    if (toProcPtr->genFlags & PROC_NO_VM) {
	/*
	 * The process that we are copying to has already deleted its VM.
	 */
	return(SYS_ARG_NOACCESS);
    }
    fromProcPtr = Proc_GetCurrentProc();

    if (fromProcPtr->genFlags & PROC_KERNEL) {
#ifdef notdef
	if (!fromKernel) {
	    panic("Vm_CopyOutProc: Kernel process not copying from kernel\n");
	}
#endif

	/*
	 * We are copying to a kernel process (an rpc server process
	 * hopefully).  Since we know that the process that we are copying
	 * from can't exit until we finish this copy we can borrow
	 * its address space and then just do a normal copy in.
	 */
	fromSegPtrArr = fromProcPtr->vmPtr->segPtrArray;
	fromSegPtrArr[VM_CODE] = toProcPtr->vmPtr->segPtrArray[VM_CODE];
	fromSegPtrArr[VM_HEAP] = toProcPtr->vmPtr->segPtrArray[VM_HEAP];
	fromSegPtrArr[VM_STACK] = toProcPtr->vmPtr->segPtrArray[VM_STACK];
	Proc_Lock(fromProcPtr);
	genFlags = fromProcPtr->genFlags;
	genFlags &= ~PROC_KERNEL;
	genFlags |= PROC_USER;
	fromProcPtr->genFlags = genFlags;
	Proc_Unlock(fromProcPtr);
	VmMach_ReinitContext(fromProcPtr);
	status = Vm_CopyOut(numBytes, fromAddr, toAddr);
	/*
	 * Change back into a kernel process.
	 */
	Proc_Lock(fromProcPtr);
	genFlags = fromProcPtr->genFlags;
	genFlags &= ~PROC_USER;
	genFlags |= PROC_KERNEL;
	fromProcPtr->genFlags = genFlags;
	Proc_Unlock(fromProcPtr);
	fromSegPtrArr[VM_CODE] = (Vm_Segment *)NIL;
	fromSegPtrArr[VM_HEAP] = (Vm_Segment *)NIL;
	fromSegPtrArr[VM_STACK] = (Vm_Segment *)NIL;
	VmMach_ReinitContext(fromProcPtr);
	return(status);
    }

    if (fromProcPtr->genFlags & PROC_NO_VM) {
	/*
	 * The process that we are copying from has already deleted its VM.
	 */
	if (!fromKernel) {
	    return(SYS_ARG_NOACCESS);
	}
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
	if (lastPage >= segOffset(&transVirtAddr) + segPtr->numPages) {
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


/*
 *----------------------------------------------------------------------
 *
 * Vm_FlushCode --
 *
 *	Flush the code at the given address from the code cache.
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
Vm_FlushCode(procPtr, addr, numBytes)
    Proc_ControlBlock	*procPtr;
    Address		addr;
    int			numBytes;
{
    Vm_VirtAddr	virtAddr;
    Vm_PTE	*ptePtr;
    int		lastPage;
    int		toFlush;

    LOCK_MONITOR;

    virtAddr.segPtr = procPtr->vmPtr->segPtrArray[VM_CODE];
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    virtAddr.page = (unsigned)addr >> vmPageShift;
    virtAddr.offset = (unsigned)addr & (vm_PageSize - 1);
    virtAddr.flags = 0;
    lastPage = ((unsigned)addr + numBytes - 1) >> vmPageShift;
    if (virtAddr.page >= virtAddr.segPtr->offset && 
        lastPage < virtAddr.segPtr->offset + virtAddr.segPtr->numPages) {

	for (ptePtr = VmGetPTEPtr(virtAddr.segPtr, virtAddr.page);
	     virtAddr.page <= lastPage;
	     virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	    toFlush = vm_PageSize - virtAddr.offset;
	    if (toFlush > numBytes) {
		toFlush = numBytes;
	    }
	    if (*ptePtr & VM_PHYS_RES_BIT) {
		VmMach_FlushCode(procPtr, &virtAddr, 
			(unsigned)(*ptePtr & VM_PAGE_FRAME_FIELD), toFlush);
	    }
	    numBytes -= toFlush;
	    virtAddr.offset = 0;
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmFindSharedSegment --
 *
 *	Take the given virtual address and find which shared segment
 *	the address falls into.  If the address does not fall in any 
 *	shared segment then the segment that is returned is NIL.
 *
 * Results:
 *	The pointer to the shared segment list entry is returned,
 *	or NIL if none found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Vm_SegProcList *
VmFindSharedSegment(sharedSegs, virtAddr)
    List_Links	 		*sharedSegs;
    Address			virtAddr;
{
    Vm_SegProcList	*segLinkPtr;

    int i=0;

    /*
     * Check the shared segment list.
     */
    CHECK_SHM_MONITOR;
    VmCheckListIntegrity(sharedSegs);
    LIST_FORALL(sharedSegs, (List_Links *) segLinkPtr) {
	i++;
	if (i>20) {
	    dprintf("VmFindSharedSegment: loop!\n");
	    break;
	}
	if (segLinkPtr->mappedStart <= virtAddr &&
		virtAddr <= segLinkPtr->mappedEnd) {
	    dprintf("VmFindSharedSegment: Address is in shared segment range\n");
	    return segLinkPtr;
	} else {
	    dprintf("VmFindSharedSegment: Address %x outside %x %x\n",
		    (int)virtAddr, (int)segLinkPtr->mappedStart,
		    (int)segLinkPtr->mappedEnd);
	}
    }
    return (Vm_SegProcList *)NIL;
}

/*
 *----------------------------------------------------------------------
 *
 * Vm_CleanupSharedProc --
 *
 *	Remove a process's shared memory structures, for when the
 *	process exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Shared memory structures associated with the process are deleted.
 *	
 *
 *----------------------------------------------------------------------
 */
void
Vm_CleanupSharedProc(procPtr)
    Proc_ControlBlock	*procPtr;	/* Process that is exiting. */
{
    int i=0;
    List_Links	*sharedSegs;	/* Process's shared segments. */

    LOCK_SHM_MONITOR;
    sharedSegs = procPtr->vmPtr->sharedSegs;
    while (0 && sharedSegs != (List_Links *)NIL) {
	if (sharedSegs == (List_Links *)NULL) {
	    dprintf("Vm_CleanupSharedProc: warning: sharedSegs == NULL\n");
	    break;
	}
	i++;
	if (i>20) {
	    dprintf("Vm_CleanupSharedProc: procExit: segment loop!\n");
	    break;
	}
	if (sharedSegs==(List_Links *)NULL) {
	    printf("Vm_CleanupSharedProc: Danger: null sharedSegs list\n");
	    break;
	}
	if (List_IsEmpty(sharedSegs)) {
	    printf("Vm_CleanupSharedProc: Danger: empty sharedSegs list\n");
	    break;
	}
	if (List_First(sharedSegs)==
		(List_Links *)NULL) {
	    break;
	}
	Vm_DeleteSharedSegment(procPtr,
		(Vm_SegProcList *)List_First(sharedSegs));
    }
    UNLOCK_SHM_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Vm_DeleteSharedSegment --
 *
 *	Remove a process's mapping of a shared segment.
 *
 *      This routine removes segProcPtr from the list of shared segment
 *      mappings and frees the structure.
 *	If the process has no more references to the segment,
 *	Vm_SegmentDelete is called on the segment.  If there are no more
 *	references to the segment, it is removed from the list of shared
 *	segments.  If the process has no more shared segments, its shared
 *	segment list is freed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	References to the segment mapping are removed.  Any unneeded data
 *	structures are unlinked and freed.
 *
 *----------------------------------------------------------------------
 */
void
Vm_DeleteSharedSegment(procPtr,segProcPtr)
    Proc_ControlBlock	*procPtr;	/* Process with mapping. */
    Vm_SegProcList		*segProcPtr;	/* Pointer to segment mapping. */
{
    Vm_SharedSegTable	*segTabPtr = segProcPtr->segTabPtr;
    Vm_Segment		*segPtr;
    Vm_VirtAddr		virtAddr;
    int			done = 0;

    CHECK_SHM_MONITOR;
    VmCheckListIntegrity(procPtr->vmPtr->sharedSegs);

    LOCK_MONITOR;
    virtAddr.page = ((int)segProcPtr->mappedStart) >> vmPageShift;
    virtAddr.segPtr = segTabPtr->segPtr;
    virtAddr.sharedPtr = segProcPtr;
    UNLOCK_MONITOR;

    VmFlushSegment(&virtAddr, ((int)segProcPtr->mappedEnd+1)>>vmPageShift);
    List_Remove((List_Links *)segProcPtr);
    VmMach_SharedSegFinish(procPtr,segProcPtr->addr);
    free((Address)segProcPtr);
    VmCheckListIntegrity((List_Links *)&sharedSegTable);
    /*
     * Check if this is the process's last reference to the segment.
     */
    segPtr = segTabPtr->segPtr;
    segTabPtr->refCount--;
    if (segTabPtr->refCount == 0) {
	done = 1;
    }
    if (!VmCheckSharedSegment(procPtr,segPtr)){
	dprintf("Vm_DeleteSharedSegment: Process has no more references to segment\n");
	if (List_IsEmpty(procPtr->vmPtr->sharedSegs)) {
	    dprintf("Vm_DeleteSharedSegment: Process has no more shared segments\n");
	    VmMach_SharedProcFinish(procPtr);
	    free((Address)procPtr->vmPtr->sharedSegs);
	    procPtr->vmPtr->sharedSegs = (List_Links *)NIL;
	}
	/*
	 * Don't want Vm_SegmentDelete to destroy swap file unless we're
	 * through with it.
	 */
	Vm_SegmentDelete(segPtr,procPtr);
	if (!done) {
	    dprintf("Vm_DeleteSharedSegment: Restoring VM_SWAP_FILE_OPENED\n");
	    segPtr->flags |= VM_SWAP_FILE_OPENED;
	}
    }
    PrintSharedSegs(procPtr);
    dprintf("Vm_DeleteSharedSegment: done\n");

}

/*
 *----------------------------------------------------------------------
 *
 * VmCheckSharedSegment --
 *
 *	See if a process has the shared segment mapped.
 *
 * Results:
 *	TRUE if the shared segment is mapped by the process.
 *	FALSE otherwise.
 *
 * Side effects:
 *	Reads the shared memory data, so the SHM lock must be held.
 *
 *----------------------------------------------------------------------
 */
Boolean
VmCheckSharedSegment(procPtr,segPtr)
    Proc_ControlBlock	*procPtr;	/* Process to check. */
    Vm_Segment		*segPtr;	/* Pointer to shared segment. */
{
    Vm_SegProcList	*sharedSeg;
    Boolean 	found=FALSE;
    /*
     * Check if segment is already on the process's list.
     */
    CHECK_SHM_MONITOR;
    dprintf("VmCheckSharedSegment: Checking if segment attached to process\n");
    VmCheckListIntegrity(procPtr->vmPtr->sharedSegs);
    LIST_FORALL(procPtr->vmPtr->sharedSegs,
	    (List_Links *)sharedSeg) {
	if (sharedSeg->segTabPtr->segPtr == segPtr) {
	    found = TRUE;
	    break;
        }
    }
    if (found) {
	dprintf("it is\n");
    } else  {
	dprintf("it isn't\n");
    }

    return found;
}

/*
 *----------------------------------------------------------------------
 *
 * VmCheckListIntegrity --
 *
 *	See if a linked list is okay.
 *
 * Results:
 *	TRUE if the list is okay.
 *	FALSE otherwise.
 *
 * Side effects:
 *	Reads the shared memory data, so the SHM lock must be held.
 *
 *----------------------------------------------------------------------
 */
void
VmCheckListIntegrity(listHdr)
    List_Links	*listHdr;	/* Header of linked list. */
{
    int i=0;
    List_Links	*list1;


    /*
    CHECK_SHM_MONITOR;
    */
    if (List_Prev(listHdr) == (List_Links *)NULL) {
	panic("List_Prev is NULL!\n");
    }
    if (List_Prev(listHdr) == (List_Links *)NIL) {
	panic("List_Prev is NIL!\n");
    }
    if (List_Next(listHdr) == (List_Links *)NULL) {
	panic("List_Next is NULL!\n");
    }
    if (List_Next(listHdr) == (List_Links *)NIL) {
	panic("List_Next is NIL!\n");
    }
    if (List_IsEmpty(listHdr)) {
	return;
    }

    LIST_FORALL(listHdr,list1) {
	i++;
	if (i>10000) {
	    panic("VmCheckListIntegrity: too long\n");
	}
	if (List_Next(List_Prev(list1))!=list1) {
	    panic("VmCheckListIntegrity: error\n");
	}
	if (List_Prev(List_Next(list1))!=list1) {
	    panic("VmCheckListIntegrity: error\n");
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PrintSharedSegs --
 *
 *	Print info on the shared segments for a proc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reads the shared memory data, so the SHM lock must be held.
 *
 *----------------------------------------------------------------------
 */
int
PrintSharedSegs(procPtr)
    Proc_ControlBlock	*procPtr;	/* Process to check. */
{
    Vm_SegProcList		*procListPtr;
    Vm_SharedSegTable	*segTabPtr;

    CHECK_SHM_MONITOR;
    dprintf("PrintSharedSegs: info for %x (%x)\n",(int)procPtr,
	    (int)procPtr->processID);
    dprintf("  Shared Segment Table:\n");
    LIST_FORALL((List_Links *)&sharedSegTable,(List_Links *)segTabPtr) {
	dprintf("  entry: %x fileNumber: %d refcount: %d segPtr: %x segNum: %d\n",
		(int)segTabPtr,segTabPtr->fileNumber, segTabPtr->refCount,
		(int)segTabPtr->segPtr,segTabPtr->segPtr->segNum);
    }
    if (procPtr->vmPtr->sharedSegs == (List_Links *)NIL) {
	dprintf("  Process list: NIL\n");
    } else {
	dprintf("  Proc: %x (%x):\n",(int)procPtr,procPtr->processID);
	LIST_FORALL(procPtr->vmPtr->sharedSegs,(List_Links *)procListPtr) {
	    dprintf("  fd: %d table: %x address: %x start: %x end: %x\n",
		    (int)procListPtr->fd, procListPtr->segTabPtr,
		    (int)procListPtr->addr, (int)procListPtr->mappedStart,
		    (int)procListPtr->mappedEnd);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Vm_CleanupSharedFile --
 *
 *	Delete segments associated with a file stream.
 *	This routine calls Vm_DeleteSharedSegment on the segments
 *	associated with the file stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Shared segments are deleted.
 *	Uses the SHM lock.
 *
 *----------------------------------------------------------------------
 */
void
Vm_CleanupSharedFile(procPtr,streamPtr)
    Proc_ControlBlock	*procPtr;	/* Process with file. */
    Fs_Stream		*streamPtr;	/* Stream to remove. */
{
    Vm_SegProcList		*segPtr;
    Vm_SegProcList		*nextPtr;
    List_Links			*sharedSegs = procPtr->vmPtr->sharedSegs;

    LOCK_SHM_MONITOR;
    if (procPtr->vmPtr->sharedSegs != (List_Links *)NIL) {
	for (segPtr=(Vm_SegProcList *)List_First(sharedSegs);
		!List_IsAtEnd(sharedSegs,(List_Links *)segPtr);
		segPtr=nextPtr) {
	    nextPtr = (Vm_SegProcList *)List_Next((List_Links *)segPtr);
	    if (segPtr->stream==streamPtr) {
		dprintf("sharedSegment being deleted in Vm_CleanupSharedFile\n");
		segPtr->segTabPtr->segPtr->flags &= ~VM_SWAP_FILE_OPENED;
		Vm_DeleteSharedSegment(procPtr,segPtr);
		if (sharedSegs == (List_Links *)NIL) {
		    break;
		}
	    }
	}
    }
    UNLOCK_SHM_MONITOR;
}
