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

.globl	start
.globl	_spriteStart
start:
_spriteStart:

	mov	%psr, %g1
	andn	%g1, 0x1f, %g1			/* set cwp to 0 */
	mov	%g1, %psr
	mov	0x2, %wim	/* set wim to window right behind us */


	call	_main
	nop
