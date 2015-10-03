/*
 * devGraphicsInt.h --
 *
 *	Declarations for the grahics devices.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVGRAPHICSINT
#define _DEVGRAPHICSINT

/*
 * Different types of supported displays.
 */
#define UNKNOWN	0
#define PMAGBA	1
#define PMAGDA	2

#define PMAGBA_BUFFER_OFFSET		(0)
#define PMAGBA_RAMDAC_OFFSET	 	(0x200000)
#define PMAGBA_IREQ_OFFSET		(0x300000)
#define PMAGBA_ROM_OFFSET 		(0x380000)
#define PMAGBA_ROM2_OFFSET		(0x3c0000)
#define PMAGBA_WIDTH			1024
#define PMAGBA_HEIGHT			864
#define PMAGBA_PLANES			8


#endif /* _DEVGRAPHICSINT */

