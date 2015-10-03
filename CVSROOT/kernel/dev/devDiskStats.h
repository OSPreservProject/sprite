/*
 * devDiskStats.h --
 *
 *	Declarations of routines for collecting statistics on Sprite disk 
 *	usage.
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

#ifndef _DISKSTATS
#define _DISKSTATS

#include <user/sysStats.h>
#include <user/fs.h>

/*
 * This structure is used for disk stats instead of a straignt Sys_DiskStats
 * because otherwise for some types of disks (SCSI), there is no place to
 * keep the busy info.  This field is wasted on the xylogics.
 */
typedef struct  DevDiskStats {
    Sync_Semaphore	mutex;		/* syncrhonize stat updates */
    int         	busy;		/* For idle check. */
    Sys_DiskStats 	diskStats;	/* Stat structure of device. */
} DevDiskStats;


/* procedures */

extern DevDiskStats *DevRegisterDisk _ARGS_((Fs_Device *devicePtr,
    char *deviceName,
    Boolean (*idleCheck) _ARGS_ ((ClientData clientData,
                                DevDiskStats *diskStatsPtr)),
    ClientData clientData));
extern void DevDiskUnregister _ARGS_((DevDiskStats *diskStatsPtr));
extern void DevPrintIOStats _ARGS_((Timer_Ticks time, ClientData clientData));
extern void Dev_StartIOStats _ARGS_((void));
extern void Dev_StopIOStats _ARGS_((void));

#endif /* _DISKSTATS */

