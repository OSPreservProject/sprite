/* * syncInt.h --
 *
 *	Declarations of internal procedures of the sync module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SYNCINT
#define _SYNCINT

/*
 *----------------------------------------------------------------------
 *
 * SyncAddLockStats --
 *
 *	Adds the statistics for the given lock to the type total.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *	
 *----------------------------------------------------------------------
 */

#define SyncAddLockStats(regPtr, lock) \
    { \
	 (regPtr)->hit += FIELD(lock, hit); \
	(regPtr)->miss += FIELD(lock, miss); \
	SyncMergePrior(FIELD(lock, priorCount), FIELD(lock, priorTypes), \
	(regPtr));}

extern 	void 	SyncSlowWait();
extern 	void 	SyncSlowLock();
extern 	void 	SyncSlowBroadcast();
extern	void	SyncEventWakeupInt();
extern	Boolean	SyncEventWaitInt();

/*
 *----------------------------------------------------------------------
 *
 * FIELD --
 *
 *	Used to get a field from either a lock or a semaphore.
 *
 * Results:
 *	Value of the field.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define FIELD(lock,name) ( \
    ((Sync_Lock *) (lock))->class == SYNC_SEMAPHORE ? \
    ((Sync_Semaphore *) (lock))->name : \
    ((Sync_Lock *) (lock))->name ) 

#endif /* _SYNCINT */
