/*
 * devTypes.h --
 *
 *	This file declares the major device types used in Sprite.
 *
 * Copyright 1988 Regents of the University of California
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

#ifndef _DEVTYPES
#define _DEVTYPES

/* constants */

/*
 * Device types:
 *
 *	DEV_TERM		Terminal devices, like the console
 *	DEV_SYSLOG		The system log device
 *	DEV_MEMORY		Null device and kernel memory area.
 *	DEV_NET			Raw ethernet device - unit number is protocol.
 *	DEV_KEYBOARD		Keyboard
 *	DEV_SCSI_DISK		Disk on the SCSI bus
 *	DEV_SCSI_TAPE		Tape drive on the SCSI bus
 *	DEV_XYLOGICS		Xylogics 450 controller
 *	DEV_SCSI_HBA		Raw SCSI HBA device driver
 *	DEV_RAID		Raw interface to disk array
 *	DEV_DEBUG		For debugging disk arrays?
 *	DEV_MOUSE		Mouse and keystroke info for window systems
 *	DEV_AUDIO		Audio device
 *
 *
 * NOTE: These numbers correspond to the major numbers for the devices
 * in /dev. Do not change them unless you redo makeDevice for all the devices
 * in /dev.
 *
 */

#define	DEV_TERM		0
#define	DEV_SYSLOG		1
#define	DEV_SCSI_WORM		2
#define	DEV_PLACEHOLDER_2	3
#define	DEV_SCSI_DISK		4
#define	DEV_SCSI_TAPE		5
#define	DEV_MEMORY		6
#define	DEV_XYLOGICS		7
#define	DEV_NET			8
#define	DEV_SCSI_HBA		9
#define	DEV_RAID		10
#define	DEV_DEBUG		11
#ifndef ds3100
#define DEV_MOUSE		12
#endif /* not ds3100 */
#define DEV_GRAPHICS		13
#define DEV_PLACEHOLDER_3	13	/* for ds3100 unused graphics device */
#define DEV_AUDIO		15
#define DEV_VMELINK		16

#ifdef ds3100
#define	DEV_CONSOLE		0
#define DEV_GRAPHICS		9
/*
 * Unit numbers for the graphics device.
 */
#define DEV_MOUSE		0
#define DEV_XCONS		1

/*
 * SCSI HBA's attached to the system.
 */
#define DEV_SII_HBA	0
#endif /* ds3100 */

/*
 * Following device(s) exist only on Sequent Symmetry
 */

#define DEV_ZDC_DISK		14

/*
 * The following device types are defined for SPUR as of 7/15/89.  They
 * should be changed so that don't overlap the device types above.
 * Also, why do two different devices have the same number?
 */

#define DEV_CC			9
#define DEV_PCC			9

/*
 * SCSI HBA's attached to the system.
 */

#define	DEV_SCSI3_HBA	0
#define	DEV_SCSI0_HBA	1
#define	DEV_JAGUAR_HBA  2

/*
 * The following exists only on the sparc station.
 */
#define DEV_SCSIC90_HBA	0

#endif /* _DEVTYPES */

