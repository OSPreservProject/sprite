/* loMem.s -
 *
 *     Contains code that is the first executed at boot time.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#include "machConst.h"
#include <regdef.h>

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME	((4 * 4) + 4 + 4)

    .globl	start
    .globl	eprol
start:
eprol:
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    li		sp, MACH_CODE_START - START_FRAME
    la		gp, _gp
    sw		zero, START_FRAME - 4(sp)	# Zero out old ra for debugger
    sw		zero, START_FRAME - 8(sp)	# Zero out old fp for debugger
    jal		main				# main(argc, argv, envp)
    add		a0, zero, zero			
    jal		Proc_Exit			# Proc_Exit(0)
