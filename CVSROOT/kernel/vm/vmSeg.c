/* vmSeg.c -
 *
 *	This file contains routines that manage the segment table.   It
 *	has routines to allocate, free, expand, and copy segments.  The 
 *	segment table structure and the lists that run through the segment
 *	table are described in vmInt.h.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <vm.h>
#include <vmInt.h>
#include <lock.h>
#include <sync.h>
#include <sys.h>
#include <list.h>
#include <stdlib.h>
#include <fs.h>
#include <status.h>
#include <string.h>
#include <stdio.h>
#include <bstring.h>
#include <assert.h>
#include <machparam.h>

Boolean	vm_NoStickySegments = FALSE;		/* TRUE if sticky segments
						 * are disabled. */
Vm_Segment		*vm_SysSegPtr;		/* The system segment. */

static	Vm_Segment      *segmentTable;		/* The table of segments. */

Vm_SharedSegTable	sharedSegTable;		/* Table of shared segs. */

/*
 * Free, inactive and dead segment lists.
 */
static	List_Links      freeSegListHdr;	
static	List_Links      inactiveSegListHdr;
static	List_Links      deadSegListHdr;
#define	freeSegList	(&freeSegListHdr)
#define	inactiveSegList	(&inactiveSegListHdr)
#define	deadSegList	(&deadSegListHdr)

/*
 * Condition to wait on when waiting for a code segment to be set up.
 */
Sync_Condition	codeSegCondition;

extern	Vm_Segment  **Fs_RetSegPtr();
static void DeleteSeg _ARGS_((register Vm_Segment *segPtr));
static void CleanSegment _ARGS_((register Vm_Segment *segPtr));
static 	void	    FillSegmentInfo();
static ReturnStatus AddToSeg _ARGS_((register Vm_Segment *segPtr,
	int firstPage, int lastPage, int newNumPages, VmSpace newSpace,
	VmSpace *oldSpacePtr));
void		    Fsio_StreamCopy();

#ifdef sequent
int	vmNumSegments = 512;
#else /* sequent */
int	vmNumSegments = 256;
#endif /* sequent */


/*
 * ----------------------------------------------------------------------------
 *
 * VmSegTableAlloc --
 *
 *     Allocate the segment table.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Segment table allocated.
 *     
 * ----------------------------------------------------------------------------
 */
