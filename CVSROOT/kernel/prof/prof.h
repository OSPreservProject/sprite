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

#ifdef __STDC__
extern ReturnStatus     Prof_Profil(short *buffer, int bufSize,
                            int offset, int scale);
extern void             Prof_Tick(Timer_Ticks time, 
                            ClientData clientData);
extern void             Prof_Disable(Proc_ControlBlock *procPtr);
extern void             Prof_Enable(Proc_ControlBlock *procPtr, short *buffer,
                            int bufSize, int offset, int scale);
extern void             Prof_RecordPC(Proc_ControlBlock *procPtr);
extern void		Prof_Init(void);
extern ReturnStatus	Prof_Start(void);
extern ReturnStatus	Prof_End(void);
extern void		Prof_CollectInfo();
extern ReturnStatus	Prof_Dump(char *dumpName);
extern ReturnStatus	Prof_DumpStub(char *pathName);
#else
extern ReturnStatus     Prof_Profil();
extern void             Prof_Tick();
extern void             Prof_Disable();
extern void             Prof_Enable();
extern void             Prof_RecordPC();
extern void		Prof_Init();
extern ReturnStatus	Prof_Start();
extern ReturnStatus	Prof_End();
extern ReturnStatus     Prof_Profil();
extern void		Prof_CollectInfo();
extern ReturnStatus	Prof_Dump();
extern ReturnStatus	Prof_DumpStub();

#endif

/*
 * Procedures to support process migration.
 */
#ifdef __STDC__
extern ReturnStatus	Prof_GetEncapSize(Proc_ControlBlock *procPtr,
                            int hostID, Proc_EncapInfo *infoPtr);
extern ReturnStatus	Prof_EncapState(Proc_ControlBlock *procPtr, int hostID,
                            Proc_EncapInfo *infoPtr, Address ptr);
extern ReturnStatus	Prof_DeencapState(Proc_ControlBlock *procPtr,
                            Proc_EncapInfo *infoPtr, Address ptr);
#else
extern ReturnStatus	Prof_GetEncapSize();
extern ReturnStatus	Prof_EncapState();
extern ReturnStatus	Prof_DeencapState();
#endif
#endif /* _PROF */

