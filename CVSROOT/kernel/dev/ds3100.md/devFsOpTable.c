/* 
 * devFsOpTable.c --
 *
 *	The operation tables for the file system devices.  
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
#include "dev.h"
#include "devInt.h"
#include "fs.h"
#include "rawBlockDev.h"
#include "devFsOpTable.h"
#include "devTypes.h"

/*
 * Device specific include files.
 */

#include "devSyslog.h"
#include "devNull.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "devNet.h"
#include "devBlockDevice.h"
#include "scsiHBADevice.h"
#include "raidExt.h"
#include "tty.h"
#include "graphics.h"

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
 *	DeviceMMap
 */

#ifndef lint
DevFsTypeOps devFsOpTable[] = {
    /*
     * Serial lines used to implement terminals.
     */
    {DEV_TERM,       DevTtyOpen, DevTtyRead, DevTtyWrite,
		     DevTtyIOControl, DevTtyClose, DevTtySelect,
		     DEV_NO_ATTACH_PROC, NoDevice, NullProc},
    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {DEV_SYSLOG,    Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect,
		    DEV_NO_ATTACH_PROC, Dev_SyslogReopen, NullProc},
    /*
     * SCSI Worm interface.
     */
    {DEV_SCSI_WORM, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc, 
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_2, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc, 
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
    /*
     * New SCSI Disk interface.
     */
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, Dev_NullSelect, DevScsiDiskAttach,
		    DevRawBlockDevReopen, NullProc},
    /*
     * SCSI Tape interface.
     */
    {DEV_SCSI_TAPE, DevSCSITapeOpen, DevSCSITapeRead, DevSCSITapeWrite,
		    DevSCSITapeIOControl, DevSCSITapeClose, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
    /*
     * /dev/null
     */
    {DEV_MEMORY,    NullProc, Dev_NullRead, Dev_NullWrite,
		    Dev_NullIOControl, NullProc, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, NullProc, NullProc},
    /*
     * Xylogics 450 disk controller.
     */
    {DEV_XYLOGICS, NullProc, Dev_NullRead, 
		   Dev_NullWrite, Dev_NullIOControl, 
		   NullProc, Dev_NullSelect, 
		   DEV_NO_ATTACH_PROC, NullProc, NullProc},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect,
		   DEV_NO_ATTACH_PROC, DevNet_FsReopen, NullProc},
    /*
     * Raw SCSI HBA interface.
     */
    {DEV_SCSI_HBA, DevSCSIDeviceOpen, Dev_NullRead, Dev_NullWrite,
                    DevSCSIDeviceIOControl, DevSCSIDeviceClose, Dev_NullSelect,
                    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
    /*
     * RAID device.
     */
    {DEV_RAID, NullProc, Dev_NullRead,
	       Dev_NullWrite, Dev_NullIOControl,
	       NullProc, Dev_NullSelect,
	       DEV_NO_ATTACH_PROC, NullProc, NullProc},

    /*
     * Debug device. (useful for debugging RAID device)
     */
    {DEV_DEBUG, NullProc, Dev_NullRead,
	       Dev_NullWrite, Dev_NullIOControl,
	       NullProc, Dev_NullSelect,
	       DEV_NO_ATTACH_PROC, NullProc, NullProc},

    /*
     * The graphics device.
     */
    {DEV_MOUSE, DevGraphicsOpen, DevGraphicsRead, DevGraphicsWrite,
		   DevGraphicsIOControl, DevGraphicsClose, DevGraphicsSelect,
		   DEV_NO_ATTACH_PROC, NoDevice, NullProc}, 
    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_3, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc, 
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
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

#else /*LINT*/

DevFsTypeOps devFsOpTable[1];

#endif /*LINT*/

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);

