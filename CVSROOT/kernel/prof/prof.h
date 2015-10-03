/*
 * prof.h --
 *
 *	External declarations of the profile module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _PROF
#define _PROF

#include "sprite.h"
#include "proc.h"
#include "procMigrate.h"
#include "timer.h"

/* procedures */

extern void Prof_Init _ARGS_((void));
extern ReturnStatus Prof_Start _ARGS_((void));

#ifdef sun3
extern void Prof_CollectInfo _ARGS_((Mach_IntrStack *stackPtr));
#else
extern void Prof_CollectInfo _ARGS_((unsigned int pc));
#endif

extern ReturnStatus Prof_End _ARGS_((void));
extern ReturnStatus Prof_Dump _ARGS_((char *dumpName));
extern ReturnStatus Prof_DumpStub _ARGS_((char *pathName));
extern ReturnStatus Prof_Profil _ARGS_((short *buffer, int bufSize, int offset, int scale));
extern void Prof_Enable _ARGS_((Proc_ControlBlock *procPtr, short *buffer, int bufSize, int offset, int scale));
extern void Prof_RecordPC _ARGS_((Proc_ControlBlock *procPtr));
extern void Prof_Disable _ARGS_((Proc_ControlBlock *procPtr));

/*
 * Procedures to support process migration.
 */

extern ReturnStatus Prof_GetEncapSize _ARGS_((Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr));
extern ReturnStatus Prof_EncapState _ARGS_((register Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr, Address ptr));
extern ReturnStatus Prof_DeencapState _ARGS_((register Proc_ControlBlock *procPtr, Proc_EncapInfo *infoPtr, Address ptr));

#endif /* _PROF */

