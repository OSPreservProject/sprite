/* 
 * vmSwapDir.c --
 *
 *	This file synchronizes opening and reopening of the VM swap
 *	directory.  The swap directory is used for error recovery.
 *	It provides a FS handle on which the VM module can wait.
 *	During error conditions the swap directory is reopened,
 *	and this needs to be synchronized so more than one process
 *	doesn't attempt to do it.  This monitor calls FS routines,
 *	and as a rule the main VM monitor lock should not be held
 *	during calls into FS, so these routines also should not be
 *	called from within VM routines protected by vmMonitorLock.
 *
 *	The routines that create and remove swap files are also in
 *	this module as they are one of the primary uses of the monitored
 *	set of procedures.
 *
 * Copyright 1989 Regents of the University of California
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
#endif /* not lint */

#include "sprite.h"
#include "fs.h"
#include "vm.h"
#include "vmInt.h"
#include "sync.h"
#include "vmSwapDir.h"

static Sync_Lock vmSwapDirLock = Sync_LockInitStatic("vmSwapDirLock");
#define LOCKPTR &vmSwapDirLock

/*
 * This is the variable for which access needs to be synchronized.
 */
Fs_Stream	*vmSwapStreamPtr = (Fs_Stream *)NIL;

/*
 * This is a reference count to the swap stream.  It can't be reopened
 * while someone is using it for recovery.
 */
static int swapStreamUsers = 0;

/*
 * This indicates if a reopen is in progress.  Only one reopen at a time
 * is allowed.
 */
static Boolean reopenInProgress = FALSE;

Boolean vmSwapFileDebug = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Vm_OpenSwapDirectory --
 *
 *	Open the swap directory for this machine.  This is used to
 *	optimize naming operations on the swap file, and, more importantly,
 *	this open directory gives VM a handle on which to wait for recovery.
 *	This routine will retry periodically until it successfully opens
 *	the swap directory.  This handles multiple invocations, and it
 *	takes care of closing the existing stream to the directory
 *	if needed.
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
ENTRY void
Vm_OpenSwapDirectory(data, callInfoPtr)
    ClientData		data;	
    Proc_CallInfo	*callInfoPtr;
{
    char		number[34];
    char		fileName[FS_MAX_PATH_NAME_LENGTH];
    ReturnStatus	status;

    LOCK_MONITOR;

    /*
     * Synchronize with processes using the swap stream pointer for recovery.
     * We just try again later, instead of blocking this Proc_ServerProc
     * on a condition variable.
     */
    if (swapStreamUsers > 0) {
	callInfoPtr->interval = 20 * timer_IntOneSecond;
	UNLOCK_MONITOR;
	return;
    }

    if (vmSwapStreamPtr != (Fs_Stream *) NIL) {
	(void) Fs_Close(vmSwapStreamPtr);
	vmSwapStreamPtr = (Fs_Stream *) NIL;
	printf("Reopening swap directory.\n");
    }

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
    } else {
	reopenInProgress = FALSE;
    }
    UNLOCK_MONITOR;
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
    Fs_Stream			*swapDirPtr;
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
    swapDirPtr = VmGetSwapStreamPtr();
    if (swapDirPtr != (Fs_Stream *)NIL) {
	origCwdPtr = procPtr->fsPtr->cwdPtr;
	procPtr->fsPtr->cwdPtr = swapDirPtr;
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
    if (swapDirPtr != (Fs_Stream *)NIL) {
	procPtr->fsPtr->cwdPtr = origCwdPtr;
	VmDoneWithSwapStreamPtr();
    }
    if (status != SUCCESS) {
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
	    if (status == FS_FILE_NOT_FOUND) {
		/*
		 * This can happen if the swap file itself is removed,
		 * or if the directory gets changed.  Reopen the directory
		 * to make sure.
		 */
		VmReopenSwapDirectory();
	    }
	}
	free(swapFileName);
    }
    if (origID != NIL) {
	procPtr->effectiveUserID = origID;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmReopenSwapDirectory --
 *
 *	Reopen the swap stream that is used for recovery.
 *	
 * Results:
 *	None.  
 *
 * Side effects:
 *	Enqueues a callback to do the real work of the reopen.
 *	Sets reopenInProgress so only one re-open of the swap directory occurs.
 *
 * ----------------------------------------------------------------------------
 */
void
VmReopenSwapDirectory()
{
    LOCK_MONITOR;
    if (!reopenInProgress) {
	reopenInProgress = TRUE;
	Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, 0);
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmGetSwapStreamPtr --
 *
 *	Return the swap stream pointer.  This has to be followed by
 *	a call to VmDoneWithSwapStreamPtr if NIL is not returned.
 *	
 *
 * Results:
 *	NIL or a valid swap stream pointer.  
 *
 * Side effects:
 *	Increments the count of users of the swap stream.
 *
 * ----------------------------------------------------------------------------
 */
Fs_Stream *
VmGetSwapStreamPtr()
{
    Fs_Stream *streamPtr;
    LOCK_MONITOR;
    streamPtr = vmSwapStreamPtr;
    if (streamPtr != (Fs_Stream *)NIL) {
	swapStreamUsers++;
    }
    UNLOCK_MONITOR;
    return(streamPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmDoneWithSwapStreamPtr --
 *
 *	This is called after the swap stream has been used, either to
 *	open a swap file or to wait for recovery.
 *	
 *
 * Results:
 *	None.  
 *
 * Side effects:
 *	Decrements the number of users of the swap stream.
 *
 * ----------------------------------------------------------------------------
 */
void
VmDoneWithSwapStreamPtr()
{
    LOCK_MONITOR;
    swapStreamUsers--;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmSwapStreamOk --
 *
 *	Returns TRUE if it is ok to use the swap stream for recovery.
 *	This is called after a PageWrite has failed.  Because this just
 *	returns a snapshot of the vmSwapStreamPtr the following scenarios
 *	are possible.  If the vmSwapStreamPtr gets closed after we return
 *	"ok", then the sharers of the Vm segment won't have been killed,
 *	but the page will still be on the dirty list, so they'll die
 *	the next time the page gets written out.
 *
 * Results:
 *	TRUE or FALSE.  
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
VmSwapStreamOk()
{
    Boolean ok;
    LOCK_MONITOR;
    ok = (vmSwapStreamPtr != (Fs_Stream *)NIL);
    UNLOCK_MONITOR;
    return(ok);
}

