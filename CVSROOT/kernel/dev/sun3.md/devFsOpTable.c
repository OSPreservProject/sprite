/* 
 * devFsOpTable.c --
 *
 *	The operation tables for the file system devices.  
 *
 * Copyright 1987, 1988 Regents of the University of California
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
#include "dev.h"
#include "devInt.h"
#include "fs.h"
#include "devFsOpTable.h"
#include "devTypesInt.h"

/*
 * Device specific include files.
 */

#include "devConsole.h"
#include "devSyslog.h"
#include "devNull.h"
#include "devSCSI.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "devSCSIWorm.h"
#include "devXylogicsDisk.h"
#include "devNet.h"


static ReturnStatus NullSelectProc();
static ReturnStatus NoDevice();
static ReturnStatus NullProc();

/*
 * Device type specific routine table:
 *	This is for the file-like operations as they apply to devices.
 *	DeviceOpen
 *	DeviceRead
 *	DeviceWrite
 *	DeviceIOControl
 *	DeviceClose
 *	DeviceSelect
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * The console.  The workstation's display and keyboard.
     */
    {DEV_CONSOLE,    Dev_ConsoleOpen, Dev_ConsoleRead, Dev_ConsoleWrite,
		     Dev_ConsoleIOControl, Dev_ConsoleClose, Dev_ConsoleSelect},
    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {DEV_SYSLOG,    Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect},
    /*
     * SCSI Worm interface.
     */
    {DEV_SCSI_WORM, Dev_SCSIWormOpen, Dev_SCSIWormRead, Dev_SCSIWormWrite,
		     Dev_SCSIWormIOControl, Dev_SCSIWormClose, NullProc},
    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_2, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc},
    /*
     * SCSI Disk interface.
     */
    {DEV_SCSI_DISK, Dev_SCSIDiskOpen, Dev_SCSIDiskRead, Dev_SCSIDiskWrite,
		    Dev_SCSIDiskIOControl, Dev_SCSIDiskClose, NullProc},
    /*
     * SCSI Tape interface.
     */
    {DEV_SCSI_TAPE, Dev_SCSITapeOpen, Dev_SCSITapeRead, Dev_SCSITapeWrite,
		    Dev_SCSITapeIOControl, Dev_SCSITapeClose, NullProc},
    /*
     * /dev/null
     */
    {DEV_MEMORY,    NullProc, Dev_NullRead, Dev_NullWrite,
		    NullProc, NullProc, NullSelectProc},
    /*
     * Xylogics 450 disk controller.
     */
    {DEV_XYLOGICS, Dev_XylogicsDiskOpen, Dev_XylogicsDiskRead,
		    Dev_XylogicsDiskWrite, Dev_XylogicsDiskIOControl,
		    Dev_XylogicsDiskClose, NullProc},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect},
};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);


/*
 * Device Block I/O operation table.  This table is sparse because not
 * all devices support block I/O.
 *	FsBlockIOInit
 *	FsBlockIO
 */
DevFsBlockOps devFsBlockOpTable[] = {
    { DEV_CONSOLE, 0 },
    { DEV_SYSLOG, 0 },
    { DEV_SCSI_WORM, 0 },
    { DEV_PLACEHOLDER_2, 0 },
    { DEV_SCSI_DISK, Dev_SCSIDiskBlockIO },
    { DEV_SCSI_TAPE, 0 },
    { DEV_MEMORY, 0 },
    { DEV_XYLOGICS, Dev_XylogicsDiskBlockIO },
    { DEV_NET, 0 },
};

/*
 * A list of disk device types that is used when probing for a disk.
 */
int devFsDefaultDiskTypes[] = { DEV_SCSI_DISK, DEV_XYLOGICS };
int devNumDiskTypes = sizeof(devFsDefaultDiskTypes) / sizeof(int);

static ReturnStatus
NullProc()
{
    return(SUCCESS);
}


static ReturnStatus
NoDevice()
{
    return(FS_INVALID_ARG);
}


/*ARGSUSED*/
static ReturnStatus
NullSelectProc(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int		inFlags;	/* FS_READBLE, FS_WRITABLE, FS_EXCEPTION. */
    int		*outFlagsPtr;	/* Copy of inFlags. */
{
    *outFlagsPtr = inFlags;
    return(SUCCESS);
}

