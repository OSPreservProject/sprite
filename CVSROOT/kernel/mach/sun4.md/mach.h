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
#include "machTypes.h"
#else
#include <kernel/machTypes.h>
#endif

#ifdef lint
#define	Mach_EnableIntr()
#define	Mach_DisableIntr()
#else
/*
 * Routines to enable and disable interrupts.  These leave unmaskable
 * interrupts enabled.  These are assembly macros to be called from C code.
 * They use the in-line capabilities of GCC.
 */
#define	Mach_EnableIntr()	{\
	register unsigned int	tmpPsr;	\
	asm volatile ( "mov	%%psr, %0;	\
			andn	%0, 0xf00, %0;	\
			mov	%0, %%psr; nop; nop; nop\n":	\
			"=r"(tmpPsr));	\
	}

#define	Mach_DisableIntr()	{\
	register unsigned int tmpPsr;	\
	asm volatile ( "mov	%%psr, %0;	\
			or	%0, 0xf00, %0;	\
			mov	%0, %%psr; nop; nop; nop\n":	\
			"=r"(tmpPsr));	\
	}
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
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

extern	Address	Mach_GetPC();
extern	Address	Mach_GetCallerPC();

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
