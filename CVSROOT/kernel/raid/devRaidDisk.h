/*
 * devRaidDisk.h --
 *
 *	Declarations of ...
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

#ifndef _DEVRAIDDISK
#define _DEVRAIDDISK

/*
 * Data structure for each disk used by raid device.
 *
 * RAID_DISK_INVALID	==> could not attach device
 * RAID_DISK_READY	==> device operational
 * RAID_DISK_FAILED	==> device considered failed (a write error occured)
 * RAID_DISK_REPLACED	==> the device is nolonger a part of the array
 * RAID_DISK_RECONSTRUCT==> the device is currently being reonstructed
 *				(IO's to the reconstructed part of the device
 *				 are allowed)
 */
typedef enum {
    RAID_DISK_INVALID, RAID_DISK_READY, RAID_DISK_FAILED, RAID_DISK_REPLACED,
    RAID_DISK_RECONSTRUCT
} RaidDiskState;

typedef struct RaidDisk {
    Sync_Semaphore	  mutex;
    RaidDiskState	  state;
    unsigned		  numValidSector; /* Used during reconstruction. */
    int			  version;
    int			  useCount;
    Fs_Device	          device;
    DevBlockDeviceHandle *handlePtr;
} RaidDisk;

/*
 * Is specified range of disk sectors "valid" on specified disk? 
 */
#define IsValid(diskPtr, startSector, numSector) 	\
    ((startSector) + (numSector) <= (diskPtr)->numValidSector)

extern void ReportRaidDiskAttachError();
extern void FailRaidDisk();
extern void ReplaceRaidDisk();

#endif _DEVRAIDDISK
