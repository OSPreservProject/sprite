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

/*
 * Flags to modify main's behavior.   Can be changed without recompiling
 * by using adb to modify the binary.
 */

Boolean main_Debug 	= FALSE; /* If TRUE then enter the debugger */
Boolean main_DoProf 	= FALSE; /* If TRUE then start profiling */
Boolean main_DoDumpInit	= TRUE; /* If TRUE then initialize dump routines */
int main_NumRpcServers	= 4;	 /* # of rpc servers to create */
Boolean main_UseAltInit = FALSE; /* TRUE -> try to use /initSprite.new */



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
}
