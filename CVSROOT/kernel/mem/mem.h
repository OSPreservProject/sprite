/*
 * mem.h --
 *
 *     Memory manager procedure headers exported by the memory module.
 *
 * Copyright 1990 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MEM
#define _MEM

#include <sprite.h>
#include <stdlib.h>

extern void Mem_DumpStats _ARGS_((void));
extern void Mem_Bin _ARGS_((int numBytes));
extern void Mem_DumpTrace _ARGS_((int blocksize));
extern void Mem_PrintConfig _ARGS_((void));
extern void Mem_PrintInUse _ARGS_((void));
extern void Mem_PrintStats _ARGS_((void));
extern void Mem_PrintStatsInt _ARGS_((void));
extern void Mem_SetPrintProc _ARGS_((void (*proc)(),ClientData data));
extern int Mem_Size _ARGS_((Address blockPtr));

extern void Mem_PrintStatsSubrInt _ARGS_((void (*PrintProc)(),
	ClientData clientData, int smallMinNum, int largeMinNum,
	int largeMaxSize));

extern void Mem_PrintStatsSubr _ARGS_((void (*PrintProc)(),
	ClientData clientData, int smallMinNum, int largeMinNum,
	int largeMaxSize));
extern void Mem_PrintConfigSubr _ARGS_((void (*PrintProc)(),
	ClientData clientData));
extern void Mem_SetTraceSizes _ARGS_((int numSizes, Mem_TraceInfo *arrayPtr));


#endif /* _MEM */
