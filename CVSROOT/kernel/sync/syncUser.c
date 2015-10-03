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
 *	We pin a user's page into memory before touching it.  This
 *	simplifies locking and lets us hold the sched master lock
 *	while touching user memory.  
 *	
 *	The calls to Vm_MakeAccessible are assumed to succeed because
 *	the calls to Vm_PinUserMemory succeeded.  The calls to 
 *	Vm_MakeAccessible come before obtaining the sched master lock; 
 *	this shouldn't matter for native Sprite (Vm_MakeAccessible 
 *	shouldn't block), but it could make a difference for the
 *	Sprite server (on Mach).  The locks set by Vm_MakeAccessible
 *	should not be held while waiting for an event.
 *
 * Copyright 1986, 1991 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <sync.h>
#include <syncInt.h>
#include <sched.h>
#include <sys.h>
#include <vm.h>


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
    Sync_UserLock *lockPtr;
{
    ReturnStatus	status = SUCCESS;
    Proc_ControlBlock	*procPtr;
    Sync_UserLock	*userLockPtr; /* lock's addr in kernel addr space */
    int			numBytes; /* number of bytes accessible */
    Boolean		waiting = TRUE;

    procPtr = Proc_GetCurrentProc();

    status = Vm_PinUserMem(VM_READWRITE_ACCESS, sizeof(*lockPtr),
			(Address)lockPtr);
    if (status != SUCCESS) {
	return(status);
    }

    while (waiting) {
	Vm_MakeAccessible(VM_READWRITE_ACCESS, sizeof(*lockPtr),
			  (Address)lockPtr, &numBytes,
			  (Address *)&userLockPtr);
	MASTER_LOCK(sched_MutexPtr);
	waiting = (Mach_TestAndSet(&(userLockPtr->inUse)) != 0);
	if (waiting) {
	    userLockPtr->waiting = TRUE;
	}
	Vm_MakeUnaccessible((Address)userLockPtr, numBytes);
	if (!waiting) {
	    break;
	}

	(void) SyncEventWaitInt((unsigned int)lockPtr, TRUE);
#ifdef spur
	Mach_InstCountEnd(1);
#endif
	MASTER_UNLOCK(sched_MutexPtr);
	MASTER_LOCK(sched_MutexPtr);
	if (Sig_Pending(procPtr)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    break;
	}
	MASTER_UNLOCK(sched_MutexPtr);
    }
#ifdef spur
    if (Mach_InstCountIsOn(1)) {
	panic("About to unlock sched_Mutex with inst count on.\n");
    }
#endif

    /* 
     * We always exit the loop holding the sched master lock.
     */
    MASTER_UNLOCK(sched_MutexPtr);

    (void)Vm_UnpinUserMem(sizeof(*lockPtr), (Address)lockPtr);
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
 *      client to safely check global state while in a monitor.  The 
 *      caller of this system call (i.e., the Sync_SlowWait library
 *      routine) must reacquire the lock before returning.
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
    Sync_UserLock	*lockPtr;
    Boolean		wakeIfSignal;
{
    ReturnStatus	status;
    Sync_UserLock 	*userLockPtr; /* lock's addr in kernel addr space */
    int			numBytes; /* number of bytes accessible */

    status = Vm_PinUserMem(VM_READWRITE_ACCESS, sizeof(*lockPtr),
			(Address)lockPtr);
    if (status != SUCCESS) {
	return(status);
    }

    Vm_MakeAccessible(VM_READWRITE_ACCESS, sizeof(*lockPtr),
		      (Address)lockPtr, &numBytes,
		      (Address *)&userLockPtr);
    MASTER_LOCK(sched_MutexPtr);
    /*
     * release the monitor lock and wait on the condition
     */
    userLockPtr->inUse = 0;
    userLockPtr->waiting = FALSE;
    Vm_MakeUnaccessible((Address)userLockPtr, numBytes);
    SyncEventWakeupInt((unsigned int)lockPtr);

    if (SyncEventWaitInt(event, wakeIfSignal)) {
	status = GEN_ABORTED_BY_SIGNAL;
    } else {
	status = SUCCESS;
    }
#ifdef spur
    Mach_InstCountEnd(1);
#endif
    MASTER_UNLOCK(sched_MutexPtr);

    (void)Vm_UnpinUserMem(sizeof(*lockPtr), (Address)lockPtr);
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
    ReturnStatus	status;
    int		*userWaitFlagPtr; /* waitFlagPtr in kernel addr space */
    int		numBytes;	/* number of bytes accessible */

    status = Vm_PinUserMem(VM_READWRITE_ACCESS, sizeof(*waitFlagPtr), 
			(Address)waitFlagPtr);
    if (status != SUCCESS) {
	return(status);
    }
    Vm_MakeAccessible(VM_READWRITE_ACCESS, sizeof(*waitFlagPtr),
		      (Address)waitFlagPtr, &numBytes,
		      (Address *)&userWaitFlagPtr);

    MASTER_LOCK(sched_MutexPtr);

    *userWaitFlagPtr = FALSE;
    Vm_MakeUnaccessible((Address)userWaitFlagPtr, numBytes);
    SyncEventWakeupInt(event);

#ifdef spur
    if (Mach_InstCountIsOn(1)) {
	panic("About to unlock sched_Mutex with inst count on.\n");
    }
#endif
    MASTER_UNLOCK(sched_MutexPtr);

    (void)Vm_UnpinUserMem(sizeof(*waitFlagPtr), (Address)waitFlagPtr);

    return(SUCCESS);
}
