/*
 * mach.h --
 *
 *     	Exported structures for the mach module.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACH
#define _MACH

#ifdef KERNEL
#include "machConst.h"
#else
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
 * Macros to disable and enable interrupts.
 */
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
 * Macro to get level of nesting of disabled interrupts.
 */
#define Mach_IntrNesting(cpu) (mach_NumDisableIntrsPtr[(cpu)])

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
#define	MACH_DELAY(n)	{ register int N = (n)*6; while (N > 0) {N--;} }

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
 * The register state of a user process which is passed to debuggers.
 */
typedef struct {
    Address		pc;			/* The program counter. */
    unsigned		regs[MACH_NUM_GPRS];	/* General purpose registers.*/
    unsigned		fpRegs[MACH_NUM_FPRS];	/* The floating point
						 * registers. */
    unsigned		fpStatusReg;		/* The floating point status
						 * register. */
    unsigned		mflo, mfhi;		/* Multiply lo and hi 
						 * registers. */
} Mach_RegState;

/*
 * The user state for a process.
 */
typedef struct {
    Mach_RegState	regState;		/* State of a process after
						 * a trap. */
    int			unixRetVal;		/* Return value from a 
						 * UNIX system call. */
} Mach_UserState;

/*
 * The kernel and user state for a process.
 */
typedef struct Mach_State {
    Mach_UserState	userState;		/* User state for a process. */
    Mach_RegState	switchRegState;		/* The state to save on a
						 * context switch. */
    Address		kernStackStart;		/* Address of the beginning of
						 * the kernel stack. */
    Address		kernStackEnd;		/* Address of the end of the
						 * the kernel stack. */
    unsigned		sstepInst;		/* The instruction that we
						 * replaced to do a single
						 * step. */
    unsigned		tlbHighEntry;		/* The TLB high entry value
						 * for the first stack page. */
    unsigned		tlbLowEntries[2];	/* The two TLB low entry values
						 * for the two stack pages. */
} Mach_State;

/*
 * The machine dependent signal structure.
 */
typedef struct {
    int		  	break1Inst;	/* The instruction that is
					 * executed upon return. */
    Mach_UserState	userState;	/* The user process machine state
					 * info. */
    unsigned		fpRegs[MACH_NUM_FPRS];	/* The floating point
						 * registers. */
    unsigned		fpStatusReg;		/* The floating point status
						 * register. */
} Mach_SigContext;

/*
 * The structure used by the debugger to hold the machine state.
 */
typedef struct {
    int		regs[MACH_NUM_GPRS];
    int		fpRegs[MACH_NUM_FPRS];
    unsigned	sig[32];
    unsigned	excPC;
    unsigned	causeReg;
    unsigned	multHi;
    unsigned	multLo;
    unsigned	fpCSR;
    unsigned	fpEIR;
    unsigned	trapCause;
    unsigned	trapInfo;
    unsigned	tlbIndex;
    unsigned	tlbRandom;
    unsigned	tlbLow;
    unsigned	tlbContext;
    unsigned	badVaddr;
    unsigned	tlbHi;
    unsigned	statusReg;
} Mach_DebugState;

/*
 * Macro to get processor number
 */
#define	Mach_GetProcessorNumber() 	0

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
extern ReturnStatus		Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern ReturnStatus		Mach_GetEncapSize();
extern Boolean			Mach_CanMigrate();

/*
 * Other routines.
 */
extern void			Mach_InitSyscall();
extern Mach_ProcessorStates	Mach_ProcessorState();
extern int			Mach_GetNumProcessors();
extern Address			Mach_GetPC();
/*
 * Machine dependent routines.
 */
extern	void		Mach_GetEtherAddress();
extern	void		Mach_ContextSwitch();
extern	int		Mach_TestAndSet();
extern	int		Mach_GetMachineType();
extern	int		Mach_GetMachineArch();
extern	Address		Mach_GetStackPointer();
extern 	void		Mach_CheckSpecialHandling();
extern 	int		Mach_GetBootArgs();	
extern  ReturnStatus	Mach_ProbeAddr();
extern	void		Mach_FlushCode();

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
