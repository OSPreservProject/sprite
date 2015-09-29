/* 
 * rawvm.c --
 *
 *	Print raw format VM statistics.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /a/newcmds/rawstat/RCS/rawvm.c,v 1.4 89/10/17 08:06:46 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "stdio.h"
#include "vmStat.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintRawVmStat --
 *
 *	Prints out statistics for the VM system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawVmStat(X)
    Vm_Stat *X;
{
    printf("Vm_Stat\n");
	ZeroPrint("numPhysPages   %8u\n", X->numPhysPages);
	ZeroPrint("numFreePages   %8u\n", X->numFreePages);
	ZeroPrint("numDirtyPages  %8u\n", X->numDirtyPages);
	ZeroPrint("numReservePages %8u\n", X->numReservePages);
	ZeroPrint("numUserPages   %8u\n", X->numUserPages);
	ZeroPrint("kernStackPages %8u\n", X->kernStackPages);
	ZeroPrint("kernMemPages   %8u\n", X->kernMemPages);
	ZeroPrint("totalFaults    %8u\n", X->totalFaults);
	ZeroPrint("totalUserFaults %8u\n", X->totalUserFaults);
	ZeroPrint("zeroFilled     %8u\n", X->zeroFilled);
	ZeroPrint("fsFilled       %8u\n", X->fsFilled);
	ZeroPrint("psFilled       %8u\n", X->psFilled);
	ZeroPrint("collFaults     %8u\n", X->collFaults);
	ZeroPrint("quickFaults    %8u\n", X->quickFaults);
	ZeroPrint("codeFaults     %8u\n", X->codeFaults);
	ZeroPrint("heapFaults     %8u\n", X->heapFaults);
	ZeroPrint("stackFaults    %8u\n", X->stackFaults);
	ZeroPrint("numAllocs      %8u\n", X->numAllocs);
	ZeroPrint("gotFreePage    %8u\n", X->gotFreePage);
	ZeroPrint("pageAllocs     %8u\n", X->pageAllocs);
	ZeroPrint("gotPageFromFS  %8u\n", X->gotPageFromFS);
	ZeroPrint("numListSearches %8u\n", X->numListSearches);
	ZeroPrint("usedFreePage   %8u\n", X->usedFreePage);
	ZeroPrint("lockSearched   %8u\n", X->lockSearched);
	ZeroPrint("refSearched    %8u\n", X->refSearched);
	ZeroPrint("dirtySearched  %8u\n", X->dirtySearched);
	ZeroPrint("reservePagesUsed %8u\n", X->reservePagesUsed);
	ZeroPrint("pagesWritten   %8u\n", X->pagesWritten);
	ZeroPrint("cleanWait      %8u\n", X->cleanWait);
	ZeroPrint("pageoutWakeup  %8u\n", X->pageoutWakeup);
	ZeroPrint("pageoutNoWork  %8u\n", X->pageoutNoWork);
	ZeroPrint("pageoutWait    %8u\n", X->pageoutWait);
	ZeroPrint("mapPageWait    %8u\n", X->mapPageWait);
	ZeroPrint("accessWait     %8u\n", X->accessWait);

	ZeroPrint("minVMPages     %8u\n", X->minVMPages);
	ZeroPrint("fsAsked        %8u\n", X->fsAsked);
	ZeroPrint("haveFreePage   %8u\n", X->haveFreePage);
	ZeroPrint("fsMap          %8u\n", X->fsMap);
	ZeroPrint("fsUnmap        %8u\n", X->fsUnmap);
	ZeroPrint("maxFSPages     %8u\n", X->maxFSPages);
	ZeroPrint("minFSPages     %8u\n", X->minFSPages);
	ZeroPrint("numCOWHeapPages %8u\n", X->numCOWHeapPages);
	ZeroPrint("numCOWStkPages %8u\n", X->numCOWStkPages);
	ZeroPrint("numCORHeapPages %8u\n", X->numCORHeapPages);
	ZeroPrint("numCORStkPages %8u\n", X->numCORStkPages);
	ZeroPrint("numCOWHeapFaults %8u\n", X->numCOWHeapFaults);
	ZeroPrint("numCOWStkFaults %8u\n", X->numCOWStkFaults);
	ZeroPrint("quickCOWFaults %8u\n", X->quickCOWFaults);
	ZeroPrint("numCORHeapFaults %8u\n", X->numCORHeapFaults);
	ZeroPrint("numCORStkFaults %8u\n", X->numCORStkFaults);
	ZeroPrint("quickCORFaults %8u\n", X->quickCORFaults);
	ZeroPrint("swapPagesCopied %8u\n", X->swapPagesCopied);
	ZeroPrint("numCORCOWHeapFaults %8u\n", X->numCORCOWHeapFaults);
	ZeroPrint("numCORCOWStkFaults %8u\n", X->numCORCOWStkFaults);
	ZeroPrint("potModPages    %8u\n", X->potModPages);
	ZeroPrint("notModPages    %8u\n", X->notModPages);
	ZeroPrint("notHardModPages %8u\n", X->notHardModPages);
	ZeroPrint("codePrefetches %8u\n", X->codePrefetches);
	ZeroPrint("heapSwapPrefetches %8u\n", X->heapSwapPrefetches);
	ZeroPrint("heapFSPrefetches %8u\n", X->heapFSPrefetches);
	ZeroPrint("stackPrefetches %8u\n", X->stackPrefetches);
	ZeroPrint("codePrefetchHits %8u\n", X->codePrefetchHits);
	ZeroPrint("heapSwapPrefetchHits %8u\n", X->heapSwapPrefetchHits);
	ZeroPrint("heapFSPrefetchHits %8u\n", X->heapFSPrefetchHits);
	ZeroPrint("stackPrefetchHits %8u\n", X->stackPrefetchHits);
	ZeroPrint("prefetchAborts %8u\n", X->prefetchAborts);

	VmMach_PrintStats(X);

}
