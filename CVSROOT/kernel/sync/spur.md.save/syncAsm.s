/* syncAsm.s --
 *
 *	Routine to grab a monitor lock.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

#include "machConst.h"


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
 *	    if (Mach_TestAndSet(&(lockPtr->inUse)) != 0) {
 *		Sync_SlowLock(lockPtr); 
 *	    }
 *	}
 *
 *----------------------------------------------------------------------
 */

    .text
    .globl _Sync_GetLock
_Sync_GetLock:
    rd_kpsw		r17
    and			r18, r17, $~MACH_KPSW_INTR_TRAP_ENA
    wr_kpsw		r18, $0

#ifdef notdef
    ld_32		r19, r11, $0
    nop
    cmp_br_delayed	eq, r19, $0, 1f
    nop
    cmp_trap		always, r0, r0, $2
    nop

1:
#endif

    test_and_set	r16, r11, $0
    nop

    wr_kpsw		r17, $0

    cmp_br_delayed	eq, r16, $0, 1f
    add_nt		r27, r11, $0
    call		_Sync_SlowLock
    nop
1:
    return		r10, $8
    nop

