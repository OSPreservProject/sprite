/* 
 * devRaidLock.c --
 *
 *	Routines for locking and unlocking RAID stripes.
 *	Note that because of the manner in which these routines are used,
 *	stripes with the same number in different arrays (i.e. different
 *	unit numbers) will share the same lock.  Therefore, care must be
 *	taken to avoid unexpected deadlocks (i.e. a single process should never
 *	simultaneously lock two stripes in different raid devices).
 *	All locks are exclusive.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sync.h"
#include <sprite.h>
#include <stdio.h>
#include "hash.h"
#include "devRaid.h"
#include "semaphore.h"
#include "devRaidProto.h"

extern char *malloc();

#define LOCK_TABLE_SIZE	4096

static Hash_Table lockTable;
static Sync_Semaphore mutex =
	Sync_SemInitStatic("devRaidLock.c: Stripe Lock Table");


/*
 *----------------------------------------------------------------------
 *
 * Raid_InitStripeLocks --
 *
 *	Should be called at least once before calling any other procedure
 *	from this module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes stripe lock table.
 *
 *----------------------------------------------------------------------
 */

void
Raid_InitStripeLocks()
{
    static int	initialized = 0;

    MASTER_LOCK(&mutex);
    if (initialized == 0) {
	initialized = 1;
        MASTER_UNLOCK(&mutex);
	Hash_Init(&lockTable, 1000, 1);
    } else {
	MASTER_UNLOCK(&mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SLockStripe --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Share lock requested stripe.
 *
 *----------------------------------------------------------------------
 */

void
Raid_SLockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Hash_Entry		*hashEntryPtr;
    Sync_Condition	*condPtr;
    condPtr = (Sync_Condition *) Malloc(sizeof(Sync_Condition));

    MASTER_LOCK(&mutex);
    hashEntryPtr = Hash_Find(&lockTable, (Address) stripe);
    while ( Hash_GetValue(hashEntryPtr) != (char *) NIL ) {
	Sync_MasterWait((Sync_Condition *) Hash_GetValue(hashEntryPtr),
		&mutex, FALSE);
        hashEntryPtr = Hash_Find(&lockTable, (Address) stripe);
    }
#ifdef TESTING
    Sync_CondInit(condPtr);
#endif TESTING
    Hash_SetValue(hashEntryPtr, condPtr);
    MASTER_UNLOCK(&mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_XLockStripe --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Exclusively lock requested stripe.
 *
 *----------------------------------------------------------------------
 */
void
Raid_XLockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Raid_SLockStripe(raidPtr, stripe);
    Raid_LogStripe(raidPtr, stripe);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SUnlockStripe --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unlocks requested stripe.
 *
 *----------------------------------------------------------------------
 */

void
Raid_SUnlockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Hash_Entry		*hashEntryPtr;
    Sync_Condition	*condPtr;

    MASTER_LOCK(&mutex);
    hashEntryPtr = Hash_Find(&lockTable, (Address) stripe);
    if ( Hash_GetValue(hashEntryPtr) == (char *) NIL ) {
	MASTER_UNLOCK(&mutex);
	panic("Error: UnlockStripe: Attempt to unlock unlocked stripe.");
    }
    condPtr = (Sync_Condition *) Hash_GetValue(hashEntryPtr);
    Sync_MasterBroadcast(condPtr);
    Hash_Delete(&lockTable, hashEntryPtr);
    MASTER_UNLOCK(&mutex);
    Free((char *) condPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_XUnlockStripe --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unlocks requested stripe.
 *
 *----------------------------------------------------------------------
 */

void
Raid_XUnlockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Raid_SUnlockStripe(raidPtr, stripe);
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_Lock --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Gains exclusive access to specified RAID device.
 *
 *----------------------------------------------------------------------
 */

void
Raid_Lock (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numWaitExclusive++;
    while (raidPtr->numReqInSys != 0) {
	Sync_MasterWait(&raidPtr->waitExclusive, &raidPtr->mutex, FALSE);
    }
    raidPtr->numWaitExclusive--;
    raidPtr->numReqInSys = -1;
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_Unlock --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases exclusive access to specified RAID device.
 *
 *----------------------------------------------------------------------
 */

void
Raid_Unlock (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numReqInSys = 0;
    if (raidPtr->numWaitExclusive > 0) {
	Sync_MasterBroadcast(&raidPtr->waitExclusive);
    } else {
	Sync_MasterBroadcast(&raidPtr->waitNonExclusive);
    }
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_BeginUse --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
Raid_BeginUse (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    while (raidPtr->numReqInSys == -1 || raidPtr->numWaitExclusive > 0) {
	Sync_MasterWait(&raidPtr->waitNonExclusive, &raidPtr->mutex, FALSE);
    }
    raidPtr->numReqInSys++;
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_EndUse --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
Raid_EndUse (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numReqInSys--;
    if (raidPtr->numReqInSys == 0) {
	Sync_MasterBroadcast(&raidPtr->waitExclusive);
    }
    MASTER_UNLOCK(&raidPtr->mutex);
}
