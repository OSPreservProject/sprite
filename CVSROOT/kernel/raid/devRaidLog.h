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

#include "fs.h"
#include "sync.h"

#define RAID_LOG_BUF_SIZE	1000

typedef struct {
    Sync_Semaphore	 mutex;
    int			 enabled;
    int			 busy;
    Sync_Condition       notBusy;
    Fs_Stream		*streamPtr;
    char		*curBufPtr;
    char		*curBuf;
    char		 buf1[RAID_LOG_BUF_SIZE];
    char		 buf2[RAID_LOG_BUF_SIZE];
    Sync_Condition	*curBufFlushedPtr;
    Sync_Condition       flushed1;
    Sync_Condition       flushed2;
} RaidLog;

extern void EnableLog();
extern void DisableLog();
extern void LogEntry();

#endif _DEV_RAID_LOG
