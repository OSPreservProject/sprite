/*
 * machAsmDefs.h --
 *
 *     Machine-dependent macros.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHASMDEFS
#define _MACHASMDEFS

#ifdef KERNEL
#include "vmSun4Const.h"
#include "machConst.h"
#else
#include <kernel/vmSun4Const.h>
#include <kernel/machConst.h>
#endif

#define	MACH_ADVANCE_WIM(REG1, REG2)			\
	mov	%wim, REG1;				\
	srl	REG1, 0x1, REG2;			\
	sll	REG1, (MACH_NWINDOWS - 1), REG1;	\
	or	REG1, REG2, REG1;			\
	set	MACH_VALID_WIM_BITS, REG2;		\
	and	REG1, REG2, REG1;			\
	mov	REG1, %wim

#define	MACH_RETREAT_WIM(REG1, REG2)			\
	mov	%wim, REG1;				\
	sll	REG1, 0x1, REG1;			\
	set	MACH_VALID_WIM_BITS, REG2;		\
	andcc	REG2, REG1, REG1;			\
	bne	happy_wim;				\
	nop;						\
	mov	0x1, REG1;				\
happy_wim:						\
	mov	REG1, %wim

/*
 * Save r16 to r23 (locals) and r24 to r31 (ins) to 16 words at
 * the top of this window's stack.
 */
#define	MACH_SAVE_WINDOW_TO_STACK()			\
	std	%r16, [%sp];				\
	std	%r18, [%sp + 8];			\
	std	%r20, [%sp + 16];			\
	std	%r22, [%sp + 24];			\
	std	%r24, [%sp + 32];			\
	std	%r26, [%sp + 40];			\
	std	%r28, [%sp + 48];			\
	std	%r30, [%sp + 56]

#define	MACH_RESTORE_WINDOW_FROM_STACK()		\
	ldd	[%sp], %r16;				\
	ldd	[%sp + 8], %r18;			\
	ldd	[%sp + 16], %r20;			\
	ldd	[%sp + 24], %r22;			\
	ldd	[%sp + 32], %r24;			\
	ldd	[%sp + 40], %r26;			\
	ldd	[%sp + 48], %r28;			\
	ldd	[%sp + 56], %r30


/*
 * Clear out the local and out registers for a new window to move into
 * or a window we're moving out of.
 * Outs: r8 to r15, locals: r16 to r23.
 */
#define	MACH_CLEAR_WINDOW()				\
	clr	%r8;					\
	clr	%r9;					\
	clr	%r10;					\
	clr	%r11;					\
	clr	%r12;					\
	clr	%r13;					\
	clr	%r14;					\
	clr	%r15;					\
	clr	%r16;					\
	clr	%r17;					\
	clr	%r18;					\
	clr	%r19;					\
	clr	%r20;					\
	clr	%r21;					\
	clr	%r22;					\
	clr	%r23





#endif /* _MACHASMDEFS */
