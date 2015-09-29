/*
 * dev.h --
 *
 *	Types, constants, and macros exported by the device module.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/dev/RCS/dev.h,v 1.4 92/04/29 22:08:53 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _DEV
#define _DEV

#include <sprite.h>
#include <cfuncproto.h>
#include <mach.h>
#include <spriteTime.h>

/*
 *	DEV_BYTES_PER_SECTOR the common size for disk sectors.
 */
#define DEV_BYTES_PER_SECTOR	512

extern Time		dev_LastConsoleInput;
extern mach_port_t 	dev_ServerPort;

extern void Dev_Init _ARGS_((void));
extern int Dev_ConsoleWrite _ARGS_((int numBytes, Address buffer));

#endif /* _DEV */
