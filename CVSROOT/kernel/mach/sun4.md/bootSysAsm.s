/*
 * bootSysAsm.s -
 *
 *     Contains code that is the first executed at boot time.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

.seg	"data"
.asciz "$Header$ SPRITE (Berkeley)"
.align 8
.seg	"text"

#include "machConst.h"

/*
 * "Start" is used for the -e option to the loader.  "SpriteStart" is
 * used for the prof module, which prepends an underscore to the name of
 * global variables and therefore can't find "_start".
 */

.align 8
.seg	"text"
.globl	start
.globl	_spriteStart
start:
_spriteStart:

	mov	%psr, %g1
	or	%g1, MACH_SR_HIGHPRIO, %g1	/* lock out interrupts */
	andn	%g1, MACH_CWP_BITS, %g1		/* set cwp to 0 */
	mov	%g1, %psr
	mov	0x2, %wim	/* set wim to window right behind us */
/*
 * The kernel has been loaded into the wrong location.  We copy it to the right
 * location by copying up 8 Meg worth of pmegs.  This is done in all contexts.
 * 8 Meg should be enough for the whole kernel.  We copy to the correct address,
 * MACH_KERN_START which is before MACH_CODE_START, which is where we told the
 * loader that the kernel would be loaded.
 * In this code, %g1 is segment, %g2 is context, %g3 is pmeg, and %g4 is
 * offset in control space to context register.  %g5 is a temporary register.
 */
	sethi	%hi(VMMACH_CONTEXT_OFF), %g4	/* set %g4 to context offset */
	or	%g4, %lo(VMMACH_CONTEXT_OFF), %g4
	clr	%g2				/* start with context 0 */
contextLoop:
						/* set context register */
	stba	%g2, [%g4] VMMACH_CONTROL_SPACE
	clr	%g3				/* start with 0th pmeg */
	sethi	%hi(MACH_KERN_START), %g1	/* pick starting segment */
	or	%g1, %lo(MACH_KERN_START), %g1
loopStart:
					/* set segment to point to new pmeg */
	stha	%g3, [%g1] VMMACH_SEG_MAP_SPACE
	add	%g3, 1, %g3			/* increment which pmeg */
	sethi	%hi(VMMACH_SEG_SIZE), %g5	/* increment which segment */
	add	%g1, %g5, %g1
	add	%g1, %lo(VMMACH_SEG_SIZE), %g1
	cmp	%g3, (0x800000 / VMMACH_SEG_SIZE)	/* last pmeg? */
	bne	loopStart			/* if not, continue */
	nop

	add	%g2, 1, %g2			/* increment context */
	cmp	%g2, VMMACH_NUM_CONTEXTS	/* last context? */
	bne	contextLoop			/* if not, continue */
	nop
						/* reset context register */
	stba	%g0, [%g4] VMMACH_CONTROL_SPACE

/*
 * Force a non-PC-relative jump to the real start of the kernel.
 */
	sethi	%hi(begin), %g1
	or	%g1, %lo(begin), %g1
	jmp	%g1				/* jump to "begin" */
	nop

begin:
	mov	%psr, %g1			/* turn interrupts back on */
	andn	%g1, MACH_ENABLE_LEVEL15_INTR, %g1
	mov	%g1, %psr
	/*
	 * Zero out the bss segment.
	 */
	sethi	%hi(_edata), %g2
	or	%g2, %lo(_edata), %g2
	sethi	%hi(_end), %g3
	or	%g3, %lo(_end), %g3
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
	mov	0x7, %g6
	bne	zeroing
	nop
doneZeroing:

	/*
	 * Set the stack pointer for the initial process.
	 * start == MACH_CODE_START == where we asked loader to start loading
	 * kernel text.
	 */
	mov	start, %o0
	call	printArg
	nop

#ifdef NOTDEF
	mov	start, %sp
	mov	%tbr, %g4	/* save prom tbr */
#endif NOTDEF
	call	_main
	nop

.align 8
/*
 * printArg:
 *
 * Move integer argument to print into %o0.  This will print
 * desired integer in hex.
 */
printArg:
	.seg	"data1"
argString:
	.ascii	"printArg: %x\012\0"
	.seg	"text"

	mov	%o0, %o1
	set	argString, %o0
	mov	%o7, %g6
	sethi   %hi(-0x17ef7c),%g1
	ld      [%g1+%lo(-0x17ef7c)],%g1
	call    %g1,2
	nop
	mov	%g6, %o7
	retl
	nop
.align 8
_machProtoVectorTable:
	rd	%tbr, %g1
	and	%g1, 0xff0, %g1
	add	%g1, %g4, %g1
	jmp	%g1
	nop
