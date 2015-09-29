/* 
 * devConfig.c --
 *
 *	Configuration table for the devices in the system.  There is
 *	a table for the possible controllers in the system, and
 *	then a table for devices.  Devices are implicitly associated
 *	with a controller.  This file should be automatically generated
 *	by a config program, but it isn't.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/sun3.md/RCS/devConfig.c,v 8.6 89/05/24 07:50:35 rab Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "boot.h"
#include "devInt.h"
#include "devTypesInt.h"
#include "fs.h"

/*
 * Per device include files.
 */
#if defined(SCSI_DISK_BOOT) || defined(SCSI_TAPE_BOOT)
#include "scsiHBA.h"
#ifdef SCSI3_BOOT
#include "scsi3.h"
#endif
#ifdef SCSI0_BOOT
#include "scsi0.h"
#endif
#endif
#ifdef XYLOGICS_BOOT
#include "xylogics450.h"
#endif

/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[] = {
   /* Name     Address,  Addr space, ID, InitProc   IntrVector  IntrRoutine. */
#ifdef SCSI3_BOOT
#ifdef SCSI3_ONBOARD
    { "SCSI3", 0x0FE12000,	DEV_OBIO,  0, DevSCSI3Init,26, DevSCSI3Intr },
#else
    { "SCSI3",  0x200000, DEV_VME_D16A24, 0, DevSCSI3Init, 64, DevSCSI3Intr },
#endif
#endif
#ifdef SCSI0_BOOT
    { "SCSI0",  0x200000, DEV_VME_D16A24, 0, DevSCSI0Init, 64, DevSCSI0Intr },
#endif
#ifdef XYLOGICS_BOOT
    { "Xylogics450", 0xee40, DEV_VME_D16A16,	 0, DevXylogics450Init,
#endif

};
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);

/*
 * Table of SCSI HBA types attached to this system.
 */

ScsiDevice *((*devScsiAttachProcs[])()) = {
#ifdef SCSI3_BOOT
    DevSCSI3AttachDevice,		/* SCSI Controller type 0. */
#endif
#ifdef SCSI0_BOOT
    DevSCSI0AttachDevice,		/* SCSI Controller type 1. */
#endif
};
int devScsiNumHBATypes = sizeof(devScsiAttachProcs) / 
			 sizeof(devScsiAttachProcs[0]);

/*
 * A list of disk devices that is used when probing for a root partition.
 * The choices are:
 * Drive 0 partition 0 of xylogics 450 controller 0.
 * SCSI Disk target ID 0 LUN 0 partition 0 on SCSI0 HBA 0.
 * SCSI Disk target ID 0 LUN 0 partition 0 on SCSI3 HBA 0. 
 */
Fs_Device devFsDefaultDiskPartitions[] = { 
#ifdef XYLOGICS_BOOT
    { -1, DEV_XYLOGICS, 0, (ClientData) NIL },	
#endif
#if defined(SCSI0_BOOT) || defined(SCSI3_BOOT)
    { -1, SCSI_MAKE_DEVICE_TYPE(DEV_SCSI_DISK, 0, 0, 0, 0, 0),
	  SCSI_MAKE_DEVICE_UNIT(DEV_SCSI_DISK, 0, 0, 0, 0, 0),
		(ClientData) NIL }, 
#endif
};
int devNumDefaultDiskPartitions = sizeof(devFsDefaultDiskPartitions) / 
			  sizeof(Fs_Device);


