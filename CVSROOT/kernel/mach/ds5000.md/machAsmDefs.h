/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _MACHASMDEFS
#define _MACHASMDEFS

#include "machConst.h"

/*
 * LEAF(x)
 *
 *	Declare a leaf routine.
 */
#define LEAF(x) \
	.globl x; \
	.ent x, 0; \
x: ; \
	.frame sp, 0, ra

/*
 * NON_LEAF(x)
 *
 *	Declare a non-leaf routine (a routine that makes other C calls).
 */
#define NON_LEAF(x, fsize, retpc) \
	.globl x; \
	.ent x, 0; \
x: ; \
	.frame sp, fsize, retpc

/*
 * END(x)
 *
 *	Mark end of a procedure.
 */
#define END(x) \
	.end x

/*
 * SAVE_REGS(baseReg, offset)
 *
 *	Save all of the registers except for the kernel temporaries in the
 *	area pointed to by baseReg + offset.
 */
#define SAVE_REGS(baseReg, offset) \
    sw		AT, (offset) +  4(baseReg); \
    sw		v0, (offset) +  8(baseReg); \
    sw		v1, (offset) +  12(baseReg); \
    sw		a0, (offset) +  16(baseReg); \
    sw		a1, (offset) +  20(baseReg); \
    sw		a2, (offset) +  24(baseReg); \
    sw		a3, (offset) +  28(baseReg); \
    sw		t0, (offset) +  32(baseReg); \
    sw		t1, (offset) +  36(baseReg); \
    sw		t2, (offset) +  40(baseReg); \
    sw		t3, (offset) +  44(baseReg); \
    sw		t4, (offset) +  48(baseReg); \
    sw		t5, (offset) +  52(baseReg); \
    sw		t6, (offset) +  56(baseReg); \
    sw		t7, (offset) +  60(baseReg); \
    sw		s0, (offset) +  64(baseReg); \
    sw		s1, (offset) +  68(baseReg); \
    sw		s2, (offset) +  72(baseReg); \
    sw		s3, (offset) +  76(baseReg); \
    sw		s4, (offset) +  80(baseReg); \
    sw		s5, (offset) +  84(baseReg); \
    sw		s6, (offset) +  88(baseReg); \
    sw		s7, (offset) +  92(baseReg); \
    sw		t8, (offset) +  96(baseReg); \
    sw		t9, (offset) +  100(baseReg); \
    sw		s8, (offset) +  120(baseReg); \
    sw		ra, (offset) +  124(baseReg)

/*
 * RESTORE_REGS(baseReg, offset)
 *
 *	Restore all of the registers except for the kernel's temporaries
 *	to the area pointed to by baseReg + offset.
 */
#define RESTORE_REGS(baseReg, offset) \
    lw		AT, (offset) +  4(baseReg); \
    lw		v0, (offset) +  8(baseReg); \
    lw		v1, (offset) +  12(baseReg); \
    lw		a0, (offset) +  16(baseReg); \
    lw		a1, (offset) +  20(baseReg); \
    lw		a2, (offset) +  24(baseReg); \
    lw		a3, (offset) +  28(baseReg); \
    lw		t0, (offset) +  32(baseReg); \
    lw		t1, (offset) +  36(baseReg); \
    lw		t2, (offset) +  40(baseReg); \
    lw		t3, (offset) +  44(baseReg); \
    lw		t4, (offset) +  48(baseReg); \
    lw		t5, (offset) +  52(baseReg); \
    lw		t6, (offset) +  56(baseReg); \
    lw		t7, (offset) +  60(baseReg); \
    lw		s0, (offset) +  64(baseReg); \
    lw		s1, (offset) +  68(baseReg); \
    lw		s2, (offset) +  72(baseReg); \
    lw		s3, (offset) +  76(baseReg); \
    lw		s4, (offset) +  80(baseReg); \
    lw		s5, (offset) +  84(baseReg); \
    lw		s6, (offset) +  88(baseReg); \
    lw		s7, (offset) +  92(baseReg); \
    lw		t8, (offset) +  96(baseReg); \
    lw		t9, (offset) +  100(baseReg); \
    lw		gp, (offset) +  112(baseReg); \
    lw		sp, (offset) +  116(baseReg); \
    lw		s8, (offset) +  120(baseReg); \
    lw		ra, (offset) +  124(baseReg)

