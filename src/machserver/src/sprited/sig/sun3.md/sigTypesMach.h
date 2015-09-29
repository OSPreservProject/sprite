/*
 * sigTypesMach.h --
 *
 *	Sun3-specific type declarations for signals.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/sig/sun3.md/RCS/sigTypesMach.h,v 1.2 92/03/12 17:52:31 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SIGTYPESMACH
#define _SIGTYPESMACH

#include <mach/sun3/thread_status.h>
#include <mach/sun3/reg.h>

/*
 * The machine dependent signal structure.
 */
typedef struct {
    struct sun_thread_state regs; /* main processor registers */
    struct fpa_regs fpRegs;	/* floating point registers */
} SigMach_Context;

#endif /* _SIGTYPESMACH */
