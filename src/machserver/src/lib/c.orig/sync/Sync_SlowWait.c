/* 
 * Sync_SlowWait.c --
 *
 *	Source code for the Sync_SlowWait library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/sync/RCS/Sync_SlowWait.c,v 1.2 91/03/14 12:16:01 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <sync.h>


/*
 * ----------------------------------------------------------------------------
 *
 * Sync_SlowWait --
 *
 *      Wait on a condition.  Set waiting flag and perform system call to 
 *	release the monitor lock and do the wait.  Reaquire the lock before
 *	returning.
 *
 * Results:
 *	TRUE if interrupted because of signal, FALSE otherwise.
 *
 * Side effects:
 *      Put the process to sleep and release the monitor lock.
 *	
 * ----------------------------------------------------------------------------
 */

Boolean
Sync_SlowWait(conditionPtr, lockPtr, wakeIfSignal)
    Sync_Condition	*conditionPtr;	/* Condition to wait on. */
    register Sync_Lock 	*lockPtr;	/* Lock to release. */
    Boolean		wakeIfSignal;	/* TRUE => wake if signal pending. */
{
    ReturnStatus	status;

    if (!lockPtr->inUse) {
	panic("Sync_SlowWait: monitor lock not held");
    }

    conditionPtr->waiting = TRUE;

    status = Sync_SlowWaitStub(conditionPtr, lockPtr, wakeIfSignal);

    Sync_GetLock(lockPtr);

    return(status);
}
