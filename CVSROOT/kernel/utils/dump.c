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
#endif /* not lint */


#include "net.h"
#include "sprite.h"
#include "mach.h"
#include "dump.h"
#include "dumpInt.h"
#include "timer.h"
#include "proc.h"
#include "sys.h"
#include "list.h"
#include "devVid.h"
#include "rpc.h"
#include "fs.h"
#include "fsutil.h"
#include "fscache.h"
#include "fspdev.h"
#include "recov.h"
#include "string.h"
#include "sched.h"
#include "devSyslog.h"
#include "mem.h"
#include <stdio.h>

/*
 * Forward references to procedures internal to this file.
 */
static void	PrintL1Menu _ARGS_((ClientData));
static void	PrintTOD _ARGS_((ClientData));
static void	PrintVersion _ARGS_((ClientData));
static void	ResetNetworks _ARGS_((ClientData));

/*
 * Table of routines and their arguments to be called on dump events.
 * Only machine independent dump events should be added to this table.
 * Machine dependent event should be added to the Local event table.
 */
static EventTableType eventTable[] = {
    {'/', PrintL1Menu, (ClientData)0,"Print Dump Event Menu"},
    {'a', RESERVED_EVENT, NULL_ARG, "Abort to PROM monitor" }, 
#ifdef notdef
    {'b', RESERVED_EVENT, NULL_ARG, 
				"Put machine into (old) serial line debugger"},
#endif
    {'c', Fscache_DumpStats, (ClientData)0, "Dump cache stats"},
    {'d', RESERVED_EVENT, NULL_ARG, "Put machine into the kernel debugger"},
    {'e', Timer_DumpStats, (ClientData) 'e', "Dump timer stats"},
    {'f', Fsutil_PrintTrace,   (ClientData) -1, "Dump filesystem trace"},
    {'i', (void (*)()) Proc_KDump, (ClientData) 0,"Info on waiting processes"},
    {'j', Dev_SyslogDisable, (ClientData) 0,"Disable/enable syslog"},
    {'m', Mem_DumpStats, (ClientData) FALSE,"Dump memory stats"},
    {'n', ResetNetworks, (ClientData)0,"Reset the network interfaces"},
    {'o', (void (*)()) Dev_VidEnable, (ClientData) 1,"Turn video on"},
    {'p', (void (*)()) Proc_Dump, (ClientData) 0,"Dump process table"},
    {'r', Sched_DumpReadyQueue,  (ClientData) 0,"Dump ready queue"},
    {'q', Fspdev_PrintTrace,  (ClientData) 200,"Dump pseudo-device trace"},
    {'s', Timer_DumpStats,   (ClientData) 's',"Reset timer stats"},
    {'t', Timer_DumpQueue,  (ClientData) 0,"Dump the timer queue"},
    {'v', PrintVersion, (ClientData) 0,"Print version string of the kernel"},
    {'w', Fsutil_SyncStub, (ClientData) FALSE, "WRITE BACK CACHE"},
    {'x', Fsutil_HandleScavengeStub, (ClientData) 0,"Scavenge filesystem handles"},
    {'y', Recov_PrintTrace, (ClientData) 50,"Dump RPC recovery trace"},
    {'z', Rpc_PrintTrace, (ClientData) 50,"Dump RPC packet trace"},
    {'1', Timer_TimerGetInfo, (ClientData) 1,"Dump info for timer counter 1"},
    {'2', Timer_TimerGetInfo, (ClientData) 2,"Dump info for timer counter 2"},
    {'3', Timer_TimerGetInfo, (ClientData) 3,"Dump info for timer counter 3"},
    {'4', Timer_TimerGetInfo, (ClientData) 4,"Dump info for timer counter 4"},
    {'5', Timer_TimerGetInfo, (ClientData) 5,"Dump info for timer counter 5"},
    {'6', PrintTOD,           (ClientData) 0,"Print time of day counters"},
	/* This MUST be the last entry */
    {'\0', LAST_EVENT,        NULL_ARG,       (char *) 0 },
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

static void
PrintL1Menu(arg)
    ClientData arg;		/* unused */
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
PrintTOD(arg)
    ClientData arg;		/* unused */
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
PrintVersion(arg)
    ClientData arg;		/* unused */
{
    extern char *SpriteVersion();
    char *v;

    v = SpriteVersion();
    printf("%s\n",v);
}

/*
 *----------------------------------------------------------------------
 *
 * ResetNetworks --
 *
 *	Reset all network interfaces.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The network interfaces are reset.
 *
 *----------------------------------------------------------------------
 */

static void
ResetNetworks(arg)
    ClientData arg;		/* unused */
{
    int			i;
    Net_Interface	*interPtr;

    i = 0; 
    interPtr = Net_NextInterface(FALSE, &i);
    while (interPtr != (Net_Interface *) NIL) {
	Net_Reset(interPtr);
	i++;
	interPtr = Net_NextInterface(FALSE, &i);
    }
}

