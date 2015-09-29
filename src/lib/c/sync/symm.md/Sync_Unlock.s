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

    .data
    .asciz "$Header: Sync_Unlock.s 1.2 90/03/05 $ SPRITE (Berkeley)"
    .align 2
    .text

/* $Log:	Sync_Unlock.s,v $
 * Revision 1.2  90/03/05  14:42:30  rbk
 * Add call to Sync_SlowBroadcast() if "waiting".
 * Cleaned up a bit (use # in line comments), and loose save/restore of
 * scratch registers.
 * 
 * Revision 1.1  90/03/05  10:13:44  rbk
 * Initial revision
 *  
 * Based on Sync_Unlock.s,v 1.1 88/06/19 14:34:19 ouster
 */

#include "kernel/machAsmDefs.h"


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

ENTRY(Sync_Unlock)
	movl	SPARG0, %ecx		# ecx = lockPtr.
	xorl	%eax, %eax		# eax = 0
	xchgl	%eax, (%ecx)		# release the lock
	cmpl	$0, 4(%ecx)		# is waiting set?
	jne	1f			# yup -- call kernel
	RETURN				# nope; done.
	/*
	 * Note the broadcast semantics for Sync_SlowBroadcast.
	 * All processes waiting on the lock will be made runnable,
	 * however, all but one will sleep again inside Sync_SlowLock.
	 */
1:	leal	4(%ecx), %eax		# push ...
	pushl	%eax			#	&(lockPtr->waiting)
	pushl	%ecx			# lockPtr (event)
	CALL	_Sync_SlowBroadcast	# Sync_SlowBroadcast(lockPtr, &waiting)
	addl	$8, %esp		# clear stack.
	RETURN				# done.
