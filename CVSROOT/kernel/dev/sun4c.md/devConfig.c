/* 
 * devConfig.c --
 *
 *	Configuration table for the devices in the system.  There is
 *	a table for the possible controllers in the system, and
 *	then a table for devices.  Devices are implicitly associated
 *	with a controller.  This file should be automatically generated
 *	by a config program, but it isn't.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "devAddrs.h"
#include "devInt.h"
#include "scsiC90.h"
#include "devTypes.h"
#include "scsiHBA.h"

/*
 * Per device include files.
 */
/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[] = {
   /* Name     Address,  Addr space, ID, InitProc   IntrVector  IntrRoutine. */
#ifdef NOTDEF
/* This seems already to be mapped at a virtual address. */
   { "DMA_UNIT", DEV_DMA_ADDR, DEV_SBUS_OB, 0, (ClientData (*)()) NIL, 3,
	   DevSCSIC90Intr},
#endif
   { "SCSIC90", DEV_SCSI_ADDR, DEV_SBUS_OB, 0, DevSCSIC90Init, 3,
	   DevSCSIC90Intr},
};
/*
 * We want to treat the dma controller and scsi device as the same device,
 * but the both must be mapped separately, so we give them separate entries,
 * but with only one init routine.
 */
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);
/*
 * Table of SCSI HBA types attached to this system.
 */

ScsiDevice *((*devScsiAttachProcs[]) _ARGS_((Fs_Device *devicePtr,
		void (*insertProc) (List_Links *elementPtr,
				    List_Links *elementListHdrPtr)))) = {
    DevSCSIC90AttachDevice,		/* SCSI Controller type 0. */
};
int devScsiNumHBATypes = sizeof(devScsiAttachProcs) / 
			 sizeof(devScsiAttachProcs[0]);
/*
 * A list of disk devices that is used when probing for a root partition.
 * The choices are:
 * SCSI Disk target ID 0 LUN 0 partition 0 on SCSIC90 HBA 0. 
 */
Fs_Device devFsDefaultDiskPartitions[] = { 
    { -1, SCSI_MAKE_DEVICE_TYPE(DEV_SCSI_DISK, DEV_SCSIC90_HBA, 0, 0, 0, 2),
          SCSI_MAKE_DEVICE_UNIT(DEV_SCSI_DISK, DEV_SCSIC90_HBA, 0, 0, 0, 2),
	(ClientData) NIL },
  };
int devNumDefaultDiskPartitions = sizeof(devFsDefaultDiskPartitions) / 
			  sizeof(Fs_Device);
