/*
 * hbatest.h --
 *
 *	Declarations of SCSI HBA disk test IOControl interface.
 *	This device is used to test HBA and disk by the RAID people.
 *	Mendel - 9/12/89
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.2 89/01/07 04:12:44 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _HBATEST
#define _HBATEST

#define IOC_HBA_DISK_IO_TEST 	(3 << 16)
#define IOC_HBA_DISK_UNIT_TEST  ((3 << 16) | 1)


typedef struct DevHBADiskTest {
    unsigned int   firstSector;		/* First sector of operation. */
    unsigned short lengthInSectors;     /* Number of sectors for operation. */
    unsigned short writeOperation;	/* TRUE if write operation, FALSE if
					 * read. */
} DevHBADiskTest;

#define	MAX_HBA_TEST_VECTOR	(4096/sizeof(DevHBADiskTest))
#define	MAX_HBA_UNIT_TESTS		2000

#endif
