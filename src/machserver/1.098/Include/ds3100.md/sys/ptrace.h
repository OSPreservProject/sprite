/*
 * ptrace.h --
 *
 *	Declarations of ...
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.7 91/02/09 13:24:52 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _SYS_PTRACE_H
#define _SYS_PTRACE_H

#define PT_TRACE_ME	0
#define PT_READ_I	1
#define PT_READ_D	2
#define PT_READ_U	3
#define PT_WRITE_I	4
#define PT_WRITE_D	5
#define PT_WRITE_U	6
#define PT_CONTINUE	7
#define PT_KILL		8
#define PT_STEP		9

#ifdef sprite
#define PT_ATTACH	10
#define PT_DETACH	11
#endif

#define GPR_BASE	0
#define NGP_REGS	32
#define FPR_BASE	(GPR_BASE+NGP_REGS)
#define NFP_REGS	32

#define SIG_BASE	(FPR_BASE+NFP_REGS)
#define NSIG_HNDLRS	32

#define SPEC_BASE	(SIG_BASE+NSIG_HNDLRS)
#define PC		(SPEC_BASE + 0)
#define CAUSE		(SPEC_BASE + 1)
#define MMHI		(SPEC_BASE + 2)
#define MMLO		(SPEC_BASE + 3)
#define FPC_CSR		(SPEC_BASE + 4)
#define FPC_EIR		(SPEC_BASE + 5)
#define TRAPCAUSE	(SPEC_BASE + 6)
#define TRAPINFO	(SPEC_BASE + 7)
#define NSPEC_REGS	8
#define NPTRC_REGS	(SPEC_BASE + NSPEC_REGS)

#define CAUSEEXEC	1
#define CAUSEFORK	2
#define CAUSEWATCH	3
#define CAUSESINGLE	4
#define CAUSEBREAK	5
#define CAUSETRACEON	6

#endif /* _SYS_PTRACE_H */

