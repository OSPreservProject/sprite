/* vmPmax.c -
 *
 *     	This file contains all hardware dependent routines for the PMAX.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include <sprite.h>
#include <vm3maxConst.h>
#include <vm.h>
#include <vmInt.h>
#include <vmMach.h>
#include <vmMachInt.h>
#include <list.h>
#include <mach.h>
#include <proc.h>
#include <sched.h>
#include <stdlib.h>
#include <sync.h>
#include <sys.h>
#include <dbg.h>
#include <bstring.h>

#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#undef MASTER_LOCK
#undef MASTER_UNLOCK
#define MASTER_LOCK(x) DISABLE_INTR()
#define MASTER_UNLOCK(x) ENABLE_INTR()
#else

/*
 * The master lock to synchronize access to the tlb.
 */
static Sync_Semaphore vmMachMutex;
static Sync_Semaphore *vmMachMutexPtr = &vmMachMutex;

#endif

/*
 * The environment variables on the memory bitmap:
 */
extern char mach_BitmapLen[];
extern char mach_BitmapAddr[];

/*----------------------------------------------------------------------
 * 
 * 			Hardware data structures
 *
 *
 *----------------------------------------------------------------------
 */

/*
 * Machine dependent flags for the flags field in the Vm_VirtAddr struct.
 * We are only allowed to use the second byte of the flags.
 *
 *	USING_MAPPED_SEG		The parsed virtual address falls into
 *					the mapping segment.
 */
#define	USING_MAPPED_SEG	0x100

/*
 * The maximum amount of kernel heap available.  Sprite expects the
 * sum of this value and the kernel start to be the kernel end.
 * Since the MIPS machines have a big hole in the address space
 * we have to add 1 Gig to cover the whole.
 *
 * Right now the amount is set to 1 Gig + 16 Meg
 */
int	vmMachKernMemSize = 0x40000000 + 0x1000000;

/*
 * Table of info about each physical page on the machine.
 */
typedef struct PhysPage {
    unsigned int	user:	    1,
			referenced: 1,
			modified:   1;
} PhysPage;
PhysPage	*vmMachPhysPageArr;

/*
 * Kernel mappings for all possible kernel memory.  Each entry contains
 * the LOW tlb entry.
 */
unsigned int	*vmMach_KernelTLBMap;

/*
 * PID allocation data structures.  We need a list of free and active
 * PIDs as well as a mapping from segment to pid and back.
 */
typedef struct PIDListElem {
    List_Links		links;
    Proc_ControlBlock	*procPtr;
    int			pid;
    List_Links		tlbList;
} PIDListElem;
static PIDListElem	pidListElems[VMMACH_NUM_PIDS];

/*
 * List of free pids.
 */
static List_Links	freePIDListHdr;
static List_Links	*freePIDList = &freePIDListHdr;

/*
 * List of active pids.
 */
static List_Links	activePIDListHdr;
static List_Links	*activePIDList = &activePIDListHdr;

/*
 * Amount of physical memory.
 */
int	vm_NumPhysPages;

extern	Address	vmMemEnd;

/*
 * A TLB hash bucket.
 */
typedef struct TLBHashBucket {
    List_Links		links;		/* Links so can be in PID chain. */
    unsigned		low;		/* The TLB low register value. */
    unsigned		high;		/* The TLB high register value. */
} TLBHashBucket;

/*
 * The TLB hash table.
 */
TLBHashBucket	vmMachTLBHashTable[VMMACH_NUM_TLB_HASH_ENTRIES];

/*
 * Performance counters.  The number of fields cannot
 */
typedef struct {
    int	savedAT;
    int utlbFaultCount;
    int utlbHitCount;
    int modFaultCount;
    int modHitCount;
    int slowModCount;
    int numInserts;
    int numCollisions;
    int numProbes;
    int numFound;
} TLBCounters;
TLBCounters *tlbCountersPtr = (TLBCounters *)(0x80000000 + VMMACH_STAT_BASE_OFFSET);

/*
 * Table of TLB entries for the user has kernel pages mapped into
 * their address space.  There is only one such process (the X server).
 */
static unsigned	userMappingTable[VMMACH_USER_MAPPING_PAGES];
static int			userMapIndex;
static Boolean			userMapped = FALSE;
static Proc_ControlBlock	*mappedProcPtr = (Proc_ControlBlock *)NIL;

/*
 * Forward declarations.
 */
static int GetNumPages _ARGS_((void));
static void PageInvalidate _ARGS_((register Vm_VirtAddr *virtAddrPtr,
	unsigned int virtPage, Boolean segDeletion));
INTERNAL static void TLBHashInsert _ARGS_((int pid, unsigned page,
	unsigned lowReg, unsigned hiReg));
INTERNAL static void TLBHashDelete _ARGS_((int pid, unsigned page));
INTERNAL static void TLBHashFlushPID _ARGS_((int pid));
INTERNAL static TLBHashBucket *TLBHashFind _ARGS_((int pid, unsigned page));
static ReturnStatus VmMach_Alloc _ARGS_((VmMach_SharedData *sharedData,
	int regionSize, Address *addr));
static void VmMach_Unalloc _ARGS_((VmMach_SharedData *sharedData,
	Address addr));


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
 *      Hardware page map for the kernel is initialized.  Also the various size
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
    int			numPages;

#if (MACH_MAX_NUM_PROCESSORS > 1) /* uniprocessor implementation */
    Sync_SemInitDynamic(&vmMachMutex, "Vm:vmMachMutex");
#endif

    /*
     * Do boot time allocation.
     */
    vm_NumPhysPages = GetNumPages();
    vmMachPhysPageArr = 
	    (PhysPage *)Vm_BootAlloc(sizeof(PhysPage) * vm_NumPhysPages);
    bzero((char *)vmMachPhysPageArr, sizeof(PhysPage) * vm_NumPhysPages);

    numPages = (unsigned) (mach_KernEnd - VMMACH_VIRT_CACHED_START) >> 
						VMMACH_PAGE_SHIFT;
    vmMach_KernelTLBMap = (unsigned *)Vm_BootAlloc(sizeof(unsigned) * numPages);
    bzero((char *)vmMach_KernelTLBMap, sizeof(unsigned) * numPages);

    /*
     * Return lots of sizes to the machine independent module who called us.
     */
    *pageSizePtr = VMMACH_PAGE_SIZE;
    *pageShiftPtr = VMMACH_PAGE_SHIFT;
    *pageTableIncPtr = VMMACH_PAGE_TABLE_INCREMENT;
    *kernMemSizePtr = vmMachKernMemSize;
    *maxProcessesPtr = VMMACH_MAX_KERN_STACKS;
    *numKernPagesPtr = vm_NumPhysPages;
    *maxSegsPtr = -1;
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
static int
GetNumPages()
{
    int			i;
    int			pages;
    int			bitmapLen;
    int			*bitmapAddr;
    int			count = 0;


    sscanf(mach_BitmapLen+2,"%x",&bitmapLen);
    sscanf(mach_BitmapAddr+2,"%x",&bitmapAddr);
    for (i=0;i<bitmapLen;i++) {
	if (bitmapAddr[i] != 0xffffffff) break;
    }
    pages = i * 32;
    for (;i<bitmapLen;i++) {
	if (bitmapAddr[i] != 0x00000000) {
	    printf("Warning: Memory fragmentation at page 0x%x\n",i * 32);
	    break;
	}
    }
    printf("%d pages of memory\n", pages);
    return(pages); 
}


