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
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * The console.  The workstation's display and keyboard.
     */
    {DEV_CONSOLE,    Dev_ConsoleOpen, Dev_ConsoleRead, Dev_ConsoleWrite,
		     Dev_ConsoleIOControl, Dev_ConsoleClose, Dev_ConsoleSelect,
		     DEV_NO_ATTACH_PROC},
    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {DEV_SYSLOG,    Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect,
		    DEV_NO_ATTACH_PROC},
    /*
     * SCSI Worm interface.
     */
    {DEV_SCSI_WORM, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC},

    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_2, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC},
    /*
     * SCSI Disk interface.
     */
    /*
     * New SCSI Disk interface.
     */
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, NullProc, DevScsiDiskAttach},
    /*
     * SCSI Tape interface.
     */
    {DEV_SCSI_TAPE, DevSCSITapeOpen, DevSCSITapeRead, DevSCSITapeWrite,
		    DevSCSITapeIOControl, DevSCSITapeClose, NullProc},
    /*
     * /dev/null
     */
    {DEV_MEMORY,    NullProc, Dev_NullRead, Dev_NullWrite,
		    NullProc, NullProc, NullSelectProc, DEV_NO_ATTACH_PROC},
    /*
     * Xylogics 450 disk controller.
     */
    {DEV_XYLOGICS, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, NullProc, DevXylogics450DiskAttach},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect, 
		   DEV_NO_ATTACH_PROC},

#ifdef SERIALB_DEBUG
    {DEV_SERIALB_OUT_QUEUE, NullProc, Dev_serialBOutTrace, NullProc,
                  NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC },

    {DEV_SERIALB_IN_QUEUE, NullProc, Dev_serialBInTrace, NullProc,
                  NullProc, NullProc, NullProc, DEV_NO_ATTACH_PROC },
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
NullSelectProc(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int		inFlags;	/* FS_READBLE, FS_WRITABLE, FS_EXCEPTION. */
    int		*outFlagsPtr;	/* Copy of inFlags. */
{
    *outFlagsPtr = inFlags;
    return(SUCCESS);
}

