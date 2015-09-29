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

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/sun3.md/RCS/devFsOpTable.c,v 8.4 89/05/24 07:50:49 rab Exp Locker: brent $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "devInt.h"
#include "rawBlockDev.h"
#include "devFsOpTable.h"
#include "devTypesInt.h"

/*
 * Device specific include files.
 */

#include "devSCSIDisk.h"
#include "devBlockDevice.h"


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
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * New SCSI Disk interface.
     */
    {DEV_SCSI_DISK, DevRawBlockDevOpen, DevRawBlockDevRead,
		    DevRawBlockDevWrite, DevRawBlockDevIOControl, 
		    DevRawBlockDevClose, NullProc, DevScsiDiskAttach},

};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);


static ReturnStatus
NullProc()
{
    return(SUCCESS);
}

