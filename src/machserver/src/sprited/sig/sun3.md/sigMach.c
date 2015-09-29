/* 
 * sigMach.c --
 *
 *	Sun 3 signals-related routines.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/sig/sun3.md/RCS/sigMach.c,v 1.3 92/03/12 17:52:44 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach_error.h>
#include <mach/exception.h>
#include <mach/sun3/exception.h>

#include <proc.h>
#include <sig.h>
#include <sigInt.h>
#include <sigMach.h>
#include <sigTypes.h>
#include <vm.h>

/* 
 * This struct defines the arguments that are passed to the signal 
 * trampoline routine. 
 */
typedef struct {
    Sig_HandleProc handlerProc;	/* address of signal handler */
    Sig_Stack sigStack;		/* arguments to signal handler */
} TrampolineStack;


/*
 *----------------------------------------------------------------------
 *
 * SigMach_SetSignalState --
 *
 *	Set up the machine-dependent state before a forced call to a signal 
 *	handler.
 *	
 * Results:
 *	Returns a success/failure code.
 *
 * Side effects:
 * 	Saves the process's current state on the signal stack.  Munges the 
 * 	process's PC to call the handler, and sets up the stack for the 
 * 	handler.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
SigMach_SetSignalState(procPtr, sigStackPtr, pc)
    Proc_LockedPCB *procPtr;	/* the process that's taking the signal */
    Sig_Stack *sigStackPtr;	/* partial info to put on the thread's stack */
    Address pc;			/* start address of the signal handler */
{
    kern_return_t kernStatus;
    SigMach_Context *machContextPtr = &sigStackPtr->contextPtr->machContext;
    struct sun_thread_state newRegs; /* registers for calling handler */
    Address topOfStack;		/* user stack pointer */
    ReturnStatus status;
    TrampolineStack userStack;	/* arguments to trampoline routine */
    mach_msg_type_number_t numWords; /* words of thread state read */

    /* 
     * Get a copy of the current state and write it out to the stack.
     */

    numWords = SUN_THREAD_STATE_REGS_COUNT;
    kernStatus = thread_get_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_REGS,
				  (thread_state_t)&machContextPtr->regs,
				  &numWords);
    if (numWords != SUN_THREAD_STATE_REGS_COUNT) {
	printf("%s: expected %d words of state, got %d.\n",
	       "SigMach_SetSignalState", SUN_THREAD_STATE_REGS_COUNT,
	       numWords);
	return FAILURE;
    }
    numWords = SUN_THREAD_STATE_FPA_COUNT;
    kernStatus = thread_get_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_FPA,
				  (thread_state_t)&machContextPtr->fpRegs,
				  &numWords);
    if (numWords != SUN_THREAD_STATE_FPA_COUNT) {
	printf("%s: expected %d words of fp state, got %d.\n",
	       "SigMach_SetSignalState", SUN_THREAD_STATE_FPA_COUNT,
	       numWords);
	return FAILURE;
    }

    bcopy(&machContextPtr->regs, &newRegs, sizeof(newRegs));
    if (sigDebug) {
	printf("set up handler: old stack ptr = 0x%x\n",
	       procPtr->pcb.processID, newRegs.sp);
    }

    /* 
     * Make sure the stack pointer is word-aligned.
     */
    topOfStack = (Address)((machContextPtr->regs.sp / sizeof(int)) *
			   sizeof(int));
    topOfStack -= sizeof(Sig_Context);
    if (sigDebug) {
	printf("set up handler: context copied to 0x%x\n",
	       topOfStack);
    }
    status = Vm_CopyOut(sizeof(Sig_Context), (Address)sigStackPtr->contextPtr,
			topOfStack);
    if (status != SUCCESS) {
	printf("%s: can't save state for pid %x: %s\n",
	       "SigMach_SetSignalState", procPtr->pcb.processID,
	       Stat_GetMsg(status));
	return FAILURE;
    }

    /* 
     * Put the arguments for the trampoline routine onto the stack.
     */

    userStack.sigStack = *sigStackPtr;
    userStack.sigStack.contextPtr = (Sig_Context *)topOfStack;
    userStack.handlerProc = (Sig_HandleProc)pc;
    if (sigDebug) {
	printf("set up handler: handler at 0x%x.\n", pc);
    }
    topOfStack -= sizeof(TrampolineStack);
    status = Vm_CopyOut(sizeof(TrampolineStack), (Address)&userStack,
			topOfStack);
    if (status != SUCCESS) {
	printf("%s: can't copy out handler args for pid %x: %s\n",
	       "SigMach_SetSignalState", procPtr->pcb.processID,
	       Stat_GetMsg(status));
	return FAILURE;
    }

    /* 
     * Set up the thread's registers so that it next invokes the trampoline 
     * routine.
     */
    
    if (procPtr->pcb.sigTrampProc == USER_NIL) {
	printf("%s: bogus trampoline address for pid %x.\n",
	       "SigMach_SetSignalState", procPtr->pcb.processID);
	return FAILURE;
    }
    newRegs.sp = (int)topOfStack;
    newRegs.pc = (int)(procPtr->pcb.sigTrampProc);
    if (sigDebug) {
	printf("set up handler: sigtramp at 0x%x, args at 0x%x\n",
	       newRegs.pc, newRegs.sp);
    }
    kernStatus = thread_set_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_REGS,
				  (thread_state_t)&newRegs,
				  SUN_THREAD_STATE_REGS_COUNT);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't set state for pid %x: %s.\n",
	       "SigMach_SetSignalState", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	return FAILURE;
    }

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * SigMach_ExcToSig --
 *
 *	sun3-specific code to map a Mach exception to a Sprite signal.
 *	XXX Incomplete.
 *
 * Results:
 *	Fills in the Sprite signal number, subcode, and address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
