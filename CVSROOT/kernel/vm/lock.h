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

#include <sync.h>
#include <dbg.h>

/*
 * Monitor declarations
 */

extern	Sync_Lock vmMonitorLock;
#define LOCKPTR &vmMonitorLock

#define Dbg_Call	printf("SHM MONITOR PROBLEM\n")

extern Sync_Lock vmShmLock;
extern int vmShmLockCnt;

#if 0
#define LOCK_SHM_MONITOR	if (vmShmLockCnt>0) {Dbg_Call;} else {Sync_GetLock(&vmShmLock); vmShmLockCnt++;}
#define UNLOCK_SHM_MONITOR	if (vmShmLockCnt==0) {Dbg_Call;} else {Sync_Unlock(&vmShmLock); vmShmLockCnt--;}
#define CHECK_SHM_MONITOR	if (vmShmLockCnt!=1) Dbg_Call
#else
#define LOCK_SHM_MONITOR
#define UNLOCK_SHM_MONITOR
#define CHECK_SHM_MONITOR
#endif

#endif /* _VMLOCK */
