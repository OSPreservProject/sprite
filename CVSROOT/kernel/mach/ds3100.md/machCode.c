/* 
 * machCode.c --
 *
 *     	C code for the mach module.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "machConst.h"
#include "machMon.h"
#include "machInt.h"
#include "mach.h"
#include "sys.h"
#include "sync.h"
#include "dbg.h"
#include "proc.h"
#include "procMigrate.h"
#include "sched.h"
#include "vm.h"
#include "vmMachInt.h"
#include "sig.h"
#include "sigMach.h"
#include "swapBuffer.h"
#include "net.h"

/*
 * Conversion of function to an unsigned value.
 */
#define F_TO_A	(Address)(unsigned)(int (*)())
/*
 *  Number of processors in the system.
 */
#ifndef NUM_PROCESSORS
#define NUM_PROCESSORS 1
#endif NUM_PROCESSORS

int mach_NumProcessors = NUM_PROCESSORS;

/*
 * TRUE if cpu was in kernel mode before the interrupt, FALSE if was in 
 * user mode.
 */
Boolean	mach_KernelMode;

/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel = FALSE;

/*
 * The machine type string is imported by the file system and
 * used when expanding $MACHINE in file names.
 */

char *mach_MachineType = "ds3100";

/*
 * The byte ordering/alignment type used with Fmt_Convert and I/O control data
 */
Fmt_Format mach_Format = SWAP_VAX_TYPE;

/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */
int mach_NumDisableInterrupts[NUM_PROCESSORS];
int *mach_NumDisableIntrsPtr = mach_NumDisableInterrupts;

/*
 * The format that the kernel stack has to be in to start a process off.
 */
typedef struct {
    int		magicNumber;		/* Magic number used to determine if
					 * the stack has been corrupted. */
    int		statusReg;		/* The status register value. */
    int		filler[10];		/* Extra space on the stack to
					 * store args and such. */
} KernelStack;

/*
 * The format of a signal stack that is pushed onto a user's stack when
 * a signal is handled.
 */
typedef struct {
    Sig_Stack	sigStack;
    Sig_Context	sigContext;
} SignalStack;

/*
 * Machine dependent variables.
 */
Address	mach_KernStart;
Address	mach_CodeStart;
Address	mach_StackBottom;
int	mach_KernStackSize;
Address	mach_KernEnd;
Address	mach_FirstUserAddr;
Address	mach_LastUserAddr;
Address	mach_MaxUserStackAddr;
int	mach_LastUserStackPage;

/*
 * The variables and tables below are used to dispatch kernel calls.
 */

#define MAXCALLS 120
#define MAXARGS  10

int machMaxSysCall;			/* Highest defined system call. */
/*
 * Tables to determine where to go to fetch the arguments for a system call.
 */
ReturnStatus (*machArgDispatch[MAXCALLS])();
ReturnStatus (*machArgDispatchTable[])() = {
    MachFetch0Args,
    MachFetch0Args,
    MachFetch0Args,
    MachFetch0Args,
    MachFetch0Args,
    MachFetch1Arg,
    MachFetch2Args,
    MachFetch3Args,
    MachFetch4Args,
    MachFetch5Args,
    MachFetch6Args,
};

