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
#include "devTypesInt.h"

/*
 * Per device include files.
 */
#include "sii.h"

/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[1];
#if 0
= {
   /* Name	Address		ID	InitProc	IntrRoutine. */
};
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);
#endif
int devNumConfigCntrlrs = 0;

/*
 * Table of SCSI HBA types attached to this system.
 */

ScsiDevice *((*devScsiAttachProcs[1])());
#if 0
= {
    DevSIIAttachDevice,		/* SCSI Controller type 0. */
};
int devScsiNumHBATypes = sizeof(devScsiAttachProcs) / 
			 sizeof(devScsiAttachProcs[0]);
#endif
int devScsiNumHBATypes = 0;

/*
 * A list of disk devices that is used when probing for a root partition.
 * Note that we put the default partition as partition C so as to use the
 * entire disk.
 */
Fs_Device devFsDefaultDiskPartitions[] = {
    { -1, SCSI_MAKE_DEVICE_TYPE(DEV_SCSI_DISK, DEV_SII_HBA, 0, 0, 0, 0),
          SCSI_MAKE_DEVICE_UNIT(DEV_SCSI_DISK, DEV_SII_HBA, 0, 0, 0, 0),
                (ClientData) NIL },
    { -1, SCSI_MAKE_DEVICE_TYPE(DEV_SCSI_DISK, DEV_SII_HBA, 0, 0, 0, 2),
          SCSI_MAKE_DEVICE_UNIT(DEV_SCSI_DISK, DEV_SII_HBA, 0, 0, 0, 2),
                (ClientData) NIL },
    };
int devNumDefaultDiskPartitions = sizeof(devFsDefaultDiskPartitions) /
                          sizeof(Fs_Device);

