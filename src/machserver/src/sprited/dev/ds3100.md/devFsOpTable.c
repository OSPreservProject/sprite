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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/dev/ds3100.md/RCS/devFsOpTable.c,v 1.3 92/04/02 21:13:38 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <status.h>
#include <dev.h>
#include <devInt.h>
#include <fs.h>
#ifdef SPRITED_LOCALDISK
#include <rawBlockDev.h>
#endif
#include <devFsOpTable.h>
#include <devTypes.h>

/*
 * Device specific include files.
 */

#include <devSyslog.h>
#include <devNull.h>
#ifdef SPRITED_LOCALDISK
#include <devSCSIDisk.h>
#include <scsiHBADevice.h>
#include <raidExt.h>
#endif
#ifdef SPRITED_LOCALTAPE
#include <devSCSITape.h>
#endif
#ifdef SPRITED_DEVNET
#include <devNet.h>
#endif
#include <devBlockDevice.h>
#include <tty.h>
#ifdef SPRITED_GRAPHICS
#include <graphics.h>
#endif

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
#ifdef SPRITED_LOCALDISK
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, Dev_NullSelect, DevScsiDiskAttach,
		    DevRawBlockDevReopen, NullProc},
#else
    {DEV_SCSI_DISK, NoDevice, NullProc, NullProc,
	 	    NullProc, NullProc, NullProc,
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

    /*
     * SCSI Tape interface.
     */
#ifdef SPRITED_LOCALTAPE
    {DEV_SCSI_TAPE, DevSCSITapeOpen, DevSCSITapeRead, DevSCSITapeWrite,
		    DevSCSITapeIOControl, DevSCSITapeClose, Dev_NullSelect,
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#else
    {DEV_SCSI_TAPE, NoDevice, NullProc, NullProc,
	 	    NullProc, NullProc, NullProc,
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

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
#ifdef SPRITED_DEVNET
    {DEV_NET,      DevNet_FsOpen, DevNet_FsRead, DevNet_FsWrite, 
		   DevNet_FsIOControl, DevNet_FsClose, DevNet_FsSelect,
		   DEV_NO_ATTACH_PROC, DevNet_FsReopen, NullProc},
#else
    {DEV_NET,      NoDevice, NullProc, NullProc,
	 	   NullProc, NullProc, NullProc,
		   DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

    /*
     * Raw SCSI HBA interface.
     */
#ifdef SPRITED_LOCALDISK
    {DEV_SCSI_HBA, DevSCSIDeviceOpen, Dev_NullRead, Dev_NullWrite,
                    DevSCSIDeviceIOControl, DevSCSIDeviceClose, Dev_NullSelect,
                    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#else
    {DEV_SCSI_HBA,  NoDevice, NullProc, NullProc,
	 	    NullProc, NullProc, NullProc,
		    DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

    /*
     * RAID device.
     */
#ifdef SPRITED_LOCALDISK
    {DEV_RAID, NullProc, Dev_NullRead,
	       Dev_NullWrite, Dev_NullIOControl,
	       NullProc, Dev_NullSelect,
	       DEV_NO_ATTACH_PROC, NullProc, NullProc},
#else
    {DEV_RAID, NoDevice, NullProc, NullProc,
	       NullProc, NullProc, NullProc,
	       DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

    /*
     * Debug device. (useful for debugging RAID device)
     */
#ifdef SPRITED_LOCALDISK
    {DEV_DEBUG, NullProc, Dev_NullRead,
	       Dev_NullWrite, Dev_NullIOControl,
	       NullProc, Dev_NullSelect,
	       DEV_NO_ATTACH_PROC, NullProc, NullProc},
#else
    {DEV_DEBUG, NoDevice, NullProc, NullProc,
	 	NullProc, NullProc, NullProc,
		DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif


    /*
     * The graphics device.
     */
#ifdef SPRITED_GRAPHICS
    {DEV_MOUSE, DevGraphicsOpen, DevGraphicsRead, DevGraphicsWrite,
		   DevGraphicsIOControl, DevGraphicsClose, DevGraphicsSelect,
		   DEV_NO_ATTACH_PROC, NoDevice, NullProc}, 
#else
    {DEV_MOUSE,    NoDevice, NullProc, NullProc,
	 	   NullProc, NullProc, NullProc,
		   DEV_NO_ATTACH_PROC, NoDevice, NullProc},
#endif

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
