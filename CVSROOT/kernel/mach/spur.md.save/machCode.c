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
#include "sync.h"
#include "dbg.h"
#include "proc.h"
#include "sched.h"
#include "vm.h"
#include "vmMachInt.h"
#include "sig.h"
#include "sigMach.h"
#include "mem.h"
#include "swapBuffer.h"
#include "dev/ccdev.h"
#include "machConfig.h"


int mach_NumProcessors = 1;

/*
 * Master control processor.
 */

extern int machMasterProcessor;

/*
 * Start per processor data. 
 */

/*
 * TRUE if cpu was in kernel mode before the interrupt, FALSE if was in 
 * user mode.
 */
Boolean	mach_KernelMode[MACH_MAX_NUM_PROCESSORS];


/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel[MACH_MAX_NUM_PROCESSORS];

/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */
int mach_NumDisableInterrupts[MACH_MAX_NUM_PROCESSORS];
int *mach_NumDisableIntrsPtr = mach_NumDisableInterrupts;

/*
 * The machine type string is imported by the file system and
 * used when expanding $MACHINE in file names.
 */
char *mach_MachineType = "spur";

/*
 * The byte ordering/alignment type used by Swap_Buffer for I/O control data.
 */
int mach_ByteOrder = SWAP_SPUR_TYPE;


/* 
 * Pointer to the state structure for the current process.
 * Allocated in low memory. This structure is indexed by NuBus slot id so
 * there is current process for each board in the system.
 */
extern Mach_State	*machCurStatePtrs[];

/*
 * Debugger state.
 */
Mach_RegState	machDebugState[MACH_MAX_NUM_PROCESSORS];
extern Mach_RegState *machDebugStatePtrs[];
int machDebugSignal[MACH_MAX_NUM_PROCESSORS];

/*
 * Cache controller state.
 */
CCdev	machCCState[MACH_MAX_NUM_PROCESSORS];

/*
 * State of each processor in the system.
 */

Mach_ProcessorStatus	mach_ProcessorStatus[MACH_MAX_NUM_PROCESSORS];

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

static int machDbgInterruptNumber;
static int machCheckSpecialInterruptNum;
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
extern int machSpecialHandlingOffset;	/* Byte offset of the special handling
					 * flag in the proc table. */
extern int machTrapTableOffset;		/* The offset of the trap table
					 * within the special user page. */

/*
 * Interrupt table struct.
 */
void	(*(interruptHandlers[MACH_NUM_INTR_TYPES]))();
void	InterruptError();

void	MachFetchArgStart();
void	MachFetchArgEnd();

/*
 * Foward routine declaration.
 */
static void EnterDebugger();
static void doNothing();

#ifdef	PATCH_IBUFFER
#endif


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
     * Initialize the trap handler offset.
     */
    machTrapTableOffset = (int) ((Mach_SpecPage *) 0)->trapTable;
    /*
     * Initialize some of the dispatching information.  The rest is
     * initialized by Mach_InitSysCall below.
     */
    machMaxSysCall = -1;
    machKcallTableOffset = (int) &((Proc_ControlBlock *) 0)->kcallTable;
    machStatePtrOffset = (int) &((Proc_ControlBlock *) 0)->machStatePtr;
    machSpecialHandlingOffset = (int) &((Proc_ControlBlock *) 0)->specialHandling;
    /*
     * Initialize the machine configuation.
     */
    Mach_ConfigInit();
    /*
     * For each processor
     */
    for (i = 0; i < MACH_MAX_NUM_PROCESSORS; i++) { 
	/*
	 * We start off with interrupts disabled.
	 */
	mach_NumDisableInterrupts[i] = 1;
	/*
	 * But not at interrupt level.
	 */
	mach_AtInterruptLevel[i] = 0;
	/*
	 * Initialized the debugStatePtrs. 
	 */
	machDebugStatePtrs[i] = &(machDebugState[i]);
	/*
  	 * Initialize the status. 
	 */

	 mach_ProcessorStatus[i] = MACH_UNINITIALIZED_STATUS;
    }

    /*
     * The processor that executes this code is the master processor.
     */
    machMasterProcessor = Mach_GetProcessorNumber();
    mach_ProcessorStatus[Mach_GetProcessorNumber()] = MACH_ACTIVE_STATUS;

    /*
     * Initialize the interrupt handler table.
     */
    for (i = 0; i < MACH_NUM_INTR_TYPES; i++) {
	interruptHandlers[i] = InterruptError;
    }
    /*
     * Turn off all timers.
     */
    Mach_Write8bitCCReg(MACH_MODE_REG,0);
    /*
     * Clear the interrupt mask register and any pending interrupts.
     */
    Mach_Write32bitCCReg((unsigned int)MACH_INTR_MASK_0,(unsigned int)0);
    Mach_Write32bitCCReg((unsigned int)MACH_INTR_STATUS_0,(unsigned int)-1);

    /*
     * Start the refresh timer.
     */
    Mach_RefreshStart();

    /*
     * Set up the debugger interrupt.
     */
    machDbgInterruptNumber = MACH_EXT_INTERRUPT_ANY;
    Mach_AllocExtIntrNumber(EnterDebugger,&machDbgInterruptNumber);
    machDbgInterruptMask = (1 << machDbgInterruptNumber);
    Mach_SetNonmaskableIntr(machDbgInterruptMask);
    /*
     * Initialize the cross processor communication.
     */
    Mach_CPC_Init();
    /*
     * Initialize the check special handling interrupt.
     */
    machCheckSpecialInterruptNum = MACH_EXT_INTERRUPT_ANY;
    Mach_AllocExtIntrNumber(doNothing,&machCheckSpecialInterruptNum);
    
#define	CHECK_OFFSETS
#ifdef CHECK_OFFSETS
    /* 
     * Check of values of _OFFSETS in machConst.h used by assembly code in 
     * loMem.s to index into C structures.
     */
#define	C(s,o)	if ((int)&(((Mach_State*)0x0)->userState.trapRegState.s)!= o){\
			panic( "Bad offset %d != %d\n", \
			(int)&(((Mach_State*)0x0)->userState.trapRegState.s)\
			,o); \
		}

    C(regs,MACH_TRAP_REGS_OFFSET); C(kpsw,MACH_TRAP_KPSW_OFFSET);
    C(curPC,MACH_TRAP_CUR_PC_OFFSET); C(nextPC, MACH_TRAP_NEXT_PC_OFFSET);
    C(insert,MACH_TRAP_INSERT_OFFSET); C(swp, MACH_TRAP_SWP_OFFSET);
    C(cwp, MACH_TRAP_CWP_OFFSET);

#undef C
#define	C2(s,o)	if ((int)&(((Mach_State*)0x0)->userState.s)!= o){\
			panic( "Bad offset %d != %d\n", \
			(int)&(((Mach_State*)0x0)->userState.s),o); \
		}
    C2(specPageAddr, MACH_SPEC_PAGE_ADDR_OFFSET); 
    C2(swpBaseAddr, MACH_SWP_BASE_ADDR_OFFSET);
    C2(swpMaxAddr, MACH_SWP_MAX_ADDR_OFFSET);
    C2(minSWP,MACH_MIN_SWP_OFFSET); C2(maxSWP,MACH_MAX_SWP_OFFSET);
    C2(newCurPC,MACH_NEW_CUR_PC_OFFSET); C2(sigNum,MACH_SIG_NUM_OFFSET); 
    C2(sigCode, MACH_SIG_CODE_OFFSET); 
    C2(oldHoldMask,MACH_OLD_HOLD_MASK_OFFSET );
    C2(faultAddr,MACH_FAULT_ADDR_OFFSET);
