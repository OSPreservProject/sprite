/* 
 * dump.c --
 *
 *	Routines to print global system states in a readable form.  Examples
 *	of structures that can be dumped are:
 *
 *		* timer queue
 *		* process table
 *		* queue of ready processes.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "mach.h"
#include "dump.h"
#include "timer.h"
#include "proc.h"
#include "sys.h"
#include "list.h"
#include "byte.h"
#include "devKeyboard.h"
#include "devTimer.h"
#include "devConsole.h"
#include "rpc.h"
#include "fs.h"
#include "fsInt.h"
#include "fsNameHash.h"
#include "net.h"
#include "mem.h"
#include "recov.h"
#include "string.h"

/*
 * Define constants, which are used instead of enumerated types to allow
 * DumpListLinksStruct to be called from Kdbx with a List_Links pointer.
 *
 * DUMP_PROC indicates a Proc_ControlBlock.
 * DUMP_TIMER indicates a Timer_QueueElement.
 */

typedef int DumpType;
#define DUMP_PROC 1
#define DUMP_TIMER 2

/*
 * Forward references to void procedures internal to this file.
 */

void	DumpList();
void	DumpListLinksStruct();
void	DumpPCB();
void	DumpTimerElement();
void	DumpTimerStats();
static void	PrintVersion();
static void	PrintTOD();
extern	void	Fs_DumpCacheStats();
extern	void	Fs_PdevPrintTrace();
extern	void	Fs_HandleScavengeStub();
extern	void	Mem_DumpStats();


/*
 *----------------------------------------------------------------------
 *
 * PrintL1Menu --
 *
 *	Dump out a list of the L1-key magic commands.
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
PrintL1Menu()
{
    Sys_Printf("Pressing the L1 key and a letter causes the following...\n");
    Sys_Printf("\ or ? - Print this menu\n");
    /*
     * MAKE SURE THIS AGREES WITH Dump_Init() !!
     */
    Sys_Printf("a - Abort to PROM monitor\n");
    Sys_Printf("b - Put machine into (old) serial line debugger\n");
    Sys_Printf("c - Dump cache stats\n");
    Sys_Printf("d - Put machine into the kernel debugger\n");
    Sys_Printf("e - Dump timer stats\n");
    Sys_Printf("f - Dump filesystem trace\n");
    Sys_Printf("h - Dump name hash statistics\n");
    Sys_Printf("k - Reset console to keyboard mode\n");
    Sys_Printf("l - Reset console to raw mode (for X)\n");
    Sys_Printf("m - Dump memory stats\n");
    Sys_Printf("n - Reset the network interface\n");
    Sys_Printf("p - Dump process table\n");
    Sys_Printf("r - Dump ready queue\n");
    Sys_Printf("q - Dump pseudo-device trace\n");
    Sys_Printf("s - Reset timer stats\n");
    Sys_Printf("t - Dump the timer queue\n");
    Sys_Printf("v - Print version string of the kernel\n");
    Sys_Printf("w - Dump the memory trace buffer\n");
    Sys_Printf("x - Scavenge filesystem handles\n");
    Sys_Printf("y - Dump RPC recovery trace\n");
    Sys_Printf("z - Dump RPC packet trace\n");

    Sys_Printf("1 - Dump info for timer counter 1\n");
    Sys_Printf("2 - Dump info for timer counter 2\n");
    Sys_Printf("3 - Dump info for timer counter 3\n");
    Sys_Printf("4 - Dump info for timer counter 4\n");
    Sys_Printf("5 - Dump info for timer counter 5\n");
    Sys_Printf("6 - Print time of day counters\n");
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_Init --
 *
 *	Establish default procedural attachments for keyboard invocation
 *	of Dump routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Keyboard procedural attachments are modified.
 *
 *----------------------------------------------------------------------
 */

