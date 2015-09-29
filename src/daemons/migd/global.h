/*
 * global.h --
 *
 *	Declarations of functions and variables for the migration
 *	daemon global daemon.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Global: /sprite/lib/forms/RCS/proto.h,v 1.5 90/01/12 12:03:25 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _GLOBAL
#define _GLOBAL

#include <mig.h>

extern int global_Debug;	/* Debugging level for global daemon. */
extern int global_CheckpointInterval;	/* Interval for saving checkpoints. */
extern Mig_Stats global_Stats;		/* Statistics. */

extern int 	Global_Init();
extern void 	Global_End();
extern void 	Global_Quit();
extern int 	Global_GetLoadInfo();
extern int 	Global_GetIdle();
extern int 	Global_DoneIoctl();
extern int 	Global_Done();
extern int 	Global_RemoveHost();
extern int 	Global_ChangeState();
extern int 	Global_HostUp();
extern int 	Global_IsHostUp();
extern int 	Global_GetStats();
extern int 	Global_ResetStats();
extern int 	Global_SetParms();
extern int 	Global_GetUpdate();






#endif /* _GLOBAL */
