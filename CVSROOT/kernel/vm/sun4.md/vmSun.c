/*
 * vmSun.c -
 *
 *     	This file contains all hardware-dependent vm C routines for Sun2's, 3's 
 *	and 4's.  I will not attempt to explain the Sun mapping hardware in 
 *	here.  See the sun architecture manuals for details on
 *	the mapping hardware.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <vmSunConst.h>
#include <machMon.h>
#include <vm.h>
#include <vmInt.h>
#include <vmMach.h>
#include <vmMachInt.h>
#include <vmTrace.h>
#include <list.h>
#include <mach.h>
#include <proc.h>
#include <sched.h>
#include <stdlib.h>
#include <sync.h>
#include <sys.h>
#include <dbg.h>
#include <net.h>
#include <stdio.h>
#include <bstring.h>

#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
#undef MASTER_LOCK
#undef MASTER_UNLOCK
#define MASTER_LOCK(x) DISABLE_INTR()
#define MASTER_UNLOCK(x) ENABLE_INTR()
#else

/*
 * The master lock to synchronize access to pmegs and context.
 */
static Sync_Semaphore vmMachMutex;
static Sync_Semaphore *vmMachMutexPtr = &vmMachMutex;
#endif

#ifndef sun4c
/*
 * Macros to translate from a virtual page to a physical page and back.
 * For the sun4c, these are no longer macros, since they are much more
 * complicated due to physical memory no longer being contiguous.  For
 * speed perhaps someday they should be converted to complicated macros
 * instead of functions.
 */
#define VirtToPhysPage(pfNum) ((pfNum) << VMMACH_CLUSTER_SHIFT)
#define PhysToVirtPage(pfNum) ((pfNum) >> VMMACH_CLUSTER_SHIFT)
#endif

/*
 * Convert from page to hardware segment, with correction for
 * any difference between virtAddrPtr offset and segment offset.
 * (This difference will only happen for shared segments.)
*/
#define PageToOffSeg(page,virtAddrPtr) (PageToSeg((page)- \
	segOffset(virtAddrPtr)+(virtAddrPtr->segPtr->offset)))

extern	Address	vmStackEndAddr;

static int GetNumPages _ARGS_((void));
#ifdef sun4c
static int VirtToPhysPage _ARGS_((int pfNum));
static int PhysToVirtPage _ARGS_((int pfNum));
static void VmMachSetSegMapInContext _ARGS_((unsigned int context,
	Address addr, unsigned int pmeg));
#ifdef NOTDEF
static void MapColorMapAndIdRomAddr _ARGS_((void));
#endif /* NOTDEF */
#endif /* sun4c */
static void MMUInit _ARGS_((int firstFreeSegment));
ENTRY static void CopySegData _ARGS_((register Vm_Segment *segPtr,
	register VmMach_SegData *oldSegDataPtr,
	register VmMach_SegData *newSegDataPtr));
static void SegDelete _ARGS_((Vm_Segment *segPtr));
INTERNAL static int PMEGGet _ARGS_((Vm_Segment  *softSegPtr, int hardSegNum,
	Boolean flags));
INTERNAL static void PMEGFree _ARGS_((int pmegNum));
ENTRY static Boolean PMEGLock _ARGS_ ((register VmMach_SegData *machPtr,
	int segNum));
INTERNAL static void SetupContext _ARGS_((register Proc_ControlBlock
	*procPtr));
static void InitNetMem _ARGS_((void));
static void VmMachFlushCacheRange _ARGS_((Address startAddr, Address endAddr));
static void FlushWholeCache _ARGS_((void));
ENTRY static void WriteHardMapSeg _ARGS_((VmMach_ProcData *machPtr));
static void PageInvalidate _ARGS_((register Vm_VirtAddr *virtAddrPtr,
	unsigned int virtPage, Boolean segDeletion));
INTERNAL static void DevBufferInit _ARGS_((void));
static void VmMachTracePage _ARGS_((register VmMachPTE pte,
	unsigned int pageNum));
static void VmMachTrap _ARGS_((void));
#ifndef sun4c
INTERNAL static void Dev32BitDMABufferInit _ARGS_((void));
#endif

/*----------------------------------------------------------------------
 * 
 * 			Hardware data structures
 *
 * Terminology: 
 *	1) Physical page frame: A frame that contains one hardware page.
 *	2) Virtual page frame:  A frame that contains VMMACH_CLUSTER_SIZE 
 *				hardware pages.
 *	3) Software segment: The segment structure used by the hardware
 *			     independent VM module.
 *	4) Hardware segment: A piece of a hardware context.
 *
 * A hardware context corresponds to a process's address space.  A context
 * is made up of many equal sized hardware segments.  The 
 * kernel is mapped into each hardware context so that the kernel has easy
 * access to user data.  One context (context 0) is reserved for use by
 * kernel processes.  The hardware contexts are defined by the array
 * contextArray which contains an entry for each context.  Each entry 
 * contains a pointer back to the process that is executing in the context 
 * and an array which is an exact duplicate of the hardware segment map for
 * the context.  The contexts are managed by keeping all contexts except for
 * the system context in a list that is kept in LRU order.  Whenever a context 
 * is needed the first context off of the list is used and the context is
 * stolen from the current process that owns it if necessary.
 *
 * PMEGs are allocated to software segments in order to allow pages to be mapped
 * into the segment's virtual address space. There are only a few hundred
 * PMEGs, which have to be shared by all segments.  PMEGs that have
 * been allocated to user segments can be stolen at any time.  PMEGs that have
 * been allocated to the system segment cannot be taken away unless the system
 * segment voluntarily gives it up.  In order to manage the PMEGs there are
 * two data structures.  One is an array of PMEG info structures that contains
 * one entry for each PMEG.  The other is an array stored with each software
 * segment struct that contains the PMEGs that have been allocated to the
 * software segment.  Each entry in the array of PMEG info structures
 * contains enough information to remove the PMEG from its software segment.
 * One of the fields in the PMEG info struct is the count of pages that have
 * been validated in the PMEG.  This is used to determine when a PMEG is no
 * longer being actively used.  
 *
 * There are two lists that are used to manage PMEGs.  The pmegFreeList 
 * contains all PMEGs that are either not being used or contain no valid 
 * page map entries; unused ones are inserted at the front of the list 
 * and empty ones at the rear.  The pmegInuseList contains all PMEGs
 * that are being actively used to map user segments and is managed as a FIFO.
 * PMEGs that are being used to map tbe kernel's VAS do not appear on the 
 * pmegInuseList. When a pmeg is needed to map a virtual address, first the
 * free list is checked.  If it is not empty then the first PMEG is pulled 
 * off of the list.  If it is empty then the first PMEG is pulled off of the
 * inUse list.  If the PMEG  that is selected is being used (either actively 
 * or inactively) then it is freed from the software segment that is using it.
 * Once the PMEG is freed up then if it is being allocated for a user segment
 * it is put onto the end of the pmegInuseList.
 *
 * Page frames are allocated to software segments even when there is
 * no PMEG to map it in.  Thus when a PMEG that was mapping a page needs to
 * be removed from the software segment that owns the page, the reference
 * and modify bits stored in the PMEG for the page must be saved.  The
 * array refModMap is used for this.  It contains one entry for each
 * virtual page frame.  Its value for a page frame or'd with the bits stored
 * in the PMEG (if any) comprise the referenced and modified bits for a 
 * virtual page frame.
 * 
 * IMPORTANT SYNCHRONIZATION NOTE:
 *
 * The internal data structures in this file have to be protected by a
 * master lock if this code is to be run on a multi-processor.  Since a
 * process cannot start executing unless VmMach_SetupContext can be
 * executed first, VmMach_SetupContext cannot context switch inside itself;
 * otherwise a deadlock will occur.  However, VmMach_SetupContext mucks with
 * contexts and PMEGS and therefore would have to be synchronized
 * on a multi-processor.  A monitor lock cannot be used because it may force
 * VmMach_SetupContext to be context switched.
 *
 * The routines in this file also muck with other per segment data structures.
 * Access to these data structures is synchronized by our caller (the
 * machine independent module).
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
 * Macros to get to and from hardware segments and pages.
 */
#define PageToSeg(page) ((page) >> (VMMACH_SEG_SHIFT - VMMACH_PAGE_SHIFT))
#define SegToPage(seg) ((seg) << (VMMACH_SEG_SHIFT - VMMACH_PAGE_SHIFT))

/*
 * Macro to set all page map entries for the given virtual address.
 */
#if (VMMACH_CLUSTER_SIZE == 1) 
#define	SET_ALL_PAGE_MAP(virtAddr, pte) { \
	VmMachSetPageMap((Address)(virtAddr), (pte)); \
}
#else
#define	SET_ALL_PAGE_MAP(virtAddr, pte) { \
    int	__i; \
    for (__i = 0; __i < VMMACH_CLUSTER_SIZE; __i++) { \
	VmMachSetPageMap((virtAddr) + __i * VMMACH_PAGE_SIZE_INT, (pte) + __i); \
    } \
}
#endif
/*
 * PMEG table entry structure.
 */
typedef struct {
    List_Links			links;		/* Links so that the pmeg */
              					/* can be in a list */
    struct      Vm_Segment      *segPtr;        /* Back pointer to segment that
                                                   this cluster is in */
    int				hardSegNum;	/* The hardware segment number
						   for this pmeg. */
    int				pageCount;	/* Count of resident pages in
						 * this pmeg. */
    int				lockCount;	/* The number of times that
						 * this PMEG has been locked.*/
    int				flags;		/* Flags defined below. */
} PMEG;

/*
 * Flags to indicate the state of a pmeg.
 *
 *    PMEG_DONT_ALLOC	This pmeg should not be reallocated.  This is 
 *			when a pmeg cannot be reclaimed until it is
 *			voluntarily freed.
 *    PMEG_NEVER_FREE	Don't ever free this pmeg no matter what anybody says.
 */
#define	PMEG_DONT_ALLOC		0x1
#define	PMEG_NEVER_FREE		0x2

/*
 * Pmeg information.  pmegArray contains one entry for each pmeg.  pmegFreeList
 * is a list of all pmegs that aren't being actively used.  pmegInuseList
 * is a list of all pmegs that are being actively used.
 */
static	PMEG   		pmegArray[VMMACH_NUM_PMEGS];
static	List_Links   	pmegFreeListHeader;
static	List_Links   	*pmegFreeList = &pmegFreeListHeader;
static	List_Links   	pmegInuseListHeader;
static	List_Links   	*pmegInuseList = &pmegInuseListHeader;

/*
 * The context table structure.
 */
typedef struct VmMach_Context {
    List_Links			links;	 /* Links so that the contexts can be
					     in a list. */
    struct Proc_ControlBlock	*procPtr;	/* A pointer to the process
						 * table entry for the process
						 * that is running in this
						 * context. */
    VMMACH_SEG_NUM		map[VMMACH_NUM_SEGS_PER_CONTEXT];
					/* A reflection of the hardware context
					 * map. */
    unsigned int		context;/* Which context this is. */
    int				flags;	/* Defined below. */
} VmMach_Context;

/*
 * Context flags:
 *
 *     	CONTEXT_IN_USE	This context is used by a process.
 */
#define	CONTEXT_IN_USE	0x1

/*
 * Context information.  contextArray contains one entry for each context. 
 * contextList is a list of contexts in LRU order.
 */
static	VmMach_Context	contextArray[VMMACH_NUM_CONTEXTS];
static	List_Links   	contextListHeader;
static	List_Links   	*contextList = &contextListHeader;

/*
 * Map containing one entry for each virtual page.
 */
static	VmMachPTE		*refModMap;

/*
 * Macro to get a pointer into a software segment's hardware segment table.
 */
#ifdef CLEAN
#define GetHardSegPtr(machPtr, segNum) \
    ((machPtr)->segTablePtr + (segNum) - (machPtr)->offset)
#else
#define GetHardSegPtr(machPtr, segNum) \
    ( ((unsigned)((segNum) - (machPtr)->offset) > (machPtr)->numSegs) ? \
    (panic("Invalid segNum\n"),(machPtr)->segTablePtr) : \
    ((machPtr)->segTablePtr + (segNum) - (machPtr)->offset) )
#endif

/*
 * Macro to check to see if address is in a hardware segment (PMEG) that
 * is used by file cache virtual addresses.
 * These kernel segment pmegs may be stolen.
 */

#define	_ROUND2SEG(x) ((unsigned int)(x)  & ~(VMMACH_SEG_SIZE-1))
#define	IN_FILE_CACHE_SEG(addr) 					     \
          ( ((unsigned int)(addr) >=					     \
		_ROUND2SEG(vmBlockCacheBaseAddr + (VMMACH_SEG_SIZE-1))) &&   \
	    ((unsigned int)(addr) < _ROUND2SEG(vmBlockCacheEndAddr)) )

/*
 * TRUE if stealing of file cache pmegs are permitted. Initialized to FALSE
 * for backward compat with old file cache code.
 */
Boolean vmMachCanStealFileCachePmegs = FALSE;

/*
 * The maximum amount of kernel code + data available.  This is set to however
 * big you want it to be to make sure that the kernel heap has enough memory
 * for all the file handles, etc.
 */
#ifdef sun2
int	vmMachKernMemSize = 2048 * 1024;
#endif
#ifdef sun3
int     vmMachKernMemSize = 8192 * 1024;
#endif
#ifdef sun4
int	vmMachKernMemSize = 32 * 1024 * 1024;
#endif

/*
 * The segment that is used to map a segment into a process's virtual address
 * space for cross-address-space copies.
 */
#define	MAP_SEG_NUM (((unsigned int) VMMACH_MAP_SEG_ADDR) >> VMMACH_SEG_SHIFT)

static	VmMach_SegData	*sysMachPtr;
Address			vmMachPTESegAddr;
Address			vmMachPMEGSegAddr;

static	Boolean		printedSegTrace;
static	PMEG		*tracePMEGPtr;

#ifdef sun4c
/*
 * Structure for mapping virtual page frame numbers to physical page frame
 * numbers and back for each memory board.
 */
typedef struct	{
    unsigned int	endVirPfNum;	/* Ending virtual page frame nmber
					 * on board. */
    unsigned int	startVirPfNum;	/* Starting virtual page frame number
					 * on board. */
    unsigned int	physStartAddr;	/* Physical address of page frame. */
    unsigned int	physEndAddr;	/* End physical address of page frame.*/
} Memory_Board;

/*
 * Pointer to board after last configured Memory board structure.
 */
static	Memory_Board	*lastMemBoard;

/*
 * Memory_Board structures for each board in the system.  This array is sorted
 * by endVirPfNum.
 */
static	Memory_Board	Mboards[6];
#endif /* sun4c */


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
    register Address	virtAddr;
    register int	i;
    int			kernPages;
    int			numPages;
#ifdef sun4c
    Mach_MemList	*memPtr;
    int			nextVframeNum, numFrames;
#endif

#if (MACH_MAX_NUM_PROCESSORS != 1) /* multiprocessor implementation */
    Sync_SemInitDynamic(&vmMachMutex, "Vm:vmMachMutex");
#endif

#ifdef sun4c
    /*
     * Initialize the physical to virtual page mappings, since memory isn't
     * contiguous.
     */
    lastMemBoard = Mboards;
    nextVframeNum = 0;
    for (memPtr = *(romVectorPtr->availMemory); memPtr != (Mach_MemList *) 0;
	    memPtr = memPtr->next) {
	if (memPtr->size != 0) {
	    numFrames = memPtr->size / VMMACH_PAGE_SIZE;
	    lastMemBoard->startVirPfNum = nextVframeNum;
	    lastMemBoard->endVirPfNum = nextVframeNum + numFrames;
	    nextVframeNum += numFrames;
	    lastMemBoard->physStartAddr = memPtr->address >> VMMACH_PAGE_SHIFT;
	    lastMemBoard->physEndAddr = lastMemBoard->physStartAddr + numFrames;
	    lastMemBoard++;
	}
    }
    if (lastMemBoard == Mboards) {
	panic("No memory boards in system configuration.");
    }
#endif
    
    kernPages = vmMachKernMemSize / VMMACH_PAGE_SIZE_INT;
    /*
     * Map all of the kernel memory that we might need one for one.  We know
     * that the monitor maps the first part of memory one for one but for some
     * reason it doesn't map enough.  We assume that the pmegs have been
     * mapped correctly.
     */
    for (i = 0, virtAddr = (Address)mach_KernStart; 
	 i < kernPages;
	 i++, virtAddr += VMMACH_PAGE_SIZE_INT) {
#ifdef sun4
        VmMachSetPageMap(virtAddr, 
	    (VmMachPTE)(VMMACH_KRW_PROT | VMMACH_RESIDENT_BIT |
		    VMMACH_DONT_CACHE_BIT | i));
#else
        VmMachSetPageMap(virtAddr, 
	    (VmMachPTE)(VMMACH_KRW_PROT | VMMACH_RESIDENT_BIT | i));
#endif /* sun4 */
    }

    /*
     * Do boot time allocation.
     */
    sysMachPtr = (VmMach_SegData *)Vm_BootAlloc(sizeof(VmMach_SegData) + 
	    (sizeof (VMMACH_SEG_NUM) * VMMACH_NUM_SEGS_PER_CONTEXT));
    numPages = GetNumPages();
    refModMap = (VmMachPTE *)Vm_BootAlloc(sizeof(VmMachPTE) * numPages);

    /*
     * Return lots of sizes to the machine independent module who called us.
     */
    *pageSizePtr = VMMACH_PAGE_SIZE;
    *pageShiftPtr = VMMACH_PAGE_SHIFT;
    *pageTableIncPtr = VMMACH_PAGE_TABLE_INCREMENT;
    *kernMemSizePtr = vmMachKernMemSize;
    *maxProcessesPtr = VMMACH_MAX_KERN_STACKS;
    *numKernPagesPtr = GetNumPages();
    /* 
     * We don't care how many software segments there are so return -1 as
     * the max.
     */
    *maxSegsPtr = -1;
}


/*
 * ----------------------------------------------------------------------------
 *
 * GetNumPages --
 *
 *     Determine how many pages of physical memory there are.
 *     For the sun4c, this determines how many physical pages of memory
 *     are available after the prom has grabbed some.
 *
 * Results:
 *     The number of physical pages available.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static int
GetNumPages()
{
#ifdef sun4c
    int	memory = 0;
    Mach_MemList	*memPtr;

    for (memPtr = *(romVectorPtr->availMemory); memPtr != (Mach_MemList *) 0;
	    memPtr = memPtr->next) {
	memory += memPtr->size;
    }
    return (memory / VMMACH_PAGE_SIZE);
#else
    return (*romVectorPtr->memoryAvail / VMMACH_PAGE_SIZE);
#endif
}

#ifdef sun4c

/*
 * ----------------------------------------------------------------------------
 *
 * VirtToPhysPage --
 *
 *     Translate from a virtual page to a physical page.
 *     This was a macro on the other suns, but for the sun4c, physical
 *     memory isn't contiguous.
 *
 * Results:
 *     An address.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static int
VirtToPhysPage(pfNum)
    int		pfNum;
{
    register	Memory_Board	*mb;

    for (mb = Mboards; mb < lastMemBoard; mb++) {
	if (pfNum < mb->endVirPfNum) {
	    break;
	}
    }
    return (mb->physStartAddr + pfNum - mb->startVirPfNum);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PhysToVirtPage --
 *
 *     Translate from a physical page to a virtual page.
 *     This was a macro on the other suns, but for the sun4c, physical
 *     memory isn't contiguous.
 *
 * Results:
 *     An address.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static int
PhysToVirtPage(pfNum)
    int		pfNum;
{
    register	Memory_Board	*mb;

    for (mb = Mboards; mb < lastMemBoard; mb++) {
	if (pfNum >= mb->physStartAddr && pfNum < mb->physEndAddr) {
	    break;
	}
    }
    return (pfNum - mb->physStartAddr + mb->startVirPfNum);
}
#endif /* sun4c */

#ifdef sun4c

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetSegMapInContext --
 *
 *	Set the segment map in a context that may not yet be mapped without
 *	causing a fault.  So far, this is only useful on the sun4c.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The segment map in another context is modified..
 *
 * ----------------------------------------------------------------------------
 */
