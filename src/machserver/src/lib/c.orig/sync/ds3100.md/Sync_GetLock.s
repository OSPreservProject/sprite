/*
 * Sync_GetLock.s --
 *
 *	Source code for the Sync_GetLock library procedure.
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
    .globl Sync_GetLock
Sync_GetLock:
#ifdef KERNEL
    mfc0	t0, MACH_COP_0_STATUS_REG
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    lw		t1, 0(a0)			# Read out old value
    li		t2, 1
    sw		t2, 0(a0)			# Set value.
    mtc0	t0, MACH_COP_0_STATUS_REG	# Restore interrupts.
    beq		t1, zero, 1f			# If not set then return
    j		Sync_SlowLock			# Missed on the lock.
1:  j		ra
#else
    li		t0, 1
    sw		t0, 0(a0)
    j		ra
#endif
