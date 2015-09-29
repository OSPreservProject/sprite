/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986 Sequent Computer Systems, Inc.
 * All rights reserved
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * $Header: DEFS.h 1.3 86/05/11 $
 *
 * DEFS.h
 *	Various defines and macros for asm files.
 */
#define CALL	call
#define RETURN	ret
#define	ENTER	pushl %ebp; movl %esp,%ebp
#define	EXIT	leave

#define SPOFF	4
#define SPARG0	SPOFF+0(%esp)
#define SPARG1	SPOFF+4(%esp)
#define SPARG2	SPOFF+8(%esp)
#define SPARG3	SPOFF+12(%esp)
#define SPARG4	SPOFF+16(%esp)
#define SPARG5	SPOFF+20(%esp)
#define SPARG6	SPOFF+24(%esp)
#define SPARG7	SPOFF+28(%esp)

#define FPOFF	8
#define FPARG0	FPOFF+0(%ebp)
#define FPARG1	FPOFF+4(%ebp)
#define FPARG2	FPOFF+8(%ebp)
#define FPARG3	FPOFF+12(%ebp)
#define FPARG4	FPOFF+16(%ebp)
#define FPARG5	FPOFF+20(%ebp)
#define FPARG6	FPOFF+24(%ebp)
#define FPARG7	FPOFF+28(%ebp)

#ifdef PROF
#define	ENTRY(x)	.text ; .globl _/**/x; .align 2; _/**/x: ; \
			.data; Lcnt:; .long 0; .text; pushl %ebp; \
			movl %esp,%ebp; leal Lcnt,%eax; CALL mcount; leave
#else
#define	ENTRY(x)	.text; .globl _/**/x; .align 2; _/**/x:
#endif
