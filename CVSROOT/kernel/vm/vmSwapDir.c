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
void
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
    Fs_Stream streamPtr;
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