void
VmSegTableAlloc()
{
    if (vmMaxMachSegs > 0) {
	if (vmMaxMachSegs < vmNumSegments) {
	    vmNumSegments = vmMaxMachSegs;
	}
    }

    segmentTable = 
	    (Vm_Segment *) Vm_BootAlloc(sizeof(Vm_Segment) * vmNumSegments);
    bzero((Address)segmentTable, vmNumSegments * sizeof(Vm_Segment));

    vm_SysSegPtr = &(segmentTable[VM_SYSTEM_SEGMENT]);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmSegTableInit --
 *
 *     Initialize the segment table.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Segment table initialized plus all segment lists.
 *     
 * ----------------------------------------------------------------------------
 */
void
VmSegTableInit()
{
    int		i;
    Vm_Segment	*segPtr;

    /*
     * Initialize the free, inactive and dead segment lists.
     */
    List_Init(freeSegList);
    List_Init(inactiveSegList);
    List_Init(deadSegList);

    List_Init((List_Links *)&sharedSegTable);

    /*
     * Initialize the segment table.  The kernel gets the system segment and
     * the rest of the segments go onto the segment free list.
     */
    vm_SysSegPtr->refCount = 1;
    vm_SysSegPtr->type = VM_SYSTEM;
    vm_SysSegPtr->offset = (unsigned int)mach_KernStart >> vmPageShift;
    vm_SysSegPtr->flags = 0;
    vm_SysSegPtr->numPages = vmFirstFreePage;
    vm_SysSegPtr->resPages = vmFirstFreePage;
    for (i = 0, segPtr = segmentTable; i < vmNumSegments; i++, segPtr++) {
	segPtr->filePtr = (Fs_Stream *)NIL;
	segPtr->swapFilePtr = (Fs_Stream *)NIL;
	segPtr->segNum = i;
	segPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	segPtr->procList = (List_Links *) &(segPtr->procListHdr);
	List_Init(segPtr->procList);
	if (i != VM_SYSTEM_SEGMENT) {
	    segPtr->ptPtr = (Vm_PTE *)NIL;
	    segPtr->machPtr = (VmMach_SegData *)NIL;
	    segPtr->flags = VM_SEG_FREE;
	    List_Insert((List_Links *) segPtr, LIST_ATREAR(freeSegList));
	}
    }
}

static Vm_Segment *FindCode _ARGS_((Fs_Stream *filePtr, VmProcLink *procLinkPtr, Boolean *usedFilePtr));


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_FindCode --
 *
 *     	Search the segment table for a code segment that has a matching 
 *	filePtr.  Call internal routine to do the work.
 *
 * Results:
 *     	A pointer to the matching segment if one is found, NIL if none found. 
 *
 * Side effects:
 *     	Memory allocated, *execInfoPtrPtr may be set to point to exec info, and
 *	*userFilePtr is set to TRUE or FALSE depending on whether the filePtr
 *	is used.  If the segment couldn't be found, the file is marked 
 *	to show that we're in the process of setting a segment up.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Vm_Segment *
Vm_FindCode(filePtr, procPtr, execInfoPtrPtr, usedFilePtr)
    Fs_Stream		*filePtr;	/* Stream for the object file. */
    Proc_ControlBlock	*procPtr;	/* Process for which segment is being
					 * allocated. */
    Vm_ExecInfo		**execInfoPtrPtr;/* Where to return relevant info from
					 * the a.out header. */
    Boolean		*usedFilePtr;	/* TRUE => Had to use the file pointer.
					 * FALSE => didn't have to use it. */
{
    register	Vm_Segment	*segPtr;
    register	VmProcLink	*procLinkPtr;

    procLinkPtr = (VmProcLink *) malloc(sizeof(VmProcLink));
    procLinkPtr->procPtr = procPtr;
    segPtr = FindCode(filePtr, procLinkPtr, usedFilePtr);
    if (segPtr == (Vm_Segment *) NIL) {
	free((Address) procLinkPtr);
    } else {
	*execInfoPtrPtr = &segPtr->execInfo;
    }

    return(segPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FindCode --
 *
 *     	Search the segment table for a code segment that has a matching 
 *	filePtr.  If one can be found, then increment the reference count
 *	and return a pointer to the segment.  If one can't be found then
 *	mark the file so that subsequent calls to this routine will wait
 *	until this code segment is initialized.
 *
 * Results:
 *     	A pointer to the matching segment if one is found, NIL if none found. 
 *
 * Side effects:
 *     	If a matching segment is found its reference count is incremented.
 *	*usedFilePtr is set to TRUE or FALSE depending on whether the filePtr
 *	needs to be used for the code segment or not.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static Vm_Segment *
FindCode(filePtr, procLinkPtr, usedFilePtr)
    Fs_Stream		*filePtr;	/* The unique identifier for this file
					   (if any) */
    VmProcLink		*procLinkPtr;	/* Used to put calling process into 
					 * list of processes using this
					 * segment. */
    Boolean		*usedFilePtr;	/* TRUE => Had to use the file pointer.
					 * FALSE => didn't have to use it. */
{
    register	Vm_Segment	**segPtrPtr;
    register	Vm_Segment	*segPtr;
    ClientData			fileHandle;

    LOCK_MONITOR;

    *usedFilePtr = FALSE;
again:
    fileHandle = Fs_GetFileHandle(filePtr);
    assert(((unsigned int) fileHandle & WORD_ALIGN_MASK) == 0);
    segPtrPtr = Fs_GetSegPtr(fileHandle);
    if (vm_NoStickySegments || *segPtrPtr == (Vm_Segment *) NIL) {
	/*
	 * There is no segment associated with this file.  Set the value to
	 * 0 so that we will know that we are about to set up this 
	 * association.
	 * XXX - if vm_NoStickySegments is TRUE, then we don't check
	 * whether there is really a segment associated with the file, 
	 * so code segments will apparently never be shared.  Is this 
	 * really what we want?  Can it cause a code segment leak?
	 */
	*segPtrPtr = (Vm_Segment *) 0;
	segPtr = (Vm_Segment *) NIL;
    } else if (*segPtrPtr == (Vm_Segment *) 0) {
	/*
	 * Someone is already trying to allocate this segment.  Wait for
	 * them to finish.
	 */
	(void)Sync_Wait(&codeSegCondition, FALSE);
	goto again;
    } else {
	segPtr = *segPtrPtr;
	if (segPtr->flags & VM_SEG_INACTIVE) {
	    if (segPtr->fileHandle != fileHandle) {
		panic("FindCode: segFileData != fileHandle\n");
	    }
	    /*
	     * The segment is inactive, so delete it from the inactive
	     * list.
	     */
	    List_Remove((List_Links *) segPtr);
	    segPtr->flags &= ~VM_SEG_INACTIVE;
	    segPtr->filePtr = filePtr;
	    *usedFilePtr = TRUE;
	}
	(segPtr->refCount)++;
	/*
	 * Put the process into list of processes sharing this segment.
	 */
	List_Insert((List_Links *) procLinkPtr, 
		    LIST_ATFRONT(segPtr->procList));
    }

    UNLOCK_MONITOR;

    return(segPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_InitCode --
 *
 *     	Set the association between the file pointer and the segment.  Also
 *	set the exec information info for the segment.  If the segment is
 *	NIL then any processes waiting for the file handle are awakened and
 *	state is cleaned up.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	Exec info filled in and file pointers segment pointer filled in.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Vm_InitCode(filePtr, segPtr, execInfoPtr)
    Fs_Stream		*filePtr;	/* File for code segment. */
    register Vm_Segment	*segPtr;	/* Segment that is being initialized. */
    Vm_ExecInfo		*execInfoPtr;	/* Information needed to exec this 
					 * object file. */
{
    register	Vm_Segment	**segPtrPtr;
    char			*fileNamePtr;
    int				length;
    ClientData			fileHandle;

    LOCK_MONITOR;

    fileHandle = Fs_GetFileHandle(filePtr);
    assert(((unsigned int) fileHandle & WORD_ALIGN_MASK) == 0);
    segPtrPtr = Fs_GetSegPtr(fileHandle);
    if (*segPtrPtr != (Vm_Segment *) 0) {
	printf("Warning: Vm_InitCode: Seg ptr = %x\n", *segPtrPtr);
    }
    *segPtrPtr = segPtr;
    if (segPtr == (Vm_Segment *) NIL) {
	/*
	 * The caller doesn't want to set up any association between the file
	 * and the segment.  In this case cleanup state, i.e., notify any 
	 * other processes that might be waiting for the caller (our 
	 * process) to finish the association.
	 * XXX - Notice that this is the only place where there is a 
	 * wakeup on codeSegCondition.  If the caller completes the 
	 * association and there are processes waiting, these 
	 * processes won't get notified until some other process kills 
	 * a partial association.
	 */
	Sync_Broadcast(&codeSegCondition);
    } else {
	extern	char	*Fsutil_GetFileName();
	
	segPtr->execInfo = *execInfoPtr;
	segPtr->fileHandle = Fs_GetFileHandle(filePtr);
	fileNamePtr = Fsutil_GetFileName(filePtr);
	if (fileNamePtr != (char *)NIL) {
	    length = strlen(fileNamePtr);
	    if (length >= VM_OBJ_FILE_NAME_LENGTH) {
		length = VM_OBJ_FILE_NAME_LENGTH - 1;
	    }
	    (void)strncpy(segPtr->objFileName, fileNamePtr, length);
	    segPtr->objFileName[length] = '\0';
	} else {
	    segPtr->objFileName[0] = '\0';
	}
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_FileChanged --
 *
 *	This routine is called by the file system when it detects that a
 *	file has been opened for writing.  If the file corresponds to
 *	an unused sticky code segment, the segment will be marked as
 *	deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Segment entry may be marked as deleted and put onto the
 *	dead segment list.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Vm_FileChanged(segPtrPtr)
    Vm_Segment		**segPtrPtr;
{
    register	Vm_Segment	*segPtr;

    LOCK_MONITOR;

    segPtr = *segPtrPtr;
    if (segPtr != (Vm_Segment *) NIL) {
	if (segPtr->refCount != 0) {
	    panic("Vm_FileChanged: In use code seg modified.\n");
	}
	List_Move((List_Links *) segPtr, LIST_ATREAR(deadSegList));
	*segPtrPtr = (Vm_Segment *) NIL;
	segPtr->fileHandle = (ClientData) NIL;
    }

    UNLOCK_MONITOR;
}

static void GetNewSegment _ARGS_((int type, Fs_Stream *filePtr, int fileAddr, int numPages, int offset, Proc_ControlBlock *procPtr, VmSpace *spacePtr, Vm_Segment **segPtrPtr, Boolean *deletePtr));


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_SegmentNew --
 *
 *      Allocate a new segment from the segment table.
 *
 * Results:
 *      A pointer to the new segment is returned if a free segment
 *      is available.  If no free segments are available, then NIL is returned.
 *
 * Side effects:
 *      Memory is allocated.
 *
 * ----------------------------------------------------------------------------
 */
Vm_Segment *
Vm_SegmentNew(type, filePtr, fileAddr, numPages, offset, procPtr)
    int			type;		/* The type of segment that this is */
    Fs_Stream		*filePtr;	/* The unique identifier for this file
					   (if any) */
    int			fileAddr;	/* The address where the segments image
					   begins in the object file. */
    int			numPages;	/* Initial size of segment (in pages) */
    int			offset;		/* At which page from the beginning of
					   the VAS that this segment begins */
    Proc_ControlBlock	*procPtr;	/* Process for which the segment is
					   being allocated. */

{
    Vm_Segment		*segPtr;
    VmSpace		space;
    Boolean		deleteSeg;

    space.procLinkPtr = (VmProcLink *) malloc(sizeof(VmProcLink));
    if (type == VM_CODE) {
	space.ptPtr = (Vm_PTE *) malloc(sizeof(Vm_PTE) * numPages);
	space.ptSize = numPages;
    } else {
	space.ptSize = ((numPages - 1) / vmPageTableInc + 1) * vmPageTableInc;
	space.ptPtr = (Vm_PTE *) malloc(sizeof(Vm_PTE) * space.ptSize);
    }
    segPtr = (Vm_Segment *)NIL;
    GetNewSegment(type, filePtr, fileAddr, numPages, offset,
		  procPtr, &space, &segPtr, &deleteSeg);
    if (segPtr != (Vm_Segment *)NIL) {
	if (deleteSeg) {
	    /*
	     * We have to recycle a code segment before we can allocate a new
	     * segment.
	     */
	    DeleteSeg(segPtr);
	    GetNewSegment(type, filePtr, fileAddr, numPages, offset,
			  procPtr, &space, &segPtr, &deleteSeg);
	}
	VmMach_SegInit(segPtr);
    } else {
	free((Address)space.procLinkPtr);
	free((Address)space.ptPtr);
    }
    return(segPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * GetNewSegment --
 *
 *     	Allocate a new segment from the segment table and return a pointer
 *	to it in *segPtrPtr.  If the new segment corresponds to a dead or
 *	inactive code segment, then *deletePtr will be set to TRUE and the 
 *	caller must cleanup the segment that we returned and call us again 
 *	with *segPtrPtr pointing to the segment that we returned.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	*segPtrPtr is set to point to a segment in the segment table to
 *	use for the new segment.  If no segments are available then *segPtrPtr
 *	is set to NIL.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
GetNewSegment(type, filePtr, fileAddr, numPages, offset, procPtr,
	      spacePtr, segPtrPtr, deletePtr)
    int			type;		/* The type of segment that this is */
    Fs_Stream		*filePtr;	/* Object file stream. */
    int			fileAddr;	/* The address where the segments image
					 * begins in the object file. */
    int			numPages;	/* The number of pages that this segment
					 * initially has. */
    int			offset;		/* At which page from the beginning of
					 * the VAS that this segment begins */
    Proc_ControlBlock	*procPtr;	/* Process for which the segment is
					 *  being allocated. */
    VmSpace		*spacePtr;	/* Memory to be used for this segment.*/    Boolean		*deletePtr;	/* TRUE if have to delete a segment*/
    Vm_Segment		**segPtrPtr;	/* IN/OUT parameter: On input if 
					 * non-nil then this segment should
					 * be used.  On output is the segment
					 * to use. */
{
    register	Vm_Segment	*segPtr;
    register	VmProcLink	*procLinkPtr;

    LOCK_MONITOR;

    if (*segPtrPtr == (Vm_Segment *)NIL) {
	if (!List_IsEmpty(deadSegList)) {
	    /*
	     * If there is a dead code segment then use it so that we can free
	     * up things.
	     */
	    segPtr = (Vm_Segment *) List_First(deadSegList);
	    *deletePtr = TRUE;
	} else if (!List_IsEmpty(freeSegList)) {
	    /*
	     * If there is a free segment then use it.
	     */
	    segPtr = (Vm_Segment *) List_First(freeSegList);
	    *deletePtr = FALSE;
	} else if (!List_IsEmpty(inactiveSegList)) {
	    /*
	     * Inactive segment is available so use it.
	     */
	    segPtr = (Vm_Segment *) List_First(inactiveSegList);
	    *deletePtr = TRUE;
	} else {
	    /*
	     * No segments are available so return a NIL pointer in *segPtrPtr. 
	     */
	    *segPtrPtr = (Vm_Segment *)NIL;
	    UNLOCK_MONITOR;
	    return;
	}
	List_Remove((List_Links *) segPtr);
	*segPtrPtr = segPtr;
    } else {
	segPtr = *segPtrPtr;
	*deletePtr = FALSE;
    }

    if (!*deletePtr) {
	/*
	 * Initialize the new segment.
	 */
	segPtr->fileHandle = (ClientData) NIL;
	segPtr->flags = 0;
	segPtr->refCount = 1;
	segPtr->filePtr = filePtr;
	segPtr->fileAddr = fileAddr;
	segPtr->numPages = numPages;
	segPtr->numCORPages = 0;
	segPtr->numCOWPages = 0;
	segPtr->type = type;
	segPtr->offset = offset;
	segPtr->swapFileName = (char *) NIL;
	segPtr->ptPtr = spacePtr->ptPtr;
	segPtr->ptSize = spacePtr->ptSize;
	bzero((Address)segPtr->ptPtr, segPtr->ptSize * sizeof(Vm_PTE));
	/*
	 * If this is a stack segment, the page table grows backwards.  
	 * Therefore all of the extra page table that we allocated must be
	 * taken off of the offset where the stack was supposed to begin.
	 */
	if (segPtr->type == VM_STACK) {
	    segPtr->offset = mach_LastUserStackPage - segPtr->ptSize + 1;
	}
	/*
	 * Put the process into the list of processes sharing this segment.
	 */
	procLinkPtr = spacePtr->procLinkPtr;
	procLinkPtr->procPtr = procPtr;
	List_Insert((List_Links *) procLinkPtr, LIST_ATFRONT(segPtr->procList));
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmSegmentDeleteInt --
 *
 *      This routine will decrement the reference count for the given segment
 *	and return a status to the caller to tell them what action to 
 *	take.
 *
 * Results:
 *	VM_DELETE_SEG -		The segment should be deleted.
 *	VM_CLOSE_OBJ_FILE -	Don't delete the segment, but close the file
 *				containing the code for the segment.
 *	VM_DELETE_NOTHING -	Don't do anything.
 *
 * Side effects:
 *      The segment table for the given segment is modified, the list of 
 *	processes sharing this segment is modified and the inactive list
 *	may be modified.  *objStreamPtrPtr is set to point to the object
 *	file to close if the status is VM_CLOSE_OBJ_FILE.  *procLinkPtrPtr
 *	is set to point to the proc link info struct to free.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY VmDeleteStatus
VmSegmentDeleteInt(segPtr, procPtr, procLinkPtrPtr, objStreamPtrPtr, migFlag)
    register Vm_Segment 	*segPtr;	/* Pointer to segment to 
						   delete. */
    register Proc_ControlBlock	*procPtr;	/* Process that was using
						   this segment. */
    VmProcLink			**procLinkPtrPtr;/* Pointer to proc link info
						  * to free. */
    Fs_Stream			**objStreamPtrPtr;/* Pointer to object file
						   * stream to close. */
    Boolean			migFlag;	/* TRUE if segment is being
						   migrated. */
{
    VmProcLink		*procLinkPtr;

    LOCK_MONITOR;

    segPtr->refCount--;
	
    /*
     * If the segment is not being migrated, then procPtr refers to a process
     * in the list of processes sharing the segment.  Remove this process from
     * the list of processes and make sure that the space is freed.
     */
    if (!migFlag) {
	procLinkPtr = (VmProcLink *)
	        List_First((List_Links *) segPtr->procList);
	while (procPtr != procLinkPtr->procPtr) {
	    if (List_IsAtEnd(segPtr->procList, (List_Links *) procLinkPtr)) {
		dprintf("Warning: segment %x not on shared seg. list\n",
		    (int)segPtr);
		dprintf("Want: %x (%x)\nHave:\n",(int)procLinkPtr->procPtr,
			(int)procLinkPtr->procPtr->processID);
		procLinkPtr = (VmProcLink *)
			List_First((List_Links *) segPtr->procList);
		do {
		    dprintf(" %x (%x)\n",(int)(procLinkPtr->procPtr),
			    (int)(procLinkPtr->procPtr->processID));
		    procLinkPtr = (VmProcLink *) List_Next((List_Links *) procLinkPtr);
		} while (!List_IsAtEnd(segPtr->procList, (List_Links *) procLinkPtr));

		panic("%s%s",
	                "VmSegmentDeleteInt: Could not find segment on shared",
			"segment list.\n");
	    }
	    procLinkPtr = (VmProcLink *) List_Next((List_Links *) procLinkPtr);
	}
	List_Remove((List_Links *) procLinkPtr);
	*procLinkPtrPtr = procLinkPtr;
    } else {
	*procLinkPtrPtr = (VmProcLink *)NIL;
    }

    if (segPtr->refCount > 0) {
	/*
	 * The segment is still being used so there is nothing to do.
	 */
	UNLOCK_MONITOR;
	return(VM_DELETE_NOTHING);
    }

    while (segPtr->ptUserCount > 0 ) {
	dprintf("VmSegmentDeleteInt: ptUserCount = %d\n",segPtr->ptUserCount);
	/*
	 * Wait until all users of the page tables of this segment are gone.
	 * The only remaining users of a deleted segment would be 
	 * prefetch processes.
	 */
	(void)Sync_Wait(&segPtr->condition, FALSE);
	dprintf("VmSegmentDeleteInt: done waiting\n");
    }
    if (segPtr->type == VM_SHARED) {
	Vm_SharedSegTable *sharedSeg;
	int found;
	found = 0;
	dprintf("Removing sharedSegTable entry\n");
	LIST_FORALL((List_Links *)&sharedSegTable,(List_Links *)sharedSeg) {
	    if (sharedSeg->segPtr == segPtr) {
		List_Remove((List_Links *)sharedSeg);
		found = 1;
		break;
	    }
	}
	if (!found) {
	    dprintf("Danger! shared segment not found on list!\n");
	} else {
	    dprintf("VmSegmentDeleteInt: shared segment removed\n");
	}
    }
    if (!vm_NoStickySegments && segPtr->type == VM_CODE &&
        !(segPtr->flags & (VM_DEBUGGED_SEG | VM_SEG_IO_ERROR))) {
	/* 
	 * Put onto the inactive list and tell our caller to close the
	 * object file.
	 */
	segPtr->flags |= VM_SEG_INACTIVE;
	*objStreamPtrPtr = segPtr->filePtr;
	segPtr->filePtr = (Fs_Stream *) NIL;
	List_Insert((List_Links *) segPtr, LIST_ATREAR(inactiveSegList));
	UNLOCK_MONITOR;
	return(VM_CLOSE_OBJ_FILE);
    } else {
	/*
	 * Otherwise tell our caller to delete us.
	 */
	UNLOCK_MONITOR;
	return(VM_DELETE_SEG);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPutOnFreeSegList --
 *
 *     	Put the given segment onto the end of the segment free list.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Segment put onto end of segment free list.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmPutOnFreeSegList(segPtr)
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    segPtr->flags = VM_SEG_FREE;
    List_Insert((List_Links *) segPtr, LIST_ATREAR(freeSegList));

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_SegmentDelete --
 *
 *     	This routine will delete the given segment by calling a monitored
 *	routine to do most of the work.  Since the calls to the memory 
 *	allocator must be done at non-monitor level, if the resources for
 *	this segment must be released then the calls to the machine dependent
 *	routine that uses the memory allocator are done here at non-monitored 
 *	level.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_SegmentDelete(segPtr, procPtr)
    register	Vm_Segment	*segPtr;
    Proc_ControlBlock		*procPtr;
{
    VmDeleteStatus	status;
    VmProcLink		*procLinkPtr;
    Fs_Stream		*objStreamPtr;

    status = VmSegmentDeleteInt(segPtr, procPtr, &procLinkPtr, &objStreamPtr,
				FALSE);
    if (status == VM_DELETE_SEG) {
	DeleteSeg(segPtr);
	VmPutOnFreeSegList(segPtr);
    } else if (status == VM_CLOSE_OBJ_FILE) {
	(void)Fs_Close(objStreamPtr);
    }

    free((Address)procLinkPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DeleteSeg --
 *
 *	Actually delete a segment.  This includes freeing all memory 
 *	resources for the segment and calling machine dependent cleanup.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	Allocated resources freed in the give segment and the pointers
 *	in the segment are set to NIL.
 *
 * ----------------------------------------------------------------------------
 */
static void
DeleteSeg(segPtr)
    register	Vm_Segment	*segPtr;
{
    if (vm_CanCOW) {
	VmCOWDeleteFromSeg(segPtr, -1, -1);
    }
    CleanSegment(segPtr);
    VmMach_SegDelete(segPtr);

    free((Address)segPtr->ptPtr);
    segPtr->ptPtr = (Vm_PTE *)NIL;
    if (segPtr->filePtr != (Fs_Stream *)NIL) {
	(void)Fs_Close(segPtr->filePtr);
	segPtr->filePtr = (Fs_Stream *)NIL;
    }
    if (segPtr->flags & VM_SWAP_FILE_OPENED) {
	VmSwapFileRemove(segPtr->swapFilePtr, segPtr->swapFileName);
	segPtr->swapFilePtr = (Fs_Stream *)NIL;
	segPtr->swapFileName = (char *)NIL;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * CleanSegment --
 *
 *	Do monitor level state cleanup for a deleted segment.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	All pages allocated to the segment are freed.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY static void
CleanSegment(segPtr)
    register Vm_Segment *segPtr;	/* Pointer to the segment to be 
					 * cleaned */
{
    register	Vm_PTE	*ptePtr;
    register	int    	i;
    Vm_VirtAddr		virtAddr;
    Vm_Segment		**segPtrPtr;

    LOCK_MONITOR;

    segPtr->flags |= VM_SEG_DEAD;

    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
    } else {
	virtAddr.page = segPtr->offset;
    }
    ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);

    if (segPtr->fileHandle != (ClientData) NIL) {
	/*
	 * This segment is associated with a file.  Find out which one and
	 * break the connection.
	 */
	segPtrPtr = Fs_GetSegPtr(segPtr->fileHandle);
	*segPtrPtr = (Vm_Segment *) NIL;
	segPtr->fileHandle = (ClientData) NIL;
    }

    /*
     * Free all pages that this segment has in real memory.
     */
    for (i = segPtr->numPages; 
         i > 0; 
	 i--, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	if (*ptePtr & VM_PHYS_RES_BIT) {
	    VmMach_PageInvalidate(&virtAddr, Vm_GetPageFrame(*ptePtr),
				  TRUE);
	    VmPageFreeInt(Vm_GetPageFrame(*ptePtr));
	}
    }

    segPtr->resPages = 0;

    UNLOCK_MONITOR;
}

static Boolean StartDelete _ARGS_((Vm_Segment *segPtr, int firstPage, int *lastPagePtr));
static ReturnStatus EndDelete _ARGS_((register Vm_Segment *segPtr, int firstPage, int lastPage));


/*
 *----------------------------------------------------------------------
 *
 * Vm_DeleteFromSeg --
 *
 *	Take the range of virtual page numbers for the given heap segment,
 *	invalidate them, make them unaccessible and make the segment
 *	smaller if necessary.
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
Vm_DeleteFromSeg(segPtr, firstPage, lastPage)
    Vm_Segment 	*segPtr;	/* The segment whose pages are being
				   invalidated. */
    int		firstPage;	/* The first page to invalidate */
    int		lastPage;	/* The second page to invalidate. */
{
    /*
     * The deletion of virtual pages from the segment is done in two
     * phases.  First the copy-on-write dependencies are cleaned up and
     * then the rest of the pages are cleaned up.  This requires some
     * synchronization.  The problem is that during and after cleaning up
     * the copy-on-write dependencies, page faults and copy-on-write forks
     * in the segment must be prevented since cleanup is done at non-monitor
     * level.  This is done by using the VM_PT_EXCL_ACC flag.  When this flag 
     * is set page faults and forks are blocked because they are unable
     * to get access to the page tables until the exclusive access flag
     * is cleared.  This flag is set by StartDelete and cleared by EndDelete. 
     * The flag is looked at by VmVirtAddrParse (the routine that is called 
     * before any page fault can occur on the segment) and by IncPTUserCount
     * (the routine that is called when a segment is duplicated for a fork).
     */
    if (!StartDelete(segPtr, firstPage, &lastPage)) {
	return(SUCCESS);
    }
    /*
     * Rid the segment of all copy-on-write dependencies.
     */
    VmCOWDeleteFromSeg(segPtr, firstPage, lastPage);

    return(EndDelete(segPtr, firstPage, lastPage));
}


/*
 *----------------------------------------------------------------------
 *
 * StartDelete --
 *
 *	Set things up to delete pages from a segment.  This involves grabbing
 *	exclusive access to the page tables for the segment.
 *
 * Results:
 *	FALSE if there is nothing to delete from this segment.
 *
 * Side effects:
 *	Page table user count incremented and exclusive access grabbed on
 *	the page tables.
 *
 *----------------------------------------------------------------------
 */
ENTRY static Boolean
StartDelete(segPtr, firstPage, lastPagePtr)
    Vm_Segment	*segPtr;
    int		firstPage;
    int		*lastPagePtr;
{
    Boolean	retVal;
    int		lastSegPage;

    LOCK_MONITOR;

    while (segPtr->ptUserCount > 0) {
	(void) Sync_Wait(&segPtr->condition, FALSE);
    }

    /*
     * If the beginning address falls past the end of the heap segment
     * then there is nothing to do so return.  If the ending address 
     * falls past the end of the heap segment then it must be rounded
     * down and the segment made smaller.
     */
    lastSegPage = segPtr->offset + segPtr->numPages - 1;
    if (firstPage <= lastSegPage) {
	if (*lastPagePtr >= lastSegPage) {
	    *lastPagePtr = lastSegPage;
	}
	/*
	 * Make sure that no one expands or shrinks the segment while 
	 * we are expanding it.
	 */
	segPtr->ptUserCount = 1;
	segPtr->flags |= VM_PT_EXCL_ACC;
	retVal = TRUE;
    } else {
	retVal = FALSE;
    }
    UNLOCK_MONITOR;
    return(retVal);
}


/*
 *----------------------------------------------------------------------
 *
 * EndDelete --
 *
 *	Clean up after a delete has finished.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table user count decremented, exclusive access on the page tables
 *	is released and the segment size may be shrunk.
 *
 *----------------------------------------------------------------------
 */
ENTRY static ReturnStatus
EndDelete(segPtr, firstPage, lastPage)
    register	Vm_Segment	*segPtr;
    int				firstPage;
    int				lastPage;
{
    register	Vm_PTE	*ptePtr;
    Vm_VirtAddr		virtAddr;
    unsigned	int	pfNum;
    ReturnStatus	status = SUCCESS;

    LOCK_MONITOR;

    if (lastPage == segPtr->offset + segPtr->numPages - 1) {
	segPtr->numPages -= lastPage - firstPage + 1;
    }

    /*
     * Free up any resident pages.
     */
    virtAddr.segPtr = segPtr;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    for (virtAddr.page = firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	if (*ptePtr & VM_PHYS_RES_BIT) {
	    if (VmPagePinned(ptePtr)) {
		status = FAILURE;
		goto exit;
	    }
	    VmMach_PageInvalidate(&virtAddr, Vm_GetPageFrame(*ptePtr), FALSE);
	    segPtr->resPages--;
	    pfNum = Vm_GetPageFrame(*ptePtr);
	    *ptePtr = 0;
	    VmPageFreeInt(pfNum);
	}
	*ptePtr = 0;
    }

exit:
    /*
     * Release exclusive access.
     */
    segPtr->ptUserCount = 0;
    segPtr->flags &= ~VM_PT_EXCL_ACC;
    Sync_Broadcast(&segPtr->condition);

    UNLOCK_MONITOR;
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmDecPTUserCount --
 *
 *     	Decrement the number of users of the page tables for this segment.
 *	If the count goes to zero then wake up anyone waiting on it.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Count of users of page table decremented.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmDecPTUserCount(segPtr)
    register	Vm_Segment		*segPtr;
{
    LOCK_MONITOR;

    segPtr->ptUserCount--;
    if (segPtr->ptUserCount == 0) {
	Sync_Broadcast(&segPtr->condition);
    }

    UNLOCK_MONITOR;
}

static void StartExpansion _ARGS_((Vm_Segment *segPtr));
static void EndExpansion _ARGS_((Vm_Segment *segPtr));
static void AllocMoreSpace _ARGS_((register Vm_Segment *segPtr, int newNumPages, register VmSpace *spacePtr));


/*
 *----------------------------------------------------------------------
 *
 * VmAddToSeg --
 *
 *	Make all pages between firstPage and lastPage be in the segment's 
 *	virtual address space.
 *
 * Results:
 *	An error if for some reason the range of virtual pages cannot be
 *	put into the segment's VAS.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmAddToSeg(segPtr, firstPage, lastPage)
    register Vm_Segment *segPtr;	/* The segment whose VAS is to be
					 * modified. */
    int	    	       	firstPage; 	/* The lowest page to put into the 
					 * VAS. */
    int	    	       	lastPage; 	/* The highest page to put into the 
					 * VAS. */
{
    VmSpace	newSpace;
    VmSpace	oldSpace;
    int		retValue;
    int		newNumPages;

    /*
     * The only segments that can be expanded are the stack, heap, and
     * shared segments.
     */
    if (segPtr->type == VM_CODE || segPtr->type == VM_SYSTEM) {
	return(VM_WRONG_SEG_TYPE);
    }

    StartExpansion(segPtr);

    if (segPtr->type == VM_STACK) {
	newNumPages = mach_LastUserStackPage - firstPage + 1;
	AllocMoreSpace(segPtr, newNumPages, &newSpace);
    } else {
	newNumPages = lastPage - segPtr->offset + 1;
	AllocMoreSpace(segPtr, newNumPages, &newSpace);
    }

    retValue = AddToSeg(segPtr, firstPage, lastPage, newNumPages, 
			newSpace, &oldSpace);
    if (oldSpace.spaceToFree && oldSpace.ptPtr != (Vm_PTE *)NIL) {
	free((Address)oldSpace.ptPtr);
    }
    VmMach_SegExpand(segPtr, firstPage, lastPage);

    EndExpansion(segPtr);

    return(retValue);
}


/*
 *----------------------------------------------------------------------
 *
 * StartExpansion --
 *
 *	Grab exclusive access to the page tables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table user count incremented and VM_PT_EXCL_ACC flag set.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
StartExpansion(segPtr)
    Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    while (segPtr->ptUserCount > 0) {
	(void)Sync_Wait(&segPtr->condition, FALSE);
    }
    segPtr->flags |= VM_PT_EXCL_ACC;
    segPtr->ptUserCount++;

    UNLOCK_MONITOR;
}



/*
 *----------------------------------------------------------------------
 *
 * EndExpansion --
 *
 *	Release the lock on the page tables that prevents expansions and
 *	deletions from the segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table user count decremented and VM_PT_EXCL_ACC flag cleared.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
EndExpansion(segPtr)
    Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    segPtr->flags &= ~VM_PT_EXCL_ACC;
    segPtr->ptUserCount--;
    Sync_Broadcast(&segPtr->condition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * AllocMoreSpace --
 *
 *	Allocate more space for the page tables for this segment that is
 *	growing.  endVirtPage is the highest accessible page if it is
 *	a heap segment and the lowest accessible page if it is a stack
 * 	segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The page table might be expanded.
 *
 *----------------------------------------------------------------------
 */
static void
AllocMoreSpace(segPtr, newNumPages, spacePtr)
    register	Vm_Segment	*segPtr;
    int				newNumPages;
    register	VmSpace		*spacePtr;
{
    /*
     * Find out the new size of the page table.
     */
    spacePtr->ptSize = ((newNumPages - 1)/vmPageTableInc + 1) * vmPageTableInc;
    /*
     * Since page tables never get smaller we can see if the page table
     * is already big enough.
     */
    if (spacePtr->ptSize <= segPtr->ptSize) {
	spacePtr->ptPtr = (Vm_PTE *) NIL;
    } else {
	spacePtr->ptPtr = (Vm_PTE *)malloc(sizeof(Vm_PTE) * spacePtr->ptSize);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * AddToSeg --
 *
 *	Make all pages between firstPage and lastPage be in the segments 
 *	virtual address space.
 *
 * Results:
 *	An error if for some reason the range of virtual pages cannot be
 *	put into the segment's VAS.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY static ReturnStatus 
AddToSeg(segPtr, firstPage, lastPage, newNumPages, newSpace, oldSpacePtr)
    register	Vm_Segment	*segPtr;	/* The segment to add the
						 * virtual pages to. */
    int				firstPage;	/* The lowest page to put
						 * into the VAS. */
    int				lastPage;	/* The highest page to put
						 * into the VAS. */
    int				newNumPages;	/* The new number of pages
						 * that will be in the
						 * segment. */
    VmSpace			newSpace;	/* Pointer to new page table
						 * if the segment has to
						 * be expanded. */
    VmSpace			*oldSpacePtr;	/* Place to return pointer
						 * to space to free. */
{
    int				copySize;
    int				byteOffset;
    register	VmProcLink	*procLinkPtr;
    register	Vm_Segment	*otherSegPtr;

    LOCK_MONITOR;

    *oldSpacePtr = newSpace;
    oldSpacePtr->spaceToFree = TRUE;

    copySize = segPtr->ptSize * sizeof(Vm_PTE);
    if (newNumPages > segPtr->ptSize) {
	if (segPtr->type == VM_HEAP) {
	    /*
	     * Go through all proc table entries for all processes sharing
	     * this segment and make sure that no stack segment is too large.
	     */
	    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
		otherSegPtr = 
			procLinkPtr->procPtr->vmPtr->segPtrArray[VM_STACK];
		if (newSpace.ptSize + segPtr->offset >= otherSegPtr->offset) {
		    UNLOCK_MONITOR;
		    return(VM_SEG_TOO_LARGE);
		}
	    }
	    /*
	     * This isn't a stack segment so just copy the page table 
	     * into the lower part, and zero the rest.
	     */
	    bcopy((Address) segPtr->ptPtr, (Address) newSpace.ptPtr, copySize);
	    bzero((Address) ((int) (newSpace.ptPtr) + copySize),
		    (newSpace.ptSize - segPtr->ptSize)  * sizeof(Vm_PTE));
	} else if (segPtr->type == VM_STACK) {
	    /*
	     * Make sure that the heap segment isn't too big.  If it is then 
	     * abort.
	     */
	    otherSegPtr = Proc_GetCurrentProc()->vmPtr->segPtrArray[VM_HEAP];
	    if (otherSegPtr->offset + otherSegPtr->ptSize >= 
		mach_LastUserStackPage - newSpace.ptSize + 1) {
		UNLOCK_MONITOR;
		return(VM_SEG_TOO_LARGE);
	    }
	    /*
	     * In this case the current page table has to be copied to the 
	     * high part of the new page table and the lower part has to be 
	     * zeroed.  Also the offset has to be adjusted to compensate for 
	     * making the page table bigger than requested.
	     */
	    byteOffset = (newSpace.ptSize - segPtr->ptSize) * sizeof(Vm_PTE);
	    bcopy((Address) segPtr->ptPtr,
		    (Address) ((int) (newSpace.ptPtr) + byteOffset), copySize);
	    bzero((Address) newSpace.ptPtr, byteOffset);
	    segPtr->offset -= newSpace.ptSize - segPtr->ptSize;
	}
	oldSpacePtr->ptPtr = segPtr->ptPtr;
	segPtr->ptPtr = newSpace.ptPtr;
	segPtr->ptSize = newSpace.ptSize;
    }
    if (newNumPages > segPtr->numPages) {
	segPtr->numPages = newNumPages;
    }
    /* 
     * Make all pages between firstPage and lastPage zero-fill-on-demand
     * members of the segment's virtual address space.
     */
    VmValidatePagesInt(segPtr, firstPage, lastPage, TRUE, FALSE);

    UNLOCK_MONITOR;

    return(SUCCESS);
}

static void IncPTUserCount _ARGS_((register Vm_Segment *segPtr));
static void CopyInfo _ARGS_((register Vm_Segment *srcSegPtr, register Vm_Segment *destSegPtr, register Vm_PTE **srcPTEPtrPtr, register Vm_PTE **destPTEPtrPtr, Vm_VirtAddr *srcVirtAddrPtr, Vm_VirtAddr *destVirtAddrPtr));
ENTRY static Boolean CopyPage _ARGS_((Vm_Segment *srcSegPtr,
	register Vm_PTE *srcPTEPtr, register Vm_PTE *destPTEPtr));


/*
 *----------------------------------------------------------------------
 *
 * Vm_SegmentDup --
 *
 *	Duplicate the given segment and return a pointer to the copy in
 *	*destSegPtrPtr.  If the segment that is being copied is shared by 
 *	other processes then the segment could be being modified while it is 
 *	being copied.  Hence there is no guarantee that the segment will
 *	be in the same state after it is duplicated as it was when this
 *	routine was called.
 *
 * Results:
 *	VM_SWAP_ERROR if swap space could not be duplicated or VM_NO_SEGMENTS
 *	if are out of segments.  Otherwise return SUCCESS.
 *
 * Side effects:
 *	New segment allocated, initialized and copied into.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_SegmentDup(srcSegPtr, procPtr, destSegPtrPtr)
    register Vm_Segment *srcSegPtr;	/* Pointer to the segment to be 
					 * duplicate. */
    Proc_ControlBlock   *procPtr; 	/* Pointer to the process for which the 
					 * segment is being duplicated. */
    Vm_Segment		**destSegPtrPtr;/* Place to return pointer to new
					 * segment. */
{
    register	Vm_Segment	*destSegPtr;
    ReturnStatus		status;
    register	Vm_PTE		*srcPTEPtr;
    register	Vm_PTE		*destPTEPtr;
    Vm_PTE			*tSrcPTEPtr;
    Vm_PTE			*tDestPTEPtr;
    Vm_VirtAddr			srcVirtAddr;
    Vm_VirtAddr			destVirtAddr;
    int				i;
#ifndef sequent    
    Address			srcAddr = (Address) NIL;
    Address			destAddr = (Address)NIL;
#endif    
    Fs_Stream			*newFilePtr;

    if (srcSegPtr->type == VM_HEAP) {
	Fsio_StreamCopy(srcSegPtr->filePtr, &newFilePtr);
    } else {
	newFilePtr = (Fs_Stream *) NIL;
    }
    /*
     * Prevent the source segment from being expanded.
     */
    IncPTUserCount(srcSegPtr);

    /*
     * Allocate the segment that we are copying to.
     */
    destSegPtr = Vm_SegmentNew(srcSegPtr->type, newFilePtr,
			       srcSegPtr->fileAddr, srcSegPtr->numPages, 
			       srcSegPtr->offset, procPtr);
    if (destSegPtr == (Vm_Segment *) NIL) {
	VmDecPTUserCount(srcSegPtr);
	if (srcSegPtr->type == VM_HEAP) {
	    (void)Fs_Close(newFilePtr);
	}
	*destSegPtrPtr = (Vm_Segment *) NIL;
	return(VM_NO_SEGMENTS);
    }
    if (vm_CanCOW) {
	if (VmSegCanCOW(srcSegPtr)) {
	    /*
	     * We are allowing copy-on-write.  Make a copy-on-ref image of the
	     * src segment in the dest segment.
	     */
	    VmSegFork(srcSegPtr, destSegPtr);
	    VmDecPTUserCount(srcSegPtr);
	    *destSegPtrPtr = destSegPtr;

	    VmSegCOWDone(srcSegPtr, FALSE);

	    return(SUCCESS);
	}
    }

    /*
     * No copy-on-write.  Do a full fledged copy of the source segment to
     * the dest segment.
     */
    CopyInfo(srcSegPtr, destSegPtr, &tSrcPTEPtr, &tDestPTEPtr, &srcVirtAddr,
	     &destVirtAddr);

    /*
     * Copy over memory.
     */
    for (i = 0, srcPTEPtr = tSrcPTEPtr, destPTEPtr = tDestPTEPtr; 
	 i < destSegPtr->numPages; 
	 i++, VmIncPTEPtr(srcPTEPtr, 1), VmIncPTEPtr(destPTEPtr, 1), 
		destVirtAddr.page++, srcVirtAddr.page++) {
	if (CopyPage(srcSegPtr, srcPTEPtr, destPTEPtr)) {
	    *destPTEPtr |= VM_REFERENCED_BIT | VM_MODIFIED_BIT |
	                   VmPageAllocate(&destVirtAddr, VM_CAN_BLOCK);
	    destSegPtr->resPages++;
#ifndef sequent
	    if (srcAddr == (Address) NIL) {
		VmMach_FlushPage(&srcVirtAddr, FALSE);
		srcAddr = VmMapPage(Vm_GetPageFrame(*srcPTEPtr));
		destAddr = VmMapPage(Vm_GetPageFrame(*destPTEPtr));
	    } else {
		VmMach_FlushPage(&srcVirtAddr, FALSE);
		VmRemapPage(srcAddr, Vm_GetPageFrame(*srcPTEPtr));
		VmRemapPage(destAddr, Vm_GetPageFrame(*destPTEPtr));
	    }
	    bcopy(srcAddr, destAddr, vm_PageSize);
#else	/* sequent */
	    VmMachCopyPage(Vm_GetPageFrame(*srcPTEPtr),
					Vm_GetPageFrame(*destPTEPtr));
#endif	/* sequent */
	    VmUnlockPage(Vm_GetPageFrame(*srcPTEPtr));
	    VmUnlockPage(Vm_GetPageFrame(*destPTEPtr));
	}
    }
#ifndef sequent    
    /*
     * Unmap any mapped pages.
     */
    if (srcAddr != (Address) NIL) {
        VmUnmapPage(srcAddr);
        VmUnmapPage(destAddr);
    }
#endif
    /*
     * Copy over swap space resources.
     */
    status = VmCopySwapSpace(srcSegPtr, destSegPtr);
    VmDecPTUserCount(srcSegPtr);

    /*
     * If couldn't copy the swap space over then return an error.
     */
    if (status != SUCCESS) {
	Vm_SegmentDelete(destSegPtr, procPtr);
	return(VM_SWAP_ERROR);
    }

    *destSegPtrPtr = destSegPtr;

    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * IncPTUserCount --
 *
 *     	Increment the count of users of the page tables for the given segment.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Count of users of page table incremented.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY static void
IncPTUserCount(segPtr)
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    while (segPtr->flags & VM_PT_EXCL_ACC) {
	(void)Sync_Wait(&segPtr->condition, FALSE);
    }
    segPtr->ptUserCount++;

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * The following routines, VmSegCanCOW, VmSegCantCOW and VmSegCOWDone,
 * synchronize the marking of a segment as non-copy-on-writeable (i.e. it
 * has to be copied at fork time). The routines that wire pages into a
 * user's address space can't allow pages that they have wired down to all
 * of a sudden become copy-on-write because this can cause page faults
 * at bad times (e.g. with interrupts disabled).  Thus before they wire
 * pages down they make sure that the segment that the pages reside in
 * cannot be made copy-on-write.  They do this by calling the routine
 * VmSegCantCOW.  The routine above (Vm_SegmentDup) wants to decide if it
 * should duplicate the segment with COW or with copy-on-fork.  It decides
 * what to do by calling the routine VmSegCanCOW which will return TRUE
 * if the segment can be copied copy-on-write and will prevent the routine
 * VmSegCantCOW from doing its thing.  Once a segment has been successfully
 * duplicated with COW then the routine VmSegCOWDone is called which allows
 * VmSegCantCOW to proceed.
 */

/*
 * ----------------------------------------------------------------------------
 *
 * VmSegCanCOW --
 *
 *     	Return TRUE if can fork this segment copy-on-write.  If the 
 *	VM_SEG_COW_IN_PROGRESS flag is set then wait until its cleared
 *	before making the decision about whether this segment can
 *	be forked copy-on-write. 
 *
 * Results:
 *	TRUE if can fork this segment copy-on-write.
 *
 * Side effects:
 *	VM_SEG_COW_IN_PROGRESS flag set if the can be made copy-on-write.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY Boolean
VmSegCanCOW(segPtr)
    Vm_Segment	*segPtr;
{
    Boolean	retVal;

    LOCK_MONITOR;

    while (segPtr->flags & VM_SEG_COW_IN_PROGRESS) {
	(void)Sync_Wait(&segPtr->condition, FALSE);
    }
    if (segPtr->flags & VM_SEG_CANT_COW) {
	retVal = FALSE;
    } else {
	retVal = TRUE;
	segPtr->flags |= VM_SEG_COW_IN_PROGRESS;
    }

    UNLOCK_MONITOR;

    return(retVal);
}



/*
 * ----------------------------------------------------------------------------
 *
 * VmSegCantCOW --
 *
 *     	Mark this segment such that it can no longer be made copy-on-write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	VM_SEG_CANT_COW flag set.
 *     
 * ----------------------------------------------------------------------------
 */
void
VmSegCantCOW(segPtr)
    Vm_Segment	*segPtr;
{
    if (!VmSegCanCOW(segPtr)) {
	return;
    }
    (void)VmCOWCopySeg(segPtr);
    VmSegCOWDone(segPtr, TRUE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmSegCOWDone --
 *
 *     	A copy-on-fork operation has completed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	VM_SEG_COW_IN_PROGRESS flag cleared.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmSegCOWDone(segPtr, cantCOW)
    Vm_Segment	*segPtr;
    Boolean	cantCOW;
{
    LOCK_MONITOR;

    if (cantCOW) {
	segPtr->flags |= VM_SEG_CANT_COW;
    }
    segPtr->flags &= ~VM_SEG_COW_IN_PROGRESS;
    Sync_Broadcast(&segPtr->condition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * CopyInfo --
 *
 *	Copy over pertinent information in the source including page
 *	tables to the destination segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table and virtual address pointers set for source and destination
 *	segments.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
CopyInfo(srcSegPtr, destSegPtr, srcPTEPtrPtr, destPTEPtrPtr, 
	 srcVirtAddrPtr, destVirtAddrPtr)
    register	Vm_Segment	*srcSegPtr;
    register	Vm_Segment	*destSegPtr;
    register	Vm_PTE		**srcPTEPtrPtr;
    register	Vm_PTE		**destPTEPtrPtr;
    Vm_VirtAddr			*srcVirtAddrPtr;
    Vm_VirtAddr			*destVirtAddrPtr;
{
    LOCK_MONITOR;

    if (srcSegPtr->type == VM_HEAP) {
	*srcPTEPtrPtr = srcSegPtr->ptPtr;
	*destPTEPtrPtr = destSegPtr->ptPtr;
	destVirtAddrPtr->page = srcSegPtr->offset;
    } else {
	destVirtAddrPtr->page = mach_LastUserStackPage - 
						srcSegPtr->numPages + 1;
	*srcPTEPtrPtr = VmGetPTEPtr(srcSegPtr, destVirtAddrPtr->page);
	*destPTEPtrPtr = VmGetPTEPtr(destSegPtr, destVirtAddrPtr->page);
    }
    destVirtAddrPtr->segPtr = destSegPtr;
    srcVirtAddrPtr->segPtr = srcSegPtr;
    srcVirtAddrPtr->page = destVirtAddrPtr->page;
    srcVirtAddrPtr->sharedPtr = (Vm_SegProcList *)NIL;
    destVirtAddrPtr->sharedPtr = (Vm_SegProcList *)NIL;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * CopyPage --
 *
 *	Determine if the page in the source segment needs to be duplicated.
 *	If the page is to be duplicated then return the source page frame
 *	locked and return TRUE.  Otherwise return FALSE.
 *
 * Results:
 *	TRUE if page needs to be duplicated, FALSE if not.
 *
 * Side effects:
 *	Source page frame may be locked.
 *
 *----------------------------------------------------------------------
 */
ENTRY static Boolean
CopyPage(srcSegPtr, srcPTEPtr, destPTEPtr)
    Vm_Segment			*srcSegPtr;
    register	Vm_PTE		*srcPTEPtr;
    register	Vm_PTE		*destPTEPtr;
{
    Boolean	residentPage;

    LOCK_MONITOR;

    while (*srcPTEPtr & VM_IN_PROGRESS_BIT) {
	(void)Sync_Wait(&srcSegPtr->condition, FALSE);
    }
    residentPage = *srcPTEPtr & VM_PHYS_RES_BIT;
    *destPTEPtr = *srcPTEPtr;
    if (residentPage) {
	/*
	 * Copy over all resident pages.  Can reload swapped pages but its a 
	 * lot cheaper to do a memory-to-memory copy than send an RPC to the
	 * server to copy the page on swap space.  
	 */
	VmLockPageInt(Vm_GetPageFrame(*srcPTEPtr));
	*destPTEPtr &= ~(VM_ON_SWAP_BIT | VM_PAGE_FRAME_FIELD);
    } else {
	*destPTEPtr &= ~VM_PAGE_FRAME_FIELD;
	/* 
	 * This page is on the swap file but not in memory so we are
	 * going to have to copy over the swap space for this page.
	 * Use the in-progress bit to mark this fact.
	 */
	if (*destPTEPtr & VM_ON_SWAP_BIT) {
	    *destPTEPtr |= VM_IN_PROGRESS_BIT;
	}
    }

    UNLOCK_MONITOR;

    return(residentPage);
}


/*
 *----------------------------------------------------------------------
 *
 * SegmentIncRef --
 *
 *	Increment the reference count for the given segment and put it into
 *	the list of processes sharing this segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given segment in the segment table is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
SegmentIncRef(segPtr, procLinkPtr) 
    register	Vm_Segment	*segPtr;
    register	VmProcLink	*procLinkPtr;
{
    LOCK_MONITOR;

    segPtr->refCount++;

    /*
     * Put the process into the list of processes sharing this segment.
     */
    List_Insert((List_Links *) procLinkPtr, LIST_ATFRONT(segPtr->procList));

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_SegmentIncRef --
 *
 *	Increment the reference count for the given segment and put it into
 *	the list of processes sharing this segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given segment in the segment table is modified.
 *
 *----------------------------------------------------------------------
 */
void
Vm_SegmentIncRef(segPtr, procPtr) 
    Vm_Segment		*segPtr;
    Proc_ControlBlock	*procPtr;
{
    register	VmProcLink	*procLinkPtr;

    procLinkPtr = (VmProcLink *) malloc(sizeof(VmProcLink));
    procLinkPtr->procPtr = procPtr;

    SegmentIncRef(segPtr, procLinkPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetSegInfo --
 *
 *
 *	This routine takes in a pointer to a proc table entry and returns
 *	the segment table information for the segments that it uses.
 *	If the proc table entry corresponds to a migrated process,
 * 	contact its current host.
 *
 * Results:
 *	SUCCESS if could get the information, SYS_ARG_NOACCESS if the
 *	the pointer to the segment table entries passed in are bad amd
 *	SYS_INVALID_ARG if one of the segment pointers in the proc table
 *	are bad.  Or, the result from the RPC to get the migrated process's
 *	info.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_GetSegInfo(infoPtr, segID, infoSize, segBufPtr)
    Proc_PCBInfo	*infoPtr;	/* User's copy of PCB.  Contains
					 * pointers to segment structures.
					 * USER_NIL => Want to use a 
					 *     specific segment number. */
    Vm_SegmentID	segID;		/* Segment number of get info for.  
					 * Ignored unless previous argument
					 * is USER_NIL. */
    int			infoSize;	/* Size of segment info structures */
    Address		segBufPtr;	/* Where to store segment information.*/
{
    Proc_PCBInfo	pcbInfo;
    Vm_Segment		*minSegAddr, *maxSegAddr, *segPtr;
    int			i;
    int			segNum;
    int			bytesToCopy;
    Vm_SegmentInfo	segmentInfo;
    int			host;
    Proc_ControlBlock	*procPtr;
    ReturnStatus 	status;

    segNum = (int) segID;
    bytesToCopy = min(sizeof(Vm_SegmentInfo), infoSize);
    minSegAddr = segmentTable;
    maxSegAddr = &(segmentTable[vmNumSegments - 1]);
    if (infoPtr != (Proc_PCBInfo *)USER_NIL) {
	if (Vm_CopyIn(sizeof(pcbInfo), (Address) infoPtr,
		      (Address) &pcbInfo) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
	/*
	 * Follow remote processes.  Use the peerHostID as a hint.
	 */
	host = 0;
	if (pcbInfo.peerHostID != (int) NIL) {
	    procPtr = Proc_LockPID(pcbInfo.processID);
	    if (procPtr != (Proc_ControlBlock *) NIL) {
		if (procPtr->state == PROC_MIGRATED) {
		    host = procPtr->peerHostID;
		}
		Proc_Unlock(procPtr);
	    }
	}
	for (i = VM_CODE; i <= VM_STACK; i++, segBufPtr += infoSize) {
	    if (pcbInfo.genFlags & PROC_KERNEL) {
		segPtr = vm_SysSegPtr;
		FillSegmentInfo(segPtr, &segmentInfo);
	    } else {
		segNum = pcbInfo.vmSegments[i];
		if (segNum < 0 || segNum >= vmNumSegments) {
		    return(SYS_INVALID_ARG);
		}
		if (host) {
		    status = Proc_GetRemoteSegInfo(host, segNum, &segmentInfo);
		    if (status != SUCCESS) {
			return(status);
		    }
		} else {
		    segPtr = &segmentTable[segNum];
		    if (segPtr < minSegAddr || segPtr > maxSegAddr) {
			return(SYS_INVALID_ARG);
		    }
		    FillSegmentInfo(segPtr, &segmentInfo);
		}
	    }
	    if (Vm_CopyOut(bytesToCopy, (Address) &segmentInfo, 
			   segBufPtr) != SUCCESS) { 
		return(SYS_ARG_NOACCESS);
	    }
	}
    } else if (segNum < 0 || segNum >= vmNumSegments) {
	return(SYS_INVALID_ARG);
    } else {
	segPtr = &segmentTable[segNum];
	if (segPtr < minSegAddr || segPtr > maxSegAddr) {
	    return(SYS_INVALID_ARG);
	}
	FillSegmentInfo(segPtr, &segmentInfo);
	if (Vm_CopyOut(bytesToCopy, (Address) &segmentInfo, 
		       segBufPtr) != SUCCESS) { 
	    return(SYS_ARG_NOACCESS);
	}
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FillSegmentInfo --
 *
 *	Converts the contents of a Vm_Segment to a Vm_SegmentInfo.
 *	This allows the kernel definition of Vm_Segment to change without
 *	affecting user programs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
FillSegmentInfo(segPtr, infoPtr)
    Vm_Segment		*segPtr;	/* Segment to convert */
    Vm_SegmentInfo	*infoPtr;	/* Conversion result */
{
    infoPtr->segNum = segPtr->segNum;
    infoPtr->refCount = segPtr->refCount;
    infoPtr->type = segPtr->type;
    if (infoPtr->type == VM_CODE) {
	(void)strncpy(infoPtr->objFileName, segPtr->objFileName,
	        VM_OBJ_FILE_NAME_LENGTH);
	infoPtr->objFileName[VM_OBJ_FILE_NAME_LENGTH -1] = '\0';
    } else {
	infoPtr->objFileName[0] = '\0';
    }
    infoPtr->numPages = segPtr->numPages;
    infoPtr->ptSize = segPtr->ptSize;
    infoPtr->resPages = segPtr->resPages;
    infoPtr->flags = segPtr->flags;
    infoPtr->ptUserCount = segPtr->ptUserCount;
    infoPtr->numCOWPages = segPtr->numCOWPages;
    infoPtr->numCORPages = segPtr->numCORPages;
    infoPtr->minAddr = segPtr->minAddr;
    infoPtr->maxAddr = segPtr->maxAddr;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * VmGetSegPtr --
 *
 *	Return a pointer to the given segment.
 *
 * Results:
 *	Pointer to given segment.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Vm_Segment *
VmGetSegPtr(segNum)
    int	segNum;
{
    return(&segmentTable[segNum]);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_EncapSegInfo --
 *
 *	Encapsulate information for a particular segment.  This is used
 * 	to send info to other hosts (for ps, e.g.).
 *
 * Results:
 *	SUCCESS, or invalid arg if the segment doesn't exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_EncapSegInfo(segNum, infoPtr)
    int	segNum;			/* Number of the segment. */
    Vm_SegmentInfo *infoPtr;	/* Pointer to encapsulated data. */
{
    Vm_Segment *segPtr;
    segPtr = &segmentTable[segNum];
    if (segPtr < segmentTable ||
	segPtr > &(segmentTable[vmNumSegments - 1])) {
	return(GEN_INVALID_ARG);
    }
    FillSegmentInfo(segPtr, infoPtr);
    return(SUCCESS);
}

