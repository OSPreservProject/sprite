/*
 * sigTypesMach.h --
 *
 *	MIPS-specific type declarations for signals.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/sig/ds3100.md/RCS/sigTypesMach.h,v 1.2 92/03/12 17:49:50 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SIGTYPESMACH
#define _SIGTYPESMACH

#include <mach/mips/thread_status.h>

/*
 * The machine dependent information for restoring state after a signal.
 * XXX Note that for Ultrix the UNIX sigcontext must match the setjmp 
 * jmp_buf, because the Ultrix longjmp does a sigreturn.  If we ever want 
 * to be binary compatible with Ultrix, we might want to make Sig_Context 
 * and SigMach_Context match, too.
 */
typedef struct {
    struct mips_thread_state regs; /* main registers */
    struct mips_coproc_state fpRegs; /* floating-point registers */
} SigMach_Context;

#endif /* _SIGTYPESMACH */
