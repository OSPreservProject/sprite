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


#ifdef lint
#define	Mach_EnableIntr()
#define	Mach_DisableIntr()
#else
/*
 * Routines to enable and disable interrupts.  These leave unmaskable
 * interrupts enabled.  These are assembly macros to be called from C code.
 * They use the in-line capabilities of GCC.
 */
#define	Mach_EnableIntr()	({\
	register unsigned int	tmpPsr;	\
	asm volatile ( "mov	%%psr, %1;	\
			andn	%1, 0xf00, %1;	\
			mov	%1, %%psr; nop; nop; nop\n":	\
			"=r"(tmpPsr):"r"(tmpPsr));	\
	})

#define	Mach_DisableIntr()	({\
	register unsigned int tmpPsr;	\
	asm volatile ( "mov	%%psr, %1;	\
			or	%1, 0xf00, %1;	\
			mov	%1, %%psr; nop; nop; nop\n":	\
			"=r"(tmpPsr):"r"(tmpPsr));	\
	})
#endif /* lint */


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
 * A macro to return the current interrupt nesting level.
 */

#define	Mach_IntrNesting(cpu)	(mach_NumDisableIntrsPtr[(cpu)])

/*
 * Delay for N microseconds.
 */
#define	MACH_DELAY(n)	{ register int N = (n)<<3; N--; while (N > 0) {N--;} }

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
 * Do to a circularity in include files now, the definitions of
 * Mach_SigContext and Mach_RegState had to be moved to a new header file:
 * machSig.h.  This is because sig.h includes mach.h because it needs
 * the definitions of Mach_SigContext (and thus Mach_RegState) in order
 * to define the Sig_Context structure's machContext field.  But if I
 * included all of mach.h, I'd get the Sig_Stack stuff in Mach_State as
 * undefined since sig.h wouldn't yet have defined that.  Ugh.
 *
 *	So - SEE machSig.h for definitions of Mach_RegState and Mach_SigContext.
 *
 */
#ifdef KERNEL
#include "machSig.h"
#else
#include <kernel/machSig.h>
#endif

typedef struct Mach_RegWindow {
     int locals[MACH_NUM_LOCALS];
     int ins[MACH_NUM_INS];
} Mach_RegWindow;

/*
 * Ugh, I have to include this here, since it needs Mach_RegState, and
 * Mach_SigContext, defined above.   But I need the include file before
 * the Sig_Stack stuff below.
 */
#ifdef KERNEL
#include "sig.h"
#else
#include <kernel/sig.h>
#endif /* KERNEL */

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
 * mach_Format defines a byte ordering/structure alignment type
 * used when servicing IOControls.  The input and output buffers for
 * IOControls have to be made right by the server.
 */
extern	Fmt_Format	mach_Format;

/*
 * Routine to initialize mach module.  Must be called first as part of boot
 * sequence.
 */
extern void	Mach_Init();

/*
 * Macro to put some primitive debugging values into a circular buffer.
 * After each value, it stamps a special mark, which gets overwritten by the
 * next value, so we always know where the end of the list is.
 */
extern	int	debugCounter;
extern	int	debugSpace[];
#define	MACH_DEBUG_ADD(thing)	\
    debugSpace[debugCounter++] = (int)(thing);	\
    if (debugCounter >= 500) {	\
	debugCounter = 0;	\
    }				\
    debugSpace[debugCounter] = (int)(0x11100111);

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
#endif /* NOTDEF */

/*
 * Migration routines.
 */
extern ReturnStatus		Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern ReturnStatus		Mach_GetEncapSize();
extern Boolean			Mach_CanMigrate();

extern void			Mach_SetHandler();
extern void			Mach_InitSyscall();
extern ReturnStatus		Mach_Probe();

/*
 * Other routines.
 */
extern Mach_ProcessorStates	Mach_ProcessorState();
extern int			Mach_GetNumProcessors();

/*
 * Machine dependent routines.
 */
extern	void	Mach_GetEtherAddress();
extern	void	Mach_ContextSwitch();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();
extern	int	Mach_GetMachineArch();
extern	void	MachFlushWindowsToStack();

/*
 * spriteStart is defined in bootSys.s with an underscore.
 */
extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;

#ifdef NOTDEF
/*
 * These routine are not needed so far in the sun4 port.
 */
extern	Address	Mach_GetStackPointer();
extern 	void	Mach_CheckSpecialHandling();
#endif /* NOTDEF */

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

#endif /* _MACH */
