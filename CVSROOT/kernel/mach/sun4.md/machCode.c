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
 * The byte ordering/alignment type used with Swap_Buffer and I/O control data.
 */
int	mach_ByteOrder = SWAP_SPARC_TYPE;

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

int	machMaxSysCall;			/* Hightest defined system call. */
int	machArgOffsets[SYS_NUM_SYSCALLS];	/* For each system call, tells how
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
char	mach_DebugStack[0x2000];	/* The debugger stack. */

/*
 * Pointer to the state structure for the current process.
 */
Mach_State	*machCurStatePtr = (Mach_State *)NIL;

/*
 * For testing correctness of defined offsets.
 */
Mach_RegState		testMachRegState;
Mach_State		testMachState;
Proc_ControlBlock	testPCB;
int		debugCounter = 0;		/* for debugging */
int		debugSpace[500];
Address		theAddrOfVmPtr = 0; 
Address		theAddrOfMachPtr = 0;
Address		oldAddrOfVmPtr = 0; 
Address		oldAddrOfMachPtr = 0;

/*
 * For debugging stuff, put values into a circular buffer.  After each value,
 * stamp a special mark, which gets overwritten by next value, so we
 * always know where the end of the list is.
 */
#define	DEBUG_ADD(thing)	\
    debugSpace[debugCounter++] = (int)(thing);	\
    if (debugCounter >= 500) {	\
	debugCounter = 0;	\
    }				\
    debugSpace[debugCounter] = (int)(0x11100111);


    


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

    /* Temporary: for debugging net module and debugger: */
    mach_NumDisableInterrupts[0] = 1;

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
#ifdef NOTDEF
    procPtr->machStatePtr->setJumpStatePtr = (Mach_SetJumpState *)NIL;
    /* also set stack end? */
#endif NOTDEF
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

    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = (Mach_State *)Vm_RawAlloc(sizeof(Mach_State));
#ifdef NOTDEF
	procPtr->machStatePtr->setJumpStatePtr = (Mach_SetJumpState *)NIL;
#endif NOTDEF
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
     */
    ((Address) statePtr->switchRegs) = (statePtr->kernStackStart) +
	    MACH_KERN_STACK_SIZE - MACH_FULL_STACK_FRAME -
	    MACH_SAVED_STATE_FRAME;
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
	/*
	 * Trap state regs are the same for child process.
	 */
	bcopy((Address)fromStatePtr->trapRegs, (Address)statePtr->trapRegs,
		sizeof (Mach_RegState));
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
    int		status;

    statePtr = procPtr->machStatePtr;
    /*
     * Fake set of trap regs on top of the stack and call MachReturnFromTrap
     * from MachRunUserProc?  Or set them specially in MachRunUserProc and
     * just call rett?  I think I'll assume the latter.  That way I just
     * put the values I want in the state register and store them directly
     * into real registers in MachRunUserProc().
     */

    /*
     * Return from trap pc.
     */
    (Address) statePtr->trapRegs->pc = (Address)entryPoint;
    (Address) statePtr->trapRegs->nextPc = (Address)entryPoint + 4;	/* correct? */
#ifdef NOTDEF
    MachUserReturn(procPtr);
#endif NOTDEF

    Mach_MonPrintf("Mach_StartUserProc, entryPoint is 0x%x\n", entryPoint);
    Mach_MonPrintf("fp arg is 0x%x\n", procPtr->machStatePtr->trapRegs->ins[MACH_FP_REG]);
    Mach_MonPrintf("Mach_StartUserProc calling MachRunUserProc\n");
#ifdef NOTDEF
    MachRunUserProc();
#else
    MachRunUserProc(entryPoint, procPtr->machStatePtr->trapRegs->ins[MACH_FP_REG]);
