/* 
 * devConfig.c --
 *
 *	Configuration table for the devices in the system.  Devices fall
 *	into two classes: those that are slaves of a controller,
 *	and those that aren't.  As such there is a configuration table for
 *	controllers and one for devices.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "devInt.h"
/*
 * Per device include files.
 */
#include "devSCSI.h"
#include "devXylogics.h"

/*
 * The controller configuration table.
 */
DevConfigController devCntrlr[] = {
/*   address,  space, ID, initProc */
    { "SCSI", 0x80000, DEV_MULTIBUS,	0, Dev_SCSIInitController, 0, 0},
/*  { "SCSI", 0x84000, DEV_MULTIBUS,	1, Dev_SCSIInitController, 0, 0}, */
    { "SCSI", 0x200000, DEV_VME_D16A24, 0, Dev_SCSIInitController, 64,
					   Dev_SCSIIntrStub},
    { "Xylogics", 0xee40, DEV_VME_D16A16,	 0, Dev_XylogicsInitController,
						72, Dev_XylogicsIntrStub},
    { "Xylogics", 0xee48, DEV_VME_D16A16,	 1, Dev_XylogicsInitController,
						73, Dev_XylogicsIntrStub},
};
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);

/*
 * The device configuration table.  Device entries for non-existent controllers
 * don't cost any time at startup.  Non-existent devices on existing
 * controllers can cost an I/O bus timeout period at start up.
 *
 * NB: There is an implicit correspondence between filesystem unit numbers
 * and particular disks.  Several entries for the same kind of device
 * result in a correspondence between those devices and ranges of unit
 * numbers. For disks, there are FS_NUM_DISK_PARTS different unit numbers
 * associated with each disk, and successive disks have consequative
 * ranges of unit numbers.
 */
DevConfigDevice devDevice[] = {
/* cntrlrID, slaveID, flags, initproc */
    { 0, 0, DEV_SCSI_DISK, Dev_SCSIInitDevice},		/* Units 0-7 */
/*  { 0, 1, DEV_SCSI_DISK, Dev_SCSIInitDevice}, */
    { 0, 4, DEV_SCSI_TAPE, Dev_SCSIInitDevice},

/*  { 1, 0, DEV_SCSI_DISK, Dev_SCSIInitDevice},		/* Units 8-15 */
/*  { 1, 1, DEV_SCSI_DISK, Dev_SCSIInitDevice},		/* Units 16-23 */
/*  { 1, 4, DEV_SCSI_TAPE, Dev_SCSIInitDevice},				*/
    { 0, 0, 0, Dev_XylogicsInitDevice},			/* Units 0-7 */
    { 0, 1, 0, Dev_XylogicsInitDevice},			/* Units 8-15 */
    { 1, 0, 0, Dev_XylogicsInitDevice},			/* Units 16-23 */
    { 1, 1, 0, Dev_XylogicsInitDevice},			/* Units 24-31 */
};
int devNumConfigDevices = sizeof(devDevice) / sizeof(DevConfigDevice);