static void
VmMachSetSegMapInContext(context, addr, pmeg)
    unsigned	int	context;
    Address		addr;
    unsigned	int	pmeg;
{
    romVectorPtr->SetSegInContext(context, addr, pmeg);
    return;
}
#endif /* sun4c */


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_AllocKernSpace --
 *
 *     Allocate memory for machine dependent stuff in the kernels VAS.
 *
 * Results:
 *     None.  Well, it returns something, Mike...  It seems to return the
 *     address of the next free area.
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
    /*
     * If the base address is at the beginning of a segment, we just want
     * to allocate this segment for vmMachPTESegAddr.  If base addr is partway
     * into a segment, we want a whole segment, so move to the next segment
     * to allocate that one.  (baseAddr + VMMACH_SEG_SIZE - 1) moves us to
     * next segment unless we were at the very beginning of one.  Then divide
     * by VMMACH_SEG_SIZE to get segment number.  Then multiply by
     * VMMACH_SEG_SIZE to get address of the begginning of the segment.
     */
    baseAddr = (Address) ((((unsigned int)baseAddr + VMMACH_SEG_SIZE - 1) / 
					VMMACH_SEG_SIZE) * VMMACH_SEG_SIZE);
    vmMachPTESegAddr = baseAddr;	/* first seg for Pte mapping */
    vmMachPMEGSegAddr = baseAddr + VMMACH_SEG_SIZE;	/* next for pmegs */
    return(baseAddr + 2 * VMMACH_SEG_SIZE);	/* end of allocated area */
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
void
VmMach_Init(firstFreePage)
    int	firstFreePage;	/* Virtual page that is the first free for the 
			 * kernel. */
{
    register 	VMMACH_SEG_NUM	*segTablePtr;
    register 	VmMachPTE	pte;
    register	int 		i;
    int 			firstFreeSegment;
    Address			virtAddr;
    Address			lastCodeAddr;
    extern	int		etext;
#ifdef sun4c
    VMMACH_SEG_NUM		pmeg;
#endif sun4c

    /*
     * Initialize the kernel's hardware segment table.
     */
    vm_SysSegPtr->machPtr = sysMachPtr;
    sysMachPtr->numSegs = VMMACH_NUM_SEGS_PER_CONTEXT;
    sysMachPtr->offset = PageToSeg(vm_SysSegPtr->offset);
    sysMachPtr->segTablePtr =
	    (VMMACH_SEG_NUM *) ((Address)sysMachPtr + sizeof(VmMach_SegData));
    for (i = 0; i < VMMACH_NUM_SEGS_PER_CONTEXT; i++) {
	sysMachPtr->segTablePtr[i] = VMMACH_INV_PMEG;
    }

    /*
     * Determine which hardware segment is the first that is not in use.
     */
    firstFreeSegment = ((firstFreePage - 1) << VMMACH_PAGE_SHIFT) / 
					VMMACH_SEG_SIZE + 1;
    firstFreeSegment += (unsigned int)mach_KernStart >> VMMACH_SEG_SHIFT;

    /* 
     * Initialize the PMEG and context tables and lists.
     */
    MMUInit(firstFreeSegment);

    /*
     * Initialize the page map.
     */
    bzero((Address)refModMap, sizeof(VmMachPTE) * GetNumPages());

    /*
     * The code segment is read only and all other in use kernel memory
     * is read/write.  Since the loader may put the data in the same page
     * as the last code page, the last code page is also read/write.
     */
    lastCodeAddr = (Address) ((unsigned)&etext - VMMACH_PAGE_SIZE);
    for (i = 0, virtAddr = (Address)mach_KernStart;
	 i < firstFreePage;
	 virtAddr += VMMACH_PAGE_SIZE, i++) {
	if (virtAddr >= (Address)MACH_CODE_START && 
	    virtAddr <= lastCodeAddr) {
	    pte = VMMACH_RESIDENT_BIT | VMMACH_KR_PROT | 
			  VirtToPhysPage(i) * VMMACH_CLUSTER_SIZE;
	} else {
	    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | 
			  VirtToPhysPage(i) * VMMACH_CLUSTER_SIZE;
#ifdef sun4
	    if (virtAddr >= vmStackEndAddr) {
		pte |= VMMACH_DONT_CACHE_BIT;
	    }
#endif /* sun4 */
        }
	SET_ALL_PAGE_MAP(virtAddr, pte);
    }

    /*
     * Protect the bottom of the kernel stack.
     */
    SET_ALL_PAGE_MAP((Address)mach_StackBottom, (VmMachPTE)VirtToPhysPage(0));

    /*
     * Invalid until the end of the last segment
     */
    for (;virtAddr < (Address) (firstFreeSegment << VMMACH_SEG_SHIFT);
	 virtAddr += VMMACH_PAGE_SIZE) {
	SET_ALL_PAGE_MAP(virtAddr, (VmMachPTE)VirtToPhysPage(0));
    }

    /* 
     * Zero out the invalid pmeg.
     */
    /* Need I flush something? */
    VmMachPMEGZero(VMMACH_INV_PMEG);

    /*
     * Finally copy the kernel's context to each of the other contexts.
     */
    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	int		segNum;

	if (i == VMMACH_KERN_CONTEXT) {
	    continue;
	}
#ifndef sun4c
	VmMachSetUserContext(i);
	for (segNum = ((unsigned int) mach_KernStart) / VMMACH_SEG_SIZE,
		 segTablePtr = vm_SysSegPtr->machPtr->segTablePtr;
		 segNum < VMMACH_NUM_SEGS_PER_CONTEXT;
		 segNum++, segTablePtr++) {

	    virtAddr = (Address) (segNum * VMMACH_SEG_SIZE);
	    /*
	     * No need to flush stuff since the other contexts haven't
	     * been used yet.
	     */
	    VmMachSetSegMap(virtAddr, (int)*segTablePtr);
	}
#else
	/*
	 * For the sun4c, there is currently a problem just copying things
	 * out of the segTable, so do it from the real hardware.  I need to
	 * figure out what's wrong.
	 */
	for (segNum = ((unsigned int) mach_KernStart) / VMMACH_SEG_SIZE;
		 segNum < VMMACH_NUM_SEGS_PER_CONTEXT; segNum++) {

	    virtAddr = (Address) (segNum * VMMACH_SEG_SIZE);
	    if (virtAddr >= (Address) VMMACH_BOTTOM_OF_HOLE && 
		    virtAddr <= (Address) VMMACH_TOP_OF_HOLE) {
		continue;
	    }
	    pmeg = VmMachGetSegMap(virtAddr);
	    VmMachSetSegMapInContext((unsigned char) i, virtAddr, pmeg);
	}
#endif
    }
    VmMachSetUserContext(VMMACH_KERN_CONTEXT);
    if (Mach_GetMachineType() == SYS_SUN_3_50) {
	unsigned int vidPage;

#define	VIDEO_START	0x100000	/* From Sun3 architecture manual */
#define	VIDEO_SIZE	0x20000
	vidPage = VIDEO_START / VMMACH_PAGE_SIZE;
	if (firstFreePage > vidPage) {
	    panic("VmMach_Init: We overran video memory.\n");
	}
	/*
	 * On 3/50's the display is kept in main memory beginning at 1 
	 * Mbyte and going for 128 kbytes.  Reserve this memory so VM
	 * doesn't try to use it.
	 */
	for (;vidPage < (VIDEO_START + VIDEO_SIZE) / VMMACH_PAGE_SIZE;
	     vidPage++) {
	    Vm_ReservePage(vidPage);
	}
    }
    /*
     * Turn on caching.
     */
    VmMachClearCacheTags();
#ifndef sun4c
    VmMachInitAddrErrorControlReg();
#endif
    VmMachInitSystemEnableReg();
#ifdef NOTDEF
    /*
     * Initialize map of invalid pmegs for later copying.
     */
    for (i = 0; i < VMMACH_NUM_SEGS_PER_CONTEXT; i++) {
	copyMap[i] = VMMACH_INV_PMEG;
    }
#endif NOTDEF
#ifdef NOTDEF
/* This is broken for now */
#ifdef sun4c
    MapColorMapAndIdRomAddr();
#endif sun4c
#endif NOTDEF

}


/*
 *----------------------------------------------------------------------
 *
 * MMUInit --
 *
 *	Initialize the context table and lists and the Pmeg table and 
 *	lists.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Context table and Pmeg table are initialized.  Also context list
 *	and pmeg list are initialized.
 *
 *----------------------------------------------------------------------
 */
static void
MMUInit(firstFreeSegment)
    int		firstFreeSegment;
{
    register	int		i;
    register	PMEG		*pmegPtr;
    register	VMMACH_SEG_NUM	*segTablePtr;
    VMMACH_SEG_NUM		pageCluster;

    /*
     * Initialize the context table.
     */
    contextArray[VMMACH_KERN_CONTEXT].flags = CONTEXT_IN_USE;
    List_Init(contextList);
    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	if (i != VMMACH_KERN_CONTEXT) {
	    contextArray[i].flags = 0;
	    List_Insert((List_Links *) &contextArray[i], 
			LIST_ATREAR(contextList));
	}
	contextArray[i].context = i;
    }

    /*
     * Initialize the page cluster list.
     */
    List_Init(pmegFreeList);
    List_Init(pmegInuseList);

    /*
     * Initialize the pmeg structure.
     */
    bzero((Address)pmegArray, VMMACH_NUM_PMEGS * sizeof(PMEG));
    for (i = 0, pmegPtr = pmegArray; i < VMMACH_NUM_PMEGS; i++, pmegPtr++) {
	pmegPtr->segPtr = (Vm_Segment *) NIL;
	pmegPtr->flags = PMEG_DONT_ALLOC;
    }

#ifdef sun2
    /*
     * Segment 0 is left alone because it is required for the monitor.
     */
    pmegArray[0].segPtr = (Vm_Segment *)NIL;
    pmegArray[0].hardSegNum = 0;
    i = 1;
#else
    i = 0;
#endif /* sun2 */

    /*
     * Invalidate all hardware segments from first segment up to the beginning
     * of the kernel.
     */
    for (; i < ((((unsigned int) mach_KernStart) & VMMACH_ADDR_MASK) >>
	    VMMACH_SEG_SHIFT); i++) {
	int	j;

	/*
	 * Copy the invalidation to all the other contexts, so that
	 * the user contexts won't have double-mapped pmegs at the low-address
	 * segments.
	 */
	for (j = 0; j < VMMACH_NUM_CONTEXTS; j++) {
	    Address	addr;

#ifndef sun4c
	    VmMachSetUserContext(j);
#endif
	    addr = (Address) (i << VMMACH_SEG_SHIFT);
	    /* Yes, do this here since user stuff would be double mapped. */
#ifndef sun4c
	    VmMachSetSegMap(addr, VMMACH_INV_PMEG);
#else
	    VmMachSetSegMapInContext((unsigned char) j, addr,
		    (unsigned char) VMMACH_INV_PMEG);
#endif
	}
    }
    VmMachSetUserContext(VMMACH_KERN_CONTEXT);
    i = ((unsigned int) mach_KernStart >> VMMACH_SEG_SHIFT);

    /*
     * Reserve all pmegs that have kernel code or heap.
     */
    for (segTablePtr = vm_SysSegPtr->machPtr->segTablePtr;
         i < firstFreeSegment;
	 i++, segTablePtr++) {
	pageCluster = VmMachGetSegMap((Address) (i << VMMACH_SEG_SHIFT));
	pmegArray[pageCluster].pageCount = VMMACH_NUM_PAGES_PER_SEG;
	pmegArray[pageCluster].segPtr = vm_SysSegPtr;
	pmegArray[pageCluster].hardSegNum = i;
	*segTablePtr = pageCluster;
    }

    /*
     * Invalidate all hardware segments that aren't in code or heap and are 
     * before the specially mapped page clusters.
     */
    for (; i < VMMACH_FIRST_SPECIAL_SEG; i++, segTablePtr++) {
	Address	addr;
	/* Yes, do this here, since user stuff would be double-mapped. */
	addr = (Address) (i << VMMACH_SEG_SHIFT);
	VmMachSetSegMap(addr, VMMACH_INV_PMEG);
    }

    /*
     * Mark the invalid pmeg so that it never gets used.
     */
    pmegArray[VMMACH_INV_PMEG].segPtr = vm_SysSegPtr;
    pmegArray[VMMACH_INV_PMEG].flags = PMEG_NEVER_FREE;

    /*
     * Now reserve the rest of the page clusters that have been set up by
     * the monitor.  Don't reserve any PMEGs that don't have any valid 
     * mappings in them.
     */
    for (; i < VMMACH_NUM_SEGS_PER_CONTEXT; i++, segTablePtr++) {
	Address		virtAddr;
	int		j;
	VmMachPTE	pte;
	Boolean		inusePMEG;

	virtAddr = (Address) (i << VMMACH_SEG_SHIFT);
        if ((virtAddr >= (Address)VMMACH_DMA_START_ADDR) &&
	    (virtAddr < (Address)(VMMACH_DMA_START_ADDR+VMMACH_DMA_SIZE))) {
	    /*
	     * Blow away anything in DMA space. 
	     */
	    pageCluster = VMMACH_INV_PMEG;
	    VmMachSetSegMap(virtAddr, VMMACH_INV_PMEG);
	} else {
	    pageCluster = VmMachGetSegMap(virtAddr);
	    if (pageCluster != VMMACH_INV_PMEG) {
		inusePMEG = FALSE;
		for (j = 0; 
		     j < VMMACH_NUM_PAGES_PER_SEG_INT; 
		     j++, virtAddr += VMMACH_PAGE_SIZE_INT) {
		    pte = VmMachGetPageMap(virtAddr);
		    if ((pte & VMMACH_RESIDENT_BIT) &&
			(pte & (VMMACH_TYPE_FIELD|VMMACH_PAGE_FRAME_FIELD)) != 0) {
			/*
			 * A PMEG contains a valid mapping if the resident
			 * bit is set and the page frame and type field
			 * are non-zero.  On Sun 2/50's the PROM sets
			 * the resident bit but leaves the page frame equal
			 * to zero.
			 */
			if (!inusePMEG) {
			    pmegArray[pageCluster].segPtr = vm_SysSegPtr;
			    pmegArray[pageCluster].hardSegNum = i;
			    pmegArray[pageCluster].flags = PMEG_NEVER_FREE;
			    inusePMEG = TRUE;
			}
		    } else {
			VmMachSetPageMap(virtAddr, (VmMachPTE)0);
		    }
		}
		virtAddr -= VMMACH_SEG_SIZE;
		if (!inusePMEG ||
		    (virtAddr >= (Address)VMMACH_DMA_START_ADDR &&
		     virtAddr < (Address)(VMMACH_DMA_START_ADDR+VMMACH_DMA_SIZE))) {
#ifdef PRINT_ZAP
		    int z;
		    /* 
		     * We didn't find any valid mappings in the PMEG or the PMEG
		     * is in DMA space so delete it.
		     */
		    printf("Zapping segment at virtAddr %x\n", virtAddr);
		    for (z = 0; z < 100000; z++) {
		    }
#endif
		    VmMachSetSegMap(virtAddr, VMMACH_INV_PMEG);
		    pageCluster = VMMACH_INV_PMEG;
		}
	    }
	}
	*segTablePtr = pageCluster;
    }

#if defined (sun3)
    /*
     * We can't use the hardware segment that corresponds to the
     * last segment of physical memory for some reason.  Zero it out
     * and can't reboot w/o powering the machine off.
     */
    dontUse = (*romVectorPtr->memoryAvail - 1) / VMMACH_SEG_SIZE;
#endif

    /*
     * Now finally, all page clusters that have a NIL segment pointer are
     * put onto the page cluster fifo.  On a Sun-3 one hardware segment is 
     * off limits for some reason.  Zero it out and can't reboot w/o 
     * powering the machine off.
     */
    for (i = 0, pmegPtr = pmegArray; i < VMMACH_NUM_PMEGS; i++, pmegPtr++) {

	if (pmegPtr->segPtr == (Vm_Segment *) NIL 
#if defined (sun3)
	    && i != dontUse
#endif
	) {
	    List_Insert((List_Links *) pmegPtr, LIST_ATREAR(pmegFreeList));
	    pmegPtr->flags = 0;
	    VmMachPMEGZero(i);
	}
    }
}

#ifdef NOTDEF
/* This is broken for now */
#ifdef sun4c
static char	colorArray[3 * 0x1000];
#ifdef ID
static char	idArray[2 * 0x1000];
#endif ID

/*
 * ----------------------------------------------------------------------------
 *
 * MapColorMapAndIdRomAddr --
 *
 *      Get the physical address of the color map for a sparc station and map
 *	it to a virtual address.  Also map the id words for the video
 *	subsystem.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Page maps are altered.  Several pages of physical memory are wasted.
 *
 * ----------------------------------------------------------------------------
 */
