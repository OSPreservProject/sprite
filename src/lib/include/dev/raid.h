/* 
 * raid.h --
 *
 *	Declarations for RAID ioctl's.
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
 * $Header: /user4/eklee/raidlib/RCS/raid.h,v 1.1 90/03/05 11:39:09 eklee Exp Locker: eklee $ SPRITE (Berkeley)
 */

#ifndef _RAID
#define _RAID

typedef struct RaidIOCParam {
    int		row;
    int		col;
    int		type;
    int		unit;
    int		startStripe;
    int		numStripe;
    int		uSec;
    int		ctrlData;
    char	buf[1];
} RaidIOCParam;

#define RAID_IOC_MASK			0xFFFF
#define IOC_DEV_RAID			(10 << 16)
#define IOC_DEV_RAID_PRINT		(IOC_DEV_RAID | 3)
#define IOC_DEV_RAID_RECONFIG		(IOC_DEV_RAID | 4)
#define IOC_DEV_RAID_HARDINIT		(IOC_DEV_RAID | 5)
#define IOC_DEV_RAID_FAIL		(IOC_DEV_RAID | 6)
#define IOC_DEV_RAID_REPLACE		(IOC_DEV_RAID | 7)
#define IOC_DEV_RAID_RECONSTRUCT	(IOC_DEV_RAID | 8)
#define IOC_DEV_RAID_IO			(IOC_DEV_RAID | 9)
#define IOC_DEV_RAID_LOCK		(IOC_DEV_RAID | 10)
#define IOC_DEV_RAID_UNLOCK		(IOC_DEV_RAID | 11)
#define IOC_DEV_RAID_SAVE_STATE		(IOC_DEV_RAID | 12)
#define IOC_DEV_RAID_ENABLE_LOG		(IOC_DEV_RAID | 13)
#define IOC_DEV_RAID_DISABLE_LOG	(IOC_DEV_RAID | 14)
#define IOC_DEV_RAID_PARITYCHECK	(IOC_DEV_RAID | 15)
#define IOC_DEV_RAID_RESTORE_STATE	(IOC_DEV_RAID | 16)
#define IOC_DEV_RAID_DISABLE		(IOC_DEV_RAID | 17)
#define IOC_DEV_RAID_ENABLE		(IOC_DEV_RAID | 18)

#endif _RAID
