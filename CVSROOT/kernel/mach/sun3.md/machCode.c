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
#include "machConst.h"
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
#include "mem.h"
#include "machMon.h"
#include "byte.h"
#include "swapBuffer.h"

int	machLastSP, machPOP;

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

#ifdef SUN3
char *mach_MachineType = "sun3";
#endif
#ifdef SUN2
char *mach_MachineType = "sun2";
#endif

/*
 * The byte ordering/alignment type used with Swap_Buffer and I/O control data
 */
int mach_ByteOrder = SWAP_SUN_TYPE;

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
    Address	userStackPtr;		/* The user's stack pointer. */
    short	statusReg;		/* The status register. */
    void	(*startFunc)();		/* Function to call when process
					 * first starts executing. */
    int		retPC;			/* Return PC that will be sitting on the
    					 * stack when startFunc is called. */
    Address	startPC;		/* PC to start executing at.  Is passed
					 * as an argument to startFunc. */
    int		fill1;			/* Filler for the debugger. */
    int		fill2;			/* Filler for the debugger. */
} KernelStack;

/*
 * The format of a signal stack that is pushed onto a user's stack when
 * a signal is handled.
 */
typedef struct {
    Address	retAddr;
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

extern	int		MachGetVBR();
MachVectorTable		*machVectorTablePtr;

/*
 * The variables and tables below are used by the assembler routine
 * in machTrap.s that dispatches kernel calls.  All of this information
 * is shared with machTrap.s;  if you change any of this, be sure to
 * change the assembler to match.
 */

#define MAXCALLS 120
#define MAXARGS  16

int machMaxSysCall;			/* Highest defined system call. */
int machArgOffsets[MAXCALLS];		/* For each system call, tells how much
					 * to add to the sp at the time of the
					 * call to get to the highest argument
					 * on the stack. */
Address machArgDispatch[MAXCALLS];	/* For each system call, gives an
					 * address to branch to, in the middle
					 * of MachFetchArgs, to copy the right
					 * # of args from user space to the
					 * kernel's stack. */
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
/* 
 * Pointer to the state structure for the current process.
 */
Mach_State	*machCurStatePtr = (Mach_State *)NIL;

void	SetupSigHandler();
void	ReturnFromSigHandler();
void	MachUserReturn();


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
    int	*vecTablePtr;
    int	*protoVecTablePtr;
    int	i;

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

    /*
     * Initialize the vector table.
     */
    vecTablePtr = (int *) MachGetVBR();
    machVectorTablePtr = (MachVectorTable *) vecTablePtr;
    protoVecTablePtr = (int *) &machProtoVectorTable;
    for (i = 0; i < MACH_NUM_EXCEPTIONS; i++) {
	if (*protoVecTablePtr != 0) {
	     *vecTablePtr = *protoVecTablePtr;
	}

	vecTablePtr++;
	protoVecTablePtr++;
    }

#ifdef SUN3
    /*
     * Initialize the autovector interrupt slots.
     */
    for (i = MACH_NUM_EXCEPTIONS ; i<256 ; i++) {
	extern int MachBrkptTrap();

	*vecTablePtr = (int)MachBrkptTrap;
	vecTablePtr++;
    }
#endif SUN3
#ifndef SUN3
    /*
     * Put the vectors at the base of the kernel if are on a Sun-2 only.  On
     * a Sun-3 they can stay where they are.  The vectors on the Sun-2 can
     * be moved this easily because virtual address 0 which is where the
     * vector table is at, points to the same physical page as virtual address 
     * mach_KernStart.
     */
    MachSetVBR(mach_KernStart);
    machVectorTablePtr = (MachVectorTable *) mach_KernStart;
#endif

    /*
     * Initialize some of the dispatching information.  The rest is
     * initialized by Mach_InitSysCall below.
     */
    machMaxSysCall = -1;
    machKcallTableOffset = (int) &((Proc_ControlBlock *) 0)->kcallTable;
    machStatePtrOffset = (int) &((Proc_ControlBlock *) 0)->machStatePtr;

    /*
     * We start off with interrupts disabled.
     */
    mach_NumDisableInterrupts[0] = 1;
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
    procPtr->machStatePtr = (struct Mach_State *)malloc(sizeof(Mach_State));
    procPtr->machStatePtr->kernStackStart = mach_StackBottom;
    procPtr->machStatePtr->setJumpStatePtr = (Mach_SetJumpState *)NIL;
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
    register	KernelStack	*stackPtr;
    register	Mach_State	*statePtr;

    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = (Mach_State *)malloc(sizeof(Mach_State));
	procPtr->machStatePtr->setJumpStatePtr = (Mach_SetJumpState *)NIL;
    }

    statePtr = procPtr->machStatePtr;
    /* 
     * Allocate a kernel stack for this process.
     */
    statePtr->kernStackStart = Vm_GetKernelStack(0);
    if (statePtr->kernStackStart == (Address)NIL) {
	return(PROC_NO_STACKS);
    }

    statePtr->switchRegs[SP] = (int)(statePtr->kernStackStart + 
			             MACH_KERN_STACK_SIZE - 
				     sizeof(KernelStack));
    /*
     * Initialize the stack so that it looks like it is in the middle of
     * Mach_ContextSwitch.
     */
    stackPtr = (KernelStack *)(statePtr->switchRegs[SP]);
    stackPtr->magicNumber = MAGIC;
    stackPtr->userStackPtr = mach_MaxUserStackAddr;
    stackPtr->statusReg = MACH_SR_HIGHPRIO;
    stackPtr->startFunc =  startFunc;
    stackPtr->fill1 = 0;
    stackPtr->fill2 = 0;
    /* 
     * Set up the state of the process.  User processes inherit from their
     * parent or the migrated process.  If the PC is not specified, take it
     * from the parent as well.
     */
    if (user) {
	statePtr->userState.userStackPtr =
		fromStatePtr->userState.userStackPtr;
	bcopy((Address)fromStatePtr->userState.trapRegs,
		  (Address)statePtr->userState.trapRegs,
		  sizeof(statePtr->userState.trapRegs));
    }
    if (startPC == (Address)NIL) {
	stackPtr->startPC = (Address)fromStatePtr->userState.excStackPtr->pc;
    } else {
	stackPtr->startPC = startPC;
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
    procPtr->machStatePtr->userState.trapRegs[D0] = retVal;
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
    register	Mach_ExcStack	*excStackPtr;

    statePtr = procPtr->machStatePtr;
    excStackPtr = (Mach_ExcStack *)
        (statePtr->kernStackStart + MACH_BARE_STACK_OFFSET - MACH_SHORT_SIZE);
    statePtr->userState.excStackPtr = excStackPtr;
    statePtr->userState.trapRegs[SP] = (int)excStackPtr;
    excStackPtr->statusReg = 0;
    excStackPtr->pc = (int)entryPoint;
    excStackPtr->vor.stackFormat = MACH_SHORT;
    MachUserReturn(procPtr);

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
    Address		userStackPtr;	/* Stack pointer for when the
						 * user process resumes 
						 * execution. */
    Address		entryPoint;		/* Where the user process is
						 * to resume execution. */
{
    procPtr->machStatePtr->userState.userStackPtr = userStackPtr;
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
    bcopy((Address)machStatePtr->userState.trapRegs,
	      (Address)debugStatePtr->regState.regs,
	      sizeof(machStatePtr->userState.trapRegs));
    debugStatePtr->regState.regs[SP] = 
			(int)machStatePtr->userState.userStackPtr;
    debugStatePtr->regState.pc = machStatePtr->userState.excStackPtr->pc;
    debugStatePtr->regState.statusReg  =
			machStatePtr->userState.excStackPtr->statusReg;
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
    return(procPtr->machStatePtr->userState.userStackPtr);
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
    if (machMaxSysCall >= MAXCALLS) {
	panic("too many kernel calls");
    }
    if (numArgs > MAXARGS) {
	panic("too many arguments to kernel call");
    }
    machArgOffsets[machMaxSysCall] = 8 + numArgs*4;
    machArgDispatch[machMaxSysCall] = 
		(16-numArgs)*2 + (Address) ((unsigned int)MachFetchArgs);
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
    int	*vecTablePtr;

    if (vectorNumber < 64 || vectorNumber > 255) {
	printf("%d: ", vectorNumber);
	printf("Warning: Bad vector number\n");
    } else {
	vecTablePtr = (int *) MachGetVBR();
	vecTablePtr[vectorNumber] = (int)handler;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachTrap --
 *
 *      The trap handler routine.  This deals with supervisor mode and 
 *	non-supervisor mode traps differently.  The only allowed supervisor
 *	mode traps are a breakpoint trap to force a context switch and a
 *	bus error in the middle of a cross address space copy. All other
 *	traps go into the debugger.  However, all types of user traps are 
 *	processed here, except for system calls, which don't pass through
 *	this procedure unless special action (like a context switch) is
 *	needed at the end of the call.
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
MachTrap(trapStack)
    Mach_TrapStack	trapStack;	/* The stack at the time of the trap.*/
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status;

    procPtr = Proc_GetActualProc();
    /*
     * Process kernel traps.
     */
    if (trapStack.excStack.statusReg & MACH_SR_SUPSTATE) {
	switch (trapStack.trapType) {
	    case MACH_TRACE_TRAP:
		/*
		 * If the trace trap occured on a user trap instruction, then
		 * the trace trap will be taken on the first instruction of 
		 * the trap handler.  When this trace trap occurs,
		 * instead of the saved status register being in user mode and 
		 * having the trace trap bit set, the status register will 
		 * be in kernel mode with no trace trap bit set.
		 * In this case we just ignore the trace trap because it 
		 * will reoccur when the user process continues.
		 */
		if (!(trapStack.excStack.statusReg & MACH_SR_TRACEMODE)) {
		    return(MACH_OK);
		}

		/*
		 * In the normal case enter the debugger with a breakpoint
		 * trap.
		 */
		return(MACH_KERN_ERROR);

	    case MACH_BUS_ERROR:

		if (trapStack.busErrorReg.timeOut) {
		    /*
		     * Allow for refresh memory time just like Unix.
		     */
		    MACH_DELAY(2000);
		}

		/*
		 * Check to see if is a parity error.
		 */

#ifndef SUN3
		if (trapStack.busErrorReg.parErrU || trapStack.busErrorReg.parErrL) {
		    panic("Parity error!!!\n");
		    return(MACH_KERN_ERROR);
		}

		if (trapStack.busErrorReg.busErr) {
		    panic("System bus error\n");
		    return(MACH_KERN_ERROR);
		}
#endif

		if (procPtr == (Proc_ControlBlock *)NIL) {
		    panic("MachTrap: Current process is NIL!! Trap PC 0x%x\n",
			   (unsigned) trapStack.excStack.pc);
		}

		if (procPtr->genFlags & PROC_USER) {
		    Boolean	protError;
		    Boolean	copyInProgress = FALSE;

		    /*
		     * A page fault on a user process while executing in
		     * the kernel.  This can happen when information is
		     * being copied back and forth between kernel and
		     * user state (indicated by particular values of the
		     * program counter), after a pointer is made accessible by 
		     * Vm_MakeAccessible (indicated by numMakeAcc > 0) or
		     * after someone did a set jump in the kernel and tried
		     * to access a user process.
		     */

		    if ((((unsigned) trapStack.excStack.pc) >= (unsigned) Vm_CopyIn)
			    && (((unsigned) trapStack.excStack.pc)
				< (unsigned) VmMachCopyEnd)) {
			copyInProgress = TRUE;
		    } else if ((((unsigned) trapStack.excStack.pc)
				>= (unsigned) MachFetchArgs)
			    && (((unsigned) trapStack.excStack.pc)
				<= (unsigned) MachFetchArgsEnd)) {
			copyInProgress = TRUE;
		    } else if ((procPtr->vmPtr->numMakeAcc == 0)
			&& (procPtr->machStatePtr->setJumpStatePtr
			== (Mach_SetJumpState *) NIL)) {
			return(MACH_KERN_ERROR);
		    }

		    protError = 
#ifdef SUN3
				!trapStack.busErrorReg.pageInvalid;
#else
				trapStack.busErrorReg.resident;
#endif
		    /*
		     * Try to fault in the page.
		     */
		    status = Vm_PageIn(
		      (Address)trapStack.excStack.tail.addrBusErr.faultAddr,
				  protError);
		    if (status != SUCCESS) {
			if (copyInProgress) {
			    return(MACH_USER_ERROR);
			} else {
			    /*
			     * Real kernel error.  Take a long jump if
			     * possible.
			     */
			    if (procPtr->machStatePtr->setJumpStatePtr != 
						(Mach_SetJumpState *) NIL) {
				Mach_LongJump(
				    procPtr->machStatePtr->setJumpStatePtr);
			    }
			    return(MACH_KERN_ERROR);
			}
		    } else {
			return(MACH_OK);
		    }
		} else {
		    /*
		     * Happened to a kernel process.  Take a long jump if 
		     * possible.
		     */
		    if (procPtr->machStatePtr->setJumpStatePtr !=
						(Mach_SetJumpState *) NIL) {
			Mach_LongJump(procPtr->machStatePtr->setJumpStatePtr);
		    }
		    return(MACH_KERN_ERROR);
		}
	    case MACH_SPURIOUS_INT:
		/*
		 * Ignore this for now because otherwise we can't debug mint
		 */
		if (!dbg_BeingDebugged) {
		    printf("MachTrap: Spurious interrupt\n");
		}
		return(MACH_OK);

	    default:
		return(MACH_KERN_ERROR);
	}
    } 

    /*
     * Process user traps.  
     */

    /* 
     * Take a context switch if one is pending for this process.
     */

    if (procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING) {
	Sched_LockAndSwitch();
    }
    /*
     * Now clear out the trace trap bit out of the status register.  This is
     * a precaution to take care of cases such as when a bus error occurs on 
     * the instruction that we are trying to trace such that the trace trap bit
     * is set but we didn't get a trace trap exception.
     */
    trapStack.excStack.statusReg &= ~MACH_SR_TRACEMODE;

    switch (trapStack.trapType) {
	case MACH_BUS_ERROR: {
	    Boolean	protError;
	    if (trapStack.busErrorReg.timeOut) {
		/*
		 * Allow for refresh memory time just like Unix.
		 */

		MACH_DELAY(2000);
	    }

	    /*
	     * Check for parity error.
	     */

#ifndef SUN3
	    if (trapStack.busErrorReg.parErrU || trapStack.busErrorReg.parErrL) {
		panic("Parity error!!!\n");
		return(MACH_KERN_ERROR);
	    }
#endif

	    /*
	     * Take a page fault. It is assumed that if the resident bit
	     * is set in the bus error register then this is a protection
	     * error.
	     */
	protError =
#ifdef SUN3
		    !trapStack.busErrorReg.pageInvalid;
#else
		    trapStack.busErrorReg.resident;
#endif
	if (Vm_PageIn((Address)trapStack.excStack.tail.addrBusErr.faultAddr, 
		      protError) != SUCCESS) {
		printf(
		    "MachTrap: Bus error in user proc %X, PC = %x, addr = %x BR Reg %x\n",
			    procPtr->processID, 
			    trapStack.excStack.pc,
			    trapStack.excStack.tail.addrBusErr.faultAddr,
			    *(short *)&trapStack.busErrorReg);
		(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
				procPtr->processID, FALSE);
	    }
	    break;
	}
	case MACH_SYSCALL_TRAP:
	    /*
	     * It used to be that all system calls passed through here, but
	     * the code was optimized to avoid calling either this procedure
	     * or Sys_SysCall in the normal case.  Control only passes through
	     * here if, at the end of a system call, it's discovered that
	     * special action must be taken.  The call has already been
	     * executed by the time things arrive here.  This code does
	     * nothing... the action will all be taken by the call to
	     * MachUserReturn below.
	     */

	    break;
		
	case MACH_BRKPT_TRAP:
	    Proc_Lock(procPtr);
	    if (procPtr->genFlags & PROC_DEBUG_ON_EXEC) {
	    	procPtr->genFlags &= ~PROC_DEBUG_ON_EXEC;
		(void) Sig_SendProc(procPtr, SIG_DEBUG, SIG_NO_CODE);
	    } else {
		(void) Sig_SendProc(procPtr, SIG_BREAKPOINT, SIG_NO_CODE);
	    }
	    Proc_Unlock(procPtr);
	    break;

	case MACH_SIG_RET_TRAP: {
	    /*
	     * We got a return from signal trap.
	     */
	    ReturnFromSigHandler(procPtr);
	    return(MACH_SIG_RETURN);
	}

	case MACH_ADDRESS_ERROR:
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ADDR_ERROR,
			    procPtr->processID, FALSE);
	    break;
	case MACH_ILLEGAL_INST:
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE);
	    break;
	case MACH_ZERO_DIV:
	    (void) Sig_Send(SIG_ARITH_FAULT, SIG_ZERO_DIV,
			    procPtr->processID, FALSE);
	    break;
	case MACH_CHK_INST:
	    (void) Sig_Send(SIG_ILL_INST, SIG_CHK,
			    procPtr->processID, FALSE);
	    break;
	case MACH_TRAPV:
	    (void) Sig_Send(SIG_ILL_INST, SIG_TRAPV,
			    procPtr->processID, FALSE);
	    break;
	case MACH_PRIV_VIOLATION:
	    (void) Sig_Send(SIG_ILL_INST, SIG_PRIV_INST,
			    procPtr->processID, FALSE);
	    break;
	case MACH_TRACE_TRAP: 
	    /*
	     * Involuntary context switch trace traps have already been taken
	     * care of above.  Here the only time we pay attention to a
	     * trace trap is if the debugger is trying to single step the 
	     * user process.
	     */
	    if (procPtr->genFlags & PROC_SINGLE_STEP_FLAG) {
		procPtr->genFlags &= ~PROC_SINGLE_STEP_FLAG;
		(void) Sig_Send(SIG_TRACE_TRAP, SIG_NO_CODE,
				procPtr->processID, FALSE);
	    }
	    break;

	case MACH_EMU1010:
	    (void) Sig_Send(SIG_ILL_INST, SIG_EMU1010,
			    procPtr->processID, FALSE);
	    break;
	case MACH_EMU1111:
	    (void) Sig_Send(SIG_ILL_INST, SIG_EMU1111,
			    procPtr->processID, FALSE);
	    break;
	case MACH_BAD_TRAP:
	    (void) Sig_Send(SIG_ILL_INST, SIG_BAD_TRAP,
			    procPtr->processID, FALSE);

	    break;
	default:
	    return(MACH_KERN_ERROR);
    } 

    MachUserReturn(procPtr);
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
void
MachUserReturn(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    SignalStack			sigStack;
    Address			pc;

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


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetExcStackSize --
 *
 *      Return the size of the trap stack.  This can vary depending on whether
 *	are on a Sun-3 or a Sun-2.
 *
 * Results:
 *      Trap stack size.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_GetExcStackSize(excStackPtr)
    Mach_ExcStack	*excStackPtr;
{
    switch (excStackPtr->vor.stackFormat) {
	case MACH_SHORT:
	    return(MACH_SHORT_SIZE);
	case MACH_THROWAWAY:
	    return(MACH_THROWAWAY_SIZE);
	case MACH_INST_EXCEPT:
	    return(MACH_INST_EXCEPT_SIZE);
	case MACH_MC68010_BUS_FAULT:
	    return(MACH_MC68010_BUS_FAULT_SIZE);
	case MACH_COPROC_MID_INSTR:
	    return(MACH_COPROC_MID_INSTR_SIZE);
	case MACH_SHORT_BUS_FAULT:
	    return(MACH_SHORT_BUS_FAULT_SIZE);
	case MACH_LONG_BUS_FAULT:
	    return(MACH_LONG_BUS_FAULT_SIZE);
	default:
	    printf("Warning: Mach_GetTrapStackSize: Bad stack format.\n");
	    return(-1);
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
#       ifdef SUN2
	return SYS_SUN2;
#       endif SUN2

#       ifdef SUN3
	return SYS_SUN3;
#       endif SUN3
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