static void
MapColorMapAndIdRomAddr()
{
    VmMachPTE		colorPte;
    VmMachPTE		idPte;
    unsigned int	otherbits;
    Address		colorVirtAddr;
    Address		idVirtAddr;

    colorPte = VmMachGetPageMap(DEV_FRAME_BUF_ADDR);
    otherbits = colorPte &(~VMMACH_PAGE_FRAME_FIELD);
    colorPte &= VMMACH_PAGE_FRAME_FIELD;
    if ((colorPte & 0xfff) != 0x800) {
	panic("MapColorMapAndIdRomAddr: frame buffer addr is weird: 0x%x\n",
		colorPte);
    }
    /* Change last bits to color map offset and id rom offset */
    colorPte &= 0xfffff000;
#ifdef ID
    idPte = colorPte;
#endif
    colorPte |= 0x400;
    colorPte |= otherbits;
#ifdef ID
    idPte |= otherbits;		/* 0 offset */
#endif

    /* allocate 2 pages on a page boundary */
#ifdef NOTDEF
    colorVirtAddr = (Address) malloc(3 * 0x1000);
#else
    colorVirtAddr = colorArray;
#endif
    ((unsigned int)colorVirtAddr) &= 0xfffff000;

    /* and another 2 pages */
#ifdef NOTDEF
    idVirtAddr = (Address) malloc(3 * 0x1000);
#else NOTDEF
#ifdef ID
    idVirtAddr = idArray;
#endif ID
#endif NOTDEF
#ifdef ID
    ((unsigned int)idVirtAddr) &= 0xfffff000;
#endif

    /* This wastes 3 pages of physical memory.  That's a shame. */
    VmMachSetPageMap(colorVirtAddr, colorPte);
    VmMachSetPageMap(colorVirtAddr + 0x1000, colorPte + 1);

#ifdef ID
    /* This wastes 3 pages of physical memory again. */
    VmMachSetPageMap(idVirtAddr, idPte);
    VmMachSetPageMap(idVirtAddr + 0x1000, idPte + 1);
    printf("colorPte 0x%x, idPte 0x%x, colorVA 0x%x, idVA 0x%x\n",
	colorPte, idPte, colorVirtAddr, idVirtAddr);
#else
    printf("colorPte 0x%x, colorVA 0x%x\n",
	colorPte, colorVirtAddr);
#endif

    return;
}
#endif sun4c
#endif NOTDEF



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
 *      Machine dependent data struct and is allocated and initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SegInit(segPtr)
    Vm_Segment	*segPtr;
{
    register	VmMach_SegData	*segDataPtr;
    int				segTableSize;
    int		i;

    if (segPtr->type == VM_CODE) {
	segTableSize =
	    (segPtr->ptSize + segPtr->offset + VMMACH_NUM_PAGES_PER_SEG - 1) / 
						    VMMACH_NUM_PAGES_PER_SEG;
    } else {
	segTableSize = segPtr->ptSize / VMMACH_NUM_PAGES_PER_SEG;
    }
    segDataPtr = (VmMach_SegData *)malloc(sizeof(VmMach_SegData) +
	(segTableSize * sizeof (VMMACH_SEG_NUM)));

    segDataPtr->numSegs = segTableSize;
    segDataPtr->offset = PageToSeg(segPtr->offset);
    segDataPtr->segTablePtr =
	    (VMMACH_SEG_NUM *) ((Address)segDataPtr + sizeof(VmMach_SegData));
    for (i = 0; i < segTableSize; i++) {
	segDataPtr->segTablePtr[i] = VMMACH_INV_PMEG;
    }
    segPtr->machPtr = segDataPtr;
    /*
     * Set the minimum and maximum virtual addresses for this segment to
     * be as small and as big as possible respectively because things will
     * be prevented from growing automatically as soon as segments run into
     * each other.
     */
    segPtr->minAddr = (Address)0;
    segPtr->maxAddr = (Address)0xffffffff;
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
 *	Memory allocated for a new hardware segment table.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_SegExpand(segPtr, firstPage, lastPage)
    register	Vm_Segment	*segPtr;	/* Segment to expand. */
    int				firstPage;	/* First page to add. */
    int				lastPage;	/* Last page to add. */
{
    int				newSegTableSize;
    register	VmMach_SegData	*oldSegDataPtr;
    register	VmMach_SegData	*newSegDataPtr;

    newSegTableSize = segPtr->ptSize / VMMACH_NUM_PAGES_PER_SEG;
    oldSegDataPtr = segPtr->machPtr;
    if (newSegTableSize <= oldSegDataPtr->numSegs) {
	return;
    }
    newSegDataPtr = 
	(VmMach_SegData *)malloc(sizeof(VmMach_SegData) + (newSegTableSize *
		sizeof (VMMACH_SEG_NUM)));
    newSegDataPtr->numSegs = newSegTableSize;
    newSegDataPtr->offset = PageToSeg(segPtr->offset);
    newSegDataPtr->segTablePtr = (VMMACH_SEG_NUM *) ((Address)newSegDataPtr +
		    sizeof(VmMach_SegData));
    CopySegData(segPtr, oldSegDataPtr, newSegDataPtr);
    free((Address)oldSegDataPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * CopySegData --
 *
 *	Copy over the old hardware segment data into the new expanded
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hardware segment table is copied.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
CopySegData(segPtr, oldSegDataPtr, newSegDataPtr)
    register	Vm_Segment	*segPtr;	/* The segment to add the
						   virtual pages to. */
    register	VmMach_SegData	*oldSegDataPtr;
    register	VmMach_SegData	*newSegDataPtr;
{
    int		i, j;

    MASTER_LOCK(vmMachMutexPtr);

    if (segPtr->type == VM_HEAP) {
	/*
	 * Copy over the hardware segment table into the lower part
	 * and set the rest to invalid.
	 */
	bcopy((Address)oldSegDataPtr->segTablePtr,
		(Address)newSegDataPtr->segTablePtr,
		oldSegDataPtr->numSegs * sizeof (VMMACH_SEG_NUM));
	j = newSegDataPtr->numSegs - oldSegDataPtr->numSegs;

	for (i = 0; i < j; i++) {
	    newSegDataPtr->segTablePtr[oldSegDataPtr->numSegs + i]
		    = VMMACH_INV_PMEG;
	}
    } else {
	/*
	 * Copy the current segment table into the high part of the
	 * new segment table and set the lower part to invalid.
	 */
	bcopy((Address)oldSegDataPtr->segTablePtr,
	    (Address)(newSegDataPtr->segTablePtr + 
	    newSegDataPtr->numSegs - oldSegDataPtr->numSegs),
	    oldSegDataPtr->numSegs * sizeof (VMMACH_SEG_NUM));
	j = newSegDataPtr->numSegs - oldSegDataPtr->numSegs;

	for (i = 0; i < j; i++) {
	    newSegDataPtr->segTablePtr[i] = VMMACH_INV_PMEG;
	}
    }
    segPtr->machPtr = newSegDataPtr;

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SegDelete --
 *
 *      Free hardware dependent resources for this software segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Machine dependent struct freed and the pointer in the segment
 *	is set to NIL.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_SegDelete(segPtr)
    register	Vm_Segment	*segPtr;    /* Pointer to segment to free. */
{
    if (vm_Tracing) {
	Vm_TraceSegDestroy	segDestroy;

	segDestroy.segNum = segPtr->segNum;
	VmStoreTraceRec(VM_TRACE_SEG_DESTROY_REC, sizeof(segDestroy),
			(Address)&segDestroy, TRUE);
    }

    SegDelete(segPtr);
    free((Address)segPtr->machPtr);
    segPtr->machPtr = (VmMach_SegData *)NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * SegDelete --
 *
 *      Free up any pmegs used by this segment.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      All pmegs used by this segment are freed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
SegDelete(segPtr)
    Vm_Segment	*segPtr;    /* Pointer to segment to free. */
{
    register	int 		i;
    register	VMMACH_SEG_NUM	*pmegPtr;
    register	VmMach_SegData	*machPtr;

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = segPtr->machPtr;
    for (i = 0, pmegPtr = (VMMACH_SEG_NUM *) machPtr->segTablePtr;
	     i < machPtr->numSegs; i++, pmegPtr++) {
	if (*pmegPtr != VMMACH_INV_PMEG) {
	    /* Flushing is done in PMEGFree */
	    PMEGFree((int) *pmegPtr);
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_GetContext --
 *
 *	Return the context for a process, given its pcb.
 *
 * Results:
 *	Context number for process. -1 if the process doesn't
 *	have a context allocated.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
VmMach_GetContext(procPtr)
    Proc_ControlBlock	*procPtr;
{
    VmMach_Context	*contextPtr;
    contextPtr = procPtr->vmPtr->machPtr->contextPtr;
    return ((contextPtr == (VmMach_Context *)NIL) ? -1 : contextPtr->context);
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
    vmPtr->machPtr->contextPtr = (VmMach_Context *)NIL;
    vmPtr->machPtr->mapSegPtr = (struct Vm_Segment *)NIL;
    vmPtr->machPtr->sharedData.allocVector = (int *)NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * PMEGGet --
 *
 *      Return the next pmeg from the list of available pmegs.  If the 
 *      lock flag is set then the pmeg is removed from the pmeg list.
 *      Otherwise it is moved to the back.
 *
 * Results:
 *      The pmeg number that is allocated.
 *
 * Side effects:
 *      A pmeg is either removed from the pmeg list or moved to the back.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static int
PMEGGet(softSegPtr, hardSegNum, flags)
    Vm_Segment 	*softSegPtr;	/* Which software segment this is. */
    int		hardSegNum;	/* Which hardware segment in the software 
				   segment that this is */
    Boolean	flags;		/* Flags that indicate the state of the pmeg. */
{
    register PMEG		*pmegPtr;
    register Vm_Segment		*segPtr;
    register VmMachPTE		*ptePtr;
    register VmMach_Context	*contextPtr;
    register int		i;
    register VmMachPTE		hardPTE;
    VmMachPTE			pteArray[VMMACH_NUM_PAGES_PER_SEG_INT];
    int	     			oldContext;
    int				pmegNum;
    Address			virtAddr;
    Boolean			found = FALSE;

    if (List_IsEmpty(pmegFreeList)) {
	
	LIST_FORALL(pmegInuseList, (List_Links *)pmegPtr) {
	    if (pmegPtr->lockCount == 0) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    panic("Pmeg lists empty\n");
	    return(VMMACH_INV_PMEG);
	}
    } else {
	pmegPtr = (PMEG *)List_First(pmegFreeList);
    }
    pmegNum = pmegPtr - pmegArray;

    if (pmegPtr->segPtr != (Vm_Segment *) NIL) {
	/*
	 * Need to steal the pmeg from its current owner.
	 */
	vmStat.machDepStat.stealPmeg++;
	segPtr = pmegPtr->segPtr;
	*GetHardSegPtr(segPtr->machPtr, pmegPtr->hardSegNum) = VMMACH_INV_PMEG;
	virtAddr = (Address) (pmegPtr->hardSegNum << VMMACH_SEG_SHIFT);
	/*
	 * Delete the pmeg from all appropriate contexts.
	 */
	oldContext = VmMachGetContextReg();
        if (segPtr->type == VM_SYSTEM) {
	    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
		VmMachSetContextReg(i);
		VmMachFlushSegment(virtAddr);
		VmMachSetSegMap(virtAddr, VMMACH_INV_PMEG);
	    }
        } else {
	    for (i = 1, contextPtr = &contextArray[1];
		 i < VMMACH_NUM_CONTEXTS; 
		 i++, contextPtr++) {
		if (contextPtr->flags & CONTEXT_IN_USE) {
		    if (contextPtr->map[pmegPtr->hardSegNum] == pmegNum) {
			VmMachSetContextReg(i);
			contextPtr->map[pmegPtr->hardSegNum] = VMMACH_INV_PMEG;
			VmMachFlushSegment(virtAddr);
			VmMachSetSegMap(virtAddr, VMMACH_INV_PMEG);
		    }
		    if (contextPtr->map[MAP_SEG_NUM] == pmegNum) {
			VmMachSetContextReg(i);
			contextPtr->map[MAP_SEG_NUM] = VMMACH_INV_PMEG;
			VmMachFlushSegment((Address)VMMACH_MAP_SEG_ADDR);
			VmMachSetSegMap((Address)VMMACH_MAP_SEG_ADDR,
					VMMACH_INV_PMEG);
		    }
		}
	    }
        }
	VmMachSetContextReg(oldContext);
	/*
	 * Read out all reference and modify bits from the pmeg.
	 */
	if (pmegPtr->pageCount > 0) {
	    Boolean	printedStealPMEG = FALSE;

	    ptePtr = pteArray;
	    VmMachReadAndZeroPMEG(pmegNum, ptePtr);
	    for (i = 0;
		 i < VMMACH_NUM_PAGES_PER_SEG_INT;
		 i++, ptePtr++) {
		hardPTE = *ptePtr;
		if ((hardPTE & VMMACH_RESIDENT_BIT) &&
		    (hardPTE & (VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT))) {
		    if (vm_Tracing) {
			if (!printedStealPMEG) {
			    short	stealPMEGRec;

			    printedStealPMEG = TRUE;
			    printedSegTrace = FALSE;
			    tracePMEGPtr = pmegPtr;
			    VmStoreTraceRec(VM_TRACE_STEAL_PMEG_REC,
					    sizeof(short), 
					    (Address)&stealPMEGRec,TRUE);
			}
			VmMachTracePage(hardPTE,
			    (unsigned int) (VMMACH_NUM_PAGES_PER_SEG_INT - i));
		    } else {
		    refModMap[PhysToVirtPage(hardPTE & VMMACH_PAGE_FRAME_FIELD)]
		     |= hardPTE & (VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT);
		    }
		}
	    }
	}
    }

    /* Initialize the pmeg and delete it from the fifo.  If we aren't 
     * supposed to lock this pmeg, then put it at the rear of the list.
     */
    pmegPtr->segPtr = softSegPtr;
    pmegPtr->hardSegNum = hardSegNum;
    pmegPtr->pageCount = 0;
    List_Remove((List_Links *) pmegPtr);
    if (!(flags & PMEG_DONT_ALLOC)) {
	List_Insert((List_Links *) pmegPtr, LIST_ATREAR(pmegInuseList));
    }
    pmegPtr->flags = flags;

    return(pmegNum);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PMEGFree --
 *
 *      Return the given pmeg to the pmeg list.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The pmeg is returned to the pmeg list.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PMEGFree(pmegNum)
    int 	pmegNum;	/* Which pmeg to free */
{
    register	PMEG	*pmegPtr;

    pmegPtr = &pmegArray[pmegNum];
    /*
     * If this pmeg can never be freed then don't free it.  This case can
     * occur when a device is mapped into a user's address space.
     */
    if (pmegPtr->flags & PMEG_NEVER_FREE) {
	return;
    }

    if (pmegPtr->pageCount > 0) {

	/*
	 * Deal with pages that are still cached in this pmeg.
	 */
	VmMachPMEGZero(pmegNum);
    }
    pmegPtr->segPtr = (Vm_Segment *) NIL;

    /*
     * I really don't understand the code here.  The original was the second
     * line.  The first line was to try to fix an error that shows up in
     * UnmapIntelPage(), but I've tried now to fix that error there, since the
     * second line breaks things elsewhere.
     */
    if (pmegPtr->pageCount == 0 || !(pmegPtr->flags & PMEG_DONT_ALLOC)) {
	List_Remove((List_Links *) pmegPtr);
    }
    pmegPtr->flags = 0;
    pmegPtr->lockCount = 0;
    /*
     * Put this pmeg at the front of the pmeg free list.
     */
    List_Insert((List_Links *) pmegPtr, LIST_ATFRONT(pmegFreeList));
}


/*
 * ----------------------------------------------------------------------------
 *
 * PMEGLock --
 *
 *      Increment the lock count on a pmeg.
 *
 * Results:
 *      TRUE if there was a valid PMEG behind the given hardware segment.
 *
 * Side effects:
 *      The lock count is incremented if there is a valid pmeg at the given
 *	hardware segment.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static Boolean
PMEGLock(machPtr, segNum)
    register VmMach_SegData	*machPtr;
    int				segNum;
{
    unsigned int pmegNum;

    MASTER_LOCK(vmMachMutexPtr);

    pmegNum = *GetHardSegPtr(machPtr, segNum);
    if (pmegNum != VMMACH_INV_PMEG) {
	pmegArray[pmegNum].lockCount++;
	MASTER_UNLOCK(vmMachMutexPtr);
	return(TRUE);
    } else {
	MASTER_UNLOCK(vmMachMutexPtr);
	return(FALSE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_SetupContext --
 *
 *      Return the value of the context register for the given process.
 *	It is assumed that this routine is called on a uni-processor right
 *	before the process starts executing.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *      The context list is modified.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY ClientData
VmMach_SetupContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    register	VmMach_Context	*contextPtr;

    MASTER_LOCK(vmMachMutexPtr);

    while (TRUE) {
	contextPtr = procPtr->vmPtr->machPtr->contextPtr;
	if (contextPtr != (VmMach_Context *)NIL) {
	    if (contextPtr != &contextArray[VMMACH_KERN_CONTEXT]) {
		if (vm_Tracing) {
		    Vm_ProcInfo	*vmPtr;

		    vmPtr = procPtr->vmPtr;
		    vmPtr->segPtrArray[VM_CODE]->traceTime = vmTraceTime;
		    vmPtr->segPtrArray[VM_HEAP]->traceTime = vmTraceTime;
		    vmPtr->segPtrArray[VM_STACK]->traceTime = vmTraceTime;
		}
		List_Move((List_Links *)contextPtr, LIST_ATREAR(contextList));
	    }
	    MASTER_UNLOCK(vmMachMutexPtr);
	    return((ClientData)contextPtr->context);
	}
        SetupContext(procPtr);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * SetupContext --
 *
 *      Initialize the context for the given process.  If the process does
 *	not have a context associated with it then one is allocated.
 *
 *	Note that this routine runs unsynchronized even though it is using
 *	internal structures.  See the note above while this is OK.  I
 * 	eliminated the monitor lock because it is unnecessary anyway and
 *	it slows down context-switching.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *      The context field in the process table entry and the context list are
 * 	both modified if a new context is allocated.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
SetupContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    register	VmMach_Context	*contextPtr;
    register	VmMach_SegData	*segDataPtr;
    register	Vm_ProcInfo	*vmPtr;
    int		stolenContext	= FALSE;

    vmPtr = procPtr->vmPtr;
    contextPtr = vmPtr->machPtr->contextPtr;

    if (procPtr->genFlags & (PROC_KERNEL | PROC_NO_VM)) {
	/*
	 * This is a kernel process or a process that is exiting.
	 * Set the context to kernel and return.
	 */
	VmMachSetContextReg(VMMACH_KERN_CONTEXT);
	vmPtr->machPtr->contextPtr = &contextArray[VMMACH_KERN_CONTEXT];
	return;
    }

    if (contextPtr == (VmMach_Context *)NIL) {
	/*
	 * In this case there is no context setup for this process.  Therefore
	 * we have to find a context, initialize the context table entry and 
	 * initialize the context stuff in the proc table.
	 */
	if (List_IsEmpty((List_Links *) contextList)) {
	    panic("SetupContext: Context list empty\n");
	}
	/* 
	 * Take the first context off of the context list.
	 */
	contextPtr = (VmMach_Context *) List_First(contextList);
	if (contextPtr->flags & CONTEXT_IN_USE) {
	    contextPtr->procPtr->vmPtr->machPtr->contextPtr =
							(VmMach_Context *)NIL;
	    vmStat.machDepStat.stealContext++;
	    stolenContext = TRUE;
	}
	/*
	 * Initialize the context table entry.
	 */
	contextPtr->flags = CONTEXT_IN_USE;
	contextPtr->procPtr = procPtr;
	vmPtr->machPtr->contextPtr = contextPtr;
	VmMachSetContextReg((int)contextPtr->context);
	if (stolenContext) {
	    VmMach_FlushCurrentContext();
	}
	/*
	 * Set the context map.
	 */
	{
	    int			i;
	    unsigned int	j;

	    /*
	     * Since user addresses are never higher than the bottom of the
	     * hole in the address space, this will save something like 30ms
	     * by ending at VMMACH_BOTTOM_OF_HOLE rather than mach_KernStart.
	     */
	    j = ((unsigned int)VMMACH_BOTTOM_OF_HOLE) >> VMMACH_SEG_SHIFT;
	    for (i = 0; i < j; i++) {
		contextPtr->map[i] = VMMACH_INV_PMEG;
	    }
	}
	segDataPtr = vmPtr->segPtrArray[VM_CODE]->machPtr;
	bcopy((Address)segDataPtr->segTablePtr, 
	    (Address) (contextPtr->map + segDataPtr->offset),
	    segDataPtr->numSegs * sizeof (VMMACH_SEG_NUM));

	segDataPtr = vmPtr->segPtrArray[VM_HEAP]->machPtr;
	bcopy((Address)segDataPtr->segTablePtr, 
		(Address) (contextPtr->map + segDataPtr->offset),
		segDataPtr->numSegs * sizeof (VMMACH_SEG_NUM));

	segDataPtr = vmPtr->segPtrArray[VM_STACK]->machPtr;
	bcopy((Address)segDataPtr->segTablePtr, 
		(Address) (contextPtr->map + segDataPtr->offset),
		segDataPtr->numSegs * sizeof (VMMACH_SEG_NUM));
	if (vmPtr->sharedSegs != (List_Links *)NIL) {
	    Vm_SegProcList *segList;
	    LIST_FORALL(vmPtr->sharedSegs,(List_Links *)segList) {
		segDataPtr = segList->segTabPtr->segPtr->machPtr;
		bcopy((Address)segDataPtr->segTablePtr, 
			(Address) (contextPtr->map+PageToSeg(segList->offset)),
			segDataPtr->numSegs);
	    }
	}
	if (vmPtr->machPtr->mapSegPtr != (struct Vm_Segment *)NIL) {
	    contextPtr->map[MAP_SEG_NUM] = vmPtr->machPtr->mapHardSeg;
	} else {
	    contextPtr->map[MAP_SEG_NUM] = VMMACH_INV_PMEG;
	}
	/*
	 * Push map out to hardware.
	 */
	VmMachCopyUserSegMap(contextPtr->map);
    } else {
	VmMachSetContextReg((int)contextPtr->context);
    }
    List_Move((List_Links *)contextPtr, LIST_ATREAR(contextList));
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_FreeContext --
 *
 *      Free the given context.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The context table and context lists are modified.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_FreeContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    register	VmMach_Context	*contextPtr;
    register	VmMach_ProcData	*machPtr;

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = procPtr->vmPtr->machPtr;
    contextPtr = machPtr->contextPtr;
    if (contextPtr == (VmMach_Context *)NIL ||
        contextPtr->context == VMMACH_KERN_CONTEXT) {
	MASTER_UNLOCK(vmMachMutexPtr);
	return;
    }

    List_Move((List_Links *)contextPtr, LIST_ATFRONT(contextList));
    contextPtr->flags = 0;
    machPtr->contextPtr = (VmMach_Context *)NIL;

    MASTER_UNLOCK(vmMachMutexPtr);
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
 *      The context table and context lists are modified.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_ReinitContext(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    VmMach_FreeContext(procPtr);
    MASTER_LOCK(vmMachMutexPtr);
    procPtr->vmPtr->machPtr->contextPtr = (VmMach_Context *)NIL;
    SetupContext(procPtr);
    MASTER_UNLOCK(vmMachMutexPtr);
}

#if defined(sun2)

static int	 allocatedPMEG;
static VmMachPTE intelSavedPTE;		/* The page table entry that is stored
					 * at the address that the intel page
					 * has to overwrite. */
static unsigned int intelPage;		/* The page frame that was allocated.*/
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_MapIntelPage --
 *
 *      Allocate and validate a page for the Intel Ethernet chip.  This routine
 *	is required in order to initialize the chip.  The chip expects 
 *	certain stuff to be at a specific virtual address when it is 
 *	initialized.  This routine sets things up so that the expected
 *	virtual address is accessible.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The old pte stored at the virtual address and the page frame that is
 *	allocated are stored in static globals.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_MapIntelPage(virtAddr) 
    Address	virtAddr; /* Virtual address where a page has to be validated
			     at. */
{
#if defined(sun2)
    VmMachPTE		pte;
    int			pmeg;

    /*
     * See if there is a PMEG already.  If not allocate one.
     */
    pmeg = VmMachGetSegMap(virtAddr);
    if (pmeg == VMMACH_INV_PMEG) {
	MASTER_LOCK(vmMachMutexPtr);
	/* No flush, since PMEGGet takes care of that. */
	allocatedPMEG = PMEGGet(vm_SysSegPtr, 
				(unsigned)virtAddr >> VMMACH_SEG_SHIFT,
				PMEG_DONT_ALLOC);
	MASTER_UNLOCK(vmMachMutexPtr);
	VmMachSetSegMap(virtAddr, allocatedPMEG);
    } else {
	allocatedPMEG = VMMACH_INV_PMEG;
	intelSavedPTE = VmMachGetPageMap(virtAddr);
    }

    /*
     * Set up the page table entry.
     */
    intelPage = Vm_KernPageAllocate();
    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | VirtToPhysPage(intelPage);
#ifdef sun4
#ifdef NOTDEF
    pte |= VMMACH_DONT_CACHE_BIT;
#endif
#endif /* sun4 */
    /* No flush since this should never be cached. */
    VmMachSetPageMap(virtAddr, pte);
#endif /* sun2  */
#ifdef sun4
    VmMachPTE		pte;
    int			pmeg;
    int			oldContext;
    int			i;

    /*
     * See if there is a PMEG already.  If not allocate one.
     */
    pmeg = VmMachGetSegMap(virtAddr);
    if (pmeg == VMMACH_INV_PMEG) {
	MASTER_LOCK(vmMachMutexPtr);
	/* No flush, since PMEGGet takes care of that. */
	pmeg = PMEGGet(vm_SysSegPtr, 
				(int)((unsigned)virtAddr) >> VMMACH_SEG_SHIFT,
				PMEG_DONT_ALLOC);
	MASTER_UNLOCK(vmMachMutexPtr);
	VmMachSetSegMap(virtAddr, pmeg);
    } 
    oldContext = VmMachGetContextReg();
    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	VmMachSetContextReg(i);
	VmMachSetSegMap(virtAddr, pmeg);
    }
    VmMachSetContextReg(oldContext);
    /*
     * Set up the page table entry.
     */
    pte = VmMachGetPageMap(virtAddr);
    if (pte == 0) {
	pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | VMMACH_DONT_CACHE_BIT |
	      VirtToPhysPage(Vm_KernPageAllocate());
	SET_ALL_PAGE_MAP(virtAddr, pte);
    } 
#endif
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_UnmapIntelPage --
 *
 *      Deallocate and invalidate a page for the intel chip.  This is a special
 *	case routine that is only for the intel ethernet chip.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The hardware segment table associated with the segment
 *      is modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_UnmapIntelPage(virtAddr) 
    Address	virtAddr;
{
#if defined(sun2)
    PMEG		*pmegPtr;
    Boolean	found = FALSE;

    if (allocatedPMEG != VMMACH_INV_PMEG) {
	/*
	 * Free up the PMEG.
	 */
	/* No flush since this should never be cached. */
	VmMachSetPageMap(virtAddr, (VmMachPTE)0);
	VmMachSetSegMap(virtAddr, VMMACH_INV_PMEG);

	MASTER_LOCK(vmMachMutexPtr);

	/*
	 * This is a little gross, but for some reason this pmeg has a 0
	 * page count.  Since it has the PMEG_DONT_ALLOC flag set, it wasn't
	 * removed from the pmegInuseList in PMEGGet().  So the remove done
	 * in PMEGFree would remove it twice, unless we check.
	 */
	LIST_FORALL(pmegInuseList, (List_Links *)pmegPtr) {
	    if (pmegPtr == &pmegArray[allocatedPMEG]) {
		found = TRUE;
	    }
	}
	if (!found) {
	    pmegPtr = &pmegArray[allocatedPMEG];
	    List_Insert((List_Links *) pmegPtr, LIST_ATREAR(pmegInuseList));
	}
	PMEGFree(allocatedPMEG);

	MASTER_UNLOCK(vmMachMutexPtr);
    } else {
	/*
	 * Restore the saved pte and free the allocated page.
	 */
	/* No flush since this should never be cached. */
	VmMachSetPageMap(virtAddr, intelSavedPTE);
    }
    Vm_KernPageFree(intelPage);
#endif
}


static Address		netMemAddr;
static unsigned int	netLastPage;


/*
 * ----------------------------------------------------------------------------
 *
 * InitNetMem --
 *
 *      Initialize the memory mappings for the network.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      PMEGS are allocated and initialized.
 *
 * ----------------------------------------------------------------------------
 */
static void
InitNetMem()
{
    VMMACH_SEG_NUM		pmeg;
    register VMMACH_SEG_NUM	*segTablePtr;
    int				i;
    int				j;
    int				lastSegNum;
    int				segNum;
    Address			virtAddr;

    /*
     * Allocate pmegs  for net mapping.
     */
    segNum = ((unsigned)VMMACH_NET_MAP_START) >> VMMACH_SEG_SHIFT;
    lastSegNum = ((unsigned)(VMMACH_NET_MAP_START+VMMACH_NET_MAP_SIZE-1)) /
			  VMMACH_SEG_SIZE;

    for (i = 0, virtAddr = (Address)VMMACH_NET_MAP_START,
	    segTablePtr = GetHardSegPtr(vm_SysSegPtr->machPtr, segNum);
	 segNum <= lastSegNum;
         i++, virtAddr += VMMACH_SEG_SIZE, segNum++) {
	pmeg = VmMachGetSegMap(virtAddr);
	if (pmeg == VMMACH_INV_PMEG) {
	    *(segTablePtr + i) = PMEGGet(vm_SysSegPtr, segNum, PMEG_DONT_ALLOC);
	    VmMachSetSegMap(virtAddr, (int)*(segTablePtr + i));
	} else {
	    *(segTablePtr + i) = pmeg;
	}
	/*
	 * Propagate the new pmeg mapping to all contexts.
	 */
	for (j = 0; j < VMMACH_NUM_CONTEXTS; j++) {
	    if (j == VMMACH_KERN_CONTEXT) {
		continue;
	    }
	    VmMachSetContextReg(j);
	    VmMachSetSegMap(virtAddr, (int)*(segTablePtr + i));
	}
	VmMachSetContextReg(VMMACH_KERN_CONTEXT);
    }
    /*
     * Repeat for the network memory range. 
     */
    segNum = ((unsigned)VMMACH_NET_MEM_START) >> VMMACH_SEG_SHIFT;
    lastSegNum = ((unsigned)(VMMACH_NET_MEM_START+VMMACH_NET_MEM_SIZE-1)) /
			 VMMACH_SEG_SIZE;

    for (i = 0, virtAddr = (Address)VMMACH_NET_MEM_START,
	    segTablePtr = GetHardSegPtr(vm_SysSegPtr->machPtr, segNum);
	 segNum <= lastSegNum;
         i++, virtAddr += VMMACH_SEG_SIZE, segNum++) {
	pmeg = VmMachGetSegMap(virtAddr);
	if (pmeg == VMMACH_INV_PMEG) {
	    *(segTablePtr + i) = PMEGGet(vm_SysSegPtr, segNum, PMEG_DONT_ALLOC);
	    VmMachSetSegMap(virtAddr, (int)*(segTablePtr + i));
	} else {
	    *(segTablePtr + i) = pmeg;
	}
	/*
	 * Propagate the new pmeg mapping to all contexts.
	 */
	for (j = 0; j < VMMACH_NUM_CONTEXTS; j++) {
	    if (j == VMMACH_KERN_CONTEXT) {
		continue;
	    }
	    VmMachSetContextReg(j);
	    VmMachSetSegMap(virtAddr, (int)*(segTablePtr + i));
	}
	VmMachSetContextReg(VMMACH_KERN_CONTEXT);
    }

    netMemAddr = (Address)VMMACH_NET_MEM_START;
    netLastPage = (((unsigned)VMMACH_NET_MEM_START) >> VMMACH_PAGE_SHIFT) - 1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_NetMemAlloc --
 *
 *      Allocate physical memory for a network driver.
 *
 * Results:
 *      The address where the memory is allocated at.
 *
 * Side effects:
 *      Memory allocated.
 *
 * ----------------------------------------------------------------------------
 */
Address
VmMach_NetMemAlloc(numBytes)
    int	numBytes;	/* Number of bytes of memory to allocated. */
{
    VmMachPTE	pte;
    Address	retAddr;
    Address	maxAddr;
    Address	virtAddr;
    static Boolean initialized = FALSE;

    if (!initialized) {
	InitNetMem();
	initialized = TRUE;
    }

    retAddr = netMemAddr;
    netMemAddr += (numBytes + 7) & ~7;	/* is this necessary for sun4? */
    /*
     * Panic if we are out of memory.  
     */
    if (netMemAddr > (Address) (VMMACH_NET_MEM_START + VMMACH_NET_MEM_SIZE)) {
	panic("VmMach_NetMemAlloc: Out of network memory\n");
    }

    maxAddr = (Address) ((netLastPage + 1) * VMMACH_PAGE_SIZE - 1);

    /*
     * Add new pages to the virtual address space until we have added enough
     * to handle this memory request.
     */
    while (netMemAddr - 1 > maxAddr) {
	maxAddr += VMMACH_PAGE_SIZE;
	netLastPage++;
	virtAddr = (Address) (netLastPage << VMMACH_PAGE_SHIFT);
	pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT |
#ifdef sun4c
	      /*
	       * For some reason on the sparcStation, we can't allow the
	       * network pages to be cached. This is really a problem and it
	       * totally breaks the driver.
	       */
	      VMMACH_DONT_CACHE_BIT | 
#endif
	      VirtToPhysPage(Vm_KernPageAllocate());
	SET_ALL_PAGE_MAP(virtAddr, pte);
    }

    bzero(retAddr, numBytes);
    return(retAddr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_NetMapPacket --
 *
 *	Map the packet pointed to by the scatter-gather array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The outScatGathArray is filled in with pointers to where the
 *	packet was mapped in.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_NetMapPacket(inScatGathPtr, scatGathLength, outScatGathPtr)
    register Net_ScatterGather	*inScatGathPtr;
    register int		scatGathLength;
    register Net_ScatterGather	*outScatGathPtr;
{
    register Address	mapAddr;
    register Address	endAddr;
    int			segNum;
    int			pageNum = 0;
#ifdef sun4c
    /*
     * The network driver on the sparcstation never accesses the data
     * through the cache, so there should be no need to flush it.
     */
    for (mapAddr = (Address)VMMACH_NET_MAP_START;
         scatGathLength > 0;
         scatGathLength--, inScatGathPtr++, outScatGathPtr++) {
        outScatGathPtr->length = inScatGathPtr->length;
        if (inScatGathPtr->length == 0) {
            continue;
        }
        /*
         * Map the piece of the packet in.  Note that we know that a packet
         * piece is no longer than 1536 bytes so we know that we will need
         * at most two page table entries to map a piece in.
         */
        VmMachSetPageMap(mapAddr, VmMachGetPageMap(inScatGathPtr->bufAddr));
        outScatGathPtr->bufAddr =
            mapAddr + ((unsigned)inScatGathPtr->bufAddr & VMMACH_OFFSET_MASK);
        mapAddr += VMMACH_PAGE_SIZE_INT;
        endAddr = inScatGathPtr->bufAddr + inScatGathPtr->length - 1;
        if (((unsigned)inScatGathPtr->bufAddr & ~VMMACH_OFFSET_MASK_INT) !=
            ((unsigned)endAddr & ~VMMACH_OFFSET_MASK_INT)) {
            VmMachSetPageMap(mapAddr, VmMachGetPageMap(endAddr));
            mapAddr += VMMACH_PAGE_SIZE_INT;
        }
    }
#else
    for (segNum = 0 ; scatGathLength > 0;
	    scatGathLength--, inScatGathPtr++, outScatGathPtr++) {
	outScatGathPtr->length = inScatGathPtr->length;
	if (inScatGathPtr->length == 0) {
	    continue;
	}
	/*
	 * For the first (VMMACH_NUM_NET_SEGS) - 1 elements in the
	 * scatter gather array, map each element into a segment, aligned
	 * with the mapping pages so that cache flushes will be avoided.
	 */
	if (segNum < VMMACH_NUM_NET_SEGS - 1) {
	    /* do silly mapping */
	    mapAddr = (Address)VMMACH_NET_MAP_START +
		    (segNum * VMMACH_SEG_SIZE);
	    /* align to same cache boundary */
	    mapAddr += ((unsigned int)inScatGathPtr->bufAddr &
		    (VMMACH_CACHE_SIZE - 1));
	    /* set addr to beginning of page */
	    (unsigned int) mapAddr &= ~VMMACH_OFFSET_MASK;
	    VmMachFlushPage(mapAddr);
	    VmMachSetPageMap(mapAddr, VmMachGetPageMap(inScatGathPtr->bufAddr));
	    outScatGathPtr->bufAddr = (Address) ((unsigned)mapAddr +
		    ((unsigned)inScatGathPtr->bufAddr & VMMACH_OFFSET_MASK));
	    mapAddr += VMMACH_PAGE_SIZE_INT;
	    endAddr = (Address)inScatGathPtr->bufAddr +
		    inScatGathPtr->length - 1;
	    if (((unsigned)inScatGathPtr->bufAddr & ~VMMACH_OFFSET_MASK_INT) !=
		((unsigned)endAddr & ~VMMACH_OFFSET_MASK_INT)) {
		VmMachFlushPage(mapAddr);
		VmMachSetPageMap(mapAddr, VmMachGetPageMap(endAddr));
	    }
	    segNum++;
	} else {
	    /*
	     * For elements beyond the last one, map them all into the
	     * last mapping segment.  Cache flushing will be necessary for
	     * these.
	     */
	    mapAddr = (Address)VMMACH_NET_MAP_START +
		    (segNum * VMMACH_SEG_SIZE) + (pageNum * VMMACH_PAGE_SIZE);
	    VmMachFlushPage(inScatGathPtr->bufAddr);
	    VmMachFlushPage(mapAddr);
	    VmMachSetPageMap(mapAddr, VmMachGetPageMap(inScatGathPtr->bufAddr));
	    outScatGathPtr->bufAddr = (Address) ((unsigned)mapAddr +
		    ((unsigned)inScatGathPtr->bufAddr & VMMACH_OFFSET_MASK));
	    mapAddr += VMMACH_PAGE_SIZE_INT;
	    pageNum++;
	    endAddr = (Address)inScatGathPtr->bufAddr +
		    inScatGathPtr->length - 1;
	    if (((unsigned)inScatGathPtr->bufAddr & ~VMMACH_OFFSET_MASK_INT) !=
		((unsigned)endAddr & ~VMMACH_OFFSET_MASK_INT)) {
		VmMachFlushPage(endAddr);
		VmMachFlushPage(mapAddr);
		VmMachSetPageMap(mapAddr, VmMachGetPageMap(endAddr));
		pageNum++;
	    }
	    printf("MapPacket: segNum is %d, pageNum is %d\n", segNum, pageNum);
	}
    }
#endif /* sun4c */
}



/*
 *----------------------------------------------------------------------
 *
 * VmMach_VirtAddrParse --
 *
 *	See if the given address falls into the special mapping segment.
 *	If so parse it for our caller.
 *
 * Results:
 *	TRUE if the address fell into the special mapping segment, FALSE
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
    Address	origVirtAddr;
    Boolean	retVal;

#ifdef sun4
    if (!VMMACH_ADDR_CHECK(virtAddr)) {
	panic("VmMach_VirtAddrParse: virt addr 0x%x falls in illegal range!\n",
		virtAddr);
    }
#endif sun4
    if (virtAddr >= (Address)VMMACH_MAP_SEG_ADDR && 
        virtAddr < (Address)mach_KernStart) {
	/*
	 * The address falls into the special mapping segment.  Translate
	 * the address back to the segment that it falls into.
	 */
	origVirtAddr = 
	    (Address)(procPtr->vmPtr->machPtr->mapHardSeg << VMMACH_SEG_SHIFT);
	origVirtAddr += (unsigned int)virtAddr & (VMMACH_SEG_SIZE - 1);
	transVirtAddrPtr->segPtr = procPtr->vmPtr->machPtr->mapSegPtr;
	transVirtAddrPtr->page = (unsigned) (origVirtAddr) >> VMMACH_PAGE_SHIFT;
	transVirtAddrPtr->offset = (unsigned)virtAddr & VMMACH_OFFSET_MASK;
	transVirtAddrPtr->flags = USING_MAPPED_SEG;
	retVal = TRUE;
    } else {
	retVal = FALSE;
    }
    return(retVal);
}

/*
 * We use this array to flush the cache by touching entries at the correct
 * offsets to clear the corresponding parts of the direct-mapped cache.
 */
volatile char	cacheFlusherArray[VMMACH_NUM_CACHE_LINES *
	VMMACH_CACHE_LINE_SIZE * 2];


/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushCacheRange --
 *
 *	Flush the cache in the given range of addresses, inclusive,
 *	no matter what context we are in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cache flushed in range of addresses.
 *
 *----------------------------------------------------------------------
 */
static void
VmMachFlushCacheRange(startAddr, endAddr)
    Address	startAddr;		/* first address to flush */
    Address	endAddr;		/* last address to flush */
{
    char	*beginFlush, *endFlush;
    char	dummy;
    char	*cacheFlushPtr;
#define	 VMMACH_CACHE_MASK (VMMACH_CACHE_SIZE - 1)
#define	 VMMACH_LINE_MASK (VMMACH_CACHE_LINE_SIZE - 1)

    if ((unsigned int )endAddr -
	    (unsigned int )startAddr >= VMMACH_CACHE_SIZE) {
	FlushWholeCache();

	return;
    }
    
	
    /* Offset into array where entry maps to 1st entry of cache. */
    cacheFlushPtr = (char *) (((unsigned int) (cacheFlusherArray +
	    VMMACH_CACHE_SIZE)) & ~VMMACH_CACHE_MASK);
    /* Get bits that map address into cache location. */
    beginFlush = (char *)((((unsigned int)startAddr) & ~VMMACH_LINE_MASK) &
	    VMMACH_CACHE_MASK);
    endFlush = (char *)((((unsigned int) endAddr) & ~VMMACH_LINE_MASK) &
	    VMMACH_CACHE_MASK);
    beginFlush = cacheFlushPtr + ((unsigned int) beginFlush);
    endFlush = cacheFlushPtr + ((unsigned int) endFlush);

    while (beginFlush <= endFlush) {
	dummy = *beginFlush;
	beginFlush += VMMACH_CACHE_LINE_SIZE;
    }
#ifdef lint
    dummy = dummy;
#endif
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FlushWholeCache --
 *
 *	Flush the whole cache.  It is important that the compiler not
 *	optimize out this loop!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cache flushed.
 *
 *----------------------------------------------------------------------
 */
static void
FlushWholeCache()
{
    int	i;
    char junk;
    register volatile char *cacheFlusherArrayPtr;

    cacheFlusherArrayPtr = cacheFlusherArray;
    for (i = 0; i < VMMACH_CACHE_SIZE; i += VMMACH_CACHE_LINE_SIZE) {
	junk = cacheFlusherArrayPtr[i];
    }
    return;
}

#if 0 /* dead code shirriff 9/90 */
char	bigBuf[VMMACH_PAGE_SIZE_INT * 2];
ReturnStatus
VmMachTestCacheFlush()
{
    unsigned int	pte1;
    unsigned int	pte2;
    unsigned int	savePte1;
    unsigned int	savePte2;
    Address	virtAddr1;
    Address	virtAddr2;
    unsigned	int	saveValue11, saveValue12, saveValue21, saveValue22;
    unsigned	int	saveValue3;
    unsigned	int	endSave3;
    unsigned	int	endSave11, endSave12, endSave21, endSave22;


    /* first addr at first page */
    virtAddr1 = (Address) bigBuf;
    /* first addr's pte */
    pte1 = VmMachGetPageMap(virtAddr1);
    pte2 = VmMachGetPageMap(virtAddr1 + 254);

    /* second addr at second page */
    virtAddr2 = (Address) (bigBuf + VMMACH_PAGE_SIZE_INT);
    /* save second addr's pte */
    savePte1 = VmMachGetPageMap(virtAddr2);
    savePte2 = VmMachGetPageMap(virtAddr2 + 254);

    /* flush second page's virtAddrs out of cache */
    VmMachFlushPage(virtAddr2);
    VmMachFlushPage(virtAddr2 + 254);
    /* give second addr the same pte as first */
    VmMachSetPageMap(virtAddr2, pte1);
    VmMachSetPageMap(virtAddr2 + 254, pte2);

    /* get current virtAddr1 value */
    saveValue11 = *virtAddr1;
    endSave11 = *(virtAddr1 + 254);

    /* get current virtAddr2 value - after flush, so really from memory */
    saveValue21 = *virtAddr2;
    endSave21 = *(virtAddr2 + 254);

    /* flush virtAddr2 from memory again */
    VmMachFlushPage(virtAddr2);
    VmMachFlushPage(virtAddr2 + 254);

    /* is something weird?  saveValue12 should equal saveValue11 */
    saveValue12 = *virtAddr1;
    endSave12 = *(virtAddr1 + 254);

    /* give virtAddr1 new value */
    *virtAddr1 = ~saveValue11;
    *(virtAddr1 + 254) = ~endSave11;

    /* should only be in cache */
    saveValue3 = *virtAddr2;
    endSave3 = *(virtAddr2 + 254);
    if (saveValue3 == ~saveValue11) {
	return FAILURE;
    }
    if (endSave3 == ~endSave11) {
	return FAILURE;
    }
    /* get rid of virtAddr2 again */
    VmMachFlushPage(virtAddr2);
    VmMachFlushPage(virtAddr2 + 254);

    /* Now try icky range flush */
    VmMachFlushCacheRange(virtAddr1, virtAddr1 + 254);

    /* Did it get to memory? */
    saveValue22 = *virtAddr2;
    endSave22 = *(virtAddr2 + 254);

    /* restore pte of second addr */
    VmMachSetPageMap(virtAddr2, savePte1);
    VmMachSetPageMap(virtAddr2 + 254, savePte2);

    if (saveValue22 != ~saveValue11) {
	return FAILURE;
    }
    if (endSave22 != ~endSave11) {
	return FAILURE;
    }
#ifdef lint
    endSave21 = endSave21;
    endSave12 = endSave12;
    saveValue21 = saveValue21;
    saveValue12 = saveValue12;
#endif
    return SUCCESS;
}
#endif /* dead code */


/*
 *----------------------------------------------------------------------
 *
 * VmMach_CopyInProc --
 *
 *	Copy from another process's address space into the current address
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
    ReturnStatus		status = SUCCESS;
    register VmMach_ProcData	*machPtr;
    Proc_ControlBlock		*toProcPtr;
    int				segOffset;
    int				bytesToCopy;
    int				oldContext;

    toProcPtr = Proc_GetCurrentProc();
    machPtr = toProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->mapHardSeg = (unsigned int) (fromAddr) >> VMMACH_SEG_SHIFT;
#ifdef sun4
    /*
     * Since this is a cross-address-space copy, we must make sure everything
     * has been flushed to the stack from our windows so that we don't
     * miss stuff on the stack not yet flushed. 
     */
    Mach_FlushWindowsToStack();
#endif

    /*
     * Do a hardware segment's worth at a time until done.
     */
    while (numBytes > 0 && status == SUCCESS) {
	segOffset = (unsigned int)fromAddr & (VMMACH_SEG_SIZE - 1);
	bytesToCopy = VMMACH_SEG_SIZE - segOffset;
	if (bytesToCopy > numBytes) {
	    bytesToCopy = numBytes;
	}
	/*
	 * First try quick and dirty copy.  If it fails, do regular copy.
	 */
	if (fromProcPtr->vmPtr->machPtr->contextPtr != (VmMach_Context *)NIL) {
	    unsigned int	fromContext;
	    unsigned int	toContext;

	    toContext = VmMachGetContextReg();
	    fromContext = fromProcPtr->vmPtr->machPtr->contextPtr->context;

	    status = VmMachQuickNDirtyCopy(bytesToCopy, fromAddr, toAddr,
		fromContext, toContext);
	    VmMachSetContextReg((int)toContext);

	    if (status == SUCCESS) {
		numBytes -= bytesToCopy;
		fromAddr += bytesToCopy;
		toAddr += bytesToCopy;
		continue;
	    }
	}
	/*
	 * Flush segment in context of fromProcPtr.  If the context is NIL, then
	 * we can't and don't have to flush it, since it will be flushed
	 * before being reused.
	 */
	if (fromProcPtr->vmPtr->machPtr->contextPtr != (VmMach_Context *)NIL) {
	    oldContext = VmMachGetContextReg();
	    VmMachSetContextReg(
		    (int)fromProcPtr->vmPtr->machPtr->contextPtr->context);
	    VmMachFlushSegment(fromAddr);
	    VmMachSetContextReg(oldContext);
	}
	/*
	 * Push out the hardware segment.
	 */
	WriteHardMapSeg(machPtr);
	/*
	 * Do the copy.
	 */
	toProcPtr->vmPtr->vmFlags |= VM_COPY_IN_PROGRESS;
	status = VmMachDoCopy(bytesToCopy,
			      (Address)(VMMACH_MAP_SEG_ADDR + segOffset),
			      toAddr);
	toProcPtr->vmPtr->vmFlags &= ~VM_COPY_IN_PROGRESS;
	if (status == SUCCESS) {
	    numBytes -= bytesToCopy;
	    fromAddr += bytesToCopy;
	    toAddr += bytesToCopy;
	} else {
	    status = SYS_ARG_NOACCESS;
	}
	/*
	 * Zap the hardware segment.
	 */
	VmMachFlushSegment((Address) VMMACH_MAP_SEG_ADDR);
	VmMachSetSegMap((Address)VMMACH_MAP_SEG_ADDR, VMMACH_INV_PMEG); 
	machPtr->mapHardSeg++;
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
    ReturnStatus		status = SUCCESS;
    register VmMach_ProcData	*machPtr;
    Proc_ControlBlock		*fromProcPtr;
    int				segOffset;
    int				bytesToCopy;
    int				oldContext;


    fromProcPtr = Proc_GetCurrentProc();
    machPtr = fromProcPtr->vmPtr->machPtr;
    machPtr->mapSegPtr = virtAddrPtr->segPtr;
    machPtr->mapHardSeg = (unsigned int) (toAddr) >> VMMACH_SEG_SHIFT;

#ifdef sun4
    /*
     * Since this is a cross-address-space copy, we must make sure everything
     * has been flushed to the stack from our windows so that we don't
     * get stuff from windows overwriting stuff we copy to the stack later
     * when they're flushed.
     */
    Mach_FlushWindowsToStack();
#endif

    /*
     * Do a hardware segment's worth at a time until done.
     */
    while (numBytes > 0 && status == SUCCESS) {
	segOffset = (unsigned int)toAddr & (VMMACH_SEG_SIZE - 1);
	bytesToCopy = VMMACH_SEG_SIZE - segOffset;
	if (bytesToCopy > numBytes) {
	    bytesToCopy = numBytes;
	}
	/*
	 * First try quick and dirty copy.  If it fails, do regular copy.
	 */
	if (toProcPtr->vmPtr->machPtr->contextPtr != (VmMach_Context *)NIL) {
	    unsigned int	fromContext;
	    unsigned int	toContext;

	    fromContext = VmMachGetContextReg();
	    toContext = toProcPtr->vmPtr->machPtr->contextPtr->context;

	    status = VmMachQuickNDirtyCopy(bytesToCopy, fromAddr, toAddr,
		    fromContext, toContext);
	    VmMachSetContextReg((int)fromContext);

	    if (status == SUCCESS) {
		numBytes -= bytesToCopy;
		fromAddr += bytesToCopy;
		toAddr += bytesToCopy;
		continue;
	    }
	}
	/*
	 * Flush segment in context of toProcPtr.  If the context is NIL, then
	 * we can't and don't have to flush it, since it will be flushed before
	 * being re-used.
	 */
	if (toProcPtr->vmPtr->machPtr->contextPtr != (VmMach_Context *)NIL) {
	    oldContext = VmMachGetContextReg();
	    VmMachSetContextReg(
		    (int)toProcPtr->vmPtr->machPtr->contextPtr->context);
	    VmMachFlushSegment(toAddr);
	    VmMachSetContextReg(oldContext);
	}
	/*
	 * Push out the hardware segment.
	 */
	WriteHardMapSeg(machPtr);
	/*
	 * Do the copy.
	 */
	fromProcPtr->vmPtr->vmFlags |= VM_COPY_IN_PROGRESS;
	status = VmMachDoCopy(bytesToCopy, fromAddr,
			  (Address) (VMMACH_MAP_SEG_ADDR + segOffset));
	fromProcPtr->vmPtr->vmFlags &= ~VM_COPY_IN_PROGRESS;
	if (status == SUCCESS) {
	    numBytes -= bytesToCopy;
	    fromAddr += bytesToCopy;
	    toAddr += bytesToCopy;
	} else {
	    status = SYS_ARG_NOACCESS;
	}
	/*
	 * Zap the hardware segment.
	 */

	/* Flush this in current context */
	VmMachFlushSegment((Address) VMMACH_MAP_SEG_ADDR);
	VmMachSetSegMap((Address)VMMACH_MAP_SEG_ADDR, VMMACH_INV_PMEG); 

	machPtr->mapHardSeg++;
    }
    machPtr->mapSegPtr = (struct Vm_Segment *)NIL;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * WriteHardMapSeg --
 *
 *	Push the hardware segment map entry out to the hardware for the
 *	given mapped segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hardware segment modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
WriteHardMapSeg(machPtr)
    VmMach_ProcData	*machPtr;
{
    MASTER_LOCK(vmMachMutexPtr);

    if (machPtr->contextPtr != (VmMach_Context *) NIL) {
	machPtr->contextPtr->map[MAP_SEG_NUM] = 
	    (int)*GetHardSegPtr(machPtr->mapSegPtr->machPtr,
	    machPtr->mapHardSeg);
    }
    VmMachSetSegMap((Address)VMMACH_MAP_SEG_ADDR, 
	 (int)*GetHardSegPtr(machPtr->mapSegPtr->machPtr, machPtr->mapHardSeg));

    MASTER_UNLOCK(vmMachMutexPtr);
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
    register	VmMachPTE	pte;
    register	Address		virtAddr;
    register	VMMACH_SEG_NUM	*pmegNumPtr;
    register	PMEG		*pmegPtr;
    register	Boolean		skipSeg = FALSE;
    Boolean			nextSeg = TRUE;
    Address			tVirtAddr;
    Address			pageVirtAddr;
    int				i;

    MASTER_LOCK(vmMachMutexPtr);

    pmegNumPtr = (VMMACH_SEG_NUM *)
	    GetHardSegPtr(segPtr->machPtr, PageToSeg(firstPage)) - 1;
    virtAddr = (Address)(firstPage << VMMACH_PAGE_SHIFT);
    while (firstPage <= lastPage) {
	if (nextSeg) {
	    pmegNumPtr++;
	    if (*pmegNumPtr != VMMACH_INV_PMEG) {
		pmegPtr = &pmegArray[*pmegNumPtr];
		if (pmegPtr->pageCount != 0) {
		    /* flush a range, so we don't care what context we're in. */
		    VmMachFlushCacheRange(virtAddr, (Address)
			    (((((unsigned int) virtAddr) + VMMACH_SEG_SIZE) &
			    ~(VMMACH_SEG_SIZE - 1)) - 1));
		    VmMachSetSegMap(vmMachPTESegAddr, (int)*pmegNumPtr);
		    skipSeg = FALSE;
		} else {
		    skipSeg = TRUE;
		}
	    } else {
		skipSeg = TRUE;
	    }
	    nextSeg = FALSE;
	}
	if (!skipSeg) {
	    /*
	     * Change the hardware page table.
	     */
	    tVirtAddr =
		((unsigned int)virtAddr & VMMACH_PAGE_MASK) + vmMachPTESegAddr;
	    for (i = 0; i < VMMACH_CLUSTER_SIZE; i++) {
		pageVirtAddr = tVirtAddr + i * VMMACH_PAGE_SIZE_INT;
		pte = VmMachGetPageMap(pageVirtAddr);
		if (pte & VMMACH_RESIDENT_BIT) {
		    Vm_TracePTEChange	pteChange;
		    if (vm_Tracing) {
			pteChange.changeType = VM_TRACE_SET_SEG_PROT;
			pteChange.segNum = segPtr->segNum;
			pteChange.pageNum = firstPage;
			pteChange.softPTE = FALSE;
			pteChange.beforePTE = pte;
		    }
		    pte &= ~VMMACH_PROTECTION_FIELD;
		    pte |= makeWriteable ? VMMACH_URW_PROT : VMMACH_UR_PROT;
		    if (vm_Tracing) {
			pteChange.afterPTE = pte;
			VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC,
					 sizeof(Vm_TracePTEChange),
					 (Address)&pteChange, TRUE);
		    }
#ifdef sun4
		    if (virtAddr >= vmStackEndAddr) {
			pte |= VMMACH_DONT_CACHE_BIT;
		    } else {
			pte &= ~VMMACH_DONT_CACHE_BIT;
		    }
#endif sun4
		    VmMachSetPageMap(pageVirtAddr, pte);
		}
	    }
	    virtAddr += VMMACH_PAGE_SIZE;
	    firstPage++;
	    if (((unsigned int)virtAddr & VMMACH_PAGE_MASK) == 0) {
		nextSeg = TRUE;
	    }
	} else {
	    int	segNum;

	    segNum = PageToSeg(firstPage) + 1;
	    firstPage = SegToPage(segNum);
	    virtAddr = (Address)(firstPage << VMMACH_PAGE_SHIFT);
	    nextSeg = TRUE;
	}
    }

    MASTER_UNLOCK(vmMachMutexPtr);
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
    register	VmMachPTE 	hardPTE;
    register	VmMach_SegData	*machPtr;
    Address   			virtAddr;
    int				pmegNum;
    int				i;
    Vm_TracePTEChange		pteChange;
#ifdef sun4
    Address			testVirtAddr;
#endif sun4

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = virtAddrPtr->segPtr->machPtr;
    pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
	    virtAddrPtr));
    if (pmegNum != VMMACH_INV_PMEG) {
	virtAddr = ((virtAddrPtr->page << VMMACH_PAGE_SHIFT) & 
			VMMACH_PAGE_MASK) + vmMachPTESegAddr;	
#ifdef sun4
	testVirtAddr = (Address) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
#endif sun4
	for (i = 0; 
	     i < VMMACH_CLUSTER_SIZE; 
	     i++, virtAddr += VMMACH_PAGE_SIZE_INT) {
	    hardPTE = VmMachReadPTE(pmegNum, virtAddr);
	    if (vm_Tracing) {
		pteChange.changeType = VM_TRACE_SET_PAGE_PROT;
		pteChange.segNum = virtAddrPtr->segPtr->segNum;
		pteChange.pageNum = virtAddrPtr->page;
		pteChange.softPTE = FALSE;
		pteChange.beforePTE = hardPTE;
	    }
	    hardPTE &= ~VMMACH_PROTECTION_FIELD;
	    hardPTE |= (softPTE & (VM_COW_BIT | VM_READ_ONLY_PROT)) ? 
					VMMACH_UR_PROT : VMMACH_URW_PROT;
	    if (vm_Tracing) {
		pteChange.afterPTE = hardPTE;
		VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC,
				 sizeof(Vm_TracePTEChange), 
				 (Address)&pteChange, TRUE);
	    }
#ifdef sun4
	    if (testVirtAddr >= vmStackEndAddr) {
		hardPTE |= VMMACH_DONT_CACHE_BIT;
	    } else {
		hardPTE &= ~VMMACH_DONT_CACHE_BIT;
	    }
#endif sun4
	    /* flush a range so we don't care what context we're in. */
	    VmMachFlushCacheRange(testVirtAddr, (Address)
		    (((((unsigned int) testVirtAddr) + VMMACH_PAGE_SIZE &
		    ~(VMMACH_PAGE_SIZE - 1)) - 1)));
	    VmMachWritePTE(pmegNum, virtAddr, hardPTE);
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
    register VmMach_SegData	*machPtr;
    register VmMachPTE 		hardPTE;  
    int				pmegNum; 
    Address			virtAddr;
    int				i;
    int				origMod;

    MASTER_LOCK(vmMachMutexPtr);

    origMod = *modPtr;

    *refPtr |= refModMap[virtFrameNum] & VMMACH_REFERENCED_BIT;
    *modPtr = refModMap[virtFrameNum] & VMMACH_MODIFIED_BIT;
    if (!*refPtr || !*modPtr) {
	machPtr = virtAddrPtr->segPtr->machPtr;
	pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
		virtAddrPtr));
	if (pmegNum != VMMACH_INV_PMEG) {
	    hardPTE = 0;
	    virtAddr = 
		((virtAddrPtr->page << VMMACH_PAGE_SHIFT) & VMMACH_PAGE_MASK) + 
		    vmMachPTESegAddr;
	    for (i = 0; i < VMMACH_CLUSTER_SIZE; i++ ) {
		hardPTE |= VmMachReadPTE(pmegNum, 
					virtAddr + i * VMMACH_PAGE_SIZE_INT);
	    }
	    *refPtr |= hardPTE & VMMACH_REFERENCED_BIT;
	    *modPtr |= hardPTE & VMMACH_MODIFIED_BIT;
	}
    }
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
ENTRY void
VmMach_GetRefModBits(virtAddrPtr, virtFrameNum, refPtr, modPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned	int		virtFrameNum;
    register	Boolean		*refPtr;
    register	Boolean		*modPtr;
{
    register VmMach_SegData	*machPtr;
    register VmMachPTE 		hardPTE;  
    int				pmegNum; 
    Address			virtAddr;
    int				i;

    MASTER_LOCK(vmMachMutexPtr);

    *refPtr = refModMap[virtFrameNum] & VMMACH_REFERENCED_BIT;
    *modPtr = refModMap[virtFrameNum] & VMMACH_MODIFIED_BIT;
    if (!*refPtr || !*modPtr) {
	machPtr = virtAddrPtr->segPtr->machPtr;
	pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
		virtAddrPtr));
	if (pmegNum != VMMACH_INV_PMEG) {
	    hardPTE = 0;
	    virtAddr = 
		((virtAddrPtr->page << VMMACH_PAGE_SHIFT) & VMMACH_PAGE_MASK) + 
		    vmMachPTESegAddr;
	    for (i = 0; i < VMMACH_CLUSTER_SIZE; i++ ) {
		hardPTE |= VmMachReadPTE(pmegNum, 
					virtAddr + i * VMMACH_PAGE_SIZE_INT);
	    }
	    if (!*refPtr) {
		*refPtr = hardPTE & VMMACH_REFERENCED_BIT;
	    }
	    if (!*modPtr) {
		*modPtr = hardPTE & VMMACH_MODIFIED_BIT;
	    }
	}
    }

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
ENTRY void
VmMach_ClearRefBit(virtAddrPtr, virtFrameNum)
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtFrameNum;
{
    register	VmMach_SegData	*machPtr;
    int				pmegNum;
    Address			virtAddr;
    int				i;
    VmMachPTE			pte;
    Vm_TracePTEChange		pteChange;

    MASTER_LOCK(vmMachMutexPtr);

    refModMap[virtFrameNum] &= ~VMMACH_REFERENCED_BIT;
    machPtr = virtAddrPtr->segPtr->machPtr;
    pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
	    virtAddrPtr));
    if (pmegNum != VMMACH_INV_PMEG) {
	virtAddr = ((virtAddrPtr->page << VMMACH_PAGE_SHIFT) & 
			VMMACH_PAGE_MASK) + vmMachPTESegAddr;
	for (i = 0; 
	     i < VMMACH_CLUSTER_SIZE;
	     i++, virtAddr += VMMACH_PAGE_SIZE_INT) {
	    pte = VmMachReadPTE(pmegNum, virtAddr);
	    if (vm_Tracing) {
		pteChange.changeType = VM_TRACE_CLEAR_REF_BIT;
		pteChange.segNum = virtAddrPtr->segPtr->segNum;
		pteChange.pageNum = virtAddrPtr->page;
		pteChange.softPTE = FALSE;
		pteChange.beforePTE = pte;
		pteChange.afterPTE = pte & ~VMMACH_REFERENCED_BIT;
		VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC,
				 sizeof(Vm_TracePTEChange), 
				 (Address)&pteChange, TRUE);
	    }
	    pte &= ~VMMACH_REFERENCED_BIT;
	    VmMachWritePTE(pmegNum, virtAddr, pte);
	}
    }

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
    register	VmMach_SegData	*machPtr;
    int				pmegNum;
    Address			virtAddr;
    int				i;
    Vm_PTE			pte;
    Vm_TracePTEChange		pteChange;

    MASTER_LOCK(vmMachMutexPtr);

    refModMap[virtFrameNum] &= ~VMMACH_MODIFIED_BIT;
    machPtr = virtAddrPtr->segPtr->machPtr;
    pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
	    virtAddrPtr));
    if (pmegNum != VMMACH_INV_PMEG) {
	virtAddr = ((virtAddrPtr->page << VMMACH_PAGE_SHIFT) & 
			VMMACH_PAGE_MASK) + vmMachPTESegAddr;
	for (i = 0; 
	     i < VMMACH_CLUSTER_SIZE; 
	     i++, virtAddr += VMMACH_PAGE_SIZE_INT) {
	    pte = VmMachReadPTE(pmegNum, virtAddr);
	    if (vm_Tracing) {
		pteChange.changeType = VM_TRACE_CLEAR_MOD_BIT;
		pteChange.segNum = virtAddrPtr->segPtr->segNum;
		pteChange.pageNum = virtAddrPtr->page;
		pteChange.softPTE = FALSE;
		pteChange.beforePTE = pte;
		pteChange.afterPTE = pte & ~VMMACH_MODIFIED_BIT;
		VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC,
				sizeof(Vm_TracePTEChange), 
				(Address)&pteChange, TRUE);
	    }
	    pte &= ~VMMACH_MODIFIED_BIT;
	    VmMachWritePTE(pmegNum, virtAddr, pte);
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
 *      this routine is called that the user context register contains the
 *	context in which the page will be validated.
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
    register  Vm_Segment	*segPtr;
    register  VMMACH_SEG_NUM	*segTablePtr;
    register  PMEG		*pmegPtr;
    register  int		hardSeg;
    register  VmMachPTE		hardPTE;
    register  VmMachPTE		tHardPTE;
    Vm_PTE    *ptePtr;
    Address	addr;
    Boolean	reLoadPMEG;  	/* TRUE if we had to reload this PMEG. */
    int		i;

    MASTER_LOCK(vmMachMutexPtr);

    segPtr = virtAddrPtr->segPtr;
    addr = (Address) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
#ifdef sun4
	if (!VMMACH_ADDR_CHECK(addr)) {
	panic("VmMach_PageValidate: virt addr 0x%x falls into illegal range!\n",
		addr);
    }
#endif sun4

    /*
     * Find out the hardware segment that has to be mapped.
     * If this is a mapping segment, this gives us the seg num
     * for the segment in the other process and the segPtr is the mapSegPtr
     * which is set to the segPtr of the other process.
     */

    hardSeg = PageToOffSeg(virtAddrPtr->page, virtAddrPtr);
    segTablePtr = (VMMACH_SEG_NUM *) GetHardSegPtr(segPtr->machPtr, hardSeg);

    reLoadPMEG = FALSE;
    if (*segTablePtr == VMMACH_INV_PMEG) {
	int flags;
	/*
	 * If there is not already a pmeg for this hardware segment, then get
	 * one and initialize it.  If this is for the kernel then make
	 * sure that the pmeg cannot be taken away from the kernel.
	 * We make an exception for PMEGs allocated only for the block cache.
	 * If we fault on kernel pmeg we reload all the mappings for the
	 * pmeg because we can't tolerate "quick" faults in some places in the
	 * kernel.
	 */
	flags = 0;
	if (segPtr == vm_SysSegPtr) {
	   if (IN_FILE_CACHE_SEG(addr) && vmMachCanStealFileCachePmegs) {
	      /*
	       * In block cache virtual addresses.
	       */
	      reLoadPMEG = TRUE;
	   } else {
	      /*
	       * Normal kernel PMEGs must still be wired.
	       */
	      flags = PMEG_DONT_ALLOC;
	   }
	}
        *segTablePtr = PMEGGet(segPtr, hardSeg, flags);
	pmegPtr = &pmegArray[*segTablePtr];
    } else {
	pmegPtr = &pmegArray[*segTablePtr];
	if (pmegPtr->pageCount == 0) {
	    /*
	     * We are using a PMEG that had a pagecount of 0.  In this case
	     * it was put onto the end of the free pmeg list in anticipation
	     * of someone stealing this empty pmeg.  Now we have to move
	     * it off of the free list.
	     */
#ifndef CLEAN
	    VmCheckListIntegrity((List_Links *)pmegPtr);
#endif
	    if (pmegPtr->flags & PMEG_DONT_ALLOC) {
		List_Remove((List_Links *)pmegPtr);
	    } else {
		List_Move((List_Links *)pmegPtr, LIST_ATREAR(pmegInuseList));
#ifndef CLEAN
		VmCheckListIntegrity((List_Links *)pmegPtr);
#endif
	    }
	}
    }
    hardPTE = VMMACH_RESIDENT_BIT | VirtToPhysPage(Vm_GetPageFrame(pte));
#ifdef sun4
    if (addr < (Address) VMMACH_DEV_START_ADDR) {
	    hardPTE &= ~VMMACH_DONT_CACHE_BIT;
    } else {
	hardPTE |= VMMACH_DONT_CACHE_BIT;
    }
#endif /* sun4 */
    if (segPtr == vm_SysSegPtr) {
	int	oldContext;
	/*
	 * Have to propagate the PMEG to all contexts.
	 */
	oldContext = VmMachGetContextReg();
	for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	    VmMachSetContextReg(i);
	    VmMachSetSegMap(addr, (int)*segTablePtr);
	}
	VmMachSetContextReg(oldContext);
	hardPTE |= VMMACH_KRW_PROT;
    } else {
	Proc_ControlBlock	*procPtr;
	VmProcLink		*procLinkPtr;
	VmMach_Context  	*contextPtr;

	procPtr = Proc_GetCurrentProc();
	if (virtAddrPtr->flags & USING_MAPPED_SEG) {
	    addr = (Address) (VMMACH_MAP_SEG_ADDR + 
				((unsigned int)addr & (VMMACH_SEG_SIZE - 1)));
	    /* PUT IT IN SOFTWARE OF MAP AREA FOR PROCESS */
	    procPtr->vmPtr->machPtr->contextPtr->map[MAP_SEG_NUM] =
		    *segTablePtr;
	} else{
	    /* update it for regular seg num */
	    procPtr->vmPtr->machPtr->contextPtr->map[hardSeg] = *segTablePtr;
	}
	VmMachSetSegMap(addr, (int)*segTablePtr);

        if (segPtr != (Vm_Segment *) NIL) {
#ifndef CLEAN
	    VmCheckListIntegrity((List_Links *)segPtr->procList);
#endif
            LIST_FORALL(segPtr->procList, (List_Links *)procLinkPtr) {
		if (procLinkPtr->procPtr->vmPtr != (Vm_ProcInfo *) NIL &&
			procLinkPtr->procPtr->vmPtr->machPtr !=
			(VmMach_ProcData *) NIL &&
			(contextPtr =
			procLinkPtr->procPtr->vmPtr->machPtr->contextPtr) !=
			(VmMach_Context *) NIL) {
		    contextPtr->map[hardSeg] = *segTablePtr;
		}
	    }
	}

	if ((pte & (VM_COW_BIT | VM_READ_ONLY_PROT)) ||
		(virtAddrPtr->flags & VM_READONLY_SEG)) {
	    hardPTE |= VMMACH_UR_PROT;
	} else {
	    hardPTE |= VMMACH_URW_PROT;
	}
    }
    tHardPTE = VmMachGetPageMap(addr);
    if (tHardPTE & VMMACH_RESIDENT_BIT) {
	hardPTE |= tHardPTE & (VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT);
	for (i = 1; i < VMMACH_CLUSTER_SIZE; i++ ) {
	    hardPTE |= VmMachGetPageMap(addr + i * VMMACH_PAGE_SIZE_INT) & 
			    (VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT);
	}
    } else {
	pmegArray[*segTablePtr].pageCount++;
    }
    if (vm_Tracing) {
	Vm_TracePTEChange	pteChange;

	pteChange.changeType = VM_TRACE_VALIDATE_PAGE;
	pteChange.segNum = segPtr->segNum;
	pteChange.pageNum = virtAddrPtr->page;
	pteChange.softPTE = FALSE;
	if (tHardPTE & VMMACH_RESIDENT_BIT) {
	    pteChange.beforePTE = tHardPTE;
	} else {
	    pteChange.beforePTE = 0;
	}
	pteChange.afterPTE = hardPTE;
	VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC,
			sizeof(Vm_TracePTEChange), 
			(Address)&pteChange, TRUE);
    }

    /* Flush something? */
    SET_ALL_PAGE_MAP(addr, hardPTE);
    if (reLoadPMEG) {
	/*
	 * Reload all pte's for this pmeg.
	 */
	unsigned int a = (hardSeg << VMMACH_SEG_SHIFT);
	int	pageCount = 0;
	for (i = 0; i < VMMACH_NUM_PAGES_PER_SEG_INT; i++ ) { 
	    ptePtr = VmGetPTEPtr(vm_SysSegPtr, (a >> VMMACH_PAGE_SHIFT));
	    if ((*ptePtr & VM_PHYS_RES_BIT)) {
		hardPTE = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | 
				VirtToPhysPage(Vm_GetPageFrame(*ptePtr));
		SET_ALL_PAGE_MAP(a, hardPTE);
		pageCount++;
	     }
	     a += VMMACH_PAGE_SIZE_INT;
	}
	pmegPtr->pageCount = pageCount;
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
 *      The page table and hardware segment tables associated with the segment
 *      are modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */
static void
PageInvalidate(virtAddrPtr, virtPage, segDeletion) 
    register	Vm_VirtAddr	*virtAddrPtr;
    unsigned 	int		virtPage;
    Boolean			segDeletion;
{
    register VmMach_SegData	*machPtr;
    register PMEG		*pmegPtr;
    VmMachPTE			hardPTE;
    int				pmegNum;
    Address			addr;
    int				i;
    Vm_TracePTEChange		pteChange;
    Address			testVirtAddr;
    VmProcLink      		*flushProcLinkPtr;
    Vm_Segment      		*flushSegPtr;
    Vm_ProcInfo     		*flushVmPtr;
    VmMach_ProcData 		*flushMachPtr;
    VmMach_Context  		*flushContextPtr;
    unsigned int    		flushContext;
    unsigned int    		oldContext;

    refModMap[virtPage] = 0;
    if (segDeletion) {
	return;
    }
    machPtr = virtAddrPtr->segPtr->machPtr;
    pmegNum = *GetHardSegPtr(machPtr, PageToOffSeg(virtAddrPtr->page,
	    virtAddrPtr));
    if (pmegNum == VMMACH_INV_PMEG) {
	return;
    }
    testVirtAddr = (Address) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
    addr = ((virtAddrPtr->page << VMMACH_PAGE_SHIFT) &
				VMMACH_PAGE_MASK) + vmMachPTESegAddr;
#ifdef sun4
    if (!VMMACH_ADDR_CHECK(addr)) {
	panic("PageInvalidate: virt addr 0x%x falls into illegal range!\n",
		addr);
    }
#endif sun4
    hardPTE = VmMachReadPTE(pmegNum, addr);
    if (vm_Tracing) {
	pteChange.changeType = VM_TRACE_INVALIDATE_PAGE;
	pteChange.segNum = virtAddrPtr->segPtr->segNum;
	pteChange.pageNum = virtAddrPtr->page;
	pteChange.softPTE = FALSE;
	pteChange.beforePTE = hardPTE;
	pteChange.afterPTE = 0;
	VmStoreTraceRec(VM_TRACE_PTE_CHANGE_REC, sizeof(Vm_TracePTEChange),
			(Address)&pteChange, TRUE);
    }
    /*
     * Invalidate the page table entry.  There's no need to flush the page if
     * the invalidation is due to segment deletion, since the whole segment
     * will already have been flushed.  Flush the page in the context in which
     * it was validated.
     */
    if (!segDeletion) {
	int	flushedP = FALSE;

        flushSegPtr = virtAddrPtr->segPtr;
        if (flushSegPtr != (Vm_Segment *) NIL) {
#ifndef CLEAN
	    VmCheckListIntegrity((List_Links *)flushSegPtr->procList);
#endif
            LIST_FORALL(flushSegPtr->procList, (List_Links *)flushProcLinkPtr) {
                flushVmPtr = flushProcLinkPtr->procPtr->vmPtr;
                if (flushVmPtr != (Vm_ProcInfo *) NIL) {
                    flushMachPtr = flushVmPtr->machPtr;
                    if (flushMachPtr != (VmMach_ProcData *) NIL) {
                        flushContextPtr = flushMachPtr->contextPtr;
                        if (flushContextPtr != (VmMach_Context *) NIL) {
                            flushContext = flushContextPtr->context;
                            /* save old context */
                            oldContext = VmMachGetContextReg();
                            /* move to page's context */
                            VmMachSetContextReg((int)flushContext);
                            /* flush page in its context */
                            VmMachFlushPage(testVirtAddr);
                            /* back to old context */
                            VmMachSetContextReg((int)oldContext);
			    flushedP = TRUE;
                        }
                    }
                }
            }
        }
	if (!flushedP) {
	    VmMachFlushPage(testVirtAddr);
	}
    }
    for (i = 0; i < VMMACH_CLUSTER_SIZE; i++, addr += VMMACH_PAGE_SIZE_INT) {
	VmMachWritePTE(pmegNum, addr, (VmMachPTE)0);
    }
    pmegPtr = &pmegArray[pmegNum];
    if (hardPTE & VMMACH_RESIDENT_BIT) {
	pmegPtr->pageCount--;
	if (pmegPtr->pageCount == 0) {
	    /*
	     * When the pageCount goes to zero, the pmeg is put onto the end
	     * of the free list so that it can get freed if someone else
	     * needs a pmeg.  It isn't freed here because there is a fair
	     * amount of overhead when freeing a pmeg so its best to keep
	     * it around in case it is needed again.
	     */
	    if (pmegPtr->flags & PMEG_DONT_ALLOC) {
		List_Insert((List_Links *)pmegPtr, 
			    LIST_ATREAR(pmegFreeList));
	    } else {
		List_Move((List_Links *)pmegPtr, 
			  LIST_ATREAR(pmegFreeList));
	    }
#ifndef CLEAN
	    VmCheckListIntegrity((List_Links *)pmegPtr);
#endif
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
    int				*intPtr;
    int				dummy;
    register VmMach_SegData	*machPtr;
    register int		firstSeg;
    register int		lastSeg;

    machPtr = virtAddrPtr->segPtr->machPtr;

    firstSeg = PageToOffSeg(virtAddrPtr->page, virtAddrPtr);
    lastSeg = PageToOffSeg(lastPage, virtAddrPtr);
    /*
     * Lock down the PMEG behind the first segment.
     */
    intPtr = (int *) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
    while (!PMEGLock(machPtr, firstSeg)) {
	/*
	 * Touch the page to bring it into memory.  We know that we can
	 * safely touch it because we wouldn't have been called if these
	 * weren't good addresses.
	 */
	dummy = *intPtr;
    }
    /*
     * Lock down the rest of the segments.
     */
    for (firstSeg++; firstSeg <= lastSeg; firstSeg++) {
	intPtr = (int *)(firstSeg << VMMACH_SEG_SHIFT);
	while (!PMEGLock(machPtr, firstSeg)) {
	    dummy = *intPtr;
	}
    }
#ifdef lint
    dummy = dummy;
#endif
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
ENTRY void
VmMach_UnpinUserPages(virtAddrPtr, lastPage)
    Vm_VirtAddr	*virtAddrPtr;
    int		lastPage;
{
    register int	firstSeg;
    register int	lastSeg;
    int			pmegNum;
    register VmMach_SegData	*machPtr;

    MASTER_LOCK(vmMachMutexPtr);

    machPtr = virtAddrPtr->segPtr->machPtr;
    firstSeg = PageToOffSeg(virtAddrPtr->page, virtAddrPtr);
    lastSeg = PageToOffSeg(lastPage, virtAddrPtr);
    for (; firstSeg <= lastSeg; firstSeg++) {
	pmegNum = *GetHardSegPtr(machPtr, firstSeg);
	if (pmegNum == VMMACH_INV_PMEG) {
	    MASTER_UNLOCK(vmMachMutexPtr);
	    panic("Pinned PMEG invalid???\n");
	    return;
	}
	pmegArray[pmegNum].lockCount--;
    }

    MASTER_UNLOCK(vmMachMutexPtr);
}


/*
 ----------------------------------------------------------------------
 *
 * VmMach_MapInDevice --
 *
 *	Map a device at some physical address into kernel virtual address.
 *	This is for use by the controller initialization routines.
 *	This routine looks for a free page in the special range of
 *	kernel virtual that is reserved for this kind of thing and
 *	sets up the page table so that it references the device.
 *
 * Results:
 *	The kernel virtual address needed to reference the device is returned.
 *
 * Side effects:
 *	The hardware page table is modified.  This may steal another
 *	page from kernel virtual space, unless a page can be cleverly re-used.
 *
 *----------------------------------------------------------------------
 */
Address
VmMach_MapInDevice(devPhysAddr, type)
    Address	devPhysAddr;	/* Physical address of the device to map in */
    int		type;		/* Value for the page table entry type field.
				 * This depends on the address space that
				 * the devices live in, ie. VME D16 or D32 */
{
    Address 		virtAddr;
    Address		freeVirtAddr = (Address)0;
    Address		freePMEGAddr = (Address)0;
    int			page;
    int			pageFrame;
    VmMachPTE		pte;

    /*
     * Get the page frame for the physical device so we can
     * compare it against existing pte's.
     */
    pageFrame = (unsigned)devPhysAddr >> VMMACH_PAGE_SHIFT_INT;

    /*
     * Spin through the segments and their pages looking for a free
     * page or a virtual page that is already mapped to the physical page.
     */
    for (virtAddr = (Address)VMMACH_DEV_START_ADDR;
         virtAddr < (Address)VMMACH_DEV_END_ADDR; ) {
	if (VmMachGetSegMap(virtAddr) == VMMACH_INV_PMEG) {
	    /* 
	     * If we can't find any free mappings we can use this PMEG.
	     */
	    if (freePMEGAddr == 0) {
		freePMEGAddr = virtAddr;
	    }
	    virtAddr += VMMACH_SEG_SIZE;
	    continue;
	}
	/*
	 * Careful, use the correct page size when incrementing virtAddr.
	 * Use the real hardware size (ignore software klustering) because
	 * we are at a low level munging page table entries ourselves here.
	 */
	for (page = 0;
	     page < VMMACH_NUM_PAGES_PER_SEG_INT;
	     page++, virtAddr += VMMACH_PAGE_SIZE_INT) {
	    pte = VmMachGetPageMap(virtAddr);
	    if (!(pte & VMMACH_RESIDENT_BIT)) {
	        if (freeVirtAddr == 0) {
		    /*
		     * Note the unused page in this special area of the
		     * kernel virtual address space.
		     */
		    freeVirtAddr = virtAddr;
		}
	    } else if ((pte & VMMACH_PAGE_FRAME_FIELD) == pageFrame &&
		       VmMachGetPageType(pte) == type) {
		/*
		 * A page is already mapped for this physical address.
		 */
		return(virtAddr + ((int)devPhysAddr & VMMACH_OFFSET_MASK));
	    }
	}
    }

    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | pageFrame;
#if defined(sun3) || defined(sun4)	/* Not just for porting purposes */
    pte |= VMMACH_DONT_CACHE_BIT;
#endif
    VmMachSetPageType(pte, type);
    if (freeVirtAddr != 0) {
	VmMachSetPageMap(freeVirtAddr, pte);
	/*
	 * Return the kernel virtual address used to access it.
	 */
	return(freeVirtAddr + ((int)devPhysAddr & VMMACH_OFFSET_MASK));
    } else if (freePMEGAddr != 0) {
	int oldContext;
	int pmeg;
	int i;

	/*
	 * Map in a new PMEG so we can use it for mapping.
	 */
	pmeg = PMEGGet(vm_SysSegPtr, 
		       (int) ((unsigned)freePMEGAddr >> VMMACH_SEG_SHIFT),
		       PMEG_DONT_ALLOC);
	oldContext = VmMachGetContextReg();
	for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	    VmMachSetContextReg(i);
	    VmMachSetSegMap(freePMEGAddr, pmeg);
	}
	VmMachSetContextReg(oldContext);
	VmMachSetPageMap(freePMEGAddr, pte);
	return(freePMEGAddr + ((int)devPhysAddr & VMMACH_OFFSET_MASK));
    } else {
	return((Address)NIL);
    }
}

/*----------------------------------------------------------------------
 *
 * DevBufferInit --
 *
 *	Initialize a range of virtual memory to allocate from out of the
 *	device memory space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The buffer struct is initialized and the hardware page map is zeroed
 *	out in the range of addresses.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static void
DevBufferInit()
{
    Address		virtAddr;
    unsigned char	pmeg;
    int			oldContext;
    int			i;
    Address	baseAddr;
    Address	endAddr;

    /*
     * Round base up to next page boundary and end down to page boundary.
     */
    baseAddr = (Address)VMMACH_DMA_START_ADDR;
    endAddr = (Address)(VMMACH_DMA_START_ADDR + VMMACH_DMA_SIZE);

    /* 
     * Set up the hardware pages tables in the range of addresses given.
     */
    for (virtAddr = baseAddr; virtAddr < endAddr; ) {
	if (VmMachGetSegMap(virtAddr) != VMMACH_INV_PMEG) {
	    printf("DevBufferInit: DMA space already valid at 0x%x\n",
		   (unsigned int) virtAddr);
	}
	/* 
	 * Need to allocate a PMEG.
	 */
	pmeg = PMEGGet(vm_SysSegPtr, 
		       (int) ((unsigned)virtAddr >> VMMACH_SEG_SHIFT),
		       PMEG_DONT_ALLOC);
	oldContext = VmMachGetContextReg();
	for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	    VmMachSetContextReg(i);
	    VmMachSetSegMap(virtAddr, (int)pmeg);
	}
	VmMachSetContextReg(oldContext);
	virtAddr += VMMACH_SEG_SIZE;
    }
}


static	Boolean	dmaPageBitMap[VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT];

static Boolean dmaInitialized = FALSE;

/*
 ----------------------------------------------------------------------
 *
 * VmMach_DMAAlloc --
 *
 *	Allocate a set of virtual pages to a routine for mapping purposes.
 *	
 * Results:
 *	Pointer into kernel virtual address space of where to access the
 *	memory, or NIL if the request couldn't be satisfied.
 *
 * Side effects:
 *	The hardware page table is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY Address
VmMach_DMAAlloc(numBytes, srcAddr)
    int		numBytes;		/* Number of bytes to map in. */
    Address	srcAddr;	/* Kernel virtual address to start mapping in.*/
{
    Address	beginAddr;
    Address	endAddr;
    int		numPages;
    int		i, j;
    VmMachPTE	pte;
    Boolean	foundIt = FALSE;
    Address	newAddr;
 
    MASTER_LOCK(vmMachMutexPtr);
    if (!dmaInitialized) {
	/* Where to allocate the memory from. */
	dmaInitialized = TRUE;
	DevBufferInit();
    }

    /* calculate number of pages needed */
						/* beginning of first page */
    beginAddr = (Address) (((unsigned int)(srcAddr)) & ~VMMACH_OFFSET_MASK_INT);
						/* beginning of last page */
    endAddr = (Address) ((((unsigned int) srcAddr) + numBytes - 1) &
	    ~VMMACH_OFFSET_MASK_INT);
    numPages = (((unsigned int) endAddr) >> VMMACH_PAGE_SHIFT_INT) -
	    (((unsigned int) beginAddr) >> VMMACH_PAGE_SHIFT_INT) + 1;

    /* see if request can be satisfied */
    for (i = 0; i < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT); i++) {
	if (dmaPageBitMap[i] == 1) {
	    continue;
	}
	/*
	 * Must be aligned in the cache to avoid write-backs of stale data
	 * from other references to stuff on this page.
	 */
	newAddr = (Address)(VMMACH_DMA_START_ADDR + (i * VMMACH_PAGE_SIZE_INT));
	if (((unsigned int) newAddr & (VMMACH_CACHE_SIZE - 1)) !=
		((unsigned int) beginAddr & (VMMACH_CACHE_SIZE - 1))) {
	    continue;
	}
	for (j = 1; j < numPages &&
		((i + j) < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT)); j++) {
	    if (dmaPageBitMap[i + j] == 1) {
		break;
	    }
	}
	if (j == numPages &&
		((i + j) < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT))) {
	    foundIt = TRUE;
	    break;
	}
    }
    if (!foundIt) {
	MASTER_UNLOCK(vmMachMutexPtr);
	panic(
	    "VmMach_DMAAlloc: unable to satisfy request for %d bytes at 0x%x\n",
		numBytes, srcAddr);
#ifdef NOTDEF
	return (Address) NIL;
#endif NOTDEF
    }
    for (j = 0; j < numPages; j++) {
	dmaPageBitMap[i + j] = 1;	/* allocate page */
	pte = VmMachGetPageMap(srcAddr);
	pte |= VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT;
	SET_ALL_PAGE_MAP(((i + j) * VMMACH_PAGE_SIZE_INT) +
		VMMACH_DMA_START_ADDR, pte);
	srcAddr += VMMACH_PAGE_SIZE;
    }
    beginAddr = (Address) (VMMACH_DMA_START_ADDR + (i * VMMACH_PAGE_SIZE_INT) +
	    (((unsigned int) srcAddr) & VMMACH_OFFSET_MASK));

    MASTER_UNLOCK(vmMachMutexPtr);
    return beginAddr;
}

/*
 ----------------------------------------------------------------------
 *
 * VmMach_DMAAllocContiguous --
 *
 *	WARNING:  this routine doesn't work yet!!
 *	Allocate a set of virtual pages to a routine for mapping purposes.
 *	
 * Results:
 *	Pointer into kernel virtual address space of where to access the
 *	memory, or NIL if the request couldn't be satisfied.
 *
 * Side effects:
 *	The hardware page table is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmMach_DMAAllocContiguous(inScatGathPtr, scatGathLength, outScatGathPtr)
    register Net_ScatterGather	*inScatGathPtr;
    register int		scatGathLength;
    register Net_ScatterGather	*outScatGathPtr;
{
    Address	beginAddr;
    Address	endAddr;
    int		numPages;
    int		i, j;
    VmMachPTE	pte;
    Boolean	foundIt = FALSE;
    int		virtPage;
    static initialized = FALSE;
    Net_ScatterGather		*inPtr;
    Net_ScatterGather		*outPtr;
    int				pageOffset;
    Address			srcAddr;
    Address			newAddr;

    if (!dmaInitialized) {
	dmaInitialized = TRUE;
	DevBufferInit();
    }
    /* calculate number of pages needed */
    inPtr = inScatGathPtr;
    outPtr = outScatGathPtr;
    numPages = 0;
    for (i = 0; i < scatGathLength; i++) {
	if (inPtr->length > 0) {
	    /* beginning of first page */
	    beginAddr = (Address) (((unsigned int)(inPtr->bufAddr)) & 
		    ~VMMACH_OFFSET_MASK_INT);
	    /* beginning of last page */
	    endAddr = (Address) ((((unsigned int) inPtr->bufAddr) + 
		inPtr->length - 1) & ~VMMACH_OFFSET_MASK_INT);
	    /* 
	     * Temporarily store the number of pages in the out scatter/gather
	     * array.
	     */
	    outPtr->length =
		    (((unsigned int) endAddr) >> VMMACH_PAGE_SHIFT_INT) -
		    (((unsigned int) beginAddr) >> VMMACH_PAGE_SHIFT_INT) + 1;
	} else {
	    outPtr->length = 0;
	}
	if ((i == 0) && (outPtr->length != 1)) {
	    panic("Help! Help! I'm being repressed!\n");
	}
	numPages += outPtr->length;
	inPtr++;
	outPtr++;
    }

    /* see if request can be satisfied */
    for (i = 0; i < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT); i++) {
	if (dmaPageBitMap[i] == 1) {
	    continue;
	}
	/*
	 * Must be aligned in the cache to avoid write-backs of stale data
	 * from other references to stuff on this page.
	 */
	newAddr = (Address)(VMMACH_DMA_START_ADDR + (i * VMMACH_PAGE_SIZE_INT));
	if (((unsigned int) newAddr & (VMMACH_CACHE_SIZE - 1)) !=
		((unsigned int) beginAddr & (VMMACH_CACHE_SIZE - 1))) {
	    continue;
	}
	for (j = 1; j < numPages &&
		((i + j) < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT)); j++) {
	    if (dmaPageBitMap[i + j] == 1) {
		break;
	    }
	}
	if (j == numPages &&
		((i + j) < (VMMACH_DMA_SIZE / VMMACH_PAGE_SIZE_INT))) {
	    foundIt = TRUE;
	    break;
	}
    }
    if (!foundIt) {
	return FAILURE;
    }
    pageOffset = i;
    inPtr = inScatGathPtr;
    outPtr = outScatGathPtr;
    for (i = 0; i < scatGathLength; i++) {
	srcAddr = inPtr->bufAddr;
	numPages = outPtr->length;
	for (j = 0; j < numPages; j++) {
	    dmaPageBitMap[pageOffset + j] = 1;	/* allocate page */
	    virtPage = ((unsigned int) srcAddr) >> VMMACH_PAGE_SHIFT;
	    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT |
		  VirtToPhysPage(Vm_GetKernPageFrame(virtPage));
	    SET_ALL_PAGE_MAP(((pageOffset + j) * VMMACH_PAGE_SIZE_INT) +
		    VMMACH_DMA_START_ADDR, pte);
	    srcAddr += VMMACH_PAGE_SIZE;
	}
	outPtr->bufAddr = (Address) (VMMACH_DMA_START_ADDR + 
		(pageOffset * VMMACH_PAGE_SIZE_INT) + 
		(((unsigned int) srcAddr) & VMMACH_OFFSET_MASK));
	pageOffset += numPages;
	outPtr->length = inPtr->length;
	inPtr++;
	outPtr++;
    }
    return SUCCESS;
}


