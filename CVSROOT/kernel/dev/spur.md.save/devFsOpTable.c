/* 
 * devFsOpTable.c --
 *
 *	Initialization of the operation switch tables used for
 *	the FS => DEV interface on Sun3 hosts.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
#include "fs.h"
#include "rawBlockDev.h"
#include "devFsOpTable.h"
#include "devTypesInt.h"

/*
 * Device specific include files.
 */

#include "devConsole.h"
#include "devNull.h"
#include "devSyslog.h"
#include "devNet.h"
#include "devCC.h"
#include "devBlockDevice.h"


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
 *	BlockDeviceAttach
 *	DeviceReopen
 *
 *
 * Since the table is indexed into the type must be at the right position in 
 * the array.  The FILLER macro is used to fill in the gaps.
 */

#define	FILLER(num)	{num,NoDevice,NullProc,NullProc,NullProc,NullProc,NullProc, DEV_NO_ATTACH_PROC, NoDevice},


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
    FILLER(2)
    FILLER(3)
    FILLER(4)
    FILLER(5)
    /*
     * /dev/null
     */
    {DEV_MEMORY,    NullProc, Dev_NullRead, Dev_NullWrite,
		    NullProc, NullProc, NullSelectProc, DEV_NO_ATTACH_PROC,
		    NullProc},
    FILLER(7)
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect,
		   DEV_NO_ATTACH_PROC, DevNet_FsReopen},
    /*
     * Cache controler device.
     */
    {DEV_CC,   	 Dev_CCOpen, Dev_CCRead, Dev_CCWrite,
		     Dev_CCIOControl, Dev_CCClose, Dev_CCSelect,
		     DEV_NO_ATTACH_PROC, Dev_CCReopen},
    /*
     * processed cache controler device.
     */
    {DEV_PCC,       Dev_PCCOpen, Dev_PCCRead, Dev_PCCWrite,
		    Dev_PCCIOControl, Dev_PCCClose, Dev_PCCSelect,
		    DEV_NO_ATTACH_PROC, Dev_PCCReopen},
};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);

/*
 * A list of disk device types that is used when probing for a disk.
 */
int devFsDefaultDiskTypes[] = { 0 };
int devNumDiskTypes = 0;


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
    int	*readPtr;		/* Read bit to clear if not readable */
    int	*writePtr;		/* Write bit to clear if not readable */
    int	*exceptPtr;		/* Except bit to clear if not readable */
{
    *exceptPtr = 0;
    return(SUCCESS);
}

