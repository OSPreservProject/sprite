|*
|* syncAsm.s --
|*
|*	Source code for the Sync_Unlock library procedure.
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
    .asciz "$Header: /sprite/src/lib/c/sync/sun3.md/RCS/Sync_Unlock.s,v 1.2 90/02/12 02:53:02 rab Exp $ SPRITE (Berkeley)"
    .even
    .text


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

    .text
    .globl _Sync_Unlock
_Sync_Unlock:

    movl	sp@(4), a0	| a0 = lockPtr
    clrl	a0@		| lockPtr->inUse = 0;

    /*
     * The check on the waiting bit races with the assignment
     * statement that clears it in Sync_SlowBroadcast. In the
     * worst case we assume someone is waiting that is really
     * just waking up because we've cleared the inUse bit.
     * That results in a wasted call to Sync_SlowBroadcast.
     */

    tstl	a0@(4)		| if (lockPtr->waiting) {
    jeq		1f		| Bail out if !lockPtr->waiting

    /*
     * Note the broadcast semantics for Sync_SlowBroadcast.
     * All processes waiting on the lock will be made runnable,
     * however, all but one will sleep again inside Sync_SlowLock.
     */

    movl	a0, d0
    addql	#4, d0		| Get address of lockPtr->waiting.
    movl	d0, sp@-	| Push &lockPtr->waiting on the stack.
    movl	a0, sp@-	| Push lockPtr on the stack.
    jbsr	_Sync_SlowBroadcast
    addql	#8, sp		| Pop the stack.

1:  rts
