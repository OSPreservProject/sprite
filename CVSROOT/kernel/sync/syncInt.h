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

extern 	void 	SyncSlowWait();
extern 	void 	SyncSlowLock();
extern 	void 	SyncSlowBroadcast();
extern	void	SyncEventWakeupInt();
extern	Boolean	SyncEventWaitInt();

#endif _SYNCINT
