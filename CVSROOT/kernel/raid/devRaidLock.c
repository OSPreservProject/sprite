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
#include "devRaidLock.h"
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
Raid_CheckPoint(raidPtr)
    Raid	*raidPtr;
{
    MASTER_LOCK(&raidPtr->log.mutex);
    if (!raidPtr->log.enabled) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    MASTER_UNLOCK(&raidPtr->log.mutex);
    printf("RAID:MSG:Checkpointing RAID\n");
    Raid_Lock(raidPtr);
#ifndef TESTING
    ClearBitVec(raidPtr->log.diskLockVec, raidPtr->log.diskLockVecNum);
#endif TESTING
    Raid_SaveLog(raidPtr);
    Raid_Unlock(raidPtr);
    printf("RAID:MSG:Checkpoint Complete\n");
}

#ifdef TESTING
#define NUM_LOG_STRIPE 2
#else
#define NUM_LOG_STRIPE 100
#endif

void
Raid_XLockStripe(raidPtr, stripe)
    Raid *raidPtr;
    int stripe;
{
    Raid_SLockStripe(raidPtr, stripe);
    if (!IsSet(raidPtr->log.diskLockVec, stripe)) {
	MASTER_LOCK(&raidPtr->mutex);
	raidPtr->numStripeLocked++;
	if (raidPtr->numStripeLocked % NUM_LOG_STRIPE == 0) {
	    MASTER_UNLOCK(&raidPtr->mutex);
	    Proc_CallFunc((void (*)
		    _ARGS_((ClientData clientData, Proc_CallInfo *callInfoPtr)))
		    Raid_CheckPoint, (ClientData) raidPtr, 0);
	} else {
	    MASTER_UNLOCK(&raidPtr->mutex);
	}
	Raid_LogStripe(raidPtr, stripe);
    }
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
