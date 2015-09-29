/*
 * console.h --
 *
 *	Declarations for things exported by devConsole.c to the rest
 *	of the device module.
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
 * $Header: /sprite/src/kernel/dev/ds3100.md/RCS/console.h,v 1.2 89/07/27 10:36:33 nelson Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVCONSOLE
#define _DEVCONSOLE

#include "dc7085.h"

extern int	DevConsoleRawProc();

extern Boolean	devDivertXInput;

/*
 * Maximum interval allowed between hitting the console cmd key and hitting the
 * command key.
 */
#define CONSOLE_CMD_INTERVAL	2

#endif /* _DEVCONSOLE */
