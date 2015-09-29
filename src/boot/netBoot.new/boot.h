/*-
 * boot.h --
 *	 Header file for sprite tftp boot program
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$Header: /sprite/src/boot/netBoot/RCS/boot.h,v 1.2 89/06/16 08:30:31 brent Exp $ SPRITE (Berkeley)"
 */
#ifndef _BOOT_H
#define _BOOT_H

#include <machparam.h>

#ifndef ASM
#include    "sunromvec.h"
#endif /* ASM */

#define BOOT_START	    (BOOT_CODE-0x4000)

#define printf	  (*romp->v_printf)
#define printhex  (*romp->v_printhex)

#endif /* _BOOT_H */
