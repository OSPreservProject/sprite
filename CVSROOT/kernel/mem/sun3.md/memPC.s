/* 
 * memPC.s --
 *
 * 	Procedures for obtain the PC of the caller of a routine.
 *
 *  $Header$ SPRITE (Berkeley)
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */


#if	defined(VAX) || defined(vax) || defined(uvax)

/*
 * The stack layout on a VAX when Mem_CallerPC is called. The calls instruction
 * on the VAX updates the frame pointer, so we have to go back two levels
 * on the stack to get the PC of the call to Mem_Alloc/Free.
 *
 *   Foo calls Mem_Alloc (or Mem_Free):
 *
 * Top of the stack (it grows down from here)
 *    +----------------------------+
 *    | ... (Foo's frame)          |
 *    |----------------------------|
 *    | Arg. to Mem_Alloc          |
 *    |----------------------------|
 *    | # of args to Mem_Alloc     |
 *    |----------------------------|
 * 16 | PC of "calls Mem_Alloc"    |	2) left in r0
 *    |----------------------------|
 * 12 | Foo's FP                   |
 *    |----------------------------|
 *  8 | Foo's AP                   |
 *    |----------------------------|
 *  4 | mask/PSW                   |
 *    |----------------------------|
 *  0 | condition handler          | :Mem_Alloc's FP
 *    |----------------------------|
 *    | local variables            |
 *    |----------------------------|
 * 16 | PC of "calls Mem_CallerPC" |
 *    |----------------------------|
 * 12 | Mem_Alloc's FP             |	1) copy to r0
 *    |----------------------------|
 *  8 | Mem_Alloc's AP             |
 *    |----------------------------|
 *  4 | mask/PSW                   |
 *    |----------------------------|
 *  0 | condition handler          | :Mem_CallerPC's FP
 *    +----------------------------+
 */

	.globl	_Mem_CallerPC
_Mem_CallerPC:
	.word 0
	movl	12(fp),r0	/* 1) r0 = fp of caller */
	movl	16(r0),r0	/* 2) r0 = pc with fp */
	ret

#endif	defined(VAX) || defined(vax) || defined(uvax)


#if	defined(mc68000) || defined(sun2) || defined(sun3)
/*
 * The stack layout on a MC680?? when Mem_CallerPC is called. The routine 
 * doesn't use a link instruction so it is still using Mem_Alloc/Free's 
 * frame pointer (reg. a6).
 *
 *   Foo calls Mem_Alloc (or Mem_Free):
 *
 * Top of the stack (it grows down from here)
 *   +----------------------------+
 *   | ....                       |
 *   |----------------------------|
 *   | Saved Foo's caller FP      |
 *   |----------------------------|
 *   | Saved registers, local var |
 *   |----------------------------|
 * 4 | PC of "jsr Mem_Alloc"      |
 *   |----------------------------|
 * 0 | Saved Foo's FP             | :A6 (Mem_Alloc's FP)
 *   |----------------------------|
 *   | Saved registers, local var |
 *   |----------------------------|
 *   | PC of "jsr Mem_CallerPC"   |
 *   |----------------------------|
 *   | ....                       | :SP
 *   +----------------------------+
 */

	.globl	_Mem_CallerPC
_Mem_CallerPC:
	movl	a6@(4),d0	/* PC of the caller of Mem_Alloc. */
	rts

#endif	defined(mc68000) || defined(SUN2) || defined(SUN3)
