/*
 * multibus.h --
 *
 *	Definitions for addresses of the mapped multibus memory.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun3.md/devMultibus.h,v 9.0 89/09/12 14:58:46 douglis Stable $ SPRITE (Berkeley)
 */

#ifndef _DEVMULTIBUS
#define _DEVMULTIBUS

/*
 * The sun monitor maps the multibus memory space starting at DEV_MULTIBUS_BASE.
 * It is appropriate to add the multibus address to this base to get
 * the mapped kernel address for the multibus device.
 */
#define DEV_MULTIBUS_BASE	0xF00000

#endif /* _DEVMULTIBUS */