/*
 ----------------------------------------------------------------------
 *
 * VmMach_DMAFree --
 *
 *	Free a previously allocated set of virtual pages for a routine that
 *	used them for mapping purposes.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	The hardware page table is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMach_DMAFree(numBytes, mapAddr)
    int		numBytes;		/* Number of bytes to map in. */
    Address	mapAddr;	/* Kernel virtual address to unmap.*/
{
    Address	beginAddr;
    Address	endAddr;
    int		numPages;
    int		i, j;
 
    MASTER_LOCK(vmMachMutexPtr);
    /* calculate number of pages to free */
						/* beginning of first page */
    beginAddr = (Address) (((unsigned int) mapAddr) & ~VMMACH_OFFSET_MASK_INT);
						/* beginning of last page */
    endAddr = (Address) ((((unsigned int) mapAddr) + numBytes - 1) &
	    ~VMMACH_OFFSET_MASK_INT);
    numPages = (((unsigned int) endAddr) >> VMMACH_PAGE_SHIFT_INT) -
	    (((unsigned int) beginAddr) >> VMMACH_PAGE_SHIFT_INT) + 1;

    i = (((unsigned int) mapAddr) >> VMMACH_PAGE_SHIFT_INT) -
	(((unsigned int) VMMACH_DMA_START_ADDR) >> VMMACH_PAGE_SHIFT_INT);
    for (j = 0; j < numPages; j++) {
	dmaPageBitMap[i + j] = 0;	/* free page */
	VmMachFlushPage(mapAddr);
	SET_ALL_PAGE_MAP(mapAddr, (VmMachPTE) 0);
	(unsigned int) mapAddr += VMMACH_PAGE_SIZE_INT;
    }
    MASTER_UNLOCK(vmMachMutexPtr);
    return;
}

