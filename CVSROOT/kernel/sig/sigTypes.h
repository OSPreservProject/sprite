/*
 * sig.h --
 *
 *     Data structures and procedure headers exported by the
 *     the signal module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

/*
 * This is because machTypes needs Sig_Context, and Sig_Context needs 
 * machTypes.
 */

#ifdef KERNEL
#include "machTypes.h"
#else
#include <kernel/machTypes.h>
#endif

#ifndef _SIGTYPES
#define _SIGTYPES

#ifdef KERNEL
#include "user/sig.h"
#include "procTypes.h"
#else
#include <sig.h>
#include <kernel/machTypes.h>
#include <kernel/procTypes.h>
#endif

/*
 * The signal context that is used to restore the state after a signal.
 */
typedef struct {
    int			oldHoldMask;	/* The signal hold mask that was in
					 * existence before this signal
					 * handler was called.  */
    Mach_SigContext	machContext;	/* The machine dependent context
					 * to restore the process from. */
} Sig_Context;

/*
 * Structure that user sees on stack when a signal is taken.
 * Sig_Context+Sig_Stack must be double word aligned for the sun4.
 * Thus there is 4 bytes of padding here.
 */
typedef struct {
    int		sigNum;		/* The number of this signal. */
    int		sigCode;    	/* The code of this signal. */
    Sig_Context	*contextPtr;	/* Pointer to structure used to restore the
				 * state before the signal. */
    int		sigAddr;	/* Address of fault. */
    int		pad;		/* Explained above. */
} Sig_Stack;

#endif /* _SIGTYPES */

