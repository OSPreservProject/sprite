/*
 * scsiHBADevice.h --
 *
 *	External definitions for the SCSI HBA device.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SCSI_HBA_DEVICE
#define _SCSI_HBA_DEVICE



extern ReturnStatus DevSCSIDeviceOpen();
extern ReturnStatus DevSCSIDeviceIOControl();
extern ReturnStatus DevSCSIDeviceClose();


#endif /* _SCSIHBADEVICE */
