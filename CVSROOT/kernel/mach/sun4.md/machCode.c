/* 
 * machCode.c --
 *
 *     C code for the mach module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "swapBuffer.h"
#include "machConst.h"
#include "machMon.h"
#include "machInt.h"
#include "mach.h"
#include "proc.h"
#include "sys.h"
#include "sched.h"
#include "vm.h"
#include "vmMachInt.h"

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
 * Sp saved into this before debugger call.
 */
int	machSavedRegisterState = 0;

/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel = FALSE;

/*
 * The machine type string is imported by the file system and
 * used when expanding $MACHINE in file names.
 */

char *mach_MachineType = "sun4";

/*
 * The byte ordering/alignment type used with Fmt_Convert and I/O control data.
 * For compatablity we set this to the old Swap_Buffer constant.
 */
Fmt_Format	mach_Format = SWAP_SPARC_TYPE;

/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */
int mach_NumDisableInterrupts[NUM_PROCESSORS];
int *mach_NumDisableIntrsPtr = mach_NumDisableInterrupts;

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

Address	machTBRAddr;			/* address of trap table.  The value
					 * is set up and stored here in
					 * bootSysAsm.s.*/
#define	MACH_NUM_VECTORS	256	/* Number of interrupt vector slots */
Address	machVectorTable[MACH_NUM_VECTORS];	/* Table of autovector and
					 * vectored interrupt handlers. */
ClientData	machInterruptArgs[MACH_NUM_VECTORS];	/* Table of clientData
					 * args to pass interrupt handlers */
int	machMaxSysCall;			/* Hightest defined system call. */
int	machArgOffsets[SYS_NUM_SYSCALLS];/* For each system call, tells how
					 * much to add to the fp at the time
					 * of the call to get to the highest
					 * argument on the stack.  */
Address	machArgDispatch[SYS_NUM_SYSCALLS];	/* For each system call, gives an
					 * address to branch to, in the
					 * middle of MachFetchArgs, to copy the
					 * right # of args from user space to
					 * the kernel's stack. */
