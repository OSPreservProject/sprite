|*
|* syncAsm.s --
|*
|*	Source code for the Sync_GetLock library procedure.
|*
|* Copyright 1988 Regents of the University of California
|* Permission to use, copy, modify, and distribute this
|* software and its documentation for any purpose and without
|* fee is hereby granted, provided that the above copyright
|* notice appear in all copies.  The University of California
|* makes no representations about the suitability of this
|* software for any purpose.  It is provided "as is" without
|* express or implied warranty.
|*

    .data
    .asciz "$Header: /sprite/src/lib/c/sync/sun3.md/RCS/Sync_GetLock.s,v 1.2 90/02/12 02:52:36 rab Exp $ SPRITE (Berkeley)"
    .even
    .text


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

    .text
    .globl _Sync_GetLock
_Sync_GetLock:

    movl	sp@(4), a0	| Move address of lockPtr->inUse to a0.

    /*
     * This TestAndSet races with the assignment statement in Sync_Unlock
     * that clears the inUse bit.  The worst case is that we incorrectly
     * assume the lock is taken just as someone clears this bit.  This is
     * ok because we'll call Sync_SlowLock which does the check again
     * inside a protected critical section.
     */

    tas		a0@		| Test and set the inUse flag.
    beq		1f		| If it wasn't set then just return.

    jra		_Sync_SlowLock | If it was set then we must do a slow lock.
1:  rts
