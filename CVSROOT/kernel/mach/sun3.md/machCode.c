/* 
 * excCode.c --
 *
 *     Contains actual declarations for some global variables, the 
 *     exception vector initialization routines, and the trap handler.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "exc.h"
#include "sys.h"
#include "sync.h"
#include "machine.h"
#include "dbg.h"
#include "proc.h"
#include "procMigrate.h"
#include "sched.h"
#include "vm.h"
#include "sig.h"
#include "sunSR.h"
#include "mem.h"
#include "sunMon.h"

/*
 * Declare global variables.
 */

int	exc_Type;

extern	int ExcGetVBR();
Exc_VectorTable	*exc_VectorTablePtr;


/*
 * ----------------------------------------------------------------------------
 *
 * Exc_Init --
 *
 *     Initialize the exception vector table.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The exception vector table is initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Exc_Init()
{
    int	*vecTablePtr;
    int	*protoVecTablePtr;
    int	i;

    vecTablePtr = (int *) ExcGetVBR();
    exc_VectorTablePtr = (Exc_VectorTable *) vecTablePtr;
    protoVecTablePtr = (int *) &exc_ProtoVectorTable;
    for (i = 0; i < EXC_NUM_EXCEPTIONS; i++) {
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
    for (i = EXC_NUM_EXCEPTIONS ; i<256 ; i++) {
	extern int Exc_BrkptTrap();

	*vecTablePtr = (int)Exc_BrkptTrap;
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
    ExcSetVBR(mach_KernStart);
    exc_VectorTablePtr = (Exc_VectorTable *) mach_KernStart;
#endif

}

/*
 * ----------------------------------------------------------------------------
 *
 * Exc_SetHandler --
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
Exc_SetHandler(vectorNumber, handler)
    int vectorNumber;	/* Vector number that the device generates */
    int (*handler)();	/* Interrupt handling procedure */
{
    int	*vecTablePtr;

    if (vectorNumber < 64 || vectorNumber > 255) {
	Sys_Printf("%d: ", vectorNumber);
	Sys_Panic(SYS_WARNING, "Bad vector number\n");
    } else {
	vecTablePtr = (int *) ExcGetVBR();
	vecTablePtr[vectorNumber] = (int)handler;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Exc_Trap --
 *
 *      The trap handler routine.  This deals with supervisor mode and 
 *	non-supervisor mode traps differently.  The only allowed supervisor
 *	mode traps are a breakpoint trap to force a context switch and a
 *	bus error in the middle of a cross address space copy. All other
 *	traps go into the debugger.  However, all types of user traps are 
 *	processed here.
 *
 * Results:
 *      EXC_KERN_ERROR if the debugger should be called after this routine 
 *	returns, EXC_USER_ERROR if a copy to/from user space caused an 
 *	unrecoverable bus error, and EXC_OK if everything worked out ok.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

int
Exc_Trap(trapStack)
    Exc_TrapStack	trapStack;
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status;

    procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());

    /*
     * Process kernel traps.
     */

    if (trapStack.excStack.statusReg & SUN_SR_SUPSTATE) {
	switch (trapStack.trapType) {
	    case EXC_TRACE_TRAP:
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
		if (!(trapStack.excStack.statusReg & SUN_SR_TRACEMODE)) {
		    return(EXC_OK);
		}

		/*
		 * In the normal case enter the debugger with a breakpoint
		 * trap.
		 */
		return(EXC_KERN_ERROR);

	    case EXC_BUS_ERROR:

		if (trapStack.busErrorReg.timeOut) {
		    /*
		     * Allow for refresh memory time just like Unix.
		     */
		    DELAY(2000);
		}

		/*
		 * Check to see if is a parity error.
		 */

#ifndef SUN3
		if (trapStack.busErrorReg.parErrU ||
		    trapStack.busErrorReg.parErrL) {
		    Sys_Panic(SYS_FATAL, "Parity error!!!\n");
		    return(EXC_KERN_ERROR);
		}

		if (trapStack.busErrorReg.busErr) {
		    Sys_Panic(SYS_FATAL, "System bus error\n");
		    return(EXC_KERN_ERROR);
		}
#endif

		/*
		 * Mousetrap.
		 */
		if (procPtr == (Proc_ControlBlock *) NIL) {
		    Mon_Printf("Exc_Trap: PC = %x, addr = %x BR Reg %x\n",
			    trapStack.excStack.pc,
			    trapStack.excStack.tail.addrBusErr.faultAddr,
			    *(short *) &trapStack.busErrorReg);
		    Sys_Panic(SYS_FATAL, "Exc_Trap: current process is nil.\n");
		}

		if (procPtr->genFlags & PROC_USER) {
		    Boolean	protError;
		    /*
		     * Is a page fault on a user process while executing in
		     * the kernel.  This can happen on calls to 
		     * Vm_Copy{In,Out} (indicated by the VM_COPY_IN_PROGRESS
		     * flag) or after a pointer is made accessible by 
		     * Vm_MakeAccessible (indicated by numMakeAcc > 0).
		     */
		    if (!(procPtr->vmPtr->vmFlags & VM_COPY_IN_PROGRESS) &&
			procPtr->vmPtr->numMakeAcc == 0) {
			return(EXC_KERN_ERROR);
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
			if (procPtr->vmPtr->vmFlags & VM_COPY_IN_PROGRESS) {
			    /*
			     * A Vm_Copy{In,Out} is in progress so return
			     * an error to the copying process.  The size of the
			     * trap stack is put into saved reg D0 so the trap
			     * handler knows how much stack to blow away.
			     */
			    switch (trapStack.excStack.vor.stackFormat) {
				case EXC_MC68010_BUS_FAULT:
				    trapStack.genRegs[D0] = 
				    		EXC_MC68010_BUS_FAULT_SIZE;
				    break;
				case EXC_SHORT_BUS_FAULT:
				    trapStack.genRegs[D0] = 
						EXC_SHORT_BUS_FAULT_SIZE;
				    break;
				case EXC_LONG_BUS_FAULT:
				    trapStack.genRegs[D0] = 
						EXC_LONG_BUS_FAULT_SIZE;
				    break;
				default:
				    Sys_Panic(SYS_FATAL, "Exc_Trap: Bad stack format.\n");
			    }
			    return(EXC_USER_ERROR);
			} else {
			    /*
			     * Real kernel error.  Take a long jump if
			     * possible.
			     */
			    if (procPtr->setJumpStatePtr != 
						(Sys_SetJumpState *) NIL) {
				Sys_LongJump(procPtr->setJumpStatePtr);
			    }
			    return(EXC_KERN_ERROR);
			}
		    } else {
			return(EXC_OK);
		    }
		} else {
		    /*
		     * Happened to a kernel process.  Take a long jump if 
		     * possible.
		     */
		    if (procPtr->setJumpStatePtr != (Sys_SetJumpState *) NIL) {
			Sys_LongJump(procPtr->setJumpStatePtr);
		    }
		    return(EXC_KERN_ERROR);
		}
	    case EXC_SPURIOUS_INT:
		/*
		 * Ignore this for now because otherwise we can't debug mint
		 */
		if (!dbg_BeingDebugged) {
		    Sys_Printf("Exc_Trap: Spurious interrupt\n");
		}
		return(EXC_OK);

	    default:
		return(EXC_KERN_ERROR);
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
    trapStack.excStack.statusReg &= ~SUN_SR_TRACEMODE;

    switch (trapStack.trapType) {
	case EXC_BUS_ERROR: {
	    Boolean	protError;
	    if (trapStack.busErrorReg.timeOut) {
		/*
		 * Allow for refresh memory time just like Unix.
		 */

		DELAY(2000);
	    }

	    /*
	     * Check for parity error.
	     */

#ifndef SUN3
	    if (trapStack.busErrorReg.parErrU ||
	        trapStack.busErrorReg.parErrL) {
		Sys_Panic(SYS_FATAL, "Parity error!!!\n");
		return(EXC_KERN_ERROR);
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
		Sys_Printf(
		    "Exc_Trap: Bus error in user proc %X, PC = %x, addr = %x BR Reg %x\n",
			    procPtr->processID, 
			    trapStack.excStack.pc,
			    trapStack.excStack.tail.addrBusErr.faultAddr,
			    *(short *) &trapStack.busErrorReg);
		(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, 
				procPtr->processID, FALSE);
	    }
	    break;
	}
	case EXC_SYSCALL_TRAP:
	    /*
	     * Perform a system call.
	     */
	     if (Sys_SysCall(&trapStack) != SUCCESS) {
		Sys_Printf("Bad system call\n");
		(void) Sig_Send(SIG_ILL_INST, SIG_BAD_SYS_CALL,
				procPtr->processID, FALSE);
	    }
	    break;
		
	case EXC_BRKPT_TRAP:
	    Proc_Lock(procPtr);
	    if (procPtr->genFlags & PROC_DEBUG_ON_EXEC) {
	    	procPtr->genFlags &= ~PROC_DEBUG_ON_EXEC;
		(void) Sig_SendProc(procPtr, SIG_DEBUG, SIG_NO_CODE);
	    } else {
		(void) Sig_SendProc(procPtr, SIG_BREAKPOINT, SIG_NO_CODE);
	    }
	    Proc_Unlock(procPtr);
	    break;

	case EXC_SIG_RET_TRAP: {
	    /*
	     * We got a return from signal trap.  The old exception stack
	     * that caused the signal in the first place is retrieved from
	     * the user stack.  Then the stack pointer (SP) and two
	     * registers a0 and d0 are set up so that the trap handler
	     * code can restore the old exception stack.
	     */

	    Address		oldStackAddr;
	    Exc_TrapStack	*oldStackPtr;
	    int			curSize;
	    int			oldSize;

	    Sig_Return(procPtr, &trapStack, &oldStackAddr);

	    oldStackPtr = (Exc_TrapStack *) Mem_Alloc(sizeof(Exc_TrapStack));
	    if (Vm_CopyIn(sizeof(Exc_TrapStack), oldStackAddr,
			  (Address) oldStackPtr) != SUCCESS) {
		Sys_Printf("Sig_Return: Bad signal stack.\n");
		Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	    }

	    curSize = Exc_GetTrapStackSize(&trapStack);
	    oldSize = Exc_GetTrapStackSize(oldStackPtr);
	    if (oldSize == -1) {
		Sys_Printf("Exc_Code: Bad signal stack type.\n");
		Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	    }
	    /*
	     * Make sure that the user didn't set the supervisor bit in
	     * the status register.
	     */
	    if (trapStack.excStack.statusReg & SUN_SR_SUPSTATE) {
		Sys_Printf("Exc_Code: User set kernel bit on signal stack\n");
		Proc_ExitInt(PROC_TERM_DESTROYED, PROC_BAD_STACK, 0);
	    }
	    trapStack.genRegs[A1] = (int)&trapStack + curSize - oldSize;
	    trapStack.genRegs[D0] = oldSize;
	    trapStack.genRegs[A0] = (int) oldStackPtr;
	    return(EXC_SIG_RETURN);
	}

	case EXC_ADDRESS_ERROR:
	    (void) Sig_Send(SIG_ADDR_FAULT, SIG_ADDR_ERROR,
			    procPtr->processID, FALSE);
	    break;
	case EXC_ILLEGAL_INST:
	    (void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE,
			    procPtr->processID, FALSE);
	    break;
	case EXC_ZERO_DIV:
	    (void) Sig_Send(SIG_ARITH_FAULT, SIG_ZERO_DIV,
			    procPtr->processID, FALSE);
	    break;
	case EXC_CHK_INST:
	    (void) Sig_Send(SIG_ILL_INST, SIG_CHK,
			    procPtr->processID, FALSE);
	    break;
	case EXC_TRAPV:
	    (void) Sig_Send(SIG_ILL_INST, SIG_TRAPV,
			    procPtr->processID, FALSE);
	    break;
	case EXC_PRIV_VIOLATION:
	    (void) Sig_Send(SIG_ILL_INST, SIG_PRIV_INST,
			    procPtr->processID, FALSE);
	    break;
	case EXC_TRACE_TRAP: 
	    /*
	     * Involuntary context switch trace traps have already been taken
	     * care of above.  Here the only time we pay attention to a
	     * trace trap is if the debugger is trying to single step the 
	     * user process.
	     */
	    if (procPtr->genFlags & PROC_SINGLE_STEP) {
		procPtr->genFlags &= ~PROC_SINGLE_STEP;
		(void) Sig_Send(SIG_TRACE_TRAP, SIG_NO_CODE,
				procPtr->processID, FALSE);
	    }
	    break;

	case EXC_EMU1010:
	    (void) Sig_Send(SIG_ILL_INST, SIG_EMU1010,
			    procPtr->processID, FALSE);
	    break;
	case EXC_EMU1111:
	    (void) Sig_Send(SIG_ILL_INST, SIG_EMU1111,
			    procPtr->processID, FALSE);
	    break;
	case EXC_BAD_TRAP:
	    (void) Sig_Send(SIG_ILL_INST, SIG_BAD_TRAP,
			    procPtr->processID, FALSE);

	    break;
	default:
	    return(EXC_KERN_ERROR);
    } 

    return(ExcUserReturn(procPtr, &trapStack));
}


/*
 * ----------------------------------------------------------------------------
 *
 * ExcUserReturn --
 *
 *      Take the proper action to return from a user exception.
 *
 * Results:
 *      Code to return to the trap handler.
 *
 * Side effects:
 *      Interrupts disabled.
 *
 * ----------------------------------------------------------------------------
 */

int
ExcUserReturn(procPtr, trapStackPtr)
    Proc_ControlBlock	*procPtr;
    Exc_TrapStack	*trapStackPtr;
{
    Boolean	gotSig = FALSE;
    int		newPC;

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
	Sys_DisableIntr();
	if (!Sig_Pending(procPtr) || gotSig) {
	    break;
	}
	Sys_EnableIntr();
	gotSig = Sig_Handle(trapStackPtr, &newPC);
    }
    
    if (procPtr->genFlags & PROC_SINGLE_STEP) {
	trapStackPtr->excStack.statusReg |= SUN_SR_TRACEMODE;
    }

    if (gotSig) {
	/*
	 * We need to create a simple stack so that the rte will return
	 * to the user program correctly.  The current stack may be
	 * from an address error and hence too big.  newSP is set so
	 * that it is at the bottom of the current trap stack.
	 */
	int	newSP;

	trapStackPtr->excStack.pc = newPC;
	newSP = (int)trapStackPtr + Exc_GetTrapStackSize(trapStackPtr) -
		EXC_SHORT_STACK;
	trapStackPtr->genRegs[SP] = newSP;
	return(newSP);
    } else {
	return(EXC_OK);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * ExcSigReturn --
 *
 *      We are returning from a signal handler.  The trap stack has been
 *	restored so all that we have to do is to do normal user return
 *	time processing.
 *
 * Results:
 *      Code to return to the trap handler.
 *
 * Side effects:
 *      Signal stack freed from proc table.
 *
 * ----------------------------------------------------------------------------
 */

int
ExcSigReturn(sigStackPtr, trapStack)
    Address		sigStackPtr;
    Exc_TrapStack	trapStack;
{
    /*
     * Turn interrupts back on since they were disabled.
     */
    Sys_EnableIntr();

    Mem_Free(sigStackPtr);

    return(ExcUserReturn(Proc_GetActualProc(Sys_GetProcessorNumber()), 
	   &trapStack));
}


/*
 * ----------------------------------------------------------------------------
 *
 * Exc_GetTrapStackSize --
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

#define	BASE_SIZE	(sizeof(Exc_TrapStack) - sizeof(Exc_ExcStack))

int
Exc_GetTrapStackSize(trapStackPtr)
    Exc_TrapStack	*trapStackPtr;
{
    switch (trapStackPtr->excStack.vor.stackFormat) {
	case EXC_SHORT:
	    return(BASE_SIZE + EXC_SHORT_SIZE);
	case EXC_THROWAWAY:
	    return(BASE_SIZE + EXC_THROWAWAY_SIZE);
	case EXC_INST_EXCEPT:
	    return(BASE_SIZE + EXC_INST_EXCEPT_SIZE);
	case EXC_MC68010_BUS_FAULT:
	    return(BASE_SIZE + EXC_MC68010_BUS_FAULT_SIZE);
	case EXC_COPROC_MID_INSTR:
	    return(BASE_SIZE + EXC_COPROC_MID_INSTR_SIZE);
	case EXC_SHORT_BUS_FAULT:
	    return(BASE_SIZE + EXC_SHORT_BUS_FAULT_SIZE);
	case EXC_LONG_BUS_FAULT:
	    return(BASE_SIZE + EXC_LONG_BUS_FAULT_SIZE);
	default:
	    Sys_Panic(SYS_WARNING, "Exc_GetTrapStackSize: Bad stack format.\n");
	    return(-1);
    }
}

