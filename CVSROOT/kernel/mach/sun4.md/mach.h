/*
 * mach.h --
 *
 *     Exported structures for the mach module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
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

#ifdef NOTDEF
/*
 * Dispatch tables for kernel calls.
 */
extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();
#endif NOTDEF

/*
 * State for each process.
 *
 * IMPORTANT NOTE: If the order or size of fields in these structures change
 *		   then the constants which give the offsets must be
 *		   changed in "machConst.h".
 */

/*
 * The register state of a process.
 */
typedef struct {
    double	aligner;			/* Force the compiler to start
					 	 * regs on a double-word
					 	 * boundary so that std's and
					 	 * ldd's can be used. */
    int		regs[MACH_NUM_ACTIVE_REGS];	/* Registers at time of trap,
						 * includes globals, ins, locals
						 * and outs. */
    int		psr;				/* processor state reg */
    int		y;				/* Multiply register */
    int		tbr;				/* trap base register - records
						 * type of trap, etc. */
    int		wim;				/* window invalid mask at time
						 * of trap - for the debugger's
						 * sake only. */
/*
 * These shouldn't be necessary since they're stored in the local registers
 * on a trap???
 */
#ifdef NOTDEF
    Address	curPC;				/* program counter */
    Address	nextPC;				/* next program counter */
#endif NOTDEF
} Mach_RegState;

/*
 * The user state for a process.
 */
typedef struct {
    Mach_RegState	trapRegs;		/* State of process at trap. */
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
    int			savedRegs[MACH_NUM_REGS_PER_WINDOW][MACH_NUM_WINDOWS];
    int			savedMask;		/* Mask showing which windows
						 * contained info that must
						 * be restored to the stack from
						 * the above buffer since the
						 * stack wasn't resident. */
} Mach_UserState;

/*
 * The kernel and user state for a process.
 */
typedef struct Mach_State {
    Mach_UserState	userState;		/* User state for a process. */
    int			switchRegs[MACH_NUM_ACTIVE_REGS + MACH_NUM_GLOBAL_REGS];
						/* Where registers are saved
						 * and restored to/from during
						 * context switches. */
    /*
     * Should this be start and end and current kernel sp?
     */
    Address		kernStackStart;		/* Address of the beginning of
						 * the kernel stack. */
} Mach_State;

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

/*
 * jmpl writes current pc into %o0 here, which is where values are returned
 * from calls.  The return from __MachGetPc must not overwrite this.
 * Since this uses a non-pc-relative jmp, we CANNOT use this routine while
 * executing before we've copied the kernel to where it was linked for.
 */
#ifdef lint
#define Mach_GetPC() 	0
#else
#define	Mach_GetPC()			\
    asm("jmpl __MachGetPc, %o0");	\
    asm("nop");
#endif /* lint */

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
 *
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
