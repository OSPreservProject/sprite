/*
 * syncAsm.s --
 *
 *	Source code for the Sync_GetLock library procedure.
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
    .asciz "$Header: /sprite/src/lib/c/sync/sun4.md/RCS/Sync_GetLock.s,v 1.2 89/07/31 17:48:11 mgbaker Exp $ SPRITE (Berkeley)"
    .align	8
    .seg	"text"

/*
 * ----------------------------------------------------------------------------
 *
 * Sync_GetLock --
 *
 *	Acquire a lock.  Other processes trying to acquire the lock
 *	will block until this lock is released.
 *
 *      A critical section of code is protected by a lock.  To safely
 *      execute the code, the caller must first call Sync_GetLock to
 *      acquire the lock on the critical section.  At the end of the
 *      critical section the caller has to call Sync_Unlock to release
 *      the lock and allow other processes to execute in the critical
 *      section.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The lock is set.  Other processes will be blocked if they try
 *      to lock the same lock.  A blocked process will try to get the
 *      lock after this process unlocks the lock with Sync_Unlock.
 *
 * C equivalent:
 *
 *	void
 *	Sync_GetLock(lockPtr)
 *	   Sync_Lock *lockPtr;
 *	{
 *	    if (Sun_TestAndSet(&(lockPtr->inUse)) != 0) {
 *		Sync_SlowLock(lockPtr); 
 *	    }
 *	}
 *
 *----------------------------------------------------------------------
 */

.seg	"text"
.globl _Sync_GetLock
_Sync_GetLock:

    /*
     * This TestAndSet races with the assignment statement in Sync_Unlock
     * that clears the inUse bit.  The worst case is that we incorrectly
     * assume the lock is taken just as someone clears this bit.  This is
     * ok because we'll call Sync_SlowLock which does the check again
     * inside a protected critical section.
     */
    /* prologue */
    set		(-104), %g1
    save	%sp, %g1, %sp
    /* end prologue */
    mov		%i0, %o0		/* put ptr in arg for next call */
    ldstub	[%i0], %i0		/* Atomically get value and set it. */
    tst		%i0			/* Test it. */
    be,a	ReturnHappy		/* If not set, just return. */
    nop
    call	_Sync_SlowLock,1
    nop
ReturnHappy:
    ret
    restore
