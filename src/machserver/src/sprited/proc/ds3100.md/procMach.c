/* 
 * procMach.c --
 *
 *	Process management for the DECstation.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation and the University of California make no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/ds3100.md/RCS/procMach.c,v 1.4 92/05/08 12:57:05 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <bstring.h>
#include <mach/mips/thread_status.h>
#include <mach/mips/vm_param.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>

#include <fs.h>
#include <proc.h>
#include <procInt.h>
#include <procMach.h>
#include <utils.h>
#include <vm.h>

/* 
 * This is the address immediately following a user process's stack.
 */
Address procMach_MaxUserStackAddr;

/* 
 * This is 1 plus the highest address that can be used in a user process.
 */
Address procMach_MaxUserAddr;


/*
 *----------------------------------------------------------------------
 *
 * ProcMachInit --
 *
 *	Initialization routine for machine-dependent process 
 *	management code. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various variables initialized.
 *
 *----------------------------------------------------------------------
 */

void
ProcMachInit()
{
    procMach_MaxUserStackAddr = Vm_TruncPage(VM_MAX_ADDRESS);
    procMach_MaxUserAddr = procMach_MaxUserStackAddr;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMachSetRegisters --
 *
 *	Set up the registers for a locked process to start execution.  Note 
 *	that we don't have to set the gp, because crt0 does that.
 *
 * Results:
 *	status code.
 *
 * Side effects:
 *	Sets the thread state for the given process.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
ProcMachSetRegisters(procPtr, topOfStack, startAddr)
    Proc_ControlBlock *procPtr;	/* the process to set up */
    Address topOfStack;		/* addr. of top of stack */
    Address startAddr;		/* where to start execution */
{
    kern_return_t kernStatus;
    struct mips_thread_state registers;

    bzero((_VoidPtr)&registers, sizeof(registers));
    /* 
     * Note that on a mips the stack pointer contains the address of the 
     * top element of the stack (rather than the next address past the top 
     * of the stack).
     */
    registers.r29 = (int)topOfStack;
    registers.pc = (int)startAddr;

    kernStatus = thread_set_state(procPtr->thread,
				  MIPS_THREAD_STATE,
				  (thread_state_t)&registers,
				  MIPS_THREAD_STATE_COUNT);
    if (kernStatus != KERN_SUCCESS) {
	printf("ProcMachSetRegiters: couldn't set thread state: %s\n",
	       mach_error_string(kernStatus));
	return Utils_MapMachStatus(kernStatus);
    }

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcGetObjInfo --
 *
 *	Translate the object file information into the machine independent
 *	form.
 *
 * Results:
 *	SUCCESS if could translate.
 *	PROC_BAD_AOUT_FORMAT if could not.
 *
 * Side effects:
 *	*objInfoPtr is filled in.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
ProcGetObjInfo(filePtr, execPtr, objInfoPtr)
    Fs_Stream		*filePtr;
    ProcExecHeader	*execPtr;
    Proc_ObjInfo	*objInfoPtr;
{
    if (execPtr->fileHeader.magic != PROC_OBJ_MAGIC) {
	return(PROC_BAD_AOUT_FORMAT);
    }
    switch (execPtr->aoutHeader.magic) {
	case PROC_ZMAGIC:
	    objInfoPtr->codeLoadAddr = execPtr->aoutHeader.codeStart;
	    objInfoPtr->codeFileOffset = 0;
	    objInfoPtr->codeSize = execPtr->aoutHeader.codeSize;
	    objInfoPtr->heapLoadAddr = execPtr->aoutHeader.heapStart;
	    objInfoPtr->heapFileOffset = execPtr->aoutHeader.codeSize;
	    objInfoPtr->heapSize = execPtr->aoutHeader.heapSize;
	    objInfoPtr->bssLoadAddr = execPtr->aoutHeader.bssStart;
	    objInfoPtr->bssSize = execPtr->aoutHeader.bssSize;
	    objInfoPtr->entry = execPtr->aoutHeader.entry;
	    objInfoPtr->unixCompat = 0;
	    break;
	case PROC_OMAGIC:
	    if (execPtr->aoutHeader.codeStart+execPtr->aoutHeader.codeSize !=
		    execPtr->aoutHeader.heapStart) {
		printf("OMAGIC output file must have data segment %s\n",
			"immediately following text segment.");
		return(PROC_BAD_AOUT_FORMAT);
	    }
	    if (execPtr->aoutHeader.codeStart <= (Address)DEFAULT_TEXT) {
		printf("OMAGIC text segment is going to collide with %s\n",
			"header segment.");
		return(PROC_BAD_AOUT_FORMAT);
	    }
	    objInfoPtr->codeLoadAddr = (Address)DEFAULT_TEXT;
	    objInfoPtr->codeFileOffset = 0;
	    objInfoPtr->codeSize = 0;
	    objInfoPtr->heapLoadAddr = execPtr->aoutHeader.codeStart;
	    objInfoPtr->heapFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	    objInfoPtr->heapSize = execPtr->aoutHeader.codeSize +
		execPtr->aoutHeader.heapSize;
	    objInfoPtr->bssLoadAddr = execPtr->aoutHeader.bssStart;
	    objInfoPtr->bssSize = execPtr->aoutHeader.bssSize;
	    objInfoPtr->entry = execPtr->aoutHeader.entry;
	    objInfoPtr->unixCompat = 0;
	    break;
	default:
	    return(PROC_BAD_AOUT_FORMAT);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMachGetUserRegs --
 *
 *	Get the current "standard" registers of the given user process.
 *
 * Results:
 *	Returns a status code.  If successful, fills in the user-space PC 
 *	and stack pointer for the given process.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
ProcMachGetUserRegs(procPtr, userPcPtr, userStackPtr)
    Proc_LockedPCB *procPtr;	/* the user process to check */
    Address *userPcPtr;		/* OUT: the PC that the process is at */
    Address *userStackPtr;	/* OUT: the process's stack pointer value */
{
    kern_return_t kernStatus;
    struct mips_thread_state registers;
    int stateCount = MIPS_THREAD_STATE_COUNT;
    ReturnStatus status = SUCCESS;

    if (!(procPtr->pcb.genFlags & PROC_USER)) {
	panic("ProcGetUserPC: not a user process.\n");
    }

    kernStatus = thread_get_state(procPtr->pcb.thread,
				  MIPS_THREAD_STATE,
				  (thread_state_t)&registers, 
				  (mach_msg_type_number_t *)&stateCount);
    if (kernStatus != KERN_SUCCESS) {
	printf("PRocMachGetUserPC: couldn't read registers: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
    } else {
	*userPcPtr = (Address)registers.pc;
	*userStackPtr = (Address)registers.r29;
    }

    return status;
}
