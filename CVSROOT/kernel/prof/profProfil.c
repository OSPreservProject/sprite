/* 
 * profProfil.c --
 *
 *	Profil system call.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#include <assert.h>
#include "sprite.h"
#include "proc.h"
#include "prof.h"
#include "sync.h"

/*
 * Program counter recorded by the interrupt handler.
 */
int Prof_InterruptPC;

#ifdef __STDC__
extern ReturnStatus Prof_Profil(short *buffer,
    int bufSize, int offset, int scale);
extern void Prof_Tick(Timer_Ticks time, 
    ClientData clientData);
extern void Prof_Disable(Proc_ControlBlock *procPtr);
#else
extern ReturnStatus Prof_Profil();
extern void Prof_Tick();
extern void Prof_Disable();
#endif

/*
 * Monitor lock for this module.
 */
static Sync_Lock profilLock = Sync_LockInitStatic("Prof:profilLock");
#define LOCKPTR &profilLock

/*
 * Information that is passed to Timer_ScheduleRoutine() to
 * queue Prof_Tick() for invocation by the call back timer
 * interrupt handler.
 */
Timer_QueueElement profTimer_QueueElement = {
    { NULL, NULL }, /* links */
    Prof_Tick,      /* routine */
    0,              /* time */
    1234,           /* clientData */
    FALSE,          /* processed */
    0               /* interval */
};

/*
 * Count of the number of processes being profiled.
 */
int profCount = 0;


/*
 *----------------------------------------------------------------------
 *
 * profProfil --
 *
 *	Profil system call.
 *
 * Results:
 *      Always returns success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_Profil(buffer, bufSize, offset, scale)
    short *buffer;
    int bufSize;
    int offset;
    int scale;
{
#ifdef NOTDEF
    Proc_ControlBlock *curProcPtr;

    curProcPtr = Proc_GetCurrentProc();
    assert(curProcPtr != (Proc_ControlBlock *) NIL);
    /*
     * If the profile routine has not already been scheduled for invocation
     * by the call back timer, then schedule it.
     */
    LOCK_MONITOR;
    if (scale != 0 && curProcPtr->Prof_Scale == 0) {
	if (profCount++ == 0) {
	    profTimer_QueueElement.interval = 20 * timer_IntOneMillisecond;
	    Timer_ScheduleRoutine(&profTimer_QueueElement, TRUE);
	}
    } else if (scale == 0 && curProcPtr->Prof_Scale != 0) {
	assert(profCount > 0);
	if (--profCount == 0) {
	    Timer_DescheduleRoutine(&profTimer_QueueElement);
	}
    } else {
	goto exit;
    }
    curProcPtr->Prof_Buffer = buffer;
    curProcPtr->Prof_BufferSize = bufSize;
    curProcPtr->Prof_Offset = offset;
    curProcPtr->Prof_Scale = scale;
    curProcPtr->Prof_PC = 0;
exit:
    UNLOCK_MONITOR;
#endif
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Prof_Tick --
 *
 *      If any processes are scheduled for profiling this routine
 *      is called on each call back timer interrupt.
 *
 * Results:
 *      Always returns SUCCESS.
 * 
 * Side effects:
 *
 *      If the current process is scheduled for profiling then the 
 *      specialHandling flag is set, and the current PC is recorded in
 *      the process control block.
 *
 *----------------------------------------------------------------------
 */ 

void
Prof_Tick(time, clientData)
    Timer_Ticks time;
    ClientData clientData;
{
    Proc_ControlBlock *curProcPtr;

    assert(clientData == profTimer_QueueElement.clientData);
    if (!mach_KernelMode) {
	curProcPtr = Proc_GetCurrentProc();
	assert(curProcPtr !=  (Proc_ControlBlock *) NIL);
	if (curProcPtr->Prof_Scale >= 2) {
	    curProcPtr->Prof_PC = Prof_InterruptPC;
	    curProcPtr->specialHandling = TRUE;
	}
    }
    Timer_ScheduleRoutine(&profTimer_QueueElement, TRUE);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Prof_RecordPC --
 *
 *
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      None.
 *  
 *----------------------------------------------------------------------
 */ 

void
Prof_RecordPC(procPtr)
    Proc_ControlBlock *procPtr;
{
    short *ptr;
    union {
	short shrt;
	char c[2];
    } u;

    ptr = &procPtr->Prof_Buffer[(((procPtr->Prof_PC -
        procPtr->Prof_Offset) * procPtr->Prof_Scale) >> 16) / sizeof(*ptr)];
    /*
     * Make sure the pointer is in the proper range.
     */
    if (ptr < procPtr->Prof_Buffer || ptr >=
        &procPtr->Prof_Buffer[procPtr->Prof_BufferSize/sizeof(short)]) {
	procPtr->Prof_PC = 0;
	return;
    }
    /*
     * Copy the counter in, increment it, and copy it back out.
     */
    if (Vm_CopyInProc(sizeof(short), procPtr, ptr, &(u.c), 1) != SUCCESS) {
	Prof_Disable(procPtr);
	return;
    }
    ++u.shrt;
    if (Vm_CopyOutProc(sizeof(short), &(u.c), 1, procPtr, ptr) != SUCCESS){
	Prof_Disable(procPtr);
	return;
    }
    procPtr->Prof_PC = 0;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_Disable --
 *
 *	Disable profiling of a process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Clears the Prof_Scale and Prof_PC fields of the PCB, and decrements
 *      the count of profiled processes.  If no other processes are being
 *      profiled, removes the profil timer from the callback timer queue.
 *
 *----------------------------------------------------------------------
 */

void
Prof_Disable(procPtr)
    Proc_ControlBlock *procPtr;
{
#ifdef NOTDEF
    LOCK_MONITOR;
    if (procPtr->Prof_Scale == 0) {
	goto exit;
    }
    assert(profCount > 0);
    procPtr->Prof_Scale = 0;
    procPtr->Prof_PC = 0;
    if (--profCount == 0) {
	Timer_DescheduleRoutine(&profTimer_QueueElement);
    }
exit:
    UNLOCK_MONITOR;
#endif
    return;
}

