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

/*
 * "Start" is used for the -e option to the loader.  "SpriteStart" is
 * used for the prof module, which prepends an underscore to the name of
 * global variables and therefore can't find "_start".
 */

.seg	"text"

/*
 *	The temporary trap table.  I don't fill it in all the way.
 *	NOTE: set instruction is a 2-word pseudo op.
 */

.globl	start
.globl	_spriteStart
start:
_spriteStart:
	/*
	 * set cwp = 0, traps enabled, intr priority to f
	 * Maybe set cwp to 1 with 0xfa1?
	 mov	0xf80, %psr
	 */

	mov	%psr, %g1
	/* set cwp to 0 and disable traps */
	andn	%g1, 0x3f, %g1
	/* set interrupt level */
	or	%g1, 0xf00, %g1
	wr	%g0, %g1, %psr
	/* turn traps back on */
	mov	%psr, %g1
	or	%g1, 0x20, %g1
	wr	%g0, %g1, %psr
	mov	0x2, %wim

	/* set wim to window right behind us */

	call	_main
	nop