#endif NOTDEF
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
    Mach_MonPrintf("Mach_ExecUserProc called with userStackPtr 0x%x\n", userStackPtr);
    if (procPtr->machStatePtr->trapRegs != (Mach_RegState *) NIL) {
	panic("Mach_ExecUserProc: machStatePtr->trapRegs was NOT NIL!\n");
    }
    procPtr->machStatePtr->trapRegs = &tmpTrapState;
    Mach_MonPrintf("Current sp, new trapRegs is 0x%x\n", procPtr->machStatePtr->trapRegs);
    /*
     * Since we're not returning, we can just user the space on our kernel
     * stack as trapRegs.  This is safe, since we only fill in the fp, pc, and
     * nextPc fields (in Mach_StartUserProc()) and these just touch the saved-
     * window section of our stack and won't mess up any of our arguments.
     */
    (Address) procPtr->machStatePtr->trapRegs->ins[MACH_FP_REG] = userStackPtr;
    Mach_MonPrintf("fp stored at 0x%x\n", &(procPtr->machStatePtr->trapRegs->ins[MACH_FP_REG]));
    Mach_MonPrintf("procPtr is 0x%x\n", procPtr);
    Mach_MonPrintf("procPtr->machStatePtr is 0x%x\n", procPtr->machStatePtr);
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
#ifdef NOTDEF
    register	Mach_State	*destStatePtr;

    destStatePtr = destProcPtr->machStatePtr;
    destStatePtr->userState.userStackPtr =
				statePtr->userState.userStackPtr;
    bcopy((Address)statePtr->userState.trapRegs,
	      (Address)destStatePtr->userState.trapRegs,
	      sizeof(int) * (MACH_NUM_GPRS - 1));
    destStatePtr->userState.excStackPtr->pc = 
				    statePtr->userState.excStackPtr->pc;
    destStatePtr->userState.excStackPtr->statusReg = 
				statePtr->userState.excStackPtr->statusReg;
#else
    panic("Mach_CopyState called.\n");
#endif NOTDEF
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
#ifdef NOTDEF
    register	Mach_State	*machStatePtr;

    machStatePtr = procPtr->machStatePtr;
    bcopy((Address)machStatePtr->userState.trapRegs,
	      (Address)debugStatePtr->regState.regs,
	      sizeof(machStatePtr->userState.trapRegs));
    debugStatePtr->regState.regs[SP] = 
			(int)machStatePtr->userState.userStackPtr;
    debugStatePtr->regState.pc = machStatePtr->userState.excStackPtr->pc;
    debugStatePtr->regState.statusReg  =
			machStatePtr->userState.excStackPtr->statusReg;
#else
    panic("Mach_GetDebugState called.\n");
#endif NOTDEF
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
#ifdef NOTDEF
    register	Mach_State	*machStatePtr;

    machStatePtr = procPtr->machStatePtr;
    bcopy((Address)debugStatePtr->regState.regs,
	      (Address)machStatePtr->userState.trapRegs,
	      sizeof(machStatePtr->userState.trapRegs) - sizeof(int));
    machStatePtr->userState.userStackPtr = 
				(Address)debugStatePtr->regState.regs[SP];
    machStatePtr->userState.excStackPtr->pc = 
				debugStatePtr->regState.pc;
    if (!(debugStatePtr->regState.statusReg & MACH_SR_SUPSTATE)) {
	machStatePtr->userState.excStackPtr->statusReg = 
				debugStatePtr->regState.statusReg;
    }
#else
    panic("Mach_SetDebugState called.\n");
#endif NOTDEF
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
#ifdef NOTDEF
    return(procPtr->machStatePtr->userState.userStackPtr);
#else
    panic("Mach_GetUserStackPtr called.\n");
#endif NOTDEF
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
	machArgDispatch[machMaxSysCall] = (10 - numArgs - 6)*16 +
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
Mach_SetHandler(vectorNumber, handler)
    int vectorNumber;	/* Vector number that the device generates */
    int (*handler)();	/* Interrupt handling procedure */
{
#ifdef NOTDEF
    int	*vecTablePtr;

    if (vectorNumber < 64 || vectorNumber > 255) {
	printf("%d: ", vectorNumber);
	printf("Warning: Bad vector number\n");
    } else {
	vecTablePtr = (int *) MachGetVBR();
	vecTablePtr[vectorNumber] = (int)handler;
    }
#else
    panic("Mach_SetHandler called\n");
#endif NOTDEF
}

