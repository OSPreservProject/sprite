/*
 * devSCSIDisk.h --
 *
 *	External definitions for the SCSI Disk.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIDISK
#define _DEVSCSIDISK

/*
 * Forward Declarations.
 */
ReturnStatus Dev_SCSIDiskOpen();
ReturnStatus Dev_SCSIDiskRead();
ReturnStatus Dev_SCSIDiskWrite();
ReturnStatus Dev_SCSIDiskIOControl();
ReturnStatus Dev_SCSIDiskClose();

ReturnStatus Dev_SCSIDiskBlockIOInit();
ReturnStatus Dev_SCSIDiskBlockIO();

#endif _DEVSCSIDISK
