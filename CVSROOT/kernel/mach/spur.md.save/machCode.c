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
#include "machCCRegs.h"
#include "machConst.h"
#include "machInt.h"
#include "mach.h"
#include "sys.h"
#include "sync.h"
#include "dbg.h"
#include "proc.h"
#include "sched.h"
#include "vm.h"
#include "vmMachInt.h"
#include "sig.h"
#include "sigSpur.h"
#include "mem.h"

/*
 *  Number of processors in the system.
 */
#ifndef NUM_PROCESSORS
#define NUM_PROCESSORS		1
#endif NUM_PROCESSORS

/*
 * TRUE if cpu was in kernel mode before the interrupt, FALSE if was in 
 * user mode.
 */
Boolean	mach_KernelMode;

int mach_NumProcessors = NUM_PROCESSORS;

/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel = FALSE;

/*
 * The machine type string is imported by the file system and
 * used when expanding $MACHINE in file names.
 */
char *mach_MachineType = "spur";

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

unsigned int	mach_CycleTime = MACH_CYCLE_TIME;

/*
 * The variables and tables below are used by the assembler routine
 * in loMem.s that dispatches kernel calls.  All of this information
 * is shared with loMem.s;  if you change any of this, be sure to
 * change the assembler to match.
 */

#define MAXCALLS 120
#define MAXARGS  12

extern	int machMaxSysCall;		/* Highest defined system call. */
int machNumArgs[MAXCALLS];		/* For each system call, gives the
					 * number of arguments. */
