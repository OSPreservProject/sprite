/*
 * atc.h --
 *
 *	Declarations for the ATC disk controller. 
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
 * $Header: /sprite/src/kernel/Cvsroot/kernel/dev/sun4.md/atc.h,v 9.1 92/08/24 14:33:51 alc Exp $ SPRITE (Berkeley)
 */

#ifndef _ATC
#define _ATC

#define IOC_DEV_ATC			(18 << 16)
#define IOC_DEV_ATC_RESET		(IOC_DEV_ATC | 3)
#define IOC_DEV_ATC_READXBUS		(IOC_DEV_ATC | 4)
#define IOC_DEV_ATC_WRITEXBUS		(IOC_DEV_ATC | 5)
#define	IOC_ATC_DEBUG_ON	        (IOC_DEV_ATC | 6)
#define	IOC_ATC_DEBUG_OFF	        (IOC_DEV_ATC | 7)
#define IOC_DEV_ATC_IO                  (IOC_DEV_ATC | 8)
#endif /* _ATC */
