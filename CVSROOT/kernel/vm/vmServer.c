/* vmServer.c -
 *
 *     	This file contains routines that read and write pages to/from the page
 *	server and file server.
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
#include "status.h"
#include "sched.h"
#include "sync.h"
#include "dbg.h"
#include "list.h"
#include "string.h"
#include "stdlib.h"
#include "proc.h"

Boolean	vmUseFSReadAhead = TRUE;
extern	Boolean	vm_NoStickySegments;
Fs_Stream	*vmSwapStreamPtr = (Fs_Stream *)NIL;

Boolean vmSwapFileDebug = FALSE;

/*
 * Condition to wait on when want to do a swap file operation but someone
 * is already doing one.  This is used for synchronization of opening
 * of swap files.  It is possible that the open of the swap file could
 * happen more than once if the open is not synchronized.
 */
Sync_Condition	swapFileCondition;

void	Fs_CacheBlocksUnneeded();


/*
 *----------------------------------------------------------------------
 *
 * VmSwapFileRemove --
 *
 *	Remove the swap file for the given segment.
 *
 *	NOTE: This does not have to be synchronized because it is assumed
 *	      that it is called after the memory for the process has already
 *	      been freed thus the swap file can't be messed with.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The swap file is removed.
 *----------------------------------------------------------------------
 */