ReturnStatus (*(mach_NormalHandlers[MAXCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
					 * the call for non-migrated processes.
					 */
ReturnStatus (*(mach_MigratedHandlers[MAXCALLS]))();
					/* For each system call, gives the
					 * address of the routine to handle
					 * the call for migrated processes. */
extern int machKcallTableOffset;	/* Byte offset of the kcallTable field
					 * in a Proc_ControlBlock. */
extern int machStatePtrOffset;		/* Byte offset of the machStatePtr
					 * field in a Proc_ControlBlock. */
/* 
 * Pointer to the state structure for the current process.
 */
extern Mach_State	*machCurStatePtr;

/*
 * Debugger state.
 */
Mach_RegState	machDebugState;

/*
 * Interrupt table struct.
 */
void	(*(interruptHandlers[MACH_NUM_INTR_TYPES]))();
void	InterruptError();

void	MachFetchArgStart();
void	MachFetchArgEnd();


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_Init --
 *
 *	Initialize the mach module.
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
    mach_LastUserStackPage = MACH_LAST_USER_STACK_PAGE;

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
    /*
     * Initialize the interrupt handler table.
     */
    for (i = 0; i < MACH_NUM_INTR_TYPES; i++) {
	interruptHandlers[i] = InterruptError;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InterruptError --
 *
 *	Handle a non-handled interrupt type.
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
InterruptError(statusRegPtr, kpsw)
    unsigned 	int	*statusRegPtr;
    unsigned	int	kpsw;
{
    Sys_Panic(SYS_FATAL, "InterruptError: Bogus interrupt, ISR=%x KPSW=%x\n",
			 *statusRegPtr, kpsw);
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
    procPtr->machStatePtr = (struct Mach_State *)Mem_Alloc(sizeof(Mach_State));
    procPtr->machStatePtr->kernStackStart = mach_StackBottom;
    procPtr->machStatePtr->kernStackEnd = mach_StackBottom + mach_KernStackSize;
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
Mach_SetupNewState(procPtr, parStatePtr, startFunc, startPC)
    Proc_ControlBlock	*procPtr;	/* Pointer to process control block
					 * to initialize state for. */
    Mach_State		*parStatePtr;	/* State of parent on fork or from
					 * other machine on migration. */
    void		(*startFunc)();	/* Function to call when process first
					 * starts executing. */
    Address		startPC;	/* Address to pass as argument to 
					 * startFunc.  If NIL then the address
					 * is taken from *parStatePtr's 
					 * exception stack. */
{
    register	Mach_State	*statePtr;

    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = (Mach_State *)Mem_Alloc(sizeof(Mach_State));
    }

    statePtr = procPtr->machStatePtr;
    /*
     * Allocate a kernel stack for this process.
     */
    statePtr->kernStackStart = Vm_GetKernelStack(1);
    if (statePtr->kernStackStart == (Address)NIL) {
	return(PROC_NO_STACKS);
    }
    statePtr->kernStackEnd = statePtr->kernStackStart + mach_KernStackSize;
    /*
     * Set up the switch regs so that things start out like we were in
     * the middle of a context switch.  When we context switch in this process
     * it will restore its state from the regs.  The restore of state
     * restores up through the parents window.  Thus when we start running
     * we will have access to our parents r26 through r31 as r10 through
     * r15.  MachContextSwitch will return to the value stored in r10 so
     * we set the value of the function to start executing at in r26.  When
     * it returns it will go back one window (which will be the exact window
     * of switchRegState.regs) and start executing in the start function.
     * The start function will expect as an argument the PC of where to start
     * the process for real.  Thus in order to set up the first arg we
     * have to put a value in MACH_INPUT_REG1.
     */
    statePtr->switchRegState.regs[MACH_SPILL_SP][1] = (int)statePtr->kernStackEnd;
    statePtr->switchRegState.kpsw = (MACH_KPSW_PREFETCH_ENA |
				     MACH_KPSW_IBUFFER_ENA |
				     MACH_KPSW_VIRT_DFETCH_ENA |
				     MACH_KPSW_VIRT_IFETCH_ENA |
				     MACH_KPSW_ALL_TRAPS_ENA |
				     MACH_KPSW_FAULT_TRAP_ENA |
				     MACH_KPSW_ERROR_TRAP_ENA);
    statePtr->switchRegState.regs[26][1] = (int)startFunc;
    /*
     * Start the cwp such that when MachContextSwitch returns it won't
     * cause an overflow.
     */
    statePtr->switchRegState.cwp = 2;
    statePtr->switchRegState.swp = statePtr->kernStackStart;
    if (startPC == (Address)NIL) {
	/*
	 * User processes inherit from their parent.  The parameter that
	 * is passed to the start function is ignored.  What is important
	 * is the first and next PCs in the trap register state which will
	 * be restored before the process is started running in user mode.
	 */
	Byte_Copy(sizeof(statePtr->userState.trapRegState),
		  (Address)&parStatePtr->userState.trapRegState,
		  (Address)&statePtr->userState.trapRegState);
    } else {
	/*
	 * Kernel processes start executing at startPC.
	 */
	statePtr->switchRegState.regs[MACH_INPUT_REG1][1] = (int)startPC;
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
    procPtr->machStatePtr->userState.trapRegState.regs[MACH_RETURN_VAL_REG][1] = retVal;
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
    statePtr->userState.minSWP = (Address)MACH_SAVED_WINDOW_STACK_BASE;
    statePtr->userState.maxSWP = (Address)(MACH_SAVED_WINDOW_STACK_BASE + 
					   VMMACH_PAGE_SIZE);
    /*
     * Allocate memory for the saved window stack.
     */
    Vm_UserMap(statePtr->userState.minSWP, statePtr->userState.maxSWP);
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
    register	Mach_RegState	*regStatePtr;
    register	Mach_State	*statePtr;

    statePtr = procPtr->machStatePtr;
    /*
     * Free up the old saved window stack.
     */
    Vm_UserUnmap(statePtr->userState.minSWP, statePtr->userState.maxSWP);

    regStatePtr = &statePtr->userState.trapRegState;
    regStatePtr->regs[MACH_SPILL_SP][1] = (int)userStackPtr;
    regStatePtr->curPC = entryPoint;
    regStatePtr->nextPC = (Address)0;
    regStatePtr->kpsw = (MACH_KPSW_PREFETCH_ENA | 
			 MACH_KPSW_IBUFFER_ENA |
			 MACH_KPSW_VIRT_DFETCH_ENA |
			 MACH_KPSW_VIRT_IFETCH_ENA |
			 MACH_KPSW_INTR_TRAP_ENA |
			 MACH_KPSW_FAULT_TRAP_ENA |
			 MACH_KPSW_ERROR_TRAP_ENA |
			 MACH_KPSW_USE_CUR_PC);
    /*
     * Start the cwp off at 4 so that when we restore state we will restore
     * window 3.  This will start the user process off with a cwp of 3 and
     * an swp of minSWP + 2.  Since we allocate more memory for a saved window
     * stack when the swp points to minSWP + 1, by setting the swp to 
     * minSWP + 2 we know that if the swp == minSWP + 1 that there is in 
     * fact another page of windows below the swp.  
     */
    regStatePtr->cwp = 4;
    regStatePtr->swp = (Address)(MACH_SAVED_WINDOW_STACK_BASE + 
				 2 * MACH_SAVED_WINDOW_SIZE);
    MachRunUserProc();
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
 *	to be used by the debugger to modify the state.  Can modify all 
 *	fields in the trap reg state struct except for the MACH_SPILL_SP reg
 *	because that is the kernel's stack pointer.
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
    Byte_Copy(sizeof(Mach_RegState),
	      (Address)&statePtr->userState.trapRegState,
	      (Address)&destProcPtr->machStatePtr->userState.trapRegState);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetStackPointer --
 *
 *	Return the stack pointer value from the mach state struct.
 *
 * Results:
 *	Stack pointer value.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Address
Mach_GetStackPointer()
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    return((Address)procPtr->machStatePtr->userState.trapRegState.regs[MACH_SPILL_SP][1]);
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
	Sys_Panic(SYS_FATAL, "out-of-order kernel call initialization");
    }
    if (machMaxSysCall >= MAXCALLS) {
	Sys_Panic(SYS_FATAL, "too many kernel calls");
    }
    if (numArgs > MAXARGS) {
	Sys_Panic(SYS_FATAL, "too many arguments to kernel call");
    }
    machNumArgs[machMaxSysCall] = numArgs;
    mach_NormalHandlers[machMaxSysCall] = normalHandler;
    mach_MigratedHandlers[machMaxSysCall] = migratedHandler;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_SetHandler --
 *
 *	Put a device driver interrupt handling routine into the interrupt
 *	vector.
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
Mach_SetHandler(intrMask, handler)
    unsigned int intrMask;	/* Interrupt status register mask bits.  If
				 * a bit is set then handler will be called
				 * when the interrupt status register has the
				 * corresponding bit set. */
    void	 (*handler)();	/* Interrupt handling procedure */
{
    int			intrType;
    unsigned	int	mask;

    intrType = 0;
    mask = 1;
    while (intrType < MACH_NUM_INTR_TYPES) {
	if (mask & intrMask) {
	    interruptHandlers[intrType] = handler;
	}
	mask = mask << 1;
	intrType++;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_AllocExtIntrNumber --
 *
 *      Allocate an external interrupt number and register the specified
 *	function under that number.
 *
 * Results:
 *     SUCCESS or FAILURE if allocation fails.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Mach_AllocExtIntrNumber(handler,intrNumberPtr)
    int          	(*handler)();  	/* Interrupt handling procedure */
    unsigned int	*intrNumberPtr;	/* IN/OUT - Desired interrupt number
					 * and return value location.
					 */
{
    unsigned int intrType;
    unsigned int intrMask;

    intrType = *intrNumberPtr;

    if (intrType == MACH_EXT_INTERRUPT_ANY) {
	/* 
	 * If intrNumber is MACH_EXT_INTERRUPT_ANY, allocate the first available
	 * interrupt number.
	 */
	for (intrType = 0; intrType <= MACH_MAX_EXT_INTERRUPT; intrType++) {
	    if (interruptHandlers[intrType] == InterruptError) {
		break;
	    }
	}
    }
    /*
     * Validate the interrupt number.
     */
    if (intrType > MACH_MAX_EXT_INTERRUPT) {
	return (FAILURE);
    }
    intrMask = (1 << intrType);

    Mach_SetHandler(intrMask, handler);
    return (SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachInterrupt --
 *
 *	Call the proper routine to handle an interrupt.
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
MachInterrupt(intrStatusReg, kpsw)
    unsigned	int	intrStatusReg;	/* The interrupt status register. */
    unsigned	int	kpsw;		/* The kernel's psw. */
{
    unsigned	int	intrMask;
    int			intrType;
    unsigned	int	statusReg;

    mach_KernelMode = !(kpsw & MACH_KPSW_PREV_MODE);
    mach_AtInterruptLevel = TRUE;
    intrType = 0;
    intrMask = 1;
    statusReg = intrStatusReg;
    while (statusReg != 0) {
	if (statusReg & intrMask) {
	    (interruptHandlers[intrType])(&statusReg);
	    statusReg &= ~intrMask;
	}
	intrMask = intrMask << 1;
	intrType++;
    }
    mach_AtInterruptLevel = FALSE;
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachUserError --
 *
 *      Process the user process's error.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
void
MachUserError(errorType)
    int	errorType;
{
    register	Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();

    switch (errorType) {
	case MACH_USER_BAD_SWP:
	    /*
	     * This error is fatal to a user process so make the process
	     * exit.
	     */
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	    break;
	case MACH_USER_FPU_EXCEPT:
	    Sig_Send(SIG_ILL_INST, SIG_FPU_EXCEPT, procPtr->processID, FALSE);
	    break;
	case MACH_USER_ILLEGAL:
	    Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE, procPtr->processID,
		     FALSE);
	    break;
	case MACH_USER_FIXNUM:
	    Sig_Send(SIG_ILL_INST, SIG_FIXNUM, procPtr->processID, FALSE);
	    break;
	case MACH_USER_OVERFLOW:
	    Sig_Send(SIG_ILL_INST, SIG_OVERFLOW, procPtr->processID, FALSE);
	    break;
	case MACH_BAD_TRAP_TYPE:
	    Sig_Send(SIG_ILL_INST, SIG_BAD_TRAP, procPtr->processID,
		     FALSE);
	    break;
	case MACH_BAD_SYS_CALL:
	    Sig_Send(SIG_ILL_INST, SIG_BAD_SYS_CALL, procPtr->processID,
		     FALSE);
	    break;
	case MACH_BREAKPOINT:
	    Sig_Send(SIG_BREAKPOINT, SIG_NO_CODE, procPtr->processID, FALSE);
	    break;
	default:
	    Sys_Panic(SYS_FATAL, "MachUserError: Unknown user error %d\n",
				 errorType);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachVMFault --
 *
 *      Handle a Virtual memory fault.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
int
MachVMFault(faultType, PC, isDestAddr, destAddr, kpsw)
    int		faultType;	/* The type of the VM fault */
    Address	PC;		/* The address of the instruction that caused
				 * the fault. */
    Boolean	isDestAddr;	/* TRUE if faulting instruction was a load or
				 * a store and thus destAddr has a value. */
    Address	destAddr;	/* Value of dest/src of store/load instruction*/
    int		kpsw;		/* The KPSW at the time of the fault. */
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();

    if (faultType & MACH_VM_FAULT_REF_BIT) {
	VmMach_SetRefBit(PC);
	if (isDestAddr) {
	    VmMach_SetRefBit(destAddr);
	}
    }
    if (faultType & MACH_VM_FAULT_DIRTY_BIT) {
	VmMach_SetDirtyBit(PC);
	if (isDestAddr) {
	    VmMach_SetDirtyBit(destAddr);
	}
    }

    if (faultType & (MACH_VM_FAULT_PROTECTION | MACH_VM_FAULT_PAGE_FAULT)) {
	Boolean		userFault;

	userFault = kpsw & MACH_KPSW_PREV_MODE;
	/*
	 * We have to handle a protection fault or a page fault.  First try
	 * the PC.
	 */
	if (PC < (Address)VMMACH_SEG_SIZE) {
	    /*
	     * The PC falls into the system segment.  If it is above
	     * the end of the text or if it is inside the text part but 
	     * it caused a fault (i.e. there is no data address) then this
	     * is an error.
	     */
	    if (PC >= (Address)&endText || !isDestAddr) {
		if (userFault) {
		    (void)Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL,
                                   procPtr->processID, FALSE);
		    return(MACH_NORM_RETURN);
		} else {
		    return(MACH_KERN_ACCESS_VIOL);
		}
	    }
	} else {
	    /*
	     * The PC falls into one of the user segments.  If this is
	     * a kernel process then this is in an error.
	     */
	    if (!userFault) {
		return(MACH_KERN_ACCESS_VIOL);
	    }
	    /* Take a page fault on this address.  Since all of the user
	     * segments are readable this can't possibly be a protection
	     * fault if the page is valid.
	     */
	    if (Vm_PageIn(PC, FALSE) != SUCCESS) {
		(void)Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL,
			       procPtr->processID, FALSE);
		return(MACH_NORM_RETURN);
	    }
	}
	/*
	 * Now we have handled the program counter's page so handle the 
	 * data address if there is one.
	 */
	if (!isDestAddr) {
	    return(MACH_NORM_RETURN);
	}
	if (Vm_PageIn(destAddr, faultType & MACH_VM_FAULT_PROTECTION) !=
								SUCCESS) {
	    if (userFault) {
		(void)Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL,
			       procPtr->processID, FALSE);
		return(MACH_NORM_RETURN);
	    } else {
		if ((unsigned)PC >= (unsigned)MachFetchArgStart &&
	            (unsigned)PC < (unsigned)MachFetchArgEnd) {
		    return(MACH_FAILED_ARG_FETCH);
		} else if ((unsigned)PC >= (unsigned)VmMachDoCopy &&
	                   (unsigned)PC < (unsigned)VmMachCopyEnd) {
		    return(MACH_FAILED_COPY);
		} else {
		    return(MACH_KERN_ACCESS_VIOL);
		}
	    }
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachUserAction --
 *
 *      Determine what type of action to take for the user process.
 *
 * Results:
 *      TRUE if need to call a signal handler and FALSE otherwise.
 *
 * Side effects:
 *      Machine state struct may be modified.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUserAction()
{
    register	Mach_State		*statePtr;
    register	Proc_ControlBlock	*procPtr;
    Sig_Stack				sigStack;
    Sig_Context				sigContext;
    Address				pc;

    procPtr = Proc_GetCurrentProc();
    procPtr->specialHandling = 0;
    /* 
     * Take a context switch if one is pending for this process.
     */
    if (procPtr->schedFlags & SCHED_CONTEXT_SWITCH_PENDING) {
	Sched_LockAndSwitch();
    }
    sigStack.contextPtr = &sigContext;
    if (Sig_Handle(procPtr, &sigStack, &pc)) {
	statePtr = procPtr->machStatePtr;
	statePtr->userState.newCurPC = pc;
	statePtr->userState.sigNum = sigStack.sigNum;
	statePtr->userState.sigCode = sigStack.sigCode;
	statePtr->userState.oldHoldMask = sigContext.oldHoldMask;
	return(TRUE);
    } else {
	return(FALSE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachGetWinMem --
 *
 *      Get more window memory for the current process.
 *
 * Results:
 *      
 *
 * Side effects:
 *      Machine state struct may be modified.
 *
 * ----------------------------------------------------------------------------
 */
void
MachGetWinMem()
{
    register Mach_State	*statePtr;
    Address		swp;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;

    swp = statePtr->userState.trapRegState.swp;
    if (swp <= statePtr->userState.minSWP + MACH_SAVED_WINDOW_SIZE) {
	/*
	 * Allocate more memory at the low end.
	 */
	if (statePtr->userState.minSWP <= (Address)MACH_SAVED_WINDOW_STACK_BASE) {
	    /*
	     * We are already at the bottom of the stack so we can't allocated
	     * any more memory.
	     */
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}
	if (statePtr->userState.maxSWP - statePtr->userState.minSWP > 
							VMMACH_PAGE_SIZE) {
	    /*
	     * Free the highest page because we never want more than two
	     * full pages at once.
	     */
	    Vm_UserUnmap(statePtr->userState.minSWP + VMMACH_PAGE_SIZE,
			 statePtr->userState.maxSWP);
	    statePtr->userState.maxSWP -= VMMACH_PAGE_SIZE;
	}
	statePtr->userState.minSWP -= VMMACH_PAGE_SIZE;
	Vm_UserMap(statePtr->userState.minSWP, statePtr->userState.maxSWP);
    } else if (swp > statePtr->userState.maxSWP - 2 * MACH_SAVED_WINDOW_SIZE) {
	/*
	 * Need to allocate more at the high end.
	 */
	if (swp > statePtr->userState.maxSWP - 1 * MACH_SAVED_WINDOW_SIZE) {
	    Sys_Panic(SYS_FATAL, "MachGetWinMem: SWP too big.\n");
	}
	if (statePtr->userState.maxSWP - statePtr->userState.minSWP >
							VMMACH_PAGE_SIZE) {
	    /*
	     * Free the lowest page because we never want more than two
	     * full pages at once.
	     */
	    Vm_UserUnmap(statePtr->userState.minSWP,
			 statePtr->userState.minSWP + VMMACH_PAGE_SIZE);
	    statePtr->userState.minSWP += VMMACH_PAGE_SIZE;
	}
	statePtr->userState.maxSWP += VMMACH_PAGE_SIZE;
	Vm_UserMap(statePtr->userState.minSWP, statePtr->userState.maxSWP);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachSigReturn --
 *
 *      Handle a return from signal trap.
 *
 * Results:
 *	MACH_NORM_RETURN. 
 *
 * Side effects:
 *      Machine state struct may be modified.
 *
 * ----------------------------------------------------------------------------
 */
int
MachSigReturn(oldHoldMask)
    unsigned	int	oldHoldMask;
{
    Sig_Stack	sigStack;
    Sig_Context	sigContext;

    sigStack.contextPtr = &sigContext;
    sigContext.oldHoldMask = oldHoldMask;
    Sig_Return(Proc_GetCurrentProc(), &sigStack);
    return(MACH_NORM_RETURN);
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
 * Mach_GetSlotId --
 *
 * Return the NuBus slot id of the processor.
 *
 * Results:
 *     The slot id.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
unsigned int
Mach_GetSlotId()
{

	return (Mach_Read8bitCCReg(MACH_SLOT_ID_REG));
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_GetMachineType --
 *
 *	Returns the machine type, 0 for SPUR prototype.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Mach_GetMachineType()
{
	return (0);
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_GetMachineArch --
 *
 *	Returns the machine architecure, SPUR.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Mach_GetMachineArch()
{
	return (SYS_SPUR);
}
