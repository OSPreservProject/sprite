/*
 * fastBootAsm.s -
 *
 *     Contains code that is the first executed upon restart.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

.seg	"data"
.asciz "$Header$ SPRITE (Berkeley)"
.align 8
.seg	"text"

#include "machConst.h"
#include "machAsmDefs.h"

.align 8
.seg	"text"
.globl	_MachDoFastBoot
_MachDoFastBoot:
	mov	%psr, %g1
	or	%g1, MACH_DISABLE_INTR, %g1	/* lock out interrupts */

	/* Turn off caching */
	set	VMMACH_SYSTEM_ENABLE_REG, %g4
	lduba	[%g4] VMMACH_CONTROL_SPACE, %g5
	andn	%g5, VMMACH_ENABLE_CACHE_BIT | VMMACH_ENABLE_DVMA_BIT, %g5
	stba	%g5, [%g4] VMMACH_CONTROL_SPACE

	/* Set up windows */
	andn	%g1, MACH_CWP_BITS, %g1		/* set cwp to 0 */
	set	MACH_ENABLE_FPP, %g2
	andn	%g1, %g2, %g1			/* disable fp unit */
	mov	%g1, %psr
	mov	0x2, %wim	/* set wim to window right behind us */

	/* start in context 0 */
	set	VMMACH_CONTEXT_OFF, %g4		/* set %g4 to context offset */
	stba	%g0, [%g4] VMMACH_CONTROL_SPACE

	/*
	 * We need to copy the initialized data from where it was stored
	 * to where it should be.  We copy from storedData to etext for
	 * storedDataSize bytes.  This is done in all contexts.
	 XXX
	 * In this code, %g1 is segment, %g2 is
	 * context, %g3 is pmeg, and %g4 is offset in control space to context
	 * register.  %g5 contains seg size.
	 */
	/*
	 * Get destPtr in %g4.
	 */
	set	_etext, %g4
	/*
	 * Get number of bytes in %g2.
	 */
	set	_edata, %g2
	sub	%g2, %g4, %g2
	/*
	 * Get srcPtr in %g3.
	 */
	set	_storedData, %g3
/*	add	%g4, 8, %g4  This would seem to be right, but breaks it. */
CopyData:
	ldd	[%g3], %g6
	std	%g6, [%g4]
	add	%g3, 8, %g3
	add	%g4, 8, %g4
	subcc	%g2, 8, %g2
	bg	CopyData
	nop
	
/*
 * Force a non-PC-relative jump to the real start of the kernel.
 */
	set	begin, %g1
	jmp	%g1				/* jump to "begin" */
	nop
begin:
	/*
	 * Zero out the bss segment.
	 */
	set	_edata, %g2
	set	_end, %g3
	cmp	%g2, %g3	/* if _edata == _end, don't zero stuff. */
	be	doneZeroing
	nop
	clr	%g1
zeroing:
	/*
	 * Use store doubles for speed.  Both %g0 and %g1 are zeroes.
	 */
	std	%g0, [%g2]
	add	%g2, 0x8, %g2
	cmp	%g2, %g3
	bne	zeroing
	nop
doneZeroing:
	/* Set variable that refreshes tbr to my trap address. */
	set	_machTBRAddr, %g2
	mov	%tbr, %g3
	and	%g3, MACH_TRAP_ADDR_MASK, %g3
	st	%g3, [%g2]

	/*
	 * Now set the stack pointer to my own stack for the first kernel
	 * process.  The stack grows towards low memory.  I start it at
	 * the beginning of the text segment (CAREFUL: if loading demand-paged,
	 * then the beginning of the text segment is 32 bytes before the
	 * first code.  Set it really at the beginning of the text segment and
	 * not at the beginning of the code.), and it can grow up to
	 * MACH_KERN_START.
	 *
	 * The %fp points to top word on stack of one's caller, so it points
	 * to the base of our stack.  %sp points to the top word on the
	 * stack for our current stack frame.   This must be set at least
	 * to enough room to save our in registers and local registers upon
	 * window overflow (and for main to store it's arguments, although it
	 * doesn't have any...).
	 */

#ifdef NOTDEF
	/*
	 * Set main_Debug to true for debugging during boot.
	 */
	set	_main_Debug, %g1
	set	1, %g2
	st	%g2, [%g1]
#endif /* NOTDEF */
	set	MACH_STACK_START, %fp
	set	(MACH_STACK_START - MACH_FULL_STACK_FRAME), %sp
	andn	%sp, 0x7, %sp			/* double-word aligned */

	call	_main
	nop
