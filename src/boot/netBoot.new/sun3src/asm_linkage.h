
/*	@(#)asm_linkage.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Entry macros for assembler subroutines.
 */
#define	IDENTITY(x) x
#define	NAME(x) IDENTITY(_)x

#ifdef GPROF
#define ENTRY(x)  \
	.globl	NAME(x); \
NAME(x): \
	link a6,#0; \
	movl #1f,a0; \
	jbsr mcount; \
	unlk a6; \
	.bss; \
	.even; \
1:	.skip 4; \
	.text

#define ENTRY2(x,y)  \
	.globl	NAME(x), NAME(y); \
NAME(x): ; \
NAME(y):   \
	link a6,#0; \
	movl #1f,a0; \
	jbsr mcount; \
	unlk a6; \
	.bss; \
	.even; \
1:	.skip 4; \
	.text

/*
 * CCENTRY is for cc known subroutines like ldivt which
 * know and use a0, a1, d0 and d1.  This means we have to
 * save them.
 */
#define CCENTRY(x)  \
	.globl	x; \
x: \
	link a6,#-16; \
	moveml #0x0303,sp@; \
	movl #1f,a0; \
	jbsr mcount; \
	moveml sp@,#0x0303; \
	unlk a6; \
	.bss; \
	.even; \
1:	.skip 4; \
	.text
#else GPROF
#define ENTRY(x)  \
	.globl	NAME(x); \
NAME(x):

#define ENTRY2(x,y)  \
	.globl	NAME(x), NAME(y); \
NAME(x): \
NAME(y):

#define CCENTRY(x)  \
	.globl	x; \
x:
#endif GPROF

/*
 * Macro for autovectored interrupts.
 */
#define IOINTR(LEVEL) \
	moveml	#0xE0E0,sp@-	/* save regs we trash <d0,d1,d2,a0,a1,a2> */;\
	movl	sp,d2		/* save copy of previous sp */;\
	cmpl	#eintstack,sp	/* on interrupt stack? */;\
	jls	1f		/* yes, skip */;\
	lea	eintstack,sp	/* no, switch to interrupt stack */;\
1:	movl	#_level/**/LEVEL/**/_vector,a2	/* get vector ptr */;\
2:	movl	a2@+,a1		/* get routine address */;\
	jsr	a1@		/* go there */;\
	tstl	d0		/* success? */;\
	beqs	2b		/* no, try next one */;\
	movl	d2,sp		/* restore stack pointer */;\
	cmpl	#0x80000000,d0	/* spurious? */;\
	jeq	rei_iop		/* yes */;\
	clrl	_level/**/LEVEL/**/_spurious	/* no, clr spurious cnt */;\
	jra	rei_iop

/*
 * Macro for vectored interrupts.
 */
#define VECINTR(ifunc, cfunc, carg) \
	.globl	ifunc;\
ifunc:;\
	moveml	#0xE0C0,sp@-	/* save regs we trash <d0,d1,d2,a0,a1> */;\
	movl	sp,d2		/* save copy of previous sp */;\
	cmpl	#eintstack,sp	/* on interrupt stack? */;\
	jls	1f		/* yes, skip */;\
	lea	eintstack,sp	/* no, switch to interrupt stack */;\
1:	movl	carg,sp@-	/* push arg to interrupt routine */;\
	jsr	cfunc		/* go to it */;\
	movl	d2,sp		/* restore stack pointer */;\
        moveml  sp@+,#0x0307    /* restore saved registers <a1,a0,d2,d1,d0> */;\
	jra	rei_io
