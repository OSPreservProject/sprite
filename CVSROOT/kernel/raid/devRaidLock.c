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
#include "hash.h"
#include "devRaid.h"
#include "debugMem.h"

extern char *malloc();

#define LOCK_TABLE_SIZE	4096

static Hash_Table lockTable;
static Sync_Semaphore mutex =
	Sync_SemInitStatic("devRaidLock.c: Stripe Lock Table");


/*
 *----------------------------------------------------------------------
 *
 * InitStripeLocks --
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
InitStripeLocks()
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
 * SLockStripe --
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
SLockStripe(raidPtr, stripe)
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
 * XLockStripe --
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
XLockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    char buf[120];

    SLockStripe(raidPtr, stripe);
    sprintf(buf, "L %d\n", stripe);
    LogEntry(raidPtr, buf);
}


/*
 *----------------------------------------------------------------------
 *
 * SUnlockStripe --
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
SUnlockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Hash_Entry		*hashEntryPtr;
    Sync_Condition	*condPtr;

    MASTER_LOCK(&mutex);
    hashEntryPtr = Hash_Find(&lockTable, (Address) stripe);
    MASTER_UNLOCK(&mutex);
#ifdef TESTING
    if ( Hash_GetValue(hashEntryPtr) == (char *) NIL ) {
	printf("Error: UnlockStripe: Attempt to unlock unlocked stripe.\n");
    }
#endif TESTING
    condPtr = (Sync_Condition *) Hash_GetValue(hashEntryPtr);
    Sync_MasterBroadcast(condPtr);
    MASTER_LOCK(&mutex);
    Hash_Delete(&lockTable, hashEntryPtr);
    MASTER_UNLOCK(&mutex);
    Free((char *) condPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * XUnlockStripe --
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
XUnlockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    char buf[120];

    sprintf(buf, "U %d\n", stripe);
    LogEntry(raidPtr, buf);
    SUnlockStripe(raidPtr, stripe);
}

/*
 *----------------------------------------------------------------------
 *
 * LockRaid --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Gains exclusive access to specified RAID device.
 *
 *----------------------------------------------------------------------
 */

int
LockRaid (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    while (raidPtr->numReqInSys != 0) {
	Sync_MasterWait(&raidPtr->waitExclusive, &raidPtr->mutex, FALSE);
    }
    raidPtr->numReqInSys = -1;
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * UnlockRaid --
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
UnlockRaid (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numReqInSys = 0;
    Sync_MasterBroadcast(&raidPtr->waitExclusive);
    Sync_MasterBroadcast(&raidPtr->waitNonExclusive);
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * BeginRaidUse --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
BeginRaidUse (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    while (raidPtr->numReqInSys == -1) {
	Sync_MasterWait(&raidPtr->waitNonExclusive, &raidPtr->mutex, FALSE);
    }
    raidPtr->numReqInSys++;
    MASTER_UNLOCK(&raidPtr->mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * EndRaidUse --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
EndRaidUse (raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numReqInSys--;
    if (raidPtr->numReqInSys == 0) {
	Sync_MasterBroadcast(&raidPtr->waitExclusive);
    }
    MASTER_UNLOCK(&raidPtr->mutex);
}
