/* 
 * devRaidLock.h --
 *
 *	Declarations for RAID device drivers.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVRAIDLOCK
#define _DEVRAIDLOCK

#include "devRaid.h"

typedef struct {
    Sync_Semaphore	mutex;
    int			val;
    Sync_Condition	wait;
} Sema;

#define LockSema(sema) (DownSema(sema))
#define UnlockSema(sema) (UpSema(sema))

#endif /* _DEVRAIDLOCK */
