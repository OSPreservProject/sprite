/* 
 * profStack.s --
 *
 *	Procedures for managing stack traces.
 *
 * Prof_ThisFP --
 *	Return the frame pointer of our caller.
 *
 * Prof_CallerFP --
 *	Return the frame pointer of our caller's caller.
 *	(mod, return frame pointer of our caller)
 *
 * Prof_NextFP --
 *	Given a frame pointer, return the frame pointer to
 *	the previous stack frame.
 *
 * Prof_ThisPC --
 *	Given a frame pointer, return the PC from which this
 *	frame was called.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */
.seg	"data"
.asciz  "$Header$ SPRITE (Berkeley)"
.align 8
.seg	"text"

#include "machConst.h"
#include "machAsmDefs.h"

#if	defined(sun4) || defined(sparc)
!	.globl	_Prof_Whence
!	.globl	_Prof_NextFP
!	.globl	_Prof_ThisFP
!	.globl	_Prof_CallerFP
!	.globl	_Prof_ThisPC
	.globl	_Prof_CallerPC
	.globl	_Prof_CalleePC

/*
 * Prof_Whence, the stack looks like this when it gets called.
 *	There is no link instruction so we are still using mcount's
 *	(or whomever's) frame pointer.
 *
 *	a6 is the frame pointer register.
 *
 *     Bottom of the stack (it grows away from here)
 *  |----------------------------|
 *  | Args to Foo                |
 *  |----------------------------|
 *  | PC of jsr Foo              |
 *  |----------------------------|
 *  | Saved FP0                  |	<--  FP1	 (Foo's frame)
 *  |----------------------------|
 *  | Saved registers            |
 *  |----------------------------|
 *  | PC of jsr mcount           |
 *  |----------------------------|
 *  | Saved FP1                  |	<--  FP2	(mcount's frame)
 *  |----------------------------|
 *  | Saved registers            |
 *  |----------------------------|
 *  | PC of jsr Prof_Whence      |
 *  |----------------------------|	<--  SP
 *  
 */
#ifdef NOTDEF
_Prof_ThisFP:
	movl	a6,d0		/* just the frame pointer */
	rts

_Prof_CallerFP:
/*	movl	a6@,a0
	movl	a0@,d0 */
	movl	a6@,d0		/* just our caller's frame pointer */
	rts

_Prof_NextFP:
	movl	sp@(4),a0
	movl	a0@,d0
	rts

_Prof_ThisPC:
	movl	sp@(4),a0
	movl	a0@(4),d0
	rts
#endif NOTDEF
    /*
     * Move the return pc for the routine that called mcount (that's our
     * current frame) into the return value register and return.
     */
_Prof_CalleePC:
    mov	%RETURN_ADDR_REG_CHILD, %o0
    retl
    nop

    /*
     * Move the return pc for the routine that called the routine that called
     * mcount (one frame before our current frame) into the return value
     * register and return.  To do this, we must back up a window and push
     * the value over to our real window and move back to our real window.
     *
     * INTERRUPTS MUST BE OFF!!!  (Or we'll be in trouble jumping about
     * windows like this.)
     */
_Prof_CallerPC:
    restore
    mov	%RETURN_ADDR_REG_CHILD, %o4
    save
    mov	%i4, %o0
    retl
    nop

#endif	defined(sun4) || defined(sparc)
