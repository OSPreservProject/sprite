/*
 * mcount.s --
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
.seg	"text"

/*
 *  Jump to C routine.
 */
    .global mcount
mcount:
    mov     %r31, %o0           /* get the caller's pc */
    mov     %r15, %o1           /* get the callee's pc */
    save    %sp, -112, %sp      /* save stack frame */
    mov     %i0, %o0            /* move caller's pc up into new stack frame */
    call    ___mcount           /* call C routine to do all the real work */
    mov     %i1, %o1            /* (delay slot) move callee's pc up */
    ret
    restore