#if 0 /* dead code shirriff 9/90 */

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_GetDevicePage --
 *
 *      Allocate and validate a page at the given virtual address.  It is
 *	assumed that this page does not fall into the range of virtual 
 *	addresses used to allocate kernel code and data and that there is
 *	already a PMEG allocate for it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The hardware segment table for the kernel is modified to validate the
 *	the page.
 *
 * ----------------------------------------------------------------------------
 */
void
VmMach_GetDevicePage(virtAddr) 
    Address	virtAddr; /* Virtual address where a page has to be 
			   * validated at. */
{
    VmMachPTE	pte;
    int		page;

    page = Vm_KernPageAllocate();
    if (page == -1) {
	panic("Vm_GetDevicePage: Couldn't get memory\n");
    }
    pte = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | VirtToPhysPage(page) |
	    VMMACH_DONT_CACHE_BIT;
    SET_ALL_PAGE_MAP(virtAddr, pte);
}
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_MapKernelIntoUser --
 *
 *      Map a portion of kernel memory into the user's heap segment.  
 *	It will only map objects on hardware segment boundaries.  This is 
 *	intended to be used to map devices such as video memory.
 *
 *	NOTE: It is assumed that the user process knows what the hell it is
 *	      doing.
 *
 * Results:
 *      Return the virtual address that it chose to map the memory at.
 *
 * Side effects:
 *      The hardware segment table for the user process's segment is modified
 *	to map in the addresses.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
