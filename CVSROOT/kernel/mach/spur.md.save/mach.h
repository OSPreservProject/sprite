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
#include "timer.h"
#include "user/fmt.h"
#else
#include <kernel/machConst.h>
#include <kernel/machCCRegs.h>
#include <kernel/timer.h>
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
 * Macros to disable and enable interrupts.
 */
#define DISABLE_INTR() {\
    register int pnum = Mach_GetProcessorNumber();\
    if (!mach_AtInterruptLevel[pnum]) { \
	Mach_DisableIntr(); \
	if (mach_NumDisableIntrsPtr[pnum] < 0) { \
	    panic("Negative interrupt count.\n"); \
	} \
	mach_NumDisableIntrsPtr[pnum]++; \
    } \
    }
#define ENABLE_INTR() {\
    register int pnum = Mach_GetProcessorNumber();\
    if (!mach_AtInterruptLevel[pnum]) { \
	mach_NumDisableIntrsPtr[pnum]--; \
	if (mach_NumDisableIntrsPtr[pnum] < 0) { \
	    panic("Negative interrupt count.\n"); \
	} \
	if (mach_NumDisableIntrsPtr[pnum] == 0) { \
	    Mach_EnableIntr(); \
	} \
    } \
    }

/*
 * Macro for enabling timer interrupts. 
 * Use this only during bootstrap before turning on all interrupts for the
 * first time. Also be sure to disable interrupts after enabling timer
 * interrupts (don't do to enables in a row).
 */

#define ENABLE_TIMER_INTR() {\
    register int pnum = Mach_GetProcessorNumber();\
    if (!mach_AtInterruptLevel[pnum]) { \
	mach_NumDisableIntrsPtr[pnum]--; \
	if (mach_NumDisableIntrsPtr[pnum] < 0) { \
	    panic("Negative interrupt count.\n"); \
	} \
	if (mach_NumDisableIntrsPtr[pnum] == 0) { \
	    Mach_EnableTimerIntr(); \
	} else { \
	    printf("Can't enable timer interrupts, too many disables.\n");\
	} \
    } \
}

/*
 * Macro to get level of nesting of disabled interrupts.
 */
#define Mach_IntrNesting(cpu) (mach_NumDisableIntrsPtr[(cpu)])

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

#define MACH_REGSTATE_CNTS "{dw71}"

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
    Address		specPageAddr;
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
 * A type which sizeof() == cache block size of machine. 
 */

typedef char	Mach_CacheBlockSizeType[VMMACH_CACHE_LINE_SIZE];

 /*
 * Per processor status info.
 */
typedef	int	Mach_ProcessorStatus;

#define	MACH_UNKNOWN_STATUS		0	/* Status unknown. */
#define	MACH_UNINITIALIZED_STATUS	1	/* Processor uninitialized. */
#define	MACH_ACTIVE_STATUS		2	/* Processor running. */
#define	MACH_IN_DEBUGGER_STATUS		3	/* Processor in debugger. */
#define	MACH_CONTINUING_STATUS		4	/* Processor continuing from
						 * debugger. */
#define	MACH_HUNG_STATUS		5	/* Processor hung. */
#define	MACH_DEAD_STATUS		6	/* Processor broken. */

/*
 * Status of each processor.
 */
extern Mach_ProcessorStatus mach_ProcessorStatus[];

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
#ifdef lint
#define	Mach_GetSlotId() 0

#else 

#define	Mach_GetSlotId() ({\
	register unsigned int	__slot_id; \
	asm volatile ("ld_external %1,r0,$0xf08\n\tNop\n ":" =r" (__slot_id) \
				: "r" (__slot_id)); \
	(__slot_id & 0xff); })

#endif /* lint */
/*
 * Macro to get processor number. The processor number is stored in the top
 * eight bits of the kpsw.
 * For testing uniprocessor, always return zero.
 */

#ifdef lint
#define	Mach_GetProcessorNumber()  0 

#else

#define	Mach_GetProcessorNumber() ({ \
	register unsigned int	__pnum; \
	asm volatile ("rd_kpsw	 %1\n\textract %1,%1,$3\n ":" =r" (__pnum) \
				: "r" (__pnum)); \
	(__pnum); })

#endif /* lint */

/*
 * A macro to test if the current processor is at interrupt level.
 */

#define	Mach_AtInterruptLevel()	\
			(mach_AtInterruptLevel[Mach_GetProcessorNumber()])

