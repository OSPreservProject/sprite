/*
 * setjmp.s --
 *
 *      Source code for the setjmp and longjmp library calls.
 *
 * Copyright (C) 1989 by Digital Equipment Corporation, Maynard MA
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 *
 * Digitial disclaims all warranties with regard to this software, including
 * all implied warranties of merchantability and fitness.  In no event shall
 * Digital be liable for any special, indirect or consequential damages or
 * any damages whatsoever resulting from loss of use, data or profits,
 * whether in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of this
 * software.
 *
 * Header: Sync_GetLock.s,v 1.1 88/06/19 14:34:17 ouster Exp $ SPRITE (DECWRL)
 */

#ifdef KERNEL
#include <regdef.h>
#else
#include <regdef.h>
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * setjmp --
 *
 *      Perform a setjmp operation.
 *
 * Results:
 *      Always returns 0.
 *
 * Side effects:
 *      The state of the world is stored away.
 *
 * C equivalent:
 *
 *      int setjmp(env)
 *	jmp_buf	env;
 *
 *----------------------------------------------------------------------
 */
    .globl setjmp
    .globl _setjmp
setjmp:
    subu	sp, sp, 32
    sw		ra, 28(sp)
    sw		a0, 24(sp)
    add		a0, zero, zero		# Get current signal mask.
    jal		sigblock
    lw		a0, 24(sp)
    lw		ra, 28(sp)
    addu	sp, sp, 32
_setjmp:
    sw		zero, 0(a0)		# On sig stack flag = 0
    sw		v0, 4(a0)		# Current signal mask.
    sw		ra, 8(a0)		# Return address.
    sw		gp, 124(a0)
    sw		sp,128(a0)
    sw    	s0,76(a0)
    sw    	s1,80(a0)
    sw    	s2,84(a0)
    sw    	s3,88(a0)
    sw    	s4,92(a0)
    sw    	s5,96(a0)
    sw    	s6,100(a0)
    sw    	s7,104(a0)
    sw    	s8,132(a0)
    swc1   	$f20,232(a0)
    swc1   	$f21,236(a0)
    swc1   	$f22,240(a0)
    swc1   	$f23,244(a0)
    swc1   	$f24,248(a0)
    swc1   	$f25,252(a0)
    swc1   	$f26,256(a0)
    swc1   	$f27,260(a0)
    swc1   	$f28,264(a0)
    swc1   	$f29,268(a0)
    swc1   	$f30,272(a0)
    swc1   	$f31,276(a0)
    cfc1	v0, $31
    sw		v0, 280(a0)
    add		v0, zero, zero
    j		ra

/*
 * ----------------------------------------------------------------------------
 *
 * longjmp --
 *
 *      Perform a longjmp operation.
 *
 * Results:
 *      Returns val.
 *
 * Side effects:
 *      State of the world is restored.
 *
 * C equivalent:
 *
 *      int longjmp(env, val)
 *	jmp_buf	env;
 *	int val;
 *
 *----------------------------------------------------------------------
 */
    .globl longjmp
    .globl _longjmp
longjmp:
    subu	sp, sp, 32
    sw		a0, 24(sp)
    sw		a1, 28(sp)

    lw		a0, 4(a0)
    jal		sigsetmask

    lw		a0, 24(sp)
    lw		a1, 28(sp)
_longjmp:
    lw		ra, 8(a0)
    lw		gp, 124(a0)
    lw		sp,128(a0)
    lw    	s0,76(a0)
    lw    	s1,80(a0)
    lw    	s2,84(a0)
    lw    	s3,88(a0)
    lw    	s4,92(a0)
    lw    	s5,96(a0)
    lw    	s6,100(a0)
    lw    	s7,104(a0)
    lw    	s8,132(a0)
    lwc1   	$f20,232(a0)
    lwc1   	$f21,236(a0)
    lwc1   	$f22,240(a0)
    lwc1   	$f23,244(a0)
    lwc1   	$f24,248(a0)
    lwc1   	$f25,252(a0)
    lwc1   	$f26,256(a0)
    lwc1   	$f27,260(a0)
    lwc1   	$f28,264(a0)
    lwc1   	$f29,268(a0)
    lwc1   	$f30,272(a0)
    lwc1   	$f31,276(a0)
    lw		v0, 280(a0)
    ctc1	v0, $31

    add		v0, a1, zero
    bne		v0, zero, 1f
    li		v0, 1
1:  j		ra

