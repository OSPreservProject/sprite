/*
 * mach.h --
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

#ifndef _MACH
#define _MACH

#ifdef KERNEL
#include "devAddrs.h"
#include "machConst.h"
#else
#include <kernel/devAddrs.h>
#include <kernel/machConst.h>
#endif

/*
 * The state of each processor: user mode or kernel mode.
 */
typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;


/*
 * Routines to enable and disable interrupts.  These leave unmaskable
 * interrupts enabled.  NOTE:  these are macros for the other suns,
 * but on the sun4 I'd have to use 2 local registers for them, so it's
 * best to make them calls to a new window.  Later, when I can run gcc
 * on all of this and tell it what registers I've used, then we can once
 * again make these macros.
 */
extern	void	Mach_DisableIntr();
extern	void	Mach_EnableIntr();

#define DISABLE_INTR() \
    if (!Mach_AtInterruptLevel()) { \
	Mach_DisableIntr(); \
	if (mach_NumDisableIntrsPtr[0] < 0) { \
	    panic("Negative interrupt count.\n"); \
	} \
	mach_NumDisableIntrsPtr[0]++; \
    }
#define ENABLE_INTR() \
    if (!Mach_AtInterruptLevel()) { \
	mach_NumDisableIntrsPtr[0]--; \
	if (mach_NumDisableIntrsPtr[0] < 0) { \
	    panic("Negative interrupt count.\n"); \
	} \
	if (mach_NumDisableIntrsPtr[0] == 0) { \
	    Mach_EnableIntr(); \
	} \
    }

/*
 * A macro to test if the current processor is at interrupt level.
 */

#define	Mach_AtInterruptLevel()	(mach_AtInterruptLevel)

/*
 * A macro to test if the current processor is in kernel mode.
 */

#define	Mach_KernelMode() (mach_KernelMode)

/*
 * Delay for N microseconds.
 */
#define	MACH_DELAY(n)	{ register int N = (n)>>1; N--; while (N > 0) {N--;} }

/*
 * The interrupt register on a sun4.
 */
#define	Mach_InterruptReg	((unsigned char *) DEV_INTERRUPT_REG_ADDR)

/*
 * Dispatch tables for kernel calls.
 */
extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();

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
 * The register state of a process.
 */
typedef struct Mach_RegState {
#ifdef NOTDEF
    /*
     * I'm handing people ptrs to this thing now, so I just have to be careful.
     */
    double	aligner;			/* Force the compiler to start
					 	 * regs on a double-word
					 	 * boundary so that std's and
					 	 * ldd's can be used. */
#endif NOTDEF
						/* Registers at time of trap:
						 * locals then ins then globals.
						 * The psr, tbr, etc, are saved
						 * in locals.  The in registers
						 * are the in registers of the
						 * window we've trapped into. */
    int		curPsr;				/* locals */
    int		pc;
    int		nextPc;
    int		tbr;
    int		y;
    int		safeTemp;
    int		volTemp1;
    int		volTemp2;
    int		ins[MACH_NUM_INS];		/* ins */
    int		globals[MACH_NUM_GLOBALS];	/* globals */
} Mach_RegState;

typedef	Mach_RegState	Mach_DebugState;


/*
 * The state for a process - saved on context switch, etc.
 */
typedef struct Mach_State {
    Mach_RegState	*trapRegs;		/* User state at trap time. */
    Mach_RegState	switchRegs;		/* Kernel state, switch time */
    int			savedRegs[MACH_NUM_WINDOW_REGS][MACH_NUM_WINDOWS];
						/* Where we save all the
						 * window's registers to if the
						 * user stack isn't resident.
						 * We could get away with 2
						 * less windows if we wanted
						 * to be tricky about recording
						 * which is the invalid window
						 * and which window we're in
						 * while saving the regs...
						 */
    int			savedMask;		/* Mask showing which windows
						 * contained info that must
						 * be restored to the stack from
						 * the above buffer since the
						 * stack wasn't resident. */
    Address		kernelStack;		/* pointer to the kernel
						 * stack for this process. */
    Address		kernStackStart;		/* beginning of kernel stack
						 * for this process. */
} Mach_State;

/*
 * The machine dependent signal structure.
  */
typedef struct Mach_SigContext {
    int	junk;	/* TBA */
} Mach_SigContext;

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

extern	Address	Mach_GetPC();

extern	Boolean	mach_KernelMode;
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel;
extern	int	*mach_NumDisableIntrsPtr;
/*
 * mach_MachineType is a string used to expand $MACHINE in pathnames.
 */
extern	char	*mach_MachineType;
/*
 * mach_ByteOrder defines a byte ordering/structure alignment type
 * used when servicing IOControls.  The input and output buffers for
 * IOControls have to be made right by the server.
 */
extern	int	mach_ByteOrder;

/*
 * Routine to initialize mach module.  Must be called first as part of boot 
 * sequence.
 */
extern void	Mach_Init();

#ifdef NOTDEF
/*
 * Routines to munge machine state struct.
 */
extern	void		Mach_InitFirstProc();
extern	ReturnStatus	Mach_SetupNewState();
extern	void		Mach_SetReturnVal();
extern	void		Mach_StartUserProc();
extern	void		Mach_ExecUserProc();
extern	void		Mach_FreeState();
extern	void		Mach_CopyState();
extern	void		Mach_GetDebugState();
extern	void		Mach_SetDebugState();
extern	Address		Mach_GetUserStackPtr();

/*
 * Migration routines.
 */
extern void			Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern int			Mach_GetEncapSize();

/*
 * Other routines.
 */
extern void			Mach_InitSyscall();
extern void			Mach_SetHandler();
extern int			Mach_GetExcStackSize();
extern Mach_ProcessorStates	Mach_ProcessorState();
extern ReturnStatus		Mach_SetJump();
extern void			Mach_UnsetJump();
extern int			Mach_GetNumProcessors();

/*
 * Machine dependent routines.
 */
extern	void	Mach_GetEtherAddress();
extern	void	Mach_ContextSwitch();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();
extern	int	Mach_GetMachineArch();
extern	Address	Mach_GetStackPointer();
extern 	void	Mach_CheckSpecialHandling();

/*
 * spriteStart is defined in bootSys.s with an underscore.
 */
extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;
#endif NOTDEF

/*
 * Machine dependent variables.
 */
extern	Address	mach_KernStart;
extern	Address	mach_CodeStart;
extern	Address	mach_StackBottom;
extern	int	mach_KernStackSize;
extern	Address	mach_KernEnd;
extern	Address	mach_FirstUserAddr;
extern	Address	mach_LastUserAddr;
extern	Address	mach_MaxUserStackAddr;
extern	int	mach_LastUserStackPage;

#endif _MACH
