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
#include "vmTrace.h"
#include "user/vm.h"
#include "sync.h"
#include "sys.h"
#include "byte.h"
#include "cvt.h"
#include "mem.h"
#include "string.h"


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
 *	to synchronize when he is expanding his HEAP segment.
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

#define SETVAR(var, val) SetVal("var", val, (int *)&(var))


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
    Vm_Command  command;
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
	    Byte_Copy(copySize, (Address) arg, buffer);
	    Vm_MakeUnaccessible((Address) arg, numBytes);
	    break;
	case VM_DO_MAKE_ACCESS_OUT:
	    Vm_MakeAccessible(0, copySize, (Address) arg, &numBytes,
			      (Address *) &arg);
	    Byte_Copy(copySize, buffer, (Address) arg);
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
	    SETVAR(vm_CanCOW, arg);
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
	    char		hostNum[CVT_INT_BUF_SIZE];

	    if (vmTraceFilePtr != (Fs_Stream *)NIL) {
		Sys_Printf("VmCmd: Tracing already running.\n");
		break;
	    }
	    vmTracesPerClock = arg;
	    Sys_Printf("Vm_Cmd: Tracing started at %d times per second\n",
			arg);

	    if (vmTraceBuffer == (char *)NIL) {
		vmTraceBuffer = (char *)Mem_Alloc(VM_TRACE_BUFFER_SIZE);
	    }
	    Byte_Zero(sizeof(vmTraceStats), (Address)&vmTraceStats);

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
	    Byte_Copy(sizeof(Vm_Stat), (Address)&vmStat, 
		      (Address)&traceStartPtr->startStats);
	    traceStartPtr->tracesPerSecond = vmTracesPerClock;
	    vmTraceFirstByte = 0;
	    vmTraceNextByte = sizeof(Vm_TraceStart);

	    (void)String_Copy(VM_TRACE_FILE_NAME, fileName);
	    (void)Cvt_UtoA((unsigned) Sys_GetHostId(), 10, hostNum);
	    (void)String_Cat(hostNum, fileName);

	    status = Fs_Open(fileName, FS_WRITE | FS_CREATE,
			     FS_FILE, 0660, &vmTraceFilePtr);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING, 
		    "Vm_Cmd: Couldn't open trace file, reason %x\n", 
		    status);
	    } else {
		vmTraceNeedsInit = TRUE;
	    }
	    break;
	}
	case VM_END_TRACING: {
	    Vm_TraceEnd		traceEnd;

	    Byte_Copy(sizeof(Vm_TraceEnd), (Address)&vmStat,
		      (Address)&traceEnd.endStats);
	    Byte_Copy(sizeof(Vm_TraceStats), (Address)&vmTraceStats,
		      (Address)&traceEnd.traceStats);
	    VmStoreTraceRec(VM_TRACE_END_REC, sizeof(Vm_TraceEnd),
			    (Address)&traceEnd, FALSE);

	    vm_Tracing = FALSE;
	    vmClockSleep = timer_IntOneSecond;
	    if (vmTraceFilePtr != (Fs_Stream *)NIL) {
		VmTraceDump((ClientData)0, (Proc_CallInfo *)NIL);
		(void)Fs_Close(vmTraceFilePtr);
		vmTraceFilePtr = (Fs_Stream *)NIL;
	    }
	    Sys_Printf("Vm_Cmd: Tracing ended: Traces=%d Drops=%d\n",
			vmTraceTime, vmTraceStats.traceDrops);
	    Sys_Printf("		       Dumps=%d Bytes=%d.\n",
			vmTraceStats.traceDumps, vmTraceNextByte);
	    Sys_Printf("		       PMEGs=%d\n",
			vmTraceStats.machStats.pmegsChecked);
	    break;
	}

        default:
            Sys_Panic(SYS_WARNING, "Vm_Cmd: Unknown command.\n");
	    status = GEN_INVALID_ARG;
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
    Sys_Printf("%s val was %d, is %d\n", descript, *valPtr, newVal);
    *valPtr = newVal;
}
