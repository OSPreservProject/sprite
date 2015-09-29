/*-
 * start.s --
 *	Function to start the loaded kernel running.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
	.data
_rcsid:
	.ascii "$Header$ SPRITE (Berkeley)\0"
	.even
hello:
	.ascii "Hi there! d0 = %x\11\0"
	.text
	
#define ASM	This is assembler code!
#include	"boot.h"

	.globl	_end, _edata, _main

	.globl	start
start:
	movw	#0x2700,sr	/* Disable interrupts */
	lea	0,a0		/* Start shifting from 0 */
	lea	BOOT_START,a1	/* Move to BOOT_START */
	movl	#_end,d1	/* Figure how many bytes need moving */
	subl	a1,d1		/* by subtracting the destination of the
				 * move from the end of our image */
	lsrl	#2,d1		/* Count in longwords */
loop:
	movl	a0@+,a1@+
	dbra	d1,loop

/*
 * Clear out any bss data we've got, now that we're in the right position.
 */
	lea	_edata,a0
	movl	#_end,d1
	subl	a0,d1
	lsrl	#2,d1
bssloop:
	clrl	a0@+
	dbra	d1,bssloop

	jsr	_main:l		/* Call main (arguments?) */ 

	movl	0xfef00c4:l,a0	/* call v_exit_to_mon */
	jsr	a0@
	movl	d0,a0
	jmp	a0@

	.globl	_startKernel
_startKernel:
	movl	sp@(4),a0	/* Expect starting address as first arg */
	jmp	a0@		/* Start the kernel */

