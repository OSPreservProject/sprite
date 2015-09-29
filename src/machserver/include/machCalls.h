/*
 * machCalls.h --
 *
 *	Declarations that should properly be in Mach header files, but
 *	aren't or can't be used.  (Example: mach.h and mach_traps.h can't
 *	be included together, at least as of 2-Oct-91, because cpp gags.)
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
 * $Header: /user5/kupfer/spriteserver/include/RCS/machCalls.h,v 1.1 92/04/29 22:35:00 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _MACHCALLS
#define _MACHCALLS

#include <cfuncproto.h>
#include <mach.h>

extern mach_port_t mach_host_self _ARGS_((void));

#endif /* _MACHCALLS */
