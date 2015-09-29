/*
 * gmcount.s --
 *
 *	Entry point for profiling routine to record each
 *      procedure call, so that the call graph can be
 *      constructed.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

.seg	"data"
.asciz	"$Header$"
.align	8

    .globl _profiling
_profiling:
    .byte   3

.seg	"text"

/*
 *  Jump to C routine.
 */
    .global mcount
mcount:
	mov	%r31, %o0           /* get the caller's pc */
	mov	%r15, %o1           /* get the callee's pc */
        save	%sp, -112, %sp
	mov     %i0, %o0
	call    ___mcount
	mov     %i1, %o1
	ret
	restore

