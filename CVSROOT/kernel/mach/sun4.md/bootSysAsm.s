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
