/*
 * machTypes.h --
 *
 *     Exported structures for the mach module.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHTYPES
#define _MACHTYPES

#ifdef KERNEL
#include "sprite.h"
#include "devAddrs.h"
#include "machConst.h"
#include "user/fmt.h"
#else
#include <kernel/devAddrs.h>
#include <kernel/machConst.h>
#include <fmt.h>
#endif

/*
 * The state of each processor: user mode or kernel mode.
 */
typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;

/*
 * State for each process.
 *
 * IMPORTANT NOTE: 1) If the order or size of fields in these structures change
 *		   then the constants which give the offsets must be
 *		   changed in "machConst.h".
 *
 *		   2) Mach_DebugState and Mach_RegState are the same thing.
 *		   If what the debugger needs changes, they may no longer be
 *		   the same thing.  Mach_RegState is used as a template for
 *		   saving registers to the stack for nesting interrupts, traps,
 *		   etc.  Therefore, the first sets of registers, locals and ins,
 *		   are in the same order as they are saved off of the sp for
 *		   a regular save window operation.  If this changes, changes
 *		   must be made in machAsmDefs.h and machConst.h.  Note that
 *		   this means the pointer to Mach_RegState for trapRegs in the
 *		   Mach_State structure is actually a pointer to registers saved
 *		   on the stack.
 *
 *		   3) Mach_State defines the use of local registers.   Should
 *		   more local registers be necessary, then some of the special
 *		   registers (tbr, etc) will need to be saved after the globals.
 *
 *		   4) Note that we must be careful about the alignment of
 *		   this structure, since it's saved and restored with load
 *		   and store double operations.  Without an aligner, this is
 *		   hard.  I'm not sure what to do about that.  Usually, this
 *		   just be space on the stack, so it will be double-word
 *		   aligned anyway.
 */

/*
 * The register state of a process: locals, then ins, then globals.
 * The psr, tbr, etc, are saved in locals.  The in registers are the in
 * registers of the window we've trapped into.  The calleeInputs is the
 * area that we must save for the C routine we call to save its 6 input
 * register arguments into if its compiled for debuggin.  The extraParams
 * area is the place that parameters beyond the sixth go, since only 6 can
 * be passed via input registers.  We limit this area to the number of extra
 * arguments in a system call, since only sys-call entries to the kernel
 * have this many args!  How do we keep it this way?  This is MESSY, since
 * actually one of the calleeInputs is for a "hidden parameter" for an agregate
 * return value, and one of them is really the beginning of the extra
 * params, but I'll fix this up later.
 */
typedef struct Mach_RegState {
    unsigned int	curPsr;				/* locals */
    unsigned int	pc;
    unsigned int	nextPc;
    unsigned int	tbr;
    unsigned int	y;
    unsigned int	safeTemp;
    unsigned int	volTemp1;
    unsigned int	volTemp2;
    unsigned int	ins[MACH_NUM_INS];		/* ins */
						/* callee saves inputs here */
    unsigned int	calleeInputs[MACH_NUM_INS];
							/* args beyond 6 */
    unsigned int	extraParams[MACH_NUM_EXTRA_ARGS];
    unsigned int	globals[MACH_NUM_GLOBALS];	/* globals */
    unsigned int 	fsr;  /* FPU state register. Bit definition 
			       * in machConst. */
    int   numQueueEntries;    /* Number of floating point queue entries
			       * active. */
    unsigned int	fregs[MACH_NUM_FPS];	/* Floating point registers.
						 * This can be treated as
						 * MACH_NUM_FPS floats or
						 * MACH_NUM_FPS/2 doubles. */
    struct {
	char          *address;	  /* Address of FP instruction. */
	unsigned int instruction; /* FP instruction value. */
    }  fqueue[MACH_FPU_MAX_QUEUE_DEPTH];  /* Queue of unfinished floating 
					   * point instructions. */
} Mach_RegState;

