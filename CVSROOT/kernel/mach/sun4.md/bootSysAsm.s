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
 * offset in control space to context register.  %g5 contains seg size.
 */
	set	VMMACH_CONTEXT_OFF, %g4		/* set %g4 to context offset */
	clr	%g2				/* start with context 0 */
	set	VMMACH_SEG_SIZE, %g5		/* for additions */
contextLoop:
						/* set context register */
	stba	%g2, [%g4] VMMACH_CONTROL_SPACE
	clr	%g3				/* start with 0th pmeg */
	set	MACH_KERN_START, %g1		/* pick starting segment */
loopStart:
					/* set segment to point to new pmeg */
	stha	%g3, [%g1] VMMACH_SEG_MAP_SPACE
	add	%g3, 1, %g3			/* increment which pmeg */
	add	%g1, %g5, %g1			/* increment which segment */
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
	set	begin, %g1
	jmp	%g1				/* jump to "begin" */
	nop

begin:
	mov	%psr, %g1			/* turn interrupts back on */
	andn	%g1, MACH_ENABLE_LEVEL15_INTR, %g1
	mov	%g1, %psr
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
	/*
	 * Now set the stack pointer to my own stack for the first kernel
	 * process.  The stack grows towards low memory.  I start it at
	 * the beginning of the text segment (CAREFUL: if loading demand-paged,
	 * then the beginning of the text segment is 32 bytes before the
	 * first code.  Set it really at the beginning of the text segment and
	 * not at the beginning of the code.), and it can grow up to
	 * MACH_KERN_START.
	 */
	set	MACH_KERN_STACK_START, %sp

	/*
	 * Now set up initial trap table by copying machProtoVectorTable
	 * into reserved space at the correct alignment.  The table must
	 * be aligned on a MACH_TRAP_ADDR_MASK boundary, and it contains
	 * ~MACH_TRAP_ADDR_MASK + 1 bytes.  We copy doubles (8 bytes at
	 * a time) for speed.  %g1 is source for copy, %g2 is destination,
	 * %g3 is the counter copy, %g4 and %g5 are the registers used for
	 * the double-word copy, %l1 is for holding the size of the table,
	 * and %l2 contains the number of bytes to copy.
	 */
	mov	0x1, %g6
	set	machProtoVectorTable, %g1		/* g1 contains src */
#ifdef NOTDEF
	mov	%g1, %o0
	call	_printArg
	nop
#endif NOTDEF
	mov	0x2, %g6
#ifdef NOTDEF
	set	reserveSpace, %o0
	call	_printArg
	nop
#endif NOTDEF
	set	reserveSpace, %g2			/* g2 to contain dest */
	set	(1 + ~MACH_TRAP_ADDR_MASK), %l1
	set	((1 + ~MACH_TRAP_ADDR_MASK) / 8), %l2	/* # bytes to copy */
	add	%g2, %l1, %g2				/* add size of table */
	and	%g2, MACH_TRAP_ADDR_MASK, %g2		/* align to 4k bound. */
#ifdef NOTDEF
	mov	%g2, %o0
	call	_printArg
	nop
#endif NOTDEF
	clr	%g3					/* clear counter */
	mov	0x3, %g6
copyingTable:
#ifdef NOTDEF
	ldda	[%g1] VMMACH_KERN_PROGRAM_SPACE, %g4	/* %g4 and %g5 in */
	mov	0x4, %g6
	stda	%g4, [%g2] VMMACH_KERN_PROGRAM_SPACE	/* %g4 and %g5 out */
#else /* NOTDEF */
	mov	0x4, %g6
	ldd	[%g1], %g4				/* %g4 and %g5 in */
	std	%g4, [%g2] 				/* %g4 and %g5 out */
	ldd	[%g1 + 8], %g4
	std	%g4, [%g2 + 8]
#endif /* NOTDEF */
	mov	0x5, %g6
	add	%g2, 16, %g2				/* incr. destination */
	add	%g3, 2, %g3				/* incr. counter */
	cmp	%g3, %l2				/* how many copies */
	bne	copyingTable
	nop
	mov	0x6, %g6
	mov	%g3, %o0
	mov	%g2, %l5
	call	_printArg				/* print counter */
	nop
	mov	%l5, %o0
	call	_printArg				/* print after dest */
	nop
	mov	0x7, %g6
	set	reserveSpace, %g2			/* g2 to be trap base */
	add	%g2, %l1, %g2				/* add size of table */
	and	%g2, MACH_TRAP_ADDR_MASK, %g2		/* align to 4k bound. */
	call	_main
	nop
.align 8
.global	_printArg
/*
 * printArg:
 *
 * Move integer argument to print into %o0.  This will print
 * desired integer in hex.  This routine uses o0, o1, l3, and l4.
 */
_printArg:
	.seg	"data1"
argString:
	.ascii	"printArg: %x\012\0"
	.seg	"text"

	mov	%o0, %o1
	set	argString, %o0
	mov	%o7, %l3
	sethi   %hi(-0x17ef7c),%l4
	ld      [%l4+%lo(-0x17ef7c)],%l4
	call    %l4,2
	nop
	mov	%l3, %o7
	retl
	nop
/*
 * Reserve twice the amount of space we need for the trap table.
 * Then copy machProtoVectorTable into it repeatedly, starting at
 * a 4k-byte alignment.  This is dumb, but the assembler doesn't allow
 * me to do much else.
 */
.align	8
machProtoVectorTable:
	rd	%tbr, %g1
	and	%g1, 0xff0, %g1
	add	%g1, %g4, %g1
	jmp	%g1
	nop				/* nop isn't copied, but quiets AS */
.align	8
reserveSpace:	.skip	0x2000
