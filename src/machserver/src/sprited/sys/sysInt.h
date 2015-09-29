/*
 * spriteInt.h --
 *
 *	Routines and types used internally by the sys module.
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
 * $Header: /r3/kupfer/spriteserver/src/sprited/sys/RCS/sysInt.h,v 1.2 91/10/18 18:07:58 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SYSINT
#define _SYSINT

#include <cfuncproto.h>

extern void SysInitSysCall _ARGS_((void));
extern void SysBufferStats _ARGS_((void));

#endif /* _SYSINT */
