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
#include "vm.h"

/*
 * Program counter recorded by the interrupt handler.
 */
/* int Prof_InterruptPC; */

/*
 * Monitor lock for this module.
 */
#ifndef lint
static Sync_Lock profilLock = Sync_LockInitStatic("Prof:profilLock");
#else
static Sync_Lock profilLock;
#endif
#define LOCKPTR &profilLock

static void tick _ARGS_((Timer_Ticks time, ClientData clientData));

/*
 * Information that is passed to Timer_ScheduleRoutine() to
 * queue tick() for invocation by the call back timer
 * interrupt handler.
 */
Timer_QueueElement profTimer_QueueElement = {
    { NULL, NULL }, /* links */
    tick,           /* routine */
    0,              /* time */
    1234,           /* clientData */
    FALSE,          /* processed */
    0               /* interval */
};

/*
 * Count of the number of processes being profiled.
 */
static int profCount = 0;


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
 *	Same as Prof_Enable.
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
    Prof_Enable(Proc_GetCurrentProc(), buffer, bufSize, offset, scale);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_Enable --
 *
 *      Enables or disables the profiling of a process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Changes the Profiling fields in the Proc_ControlBlock, increments
 *      or decrements ProfCount, and schedules or deschedules tick()
 *      in the call back queue.
 *
 *----------------------------------------------------------------------
 */

void
Prof_Enable(procPtr, buffer, bufSize, offset, scale)
    Proc_ControlBlock *procPtr;
    short *buffer;
    int bufSize;
    int offset;
    int scale;
{


#if 0
    printf("Prof_Enable(p = %x, id = %x, buffer = %08x, bufSize = %d, offset = %d, scale = %d\n",
	procPtr, procPtr->processID, buffer, bufSize, offset, scale);
    printf("profCount = %d\n", profCount);
#endif    

    assert(procPtr != (Proc_ControlBlock *) NIL);
    LOCK_MONITOR;
    if (scale != 0 && scale != 1 && procPtr->Prof_Scale == 0) {
	/*
	 * If the profile routine has not already been scheduled for invocation
	 * by the call back timer, then schedule it.
	 */
	if (profCount == 0) {
	    profTimer_QueueElement.interval = 20 * timer_IntOneMillisecond;
	    Timer_ScheduleRoutine(&profTimer_QueueElement, TRUE);
	}
	++profCount;
    } else if ((scale == 0 || scale == 1) && procPtr->Prof_Scale != 0) {
	assert(profCount > 0);
	/*
	 * Disable profiling.  If there are no other processes being profiled,
	 * then remove tick() from the call back queue.
	 */
	--profCount;
	if (profCount == 0) {
	    Timer_DescheduleRoutine(&profTimer_QueueElement);
	}
    }
    procPtr->Prof_Buffer = buffer;
    procPtr->Prof_BufferSize = bufSize;
    procPtr->Prof_Offset = offset;
    procPtr->Prof_Scale = scale;
    procPtr->Prof_PC = 0;
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * tick --
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

/*ARGSUSED*/
static void
tick(time, clientData)
    Timer_Ticks time;
    ClientData clientData;
{
    Proc_ControlBlock *curProcPtr;

    LOCK_MONITOR;
    assert(profCount != 0);
#if 0    
    if (profCount == 0) {
	printf("Descheduling profil timer\n");
	Timer_DescheduleRoutine(&profTimer_QueueElement);
	return;
    }
#endif    
#if 0
    printf("Prof tick, mach_KernelMode = %d, profCount = %d\n",
	mach_KernelMode, profCount);
#endif
    assert(clientData == profTimer_QueueElement.clientData);
    if (!mach_KernelMode) {
	assert(profCount);
	curProcPtr = Proc_GetCurrentProc();
	assert(curProcPtr !=  (Proc_ControlBlock *) NIL);
#if 0
	printf("Prof tick, scale=%d, pc=%08x, profCount = %d\n",
	    curProcPtr->Prof_Scale,	curProcPtr->Prof_PC, profCount);
#endif
	if (curProcPtr->Prof_Scale >= 2) {
	    curProcPtr->specialHandling = TRUE;
	}
    }
    Timer_ScheduleRoutine(&profTimer_QueueElement, TRUE);
    UNLOCK_MONITOR;
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

    assert(procPtr->Prof_Scale);
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
    if (Vm_CopyInProc(sizeof(short), procPtr, (Address) ptr, u.c, 1)
      != SUCCESS) {
	Prof_Disable(procPtr);
	return;
    }
    ++u.shrt;

#if 0
    printf("Prof_RecordPC(), shrt = %d,  profCount = %d\n", u.shrt, profCount);
#endif    

    if (Vm_CopyOutProc(sizeof(short), u.c, 1, procPtr, (Address) ptr)
      != SUCCESS) {
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
    LOCK_MONITOR;
    if (procPtr->Prof_Scale != 0) {
	assert(profCount > 0);
	procPtr->Prof_Scale = 0;
	procPtr->Prof_PC = 0;
	--profCount;
	if (profCount == 0) {
	    Timer_DescheduleRoutine(&profTimer_QueueElement);
	}
    }
    UNLOCK_MONITOR;
    return;
}

