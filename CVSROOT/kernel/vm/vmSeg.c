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

#include "sprite.h"
#include "vm.h"
#include "vmInt.h"
#include "lock.h"
#include "sync.h"
#include "sys.h"
#include "list.h"
#include "mem.h"
#include "byte.h"
#include "machine.h"
#include "fs.h"
#include "status.h"
#include "string.h"

/*
 * TRUE if sticky segments are disabled.
 */
Boolean	vm_NoStickySegments = FALSE;

/*
 * Declaration of variables global to this module.
 */

Vm_Segment		*vm_SysSegPtr;

/*
 * Variables local to this file.
 */

static	Vm_Segment       *segmentTable;		/* The table of segments. */
static	int		numSegments;		/* The number of segments in
						   the segment table. */

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

static	Sync_Condition	codeSegCondition;

extern	Vm_Segment	**Fs_RetSegPtr();

void	VmCleanSegment();


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
    numSegments = 256;
    segmentTable = 
		(Vm_Segment *) Vm_BootAlloc(sizeof(Vm_Segment) * numSegments);
    Byte_Zero(numSegments * sizeof(Vm_Segment), (Address)segmentTable);

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

    /*
     * Initialize the segment table.  The kernel gets the system segment and
     * the rest of the segments go onto the segment free list.  The page 
     * table and the machine dependent data field are initialized by the 
     * machine dependent routines in Vm_Init.
     */
    vm_SysSegPtr->refCount = 1;
    vm_SysSegPtr->type = VM_SYSTEM;
    vm_SysSegPtr->offset = (unsigned int)mach_KernStart >> vmPageShift;
    vm_SysSegPtr->flags = 0;
    vm_SysSegPtr->numPages = vmFirstFreePage;
    vm_SysSegPtr->resPages = vmFirstFreePage;

    for (i = 0, segPtr = segmentTable; i < numSegments; i++, segPtr++) {
	segPtr->filePtr = (Fs_Stream *)NIL;
	segPtr->swapFilePtr = (Fs_Stream *)NIL;
	segPtr->segNum = i;
	segPtr->numCORPages = 0;
	segPtr->numCOWPages = 0;
	segPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	segPtr->procList = (List_Links *) &(segPtr->procListHdr);
	List_Init(segPtr->procList);
	if (i != VM_SYSTEM_SEGMENT) {
	    segPtr->flags = VM_SEG_FREE;
	    segPtr->refCount = 0;
	    segPtr->notExpandCount = 0;
	    List_Insert((List_Links *) segPtr, LIST_ATREAR(freeSegList));
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * CleanSegment --
 *
 *     Clean up the state information for a segment.  This involves freeing 
 *     all allocated pages.  This is called when a segment is deleted.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All pages allocated to the segment are freed.
 *     
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
CleanSegment(segPtr, spacePtr, fileInfoPtr)
    register Vm_Segment *segPtr;	/* Pointer to the segment to be 
					 * cleaned */
    register VmSpace	*spacePtr;	/* Pointer to structure that contains 
				   	 * space that has to be deallocated. */
    register VmFileInfo	*fileInfoPtr;	/* Pointer where to store stream to 
					 * close. */
{
    register	Vm_PTE	*ptePtr;
    register	int    	i;
    Vm_VirtAddr		virtAddr;
    Vm_Segment		**segPtrPtr;

    segPtr->flags |= VM_SEG_DEAD;

    virtAddr.segPtr = segPtr;
    if (segPtr->type == VM_STACK) {
	virtAddr.page = mach_LastUserStackPage - segPtr->numPages + 1;
    } else {
	virtAddr.page = segPtr->offset;
    }
    ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);

    if (segPtr->filePtr != (Fs_Stream *) NIL) {
	/*
	 * Need to close the file descriptor for this segment.  Don't close it 
	 * here because we don't want to do a file system operation with the 
	 * VM monitor lock down.  It will be closed at a higher level.
	 */
	if (!vm_NoStickySegments && segPtr->type == VM_CODE) {
	    Sys_Panic(SYS_FATAL, "CleanSegment: Non-nil file pointer\n");
	}
	fileInfoPtr->objStreamPtr = segPtr->filePtr;
	segPtr->filePtr = (Fs_Stream *) NIL;
    }
    if (segPtr->fileHandle != (ClientData) NIL) {
	/*
	 * This segment is associated with a file.  Find out which one and
	 * break the connection.
	 */
	segPtrPtr = Fs_RetSegPtr(segPtr->fileHandle);
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
	    VmMach_PageInvalidate(&virtAddr, VmGetPageFrame(*ptePtr),
				  TRUE);
	    VmPageFreeInt(VmGetPageFrame(*ptePtr));
	}
    }

    segPtr->resPages = 0;

    spacePtr->spaceToFree = TRUE;
    spacePtr->ptPtr = segPtr->ptPtr;
    segPtr->ptPtr = (Vm_PTE *) NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * GetNewSegment --
 *
 *     	Allocate a new segment from the segment table.  If the segment is
 *	of type VM_CODE then it will have a flag set that indicates that
 *	its page tables have not been initialized.  It can be cleared by
 *	calling Vm_InitPageTable.
 *
 * Results:
 *      A pointer to the new segment is returned if a dead, free, or inactive 
 *	segment is available.  If no segments are available, then 
 *	NIL is returned.   If a dead or inactive segment is used here then
 *	*spacePtr is filled in with the space that was allocated
 *	to the segment.
 *
 * Side effects:
 *     A new segment out of the segment table is allocated.  Also if no
 *     free segments can be found, an inactive segment will be freed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Vm_Segment *
GetNewSegment(type, filePtr, fileAddr, numPages, offset, procPtr,
	      spacePtr, fileInfoPtr)
    int			type;		/* The type of segment that this is */
    Fs_Stream		*filePtr;	/* The unique identifier for this file
					   (if any) */
    int			fileAddr;	/* The address where the segments image
					   begins in the object file. */
    int			numPages;	/* The number of pages that this segment
					   initially has. */
    int			offset;		/* At which page from the beginning of
					   the VAS that this segment begins */
    Proc_ControlBlock	*procPtr;	/* Process for which the segment is
					   being allocated. */
	    /* IN/OUT parameter:  Passes in memory that has been allocated for
	       this segment and passes out any memory that has to be freed
	       either because we are out of segments or an inactive segment
	       was used. */
    VmSpace		*spacePtr;	
    VmFileInfo		*fileInfoPtr;	/* Where to return pointers to streams
					 * opened by a segment that is being
					 * reused. */
{
    register	Vm_Segment	*segPtr;
    register	VmProcLink	*procLinkPtr;
    Boolean			recycled;
    VmSpace			oldSpace;

    LOCK_MONITOR;

    if (!List_IsEmpty(deadSegList)) {
	/*
	 * If there is a dead code segment then use it so that we can free
	 * up things.
	 */
	segPtr = (Vm_Segment *) List_First(deadSegList);
	recycled = TRUE;
    } else if (!List_IsEmpty(freeSegList)) {
	/*
	 * If there is a free segment then use it.
	 */
	segPtr = (Vm_Segment *) List_First(freeSegList);
	spacePtr->spaceToFree = FALSE;
	recycled = FALSE;
    } else if (!List_IsEmpty(inactiveSegList)) {
	/*
	 * Inactive segment is available so use it.
	 */
	segPtr = (Vm_Segment *) List_First(inactiveSegList);
	recycled = TRUE;
    } else {
	/*
	 * No segments are available so return a NIL pointer in *segPtrPtr 
	 * and make sure that the caller frees the space that he sent in.
	 */
	spacePtr->spaceToFree = TRUE;
	UNLOCK_MONITOR;
	return((Vm_Segment *) NIL);
    }

    List_Remove((List_Links *) segPtr);
    if (recycled) {
	/*
	 * Cleanup the state of the dead or inactive segment. Any space used 
	 * by the segment will be stored in oldSpace.
	 */
	CleanSegment(segPtr, &oldSpace, fileInfoPtr);
	if (segPtr->flags & VM_SWAP_FILE_OPENED) {
	    fileInfoPtr->swapStreamPtr = segPtr->swapFilePtr;
	    fileInfoPtr->swapFileName = segPtr->swapFileName;
	    fileInfoPtr->segNum = segPtr->segNum;
	}
    }

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
    Byte_Zero(segPtr->ptSize * sizeof(Vm_PTE), (Address)segPtr->ptPtr);
    /*
     * If this is a stack segment, the page table grows backwards.  Therefore
     * all of the extra page table that we allocated must be taken off of the
     * offset where the stack was supposed to begin.
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
    /*
     * If a dead or inactive segment was used then return the space that it 
     * used in *spacePtr.
     */
    if (recycled) {
	*spacePtr = oldSpace;
	spacePtr->procLinkPtr = (VmProcLink *) NIL;
	spacePtr->spaceToFree = TRUE;
    }
	
    UNLOCK_MONITOR;
    return(segPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FindCode --
 *
 *     	Search the segment table for a code segment that has a matching 
 *	filePtr.  If one can be found, then increment the reference coutn
 *	and return a pointer to the segment.  If one can't be found then
 *	mark the file so that subsequent calls to this routine will wait
 *	until this code segment is initialized.
 *
 * Results:
 *     	A pointer to the matching segment if one is found, NIL if none found. 
 *
 * Side effects:
 *     	If a matching segment is found its reference count is incremented.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Vm_Segment *
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

    LOCK_MONITOR;

    *usedFilePtr = FALSE;
again:
    segPtrPtr = Fs_RetSegPtr((ClientData) filePtr->handlePtr);
    if (vm_NoStickySegments || *segPtrPtr == (Vm_Segment *) NIL) {
	/*
	 * There is no segment associated with this file.  Set the value to
	 * 0 so that we will know that we are about to set up this association.
	 */
	*segPtrPtr = (Vm_Segment *) 0;
	segPtr = (Vm_Segment *) NIL;
    } else if (*segPtrPtr == (Vm_Segment *) 0) {
	/*
	 * Someone is already trying to allocate this segment.  Wait for
	 * them to finish.
	 */
	Sync_Wait(&codeSegCondition, FALSE);
	goto again;
    } else {
	segPtr = *segPtrPtr;
	if (segPtr->flags & VM_SEG_INACTIVE) {
	    if (segPtr->fileHandle != (ClientData) filePtr->handlePtr) {
		Sys_Panic(SYS_FATAL, "FindCode: fileData != handlePtr\n");
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
	List_Insert((List_Links *) procLinkPtr, LIST_ATFRONT(segPtr->procList));
    }

    UNLOCK_MONITOR;

    return(segPtr);
}


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
 *     	Memory allocated.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Vm_Segment *
Vm_FindCode(filePtr, procPtr, execInfoPtrPtr, usedFilePtr)
    Fs_Stream		*filePtr;	/* The unique identifier for this file
					   (if any) */
    Proc_ControlBlock	*procPtr;	/* Process for which segment is being
					   allocated. */
    Vm_ExecInfo		**execInfoPtrPtr;/* Where to return relevant info from
					 * the a.out header. */
    Boolean		*usedFilePtr;	/* TRUE => Had to use the file pointer.
					 * FALSE => didn't have to use it. */
{
    register	Vm_Segment	*segPtr;
    VmProcLink			*procLinkPtr;

    procLinkPtr = (VmProcLink *) Mem_Alloc(sizeof(VmProcLink));
    procLinkPtr->procPtr = procPtr;
    segPtr = FindCode(filePtr, procLinkPtr, usedFilePtr);
    if (segPtr == (Vm_Segment *) NIL) {
	Mem_Free((Address) procLinkPtr);
    } else {
	*execInfoPtrPtr = &segPtr->execInfo;
    }

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

    LOCK_MONITOR;

    segPtrPtr = Fs_RetSegPtr((ClientData) filePtr->handlePtr);
    if (*segPtrPtr != (Vm_Segment *) 0) {
	Sys_Panic(SYS_FATAL, "Vm_InitCode: Seg ptr = %x\n", *segPtrPtr);
    }
    *segPtrPtr = segPtr;
    if (segPtr == (Vm_Segment *) NIL) {
	/*
	 * The caller doesn't want to set up any association between the file
	 * and the segment.  In this case cleanup state.
	 */
	Sync_Broadcast(&codeSegCondition);
    } else {
	extern	char	*Fs_GetFileName();
	
	segPtr->execInfo = *execInfoPtr;
	segPtr->fileHandle = (ClientData) filePtr->handlePtr;
	fileNamePtr = Fs_GetFileName(filePtr);
	if (fileNamePtr != (char *)NIL) {
	    length = String_Length(fileNamePtr);
	    if (length >= VM_OBJ_FILE_NAME_LENGTH) {
		length = VM_OBJ_FILE_NAME_LENGTH - 1;
	    }
	    String_NCopy(length, fileNamePtr, segPtr->objFileName);
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
 *	an object file that is being used then the corresponding code
 *	segment will be marked as deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Segment entry may be marked as deleted and possibly put onto the
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
	    Sys_Panic(SYS_FATAL, "Vm_FileChanged: In use code seg modified.\n");
	}
	List_Move((List_Links *) segPtr, LIST_ATREAR(deadSegList));
	*segPtrPtr = (Vm_Segment *) NIL;
	segPtr->fileHandle = (ClientData) NIL;
    }

    UNLOCK_MONITOR;
}



/*
 * ----------------------------------------------------------------------------
 *
 * Vm_SegmentNew --
 *
 *      Allocate a new segment from the segment table by calling the internal
 *      segment allocation routine GetNewSegment.  
 *
 * Results:
 *      A pointer to the new segment is returned if a free segment
 *      is available.  If no free segments are available, then NIL is returned.
 *
 * Side effects:
 *      The segment table entry is initialized.
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
    Vm_Segment         	  	*segPtr;
    VmSpace			space;
    VmFileInfo			fileInfo;

    space.procLinkPtr = (VmProcLink *) Mem_Alloc(sizeof(VmProcLink));
    if (type == VM_CODE) {
	space.ptPtr = (Vm_PTE *) Mem_Alloc(sizeof(Vm_PTE) * numPages);
	space.ptSize = numPages;
    } else {
	space.ptSize = ((numPages - 1) / vmPageTableInc + 1) * vmPageTableInc;
	space.ptPtr = (Vm_PTE *) Mem_Alloc(sizeof(Vm_PTE) * space.ptSize);
    }
    fileInfo.objStreamPtr = (Fs_Stream *) NIL;
    fileInfo.swapStreamPtr = (Fs_Stream *) NIL;
    segPtr = GetNewSegment(type, filePtr, fileAddr, numPages, offset, procPtr,
			   &space, &fileInfo);
    if (segPtr != (Vm_Segment *)NIL) {
	VmMach_SegInit(segPtr);
    }
    if (space.spaceToFree) {
	if (space.procLinkPtr != (VmProcLink *) NIL) {
	    Mem_Free((Address) space.procLinkPtr);
	}
	if (space.ptPtr != (Vm_PTE *)NIL) {
	    Mem_Free((Address)space.ptPtr);
	}
    }
    if (fileInfo.objStreamPtr != (Fs_Stream *) NIL) {
	Fs_Close(fileInfo.objStreamPtr);
    }
    if (fileInfo.swapStreamPtr != (Fs_Stream *) NIL) {
	VmSwapFileRemove(fileInfo.swapStreamPtr, fileInfo.swapFileName);
    }

    return(segPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmSegmentDeleteInt --
 *
 *      This routine will decrement the reference count for the given segment.
 *      If the reference count goes to zero, then one of two actions occurs.
 *      If the segment is code, then it is put on the inactive list.
 *      Otherwise the segments resources are freed up and it is put on the 
 *      free list.
 *
 *	If the segment is being migrated, it has already been cleaned
 *	but its space has not been reclaimed.
 *
 * Results:
 *	DELETE_SEG	-	The segment should be deleted.
 *	CLOSE_OBJ_FILE	-	Don't delete the segment, but close the file
 *				containing the code for the segment.
 *	DELETE_NOTHING	-	Don't do anything.
 *
 * Side effects:
 *      The segment table for the given segment is modified, either the
 *      free or inactive lists are modified and the list of processes
 *	sharing this segment is modified.
 */
ENTRY VmDeleteStatus
VmSegmentDeleteInt(segPtr, procPtr, spacePtr, fileInfoPtr, migFlag)
    register Vm_Segment 	*segPtr;	/* Pointer to segment to 
						   delete. */
    register Proc_ControlBlock	*procPtr;	/* Process that was using
						   this segment. */
    VmSpace			*spacePtr;	/* Used to return memory to
						   free to caller. */
    VmFileInfo			*fileInfoPtr;	/* Pointer to object and swap
						 * file information for the
						 * segment that is being
						 * deleted. */
    Boolean			migFlag;	/* TRUE if segment is being
						   migrated. */
{
    VmProcLink		*procLinkPtr;

    LOCK_MONITOR;

    segPtr->refCount--;
	
    /*
     * If the segment is not being migrated, then procPtr refers to a processes
     * in the list of processes sharing the segment.  Remove this process from
     * the list of processes and make sure that the space is freed.
     */
    if (!migFlag) {
	procLinkPtr = (VmProcLink *)
	        List_First((List_Links *) segPtr->procList);
	while (procPtr != procLinkPtr->procPtr) {
	    if (List_IsAtEnd(segPtr->procList, (List_Links *) procLinkPtr)) {
		Sys_Panic(SYS_FATAL, "%s%s",
	                "VmSegmentDeleteInt: Could not find segment on shared",
			"segment list.\n");
	    }
	    procLinkPtr = (VmProcLink *) List_Next((List_Links *) procLinkPtr);
	}
	List_Remove((List_Links *) procLinkPtr);

	spacePtr->procLinkPtr = procLinkPtr;
    }

    /*
     * If the segment is still being used then there is nothing to do.
     */
    if (segPtr->refCount > 0) {
	UNLOCK_MONITOR;
	return(VM_DELETE_NOTHING);
    }

    /* 
     * If a code segment put onto the inactive list if we are using
     * sticky segments.
     */
    if (!vm_NoStickySegments && segPtr->type == VM_CODE) {
	segPtr->flags |= VM_SEG_INACTIVE;
	fileInfoPtr->objStreamPtr = segPtr->filePtr;
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
    Proc_ControlBlock	*procPtr;
{
    VmSpace		space;
    VmFileInfo		fileInfo;
    VmDeleteStatus	status;

    fileInfo.objStreamPtr = (Fs_Stream *) NIL;
    status = VmSegmentDeleteInt(segPtr, procPtr, &space, &fileInfo, FALSE);
    if (status == VM_DELETE_SEG) {
	if (vm_CanCOW) {
	    VmCOWDeleteFromSeg(segPtr, -1, -1);
	}
	VmCleanSegment(segPtr, &space, FALSE, &fileInfo);
	VmMach_SegDelete(segPtr);
	if (space.ptPtr != (Vm_PTE *) NIL) {
	    Mem_Free((Address) space.ptPtr);
	}
	if (segPtr->flags & VM_SWAP_FILE_OPENED) {
	    VmSwapFileRemove(segPtr->swapFilePtr, segPtr->swapFileName);
	}
	if (fileInfo.objStreamPtr != (Fs_Stream *) NIL) {
	    Fs_Close(fileInfo.objStreamPtr);
	}
	VmPutOnFreeSegList(segPtr);
    } else if (status == VM_CLOSE_OBJ_FILE) {
	Fs_Close(fileInfo.objStreamPtr);
    }

    Mem_Free((Address) space.procLinkPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmCleanSegment --
 *
 *	Do monitor level cleanup for this deleted segment.
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
VmCleanSegment(segPtr, spacePtr, migrating, fileInfoPtr)
    Vm_Segment	*segPtr;
    VmSpace	*spacePtr;
    Boolean	migrating;
    VmFileInfo	*fileInfoPtr;
{
    LOCK_MONITOR;

    if (!migrating) {
	CleanSegment(segPtr, spacePtr, fileInfoPtr);
    } else {
	spacePtr->spaceToFree = TRUE;
	spacePtr->ptPtr = segPtr->ptPtr;
	segPtr->ptPtr = (Vm_PTE *) NIL;
    }

    UNLOCK_MONITOR;
}

Boolean	StartDelete();
void	EndDelete();


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
void
Vm_DeleteFromSeg(segPtr, firstPage, lastPage)
    Vm_Segment 	*segPtr;	/* The segment whose pages are being
				   invalidated. */
    int		firstPage;	/* The first page to invalidate */
    int		lastPage;	/* The second page to invalidate. */
{
    /*
     * The deletion of virtual pages from the segment is done in two
     * phases.  First the copy-on-write dependencies are cleaned up and
     * then the rest of the pages are cleaned up.  This requires some ugly
     * synchronization.  The problem is that during and after cleaning up
     * the copy-on-write dependencies, page faults and copy-on-write forks
     * in the segment must be prevented since cleanup is done at non-monitor
     * level.  This is done by using the VM_ADD_DEL_VA flag.  When this flag 
     * is set page faults and forks are blocked.  This flag is set by
     * StartDelete and cleared by EndDelete.  The flag is looked at by
     * VmVirtAddrParse (the routine that is called before any page fault
     * can occur on the segment) and by IncExpandCount (the routine that is
     * called when a segment is duplicated for a fork).
     */
    if (!StartDelete(segPtr, firstPage, &lastPage)) {
	return;
    }
    /*
     * The segment is now not expandable.  Now rid the segment of all 
     * copy-on-write dependencies.
     */
    VmCOWDeleteFromSeg(segPtr, firstPage, lastPage);

    EndDelete(segPtr, firstPage, lastPage);
}


/*
 *----------------------------------------------------------------------
 *
 * StartDelete --
 *
 *	Set things up to delete pages from a segment.  This involves making
 *	the segment not expandable.
 *
 * Results:
 *	FALSE if there is nothing to delete from this segment.
 *
 * Side effects:
 *	Expand count incremented.
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

    while (segPtr->notExpandCount > 0) {
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
	segPtr->notExpandCount = 1;
	segPtr->flags |= VM_ADD_DEL_VA;
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
 *	Expand count decremented and the segment size may be shrunk.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
EndDelete(segPtr, firstPage, lastPage)
    register	Vm_Segment	*segPtr;
    int				firstPage;
    int				lastPage;
{
    register	Vm_PTE	*ptePtr;
    Vm_VirtAddr		virtAddr;
    unsigned	int	pfNum;

    LOCK_MONITOR;

    if (lastPage == segPtr->offset + segPtr->numPages - 1) {
	segPtr->numPages -= lastPage - firstPage + 1;
    }

    /*
     * Free up any resident pages.
     */
    virtAddr.segPtr = segPtr;
    for (virtAddr.page = firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	if (*ptePtr & VM_PHYS_RES_BIT) {
	    VmMach_PageInvalidate(&virtAddr, VmGetPageFrame(*ptePtr), FALSE);
	    segPtr->resPages--;
	    pfNum = VmGetPageFrame(*ptePtr);
	    *ptePtr = 0;
	    VmPageFreeInt(pfNum);
	}
	*ptePtr = 0;
    }

    /*
     * The segment can now be expanded.
     */
    segPtr->notExpandCount = 0;
    segPtr->flags &= ~VM_ADD_DEL_VA;
    Sync_Broadcast(&segPtr->condition);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmDecExpandCount --
 *
 *     	Decrement the number of times that the heap segment was prevented from
 *	expanding.  If the count goes to zero then wake up anyone waiting
 *	on it.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Count of times prevented from expanding is decremented.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmDecExpandCount(segPtr)
    register	Vm_Segment		*segPtr;
{
    LOCK_MONITOR;

    segPtr->notExpandCount--;
    if (segPtr->notExpandCount == 0) {
	Sync_Broadcast(&segPtr->condition);
    }

    UNLOCK_MONITOR;
}

void	StartExpansion();
void	EndExpansion();
void	AllocMoreSpace();


/*
 *----------------------------------------------------------------------
 *
 * VmAddToSeg --
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
ReturnStatus
VmAddToSeg(segPtr, firstPage, lastPage)
    register Vm_Segment *segPtr;	/* The segment whose VAS is to be
					   modified. */
    int	    	       	firstPage; 	/* The lowest page to put into the 
					   VAS. */
    int	    	       	lastPage; 	/* The highest page to put into the 
					   VAS. */
{
    VmSpace	newSpace;
    VmSpace	oldSpace;
    int		retValue;
    int		newNumPages;

    /*
     * The only segments that can be expanded are the stack and heap segments.
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
	Mem_Free((Address)oldSpace.ptPtr);
    }
    VmMach_SegExpand(segPtr);

    EndExpansion(segPtr);

    return(retValue);
}


/*
 *----------------------------------------------------------------------
 *
 * StartExpansion --
 *
 *	Mark the page tables as expansion in progress so that no other
 *	expansions or deletions can happen on the segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Expand count incremented and VM_ADD_DEL_VA flag set.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
StartExpansion(segPtr)
    Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    while (segPtr->notExpandCount > 0) {
	(void)Sync_Wait(&segPtr->condition, FALSE);
    }
    segPtr->flags |= VM_ADD_DEL_VA;
    segPtr->notExpandCount++;

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
 *	Expand count decremented and VM_ADD_DEL_VA flag cleared.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
EndExpansion(segPtr)
    Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    segPtr->flags &= ~VM_ADD_DEL_VA;
    segPtr->notExpandCount--;
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
	spacePtr->ptPtr = (Vm_PTE *)Mem_Alloc(sizeof(Vm_PTE) * spacePtr->ptSize);
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
ENTRY ReturnStatus 
AddToSeg(segPtr, firstPage, lastPage, newNumPages, newSpace, oldSpacePtr)
    register	Vm_Segment	*segPtr;	/* The segment to add the
						   virtual pages to. */
    int				firstPage;	/* The lowest page to put
						   into the VAS. */
    int				lastPage;	/* The highest page to put
						   into the VAS. */
    int				newNumPages;	/* The new number of pages
						 * that will be in the
						 * segment. */
    VmSpace			newSpace;	/* Pointer to new page table
						   if the segment has to
						   be expanded. */
    VmSpace			*oldSpacePtr;	/* Place to return pointer
						   to space to free. */
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
	    Byte_Copy(copySize, (Address) segPtr->ptPtr, 
			(Address) newSpace.ptPtr);
	    Byte_Zero((newSpace.ptSize - segPtr->ptSize)  * sizeof(Vm_PTE),
		      (Address) ((int) (newSpace.ptPtr) + copySize));
	} else {
	    /*
	     * Make sure that the heap segment isn't too big.  If it is then 
	     * abort.
	     */
	    otherSegPtr = proc_RunningProcesses[0]->vmPtr->segPtrArray[VM_HEAP];
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
	    Byte_Copy(copySize, (Address) segPtr->ptPtr, 
		      (Address) ((int) (newSpace.ptPtr) + byteOffset));
	    Byte_Zero(byteOffset, (Address) newSpace.ptPtr);
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

void	IncExpandCount();
void	CopyInfo();
Boolean	CopyPage();


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
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_SegmentDup(srcSegPtr, procPtr, destSegPtrPtr)
    register Vm_Segment *srcSegPtr;	/* Pointer to the segment to be 
					   duplicate. */
    Proc_ControlBlock   *procPtr; 	/* Pointer to the process for which the 
				   	   segment is being duplicated. */
    Vm_Segment		**destSegPtrPtr; /* Place to return pointer to new
					    segment. */
{
    register	Vm_Segment	*destSegPtr;
    ReturnStatus		status;
    register	Vm_PTE	*srcPTEPtr;
    register	Vm_PTE	*destPTEPtr;
    Vm_PTE		*tSrcPTEPtr;
    Vm_PTE		*tDestPTEPtr;
    Vm_VirtAddr		srcVirtAddr;
    Vm_VirtAddr		destVirtAddr;
    int			i;
    Address		srcAddr;
    Address		destAddr;
    Fs_Stream		*newFilePtr;

    if (srcSegPtr->type == VM_HEAP) {
	/*
	 * Prevent the source segment from being expanded if it is a heap 
	 * segment.  Stack segments can't be expanded because they can't be
	 * used by anybody but the process that is calling us.
	 */
	IncExpandCount(srcSegPtr);
	Fs_StreamCopy(srcSegPtr->filePtr, &newFilePtr, procPtr->processID);
    } else {
	newFilePtr = (Fs_Stream *) NIL;
    }

    /*
     * Allocate the segment that we are copying to.
     */
    destSegPtr = Vm_SegmentNew(srcSegPtr->type, newFilePtr,
			       srcSegPtr->fileAddr, srcSegPtr->numPages, 
			       srcSegPtr->offset, procPtr);
    if (destSegPtr == (Vm_Segment *) NIL) {
	if (srcSegPtr->type == VM_HEAP) {
	    VmDecExpandCount(srcSegPtr);
	    Fs_Close(newFilePtr);
	}
	*destSegPtrPtr = (Vm_Segment *) NIL;
	return(VM_NO_SEGMENTS);
    }

    if (vm_CanCOW) {
	/*
	 * We are allowing copy-on-write.  Make a copy-on-ref image of the
	 * src segment in the dest segment.
	 */
	VmSegFork(srcSegPtr, destSegPtr);
	if (srcSegPtr->type == VM_HEAP) {
	    VmDecExpandCount(srcSegPtr);
	}
	*destSegPtrPtr = destSegPtr;

	return(SUCCESS);
    }

    /*
     * No copy-on-write.  Do a full fledged copy of the source segment to
     * the dest segment.
     */
    CopyInfo(srcSegPtr, destSegPtr, &tSrcPTEPtr, &tDestPTEPtr, &srcVirtAddr,
	     &destVirtAddr);
    srcAddr = (Address) NIL;

    /*
     * Copy over memory.
     */
    for (i = 0, srcPTEPtr = tSrcPTEPtr, destPTEPtr = tDestPTEPtr; 
	 i < destSegPtr->numPages; 
	 i++, VmIncPTEPtr(srcPTEPtr, 1), VmIncPTEPtr(destPTEPtr, 1), 
		destVirtAddr.page++) {
	if (CopyPage(srcSegPtr, srcPTEPtr, destPTEPtr)) {
	    *destPTEPtr |= VM_REFERENCED_BIT | VM_MODIFIED_BIT |
	                   VmPageAllocate(&destVirtAddr, TRUE);
	    destSegPtr->resPages++;
	    if (srcAddr == (Address) NIL) {
		srcAddr = VmMapPage(VmGetPageFrame(*srcPTEPtr));
		destAddr = VmMapPage(VmGetPageFrame(*destPTEPtr));
	    } else {
		VmRemapPage(srcAddr, VmGetPageFrame(*srcPTEPtr));
		VmRemapPage(destAddr, VmGetPageFrame(*destPTEPtr));
	    }
	    Byte_Copy(vm_PageSize, srcAddr, destAddr);
	    VmUnlockPage(VmGetPageFrame(*srcPTEPtr));
	    VmUnlockPage(VmGetPageFrame(*destPTEPtr));
	}
    }
    /*
     * Unmap any mapped pages.
     */
    if (srcAddr != (Address) NIL) {
        VmUnmapPage(srcAddr);
        VmUnmapPage(destAddr);
    }

    /*
     * Copy over swap space resources.
     */
    status = VmCopySwapSpace(srcSegPtr, destSegPtr);

    if (srcSegPtr->type == VM_HEAP) {
	VmDecExpandCount(srcSegPtr);
    }

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
 * IncExpandCount --
 *
 *     	Increment the number of times that the heap segment was prevented from
 *	expanding.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Count of times prevented from expanding is increment.
 *     
 * ----------------------------------------------------------------------------
 */
ENTRY static void
IncExpandCount(segPtr)
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    while (segPtr->flags & VM_ADD_DEL_VA) {
	(void)Sync_Wait(&segPtr->condition, FALSE);
    }
    segPtr->notExpandCount++;

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
 *	Page tables in the destination segment become copy of the source
 *	segments page table and the two pte pointers are set.	
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
	VmLockPageInt(VmGetPageFrame(*srcPTEPtr));
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
ENTRY void
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

    procLinkPtr = (VmProcLink *) Mem_Alloc(sizeof(VmProcLink));
    procLinkPtr->procPtr = procPtr;

    SegmentIncRef(segPtr, procLinkPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetSegInfo --
 *
 *	This routine takes in a pointer to a proc table entry and returns
 *	the segment table information for the segments that it uses.
 *
 * Results:
 *	SUCCESS if could get the information, SYS_ARG_NOACCESS if the
 *	the pointer to the segment table entries passed in are bad amd
 *	SYS_INVALID_ARG if one of the segment pointers in the proc table
 *	are bad.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_GetSegInfo(procPtr, segNum, segBufPtr)
    Proc_ControlBlock	*procPtr;	/* User's copy of PCB.  Contains
					 * pointers to segment structures.
					 * USER_NIL => Want to use a 
					 *     specific segment number. */
    int			segNum;		/* Segment number of get info for.  
					 * Ignored unless previous argument
					 * is USER_NIL. */
    Vm_Segment		*segBufPtr;	/* Where to store segment information.*/
{
    Proc_ControlBlock	pcb;
    Vm_Segment		*minSegAddr, *maxSegAddr, *segPtr;
    int			i;

    if (procPtr != (Proc_ControlBlock *)USER_NIL) {
	if (Vm_CopyIn(sizeof(pcb), (Address) procPtr,
		      (Address) &pcb) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
	if (procPtr->vmPtr == (Vm_ProcInfo *)NIL) {
	    return(SYS_INVALID_ARG);
	}
	minSegAddr = segmentTable;
	maxSegAddr = &(segmentTable[numSegments - 1]);
	for (i = VM_CODE; i <= VM_STACK; i++, segBufPtr++) {
	    if (pcb.genFlags & PROC_KERNEL) {
		segPtr = vm_SysSegPtr;
	    } else {
		segPtr = pcb.vmPtr->segPtrArray[i];
		if (segPtr < minSegAddr || segPtr > maxSegAddr) {
		    return(SYS_INVALID_ARG);
		}
	    }

	    if (Vm_CopyOut(sizeof(Vm_Segment), (Address) segPtr, 
			   (Address)segBufPtr) != SUCCESS) { 
		return(SYS_ARG_NOACCESS);
	    }
	}
    } else if (segNum < 0 || segNum >= numSegments) {
	return(SYS_INVALID_ARG);
    } else {
	segPtr = &segmentTable[segNum];
	if (Vm_CopyOut(sizeof(Vm_Segment), (Address) segPtr, 
		       (Address) segBufPtr) != SUCCESS) { 
	    return(SYS_ARG_NOACCESS);
	}
    }

    return(SUCCESS);
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
