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
#include "vmMach.h"
#include "vm.h"
#include "vmInt.h"
#include "lock.h"
#include "proc.h"
#include "fs.h"
#include "stdlib.h"
#include "byte.h"

static ReturnStatus 	EncapsulateInfo();
static ReturnStatus	FlushSegment();
static void 		FreeSegment();
ENTRY static void 	PrepareFlush();
ENTRY static void 	LoadSegment();

Boolean vmMigLeakDebug = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Vm_FreezeSegments --
 *
 *	Freeze all segments associated with a process, as well as marking
 *	any other processes sharing memory with this process for migration
 *	as well.
 *
 * Results:
 *	A linked list of processes is returned.  The processes are all
 *	the processes sharing memory, including the process that is passed
 *	into Vm_FreezeSegments.  The length of the list is also returned.
 *	SUCCESS is returned but other ReturnStatuses may be returned at
 *	a later point.
 *
 * Side effects:
 *	The processes are suspended and marked for migration.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
Vm_FreezeSegments(procPtr, nodeID, procListPtr, numProcsPtr)
    Proc_ControlBlock	*procPtr;	/* The process being migrated */
    int			nodeID;	        /* Node to which it migrates */
    List_Links		**procListPtr;	/* Pointer to header of process list */
    int			*numProcsPtr;	/* Number of processes sharing */
{
    Vm_Segment 		*segPtr;
#ifdef WONT_WORK_YET
    register VmProcLink	*procLinkPtr;
    register Proc_ControlBlock	*shareProcPtr;
#endif

    LOCK_MONITOR;

    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];
#ifdef WONT_WORK_YET
    *numProcsPtr = 0;
    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	shareProcPtr = procLinkPtr->procPtr;
	Proc_Lock(shareProcPtr);	/* DEADLOCK ????? */
	Proc_FlagMigration(shareProcPtr, nodeID);
	Proc_Unlock(shareProcPtr);
	*numProcsPtr += 1;
    }
#else
    if (segPtr->refCount > 1) {
	panic("Can't migrate process sharing heap.\n");
    }
    *numProcsPtr = 1;
