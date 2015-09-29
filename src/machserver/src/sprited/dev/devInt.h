/*
 * devInt.h --
 *
 *	Internal declarations for the dev module.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/dev/RCS/devInt.h,v 1.1 92/03/23 14:51:34 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVINT
#define _DEVINT

#include <tty.h>
#include <cfuncproto.h>

#define DEV_CONSOLE_UNIT	0 /* unit number for the console */

extern DevTty *DevTtyAttach _ARGS_((int unit));

#endif /* _DEVINT */