void
Dump_Init()
{
    Dev_KbdQueueAttachProc('/', PrintL1Menu, (ClientData)0);
    /*
     * BE SURE TO UPDATE PrintL1Menu() !!
     */
    /* 'a' is reserved for aborting a machine to the monitor */
    /* 'b' is reserved for putting a machine into serial line debugger */
    Dev_KbdQueueAttachProc('c', Fs_DumpCacheStats, (ClientData)0);
    /* 'd' is reserved for putting a machine into the debugger */
    Dev_KbdQueueAttachProc('e', DumpTimerStats,   (ClientData) 'e');
    Dev_KbdQueueAttachProc('f', Fs_PrintTrace,   (ClientData) 50);
    Dev_KbdQueueAttachProc('h', Fs_NameHashStats, (ClientData)fsNameTablePtr);
    Dev_KbdQueueAttachProc('k', Dev_ConsoleReset, (ClientData) TRUE);
    Dev_KbdQueueAttachProc('l', Dev_ConsoleReset, (ClientData) FALSE);
    Dev_KbdQueueAttachProc('m', Mem_DumpStats, (ClientData) FALSE);
    Dev_KbdQueueAttachProc('n', Net_Reset, (ClientData)0);
    Dev_KbdQueueAttachProc('p', Dump_ProcessTable, (ClientData) 0);
    Dev_KbdQueueAttachProc('r', Dump_ReadyQueue,  (ClientData) 0);
    Dev_KbdQueueAttachProc('q', Fs_PdevPrintTrace,  (ClientData) 200);
    Dev_KbdQueueAttachProc('s', DumpTimerStats,   (ClientData) 's');
    Dev_KbdQueueAttachProc('t', Dump_TimerQueue,  (ClientData) 0);
    Dev_KbdQueueAttachProc('v', PrintVersion, (ClientData) 0);
    Dev_KbdQueueAttachProc('w', Mem_DumpTrace, (ClientData) -1);
    Dev_KbdQueueAttachProc('x', Fs_HandleScavengeStub, (ClientData) 0);
    Dev_KbdQueueAttachProc('y', Recov_PrintTrace, (ClientData) 50);
    Dev_KbdQueueAttachProc('z', Rpc_PrintTrace, (ClientData) 50);

    Dev_KbdQueueAttachProc('1', Dev_TimerGetInfo, (ClientData) 1);
    Dev_KbdQueueAttachProc('2', Dev_TimerGetInfo, (ClientData) 2);
    Dev_KbdQueueAttachProc('3', Dev_TimerGetInfo, (ClientData) 3);
    Dev_KbdQueueAttachProc('4', Dev_TimerGetInfo, (ClientData) 4);
    Dev_KbdQueueAttachProc('5', Dev_TimerGetInfo, (ClientData) 5);
    Dev_KbdQueueAttachProc('6', PrintTOD, (ClientData) 0);
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_ProcessTable --
 *
 *	Output all active proceses table entries on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

void
Dump_ProcessTable()
{
    Boolean empty;
    int i;
    Proc_ControlBlock *procPtr;

    
    DISABLE_INTR();
    empty = TRUE;
    Sys_Printf("\n");
    Sys_Printf("%6s %6s %5s %10s %10s %8s %8s   %s\n",
	"PCBPtr", "ID", "wtd", "user", "kernel", "event", "state", "name");
    for (i = 0; i < proc_MaxNumProcesses; i++) {
	procPtr = proc_PCBTable[i];
	if (procPtr->state != PROC_UNUSED) {
	    empty = FALSE;
	    DumpListLinksStruct((List_Links *) procPtr, DUMP_PROC);
	}
    }
    if (empty == TRUE) {
	Sys_Printf("List is empty.");
    }
    Sys_Printf("\n");
    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_ReadyQueue --
 *
 *	Output the ready queue on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */
void
Dump_ReadyQueue()
{
    extern List_Links *schedReadyQueueHdrPtr;

    DumpList(schedReadyQueueHdrPtr, DUMP_PROC);
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_TimerQueue --
 *
 *	Output the timer queue on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

void
Dump_TimerQueue()
{
    extern List_Links *timerQueueList;
    Timer_Ticks	ticks;
    Time	time;

    Timer_GetCurrentTicks(&ticks);
    Timer_TicksToTime(ticks, &time);
    Sys_Printf("Now: %d.%06u sec\n", time.seconds, time.microseconds);
    DumpList(timerQueueList, DUMP_TIMER);
}


/*
 *----------------------------------------------------------------------
 *
 * DumpList --
 *
 *	Output the structures contained in a linked list, with interrupts
 *	disabled throughout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */
    
void
DumpList(hdrPtr, type)
    List_Links *hdrPtr;
    DumpType type;
{
    List_Links *itemPtr;

    
    if (List_IsEmpty(hdrPtr)) {
	Sys_Printf("\nList is empty.\n");
    } else {
	Sys_Printf("\n");
	DISABLE_INTR();
	LIST_FORALL(hdrPtr, itemPtr) {
	    DumpListLinksStruct(itemPtr, type);
	}
	ENABLE_INTR();
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DumpListLinksStruct --
 *
 *	Given the ``type'' of a List_Links (based on a defined constant),
 *	print out relevant info for the structure to which the List_Links
 *	points.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is displayed on the display (implying that interrupts are
 *	disabled at length).
 *
 *----------------------------------------------------------------------
 */

void
DumpListLinksStruct(itemPtr, type)
    List_Links *itemPtr;   /* pointer to List_Links representation of struct */
    DumpType type;      /* defined constant indicating which typecast to use */
{
    switch(type) {
	case DUMP_PROC:
	    DumpPCB((Proc_ControlBlock *) itemPtr);
	    break;
	case DUMP_TIMER:
	    DumpTimerElement((Timer_QueueElement *) itemPtr);
	    break;
	default:
	    Sys_Panic(SYS_WARNING,
		      "DumpListLinksStruct: unknown type %d", type);
	    break;
    }
}
    

/*
 *----------------------------------------------------------------------
 *
 * DumpPCB --
 *
 *	Output the more important parts of a Proc_ControlBlock on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

void
DumpPCB(procPtr)
    Proc_ControlBlock *procPtr;
{
   
    static char *states[] = {
	"unused",
	"running",
	"ready",
	"waiting",
	"exiting",
	"dead",
	"debug",
	"migrated",
	"new",
	"suspended",
    };
    Proc_State state;

    state = procPtr->state;
    switch (state) {
	case PROC_UNUSED:
	case PROC_RUNNING:
	case PROC_READY:
	case PROC_WAITING:
	case PROC_EXITING:
	case PROC_DEAD:
	case PROC_MIGRATED:
	case PROC_NEW:
	case PROC_SUSPENDED:
	break;
	default:
	Sys_Panic(SYS_FATAL, "DumpPCB: invalid proess state: %d.\n", state);
    }
    /*
     * A header describing the fields has already been printed.
     */
    Sys_Printf(
	       " %6x %6x %5d [%1d,%6d] [%1d,%6d] %8x %8s",
	       procPtr, 
	       procPtr->processID, 
	       procPtr->weightedUsage, 
#ifdef SUN2
	       procPtr->userCpuUsage.high,
	       procPtr->userCpuUsage.low,
	       procPtr->kernelCpuUsage.high, 
	       procPtr->kernelCpuUsage.low,
#endif
#ifdef SUN3
	       procPtr->userCpuUsage.seconds,
	       procPtr->userCpuUsage.microseconds,
	       procPtr->kernelCpuUsage.seconds, 
	       procPtr->kernelCpuUsage.microseconds,
#endif
	       procPtr->event,
	       states[(int) state]);
    if (procPtr->argString != (Address) NIL) {
	char cmd[30];
	char *space;

	String_NCopy(30, procPtr->argString, cmd);
	space = String_FindChar(cmd, ' ');
	if (space != (char *) NULL) {
	    *space = '\0';
	}
	Sys_Printf(" %s\n", cmd);
    } else {
	Sys_Printf("\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DumpTimerElement --
 *
 *	Output the more important parts of a Timer_QueueElement on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

void
DumpTimerElement(timerPtr)
    Timer_QueueElement *timerPtr;
{
    Time  	time;

    Timer_TicksToTime(timerPtr->time, &time);

    Sys_Printf("(*0x%x)(0x%x) @ %d.%06u\n",
	    (Address) timerPtr->routine, 
	    (Address) timerPtr->clientData,
	    time.seconds, time.microseconds);
}


/*
 *----------------------------------------------------------------------
 *
 * DumTimerStats --
 *
 *	Initializes and prints the timer module statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

void
DumpTimerStats(arg)
    ClientData	arg;
{
    static   Timer_Ticks	start;
    static   Timer_Ticks	end;
    Timer_Ticks	diff;
    Time  	time;

    if (arg ==  (ClientData) 's') {
	Timer_GetCurrentTicks(&start);
	Byte_Zero(sizeof(timer_Statistics), (Address) &timer_Statistics);
    } else {
	Timer_GetCurrentTicks(&end);
	Timer_SubtractTicks(end, start, &diff);
	Timer_TicksToTime(diff, &time);

	Sys_Printf(
	"\n%d.%06d cb %d prof %d spur %d; Sched %d Res %d Des %d\n",
	    time.seconds, time.microseconds,
	    timer_Statistics.callback,
	    timer_Statistics.profile,
	    timer_Statistics.spurious,
	    timer_Statistics.schedule,
	    timer_Statistics.resched,
	    timer_Statistics.desched
	);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintTOD --
 *
 *	Prints the time of day using one of two sources for the time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

static void
PrintTOD()
{
    Time time1, time2, diff;

    Timer_GetTimeOfDay(&time1, (int *) NIL, (Boolean *) NIL);
    Timer_GetRealTimeOfDay(&time2, (int *) NIL, (Boolean *) NIL);
    Sys_Printf("Fast: %d.%06d\n", time1.seconds, time1.microseconds);
    Sys_Printf("Slow: %d.%06d\n", time2.seconds, time2.microseconds);
    Time_Subtract(time2, time1, &diff);
    Sys_Printf("diff: %d.%06d\n", diff.seconds, diff.microseconds);

    /*
    Timer_GetRealTimeOfDay(&time1, (int *) NIL, (Boolean *) NIL);
    Timer_GetRealTimeOfDay(&time2, (int *) NIL, (Boolean *) NIL);
    Time_Subtract(time2, time1, &diff);
    Sys_Printf("Slow diff: %d.%06d\n", diff.seconds, diff.microseconds);
    */
}


/*
 *----------------------------------------------------------------------
 *
 * PrintVersion --
 *
 *	Prints the kernel's version on the console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

static void
PrintVersion()
{
    extern char *SpriteVersion();
    char *v;

    v = SpriteVersion();
    Sys_Printf("%s\n",v);
}
