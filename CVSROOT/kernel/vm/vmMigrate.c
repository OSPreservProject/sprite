/*
 * vmMigrate.c --
 *
 *	Routines to handle process migration from the standpoint of virtual
 *	memory.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vm.h"
#include "vmInt.h"
#include "lock.h"
#include "proc.h"
#include "procMigrate.h"
#include "fs.h"
#include "fsio.h"
#include "stdlib.h"
#include "byte.h"
#include "stdio.h"
#include "bstring.h"
    

static ReturnStatus EncapSegment _ARGS_((Vm_Segment *segPtr,
	Proc_ControlBlock *procPtr, Address *bufPtrPtr));
ENTRY static void PrepareSegment _ARGS_((Vm_Segment *segPtr));
static ReturnStatus FlushSegment _ARGS_((Vm_Segment *segPtr));
static void FreePages _ARGS_((Vm_Segment *segPtr));
ENTRY static void LoadSegment _ARGS_((int length, register Address buffer,
	register Vm_Segment *segPtr));
ENTRY static ReturnStatus CheckSharers _ARGS_((register Vm_Segment *segPtr,
	Proc_EncapInfo *infoPtr));

/*
 * Define the number of ints transferred... FIXME: change to a struct.
 */
#define NUM_FIELDS 5



/*
 *----------------------------------------------------------------------
 *
 * Vm_InitiateMigration --
 *
 *	Set up a process for migration.  Lock the dirty pages of each
 *	segment, and free the rest.  Flush the dirty pages to disk.
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	The pages in the process are flushed.  Any copy-on-write pages
 *	are isolated.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Vm_InitiateMigration(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    int			seg;
    Vm_Segment 		*segPtr;
    Vm_Segment 		**segPtrPtr;
    int			fsSize;
    int			size = 0;
    ReturnStatus	status;
    int			varSize;


    segPtrPtr = procPtr->vmPtr->segPtrArray;
    status = CheckSharers(segPtrPtr[VM_HEAP], infoPtr);
    if (status != SUCCESS) {
	return(status);
    }
    fsSize = Fs_GetEncapSize();

    /*
     * Prepare each segment for migration.  This involves some work in
     * a monitored procedure and an unmonitored one.
     */
    for (seg = VM_CODE; seg < VM_NUM_SEGMENTS; seg++) {
	segPtr = segPtrPtr[seg];
	if (segPtr->type != VM_CODE) {
	    if (vm_CanCOW) {
		/*
		 * Get rid of all copy-on-write dependencies.
		 */
		status = VmCOWCopySeg(segPtr);
		if (status != SUCCESS) {
		    printf("Warning: Vm_MigrateSegment: Could not copy segment\n");
		    return(status);
		}
	    }
	    PrepareSegment(segPtr);
	    /*
	     * Unlock the process while flushing it to the server -- we might
	     * have to wait a while while this is going on.
	     */
	    Proc_Unlock(procPtr);
	    status = FlushSegment(segPtr);
	    Proc_Lock(procPtr);
	    if (status != SUCCESS) {
		return(status);
	    }
	    varSize = segPtr->ptSize * sizeof(Vm_PTE);
	} else {
	    varSize = sizeof(Vm_ExecInfo);
	}
	size += NUM_FIELDS * sizeof(int) + varSize + fsSize;
    }
    infoPtr->size = size;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_EncapState --
 *
 *	Encapsulate the state of a process's virtual memory.  All the
 *	work in going through its page tables has been done, so
 *	just wait for all its pages to be written to disk, then
 *	package up the state.
 *
 * Results:
 *	If any error occurs with writing the swap files, an error is
 *	indicated; otherwise, SUCCESS is returned.
 *
 * Side effects:
 *	Each of the process's segments is freed.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Vm_EncapState(procPtr, hostID, infoPtr, bufferPtr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address bufferPtr;			   /* Pointer to allocated buffer */
{
    ReturnStatus status;
    Vm_Segment 		*segPtr;
    Vm_Segment 		**segPtrPtr;
    int			seg;


    /*
     * Encapsulate the virtual memory, and set up the process so the kernel
     * knows the process has no VM on this machine.
     */
	
    procPtr->genFlags |= PROC_NO_VM;

    segPtrPtr = procPtr->vmPtr->segPtrArray;

    /*
     * Encapsulate each segment.  If it's not a code segment, it must
     * be freed up.
     */
    for (seg = VM_CODE; seg < VM_NUM_SEGMENTS; seg++) {
	segPtr = segPtrPtr[seg];
	if (segPtr->type != VM_CODE) {
	    FreePages(segPtr);
	    if (segPtr->flags & VM_SEG_IO_ERROR) {
		return(VM_SWAP_ERROR);
	    }
	}
        status = EncapSegment(segPtr, procPtr, &bufferPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_DeencapState --
 *
 *	Deencapsulate the state of a process's virtual memory. 
 *	For each segment, get the information from a foreign
 *	Vm_Segment from a buffer and create a segment for it on this
 *	(the remote) node.  Set up the file pointer for its swap file,
 *	if it is a stack or heap segment.  If it is a code segment, do
 *	a Vm_SegmentFind (just as Proc_Exec does) and initialize the
 *	page tables if necessary, then return.
 *
 * Results:
 *	If any error occurs with deencapsulating the swap files, an error is
 *	indicated; otherwise, SUCCESS is returned.
 *
 * Side effects:
 *	Creates segments for the process.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Vm_DeencapState(procPtr, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address buffer;			   /* Pointer to allocated buffer */
{
    ReturnStatus status;
    Vm_Segment 		*segPtr;
    int			seg;
    int 	offset;
    int 	fileAddr;
    int 	type;
    int 	numPages;
    int 	varSize;
    int 	ptSize;
    Fs_Stream 	*filePtr;
    int 	fsInfoSize;
    Vm_ExecInfo	*execInfoPtr;
    Vm_ExecInfo	*oldExecInfoPtr;
    Boolean	usedFile;


    fsInfoSize = Fs_GetEncapSize();

    for (seg = VM_CODE; seg < VM_NUM_SEGMENTS; seg++) {
	Byte_EmptyBuffer(buffer, int, offset);
	Byte_EmptyBuffer(buffer, int, fileAddr);
	Byte_EmptyBuffer(buffer, int, type);
	if (type != seg) {
	    if (proc_MigDebugLevel > 0) {
		panic("Vm_DeencapState: mismatch getting segment %d\n",
		      seg);
		return(FAILURE);
	    }
	}
	Byte_EmptyBuffer(buffer, int, numPages);
	Byte_EmptyBuffer(buffer, int, ptSize);
	switch (type) {
	    case VM_CODE: {
		varSize = sizeof(Vm_ExecInfo);
		oldExecInfoPtr = (Vm_ExecInfo *) buffer;
		buffer += varSize;
		status = Fsio_DeencapStream(buffer, &filePtr);
		buffer += fsInfoSize;
		if (status != SUCCESS) {
		    printf("Vm_DeencapState: Fsio_DeencapStream returned status %x.\n",
			   status);
		    return(status);
		}
		segPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
		if (segPtr == (Vm_Segment *) NIL) {
		    segPtr = Vm_SegmentNew(VM_CODE, filePtr, fileAddr,
					   numPages, offset, procPtr);
		    if (segPtr == (Vm_Segment *) NIL) {
			Vm_InitCode(filePtr, (Vm_Segment *) NIL,
				    (Vm_ExecInfo *) NIL);
			(void)Fs_Close(filePtr);
			return(VM_NO_SEGMENTS);
		    }
		    Vm_ValidatePages(segPtr, offset, 
				     offset + numPages - 1, FALSE, TRUE);
		    Vm_InitCode(filePtr, segPtr, oldExecInfoPtr);
		} else {
		    if (!usedFile) {
			(void)Fs_Close(filePtr);
		    }
		}
		procPtr->vmPtr->segPtrArray[type] = segPtr;
		break;
	    }
	    case VM_HEAP: {
		Fsio_StreamCopy(procPtr->vmPtr->segPtrArray[VM_CODE]->filePtr,
				    &filePtr);
		if (filePtr == (Fs_Stream *) NIL) {
		    panic("Vm_DeencapState: no code file pointer.\n");
		}
		break;
	    }
	    case VM_STACK: {
		filePtr = (Fs_Stream *) NIL;
		break;
	    }
	    default: {
		panic("Vm_DeencapState: unknown segment type.\n");
	    }
	}
	if (type != VM_CODE) {
	    segPtr = Vm_SegmentNew(type, filePtr, fileAddr, numPages,
				   offset, procPtr);
	    if (segPtr == (Vm_Segment *) NIL) {
		return(VM_NO_SEGMENTS);
	    }
	    procPtr->vmPtr->segPtrArray[type] = segPtr;
	    segPtr->ptSize = ptSize;
	    varSize = ptSize * sizeof(Vm_PTE);
	    LoadSegment(varSize, buffer, segPtr);
	    buffer += varSize;
	    if (proc_MigDebugLevel > 4) {
		printf("Deencapsulating swap file for segment %d.\n", type);
	    }
	    status = Fsio_DeencapStream(buffer, &segPtr->swapFilePtr);
	    buffer += fsInfoSize;
	    if (status != SUCCESS) {
		printf("Vm_DeencapState: Fsio_DeencapStream on swapFile returned status %x.\n",
		       status);
		return(status);
	    }
	    if (proc_MigDebugLevel > 4) {
		printf("Deencapsulated swap file successfully.\n");
	    }
	    if (segPtr->swapFileName != (char *) NIL) { 
		free(segPtr->swapFileName);
		segPtr->swapFileName = (char *) NIL;
	    }
	    segPtr->flags |= VM_SWAP_FILE_OPENED;
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_FinishMigration --
 *
 *	Clean up the state of a process's virtual memory after migration.
 *	The process control blocked is assumed to be unlocked on entry.
 *
 *		.. OBSOLETE ..
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Deletes segments for the process.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Vm_FinishMigration(procPtr, hostID, infoPtr, bufferPtr, failure)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address bufferPtr;			   /* Pointer to allocated buffer */
    int failure;			   /* indicates whether migration
					      succeeded */
{
    int seg;
    
    if (proc_MigDebugLevel > 4) {
	printf("Vm_FinishMigration called.\n");
    }
    
    for (seg = VM_CODE; seg < VM_NUM_SEGMENTS; seg++) {
	if (proc_MigDebugLevel > 5) {
	    printf("Vm_FinishMigration deleting segment %d.\n", seg);
	}
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[seg], procPtr);
    }
    /*
     * Would also need to set PROC_NO_VM here...
     */
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * EncapSegment --
 *
 * 	Copy the information from a Vm_Segment into a buffer, ready to
 *	be transferred to another node.  We have to duplicate the
 *	stream to the swap or code file for the segment because
 *	Fsio_EncapStream effectively closes the stream.  By dup'ing the
 *	stream we can later call Vm_SegmentDelete which will close the
 *	stream (again).
 *
 * Results:
 *      If an error occurred writing the swap file, VM_SWAP_ERROR is
 *	returned, else SUCCESS.  The new pointer into the buffer
 *	is returned in *bufPtrPtr.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

static ReturnStatus
EncapSegment(segPtr, procPtr, bufPtrPtr)
    Vm_Segment	*segPtr;	/* Pointer to the segment to be migrated */
    Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    Address	*bufPtrPtr;	/* pointer to pointer into buffer */
{
    register Address ptr;
    int varSize;
    ReturnStatus status;
    Fs_Stream *dummyStreamPtr;

    if (segPtr->type != VM_CODE) {
	varSize = segPtr->ptSize * sizeof(Vm_PTE);
    } else {
	varSize = sizeof(Vm_ExecInfo);
    }

    ptr = *bufPtrPtr;
    bcopy((Address) &segPtr->offset, ptr, NUM_FIELDS * sizeof(int));
    ptr += NUM_FIELDS * sizeof(int);
    if (segPtr->type != VM_CODE) {
	bcopy((Address) segPtr->ptPtr, ptr, varSize);
	ptr += varSize;
	Fsio_StreamCopy(segPtr->swapFilePtr, &dummyStreamPtr);
	status = Fsio_EncapStream(segPtr->swapFilePtr, ptr);
    } else {
	bcopy((Address) &segPtr->execInfo, ptr, varSize);
	ptr += varSize;
	Fsio_StreamCopy(segPtr->filePtr, &dummyStreamPtr);
	status = Fsio_EncapStream(segPtr->filePtr, ptr);
    }
    if (status != SUCCESS) {
	return(status);
    }
    *bufPtrPtr = ptr + Fs_GetEncapSize();

    if (proc_MigDebugLevel > 4) {
	printf("Deleting segment %d from encapsulation routine.\n",
	       segPtr->type);
    }
    Proc_Unlock(procPtr);
    VmMach_HandleSegMigration(segPtr);
    Vm_SegmentDelete(segPtr, procPtr);
    Proc_Lock(procPtr);
    if (proc_MigDebugLevel > 4) {
	printf("Deleted segment.\n");
    }

    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * LoadSegment --
 *
 *	Copy the page table for a segment from a buffer area into the
 * 	segment's page table.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The page table is loaded.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
LoadSegment(length, buffer, segPtr)
    int				length;
    register	Address		buffer;
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    bcopy(buffer, (Address) segPtr->ptPtr, length);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * CheckSharers --
 *
 *	Verify that a process is not using shared memory.  
 *
 * Results:
 *	SUCCESS if not, or GEN_PERMISSION_DENIED if so.
 *	Once we can handle shared memory processes, it will return
 *	SUCCESS and rely on the special flag in the encapInfo structure
 *	to fix things up.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY static ReturnStatus
CheckSharers(segPtr, infoPtr)
    register	Vm_Segment	*segPtr;
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
    
{
    LOCK_MONITOR;

    if (segPtr->refCount > 1) {
	infoPtr->special = 1;
	if (proc_MigDebugLevel > 0) {
	    printf("Vm_InitiateMigration: can't migrate process sharing heap.\n");
	}
	UNLOCK_MONITOR;
	return(FAILURE);
    }
    UNLOCK_MONITOR;
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PrepareSegment --
 *
 *	Set up a segment to be migrated.  Lock its dirty pages and free
 *	the rest.  
 *
 * Results:
 *     	The number of pages flushed is returned.
 *
 * Side effects:
 *     	All modified pages allocated to the segment are locked; other
 *	pages are freed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
PrepareSegment(segPtr)
    Vm_Segment	*segPtr;	/* Pointer to the segment to be flushed */
{
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    Boolean		referenced;
    Boolean		modified;
    register int	i;

    LOCK_MONITOR;

    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *) NIL;

    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
	ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    } else {
	virtAddr.page = segPtr->offset;
	ptePtr = segPtr->ptPtr;
    }

    /*
     * Free all clean pages that this segment has in real memory.
     * Lock the dirty ones.
     */

    for (i = 0; 
	 i < segPtr->numPages; 
	 i++, virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	/*
	 * If the page is not resident in memory then go to the next page.
	 */
	if (!(*ptePtr & VM_PHYS_RES_BIT)) {
	    continue;
	}
	/*
	 * The page is resident so lock it if it needs to be written, or else
	 * free the page frame.
	 */
	VmMach_GetRefModBits(&virtAddr, Vm_GetPageFrame(*ptePtr), &referenced,
			     &modified);
	if ((*ptePtr & VM_MODIFIED_BIT) || modified) {
	    VmLockPageInt(Vm_GetPageFrame(*ptePtr));
	} else {
	    VmPageFreeInt(Vm_GetPageFrame(*ptePtr));
	    *ptePtr &= ~(VM_PHYS_RES_BIT | VM_PAGE_FRAME_FIELD);
	    segPtr->resPages--;
	}
    }

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FlushSegment --
 *
 *     	Flush the dirty pages of a segment to disk.  This part is done
 *	without the monitor lock because it calls monitored procedures.
 *
 * Results:
 *     	If the swap file cannot be opened, the error is propagated.  Otherwise,
 *	SUCCESS is returned.
 *
 * Side effects:
 *     All modified pages allocated to the segment are forced to disk.
 *
 * ----------------------------------------------------------------------------
 */
static ReturnStatus
FlushSegment(segPtr)
    Vm_Segment 	*segPtr;	/* Pointer to the segment to be flushed */
{
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    int    		i;
    ReturnStatus	status;
#ifndef CLEAN
    int			pagesWritten = 0;
#endif /* CLEAN */

    /*
     * Open the swap file unconditionally.
     */
    
    VmSwapFileLock(segPtr);
    if (!(segPtr->flags & VM_SWAP_FILE_OPENED)) {
	status = VmOpenSwapFile(segPtr);
	if (status != SUCCESS) {
	    VmSwapFileUnlock(segPtr);
	    return(status);
	}
    }
    VmSwapFileUnlock(segPtr);

    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *) NIL;

    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
	ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    } else {
	virtAddr.page = segPtr->offset;
	ptePtr = segPtr->ptPtr;
    }

    /*
     * Go through the page table and cause all modified pages to be
     * written to disk.  During encapsulation time, we'll start at the
     * top and free each page.  This has the side-effect of waiting
     * for dirty pages to go to disk.  Note that by doing this
     * two-pass write, multiple pages may be written to disk at once
     * since the writes are asynchronous.
     */
    
    for (i = 0; 
	 i < segPtr->numPages; 
	 i++, virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	/*
	 * If the page is not resident in memory then go to the next page.
	 */
	if (!(*ptePtr & VM_PHYS_RES_BIT)) {
	    continue;
	}
	/*
	 * The page is dirty so put it on the dirty list.  Wait later on
	 * for it to be written out.
	 */
#ifndef CLEAN
	pagesWritten++;
#endif /* CLEAN */
	
	VmPutOnDirtyList(Vm_GetPageFrame(*ptePtr));
    }
#ifndef CLEAN
	if (proc_MigDoStats) {
	    Proc_MigAddToCounter(pagesWritten,
				 &proc_MigStats.varStats.pagesWritten,
				 &proc_MigStats.squared.pagesWritten);
	}
#endif /* CLEAN */
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * FreePages --
 *
 *	Free the pages of a segment (waiting for them to be written first).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pages are freed..
 *
 *----------------------------------------------------------------------
 */

static void
FreePages(segPtr)
    Vm_Segment *segPtr;		/* segment whose pages should be freed */
{
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    int    		i;

    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *) NIL;

    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
	ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    } else {
	virtAddr.page = segPtr->offset;
	ptePtr = segPtr->ptPtr;
    }

    for (i = 0; 
	 i < segPtr->numPages; 
	 i++, virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	/*
	 * If the page is not resident in memory then go to the next page.
	 */
	if (!(*ptePtr & VM_PHYS_RES_BIT)) {
	    continue;
	}
	VmPageFree(Vm_GetPageFrame(*ptePtr));
	segPtr->resPages--;
	*ptePtr = VM_VIRT_RES_BIT | VM_ON_SWAP_BIT;
    }
}
