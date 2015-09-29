if (printzero || statsPtr->log.segWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.segWrites.low %u\n",statsPtr->log.segWrites.low);
if (printzero || statsPtr->log.segWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.segWrites.high %u\n",statsPtr->log.segWrites.high);
if (printzero || statsPtr->log.partialWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialWrites.low %u\n",statsPtr->log.partialWrites.low);
if (printzero || statsPtr->log.partialWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialWrites.high %u\n",statsPtr->log.partialWrites.high);
if (printzero || statsPtr->log.emptyWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.emptyWrites.low %u\n",statsPtr->log.emptyWrites.low);
if (printzero || statsPtr->log.emptyWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.emptyWrites.high %u\n",statsPtr->log.emptyWrites.high);
if (printzero || statsPtr->log.blocksWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.blocksWritten.low %u\n",statsPtr->log.blocksWritten.low);
if (printzero || statsPtr->log.blocksWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.blocksWritten.high %u\n",statsPtr->log.blocksWritten.high);
if (printzero || statsPtr->log.bytesWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.bytesWritten.low %u\n",statsPtr->log.bytesWritten.low);
if (printzero || statsPtr->log.bytesWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.bytesWritten.high %u\n",statsPtr->log.bytesWritten.high);
if (printzero || statsPtr->log.summaryBlocksWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.summaryBlocksWritten.low %u\n",statsPtr->log.summaryBlocksWritten.low);
if (printzero || statsPtr->log.summaryBlocksWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.summaryBlocksWritten.high %u\n",statsPtr->log.summaryBlocksWritten.high);
if (printzero || statsPtr->log.summaryBytesWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.summaryBytesWritten.low %u\n",statsPtr->log.summaryBytesWritten.low);
if (printzero || statsPtr->log.summaryBytesWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.summaryBytesWritten.high %u\n",statsPtr->log.summaryBytesWritten.high);
if (printzero || statsPtr->log.wasteBlocks.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.wasteBlocks.low %u\n",statsPtr->log.wasteBlocks.low);
if (printzero || statsPtr->log.wasteBlocks.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.wasteBlocks.high %u\n",statsPtr->log.wasteBlocks.high);
if (printzero || statsPtr->log.newSegments.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.newSegments.low %u\n",statsPtr->log.newSegments.low);
if (printzero || statsPtr->log.newSegments.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.newSegments.high %u\n",statsPtr->log.newSegments.high);
if (printzero || statsPtr->log.cleanSegWait.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanSegWait.low %u\n",statsPtr->log.cleanSegWait.low);
if (printzero || statsPtr->log.cleanSegWait.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanSegWait.high %u\n",statsPtr->log.cleanSegWait.high);
if (printzero || statsPtr->log.useOldSegment.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.useOldSegment.low %u\n",statsPtr->log.useOldSegment.low);
if (printzero || statsPtr->log.useOldSegment.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.useOldSegment.high %u\n",statsPtr->log.useOldSegment.high);
if (printzero || statsPtr->log.locks.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.locks.low %u\n",statsPtr->log.locks.low);
if (printzero || statsPtr->log.locks.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.locks.high %u\n",statsPtr->log.locks.high);
if (printzero || statsPtr->log.lockWaits.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.lockWaits.low %u\n",statsPtr->log.lockWaits.low);
if (printzero || statsPtr->log.lockWaits.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.lockWaits.high %u\n",statsPtr->log.lockWaits.high);
if (printzero || statsPtr->log.fsyncWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncWrites.low %u\n",statsPtr->log.fsyncWrites.low);
if (printzero || statsPtr->log.fsyncWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncWrites.high %u\n",statsPtr->log.fsyncWrites.high);
if (printzero || statsPtr->log.fsyncPartialWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncPartialWrites.low %u\n",statsPtr->log.fsyncPartialWrites.low);
if (printzero || statsPtr->log.fsyncPartialWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncPartialWrites.high %u\n",statsPtr->log.fsyncPartialWrites.high);
if (printzero || statsPtr->log.fsyncBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncBytes.low %u\n",statsPtr->log.fsyncBytes.low);
if (printzero || statsPtr->log.fsyncBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncBytes.high %u\n",statsPtr->log.fsyncBytes.high);
if (printzero || statsPtr->log.fsyncPartialBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncPartialBytes.low %u\n",statsPtr->log.fsyncPartialBytes.low);
if (printzero || statsPtr->log.fsyncPartialBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fsyncPartialBytes.high %u\n",statsPtr->log.fsyncPartialBytes.high);
if (printzero || statsPtr->log.partialWriteBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialWriteBytes.low %u\n",statsPtr->log.partialWriteBytes.low);
if (printzero || statsPtr->log.partialWriteBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialWriteBytes.high %u\n",statsPtr->log.partialWriteBytes.high);
if (printzero || statsPtr->log.cleanPartialWriteBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanPartialWriteBytes.low %u\n",statsPtr->log.cleanPartialWriteBytes.low);
if (printzero || statsPtr->log.cleanPartialWriteBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanPartialWriteBytes.high %u\n",statsPtr->log.cleanPartialWriteBytes.high);
if (printzero || statsPtr->log.fileBytesWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fileBytesWritten.low %u\n",statsPtr->log.fileBytesWritten.low);
if (printzero || statsPtr->log.fileBytesWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.fileBytesWritten.high %u\n",statsPtr->log.fileBytesWritten.high);
if (printzero || statsPtr->log.cleanFileBytesWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanFileBytesWritten.low %u\n",statsPtr->log.cleanFileBytesWritten.low);
if (printzero || statsPtr->log.cleanFileBytesWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.cleanFileBytesWritten.high %u\n",statsPtr->log.cleanFileBytesWritten.high);
if (printzero || statsPtr->log.partialFileBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialFileBytes.low %u\n",statsPtr->log.partialFileBytes.low);
if (printzero || statsPtr->log.partialFileBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsLogStats.log.partialFileBytes.high %u\n",statsPtr->log.partialFileBytes.high);
if (printzero || statsPtr->checkpoint.count.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.count.low %u\n",statsPtr->checkpoint.count.low);
if (printzero || statsPtr->checkpoint.count.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.count.high %u\n",statsPtr->checkpoint.count.high);
if (printzero || statsPtr->checkpoint.segWrites.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.segWrites.low %u\n",statsPtr->checkpoint.segWrites.low);
if (printzero || statsPtr->checkpoint.segWrites.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.segWrites.high %u\n",statsPtr->checkpoint.segWrites.high);
if (printzero || statsPtr->checkpoint.blocksWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.blocksWritten.low %u\n",statsPtr->checkpoint.blocksWritten.low);
if (printzero || statsPtr->checkpoint.blocksWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.blocksWritten.high %u\n",statsPtr->checkpoint.blocksWritten.high);
if (printzero || statsPtr->checkpoint.bytesWritten.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.bytesWritten.low %u\n",statsPtr->checkpoint.bytesWritten.low);
if (printzero || statsPtr->checkpoint.bytesWritten.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.bytesWritten.high %u\n",statsPtr->checkpoint.bytesWritten.high);
if (printzero || statsPtr->checkpoint.totalBlocks.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.totalBlocks.low %u\n",statsPtr->checkpoint.totalBlocks.low);
if (printzero || statsPtr->checkpoint.totalBlocks.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.totalBlocks.high %u\n",statsPtr->checkpoint.totalBlocks.high);
if (printzero || statsPtr->checkpoint.totalBytes.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.totalBytes.low %u\n",statsPtr->checkpoint.totalBytes.low);
if (printzero || statsPtr->checkpoint.totalBytes.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.totalBytes.high %u\n",statsPtr->checkpoint.totalBytes.high);
if (printzero || statsPtr->checkpoint.samples.low) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.samples.low %u\n",statsPtr->checkpoint.samples.low);
if (printzero || statsPtr->checkpoint.samples.high) 
	fprintf(outFile,"Lfs_Stats.LfsCheckPointStats.checkpoint.samples.high %u\n",statsPtr->checkpoint.samples.high);
if (printzero || statsPtr->cleaning.startRequests.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.startRequests.low %u\n",statsPtr->cleaning.startRequests.low);
if (printzero || statsPtr->cleaning.startRequests.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.startRequests.high %u\n",statsPtr->cleaning.startRequests.high);
if (printzero || statsPtr->cleaning.alreadyActive.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.alreadyActive.low %u\n",statsPtr->cleaning.alreadyActive.low);
if (printzero || statsPtr->cleaning.alreadyActive.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.alreadyActive.high %u\n",statsPtr->cleaning.alreadyActive.high);
if (printzero || statsPtr->cleaning.getSegsRequests.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.getSegsRequests.low %u\n",statsPtr->cleaning.getSegsRequests.low);
if (printzero || statsPtr->cleaning.getSegsRequests.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.getSegsRequests.high %u\n",statsPtr->cleaning.getSegsRequests.high);
if (printzero || statsPtr->cleaning.segsToClean.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segsToClean.low %u\n",statsPtr->cleaning.segsToClean.low);
if (printzero || statsPtr->cleaning.segsToClean.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segsToClean.high %u\n",statsPtr->cleaning.segsToClean.high);
if (printzero || statsPtr->cleaning.numSegsToClean.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.numSegsToClean.low %u\n",statsPtr->cleaning.numSegsToClean.low);
if (printzero || statsPtr->cleaning.numSegsToClean.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.numSegsToClean.high %u\n",statsPtr->cleaning.numSegsToClean.high);
if (printzero || statsPtr->cleaning.segReads.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segReads.low %u\n",statsPtr->cleaning.segReads.low);
if (printzero || statsPtr->cleaning.segReads.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segReads.high %u\n",statsPtr->cleaning.segReads.high);
if (printzero || statsPtr->cleaning.readErrors.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.readErrors.low %u\n",statsPtr->cleaning.readErrors.low);
if (printzero || statsPtr->cleaning.readErrors.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.readErrors.high %u\n",statsPtr->cleaning.readErrors.high);
if (printzero || statsPtr->cleaning.readEmpty.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.readEmpty.low %u\n",statsPtr->cleaning.readEmpty.low);
if (printzero || statsPtr->cleaning.readEmpty.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.readEmpty.high %u\n",statsPtr->cleaning.readEmpty.high);
if (printzero || statsPtr->cleaning.bytesCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.bytesCleaned.low %u\n",statsPtr->cleaning.bytesCleaned.low);
if (printzero || statsPtr->cleaning.bytesCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.bytesCleaned.high %u\n",statsPtr->cleaning.bytesCleaned.high);
if (printzero || statsPtr->cleaning.cacheBlocksUsed.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.cacheBlocksUsed.low %u\n",statsPtr->cleaning.cacheBlocksUsed.low);
if (printzero || statsPtr->cleaning.cacheBlocksUsed.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.cacheBlocksUsed.high %u\n",statsPtr->cleaning.cacheBlocksUsed.high);
if (printzero || statsPtr->cleaning.segWrites.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segWrites.low %u\n",statsPtr->cleaning.segWrites.low);
if (printzero || statsPtr->cleaning.segWrites.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.segWrites.high %u\n",statsPtr->cleaning.segWrites.high);
if (printzero || statsPtr->cleaning.blocksWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.blocksWritten.low %u\n",statsPtr->cleaning.blocksWritten.low);
if (printzero || statsPtr->cleaning.blocksWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.blocksWritten.high %u\n",statsPtr->cleaning.blocksWritten.high);
if (printzero || statsPtr->cleaning.bytesWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.bytesWritten.low %u\n",statsPtr->cleaning.bytesWritten.low);
if (printzero || statsPtr->cleaning.bytesWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.bytesWritten.high %u\n",statsPtr->cleaning.bytesWritten.high);
if (printzero || statsPtr->cleaning.summaryBlocksRead.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.summaryBlocksRead.low %u\n",statsPtr->cleaning.summaryBlocksRead.low);
if (printzero || statsPtr->cleaning.summaryBlocksRead.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.cleaning.summaryBlocksRead.high %u\n",statsPtr->cleaning.summaryBlocksRead.high);
if (printzero || statsPtr->blockio.reads.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.reads.low %u\n",statsPtr->blockio.reads.low);
if (printzero || statsPtr->blockio.reads.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.reads.high %u\n",statsPtr->blockio.reads.high);
if (printzero || statsPtr->blockio.bytesReads.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.bytesReads.low %u\n",statsPtr->blockio.bytesReads.low);
if (printzero || statsPtr->blockio.bytesReads.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.bytesReads.high %u\n",statsPtr->blockio.bytesReads.high);
if (printzero || statsPtr->blockio.allocs.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.allocs.low %u\n",statsPtr->blockio.allocs.low);
if (printzero || statsPtr->blockio.allocs.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.allocs.high %u\n",statsPtr->blockio.allocs.high);
if (printzero || statsPtr->blockio.fastAllocs.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.fastAllocs.low %u\n",statsPtr->blockio.fastAllocs.low);
if (printzero || statsPtr->blockio.fastAllocs.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.fastAllocs.high %u\n",statsPtr->blockio.fastAllocs.high);
if (printzero || statsPtr->blockio.slowAllocs.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.slowAllocs.low %u\n",statsPtr->blockio.slowAllocs.low);
if (printzero || statsPtr->blockio.slowAllocs.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.slowAllocs.high %u\n",statsPtr->blockio.slowAllocs.high);
if (printzero || statsPtr->blockio.slowAllocFails.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.slowAllocFails.low %u\n",statsPtr->blockio.slowAllocFails.low);
if (printzero || statsPtr->blockio.slowAllocFails.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.slowAllocFails.high %u\n",statsPtr->blockio.slowAllocFails.high);
if (printzero || statsPtr->blockio.totalBytesRead.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.totalBytesRead.low %u\n",statsPtr->blockio.totalBytesRead.low);
if (printzero || statsPtr->blockio.totalBytesRead.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.totalBytesRead.high %u\n",statsPtr->blockio.totalBytesRead.high);
if (printzero || statsPtr->blockio.totalBytesWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.totalBytesWritten.low %u\n",statsPtr->blockio.totalBytesWritten.low);
if (printzero || statsPtr->blockio.totalBytesWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.totalBytesWritten.high %u\n",statsPtr->blockio.totalBytesWritten.high);
if (printzero || statsPtr->blockio.segCacheHits.low) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.segCacheHits.low %u\n",statsPtr->blockio.segCacheHits.low);
if (printzero || statsPtr->blockio.segCacheHits.high) 
	fprintf(outFile,"LfsLogStats.LfsLogCleanStats.blockio.segCacheHits.high %u\n",statsPtr->blockio.segCacheHits.high);
if (printzero || statsPtr->desc.fetches.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetches.low %u\n",statsPtr->desc.fetches.low);
if (printzero || statsPtr->desc.fetches.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetches.high %u\n",statsPtr->desc.fetches.high);
if (printzero || statsPtr->desc.goodFetch.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.goodFetch.low %u\n",statsPtr->desc.goodFetch.low);
if (printzero || statsPtr->desc.goodFetch.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.goodFetch.high %u\n",statsPtr->desc.goodFetch.high);
if (printzero || statsPtr->desc.fetchCacheMiss.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetchCacheMiss.low %u\n",statsPtr->desc.fetchCacheMiss.low);
if (printzero || statsPtr->desc.fetchCacheMiss.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetchCacheMiss.high %u\n",statsPtr->desc.fetchCacheMiss.high);
if (printzero || statsPtr->desc.fetchSearched.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetchSearched.low %u\n",statsPtr->desc.fetchSearched.low);
if (printzero || statsPtr->desc.fetchSearched.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.fetchSearched.high %u\n",statsPtr->desc.fetchSearched.high);
if (printzero || statsPtr->desc.stores.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.stores.low %u\n",statsPtr->desc.stores.low);
if (printzero || statsPtr->desc.stores.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.stores.high %u\n",statsPtr->desc.stores.high);
if (printzero || statsPtr->desc.freeStores.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.freeStores.low %u\n",statsPtr->desc.freeStores.low);
if (printzero || statsPtr->desc.freeStores.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.freeStores.high %u\n",statsPtr->desc.freeStores.high);
if (printzero || statsPtr->desc.accessTimeUpdate.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.accessTimeUpdate.low %u\n",statsPtr->desc.accessTimeUpdate.low);
if (printzero || statsPtr->desc.accessTimeUpdate.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.accessTimeUpdate.high %u\n",statsPtr->desc.accessTimeUpdate.high);
if (printzero || statsPtr->desc.dirtyList.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.dirtyList.low %u\n",statsPtr->desc.dirtyList.low);
if (printzero || statsPtr->desc.dirtyList.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.dirtyList.high %u\n",statsPtr->desc.dirtyList.high);
if (printzero || statsPtr->desc.truncs.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.truncs.low %u\n",statsPtr->desc.truncs.low);
if (printzero || statsPtr->desc.truncs.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.truncs.high %u\n",statsPtr->desc.truncs.high);
if (printzero || statsPtr->desc.truncSizeZero.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.truncSizeZero.low %u\n",statsPtr->desc.truncSizeZero.low);
if (printzero || statsPtr->desc.truncSizeZero.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.truncSizeZero.high %u\n",statsPtr->desc.truncSizeZero.high);
if (printzero || statsPtr->desc.delete.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.delete.low %u\n",statsPtr->desc.delete.low);
if (printzero || statsPtr->desc.delete.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.delete.high %u\n",statsPtr->desc.delete.high);
if (printzero || statsPtr->desc.inits.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.inits.low %u\n",statsPtr->desc.inits.low);
if (printzero || statsPtr->desc.inits.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.inits.high %u\n",statsPtr->desc.inits.high);
if (printzero || statsPtr->desc.getNewFileNumber.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.getNewFileNumber.low %u\n",statsPtr->desc.getNewFileNumber.low);
if (printzero || statsPtr->desc.getNewFileNumber.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.getNewFileNumber.high %u\n",statsPtr->desc.getNewFileNumber.high);
if (printzero || statsPtr->desc.scans.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.scans.low %u\n",statsPtr->desc.scans.low);
if (printzero || statsPtr->desc.scans.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.scans.high %u\n",statsPtr->desc.scans.high);
if (printzero || statsPtr->desc.free.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.free.low %u\n",statsPtr->desc.free.low);
if (printzero || statsPtr->desc.free.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.free.high %u\n",statsPtr->desc.free.high);
if (printzero || statsPtr->desc.mapBlocksWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.mapBlocksWritten.low %u\n",statsPtr->desc.mapBlocksWritten.low);
if (printzero || statsPtr->desc.mapBlocksWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.mapBlocksWritten.high %u\n",statsPtr->desc.mapBlocksWritten.high);
if (printzero || statsPtr->desc.mapBlockCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.mapBlockCleaned.low %u\n",statsPtr->desc.mapBlockCleaned.low);
if (printzero || statsPtr->desc.mapBlockCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.mapBlockCleaned.high %u\n",statsPtr->desc.mapBlockCleaned.high);
if (printzero || statsPtr->desc.descMoved.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMoved.low %u\n",statsPtr->desc.descMoved.low);
if (printzero || statsPtr->desc.descMoved.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMoved.high %u\n",statsPtr->desc.descMoved.high);
if (printzero || statsPtr->desc.descMapBlockAccess.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMapBlockAccess.low %u\n",statsPtr->desc.descMapBlockAccess.low);
if (printzero || statsPtr->desc.descMapBlockAccess.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMapBlockAccess.high %u\n",statsPtr->desc.descMapBlockAccess.high);
if (printzero || statsPtr->desc.descMapBlockMiss.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMapBlockMiss.low %u\n",statsPtr->desc.descMapBlockMiss.low);
if (printzero || statsPtr->desc.descMapBlockMiss.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.descMapBlockMiss.high %u\n",statsPtr->desc.descMapBlockMiss.high);
if (printzero || statsPtr->desc.residentCount.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.residentCount.low %u\n",statsPtr->desc.residentCount.low);
if (printzero || statsPtr->desc.residentCount.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.residentCount.high %u\n",statsPtr->desc.residentCount.high);
if (printzero || statsPtr->desc.cleaningFetch.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.cleaningFetch.low %u\n",statsPtr->desc.cleaningFetch.low);
if (printzero || statsPtr->desc.cleaningFetch.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.cleaningFetch.high %u\n",statsPtr->desc.cleaningFetch.high);
if (printzero || statsPtr->desc.cleaningFetchMiss.low) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.cleaningFetchMiss.low %u\n",statsPtr->desc.cleaningFetchMiss.low);
if (printzero || statsPtr->desc.cleaningFetchMiss.high) 
	fprintf(outFile,"LfsLogStats.LfsDescStats.desc.cleaningFetchMiss.high %u\n",statsPtr->desc.cleaningFetchMiss.high);
if (printzero || statsPtr->index.get.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.get.low %u\n",statsPtr->index.get.low);
if (printzero || statsPtr->index.get.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.get.high %u\n",statsPtr->index.get.high);
if (printzero || statsPtr->index.set.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.set.low %u\n",statsPtr->index.set.low);
if (printzero || statsPtr->index.set.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.set.high %u\n",statsPtr->index.set.high);
if (printzero || statsPtr->index.getFetchBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getFetchBlock.low %u\n",statsPtr->index.getFetchBlock.low);
if (printzero || statsPtr->index.getFetchBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getFetchBlock.high %u\n",statsPtr->index.getFetchBlock.high);
if (printzero || statsPtr->index.setFetchBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.setFetchBlock.low %u\n",statsPtr->index.setFetchBlock.low);
if (printzero || statsPtr->index.setFetchBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.setFetchBlock.high %u\n",statsPtr->index.setFetchBlock.high);
if (printzero || statsPtr->index.growFetchBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.growFetchBlock.low %u\n",statsPtr->index.growFetchBlock.low);
if (printzero || statsPtr->index.growFetchBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.growFetchBlock.high %u\n",statsPtr->index.growFetchBlock.high);
if (printzero || statsPtr->index.getFetchHit.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getFetchHit.low %u\n",statsPtr->index.getFetchHit.low);
if (printzero || statsPtr->index.getFetchHit.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getFetchHit.high %u\n",statsPtr->index.getFetchHit.high);
if (printzero || statsPtr->index.setFetchHit.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.setFetchHit.low %u\n",statsPtr->index.setFetchHit.low);
if (printzero || statsPtr->index.setFetchHit.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.setFetchHit.high %u\n",statsPtr->index.setFetchHit.high);
if (printzero || statsPtr->index.truncs.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.truncs.low %u\n",statsPtr->index.truncs.low);
if (printzero || statsPtr->index.truncs.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.truncs.high %u\n",statsPtr->index.truncs.high);
if (printzero || statsPtr->index.deleteFetchBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.deleteFetchBlock.low %u\n",statsPtr->index.deleteFetchBlock.low);
if (printzero || statsPtr->index.deleteFetchBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.deleteFetchBlock.high %u\n",statsPtr->index.deleteFetchBlock.high);
if (printzero || statsPtr->index.deleteFetchBlockMiss.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.deleteFetchBlockMiss.low %u\n",statsPtr->index.deleteFetchBlockMiss.low);
if (printzero || statsPtr->index.deleteFetchBlockMiss.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.deleteFetchBlockMiss.high %u\n",statsPtr->index.deleteFetchBlockMiss.high);
if (printzero || statsPtr->index.getCleaningFetchBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getCleaningFetchBlock.low %u\n",statsPtr->index.getCleaningFetchBlock.low);
if (printzero || statsPtr->index.getCleaningFetchBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getCleaningFetchBlock.high %u\n",statsPtr->index.getCleaningFetchBlock.high);
if (printzero || statsPtr->index.getCleaningFetchHit.low) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getCleaningFetchHit.low %u\n",statsPtr->index.getCleaningFetchHit.low);
if (printzero || statsPtr->index.getCleaningFetchHit.high) 
	fprintf(outFile,"LfsLogStats.LfsIndexStats.index.getCleaningFetchHit.high %u\n",statsPtr->index.getCleaningFetchHit.high);
if (printzero || statsPtr->layout.calls.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.calls.low %u\n",statsPtr->layout.calls.low);
if (printzero || statsPtr->layout.calls.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.calls.high %u\n",statsPtr->layout.calls.high);
if (printzero || statsPtr->layout.dirtyFiles.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyFiles.low %u\n",statsPtr->layout.dirtyFiles.low);
if (printzero || statsPtr->layout.dirtyFiles.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyFiles.high %u\n",statsPtr->layout.dirtyFiles.high);
if (printzero || statsPtr->layout.dirtyBlocks.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyBlocks.low %u\n",statsPtr->layout.dirtyBlocks.low);
if (printzero || statsPtr->layout.dirtyBlocks.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyBlocks.high %u\n",statsPtr->layout.dirtyBlocks.high);
if (printzero || statsPtr->layout.dirtyBlocksReturned.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyBlocksReturned.low %u\n",statsPtr->layout.dirtyBlocksReturned.low);
if (printzero || statsPtr->layout.dirtyBlocksReturned.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.dirtyBlocksReturned.high %u\n",statsPtr->layout.dirtyBlocksReturned.high);
if (printzero || statsPtr->layout.filledRegion.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.filledRegion.low %u\n",statsPtr->layout.filledRegion.low);
if (printzero || statsPtr->layout.filledRegion.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.filledRegion.high %u\n",statsPtr->layout.filledRegion.high);
if (printzero || statsPtr->layout.segWrites.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.segWrites.low %u\n",statsPtr->layout.segWrites.low);
if (printzero || statsPtr->layout.segWrites.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.segWrites.high %u\n",statsPtr->layout.segWrites.high);
if (printzero || statsPtr->layout.cacheBlocksWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cacheBlocksWritten.low %u\n",statsPtr->layout.cacheBlocksWritten.low);
if (printzero || statsPtr->layout.cacheBlocksWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cacheBlocksWritten.high %u\n",statsPtr->layout.cacheBlocksWritten.high);
if (printzero || statsPtr->layout.descBlockWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descBlockWritten.low %u\n",statsPtr->layout.descBlockWritten.low);
if (printzero || statsPtr->layout.descBlockWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descBlockWritten.high %u\n",statsPtr->layout.descBlockWritten.high);
if (printzero || statsPtr->layout.descWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descWritten.low %u\n",statsPtr->layout.descWritten.low);
if (printzero || statsPtr->layout.descWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descWritten.high %u\n",statsPtr->layout.descWritten.high);
if (printzero || statsPtr->layout.filesWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.filesWritten.low %u\n",statsPtr->layout.filesWritten.low);
if (printzero || statsPtr->layout.filesWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.filesWritten.high %u\n",statsPtr->layout.filesWritten.high);
if (printzero || statsPtr->layout.cleanings.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanings.low %u\n",statsPtr->layout.cleanings.low);
if (printzero || statsPtr->layout.cleanings.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanings.high %u\n",statsPtr->layout.cleanings.high);
if (printzero || statsPtr->layout.descBlocksCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descBlocksCleaned.low %u\n",statsPtr->layout.descBlocksCleaned.low);
if (printzero || statsPtr->layout.descBlocksCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descBlocksCleaned.high %u\n",statsPtr->layout.descBlocksCleaned.high);
if (printzero || statsPtr->layout.descCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descCleaned.low %u\n",statsPtr->layout.descCleaned.low);
if (printzero || statsPtr->layout.descCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descCleaned.high %u\n",statsPtr->layout.descCleaned.high);
if (printzero || statsPtr->layout.descCopied.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descCopied.low %u\n",statsPtr->layout.descCopied.low);
if (printzero || statsPtr->layout.descCopied.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descCopied.high %u\n",statsPtr->layout.descCopied.high);
if (printzero || statsPtr->layout.fileCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.fileCleaned.low %u\n",statsPtr->layout.fileCleaned.low);
if (printzero || statsPtr->layout.fileCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.fileCleaned.high %u\n",statsPtr->layout.fileCleaned.high);
if (printzero || statsPtr->layout.fileVersionOk.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.fileVersionOk.low %u\n",statsPtr->layout.fileVersionOk.low);
if (printzero || statsPtr->layout.fileVersionOk.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.fileVersionOk.high %u\n",statsPtr->layout.fileVersionOk.high);
if (printzero || statsPtr->layout.blocksCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCleaned.low %u\n",statsPtr->layout.blocksCleaned.low);
if (printzero || statsPtr->layout.blocksCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCleaned.high %u\n",statsPtr->layout.blocksCleaned.high);
if (printzero || statsPtr->layout.blocksCopied.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCopied.low %u\n",statsPtr->layout.blocksCopied.low);
if (printzero || statsPtr->layout.blocksCopied.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCopied.high %u\n",statsPtr->layout.blocksCopied.high);
if (printzero || statsPtr->layout.blocksCopiedHit.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCopiedHit.low %u\n",statsPtr->layout.blocksCopiedHit.low);
if (printzero || statsPtr->layout.blocksCopiedHit.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.blocksCopiedHit.high %u\n",statsPtr->layout.blocksCopiedHit.high);
if (printzero || statsPtr->layout.cleanNoHandle.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanNoHandle.low %u\n",statsPtr->layout.cleanNoHandle.low);
if (printzero || statsPtr->layout.cleanNoHandle.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanNoHandle.high %u\n",statsPtr->layout.cleanNoHandle.high);
if (printzero || statsPtr->layout.cleanLockedHandle.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanLockedHandle.low %u\n",statsPtr->layout.cleanLockedHandle.low);
if (printzero || statsPtr->layout.cleanLockedHandle.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.cleanLockedHandle.high %u\n",statsPtr->layout.cleanLockedHandle.high);
if (printzero || statsPtr->layout.descLayoutBytes.low) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descLayoutBytes.low %u\n",statsPtr->layout.descLayoutBytes.low);
if (printzero || statsPtr->layout.descLayoutBytes.high) 
	fprintf(outFile,"LfsLogStats.LfsFileLayoutStats.layout.descLayoutBytes.high %u\n",statsPtr->layout.descLayoutBytes.high);
if (printzero || statsPtr->segusage.blocksFreed.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksFreed.low %u\n",statsPtr->segusage.blocksFreed.low);
if (printzero || statsPtr->segusage.blocksFreed.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksFreed.high %u\n",statsPtr->segusage.blocksFreed.high);
if (printzero || statsPtr->segusage.bytesFreed.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.bytesFreed.low %u\n",statsPtr->segusage.bytesFreed.low);
if (printzero || statsPtr->segusage.bytesFreed.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.bytesFreed.high %u\n",statsPtr->segusage.bytesFreed.high);
if (printzero || statsPtr->segusage.usageSet.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.usageSet.low %u\n",statsPtr->segusage.usageSet.low);
if (printzero || statsPtr->segusage.usageSet.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.usageSet.high %u\n",statsPtr->segusage.usageSet.high);
if (printzero || statsPtr->segusage.blocksWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksWritten.low %u\n",statsPtr->segusage.blocksWritten.low);
if (printzero || statsPtr->segusage.blocksWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksWritten.high %u\n",statsPtr->segusage.blocksWritten.high);
if (printzero || statsPtr->segusage.blocksCleaned.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksCleaned.low %u\n",statsPtr->segusage.blocksCleaned.low);
if (printzero || statsPtr->segusage.blocksCleaned.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.blocksCleaned.high %u\n",statsPtr->segusage.blocksCleaned.high);
if (printzero || statsPtr->segusage.segUsageBlockAccess.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.segUsageBlockAccess.low %u\n",statsPtr->segusage.segUsageBlockAccess.low);
if (printzero || statsPtr->segusage.segUsageBlockAccess.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.segUsageBlockAccess.high %u\n",statsPtr->segusage.segUsageBlockAccess.high);
if (printzero || statsPtr->segusage.segUsageBlockMiss.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.segUsageBlockMiss.low %u\n",statsPtr->segusage.segUsageBlockMiss.low);
if (printzero || statsPtr->segusage.segUsageBlockMiss.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.segUsageBlockMiss.high %u\n",statsPtr->segusage.segUsageBlockMiss.high);
if (printzero || statsPtr->segusage.residentCount.low) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.residentCount.low %u\n",statsPtr->segusage.residentCount.low);
if (printzero || statsPtr->segusage.residentCount.high) 
	fprintf(outFile,"LfsLogStats.LfsSegUsageStats.segusage.residentCount.high %u\n",statsPtr->segusage.residentCount.high);
if (printzero || statsPtr->backend.startRequests.low) 
	fprintf(outFile,"LfsLogStats.LfsCacheBackendStats.backend.startRequests.low %u\n",statsPtr->backend.startRequests.low);
if (printzero || statsPtr->backend.startRequests.high) 
	fprintf(outFile,"LfsLogStats.LfsCacheBackendStats.backend.startRequests.high %u\n",statsPtr->backend.startRequests.high);
if (printzero || statsPtr->backend.alreadyActive.low) 
	fprintf(outFile,"LfsLogStats.LfsCacheBackendStats.backend.alreadyActive.low %u\n",statsPtr->backend.alreadyActive.low);
if (printzero || statsPtr->backend.alreadyActive.high) 
	fprintf(outFile,"LfsLogStats.LfsCacheBackendStats.backend.alreadyActive.high %u\n",statsPtr->backend.alreadyActive.high);
if (printzero || statsPtr->dirlog.entryAllocNew.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocNew.low %u\n",statsPtr->dirlog.entryAllocNew.low);
if (printzero || statsPtr->dirlog.entryAllocNew.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocNew.high %u\n",statsPtr->dirlog.entryAllocNew.high);
if (printzero || statsPtr->dirlog.entryAllocOld.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocOld.low %u\n",statsPtr->dirlog.entryAllocOld.low);
if (printzero || statsPtr->dirlog.entryAllocOld.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocOld.high %u\n",statsPtr->dirlog.entryAllocOld.high);
if (printzero || statsPtr->dirlog.entryAllocFound.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocFound.low %u\n",statsPtr->dirlog.entryAllocFound.low);
if (printzero || statsPtr->dirlog.entryAllocFound.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocFound.high %u\n",statsPtr->dirlog.entryAllocFound.high);
if (printzero || statsPtr->dirlog.entryAllocWaits.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocWaits.low %u\n",statsPtr->dirlog.entryAllocWaits.low);
if (printzero || statsPtr->dirlog.entryAllocWaits.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.entryAllocWaits.high %u\n",statsPtr->dirlog.entryAllocWaits.high);
if (printzero || statsPtr->dirlog.newLogBlock.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.newLogBlock.low %u\n",statsPtr->dirlog.newLogBlock.low);
if (printzero || statsPtr->dirlog.newLogBlock.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.newLogBlock.high %u\n",statsPtr->dirlog.newLogBlock.high);
if (printzero || statsPtr->dirlog.fastFindFail.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.fastFindFail.low %u\n",statsPtr->dirlog.fastFindFail.low);
if (printzero || statsPtr->dirlog.fastFindFail.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.fastFindFail.high %u\n",statsPtr->dirlog.fastFindFail.high);
if (printzero || statsPtr->dirlog.findEntrySearch.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.findEntrySearch.low %u\n",statsPtr->dirlog.findEntrySearch.low);
if (printzero || statsPtr->dirlog.findEntrySearch.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.findEntrySearch.high %u\n",statsPtr->dirlog.findEntrySearch.high);
if (printzero || statsPtr->dirlog.dataBlockWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.dataBlockWritten.low %u\n",statsPtr->dirlog.dataBlockWritten.low);
if (printzero || statsPtr->dirlog.dataBlockWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.dataBlockWritten.high %u\n",statsPtr->dirlog.dataBlockWritten.high);
if (printzero || statsPtr->dirlog.blockWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.blockWritten.low %u\n",statsPtr->dirlog.blockWritten.low);
if (printzero || statsPtr->dirlog.blockWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.blockWritten.high %u\n",statsPtr->dirlog.blockWritten.high);
if (printzero || statsPtr->dirlog.bytesWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.bytesWritten.low %u\n",statsPtr->dirlog.bytesWritten.low);
if (printzero || statsPtr->dirlog.bytesWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.bytesWritten.high %u\n",statsPtr->dirlog.bytesWritten.high);
if (printzero || statsPtr->dirlog.cleaningBytesWritten.low) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.cleaningBytesWritten.low %u\n",statsPtr->dirlog.cleaningBytesWritten.low);
if (printzero || statsPtr->dirlog.cleaningBytesWritten.high) 
	fprintf(outFile,"LfsLogStats.LfsDirLogStats.dirlog.cleaningBytesWritten.high %u\n",statsPtr->dirlog.cleaningBytesWritten.high);
