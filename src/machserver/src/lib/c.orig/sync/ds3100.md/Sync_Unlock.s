/*
 * Sync_Unlock.s --
 *
 *	Source code for the Sync_Unlock library procedure.
 *
 * Copyright (C) 1989 by Digital Equipment Corporation, Maynard MA
 *
 *			All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 *
 * Digitial disclaims all warranties with regard to this software, including
 * all implied warranties of merchantability and fitness.  In no event shall
 * Digital be liable for any special, indirect or consequential damages or
 * any damages whatsoever resulting from loss of use, data or profits,
 * whether in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of this
 * software.
 *
 * Header: Sync_GetLock.s,v 1.1 88/06/19 14:34:17 ouster Exp $ SPRITE (DECWRL)
 */

#ifdef KERNEL
#include <regdef.h>
#include "machConst.h"
#else
#include <regdef.h>
#include "kernel/machConst.h"
#endif


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
    .globl Sync_Unlock
Sync_Unlock:
    sw		zero, 0(a0)	# lockPtr->inUse = 0

    /*
     * The check on the waiting bit races with the assignment
     * statement that clears it in Sync_SlowBroadcast. In the
     * worst case we assume someone is waiting that is really
     * just waking up because we've cleared the inUse bit.
     * That results in a wasted call to Sync_SlowBroadcast.
     */
    lw		t0, 4(a0)	# if (lockPtr->waiting)
    beq		t0, zero, 1f	# Bail out if !lockPtr->waiting

    /*
     * Note the broadcast semantics for Sync_SlowBroadcast.
     * All processes waiting on the lock will be made runnable,
     * however, all but one will sleep again inside Sync_SlowLock.
     */
    add		a1, a0, 4	# Pass address of lockPtr->waiting as 2nd arg
    j		Sync_SlowBroadcast
1:  j 		ra
