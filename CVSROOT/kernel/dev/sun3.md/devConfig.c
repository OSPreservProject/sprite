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
    { "SCSI", 0x80000, DEV_MULTIBUS,	0, Dev_SCSIInitController, 0, 0,
				    FALSE, Dev_SCSIIdleCheck, 0, 0},
/*  { "SCSI", 0x84000, DEV_MULTIBUS,	1, Dev_SCSIInitController, 0, 0}, */
    { "SCSI", 0x200000, DEV_VME_D16A24, 0, Dev_SCSIInitController, 64,
				    Dev_SCSIIntrStub, 
				    FALSE, Dev_SCSIIdleCheck, 0, 0},
    { "SCSI", 0x0FE12000, DEV_OBIO,	0, Dev_SCSIInitController, 64,
				    Dev_SCSIIntrStub, 
				    FALSE, Dev_SCSIIdleCheck, 0, 0},
    { "Xylogics", 0xee40, DEV_VME_D16A16,	 0, Dev_XylogicsInitController,
				    72, Dev_XylogicsIntrStub,
				    FALSE, Dev_XylogicsIdleCheck, 0, 0},
    { "Xylogics", 0xee48, DEV_VME_D16A16,	 1, Dev_XylogicsInitController,
				    73, Dev_XylogicsIntrStub, 
				    FALSE, Dev_XylogicsIdleCheck, 0, 0},
};
int devNumConfigCntrlrs = sizeof(devCntrlr) / sizeof(DevConfigController);

/*
 * The device configuration table.  Device entries for non-existent controllers
 * don't cost any time at startup.  Non-existent devices on existing
 * controllers can cost an I/O bus timeout period at start up.
 *
 * NB: There is an implicit correspondence between filesystem device
 * unit numbers ("minor" type) and disk entries in the table below.
 * This dependence is based (up to) 8 disk partitions per disk.
 * Several entries for the same kind of device result in a correspondence 
 * between those devices and ranges of unit numbers.
 *
 * SCSI: The cntrlrID corresponds to the Host Bus Adaptor.
 *	The slaveId corresponds to the SCSI target ID.
 *	The flags field embeds a type and the LUN (Logical Unit) of the
 *		device within the SCSI target. 
 */
DevConfigDevice devDevice[] = {
/* cntrlrID, slaveID, flags, initproc */		/* Device unit # */
    /*
     * For historical reasons LUN 1 is initialized first and corresponds
     * to rsd0, while LUN 0 comes second and maps to rsd1.
     */
    { 0, 0, 1 | SCSI_DISK, Dev_SCSIInitDevice},	/* rsd units 0-7 */
    { 0, 0, 0 | SCSI_DISK, Dev_SCSIInitDevice},	/* rsd units 8-15 */

/*  { 0, 1, 0 | SCSI_DISK, Dev_SCSIInitDevice},	/* rsd units 16-23 */
/*  { 0, 1, 1 | SCSI_DISK, Dev_SCSIInitDevice},	/* rsd units 24-31 */
    { 0, 3, 0 | SCSI_WORM, Dev_SCSIInitDevice},	/* scsiWorm 0-7 */
    { 0, 4, 0 | SCSI_TAPE, Dev_SCSIInitDevice},	/* tape0 (shoebox) */
    { 0, 5, 0 | SCSI_TAPE, Dev_SCSIInitDevice},	/* tape1 (exabyte) */

    { 0, 0, 0, Dev_XylogicsInitDevice},			/* rxy units 0-7 */
    { 0, 1, 0, Dev_XylogicsInitDevice},			/* rxy units 8-15 */
    { 1, 0, 0, Dev_XylogicsInitDevice},			/* rxy units 16-23 */
    { 1, 1, 0, Dev_XylogicsInitDevice},			/* rxy units 24-31 */
};
int devNumConfigDevices = sizeof(devDevice) / sizeof(DevConfigDevice);
