/*
 * main.h --
 *
 *     Procedure headers exported by the main level module.
 *
 * Copyright 1990 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MAIN
#define _MAIN

#include <sprite.h>

extern void Main _ARGS_(());
extern void Main_InitVars _ARGS_((void));
extern void Main_HookRoutine _ARGS_((void));
extern char *SpriteVersion _ARGS_((void));

/*
 * Temporary declaration until I change vm.
 */
extern Address vmMemEnd;

extern int main_PanicOK;	/* Set to 1 when we've done enough
				 * initialization to panic. */


/*
 * Flags defined in mainHook.c to modify startup behavior.
 */
extern Boolean main_Debug;      /* If TRUE then enter the debugger */
extern Boolean main_DoProf;     /* If TRUE then start profiling */
extern Boolean main_DoDumpInit; /* If TRUE then initialize dump routines */
extern int main_NumRpcServers;  /* # of rpc servers to spawn off */
extern char   *main_AltInit;    /* If non-null, then it gives name of
                                 * alternate init program. */
extern Boolean main_AllowNMI;   /* If TRUE then allow non-maskable interrupts.*/

#endif /* _MAIN */
