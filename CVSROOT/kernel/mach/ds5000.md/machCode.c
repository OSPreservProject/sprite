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

#include <sprite.h>
#include <stddef.h>
#include <assert.h>

#include <machConst.h>
#include <machMon.h>
#include <machInt.h>
#include <mach.h>
#include <sys.h>
#include <sync.h>
#include <dbg.h>
#include <proc.h>
#include <procMigrate.h>
#include <procUnixStubs.h>
#include <sched.h>
#include <vm.h>
#include <vmMach.h>
#include <sig.h>
#include <sigMach.h>
#include <swapBuffer.h>
#include <net.h>
#include <ultrixSignal.h>

/*
 * Flag to set for new Unix compatiblity code.  Once the new
 * code is working well enough, this flag should  be eliminated
 * and the old compatibility code in machAsm.s, machUNIXSyscall.c etc.
 * can be deleted.
 */

int machNewUnixCompat;

/*
 * Conversion of function to an unsigned value.
 */
#define F_TO_A	(Address)(unsigned)(int (*)())
/*
 *  Number of processors in the system.
 */
#ifndef NUM_PROCESSORS
#define NUM_PROCESSORS 1
#endif

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

char *mach_MachineType = "ds5000";

extern int debugProcStubs;

/*
 * The byte ordering/alignment type used with Fmt_Convert and I/O control data
 */
Fmt_Format mach_Format = FMT_MIPS_FORMAT;

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

char	mach_BitmapAddr[16];
char	mach_BitmapLen[8];

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
MachStringTable	machMonBootParam;	/* Parameters from boot line. */
/* 
 * Pointer to the state structure for the current process and the
 * current owner of the floating point unit.
 */
Mach_State	*machCurStatePtr = (Mach_State *)NIL;
Mach_State	*machFPCurStatePtr = (Mach_State *)NIL;


extern void PrintError _ARGS_((void));
static void PrintInst _ARGS_((unsigned pc, unsigned inst));
static void SoftFPReturn _ARGS_((void));
static void MemErrorInterrupt _ARGS_((void));

ReturnStatus	(*machInterruptRoutines[MACH_NUM_HARD_INTERRUPTS]) _ARGS_((
		    unsigned int statusReg, unsigned int causeReg, 
		    Address pc, ClientData data));
void		(*machIOInterruptRoutines[MACH_NUM_IO_SLOTS]) _ARGS_((
		    unsigned int statusReg, unsigned int causeReg, 
		    Address pc, ClientData data));
ClientData	machIOInterruptArgs[MACH_NUM_IO_SLOTS];
ClientData	machInterruptArgs[MACH_NUM_HARD_INTERRUPTS];

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

static void SetupSigHandler _ARGS_((register Proc_ControlBlock *procPtr, 
			register SignalStack *sigStackPtr, Address pc));
static void ReturnFromSigHandler _ARGS_((register Proc_ControlBlock *procPtr));
static ReturnStatus Interrupt _ARGS_((unsigned statusReg, unsigned causeReg, 
			Address pc));
static ReturnStatus MachStdHandler _ARGS_((unsigned int statusReg, 
			unsigned int causeReg, Address pc, ClientData data));
static ReturnStatus MachIOInterrupt _ARGS_((unsigned int statusReg, 
			unsigned int causeReg, Address pc, ClientData data));
static ReturnStatus MachMemInterrupt _ARGS_((unsigned int statusReg, 
			unsigned int causeReg, Address pc, ClientData data));
extern ReturnStatus MachFPInterrupt _ARGS_((unsigned int statusReg, 
			unsigned int causeReg, Address pc, ClientData data));

/*
 * Preallocate all machine state structs.
 */
Mach_State	machStateTable[VMMACH_MAX_KERN_STACKS];
int		nextStateIndex = 0;

/*
 *  Save the bad address that caused an exception. This makes debugging
 *  of TLB misses easier.
 */

Address		machBadVaddr = (Address) NIL;

/*
 * Set to TRUE if we are inside of Mach_Probe.
 */

Boolean		machInProbe = FALSE;

/*
 * Read address of Mach_Probe.
 */

Address		machProbeReadAddr = (Address) NIL;
typedef struct trace {
    char	*srcPtr;
    char	*destPtr;
    int		dest1;
    int		dest2;
    int		src;
} Trace;

