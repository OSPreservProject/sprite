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
#include "dumpInt.h"
#include "timer.h"
#include "proc.h"
#include "sys.h"
#include "list.h"
#include "byte.h"
#include "devTimer.h"
#include "rpc.h"
#include "fs.h"
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
void	DumpTimerElement();
void	DumpTimerStats();
void 	PrintL1Menu();
void	DumpPCB();
void	Dump_ProcessTable();
static void	PrintVersion();
static void	PrintTOD();
extern	void	Fs_DumpCacheStats();
extern	void	Fs_PdevPrintTrace();
extern	void	Fs_HandleScavengeStub();
extern	void	Mem_DumpStats();
/*
 * Table of routines and their arguments to be called on dump events.
 * Only machine independent dump events should be added to this table.
 * Machine dependent event should be added to the Local event table.
 */
static EventTableType eventTable[] = {
    {'/', PrintL1Menu, (ClientData)0,"Print Dump Event Menu"},
    {'a', RESERVED_EVENT, NULL_ARG, "Abort to PROM monitor" }, 
    {'b', RESERVED_EVENT, NULL_ARG, 
				"Put machine into (old) serial line debugger"},
    {'c', Fs_DumpCacheStats, (ClientData)0, "Dump cache stats"},
    {'d', RESERVED_EVENT, NULL_ARG, "Put machine into the kernel debugger"},
    {'e', DumpTimerStats, (ClientData) 'e', "Dump timer stats"},
    {'f', Fs_PrintTrace,   (ClientData) -1, "Dump filesystem trace"},
#ifdef brent
    {'h', Fs_NameHashStats, (ClientData)fsNameTablePtr,"Dump name hash stats"},
#endif
    {'m', Mem_DumpStats, (ClientData) FALSE,"Dump memory stats"},
    {'n', Net_Reset, (ClientData)0,"Reset the network interface"},
    {'p', Dump_ProcessTable, (ClientData) 0,"Dump process table"},
    {'r', Dump_ReadyQueue,  (ClientData) 0,"Dump ready queue"},
    {'q', Fs_PdevPrintTrace,  (ClientData) 200,"Dump pseudo-device trace"},
    {'s', DumpTimerStats,   (ClientData) 's',"Reset timer stats"},
    {'t', Dump_TimerQueue,  (ClientData) 0,"Dump the timer queue"},
    {'v', PrintVersion, (ClientData) 0,"Print version string of the kernel"},
    {'w', Mem_DumpTrace, (ClientData) -1,"Dump the memory trace buffer"},
    {'x', Fs_HandleScavengeStub, (ClientData) 0,"Scavenge filesystem handles"},
    {'y', Recov_PrintTrace, (ClientData) 50,"Dump RPC recovery trace"},
    {'z', Rpc_PrintTrace, (ClientData) 50,"Dump RPC packet trace"},

    {'1', Dev_TimerGetInfo, (ClientData) 1,"Dump info for timer counter 1"},
    {'2', Dev_TimerGetInfo, (ClientData) 2,"Dump info for timer counter 2"},
    {'3', Dev_TimerGetInfo, (ClientData) 3,"Dump info for timer counter 3"},
    {'4', Dev_TimerGetInfo, (ClientData) 4,"Dump info for timer counter 4"},
    {'5', Dev_TimerGetInfo, (ClientData) 5,"Dump info for timer counter 5"},
    {'6', PrintTOD, (ClientData) 0,"Print time of day counters"},
	/* This MUST be the last entry */
    {'\000', LAST_EVENT, NULL_ARG, (char *) 0 }, 
};



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
    EventTableType	*entry;

    Sys_Printf("Pressing the L1 key and a letter causes the following...\n");
    Sys_Printf("/ or ? - Print this menu\n");
    for (entry = eventTable; entry->routine != LAST_EVENT; entry++) {
	Sys_Printf("%c - %s\n",entry->key, entry->description);
    }
    /*
     * Show the machine dependent bindings.
     */
    Sys_Printf("Machine dependent entries....\n");
    Dump_Show_Local_Menu();
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_Init --
 *
 *	Establish default procedural attachments for Dump events.
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
Dump_Init()
{
    Dump_Register_Events(eventTable);

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
    Time	kernelTime, userTime;

    static char *states[] = {
	"unused",
	"running",
	"ready",
	"waiting",
	"exiting",
	"dead",
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
    Timer_TicksToTime(procPtr->userCpuUsage, &userTime);
    Timer_TicksToTime(procPtr->kernelCpuUsage, &kernelTime);
    Sys_Printf(
	       " %6x %6x %5d [%1d,%6d] [%1d,%6d] %8x %8s",
	       procPtr, 
	       procPtr->processID, 
	       procPtr->weightedUsage, 
	       userTime.seconds,
	       userTime.microseconds,
	       kernelTime.seconds, 
	       kernelTime.microseconds,
	       procPtr->event,
	       states[(int) state]);
    if (procPtr->argString != (Address) NIL) {
	char cmd[30];
	char *space;

	(void) String_NCopy(30, procPtr->argString, cmd);
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
