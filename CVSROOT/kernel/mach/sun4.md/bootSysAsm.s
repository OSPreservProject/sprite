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
#include "machAsmDefs.h"

/*
 * "Start" is used for the -e option to the loader.  "SpriteStart" is
 * used for the prof module, which prepends an underscore to the name of
 * global variables and therefore can't find "_start".
 *
 * I use a lot global registers here for start up.  Elsewhere I'm careful.
 */

.align 8
.seg	"text"
.globl	start
.globl	_spriteStart
start:
_spriteStart:
	mov	%psr, %g1
	or	%g1, MACH_DISABLE_INTR, %g1	/* lock out interrupts */
	andn	%g1, MACH_CWP_BITS, %g1		/* set cwp to 0 */
	mov	%g1, %psr
	mov	0x2, %wim	/* set wim to window right behind us */
	/*
	 * The kernel has been loaded into the wrong location.
	 * We copy it to the right location by copying up 8 Meg worth of pmegs.
	 * This is done in all contexts.  8 Meg should be enough for the whole
	 * kernel.  We copy to the correct address, MACH_KERN_START which is
	 * before MACH_CODE_START, which is where we told the linker that the
	 * kernel would be loaded.  In this code, %g1 is segment, %g2 is
	 * context, %g3 is pmeg, and %g4 is offset in control space to context
	 * register.  %g5 contains seg size.
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
	 *
	 * The %fp points to top word on stack of one's caller, so it points
	 * to the base of our stack.  %sp points to the top word on the
	 * stack for our current stack frame.   This must be set at least
	 * to enough room to save our in registers and local registers upon
	 * window overflow.
	 */
	set	MACH_STACK_START, %fp
	set	(MACH_STACK_START - MACH_SAVED_WINDOW_SIZE), %sp
	andn	%sp, 0x7, %sp			/* double-word aligned */

	/*
	 * Now set up initial trap table by copying machProtoVectorTable
	 * into reserved space at the correct alignment.  The table must
	 * be aligned on a MACH_TRAP_ADDR_MASK boundary, and it contains
	 * ~MACH_TRAP_ADDR_MASK + 1 bytes.  We copy doubles (8 bytes at
	 * a time) for speed.  %g1 is source for copy, %g2 is destination,
	 * %g3 is the counter copy, %g4 and %g5 are the registers used for
	 * the double-word copy, %l1 is for holding the size of the table,
	 * and %l2 contains the number of bytes to copy.  %g6 stores the
	 * original destination, so that we can do some further copies, and
	 * so that we can put it into the tbr..
	 */
	set	machProtoVectorTable, %g1		/* g1 contains src */
	set	reserveSpace, %g2			/* g2 to contain dest */
	set	(1 + ~MACH_TRAP_ADDR_MASK), %l1
	set	((1 + ~MACH_TRAP_ADDR_MASK) / 8), %l2	/* # bytes to copy */
	add	%g2, %l1, %g2				/* add size of table */
	and	%g2, MACH_TRAP_ADDR_MASK, %g2		/* align to 4k bound. */
	mov	%g2, %g6				/* keep value of dest */
	clr	%g3					/* clear counter */
copyingTable:
	ldd	[%g1], %g4				/* copy first 2 words */
	std	%g4, [%g2]
	ldd	[%g1 + 8], %g4				/* next 2 words */
	std	%g4, [%g2 + 8]
	add	%g2, 16, %g2				/* incr. destination */
	add	%g3, 2, %g3				/* incr. counter */
	cmp	%g3, %l2				/* how many copies */
	bne	copyingTable
	nop

	/*
	 * Now copy in the overflow and underflow trap code.  These traps
	 * bypass the regular preamble and postamble for speed, and because
	 * they are coded so that the only state they need save is the psr.
	 * %g6 was the trap table address saved from above.
	 */
	set	machProtoWindowOverflow, %g1		/* new src */
	set	MACH_WINDOW_OVERFLOW, %g2		/* get trap offset */
	add	%g6, %g2, %g2				/* offset in table */
	ldd	[%g1], %g4				/* copy first 2 words */
	std	%g4, [%g2]
	ldd	[%g1 + 8], %g4				/* copy next 2 words */
	std	%g4, [%g2 + 8]

	set	machProtoWindowUnderflow, %g1		/* new src */
	set	MACH_WINDOW_UNDERFLOW, %g2		/* get trap type */
	add	%g6, %g2, %g2				/* offset in table */
	ldd	[%g1], %g4				/* copy first 2 words */
	std	%g4, [%g2]
	ldd	[%g1 + 8], %g4				/* copy next 2 words */
	std	%g4, [%g2 + 8]

	mov	%g6, %tbr			/* switch in my trap address */
	MACH_WAIT_FOR_STATE_REGISTER()			/* let it settle for
							 * the necessary
							 * amount of time.  Note
							 * that during this
							 * wait period, we
							 * may get an interrupt
							 * to the old tbr if
							 * interrupts are
							 * disabled.  */
#ifdef NOTDEF
	mov	%psr, %g1			/* turn interrupts back on */
	and	%g1, MACH_ENABLE_INTR, %g1
	mov	%g1, %psr
	MACH_WAIT_FOR_STATE_REGISTER()
#endif NOTDEF
	call	_main
	nop
.align 8
.global	_PrintArg
/*
 * PrintArg:
 *
 * Move integer argument to print into %o0.  This will print
 * desired integer in hex.  This routine uses o0, o1, VOL_TEMP1, and VOL_TEMP2.
 */
_PrintArg:
	.seg	"data1"
argString:
	.ascii	"PrintArg: %x\012\0"
	.seg	"text"

	mov	%o0, %o1
	set	argString, %o0
	mov	%o7, %VOL_TEMP1
	sethi   %hi(-0x17ef7c),%VOL_TEMP2
	ld      [%VOL_TEMP2+%lo(-0x17ef7c)],%VOL_TEMP2
	call    %VOL_TEMP2, 2
	nop
	mov	%VOL_TEMP1, %o7
	retl
	nop

.globl	_MachTrap
/*
 * Reserve twice the amount of space we need for the trap table.
 * Then copy machProtoVectorTable into it repeatedly, starting at
 * a 4k-byte alignment.  This is dumb, but the assembler doesn't allow
 * me to do much else.
 *
 * Note that this filler cannot use l1 or l2 since that's where pc and npc
 * are written in a trap.
 */
.align	8
machProtoVectorTable:
	sethi	%hi(_MachTrap), %VOL_TEMP1	/* set _MachTrap, %VOL_TEMP1 */
	or	%VOL_TEMP1, %lo(_MachTrap), %VOL_TEMP1
	jmp	%VOL_TEMP1		/* must use non-pc-relative jump here */
	nop

machProtoWindowOverflow:
	sethi	%hi(MachHandleWindowOverflowTrap), %VOL_TEMP1
	or	%VOL_TEMP1, %lo(MachHandleWindowOverflowTrap), %VOL_TEMP1
	jmp	%VOL_TEMP1
	rd	%psr, %CUR_PSR_REG

machProtoWindowUnderflow:
	sethi	%hi(MachHandleWindowUnderflowTrap), %VOL_TEMP1
	or	%VOL_TEMP1, %lo(MachHandleWindowUnderflowTrap), %VOL_TEMP1
	jmp	%VOL_TEMP1
	rd	%psr, %CUR_PSR_REG

.align	8
reserveSpace:	.skip	0x2000