extern Trace *ncopyBuffer;
extern int ncopyCount;
extern int ncopyPath;

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
Mach_Init(boot_argc,boot_argv)
int	boot_argc;	/* Argc from boot sequence. */
MachStringTable	*boot_argv;	/* Boot sequence strings. */
{
    extern char end[], edata[];
    int offset, i;
    char buf[256];
    volatile unsigned int 	*csrPtr = (unsigned int *) MACH_CSR_ADDR;
    char	*copyPtr;

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

    /*
     * Copy the boot parameter structure. The original location will get
     * unmapped during vm initialization so we need to get our own copy.
     * Depending on how the machine was booted, the boot arguments 
     * may or may not be parsed.  So we'll glob them all together and
     * then parse them.
     */
    buf[0] = '\0';
    for (i = 0; i < boot_argc; i++) {
	strcat(buf,boot_argv->argPtr[i]);
	strcat(buf," ");
    }
    Mach_ArgParse(buf,&machMonBootParam);

    /*
     * Get information on the memory bitmap.  This gets clobbered later.
     */
    /*
     * IMPORTANT NOTE:  Don't use bcopy to do this, at least while
     * bcopy contains the calls to Vm_CheckAccessible.  Those calls add a
     * frame to the stack, causing it to overwrite the very information
     * we're trying to copy.  JHH 4/18/91
     */
    copyPtr = Mach_MonGetenv("bitmaplen");
    for(i = 0; i < sizeof(mach_BitmapLen); i++) {
	mach_BitmapLen[i] = *copyPtr++;
    }
    copyPtr = Mach_MonGetenv("bitmap");
    for(i = 0; i < sizeof(mach_BitmapAddr); i++) {
	mach_BitmapAddr[i] = *copyPtr++;
    }

    /*
     * Set up the CSR.  Turn on memory ECC. Interrupts from IO slots are
     * turned on as handlers are registered. 
     */
    *csrPtr = MACH_CSR_CORRECT;


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
     * Set all interrupt handlers to a default, then install handlers for
     * those interrupts processed by the mach module.
     */
    for (i = 0; i < MACH_NUM_HARD_INTERRUPTS; i++ ) {
	Mach_SetHandler(i, MachStdHandler, (ClientData) i);
    }
    Mach_SetHandler(MACH_IO_INTR, MachIOInterrupt, NIL);
    Mach_SetHandler(MACH_MEM_INTR, MachMemInterrupt, NIL);
    Mach_SetHandler(MACH_FPU_INTR, MachFPInterrupt, NIL);

    /*
     * Clear out the IO interrupt handlers.
     */
    for (i = 0; i < MACH_NUM_IO_SLOTS; i++ ) {
	Mach_SetIOHandler(i, (void (*)()) NIL, (ClientData) NIL);
    }
    /*
     * Enable the memory interrupt, because we need it for Mach_Probe to
     * work.  Disabling interrupts has the side-effect of enabling the
     * memory interrupt.
     */
    Mach_DisableIntr();
    /*
     * Clear out the i and d caches.
     */
    MachConfigCache();
    MachFlushCache();
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_SetHandler --
 *
#ifndef ds5000
 *	Put a interrupt handling routine into the table.
#else
 *	Register an interrupt handler for R3000 interrupts.  Interrupt
 *	handlers are of the form:
#endif
 *
#ifdef ds5000
 *	void
 *	Handler(statusReg, causeReg, pc, data)
 *		unsigned int	statusReg;	Status register.
 *		unsigned int	causeReg;	Cause register.
 *		Address		pc;		PC where the interrupt 
 *						occurred.
 *		ClientData	data		Callback data
 *	
 *
#endif
 * Results:
 *     None.
 *
 * Side effects:
 *     The interrupt handling table is modified.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_SetHandler(level, handler, clientData)
    int level;			/* Interrupt level. */
    ReturnStatus (*handler) _ARGS_((unsigned int statusReg, 
			    unsigned int causeReg,
			    Address pc, ClientData data)); 
			        /* Interrupt handler. */
    ClientData	clientData; 	/* Data to pass handler. */
{
    /*
     * Check that it is valid.  
     */
    if ((level < 0) || (level >= MACH_NUM_HARD_INTERRUPTS)){
	panic("Warning: Bad interrupt level %d\n", level);
    } else {
	machInterruptRoutines[level] = handler;
	machInterruptArgs[level] = clientData;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MachStdHandler --
 *
 *	The default handler for hard interrupts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out an error message.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
MachStdHandler(statusReg, causeReg, pc, data)
    unsigned int	statusReg;	/* Status register. */
    unsigned int	causeReg;	/* Cause register. */
    Address		pc;		/* PC. */
    ClientData		data;		/* Interrupt level. */
{
    int		level;

    level = (int) data;
    printf("WARNING: no handler for level %d interrupt!\n", level);
    return MACH_OK;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_SetIOHandler --
 *
 *	Register an interrupt handler for IO device interrupts.  All IO
 *	device interrupts are merged into the same R3000 interrupt
 *	(level 0). Interrupt handlers are of the form:
 *
 *	void
 *	Handler(data)
 *		ClientData	data		Callback data
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The IO interrupt handling table is modified.  If the handler is not
 *	NIL then the IO interrupt for the slot is turned on in the CSR.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_SetIOHandler(slot, handler, clientData)
    int slot;			/* Interrupt number to set */
    void (*handler) _ARGS_((ClientData data)); /* Interrupt handler. */
    ClientData	clientData; 	/* Data to pass handler. */
{
    volatile unsigned int 	*csrPtr = (unsigned int *) MACH_CSR_ADDR;
    unsigned int		mask;
    /*
     * Check that it is valid.  
     */
    if ((slot < 0) || (slot >= MACH_NUM_IO_SLOTS)){
	panic("Warning: Bad slot number %d\n",slot);
    } else {
	if ((machIOInterruptRoutines[slot] != (void (*)()) NIL) &&
	    (handler != (void (*)()) NIL)) {
	    Mach_MonPrintf("WARNING: replacing existing handler for slot %d.\n",
		slot);
	}
	machIOInterruptRoutines[slot] = handler;
	machIOInterruptArgs[slot] = clientData;
	if (handler != (void (*)()) NIL) {
	    mask = ((1 << slot) << MACH_CSR_IOINTEN_SHIFT);
	    if (!(mask & MACH_CSR_IOINTEN)) {
		panic("Goof up computing interrupt enable bits in csr.\n");
	    }
	    *csrPtr |= mask;
	}
    }
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

    assert(offsetof(Proc_ControlBlock, unixErrno) == MACH_UNIX_ERRNO_OFFSET);
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
Mach_SetReturnVal(procPtr, retVal, retVal2)
    Proc_ControlBlock	*procPtr;	/* Process to set return value for. */
    int			retVal;		/* Value for process to return. */
    int			retVal2;	/* Second return value. */
{
    procPtr->machStatePtr->userState.regState.regs[V0] = (unsigned)retVal;
    procPtr->machStatePtr->userState.regState.regs[V1] = (unsigned)retVal2;
}

/*----------------------------------------------------------------------------
 *
 * Mach_Return2 --
 *
 *      Set the second return value for Unix compat. routines that
 *      return two values.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      v1 <- val
 *
 *----------------------------------------------------------------------------
 */
void
Mach_Return2(val)
int val;
{
    Proc_GetActualProc()->machStatePtr->userState.regState.regs[V1] =
	    (unsigned)val;
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

    machBadVaddr = badVaddr;
    cause = (causeReg & MACH_CR_EXC_CODE) >> MACH_CR_EXC_CODE_SHIFT;
    if (cause != MACH_EXC_INT) {
	Mach_EnableIntr();
    }

    procPtr = Proc_GetActualProc();
    switch (cause) {
	case MACH_EXC_INT:
	    status = Interrupt(statusReg, causeReg, pc);
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
				procPtr->processID, FALSE, badVaddr);
	    }
	    break;
	case MACH_EXC_TLB_LD_MISS:
	case MACH_EXC_TLB_ST_MISS:
	    if (VmMach_TLBFault(badVaddr) != SUCCESS) {
		printf("Bad user TLB fault in process %x: pc=%x addr=%x\n",
			procPtr->processID, pc, badVaddr);
		(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
				procPtr->processID, FALSE, badVaddr);
	    }
	    break;
	case MACH_EXC_ADDR_ERR_LD:
	case MACH_EXC_ADDR_ERR_ST:
	    printf("Address fault in process %x: pc=%x addr=%x\n",
			procPtr->processID, pc, badVaddr);
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE, badVaddr);
	    break;
	case MACH_EXC_BUS_ERR_IFETCH:
	    printf("MachExceptionHandler: User bus error on ifetch");
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE, badVaddr);
	    break;
	case MACH_EXC_BUS_ERR_LD_ST:
	    printf("MachExceptionHandler: User bus error on ld or st");
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
			    procPtr->processID, FALSE, badVaddr);
	    break;
	case MACH_EXC_SYSCALL:
	    if (!MachUNIXSyscall()) {
		printf("MachExceptionHandler: Bad syscall magic for proc %x\n",
							procPtr->processID);
		(void) Sig_Send(SIG_ILL_INST, SIG_BAD_TRAP,
				procPtr->processID, FALSE, pc);
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
#if 0
	    printf("Pc = 0x%x, Instruction = 0x%x\n", pc, inst);
	    printf("Cause register = 0x%x\n", causeReg);
#endif
	    switch ((int)(inst & MACH_BREAK_CODE_FIELD)) {
		case MACH_BREAKPOINT_VAL:
		    Proc_Lock(procPtr);
		    if (procPtr->genFlags & PROC_DEBUG_ON_EXEC) {
			procPtr->genFlags &= ~PROC_DEBUG_ON_EXEC;
			(void) Sig_SendProc(procPtr, SIG_DEBUG, 
					    SIG_NO_CODE, pc);
		    } else {
			(void) Sig_SendProc(procPtr, SIG_BREAKPOINT, 
					    SIG_NO_CODE, pc);
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
		    (void) Sig_SendProc(procPtr, SIG_TRACE_TRAP, SIG_NO_CODE,
			    pc);
		    Proc_Unlock(procPtr);
		    break;
		}

		case MACH_SIG_RET_VAL:
		    ReturnFromSigHandler(procPtr);
		    break;
		default:
		    printf("Bogus bp-trap\n");
		    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
				    procPtr->processID, FALSE, pc);
		    break;
	    }
	    break;
	}
	case MACH_EXC_RES_INST:
	    printf("Reserved instruction in process %x at pc=%x\n",
			procPtr->processID, pc);
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE, pc);
	    break;
	case MACH_EXC_COP_UNUSABLE:
	    MachSwitchFPState(machFPCurStatePtr, machCurStatePtr);
	    machFPCurStatePtr = machCurStatePtr;
	    break;
	case MACH_EXC_OVFLOW:
	    printf("Overflow exception in process %x at pc=%x\n",
			procPtr->processID, pc);
	    (void) Sig_Send(SIG_ARITH_FAULT, SIG_OVERFLOW,
			    procPtr->processID, FALSE, pc);
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
    machBadVaddr = badVaddr;

    /*
     * Process kernel traps.
     */
    procPtr = Proc_GetActualProc();
    switch (cause) {
	case MACH_EXC_INT:
	    status = Interrupt(statusReg, causeReg, pc);
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
		if (procPtr != (Proc_ControlBlock *)NIL) {
		    printf("procPtr->vmPtr->numMakeAcc = %d\n", 
			procPtr->vmPtr->numMakeAcc);
		}
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
		    printf("badVaddr = 0x%x\n", badVaddr);
		    return(MACH_KERN_ERROR);
		}
	    } else {
		return(MACH_OK);
	    }
	}

	case MACH_EXC_ADDR_ERR_LD:
	    printf("MachKernelExceptionHandler: %s: addr: %x PC: %x\n",
		    "Address error on load", badVaddr, pc);
	    return(MACH_KERN_ERROR);
	case MACH_EXC_ADDR_ERR_ST:
	    printf("MachKernelExceptionHandler:  Address error on store\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_BUS_ERR_IFETCH:
	    printf("MachKernelExceptionHandler:  Bus error on ifetch\n");
	    return(MACH_KERN_ERROR);
	case MACH_EXC_BUS_ERR_LD_ST:
	    if (machInProbe) {
		return(MACH_OK);
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
static ReturnStatus
Interrupt(statusReg, causeReg, pc)
    unsigned	statusReg;
    unsigned	causeReg;
    Address	pc;
{
    int		n;
    unsigned	mask;
    ReturnStatus status = MACH_OK;

    mask = (causeReg & statusReg & MACH_CR_INT_PENDING) >> 
						MACH_CR_HARD_INT_SHIFT;
    mach_KernelMode = !(statusReg & MACH_SR_KU_PREV);
    mach_AtInterruptLevel = 1;

    /*
     * If there is a memory interrupt pending, and we have all other
     * interrupts off, then only handle that interrupt.
     */

    if (mach_NumDisableIntrsPtr[Mach_GetProcessorNumber()] > 0) {
	if (mask & (MACH_INT_MASK_3 >> MACH_CR_HARD_INT_SHIFT)) {
	    status = machInterruptRoutines[3](statusReg, causeReg, pc, 
		    machInterruptArgs[3]); 
	    goto exit;
	} else {
	    panic("Interrupt while at interrupt level, (0x%x) pc 0x%x.\n", 
		mask, pc);
	}
    }
    n = 0;
    while (mask != 0) {
	if (mask & 1) {
	    (void) machInterruptRoutines[n](statusReg, causeReg, pc, 
		machInterruptArgs[n]); 
	}
	mask >>= 1;
	n++;
    }
exit:
    mach_AtInterruptLevel = 0;
    return(status);
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
    SignalStack	sigStack;
    Address	pc;
    int		restarted = 0;


    if (procPtr->Prof_Scale >= 2 && procPtr->Prof_PC != 0) {
	Prof_RecordPC(procPtr);
    }

    if (procPtr->unixProgress != PROC_PROGRESS_NOT_UNIX &&
	    procPtr->unixProgress != PROC_PROGRESS_UNIX && debugProcStubs) {
	printf("UnixProgress = %d entering MachUserReturn\n",
		procPtr->unixProgress);
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
	    if (procPtr->unixProgress == PROC_PROGRESS_RESTART ||
		    procPtr->unixProgress > 0) {
		/*
		 * If we received a normal signal, we want to restart
		 * the system call when we leave.
		 * If we received a migrate signal, we will get here on
		 * the new machine.
		 */

		/*
		 * Mangle the PC so we restart the trap after we leave
		 * the kernel.
		 */
		restarted = 1;
		if (debugProcStubs) {
		    printf("Restarting system call with progress %d\n",
			    procPtr->unixProgress);
		    printf("Our PC = %x\n",
			    machCurStatePtr->userState.regState.pc);
		}
		machCurStatePtr->userState.regState.pc -= 4;
		if (debugProcStubs) {
		    printf("Now our PC = %x\n",
			    machCurStatePtr->userState.regState.pc);
		    printf("V0 was %d and our call was %d\n", 
			    machCurStatePtr->userState.regState.regs[V0],
			    machCurStatePtr->userState.unixRetVal);
		}
		machCurStatePtr->userState.regState.regs[V0] =
		    machCurStatePtr->userState.unixRetVal;
		procPtr->unixProgress = PROC_PROGRESS_UNIX;
	    }
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
	    } else if (restarted && debugProcStubs) {
		printf("No signal, yet we restarted system call!!?!\n");
	    }
	}
    }
    
    /*
     * It is possible for Sig_Handle to mask the migration signal
     * if a process is not in a state where it can be migrated.
     * As soon as we return to user mode, though, we will allow migration.
     */
    Sig_AllowMigration(procPtr);

    if (procPtr->unixProgress != PROC_PROGRESS_NOT_UNIX &&
	    procPtr->unixProgress != PROC_PROGRESS_UNIX && debugProcStubs) {
	printf("UnixProgress = %d leaving MachUserReturn\n", procPtr->unixProgress);
    }

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
    if (procPtr->unixProgress != PROC_PROGRESS_NOT_UNIX) {
	int unixSignal;
	if (Compat_SpriteSignalToUnix(sigStackPtr->sigStack.sigNum,
		&unixSignal) != SUCCESS) {
	    printf("Signal %d invalid in SetupSigHandler\n",
		    sigStackPtr->sigStack.sigNum);
	} else {
	    userStatePtr->regState.regs[A0] = unixSignal;
	}
    } else {
	userStatePtr->regState.regs[A0] = sigStackPtr->sigStack.sigNum;
    }
    userStatePtr->regState.regs[A1] = sigStackPtr->sigStack.sigCode;
    userStatePtr->regState.regs[A2] = usp + MACH_STAND_FRAME_SIZE;
    userStatePtr->regState.regs[A3] = sigStackPtr->sigStack.sigAddr;
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
    return SYS_DS5000;
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
    int		i;
    int		offset;

    bcopy(machMonBootParam.strings, buffer, 
	  (bufferSize < 256) ? bufferSize : 256);
    offset = (int) machMonBootParam.strings - (int) buffer;
    for(i = 0; i < argc; i++) {
	if (machMonBootParam.argPtr[i] == (char *)NULL) break;
	argv[i] = (char *) (machMonBootParam.argPtr[i] - (char *) offset);
    }
    return i;
}

