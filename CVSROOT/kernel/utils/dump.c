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
#include "rpc.h"
#include "fs.h"
#include "net.h"
#include "mem.h"
#include "recov.h"
#include "string.h"
#include "sched.h"

/*
 * Forward references to void procedures internal to this file.
 */

void	DumpList();
void	DumpListLinksStruct();
void	DumpTimerElement();
void	DumpTimerStats();
void 	PrintL1Menu();
static void	PrintVersion();
static void	PrintTOD();
extern	void	Mem_DumpStats();
extern	void	Timer_TimerGetInfo();
extern void 	Sched_DumpReadyQueue();
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
    {'e', Timer_DumpStats, (ClientData) 'e', "Dump timer stats"},
    {'f', Fs_PrintTrace,   (ClientData) -1, "Dump filesystem trace"},
    {'h', Fs_NameHashStats, (ClientData)NULL, "Dump name hash stats"},
    {'m', Mem_DumpStats, (ClientData) FALSE,"Dump memory stats"},
    {'n', Net_Reset, (ClientData)0,"Reset the network interface"},
    {'p', Proc_Dump, (ClientData) 0,"Dump process table"},
    {'r', Sched_DumpReadyQueue,  (ClientData) 0,"Dump ready queue"},
    {'q', Fs_PdevPrintTrace,  (ClientData) 200,"Dump pseudo-device trace"},
    {'s', Timer_DumpStats,   (ClientData) 's',"Reset timer stats"},
    {'t', Timer_DumpQueue,  (ClientData) 0,"Dump the timer queue"},
    {'v', PrintVersion, (ClientData) 0,"Print version string of the kernel"},
    {'w', Mem_DumpTrace, (ClientData) -1,"Dump the memory trace buffer"},
    {'x', Fs_HandleScavengeStub, (ClientData) 0,"Scavenge filesystem handles"},
    {'y', Recov_PrintTrace, (ClientData) 50,"Dump RPC recovery trace"},
    {'z', Rpc_PrintTrace, (ClientData) 50,"Dump RPC packet trace"},

    {'1', Timer_TimerGetInfo, (ClientData) 1,"Dump info for timer counter 1"},
    {'2', Timer_TimerGetInfo, (ClientData) 2,"Dump info for timer counter 2"},
    {'3', Timer_TimerGetInfo, (ClientData) 3,"Dump info for timer counter 3"},
    {'4', Timer_TimerGetInfo, (ClientData) 4,"Dump info for timer counter 4"},
    {'5', Timer_TimerGetInfo, (ClientData) 5,"Dump info for timer counter 5"},
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

    printf("Pressing the L1 key and a letter causes the following...\n");
    printf("/ or ? - Print this menu\n");
    for (entry = eventTable; entry->routine != LAST_EVENT; entry++) {
	printf("%c - %s\n",entry->key, entry->description);
    }
    /*
     * Show the machine dependent bindings.
     */
    printf("Machine dependent entries....\n");
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
    printf("Fast: %d.%06d\n", time1.seconds, time1.microseconds);
    printf("Slow: %d.%06d\n", time2.seconds, time2.microseconds);
    Time_Subtract(time2, time1, &diff);
    printf("diff: %d.%06d\n", diff.seconds, diff.microseconds);

    /*
    Timer_GetRealTimeOfDay(&time1, (int *) NIL, (Boolean *) NIL);
    Timer_GetRealTimeOfDay(&time2, (int *) NIL, (Boolean *) NIL);
    Time_Subtract(time2, time1, &diff);
    printf("Slow diff: %d.%06d\n", diff.seconds, diff.microseconds);
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
    printf("%s\n",v);
}
