/*
 * $Copyright:	$
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
 * $Header: Setjmp.h 1.2 86/05/11 $
 *
 * Common definitions for setjmp and longjmp
 */
#define MAGIC	0xfeedface		/* like some deadbeef maybe? */
#define SIGALL	0xffffffff		/* mask to block all signals */
#define STDERR	2			/* fd to write errors */

/*
 * C version of setjmp buffer
 *
 * struct sj_buffer {
 *	long	regEDI, regESI, regEBX;
 * 	long	fp, sp;
 * 	long	pc;
 * 	long	signal_mask;
 * 	long	onstack;
 * 	long	magic;
 * }
 */

/*
 * Setjmp buffer
 */
#define	REGEDI	0*4	/* %edx register */
#define	REGESI	1*4	/* %esi register */
#define	REGEBX	2*4	/* %ebx register */
#define	FPTR	3*4	/* frame pointer */
#define	SPTR	4*4	/* stack pointer */
#define	PCTR	5*4	/* pc */
#define	SMSK	6*4	/* signal mask */
#define	SSTK	7*4	/* signal stack boolean */
#define	MGIC	8*4	/* MAGIC value */
