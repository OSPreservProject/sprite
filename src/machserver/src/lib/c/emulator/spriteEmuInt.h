/*
 * spriteEmuInt.h --
 *
 *	Internal declarations for the Sprite emulator library.
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
 * $Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/spriteEmuInt.h,v 1.3 92/03/12 19:22:50 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITEEMUINT
#define _SPRITEEMUINT

#include <mach.h>
#include <status.h>

extern mach_port_t SpriteEmu_ServerPort _ARGS_((void));

extern void SpriteEmu_TakeSignals _ARGS_((void));

/* 
 * There is a declaration for mach_thread_self in <mach/mach_traps.h>, but
 * there's a problem with the declaration for mach_task_self that makes 
 * various cpp's unhappy.
 */
extern mach_port_t mach_thread_self();

extern ReturnStatus Utils_MapMachStatus _ARGS_((kern_return_t kernStatus));
				/* XXX this probably should go elsewhere, 
				 * with a different name. */

#endif /* _SPRITEEMUINT */
