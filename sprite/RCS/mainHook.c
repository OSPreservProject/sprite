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
#include "main.h"

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

#ifdef notdef
/*
 * Malloc/Free tracing.  This array defines which object sizes will
 * be traced by malloc and free.  They'll record the caller's PC and
 * the number of objects it currently has allocated.  The sizes reflect
 * the 8 byte granularity of the memory allocator, plus the 16 bytes
 * of overhead on each bin.
 */
#include "fs.h"
#include "fsPdev.h"
#include "fsDisk.h"
#include "fsDevice.h"
#include "fsFile.h"
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
    { 96, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { 112, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { sizeof(FsFileDescriptor), (MEM_STORE_TRACE) },
    { 144, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { sizeof(FsDeviceIOHandle), (MEM_STORE_TRACE) },
    { sizeof(PdevControlIOHandle), (MEM_STORE_TRACE) },
    { 216, (MEM_STORE_TRACE | MEM_DONT_USE_ORIG_SIZE) },
    { sizeof(PdevServerIOHandle), (MEM_STORE_TRACE) },
    { sizeof(FsRmtFileIOHandle), (MEM_STORE_TRACE) },
    { sizeof(FsLocalFileIOHandle), (MEM_STORE_TRACE) },
};
#endif

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
#ifdef notdef
    Mem_SetTraceSizes(sizeof(mainMemTraceInfo) / sizeof(Mem_TraceInfo),
			mainMemTraceInfo);
#endif
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

