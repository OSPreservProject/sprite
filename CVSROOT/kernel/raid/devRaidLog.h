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

#ifndef _DEVRAIDLOG
#define _DEVRAIDLOG

#include "sync.h"
#include <sprite.h>
#include "devRaid.h"
#include "miscutil.h"

#define RAID_LOG_BUF_SIZE	1000

/*
 * Location and size of log offsets on the log disk for configuration info,
 * disk state and parity-stripe state.
 */

/*
 * Minimum unit of log update.
 * The minumum unit of log update must be at least 8 words (32 bytes).
 */
#define MinUpdateSize(raidPtr)	\
	(MAX(32, (raidPtr)->log.logHandlePtr->minTransferUnit))
#define RoundToUpdateSize(raidPtr, bytes)	\
	(RoundUp(bytes, (raidPtr)->log.logHandlePtr->minTransferUnit))

#define ParamSize(raidPtr)	(MinUpdateSize(raidPtr))
#define ParamLoc(raidPtr)	((raidPtr)->log.logDevOffset)

#define DiskID(raidPtr, col, row)	((col)*(raidPtr)->numRow+(row))
#define DiskSize(raidPtr)	\
	(MinUpdateSize(raidPtr)*((raidPtr)->numCol)*((raidPtr)->numRow))
#define DiskLoc(raidPtr, col, row)	\
	((raidPtr)->log.logDevOffset + ParamSize(raidPtr) +	\
		MinUpdateSize(raidPtr)*DiskID(raidPtr, col, row))

#define VecSize(raidPtr)	\
	(RoundToUpdateSize(raidPtr, Bit_NumBytes((raidPtr)->numStripe)))
#define VecLoc(raidPtr)		\
	((raidPtr)->log.logDevOffset + ParamSize(raidPtr) + DiskSize(raidPtr))

#define LogSize(raidPtr)	\
	(ParamSize(raidPtr) + DiskSize(raidPtr) + VecSize(raidPtr))

typedef struct {
    Sync_Semaphore	 mutex;
    int			 enabled;
    int			 busy;
    Fs_Device		 logDev;
    int			 logDevOffset;
    int			 logDevEndOffset;
    DevBlockDeviceHandle *logHandlePtr;
    int			*diskLockVec;	/* disk image of locked stripes */
    int			*lockVec;	/* actually locked stripes */
					/* the disk image is unlocked lazily */
    int			 numStripeLocked;
    Sync_Condition       flushed;
} RaidLog;

#endif /* _DEVRAIDLOG */
