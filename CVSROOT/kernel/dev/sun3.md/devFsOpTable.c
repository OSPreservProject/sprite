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
#include "devTypes.h"

/*
 * Device specific include files.
 */

#include "devSyslog.h"
#include "devNull.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "xylogics450.h"
#include "devNet.h"
#include "devTMR.h"
#include "devBlockDevice.h"
#include "devfb.h"
#include "scsiHBADevice.h"
#include "raidExt.h"
#include "tty.h"
#include "mouse.h"


static ReturnStatus nullOpenProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, Fs_NotifyToken notifyToken, int *flagsPtr));
static ReturnStatus noOpenProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, Fs_NotifyToken notifyToken, int *flagsPtr));
static ReturnStatus nullReadProc _ARGS_ ((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
static ReturnStatus nullWriteProc _ARGS_ ((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
static ReturnStatus nullCloseProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, int numUsers, int numWriters));
static ReturnStatus nullSelectProc _ARGS_ ((Fs_Device *devicePtr,
    int *readPtr, int *writePtr, int *exceptPtr));
static ReturnStatus nullReopenProc _ARGS_ ((Fs_Device *devicePtr,
    int numUsers, int numWriters, Fs_NotifyToken notifyToken, int *flagsPtr));
static ReturnStatus noReopenProc _ARGS_ ((Fs_Device *devicePtr,
    int numUsers, int numWriters, Fs_NotifyToken notifyToken, int *flagsPtr));
static ReturnStatus noMmapProc _ARGS_ ((Fs_Device *devicePtr,
    Address startAddr, int length, int offset, Address *newAddrPtr));


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
 *      DeviceMmap
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * Serial lines used to implement terminals.
     */
    {DEV_TERM,       DevTtyOpen, DevTtyRead, DevTtyWrite,
		     DevTtyIOControl, DevTtyClose, DevTtySelect,
		     DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},

    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {DEV_SYSLOG,    Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect,
		    DEV_NO_ATTACH_PROC, Dev_SyslogReopen, noMmapProc},
    /*
     * SCSI Worm interface.
     */
    {DEV_SCSI_WORM, Dev_TimerOpen, Dev_TimerRead, nullWriteProc,
		    Dev_TimerIOControl, nullCloseProc, nullSelectProc,
		    DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},
    /*
     * The following device number is unused.
     */
    {DEV_PLACEHOLDER_2, noOpenProc, nullReadProc, nullWriteProc,
		    Dev_NullIOControl, nullCloseProc, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},
    /*
     * New SCSI Disk interface.
     */
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl,
		    DevRawBlockDevClose, Dev_NullSelect, DevScsiDiskAttach,
		    DevRawBlockDevReopen, noMmapProc},
    /*
     * SCSI Tape interface.
     */
    {DEV_SCSI_TAPE, DevSCSITapeOpen, DevSCSITapeRead, DevSCSITapeWrite,
		    DevSCSITapeIOControl, DevSCSITapeClose, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},
    /*
     * /dev/null
     */
    {DEV_MEMORY,    nullOpenProc, Dev_NullRead, Dev_NullWrite,
		    Dev_NullIOControl, nullCloseProc, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, nullReopenProc, noMmapProc},
    /*
     * Xylogics 450 disk controller.
     */
    {DEV_XYLOGICS, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, Dev_NullSelect,
		    DevXylogics450DiskAttach, DevRawBlockDevReopen,
                    noMmapProc},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect, 
		   DEV_NO_ATTACH_PROC, DevNet_FsReopen, noMmapProc},
    /*
     * Raw SCSI HBA interface.
     */
    {DEV_SCSI_HBA, DevSCSIDeviceOpen, Dev_NullRead, Dev_NullWrite,
		    DevSCSIDeviceIOControl, DevSCSIDeviceClose, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},
    /*
     * RAID device.
     */ 
    {DEV_RAID, DevRawBlockDevOpen, DevRawBlockDevRead,
                    DevRawBlockDevWrite, DevRawBlockDevIOControl,
                    DevRawBlockDevClose, nullSelectProc, DevRaidAttach,
                    DevRawBlockDevReopen, noMmapProc},
    /*
     * Debug device. (useful for debugging RAID device)
     */ 
    {DEV_DEBUG, DevRawBlockDevOpen, DevRawBlockDevRead,
                    DevRawBlockDevWrite, DevRawBlockDevIOControl,
                    DevRawBlockDevClose, nullSelectProc, DevDebugAttach,
                    DevRawBlockDevReopen, noMmapProc},
    /*
     * Event devices for window systems.
     */
    {DEV_MOUSE,    DevMouseOpen, DevMouseRead, DevMouseWrite,
		   DevMouseIOControl, DevMouseClose, DevMouseSelect,
		   DEV_NO_ATTACH_PROC, noReopenProc, noMmapProc},
    /*
     * Frame buffer device.
     */
    {DEV_GRAPHICS, DevFBOpen, nullReadProc, nullWriteProc,
                   DevFBIOControl, DevFBClose, nullSelectProc,
                   DEV_NO_ATTACH_PROC, noReopenProc, DevFBMMap},
};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);

static ReturnStatus
nullOpenProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, Fs_NotifyToken notifyToken, int *flagsPtr))
{
    return SUCCESS;
}

static ReturnStatus
noOpenProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, Fs_NotifyToken notifyToken, int *flagsPtr))
{
    return FS_INVALID_ARG;
}

static ReturnStatus
nullReadProc _ARGS_ ((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr))
{
    return SUCCESS;
}

static ReturnStatus
nullWriteProc _ARGS_ ((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr))
{
    return SUCCESS;
}

static ReturnStatus
nullCloseProc _ARGS_ ((Fs_Device *devicePtr,
    int flags, int numUsers, int numWriters))
{
    return SUCCESS;
}

static ReturnStatus
nullSelectProc _ARGS_ ((Fs_Device *devicePtr,
    int *readPtr, int *writePtr, int *exceptPtr))
{
    return SUCCESS;
}

static ReturnStatus
nullReopenProc _ARGS_ ((Fs_Device *devicePtr,
    int numUsers, int numWriters, Fs_NotifyToken notifyToken, int *flagsPtr))
{
    return SUCCESS;
}

static ReturnStatus
noReopenProc _ARGS_ ((Fs_Device *devicePtr,
    int numUsers, int numWriters, Fs_NotifyToken notifyToken, int *flagsPtr))
{
    return FS_INVALID_ARG;
}

static ReturnStatus
noMmapProc _ARGS_ ((Fs_Device *devicePtr,
    Address startAddr, int length, int offset, Address *newAddrPtr))
{
    return FS_INVALID_ARG;
}

