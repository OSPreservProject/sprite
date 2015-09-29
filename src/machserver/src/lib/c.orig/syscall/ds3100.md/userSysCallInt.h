/*
 * userSysCallInt.h --
 *
 *     Contains macro for stubs for user-level system calls.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/userSysCallInt.h,v 1.2 89/07/26 18:24:50 nelson Exp $ SPRITE (Berkeley)
 */

#include "kernel/sysSysCall.h"
#include "kernel/ds3100.md/machConst.h"
#include <regdef.h>

#ifndef _USERSYSCALLINT
#define _USERSYSCALLINT

/*
 * Magic number to put into t1 so we know that this is a Sprite system 
 * call and not a UNIX one.
 */
#define	SYS_MAGIC	0xbab1fade

/*
 * ----------------------------------------------------------------------------
 *
 * SYS_CALL --
 *
 *      Define a user-level system call.  The call sets up a trap into a 
 *	system-level routine with the appropriate constant passed as
 * 	an argument to specify the type of system call.
 * ----------------------------------------------------------------------------
 */

#define SYS_CALL(name, constant) \
	.globl name; name: \
	li t0, constant; \
	li t1, SYS_MAGIC; \
	syscall; \
	j ra

#endif _USERSYSCALLINT
