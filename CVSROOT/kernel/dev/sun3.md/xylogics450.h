/*
 * xylogics450.h --
 *
 *	Declarations for the Xylogics 450 controller. 
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _XYLOGICS450
#define _XYLOGICS450

#include "devBlockDevice.h"

/*
 * Format of the Fs_Device unit number for devices attached to a Xylogics450
 * HBA.  The 16 bits of the Disk unit number are used as follows:
 *
 *  | Bits 15 to 7 | Bits 6 to 5 |   Bits 4 to 3      |    Bits 2 to 0       |
 *  |    Not used  | Ctrl number |   Disk Number      |    Partition  Number |
 * 
 * XYLOGICS_DISK_NUM_FROM_DEVUNIT() - Extract disk number from the Device
 *				      unit number.
 * XYLOGICS_CTRL_NUM_FROM_DEVUNIT() - Extract the controller number from
 *				      the Fs_Deviceunit number.
 */
#define	XYLOGICS_DISK_NUM_FROM_DEVUNIT(unit)	(((unit)>>3)&0x3)
#define	XYLOGICS_CTRL_NUM_FROM_DEVUNIT(unit)    (((unit)>>5)&0x3)
#define	XYLOGICS_PARTITION_NUM_FROM_DEVUNIT(unit)	((unit)&0x3)

extern ClientData DevXylogics450Init();
extern Boolean	DevXylogics450Intr();		
extern DevBlockDeviceHandle	*DevXylogics450DiskAttach();

#endif /* _XYLOGICS450 */