#undef C2
#define	C3(s,o)	if ((int)&(((Mach_State*)0x0)->s)!= o){\
			panic( "Bad offset %d != %d\n", \
			(int)&(((Mach_State*)0x0)->s),o); \
			}
    C3(kernStackStart, MACH_KERN_STACK_START_OFFSET);
    C3(kernStackEnd, MACH_KERN_STACK_END_OFFSET);
#undef C3
#define	C4(s,o)	if ((int)&(((Mach_State*)0x0)->switchRegState.s)!= o){\
			panic( "Bad offset %d != %d\n", \
			(int)&(((Mach_State*)0x0)->switchRegState.s),o); \
		}
    C4(regs,MACH_SWITCH_REGS_OFFSET); C4(kpsw,MACH_SWITCH_KPSW_OFFSET);
    C4(upsw,MACH_SWITCH_UPSW_OFFSET); C4(curPC, MACH_SWITCH_CUR_PC_OFFSET);
    C4(nextPC, MACH_SWITCH_NEXT_PC_OFFSET); 
    C4(insert, MACH_SWITCH_INSERT_OFFSET);
    C4(swp, MACH_SWITCH_SWP_OFFSET); C4(cwp, MACH_SWITCH_CWP_OFFSET);
#undef C4
#endif

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
InterruptError(statusRegPtr)
    unsigned 	int	*statusRegPtr;
{
    panic( "InterruptError: Bogus interrupt, ISR=0x%x\n",
			 *statusRegPtr);
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
    bzero((char *)procPtr->machStatePtr, sizeof(Mach_State));
    procPtr->machStatePtr->kernStackStart = mach_StackBottom;
    procPtr->machStatePtr->kernStackEnd = mach_StackBottom + mach_KernStackSize;
    machCurStatePtrs[Mach_GetProcessorNumber()] = procPtr->machStatePtr;
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
/*ARGSUSED*/
ReturnStatus
Mach_SetupNewState(procPtr, parStatePtr, startFunc, startPC, user)
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
    Boolean		user;		/* TRUE if is a user process. */
{
    register	Mach_State	*statePtr;

    if (procPtr->machStatePtr == (Mach_State *)NIL) {
	procPtr->machStatePtr = (Mach_State *)Vm_RawAlloc(sizeof(Mach_State));
    }

   statePtr = procPtr->machStatePtr;
   bzero((char *)statePtr, sizeof(Mach_State));
    /*
     * Allocate a kernel stack for this process.
     */
    statePtr->kernStackStart = Vm_GetKernelStack(2);
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
     * r15.  MachContextSwitch will return to the value stored in the curPC 
     * field so we set the value of the function to start executing in this 
     * field.  When it returns it will go back one window (which will be the
     * exact window of switchRegState.regs) and start executing in the start
     * function.  The start function will expect as an argument the PC of
     * where to start the process for real.  Thus in order to set up the 
     * first arg we have to put a value in MACH_INPUT_REG1.
     */
    statePtr->switchRegState.regs[MACH_SPILL_SP][0] = (int)statePtr->kernStackEnd;
    statePtr->switchRegState.kpsw = (
#ifdef WITH_IBUFFER
#ifdef WITH_PREFETCH
				     MACH_KPSW_PREFETCH_ENA |
#endif
				     MACH_KPSW_IBUFFER_ENA |
#endif
				     MACH_KPSW_VIRT_DFETCH_ENA |
				     MACH_KPSW_VIRT_IFETCH_ENA |
				     MACH_KPSW_ALL_TRAPS_ENA |
				     MACH_KPSW_FAULT_TRAP_ENA |
				     MACH_KPSW_ERROR_TRAP_ENA |
				     MACH_KPSW_INTR_TRAP_ENA);
    /*
     * Clear the upsw so the kernel doesn't take random user traps.
     */
    statePtr->switchRegState.upsw = 0;
    /*
     * MachContextSwitch does a "return curPC,$8" so the starting address
     * should be startFunc - 8.
     */
    statePtr->switchRegState.curPC = (Address)((unsigned int)startFunc) - 8;
    /*
     * Start the cwp such that when MachContextSwitch returns it won't
     * cause an underflow.  Note that we want the second window so we set
     * the cwp to 8 since the window number is shifted over by 2 bits.
     */
    statePtr->switchRegState.cwp = 8;
    statePtr->switchRegState.swp = statePtr->kernStackStart;
    if (startPC == (Address)NIL) {
	/*
	 * User processes inherit from their parent.  The parameter that
	 * is passed to the start function is ignored.  What is important
	 * is the first and next PCs in the trap register state which will
	 * be restored before the process is started running in user mode.
	 */
	bcopy((Address)&parStatePtr->userState.trapRegState,
	      (Address)&statePtr->userState.trapRegState,
	      sizeof(statePtr->userState.trapRegState));
	/* 
	 * User processes also inherit saved window state from their
	 * parent.
	 */
	statePtr->userState.specPageAddr = parStatePtr->userState.specPageAddr;
	statePtr->userState.swpBaseAddr = parStatePtr->userState.swpBaseAddr;
	statePtr->userState.swpMaxAddr = parStatePtr->userState.swpMaxAddr;
	statePtr->userState.minSWP = parStatePtr->userState.minSWP;
	statePtr->userState.maxSWP = parStatePtr->userState.maxSWP;
    } else {
	/*
	 * Kernel processes start executing at startPC.
	 */
	statePtr->switchRegState.regs[MACH_INPUT_REG1][0] = (int)startPC;
	statePtr->userState.minSWP = 0;
	statePtr->userState.maxSWP = 0;
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
    procPtr->machStatePtr->userState.trapRegState.regs[MACH_RETURN_VAL_REG][0] = retVal;
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
/*ARGSUSED*/
void
Mach_StartUserProc(procPtr, entryPoint)
    Proc_ControlBlock	*procPtr;	/* Process control block for process
					 * to start. */
    Address		entryPoint;	/* Where process is to start
					 * executing. */
{
    register	Mach_State	*statePtr;

    /*
     * Allocate memory for the saved window stack.
     */
    statePtr = procPtr->machStatePtr;
    if (Vm_PinUserMem(VM_READWRITE_ACCESS, 
			statePtr->userState.maxSWP - statePtr->userState.minSWP,
	                statePtr->userState.minSWP) != SUCCESS) {
	panic( "Mach_StartUserProc: Couldn't pin pages.\n");
    }

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
    if (statePtr->userState.maxSWP != 0) {
	Vm_UnpinUserMem(statePtr->userState.maxSWP - statePtr->userState.minSWP,
		              statePtr->userState.minSWP);
    }
    /*
     * Allocate a new saved window stack.
     */
    statePtr->userState.specPageAddr = (Address)MACH_SAVED_WINDOW_STACK_BASE;
    statePtr->userState.swpBaseAddr = statePtr->userState.specPageAddr +
							    VMMACH_PAGE_SIZE;
    statePtr->userState.swpMaxAddr = (Address) (MACH_SAVED_WINDOW_STACK_BASE +
						MACH_SW_STACK_SIZE);
    statePtr->userState.minSWP = statePtr->userState.swpBaseAddr;
    statePtr->userState.maxSWP = statePtr->userState.swpBaseAddr +
							    VMMACH_PAGE_SIZE;
    if (Vm_PinUserMem(VM_READWRITE_ACCESS, VMMACH_PAGE_SIZE,
	                statePtr->userState.minSWP) != SUCCESS) {
	panic( "Mach_ExecUserProc: Couldn't pin stack.\n");
    }

    regStatePtr = &statePtr->userState.trapRegState;
    regStatePtr->regs[MACH_SPILL_SP][0] = (int)userStackPtr;
    regStatePtr->curPC = entryPoint;
    regStatePtr->nextPC = (Address)0;
    regStatePtr->upsw = 0;
    regStatePtr->kpsw = (
#ifdef WITH_IBUFFER
#ifdef WITH_PREFETCH
			 MACH_KPSW_PREFETCH_ENA | 
#endif
			 MACH_KPSW_IBUFFER_ENA |
#endif
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
    regStatePtr->cwp = 4 << 2;
    regStatePtr->swp = statePtr->userState.swpBaseAddr + 
						2 * MACH_SAVED_WINDOW_SIZE;
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
    bcopy((Address)&statePtr->userState.trapRegState,
	  (Address)&destProcPtr->machStatePtr->userState.trapRegState,
	  sizeof(Mach_RegState));
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
    bcopy((Address)&procPtr->machStatePtr->userState.trapRegState,
	  (Address)&debugStatePtr->regState, sizeof(Mach_RegState));
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
    register	Mach_State	*statePtr;
    int				origKpsw;

/*
 * Bits in the KPSW that the user may set or reset.
 */
#define	USER_KPSW_BITS (MACH_KPSW_PREFETCH_ENA|MACH_KPSW_IBUFFER_ENA\
				|MACH_KPSW_USE_CUR_PC)

    statePtr = procPtr->machStatePtr;
    origKpsw = (statePtr->userState.trapRegState.kpsw & ~USER_KPSW_BITS) | 
			(debugStatePtr->regState.kpsw & USER_KPSW_BITS);
    bcopy((Address)&debugStatePtr->regState,
	 (Address)&statePtr->userState.trapRegState, sizeof(Mach_RegState));
    statePtr->userState.trapRegState.kpsw = origKpsw;
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
    return((Address)procPtr->machStatePtr->userState.trapRegState.regs[MACH_SPILL_SP][0]);
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
	panic( "out-of-order kernel call initialization");
    }
    if (machMaxSysCall >= MAXCALLS) {
	panic( "too many kernel calls");
    }
    if (numArgs > MAXARGS) {
	panic( "too many arguments to kernel call");
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
    machIntrMask |= intrMask;
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
    void          	(*handler)();  	/* Interrupt handling procedure */
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
        *intrNumberPtr = intrType;
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
 * Mach_SetNonmaskableIntr --
 *
 *      Add the non-maskable interrupt mask to the given value.  This will
 *	define which interrupts are non-maskable.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_SetNonmaskableIntr(mask)
    unsigned 	int	mask;
{
    machNonmaskableIntrMask |= mask;
    machIntrMask |= mask;
}

static unsigned int globStatusReg;

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
    register unsigned	int	intrMask;
    register int		intrType;
    unsigned	int	statusReg;

    globStatusReg = intrStatusReg;
    mach_KernelMode[Mach_GetProcessorNumber()] = !(kpsw & MACH_KPSW_PREV_MODE);
    /*
     * Do any nonmaskable interrupts first.
     */ 
    if (intrStatusReg & machNonmaskableIntrMask) { 
	if (intrStatusReg & machDbgInterruptMask) {
	    EnterDebugger(&intrStatusReg);
	}
	if (intrStatusReg & (1 << mach_CpcInterruptNumber)) {
	    machExecuteCall(&intrStatusReg);
	}
	/* Mach_RefreshInterrupt(); */
	intrStatusReg &= ~machNonmaskableIntrMask;
    }
    if (read_physical_word(0x40000) & 0x4) {
	Mach_RefreshInterrupt();
    }
    if (intrStatusReg == 0) {
	return;
    }	
    if (Mach_AtInterruptLevel() || 
	mach_NumDisableInterrupts[Mach_GetProcessorNumber()] > 0) {
	panic( "Non-maskable interrupt while interrupts disabled\n");
    }
    mach_AtInterruptLevel[Mach_GetProcessorNumber()] = TRUE;
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
    mach_AtInterruptLevel[Mach_GetProcessorNumber()] = FALSE;
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
int
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
		/*
 		 * Breakpoint and signal step trap should return to the
		 * current (traping) PC.
		 */
	    procPtr->machStatePtr->userState.trapRegState.kpsw |= 
					MACH_KPSW_USE_CUR_PC;
	    Sig_Send(SIG_BREAKPOINT, SIG_NO_CODE, procPtr->processID, FALSE);
	    break;
	case MACH_SINGLE_STEP:
	    procPtr->machStatePtr->userState.trapRegState.kpsw |= 
					MACH_KPSW_USE_CUR_PC;
	    Sig_Send(SIG_TRACE_TRAP, SIG_NO_CODE, procPtr->processID, FALSE);
	    break;
	default:
	    panic( "MachUserError: Unknown user error %d\n",
				 errorType);
    }
    return(MACH_NORM_RETURN);
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachVMDataFault --
 *
 *      Handle a virtual memory fault on the data of an instruction.
 *
 * Results:
 *      MACH_NORM_RETURN:	Were able to handle the fault or was a 
 *				user process
 *	MACH_FAILED_ARG_FETCH:	Couldn't handle the fault but it occured during
 *				the argument fetch for a system call.
 *	MACH_FAILED_COPY:	Couldn't handle the fault but it occured during
 *				a copy-in or copy-out operation.
 *	MACH_KERN_ACCESS_VIOL:	Was a kernel access violation.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
int
MachVMDataFault(faultType, PC, destAddr, kpsw)
    int		faultType;	/* The type of the VM fault */
    Address	PC;		/* PC of the fault. */
    Address	destAddr;	/* Value of dest/src of store/load instruction*/
    int		kpsw;		/* The KPSW at the time of the fault. */
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();

    if (procPtr == (Proc_ControlBlock *)NIL ||
	(procPtr->genFlags & PROC_KERNEL)) {
	return(MACH_KERN_ACCESS_VIOL);
    }

    if (faultType & MACH_VM_FAULT_REF_BIT) {
	VmMach_SetRefBit(destAddr);
    } else if (faultType & MACH_VM_FAULT_DIRTY_BIT) {
	VmMach_SetModBit(destAddr);
    } else if (faultType & (MACH_VM_FAULT_PROTECTION | 
			    MACH_VM_FAULT_PAGE_FAULT)) {
	/*
	 * We have to handle a protection fault or a page fault.  First try
	 * the PC.
	 */
	if (Vm_PageIn(destAddr, faultType & MACH_VM_FAULT_PROTECTION) !=
								SUCCESS) {
	    if (kpsw & MACH_KPSW_PREV_MODE) {
		procPtr->machStatePtr->userState.faultAddr = destAddr;
		(void)Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL,
			       procPtr->processID, FALSE);
		printf("Process 0x%x fault %d at 0x%x fault address 0x%x\n",
			procPtr->processID, faultType,(unsigned)PC, destAddr);
	    } else {
		if (PC >= (Address)((unsigned int)((int (*)())MachFetchArgStart)) &&
	            PC < (Address)((unsigned int)((int (*)())MachFetchArgEnd))) {
		    return(MACH_FAILED_ARG_FETCH);
		} else if (PC >= (Address)((unsigned int)((int (*)())VmMachDoCopy)) &&
	                   PC < (Address)((unsigned int)((int (*)())VmMachCopyEnd))) {
		    return(MACH_FAILED_COPY);
		} else {
		    return(MACH_KERN_ACCESS_VIOL);
		}
	    }
	}
    }
    return(MACH_NORM_RETURN);
}


/*
 * ----------------------------------------------------------------------------
 *
 * MachVMPCFault --
 *
 *      Handle a Virtual memory fault on the program counter.
 *
 * Results:
 *	MACH_NORM_RETURN:	Handled the fault.
 *	MACH_KERN_ACCESS_VIOL:	Tried to take the fault from kernel mode.
 *	MACH_USER_ACCESS_VIOL:	Couldn't handle the fault and came from user
 *				mode.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
int
MachVMPCFault(faultType, PC, kpsw)
    int		faultType;	/* The type of the VM fault */
    Address	PC;		/* The address of the instruction that caused
				 * the fault. */
    int		kpsw;		/* The KPSW at the time of the fault. */
{
    Proc_ControlBlock	*procPtr;
    extern int	etext;

    if ((kpsw & MACH_KPSW_PREV_MODE) == 0) {
	if (PC > (Address)&etext || PC < (Address)VMMACH_PAGE_SIZE) {
	    return(MACH_KERN_ACCESS_VIOL);
	} else {
	    return(MACH_NORM_RETURN);
	}
    } else {
	/*
  	 * Can't fault on kernel PC. 
	 */
	if (PC < (Address)&etext && PC > (Address)VMMACH_PAGE_SIZE) {
	    return(MACH_NORM_RETURN);
	}
	if (faultType & MACH_VM_FAULT_REF_BIT) {
	    VmMach_SetRefBit(PC);
	    return(MACH_NORM_RETURN);
	} else if (faultType & MACH_VM_FAULT_PAGE_FAULT) {
	    /* 
	     * Take a page fault on this address.
	     */
	    if (Vm_PageIn(PC, FALSE) != SUCCESS) {
		procPtr = Proc_GetCurrentProc();
		procPtr->machStatePtr->userState.faultAddr = PC;
		(void)Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL,
			       procPtr->processID, FALSE);
		printf("Process 0x%x fault on PC at 0x%x\n",
			procPtr->processID,(unsigned int)PC);
		return(MACH_USER_ACCESS_VIOL);
	    } else {
		return(MACH_NORM_RETURN);
	    }
	} else {
	    return(MACH_NORM_RETURN);
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
    Address				usp;

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
	statePtr->userState.trapRegState.regs[MACH_SPILL_SP][0] -=
							sizeof(sigContext);
	usp = (Address)statePtr->userState.trapRegState.regs[MACH_SPILL_SP][0];
	sigContext.machContext.faultAddr = statePtr->userState.faultAddr;
	/*
	 * Copy out the signal context stuff minus the trap state.
	 */
	if (Vm_CopyOut(sizeof(sigContext) - sizeof(Mach_RegState),
		       (Address)&sigContext, usp) != SUCCESS) {
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}
	/*
	 * Copy out the trap register state.
	 */
	usp += (unsigned)&sigContext.machContext.regState -
	       (unsigned)&sigContext;
	if (Vm_CopyOut(sizeof(Mach_RegState),
		       (Address)&statePtr->userState.trapRegState,
		       usp) != SUCCESS) {
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}
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
	if (statePtr->userState.minSWP <= statePtr->userState.swpBaseAddr) {
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
	    statePtr->userState.maxSWP -= VMMACH_PAGE_SIZE;
	    Vm_UnpinUserMem(VMMACH_PAGE_SIZE, statePtr->userState.maxSWP);
	}
	statePtr->userState.minSWP -= VMMACH_PAGE_SIZE;
	if (Vm_PinUserMem(VM_READWRITE_ACCESS, VMMACH_PAGE_SIZE, 
				statePtr->userState.minSWP) != SUCCESS) {
	    printf("Warning: MachGetWinMem: Low pin failed\n");
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}
    } else if (swp > statePtr->userState.maxSWP - 2 * MACH_SAVED_REG_SET_SIZE){
	/*
	 * Need to allocate more at the high end.
	 */
	if (swp > statePtr->userState.maxSWP - MACH_SAVED_REG_SET_SIZE) {
	    panic( "MachGetWinMem: SWP too big.\n");
	}
	if (statePtr->userState.maxSWP - statePtr->userState.minSWP >
							VMMACH_PAGE_SIZE) {
	    /*
	     * Free the lowest page because we never want more than two
	     * full pages at once.
	     */
	    Vm_UnpinUserMem(VMMACH_PAGE_SIZE, statePtr->userState.minSWP);
	    statePtr->userState.minSWP += VMMACH_PAGE_SIZE;
	}
	if (statePtr->userState.maxSWP + VMMACH_PAGE_SIZE >=
					statePtr->userState.swpMaxAddr) {
	    /*
	     * We are already at the top of the stack so we can't allocated
	     * any more memory.
	     */
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}

	if (Vm_PinUserMem(VM_READWRITE_ACCESS, VMMACH_PAGE_SIZE, 
				statePtr->userState.maxSWP) != SUCCESS) {
	    printf("Warning: MachGetWinMem: High pin failed\n");
	    Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	}
	statePtr->userState.maxSWP += VMMACH_PAGE_SIZE;
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
MachSigReturn()
{
    register Mach_State	*statePtr;
    Sig_Stack		sigStack;
    Sig_Context		sigContext;
    Proc_ControlBlock	*procPtr;
    Address		usp;
    int			savedKPSW;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;
    usp = (Address)statePtr->userState.trapRegState.regs[MACH_SPILL_SP][0];
    /*
     * Copy in the normal context stuff.
     */
    if (Vm_CopyIn(sizeof(sigContext) - sizeof(Mach_RegState), usp,
	    (Address)&sigContext) != SUCCESS) {
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    /*
     * Now copy in the register state.
     */
    savedKPSW = statePtr->userState.trapRegState.kpsw & ~MACH_KPSW_USE_CUR_PC;
    usp += (unsigned)&sigContext.machContext.regState -
	   (unsigned)&sigContext;
    if (Vm_CopyIn(sizeof(Mach_RegState), usp,
	          (Address)&statePtr->userState.trapRegState) != SUCCESS) {
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    statePtr->userState.trapRegState.regs[MACH_SPILL_SP][0] +=
							sizeof(sigContext);
    
    statePtr->userState.trapRegState.kpsw = savedKPSW |
	    (statePtr->userState.trapRegState.kpsw & MACH_KPSW_USE_CUR_PC);
    sigStack.contextPtr = &sigContext;
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
    if (mach_KernelMode[processor]) {
	return(MACH_KERNEL);
    } else {
	return(MACH_USER);
    }
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

static	ReturnStatus	PinSWP();

/*
 *----------------------------------------------------------------------
 *
 * MachUserContextSwitch --
 *
 *	Perform a user level context switch.
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
MachUserContextSwitch(specPageAddr, swpMaxAddr, newSWP)
    register	Address	specPageAddr;	/* The address of the new special page.
					 * The base of the saved window stack
					 * is specPageAddr + VMMACH_PAGE_SIZE.*/
    Address		swpMaxAddr;	/* The new max address of the saved 
					 * window stack. */
    Address		newSWP;		/* The value of the new swp. */
{
    register	Mach_SpecPage	*specPagePtr;
    register	Mach_State	*statePtr;
    register	Address		swpBaseAddr;
    Address			minSWP;
    Address			maxSWP;
    Proc_ControlBlock		*procPtr;
    ReturnStatus		status = SUCCESS;
    int				savedKPSW;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;

    /*
     * Page align the spec page addr and then add one page to it to point
     * to the actual base of the saved window stack.
     */
    specPageAddr = (Address)((unsigned)specPageAddr & ~(VMMACH_PAGE_SIZE - 1));
    swpBaseAddr = specPageAddr + VMMACH_PAGE_SIZE;

    status = PinSWP(swpBaseAddr, swpMaxAddr, newSWP, &minSWP, &maxSWP);
    if (status != SUCCESS) {
	goto done;
    }

    /*
     * Copy the current state out onto the current stack.
     */
    specPagePtr = (Mach_SpecPage *)statePtr->userState.specPageAddr;
    if (Vm_CopyOut(sizeof(Mach_RegState),
		   (Address)&statePtr->userState.trapRegState,
		   (Address)&specPagePtr->switchState.regState) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
	Vm_UnpinUserMem(maxSWP - minSWP, minSWP);
	goto done;
    }
    /*
     * Put out the current bounds of the saved window stack.  We don't have
     * to use copy-out here because we know that *specPagePtr points to a valid
     * page because otherwise we wouldn't have gotten this far.
     */
    specPagePtr->switchState.specPageAddr = statePtr->userState.specPageAddr;
    specPagePtr->switchState.swpMaxAddr = statePtr->userState.swpMaxAddr;

    /*
     * Copy in the new state.
     */
    specPagePtr = (Mach_SpecPage *)specPageAddr;
    savedKPSW = statePtr->userState.trapRegState.kpsw;
    if (Vm_CopyIn(sizeof(Mach_RegState),
		  (Address)&specPagePtr->switchState.regState,
		  (Address)&statePtr->userState.trapRegState) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
	Vm_UnpinUserMem(maxSWP - minSWP, minSWP);
	goto done;
    }
    statePtr->userState.trapRegState.kpsw = savedKPSW | 
	    (statePtr->userState.trapRegState.kpsw & MACH_KPSW_USE_CUR_PC);
    /*
     * Free up the old stack and store the pointers to the new one.
     */
    Vm_UnpinUserMem(statePtr->userState.maxSWP - statePtr->userState.minSWP,
		    statePtr->userState.minSWP);
    statePtr->userState.specPageAddr = specPageAddr;
    statePtr->userState.swpBaseAddr = swpBaseAddr;
    statePtr->userState.swpMaxAddr = swpMaxAddr;
    statePtr->userState.minSWP = minSWP;
    statePtr->userState.maxSWP = maxSWP;

done:
    statePtr->userState.trapRegState.regs[MACH_RETURN_VAL_REG][0] = status;
    return(MACH_NORM_RETURN);
}


/*
 *----------------------------------------------------------------------
 *
 * PinSWP --
 *
 *	Pin down new saved window stack.
 *
 * Results:
 *	SYS_ARG_NOACCESS if couldn't wire down the saved window stack,
 *	SUCCESS otherwise.
 *
 * Side effects:
 *	*minSWPPtr and *maxSWPPtr are set to point to the minimum
 *	and maximum address that we pinned.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PinSWP(swpBaseAddr, swpMaxAddr, swp, minSWPPtr, maxSWPPtr)
    register	Address	swpBaseAddr;
    register	Address	swpMaxAddr;
    register	Address	swp;
    register	Address	*minSWPPtr;
    register	Address	*maxSWPPtr;
{
    if (swp < swpBaseAddr || swp >= swpMaxAddr) {
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Compute the new minimum and maximum addresses for the saved window
     * stack.
     */
    *minSWPPtr = (Address) ((unsigned)(swp - MACH_SAVED_WINDOW_SIZE) &
				      ~(VMMACH_PAGE_SIZE - 1));
    *maxSWPPtr = swp + 2 * MACH_SAVED_REG_SET_SIZE + VMMACH_PAGE_SIZE -
						    MACH_SAVED_WINDOW_SIZE;
    *maxSWPPtr = (Address) ((unsigned)*maxSWPPtr & ~(VMMACH_PAGE_SIZE - 1));
    if (*minSWPPtr < swpBaseAddr || *maxSWPPtr > swpMaxAddr) {
	return(SYS_ARG_NOACCESS);
    }
    /*
     * Pin down the new stack.
     */
    if (Vm_PinUserMem(VM_READWRITE_ACCESS, *maxSWPPtr - *minSWPPtr,
		      *minSWPPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * MachSaveUserState --
 *
 *	Save the current user processes state into the special page.
 *
 * Results:
 *	MACH_NORM_RETURN
 *
 * Side effects:
 *	The user's special page saved state is modified.
 *
 *----------------------------------------------------------------------
 */
int
MachSaveUserState()
{
    register	Mach_State	*statePtr;
    Proc_ControlBlock		*procPtr;
    Mach_SpecPage		*specPagePtr;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;
    specPagePtr = (Mach_SpecPage *)statePtr->userState.specPageAddr;

    if (Vm_CopyOut(sizeof(Mach_RegState),
		        (Address)&statePtr->userState.trapRegState,
		    (Address)&specPagePtr->savedState.regState) != SUCCESS) {
	panic( "MachSaveUserState: Save failed\n");
    }
    /*
     * Put out the current bounds of the saved window stack.  We don't have
     * to use copy-out here because we know that *specPagePtr points to a valid
     * page because otherwise we wouldn't have gotten this far.
     */
    specPagePtr->savedState.specPageAddr = (Address)specPagePtr;
    specPagePtr->savedState.swpMaxAddr = statePtr->userState.swpMaxAddr;

    return(MACH_NORM_RETURN);
}

/*
 *----------------------------------------------------------------------
 *
 * MachRestoreUserState --
 *
 *	Restore the current user processes state from the special page.
 *
 * Results:
 *	MACH_NORM_RETURN
 *
 * Side effects:
 *	The user's special page saved state is modified.
 *
 *----------------------------------------------------------------------
 */
int
MachRestoreUserState()
{
    register	Mach_State	*statePtr;
    register	Mach_SpecPage	*specPagePtr;
    Proc_ControlBlock		*procPtr;
    int				savedKPSW;
    Address			minSWP;
    Address			maxSWP;
    ReturnStatus		status;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;
    specPagePtr = (Mach_SpecPage *)statePtr->userState.specPageAddr;

    savedKPSW = statePtr->userState.trapRegState.kpsw;
    /*
     * Copy in the state to restore.  Note that once we can copy in the
     * state we know that we can get the rest without a copy-in because
     * the page must be valid if the copy-in succeeds.
     */
    if (Vm_CopyIn(sizeof(Mach_RegState),
		  (Address)&specPagePtr->savedState.regState,
		  (Address)&statePtr->userState.trapRegState) != SUCCESS) {
	panic( "MachRestoreUserState: Restore failed\n");
    }
    statePtr->userState.specPageAddr =
		    (Address)((unsigned)specPagePtr->savedState.specPageAddr & 
					~(VMMACH_PAGE_SIZE - 1));
    statePtr->userState.swpBaseAddr = 
			statePtr->userState.specPageAddr + VMMACH_PAGE_SIZE;
    statePtr->userState.swpMaxAddr = specPagePtr->savedState.swpMaxAddr;

    statePtr->userState.trapRegState.kpsw = savedKPSW | 
	    (statePtr->userState.trapRegState.kpsw & MACH_KPSW_USE_CUR_PC);
    status = PinSWP(statePtr->userState.swpBaseAddr, 
		    statePtr->userState.swpMaxAddr, 
		    statePtr->userState.trapRegState.swp, &minSWP, &maxSWP);
    if (status != SUCCESS) {
	Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
    }
    Vm_UnpinUserMem(statePtr->userState.maxSWP - statePtr->userState.minSWP,
		    statePtr->userState.minSWP);
    statePtr->userState.minSWP = minSWP;
    statePtr->userState.maxSWP = maxSWP;

    return(MACH_NORM_RETURN);
}

/*
 *----------------------------------------------------------------------
 *
 * MachUserTestAndSet --
 *
 *	User level test and set.  This is a hack for now until they figure
 *	out how to do a real test and set at user level in hardware
 *	(won't work with interrupts enable).
 *
 * Results:
 *	MACH_NORM_RETURN
 *
 * Side effects:
 *	The given pointer is test and set and the user's return val reg
 *	is set to the value returned by test and set.
 *
 *----------------------------------------------------------------------
 */
int
MachUserTestAndSet(tasAddr)
    Address	tasAddr;	/* Address to test and set. */
{
    ReturnStatus	status;
    Proc_ControlBlock	*procPtr;
    Mach_State		*statePtr;

    procPtr = Proc_GetCurrentProc();
    statePtr = procPtr->machStatePtr;

    status = Vm_PinUserMem(VM_READWRITE_ACCESS, 4, tasAddr);
    if (status == SUCCESS) {
	statePtr->userState.trapRegState.regs[27][0] = Mach_TestAndSet(tasAddr);
    }
    statePtr->userState.trapRegState.regs[28][0] = status;
    Vm_UnpinUserMem(4, tasAddr);
    return(MACH_NORM_RETURN);
}


/*
 *----------------------------------------------------------------------
 *
 * MachEnterKernelDebugger --
 *
 *	Enter the kernel debugger. This routine is called when a processor
 * 	hits a break_point trap or single step trap or is signaled into the
 *	debugger by another processor.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void 
MachEnterKernelDebugger(signal, statePtr)
    int			signal;		/* Signal causing the trap. */
    Mach_RegState	*statePtr;	/* State of the processor. */
{
    static	int	debuggerSemaphore = 0;	
    Boolean	firstInDebugger;	/* Is this the first processor to
					 * enter debugger. */
    int		pnum;

    /*
     * If we are already in  debugger then ignore the interrupt.
     */
    if (mach_ProcessorStatus[Mach_GetProcessorNumber()] == MACH_IN_DEBUGGER_STATUS) {
	return;
    }
    /*
     * See if this is the first processor to enter the debugger.
     */
    firstInDebugger = (Boolean) (Mach_TestAndSet(&debuggerSemaphore) == 0);

    /*
     * Update our register state.
     */
    machDebugSignal[Mach_GetProcessorNumber()] = signal;
    DevCCReadBytesFromRegs(0,DEV_CC_MAX_OFFSET, 
			&(machCCState[Mach_GetProcessorNumber()]));
    /*
     * Mark our state as in the debugger.
     */
    mach_ProcessorStatus[Mach_GetProcessorNumber()] = MACH_IN_DEBUGGER_STATUS;

    /*
     * If we are the first processor, signal the rest of the processors.
     */
    if (firstInDebugger) {
	for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
	    if (mach_ProcessorStatus[pnum] == MACH_ACTIVE_STATUS) {
		Mach_SignalProcessor(pnum, machDbgInterruptNumber);
	    }
	}
    }
    /*
     * If we are the master processor, wait a little while for the rest of
     * the procesors to enter the debugger and then wait kdbx.
     */
    if (Mach_GetProcessorNumber() == machMasterProcessor) {
	Boolean	allInDebugger = FALSE;
	int	count = 0;
	for (count = 0; !allInDebugger && (count < 10); count++) {
	    MACH_DELAY(100);
	    allInDebugger = TRUE;
	    for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
		if (mach_ProcessorStatus[pnum] == MACH_ACTIVE_STATUS) {
		    allInDebugger = FALSE;
		}
	    }
	    REFRESH_CPU();
	}
	/*
	 * Mark processor that wont enter debugger as hung.
	 */
	if (!allInDebugger) {
	   for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
		if (mach_ProcessorStatus[pnum] == MACH_ACTIVE_STATUS) {
		    printf("Warning: Processor %d wont enter debugger.\n",
				pnum);
		    mach_ProcessorStatus[pnum] = MACH_HUNG_STATUS;
		}
	    }
	}
	kdb(signal,statePtr);
	/*
	 * Clear the debugger semaphore.
	 */
	debuggerSemaphore = 0;
    } else {
	/*
	 * If we are not the master processor, spin waiting for the
	 * master to change our state. We must continue to refresh the
	 * dynamic registers during the wait.
	 */
	register Mach_ProcessorStatus	* volatile spinWaitPtr;
	register int	count;

	spinWaitPtr = &(mach_ProcessorStatus[Mach_GetProcessorNumber()]);
	count = 0;
	while (*spinWaitPtr == MACH_IN_DEBUGGER_STATUS) {
		count++;
		if (count > 10000) {
		    count = 0;
		    REFRESH_CPU();
		}
	}
    }
   mach_ProcessorStatus[Mach_GetProcessorNumber()] = MACH_ACTIVE_STATUS;
}


/*
 *----------------------------------------------------------------------
 *
 * MachContinueProcessors --
 *
 *	Continue all processors in the debugger.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void 
MachContinueProcessors()
{
    int		pnum;

    /*
     * If not master process, then do nothing.
     */
    if (Mach_GetProcessorNumber() != machMasterProcessor) {
	return;
    }
    /*
     * Slave processors in the debugger spin waiting for there state to
     * change.
     */
    for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
	if (mach_ProcessorStatus[pnum] == MACH_IN_DEBUGGER_STATUS) {
		mach_ProcessorStatus[pnum] = MACH_CONTINUING_STATUS;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * EnterDebugger --
 *
 *	Called when a processor receives a enter debugger signal.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void 
EnterDebugger(statusRegPtr)
    unsigned int	*statusRegPtr;
{
    DBG_CALL;
}

/*
 * Start of First process for each processor.
 */
Mach_State *machFirstProcState[MACH_MAX_NUM_PROCESSORS];


/*
 *----------------------------------------------------------------------
 *
 * Mach_SpinUpProcessor(pnum) --
 *
 *	Start up processor pnum
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Mach_SpinUpProcessor(pnum,procPtr)
    int			pnum;		/* Processor number. */
    Proc_ControlBlock	*procPtr;	/* Initial processor for processor. */
{
    ReturnStatus	status;
    Mach_Board		board;
    int			i;
    /*
     * Verify that the specified processor is in the system.
     */
    status = Mach_FindBoardDescription(MACH_CONFIG_CPU_BOARD,pnum,FALSE,
					&board);
    if (status != SUCCESS) {
	printf("Warning: Trying to spin up nonexistant processor (%d)\n",
		pnum);
	return(status);
    }

    if (mach_ProcessorStatus[pnum] != MACH_UNINITIALIZED_STATUS) {
	printf("Warning: Trying to spin up running processor (%d)\n",pnum);
	return (FAILURE);
    }

    /* 
     * Set up the first process for processor and assign processor number.
     */
    machFirstProcState[pnum] = procPtr->machStatePtr;
    proc_RunningProcesses[pnum] = procPtr;
    machCurStatePtrs[pnum] = procPtr->machStatePtr;
    /*
     * Wake up the processor. 
     */
    status = Mach_SignalProcessor(pnum, MACH_SPINUP_INTERRUPT_NUM);
    if (status != SUCCESS) {
	printf("Warning: Can't signal to wake up processor %d\n",pnum);
	return (status);
    }
    return (SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_GetNumProcessor() --
 *
 *	Return the number of processors in the system.
 *
 * Results:
 *
 * Side effects:
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
 * Mach_InitSlaveProcessor() --
 *
 *	Initialized the machine state of a slave (non-master) processor.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void 
Mach_InitSlaveProcessor()
{
    /*
     * Turn off all timers.
     */
    Mach_Write8bitCCReg(MACH_MODE_REG,0);
    /*
     * Clear the interrupt mask register and any pending interrupts.
     */
    Mach_Write32bitCCReg((unsigned int)MACH_INTR_MASK_0,(unsigned int)0);
    Mach_Write32bitCCReg((unsigned int)MACH_INTR_STATUS_0,(unsigned int)-1);
    /*
     * Start refersh interrupt.
     */
    Mach_RefreshStart();
    /*
     * Set status to active.
     */
    mach_ProcessorStatus[Mach_GetProcessorNumber()] = MACH_ACTIVE_STATUS;
    /*
     * Increment the number of active processors.  This code assumes that
     * processors are started one at a time so we don't need protection on
     * the mach_NumProcessors variable.
     */
     /* mach_NumProcessors += 1; */
    if (mach_NumProcessors < Mach_GetProcessorNumber()+1) {
    	mach_NumProcessors = Mach_GetProcessorNumber()+1;
    }

}

static void doNothing()
{
	return;
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_CheckSpecialHandling() --
 *
 *	Force the specified processor to check it special handling flag
 *	of the executing process.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

Mach_CheckSpecialHandling(processorNum)
    int	processorNum;	/* Processor number to act on. */
{ 
    Mach_SignalProcessor(processorNum, machCheckSpecialInterruptNum);

}


#ifdef PATCH_IBUFFER

#define	IBUF_TRACE
#ifdef IBUF_TRACE

#define	TRACE_BUF_SIZE 128
static int	trace_end = 0;
static struct ibuf_trace {
    int	   r10;	
    int	   r16;
    int	   r26;
    int	   choice;	
} ibuf_trace[TRACE_BUF_SIZE];
#endif
/*
 * Some ipatch stats.
 */
struct ipatch_stats {
    int		numCalls;	/* Number calls to PatchIbuffer */
    int		numUserCalls;	/* Number calls to patch user Ibuffer. */
    int		numCallsWithIbuffer;	/* Number calls with ibuffer enabled. */
    int		numUserCallsWithIbuffer;/* Number calls with ibuffer enabled.*/
    int		numR10zero;		/* Number r10 == zero. */
    int		numR26zero;		/* Number r26 == zero. */
    int		numR10fault;		/* Number r10 not valid. */
    int		numR26fault;		/* Number r26 not valid. */
    int		numGoodInst;		/* Number of harmless inst. */
    int		numR10notCall;		/* Number of r10 not call inst. */
    int		numR26notCall;		/* Number of r26 not call inst. */
} ipatch_stats;



/*
 *----------------------------------------------------------------------
 *
 * MachPatchIbuffer() --
 *
 *	Return the correct current PC after an interrupt.  When the
 *	ibuffer is enabled, the current PC of an interrupt is written in
 *	one of two places.  This routine returns the correct curPC.
 *
 * Results: 
 *	The correct current PC of a trap.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
int 
MachPatchIbuffer(kpsw,r10,r16,r26)
    int	kpsw;		/* The kpsw at trap time. */
    int	r10;		/* R10 of the trap window. */
    int	r16;		/* R16 of the trap window. */
    int	r26;		/* R26 of the trap window. */
{
    register unsigned int	nextInst, nextOPCode;
    register unsigned int	curInst, curOPCode;
    int		r10notvalid = 0;
    ipatch_stats.numCalls++;
    /*
     * If the ibuffer is not enabled then the current PC is always in
     * R10.
     */
    if (!(kpsw & MACH_KPSW_IBUFFER_ENA)) {
	return r10;
    }
    ipatch_stats.numCallsWithIbuffer++;

    /*
     * If the ibuffer is enabled then the current PC is either in R10 or
     * R16.  If r10 is 0 then current PC must be in R26.  If R26 is also
     * zero something is very wrong.
     */
    if (r10 == 0) {
	if (r26 == 0) {
	    panic("No current pc in MachPatchIbuffer.");
	} else {
	    ipatch_stats.numR10zero++;
	    return r26;
	}
    }
    /*
     * If R10 is not zero but R26 is zero then R10 be the correct current PC.
     */
    if (r26 == 0) {
	ipatch_stats.numR26zero++;
	return r10;
    }
    /*
     * If R10 doesn't point to a validate address, it can't be the correct
     * current PC. (This doesn't work for multiprocessors!!!)
     * THIS is also not true for uniprocessors.
     */

    if (!Vm_ValidateRange(r10,4)) {
	ipatch_stats.numR10fault++;
	r10notvalid = 1;
    }

	/*
 	 * If r16 is not valid we must assume that the pc was put in the 
	 * correct place.
	 */
    if (!Vm_ValidateRange(r16,4)) {
	return r10;
    }
    /*
     * If current PC is only messed up when the next pc instruction is one
     * of the follow:
     *	1) a call
     *	2) a return
     *  3) a return_trap
     *	4) a wr_special	of cwp
     * If it is not one of this instructions then r10 is the current PC.
     */
    nextInst = (* (unsigned int *) r16);
    nextOPCode = nextInst >> 25;
    if (!(((nextOPCode >= 0x70) && (nextOPCode <= 0x77)) || 	/* A call */
	(nextOPCode == 0x58)		       ||	/* A return */
	(nextOPCode == 0x59)				/* A return_trap */
		)) {		/* Should also check wr_special cwp. */
	ipatch_stats.numGoodInst++;
	return r10;
    }
    /*
     * If the r26 is not accessible then r10 must be the current PC.
     */
    if (!Vm_ValidateRange(r26,4)) {
	ipatch_stats.numR26fault++;
	return r10;
    }
    if ((r10&~3)+4 == (r16&~3)) {
	return r10;
    }
    /*
     * Check the instruction pointed to by r10. If this instruction is
     * call instruction we assume that the trap wrote the current PC into
     * r26. This follows because the instruction before a call or return
     * is never a call.
     */
    if (!r10notvalid) {
	    curInst = (* (unsigned int *) r10);
	    curOPCode = curInst >> 25;
	    if (!((curOPCode >= 0x70) && (curOPCode <= 0x77))) { 
		/* Not a call inst. */
		ipatch_stats.numR10notCall++;
		return r10;
	    }
     }
    curInst = (* (unsigned int *) r26);
    curOPCode = curInst >> 25;
    if (((curOPCode >= 0x70) && (curOPCode <= 0x77))) { /* Is a call inst. */
	panic("Both current PCs are calls in  MachPatchIbuffer.");
    }
    ipatch_stats.numR26notCall++;
    return r26;
}

/*
 *----------------------------------------------------------------------
 *
 * MachPatchUserModeIbuffer() --
 *
 *	Patch the current PC in the Mach_State structure of the currently
 *	executing process.
 *
 * Results: 
 *	None.
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

int 
MachPatchUserModeIbuffer(kpsw)
    unsigned int	kpsw;		/* Trap KPSW */
{
    Mach_State	*statePtr;
    int errorCurPC;
#ifdef IBUF_TRACE
    int		index;
#endif

    ipatch_stats.numUserCalls++;
    /*
     * If the ibuffer is not enabled then no patch is necessary.
     */
    statePtr = machCurStatePtrs[Mach_GetProcessorNumber()];
    if ((kpsw & MACH_KPSW_IBUFFER_ENA)) {
	ipatch_stats.numUserCallsWithIbuffer++;
	/*
	 * The trapping R26 was stuffed into trapRegState.aligner by 
	 * SaveState.
	 */
	errorCurPC =  * (int *) &(statePtr->userState.trapRegState.aligner);
#ifdef IBUF_TRACE
	index = trace_end++;
	trace_end = trace_end % TRACE_BUF_SIZE;
	ibuf_trace[index].r10 = (int) statePtr->userState.trapRegState.curPC;
	ibuf_trace[index].r16 = (int) statePtr->userState.trapRegState.nextPC;
	ibuf_trace[index].r26 = errorCurPC;
	ibuf_trace[index].choice = (int)
#endif
	statePtr->userState.trapRegState.curPC = (Address)
	    MachPatchIbuffer(kpsw, statePtr->userState.trapRegState.curPC,
				    statePtr->userState.trapRegState.nextPC,
				    errorCurPC);
    }
    return (int) statePtr->userState.trapRegState.curPC;

}


/*
 *----------------------------------------------------------------------
 *
 * MachPatchIbufferOnFault() --
 *
 *	Return the correct current PC after an fault.  When the
 *	ibuffer is enabled, the current PC of an fault is written in
 *	one of two places.  This routine returns the correct curPC.
 *
 * Results: 
 *	The correct current PC of a trap.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
int 
MachPatchIbufferOnFault(kpsw,r10,r16,r26)
    int	kpsw;		/* The kpsw at trap time. */
    int	r10;		/* R10 of the trap window. */
    int	r16;		/* R16 of the trap window. */
    int	r26;		/* R26 of the trap window. */
{
    register unsigned int	nextInst, nextOPCode;
    register unsigned int	curInst, curOPCode;
    ipatch_stats.numCalls++;
    /*
     * If the ibuffer is not enabled then the current PC is always in
     * R10.
     */
    if (!(kpsw & MACH_KPSW_IBUFFER_ENA)) {
	return r10;
    }
    ipatch_stats.numCallsWithIbuffer++;

    /*
     * If the ibuffer is enabled then the current PC is either in R10 or
     * R16.  If r10 is 0 then current PC probably is in R26.  If R26 is also
     * zero something is  wrong.
     */
    if (r10 == 0) {
	if (r26 == 0) {
	    return r10;
	} else {
	    return r26;
	}
    }
    /*
     * If R10 is not zero but R26 is zero then R10 be the correct current PC.
     */
    if (r26 == 0) {
	return r10;
    }
    /*
     * If the next PC is not accessable, then assume trap put PC in correct
     * registers. 

    if (!Vm_ValidateRange(r16,4)) {
	return r10;
    }
    /*
     * If current PC is only messed up when the next pc instruction is one
     * of the follow:
     *	1) a call
     *	2) a return
     *  3) a 
     *	4) a wr_special	of cwp
     * If it is not one of this instructions then r10 is the current PC.
     */
    nextInst = (* (unsigned int *) r16);
    nextOPCode = nextInst >> 25;
    if (!(((nextOPCode >= 0x70) && (nextOPCode <= 0x77)) || 	/* A call */
	(nextOPCode == 0x58)		       ||	/* A return */
	(nextOPCode == 0x59)				/* A return_trap */
		)) {		/* Should also check wr_special cwp. */
	ipatch_stats.numGoodInst++;
	return r10;
    }
    if ((r16&~3) == ((r10&~3) + 4)) {
	return r10;
    } 
    if ((r16&~3) == ((r26&~3) + 4)) {
	return r26;
    }
    return r10;
}

/*
 *----------------------------------------------------------------------
 *
 * MachPatchUserModeIbufferOnFault() --
 *
 *	Patch the current PC in the Mach_State structure of the currently
 *	executing process.
 *
 * Results: 
 *	None.
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

int 
MachPatchUserModeIbufferOnFault(kpsw)
    unsigned int	kpsw;		/* Trap KPSW */
{
    Mach_State	*statePtr;
    int errorCurPC;
#ifdef IBUF_TRACE
    int		index;
#endif

    ipatch_stats.numUserCalls++;
    /*
     * If the ibuffer is not enabled then no patch is necessary.
     */
    statePtr = machCurStatePtrs[Mach_GetProcessorNumber()];
    if ((kpsw & MACH_KPSW_IBUFFER_ENA)) {
	ipatch_stats.numUserCallsWithIbuffer++;
	/*
	 * The trapping R26 was stuffed into trapRegState.aligner by 
	 * SaveState.
	 */
	errorCurPC =  * (int *) &(statePtr->userState.trapRegState.aligner);
#ifdef IBUF_TRACE
	index = trace_end++;
	trace_end = trace_end % TRACE_BUF_SIZE;
	ibuf_trace[index].r10 = (int) statePtr->userState.trapRegState.curPC;
	ibuf_trace[index].r16 = (int) statePtr->userState.trapRegState.nextPC;
	ibuf_trace[index].r26 = errorCurPC;
	ibuf_trace[index].choice = (int)
#endif
	statePtr->userState.trapRegState.curPC = (Address)
	    MachPatchIbuffer(kpsw, statePtr->userState.trapRegState.curPC,
				    statePtr->userState.trapRegState.nextPC,
				    errorCurPC);
    }
    return (int) statePtr->userState.trapRegState.curPC;

}
#endif

