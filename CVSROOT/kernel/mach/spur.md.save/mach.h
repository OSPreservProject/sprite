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

#include "machConst.h"
#include "machCCRegs.h"

/*
 * The state of each processor: user mode or kernel mode.
 */
typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;

/*
 * Macros to disable and enable interrupts.
 */
#define DISABLE_INTR() \
    if (!mach_AtInterruptLevel) { \
	Mach_DisableIntr(); \
	if (mach_NumDisableIntrsPtr[0] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	mach_NumDisableIntrsPtr[0]++; \
    }
#define ENABLE_INTR() \
    if (!mach_AtInterruptLevel) { \
	mach_NumDisableIntrsPtr[0]--; \
	if (mach_NumDisableIntrsPtr[0] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	if (mach_NumDisableIntrsPtr[0] == 0) { \
	    Mach_EnableIntr(); \
	} \
    }

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

extern	Boolean	mach_KernelMode;
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel;
extern	int	*mach_NumDisableIntrsPtr;

/*
 * Delay for N microseconds.
 */
#define	MACH_DELAY(n)	{ register int N = (n)<<2; N--; while (N > 0) {N--;} }

/*
 * Dispatch tables for kernel calls.
 */
extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();

/*
 * State for each process.
 *
 * IMPORTANT NOTE: If the order or size of fields in these structures change
 *		   then the constants which give the offsets must be
 *		   changed in "machConst.h".
 */

/*
 * The register state for a process.
 */
typedef struct {
    int		regs[MACH_NUM_ACTIVE_REGS][2];	/* Registers at time of trap.*/
    int		kpsw;				/* Kernel psw. */
    int		upsw;				/* User psw. */
    int		curPC;				/* Current program counter. */
    int		nextPC;				/* Next program counter. */
    int		trapType;			/* One of MACH_USE_CUR_PC or
						 * MACH_USE_NEXT_PC. */
    int		insert;				/* The insert register. */
    int		swp;				/* The saved window pointer. */
    int		cwp;				/* Current window pointer. */
    int		usp;				/* User stack pointer. */
} Mach_RegState;

/*
 * The user state for a process.
 */
typedef struct {
    Mach_RegState	trapRegState;	/* State of process at trap. */
    Address		minSWP;		/* Min and max values for the saved */
    Address		maxSWP;		/* 	window pointer. */
    /*
     * Signal information.
     */
    Address		newCurPC;	/* Saved first PC for when calling a
					 * signal handler. */
    int			sigNum;		/* Signal number to pass to signal
					 * handler. */
    int			sigCode;	/* Signal code to pass to signal
					 * handler. */
    int			oldHoldMask;	/* The saved hold mask. */
} Mach_UserState;

/*
 * The kernel and user state for a process.
 */
typedef struct Mach_State {
    Mach_UserState	userState;		/* User state for a process. */
    Mach_RegState	switchRegState;		/* The state to save on the
						 * switch. */
    Address		kernStackStart;		/* Address of the beginning of
						 * the kernel stack. */
    Address		kernStackEnd;		/* Address of the end of the
						 * kernel stack. */
} Mach_State;

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

extern	Boolean	mach_KernelMode;
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel;
extern	int	*mach_NumDisableIntrsPtr;
/*
 * Routine to initialize mach module.  Must be called first as part of boot 
 * sequence.
 */
extern void	Mach_Init();

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

/*
 * Other routines.
 */
extern void			Mach_InitSyscall();
extern void			Mach_SetHandler();
extern int			Mach_GetExcStackSize();
extern Mach_ProcessorStates	Mach_ProcessorState();
extern void			Mach_UnsetJump();

/*
 * Machine dependent routines.
 */
extern	void	Mach_GetEtherAddress();
extern	void	Mach_ContextSwitch();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();
extern	Address	Mach_GetStackPointer();
extern	void	Mach_DisableIntr();
extern	void	Mach_EnableIntr();
extern	unsigned int Mach_GetSlotId();
extern  ReturnStatus Mach_AllocExtIntrNumber();
/*
 * spriteStart is defined in bootSys.s with an underscore.
 */
extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;

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