ReturnStatus (*(mach_NormalHandlers[MAXCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
					 * the call for non-migrated processes.
					 */
ReturnStatus (*(mach_MigratedHandlers[MAXCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
					 * the call for migrated processes. */
int machKcallTableOffset;		/* Byte offset of the kcallTable field
					 * in a Proc_ControlBlock. */
int machStatePtrOffset;			/* Byte offset of the machStatePtr
					 * field in a Proc_ControlBlock. */
int machSpecialHandlingOffset;		/* Byte offset of the specialHandling
					 * field in a Proc_ControlBlock. */
/* 
 * Pointer to the state structure for the current process and the
 * current owner of the floating point unit.
 */
Mach_State	*machCurStatePtr = (Mach_State *)NIL;
Mach_State	*machFPCurStatePtr = (Mach_State *)NIL;

extern Boolean Dev_SIIIntr();
extern void Net_Intr();
extern void Timer_TimerServiceInterrupt();
extern void Dev_DC7085Interrupt();
extern void MachFPInterrupt();

static void MemErrorInterrupt();
/*
 * The interrupt handler table.
 */
void (*interruptHandlers[MACH_NUM_HARD_INTERRUPTS])() = {
    (void (*)())Dev_SIIIntr,
    Net_Intr,
    Dev_DC7085Interrupt,
    Timer_TimerServiceInterrupt,
    MemErrorInterrupt,
    MachFPInterrupt,
};

extern void Mach_KernGenException();
extern void Mach_UserGenException();
extern void VmMach_KernTLBException();
extern void VmMach_TLBModException();
extern void VmMach_UTLBMiss();
extern void VmMach_EndUTLBMiss();

/*
 * The kernel exception handlers.
 */
void (*machKernExcTable[])() = {
    Mach_KernGenException,
    VmMach_TLBModException,
    VmMach_KernTLBException,
    VmMach_KernTLBException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
    Mach_KernGenException,
};

/*
 * The kernel exception handlers.
 */
void (*machUserExcTable[])() = {
    Mach_UserGenException,
    VmMach_TLBModException,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
    MachSysCall,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
    Mach_UserGenException,
};

/*
 * Size of the instruction and data caches.
 */
unsigned	machDataCacheSize;
unsigned	machInstCacheSize;

/*
 * The debugger structure.
 */
Mach_DebugState	mach_DebugState;
Mach_DebugState *machDebugStatePtr = &mach_DebugState;

static void		SetupSigHandler();
static void		ReturnFromSigHandler();
static ReturnStatus 	Interrupt();

/*
 * Preallocate all machine state structs.
 */
Mach_State	machStateTable[VMMACH_MAX_KERN_STACKS];
int		nextStateIndex = 0;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_Init --
 *
 *	Initialize the exception vector table and some of the dispatching
 *	tables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The exception vector table is initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_Init()
{
    extern char end[], edata[];

    /*
     * Zero out the bss segment.
     */
    bzero(edata, end - edata);

    /*
     * Set exported machine dependent variables.
     */
    mach_KernStart = (Address)MACH_KERN_START;
    mach_KernEnd = (Address)MACH_KERN_END;
    mach_CodeStart = (Address)MACH_CODE_START;
    mach_StackBottom = (Address)MACH_STACK_BOTTOM;
    mach_KernStackSize = MACH_KERN_STACK_SIZE;
    mach_FirstUserAddr = (Address)MACH_FIRST_USER_ADDR;
    mach_LastUserAddr = (Address)MACH_LAST_USER_ADDR;
    mach_MaxUserStackAddr = (Address)MACH_MAX_USER_STACK_ADDR;
    mach_LastUserStackPage = (MACH_MAX_USER_STACK_ADDR - 1) / VMMACH_PAGE_SIZE;

#ifdef notdef
    /*
     * Copy the boot parameter structure. The original location will get
     * unmapped during vm initialization so we need to get our own copy.
     */
    machMonBootParam = **(romVectorPtr->bootParam);
    offset = (int) *(romVectorPtr->bootParam) - (int) &(machMonBootParam);
    for (i = 0; i < 8; i++) {
	if (machMonBootParam.argPtr[i] != (char *) 0 &&
	 machMonBootParam.argPtr[i] != (char *) NIL) {
	    machMonBootParam.argPtr[i] -= (char *) offset;
	}
    }
#endif

    /*
     * Initialize some of the dispatching information.  The rest is
     * initialized by Mach_InitSysCall below.
     */
    machMaxSysCall = -1;
    machKcallTableOffset = (int) &((Proc_ControlBlock *) 0)->kcallTable;
    machStatePtrOffset = (int) &((Proc_ControlBlock *) 0)->machStatePtr;
    machSpecialHandlingOffset = (int) 
				&((Proc_ControlBlock *) 0)->specialHandling;

    /*
     * We start off with interrupts disabled.
     */
    mach_NumDisableInterrupts[0] = 1;

    /*
     * Copy down exception vector code.
     */
    if (F_TO_A VmMach_EndUTLBMiss - F_TO_A VmMach_UTLBMiss > 0x80) {
	panic("Mach_Init: UTLB code too large\n");
    }
    bcopy(F_TO_A VmMach_UTLBMiss, F_TO_A MACH_UTLB_MISS_EXC_VEC,
		F_TO_A VmMach_EndUTLBMiss - F_TO_A VmMach_UTLBMiss);
    bcopy(F_TO_A MachException, (Address)MACH_GEN_EXC_VEC,
	      F_TO_A MachEndException - F_TO_A MachException);

    /*
     * Clear out the i and d caches.
     */
    Mach_MonPrintf("Configuring cache: ");
    MachConfigCache();
    Mach_MonPrintf("data cache size =%x inst cache size=%x\n",
		   machDataCacheSize, machInstCacheSize);
    MachFlushCache();
}

static Vm_ProcInfo	mainProcInfo;
static Mach_State	mainMachState;
static VmMach_ProcData	mainProcData;


/*
 *----------------------------------------------------------------------
 *
 * Mach_InitFirstProc --
 *
 *	Initialize the machine state struct for the very first process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Machine info allocated and stack start set up.
 *
 *----------------------------------------------------------------------
 */
void
Mach_InitFirstProc(procPtr)
    Proc_ControlBlock	*procPtr;
{
    procPtr->machStatePtr = &mainMachState;
    procPtr->machStatePtr->kernStackStart = mach_StackBottom;
    procPtr->machStatePtr->kernStackEnd = 
				(mach_StackBottom + mach_KernStackSize);
    /*
     * Set up the TLB entries.  This code depends
     * implictly upon MACH_KERN_STACK_PAGES.
     */
    procPtr->machStatePtr->tlbHighEntry  = 0;
    procPtr->machStatePtr->tlbLowEntries[0] = 0;
    procPtr->machStatePtr->tlbLowEntries[1] = 0;
    procPtr->machStatePtr->tlbLowEntries[2] = 0;
    procPtr->vmPtr = &mainProcInfo;
    procPtr->vmPtr->machPtr = &mainProcData;
    VmMach_ProcInit(&mainProcInfo);
    machCurStatePtr = procPtr->machStatePtr;
    VmMach_SetupContext(procPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_SetupNewState --
 *
 *	Initialize the machine state for this process.  This includes 
 *	allocating and initializing a kernel stack.  Assumed that will
 *	be called when starting a process after a fork or restarting a
 *	process after a migration.
 *
 * Results:
 *	PROC_NO_STACKS if couldn't allocate a kernel stack.  SUCCESS otherwise.
 *
 * Side effects:
 *	Machine state in the destination process control block is overwritten.
 *
 *----------------------------------------------------------------------
 */ 
ReturnStatus
Mach_SetupNewState(procPtr, fromStatePtr, startFunc, startPC, user)
    Proc_ControlBlock	*procPtr;	/* Pointer to process control block
					 * to initialize state for. */
    Mach_State		*fromStatePtr;	/* State of parent on fork or from
					 * other machine on migration. */
    void		(*startFunc)();	/* Function to call when process first
					 * starts executing. */
    Address		startPC;	/* Address to pass as argument to 
					 * startFunc.  If NIL then the address
					 * is taken from *fromStatePtr's 
					 * exception stack. */
    Boolean		user;		/* TRUE if is a user process. */
{
    register	KernelStack	*stackPtr;
    register	Mach_State	*statePtr;
    unsigned			virtPage;

    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = &machStateTable[nextStateIndex];
	nextStateIndex++;
	if (nextStateIndex >= VMMACH_MAX_KERN_STACKS) {
	    panic("Mach_SetupNewState: Out of machine state structs\n");
	}
    }

    statePtr = procPtr->machStatePtr;
    /* 
     * Allocate a kernel stack for this process.
     */
    statePtr->kernStackStart = Vm_GetKernelStack(0);
    if (statePtr->kernStackStart == (Address)NIL) {
	return(PROC_NO_STACKS);
    }
    statePtr->kernStackEnd = statePtr->kernStackStart + MACH_KERN_STACK_SIZE;
    /*
     * Set up the TLB entries.  This code depends
     * implictly upon MACH_KERN_STACK_PAGES.
     */
    virtPage = (unsigned)(statePtr->kernStackStart + VMMACH_PAGE_SIZE) >>
							VMMACH_PAGE_SHIFT;
    statePtr->tlbHighEntry = (virtPage << VMMACH_TLB_VIRT_PAGE_SHIFT) |
			     (VMMACH_KERN_PID << VMMACH_TLB_PID_SHIFT);
    virtPage -= VMMACH_VIRT_CACHED_START_PAGE;
    statePtr->tlbLowEntries[0] = vmMach_KernelTLBMap[virtPage];
    statePtr->tlbLowEntries[1] = vmMach_KernelTLBMap[virtPage + 1];
    statePtr->tlbLowEntries[2] = vmMach_KernelTLBMap[virtPage + 2];

    statePtr->switchRegState.regs[SP] =
		    (unsigned)(statePtr->kernStackEnd - sizeof(KernelStack));
    /*
     * Initialize the stack so that it looks like it is in the middle of
     * Mach_ContextSwitch.
     */
    stackPtr = (KernelStack *)(statePtr->switchRegState.regs[SP]);
    stackPtr->magicNumber = MAGIC;
    stackPtr->statusReg = 0;
    statePtr->switchRegState.regs[RA] = (unsigned)startFunc;
    /*
     * Set up the user's stack pointer.
     */
    statePtr->userState.regState.regs[SP] = (unsigned)mach_MaxUserStackAddr;
    /* 
     * Set up the state of the process.  User processes inherit from their
     * parent or the migrated process.  If the PC is not specified, take it
     * from the parent as well.
     */
    if (user) {
	bcopy((Address)&fromStatePtr->userState,
		  (Address)&statePtr->userState,
		  sizeof(statePtr->userState));
	statePtr->userState.regState.pc += 4;
    }
    if (startPC == (Address)NIL) {
	statePtr->switchRegState.regs[A0] = 
			(unsigned)fromStatePtr->userState.regState.pc + 4;
    } else {
	statePtr->switchRegState.regs[A0] = (unsigned)startPC;
    }
    statePtr->userState.regState.fpStatusReg = 0;

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_SetReturnVal --
 *
 *	Set the return value for a process from a system call.  Intended to
 *	be called by the routine that starts a user process after a fork.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Register D0 is set in the user registers.
 *
 *----------------------------------------------------------------------
 */ 
void
Mach_SetReturnVal(procPtr, retVal)
    Proc_ControlBlock	*procPtr;	/* Process to set return value for. */
    int			retVal;		/* Value for process to return. */
{
    procPtr->machStatePtr->userState.regState.regs[V0] = (unsigned)retVal;
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_StartUserProc --
 *
 *	Start a user process executing for the first time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stack pointer and the program counter set for the process and
 *	the current process's image is replaced.
 *
 *----------------------------------------------------------------------
 */
void
Mach_StartUserProc(procPtr, entryPoint)
    Proc_ControlBlock	*procPtr;	/* Process control block for process
					 * to start. */
    Address		entryPoint;	/* Where process is to start
					 * executing. */
{
    register	Mach_State	*statePtr;

    statePtr = procPtr->machStatePtr;
    statePtr->userState.regState.pc = entryPoint;
    (void)MachUserReturn(procPtr);

    MachRunUserProc(entryPoint, statePtr->userState.regState.regs[SP]);
    /* THIS DOES NOT RETURN */
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_ExecUserProc --
 *
 *	Replace the calling user process's image with a new one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stack pointer set for the process.
 *
 *----------------------------------------------------------------------
 */
void
Mach_ExecUserProc(procPtr, userStackPtr, entryPoint)
    Proc_ControlBlock	*procPtr;		/* Process control block for
						 * process to exec. */
    Address		userStackPtr;	/* Stack pointer for when the
						 * user process resumes 
						 * execution. */
    Address		entryPoint;		/* Where the user process is
						 * to resume execution. */
{
    procPtr->machStatePtr->userState.regState.regs[SP] =
						(unsigned)userStackPtr;
    Mach_StartUserProc(procPtr, entryPoint);
    /* THIS DOES NOT RETURN */
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_FreeState --
 *
 *	Free up the machine state for the given process control block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Free up the kernel stack.
 *
 *----------------------------------------------------------------------
 */
void
Mach_FreeState(procPtr)
    Proc_ControlBlock	*procPtr;	/* Process control block to free
					 * machine state for. */
{
    if (procPtr->machStatePtr->kernStackStart != (Address)NIL) {
	Vm_FreeKernelStack(procPtr->machStatePtr->kernStackStart);
	procPtr->machStatePtr->kernStackStart = (Address)NIL;
    }
    if (procPtr->machStatePtr == machFPCurStatePtr) {
	machFPCurStatePtr = (Mach_State *)NIL;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_CopyState --
 *
 *	Copy the state from the given state structure to the machine
 *	state structure for the destination process control block.  Intended
 *	to be used by the debugger to modify the state.The only fields
 *	that can be modified are the following:
 *
 *	    1) user stack pointer
 *	    2) all trap registers except for the stack pointer because the
 *	       stack pointer in the trap registers is the kernel stack pointer.
 *	    3) the PC, VOR and status register in the exception stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Machine state in the destination process control block is overwritten.
 *
 *----------------------------------------------------------------------
 */ 
void
Mach_CopyState(statePtr, destProcPtr)
    Mach_State		*statePtr;	/* Pointer to state to copy from. */
    Proc_ControlBlock	*destProcPtr;	/* Process control block to copy
					 * state to. */
{
    bcopy((Address)&statePtr->userState,
	      (Address)&destProcPtr->machStatePtr->userState,
	      sizeof(statePtr->userState));
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetDebugState --
 *
 *	Extract the appropriate fields from the machine state struct
 *	and store them into the debug struct.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Debug struct filled in from machine state struct.
 *
 *----------------------------------------------------------------------
 */ 
void
Mach_GetDebugState(procPtr, debugStatePtr)
    Proc_ControlBlock	*procPtr;
    Proc_DebugState	*debugStatePtr;
{
    register	Mach_State	*machStatePtr;

    machStatePtr = procPtr->machStatePtr;
    bcopy((Address)machStatePtr->userState.regState.regs,
	      (Address)debugStatePtr->regState.regs,
	      sizeof(machStatePtr->userState.regState.regs));
    debugStatePtr->regState.pc = machStatePtr->userState.regState.pc;
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_SetDebugState --
 *
 *	Extract the appropriate fields from the debug struct
 *	and store them into the machine state struct.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Machine state struct filled in from the debug state struct.
 *
 *----------------------------------------------------------------------
 */ 
void
Mach_SetDebugState(procPtr, debugStatePtr)
    Proc_ControlBlock	*procPtr;
    Proc_DebugState	*debugStatePtr;
{
    register	Mach_State	*machStatePtr;

    machStatePtr = procPtr->machStatePtr;
    bcopy((Address)debugStatePtr->regState.regs,
	      (Address)machStatePtr->userState.regState.regs,
	      sizeof(machStatePtr->userState.regState.regs));
    machStatePtr->userState.regState.pc = debugStatePtr->regState.pc;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetUserStackPtr --
 *
 *	Return the user stack pointer from the machine state struct for the
 *	given process.
 *
 * Results:
 *	The value of the user stack pointer when the process trapped.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Address
Mach_GetUserStackPtr(procPtr)
    Proc_ControlBlock	*procPtr;
{
    return((Address)procPtr->machStatePtr->userState.regState.regs[SP]);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetStackPointer --
 *
 *	Return the value of the stack pointer.
 *
 * Results:
 *	The value of the user's stack pointer.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Address
Mach_GetStackPointer(procPtr)
    Proc_ControlBlock	*procPtr;
{
    return((Address)procPtr->machStatePtr->userState.regState.regs[SP]);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_InitSyscall --
 *
 *	During initialization, this procedure is called once for each
 *	kernel call, in order to set up information used to dispatch
 *	the kernel call.  This procedure must be called once for each
 *	kernel call, in order starting at 0.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the dispatch tables for the kernel call.
 *
 *----------------------------------------------------------------------
 */
void
Mach_InitSyscall(callNum, numArgs, normalHandler, migratedHandler)
    int callNum;			/* Number of the system call. */
    int numArgs;			/* Number of one-word arguments passed
					 * into call on stack. */
    ReturnStatus (*normalHandler)();	/* Procedure to process kernel call
					 * when process isn't migrated. */
    ReturnStatus (*migratedHandler)();	/* Procedure to process kernel call
					 * for migrated processes. */
{
    machMaxSysCall++;
    if (machMaxSysCall != callNum) {
	printf("Warning: out-of-order kernel call initialization, call %d\n",
	       callNum);
    }
    if (machMaxSysCall >= MAXCALLS) {
	printf("Warning: too many kernel calls.\n");
	machMaxSysCall--;
	return;
    }
    if (numArgs > MAXARGS) {
	printf("Warning: too many arguments to kernel call %d\n", callNum);
	numArgs = MAXARGS;
    }
    machArgDispatch[machMaxSysCall] = machArgDispatchTable[numArgs];
    mach_NormalHandlers[machMaxSysCall] = normalHandler;
    mach_MigratedHandlers[machMaxSysCall] = migratedHandler;
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachUserExceptionHandler --
 *
 *      Handle a user exception.
 *
 * Results:
 *      TRUE if should enable the floating point coprocessor.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUserExceptionHandler(statusReg, causeReg, badVaddr, pc)
    unsigned	statusReg;	/* The status register at the time of the
				 * exception. */
    unsigned	causeReg;	/* The cause register - contains the type
				 * of exception. */
    Address	badVaddr;	/* The address (if any) that the fault
				 * occured on. */
    Address	pc;		/* Program counter where to continue. */
{
    register	Proc_ControlBlock	*procPtr;
    int					cause;
    Boolean				retVal;
    ReturnStatus			status;

    cause = (causeReg & MACH_CR_EXC_CODE) >> MACH_CR_EXC_CODE_SHIFT;
    if (cause != MACH_EXC_INT) {
	Mach_EnableIntr();
    }

    procPtr = Proc_GetActualProc();
    switch (cause) {
	case MACH_EXC_INT:
	    status = Interrupt(statusReg, causeReg);
	    if (status != MACH_OK) {
		panic("MachUserExceptionHandler: nested interrupts.\n");
	    }
	    /* 
	     * Enable interrupts so that we can do the user mode return
	     * checks.
	     */
	    Mach_EnableIntr();
	    break;
	case MACH_EXC_TLB_MOD:
	    if (VmMach_TLBModFault(badVaddr) != SUCCESS) {
		printf("Protection fault in process %x: pc=%x addr=%x\n",
			procPtr->processID, pc, badVaddr);
		(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
				procPtr->processID, FALSE);
	    }
	    break;
	case MACH_EXC_TLB_LD_MISS:
	case MACH_EXC_TLB_ST_MISS:
	    if (VmMach_TLBFault(badVaddr) != SUCCESS) {
		printf("Bad user TLB fault in process %x: pc=%x addr=%x\n",
			procPtr->processID, pc, badVaddr);
		(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
				procPtr->processID, FALSE);
	    }
	    break;
	case MACH_EXC_ADDR_ERR_LD:
	case MACH_EXC_ADDR_ERR_ST:
	    printf("Address fault in process %x: pc=%x addr=%x\n",
			procPtr->processID, pc, badVaddr);
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE);
	    break;
	case MACH_EXC_BUS_ERR_IFETCH:
	    printf("MachExceptionHandler: User bus error on ifetch");
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE);
	    break;
	case MACH_EXC_BUS_ERR_LD_ST:
	    printf("MachExceptionHandler: User bus error on ld or st");
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE);
	    break;
	case MACH_EXC_SYSCALL:
	    if (!MachUNIXSyscall()) {
		printf("MachExceptionHandler: Bad syscall magic for proc %x\n",
							procPtr->processID);
		(void) Sig_Send(SIG_ILL_INST, SIG_BAD_TRAP,
				procPtr->processID, FALSE);
	    }
	    break;
	case MACH_EXC_BREAK: {
	    unsigned inst;
	    /*
	     * Check to see if this is a return from signal handler
	     * break or a normal breakpoint.
	     */
	    if (Vm_CopyIn(sizeof(int), pc, (Address)&inst) != SUCCESS) {
		panic("MachExceptionHandler: Couldn't fetch break inst.");
	    }
	    switch ((int)(inst & MACH_BREAK_CODE_FIELD)) {
		case MACH_BREAKPOINT_VAL:
		    Proc_Lock(procPtr);
		    if (procPtr->genFlags & PROC_DEBUG_ON_EXEC) {
			procPtr->genFlags &= ~PROC_DEBUG_ON_EXEC;
			(void) Sig_SendProc(procPtr, SIG_DEBUG, 
					    SIG_NO_CODE);
		    } else {
			(void) Sig_SendProc(procPtr, SIG_BREAKPOINT, 
					    SIG_NO_CODE);
		    }
		    Proc_Unlock(procPtr);
		    break;
		case MACH_SSTEP_VAL: {
		    ReturnStatus	status;

		    Vm_ChangeCodeProt(procPtr, pc, 4, TRUE);
		    status = Vm_CopyOut(4, (Address)&machCurStatePtr->sstepInst,					pc);
		    Vm_ChangeCodeProt(procPtr, pc, 4, FALSE);
		    Vm_FlushCode(procPtr, pc, 4);
		    if (status != SUCCESS) {
			panic("MachUserExceptionHandler: Bad sstep PC\n");
		    }
		    Proc_Lock(procPtr);
		    (void) Sig_SendProc(procPtr, SIG_TRACE_TRAP, SIG_NO_CODE);
		    Proc_Unlock(procPtr);
		    break;
		}

		case MACH_SIG_RET_VAL:
		    ReturnFromSigHandler(procPtr);
		    break;
		default:
		    printf("Bogus bp-trap\n");
		    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
				    procPtr->processID, FALSE);
		    break;
	    }
	    break;
	}
	case MACH_EXC_RES_INST:
	    printf("Reserved instruction in process %x at pc=%x\n",
			procPtr->processID, pc);
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE);
	    break;
	case MACH_EXC_COP_UNUSABLE:
	    MachSwitchFPState(machFPCurStatePtr, machCurStatePtr);
	    machFPCurStatePtr = machCurStatePtr;
	    break;
	case MACH_EXC_OVFLOW:
	    printf("Overflow exception in process %x at pc=%x\n",
			procPtr->processID, pc);
	    (void) Sig_Send(SIG_ARITH_FAULT, SIG_OVERFLOW,
			    procPtr->processID, FALSE);
	    break;
    }
    retVal = MachUserReturn(procPtr);

    return(retVal);
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachKernelExceptionHandler --
 *
 *      Handle a kernel exception.
 *
 * Results:
 *      MACH_KERN_ERROR if the debugger should be called after this routine 
 *	returns, MACH_USER_ERROR if a copy to/from user space caused an 
 *	unrecoverable bus error, and MACH_OK if everything worked out ok.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
int
MachKernelExceptionHandler(statusReg, causeReg, badVaddr, pc)
    unsigned	statusReg;	/* The status register at the time of the
				 * exception. */
    unsigned	causeReg;	/* The cause register - contains the type
				 * of exception. */
    Address	badVaddr;	/* The address (if any) that the fault
				 * occured on. */
    Address	pc;		/* Program counter where to continue. */
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status;
    int					cause;

    cause = (causeReg & MACH_CR_EXC_CODE) >> MACH_CR_EXC_CODE_SHIFT;

    /*
     * Process kernel traps.
     */
    procPtr = Proc_GetActualProc();
    switch (cause) {
	case MACH_EXC_INT:
	    status = Interrupt(statusReg, causeReg);
	    return(status);
	case MACH_EXC_TLB_LD_MISS:
	case MACH_EXC_TLB_ST_MISS:
	case MACH_EXC_TLB_MOD: {
	    Boolean	copyInProgress = FALSE;

	    if (statusReg & MACH_SR_INT_ENA_PREV) {
		/*
		 * Enable interrupts.
		 */
		Mach_EnableIntr();
	    }

	    if (pc >= F_TO_A Vm_CopyIn && pc < F_TO_A VmMachCopyEnd) {
		copyInProgress = TRUE;
	    } else if (pc >= F_TO_A MachFetchArgs &&
	               pc <= F_TO_A MachFetchArgsEnd) {
		copyInProgress = TRUE;
	    } else if (badVaddr < (Address)VMMACH_VIRT_CACHED_START &&
		       (badVaddr >= (Address)VMMACH_PHYS_CACHED_START ||
			procPtr == (Proc_ControlBlock *)NIL ||
			procPtr->vmPtr->numMakeAcc == 0)) {
		printf("Bad kernel TLB Fault\n");
		return(MACH_KERN_ERROR);
	    }
	    if (((causeReg & MACH_CR_EXC_CODE) >> 
			MACH_CR_EXC_CODE_SHIFT) == MACH_EXC_TLB_MOD) {
		status = VmMach_TLBModFault(badVaddr);
	    } else {
		status = VmMach_TLBFault(badVaddr);
	    }
	    if (status != SUCCESS) {
		if (copyInProgress) {
		    return(MACH_USER_ERROR);
		} else {
		    return(MACH_KERN_ERROR);
		}
	    } else {
		return(MACH_OK);
	    }
	}

	case MACH_EXC_ADDR_ERR_LD:
	    printf("MachKernelExceptionHandler:  Address error on load\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_ADDR_ERR_ST:
	    printf("MachKernelExceptionHandler:  Address error on store\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_BUS_ERR_IFETCH:
	    printf("MachKernelExceptionHandler:  Bus error on ifetch\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_BUS_ERR_LD_ST:
	    if (pc >= F_TO_A Mach_ProbeAddr &&
	        pc <= F_TO_A MachProbeAddrEnd) {
		return(MACH_USER_ERROR);
	    }
	    printf("MachKernelExceptionHandler:  Bus error on load or store\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_SYSCALL:
	    printf("MachKernelExceptionHandler:  System call in kernel mode\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_BREAK:
	    return(MACH_KERN_ERROR);
	case MACH_EXC_RES_INST:
	    printf("MachKernelExceptionHandler:  Reserved instruction\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_COP_UNUSABLE:
	    printf("MachKernelExceptionHandler:  Coprocessor unusable\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_OVFLOW:
	    printf("MachKernelExceptionHandler:  Overflow\n");
	    return(MACH_KERN_ERROR);
	default:
	    printf("MachKernelExceptionHandler: Unknown exception\n");
	    return(MACH_KERN_ERROR);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Interrupt --
 *
 *      Call the proper interrupt handler for the given interrupt.
 *
 * Results:
 *      MACH_KERN_ERROR if the machine should go into the debugger, or MACH_OK
 *	otherwise.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
#define DEBUG_INTR
#ifdef DEBUG_INTR
static int lastInterruptCalled = -1;
#endif  /* DEBUG_INTR */


static ReturnStatus
Interrupt(statusReg, causeReg)
    unsigned	statusReg;
    unsigned	causeReg;
{
    int		n;
    unsigned	mask;

#ifdef DEBUG_INTR
    if (mach_AtInterruptLevel) {
	printf("Received interrupt while at interrupt level.\n");
    }
    if (mach_NumDisableIntrsPtr[0] > 0) {
	printf("Received interrupt with mach_NumDisableIntrsPtr[0] = %d.\n",
	       mach_NumDisableIntrsPtr[0]);
    }
#endif /* DEBUG_INTR */
    
    mach_KernelMode = !(statusReg & MACH_SR_KU_PREV);
    mach_AtInterruptLevel = 1;
    n = 0;
    mask = (causeReg & statusReg & MACH_CR_INT_PENDING) >> 
						MACH_CR_HARD_INT_SHIFT;
    while (mask != 0) {
	if (mask & 1) {
#ifdef DEBUG_INTR
	    if (n >= MACH_NUM_HARD_INTERRUPTS) {
		printf("Bogus index (%d) for interrupt handler\n", n);
		mach_AtInterruptLevel = 0;
		return(MACH_KERN_ERROR);
	    }
	    lastInterruptCalled = n;
#endif /* DEBUG_INTR */
	    interruptHandlers[n]();
	}
	mask >>= 1;
	n++;
    }

    mach_AtInterruptLevel = 0;
#ifdef DEBUG_INTR
    lastInterruptCalled = -1;
#endif /* DEBUG_INTR */
    return(MACH_OK);
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachUserReturn --
 *
 *      Take the proper action to return from a user exception.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Interrupts disabled.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUserReturn(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    SignalStack			sigStack;
    Address			pc;


    if (procPtr->Prof_Scale != 0 && procPtr->Prof_PC != 0) {
	Prof_RecordPC(procPtr);
    }

    /* 
     * Take a context switch if one is pending for this process.
     */
    if (procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING) {
	Sched_LockAndSwitch();
    }

    while (TRUE) {
	/*
	 * See if we are supposed to single-step this process.  If so
	 * put a break in the proper place.  Don't worry if we miss a signal
	 * because we will be back in real soon anyway (after one instruction).
	 */
	if (procPtr->genFlags & PROC_SINGLE_STEP_FLAG) {
	    Address		breakPC;
	    unsigned		breakInst;
	    ReturnStatus	status;
	    int			accLen;
	    Address		newAddr;

	    if (machFPCurStatePtr == machCurStatePtr) {
		MachGetCurFPState(machCurStatePtr);
	    }
	    Vm_MakeAccessible(VM_READONLY_ACCESS, sizeof(Address),
			      machCurStatePtr->userState.regState.pc, 
			      &accLen, &newAddr);
	    if (accLen != sizeof(Address)) {
		printf("MachUserReturn: Can't fetch user's trap PC\n");
		break;
	    }
	    breakPC = (Address)
		    MachEmulateBranch(machCurStatePtr->userState.regState.regs,
				      machCurStatePtr->userState.regState.pc,
			     machCurStatePtr->userState.regState.fpStatusReg,
						 TRUE);
	    Vm_MakeUnaccessible(newAddr, sizeof(Address));
#ifdef notdef
	    printf("breakPC=%x\n", (unsigned)breakPC);
#endif
	    if (Vm_CopyIn(4, breakPC,
			  (Address)&machCurStatePtr->sstepInst) != SUCCESS) {
		printf("Bad single-step address\n");
	    } else {
		breakInst = MACH_SSTEP_VAL | 0xd;
		Vm_ChangeCodeProt(procPtr, breakPC, 4, TRUE);
		status = Vm_CopyOut(4, (Address)&breakInst, breakPC);
		Vm_ChangeCodeProt(procPtr, breakPC, 4, FALSE);
		Vm_FlushCode(procPtr, breakPC, 4);
		if (status != SUCCESS) {
		    printf("Bad single-step address\n");
		}
	    }
	    procPtr->genFlags &= ~PROC_SINGLE_STEP_FLAG;
	    Mach_DisableIntr();
	    break;
	} else {
	    /*
	     * Disable interrupts.  Note that we don't use the DISABLE_INTR 
	     * macro because it increments the nesting depth of interrupts
	     * which we don't want because there is an implicit enable
	     * interrupts on rte.
	     */
	    Mach_DisableIntr();
	    if (!Sig_Pending(procPtr)) {
		break;
	    }
	    Mach_EnableIntr();
	    sigStack.sigStack.contextPtr = &sigStack.sigContext;
	    if (Sig_Handle(procPtr, &sigStack.sigStack, &pc)) {
		SetupSigHandler(procPtr, &sigStack, pc);
		Mach_DisableIntr();
		break;
	    }
	}
    }
    
    /*
     * It is possible for Sig_Handle to mask the migration signal
     * if a process is not in a state where it can be migrated.
     * As soon as we return to user mode, though, we will allow migration.
     */
    Sig_AllowMigration(procPtr);

    return(machFPCurStatePtr == machCurStatePtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Routines to set up and return from signal handlers.
 *
 * In order to call a handler four things must be done:
 *
 *	1) The current state of the process must be saved so that when
 *	   the handler returns a normal return to user space can occur.
 *	2) The user stack must be set up so that the signal number and the
 *	   the signal code are passed to the handler.
 *	3) Things must be set up so that when the handler returns it returns
 *	   back into the kernel so that state can be cleaned up.
 *	4) The trap stack that was created when the kernel was entered and is
 *	   used to return a process to user space must be modified so that
 *	   the signal handler is called instead of executing the
 *	   normal return.
 *
 * The last one is done by simply changing the program counter where the
 * user process will execute on return to be the address of the signal
 * handler and the user stack pointer to point to the proper place on
 * the user stack.  The first three of these are accomplished by 
 * setting up the user's registers properly.  The return address register is the
 * return address where the handler will start executing upon return.  But 
 * this is just the address of a trap instruction that is stored on the stack
 * below.  Thus when a handler returns it will execute a trap instruction 
 * and drop back into the kernel. 
 */


/*
 * ----------------------------------------------------------------------------
 *
 * SetupSigHandler --
 *
 *      Save machine state on the users stack and set up the exception stack
 *	so that the user will call the signal handler on return. In order to
 * Results:
 *      None.
 *
 * Side effects:
 *      Signal stack set up and saved.
 *
 * ----------------------------------------------------------------------------
 */
static void
SetupSigHandler(procPtr, sigStackPtr, pc)
    register	Proc_ControlBlock	*procPtr;
    register	SignalStack		*sigStackPtr;
    Address				pc;
{
    Mach_UserState	*userStatePtr;
    unsigned		usp;

    userStatePtr = &procPtr->machStatePtr->userState;
    usp = userStatePtr->regState.regs[SP] - sizeof(Sig_Context) - 
	  MACH_STAND_FRAME_SIZE;

    sigStackPtr->sigContext.machContext.break1Inst = 0x1000d;
    /*
     * Copy the user state onto the signal stack.
     */
    bcopy((Address)userStatePtr,
	      (Address)&(sigStackPtr->sigContext.machContext.userState),
	      sizeof(Mach_UserState));
    /*
     * Copy over the floating point state.
     */
    if (machFPCurStatePtr == machCurStatePtr) {
	MachGetCurFPState(machCurStatePtr);
    }
    bcopy((Address)procPtr->machStatePtr->userState.regState.fpRegs,
	  (Address)sigStackPtr->sigContext.machContext.fpRegs,
	  MACH_NUM_FPRS * sizeof(int));
    sigStackPtr->sigContext.machContext.fpStatusReg =
			procPtr->machStatePtr->userState.regState.fpStatusReg;
    /*
     * Copy the stack out to user space.
     */
    if (Vm_CopyOut(sizeof(Sig_Context), (Address)&sigStackPtr->sigContext,
		    (Address)(usp + MACH_STAND_FRAME_SIZE)) != SUCCESS) {
        printf("Warning: HandleSig: No room on stack for signal, PID=%x.\n",
                  procPtr->processID);
        Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    /*
     * Now set up the registers correctly.
     */
    userStatePtr->regState.regs[SP] = usp;
    userStatePtr->regState.regs[A0] = sigStackPtr->sigStack.sigNum;
    userStatePtr->regState.regs[A1] = sigStackPtr->sigStack.sigCode;
    userStatePtr->regState.regs[A2] = usp + MACH_STAND_FRAME_SIZE;
    userStatePtr->regState.pc = pc;
    userStatePtr->regState.regs[RA] = usp + MACH_STAND_FRAME_SIZE + 
		(unsigned)&((Sig_Context *)0)->machContext.break1Inst;
}


/*
 * ----------------------------------------------------------------------------
 *
 * ReturnFromSigHandler --
 *
 *      Process a return from a signal handler.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *      Signal stack struct and size filled in the machine struct for the
 *	given process.
 *
 * ----------------------------------------------------------------------------
 */
static void
ReturnFromSigHandler(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    register	Mach_State	*statePtr;
    SignalStack			sigStack;

    statePtr = procPtr->machStatePtr;
    /*
     * Copy the signal stack in.
     */
    if (Vm_CopyIn(sizeof(Sig_Context),
		  (Address)(statePtr->userState.regState.regs[SP] + 
			    MACH_STAND_FRAME_SIZE),
		  (Address) &sigStack.sigContext) != SUCCESS) {
	printf("%s Mach_Code: Stack too small to extract trap info, PID=%x.\n",
		"Warning:", procPtr->processID);
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    sigStack.sigStack.contextPtr = &sigStack.sigContext;
    /*
     * Take the proper action on return from a signal.
     */
    Sig_Return(procPtr, &sigStack.sigStack);
    /*
     * Restore user state.
     */
    bcopy((Address)&sigStack.sigContext.machContext.userState,
	      (Address)&statePtr->userState, sizeof(statePtr->userState));
    /*
     * Copy in the floating point state.
     */
    bcopy((Address)sigStack.sigContext.machContext.fpRegs,
	  (Address)statePtr->userState.regState.fpRegs,
	  MACH_NUM_FPRS * sizeof(int));
    statePtr->userState.regState.fpStatusReg = 
	sigStack.sigContext.machContext.fpStatusReg & ~MACH_FPC_EXCEPTION_BITS;
    if (machFPCurStatePtr == machCurStatePtr) {
	machFPCurStatePtr = (Mach_State *)NIL;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_ProcessorState --
 *
 *	Determines what state the processor is in.
 *
 * Results:
 *	MACH_USER	if was at user level
 *	MACH_KERNEL	if was at kernel level
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
Mach_ProcessorStates 
Mach_ProcessorState(processor)
    int processor;	/* processor number for which info is requested */
{
    if (mach_KernelMode) {
	return(MACH_KERNEL);
    } else {
	return(MACH_USER);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetMachineArch --
 *
 *	Return the machine architecture.
 *
 * Results:
 *	The machine architecture.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_GetMachineArch()
{
    return SYS_DS3100;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetMachineType --
 *
 *	Return the machine type.
 *
 * Results:
 *	0.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_GetMachineType()
{
    return 0;
}


/*
 * ----------------------------------------------------------------------------
 *
 *  Mach_CheckSpecialHandling--
 *
 *	Forces a processor to check the special handling flag of a process.
 *	This should only be called on a multiprocessor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_CheckSpecialHandling(pnum)
    int		pnum;		/* Processor number. */
{
    panic("Mach_CheckSpecialHandling called for processor %d\n",pnum);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetNumProcessors() --
 *
 *	Return the number of processors in the system.  NOTE: This should
 *	really be in a machine-independent area of the mach module.  Note
 *	further: if this is used only as a system call, it should return
 *	a ReturnStatus!
 *
 * Results:
 *	The number of processors is returned.  
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Mach_GetNumProcessors()
{
	return (mach_NumProcessors);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetBootArgs --
 *
 *	Returns the arguments out of the boot parameter structure. 
 *
 * Results:
 *	Number of elements returned in argv.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Mach_GetBootArgs(argc, bufferSize, argv, buffer)
	int	argc;			/* Number of elements in argv */
	int	bufferSize;		/* Size of buffer */
	char	**argv;			/* Ptr to array of arg pointers */
	char	*buffer;		/* Storage for arguments */
{
#ifdef notdef
    int		i;
    int		offset;

    bcopy(machMonBootParam.strings, buffer, 
	  (bufferSize < 100) ? bufferSize : 100);
    offset = (int) machMonBootParam.strings - (int) buffer;
    for(i = 0; i < argc; i++) {
	if (machMonBootParam.argPtr[i] == (char *) 0 ||
	    machMonBootParam.argPtr[i] == (char *) NIL) {
	    break;
	}
	argv[i] = (char *) (machMonBootParam.argPtr[i] - (char *) offset);
    }
    return i;
#endif
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetEtherAddress --
 *
 *	Return the ethernet address out of the rom.
 *
 * Results:
 *	Number of elements returned in argv.
 *
 * Side effects:
 *	*etherAddrPtr gets the ethernet address.
 *
 *----------------------------------------------------------------------
 */
void
Mach_GetEtherAddress(etherAddrPtr)
    Net_EtherAddress	*etherAddrPtr;
{
    volatile unsigned    *romPtr = (unsigned *)0xBD000000;

    etherAddrPtr->byte1 = (romPtr[0] >> 8) & 0xff;
    etherAddrPtr->byte2 = (romPtr[1] >> 8) & 0xff;
    etherAddrPtr->byte3 = (romPtr[2] >> 8) & 0xff;
    etherAddrPtr->byte4 = (romPtr[3] >> 8) & 0xff;
    etherAddrPtr->byte5 = (romPtr[4] >> 8) & 0xff;
    etherAddrPtr->byte6 = (romPtr[5] >> 8) & 0xff;
}


/*
 *----------------------------------------------------------------------
 *
 * MemErrorInterrupt --
 *
 *	Handler an interrupt for the DZ device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
MemErrorInterrupt()
{
    unsigned short *sysCSRPtr = (unsigned short *)0xbe000000;
    unsigned short csr;

    csr = *sysCSRPtr;

    if (csr & MACH_CSR_MEM_ERR) {
	panic("Mem error interrupt\n");
    } else {
	*sysCSRPtr = MACH_CSR_VINT | csr | 0x00ff;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_FlushCode --
 *
 *	Flush the kernel code from the icache at the given address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Mach_FlushCode(addr, len) 
    Address	addr;
    unsigned	len;
{
    unsigned cacheAddr;

    cacheAddr = VMMACH_PHYS_UNCACHED_START - machInstCacheSize +  
		((unsigned)addr & (machInstCacheSize - 1));
#ifdef notdef
    printf("len=%d addr=%x cacheAddr=%x\n", len, addr, cacheAddr);
#endif
    MachCleanICache(cacheAddr, len);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_SendSignal
 *
 *	Send either an illegal instruction or a floating point exception
 *	to the current process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Mach_SendSignal(sigType)
    int	sigType;
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetActualProc();

    printf("Mach_SendSignal: Sending signal\n");

    switch ((int)sigType) {
	case MACH_SIGFPE:
	    (void) Sig_Send(SIG_ILL_INST, SIG_FP_EXCEPTION,
			    procPtr->processID, FALSE);
	    break;
	case MACH_SIGILL:
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE);
	    break;
	default:
	    panic("Mach_SendSignal: Bad signal type\n");
	    break;
    }
}

PrintError()
{
    panic("Error on stack\n");
}

PrintInst(pc, inst)
    unsigned pc;
    unsigned inst;
{
    printf("Emulating 0x%x: 0x%x\n", pc, inst);
}

SoftFPReturn()
{
    printf("SoftFPReturn\n");
}
