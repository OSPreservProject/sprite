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
	and	%g1, MACH_SR_HIGHPRIO, %g1	/* lock out interrupts */
	andn	%g1, MACH_CWP_BITS, %g1		/* set cwp to 0 */
	mov	%g1, %psr
	mov	0x2, %wim	/* set wim to window right behind us */
	mov	0x1, %g6
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
	mov	0x2, %g6
contextLoop:
						/* set context register */
	stba	%g2, [%g4] VMMACH_CONTROL_SPACE
	clr	%g3				/* start with 0th pmeg */
	sethi	%hi(MACH_KERN_START), %g1	/* pick starting segment */
	or	%g1, %lo(MACH_KERN_START), %g1
	mov	0x3, %g6
loopStart:
					/* set segment to point to new pmeg */
	stha	%g3, [%g1] VMMACH_SEG_MAP_SPACE
	add	%g3, 1, %g3			/* increment which pmeg */
	sethi	%hi(VMMACH_SEG_SIZE), %g5	/* increment which segment */
	add	%g1, %g5, %g1
	add	%g1, %lo(VMMACH_SEG_SIZE), %g1
	mov	0x4, %g6
	cmp	%g3, (0x800000 / VMMACH_SEG_SIZE)	/* last pmeg? */
	bne	loopStart			/* if not, continue */
	nop

	add	%g2, 1, %g2			/* increment context */
	mov	0x5, %g6
	cmp	%g2, VMMACH_NUM_CONTEXTS	/* last context? */
	bne	contextLoop			/* if not, continue */
	nop
						/* reset context register */
	mov	0x6, %g6
	stba	%g0, [%g4] VMMACH_CONTROL_SPACE

/*
 * Force a non-PC-relative jump to the real start of the kernel.
 */
	sethi	%hi(begin), %g1
	or	%g1, %lo(begin), %g1
	jmp	%g1
	nop

begin:
	mov	0x7, %g6
	mov	%psr, %g1			/* turn interrupts back on */
	or	%g1, MACH_ENABLE_LEVEL15_INTR, %g1
	mov	%g1, %psr
	mov	0x8, %g6
#ifdef NOTDEF
	mov	%tbr, %g4	/* save prom tbr */
	call	_main
#endif NOTDEF
	mov	_machProtoVectorTable, %o1
	call	printArg
	nop

.align 8
/*
 * printArg:
 *
 * Move integer argument to print into %o1.  This will infinite loop printing
 * desired integer.
 */
printArg:
	.seg	"data1"
printArg2:
	.ascii  "Hello World! arg is %x\012\0"
	.seg    "text"

	sethi   %hi(-0x17ef7c),%g1
	ld      [%g1+%lo(-0x17ef7c)],%g1
	mov	0x9, %g6
	set     printArg2,%o0
	call    %g1,2
	nop
endloop:
	b	printArg
	nop
	ret
	nop
.align 8
_machProtoVectorTable:			/* 0x48 */
	rd	%tbr, %g1
	and	%g1, 0xff0, %g1
	add	%g1, %g4, %g1
	jmp	%g1
	nop
