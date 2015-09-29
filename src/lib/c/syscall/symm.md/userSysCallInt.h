/*|*
|* userSysCallInt.h --
|*
|*     Contains macro for stubs for user-level system calls.
|*
|* Copyright 1985, 1988 Regents of the University of California
|* Permission to use, copy, modify, and distribute this
|* software and its documentation for any purpose and without
|* fee is hereby granted, provided that the above copyright
|* notice appear in all copies.  The University of California
|* makes no representations about the suitability of this
|* software for any purpose.  It is provided "as is" without
|* express or implied warranty.
|*
|* rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/userSysCallInt.h,v 1.1 90/01/19 10:19:41 fubar Exp $ SPRITE (Berkeley)
|*
*/

#include "kernel/sysSysCall.h"
#include "kernel/machAsmDefs.h"
#include "kernel/machTrap.h"
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
 * ----------------------------------------------------------------------------
 */


/*
 * These are different in usage than in Dynix!  'c' is a constant,
 * not the syscall name.
 */
#define SVC0(s,c)         movl $0, %ecx; SVC(0,c)
#define SVC1(s,c)         leal SPARG0, %ecx; SVC(1,c)
#define SVC2(s,c)         leal SPARG0, %ecx; SVC(2,c)
#define SVC3(s,c)         leal SPARG0, %ecx; SVC(3,c)
#define SVC4(s,c)         leal SPARG0, %ecx; SVC(4,c)
#define SVC5(s,c)         leal SPARG0, %ecx; SVC(5,c)
#define SVC6(s,c)         leal SPARG0, %ecx; SVC(6,c)
#define SVC7(s,c)         leal SPARG0, %ecx; SVC(7,c)
#define SVC8(s,c)         leal SPARG0, %ecx; SVC(8,c)
#define SVC9(s,c)         leal SPARG0, %ecx; SVC(9,c)
#define SVC10(s,c)        leal SPARG0, %ecx; SVC(10,c)

/*
 * Actual SYS_CALL definition differs from normal Sprite definition:
 * Normal sprite def is "SYS_CALL(name, constant)"
 * our definition is "SYS_CALL(argcount, name, constant)"
 */

#define SYS_CALL(a,s,c)    ENTRY(s); SVC/**/a(s,c); RETURN

/* 
 * If we want to use the Dynix "SYS_REL" system call version
 * stuff, it'll go in here.  For now, forget about it.
 *
 * n == number of args,
 * c == system call number.
 */
#define SVC(n,c)    movl $c, %eax; int $T_SPRITE_SYSCALL


#ifdef noway
/* this will just cause a breakpoint trap, with the syscall number
 * in %eax.
 */
#define SYS_CALL(name, constant) \
	.text; \
	.align 2; \
	.globl _/**/name; _/**/name: \
	movl $constant, %eax; \
	int $3;

#endif _USERSYSCALLINT


#endif /* noway */
