/* vmSysCall.c -
 *
 * 	This file contains routines that handle virtual memory system
 *	calls.
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
#include "vmMachInt.h"
#include "vmTrace.h"
#include "user/vm.h"
#include "sync.h"
#include "sys.h"
#include "stdlib.h"
#include "string.h"
#include "fs.h"
#include "sys/mman.h"

extern Vm_SharedSegTable sharedSegTable;

int vmShmDebug = 0;	/* Shared memory debugging flag. */


/*
 *----------------------------------------------------------------------
 *
 * Vm_PageSize --
 *
 *	Return the hardware page size.
 *
 * Results:
 *	Status from copying the page size out to user space.
 *
 * Side effects:
 *	Copy page size out to user address space.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_PageSize(pageSizePtr)
    int	*pageSizePtr;
{
    int			pageSize = vm_PageSize;

    return(Vm_CopyOut(4, (Address) &pageSize, (Address) pageSizePtr));
}


/***************************************************************************
 *
 *	The following two routines, Vm_CreateVA and Vm_DestroyVA, are used
 *	to allow users to add or delete ranges of valid virtual addresses
 *	from their virtual address space.  Since neither of these routiens
 *	are monitored (although they call monitored routines), there are 
 *	potential synchronization problems for users sharing memory who
 *	expand their heap segment.  The problems are caused if two or more
 *	users attempt to simultaneously change the size of the heap segment 
 *	to different sizes. The virtual memory system will not have any 
 *	problems handling the conflicting requests, however the actual range 
 *	of valid virtual addresses is unpredictable.  It is up to the user
 *	to synchronize when expanding the HEAP segment.
 */


