/* 
 * semaphore.c --
 *
 *	Implements semaphores.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "semaphore.h"


/*
 *----------------------------------------------------------------------
 *
 * InitSema
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
InitSema(semaPtr, name, val)
    Sema	*semaPtr;
    char	*name;
    int		val;
{
    Sync_SemInitDynamic(&semaPtr->mutex, name);
    semaPtr->val = val;
#ifdef TESTING
    Sync_CondInit(&semaPtr->wait);
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * DownSema
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
DownSema(semaPtr)
    Sema	*semaPtr;
{
    MASTER_LOCK(&semaPtr->mutex);
    while (semaPtr->val <= 0) {
	Sync_MasterWait(&semaPtr->wait, &semaPtr->mutex, FALSE);
    }
    semaPtr->val--;
    MASTER_UNLOCK(&semaPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * UpSema
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
UpSema(semaPtr)
    Sema	*semaPtr;
{
    MASTER_LOCK(&semaPtr->mutex);
    semaPtr->val++;
    Sync_MasterBroadcast(&semaPtr->wait);
    MASTER_UNLOCK(&semaPtr->mutex);
}
