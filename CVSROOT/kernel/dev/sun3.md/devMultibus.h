/*
 * multibus.h --
 *
 *	Definitions for addresses of the mapped multibus memory.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MULTIBUS
#define _MULTIBUS

/*
 * The sun monitor maps the multibus memory space starting at MULTIBUS_BASE.
 * It is appropriate to add the multibus address to this base to get
 * the mapped kernel address for the multibus device.
 */
#define MULTIBUS_BASE	0xF00000

#endif _MULTIBUS
