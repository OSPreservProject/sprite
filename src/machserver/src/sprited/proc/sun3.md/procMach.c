/* 
 * procMach.c --
 *
 *	Process management for the sun3.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/sun3.md/RCS/procMach.c,v 1.11 92/05/08 15:10:12 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <bstring.h>
#include <mach/sun3/thread_status.h>
#include <mach/sun3/vm_param.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>
#include <sun3/psl.h>

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
    procMach_MaxUserStackAddr = Vm_TruncPage(KERNELBASE);
    procMach_MaxUserAddr = procMach_MaxUserStackAddr;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMachSetRegisters --
 *
 *	Set up the registers for a locked process to start execution.
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
    struct sun_thread_state registers;

    bzero(&registers, sizeof(registers));
    /* 
     * Note that on a sun3 the stack pointer contains the address of the 
     * top element of the stack.
     */
    registers.sp = (int)topOfStack;
    registers.pc = (int)startAddr;
    registers.sr = PSL_USERSET;

    kernStatus = thread_set_state(procPtr->thread, SUN_THREAD_STATE_REGS,
				  (thread_state_t)&registers,
				  SUN_THREAD_STATE_REGS_COUNT);
    if (kernStatus != KERN_SUCCESS) {
	printf("ProcMachSetRegisters: couldn't set thread state: %s\n",
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
 *	XXX UNIX compatibility stuff ripped out for simplicity's sake. 
 *	Needs to be put back in.
 *
 * Results:
 *	Returns SUCCESS if could translate, PROC_BAD_AOUT_FORMAT if could not.
 *	*objInfoPtr is filled in.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
ProcGetObjInfo(filePtr, execPtr, objInfoPtr)
    Fs_Stream		*filePtr; /* the object file */
    ProcExecHeader	*execPtr; /* the raw exec header from the file */
    Proc_ObjInfo	*objInfoPtr; /* processed exec info (OUT) */
{
#ifdef sun3
    if (execPtr->machineType != PROC_MC68010 &&
        execPtr->machineType != PROC_MC68020) {
	printf("ProcGetObjInfo: bad machine type: 0x%x\n",
	       execPtr->machineType);
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
#ifdef sun4
    /*
     * Sun's compiler includes a tool version number or something in the
     * top 16 bits of the machineType field, so we can only look at the
     * low 16 bits.
     */
    if ((execPtr->machineType & 0xff) != PROC_SPARC) {
	return(PROC_BAD_AOUT_FORMAT);
    }
    if (execPtr->dynamic) {
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
    switch (execPtr->magic) {

    case PROC_ZMAGIC:
	objInfoPtr->codeLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->codeFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->codeSize = execPtr->code;
	objInfoPtr->heapLoadAddr = (Address)PROC_DATA_LOAD_ADDR(*execPtr);
	objInfoPtr->heapFileOffset = PROC_DATA_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data;
	objInfoPtr->bssLoadAddr = (Address)PROC_BSS_LOAD_ADDR(*execPtr);
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	objInfoPtr->unixCompat = 0;
	break;

    case PROC_OMAGIC:
	objInfoPtr->codeLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->codeFileOffset = 0;
	objInfoPtr->codeSize = 0;
	objInfoPtr->heapLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->heapFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data + execPtr->code;
	objInfoPtr->bssLoadAddr = (Address)PROC_BSS_LOAD_ADDR(*execPtr);
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	objInfoPtr->unixCompat = 0;
	break;


    default:
	printf("bad magic number: 0x%x\n", execPtr->magic);
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
    struct sun_thread_state registers;
    int stateCount = SUN_THREAD_STATE_REGS_COUNT;
    ReturnStatus status = SUCCESS;

    if (!(procPtr->pcb.genFlags & PROC_USER)) {
	panic("ProcGetUserPC: not a user process.\n");
    }

    kernStatus = thread_get_state(procPtr->pcb.thread,
				  SUN_THREAD_STATE_REGS,
				  (thread_state_t)&registers, 
				  (mach_msg_type_number_t *)&stateCount);
    if (kernStatus != KERN_SUCCESS) {
	printf("PRocMachGetUserPC: couldn't read registers: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
    } else {
	*userPcPtr = (Address)registers.pc;
	*userStackPtr = (Address)registers.sp;
    }

    return status;
}