#ifdef NOTDEF

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
void
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

    /*
     * Check for signals.  Interrupts are disabled because we have to 
     * make sure that we don't miss a signal.  Interrupts will be reenabled
     * automatically upon the rte.  
     */
    while (TRUE) {
	/*
	 * Disable interrupts.  Note that we don't use the DISABLE_INTR macro
	 * because it increments the nesting depth of interrupts which we don't
	 * want because there is an implicit enable interrupts on rte.
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
    
    if ((procPtr->genFlags & PROC_SINGLE_STEP_FLAG) ||
	(procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING)) {
	/*
	 * Set the trace trap bit if we are supposed to single-step this
	 * process or a context switch is pending.  We check for a context
	 * switch pending here even though we just checked above just in
	 * case we got preempted while dealing with signals.
	 */
	procPtr->machStatePtr->userState.excStackPtr->statusReg |= 
							MACH_SR_TRACEMODE;
    }

    /*
     * It is possible for Sig_Handle to mask the migration signal
     * if a process is not in a state where it can be migrated.
     * As soon as we return to user mode, though, we will allow migration.
     */
    Sig_AllowMigration(procPtr);

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
void
SetupSigHandler(procPtr, sigStackPtr, pc)
    register	Proc_ControlBlock	*procPtr;
    register	SignalStack		*sigStackPtr;
    Address				pc;
{
    register	Mach_State	*statePtr;
    Address			usp;
    int				excStackSize;
    Mach_ExcStack		*excStackPtr;

    statePtr = procPtr->machStatePtr;
    usp = statePtr->userState.userStackPtr - sizeof(SignalStack);
    sigStackPtr->sigStack.contextPtr =
	(Sig_Context *)(usp + (unsigned int)(&sigStackPtr->sigContext) -
			      (unsigned int)sigStackPtr);
    sigStackPtr->sigContext.machContext.trapInst = 0x4e424e42;
    sigStackPtr->retAddr =
	usp + (unsigned int)(&sigStackPtr->sigContext.machContext.trapInst) -
	      (unsigned int)sigStackPtr;
    /*
     * Copy the exception stack onto the signal stack.
     */
    excStackSize = Mach_GetExcStackSize(statePtr->userState.excStackPtr);
    bcopy((Address)statePtr->userState.excStackPtr,
	      (Address)&(sigStackPtr->sigContext.machContext.excStack),
	      excStackSize);
    /*
     * Copy the user state onto the signal stack.
     */
    bcopy((Address)&statePtr->userState,
	      (Address)&(sigStackPtr->sigContext.machContext.userState),
	      sizeof(Mach_UserState));
    /*
     * Copy the stack out to user space.
     */
    if (Vm_CopyOut(sizeof(SignalStack), (Address)sigStackPtr, 
			(Address)usp) != SUCCESS) {
        printf("Warning: HandleSig: No room on stack for signal, PID=%x.\n",
                  procPtr->processID);
        Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    /*
     * We need to make a short stack to allow the process to start executing.
     * The current exception stack is at least as big, maybe bigger than we
     * need.  Since we saved the true exception stack above, we can just
     * overwrite the current stack with a short stack and point the stack
     * pointer at it.
     */
    if (statePtr->userState.excStackPtr !=
			(Mach_ExcStack *)statePtr->userState.trapRegs[SP]) {
	panic("Mach_HandleSig: SP != excStackPtr\n");
    }
    statePtr->userState.userStackPtr = usp;
    excStackPtr = (Mach_ExcStack *) ((Address)statePtr->userState.excStackPtr + 
				     excStackSize - MACH_SHORT_SIZE);
    statePtr->userState.trapRegs[SP] = (int)excStackPtr;
    excStackPtr->statusReg = 0;
    excStackPtr->vor.stackFormat = MACH_SHORT;
    excStackPtr->pc = (int)pc;
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
void
ReturnFromSigHandler(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    register	Mach_State	*statePtr;
    int				curSize;
    int				oldSize;
    SignalStack			sigStack;

    statePtr = procPtr->machStatePtr;
    /*
     * Copy the signal stack in.
     */
    if (Vm_CopyIn(sizeof(Sig_Stack) + sizeof(Sig_Context),
		  (Address) (statePtr->userState.userStackPtr), 
		  (Address) &sigStack.sigStack) != SUCCESS) {
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
     * Restore user state.  Be careful not to clobber the stack
     * pointer.
     */
    statePtr->userState.userStackPtr = 
		    sigStack.sigContext.machContext.userState.userStackPtr;
    bcopy((Address)sigStack.sigContext.machContext.userState.trapRegs,
	      (Address)statePtr->userState.trapRegs,
	      sizeof(int) * (MACH_NUM_GPRS - 1));

    /*
     * Verify that the exception stack is OK.
     */
    curSize = Mach_GetExcStackSize(statePtr->userState.excStackPtr);
    oldSize = Mach_GetExcStackSize(&sigStack.sigContext.machContext.excStack);
    if (oldSize == -1) {
	printf("Mach_Code: Bad signal stack type.\n");
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    if (sigStack.sigContext.machContext.excStack.statusReg & MACH_SR_SUPSTATE) {
	printf("Mach_Code: User set kernel bit on signal stack\n");
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    /*
     * Copy the exception stack in.
     */
    bcopy((Address)&sigStack.sigContext.machContext.excStack,
	      (Address)&statePtr->sigExcStack, oldSize);
    statePtr->sigExcStackSize = oldSize;
    /*
     * Set the restored stack pointer to point to where the
     * old exception stack is to be restored to.
     */
    statePtr->userState.trapRegs[SP] += curSize - oldSize;
}
#endif NOTDEF


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

#ifdef NOTDEF

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_UnsetJump --
 *
 *	Clear out the pointer to the saved state from a set jump.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	setJumpStatePtr field of proc table for current process is nil'd out.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_UnsetJump()
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    procPtr->machStatePtr->setJumpStatePtr = (Mach_SetJumpState *) NIL;
}
#endif NOTDEF


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
 * MachNoLevel0Interrupt() -
 *
 *	Although there's a level 0 interrupt slot, there's supposedly no
 *	such thing as a level 0 interrupt, according to the sparc
 *	architecture manual, so complain.
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
MachNoLevel0Interrupt()
{
    printf("Received a level 0 interrupt.  There's no such thing!\n");
    return;
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
 *	A page causing a memory access error is made valid.
 *
 *----------------------------------------------------------------------
 */
int
MachPageFault(busErrorReg, addrErrorReg, trapPsr, pcValue)
    unsigned	char	busErrorReg;
    Address		addrErrorReg;
    unsigned	int	trapPsr;
    Address		pcValue;
{
    Proc_ControlBlock	*procPtr;
    Boolean		protError;
    Boolean		copyInProgress = FALSE;
    ReturnStatus	status;

    procPtr = Proc_GetActualProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("MachPageFault: Current process is NIL!!  Trap pc is 0x%x, addr 0x%x\n",
		(unsigned) pcValue, addrErrorReg);
    }
    /* process kernel page fault */
    if (trapPsr & MACH_PS_BIT) {		/* kernel mode before trap */
	if (!(procPtr->genFlags & PROC_USER)) {
	    /*
	     * This fault happened inside the kernel and it wasn't on behalf
	     * of a user process.  This is an error.
	     */
	    panic("MachPageFault: page fault in kernel process! pc is 0x%x\n",
		    (unsigned) pcValue);
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
	    panic("MachPageFault: kernel page fault at illegal pc: 0x%x, addr 0x%x\n",
		    pcValue, addrErrorReg);
	}
	protError = (busErrorReg & MACH_PROT_ERROR);
	/*
	 * Try to fault in the page.
	 */
	if (addrErrorReg == (Address) NIL) {
	    if (copyInProgress) {
		return MACH_USER_ERROR;
	    } else {
		panic("MachPageFault: kernel tried to fault NIL address at pc 0x%x.\n", pcValue);
	    }
	}
	status = Vm_PageIn(addrErrorReg, protError);
	if (status != SUCCESS) {
	    if (copyInProgress) {
		/* user error */
		return MACH_USER_ERROR;
	    } else {
		/* kernel error */
		panic(
	    "MachPageFault: couldn't page in kernel page at 0x%x, pc 0x%x\n",
			addrErrorReg, pcValue);
	    }
	}
	return MACH_OK;
    }
    /* user page fault */
    printf("Page fault in user process, pc 0x%x, addr 0x%x, busError 0x%x\n",
	    pcValue, addrErrorReg, (short) busErrorReg);
    {
	unsigned	int	pte;
	unsigned	int	context;
	unsigned	int	the_seg;

	pte = VmMachGetPageMap(addrErrorReg);
	printf("pte is at first 0x%x\n", pte);
	context = VmMachGetContextReg();
	printf("context is at first 0x%x\n", context);
	the_seg = VmMachGetSegMap(addrErrorReg);
	printf("the_seg is at first 0x%x\n", the_seg);
    }
    protError = busErrorReg & MACH_PROT_ERROR;
    if (Vm_PageIn(addrErrorReg, protError) != SUCCESS) {
	printf(
    "MachPageFault: Bus error in user proc %x, PC = %x, addr = %x BR Reg %x\n",
		procPtr->processID, pcValue, addrErrorReg, (short) busErrorReg);
	/* Kill user process */
#ifdef NOTDEF
	Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, procPtr->processID, FALSE);
#else
	/* Can I do this here temporarily this way? */
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
#endif NOTDEF
    }
    {
	unsigned	int	pte;
	unsigned	int	context;
	unsigned	int	the_seg;

	pte = VmMachGetPageMap(addrErrorReg);
	printf("Returning okay, pte = 0x%x.\n", pte);
	context = VmMachGetContextReg();
	printf("context is now 0x%x\n", context);
	the_seg = VmMachGetSegMap(addrErrorReg);
	printf("the_seg is now 0x%x\n", the_seg);
    }
    return(MACH_OK);
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
    Sig_Stack		sigStack;
    Sig_Context		sigContext;
    Address		pc;

    procPtr = Proc_GetCurrentProc();
    procPtr->specialHandling = 0;
    /*
     * Take a context switch if one is pending for this process.
     * If other stuff, such as flushing the windows to the stack needs to
     * be done, it will happen when the process is switched back in again.
     */
    if (procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING) {
	Sched_LockAndSwitch();
    }
    /*
     * Should I flush windows to the stack before or after dealing with
     * a signal?  I'm not sure I understand the choices.  I'll do it first.
     */
    machStatePtr = procPtr->machStatePtr;
    /*
     * This is a while, since the Vm code may make it necessary to save
     * further windows that weren't set when we got here.  At least I think
     * that might happen.  I should test it to see if Vm ever calls that
     * deeply, but this is safer in case someone changes that code.
     */
    while (machStatePtr->savedMask != 0) {
	int	i;

	DEBUG_ADD(0xbbbbbbbb);
	DEBUG_ADD(machStatePtr->savedMask);
	for (i = 0; i < MACH_NUM_WINDOWS; i++) {
	    if ((1 << i) & machStatePtr->savedMask) {
		/* clear the mask for this window */
		machStatePtr->savedMask &= ~(1 << i);
		/*
		 * Push the window to the stack.  Should I re-verify the stack
		 * pointer?  I don't think I need to, since it's stored in
		 * kernel space and I verified it before storing it.
		 */
		if (Vm_CopyOut(MACH_SAVED_WINDOW_SIZE,
			(Address)&(machStatePtr->savedRegs[i]),
			machStatePtr->savedSps[i]) != SUCCESS) {
		    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
		}
	    }
	}
    }
    /*
     * Now check for signal stuff.
     */
    sigStack.contextPtr = &sigContext;
    if (Sig_Handle(procPtr, &sigStack, &pc)) {
	panic("MachUserAction: can't do signal stuff yet!\n");
    }
    return FALSE;
}
