/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)signal.h	7.3 (Berkeley) 5/14/88
 * $Header: /sprite/src/lib/include/RCS/signal.h,v 1.10 90/12/18 18:39:43 kupfer Exp $
 */

#ifndef _SYS_SIGNAL
#define _SYS_SIGNAL

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  On some systems it is also made
 * available to the handler to allow it to properly restore state if a
 * non-standard exit is performed.  However, user programs should not
 * rely on having access to this information.
 */
#define SPARC_MAXREGWINDOW 31		/* max # of register windows */
struct	sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	int	sc_sp;			/* sp to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_npc;			/* npc to restore */
	int	sc_psr;			/* psr to restore */
	int	sc_g1;			/* g1 to restore */
	int	sc_o0;			/* o0 to restore */
	int	sc_wbcnt;		/* outstanding windows */
	char	*sc_spbuf[SPARC_MAXREGWINDOW]; /* sp's in each window */
	int	sc_wbuf[SPARC_MAXREGWINDOW][16]; /* register windows */
};

#endif /* _SYS_SIGNAL */
