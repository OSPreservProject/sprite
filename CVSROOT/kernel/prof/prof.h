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

/* procedures */

extern void		Prof_Init();
extern ReturnStatus	Prof_Start();
extern ReturnStatus	Prof_End();
extern ReturnStatus     Prof_Profil();
extern void		Prof_CollectInfo();
extern ReturnStatus	Prof_Dump();
extern ReturnStatus	Prof_DumpStub();

#endif /* _PROF */