/*
 *----------------------------------------------------------------------
 *
#ifndef ds5000
 * Mach_GetEtherAddress --
#else
 * MachMemInterrupt --
#endif
 *
#ifndef ds5000
 *	Return the ethernet address out of the rom.
#else
 *	Handle an interrupt from the memory controller.
#endif
 *
 * Results:
#ifndef ds5000
 *	Number of elements returned in argv.
#else
 *	None.
#endif
 *
 * Side effects:
#ifndef ds5000
 *	*etherAddrPtr gets the ethernet address.
#else
 *	None.
#endif
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
MachMemInterrupt(statusReg, causeReg, pc, data)
    unsigned	statusReg;		/* Status register. */
    unsigned	causeReg;		/* Cause register. */
    Address	pc;			/* PC. */
    ClientData	data;			/* Callback data. */

{
    volatile unsigned int *erradrPtr = 
	    (volatile unsigned int *) MACH_ERRADR_ADDR;
    unsigned int erradr;
    volatile unsigned int *chksynPtr = 
	    (volatile unsigned int *) MACH_CHKSYN_ADDR;
    unsigned int chksyn;
    unsigned int address;
    int		column;
    ReturnStatus	status = MACH_OK;

    erradr = *erradrPtr;
    if (!(erradr & MACH_ERRADR_VALID)) {
	printf("Received memory interrupt but ERRADR not valid.\n");
	return;
    }
    address = erradr & MACH_ERRADR_ADDRESS;
    switch(erradr & (MACH_ERRADR_CPU | MACH_ERRADR_WRITE | MACH_ERRADR_ECCERR)){
	case 0: {
	    /*
	     * For IO space addresses the 27 bits in the ERRADR must be
	     * shifted by 2, then the top bits set.  This isn't documented
	     * anywhere.
	     */
	    address = MACH_IO_SLOT_BASE | (address << 2);
	    panic("DMA read overrun at address 0x%x\n", address);
	    break;
	} 
	case (MACH_ERRADR_ECCERR) :  {
	    /*
	     * Compensate for the address pipeline. 
	     * See page 26 of the functional spec.
	     */
	    column = (int) (address & 0xfff); 
	    column -= 5;
	    address = (address & ~0xfff) | (unsigned int) column;
	    printf("ECC read error during DMA at address 0x%x\n", address);
	    break;
	}
	case (MACH_ERRADR_WRITE) : {
	    address = MACH_IO_SLOT_BASE | (address << 2);
	    panic("DMA write overrun at address 0x%x, pc = 0x%x\n", 
	    address, pc);
	    break;
	} 
	case (MACH_ERRADR_WRITE | MACH_ERRADR_ECCERR) : {
	    printf("Holy bogus hardware, Batman!\n");
	    printf("We got an illegal value in the ERRADR status register.\n");
	    break;
	}
	case (MACH_ERRADR_CPU) : {
	    address = MACH_IO_SLOT_BASE | (address << 2);
	    if (machInProbe) {
		status = MACH_USER_ERROR;
		machInProbe = FALSE;
		break;
	    }
	    panic(
	"Timeout during CPU read of IO address 0x%x, pc = 0x%x\n", 
	address, pc);
	    Mach_SendSignal(MACH_SIGILL);
	    break;
	}
	case (MACH_ERRADR_CPU | MACH_ERRADR_ECCERR) : {
	    /*
	     * Compensate for the address pipeline. 
	     * See page 26 of the functional spec.
	     */
	    column = (int) (address & 0xfff); 
	    column -= 5;
	    address = (address & ~0xfff) | (unsigned int) column;
	    printf("ECC read error during CPU access of address 0x%x\n", 
		address);
	    break;
	}
	case (MACH_ERRADR_CPU | MACH_ERRADR_WRITE) : {
	    address = MACH_IO_SLOT_BASE | (address << 2);
	    if (machInProbe) {
		status = MACH_USER_ERROR;
		machInProbe = FALSE;
		break;
	    }
	    panic(
	"Timeout during CPU write of IO address 0x%x\n, pc = 0x%x\n", 
	address, pc);
	    break;
	}
	case (MACH_ERRADR_CPU | MACH_ERRADR_WRITE | MACH_ERRADR_ECCERR) : {
	    printf("ECC partial memory write error by CPU at address 0x%x\n",
		address);
	    break;
	}
    }
    if (erradr & MACH_ERRADR_ECCERR) {
	chksyn = *chksynPtr;
	if (address & 0x1) {
	    printf("ECC error was in the high bank.\n");
	    printf("%s bit error\n", (chksyn & MACH_CHKSYN_SNGHI) ? 
		"single" : "multiple");
	    printf("Syndrome bits = 0x%x\n", chksyn & MACH_CHKSYN_SYNHI);
	    if (chksyn & MACH_CHKSYN_VLDHI) {
		printf("Check bits = 0x%x\n", chksyn & MACH_CHKSYN_CHKHI);
	    } else {
		printf("Check bits not valid.\n");
	    }
	} else {
	    printf("ECC error was in the low bank.\n");
	    printf("%s bit error\n", (chksyn & MACH_CHKSYN_SNGLO) ? 
		"single" : "multiple");
	    printf("Syndrome bits = 0x%x\n", chksyn & MACH_CHKSYN_SYNLO);
	    if (chksyn & MACH_CHKSYN_VLDLO) {
		printf("Check bits = 0x%x\n", chksyn & MACH_CHKSYN_CHKLO);
	    } else {
		printf("Check bits not valid.\n");
	    }
	}
    }
    /*
     * Clear the ERRADR and CHKSYN registers.
     */
    *erradrPtr = 0;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
#ifndef ds5000
 * MemErrorInterrupt --
#else
 * MachIOInterrupt --
#endif
 *
#ifndef ds5000
 *	Handler an interrupt for the DZ device.
#else
 *	Handle an interrupt from one of the IO slots.
#endif
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
MachIOInterrupt(statusReg, causeReg, pc, data)
    unsigned int	statusReg;	/* Status register. */
    unsigned int	causeReg;	/* Cause register. */
    Address		pc;		/* PC. */
    ClientData		data;		/* Not used. */
{
    volatile unsigned int 	*csrPtr = (unsigned int *) MACH_CSR_ADDR;
    unsigned int 		csr;
    int				slot;
    unsigned int		ioint;
    int				i;

    csr = *csrPtr;
    ioint = csr & MACH_CSR_IOINT;
    if (ioint == 0) {
	panic("No interrupt pending on an IO slot.\n");
    }
    for (slot = 0; ioint != 0; slot++, ioint >>= 1) {
	if (ioint & 1) {
	    if (machIOInterruptRoutines[slot] != (void (*)()) NIL) {
		machIOInterruptRoutines[slot](machIOInterruptArgs[slot]);
	    }
	}
    }
    return MACH_OK;
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
			    procPtr->processID, FALSE, (Address)0);
	    break;
	case MACH_SIGILL:
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE, (Address)0);
	    break;
	default:
	    panic("Mach_SendSignal: Bad signal type\n");
	    break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_ArgParseCode --
 *
 *	Parse an argument string.
 *      Note: this replaces the DECstation argvize prom call that has
 *	been disabled on the ds5000 for no good reason.
 *
 * Results:
 *	Returns argc,argv in table.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Mach_ArgParseCode(string,table)
