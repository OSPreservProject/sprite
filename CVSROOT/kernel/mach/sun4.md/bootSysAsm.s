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
	/*
	 * set cwp = 0, traps off, intr priority to f
	 * Maybe set intr. prior to 0 with 0x80, or cwp to 1 with 0xf81?
	 * (Sun code just turns off interrupts.)
	
	mov	0xf80, %psr
/*
 * Do I need to set any context stuff?
 * For now, don't even copy the kernel up.
 */
	call	_main
	nop
