/*
 * syncAsm.s --
 *
 *	Source code for the Sync_Unlock library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

    .seg	"data"
    .asciz "$Header: /sprite/src/lib/c/sync/sun4.md/RCS/Sync_Unlock.s,v 1.1 89/02/24 17:03:13 mgbaker Exp Locker: mgbaker $ SPRITE (Berkeley)"
    .align	8
    .seg	"text"

/*
 *----------------------------------------------------------------------------
 *
 * Sync_Unlock --
 *
 *      Release a lock.  This is called at the end of a critical
 *      section of code to allow other processes to execute within the
 *      critical section.  If any processes are waiting to acquire this
 *      lock they are made runnable.  They will try to gain the lock
 *      again the next time they run.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock is cleared.  Processes waiting on the lock are made runnable.
 *
 * C equivalent:
 *
 *	void
 *	Sync_Unlock(lockPtr)
 *	    Sync_Lock *lockPtr;
 *	{
 *	    lockPtr->inUse = 0;
 *	    if (lockPtr->waiting) {
 *		Sync_SlowBroadcast((int)lockPtr, &lockPtr->waiting);
 *	    }
 *	}
 *
 *----------------------------------------------------------------------------
 */

.seg	"text"
.globl _Sync_Unlock
_Sync_Unlock:

    /* prologue */
    set		(-104), %g1
    save	%sp, %g1, %sp
    /* end prologue */

    st		%g0, [%i0]	/* lockPtr->inUse = 0; */

    /*
     * The check on the waiting bit races with the assignment
     * statement that clears it in Sync_SlowBroadcast. In the
     * worst case we assume someone is waiting that is really
     * just waking up because we've cleared the inUse bit.
     * That results in a wasted call to Sync_SlowBroadcast.
     */

    ld		[%i0 + 4], %g1		/* get lockPtr->waiting */
    tst		%g1			/* if (lockPtr->waiting) { */
    be		BailOut			/*  Bail out if !lockPtr->waiting */

    /*
     * Note the broadcast semantics for Sync_SlowBroadcast.
     * All processes waiting on the lock will be made runnable,
     * however, all but one will sleep again inside Sync_SlowLock.
     */
    nop
    mov		%i0, %g1	/* get address of lockPtr->waiting */
    add		%g1, 4, %g1
    mov		%i0, %o0	/* first arg is lockPtr */
    mov		%g1, %o1	/* next arg is &lockPtr->waiting */
    call	_Sync_SlowBroadcast,2
    nop

BailOut:
    ret
    restore