/*
 * A macro to test if the current processor is in kernel mode.
 */

#define	Mach_KernelMode() (mach_KernelMode[Mach_GetProcessorNumber()])

/*
 * A macro to return the current PC. 
 */
#ifdef lint
#define Mach_GetPC() 	(Address)0
#else
#define Mach_GetPC() \
    ({\
	register Address __pc; \
	asm volatile ("rd_special %1,pc\n":"=r" (__pc):"r"(__pc));\
	(__pc);\
    })
#endif

/*
 * Mach_Time is a union used to represent time in both the kernel and
 * in user programs.  User programs should use the time field and
 * the kernel should use the ticks.  It is converted when passed to
 * user programs.
 */

typedef union {
    Timer_Ticks	ticks;  /* kernel */
    Time	time;   /* user */
} Mach_Time;

/*
 * Structure for storing instruction count information.
 */

typedef struct {
    Boolean		on;	 	/* Is counting on? */
    unsigned int	start; 	  	/* starting count */
    unsigned int	end;	  	/* ending count */
    unsigned int	total;    	/* total of all runs */
    int			runs;  	  	/* number of runs */
    Address		startPC;   	/* pc where current run began */
    Address		endPC;     	/* pc at end of current run */
    int			sofar;     	/* count from start, excluding pauses */
    Timer_Ticks		startTime; 	/* time when run started */
    Timer_Ticks		endTime;   	/* time when run started */
    Mach_Time		totalTime;   	/* total time of all runs */
    Timer_Ticks		sofarTime;   	/* time from start, without pauses */
} Mach_InstCountInfo;

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetBootArgs --
 *
 * Spur doesn't implement parameters to the prom, so just return 0.
 *
 * ----------------------------------------------------------------------------
 */
#define Mach_GetBootArgs(argc, bufferSize, argv, buffer) (0)

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountStart --
 *
 * Start counting instructions. The index parameter differentiates between
 * different counts. Interrupts should be off.
 *
 * ----------------------------------------------------------------------------
 */

#define Mach_InstCountStart(index) { \
    int modeReg; \
    if (mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)){\
	mach_InstCount[(index)].runs++; \
	if (mach_InstCount[(index)].on == TRUE) { \
	    panic("Instruction counting already on.\n"); \
	} \
	modeReg = Dev_CCIdleCounters(FALSE, MACH_MODE_PERF_COUNTER_OFF); \
	mach_InstCount[(index)].start = Mach_Read32bitCCReg(0x15 << 8); \
	Timer_ReadT0(&mach_InstCount[(index)].startTime);   \
	(void) Dev_CCIdleCounters(TRUE, modeReg); \
	mach_InstCount[(index)].on = TRUE; \
	mach_InstCount[(index)].startPC = Mach_GetPC(); \
	mach_InstCount[(index)].sofar = 0; \
	mach_InstCount[(index)].sofarTime = timer_TicksZeroSeconds; \
    } \
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountEnd --
 *
 * Stop counting instructions. The index parameter differentiates between
 * different counts. The number of instructions since the start is added
 * to the total. Interrupts should be off.
 *
 * ----------------------------------------------------------------------------
 */

#define Mach_InstCountEnd(index) { \
    int modeReg; \
    int diff; \
    Timer_Ticks	diffTime; \
    if (mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)&&\
	(mach_InstCount[(index)].on == TRUE)) { \
	modeReg = Dev_CCIdleCounters(FALSE, MACH_MODE_PERF_COUNTER_OFF); \
	mach_InstCount[(index)].end = Mach_Read32bitCCReg(0x15 << 8); \
	 Timer_ReadT0(&mach_InstCount[(index)].endTime) ; \
	(void) Dev_CCIdleCounters(TRUE, modeReg); \
	diff = mach_InstCount[(index)].end - mach_InstCount[(index)].start; \
 	Timer_SubtractTicks(mach_InstCount[(index)].endTime, \
			    mach_InstCount[(index)].startTime, \
			    &diffTime);  \
	if (diff > 20000) { \
	    panic("diff is %d.\n", diff); \
	} \
	diff += mach_InstCount[(index)].sofar; \
 	Timer_AddTicks(diffTime, mach_InstCount[(index)].sofarTime, \
	    &diffTime);  \
	if (diff > 0) { \
	    mach_InstCount[(index)].total +=  diff;\
 	    Timer_AddTicks(diffTime, mach_InstCount[(index)].totalTime.ticks,\
		    &mach_InstCount[(index)].totalTime.ticks); \
	} \
	mach_InstCount[(index)].on = FALSE; \
	mach_InstCount[(index)].endPC = Mach_GetPC(); \
	mach_InstCount[(index)].sofar = 0; \
    } \
}
/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountOff --
 *
 * Stop counting instructions, but don't record results.
 *
 * ----------------------------------------------------------------------------
 */

