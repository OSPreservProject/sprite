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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "devTypesInt.h"


/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[1];
   /* Name     Address,  Addr space, ID, InitProc   IntrVector  IntrRoutine. */

int devNumConfigCntrlrs = 0;

/*
 * Table of SCSI HBA types attached to this system.
 */

ScsiDevice *((*devScsiAttachProcs[1])());
int devScsiNumHBATypes = 0;

/*
 * A list of disk devices that is used when probing for a root partition.
 * The choices are:
 */
Fs_Device devFsDefaultDiskPartitions[1];

int devNumDefaultDiskPartitions = 0;

