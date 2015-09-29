/* start.s -
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
 * $Header: /sprite/src/boot/decprom/ds3100.md/RCS/start.s,v 1.1 90/02/16 16:19:39 shirriff Exp $ SPRITE (DECWRL)
 */

#include <regdef.h>
#include "kernel/machConst.h"

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME	((4 * 4) + 4 + 4)
#define Init	0xbfc00018

    .globl	start
start:
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    li		sp, MACH_CODE_START - START_FRAME
    la		gp, _gp
    sw		zero, START_FRAME - 4(sp)	# Zero out old ra for debugger
    sw		zero, START_FRAME - 8(sp)	# Zero out old fp for debugger
    jal		main				# main(argc, argv, envp)
    li		a0, Init			# done, so call prom
    j		a0

    .globl	Boot_Transfer
Boot_Transfer:
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    li		sp, MACH_CODE_START - START_FRAME
    la		gp, _gp
    sw		zero, START_FRAME - 4(sp)	# Zero out old ra for debugger
    sw		zero, START_FRAME - 8(sp)	# Zero out old fp for debugger
    move	t0,a0				#shift arguments down
    move	a0,a1
    move	a1,a2
    move	a2,a3
    jal		t0				# Jump to routine
    li		a0, Init			# done, so call prom
    j		a0