#define Mach_InstCountOff(index) { \
    if (mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)){\
	mach_InstCount[(index)].on = FALSE; \
    } \
}
/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountResume --
 *
 * Resume counting instructions.
 *
 * ----------------------------------------------------------------------------
 */
#define Mach_InstCountResume(index) { \
    int modeReg; \
    if (mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)&&\
	(mach_InstCount[(index)].on == TRUE)){\
	modeReg = Dev_CCIdleCounters(FALSE, MACH_MODE_PERF_COUNTER_OFF); \
	mach_InstCount[(index)].start = Mach_Read32bitCCReg(0x15 << 8); \
	Timer_ReadT0(&mach_InstCount[(index)].startTime); \
	(void) Dev_CCIdleCounters(TRUE, modeReg); \
	mach_InstCount[(index)].startPC = Mach_GetPC(); \
    } \
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountPause --
 *
 * Stop counting instructions, and store count so far for later continuation.
 *
 * ----------------------------------------------------------------------------
 */
#define Mach_InstCountPause(index) { \
    int modeReg; \
    int diff; \
    if (mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)&&\
	(mach_InstCount[(index)].on == TRUE)) { \
	modeReg = Dev_CCIdleCounters(FALSE, MACH_MODE_PERF_COUNTER_OFF); \
	mach_InstCount[(index)].end = Mach_Read32bitCCReg(0x15 << 8); \
	Timer_ReadT0(&mach_InstCount[(index)].endTime); \
	(void) Dev_CCIdleCounters(TRUE, modeReg); \
	diff = mach_InstCount[(index)].end - mach_InstCount[(index)].start; \
	Timer_SubtractTicks(mach_InstCount[(index)].endTime, \
			    mach_InstCount[(index)].startTime, \
			    &diffTime); \
	if (diff > 20000) { \
	    panic("diff is %d.\n", diff); \
	} \
	if (diff > 0) { \
	    mach_InstCount[(index)].sofar +=  diff;\
	    Timer_AddTicks(diffTime, mach_InstCount[(index)].sofarTime, \
		    &mach_InstCount[(index)].sofarTime); \
	} \
	mach_InstCount[(index)].endPC = Mach_GetPC(); \
    } \
}
/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InstCountIsOn --
 *
 *	TRUE if instruction counting is on.
 *
 * ----------------------------------------------------------------------------
 */

#define Mach_InstCountIsOn(index) \
    ((mach_DoInstCounts&&((index) >= 0) && ((index) < MACH_MAX_INST_COUNT)&&\
	(mach_InstCount[(index)].on == TRUE)) ? TRUE : FALSE)


extern	Boolean	mach_KernelMode[];
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel[];
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
extern ReturnStatus		Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern ReturnStatus		Mach_GetEncapSize();

/*
 * Other routines.
 */
extern void			Mach_InitSyscall();
extern void			Mach_SetHandler();
extern int			Mach_GetExcStackSize();
extern Mach_ProcessorStates	Mach_ProcessorState();

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
extern  void	Mach_EnableTimerIntr();
extern  ReturnStatus Mach_AllocExtIntrNumber();
extern	void	Mach_SetNonmaskableIntr();
extern  ReturnStatus Mach_CallProcessor();
extern int	Mach_GetNumProcessors();
extern Mach_RegState *Mach_GetDebugStateInfo();
extern void	Mach_GetInstCountInfo();
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
extern	int	mach_SpecialStackSize;
extern	Address	mach_KernEnd;
extern	Address	mach_FirstUserAddr;
extern	Address	mach_LastUserAddr;
extern	Address	mach_MaxUserStackAddr;
extern	int	mach_LastUserStackPage;

extern 	Mach_InstCountInfo mach_InstCount[MACH_MAX_INST_COUNT];
extern  Boolean	mach_DoInstCounts;

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