/*
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
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
 *     All virtual memory linked lists and arrays are initialized.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_Init(firstFreePage)
    int	firstFreePage;	/* Virtual page that is the first free for the 
			 * kernel. */
{
    register	int 		i;

    /*
     * Initialize pid lists.  0 is invalid and PID 1 is for kernel processes.
     */
    List_Init(freePIDList);
    List_Init(activePIDList);
    for (i = 2; i < VMMACH_NUM_PIDS; i++) {
	List_Insert((List_Links *)&pidListElems[i], 
		    LIST_ATREAR(freePIDList));
	pidListElems[i].pid = i;
	List_Init(&pidListElems[i].tlbList);
    }

    /*
     * Push vmMemEnd up to the beginning of dynamic memory.
     */
    vmMemEnd = (Address)VMMACH_VIRT_CACHED_START;
    vm_SysSegPtr->numPages = VMMACH_VIRT_CACHED_START_PAGE - 
				vm_SysSegPtr->offset;

    /*
     * Mark all entries in the TLB as invalid.  
     */
    VmMachFlushTLB();
    for (i = 0; i < VMMACH_FIRST_RAND_ENTRY; i++) {
	VmMachWriteIndexedTLB(i, 0, (unsigned)VMMACH_PHYS_CACHED_START_PAGE);
    }
    /*
     * Zero out the TLB fault counters which are in low memory.
     */
    bzero((char *)tlbCountersPtr, sizeof(TLBCounters));
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SegInit --
 *
 *      Initialize hardware dependent data for a segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Minimum and maximum address set in the segment table entry.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SegInit(segPtr)
    Vm_Segment	*segPtr;
{
    /*
     * Set the minimum and maximum virtual addresses for this segment to
     * be as small and as big as possible respectively because things will
     * be prevented from growing automatically as soon as segments run into
     * each other.
     */
    segPtr->minAddr = (Address)0;
    segPtr->maxAddr = (Address)0x7fffffff;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SegExpand --
 *
 *	Don't have to do anything.
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
VmMach_SegExpand(segPtr, firstPage, lastPage)
    Vm_Segment	*segPtr;	/* Segment to expand. */
    int		firstPage;	/* First page to add. */
    int		lastPage;	/* Last page to add. */
{
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SegDelete --
 *
 *      Don't have to do anything.
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
void
VmMach_SegDelete(segPtr)
    Vm_Segment	*segPtr;    /* Pointer to segment to free. */
{
}


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
    if (vmPtr->machPtr == (VmMach_ProcData *)NIL) {
	vmPtr->machPtr = (VmMach_ProcData *)malloc(sizeof(VmMach_ProcData));
    }
    vmPtr->machPtr->mapSegPtr = (struct Vm_Segment *)NIL;
    vmPtr->machPtr->pid = VMMACH_INV_PID;
    vmPtr->machPtr->sharedData.allocVector = (int *)NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SetupContext --
 *
 *      Allocate a PID to the current process.
 *	
 * Results:
 *      PID allocated.
 *
 * Side effects:
 *      PID may be moved to end of active list and an entry may be taken off
 *	of the free list.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY ClientData
VmMach_SetupContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    VmMach_ProcData	*machPtr;
    PIDListElem		*pidPtr;

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = procPtr->vmPtr->machPtr;
    if (procPtr->genFlags & (PROC_KERNEL | PROC_NO_VM)) {
	/*
	 * This is a kernel process or a process that is exiting.  These
	 * processes use the kernel's pid.
	 */
	machPtr->pid = VMMACH_KERN_PID;
	VmMachSetPID(VMMACH_KERN_PID);
	MASTER_UNLOCK(vmMachMutexPtr);
	return((ClientData)machPtr->pid);
    } 

    if (machPtr->pid == VMMACH_INV_PID) {
	/*
	 * Allocate us a new PID.
	 */
	if (List_IsEmpty(freePIDList)) {
	    vmStat.machDepStat.stealPID++;
	    pidPtr = (PIDListElem *)List_First(activePIDList);
	    pidPtr->procPtr->vmPtr->machPtr->pid = VMMACH_INV_PID;
	    VmMachFlushPIDFromTLB(pidPtr->pid);
	    TLBHashFlushPID(pidPtr->pid);
	} else {
	    pidPtr = (PIDListElem *)List_First(freePIDList);
	}
	machPtr->pid = pidPtr->pid;
	pidPtr->procPtr = procPtr;
    } else {
	pidPtr = &pidListElems[machPtr->pid];
    }
    /*
     * Move the PID to the end of the PID list and set the hardware PID
     * entry.
     */
    List_Move((List_Links *)pidPtr, LIST_ATREAR(activePIDList));
    VmMachSetPID(machPtr->pid);

    MASTER_UNLOCK(vmMachMutexPtr);
    return((ClientData)machPtr->pid);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_FreeContext --
 *
 *      Release the PID for the current process and flush the TLB for this
 *	PID.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      PID entry moved from active list to free list and TLB flushed for
 *	the PID.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_FreeContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    PIDListElem		*pidPtr;
    VmMach_ProcData	*machPtr;

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = procPtr->vmPtr->machPtr;
    if (machPtr->pid == VMMACH_INV_PID || machPtr->pid == VMMACH_KERN_PID) {
	machPtr->pid = VMMACH_INV_PID;
	MASTER_UNLOCK(vmMachMutexPtr);
	return;
    }
    pidPtr = &pidListElems[machPtr->pid];
    List_Move((List_Links *)pidPtr, LIST_ATREAR(freePIDList));
    VmMachFlushPIDFromTLB(machPtr->pid);
    TLBHashFlushPID(machPtr->pid);
    machPtr->pid = VMMACH_INV_PID;

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_ReinitContext --
 *
 *	Free the current PID and set up another one.  This is called by
 *	routines such as Proc_Exec that add things to the context and
 *	then have to abort or start a process running with a new image.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The pid value in the machine dependent VM info for the process.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_ReinitContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    VmMach_FreeContext(procPtr);
    (void)VmMach_SetupContext(procPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_VirtAddrParse --
 *
 *	See if the given address falls into the special mapping page.
 *	If so parse it for our caller.
 *
 * Results:
 *	TRUE if the address fell into the special mapping page, FALSE
 *	otherwise.
 *
 * Side effects:
 *	*transVirtAddrPtr may be filled in.
 *
 *----------------------------------------------------------------------
 */
Boolean
VmMach_VirtAddrParse(procPtr, virtAddr, transVirtAddrPtr)
    Proc_ControlBlock		*procPtr;
    Address			virtAddr;
    register	Vm_VirtAddr	*transVirtAddrPtr;
{
    VmMach_ProcData	*machPtr;

    if (virtAddr >= (Address)VMMACH_MAPPED_PAGE_ADDR) {
	machPtr = procPtr->vmPtr->machPtr;
	/*
	 * The address falls into the special mapping page.  Translate
	 * the address back to the segment that it falls into.
	 */
	transVirtAddrPtr->segPtr = machPtr->mapSegPtr;
	transVirtAddrPtr->page = machPtr->mappedPage;
	transVirtAddrPtr->offset = (unsigned)virtAddr & VMMACH_OFFSET_MASK;
	transVirtAddrPtr->flags = USING_MAPPED_SEG;
	transVirtAddrPtr->sharedPtr = (Vm_SegProcList *) machPtr->sharedPtr;
	return(TRUE);
    } else {
	return(FALSE);
    }
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
/*ARGSUSED*/
ENTRY ReturnStatus
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
    ReturnStatus		status = SUCCESS;
    register VmMach_ProcData	*machPtr;
    Proc_ControlBlock		*toProcPtr;
    int				pageOffset;
    int				bytesToCopy;

/*
    printf("VmMach_CopyInProc: num=%x from=%x toAddr=%x toK=%d\n",
		numBytes, fromAddr, toAddr, toKernel);
 */
    toProcPtr = Proc_GetCurrentProc();
    machPtr = toProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->sharedPtr = (Address) virtAddrPtr->sharedPtr;
    machPtr->mappedPage = (unsigned int) (fromAddr) >> VMMACH_PAGE_SHIFT;
    /*
     * Do a page worths at a time.
     */
    while (numBytes > 0 && status == SUCCESS) {
	pageOffset = (unsigned int)fromAddr & (VMMACH_PAGE_SIZE - 1);
	bytesToCopy = VMMACH_PAGE_SIZE - pageOffset;
	if (bytesToCopy > numBytes) {
	    bytesToCopy = numBytes;
	}
	/*
	 * Do the copy.
	 */
	toProcPtr->vmPtr->vmFlags |= VM_COPY_IN_PROGRESS;
	status = VmMachDoCopy(bytesToCopy,
			      (Address)(VMMACH_MAPPED_PAGE_ADDR + pageOffset),
			      toAddr);
	toProcPtr->vmPtr->vmFlags &= ~VM_COPY_IN_PROGRESS;

	MASTER_LOCK(vmMachMutexPtr);
	TLBHashDelete(machPtr->pid, (unsigned)VMMACH_MAPPED_PAGE_NUM);
	VmMachFlushPageFromTLB(machPtr->pid, (unsigned)VMMACH_MAPPED_PAGE_NUM);
	MASTER_UNLOCK(vmMachMutexPtr);

	if (status == SUCCESS) {
	    numBytes -= bytesToCopy;
	    fromAddr += bytesToCopy;
	    toAddr += bytesToCopy;
	} else {
	    status = SYS_ARG_NOACCESS;
	}
	machPtr->mappedPage++;
    }
    machPtr->mapSegPtr = (struct Vm_Segment *)NIL;
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
/*ARGSUSED*/
ENTRY ReturnStatus
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
    ReturnStatus		status = SUCCESS;
    register VmMach_ProcData	*machPtr;
    Proc_ControlBlock		*fromProcPtr;
    int				pageOffset;
    int				bytesToCopy;


/*
    printf("VmMach_CopyOutProc: num=%x from=%x fromK=%d toAddr=%x\n",
		numBytes, fromAddr, fromKernel, toAddr);
 */
    fromProcPtr = Proc_GetCurrentProc();
    machPtr = fromProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->mappedPage = (unsigned int) (toAddr) >> VMMACH_PAGE_SHIFT;
    machPtr->sharedPtr = (Address) virtAddrPtr->sharedPtr;
    /*
     * Do a hardware segments worth at a time until done.
     */
    while (numBytes > 0 && status == SUCCESS) {
	pageOffset = (unsigned)toAddr & (VMMACH_PAGE_SIZE - 1);
	bytesToCopy = VMMACH_PAGE_SIZE - pageOffset;
	if (bytesToCopy > numBytes) {
	    bytesToCopy = numBytes;
	}
	/*
	 * Do the copy.
	 */
	fromProcPtr->vmPtr->vmFlags |= VM_COPY_IN_PROGRESS;
	status = VmMachDoCopy(bytesToCopy, fromAddr,
			      (Address) (VMMACH_MAPPED_PAGE_ADDR + pageOffset));
	fromProcPtr->vmPtr->vmFlags &= ~VM_COPY_IN_PROGRESS;

	MASTER_LOCK(vmMachMutexPtr);
	TLBHashDelete(machPtr->pid, (unsigned)VMMACH_MAPPED_PAGE_NUM);
	VmMachFlushPageFromTLB(machPtr->pid, (unsigned)VMMACH_MAPPED_PAGE_NUM);
	MASTER_UNLOCK(vmMachMutexPtr);

	if (status == SUCCESS) {
	    numBytes -= bytesToCopy;
	    fromAddr += bytesToCopy;
	    toAddr += bytesToCopy;
	} else {
	    status = SYS_ARG_NOACCESS;
	}

	machPtr->mappedPage++;
    }
    machPtr->mapSegPtr = (struct Vm_Segment *)NIL;
    return(status);
}


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
    int				lastPage;   /* First page to set protection
					     * for. */
    Boolean			makeWriteable;/* TRUE => make the pages 
					       *	 writable.
					       * FALSE => make readable only.*/
{

    VmProcLink		*procLinkPtr;
    int			i;
    TLBHashBucket	*bucketPtr;

    MASTER_LOCK(vmMachMutexPtr);

    if (!makeWriteable || segPtr->type == VM_CODE) {
	VmMachFlushTLB();
    }

    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	for (i = firstPage; i <= lastPage; i++) {
	    bucketPtr= TLBHashFind(procLinkPtr->procPtr->vmPtr->machPtr->pid,
				   (unsigned)i);
	    if (bucketPtr != (TLBHashBucket *)NIL) {
		if (makeWriteable) {
		    bucketPtr->low |= VMMACH_TLB_ENTRY_WRITEABLE;
		} else {
		    bucketPtr->low &= ~VMMACH_TLB_ENTRY_WRITEABLE;
		}
#ifdef notdef
		if (segPtr->type == VM_CODE) {
		    bucketPtr->low |= VMMACH_TLB_NON_CACHEABLE_BIT;
		}
#endif
	    }
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_FlushCode --
 *
 *	Flush the specified address range from the instruction cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Code is flushed from the cache.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_FlushCode(procPtr, virtAddrPtr, physPage, numBytes)
    Proc_ControlBlock	*procPtr;
    Vm_VirtAddr		*virtAddrPtr;
    unsigned		physPage;
    int			numBytes;
{
    Mach_FlushCode((Address)((physPage << VMMACH_PAGE_SHIFT) +
	    virtAddrPtr->offset), (unsigned)numBytes);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SetPageProt --
 *
 *	Set the protection in hardware and software for the given virtual
 *	page.  
 *
 *	IMPORTANT: We assume that we have access to the list of processes
 *		   that are using this segment (i.e. the caller has the
 *		   virtual memory monitor lock down).
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
    Vm_Segment		*segPtr;
    VmProcLink		*procLinkPtr;
    int			pid;
    TLBHashBucket	*bucketPtr;

    MASTER_LOCK(vmMachMutexPtr);

    segPtr = virtAddrPtr->segPtr;
    if (softPTE & (VM_COW_BIT | VM_READ_ONLY_PROT)) {
	LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	    pid = procLinkPtr->procPtr->vmPtr->machPtr->pid;
	    (void) VmMachClearTLBModBit(pid, virtAddrPtr->page);
	    bucketPtr = TLBHashFind(pid, (unsigned)virtAddrPtr->page);
	    if (bucketPtr != (TLBHashBucket *)NIL) {
		bucketPtr->low &= ~VMMACH_TLB_MOD_BIT;
	    }
	    bucketPtr = TLBHashFind(pid, (unsigned)virtAddrPtr->page);
	    if (bucketPtr != (TLBHashBucket *)NIL) {
		bucketPtr->low &= ~VMMACH_TLB_ENTRY_WRITEABLE;
	    }
	}
    } else {
	LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	    bucketPtr = TLBHashFind(procLinkPtr->procPtr->vmPtr->machPtr->pid,
				    (unsigned)virtAddrPtr->page);
	    if (bucketPtr != (TLBHashBucket *)NIL) {
		bucketPtr->low |= VMMACH_TLB_ENTRY_WRITEABLE;
	    }
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
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
ENTRY void
VmMach_AllocCheck(virtAddrPtr, virtFrameNum, refPtr, modPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
    register	Boolean		*refPtr;
    register	Boolean		*modPtr;
{
    Boolean	origMod;

    MASTER_LOCK(vmMachMutexPtr);

    origMod = *modPtr;

    *refPtr |= vmMachPhysPageArr[virtFrameNum].referenced;
    *modPtr = vmMachPhysPageArr[virtFrameNum].modified;
    if (!*refPtr) {
	/*
	 * Invalidate the page so that it will force a fault if it is
	 * referenced.  Since our caller has blocked all faults on this
	 * page, by invalidating it we can guarantee that the reference and
	 * modify information that we are returning will be valid until
	 * our caller reenables faults on this page.
	 */
	PageInvalidate(virtAddrPtr, virtFrameNum, FALSE);

	if (origMod && !*modPtr) {
	    /*
	     * This page had the modify bit set in software but not in
	     * hardware.
	     */
	    vmStat.notHardModPages++;
	}
    }
    *modPtr |= origMod;

    MASTER_UNLOCK(vmMachMutexPtr);
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
    Vm_VirtAddr			*virtAddrPtr;
    unsigned	int		virtFrameNum;
    register	Boolean		*refPtr;
    register	Boolean		*modPtr;
{
    MASTER_LOCK(vmMachMutexPtr);

    *refPtr = vmMachPhysPageArr[virtFrameNum].referenced;
    *modPtr = vmMachPhysPageArr[virtFrameNum].modified;

    MASTER_UNLOCK(vmMachMutexPtr);
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
    Vm_VirtAddr		*virtAddrPtr;
    unsigned 	int	virtFrameNum;
{
    MASTER_LOCK(vmMachMutexPtr);

    vmMachPhysPageArr[virtFrameNum].referenced = 0;

    MASTER_UNLOCK(vmMachMutexPtr);
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
    register	Vm_Segment	*segPtr;
    VmProcLink			*procLinkPtr;
    int				pid;
    TLBHashBucket		*hashBucketPtr;

    MASTER_LOCK(vmMachMutexPtr);

    vmMachPhysPageArr[virtFrameNum].modified = 0;

    segPtr = virtAddrPtr->segPtr;
    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	pid = procLinkPtr->procPtr->vmPtr->machPtr->pid;
	(void) VmMachClearTLBModBit(pid, virtAddrPtr->page);
	hashBucketPtr = TLBHashFind(pid, (unsigned)virtAddrPtr->page);
	if (hashBucketPtr != (TLBHashBucket *)NIL) {
	    hashBucketPtr->low &= ~VMMACH_TLB_MOD_BIT;
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_PageValidate --
 *
 *      Validate a page for the given virtual address.  It is assumed that when
 *      this routine is called that the pid register contains the pid in which
 *	this page will be validated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The page table and hardware segment tables associated with the segment
 *      are modified to validate the page.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_PageValidate(virtAddrPtr, pte) 
    register	Vm_VirtAddr	*virtAddrPtr;
    Vm_PTE			pte;
{
    Proc_ControlBlock	*procPtr;
    VmMach_ProcData	*machPtr;
    unsigned		lowEntry;
    unsigned		virtPage;
    int			retVal;
    int			page;

    MASTER_LOCK(vmMachMutexPtr);

    procPtr = Proc_GetCurrentProc();
    machPtr = procPtr->vmPtr->machPtr;
    if (machPtr->pid == 0) {
	VmMach_ReinitContext(procPtr);
    }

    /*
     * Set up the TLB entry and the physical page info.
     */
    if (virtAddrPtr->segPtr == vm_SysSegPtr) {
	lowEntry = ((pte & VM_PAGE_FRAME_FIELD) << VMMACH_TLB_PHYS_PAGE_SHIFT)| 
			VMMACH_TLB_VALID_BIT | VMMACH_TLB_GLOBAL_BIT | 
			VMMACH_TLB_MOD_BIT;
	retVal = VmMachWriteTLB(lowEntry,
	      (unsigned) ((virtAddrPtr->page << VMMACH_TLB_VIRT_PAGE_SHIFT) | 
		       (VMMACH_KERN_PID << VMMACH_TLB_PID_SHIFT)));
	if (retVal >= 0) {
	    panic("VmMach_PageValidate: Kern TLB entry found\n");
	}
	page = virtAddrPtr->page - VMMACH_VIRT_CACHED_START_PAGE;
	if (page >= 0) {
	    vmMach_KernelTLBMap[page] = lowEntry;
	}
    } else {
	unsigned	highEntry;

	vmMachPhysPageArr[pte & VM_PAGE_FRAME_FIELD].user = 1;
	lowEntry = ((pte & VM_PAGE_FRAME_FIELD) << 
				VMMACH_TLB_PHYS_PAGE_SHIFT) | 
			VMMACH_TLB_VALID_BIT;
	if (!(pte & (VM_COW_BIT | VM_READ_ONLY_PROT)) && !(virtAddrPtr->flags
		& VM_READONLY_SEG)) {
	    lowEntry |= VMMACH_TLB_ENTRY_WRITEABLE;
	}
	if (virtAddrPtr->flags & USING_MAPPED_SEG) {
	    virtPage = VMMACH_MAPPED_PAGE_NUM;
	} else {
	    virtPage = virtAddrPtr->page;
	}
	if (machPtr->modPage == virtPage) {
	    /*
	     * We are validating after a TLB modified fault so set the modify
	     * bit.
	     */
	    lowEntry |= VMMACH_TLB_MOD_BIT;
	    vmMachPhysPageArr[pte & VM_PAGE_FRAME_FIELD].modified = 1;
	}

	highEntry = (virtPage << VMMACH_TLB_VIRT_PAGE_SHIFT) |
		    (machPtr->pid << VMMACH_TLB_PID_SHIFT);
	TLBHashInsert(machPtr->pid, virtPage, lowEntry, highEntry);
	retVal = VmMachWriteTLB(lowEntry, highEntry);
	if (retVal >= 0 && !(machPtr->modPage == virtPage)) {
	    panic("VmMach_PageValidate: Non-modified user TLB entry found\n");
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PageInvalidate --
 *
 *      Invalidate a page for the given segment.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The page table and hardware tables associated with the segment
 *      are modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
INTERNAL static void
PageInvalidate(virtAddrPtr, virtPage, segDeletion) 
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtPage;
    Boolean			segDeletion;
{
    PhysPage		*physPagePtr;

    physPagePtr = &vmMachPhysPageArr[virtPage];
    if ((physPagePtr->user && virtAddrPtr->segPtr != vm_SysSegPtr) ||
        !physPagePtr->user) {
	/*
	 * Clear out reference and modified info for this page.  We can
	 * only clear it if one of two conditions hold:
	 *
	 *	1) It has been validated in a user's address space and
	 *	   is now being invalidated from a user's address space.
	 *	2) It has never been validated in a user's address space.
	 *
	 * We have to make this distinction because a page can be mapped
	 * both by a user and the kernel at the same time and we need to
	 * make sure that the kernel invalidation doesn't wipe out good
	 * user reference and modify information.
	 */
	physPagePtr->user = 0;
	physPagePtr->referenced = 0;
	physPagePtr->modified = 0;
    }

    if (virtAddrPtr->segPtr == vm_SysSegPtr) {
	int	page;

	page = virtAddrPtr->page - VMMACH_VIRT_CACHED_START_PAGE;
	if (page >= 0) {
	    vmMach_KernelTLBMap[page] = 0;
	}
	(void) VmMachFlushPageFromTLB(VMMACH_KERN_PID, 
				      (unsigned)virtAddrPtr->page);
    } else {
	VmProcLink	*procLinkPtr;
	int		pid;

	/*
	 * Flush the page from all pids.
	 */
	LIST_FORALL(virtAddrPtr->segPtr->procList, (List_Links *) procLinkPtr) {
	    pid = procLinkPtr->procPtr->vmPtr->machPtr->pid;
	    (void) VmMachFlushPageFromTLB(pid, (unsigned)virtAddrPtr->page);
	    TLBHashDelete(pid, (unsigned)virtAddrPtr->page);
	}
	if (virtAddrPtr->segPtr->type == VM_CODE) {
	    /*
	     * If a code page flush it from the instruction cache.
	     */
	    Mach_FlushCode((Address)(virtPage << VMMACH_PAGE_SHIFT),
		    VMMACH_PAGE_SIZE);
	}
    }
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
 *      The page table and hardware segment tables associated with the segment
 *      are modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_PageInvalidate(virtAddrPtr, virtPage, segDeletion) 
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtPage;
    Boolean			segDeletion;
{
    MASTER_LOCK(vmMachMutexPtr);

    PageInvalidate(virtAddrPtr, virtPage, segDeletion);

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_PinUserPages --
 *
 *	Force a user page to be resident in memory.
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
VmMach_PinUserPages(mapType, virtAddrPtr, lastPage)
    int		mapType;
    Vm_VirtAddr	*virtAddrPtr;
    int		lastPage;
{
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
ENTRY void
VmMach_UnpinUserPages(virtAddrPtr, lastPage)
    Vm_VirtAddr	*virtAddrPtr;
    int		lastPage;
{
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_FlushPage --
 *
 *	Flush the page at the given virtual address from all caches.  We
 *	don't have to do anything on the Sun-2 and Sun-3 workstations
 *	that we have.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given page is flushed from the caches.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_FlushPage(virtAddrPtr, invalidate)
    Vm_VirtAddr	*virtAddrPtr;
    Boolean	invalidate;	/* Should invalidate the pte after flushing. */
{
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_HandleSegMigration --
 *
 *	Handle machine-dependent aspects of segment preparation for migration.
 *	There's nothing to do on this machine.
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
VmMach_HandleSegMigration(segPtr)
    Vm_Segment	*segPtr;
{
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * VmMach_Cmd --
 *
 *	Machine dependent vm commands.
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
ReturnStatus
VmMach_Cmd(command, arg)
    int	command;
    int arg;
{
    return(GEN_INVALID_ARG);
}



/*
 *----------------------------------------------------------------------
 *
 * VmMach_TLBFault --
 *
 *	Handle a TLB fault.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
VmMach_TLBFault(virtAddr)
    Address	virtAddr;
{

    if (virtAddr >= (Address)VMMACH_VIRT_CACHED_START &&
        virtAddr < (Address)VMMACH_MAPPED_PAGE_ADDR) {
	dprintf("fault 1\n");
	return(FAILURE);
    } else if (virtAddr >= (Address)VMMACH_USER_MAPPING_BASE_ADDR &&
	       virtAddr < (Address)VMMACH_PHYS_CACHED_START) {
	/*
	 * This address falls into the range of address that are used to
	 * map kernel pages into a user's address space.
	 */
	unsigned virtPage, lowEntry, highEntry;
	int pid;

	if (Proc_GetCurrentProc() != mappedProcPtr) {
	    dprintf("fault 2\n");
	    return(FAILURE);
	}
	pid = mappedProcPtr->vmPtr->machPtr->pid;
	virtPage = (unsigned)virtAddr >> VMMACH_PAGE_SHIFT;


	lowEntry = userMappingTable[virtPage - VMMACH_USER_MAPPING_BASE_PAGE];
	if (lowEntry == 0) {
	    dprintf("fault 3\n");
	    return(FAILURE);
	}
	highEntry = (virtPage << VMMACH_TLB_VIRT_PAGE_SHIFT) |
		    (pid << VMMACH_TLB_PID_SHIFT);

	MASTER_LOCK(vmMachMutexPtr);
	TLBHashInsert(pid, virtPage, lowEntry, highEntry);
	(void)VmMachWriteTLB(lowEntry, highEntry);
	MASTER_UNLOCK(vmMachMutexPtr);

	return(SUCCESS);
    } else {
	ReturnStatus status;
	status = Vm_PageIn(virtAddr, FALSE);
	if (status != SUCCESS) {
	    dprintf("fault 4\n");
	}
	return status;
	/*
	return(Vm_PageIn(virtAddr, FALSE));
	*/
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_TLBModFault --
 *
 *	Handle a TLB modify fault.
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
VmMach_TLBModFault(virtAddr)
    Address	virtAddr;
{
    Proc_ControlBlock		*procPtr;
    ReturnStatus		status;

    if (virtAddr >= (Address)VMMACH_VIRT_CACHED_START &&
	virtAddr < (Address)VMMACH_MAPPED_PAGE_ADDR) {
	/*
	 * We shouldn't get TLB modified faults for kernel virtual
	 * addresses because the modify bit was set when we validated the 
	 * page.
	 */
	panic("VmMach_TLBModFault: Kernel TLB mod fault\n");
	return(FAILURE);
    }
    tlbCountersPtr->slowModCount++;
    procPtr = Proc_GetCurrentProc();
    procPtr->vmPtr->machPtr->modPage = (unsigned)virtAddr >> VMMACH_PAGE_SHIFT;
    status = Vm_PageIn(virtAddr, TRUE);
    procPtr->vmPtr->machPtr->modPage = 0;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_MapKernelIntoUser --
 *
 *	Unimplemented on PMAX.
 *
 * Results:
 *	FAILURE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmMach_MapKernelIntoUser()
{
    return(FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_Trace --
 *
 *	Virtual memory tracing.
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
 * VmMach_MakeDebugAccessible --
 *
 *	Make the given address accessible for the debugger.
 *
 * Results:
 *	TRUE if could make accessible, FALSE if couldn't.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
VmMach_MakeDebugAccessible(addr)
    unsigned	addr;
{
    unsigned virtPage, lowEntry, highEntry;

    if (addr < (unsigned)VMMACH_VIRT_CACHED_START) {
	return(addr > (unsigned)VMMACH_PHYS_CACHED_START &&
	       addr < (unsigned)VMMACH_PHYS_CACHED_START + 
			vm_NumPhysPages * VMMACH_PAGE_SIZE);
    }
    if (addr > (unsigned)mach_KernEnd) {
	return(FALSE);
    }
    virtPage = addr >> VMMACH_PAGE_SHIFT;
    lowEntry = vmMach_KernelTLBMap[virtPage - VMMACH_VIRT_CACHED_START_PAGE];
    if (lowEntry == 0) {
	return(FALSE);
    }
    /*
     * Use the special reserved TLB entry (entry 0).
     */
    highEntry = (virtPage << VMMACH_TLB_VIRT_PAGE_SHIFT) |
		        (VMMACH_KERN_PID << VMMACH_TLB_PID_SHIFT);
    VmMachWriteIndexedTLB(0, lowEntry, highEntry);
    return(TRUE);
}

#define TLB_HASH(pid, page) \
    ((page) ^ ((pid) << (VMMACH_PID_HASH_SHIFT + VMMACH_TLB_PID_SHIFT - \
			 VMMACH_BUCKET_SIZE_SHIFT))) & VMMACH_HASH_MASK_2


/*
 *----------------------------------------------------------------------
 *
 * TLBHashFind --
 *
 *	Find the entry with the given key in the hash table.
 *
 * Results:
 *	Pointer to hash table entry if found, NIL otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static TLBHashBucket *
TLBHashFind(pid, page)
    int		pid;
    unsigned	page;
{
    TLBHashBucket	*hashBucketPtr;

    tlbCountersPtr->numProbes++;

    hashBucketPtr = &vmMachTLBHashTable[TLB_HASH(pid, page)];
    if (hashBucketPtr->high == ((page << VMMACH_TLB_VIRT_PAGE_SHIFT) |
				(pid << VMMACH_TLB_PID_SHIFT))) {
	tlbCountersPtr->numFound++;
	return(hashBucketPtr);
    } else {
	return((TLBHashBucket *)NIL);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TLBHashDelete --
 *
 *	Find the entry with the given key in the hash table and delete it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Entry may be deleted from the hash table.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static void
TLBHashDelete(pid, page)
    int		pid;
    unsigned	page;
{
    TLBHashBucket	*hashBucketPtr;

    hashBucketPtr = &vmMachTLBHashTable[TLB_HASH(pid, page)];
    if (hashBucketPtr->high == ((page << VMMACH_TLB_VIRT_PAGE_SHIFT) |
				(pid << VMMACH_TLB_PID_SHIFT))) {
	hashBucketPtr->high = 0;
	List_Remove((List_Links *)hashBucketPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TLBHashInsert --
 *
 *	Insert the entry in the hash table if it is not already there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static void
TLBHashInsert(pid, page, lowReg, hiReg)
    int		pid;
    unsigned	page;
    unsigned	lowReg;
    unsigned	hiReg;
{
    TLBHashBucket	*hashBucketPtr;

    if (pid == VMMACH_KERN_PID) {
	/*
	 * Don't insert any entries for the kernel pid.  This will only 
	 * happen because a process with no VM is doing a CopyOutProc.
	 */
	return;
    }

    tlbCountersPtr->numInserts++;
    hashBucketPtr = &vmMachTLBHashTable[TLB_HASH(pid, page)];
    if (hashBucketPtr->high == hiReg) {
	return;
    }
    if (hashBucketPtr->high != 0) {
	tlbCountersPtr->numCollisions++;
	List_Remove((List_Links *)hashBucketPtr);
    }
    hashBucketPtr->low = lowReg;
    hashBucketPtr->high = hiReg;
    List_Insert((List_Links *)hashBucketPtr,
		LIST_ATREAR(&pidListElems[pid].tlbList));
}


/*
 *----------------------------------------------------------------------
 *
 * TLBHashFlushPID --
 *
 *	Flush all entries for the given PID from the hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All entries for the given PID are flushed from the hash table.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static void
TLBHashFlushPID(pid)
    int	pid;
{
    PIDListElem		*pidPtr;
    TLBHashBucket	*hashBucketPtr;

    pidPtr = &pidListElems[pid];
    while (!List_IsEmpty(&pidPtr->tlbList)) {
	hashBucketPtr = (TLBHashBucket *)List_First(&pidPtr->tlbList);
	List_Remove((List_Links *)hashBucketPtr);
	hashBucketPtr->high = 0;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_MakeNonCacheable --
 *
 *	Make the given page non-cacheable.
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
VmMach_MakeNonCacheable(procPtr, addr)
    Proc_ControlBlock	*procPtr;
    Address		addr;
{
    TLBHashBucket	*bucketPtr;

    MASTER_LOCK(vmMachMutexPtr);

    VmMachFlushTLB();
    bucketPtr = TLBHashFind(procPtr->vmPtr->machPtr->pid,
			    (unsigned)addr >> VMMACH_PAGE_SHIFT);
    if (bucketPtr != (TLBHashBucket *)NIL) {
	bucketPtr->low |= VMMACH_TLB_NON_CACHEABLE_BIT;
    } else {
	printf("VmMach_MakeNonCacheable: Not found\n");
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_UserMap --
 *
 *	Map the given range of kernel physical addresses into the
 *	given user's virtual address space.  This is only used for the X
 *	server and thus	there can only be one user mapping at once.
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
VmMach_UserMap(numBytes, physAddr, firstTime, cache)
    int		numBytes;
    Address	physAddr;
    Boolean	firstTime;	/* TRUE => first time called. */
    Boolean	cache;		/* Should the page be cacheable?. */
{
    int		i;
    Address	retAddr;
    unsigned	firstPhysPage;
    unsigned	lastPhysPage;
    unsigned 	tlbBits;

    if (firstTime) {
	if (userMapped) {
	    return((Address)NIL);
	}
	bzero((char *)userMappingTable, sizeof(userMappingTable));
	userMapIndex = 0;
	userMapped = TRUE;
	mappedProcPtr = Proc_GetCurrentProc();
    }
    tlbBits = VMMACH_TLB_VALID_BIT | VMMACH_TLB_MOD_BIT;
    if (!cache) {
	tlbBits |= VMMACH_TLB_NON_CACHEABLE_BIT;
    }
    retAddr = (Address) (VMMACH_USER_MAPPING_BASE_ADDR +
                         userMapIndex * VMMACH_PAGE_SIZE + 
			 ((unsigned)physAddr & VMMACH_OFFSET_MASK));
    firstPhysPage = (unsigned)physAddr >> VMMACH_PAGE_SHIFT;
    lastPhysPage = (unsigned)(physAddr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    for (i = firstPhysPage; i <= lastPhysPage; i++) {
	userMappingTable[userMapIndex] = (i << VMMACH_TLB_PHYS_PAGE_SHIFT) |
			     tlbBits;
	userMapIndex++;
	if (i <= lastPhysPage && userMapIndex == VMMACH_USER_MAPPING_PAGES) {
	    return((Address)NIL);
	}
    }

    return(retAddr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_UserUnmap --
 *
 *	Unmap all pages mapped into a user's address space.
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
VmMach_UserUnmap()
{
    int			pid;
    Proc_ControlBlock	*procPtr;

    if (!userMapped) {
	return;
    }
    userMapped = FALSE;
    procPtr = Proc_GetCurrentProc();
    if (procPtr != mappedProcPtr) {
	printf("VmMach_UserUnmap: Different process is unmapping\n");
    }
    mappedProcPtr = (Proc_ControlBlock *)NIL;
    pid = procPtr->vmPtr->machPtr->pid;
    if (pid != VMMACH_INV_PID && pid != VMMACH_KERN_PID) {
	MASTER_LOCK(vmMachMutexPtr);
	TLBHashFlushPID(pid);
	MASTER_UNLOCK(vmMachMutexPtr);
    }
}


#define ALLOC(x,s)	(sharedData->allocVector[(x)]=s)
#define FREE(x)		(sharedData->allocVector[(x)]=0)
#define SIZE(x)		(sharedData->allocVector[(x)])
#define ISFREE(x)	(sharedData->allocVector[(x)]==0)



/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_Alloc --
 *
 *      Allocates a region of shared memory;
 *
 * Results:
 *      SUCCESS if the region can be allocated.
 *	The starting address is returned in addr.
 *
 * Side effects:
 *      The allocation vector is updated.
 *
 * ----------------------------------------------------------------------------
 */
static ReturnStatus
VmMach_Alloc(sharedData, regionSize, addr)
    VmMach_SharedData	*sharedData;	/* Pointer to shared memory info.  */
    int			regionSize;	/* Size of region to allocate. */
    Address		*addr;		/* Address of region. */
{
    int numBlocks = (regionSize+VMMACH_SHARED_BLOCK_SIZE-1) /
	    VMMACH_SHARED_BLOCK_SIZE;
    int i, blockCount, firstBlock;

    if (sharedData->allocVector == (int *)NULL || sharedData->allocVector ==
	    (int *)NIL) {
	dprintf("VmMach_Alloc: allocVector uninitialized!\n");
    }

    /*
     * Loop through the alloc vector until we find numBlocks free blocks
     * consecutively.
     */
    blockCount = 0;
    for (i=sharedData->allocFirstFree;
	    i<=VMMACH_SHARED_NUM_BLOCKS-1 && blockCount<numBlocks;i++) {
	if (ISFREE(i)) {
	    blockCount++;
	} else {
	    blockCount = 0;
	    if (i==sharedData->allocFirstFree) {
		sharedData->allocFirstFree++;
	    }
	}
    }
    if (blockCount < numBlocks) {
	dprintf("VmMach_Alloc: got %d blocks of %d of %d total\n",
		blockCount,numBlocks,VMMACH_SHARED_NUM_BLOCKS);
	return VM_NO_SEGMENTS;
    }
    firstBlock = i-blockCount;
    if (firstBlock == sharedData->allocFirstFree) {
	sharedData->allocFirstFree += blockCount;
    }
    *addr = (Address)(firstBlock*VMMACH_SHARED_BLOCK_SIZE +
	    VMMACH_SHARED_START_ADDR);
    for (i = firstBlock; i<firstBlock+numBlocks; i++) {
	ALLOC(i,numBlocks);
    }
    dprintf("VmMach_Alloc: got %d blocks at %d (%x)\n",
	    numBlocks,firstBlock,*addr);
    return SUCCESS;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_Unalloc --
 *
 *      Frees a region of shared address space.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The allocation vector is updated.
 *
 * ----------------------------------------------------------------------------
 */

static void
VmMach_Unalloc(sharedData, addr)
    VmMach_SharedData	*sharedData;	/* Pointer to shared memory info. */
    Address	addr;		/* Address of region. */
{
    int firstBlock = ((int)addr-VMMACH_SHARED_START_ADDR) /
	    VMMACH_SHARED_BLOCK_SIZE;
    int numBlocks = SIZE(firstBlock);
    int i;

    dprintf("VmMach_Unalloc: freeing %d blocks at %x\n",numBlocks,addr);
    if (firstBlock < sharedData->allocFirstFree) {
	sharedData->allocFirstFree = firstBlock;
    }
    for (i=0;i<numBlocks;i++) {
	if (ISFREE(i+firstBlock)) {
	    printf("Freeing free shared address %d %d %d\n",i,i+firstBlock,
		    (int)addr);
	    return;
	}
	FREE(i+firstBlock);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SharedStartAddr --
 *
 *      Determine the starting address for a shared segment.
 *
 * Results:
 *      Returns the proper start address for the segment.
 *
 * Side effects:
 *      Allocates part of the shared address space.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
VmMach_SharedStartAddr(procPtr,size,reqAddr)
    Proc_ControlBlock	*procPtr;
    int             size;           /* Length of shared segment. */
    Address         *reqAddr;        /* Requested start address. */
{
    return VmMach_Alloc(&procPtr->vmPtr->machPtr->sharedData, size, reqAddr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SharedProcStart --
 *
 *      Perform machine dependent initialization of shared memory
 *	for this process.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage allocation structures are initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SharedProcStart(procPtr)
    Proc_ControlBlock	*procPtr;
{
    VmMach_SharedData	*sharedData = &procPtr->vmPtr->machPtr->sharedData;
    dprintf("VmMach_SharedProcStart: initializing proc's allocVector\n");
    if (sharedData->allocVector != (int *)NIL) {
	panic("VmMach_SharedProcStart: allocVector not NIL\n");
    }
    sharedData->allocVector =
	    (int *)malloc(VMMACH_SHARED_NUM_BLOCKS*sizeof(int));
    sharedData->allocFirstFree = 0;
    bzero((Address) sharedData->allocVector, VMMACH_SHARED_NUM_BLOCKS*
	    sizeof(int));
    procPtr->vmPtr->sharedStart = (Address) VMMACH_SHARED_START_ADDR;
    procPtr->vmPtr->sharedEnd = (Address) VMMACH_SHARED_START_ADDR+
	    VMMACH_USER_SHARED_PAGES*VMMACH_PAGE_SIZE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SharedSegFinish --
 *
 *      Perform machine dependent cleanup of shared memory
 *	for this segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage allocation structures are freed.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SharedSegFinish(procPtr,addr)
    Proc_ControlBlock	*procPtr;
    Address		addr;
{
    VmMach_Unalloc(&procPtr->vmPtr->machPtr->sharedData,addr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SharedProcFinish --
 *
 *      Perform machine dependent cleanup of shared memory
 *	for this process.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The storage allocation structures are freed.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SharedProcFinish(procPtr)
    Proc_ControlBlock	*procPtr;
{
    dprintf("VmMach_SharedProcFinish: freeing process's allocVector\n");
    free((Address)procPtr->vmPtr->machPtr->sharedData.allocVector);
    procPtr->vmPtr->machPtr->sharedData.allocVector;
    procPtr->vmPtr->machPtr->sharedData.allocVector = (int *)NIL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_CopySharedMem --
 *
 *      Copies machine-dependent shared memory data structures to handle
 *	a fork.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The new process gets a copy of the shared memory structures.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_CopySharedMem(parentProcPtr, childProcPtr)
    Proc_ControlBlock   *parentProcPtr; /* Parent process. */
    Proc_ControlBlock   *childProcPtr;  /* Child process. */
{
    VmMach_SharedData	*childSharedData =
	    &childProcPtr->vmPtr->machPtr->sharedData;
    VmMach_SharedData	*parentSharedData =
	    &parentProcPtr->vmPtr->machPtr->sharedData;

    VmMach_SharedProcStart(childProcPtr);

    bcopy(parentSharedData->allocVector, childSharedData->allocVector,
	    VMMACH_SHARED_NUM_BLOCKS*sizeof(int));
    childSharedData->allocFirstFree = parentSharedData->allocFirstFree;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_LockCachePage --
 *
 *      Perform machine dependent locking of a kernel resident file cache
 *	page.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_LockCachePage(kernelAddress)
    Address	kernelAddress;	/* Address on page to lock. */
{
    /*
     * Ds3100 leaves file cache pages always available so there is no need to
     * lock or unlock them.
     */
    return;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_UnlockCachePage --
 *
 *      Perform machine dependent unlocking of a kernel resident page.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_UnlockCachePage(kernelAddress)
    Address	kernelAddress;	/* Address on page to unlock. */
{
    /*
     * Ds3100 leaves file cache pages always available so there is no need to
     * lock or unlock them.
     */
    return;
}

