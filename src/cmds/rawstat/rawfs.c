/* 
 * rawfs.c --
 *
 *	Print raw format FS statistics.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawfs.c,v 1.7 90/09/24 14:40:29 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "stdio.h"
#include "sysStats.h"
#include "kernel/fsStat.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsCltName --
 *
 *	Prints fsStats.cltName.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsCltName(X)
    Fs_NameOpStats *X;
{
    printf("fsStats.cltName\n");
    ZeroPrint("numReadOpens   %8u\n", X->numReadOpens);
    ZeroPrint("numWriteOpens  %8u\n", X->numWriteOpens);
    ZeroPrint("numReadWriteOpens %8u\n", X->numReadWriteOpens);
    ZeroPrint("chdirs         %8u\n", X->chdirs);
    ZeroPrint("makeDevices    %8u\n", X->makeDevices);
    ZeroPrint("makeDirs       %8u\n", X->makeDirs);
    ZeroPrint("removes        %8u\n", X->removes);
    ZeroPrint("removeDirs     %8u\n", X->removeDirs);
    ZeroPrint("renames        %8u\n", X->renames);
    ZeroPrint("hardLinks      %8u\n", X->hardLinks);
    ZeroPrint("symLinks       %8u\n", X->symLinks);
    ZeroPrint("getAttrs       %8u\n", X->getAttrs);
    ZeroPrint("setAttrs       %8u\n", X->setAttrs);
    ZeroPrint("getAttrIDs     %8u\n", X->getAttrIDs);
    ZeroPrint("setAttrIDs     %8u\n", X->setAttrIDs);
    ZeroPrint("getIOAttrs     %8u\n", X->getIOAttrs);
    ZeroPrint("setIOAttrs     %8u\n", X->setIOAttrs);

}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsSrvName --
 *
 *	Prints fsStats.srvName.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsSrvName(X)
    Fs_NameOpStats *X;
{
    printf("fsStats.srvName\n");
    ZeroPrint("numReadOpens   %8u\n", X->numReadOpens);
    ZeroPrint("numWriteOpens  %8u\n", X->numWriteOpens);
    ZeroPrint("numReadWriteOpens %8u\n", X->numReadWriteOpens);
    ZeroPrint("chdirs         %8u\n", X->chdirs);
    ZeroPrint("makeDevices    %8u\n", X->makeDevices);
    ZeroPrint("makeDirs       %8u\n", X->makeDirs);
    ZeroPrint("removes        %8u\n", X->removes);
    ZeroPrint("removeDirs     %8u\n", X->removeDirs);
    ZeroPrint("renames        %8u\n", X->renames);
    ZeroPrint("hardLinks      %8u\n", X->hardLinks);
    ZeroPrint("symLinks       %8u\n", X->symLinks);
    ZeroPrint("getAttrs       %8u\n", X->getAttrs);
    ZeroPrint("setAttrs       %8u\n", X->setAttrs);
    ZeroPrint("getAttrIDs     %8u\n", X->getAttrIDs);
    ZeroPrint("setAttrIDs     %8u\n", X->setAttrIDs);
    ZeroPrint("getIOAttrs     %8u\n", X->getIOAttrs);
    ZeroPrint("setIOAttrs     %8u\n", X->setIOAttrs);

}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsGen --
 *
 *	Prints fsStats.gen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsGen(X)
    Fs_GeneralStats *X;
{
    printf("fsStats.gen\n");
    ZeroPrint("physBytesRead  %8u\n", X->physBytesRead);
    ZeroPrint("fileBytesRead  %8u\n", X->fileBytesRead);
    ZeroPrint("fileReadOverflow %8u\n", X->fileReadOverflow);
    ZeroPrint("remoteBytesRead %8u\n", X->remoteBytesRead);
    ZeroPrint("remoteReadOverflow %8u\n", X->remoteReadOverflow);
    ZeroPrint("deviceBytesRead %8u\n", X->deviceBytesRead);
    ZeroPrint("physBytesWritten %8u\n", X->physBytesWritten);
    ZeroPrint("fileBytesWritten %8u\n", X->fileBytesWritten);
    ZeroPrint("fileWriteOverflow %8u\n", X->fileWriteOverflow);
    ZeroPrint("remoteBytesWritten %8u\n", X->remoteBytesWritten);
    ZeroPrint("remoteWriteOverflow %8u\n", X->remoteWriteOverflow);
    ZeroPrint("deviceBytesWritten %8u\n", X->deviceBytesWritten);
    ZeroPrint("fileBytesDeleted %8u\n", X->fileBytesDeleted);
    ZeroPrint("fileDeleteOverflow %8u\n", X->fileDeleteOverflow);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsBlockCache --
 *
 *	Prints fsStats.blockCache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsBlockCache(X)
    Fs_BlockCacheStats *X;
{
    printf("fsStats.blockCache\n");
    ZeroPrint("readAccesses   %8u\n", X->readAccesses);
    ZeroPrint("bytesRead      %8u\n", X->bytesRead);
    ZeroPrint("bytesReadOverflow %8u\n", X->bytesReadOverflow);
    ZeroPrint("readHitsOnDirtyBlock %8u\n", X->readHitsOnDirtyBlock);
    ZeroPrint("readHitsOnCleanBlock %8u\n", X->readHitsOnCleanBlock);
    ZeroPrint("readZeroFills  %8u\n", X->readZeroFills);
    ZeroPrint("domainReadFails %8u\n", X->domainReadFails);
    ZeroPrint("readAheads     %8u\n", X->readAheads);
    ZeroPrint("readAheadHits  %8u\n", X->readAheadHits);
    ZeroPrint("allInCacheCalls %8u\n", X->allInCacheCalls);
    ZeroPrint("allInCacheTrue %8u\n", X->allInCacheTrue);
    ZeroPrint("writeAccesses  %8u\n", X->writeAccesses);
    ZeroPrint("bytesWritten   %8u\n", X->bytesWritten);
    ZeroPrint("bytesWrittenOverflow %8u\n", X->bytesWrittenOverflow);
    ZeroPrint("appendWrites   %8u\n", X->appendWrites);
    ZeroPrint("overWrites     %8u\n", X->overWrites);
    ZeroPrint("writeZeroFills1 %8u\n", X->writeZeroFills1);
    ZeroPrint("writeZeroFills2 %8u\n", X->writeZeroFills2);
    ZeroPrint("partialWriteHits %8u\n", X->partialWriteHits);
    ZeroPrint("partialWriteMisses %8u\n", X->partialWriteMisses);
    ZeroPrint("blocksWrittenThru %8u\n", X->blocksWrittenThru);
    ZeroPrint("dataBlocksWrittenThru %8u\n", X->dataBlocksWrittenThru);
    ZeroPrint("indBlocksWrittenThru %8u\n", X->indBlocksWrittenThru);
    ZeroPrint("descBlocksWrittenThru %8u\n", X->descBlocksWrittenThru);
    ZeroPrint("dirBlocksWrittenThru %8u\n", X->dirBlocksWrittenThru);
    ZeroPrint("fragAccesses   %8u\n", X->fragAccesses);
    ZeroPrint("fragHits       %8u\n", X->fragHits);
    ZeroPrint("fragZeroFills  %8u\n", X->fragZeroFills);
    ZeroPrint("fileDescReads  %8u\n", X->fileDescReads);
    ZeroPrint("fileDescReadHits %8u\n", X->fileDescReadHits);
    ZeroPrint("fileDescWrites %8u\n", X->fileDescWrites);
    ZeroPrint("fileDescWriteHits %8u\n", X->fileDescWriteHits);
    ZeroPrint("indBlockAccesses %8u\n", X->indBlockAccesses);
    ZeroPrint("indBlockHits   %8u\n", X->indBlockHits);
    ZeroPrint("indBlockWrites %8u\n", X->indBlockWrites);
    ZeroPrint("dirBlockAccesses %8u\n", X->dirBlockAccesses);
    ZeroPrint("dirBlockHits   %8u\n", X->dirBlockHits);
    ZeroPrint("dirBlockWrites %8u\n", X->dirBlockWrites);
    ZeroPrint("dirBytesRead   %8u\n", X->dirBytesRead);
    ZeroPrint("dirBytesWritten %8u\n", X->dirBytesWritten);
    ZeroPrint("vmRequests     %8u\n", X->vmRequests);
    ZeroPrint("triedToGiveToVM %8u\n", X->triedToGiveToVM);
    ZeroPrint("vmGotPage      %8u\n", X->vmGotPage);
    ZeroPrint("partFree       %8u\n", X->partFree);
    ZeroPrint("totFree        %8u\n", X->totFree);
    ZeroPrint("unmapped       %8u\n", X->unmapped);
    ZeroPrint("lru            %8u\n", X->lru);
    ZeroPrint("minCacheBlocks %8u\n", X->minCacheBlocks);
    ZeroPrint("maxCacheBlocks %8u\n", X->maxCacheBlocks);
    ZeroPrint("maxNumBlocks   %8u\n", X->maxNumBlocks);
    ZeroPrint("numCacheBlocks %8u\n", X->numCacheBlocks);
    ZeroPrint("numFreeBlocks  %8u\n", X->numFreeBlocks);
    ZeroPrint("blocksPitched  %8u\n", X->blocksPitched);
    ZeroPrint("blocksFlushed  %8u\n", X->blocksFlushed);
    ZeroPrint("migBlocksFlushed  %8u\n", X->migBlocksFlushed);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsAlloc --
 *
 *	Prints fsStats.alloc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsAlloc(X)
    Fs_AllocStats *X;
{
    printf("fsStats.alloc\n");
    ZeroPrint("blocksAllocated %8u\n", X->blocksAllocated);
    ZeroPrint("blocksFreed    %8u\n", X->blocksFreed);
    ZeroPrint("cylsSearched   %8u\n", X->cylsSearched);
    ZeroPrint("cylHashes      %8u\n", X->cylHashes);
    ZeroPrint("cylBitmapSearches %8u\n", X->cylBitmapSearches);
    ZeroPrint("fragsAllocated %8u\n", X->fragsAllocated);
    ZeroPrint("fragsFreed     %8u\n", X->fragsFreed);
    ZeroPrint("fragToBlock    %8u\n", X->fragToBlock);
    ZeroPrint("fragUpgrades   %8u\n", X->fragUpgrades);
    ZeroPrint("fragsUpgraded  %8u\n", X->fragsUpgraded);
    ZeroPrint("badFragList    %8u\n", X->badFragList);
    ZeroPrint("fullBlockFrags %8u\n", X->fullBlockFrags);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsHandle --
 *
 *	Prints fsStats.alloc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsHandle(X)
    Fs_HandleStats *X;
{
    printf("fsStats.handle\n");
    ZeroPrint("maxNumber      %8u\n", X->maxNumber);
    ZeroPrint("exists         %8u\n", X->exists);
    ZeroPrint("installCalls   %8u\n", X->installCalls);
    ZeroPrint("installHits    %8u\n", X->installHits);
    ZeroPrint("fetchCalls     %8u\n", X->fetchCalls);
    ZeroPrint("fetchHits      %8u\n", X->fetchHits);
    ZeroPrint("release        %8u\n", X->release);
    ZeroPrint("locks          %8u\n", X->locks);
    ZeroPrint("lockWaits      %8u\n", X->lockWaits);
    ZeroPrint("unlocks        %8u\n", X->unlocks);
    ZeroPrint("created        %8u\n", X->created);
    ZeroPrint("lruScans       %8u\n", X->lruScans);
    ZeroPrint("lruChecks      %8u\n", X->lruChecks);
    ZeroPrint("lruHits        %8u\n", X->lruHits);
    ZeroPrint("lruEntries     %8u\n", X->lruEntries);
    ZeroPrint("limbo          %8u\n", X->limbo);
    ZeroPrint("versionMismatch %8u\n", X->versionMismatch);
    ZeroPrint("cacheFlushes   %8u\n", X->cacheFlushes);
    ZeroPrint("segmentFetches %8u\n", X->segmentFetches);
    ZeroPrint("segmentHits    %8u\n", X->segmentHits);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsNameCache --
 *
 *	Prints fsStats.nameCache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsNameCache(X)
    Fs_NameCacheStats *X;
{
    printf("fsStats.nameCache\n");
    ZeroPrint("accesses       %8u\n", X->accesses);
    ZeroPrint("hits           %8u\n", X->hits);
    ZeroPrint("replacements   %8u\n", X->replacements);
    ZeroPrint("size           %8u\n", X->size);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsPrefix --
 *
 *	Prints fsStats.prefix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsPrefix(X)
	Fs_PrefixStats *X;
{
    printf("fsStats.prefix\n");
    ZeroPrint("relative       %8u\n", X->relative);
    ZeroPrint("absolute       %8u\n", X->absolute);
    ZeroPrint("redirects      %8u\n", X->redirects);
    ZeroPrint("loops          %8u\n", X->loops);
    ZeroPrint("timeouts       %8u\n", X->timeouts);
    ZeroPrint("stale          %8u\n", X->stale);
    ZeroPrint("found          %8u\n", X->found);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsLookup --
 *
 *	Prints fsStats.lookup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsLookup(X)
    Fs_LookupStats *X;
{
    printf("fsStats.lookup\n");
    ZeroPrint("number         %8u\n", X->number);
    ZeroPrint("numComponents  %8u\n", X->numComponents);
    ZeroPrint("numSpecial     %8u\n", X->numSpecial);
    ZeroPrint("forDelete      %8u\n", X->forDelete);
    ZeroPrint("forLink        %8u\n", X->forLink);
    ZeroPrint("forRename      %8u\n", X->forRename);
    ZeroPrint("forCreate      %8u\n", X->forCreate);
    ZeroPrint("symlinks       %8u\n", X->symlinks);
    ZeroPrint("redirect       %8u\n", X->redirect);
    ZeroPrint("remote         %8u\n", X->remote);
    ZeroPrint("parent         %8u\n", X->parent);
    ZeroPrint("notFound       %8u\n", X->notFound);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsObject --
 *
 *	Prints fsStats.object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsObject(X)
	Fs_ObjectStats *X;
{
    printf("fsStats.object\n");
    ZeroPrint("streams        %8u\n", X->streams);
    ZeroPrint("streamClients  %8u\n", X->streamClients);
    ZeroPrint("files          %8u\n", X->files);
    ZeroPrint("rmtFiles       %8u\n", X->rmtFiles);
    ZeroPrint("pipes          %8u\n", X->pipes);
    ZeroPrint("devices        %8u\n", X->devices);
    ZeroPrint("controls       %8u\n", X->controls);
    ZeroPrint("pseudoStreams  %8u\n", X->pseudoStreams);
    ZeroPrint("remote         %8u\n", X->remote);
    ZeroPrint("directory      %8u\n", X->directory);
    ZeroPrint("dirFlushed     %8u\n", X->dirFlushed);
    ZeroPrint("fileClients    %8u\n", X->fileClients);
    ZeroPrint("other          %8u\n", X->other);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsRecovery --
 *
 *	Prints fsStats.recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsRecovery(X)
	Fs_RecoveryStats *X;
{
    printf("fsStats.recovery\n");
    ZeroPrint("number         %8u\n", X->number);
    ZeroPrint("wants          %8u\n", X->wants);
    ZeroPrint("waitOK         %8u\n", X->waitOK);
    ZeroPrint("waitFailed     %8u\n", X->waitFailed);
    ZeroPrint("waitAbort      %8u\n", X->waitAbort);
    ZeroPrint("timeout        %8u\n", X->timeout);
    ZeroPrint("failed         %8u\n", X->failed);
    ZeroPrint("deleted        %8u\n", X->deleted);
    ZeroPrint("offline        %8u\n", X->offline);
    ZeroPrint("succeeded      %8u\n", X->succeeded);
    ZeroPrint("clientCrashed  %8u\n", X->clientCrashed);
    ZeroPrint("clientRecovered %8u\n", X->clientRecovered);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsConsist --
 *
 *	Prints fsStats.consist.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsConsist(X)
	Fs_ConsistStats *X;
{
    printf("fsStats.consist\n");
    ZeroPrint("files          %8d\n", X->files);
    ZeroPrint("clients        %8d\n", X->clients);
    ZeroPrint("notCaching     %8d\n", X->notCaching);
    ZeroPrint("readCachingMyself    %8d\n", X->readCachingMyself);
    ZeroPrint("readCachingOther    %8d\n", X->readCachingOther);
    ZeroPrint("writeCaching   %8d\n", X->writeCaching);
    ZeroPrint("writeBack      %8d\n", X->writeBack);
    ZeroPrint("readInvalidate %8d\n", X->readInvalidate);
    ZeroPrint("writeInvalidate %8d\n", X->writeInvalidate);
    ZeroPrint("nonFiles       %8d\n", X->nonFiles);
    ZeroPrint("swap           %8d\n", X->swap);
    ZeroPrint("cacheable      %8d\n", X->cacheable);
    ZeroPrint("uncacheable    %8d\n", X->uncacheable);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsWriteBack --
 *
 *	Prints fsStats.writeBack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsWriteBack(X)
	Fs_WriteBackStats *X;
{
    printf("fsStats.writeBack\n");
    ZeroPrint("passes         %8d\n", X->passes);
    ZeroPrint("files          %8d\n", X->files);
    ZeroPrint("blocks         %8d\n", X->blocks);
    ZeroPrint("maxBlocks         %8d\n", X->maxBlocks);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawRemoteIO --
 *
 *	Prints fsStats.rmtIO.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawRemoteIO(X)
	Fs_RemoteIOStats *X;
{
    printf("fsStats.rmtIO\n");
    ZeroPrint("bytesReadForCache %8d\n", X->bytesReadForCache);
    ZeroPrint("bytesWrittenFromCache %8d\n", X->bytesWrittenFromCache);
    ZeroPrint("uncacheableBytesRead %8d\n", X->uncacheableBytesRead);
    ZeroPrint("uncacheableBytesWritten %8d\n", X->uncacheableBytesWritten);
    ZeroPrint("sharedStreamBytesRead %8d\n", X->sharedStreamBytesRead);
    ZeroPrint("sharedStreamBytesWritten %8d\n", X->sharedStreamBytesWritten);
    ZeroPrint("hitsOnVMBlock %8d\n", X->hitsOnVMBlock);
    ZeroPrint("missesOnVMBlock %8d\n", X->missesOnVMBlock);
    ZeroPrint("bytesReadForVM %8d\n", X->bytesReadForVM);
    ZeroPrint("bytesWrittenForVM %8d\n", X->bytesWrittenForVM);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawFsMig --
 *
 *	Prints fsStats.mig.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawFsMig(X)
	Fs_MigStats *X;
{
    printf("fsStats.mig\n");
    ZeroPrint("filesEncapsulated %8d\n", X->filesEncapsulated);
    ZeroPrint("encapSquared %8d\n", X->encapSquared);
    ZeroPrint("filesDeencapsulated %8d\n", X->filesDeencapsulated);
    ZeroPrint("deencapSquared %8d\n", X->deencapSquared);
    ZeroPrint("consistActions %8d\n", X->consistActions);
    ZeroPrint("readOnlyFiles %8d\n", X->readOnlyFiles);
    ZeroPrint("alreadyThere %8d\n", X->alreadyThere);
    ZeroPrint("uncacheableFiles %8d\n", X->uncacheableFiles);
    ZeroPrint("cacheWritableFiles %8d\n", X->cacheWritableFiles);
    ZeroPrint("uncacheToCacheFiles %8d\n", X->uncacheToCacheFiles);
    ZeroPrint("cacheToUncacheFiles %8d\n", X->cacheToUncacheFiles);
    ZeroPrint("errorsOnDeencap %8d\n", X->errorsOnDeencap);
}

