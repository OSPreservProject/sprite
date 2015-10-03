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

#define SyncAddLockStats(regPtr, lock) 					\
    { 									\
	 (regPtr)->hit += FIELD(lock, hit); 				\
	(regPtr)->miss += FIELD(lock, miss); 				\
	if (((Sync_Lock *) lock)->type == SYNC_LOCK) { 			\
	    SyncMergePrior((Sync_Lock *) (lock), (regPtr));		\
	} else {							\
	    SyncMergePrior((Sync_Semaphore *) (lock), (regPtr));	\
	}								\
    }

extern void SyncEventWakeupInt _ARGS_((unsigned int event));
extern Boolean SyncEventWaitInt _ARGS_((unsigned int event, 
					Boolean wakeIfSignal));

/*
 *----------------------------------------------------------------------
 *
 * FIELD --
 *
 *	Some of the routines take either semaphores or locks as parameters.
 *	This macro is used to get the desired field from the object,
 *	regardless of its type.
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