ReturnStatus	(*(mach_NormalHandlers[SYS_NUM_SYSCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
					 * the call for non-migrated processes.
				         */
ReturnStatus (*(mach_MigratedHandlers[SYS_NUM_SYSCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
				         * the call for migrated processes. */
int	machKcallTableOffset;		/* Byte offset of the kcallTable field
					 * in a Proc_ControlBlock. */
int	machStatePtrOffset;		/* Byte offset of the machStatePtr
					 * field in a Proc_ControlBlock. */
int	machSpecialHandlingOffset;	/* Byte offset of the specialHandling
					 * field in a Proc_ControlBlock. */
int	MachPIDOffset;			/* Byte offset of pid in PCB */
char	mach_DebugStack[0x2000];	/* The debugger stack. */
unsigned int	machDebugStackStart;	/* Contains address of base of debugger
					 * stack. */
int	machSignalStackSizeOnStack;	/* size of mach module sig stack */
int	machSigStackSize;		/* size of Sig_Stack structure */
int	machSigStackOffsetOnStack;	/* offset of sigStack field in
					 * MachSignalStack structure on the
					 * the user stack. */
int	machSigStackOffsetInMach;	/* offset to sigStack field in mach
					 * state structure. */
int	machSigContextSize;		/* size of Sig_Context structure */
int	machSigContextOffsetOnStack;	/* offset of sigContext field in
					 * MachSignalStack structure on the
					 * user stack. */
int	machSigContextOffsetInMach;	/* offset to sigContext field in mach
					 * state structure. */
int	machSigUserStateOffsetOnStack;	/* offset of machine-dependent field
					 * on the stack, called machContext,
					 * in the Sig_Context part of the
					 * MachSignalStack structure. */
int	machSigTrapInstOffsetOnStack;	/* offset of trapInst field in
					 * MachSignalStack on user stack. */
int	machSigNumOffsetInSig;		/* offset of sigNum field in
					 * Sig_Stack structure. */
int	machSigCodeOffsetInSig;		/* offset of sigCode field in
					 * Sig_Stack structure. */
int	machSigPCOffsetOnStack;		/* offset of pcValue field in
					 * MachSignalStack on user stack. */

#ifdef FP_ENABLED
unsigned int	machSavedFsr;		/* saved %fsr on fp exception trap */
#endif FP_ENABLED

/*
 * Pointer to the state structure for the current process.
 */
Mach_State	*machCurStatePtr = (Mach_State *)NIL;

char	MachRunUserDeathString[] =
				"MachRunUserProc: killing process!\n"; 
char	MachHandleSignalDeathString[] =
				"MachHandleSignal: killing process!\n"; 
char	MachReturnFromSignalDeathString[] =
				"MachReturnFromSignal: killing process!\n"; 
char	MachReturnFromTrapDeathString[] =
				"MachReturnFromTrap: killing process!\n"; 
char	MachHandleWindowUnderflowDeathString[] =
				"MachHandleWindowUnderflow: killing process!\n";

/*
 * For testing correctness of defined offsets.
 */
Mach_RegState		testMachRegState;
Mach_State		testMachState;
Proc_ControlBlock	testPCB;
MachSignalStack		testSignalStack;
Sig_Context		testContext;
Sig_Stack		testStack;
int			debugCounter = 0;		/* for debugging */
int			debugSpace[500];
Address			theAddrOfVmPtr = 0; 
Address			theAddrOfMachPtr = 0;
Address			oldAddrOfVmPtr = 0; 
Address			oldAddrOfMachPtr = 0;

Address			machRomVectorPtr;
MachMonBootParam	machMonBootParam;

extern	void	MachFlushWindowsToStack();
    


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_Init --
 *
 *	Initialize some stuff.
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
Mach_Init()
{
    int		i;
    extern	void	MachVectoredInterrupt();
    extern	void	MachHandleDebugTrap();
    int		offset;

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

#define	CHECK_SIZE(c, d)		\
    if (sizeof (c) != d) {		\
	panic("Bad size for structure.  Redo machConst.h!\n");\
    }
	
#define	CHECK_OFFSETS(s, o)		\
    if ((unsigned int)(&(testMachState.s)) - \
	    (unsigned int)(&testMachState) != o) {\
	panic("Bad offset for registers.  Redo machConst.h!\n");\
    }
#define	CHECK_TRAP_REG_OFFSETS(s, o)		\
    if ((unsigned int)(&(testMachRegState.s)) -	\
	(unsigned int)(&testMachRegState) != o) {\
	panic("Bad offset for trap registers.  Redo machConst.h!\n");\
    }

    CHECK_SIZE(Mach_RegState, MACH_SAVED_STATE_FRAME);
    CHECK_OFFSETS(trapRegs, MACH_TRAP_REGS_OFFSET);
    CHECK_OFFSETS(switchRegs, MACH_SWITCH_REGS_OFFSET);
    CHECK_OFFSETS(savedRegs, MACH_SAVED_REGS_OFFSET);
    CHECK_OFFSETS(savedMask, MACH_SAVED_MASK_OFFSET);
    CHECK_OFFSETS(savedSps, MACH_SAVED_SPS_OFFSET);
    CHECK_OFFSETS(kernStackStart, MACH_KSP_OFFSET);
    CHECK_TRAP_REG_OFFSETS(curPsr, MACH_LOCALS_OFFSET);
    CHECK_TRAP_REG_OFFSETS(ins, MACH_INS_OFFSET);
    CHECK_TRAP_REG_OFFSETS(globals, MACH_GLOBALS_OFFSET);

#ifdef sun4c
    if ((*(romVectorPtr->virtMemory))->address != VMMACH_DEV_START_ADDR ||
	    (VMMACH_DEV_START_ADDR + (*(romVectorPtr->virtMemory))->size - 1)
	    != VMMACH_DEV_END_ADDR) {
	panic("VMMACH_DEV_START_ADDR and VMMACH_DEV_END_ADDR are wrong.\n");
    }
#endif /* sun4c */
#undef CHECK_SIZE
#undef CHECK_OFFSETS
    /*
     * Initialize some of the dispatching information.  The rest is initialized
     * by Mach_InitSysCall, below.
     */

    /*
     * Get offset of machStatePtr in proc control blocks.  This one is
     * subject to a different module, so it's easier not to use a constant.
     */
    machStatePtrOffset = (unsigned int)(&(testPCB.machStatePtr)) -
	    (unsigned int)(&testPCB);
    machKcallTableOffset = (unsigned int)(&(testPCB.kcallTable)) -
	    (unsigned int)(&testPCB);
    machSpecialHandlingOffset = (unsigned int)(&(testPCB.specialHandling)) -
	    (unsigned int)(&testPCB);
    machMaxSysCall = -1;
    MachPIDOffset = (unsigned int)(&(testPCB.processID)) -
	    (unsigned int)(&testPCB);

    /*
     * Initialize all the horrid offsets for dealing with getting stuff from
     * signal things in the mach state structure to signal things on the user
     * stack.
     */
    machSignalStackSizeOnStack = sizeof (MachSignalStack);
    if ((machSignalStackSizeOnStack & 0x7) != 0) {
	panic("MachSignalStack struct must be a multiple of double-words!\n");
    }

    machSigStackSize = sizeof (Sig_Stack);
    machSigStackOffsetOnStack = (unsigned int)(&(testSignalStack.sigStack)) -
	    (unsigned int)(&testSignalStack);
    machSigStackOffsetInMach = (unsigned int)(&(testMachState.sigStack)) -
	    (unsigned int)(&testMachState);
  
    machSigContextSize = sizeof (Sig_Context);
    machSigContextOffsetOnStack = (unsigned int)(&(testSignalStack.sigContext))-
	    (unsigned int)(&testSignalStack);
    machSigContextOffsetInMach = (unsigned int)(&(testMachState.sigContext)) -
	    (unsigned int)(&testMachState);

    machSigUserStateOffsetOnStack =
	    (unsigned int) (&(testSignalStack.sigContext.machContext.userState))
	    - (unsigned int)(&testSignalStack);
    
    machSigTrapInstOffsetOnStack =
	    (unsigned int) (&(testSignalStack.sigContext.machContext.trapInst))
	    - (unsigned int)(&testSignalStack);

    machSigNumOffsetInSig = (unsigned int) (&(testStack.sigNum)) -
	    (unsigned int)(&testStack);
    machSigCodeOffsetInSig = (unsigned int) (&(testStack.sigCode)) -
	    (unsigned int)(&testStack);
 
    machSigPCOffsetOnStack =
	    (unsigned int)(&(testSignalStack.sigContext.machContext.pcValue)) -
	    (unsigned int)(&testSignalStack);

    /*
     * base of the debugger stack
     */
    machDebugStackStart = (unsigned int) mach_DebugStack +
						sizeof (mach_DebugStack);

    /*
     * Initialize the interrupt vector table.
     */
    for (i = 0; i < MACH_NUM_VECTORS; i++) {
#ifndef sun4c
	if (i == 13 || i == 11 || i == 9 || i == 7 || i == 5 || i == 3 ||
		i == 2) {
	    machVectorTable[i] = (Address) MachVectoredInterrupt;
	    /*
	     * Set arg to vme vector address for this trap level.
	     * In the high bits of the address, we want MACH_VME_INTR_VECTOR.
	     * In bits 3 to 1 we want the VME bus level.  We want bit 0 to be
	     * 1.  So, for example, for interrupt level 13, we want to put
	     * VME level7 into bits 3 to 1 and then set bit 0 high.  But this
	     * is equivalent to putting the number 14 into bits 3 to 0 and then
	     * setting bit 0 high.  So this means adding 1 to the interrupt
	     * level and then setting bit 0 high.
	     */
	    machInterruptArgs[i] = (ClientData)
		    (MACH_VME_INTR_VECTOR | ((i + 1) | 1));
	} else { 
#endif
	    machVectorTable[i] = (Address) MachHandleDebugTrap;
	    machInterruptArgs[i] = (ClientData) 0;
#ifndef sun4c
	}
#endif
    }

    /* Temporary: for debugging net module and debugger: */
    mach_NumDisableInterrupts[0] = 1;

    /*
     * Copy the boot parameter structure. The original location will get
     * unmapped during vm initialization so we need to get our own copy.
     */
    machMonBootParam = **(romVectorPtr->bootParam);
    offset = (unsigned int) *(romVectorPtr->bootParam) - 
	     (unsigned int) &(machMonBootParam);
    for (i = 0; i < 8; i++) {
	if (machMonBootParam.argPtr[i] != (char *) 0 &&
	 machMonBootParam.argPtr[i] != (char *) NIL) {
	    machMonBootParam.argPtr[i] -= offset;
	}
    }
#ifndef sun4c
    /*
     * Clear out the line input buffer to the prom so we don't get extra
     * characters at the end of shorter reboot strings.
     */
    bzero(romVectorPtr->lineBuf, *romVectorPtr->lineSize);
#endif

#ifdef sun4c
    /*
     * This gets turned on by the profiler init when it is called.
     */
    *Mach_InterruptReg &= ~MACH_ENABLE_LEVEL14_INTR;
#endif
    

    return;
}


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
    procPtr->machStatePtr = (Mach_State *)Vm_RawAlloc(sizeof(Mach_State));
    bzero((char *)procPtr->machStatePtr, sizeof (Mach_State));
    procPtr->machStatePtr->kernStackStart = mach_StackBottom;
    procPtr->machStatePtr->trapRegs = (Mach_RegState *) NIL;
    procPtr->machStatePtr->switchRegs = (Mach_RegState *) NIL;
    machCurStatePtr = procPtr->machStatePtr;
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
    register	Mach_RegState	*stackPtr;
    register	Mach_State	*statePtr;

    /*
     * If it's a user process forking, we must make sure all its windows have
     * been saved to the stack so that when the register state and the stack
     * are copied to the new process, it will get the real stuff.
     */
    if (user) {
	Mach_DisableIntr();
	MachFlushWindowsToStack();
	Mach_EnableIntr();
    }
    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = (Mach_State *)Vm_RawAlloc(sizeof(Mach_State));
    }
    bzero((char *) procPtr->machStatePtr, sizeof (Mach_State));
    statePtr = procPtr->machStatePtr;
    statePtr->trapRegs = (Mach_RegState *)NIL;
    /* 
     * Allocate a kernel stack for this process.
     */
    statePtr->kernStackStart = (Address) Vm_GetKernelStack(0);
    if (statePtr->kernStackStart == (Address)NIL) {
	return(PROC_NO_STACKS);
    }

    /*
     * Pointer to context switch register's save area is also a pointer
     * to the top of the stack, since the regs are saved there.
     * If this is a kernel process, we only need space MACH_SAVED_STATE_FRAME
     * + MACH_FULL_STACK_FRAME, but if it's a user process we need more:
     * (2 * MACH_SAVED_STATE_FRAME).  Both types of processes need the
     * first MACH_SAVED_STATE_FRAME for their context switch regs area.  A
     * kernel process then only needs space under that on its stack for its
     * first routine to store its arguments in its "caller's" stack frame, so
     * this extra space just fakes a caller's stack frame.  But for a user
     * process, we have trap regs.  And these trapRegs get stored under
     * the context switch regs on the kernel stack.
     */
    ((Address) statePtr->switchRegs) = (statePtr->kernStackStart) +
	    MACH_KERN_STACK_SIZE - (2 * MACH_SAVED_STATE_FRAME);
    (unsigned int) (statePtr->switchRegs) &= ~0x7;/* should be okay already */
    /*
     * Initialize the stack so that it looks like it is in the middle of
     * Mach_ContextSwitch.
     */
    stackPtr = statePtr->switchRegs;	/* stack pointer is set from this. */
    /*
     * Fp is set to saved window area for window we'll return to.  The area for
     * the window of Mach_ContextSwitch is a Mach_RegState.  Below this on
     * the stack (at higher address than) is the saved window area of the
     * routine we'll return to from Mach_ContextSwitch.  So the fp must be
     * set to the top of this saved window area.
     */
    *((Address *)(((Address)stackPtr) + MACH_FP_OFFSET)) =
	    ((Address)stackPtr) + MACH_SAVED_STATE_FRAME;
    /*
     * We are to return to startFunc from Mach_ContextSwitch, but
     * Mach_ContextSwitch will do a return to retPC + 8, so we subtract
     * 8 from it here to get to the right place.
     */
    *((Address *)(((Address)stackPtr) + MACH_RETPC_OFFSET)) =
	    ((Address)startFunc) - 8;

    /*
     * Set the psr to restore to have traps enabled and interrupts off.
     */
    stackPtr->curPsr = MACH_HIGH_PRIO_PSR;
    /* 
     * Set up the state of the process.  User processes inherit from their
     * parent or the migrated process.  If the PC is not specified, take it
     * from the parent as well.
     */
    if (user) {
	Proc_ControlBlock	*curProcPtr;	/* Pointer to current proc */

	/*
	 * Trap state regs are the same for child process.
	 */
	(Address) (statePtr->trapRegs) =
		((Address) stackPtr) + MACH_SAVED_STATE_FRAME;
	bcopy((Address)fromStatePtr->trapRegs, (Address)statePtr->trapRegs,
		sizeof (Mach_RegState));
	/*
	 * Check to see if any register windows were saved to internal buffer
	 * in MachFlushWindowsToStack(), above.  If so, copy the buffer state
	 * to the new process, so that when it returns from the fork trap, it
	 * will copy out the saved windows to its stack.
	 */
	curProcPtr = Proc_GetCurrentProc();
	if (curProcPtr->machStatePtr != fromStatePtr) {
	    panic("%s %s\n", "Mach_SetupNewState: Forking process must",
		    "be current process!");
	}
	if (fromStatePtr->savedMask != 0) {
	    procPtr->specialHandling = 1;
	    statePtr->savedMask = fromStatePtr->savedMask;
	    bcopy((Address) fromStatePtr->savedRegs,
		    (Address) statePtr->savedRegs,
		    sizeof (fromStatePtr->savedRegs));
	    bcopy((Address) fromStatePtr->savedSps,
		    (Address) statePtr->savedSps,
		    sizeof (fromStatePtr->savedSps));
	}
    }
    if (startPC == (Address)NIL) {
	*((Address *)(((Address)stackPtr) + MACH_ARG0_OFFSET)) =
		(Address) fromStatePtr->trapRegs->pc;
    } else {
	/*
	 * The first argument to startFunc is supposed to be startPC.  But that
	 * would be in an in register in startFunc's window which is one before
	 * Mach_ContextSwitch's window.  But startFunc will do a save operation
	 * at the beginning so it will actually be executing in Mach_ContextS's
	 * window, so arg0 to startFunc must actually be arg0 that is restored
	 * at the end of Mach_ContextSwitch, so we have to put it in the in
	 * register area of Mach_RegState area on the stack.  Weird.
	 */
	*((Address *)(((Address)stackPtr) + MACH_ARG0_OFFSET)) = startPC;
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_SetReturnVal --
 *
 *	Set the return value for a process from a system call.  Intended to
 *	be called by the routine that starts a user process after a fork.
 *	Interrupts must be off here!
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
    if (procPtr->machStatePtr->trapRegs == (Mach_RegState *) NIL ||
	    procPtr->machStatePtr->trapRegs == (Mach_RegState *) 0) {
	return;
    }
    procPtr->machStatePtr->trapRegs->ins[0] = retVal;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_StartUserProc --
 *
 *	Start a user process executing for the first time.
 *	Interrupts must be off here!
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
    /*
     * MachRunUserProc will put the values from our trap regs into the actual
     * registers so that we'll be in shape to rett back to user mode.
     */
    Mach_DisableIntr();

    /*
     * Return from trap pc.
     */
    (Address) statePtr->trapRegs->pc = (Address)entryPoint;
    (Address) statePtr->trapRegs->nextPc = (Address)entryPoint + 4;

    MachRunUserProc();
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
    Address		userStackPtr;		/* Stack pointer for when the
						 * user process resumes 
						 * execution. */
    Address		entryPoint;		/* Where the user process is
						 * to resume execution. */
{
    Mach_RegState	tmpTrapState;

    /*
     * Ugh.  They set this to a different register set in the sun3 and
     * then set this reg differently in Mach_StartUserProc.  What should I
     * be doing?
     */
    /*
     * EEK -- Make sure there's enough space here for thing to have
     * stored its 6 input reg args in a caller's stack frame.
     */
     /*
      * We do not call DISABLE_INTR here because there's an implicit enable
      * of interrupts in MachRunUserProc().
      */
    Mach_DisableIntr();
    machCurStatePtr = procPtr->machStatePtr;
    /*
     * Since we're not returning, we can just use this space on our kernel
     * stack as trapRegs.  This is safe, since we only fill in the fp, tbr,
     * pc, and nextPc fields (in Mach_StartUserProc()) and these just touch
     * the saved-window section of our stack and won't mess up any of our
     * arguments.
     */
    procPtr->machStatePtr->trapRegs = &tmpTrapState;
    /*
     * The user stack pointer gets MACH_FULL_STACK_FRAME subtracted from it
     * so that the user stack has space for its first routine to store its
     * arguments in its caller's stack frame.  (So we create a fake caller's
     * stack frame this way.)
     */
    (Address) procPtr->machStatePtr->trapRegs->ins[MACH_FP_REG] =
	    userStackPtr - MACH_FULL_STACK_FRAME;
    procPtr->machStatePtr->trapRegs->curPsr = MACH_FIRST_USER_PSR;
    (Address) procPtr->machStatePtr->trapRegs->pc = entryPoint;
    (Address) procPtr->machStatePtr->trapRegs->tbr = machTBRAddr;
    /*
     * Return value is cleared for exec'ing user process.  This shouldn't
     * matter since a good exec won't return.
     */
    (Address) procPtr->machStatePtr->trapRegs->ins[0] = 0;
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
    bcopy((Address)machStatePtr->trapRegs,
	      (Address)(&debugStatePtr->regState), sizeof(Mach_RegState));
    return;
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

/* y, pc's g1-g7 all in's*/
    machStatePtr = procPtr->machStatePtr;
    machStatePtr->trapRegs->pc = debugStatePtr->regState.pc;
    machStatePtr->trapRegs->nextPc = debugStatePtr->regState.nextPc;
    machStatePtr->trapRegs->y = debugStatePtr->regState.y;
    bcopy((Address)debugStatePtr->regState.ins,
	    (Address)machStatePtr->trapRegs->ins, MACH_NUM_INS * sizeof (int));
    bcopy((Address)debugStatePtr->regState.globals,
	    (Address)machStatePtr->trapRegs->globals,
	    MACH_NUM_GLOBALS * sizeof (int));
    return;
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
	panic("out-of-order kernel call initialization");
    }
    if (machMaxSysCall >= SYS_NUM_SYSCALLS) {
	panic("too many kernel calls");
    }
    if (numArgs > SYS_MAX_ARGS) {
	panic("too many arguments to kernel call");
    }
    /*
     * Offset of beginning of args on stack - offset from frame pointer.
     * Figure out offset  from fp of params past the sixth.
     * It copies from last arg to first
     * arg in that order, so we start offset at bottom of last arg.
     */
    /*
     * TURN THESE INTO PROPER CONSTANTS!
     */
    /*
     * We copy going towards higher addresses, rather than lower, as the sun3
     * does it.  So our offset is at top of extra parameters to copy, and not
     * below them (stack-wise speaking, not address-wise speaking).
     */
    machArgOffsets[machMaxSysCall] = MACH_SAVED_WINDOW_SIZE +
	    MACH_ACTUAL_HIDDEN_AND_INPUT_STORAGE;
	
    /*
     * Where to jump to in fetching routine to copy the right amount from
     * the stack.  The fewer args we have, the farther we jump...  Figure out
     * how many are in registers, then do rest from stack.  There's instructions
     * to copy 10 words worth, for now, since 6 words worth of arguments are
     * in the input registers.  If this number changes, change machTrap.s
     * and the jump offset below!
     */
    if (numArgs <= 6) {
	machArgDispatch[machMaxSysCall] =  (Address) MachFetchArgsEnd;
    } else {
	machArgDispatch[machMaxSysCall] = (10 - (numArgs - 6))*16 +
		((Address)MachFetchArgs);
    }
    mach_NormalHandlers[machMaxSysCall] = normalHandler;
    mach_MigratedHandlers[machMaxSysCall] = migratedHandler;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_SetHandler --
 *
 *	Put a device driver interrupt handling routine into the autovector.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The exception vector table is modified.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_SetHandler(vectorNumber, handler, clientData)
    int vectorNumber;		/* Vector number that the device generates */
    int (*handler)();		/* Interrupt handling procedure */
    ClientData	clientData;	/* ClientData for interrupt callback routine. */
{

    if (vectorNumber < 0 || vectorNumber > 255) {
	panic("Warning: Bad vector number %d\n", vectorNumber);
    } else {
	machVectorTable[vectorNumber] = (Address) handler;
	machInterruptArgs[vectorNumber] = (ClientData) clientData;
    }
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
 * setting up the user stack properly.  The top entry on the stack is the
 * return address where the handler will start executing upon return.  But 
 * this is just the address of a trap instruction that is stored on the stack
 * below.  Thus when a handler returns it will execute a trap instruction 
 * and drop back into the kernel. 
 */


/*
 * ----------------------------------------------------------------------------
 *
 * MachCallSigReturn --
 *
 *      Process a return from a signal handler.  Call the Sig_Return
 *	routine with appropriate args.
 *	
 * Results:
 *      None.
 *
 * Side effects:
 *	Whatever Sig_Return does.
 *
 * ----------------------------------------------------------------------------
 */
void
MachCallSigReturn()
{
    Proc_ControlBlock	*procPtr;
    Mach_State		*statePtr;
    Sig_Stack		*sigStackPtr;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;

    sigStackPtr = &(statePtr->sigStack);
    sigStackPtr->contextPtr = &(statePtr->sigContext);

    /*
     * Take the proper action on return from a signal.
     */
    Sig_Return(procPtr, sigStackPtr);
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
 *	Return the machine architecture (SYS_SUN2 or SYS_SUN3).
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
#       ifdef sun2
	return SYS_SUN2;
#       endif sun2

#       ifdef sun3
	return SYS_SUN3;
#       endif sun3

#	ifdef sun4
	return SYS_SUN4;
#	endif sun4
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
 * MachIntrNotHandledYet() -
 *
 *	I don't handle this level of interrupt yet.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
void
MachIntrNotHandledYet()
{
    printf("Received an interrupt I don't handle yet.\n");
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * MachPageFault() -
 *
 *	Handle a page fault.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	A page causing a memory access error is made valid.  If it's an
 *	illegal page fault in the kernel, we will call panic.
 *
 *----------------------------------------------------------------------
 */
void
MachPageFault(busErrorReg, addrErrorReg, trapPsr, pcValue)
#ifdef sun4c
    unsigned	int	busErrorReg;
#else
    unsigned	char	busErrorReg;
#endif
    Address		addrErrorReg;
    unsigned	int	trapPsr;
    Address		pcValue;
{
    Proc_ControlBlock	*procPtr;
    Boolean		protError;
    Boolean		copyInProgress = FALSE;
    ReturnStatus	status;

    /*
     * Are we poking at or peeking into memory-mapped devices?
     * We must check this before looking for the current process, since this
     * can happen during boot-time before we have set up processes.
     */
    if ((pcValue >= (Address) &Mach_ProbeStart)  &&
	    (pcValue < (Address) &Mach_ProbeEnd)) {
	/*
	 * This doesn't return to here.  It erases the fact that the
	 * page fault happened and makes the probe routine that
	 * got the page fault return FAILURE to its caller.  We must turn off
	 * interrupts before calling MachHandleBadProbe().
	 */
	Mach_DisableIntr();
	MachHandleBadProbe();
	Mach_EnableIntr();
    }
    procPtr = Proc_GetActualProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic(
	"MachPageFault: Current process is NIL!!  Trap pc is 0x%x, addr 0x%x\n",
		(unsigned) pcValue, addrErrorReg);
    }
    /* process kernel page fault */
    if (trapPsr & MACH_PS_BIT) {		/* kernel mode before trap */
	if (!(procPtr->genFlags & PROC_USER)) {
	    /*
	     * This fault happened inside the kernel and it wasn't on behalf
	     * of a user process.  This is an error.
	     */
	    panic(
"MachPageFault: page fault in kernel process! pc:0x%x, addr:0x%x, Error:0x%x\n",
		    pcValue, addrErrorReg, busErrorReg);
	}
	/*
	 * A page fault on a user process while executing in
	 * the kernel.  This can happen when information is
	 * being copied back and forth between kernel and user state
	 * (indicated by particular values of the program counter).
	 */
	if ((pcValue >= (Address) Vm_CopyIn) &&
		(pcValue < (Address) VmMachCopyEnd)) {
	    copyInProgress = TRUE;
	} else if ((pcValue >= (Address) MachFetchArgs) &&
		(pcValue <= (Address) MachFetchArgsEnd)) {
	    copyInProgress = TRUE;
	} else if (procPtr->vmPtr->numMakeAcc == 0) {
	    /*
	     * ERROR: pc faulted in a bad place!
	     */
	    panic(
	    "MachPageFault: kernel page fault at illegal pc: 0x%x, addr 0x%x\n",
		    pcValue, addrErrorReg);
	}
	protError = (busErrorReg & MACH_PROT_ERROR);
	/*
	 * Try to fault in the page.
	 *
	 * Since this is called from trap handlers only, we know interrupts
	 * are off here.  The Vm system needs them on?
	 */
	status = Vm_PageIn(addrErrorReg, protError);
	if (status != SUCCESS) {
	    if (copyInProgress) {
		/*
		 * This doesn't return to here.  It erases the fact that the
		 * page fault happened and makes the copy routine that
		 * got the page fault return SYS_ARG_NO_ACCESS to its caller.
		 * We must turn off interrupts before calling
		 * MachHandleBadArgs().
		 */
		Mach_DisableIntr();
		MachHandleBadArgs();
		Mach_EnableIntr();
	    } else {
		/* kernel error */
		panic(
	    "MachPageFault: couldn't page in kernel page at 0x%x, pc 0x%x\n",
			addrErrorReg, pcValue);
	    }
	}
	return;
    }
    /* user page fault */
    protError = busErrorReg & MACH_PROT_ERROR;
    if (Vm_PageIn(addrErrorReg, protError) != SUCCESS) {
	printf(
    "MachPageFault: Bus error in user proc %x, PC = %x, addr = %x BR Reg %x\n",
#ifdef sun4c
		procPtr->processID, pcValue, addrErrorReg, busErrorReg);
#else
		procPtr->processID, pcValue, addrErrorReg, (short) busErrorReg);
#endif
	/* Kill user process */
	Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, procPtr->processID, FALSE);
	return;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * MachUserAction() -
 *
 *	Check what sort of user-process action is needed.  We already know
 *	that some sort of action is needed, since the specialHandling flag
 *	should be checked before calling this routine.  The possible actions
 *	are to take a context switch, to push saved user windows from the mach
 *	state structure out to the user stack, or to set things up to handle
 *	pending signals.  We assume traps are enabled before this routine is
 *	called.
 *
 * Results:
 *	The return value TRUE indicates we have a pending signal.  FALSE
 *	indicates that we do not.
 *
 * Side effects:
 *	The mach state structure may change, especially the mask that indicates
 *	which windows were saved into the internal buffer.
 *
 *----------------------------------------------------------------------
 */
int
MachUserAction()
{
    Proc_ControlBlock	*procPtr;
    Mach_State		*machStatePtr;
    Sig_Stack		*sigStackPtr;
    Address		pc;

    procPtr = Proc_GetCurrentProc();
HandleItAgain:
    procPtr->specialHandling = 0;
    /*
     * Take a context switch if one is pending for this process.
     * If other stuff, such as flushing the windows to the stack needs to
     * be done, it will happen when the process is switched back in again.
     * We came from MachReturnFromTrap, where interrupts were off, so we
     * must turn them on.
     */
    Mach_EnableIntr();
    if (procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING) {
	Sched_LockAndSwitch();
    }

    machStatePtr = procPtr->machStatePtr;

    /*
     * Save the windows that were stored in internal buffers to the user stack.
     * The windows were saved to internal buffers due to the user stack not
     * being resident.  The overflow handler can't take page faults, but
     * we can.
     */
    if (machStatePtr->savedMask != 0) {
	int	i;

	for (i = 0; i < MACH_NUM_WINDOWS; i++) {
	    if ((1 << i) & machStatePtr->savedMask) {
		/*
		 * Clear the mask for this window.  We must turn off interrupts
		 * to do this, since changing the savedMask must be an
		 * atomic operation.  If it weren't, and an interrupt came
		 * in that caused us to save some other window to the stack
		 * after we have read the savedMask, we would overwrite the
		 * fact when storing the saved Mask...
		 */
		Mach_DisableIntr();
		machStatePtr->savedMask &= ~(1 << i);
		Mach_EnableIntr();
		/*
		 * Push the window to the stack.
		 */
		if (Vm_CopyOut(MACH_SAVED_WINDOW_SIZE,
			(Address)&(machStatePtr->savedRegs[i]),
			machStatePtr->savedSps[i]) != SUCCESS) {
		    printf("MachUserAction: pid 0x%x being killed: %s 0x%x.\n",
			    procPtr->processID, "bad stack pointer?",
			    machStatePtr->savedSps[i]);
		    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
		}
	    }
	}
    }
    /*
     * We must check again here to see if the specialHandling flag has been
     * set again.  We've been taking interrupts this whole time, and the
     * Vm code above may have called deeply, so we may have saved further
     * user windows to internal buffers.  If we have, go back up to the
     * beginning of the routine, and do this all again.
     */

    /*
     * For now, we also flush all the windows to make sure the signal-handling
     * stuff below won't cause us to save windows to internal buffers.  This
     * is a slow thing to do, but it is currenlty unclear what to do if
     * the signal handler causes a window to get saved to an internal buffer.
     * It will have to call some sort of MachUserAction-type routine itself
     * in that case.
     */
    Mach_DisableIntr();
    MachFlushWindowsToStack();
    if (procPtr->specialHandling != 0) {
	goto HandleItAgain;
    }
    Mach_EnableIntr();

    /*
     * Now check for signal stuff.
     */
    sigStackPtr = &(machStatePtr->sigStack);
    sigStackPtr->contextPtr = &(machStatePtr->sigContext);
    if (Sig_Handle(procPtr, sigStackPtr, &pc)) {
	machStatePtr->sigContext.machContext.pcValue = pc;
	machStatePtr->sigContext.machContext.trapInst = MACH_SIG_TRAP_INSTR;
	/* leave interrupts disabled */
	Mach_DisableIntr();
	return TRUE;
    }
    /*
     * Go back to MachReturnFromTrap.  We are expected to have interrupts
     * off there.
     */
    Mach_DisableIntr();
	
    return FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * MachHandleWeirdoInstruction --
 *
 *	Handle an instruction trap, such as an illegal instruction trap,
 *	an unaligned address, or a floating point problem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If it occured in the kernel, we panic.  If it occured in a user
 *	program, we take appropriate signal action.
 *
 *----------------------------------------------------------------------
 */
void
MachHandleWeirdoInstruction(trapType, pcValue, trapPsr)
    int			trapType;
    Address		pcValue;
    unsigned	int	trapPsr;
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	printf("%s: pc = 0x%x, trapType = %d\n",
		"MachHandleWeirdoInstruction", pcValue, trapType);
	panic("Current process was NIL!\n");
    }
    /*
     * Handle kernel-mode traps.
     */
    if (trapPsr & MACH_PS_BIT) {
	switch (trapType) {
	case MACH_ILLEGAL_INSTR:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: illegal",
		    "instruction trap in the kernel!");
	    break;
	case MACH_PRIV_INSTR:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: privileged",
		    "instruction trap in the kernel!");
	    break;
	case MACH_MEM_ADDR_ALIGN:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: unaligned",
		    "address trap in the kernel!");
	    break;
	case MACH_TAG_OVERFLOW:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: tag",
		    "overflow trap in the kernel!");
	    break;
	case MACH_FP_EXCEP:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: fp",
		    "exception trap in the kernel!");
#ifdef FP_ENABLED
	    printf("%fsr is 0x%x\n", machSavedFsr);
#endif FP_ENABLED
	    break;
	case MACH_FP_DISABLED:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: fp unit",
		    "disabled trap in the kernel!");
	    break;
	default:
	    printf("%s %s\n", "MachHandleWeirdoInstruction: hit default",
		    "in case statement - bad trap instruction called us!");
	    break;
	}
	panic("%s %s %s %x %s %x\n",
		"MachHandleWeirdoInstruction: the error occured in a",
		procPtr->genFlags & PROC_USER ? "user" : "kernel",
		"process, with procPtr =", (unsigned int) procPtr,
		"and pc =", pcValue);
    }
    /*
     * The trap occured in user-mode.
     */
    switch (trapType) {
    case MACH_ILLEGAL_INSTR:
	(void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE, procPtr->processID,
		FALSE);
	break;
    case MACH_PRIV_INSTR:
	(void) Sig_Send(SIG_ILL_INST, SIG_PRIV_INST, procPtr->processID, FALSE);
	break;
    case MACH_MEM_ADDR_ALIGN:
	(void) Sig_Send(SIG_ADDR_FAULT, SIG_ADDR_ERROR, procPtr->processID,
		FALSE);
	break;
    case MACH_FP_EXCEP:
#ifdef FP_ENABLED
	printf("%s.  %fsr was 0x%x\n",
		"MachHandleWeirdoInstruction: fp exception from user process.",
		machSavedFsr);
#endif FP_ENABLED
    (void) Sig_Send(SIG_ARITH_FAULT, SIG_ILL_INST_CODE, procPtr->processID,
		FALSE);
	break;
    case MACH_FP_DISABLED: {
	Mach_State 	*machStatePtr;
	int		curSp;
	Mach_RegWindow	*curWindow;

	/* 
	 * Insure the user's registers are memory resident.
	 */
	MachFlushWindowsToStack();
	machStatePtr = procPtr->machStatePtr;
	/*
	 * See if the user's window was saved to the internal buffers.
	 */
	curSp = machStatePtr->trapRegs->ins[MACH_FP_REG];
	curWindow = (Mach_RegWindow *) curSp;
	if (machStatePtr->savedMask != 0) {
	    int	i;
	    for (i = 0; i < MACH_NUM_WINDOWS; i++) {
		if (((1 << i) & machStatePtr->savedMask) &&
		    (machStatePtr->savedSps[i] == curSp))  {
		    curWindow = (Mach_RegWindow *)
				    &(machStatePtr->savedRegs[i][0]);
		}
	    }
	}
	MachFPU_Emulate(procPtr->processID,pcValue, machStatePtr->trapRegs, 
			curWindow,  &machStatePtr->fpuState);
	break;
	}
    case MACH_TAG_OVERFLOW:
	panic("%s %s\n", "MachHandleWeirdoInstruction: tag",
		"overflow trap in user process, but I don't deal with it yet.");
	break;
    default:
	panic("%s %s\n", "MachHandleWeirdoInstruction: hit default",
		"in case statement - bad trap instruction from user mode.");
	break;

    }
    return;
}

#ifdef sun4c
void
Mach_PrintInterruptReg()
{
    unsigned char	interReg;

    interReg = *Mach_InterruptReg;
    Mach_MonPrintf("Interrupt register: 0x%x\n", interReg);
    return;
}

void
Mach_PrintBusErrorRegs()
{
    unsigned int	busErrorReg;

    busErrorReg = MachGetSyncErrorReg();
    Mach_MonPrintf("Sync error register: 0x%x\n", busErrorReg);
    busErrorReg = MachGetSyncErrorAddrReg();
    Mach_MonPrintf("Sync error addr register: 0x%x\n", busErrorReg);
    busErrorReg = MachGetASyncErrorReg();
    Mach_MonPrintf("ASync error register: 0x%x\n", busErrorReg);
    busErrorReg = MachGetASyncErrorAddrReg();
    Mach_MonPrintf("ASync error addr register: 0x%x\n", busErrorReg);
    return;
}

void
Mach_PrintSystemEnableReg()
{
    unsigned char	systemEnableReg;

    systemEnableReg = MachGetSystemEnableReg();
    Mach_MonPrintf("System enable register: 0x%x\n", systemEnableReg);
    return;
}
#endif

void
MachTestContexts()
{
    int	i;
    int	j;

    for (i = 0; i < VMMACH_NUM_CONTEXTS; i++) {
	VmMachSetUserContext(i);
	j = i;
    }
    return;
}

void
FlushTheWindows(num)
int	num; 
{
    num--;
    if (num > 0) {
	FlushTheWindows(num);
    }
    return;
}

/*
 * We want to do NWINDOWS - 1 saves and then restores to make sure all our
 * register windows have been saved to the stack.  Calling here does one save,
 * so we want to do NWINDOWS - 2 more calls and returns.
 */
void
MachFlushWindowsToStack()
{
    FlushTheWindows(MACH_NUM_WINDOWS - 2);
    return;
}

void
MachUserDebug()
{
    Proc_ControlBlock	*procPtr; 

    procPtr = Proc_GetCurrentProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("MachUserDebug: current process was NIL!\n");
    }
    Sig_Send(SIG_BREAKPOINT, SIG_NO_CODE, procPtr->processID, FALSE);

    return;
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

int
Mach_GetBootArgs(argc, bufferSize, argv, buffer)
	int	argc;			/* Number of elements in argv */
	int	bufferSize;		/* Size of buffer */
	char	**argv;			/* Ptr to array of arg pointers */
	char	*buffer;		/* Storage for arguments */
{
    int		i;
    int		offset;

    bcopy(machMonBootParam.strings, buffer, 
	  (bufferSize < 100) ? bufferSize : 100);
    offset = (unsigned int) machMonBootParam.strings - (unsigned int) buffer;
    for(i = 0; i < argc; i++) {
	if (machMonBootParam.argPtr[i] == (char *) 0 ||
	    machMonBootParam.argPtr[i] == (char *) NIL) {
	    break;
	}
	argv[i] = (char *) (machMonBootParam.argPtr[i] - (char *) offset);
    }
    return i;
}

