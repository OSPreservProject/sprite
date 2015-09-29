/*
 * stdfb.h --
 *
 *	Stdlarations of the /dev/stdfb device. 
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
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.7 91/02/09 13:24:52 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _STDFB
#define _STDFB

/* constants */

#define IOC_STDFB (17 << 16)

#define IOC_STDFB_INFO 	(IOC_STDFB | 1)

/*
 * Structure returned by IOC_STDFB_INFO ioctl.
 */

typedef struct Dev_StdFBInfo {
    int		type;		/* Type of display. See below. */
    int		width;		/* Width in pixels. */
    int		height;		/* Height in pixels. */
    int		planes;		/* Number of planes. */
} Dev_StdFBInfo;

/*
 * Valid types of displays.
 */

#define DEV_STDFB_UNKNOWN	0
#define DEV_STDFB_PMAGBA	1	/* Standard ds5000 color frame buffer.*/

#endif /* _STDFB */