VmMach_MapKernelIntoUser(kernelVirtAddr, numBytes, userVirtAddr,
			 realVirtAddrPtr) 
    unsigned int	kernelVirtAddr;		/* Kernel virtual address
					 	 * to map in. */
    int	numBytes;				/* Number of bytes to map. */
    unsigned int	userVirtAddr; 		/* User virtual address to
					 	 * attempt to start mapping
						 * in at. */
    unsigned int	*realVirtAddrPtr;	/* Where we were able to start
					 	 * mapping at. */
{
    Address             newUserVirtAddr;
    ReturnStatus        status;

    status = VmMach_IntMapKernelIntoUser(kernelVirtAddr, numBytes,
            userVirtAddr, &newUserVirtAddr);

    if (status != SUCCESS) {
        return status;
    }

    return Vm_CopyOut(4, (Address) &newUserVirtAddr, (Address) realVirtAddrPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_IntMapKernelIntoUser --
 *
 *      Map a portion of kernel memory into the user's heap segment.
 *      It will only map objects on hardware segment boundaries.  This is
 *      intended to be used to map devices such as video memory.
 *
 *      This routine can be called from within the kernel since it doesn't
 *      do a Vm_CopyOut of the new user virtual address.
 *
 *      NOTE: It is assumed that the user process knows what the hell it is
 *            doing.
 *
 * Results:
 *      SUCCESS or FAILURE status.
 *      Return the virtual address that it chose to map the memory at in
 *      an out parameter.
 *
 * Side effects:
 *      The hardware segment table for the user process's segment is modified
 *      to map in the addresses.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
VmMach_IntMapKernelIntoUser(kernelVirtAddr, numBytes, userVirtAddr, newAddrPtr)
    unsigned int        kernelVirtAddr;         /* Kernel virtual address
                                                 * to map in. */
    int numBytes;                               /* Number of bytes to map. */
    unsigned int        userVirtAddr;           /* User virtual address to
                                                 * attempt to start mapping
                                                 * in at. */
    Address             *newAddrPtr;            /* New user address. */
{
    int                         numSegs;
    int                         firstPage;
    int                         numPages;
    Proc_ControlBlock           *procPtr;
    register    Vm_Segment      *segPtr;
    int                         hardSegNum;
    int                         i;
    unsigned int                pte;

#ifdef NOTDEF
    /* for debugging */
    printf("KernelVA 0x%x, numBytes 0x%x, userVA 0x%x.\n",
            kernelVirtAddr, numBytes, userVirtAddr);
#endif /* NOTDEF */
    procPtr = Proc_GetCurrentProc();
    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];

    numSegs = numBytes >> VMMACH_SEG_SHIFT;
    numPages = numSegs * VMMACH_SEG_SIZE / VMMACH_PAGE_SIZE;

    /*
     * Make user virtual address hardware segment aligned (round up) and
     * make sure that there is enough space to map things.
     */
    hardSegNum =
            (unsigned int) (userVirtAddr + VMMACH_SEG_SIZE - 1) >> VMMACH_SEG_SHIFT;
    userVirtAddr = hardSegNum << VMMACH_SEG_SHIFT;
    if (hardSegNum + numSegs > VMMACH_NUM_SEGS_PER_CONTEXT) {
        return(SYS_INVALID_ARG);
    }

    /*
     * Make sure will fit into the kernel's VAS.  Assume that is hardware
     * segment aligned.
     */
    hardSegNum = (unsigned int) (kernelVirtAddr) >> VMMACH_SEG_SHIFT;
    if (hardSegNum + numSegs > VMMACH_NUM_SEGS_PER_CONTEXT) {
        return(SYS_INVALID_ARG);
    }

    /*
     * Invalidate all virtual memory for the heap segment of this process
     * in the given range of virtual addresses that we are to map.  This
     * assures us that there aren't any hardware pages allocated for this
     * segment in this range of addresses.
     */
    firstPage = (unsigned int) (userVirtAddr) >> VMMACH_PAGE_SHIFT;
    (void)Vm_DeleteFromSeg(segPtr, firstPage, firstPage + numPages - 1);

    /*
     * Now go into the kernel's hardware segment table and copy the
     * segment table entries into the heap segments hardware segment table.
     */

    bcopy((Address)GetHardSegPtr(vm_SysSegPtr->machPtr, hardSegNum),
        (Address)GetHardSegPtr(segPtr->machPtr,
                (unsigned int)userVirtAddr >> VMMACH_SEG_SHIFT),
                numSegs * sizeof (VMMACH_SEG_NUM));
    for (i = 0; i < numSegs * VMMACH_NUM_PAGES_PER_SEG_INT; i++) {
        pte = VmMachGetPageMap((Address)(kernelVirtAddr +
                (i * VMMACH_PAGE_SIZE_INT)));
        pte &= ~VMMACH_KR_PROT;
        pte |= VMMACH_URW_PROT;
        VmMachSetPageMap((Address)(kernelVirtAddr + (i*VMMACH_PAGE_SIZE_INT)),
                pte);
    }

    /*
     * Make sure this process never migrates.
     */
    Proc_NeverMigrate(procPtr);

    /*
     * Reinitialize this process's context using the new segment table.
     */
    VmMach_ReinitContext(procPtr);

    *newAddrPtr = (Address) userVirtAddr;

#ifdef NOTDEF
    /* for debugging */
    printf("From Map kernel into user: new user addr is 0x%x.\n",
            userVirtAddr);
#endif /* NOTDEF */
    return SUCCESS;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_Trace --
 *
 *      Scan through all of the PMEGs generating trace records for those pages
 *	that have been referenced or modified since the last time that we
 *	checked.
 *  
 * Results:
 *      None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmMach_Trace()
{
    register	PMEG			*pmegPtr;
    register	Vm_Segment		*segPtr;
    register	int			pmegNum;
    register	int			curTraceTime;

    MASTER_LOCK(vmMachMutexPtr);

    /*
     * Save the current trace time and then increment it to ensure that
     * any segments that get used while we are scanning memory won't get 
     * missed.
     */
    curTraceTime = vmTraceTime;
    vmTraceTime++;

    /*
     * Spin through all of the pmegs.
     */
    for (pmegNum = 0, pmegPtr = pmegArray;
	 pmegNum < VMMACH_NUM_PMEGS;
	 pmegPtr++, pmegNum++) {
	segPtr = pmegPtr->segPtr;
	if ((pmegPtr->flags & PMEG_NEVER_FREE) ||
	    segPtr == (Vm_Segment *)NIL ||
	    segPtr->traceTime < curTraceTime) {
	    continue;
	}
	vmTraceStats.machStats.pmegsChecked++;
	printedSegTrace = FALSE;
	tracePMEGPtr = pmegPtr;
	VmMachTracePMEG(pmegNum);
    }

    MASTER_UNLOCK(vmMachMutexPtr);

    VmCheckTraceOverflow();
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMachTracePage --
 *
 *      Generate a trace record for the given page.
 *  
 * Results:
 *      None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
VmMachTracePage(pte, pageNum)
    register	VmMachPTE	pte;	/* Page table entry to be traced. */
    unsigned	int		pageNum;/* Inverse of page within PMEG. */
{
    Vm_TraceSeg			segTrace;
    Vm_TracePage		pageTrace;
    register	PMEG		*pmegPtr;
    register	Vm_Segment	*segPtr;

    refModMap[PhysToVirtPage(pte & VMMACH_PAGE_FRAME_FIELD)] |=
			pte & (VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT);
    if (!printedSegTrace) {
	/*
	 * Trace of the segment.
	 */
	printedSegTrace = TRUE;
	pmegPtr = tracePMEGPtr;
	segPtr = pmegPtr->segPtr;
	segTrace.hardSegNum = pmegPtr->hardSegNum;
	segTrace.softSegNum = segPtr->segNum;
	segTrace.segType = segPtr->type;
	segTrace.refCount = segPtr->refCount;
	VmStoreTraceRec(VM_TRACE_SEG_REC, sizeof(Vm_TraceSeg), 
			(Address)&segTrace, FALSE);
    }

    /*
     * Trace the page.
     */
    pageTrace = VMMACH_NUM_PAGES_PER_SEG_INT - pageNum;
    if (pte & VMMACH_REFERENCED_BIT) {
	pageTrace |= VM_TRACE_REFERENCED;
    } 
    if (pte & VMMACH_MODIFIED_BIT) {
	pageTrace |= VM_TRACE_MODIFIED;
    }
    VmStoreTraceRec(0, sizeof(Vm_TracePage), (Address)&pageTrace, FALSE);
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
    Address	virtAddr;

    /* on sun4, ignore invalidate parameter? */
    virtAddr = (Address) (virtAddrPtr->page << VMMACH_PAGE_SHIFT);
    VmMachFlushPage(virtAddr);
    if (invalidate) {
	VmMachSetPageMap(virtAddr, (VmMachPTE)0);
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_SetProtForDbg --
 *
 *	Set the protection of the kernel pages for the debugger.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The protection is set for the given range of kernel addresses.
 *
 *----------------------------------------------------------------------
 */
void
VmMach_SetProtForDbg(readWrite, numBytes, addr)
    Boolean	readWrite;	/* TRUE if should make pages writable, FALSE
				 * if should make read-only. */
    int		numBytes;	/* Number of bytes to change protection for. */
    Address	addr;		/* Address to start changing protection at. */
{
    register	Address		virtAddr;
    register	VmMachPTE 	pte;
    register	int		firstPage;
    register	int		lastPage;
    int		oldContext;
    int		i;

    /*
     * This should only be called with kernel text pages so we modify the
     * PTE for the address in all the contexts. Note that we must flush
     * the page from the change before changing the protections to avoid
     * write back errors.
     */
    oldContext = VmMachGetContextReg();
    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	VmMachSetContextReg(i);
	firstPage = (unsigned)addr >> VMMACH_PAGE_SHIFT;
	lastPage = ((unsigned)addr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
	for (; firstPage <= lastPage; firstPage++) {
	    virtAddr = (Address) (firstPage << VMMACH_PAGE_SHIFT);
	    pte = VmMachGetPageMap(virtAddr);
	    pte &= ~VMMACH_PROTECTION_FIELD;
	    pte |= readWrite ? VMMACH_KRW_PROT : VMMACH_KR_PROT;
	    VmMachFlushPage(virtAddr);
	    SET_ALL_PAGE_MAP(virtAddr, pte);
	}
    }
    VmMachSetContextReg(oldContext);
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
 * VmMach_HandleSegMigration --
 *
 *	Do machine-dependent aspects of segment migration.  On the sun4's,
 *	this means flush the segment from the virtually addressed cache.
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
VmMach_HandleSegMigration(segPtr)
    Vm_Segment		*segPtr;	/* Pointer to segment to be migrated. */
{
    Address	virtAddr;

    virtAddr = (Address) (segPtr->offset << VMMACH_PAGE_SHIFT);
    VmMachFlushSegment(virtAddr);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * VmMach_FlushCode --
 *
 *      Does nothing on this machine.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMach_FlushCode(procPtr, virtAddrPtr, virtPage, numBytes)
    Proc_ControlBlock   *procPtr;
    Vm_VirtAddr         *virtAddrPtr;
    unsigned            virtPage;
    int                 numBytes;
{
}


/*
 * Dummy function which will turn out to be the function that the debugger
 * prints out on a backtrace after a trap.  The debugger gets confused
 * because trap stacks originate from assembly language stacks.  I decided
 * to make a dummy procedure because it was to confusing seeing the
 * previous procedure (VmMach_MapKernelIntoUser) on every backtrace.
 */
static void
VmMachTrap()
{
}

/*
 * This looks like dead code.
 */
#if 0
/*
 *----------------------------------------------------------------------
 *
 * ByteFill --
 *
 *	Fill numBytes at the given address.  This routine is optimized to do 
 *      4-byte fills.  However, if the address is odd then it is forced to
 *      do single byte fills.
 *
 * Results:
 *	numBytes bytes of the fill byte are placed at *destPtr at the 
 *	given address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ByteFill(fillByte, numBytes, destPtr)
    register unsigned char fillByte;	/* The byte to be filled in. */
    register int numBytes;	/* The number of bytes to be filled in. */
    Address destPtr;		/* Where to fill. */
{
    register unsigned int fillInt = 
	(fillByte) | (fillByte << 8) | (fillByte << 16) | (fillByte << 24);

    register int *dPtr = (int *) destPtr;
    
    /*
     * If the address is on an aligned boundary then fill in as much
     * as we can in big transfers (and also avoid loop overhead by
     * storing many fill ints per iteration).  Once we have less than
     * 4 bytes to fill then it must be done by byte copies.
     */
    /*
     * On the sun4, I should try double-words...
     */
#define WORDMASK	0x1

    if (((int) dPtr & WORDMASK) == 0) {
	while (numBytes >= 32) {
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    *dPtr++ = fillInt;
	    numBytes -= 32;
	}
	while (numBytes >= 4) {
	    *dPtr++ = fillInt;
	    numBytes -= 4;
	}
	destPtr = (char *) dPtr;
    }

    /*
     * Fill in the remaining bytes
     */

    while (numBytes > 0) {
	*destPtr++ = fillByte;
	numBytes--;
    }
}
#endif

#ifndef sun4c

/*----------------------------------------------------------------------
 *
 * Dev32BitBufferInit --
 *
 *	Initialize a range of virtual memory to allocate from out of the
 *	device memory space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The buffer struct is initialized and the hardware page map is zeroed
 *	out in the range of addresses.
 *
 *----------------------------------------------------------------------
 */
INTERNAL static void
Dev32BitDMABufferInit()
{
    Address		virtAddr;
    unsigned char	pmeg;
    int			oldContext;
    Address	baseAddr;
    Address	endAddr;

    if ((VMMACH_32BIT_DMA_SIZE & (VMMACH_CACHE_SIZE - 1)) != 0) {
	panic(
"Dev32BitDMABufferInit: 32-bit DMA area must be a multiple of cache size.\n");
    }

    VmMachSetup32BitDVMA(); 
    /*
     * Round base up to next page boundary and end down to page boundary.
     */
    baseAddr = (Address)VMMACH_32BIT_DMA_START_ADDR;
    endAddr = (Address)(VMMACH_32BIT_DMA_START_ADDR + VMMACH_32BIT_DMA_SIZE);

    /* 
     * Set up the hardware pages tables in the range of addresses given.
     */
    oldContext = VmMachGetContextReg();
    VmMachSetContextReg(0);
    for (virtAddr = baseAddr; virtAddr < endAddr; ) {
	if (VmMachGetSegMap(virtAddr) != VMMACH_INV_PMEG) {
	    printf("Dev32BitDMABufferInit: DMA space already valid at 0x%x\n",
		   (unsigned int) virtAddr);
	}
	/* 
	 * Need to allocate a PMEG.
	 */
	pmeg = PMEGGet(vm_SysSegPtr, 
		       (int) ((unsigned)virtAddr >> VMMACH_SEG_SHIFT),
		       PMEG_DONT_ALLOC);
	if (pmeg == VMMACH_INV_PMEG) {
	    panic("Dev32BitDMABufferInit: unable to get a pmeg.\n");
	}
        VmMachSetSegMap(virtAddr, (int)pmeg);
	virtAddr += VMMACH_SEG_SIZE;
    }
    VmMachSetContextReg(oldContext);
}


static	Boolean	userdmaPageBitMap[VMMACH_32BIT_DMA_SIZE / VMMACH_PAGE_SIZE_INT];


/*
 ----------------------------------------------------------------------
 *
 * VmMach_32BitDMAAlloc --
 *
 *	Allocate a set of virtual pages to a routine for mapping purposes.
 *	
 * Results:
 *	Pointer into kernel virtual address space of where to access the
 *	memory, or NIL if the request couldn't be satisfied.
 *
 * Side effects:
 *	The hardware page table is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY Address
VmMach_32BitDMAAlloc(numBytes, srcAddr)
    int		numBytes;		/* Number of bytes to map in. */
    Address	srcAddr;	/* Kernel virtual address to start mapping in.*/
{
    Address	beginAddr;
    Address	endAddr;
    int		numPages;
    int		i, j;
    VmMachPTE	pte;
    Boolean	foundIt = FALSE;
    static initialized = FALSE;
    unsigned	oldContext;
    int		align;
 
    MASTER_LOCK(vmMachMutexPtr);
    if (!initialized) {
	initialized = TRUE;
	Dev32BitDMABufferInit();
    }

    /* calculate number of pages needed */
						/* beginning of first page */
    beginAddr = (Address) (((unsigned int)(srcAddr)) & ~VMMACH_OFFSET_MASK_INT);
						/* beginning of last page */
    endAddr = (Address) ((((unsigned int) srcAddr) + numBytes - 1) &
	    ~VMMACH_OFFSET_MASK_INT);
    numPages = (((unsigned int) endAddr) >> VMMACH_PAGE_SHIFT_INT) -
	    (((unsigned int) beginAddr) >> VMMACH_PAGE_SHIFT_INT) + 1;

    /* set first addr to first entry that is also cache aligned */
    align = (unsigned int) beginAddr & (VMMACH_CACHE_SIZE - 1);
    align -= (unsigned int) VMMACH_32BIT_DMA_START_ADDR &
	    (VMMACH_CACHE_SIZE - 1);
    if ((int) align < 0) {
	align += VMMACH_CACHE_SIZE;
    }
    /* see if request can be satisfied, incrementing by cache size in loop */
    for (i = align / VMMACH_PAGE_SIZE_INT;
	    i < (VMMACH_32BIT_DMA_SIZE / VMMACH_PAGE_SIZE_INT);
	    i += (VMMACH_CACHE_SIZE / VMMACH_PAGE_SIZE_INT)) {
	if (userdmaPageBitMap[i] == 1) {
	    continue;
	}
	for (j = 1; (j < numPages) &&
		((i + j) < (VMMACH_32BIT_DMA_SIZE / VMMACH_PAGE_SIZE_INT));
		j++) {
	    if (userdmaPageBitMap[i + j] == 1) {
		break;
	    }
	}
	if ((j == numPages) &&
		((i + j) < (VMMACH_32BIT_DMA_SIZE / VMMACH_PAGE_SIZE_INT))) {
	    foundIt = TRUE;
	    break;
	}
    }

    if (!foundIt) {
	MASTER_UNLOCK(vmMachMutexPtr);
	panic(
    "VmMach_32BitDMAAlloc: unable to satisfy request for %d bytes at 0x%x\n",
		numBytes, srcAddr);
#ifdef NOTDEF
	return (Address) NIL;
#endif NOTDEF
    }
    oldContext = VmMachGetContextReg();
    VmMachSetContextReg(0);
    for (j = 0; j < numPages; j++) {
	userdmaPageBitMap[i + j] = 1;	/* allocate page */
	pte = VmMachGetPageMap(srcAddr);
	pte = (pte & ~VMMACH_PROTECTION_FIELD) | VMMACH_RESIDENT_BIT | 
		    VMMACH_URW_PROT;

	SET_ALL_PAGE_MAP(((i + j) * VMMACH_PAGE_SIZE_INT) +
		VMMACH_32BIT_DMA_START_ADDR, pte);
	srcAddr += VMMACH_PAGE_SIZE;
    }
    VmMachSetContextReg((int)oldContext);
    beginAddr = (Address) (VMMACH_32BIT_DMA_START_ADDR +
	    (i * VMMACH_PAGE_SIZE_INT) +
	    (((unsigned int) srcAddr) & VMMACH_OFFSET_MASK));

    /* set high VME addr bit */
    (unsigned) beginAddr |= VMMACH_VME_ADDR_BIT;
    MASTER_UNLOCK(vmMachMutexPtr);
    return (Address) beginAddr;
}


/*
 ----------------------------------------------------------------------
 *
 * VmMach_32BitDMAFree --
 *
 *	Free a previously allocated set of virtual pages for a routine that
 *	used them for mapping purposes.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	The hardware page table is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
VmMach_32BitDMAFree(numBytes, mapAddr)
    int		numBytes;		/* Number of bytes to map in. */
    Address	mapAddr;	/* Kernel virtual address to unmap.*/
{
    Address	beginAddr;
    Address	endAddr;
    int		numPages;
    int		i, j;
    int         oldContext;

    MASTER_LOCK(vmMachMutexPtr);
    /* calculate number of pages to free */
    /* Clear the VME high bit from the address */
    mapAddr = (Address) ((unsigned)mapAddr & ~VMMACH_VME_ADDR_BIT);
						/* beginning of first page */
    beginAddr = (Address) (((unsigned int) mapAddr) & ~VMMACH_OFFSET_MASK_INT);
						/* beginning of last page */
    endAddr = (Address) ((((unsigned int) mapAddr) + numBytes - 1) &
	    ~VMMACH_OFFSET_MASK_INT);
    numPages = (((unsigned int) endAddr) >> VMMACH_PAGE_SHIFT_INT) -
	    (((unsigned int) beginAddr) >> VMMACH_PAGE_SHIFT_INT) + 1;

    i = (((unsigned int) mapAddr) >> VMMACH_PAGE_SHIFT_INT) -
	(((unsigned int) VMMACH_32BIT_DMA_START_ADDR) >> VMMACH_PAGE_SHIFT_INT);
    oldContext = VmMachGetContextReg();
    VmMachSetContextReg(0);
    for (j = 0; j < numPages; j++) {
	userdmaPageBitMap[i + j] = 0;	/* free page */
	VmMachFlushPage(mapAddr);
	SET_ALL_PAGE_MAP(mapAddr, (VmMachPTE) 0);
	(unsigned int) mapAddr += VMMACH_PAGE_SIZE_INT;
    }
    VmMachSetContextReg(oldContext);
    MASTER_UNLOCK(vmMachMutexPtr);
    return;
}

#endif /* not sun4c */


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

    dprintf("VmMach_Unalloc: freeing %d blocks at %x\n",firstBlock,addr);
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
/*ARGSUSED*/
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
    procPtr->vmPtr->sharedEnd = (Address) VMMACH_SHARED_START_ADDR +
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
    procPtr->vmPtr->machPtr->sharedData.allocVector = (int *)NIL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_CopySharedMem --
 *
 *      Copies machine-dependent shared memory data structures to handle
 *      a fork.
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
    VmMach_SharedData   *childSharedData =
            &childProcPtr->vmPtr->machPtr->sharedData;
    VmMach_SharedData   *parentSharedData =
            &parentProcPtr->vmPtr->machPtr->sharedData;

    VmMach_SharedProcStart(childProcPtr);

    bcopy((Address)parentSharedData->allocVector,
	    (Address)childSharedData->allocVector,
            VMMACH_SHARED_NUM_BLOCKS*sizeof(int));
    childSharedData->allocFirstFree = parentSharedData->allocFirstFree;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmMach_CopySharedMem --
 *
 *      Copies machine-dependent shared memory data structures to handle
 *      a fork.
 *
 * Results:
 *      None. (This routine is stubbed out for the sun4.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
VmMachTracePMEG(pmeg)
int pmeg;
{
    panic("VmMachTracePMEG called.\n");
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
    register  Vm_VirtAddr	virtAddr;
    register  VMMACH_SEG_NUM	*segTablePtr, pmeg;
    register  int		hardSeg;
    Vm_PTE    *ptePtr;
    VmMachPTE		hardPTE;
    /*
     * Ignore pages not in cache pmeg range.
     */
    if (!IN_FILE_CACHE_SEG(kernelAddress)) {
	return;
    }

    MASTER_LOCK(vmMachMutexPtr);

    pmeg = VmMachGetSegMap(kernelAddress);
    if (pmeg == VMMACH_INV_PMEG) {
	int	oldContext, i;
	unsigned int a;
	/*
	 *  If not a valid PMEG install a new pmeg and load its mapping. 
	 */
	virtAddr.segPtr = vm_SysSegPtr;
	virtAddr.page = ((unsigned int) kernelAddress) >> VMMACH_PAGE_SHIFT;
	virtAddr.offset = 0;
	virtAddr.flags = 0;
	virtAddr.sharedPtr = (Vm_SegProcList *) NIL;
    
	hardSeg = PageToOffSeg(virtAddr.page, (&virtAddr));
	segTablePtr = (VMMACH_SEG_NUM *) 
			    GetHardSegPtr(vm_SysSegPtr->machPtr, hardSeg);
	if (*segTablePtr != VMMACH_INV_PMEG) {
	    panic("VmMach_LockCachePage: Bad segTable entry.\n");
	}
	*segTablePtr = pmeg = PMEGGet(vm_SysSegPtr, hardSeg, 0);
	/*
	 * Have to propagate the PMEG to all contexts.
	 */
	oldContext = VmMachGetContextReg();
	for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	    VmMachSetContextReg(i);
	    VmMachSetSegMap(kernelAddress, pmeg);
	}
	VmMachSetContextReg(oldContext);
	/*
	 * Reload the entire PMEG.
	 */
	a = (hardSeg << VMMACH_SEG_SHIFT);
	for (i = 0; i < VMMACH_NUM_PAGES_PER_SEG_INT; i++ ) { 
	    ptePtr = VmGetPTEPtr(vm_SysSegPtr, (a >> VMMACH_PAGE_SHIFT));
	    if ((*ptePtr & VM_PHYS_RES_BIT)) {
		hardPTE = VMMACH_RESIDENT_BIT | VMMACH_KRW_PROT | 
				VirtToPhysPage(Vm_GetPageFrame(*ptePtr));
		SET_ALL_PAGE_MAP(a, hardPTE);
		pmegArray[pmeg].pageCount++;
	     }
	     a += VMMACH_PAGE_SIZE_INT;
	}
    }
    pmegArray[pmeg].lockCount++;

    MASTER_UNLOCK(vmMachMutexPtr);
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
    register  VMMACH_SEG_NUM	pmeg;

    if (!IN_FILE_CACHE_SEG(kernelAddress)) {
	return;
    }

    MASTER_LOCK(vmMachMutexPtr);

    pmeg = VmMachGetSegMap(kernelAddress);

    pmegArray[pmeg].lockCount--;
    if (pmegArray[pmeg].lockCount < 0) {
	panic("VmMach_UnlockCachePage lockCount < 0\n");
    }

    MASTER_UNLOCK(vmMachMutexPtr);
    return;
}