VmSwapFileRemove(swapStreamPtr, swapFileName)
    Fs_Stream	*swapStreamPtr;
    char	*swapFileName;
{
    Proc_ControlBlock	*procPtr;
    int			origID = NIL;
    ReturnStatus	status;

    (void)Fs_Close(swapStreamPtr);
    procPtr = Proc_GetEffectiveProc();
    if (procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
	origID = procPtr->effectiveUserID;
	procPtr->effectiveUserID = PROC_SUPER_USER_ID;
    }
    if (swapFileName != (char *) NIL) {
	if (vmSwapFileDebug) {
	    status = SUCCESS;
	    printf("VmSwapFileRemove: not removing swap file %s.\n",
		       swapFileName);
	} else {
	    status = Fs_Remove(swapFileName);
	}
	if (status != SUCCESS) {
	    printf("Warning: VmSwapFileRemove: Fs_Remove(%s) returned %x.\n",
		      swapFileName, status);
	    if (status == FS_FILE_NOT_FOUND &&
		vmSwapStreamPtr != (Fs_Stream *) NIL) {
		/*
		 * This can happen if the swap file itself is removed,
		 * or if the directory gets changed.  Reopen the directory
		 * to make sure.
		 */
		(void) Fs_Close(vmSwapStreamPtr);
		vmSwapStreamPtr = (Fs_Stream *) NIL;
		printf("Reopening swap directory.\n");
		Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, 0);
	    }
	}
	free(swapFileName);
    }
    if (origID != NIL) {
	procPtr->effectiveUserID = origID;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmSwapFileLock --
 *
 *	Set a lock on the swap file for the given segment.  If the lock
 *	is already set then wait.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Flags field in segment table entry is modified to make the segment
 *	locked.
 *----------------------------------------------------------------------
 */

ENTRY void
VmSwapFileLock(segPtr)
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    while (segPtr->flags & VM_SWAP_FILE_LOCKED) {
	(void) Sync_Wait(&swapFileCondition, FALSE);
    }
    segPtr->flags |= VM_SWAP_FILE_LOCKED;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSwapFileUnlock --
 *
 *	Clear the lock on the swap file for the given segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Flags field in segment table entry is modified to make the segment
 *	unlocked and anyone waiting for the segment is awakened.
 *----------------------------------------------------------------------
 */

ENTRY void
VmSwapFileUnlock(segPtr)
    register	Vm_Segment	*segPtr;
{
    LOCK_MONITOR;

    segPtr->flags &= ~VM_SWAP_FILE_LOCKED;
    Sync_Broadcast(&swapFileCondition);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * VmPageServerRead --
 *
 *	Read the given page from the swap file into the given page frame.
 *	This routine will panic if the swap file does not exist.
 *
 *	NOTE: It is assumed that the page frame that is to be written into
 *	      cannot be given to another segment.
 *
 * Results:
 *	SUCCESS if the page server could be read from or an error if either
 *	a swap file could not be opened or the page server could not be
 *	read from the swap file.
 *
 * Side effects:
 *	The hardware page is written into.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmPageServerRead(virtAddrPtr, pageFrame)
    Vm_VirtAddr			*virtAddrPtr;
    unsigned	int		pageFrame;
{
    register	int		mappedAddr;
    register	Vm_Segment	*segPtr;
    int				status;
    int				pageToRead;

    segPtr = virtAddrPtr->segPtr;
    if (!(segPtr->flags & VM_SWAP_FILE_OPENED)) {
	panic("VmPageServerRead: Trying to read from non-existent swap file.\n");
    }

    if (segPtr->type == VM_STACK) {
	pageToRead = mach_LastUserStackPage - virtAddrPtr->page;
    } else if (segPtr->type == VM_SHARED) {
	pageToRead= virtAddrPtr->page - segOffset(virtAddrPtr) +
		(virtAddrPtr->sharedPtr->fileAddr>>vmPageShift);
    } else {
	pageToRead = virtAddrPtr->page - segPtr->offset;
    }

    /*
     * Map the page into the kernel's address space and fill it from the
     * file server.
     */
    mappedAddr = (int) VmMapPage(pageFrame);
    status = Fs_PageRead(segPtr->swapFilePtr, (Address) mappedAddr,
			 pageToRead << vmPageShift, vm_PageSize, FS_SWAP_PAGE);
    VmUnmapPage((Address) mappedAddr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * VmOpenSwapDirectory --
 *
 *	Open the swap directory for this machine.  This is needed for 
 *	recovery.  This is called periodically until it successfully opens
 *	the swap directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Swap directory stream pointer is set.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
void
Vm_OpenSwapDirectory(data, callInfoPtr)
    ClientData		data;	
    Proc_CallInfo	*callInfoPtr;
{
    char		number[34];
    char		fileName[FS_MAX_PATH_NAME_LENGTH];
    ReturnStatus	status;

    (void)strcpy(fileName, VM_SWAP_DIR_NAME);
    (void)sprintf(number, "%u", (unsigned) Sys_GetHostId());
    (void)strcat(fileName, number);
    status = Fs_Open(fileName, FS_FOLLOW, FS_DIRECTORY, 0, &vmSwapStreamPtr);
    if (status != SUCCESS) {
	/*
	 * It didn't work, retry in 20 seconds.
	 */
	callInfoPtr->interval = 20 * timer_IntOneSecond;
	vmSwapStreamPtr = (Fs_Stream *)NIL;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmOpenSwapFile --
 *
 *	Open a swap file for this segment.  Store the name of the swap
 *	file with the segment.
 *
 * Results:
 *	SUCCESS unless swap file could not be opened.
 *
 * Side effects:
 *	Swap file pointer is set in the segment's data struct.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmOpenSwapFile(segPtr)
    register	Vm_Segment	*segPtr;
{
    int				status;
    Proc_ControlBlock		*procPtr;
    int				origID = NIL;
    char			fileName[FS_MAX_PATH_NAME_LENGTH];
    char			*swapFileNamePtr;
    Fs_Stream			*origCwdPtr;

    if (segPtr->swapFileName == (char *) NIL) {
	/*
	 * There is no swap file yet so open one.  This may entail assembling 
	 * the file name first.  The file name is the segment number.
	 */
	VmMakeSwapName(segPtr->segNum, fileName);
	segPtr->swapFileName = (char *) malloc(strlen(fileName) + 1);
	(void) strcpy(segPtr->swapFileName, fileName);
    }
#ifdef SWAP_FILE_DEBUG
    printf("Opening swap file %s.\n", segPtr->swapFileName);
#endif /* SWAP_FILE_DEBUG */
    procPtr = Proc_GetEffectiveProc();
    if (procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
	origID = procPtr->effectiveUserID;
	procPtr->effectiveUserID = PROC_SUPER_USER_ID;
    }
    /*
     * We want the swap file open to happen relative to the swap directory
     * for this machine if possible.  This is so that if the swap directory
     * is a symbolic link and the swap open fails we know that it failed
     * because the swap server is down, not the server of the symbolic link.
     */
    origCwdPtr = procPtr->fsPtr->cwdPtr;
    if (vmSwapStreamPtr != (Fs_Stream *)NIL) {
	procPtr->fsPtr->cwdPtr = vmSwapStreamPtr;
	(void)sprintf(fileName, "%u", (unsigned) segPtr->segNum);
	swapFileNamePtr = fileName;
    } else {
	swapFileNamePtr = segPtr->swapFileName;
    }
    status = Fs_Open(swapFileNamePtr, 
		     FS_READ | FS_WRITE | FS_CREATE | FS_TRUNC | FS_SWAP,
		     FS_FILE, 0660, &segPtr->swapFilePtr);
    if (origID != NIL) {
	procPtr->effectiveUserID = origID;
    }
    procPtr->fsPtr->cwdPtr = origCwdPtr;
    if (status != SUCCESS) {
	segPtr->swapFilePtr = (Fs_Stream *)NIL;
	printf("%s VmOpenSwapFile: Could not open swap file %s, reason 0x%x\n", 
		"Warning:", segPtr->swapFileName, status);
	return(status);
    }

    segPtr->flags |= VM_SWAP_FILE_OPENED;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * VmMakeSwapName --
 *
 *	Given a buffer to hold the name of the swap file, return the
 *	name corresponding to the given segment.  FileName is assumed to
 *	hold a string long enough for the maximum swap file name.
 *
 * Results:
 *	The pathname is returned in fileName.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
VmMakeSwapName(segNum, fileName)
    int  segNum;		/* segment for which to create name */ 
    char *fileName;		/* pointer to area to hold name */
{
    char number[34];

    (void)strcpy(fileName, VM_SWAP_DIR_NAME);
    (void)sprintf(number, "%u", (unsigned) Sys_GetHostId());
    (void)strcat(fileName, number);
    (void)strcat(fileName, "/");
    (void)sprintf(number, "%u", (unsigned) (segNum));
    (void)strcat(fileName, number);
}


/*
 *----------------------------------------------------------------------
 *
 * VmPageServerWrite --
 *
 *	Write the given page frame to the swap file.  If the swap file is
 *	not open yet then it will be open.
 *
 *	NOTE: It is assumed that the page frame that is to be read from
 *	      cannot be given to another segment.
 *
 * Results:
 *	SUCCESS if the page server could be written to or an error if either
 *	a swap file could not be opened or the page server could not be
 *	written to.
 *
 * Side effects:
 *	If no swap file exists, then one is created.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmPageServerWrite(virtAddrPtr, pageFrame)
    Vm_VirtAddr		*virtAddrPtr;
    unsigned int	pageFrame;
{
    register	int		mappedAddr;
    register	Vm_Segment	*segPtr;
    ReturnStatus		status;
    int				pageToWrite;

    vmStat.pagesWritten++;

    segPtr = virtAddrPtr->segPtr;

    /*
     * Lock the swap file while opening it so that we don't have more than
     * one swap file open at a time.
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

    if (segPtr->type == VM_STACK) {
	pageToWrite = mach_LastUserStackPage - virtAddrPtr->page;
    } else {
	pageToWrite = virtAddrPtr->page - segOffset(virtAddrPtr);
    }

    /*
     * Map the page into the kernel's address space and write it out.
     */
    VmMach_FlushPage(virtAddrPtr, FALSE);
    mappedAddr = (int) VmMapPage(pageFrame);
    status = Fs_PageWrite(segPtr->swapFilePtr, (Address) mappedAddr,
			  pageToWrite << vmPageShift, vm_PageSize);
    VmUnmapPage((Address) mappedAddr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * VmCopySwapSpace --
 *
 *	Copy the swap space for all pages that have been written out to swap
 *	space for the source segment into the destination segments swap space.
 *
 * Results:
 *	Error if swap file could not be opened, read or written.  Otherwise
 *	SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
VmCopySwapSpace(srcSegPtr, destSegPtr)
    register	Vm_Segment	*srcSegPtr;
    register	Vm_Segment	*destSegPtr;
{
    register	int	page;
    register	Vm_PTE	*ptePtr;
    ReturnStatus	status;
    register	int	i;

    VmSwapFileLock(srcSegPtr);
    if (!(srcSegPtr->flags & VM_SWAP_FILE_OPENED)) {
	VmSwapFileUnlock(srcSegPtr);
	return(SUCCESS);
    }
    VmSwapFileUnlock(srcSegPtr);

    VmSwapFileLock(destSegPtr);
    if (!(destSegPtr->flags & VM_SWAP_FILE_OPENED)) {
	status = VmOpenSwapFile(destSegPtr);
	if (status != SUCCESS) {
	    VmSwapFileUnlock(destSegPtr);
	    return(status);
	}
    }
    VmSwapFileUnlock(destSegPtr);

    if (destSegPtr->type == VM_STACK) {
	page = destSegPtr->numPages - 1;
	ptePtr = VmGetPTEPtr(destSegPtr, mach_LastUserStackPage - 
			                 destSegPtr->numPages + 1);
    } else {
	page = 0;
    	ptePtr = destSegPtr->ptPtr;
    }

    for (i = 0; i < destSegPtr->numPages; i++, VmIncPTEPtr(ptePtr, 1)) {

	if (*ptePtr & VM_IN_PROGRESS_BIT) {
	    *ptePtr &= ~VM_IN_PROGRESS_BIT;
	    /*
	     * The page is on the swap file and not in memory.  Need to copy
	     * the page in the file.
	     */
	    vmStat.swapPagesCopied++;
	    status = Fs_PageCopy(srcSegPtr->swapFilePtr, 
				destSegPtr->swapFilePtr, 
				page << vmPageShift, vm_PageSize);
	    if (status != SUCCESS) {
		break;
	    }
	}
	if (destSegPtr->type == VM_STACK) {
	    page--;
	} else {
	    page++;
	}
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * VmFileServerRead --
 *
 *	Read the given page from the file server into the given page frame.
 *
 *	NOTE: It is assumed that the page frame that is to be written into
 *	      cannot be given to another segment.
 *
 * Results:
 *	Error if file server could not be read from, SUCCESS otherwise.
 *
 * Side effects:
 *	The hardware page is written into.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
VmFileServerRead(virtAddrPtr, pageFrame)
    Vm_VirtAddr		*virtAddrPtr;
    unsigned int	pageFrame;
{
    register	int		mappedAddr;
    register	Vm_Segment	*segPtr;
    int				length;
    int				status;
    int				offset;

    segPtr = virtAddrPtr->segPtr;

    /*
     * Map the page frame into the kernel's address space.
     */
    mappedAddr = (int) VmMapPage(pageFrame);
    /*
     * The address to read is just the page offset into the segment
     * ((page - offset) << vmPageShift) plus the offset of this segment into
     * the file (fileAddr).
     */
    length = vm_PageSize;
    offset = ((virtAddrPtr->page - segPtr->offset) << vmPageShift) + 
	    segPtr->fileAddr;
    if (vmPrefetch || !vmUseFSReadAhead) {
	/*
	 * If we are using prefetch then do the reads ourselves.
	 */
	if (segPtr->type == VM_CODE && !vm_NoStickySegments) {
	    status = Fs_PageRead(segPtr->filePtr, (Address)mappedAddr, offset,
				 length, FS_CODE_PAGE);
	} else {
	    status = Fs_PageRead(segPtr->filePtr, (Address)mappedAddr, offset,
				 length, FS_HEAP_PAGE);
	}
    } else {
	/*
	 * No prefetch so use the file system full blown mechanism so
	 * that we can take advantage of its read ahead.
	 */
	status = Fs_Read(segPtr->filePtr, (Address) mappedAddr, offset,
			 &length);
	if (status == SUCCESS && !vm_NoStickySegments && 
	    segPtr->type == VM_CODE) {
	    /*
	     * Tell the file system that we just read some file system blocks
	     * into virtual memory.
	     */
	    Fs_CacheBlocksUnneeded(segPtr->filePtr, offset, vm_PageSize, TRUE);
	}
    }
    VmUnmapPage((Address) mappedAddr);
    if (status != SUCCESS) {
	printf("%s VmFileServerRead: Error %x from Fs_Read or Fs_PageRead\n",
		"Warning:", status);
	return(status);
    }

    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * VmCopySwapPage --
 *
 *	Copy the swap page from the source segment's swap file to the
 *	destination.
 *
 * Results:
 *	Error if swap file could not be opened, read or written.  Otherwise
 *	SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmCopySwapPage(srcSegPtr, virtPage, destSegPtr)
    register	Vm_Segment	*srcSegPtr;	/* Source for swap file. */
    int				virtPage;	/* Virtual page to copy. */
    register	Vm_Segment	*destSegPtr;	/* Destination swap file. */
{
    int			pageToCopy;
    ReturnStatus	status;

    /*
     * Lock the swap file while opening it so that we don't have more than
     * one swap file open at a time.
     */
    VmSwapFileLock(destSegPtr);
    if (!(destSegPtr->flags & VM_SWAP_FILE_OPENED)) {
	status = VmOpenSwapFile(destSegPtr);
	if (status != SUCCESS) {
	    VmSwapFileUnlock(destSegPtr);
	    return(status);
	}
    }
    VmSwapFileUnlock(destSegPtr);

    vmStat.swapPagesCopied++;
    if (destSegPtr->type == VM_STACK) {
	pageToCopy = mach_LastUserStackPage - virtPage;
    } else {
	pageToCopy = virtPage - destSegPtr->offset;
    }

    status = Fs_PageCopy(srcSegPtr->swapFilePtr, 
			 destSegPtr->swapFilePtr, 
			 pageToCopy << vmPageShift, vm_PageSize);

    return(status);
}
