/* vmSubr.c -
 *
 *     This file contains miscellaneous virtual memory routines.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmSubr.c,v 1.17 92/07/13 21:12:45 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <bstring.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>
#include <string.h>

#include <timer.h>
#include <utils.h>
#include <vm.h>
#include <vmInt.h>

static Boolean copyDebug = FALSE;

/* Forward references: */
static ReturnStatus VmCopy _ARGS_((vm_size_t requestedBytes,
			mach_port_t fromTask, Address fromAddr,
			mach_port_t toTask, Address toAddr));


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
    int			numBytes;	/* The maximum number of bytes
					 * to copy in. */
    Proc_LockedPCB	*fromProcPtr;	/* Which process to copy from.*/
    Address		fromAddr;	/* The address to copy from */
    Address		toAddr;		/* The address to copy to */
    Boolean		toKernel;	/* This copy is happening to 
					 * the kernel's address space.*/
{
    mach_port_t toTask;		/* Mach task to copy to */
    mach_port_t fromTask;	/* Mach task to copy from */
    Proc_ControlBlock *curProcPtr; /* current user process */

    curProcPtr = Proc_GetCurrentProc();
    if (copyDebug) {
	printf("Vm_CopyInProc: from pid %x [%x + %x] -> %x",
	       fromProcPtr->pcb.processID, fromAddr, numBytes, toAddr);
	if (toKernel) {
	    printf(" (sprited)\n");
	} else {
	    printf(" (pid %x)\n", curProcPtr->processID);
	}
    }

    if (fromProcPtr->pcb.genFlags & PROC_KERNEL) {
	panic("Vm_CopyInProc wants to copy in from kernel process.\n");
    }
    if (fromProcPtr->pcb.taskInfoPtr == NULL) {
	fromTask = MACH_PORT_NULL;
    } else {
	fromTask = fromProcPtr->pcb.taskInfoPtr->task;
    }
    if (toKernel) {
	toTask = mach_task_self();
    } else if (curProcPtr->taskInfoPtr == NULL) {
	toTask = MACH_PORT_NULL;
    } else {
	toTask = curProcPtr->taskInfoPtr->task;
    }
    if (toTask == MACH_PORT_NULL || fromTask == MACH_PORT_NULL) {
	return SYS_ARG_NOACCESS;
    }

    return VmCopy((vm_size_t)numBytes, fromTask, fromAddr, toTask, toAddr);
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
 *	SUCCESS if the copy succeeded, SYS_ARG_NOACCESS if sourceAddr is 
 *	invalid. 
 *
 * Side effects:
 *	What toAddr points to is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_CopyOutProc(numBytes, sourceAddr, fromKernel, toProcPtr, destAddr)
    int			numBytes;	/* The number of bytes to copy out */
    Address		sourceAddr;	/* The address to copy from */
    Boolean		fromKernel;	/* This copy is happening from
					 * the kernel's address space.*/
    Proc_LockedPCB	*toProcPtr; 	/* Which process to copy to.*/
    Address		destAddr; 	/* The address to copy to */
{
    mach_port_t toTask;		/* Mach task to copy to */
    mach_port_t fromTask;	/* Mach task to copy from */
    Proc_ControlBlock *curProcPtr; /* current user process */

    curProcPtr = Proc_GetCurrentProc();
    if (copyDebug) {
	printf("Vm_CopyOutProc: ");
	if (fromKernel) {
	    printf("(sprited) ");
	} else {
	    printf("(pid %x) ", curProcPtr->processID);
	}
	printf("[%x + %x] -> %x (to pid %x)\n",
	       sourceAddr, numBytes, destAddr, toProcPtr->pcb.processID);
    }

    if (toProcPtr->pcb.genFlags & PROC_KERNEL) {
	panic("Vm_CopyOutProc wants to copy to kernel process.\n");
    }
    if (toProcPtr->pcb.taskInfoPtr == NULL) {
	toTask = MACH_PORT_NULL;
    } else {
	toTask = toProcPtr->pcb.taskInfoPtr->task;
    }
    if (fromKernel) {
	fromTask = mach_task_self();
    } else if (curProcPtr->taskInfoPtr == NULL) {
	fromTask = MACH_PORT_NULL;
    } else {
	fromTask = curProcPtr->taskInfoPtr->task;
    }
    if (toTask == MACH_PORT_NULL || fromTask == MACH_PORT_NULL) {
	return SYS_ARG_NOACCESS;
    }

    return VmCopy((vm_size_t)numBytes, fromTask, sourceAddr, toTask,
		  destAddr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyIn --
 *
 *	Copy from current process's address space into the server's.  
 *	In native Sprite, this routine can be called with the current 
 *	process locked or unlocked.  This means that we can't lock the 
 *	current process here.  We should be protected from nasty
 *	changes to the PCB by the PROC_BEING_SERVED flag.
 *
 * Results:
 *	SUCCESS if the copy went okay.  Returns SYS_ARG_NOACCESS if the 
 *	requested region is inaccessible.
 *
 * Side effects:
 *	Bytes are copied into the destination buffer.  Might update 
 *	instrumentation in the current process's PCB.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_CopyIn(numBytes, sourcePtr, destPtr)
    int numBytes;		/* number of bytes to copy */
    Address sourcePtr;		/* address in user process */
    Address destPtr;		/* address in Sprite server */
{
    Time startTime;		/* instrumentation */
    Time endTime;
    ReturnStatus status;
    Proc_ControlBlock *procPtr;

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }
    /* 
     * We don't know for sure that the current process is locked, but the 
     * rest of the server knows not to disturb the VM of a process that's 
     * having a request processed.
     */
    status = Vm_CopyInProc(numBytes, Proc_AssertLocked(Proc_GetCurrentProc()),
			   sourcePtr, destPtr, TRUE);
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	procPtr = Proc_GetCurrentProc();
	Time_Add(endTime, procPtr->copyInTime, &procPtr->copyInTime);
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CopyOut --
 *
 *	Copy from the server's address space into a user process's address 
 *	space.
 *
 * Results:
 *	SUCCESS if the copy went okay.  Returns SYS_ARG_NOACCESS if the 
 *	destination region is unwritable or non-existent.
 *
 * Side effects:
 *	Bytes are copied into the user process's address space.  Might
 *	update instrumentation in the current process's PCB.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_CopyOut(numBytes, sourcePtr, destPtr)
    int numBytes;		/* number of bytes to copy */
    Address sourcePtr;		/* address in Sprite server */
    Address destPtr;		/* address in user process */
{
    Time startTime;		/* instrumentation */
    Time endTime;
    ReturnStatus status;
    Proc_ControlBlock *procPtr;

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }
    /* 
     * We don't know for sure that the current process is locked, but the 
     * rest of the server knows not to disturb the VM of a process that's 
     * having a request processed.
     */
    status = Vm_CopyOutProc(numBytes, sourcePtr, TRUE,
			    Proc_AssertLocked(Proc_GetCurrentProc()),
			    destPtr);
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	procPtr = Proc_GetCurrentProc();
	Time_Add(endTime, procPtr->copyOutTime, &procPtr->copyOutTime);
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * VmCopy --
 *
 *	Copy bytes from one task to another.
 *
 * Results:
 *	Returns SUCCESS if the copy went okay.  Returns SYS_ARG_NOACCESS if 
 *	there were problems accessing either the source or destination 
 *	regions.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
VmCopy(requestedBytes, fromTask, fromAddr, toTask, toAddr)
    vm_size_t requestedBytes;	/* number of bytes to copy */
    mach_port_t fromTask;	/* task to copy from */
    Address fromAddr;		/* where to start copying from */
    mach_port_t toTask;		/* task to copy to */
    Address toAddr;		/* where to start copying into */
{
    Address startFromPage;	/* start addr of page-aligned "from" region */
    Address endFromPage;	/* next page after "from" region */
    Address startToPage;	/* start addr of page-aligned "to" region */
    Address endToPage;		/* next page after "to" region */
    Address fromBuffer = NULL;	/* bytes read from "from" task */
    vm_size_t fromBufferSize;	/* number of bytes in fromBuffer */
    Address toBuffer = NULL;	/* bytes read from "to" task */
    vm_size_t toBufferSize;	/* number of bytes in toBuffer */
    Address writeBuffer;	/* which buffer to actually write */
    vm_size_t writeSize;	/* number of bytes in writeBuffer */
    kern_return_t kernStatus;
    ReturnStatus status = SUCCESS;

    /* 
     * If there's nothing to copy, quit now.  This isn't just a performance 
     * optimization: in the MK63 kernel, vm_write apparently hangs if you
     * do a vm_read from an externally managed object and then vm_write the
     * same buffer back without first changing it.
     */
    if (requestedBytes == 0) {
	return SUCCESS;
    }

    if (copyDebug) {
	printf("VmCopy: want (%x+%x) -> %x\n", fromAddr, requestedBytes,
	       toAddr);
    }

    /* 
     * Assume that the amount to be copied is small enough that bcopy is 
     * cheaper than vm_read/vm_write.  So, if we are copying from the 
     * server, do nothing here.  Otherwise, read in the pages containing 
     * the source bytes and update the "from" address.
     */
    
    if (fromTask != mach_task_self()) {
	startFromPage = Vm_TruncPage(fromAddr);
	endFromPage = Vm_RoundPage(fromAddr + requestedBytes);
	kernStatus = vm_read(fromTask, (vm_address_t)startFromPage,
			     (vm_size_t)(endFromPage - startFromPage),
			     (pointer_t *)&fromBuffer,
			     (mach_msg_type_number_t *)&fromBufferSize);
	if (kernStatus == KERN_PROTECTION_FAILURE ||
	    kernStatus == KERN_INVALID_ADDRESS) {
	    status = SYS_ARG_NOACCESS;
	    goto bailOut;
	} else if (kernStatus != KERN_SUCCESS) {
	    panic("VmCopy read failed: %s\n", mach_error_string(kernStatus));
	} else if (fromBufferSize != endFromPage - startFromPage) {
	    printf("VmCopy: short read.\n");
	    status = SYS_ARG_NOACCESS;
	    goto bailOut;
	}
	if (copyDebug) {
	    printf("VmCopy: `from' buffer at %x: ", fromBuffer);
	}
	fromAddr = fromBuffer + (fromAddr - startFromPage);
    }

    /* 
     * fromAddr now points at a directly accessible copy of the "from"
     * bytes.  If we are copying to the server, just do a bcopy.  (Not only
     * is this faster, it is sometimes necessary to avoid clobbering
     * variables used by VmCopy.)
     * 
     * Otherwise, we are writing into a user process.  If we are copying
     * entire aligned pages, then a simple call to vm_write suffices.
     * If the pages are unaligned, or we're not copying entire pages, then 
     * we first have to read in the destination region, overwrite it with 
     * the new bytes, then write the region back out again.
     */
    
    if (toTask == mach_task_self()) {
	if (copyDebug) {
	    printf("VmCopy: bcopy (%x + %x) -> %x\n",
		   fromAddr, requestedBytes, toAddr);
	}
	bcopy(fromAddr, toAddr, requestedBytes);
    } else {
	startToPage = Vm_TruncPage(toAddr);
	endToPage = Vm_RoundPage(toAddr + requestedBytes);
	if (fromAddr == Vm_TruncPage(fromAddr) && toAddr == startToPage &&
	    requestedBytes == trunc_page(requestedBytes)) {
	    writeBuffer = fromAddr;
	    writeSize = requestedBytes;
	} else {
	    /* 
	     * Unaligned case.
	     */
	    kernStatus = vm_read(toTask, (vm_address_t)startToPage,
				 (vm_size_t)(endToPage - startToPage),
				 (pointer_t *)&toBuffer,
				 (mach_msg_type_number_t *)&toBufferSize);
	    if (kernStatus == KERN_PROTECTION_FAILURE ||
		kernStatus == KERN_INVALID_ADDRESS) {
		status = SYS_ARG_NOACCESS;
		goto bailOut;
	    } else if (kernStatus != KERN_SUCCESS) {
		panic("VmCopy second read failed: %s\n",
		      mach_error_string(kernStatus));
	    } else if (toBufferSize != endToPage - startToPage) {
		printf("VmCopy: short second read.\n");
		status = SYS_ARG_NOACCESS;
		goto bailOut;
	    }
	    if (copyDebug) {
		printf("VmCopy: `to' buffer at %x ", toBuffer);
		printf("VmCopy: bcopy (%x + %x) -> %x\n",
		       fromAddr, requestedBytes,
		       toBuffer + (toAddr - startToPage));
	    }
	    bcopy(fromAddr, toBuffer + (toAddr - startToPage),
		  requestedBytes);
	    writeBuffer = toBuffer;
	    writeSize = toBufferSize;
	}
	
	/* 
	 * Okay, now do the write.  writeBuffer has the address of the 
	 * start of the region to write, and writeSize tells how big the 
	 * region is.
	 */
	
	if (copyDebug) {
	    printf("VmCopy: write (%x + %x) -> %x",
		   writeBuffer, writeSize, startToPage);
	}
	kernStatus = vm_write(toTask, (vm_address_t)startToPage,
			      (pointer_t)writeBuffer, writeSize);
	if (copyDebug) {
	    printf(".\n");
	}
	if (kernStatus == KERN_PROTECTION_FAILURE ||
	    kernStatus == KERN_INVALID_ADDRESS) {
	    status = SYS_ARG_NOACCESS;
	    goto bailOut;
	} else if (kernStatus != KERN_SUCCESS) {
	    panic("VmCopy write failed: %s\n", mach_error_string(kernStatus));
	}
    }

 bailOut:
    if (copyDebug) {
	printf("\n");
    }
    if (fromBuffer != NULL) {
	vm_deallocate(mach_task_self(), (vm_address_t)fromBuffer,
		      fromBufferSize);
    }
    if (toBuffer != NULL) {
	vm_deallocate(mach_task_self(), (vm_address_t)toBuffer,
		      toBufferSize);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MakeAccessible --
 *
 *	Map memory from the current user process into the server's address 
 *	space.  As with Vm_Copy{In,Out}, we require that other parts of the 
 *	server recognize that the current process has a pending request, so 
 *	that they don't do anything bad to the process's VM.
 *	
 *	Do not call this routine with the current process locked, or you 
 *	will deadlock yourself.
 *
 * Results:
 *	Fills in the server address that corresponds to the requested user 
 *	address.  Also fills in the actual number of bytes made accessible, 
 *	which will never be greater than the number of requested bytes.
 *	In case of problems, the server address is set to NULL, and the
 *	byte count is set to 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Vm_MakeAccessible(accessType, numBytes, userAddr, retBytesPtr, retAddrPtr)
    int			accessType;	/* One of VM_READONLY_ACCESS, 
					 * VM_OVERWRITE_ACCESS, 
					 * VM_READWRITE_ACCESS. */
    int			numBytes;	/* The maximum number of bytes to make 
					 * accessible. */
    Address		userAddr;	/* The address in the user's address
					 * space to start at. */
    register	int	*retBytesPtr;	/* The actual number of bytes 
					 * made accessible. */
    register	Address	*retAddrPtr;	/* The server address that can be 
					 * used  to access the bytes. */
{
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();
    Vm_Segment *segPtr = NULL;	/* the segment for the given address range */
    kern_return_t kernStatus;
    Boolean suspended = FALSE;	/* was the task suspended? */
    vm_offset_t offset;		/* segment offset for userAddr */
    vm_offset_t mapOffset;	/* start of page for offset */
    vm_size_t mapBytes;		/* number of bytes to actually map */
    Address serverRegionAddr;	/* server address for start of mapped region */
    ReturnStatus status;
    vm_prot_t protectCode;	/* Mach protection code */

    /* 
     * Sanity checks.  Some routines (e.g., Test_Rpc) call with a byte
     * count of 0 (don't ask me what this is supposed to mean).  This 
     * causes problems when we go to compute the arguments to vm_map, so 
     * insist on getting at least one byte.
     */
    if (procPtr->genFlags & PROC_KERNEL) {
	panic("Vm_MakeAccessible called with kernel process.\n");
    }
    if (numBytes < 0) {
	panic("Vm_MakeAccessible: negative byte count (%d)\n",
	      numBytes);
    }
    if (numBytes == 0) {
	numBytes = 1;
    }

    /* 
     * Set up for an error return.
     */
    *retBytesPtr = 0;
    *retAddrPtr = NULL;

    protectCode = Utils_MapSpriteProtect(accessType);

    /* 
     * Make sure the process's task is suspended, so that VM can't change 
     * while we're working.  (Maybe this is unnecessary?)
     */
    if (procPtr->taskInfoPtr == NULL) {
	goto done;
    }
    kernStatus = task_suspend(procPtr->taskInfoPtr->task);
    if (kernStatus != KERN_SUCCESS) {
	goto done;
    }
    suspended = TRUE;

    /* 
     * Figure out which segment corresponds to the given address and where 
     * in the segment the address starts.  If the requested range falls off 
     * the end of the segment, numBytes might be reduced to the number of 
     * available bytes.
     * XXX if a non-Sprite memory object occupies some part of the range, 
     * this routine will return a failure code, so we'll pretend that the 
     * region is unreadable.
     */
    status = VmAddrParse(procPtr, userAddr, &numBytes, &segPtr, &offset);
    if (status != SUCCESS) {
	printf("Vm_MakeAccessible: couldn't get segment info: %s\n",
	       Stat_GetMsg(status));
	goto done;
    }

    /* 
     * We want to map the segment on a page boundary, so figure out what 
     * address and size to actually ask for.
     */
    mapOffset = trunc_page(offset);
    mapBytes = round_page(offset + numBytes) - mapOffset;

    /* 
     * Map the segment into the server's address space, and mark the 
     * segment so that we know it's in use.
     */

    serverRegionAddr = 0;
    kernStatus = vm_map(mach_task_self(), (vm_address_t *)&serverRegionAddr,
			mapBytes, 0, TRUE, segPtr->requestPort, mapOffset,
			FALSE, protectCode, protectCode, VM_INHERIT_NONE);
    if (kernStatus == KERN_SUCCESS) {
	VmSegmentLock(segPtr);
	if (segPtr->controlPort == MACH_PORT_NULL) {
	    segPtr->controlPort = MACH_PORT_DEAD;
	}
	VmSegmentUnlock(segPtr);
    }
    Vm_SegmentRelease(segPtr);

    if (kernStatus != KERN_SUCCESS) {
	goto done;
    }

    /* 
     * Success.  Return the address in the mapped region corresponding to 
     * the requested address.  For the byte count, return the (possibly 
     * truncated) requested number of bytes.
     */
    *retBytesPtr = numBytes;
    *retAddrPtr = serverRegionAddr + (offset - mapOffset);

 done:
    if (suspended) {
	(void)task_resume(procPtr->taskInfoPtr->task);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MakeUnacessible --
 *
 *	Take the given server virtual address and make the range of pages
 *	that it addresses unaccessible.  
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
Vm_MakeUnaccessible(addr, numBytes)
    Address		addr;
    int		numBytes;
{
    kern_return_t kernStatus;

    kernStatus = vm_deallocate(mach_task_self(), (vm_address_t)addr, 
			       (vm_size_t)numBytes);
    if (kernStatus != KERN_SUCCESS) {
	printf("Vm_MakeUnaccessible not happy: %s\n",
	       mach_error_string(kernStatus));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_StringNCopy --
 *
 *	Copy a NULL terminated string in from a user process.
 *
 * Results:
 *	Returns SUCCESS or SYS_ARG_NOACCESS.  Fills in the length of the 
 *	resulting string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_StringNCopy(numBytes, sourcePtr, destPtr, stringLengthPtr)
    int numBytes;		/* maximum number of bytes to copy */
    Address sourcePtr;		/* where to copy from (user address) */
    Address destPtr;		/* where to copy to (server address) */
    int *stringLengthPtr;	/* OUT: length of copied string */
{
    Address mappedAddr;		/* sourcePtr, mapped into the server */
    int bytesMapped;		/* number of bytes actually mapped */

    *stringLengthPtr = 0;

    /* 
     * This can fail if sourcePtr refers to memory that isn't backed by 
     * Sprite.  "Life is hard."
     */
    Vm_MakeAccessible(VM_READONLY_ACCESS, numBytes, sourcePtr,
		      &bytesMapped, &mappedAddr);
    if (mappedAddr == NULL) {
	return SYS_ARG_NOACCESS;
    } else {
	register char *fromPtr, *toPtr;
	/* 
	 * We don't use strncpy for two reasons: (a) it zeros out the tail
	 * of the destination buffer, which we don't need and don't want to
	 * pay for; (b) our caller wants to know how many non-null bytes
	 * were copied.
	 */
	for (fromPtr = mappedAddr, toPtr = destPtr;
	     	fromPtr < mappedAddr + bytesMapped && *fromPtr != '\0';
	     	fromPtr++, toPtr++) {
	    *toPtr = *fromPtr;
	}
	if (fromPtr < mappedAddr + numBytes) {
	    *toPtr = '\0';
	}
	*stringLengthPtr = fromPtr - mappedAddr;
	Vm_MakeUnaccessible(mappedAddr, bytesMapped);
    }

    return SUCCESS;
}
