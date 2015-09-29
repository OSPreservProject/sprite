/*
 * Copyright (c) 1991, Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)machSignal.h	7.3 (Berkeley) 5/14/88
 * $Header: /sprite/src/lib/include/ds3100.md/sys/RCS/machSignal.h,v 1.2 91/09/24 21:30:51 shirriff Exp $
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
/*
 * WARNING: THE sigcontext MUST BE KEPT CONSISTENT WITH /usr/include/setjmp.h
 * AND THE LIBC ROUTINES setjmp() AND longjmp()
 */
/*
 * This declaration comes from:
 * /sprite/src/kernel/mach/ds3100.md/ultrixSignal.h.
 */
struct	sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	int	sc_pc;			/* pc at time of signal */
	/*
	 * General purpose registers
	 */
	int	sc_regs[32];	/* processor regs 0 to 31 */
	int	sc_mdlo;	/* mul/div low */
	int	sc_mdhi;	/* mul/div high */
	/*
	 * Floating point coprocessor state
	 */
	int	sc_ownedfp;	/* fp has been used */
	int	sc_fpregs[32];	/* fp regs 0 to 31 */
	int	sc_fpc_csr;	/* floating point control and status reg */
	int	sc_fpc_eir;	/* floating point exception instruction reg */
	/*
	 * END OF REGION THAT MUST AGREE WITH setjmp.h
	 * END OF jmp_buf REGION
	 */
	/*
	 * System coprocessor registers at time of signal
	 */
	int	sc_cause;	/* cp0 cause register */
	int	sc_badvaddr;	/* cp0 bad virtual address */
	int	sc_badpaddr;	/* cpu bd bad physical address */
};
#endif /* _SYS_SIGNAL */