#endif    
    *procListPtr = segPtr->procList;

    UNLOCK_MONITOR;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MigrateSegment --
 *
 *	Force a segment to disk and package the information for it into a
 *	buffer.  Allocate space for the buffer and return a pointer to it, as
 *	well as the length of the buffer.  If the segment is a code segment,
 *	do not search for dirty pages; otherwise, flush dirty pages to the
 *	swap file.
 *
 * Results:
 *	A pointer to the buffer containing the segment information is
 *	returned.  The size of the buffer is returned, as is the number of
 *	pages written.  Also, a ReturnStatus
 *	indicates SUCCESS or FAILURE.
 *
 * Side effects:
 *	The segment is forced to disk.  Memory is allocated (to be freed
 *	by the proc module after being sent to the remote node).
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_MigrateSegment(segPtr, bufferPtr, bufferSizePtr, numPagesPtr)
    Vm_Segment 	*segPtr;	/* Pointer to segment to migrate */
    Address	*bufferPtr;
    int		*bufferSizePtr;
    int		*numPagesPtr;	/* Number of pages flushed */
{
    ReturnStatus status;

    *numPagesPtr = 0;
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
	PrepareFlush(segPtr, numPagesPtr);
	status = FlushSegment(segPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    status = EncapsulateInfo(segPtr, bufferPtr, bufferSizePtr);
#ifdef CANT_DO_YET
    FreeSegment(segPtr);
#endif
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * EncapsulateInfo --
 *
 *     	Copy the information from a Vm_Segment into a buffer, ready to be
 *	transferred to another node.  We have to duplicate the stream to the
 *	swap or code file for the segment because Fs_EncapStream effectively
 *	closes the stream.  By dup'ing the stream the proc module can
 *	safely call Vm_DeleteSegment which will close the stream (again).
 *
 * Results:
 *      A pointer to the buffer and its length are returned.
 *
 * Side effects:
 *      Memory for the buffer is allocated.
 *
 * ----------------------------------------------------------------------------
 */

#define NUM_FIELDS 5

static ReturnStatus
EncapsulateInfo(segPtr, bufferPtr, bufferSizePtr)
    Vm_Segment	*segPtr;	/* Pointer to the segment to be migrated */
    Address	*bufferPtr;
    int		*bufferSizePtr;
{
    Address buffer;
    Address ptr;
    int bufferSize;
    int varSize;
    ReturnStatus status;
    Fs_Stream *dummyStreamPtr;

    if (segPtr->type != VM_CODE) {
	varSize = segPtr->ptSize * sizeof(Vm_PTE);
    } else {
	varSize = sizeof(Vm_ExecInfo);
    }

    bufferSize = NUM_FIELDS * sizeof(int) + varSize + Fs_GetEncapSize();
    buffer = (Address) malloc(bufferSize);
    ptr = buffer;
    bcopy((Address) &segPtr->offset, ptr, NUM_FIELDS * sizeof(int));
    ptr += NUM_FIELDS * sizeof(int);
    if (segPtr->type != VM_CODE) {
	bcopy((Address) segPtr->ptPtr, ptr, varSize);
	ptr += varSize;
	(void)Fs_StreamCopy(segPtr->swapFilePtr, &dummyStreamPtr);
	status = Fs_EncapStream(segPtr->swapFilePtr, ptr);
    } else {
	bcopy((Address) &segPtr->execInfo, ptr, varSize);
	ptr += varSize;
	(void)Fs_StreamCopy(segPtr->filePtr, &dummyStreamPtr);
	status = Fs_EncapStream(segPtr->filePtr, ptr);
    }
    if (status != SUCCESS) {
	return(status);
    }
    *bufferPtr = buffer;
    *bufferSizePtr = bufferSize;
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_ReceiveSegmentInfo --
 *
 *     	Get the information from a foreign Vm_Segment from a buffer and create
 *	a segment for it on this (the remote) node.  Set up the file pointer
 *	for its swap file, if it is a stack or heap segment.  If it is
 *	a code segment, do a Vm_SegmentFind (just as Proc_Exec does) and
 *	initialize the page tables if necessary, then return.
 *
 * Results:
 *      SUCCESS - the segment was created successfully.
 *	Error codes will be propagated from other routines.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Vm_ReceiveSegmentInfo(procPtr, buffer)
    Proc_ControlBlock	*procPtr;/* Control block for foreign process */
    Address		buffer;	 /* Buffer containing segment information */
{
    int 	offset;
    int 	fileAddr;
    int 	type;
    int 	numPages;
    int 	varSize;
    int 	ptSize;
    Vm_Segment 	*segPtr;
    ReturnStatus status;
    Fs_Stream 	*filePtr;
    int 	fsInfoSize;
    Vm_ExecInfo	*execInfoPtr;
    Vm_ExecInfo	*oldExecInfoPtr;
    Boolean	usedFile;

    fsInfoSize = Fs_GetEncapSize();

    Byte_EmptyBuffer(buffer, int, offset);
    Byte_EmptyBuffer(buffer, int, fileAddr);
    Byte_EmptyBuffer(buffer, int, type);
    Byte_EmptyBuffer(buffer, int, numPages);
    Byte_EmptyBuffer(buffer, int, ptSize);
    switch (type) {
	case VM_CODE:
	    varSize = sizeof(Vm_ExecInfo);
	    oldExecInfoPtr = (Vm_ExecInfo *) buffer;
	    buffer += varSize;
	    status = Fs_DeencapStream(buffer, &filePtr);
	    buffer += fsInfoSize;
	    if (status != SUCCESS) {
		printf("Vm_ReceiveSegmentInfo: Fs_DeencapStream returned status %x.\n",
			   status);
		return(status);
	    }
	    segPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
	    if (segPtr == (Vm_Segment *) NIL) {
		if (vmMigLeakDebug) {
		    printf("Calling Vm_SegmentNew for code.\n");
		}
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
	    return(SUCCESS);
	case VM_HEAP:
	    (void)Fs_StreamCopy(procPtr->vmPtr->segPtrArray[VM_CODE]->filePtr,
			  &filePtr);
	    if (filePtr == (Fs_Stream *) NIL) {
		panic("Vm_ReceiveSegmentInfo: no code file pointer.\n");
	    }
	    break;
	case VM_STACK:
	    filePtr = (Fs_Stream *) NIL;
	    break;
	default:
	    panic("Vm_ReceiveSegmentInfo: unknown segment type.\n");
    }
    if (vmMigLeakDebug) {
	printf("Calling Vm_SegmentNew for type = %d.\n", type);
    }
    segPtr = Vm_SegmentNew(type, filePtr, fileAddr, numPages, offset, procPtr);
    if (segPtr == (Vm_Segment *) NIL) {
	return(VM_NO_SEGMENTS);
    }
    procPtr->vmPtr->segPtrArray[type] = segPtr;
    segPtr->ptSize = ptSize;
    varSize = ptSize * sizeof(Vm_PTE);
    LoadSegment(varSize, buffer, segPtr);
    buffer += varSize;
    status = Fs_DeencapStream(buffer, &segPtr->swapFilePtr);
    if (status != SUCCESS) {
	printf("Vm_ReceiveSegmentInfo: Fs_DeencapStream on swapFile returned status %x.\n",
		   status);
	return(status);
    }
    if (segPtr->swapFileName != (char *) NIL) { 
	free(segPtr->swapFileName);
	segPtr->swapFileName = (char *) NIL;
    }
    segPtr->flags |= VM_SWAP_FILE_OPENED;
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
 * ----------------------------------------------------------------------------
 *
 * FlushSegment --
 *
 *     	Flush the dirty pages of a segment to disk.  The monitor lock is not
 *	needed because all the pages should have been locked beforehand.
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
    Vm_PTE		*firstPTEPtr;
    int			firstPage;
    ReturnStatus	status;

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

    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
	ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    } else {
	virtAddr.page = segPtr->offset;
	ptePtr = segPtr->ptPtr;
    }

    /*
     * Go through the page table and cause all modified pages to be
     * written to disk.  Then start at the top and free each page.  This
     * has the side-effect of waiting for dirty pages to go to disk.
     * Note that by doing this two-pass write, multiple pages may be
     * written to disk at once since the writes are asynchronous.
     */
    
    firstPTEPtr = ptePtr;
    firstPage = virtAddr.page;

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
	VmPutOnDirtyList(Vm_GetPageFrame(*ptePtr));
    }
    ptePtr = firstPTEPtr;
    virtAddr.page = firstPage;
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
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PrepareFlush --
 *
 * 	Lock the dirty pages of a segment, and free the rest.  The
 *	monitor lock is not needed because all the pages should have
 *	been locked beforehand.
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
PrepareFlush(segPtr, numPagesPtr)
    Vm_Segment	*segPtr;	/* Pointer to the segment to be flushed */
    int		*numPagesPtr;	/* Number of pages flushed */
{
    Vm_PTE		*ptePtr;
    Vm_VirtAddr		virtAddr;
    Boolean		referenced;
    Boolean		modified;
    register int	i;

    LOCK_MONITOR;

    virtAddr.segPtr = segPtr;

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
	    *numPagesPtr += 1;
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
 * FreeSegment --
 *
 *     This routine will delete the given migrated segment by calling a
 *     monitored routine to do most of the work.  Since the calls to the
 *     memory allocator must be done at non-monitor level, if the resources
 *     for this segment must be released then the calls to the machine
 *     dependent routine that uses the memory allocator are done here at
 *     non-monitored level.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The segment is deleted.
 *
 * ----------------------------------------------------------------------------
 */
static void
FreeSegment(segPtr)
    register	Vm_Segment      *segPtr;
{
    VmProcLink		*procLinkPtr;
    Fs_Stream		*objStreamPtr;
    VmDeleteStatus	status;

    status = VmSegmentDeleteInt(segPtr, (Proc_ControlBlock *) NIL,
				&procLinkPtr, &objStreamPtr, TRUE);
    if (status == VM_DELETE_SEG) {
	VmMach_SegDelete(segPtr);
	free((Address)segPtr->ptPtr);
	segPtr->ptPtr = (Vm_PTE *)NIL;
	VmPutOnFreeSegList(segPtr);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_MigSegmentDelete --
 *
 *     	Temporary routine to give access to the static FreeSegment routine,
 *     	while segments for migrated processes are freed upon exit rather than
 *	upon initial migration.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The segment is deleted.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_MigSegmentDelete(segPtr)
    Vm_Segment      *segPtr;
{
    if (segPtr != (Vm_Segment *) NIL) {
	FreeSegment(segPtr);
    }
}