/*
 * Temporary hack so we can add fpu stuff without recompiling debuggers.
 */
#ifdef NOTDEF
typedef	Mach_RegState	Mach_DebugState;
#else
typedef	struct	Mach_DebugState {
    unsigned int	curPsr;				/* locals */
    unsigned int	pc;
    unsigned int	nextPc;
    unsigned int	tbr;
    unsigned int	y;
    unsigned int	safeTemp;
    unsigned int	volTemp1;
    unsigned int	volTemp2;
    unsigned int	ins[MACH_NUM_INS];		/* ins */
						/* callee saves inputs here */
    unsigned int	calleeInputs[MACH_NUM_INS];
							/* args beyond 6 */
    unsigned int	extraParams[MACH_NUM_EXTRA_ARGS];
    unsigned int	globals[MACH_NUM_GLOBALS];	/* globals */
} Mach_DebugState;
#endif /* NOTDEF */

/*
 * The machine-dependent signal structure.
 */
typedef struct Mach_SigContext {
    unsigned	int	trapInst;	/* the sig ret trap instruction */
    Address	pcValue;		/* pc to go to for signal */
    Mach_RegState	userState;	/* user trap state before signal */
} Mach_SigContext;

/*
 * Mary will fix this!  Ask her about it.
 */
#ifdef KERNEL
#include "sprite.h"
#include "devAddrs.h"
#include "machConst.h"
#include "user/fmt.h"
#include "sigTypes.h"
#else
#include <kernel/devAddrs.h>
#include <kernel/machConst.h>
#include <fmt.h>
#include <kernel/sigTypes.h>
#endif

typedef struct Mach_RegWindow {
     int locals[MACH_NUM_LOCALS];
     int ins[MACH_NUM_INS];
} Mach_RegWindow;

/*
 * The state for a process - saved on context switch, etc.
 */
typedef struct Mach_State {
    Mach_RegState	*trapRegs;		/* User state at trap time. */
    Mach_RegState	*switchRegs;		/* Kernel state, switch time */
    int			savedRegs[MACH_NUM_WINDOWS][MACH_NUM_WINDOW_REGS];
						/* Where we save all the
						 * window's registers to if the
						 * user stack isn't resident.
						 * We could get away with 2
						 * less windows if we wanted
						 * to be tricky about recording
						 * which is the invalid window
						 * and which window we're in
						 * while saving the regs...  */
    int			savedMask;		/* Mask showing which windows
						 * contained info that must
						 * be restored to the stack from
						 * the above buffer since the
						 * stack wasn't resident. */
    int			savedSps[MACH_NUM_WINDOWS];
						/* sp for each saved window
						 * stored here to make it easy
						 * to copy the stuff back out
						 * to the user stack in the
						 * correct place.  */
    Address		kernStackStart;		/* top of kernel stack
						 * for this process. */
    int			fpuStatus;		/* FPU status. See below. */
    Sig_Stack		sigStack;		/* sig stack holder for setting
						 * up signal handling */
    Sig_Context		sigContext;		/* sig context holder for
						 * setting up signal handling */
    int			lastSysCall;		/* Needed for migration. */
} Mach_State;

/*
 * Values for the fpuStatus field.
 * MACH_FPU_ACTIVE - FPU is active for this process.
 * MACH_FPU_EXCEPTION_PENDING - The process caused a FPU exception to occur.
 */
#define	MACH_FPU_ACTIVE			0x1
#define	MACH_FPU_EXCEPTION_PENDING      0x2


/*
 * Structure on top of user stack when Sig_Handler is called.  This must
 * a multiple of double-words!!!!
 */
typedef	struct {
    Mach_RegState	extraSpace;	/* saved-window, etc, space */
    Sig_Stack		sigStack;	/* signal stack for Sig_Handler */
    Sig_Context		sigContext;	/* the signal context */
} MachSignalStack;

#endif /* _MACHTYPES */
