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
#include "machConst.h"
#include "machCCRegs.h"
#else
#include <kernel/machConst.h>
#include <kernel/machCCRegs.h>
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
#define DISABLE_INTR() {\
    register int pnum = Mach_GetProcessorNumber();\
    if (!mach_AtInterruptLevel[pnum]) { \
	Mach_DisableIntr(); \
	if (mach_NumDisableIntrsPtr[pnum] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	mach_NumDisableIntrsPtr[pnum]++; \
    } \
    }
#define ENABLE_INTR() {\
    register int pnum = Mach_GetProcessorNumber();\
    if (!mach_AtInterruptLevel[pnum]) { \
	mach_NumDisableIntrsPtr[pnum]--; \
	if (mach_NumDisableIntrsPtr[pnum] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	if (mach_NumDisableIntrsPtr[pnum] == 0) { \
	    Mach_EnableIntr(); \
	} \
    } \
    }

/*
 * Delay for N microseconds.
 */
#define	MACH_DELAY(n)	{ register int N = (n)>>1; N--; while (N > 0) {N--;} }

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
    double	aligner;			/* Force the compiler to start
						 * regs on a double word boundry
						 * so that st_64's can be used.
						 */
    int		regs[MACH_NUM_ACTIVE_REGS][2];	/* Registers at time of trap.*/
    int		kpsw;				/* Kernel psw. */
    int		upsw;				/* User psw. */
    Address	curPC;				/* Current program counter. */
    Address	nextPC;				/* Next program counter. */
    int		insert;				/* The insert register. */
    Address	swp;				/* The saved window pointer. */
    int		cwp;				/* Current window pointer. */
} Mach_RegState;

/*
 * The user state for a process.
 */
typedef struct {
    Mach_RegState	trapRegState;	/* State of process at trap. */
    Address		specPageAddr;	/* The base address of the special
					 * page that contains user trap
					 * handler and context switch state.
					 * This is always "swpBase - 
					 * VMMACH_PAGE_SIZE". */
    Address		swpBaseAddr;	/* Where the saved window stack
					 * begins. */
    Address		swpMaxAddr;	/* Where the saved window stack
					 * ends. */
    Address		minSWP;		/* Current min and max values for the*/
    Address		maxSWP;		/*    saved window pointer.   These  */
					/*    correspond to the pages that   */
					/*    are wired down in memory.      */
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
    Address		faultAddr;	/* The fault address if a signal
					 * is to be sent because of a fault. */
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
 * Machine dependent signal context.
 */
typedef struct Mach_SigContext {
    Address		faultAddr;	/* The fault address if the signal
					 * was sent because of a fault. */
    Mach_RegState	regState;	/* Register state at the time of the
					 * trap that caused the signal. */
} Mach_SigContext;

/*
 * The trap handler information.
 */
typedef struct Mach_TrapHandler {
    void	(*handler)();		/* The trap handler to call. */
    int		handlerType;		/* The type of trap handler.  One of
					 * MACH_PLAIN_INTERFACE, 
					 * MACH_INT_OPERAND_INTERFACE or
					 * MACH_FLOAT_OPERAND_INTERFACE. */
} Mach_TrapHandler;

/*
 * The state that is saved for users.
 */
typedef struct Mach_SavedState {
    Mach_RegState	regState;
    Address		swpBaseAddr;
    Address		swpMaxAddr;
} Mach_SavedState;

/*
 * The format of the special user page.
 */
typedef struct Mach_SpecPage {
    Mach_SavedState	switchState;	/* Where the registers are saved on
					 * on a user context switch trap. */
    Mach_SavedState	savedState;	/* Where the state is saved/restored
					 * to/from on user save and restore
					 * traps. */
    Mach_TrapHandler	trapTable[MACH_NUM_USER_CMP_TRAPS + 
				  MACH_NUM_OTHER_USER_TRAPS];
} Mach_SpecPage;

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetSlotId --
 *
 * Return the NuBus slot id of the processor. This is coded as a macro for
 * speed.
 *
 * ----------------------------------------------------------------------------
 */
#define	Mach_GetSlotId() ({\
	register unsigned int	__slot_id; \
	asm volatile ("ld_external %1,r0,$0xf08\n\tNop\n ":" =r" (__slot_id) \
				: "r" (__slot_id)); \
	(__slot_id & 0xff); })

/*
 * Macro to get processor number. The processor number is stored in the top
 * eight bits of the kpsw.
 * For testing uniprocessor, always return zero.
 */
#define	Mach_GetProcessorNumber() ({ \
	register unsigned int	__pnum; \
	asm volatile ("rd_kpsw	 %1\n\textract %1,%1,$3\n ":" =r" (__pnum) \
				: "r" (__pnum)); \
	(__pnum = 0); })

/*
 * A macro to test if the current processor is at interrupt level.
 */

#define	Mach_AtInterruptLevel()	\
			(mach_AtInterruptLevel[Mach_GetProcessorNumber()])

/*
 * A macro to test if the current processor is in kernel mode.
 */

#define	Mach_KernelMode() (mach_KernelMode[Mach_GetProcessorNumber()])

extern	Boolean	mach_KernelMode[];
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel[];
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
extern void			Mach_UnsetJump();

/*
 * Machine dependent routines.
 */
extern	void	Mach_GetEtherAddress();
extern	void	Mach_ContextSwitch();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();
extern	int	Mach_GetMachineArch();
extern	Address	Mach_GetStackPointer();
extern	void	Mach_DisableIntr();
extern	void	Mach_EnableIntr();
extern  ReturnStatus Mach_AllocExtIntrNumber();
extern	void	Mach_RefreshStart();
extern	void	Mach_RefreshInterrupt();
extern	void	Mach_SetNonmaskableIntr();

/*
 * Routines to read and write physical memory.
 */
extern unsigned int    Mach_ReadPhysicalWord();
extern void	       Mach_WritePhysicalWord();

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

/*
 * mach_CycleTime - The cycle time of the machine (and hence the T{0,1,2}
 * counters in the Cache Controller) in cycles per second.  This number
 * is intended for use by the devTimer module.
 */

extern 	unsigned int	mach_CycleTime;

/*
 * The address of the UART.
 */
extern	Address	mach_UARTAddr;

#endif _MACH
