/* 
 * devRaidLog.h --
 *
 *	Implements logging and recovery for raid devices.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef _DEV_RAID_LOG
#define _DEV_RAID_LOG

#include "sync.h"
#include "bitvec.h"
#include "devRaid.h"

#define RAID_LOG_BUF_SIZE	1000

typedef struct {
    Sync_Semaphore	 mutex;
    int			 enabled;
    int			 busy;
    DevBlockDeviceHandle *logHandlePtr;
    int			 logDevOffset;
    int			 diskLockVecNum;
    int			 diskLockVecSize;
    BitVec		 diskLockVec;
    int			 minLogElem;
    int			 maxLogElem;
    Sync_Condition	*waitCurBufPtr;
    Sync_Condition	*waitNextBufPtr;
    Sync_Condition       flushed1;
    Sync_Condition       flushed2;
} RaidLog;

#endif _DEV_RAID_LOG