char *string;
MachStringTable *table;
{
    char *ptr1, *ptr2,*end2;
    table->num = 0;
    ptr1 = string;
    ptr2 = table->strings;
    end2 = table->strings+256;
    table->argPtr[0] = ptr2;
    for (;*ptr1 != 0 && ptr2<end2; ptr2++) {
	if (*ptr1 == ' ') {
	    while(*ptr1 == ' ') {
		ptr1++;
	    }
	    if (*ptr1 == '\0') {
		break;
	    }
	    *ptr2 = '\0';
	    table->num++;
	    if (table->num>=16) break;
	    table->argPtr[table->num] = ptr2+1;
	} else {
	    *ptr2 = *ptr1;
	    ptr1++;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_GetEtherAddress --
 *
 *	Returns the ethernet address of the first ethernet interface.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*etherAddrPtr gets the ethernet address.
 *
 *----------------------------------------------------------------------
 */

void
Mach_GetEtherAddress(etherAddrPtr)
    Net_EtherAddress	*etherAddrPtr;	/* Place to put ethernet address. */
{
    Net_Interface	*interPtr;

    interPtr = Net_GetInterface(NET_NETWORK_ETHER, 0);
    if (interPtr != (Net_Interface *) NIL) {
	NET_ETHER_ADDR_COPY(interPtr->netAddress[NET_PROTO_RAW].ether,
	    *etherAddrPtr);
    }
}

#define READ_ROM(from, to, count) { 			\
    int	_i;						\
    for(_i = 0; _i < (count); _i++) {			\
	status = Mach_Probe(1, (from), &((to)[_i]));	\
	if (status != SUCCESS) {			\
	    return status;				\
	}						\
	(from) += 4;					\
    }							\
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_GetSlotInfo --
 *
 *	Read the standard information from the ROM in a TURBOchannel
 * 	slot.  Any trailing spaces are converted to null characters.
 *	See page 13 of the TURBOchannel Hardware Specification for
 * 	details on the format of the ROM.
 *
 * Results:
 *	SUCCESS if the ROM was read ok, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Mach_GetSlotInfo(romAddr, infoPtr)
    char		*romAddr;	/* ROM address. */
    Mach_SlotInfo	*infoPtr;	/* Slot information. */
{
    ReturnStatus	status = SUCCESS;
    unsigned char	value;
    int			i;


    /*
     * Move to the start of the standard TURBOchannel rom info. 
     */
    romAddr += MACH_IO_ROM_OFFSET + 0x10;
    /*
     * Check the test pattern.
     */
    READ_ROM(romAddr, &value, 1);
    if (value != 0x55) {
	return FAILURE;
    }
    READ_ROM(romAddr, &value, 1);
    if (value != 0x00) {
	return FAILURE;
    }
    READ_ROM(romAddr, &value, 1);
    if (value != 0xaa) {
	return FAILURE;
    }
    READ_ROM(romAddr, &value, 1);
    if (value != 0xff) {
	return FAILURE;
    }
    /*
     * Everything looks cool so read out the info.
     */
    READ_ROM(romAddr, infoPtr->revision, 8);
    READ_ROM(romAddr, infoPtr->vendor, 8);
    READ_ROM(romAddr, infoPtr->module, 8);
    READ_ROM(romAddr, infoPtr->type, 4);
    /*
     * Get rid of trailing spaces
     */
    for (i = 7; i >= 0; i--) {
	if (infoPtr->revision[i] == ' ') {
	    infoPtr->revision[i] = '\0';
	}
	if (infoPtr->revision[i] != '\0') {
	    break;
	} 
    }
    for (i = 7; i >= 0; i--) {
	if (infoPtr->vendor[i] == ' ') {
	    infoPtr->vendor[i] = '\0';
	}
	if (infoPtr->vendor[i] != '\0') {
	    break;
	} 
    }
    for (i = 7; i >= 0; i--) {
	if (infoPtr->module[i] == ' ') {
	    infoPtr->module[i] = '\0';
	}
	if (infoPtr->module[i] != '\0') {
	    break;
	} 
    }
    for (i = 3; i >= 0; i--) {
	if (infoPtr->type[i] == ' ') {
	    infoPtr->type[i] = '\0';
	}
	if (infoPtr->type[i] != '\0') {
	    break;
	} 
    }
    /*
     * Make sure all the strings are null terminated.
     */
    infoPtr->revision[8] = '\0';
    infoPtr->vendor[8] = '\0';
    infoPtr->module[8] = '\0';
    infoPtr->type[4] = '\0';
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_SigreturnStub --
 *
 *	Procedure to map from Unix sigreturn system call to Sprite.
 *	On the decstation, this is used for returning from a signal.
 *	Note: This routine is exactly the same as MachUNIXLongJumpReturn.
 *	Presumably the other routine will go away as soon as Unix
 *	compatibility is working.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Mach_SigreturnStub(sigContextPtr)
    struct sigcontext *sigContextPtr;
{
    struct sigcontext	sigContext;
    Mach_RegState	*regsPtr;
    int			dummy;
    ReturnStatus	status;
    extern Mach_State   *machCurStatePtr;

    status = Vm_CopyIn(sizeof(struct sigcontext), (Address)sigContextPtr,
		       (Address)&sigContext);
    if (status != SUCCESS) {
	return(status);
    }
    regsPtr = &machCurStatePtr->userState.regState;
    regsPtr->pc = (Address)(sigContext.sc_pc - 4);
    bcopy(sigContext.sc_regs, regsPtr->regs, sizeof(sigContext.sc_regs));
    regsPtr->mflo = sigContext.sc_mdlo;
    regsPtr->mfhi = sigContext.sc_mdhi;
    bcopy(sigContext.sc_fpregs, regsPtr->fpRegs, sizeof(sigContext.sc_fpregs));
    regsPtr->fpStatusReg = sigContext.sc_fpc_csr;
    Proc_GetCurrentProc()->sigHoldMask = sigContext.sc_mask;
    return(SUCCESS);
}
