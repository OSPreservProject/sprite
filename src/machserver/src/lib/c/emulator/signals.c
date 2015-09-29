/* 
 * signals.c --
 *
 *	Emulator code for dealing with user signal handlers.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/signals.c,v 1.4 92/07/16 18:04:25 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <mach/message.h>
#include <mach_error.h>
#include <sprite.h>
#include <sig.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>

/* 
 * This port will never have anything sent to it, so we can wait on it 
 * if Sig_ReturnStub returns before the server has resumed the process.  
 * This is better than thread_suspend, because it means the server doesn't 
 * have to worry about getting the right number of thread_resume's.
 */
static mach_msg_header_t waitMsg;
static mach_port_t waitPort;
static Boolean initialized = FALSE;

static void Init _ARGS_((void));
static void SpriteEmu_SigTramp _ARGS_((foo));


/*
 *----------------------------------------------------------------------
 *
 * Init --
 *
 *	Initialize the private "wait" variables.  Note that this only 
 *	needed if there is a user signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the file's private variables.
 *
 *----------------------------------------------------------------------
 */

static void
Init()
{
    kern_return_t kernStatus;

    mach_init();
    kernStatus = mach_port_allocate(mach_task_self(),
				    MACH_PORT_RIGHT_RECEIVE, &waitPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("C runtime error: can't allocate port for sigreturn: %s.\n",
	      mach_error_string(kernStatus));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SetAction --
 *
 *	Set the handler for a given signal.
 *
 * Results:
 *	Returns a Sprite status code.
 *
 * Side effects:
 *	Registers the handler for the Sprite signal and registers the 
 *	signals "trampoline" code.
 *
 *----------------------------------------------------------------------
 */
    
ReturnStatus
Sig_SetAction(sigNum, newActionPtr, oldActionPtr)
    int		sigNum;	       /* The signal for which the action is to be 
				  set. */
    Sig_Action	*newActionPtr; /* The actions to take for the signal. */
    Sig_Action	*oldActionPtr; /* The action that was taken for the signal. */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Sig_Action dummyAction;	/* in case the old action wasn't requested */
    Boolean sigPending;

    if (!initialized) {
	Init();
	initialized = TRUE;
    }
    
    if (oldActionPtr == NULL) {
	oldActionPtr = &dummyAction;
    }
    kernStatus = Sig_SetActionStub(SpriteEmu_ServerPort(), sigNum,
				   *newActionPtr,
				   (vm_address_t)((Address)SpriteEmu_SigTramp),
				   &status, oldActionPtr, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * SpriteEmu_SigTramp --
 *
 *	Signals "trampoline" code: call the user signal handler and 
 *	clean up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SpriteEmu_SigTramp(handlerProc, sigNum, code, sigContextPtr, sigAddr)
    Sig_HandleProc handlerProc;	/* user signal handler */
    int sigNum;			/* Sprite signal number */
    int code;			/* Sprite signal subcode */
    Sig_Context *sigContextPtr;
    Address sigAddr;
{
    kern_return_t kernStatus;

    (handlerProc)(sigNum, code, sigContextPtr, sigAddr);
    (void)Sig_ReturnStub(SpriteEmu_ServerPort(), (vm_address_t)sigContextPtr);
    /* 
     * Just sit here until the server resumes us at the point where the 
     * signal was taken.
     */
    kernStatus = mach_msg(&waitMsg, MACH_RCV_MSG, 0, sizeof(waitMsg), waitPort,
			  0, MACH_PORT_NULL);
    panic("SpriteEmu_SigTramp: couldn't wait: %s.\n",
	  mach_error_string(kernStatus));
}


/*
 *----------------------------------------------------------------------
 *
 * SpriteEmu_TakeSignals --
 *
 *	Get information about pending signals and call the user signal 
 *	handlers.
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
SpriteEmu_TakeSignals()
{
    Boolean sigPending = TRUE;
    int sigNum;			/* Sprite signal number */
    int sigCode;		/* Sprite signal subcode */
    Sig_Context sigContext;
    Address sigAddr;		/* address of fault or whatever */
    Sig_HandleProc handlerProc;	/* user signal handler */
    ReturnStatus status;
    kern_return_t kernStatus;
    int dummyMask;		/* keep MIG happy */

    /* 
     * As long as there are pending signals, get the signal information 
     * from the server, call the handler, and clean up.
     */
    
    while (sigPending) {
	kernStatus = Sig_GetSignalStub(SpriteEmu_ServerPort(), &status,
				       (vm_address_t *)&handlerProc, &sigNum,
				       &sigCode, (vm_address_t)&sigContext,
				       (vm_address_t *)&sigAddr);
	if (kernStatus != KERN_SUCCESS) {
	    status = Utils_MapMachStatus(kernStatus);
	}
	if (status != SUCCESS) {
	    panic("SpriteEmu_TakeSignals: can't get signal information: %s\n",
		  Stat_GetMsg(status));
	}
	/* 
	 * If the handler is given as null, that means there aren't really 
	 * any more signals to handle.
	 */
	if (handlerProc == NULL) {
	    return;
	}
	
	(*handlerProc)(sigNum, sigCode, &sigContext, sigAddr);
	kernStatus = Sig_SetHoldMaskStub(SpriteEmu_ServerPort(),
					 sigContext.oldHoldMask, &dummyMask,
					 &sigPending);
	if (kernStatus != KERN_SUCCESS) {
	    panic("SpriteEmu_TakeSignals: couldn't reset signals mask: %s\n",
		  mach_error_string(kernStatus));
	}
    }
}