/*
 *----------------------------------------------------------------------
 *
 * Vm_CreateVA --
 *
 *	Validate the given range of addresses.  If necessary the segment
 *	is expanded to make room.
 *
 * Results:
 *	VM_WRONG_SEG_TYPE if tried to validate addresses for a stack or
 *	code segment and VM_SEG_TOO_LARGE if the segment cannot be
 *	expanded to fill the size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_CreateVA(address, size)
    Address address;	/* Address to validate from. */
    int	    size;	/* Number of bytes to validate. */
{
    register Vm_Segment *segPtr;
    int			firstPage, lastPage;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    firstPage = (unsigned) (address) >> vmPageShift;
    lastPage = (unsigned) ((int) address + size - 1) >> vmPageShift;

    segPtr = (Vm_Segment *) procPtr->vmPtr->segPtrArray[VM_HEAP];

    /*
     * Make sure that the beginning address falls into the heap segment and
     * not the code segment.
     */
    if (firstPage < segPtr->offset) {
	return(VM_WRONG_SEG_TYPE);
    }

    /*
     * Make sure that the ending page is not greater than the highest
     * possible page.  Since there must be one stack page and one page
     * between the stack and the heap, the highest possible heap page is
     * mach_LastUserStackPage - 2 or segPtr->maxAddr, whichever is lower.
     */
    if (lastPage > mach_LastUserStackPage - 2 ||
        address + size - 1 > segPtr->maxAddr) {
	return(VM_SEG_TOO_LARGE);
    }

    /*
     * Make pages between firstPage and lastPage part of the heap segment's
     * virtual address space.
     */
    return(VmAddToSeg(segPtr, firstPage, lastPage));
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_DestroyVA --
 *
 *	Invalidate the given range of addresses.  If the starting address
 *	is too low then an error message is returned.
 *
 * Results:
 *	VM_WRONG_SEG_TYPE if tried to invalidate addresses for a code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_DestroyVA(address, size)
    Address address;	/* Address to validate from. */
    int	    size;	/* Number of bytes to validate. */
{
    register Vm_Segment *segPtr;
    int			firstPage, lastPage;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    firstPage = (unsigned) (address) >> vmPageShift;
    lastPage = (unsigned) ((int) address + size - 1) >> vmPageShift;

    segPtr = (Vm_Segment *) procPtr->vmPtr->segPtrArray[VM_HEAP];

    /*
     * Make sure that the beginning address falls into the 
     * heap segment.
     */
    if (firstPage < segPtr->offset) {
	return(VM_WRONG_SEG_TYPE);
    }

    /*
     * Make pages between firstPage and lastPage not members of the segment's
     * virtual address spac.e
     */
    return(Vm_DeleteFromSeg(segPtr, firstPage, lastPage));
}

static int	copySize = 4096;

#ifndef CLEAN
static char	buffer[8192];
#endif CLEAN

void	SetVal();

/*
 * The # construct to turn a variable into a string is not handled correctly
 * on the current sun4 compiler/preprocessor set up.  This will change when
 * it's handled correctly.
 */
#if !defined(sun4) && !defined(mips)
#define SETVAR(var, val) SetVal(#var, val, (int *)&(var))
#else
#define	SETVAR(var, val) SetVal("var val", val, (int *)&(var))
#endif /* sun4 */


/*
 *----------------------------------------------------------------------
 *
 * Vm_Cmd --
 *
 *      This routine allows a user level program to give commands to
 *      the virtual memory system.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Some parameter of the virtual memory system will be modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_Cmd(command, arg)
    int		command;
    int         arg;
{
    int			numBytes;
    ReturnStatus	status = SUCCESS;
    int			numModifiedPages;
 
    switch (command) {
	case VM_COUNT_DIRTY_PAGES:
	    numModifiedPages = VmCountDirtyPages();
	    if (Vm_CopyOut(sizeof(int), (Address) &numModifiedPages,
			   (Address) arg) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	    break;
	case VM_FLUSH_SEGMENT: {
	    extern int	vmNumSegments;
	    int		intArr[3];
	    Vm_Segment	*segPtr;
	    int		lowPage;
	    int		highPage;

	    status = Vm_CopyIn(3 * sizeof(int), (Address)arg, 
				(Address)intArr);
	    if (status != SUCCESS) {
		break;
	    }
	    if (intArr[0] <= 0 || intArr[0] >= vmNumSegments) {
		status = SYS_INVALID_ARG;
		break;
	    }
	    segPtr = VmGetSegPtr(intArr[0]);
	    if (segPtr->type == VM_STACK) {
		lowPage = mach_LastUserStackPage - segPtr->numPages + 1;
		highPage = mach_LastUserStackPage;
	    } else {
		lowPage = segPtr->offset;
		highPage = segPtr->offset + segPtr->numPages - 1;
	    }
	    if (intArr[1] >= lowPage && intArr[1] <= highPage) {
		lowPage = intArr[1];
	    }
	    if (intArr[2] >= lowPage && intArr[2] <= highPage) {
		highPage = intArr[2];
	    }
	    VmFlushSegment(segPtr, lowPage, highPage);
	    break;
	}
	case VM_SET_FREE_WHEN_CLEAN:
	    SETVAR(vmFreeWhenClean, arg);
	    break;
	case VM_SET_MAX_DIRTY_PAGES:
	    SETVAR(vmMaxDirtyPages, arg);
	    break;
	case VM_SET_PAGEOUT_PROCS:
	    SETVAR(vmMaxPageOutProcs, arg);
	    break;
        case VM_SET_CLOCK_PAGES:
            SETVAR(vmPagesToCheck, arg);
            break;
        case VM_SET_CLOCK_INTERVAL: {
	    SETVAR(vmClockSleep, (int) (arg * timer_IntOneSecond));
            break;
	}
	case VM_SET_COPY_SIZE:
	    SETVAR(copySize, arg);
	    break;
#ifndef CLEAN
	case VM_DO_COPY_IN:
	    (void)Vm_CopyIn(copySize, (Address) arg, buffer);
	    break;
	case VM_DO_COPY_OUT:
	    (void)Vm_CopyOut(copySize, buffer, (Address) arg);
	    break;
	case VM_DO_MAKE_ACCESS_IN:
	    Vm_MakeAccessible(0, copySize, (Address) arg, &numBytes,
			      (Address *) &arg);
	    bcopy((Address) arg, buffer, copySize);
	    Vm_MakeUnaccessible((Address) arg, numBytes);
	    break;
	case VM_DO_MAKE_ACCESS_OUT:
	    Vm_MakeAccessible(0, copySize, (Address) arg, &numBytes,
			      (Address *) &arg);
	    bcopy(buffer, (Address) arg, copySize);
	    Vm_MakeUnaccessible((Address) arg, numBytes);
	    break;
#endif CLEAN
	case VM_GET_STATS:
	    vmStat.kernMemPages = 
		    (unsigned int)(vmMemEnd - mach_KernStart) / vm_PageSize;
	    if (Vm_CopyOut(sizeof(Vm_Stat), (Address) &vmStat, 
			   (Address) arg) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	    break;
	case VM_SET_COW:
	    /*
	     * It's okay to turn on COW when it's off, but not the other
	     * way around.
	     */
	    if (arg || !vm_CanCOW) {
		SETVAR(vm_CanCOW, arg);
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    break;
	case VM_SET_FS_PENALTY:
	    if (arg <= 0) {
		/*
		 * Caller is setting an absolute penalty.
		 */
		SETVAR(vmCurPenalty, -arg);
	    } else {
		SETVAR(vmFSPenalty, arg);
		SETVAR(vmCurPenalty, (vmStat.fsMap - vmStat.fsUnmap) / 
					vmPagesPerGroup * vmFSPenalty);
	    }
	    break;
	case VM_SET_NUM_PAGE_GROUPS: {
	    int	numPages;
	    int curGroup;
	    numPages = vmPagesPerGroup * vmNumPageGroups;
	    SETVAR(vmNumPageGroups, arg);
	    SETVAR(vmPagesPerGroup, numPages / vmNumPageGroups);
	    curGroup = (vmStat.fsMap - vmStat.fsUnmap) / vmPagesPerGroup;
	    SETVAR(vmCurPenalty, curGroup * vmFSPenalty);
	    SETVAR(vmBoundary, (curGroup + 1) * vmPagesPerGroup);
	    break;
	}
	case VM_SET_ALWAYS_REFUSE:
	    SETVAR(vmAlwaysRefuse, arg);
	    break;
	case VM_SET_ALWAYS_SAY_YES:
	    SETVAR(vmAlwaysSayYes, arg);
	    break;
	case VM_RESET_FS_STATS:
	    vmStat.maxFSPages = vmStat.fsMap - vmStat.fsUnmap;
	    vmStat.minFSPages = vmStat.fsMap - vmStat.fsUnmap;
	    break;
	case VM_SET_COR_READ_ONLY:
	    SETVAR(vmCORReadOnly, arg);
	    break;
	case VM_SET_PREFETCH:
	    SETVAR(vmPrefetch, arg);
	    break;
	case VM_SET_USE_FS_READ_AHEAD:
	    SETVAR(vmUseFSReadAhead, arg);
	    break;
	case VM_START_TRACING: {
	    ReturnStatus	status;
	    void		VmTraceDaemon();
	    Vm_TraceStart	*traceStartPtr;
	    extern int		etext;
	    char		fileName[100];
	    char		hostNum[34];

	    if (vmTraceFilePtr != (Fs_Stream *)NIL) {
		printf("VmCmd: Tracing already running.\n");
		break;
	    }
	    vmTracesPerClock = arg;
	    printf("Vm_Cmd: Tracing started at %d times per second\n",
			arg);

	    if (vmTraceBuffer == (char *)NIL) {
		vmTraceBuffer = (char *)malloc(VM_TRACE_BUFFER_SIZE);
	    }
	    bzero((Address)&vmTraceStats, sizeof(vmTraceStats));

	    traceStartPtr = (Vm_TraceStart  *)vmTraceBuffer;
	    traceStartPtr->recType = VM_TRACE_START_REC;
	    traceStartPtr->hostID = Sys_GetHostId();
	    traceStartPtr->pageSize = vm_PageSize;
	    traceStartPtr->numPages = vmStat.numPhysPages;
	    traceStartPtr->codeStartAddr = mach_KernStart;
	    traceStartPtr->dataStartAddr = (Address)&etext;
	    traceStartPtr->stackStartAddr = vmStackBaseAddr;
	    traceStartPtr->mapStartAddr = vmMapBaseAddr;
	    traceStartPtr->cacheStartAddr = vmBlockCacheBaseAddr;
	    traceStartPtr->cacheEndAddr = vmBlockCacheEndAddr;
	    bcopy((Address)&vmStat, (Address)&traceStartPtr->startStats,
		    sizeof(Vm_Stat));
	    traceStartPtr->tracesPerSecond = vmTracesPerClock;
	    vmTraceFirstByte = 0;
	    vmTraceNextByte = sizeof(Vm_TraceStart);

	    (void)strcpy(fileName, VM_TRACE_FILE_NAME);
	    (void)sprintf(hostNum, "%u", (unsigned) Sys_GetHostId());
	    (void)strcat(fileName, hostNum);

	    status = Fs_Open(fileName, FS_WRITE | FS_CREATE,
			     FS_FILE, 0660, &vmTraceFilePtr);
	    if (status != SUCCESS) {
		printf("Warning: Vm_Cmd: Couldn't open trace file, reason %x\n",
		    status);
	    } else {
		vmTraceNeedsInit = TRUE;
	    }
	    break;
	}
	case VM_END_TRACING: {
	    Vm_TraceEnd		traceEnd;

	    bcopy((Address)&vmStat, (Address)&traceEnd.endStats,
		    sizeof(Vm_TraceEnd));
	    bcopy((Address)&vmTraceStats, (Address)&traceEnd.traceStats,
		    sizeof(Vm_TraceStats));
	    VmStoreTraceRec(VM_TRACE_END_REC, sizeof(Vm_TraceEnd),
			    (Address)&traceEnd, FALSE);

	    vm_Tracing = FALSE;
	    vmClockSleep = timer_IntOneSecond;
	    if (vmTraceFilePtr != (Fs_Stream *)NIL) {
		VmTraceDump((ClientData)0, (Proc_CallInfo *)NIL);
		(void)Fs_Close(vmTraceFilePtr);
		vmTraceFilePtr = (Fs_Stream *)NIL;
	    }
	    printf("Vm_Cmd: Tracing ended: Traces=%d Drops=%d\n",
			vmTraceTime, vmTraceStats.traceDrops);
	    printf("		       Dumps=%d Bytes=%d.\n",
			vmTraceStats.traceDumps, vmTraceNextByte);
	    break;
	}
	case VM_SET_WRITEABLE_PAGEOUT:
	    SETVAR(vmWriteablePageout, arg);
	    break;
	case VM_SET_WRITEABLE_REF_PAGEOUT:
	    SETVAR(vmWriteableRefPageout, arg);
	    break;
	case 1999:
	    SETVAR(vmShmDebug, arg);
	    break;

        default:
	    status = VmMach_Cmd(command, arg);
            break;
    }
 
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SetVal --
 *
 *      Set a given VM variable.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The given variable is set.
 *
 *----------------------------------------------------------------------
 */
void
SetVal(descript, newVal, valPtr)
    char	*descript;
    int		newVal;
    int		*valPtr;
{
    printf("%s val was %d, is %d\n", descript, *valPtr, newVal);
    *valPtr = newVal;
}

/*
 *----------------------------------------------------------------------
 *
 * Vm_Mmap --
 *
 *	Map a page.
 *
 * Results:
 *	Status from the map.
 *
 * Side effects:
 *	Maps the page.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_Mmap(startAddr, length, prot, share, streamID, offset, mappedAddr)
    Address	startAddr;	/* Requested starting virt-addr. */
    int		length;		/* Length of mapped segment. */
    int		prot;		/* Protection for mapped segment. */
    int		share;		/* Private/shared flag. */
    int		streamID;	/* Open file to be mapped. */
    int		offset;		/* Offset into mapped file. */
    Address	*mappedAddr;	/* Mapped address. */
{
    Proc_ControlBlock	*procPtr;
    Fs_Stream 		*streamPtr;
    int			pnum;
    ReturnStatus	status = SUCCESS;
    Fs_Attributes	attr;
    Vm_SharedSegTable	*segTabPtr;
    Vm_Segment		*segPtr;
    Vm_SegProcList		*sharedSeg;
    Vm_VirtAddr		virtAddr;
    Fs_UserIDs		ids;
    Fs_Stream 		*filePtr;

    dprintf("Vm_Mmap: Entering\n");
    if ( ((int)startAddr & (vm_PageSize-1)) ||
	    ((int)offset & (vm_PageSize-1))) {
	printf("Vm_Mmap: Invalid start or offset\n");
	return VM_WRONG_SEG_TYPE;
    }
    startAddr = VmMach_SharedStart(startAddr,length);
    procPtr = Proc_GetCurrentProc();
    status = Fs_GetStreamPtr(procPtr,streamID,&streamPtr);
    dprintf("Vm_Mmap: procPtr: %x  streamID: %d streamPtr: %x\n",
	    procPtr,streamID,(int)streamPtr);
    if (status != SUCCESS) {
	printf("Vm_Mmap: Fs_GetStreamPtr failure\n");
	return status;
    }
    Fs_StreamCopy(streamPtr,&filePtr);

    status = Fs_GetAttrStream(filePtr,&attr);
    if (status != SUCCESS) {
	printf("Vm_Mmap: Fs_GetAttrStream failure\n");
	return status;
    }
    printf("file: fileNumber %d size %d\n",attr.fileNumber, attr.size);

    /* 
     * Check permissions.
     */
    if (attr.type != FS_FILE) {
	printf("Vm_Mmap: not a file\n");
	return VM_WRONG_SEG_TYPE;
    }
    if (!(filePtr->flags & FS_READ) || !(prot & (PROT_READ | PROT_WRITE)) ||
	    ((prot & PROT_WRITE) && !(filePtr->flags & FS_WRITE)) ||
	    ((prot & PROT_EXEC) && !(filePtr->flags & FS_EXECUTE))) {
	printf("Vm_Mmap: protection failure\n");
	return VM_WRONG_SEG_TYPE;
    }

    /*
     * See if a shared segment is already using the specified file.
     */
    segPtr = (Vm_Segment *) NULL;
    LIST_FORALL((List_Links *)&sharedSegTable, (List_Links *)segTabPtr) {
	if (segTabPtr->serverID == attr.serverID && segTabPtr->domain ==
		    attr.domain && segTabPtr->fileNumber == attr.fileNumber) {
	    segPtr = segTabPtr->segPtr;
	    break;
	}
    }

    pnum = (int)startAddr>>vmPageShift;
    pnum = 10288;

    if (segPtr == (Vm_Segment *)NULL) {
	dprintf("Vm_Mmap: New segment\n");
	/*
	 * Create a new segment and add to the shared segment list.
	 */
	
	/*
	status = Fs_Open("/sprite/users/shirriff/tmp", FS_READ |
		FS_WRITE, FS_FILE, 0660, &filePtr);
	segTabPtr->sharedSegment = Vm_SegmentNew(VM_SHARED,filePtr,0,
		1,pnum,procPtr);
	Vm_ValidatePages(sharedSegment,pnum,pnum+(length>>vmPageShift)-1,
		FALSE,TRUE);
	*/
	segTabPtr = (Vm_SharedSegTable*) malloc(sizeof(Vm_SharedSegTable));
	dprintf("Vm_Mmap: creating new segment\n");
	segTabPtr->segPtr = Vm_SegmentNew(VM_SHARED,(Fs_Stream *)NIL,
		0,length>>vmPageShift,pnum,procPtr);
		/*
		offset,length>>vmPageShift,pnum,procPtr);
		*/
	segTabPtr->segPtr->swapFilePtr = filePtr;
	segTabPtr->segPtr->flags |= VM_SWAP_FILE_OPENED;
	segTabPtr->serverID = attr.serverID;
	segTabPtr->domain = attr.domain;
	segTabPtr->fileNumber = attr.fileNumber;
	segTabPtr->refCount = 0;
	dprintf("Vm_Mmap: Validating pages %x to %x\n",
		pnum,pnum+(length>>vmPageShift)-1);
	Vm_ValidatePages(segTabPtr->segPtr,pnum,pnum+(length>>vmPageShift)-1,
		FALSE,TRUE);
	dprintf("Vm_Mmap: Finished validating pages\n");
	List_Insert((List_Links *)segTabPtr,
		LIST_ATFRONT((List_Links *)&sharedSegTable));
    } else {
	int i;
	/*
	startAddr = (Address)(segPtr->offset<<vmPageShift);
	*/
	if (length > (segPtr->numPages<<vmPageShift)) {
	    dprintf("Vm_Mmap: Enlarging segment: 0 to %d\n",
		    ((int)length-1)>>vmPageShift);
	    status = VmAddToSeg(segPtr,segPtr->offset,segPtr->offset+
		    (length>>vmPageShift)-1);
	    if (status != SUCCESS) {
		printf("VmAddToSeg failure\n");
		return status;
	    }
	}
	for (i=0;i<32;i++) {
	    if (segPtr->ptPtr[i] & VM_VIRT_RES_BIT) {
		dprintf("1");
	    } else {
		dprintf("0");
	    }
	}
	dprintf("\n");
	if (procPtr->vmPtr->sharedSegs == (List_Links *)NULL ||
	    !VmCheckSharedSegment(procPtr,segPtr)) {
	    /*
	     * Process is not using segment.
	     */
	    dprintf("Vm_Mmap: Adding reference to proc to segment\n");
	    Vm_SegmentIncRef(segPtr,procPtr);
	} else {
	    /*
	     * Process is already using segment.
	     */
	    dprintf("Vm_Mmap: Process is using segment.\n");
	}
    }

    /*
    VmVirtAddrParse(procPtr,startAddr, &virtAddr);
    if (virtAddr.segPtr != (Vm_Segment *)NULL) {
	dprintf("Vm_Mmap: Page already in use\n");
    }
    */

    /*
     * Unmap any current mapping at the address range.
     */
    status=Vm_Munmap(startAddr,length,1);
    if (status != SUCCESS) {
	printf("Vm_Mmap: Vm_Munmap failure\n");
	return status;
    }

    /*
     * Add the segment to the process's list of shared segments.
     */
    dprintf("Vm_Mmap: Adding segment to list\n");
    if (procPtr->vmPtr->sharedSegs == (List_Links *)NULL) {
	dprintf("Vm_Mmap: New proc list\n");
	procPtr->vmPtr->sharedSegs = (List_Links *)
		malloc(sizeof(Vm_SegProcList));
	List_Init((List_Links *)procPtr->vmPtr->sharedSegs);
    }
    sharedSeg = (Vm_SegProcList *)malloc(sizeof(Vm_SegProcList));
    sharedSeg->fd = streamID;
    sharedSeg->stream = filePtr;
    sharedSeg->segTabPtr = segTabPtr;
    segTabPtr->refCount++;
    sharedSeg->addr = startAddr;
    sharedSeg->mappedStart = startAddr;
    sharedSeg->offset = (int)startAddr>>vmPageShift;
    sharedSeg->mappedEnd = startAddr+length-1;
    sharedSeg->prot = (prot&PROT_WRITE) ? 0 : VM_READONLY_SEG;
    if ((int)sharedSeg->segTabPtr == -1) {
	dprintf("Vm_Mmap: Danger: sharedSeg->segTabPtr is -1!\n");
    }
    List_Insert((List_Links *)sharedSeg,
	    LIST_ATFRONT((List_Links *)procPtr->vmPtr->sharedSegs));

    PrintSharedSegs(procPtr);
    dprintf("Vm_Mmap: Completed page mapping\n");
    dprintf("HEAP->ptSize: %x HEAP->offset: %x STACK->offset %x\n",
	    procPtr->vmPtr->segPtrArray[VM_HEAP]->ptSize,
	    procPtr->vmPtr->segPtrArray[VM_HEAP]->offset,
	    procPtr->vmPtr->segPtrArray[VM_STACK]->offset);
    *mappedAddr = startAddr;
    return status ;

}

/*
 *----------------------------------------------------------------------
 *
 * Vm_Munmap --
 *
 *	Unmaps the process's pages in a specified address range.
 *
 * Results:
 *	Status from the unmap operation.
 *
 * Side effects:
 *	Unmaps the pages.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_Munmap(startAddr, length, noError)
    Address	startAddr;	/* Starting virt-addr. */
    int		length;		/* Length of mapped segment. */
    int		noError;	/* 1 if don't care about errors
				   from absent segments. */
{
    ReturnStatus	status = SUCCESS;
    Proc_ControlBlock	*procPtr;
    Vm_SegProcList		*segProcPtr;
    Vm_SegProcList		*newSegProcPtr;
    Address		addr;
    Address		addr1;
    Address		endAddr;
    Vm_VirtAddr		virtAddr;

    virtAddr.flags = 0;
    virtAddr.offset = 0;

    dprintf("Vm_Munmap: Unmapping shared pages\n");

    procPtr = Proc_GetCurrentProc();
    endAddr = startAddr+length;
    for (addr=startAddr; addr<endAddr; ) {

	dprintf("Vm_Munmap: eliminating %x to %x\n",(int)addr,(int)endAddr-1);
	/* Find the segment corresponding to this address. */
	if (procPtr->vmPtr->sharedSegs != (List_Links *)NULL) {
	    segProcPtr = VmFindSharedSegment(procPtr->vmPtr->sharedSegs,addr);
	} else if (noError) {
	    return SUCCESS;
	} else {
	    dprintf("Vm_Munmap: Process has no shared segments\n");
	    return VM_WRONG_SEG_TYPE;
	}

	/* There are three possibilities:
	   1. The unmap can remove a mapping.
	   2. The unmap can shrink a mapping.
	   3. The unmap can split a mapping.
	*/

	if (segProcPtr == (Vm_SegProcList *)NULL) {
	    if (noError) {
		/*
		 * Move to next mapped segment, so we can keep unmapping.
		 */
		addr1 = endAddr;
		LIST_FORALL(procPtr->vmPtr->sharedSegs,
			(List_Links *)segProcPtr) {
		    if (segProcPtr->mappedStart > addr && 
			segProcPtr->mappedStart < addr1) {
			addr1 = segProcPtr->mappedStart;
		    }
		}
	    } else {
		dprintf("Vm_Munmap: Page to unmap not in valid range\n");
		status = VM_WRONG_SEG_TYPE;
	    }
	} else {
	    addr1 = segProcPtr->mappedEnd+1;

	    /*
	     * Invalidate the pages to be mapped out.
	     */
	    virtAddr.page = max((int)segProcPtr->mappedStart,(int)addr)
		    >> vmPageShift;
	    virtAddr.segPtr = segProcPtr->segTabPtr->segPtr;
	    virtAddr.sharedPtr = segProcPtr;
	    dprintf("Vm_Munmap: invalidating: %x to %x\n",
		    (int)virtAddr.page<<vmPageShift,
		    min((int)segProcPtr->mappedEnd+1, (int)endAddr));
		    
	    VmPageInvalidateRange(&virtAddr,min((int)segProcPtr->mappedEnd+1,
		    (int)endAddr)-(((int)virtAddr.page)<<vmPageShift));

	    if (segProcPtr->mappedStart < addr) {
		dprintf("Vm_Munmap: Shortening mapped region (1)\n");
		if (segProcPtr->mappedEnd >= endAddr) {
		    dprintf("Vm_Munmap: Splitting mapped region\n");
		    /*
		     * Split the mapped region.
		     */
		    newSegProcPtr = (Vm_SegProcList *)malloc(sizeof(Vm_SegProcList));
		    *newSegProcPtr = *segProcPtr;
		    newSegProcPtr->mappedStart = endAddr;
		    newSegProcPtr->mappedEnd = segProcPtr->mappedEnd;
		    segProcPtr->segTabPtr->refCount++;
		    List_Insert((List_Links *)newSegProcPtr, LIST_AFTER(
			    (List_Links *)segProcPtr));
		}
		/*
		 * Shrink the first mapping.
		 */
		segProcPtr->mappedEnd = addr-1;
	    } else if (segProcPtr->mappedEnd >= endAddr) {
		/*
		 * Shrink the mapped region.
		 */
		dprintf("Vm_Munmap: Shortening mapped region (2)\n");
		segProcPtr->mappedStart = endAddr;
	    } else {
		dprintf("Vm_Munmap: Removing mapped region\n");
		/*
		 * Remove the mapped region.
		 */
		VmDeleteSharedSegment(procPtr,segProcPtr);
	    }
	}
	addr = addr1;
    }

    PrintSharedSegs(procPtr);
    dprintf("Vm_Munmap: done\n");
    return status ;
}