SigMach_ExcToSig(exceptionType, exceptionCode, exceptionSubcode, sigNumPtr,
		 codePtr, sigAddrPtr)
    int exceptionType;
    int exceptionCode;
    int exceptionSubcode;
    int *sigNumPtr;		/* OUT: signal number */
    int *codePtr;		/* OUT: signal subcode */
    Address *sigAddrPtr;	/* OUT: the guilty address, if any */
{
    int sigNum = 0;
    int code = SIG_NO_CODE;

    /* 
     * Look for machine-specific exception codes.  If we don't find one 
     * that matches, use the machine-independent mapping.
     */

    switch (exceptionType) {
    case EXC_BREAKPOINT:
	switch (exceptionCode) {
	case EXC_SUN3_TRACE:
	    sigNum = SIG_TRACE_TRAP;
	    break;
	case EXC_SUN3_BREAKPOINT:
	    sigNum = SIG_BREAKPOINT;
	    break;
	}
	break;
    }

    if (sigNum != 0) {
	*sigNumPtr = sigNum;
	*codePtr = code;
	*sigAddrPtr = (Address)exceptionSubcode;
    } else {
	Sig_ExcToSig(exceptionType, exceptionCode, exceptionSubcode, sigNumPtr,
		     codePtr, sigAddrPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SigMach_RestoreState --
 *
 *	Restore registers that were saved before calling the signal 
 *	handler.
 *
 * Results:
 *	Returns success/failure.
 *
 * Side effects:
 *	Changes the registers for the thread.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
SigMach_RestoreState(procPtr, sigContextPtr)
    Proc_LockedPCB *procPtr;	/* the process that gets changed */
    Sig_Context *sigContextPtr;	/* holds the register values to use */
{
    kern_return_t kernStatus;
    SigMach_Context *machContextPtr;

    machContextPtr = &sigContextPtr->machContext;
    kernStatus = thread_set_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_REGS,
				  (thread_state_t)&machContextPtr->regs,
				  SUN_THREAD_STATE_REGS_COUNT);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't restore registers for pid %x: %s\n",
	       "SigMach_RestoreState", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	return FAILURE;
    }
    kernStatus = thread_set_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_FPA,
				  (thread_state_t)&machContextPtr->fpRegs,
				  SUN_THREAD_STATE_FPA_COUNT);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: can't restore fp registers for pid %x: %s\n",
	       "SigMach_RestoreState", procPtr->pcb.processID,
	       mach_error_string(kernStatus));
	return FAILURE;
    }

    return SUCCESS;
}
