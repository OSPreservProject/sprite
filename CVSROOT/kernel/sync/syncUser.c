/* 
 * syncUser.c --
 *
 *	These are system call routines of the Synchronization module that
 *	support monitors for user-level code.
 *
 *	A process is blocked by making it wait on an event.  An event is
 *	just an uninterpreted integer that gets 'signaled' by the routine
 *	Sync_UserSlowBroadcast.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sync.h"
#include "syncInt.h"
#include "sched.h"
#include "sys.h"
#include "vm.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_SlowLockStub --
 *
 *	Stub for the Sync_SlowLock system call.
 *	Acquire a lock while holding the synchronization master lock.
 *
 *      Inside the critical section the inUse bit is checked.  If we have
 *      to wait the process is put to sleep waiting on an event associated
 *      with the lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The lock is acquired when this procedure returns.  The process may
 *      have been put to sleep while waiting for the lock to become
 *      available.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Sync_SlowLockStub(lockPtr)
    Sync_Lock *lockPtr;
{
    Sys_SetJumpState	setJumpState;
    ReturnStatus	status = SUCCESS;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());

    /*
     * We have to use set-jump instead of MakeAccessible because we would have 
     * to sleep for an indeterminate amount of time with something made
     * accessible which is not a good idea.  Note that we assume that a user
     * pointer is directly accessible from the kernel.
     */
    if (Sys_SetJump(&setJumpState) == SUCCESS) {
	MASTER_LOCK(sched_Mutex);

	while (Mach_TestAndSet(&(lockPtr->inUse)) != 0) {
	    lockPtr->waiting = TRUE;
	    (void) SyncEventWaitInt((unsigned int)lockPtr, TRUE);
	    MASTER_UNLOCK(sched_Mutex);
	    MASTER_LOCK(sched_Mutex);
	    if (Sig_Pending(procPtr)) {
		status = GEN_ABORTED_BY_SIGNAL;
		break;
	    }
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }
    MASTER_UNLOCK(sched_Mutex);
    Sys_UnsetJump();
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_SlowWaitStub --
 *
 *	Stub for the Sync_SlowWait system call.
 *      Wait on an event.  The lock is released and the process is blocked
 *      on the event.  A future call to Sync_UserSlowBroadcast will signal the
 *      event and make this process runnable again.
 *
 *      This can only be called while a lock is held.  This forces our
 *      client to safely check global state while in a monitor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Put the process to sleep and release the monitor lock.  Other
 *      processes waiting on the monitor lock become runnable.
 *	
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Sync_SlowWaitStub(event, lockPtr, wakeIfSignal)
    unsigned 	int 	event;
    Sync_Lock 		*lockPtr;
    Boolean		wakeIfSignal;
{
    ReturnStatus	status;
    Sys_SetJumpState	setJumpState;

    /*
     * We have to use set-jump instead of MakeAccessible because we would have 
     * to sleep for an indeterminate amount of time with something made
     * accessible which is not a good idea.  Note that we assume that a user
     * pointer is directly accessible from the kernel.
     */
    if (Sys_SetJump(&setJumpState) == SUCCESS) {
	MASTER_LOCK(sched_Mutex);
	/*
	 * release the monitor lock and wait on the condition
	 */
	lockPtr->inUse = 0;
	lockPtr->waiting = FALSE;
	SyncEventWakeupInt((unsigned int)lockPtr);

	if (SyncEventWaitInt(event, wakeIfSignal)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	} else {
	    status = SUCCESS;
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }
    MASTER_UNLOCK(sched_Mutex);
    Sys_UnsetJump();
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_SlowBroadcastStub --
 *
 *	Stub for the Sync_SlowBroadcast system call.
 *      Mark all processes waiting on an event as runable.  The flag that
 *      indicates there are waiters is cleared here inside the protected
 *      critical section.  This has "broadcast" semantics because everyone
 *      waiting is made runable.  We don't yet have a mechanism to wake up
 *      just one waiting process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Make processes waiting on the event runnable.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Sync_SlowBroadcastStub(event, waitFlagPtr)
    unsigned int event;
    int *waitFlagPtr;
{
    int *newWaitFlagPtr;
    int len;

    Vm_MakeAccessible(VM_READWRITE_ACCESS, 
			sizeof(int), (Address) waitFlagPtr, 
			&len, (Address *) &newWaitFlagPtr);
    if (len != sizeof(int)) {
	return(SYS_ARG_NOACCESS);
    }

    MASTER_LOCK(sched_Mutex);

    *newWaitFlagPtr = FALSE;
    SyncEventWakeupInt(event);

    MASTER_UNLOCK(sched_Mutex);

    Vm_MakeUnaccessible((Address) newWaitFlagPtr, len);
    return(SUCCESS);
}
