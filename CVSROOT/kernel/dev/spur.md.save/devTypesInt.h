/*
 * devTypesInt.h --
 *
 *	Declarations of device type numbers for SPUR machines.
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

#ifndef _DEVTYPESINT
#define _DEVTYPESINT

/* constants */

/*
 * Device types:
 *
 *	DEV_CONSOLE		The console - basic character input/output
 *	DEV_SYSLOG		The system log device
 *	DEV_MEMORY		Null device and kernel memory area.
 *	DEV_NET			Raw ethernet device - unit number is protocol.
 *	DEV_CC			SPUR Cache Controller.
 *	DEV_PCC			SPUR Cache Controller processed.
 *
 * NOTE: These numbers correspond to the major numbers for the devices
 * in /dev. Do not change them unless you redo makeDevice for all the devices
 * in /dev.
 *
 */

#define	DEV_CONSOLE		0
#define	DEV_SYSLOG		1
#define	DEV_MEMORY		6
#define	DEV_NET			8
#define	DEV_CC			9
#define	DEV_PCC			9

#endif _DEVTYPESINT