/*
 * SAVE_KERNEL_REGS()
 *
 *	Save all of the kernel's relevant registers onto the stack.
 */
#define SAVE_KERNEL_REGS(offset) \
    sw		AT,(offset) + 0(sp); \
    sw		v0,(offset) + 4(sp); \
    sw		v1,(offset) + 8(sp); \
    sw		a0,(offset) + 12(sp); \
    sw		a1,(offset) + 16(sp); \
    sw		a2,(offset) + 20(sp); \
    sw		a3,(offset) + 24(sp); \
    sw		t0,(offset) + 28(sp); \
    sw		t1,(offset) + 32(sp); \
    sw		t2,(offset) + 36(sp); \
    sw		t3,(offset) + 40(sp); \
    sw		t4,(offset) + 44(sp); \
    sw		t5,(offset) + 48(sp); \
    sw		t6,(offset) + 52(sp); \
    sw		t7,(offset) + 56(sp); \
    sw		s0,(offset) + 60(sp); \
    sw		s1,(offset) + 64(sp); \
    sw		s2,(offset) + 68(sp); \
    sw		s3,(offset) + 72(sp); \
    sw		s4,(offset) + 76(sp); \
    sw		s5,(offset) + 80(sp); \
    sw		s6,(offset) + 84(sp); \
    sw		s7,(offset) + 88(sp); \
    sw		t8,(offset) + 92(sp); \
    sw		t9,(offset) + 96(sp); \
    sw		gp,(offset) + 100(sp); \
    sw		s8,(offset) + 104(sp); \
    sw		ra,(offset) + 108(sp)

/*
 * RESTORE_KERNEL_REGS()
 *
 *	Restore all of the kernel's relevant registers from the stack.
 */
#define RESTORE_KERNEL_REGS(offset) \
    lw		AT,(offset) + 0(sp); \
    lw		v0,(offset) + 4(sp); \
    lw		v1,(offset) + 8(sp); \
    lw		a0,(offset) + 12(sp); \
    lw		a1,(offset) + 16(sp); \
    lw		a2,(offset) + 20(sp); \
    lw		a3,(offset) + 24(sp); \
    lw		t0,(offset) + 28(sp); \
    lw		t1,(offset) + 32(sp); \
    lw		t2,(offset) + 36(sp); \
    lw		t3,(offset) + 40(sp); \
    lw		t4,(offset) + 44(sp); \
    lw		t5,(offset) + 48(sp); \
    lw		t6,(offset) + 52(sp); \
    lw		t7,(offset) + 56(sp); \
    lw		s0,(offset) + 60(sp); \
    lw		s1,(offset) + 64(sp); \
    lw		s2,(offset) + 68(sp); \
    lw		s3,(offset) + 72(sp); \
    lw		s4,(offset) + 76(sp); \
    lw		s5,(offset) + 80(sp); \
    lw		s6,(offset) + 84(sp); \
    lw		s7,(offset) + 88(sp); \
    lw		t8,(offset) + 92(sp); \
    lw		t9,(offset) + 96(sp); \
    lw		gp,(offset) + 100(sp); \
    lw		s8,(offset) + 104(sp); \
    lw		ra,(offset) + 108(sp)

#define SAVED_REG_SIZE		112
#define STAND_FRAME_SIZE	24
#define STAND_RA_OFFSET		20

/*
 * Macros to panic and printf from assembly language.
 */
#define PANIC(msg) \
	la	a0, 9f; \
	jal	panic; \
	MSG(msg)

#define	PRINTF(msg) \
	la	a0,9f; \
	jal	printf; \
	MSG(msg)

#define	MSG(msg) \
	.rdata; \
9:	.asciiz	msg; \
	.text

#endif /* _MACHASMDEFS */
