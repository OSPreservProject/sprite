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

extern void InitStripeLocks();
extern void LockStripe();
extern void UnlockStripe();
extern int  LockRaid();
extern void UnlockRaid();
extern void BeginRaidUse();
extern void EndRaidUse();
