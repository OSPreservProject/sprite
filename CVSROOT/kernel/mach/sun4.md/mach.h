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
#include <machTypes.h>
#include <user/setjmp.h>
#else
#include <kernel/machTypes.h>
#include <setjmp.h>
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

#define	Mach_KernelMode()	(mach_KernelMode)

/*
 * A macro to return the current interrupt nesting level.
 */

#define	Mach_IntrNesting(cpu)	(mach_NumDisableIntrsPtr[(cpu)])

/*
 * Delay for N microseconds.
 */
#ifdef sun4c
extern unsigned int		machClockRate;
#define	MACH_DELAY(n)	{ \
	register int N = (n) * machClockRate / 30 - 22; \
	while (N > 0) { N--; } \
    }
#else
#define	MACH_DELAY(n)	{ register int N = (n)<<3; N--; while (N > 0) {N--;} }
#endif

/*
 * The interrupt register on a sun4.
 */
#ifdef sun4c
extern unsigned char		*machInterruptReg;
#define	Mach_InterruptReg	machInterruptReg
#else
#define	Mach_InterruptReg	((unsigned char *) DEV_INTERRUPT_REG_ADDR)
#endif

/*
 * Suns don't have a write buffer, but this macro makes it easier to
 * write machine-independent device drivers for both the Decstations and Suns.
 */
#define Mach_EmptyWriteBuffer()

#define Mach_SetErrno(err) Proc_GetActualProc()->unixErrno = (err)

/*
 * Dispatch tables for kernel calls.
 */
extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

/*
 * Macro to get the user's stack pointer.
 */
#define Mach_UserStack() ((Address)machCurStatePtr->trapRegs->ins[6])

extern	Address	Mach_GetPC _ARGS_((void));
extern	Address	Mach_GetCallerPC _ARGS_((void));

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
extern void	Mach_Init _ARGS_((void));

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

/*
 * Routines to munge machine state struct.
 */
#ifdef KERNEL
#include <procMigrate.h>
#else
#include <kernel/procMigrate.h>
#endif

extern void Mach_InitFirstProc _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus Mach_SetupNewState _ARGS_((Proc_ControlBlock *procPtr, Mach_State *fromStatePtr, void (*startFunc)(), Address startPC, Boolean user));
extern void Mach_SetReturnVal _ARGS_((Proc_ControlBlock *procPtr, int retVal,
	int retVal2));
extern void Mach_StartUserProc _ARGS_((Proc_ControlBlock *procPtr, Address entryPoint));
extern void Mach_ExecUserProc _ARGS_((Proc_ControlBlock *procPtr, Address userStackPtr, Address entryPoint));
extern void Mach_FreeState _ARGS_((Proc_ControlBlock *procPtr));
extern void Mach_GetDebugState _ARGS_((Proc_ControlBlock *procPtr, Proc_DebugState *debugStatePtr));
extern void Mach_SetDebugState _ARGS_((Proc_ControlBlock *procPtr, Proc_DebugState *debugStatePtr));

/*
 * Migration routines.
 */
extern ReturnStatus Mach_EncapState _ARGS_((register Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr, Address buffer));
extern ReturnStatus Mach_DeencapState _ARGS_((register Proc_ControlBlock *procPtr, Proc_EncapInfo *infoPtr, Address buffer));
extern ReturnStatus Mach_GetEncapSize _ARGS_((Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr));
extern Boolean Mach_CanMigrate _ARGS_((Proc_ControlBlock *procPtr));
extern int Mach_GetLastSyscall _ARGS_((void));


extern void Mach_SetHandler _ARGS_((int vectorNumber, int (*handler)(), ClientData clientData));

extern void Mach_InitSyscall _ARGS_((int callNum, int numArgs, ReturnStatus (*normalHandler)(), ReturnStatus (*migratedHandler)()));

extern ReturnStatus	Mach_Probe _ARGS_((int byteCount, Address readAddress, Address writeAddress));

/*
 * Other routines.
 */
extern Mach_ProcessorStates Mach_ProcessorState _ARGS_((int processor));
extern int Mach_GetNumProcessors _ARGS_((void));
extern int Mach_GetBootArgs _ARGS_((int argc, int bufferSize, char **argv, char *buffer));


/*
 * Machine dependent routines.
 */
extern	Net_EtherAddress	*Mach_GetEtherAddress _ARGS_((Net_EtherAddress *etherAddress));
extern	void	Mach_ContextSwitch _ARGS_((Proc_ControlBlock *fromProcPtr, Proc_ControlBlock *toProcPtr));
extern	int	Mach_TestAndSet _ARGS_((int *intPtr));
extern	int	Mach_GetMachineType _ARGS_((void));
extern int Mach_GetMachineArch _ARGS_((void));
extern void Mach_FlushWindowsToStack _ARGS_((void));
extern	Address	Mach_GetStackPointer _ARGS_((void));
extern void Mach_CheckSpecialHandling _ARGS_((int pnum));
extern void Mach_Return2 _ARGS_((int val));
extern int Mach_SigreturnStub _ARGS_((jmp_buf *jmpBuf));


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
extern	Mach_State  *machCurStatePtr;

/*
 * New fast boot stuff.
 */
extern	int	storedDataSize;
extern	char	storedData[];
extern	char	*mach_RestartTablePtr;
extern	void	Mach_FastBoot	_ARGS_((void));

#endif /* _MACH */
