/*
 * lock.h --
 *
 *	Header file to be used by files whose routines use the global vm
 *	monitor.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMLOCK
#define _VMLOCK

#include "sync.h"

/*
 * Monitor declarations
 */

extern	Sync_Lock vmMonitorLock;
#define LOCKPTR &vmMonitorLock

#endif /* _VMLOCK */
