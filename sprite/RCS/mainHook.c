/* 
 * mainHook.c --
 *
 *	Definitions to modify the behavior of the main routine.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "mem.h"

/*
 * Flags to modify main's behavior.   Can be changed without recompiling
 * by using adb to modify the binary.
 */

Boolean main_Debug 	= FALSE; /* If TRUE then enter the debugger */
Boolean main_DoProf 	= FALSE; /* If TRUE then start profiling */
Boolean main_DoDumpInit	= TRUE; /* If TRUE then initialize dump routines */
int main_NumRpcServers	= 2;	 /* # of rpc servers to create */
char *main_AltInit	= NULL;  /* If non-null then contains name of
				  * alternate init program to use. */
Boolean main_AllowNMI = FALSE;	 /* TRUE -> allow non-maskable intrrupts */

/*
 * These memory tracing sizes are based on a granularity of 8 bytes
 * per bucket in the memory allocator.  This granuarity stems from the
 * use of a double-word for administrative info that is used so that
 * allocated objects on the SPUR machine are all 8 byte aligned.
 * To repeat, these are bucket sizes and reflect the implementation of
 * the memory allocator.
 */
Mem_TraceInfo mainMemTraceInfo[] = {
    { 24, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE)  },
    { 32, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 40, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE)  },
    { 48, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE)  },
    { 56, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 64, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 72, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 80, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 88, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 136, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 288, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
};


/*
 *----------------------------------------------------------------------
 *
 * Main_HookRoutine --
 *
 *	A routine called by main() just before the init program
 *	is started.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

void
Main_HookRoutine()
{
    Mem_SetTraceSizes(sizeof(mainMemTraceInfo) / sizeof(Mem_TraceInfo),
			mainMemTraceInfo);
}


/*
 *----------------------------------------------------------------------
 *
 * Main_InitVars --
 *
 *	A routine called by main() before it does anything.  Can only be used
 *	to initialize variables and nothing else.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */
void
Main_InitVars()
{
}

