/* 
 * devFsOpTable.c --
 *
 *	Initialization of the operation switch tables used for
 *	the FS => DEV interface on Sun3 hosts.
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
#include "rawBlockDev.h"
#include "devFsOpTable.h"
#include "devTypesInt.h"

/*
 * Device specific include files.
 */

#include "devConsole.h"
#include "devSyslog.h"
#include "devNull.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "xylogics450.h"
#include "devNet.h"
#include "devTMR.h"
#include "devBlockDevice.h"


static ReturnStatus NullSelectProc();
static ReturnStatus NoDevice();
static ReturnStatus NullProc();

#ifdef SERIALB_DEBUG
extern ReturnStatus Dev_serialBOutTrace();
extern ReturnStatus Dev_serialBInTrace();
#endif


/*
 * Device type specific routine table:
 *	This is for the file-like operations as they apply to devices.
 *	DeviceOpen
 *	DeviceRead
 *	DeviceWrite
 *	DeviceIOControl
 *	DeviceClose
 *	DeviceSelect
 *	BlockDeviceAttach
 *	DeviceReopen
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * The console.  The workstation's display and keyboard.
     */
    {DEV_CONSOLE,    Dev_ConsoleOpen, Dev_ConsoleRead, Dev_ConsoleWrite,
		     Dev_ConsoleIOControl, Dev_ConsoleClose, Dev_ConsoleSelect,
		     DEV_NO_ATTACH_PROC, Dev_ConsoleReopen},
    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {DEV_SYSLOG,    Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect,
		    DEV_NO_ATTACH_PROC, Dev_SyslogReopen},
    /*
     * SCSI Worm interface.
     */
    {DEV_SCSI_WORM, Dev_TimerOpen, Dev_TimerRead, NullProc,
		    Dev_TimerIOControl, NullProc, NullProc,
		    DEV_NO_ATTACH_PROC, NoDevice},

    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_2, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullSelectProc,
		    DEV_NO_ATTACH_PROC, NoDevice},
    /*
     * SCSI Disk interface.
     */
    /*
     * New SCSI Disk interface.
     */
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, NullSelectProc, DevScsiDiskAttach,
		    DevRawBlockDevReopen},
    /*
     * SCSI Tape interface.
     */
    {DEV_SCSI_TAPE, DevSCSITapeOpen, DevSCSITapeRead, DevSCSITapeWrite,
		    DevSCSITapeIOControl, DevSCSITapeClose, NullSelectProc,
		    DEV_NO_ATTACH_PROC, NoDevice},
    /*
     * /dev/null
     */
    {DEV_MEMORY,    NullProc, Dev_NullRead, Dev_NullWrite,
		    NullProc, NullProc, NullSelectProc, DEV_NO_ATTACH_PROC,
		    NullProc},
    /*
     * Xylogics 450 disk controller.
     */
    {DEV_XYLOGICS, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, NullSelectProc,
		    DevXylogics450DiskAttach, DevRawBlockDevReopen},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect, 
		   DEV_NO_ATTACH_PROC, DevNet_FsReopen},

#ifdef SERIALB_DEBUG
    {DEV_SERIALB_OUT_QUEUE, NullProc, Dev_serialBOutTrace, NullProc,
                  NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC,
		  NoDevice},

    {DEV_SERIALB_IN_QUEUE, NullProc, Dev_serialBInTrace, NullProc,
                  NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC,
		  NoDevice},
#endif

};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);


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
NullSelectProc(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int *readPtr;		/* Read bit */
    int *writePtr;		/* Write bit */
    int *exceptPtr;		/* Exception bit */
{
    /*
     * Leave the read and write bits on.  This is used with /dev/null.
     */
    *exceptPtr = 0;
    return(SUCCESS);
}

