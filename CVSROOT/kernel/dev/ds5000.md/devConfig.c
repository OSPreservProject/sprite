/* 
 * devConfig.c --
 *
 *	Configuration table for the devices in the system.  There is
 *	a table for the possible controllers in the system, and
 *	then a table for devices.  Devices are implicitly associated
 *	with a controller.  This file should be automatically generated
 *	by a config program, but it isn't.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "fs.h"
#include "devTypes.h"

/*
 * Per device include files.
 */
#include "scsiC90.h"

/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[] = {
   /* Name	Slot	ID	InitProc. */
    {"SCSI#0",   5, 0, DevSCSIC90Init},
    {"SCSI#1", 0, 1, DevSCSIC90Init},
    {"SCSI#2", 1, 2, DevSCSIC90Init},
    {"SCSI#3", 2, 3, DevSCSIC90Init},
};
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);

/*
 * Table of SCSI HBA types attached to this system.
 */

ScsiDevice *((*devScsiAttachProcs[]) ()) = {
    DevSCSIC90AttachDevice,		/* SCSI Controller type 0. */
};
int devScsiNumHBATypes = sizeof(devScsiAttachProcs) / 
			 sizeof(devScsiAttachProcs[0]);

/*
 * A list of disk devices that is used when probing for a root partition.
 * SCSI Disk target ID 0 LUN 0 partition 0 on SCSIC90 HBA 0. 
 */
#if 0
Fs_Device devFsDefaultDiskPartitions[] = {
    { -1, SCSI_MAKE_DEVICE_TYPE(DEV_SCSI_DISK, DEV_SCSIC90_HBA, 0, 0, 0, 2),
          SCSI_MAKE_DEVICE_UNIT(DEV_SCSI_DISK, DEV_SCSIC90_HBA, 0, 0, 0, 2),
	(ClientData) NIL },
  };
int devNumDefaultDiskPartitions = sizeof(devFsDefaultDiskPartitions) / 
			  sizeof(Fs_Device);
#endif
Fs_Device devFsDefaultDiskPartitions[1];
int devNumDefaultDiskPartitions = 0;

