/*
 * devVid.h --
 *
 *	Declarations of the video display module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/kernel/Cvsroot/kernel/dev/devVid.h,v 9.3 91/04/16 17:12:59 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVVID
#define _DEVVID

#include <sprite.h>

extern ReturnStatus Dev_VidEnable _ARGS_((Boolean onOff));

#endif /* _DEVVID */
