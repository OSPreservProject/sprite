/* 
 * profStack.s --
 *
 * Procedures for managing stack traces.
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
 *  $Header$ SPRITE (Berkeley)
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
#endif not lint


#if	defined(VAX) || defined(vax)
	/*
	 * The calls instruction on the VAX updates the
	 * frame pointer, so we have to go back two levels
	 * on the stack.
	 */
	.globl	_Prof_Whence
	.globl	_Prof_NextFP
	.globl	_Prof_ThisPC
_Prof_Whence:
	.word	0
	movl	12(fp),r0	/* r0/ fp of caller */
	movl	12(r0),r0	/* r0/ fp of caller's caller */
	ret

_Prof_NextFP:
	.word 	0
	movl	4(ap),r0	/* r0/ argument fp */
	movl	12(r0),r0	/* r0/ previous fp */
	ret

_Prof_ThisPC:
	.word	0
	movl	4(ap),r0	/* r0/ argument fp */
	movl	16(r0),r0	/* r0/ pc with fp */
	ret
#endif	defined(VAX) || defined(vax)


#if	defined(mc68000) || defined(SUN2)
|	.globl	_Prof_Whence
	.globl	_Prof_NextFP
	.globl	_Prof_ThisFP
	.globl	_Prof_CallerFP
	.globl	_Prof_ThisPC
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
#endif	defined(mc68000) || defined(SUN2)
