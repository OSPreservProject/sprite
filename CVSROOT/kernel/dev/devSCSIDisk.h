/*
 * devSCSIDisk.h --
 *
 *	External definitions for disks on the SCSI I/O bus.
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIDISK
#define _DEVSCSIDISK

/*
 * State info for an SCSI Disk.  This gets copied from the disk label.
 */
typedef struct DevSCSIDisk {
    int numCylinders;	/* The number of cylinders on the disk */
    int numHeads;	/* The number of heads */
    int numSectors;	/* The number of sectors per track */
    DevDiskMap map[DEV_NUM_DISK_PARTS];	/* The partition map */
    int type;		/* Type of the drive, needed for error checking */
} DevSCSIDisk;

/*
 * A table of SCSI disk info is kept.  It is used to map
 * from unitNumber to the correct SCSI disk and partition.
 */
extern DevSCSIDevice *scsiDisk[];
extern int scsiDiskIndex;

/*
 * SCSI_MAX_DISKS the maximum number of disk devices that can be hung
 *	off ALL the SCSI controllers together.
 */
#define SCSI_MAX_DISKS	4

/*
 * Disk drive types:
 *	SCSI_GENERIC_DISK	Old style (non-extended sense) disks.  This
 *				is found in 2/120's.
 *	SCSI_CLASS7_DISK	A new style disk returning Class 7 extended
 *				sense data.
 */
#define SCSI_GENERIC_DISK	0
#define SCSI_CLASS7_DISK	1

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

ReturnStatus DevSCSIDiskError();

#endif _DEVSCSIDISK
