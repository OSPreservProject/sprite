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
 * rcs = $Header: /sprite/src/lib/c/syscall/sun4.md/RCS/userSysCallInt.h,v 1.3 89/05/09 23:14:46 rab Exp Locker: rab $ SPRITE (Berkeley)
 */

#include "kernel/sysSysCall.h"
#include "kernel/machConst.h"
#ifndef _USERSYSCALLINT
#define _USERSYSCALLINT
/*
 * ----------------------------------------------------------------------------
 *
 * SYS_CALL --
 *
 *      Define a user-level system call.  The call sets up a trap into a 
 *	system-level routine with the appropriate constant passed as
 * 	an argument to specify the type of system call.
 *
 *	1) Put constant into global register %g1.  This is what sun OS does,
 *	so the compiler and C library have agreed it's okay to trash g1 when
 *	taking a system call.
 *	2) Execute a software trap instruction.  Arguments to the trap
 *	are left in the output registers, since this is a leaf routine, and
 *	they will become the input registers to the trap window.
 *	3) The return value is left in %o0, since this is a leaf routine,
 *	and it will be found correctly in %o0 by our caller.
 *
 * ----------------------------------------------------------------------------
 */

#ifdef __STDC__
#define SYS_CALL(name, constant)	\
	.globl _ ## name; _ ## name:	\
	set	constant, %g1;		\
	ta MACH_SYSCALL_TRAP; 		\
	retl;				\
	nop
#else
#define SYS_CALL(name, constant)	\
	.globl _/**/name; _/**/name:	\
	set	constant, %g1;		\
	ta MACH_SYSCALL_TRAP; 		\
	retl;				\
	nop
#endif
#endif /* _USERSYSCALLINT */
