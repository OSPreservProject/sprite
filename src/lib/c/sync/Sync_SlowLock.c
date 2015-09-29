/* 
 * Sync_SlowLock.c --
 *
 *	Source code for the Sync_SlowLock library procedure.
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

#ifndef lint
static char rcsid[] = "$Header: Sync_SlowLock.c,v 1.3 88/07/25 11:12:00 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <stdio.h>
#include <sync.h>


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_SlowLock --
 *
 *      Aquire a lock.  This is the second level lock routine.  Sync_GetLock
 *	has already been called and it discovered that the lock is already
 *	held by someone else.  This routine performs the system call to
 *	go into the kernel and go through the slower locking procedure.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *      Lock aquired on exit.
 *	
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Sync_SlowLock(lockPtr)
    Sync_Lock	*lockPtr;
{
    ReturnStatus	status;

    do {
	status = Sync_SlowLockStub(lockPtr);
    } while (status == GEN_ABORTED_BY_SIGNAL);
    if (status != SUCCESS) {
	panic("Sync_SlowLock: Could not aquire lock\n");
    }
    return(SUCCESS);
}
