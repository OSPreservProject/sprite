/* 
 * fscmd.c --
 *
 *	Command-line interface to the Fs_Command system call.
 *	This is used to set various filesystem parameters, ie.
 *	min/max cache size, cache write-back, debug printing.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fscmd/RCS/fscmd.c,v 1.20 90/02/16 11:26:37 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "status.h"
#include "option.h"
#include "fs.h"
#include "fsCmd.h"
#include "stdio.h"
/*
 * Command line options.
 */

int minBlocks = -1;
int maxBlocks = -1;
int	cacheWriteBack = -1;
Boolean	flushCache = FALSE;
int	fsTracing = -1;
int	cacheDebug = -1;
int	pdevDebug = -1;
int	migDebug = -1;
int	nameCaching = -1;
int	clientCaching = -1;
int	contextSwitches = -1;
int	noStickySegs = -1;
int	numCleanerProcs = -1;
int	readAhead = -1;
int	readAheadTracing = -1;
int	writeThrough = -1;
int	writeBackOnClose = -1;
int	delayTmpFiles = -1;
int	tmpDirNum = -1;
int	writeBackASAP = -1;
int	wbOnLastDirtyBlock = -1;
int	writeBackInterval = -1;
int	blockSkew = -1;
int	largeFileMode = -1;
int	maxFilePortion = -1;
int	deleteHistograms = -1;
Boolean zeroStats = FALSE;
char    *updateSummary = NULL;

Option optionArray[] = {
    {OPT_INT, "B", (Address) &blockSkew, "\tSet block gap for allocation."},
    {OPT_INT, "C", (Address) &clientCaching, "\tSet client caching flag."},
    {OPT_INT, "D", (Address) &cacheDebug, "\tSet cache debug flag."},
    {OPT_INT, "H", (Address)&deleteHistograms, "\tSet delete histograms flag."},
    {OPT_INT, "L", (Address)&maxFilePortion, "\tSet large file portion."},
    {OPT_INT, "M", (Address) &maxBlocks, "\tMaximum number of blocks allowed in file system cache."},
    {OPT_INT, "N", (Address) &nameCaching, "\tSet name caching flag."},
    {OPT_TRUE, "O", (Address)&zeroStats, "\tZero fsStat counters."},
    {OPT_INT, "R", (Address) &readAheadTracing, "\tSet read ahead tracing flag."},
    {OPT_INT, "S", (Address) &writeBackInterval, "\tSet cache write back interval."},
    {OPT_INT, "W", (Address) &writeBackOnClose, "\tSet write-back-on-close flag"},
    {OPT_INT, "X", (Address) &tmpDirNum, "\tSet tmp directory number."},
    {OPT_INT, "Z", (Address) &contextSwitches, "\tDo context switches."},
    {OPT_INT, "b", (Address) &numCleanerProcs, "\tSet the maximum number of block cleaner processes."},
    {OPT_TRUE, "f", (Address) &flushCache, "\tFlush and invalidate cache."},
    {OPT_INT, "g", (Address) &migDebug, "\tSet migration debug flag."},
    {OPT_INT, "l", (Address)&largeFileMode, "\tSet large file mode for cache."},
    {OPT_INT, "m", (Address) &minBlocks, "\tMinimum number of blocks allowed in file system cache."},
    {OPT_INT, "p", (Address) &pdevDebug, "\tSet pseudo-device debug flag."},
    {OPT_INT, "r", (Address) &readAhead, "\tSet number of blocks of read ahead."},
    {OPT_INT, "s", (Address) &cacheWriteBack, "\tSet cache write back flag."},
    {OPT_INT, "t", (Address) &fsTracing, "\tSet fs tracing value."},
    {OPT_STRING, "u", (Address)&updateSummary, "\tUpdate summary info from disk."},
    {OPT_INT, "v", (Address) &noStickySegs, "\tSet no sticky segments flag."},
    {OPT_INT, "w", (Address) &writeThrough, "\tSet write through flag."},
    {OPT_INT, "x", (Address) &delayTmpFiles, "\tSet delay tmp files flag."},
    {OPT_INT, "y", (Address) &writeBackASAP, "\tSet write back ASAP flag."},
    {OPT_INT, "z", (Address) &wbOnLastDirtyBlock, "\tSet write back on last dirty block flag."},
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Collects arguments and branch to the code for the fs command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Fs_Command...
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;	/* status of system calls */

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    if (minBlocks != -1) {
	(void) Fs_Command(FS_RAISE_MIN_CACHE_SIZE, sizeof(int), &minBlocks);
    }
    if (maxBlocks != -1) {
	(void) Fs_Command(FS_LOWER_MAX_CACHE_SIZE, sizeof(int), &maxBlocks);
    }
    if (flushCache) {
	/*
	 * Flush the filesystem cache.
	 */
	int numLockedBlocks;
	status = Fs_Command(FS_EMPTY_CACHE, sizeof(int),
					    (Address) &numLockedBlocks);
	if (numLockedBlocks > 0) {
	    fprintf(stderr, "%d locked blocks left\n",
				      numLockedBlocks);
	}
    }
    if (zeroStats) {
	/*
	 * Zero out the file system statistics.
	 */
	(void) Fs_Command(FS_ZERO_STATS, 0, (Address) NULL);
    }
    if (updateSummary != NULL) {
	/*
	 * Reread summary information from disk and update in-memory copy.
	 */
	status = Fs_Command(FS_REREAD_SUMMARY_INFO, strlen(updateSummary)+1,
			updateSummary);
    }
    if (writeBackInterval != -1) {
	int	newInterval;

	newInterval = writeBackInterval;
	Fs_Command(FS_SET_WRITE_BACK_INTERVAL, sizeof(int),
		   &writeBackInterval);
	printf("Cache write back interval was %d, now %d\n",
		writeBackInterval, newInterval);
    }
    /*
     * Set various file system flags.  The Fs_Command system call returns
     * the old value of the flag in place of the value passed in.
     */
    if (cacheWriteBack != -1) {
	/*
	 * Set the cache write-back flag.
	 */
	status = SetFlag(FS_DISABLE_FLUSH, cacheWriteBack, "Cache write-back");
    }
    if (fsTracing != -1) {
	SetFlag(FS_SET_TRACING, fsTracing, "Filesystem tracing");
    }
    if (cacheDebug != -1) {
	status = SetFlag(FS_SET_CACHE_DEBUG, cacheDebug, "Cache debug prints");
    }
    if (migDebug != -1) {
	status = SetFlag(FS_SET_MIG_DEBUG, migDebug, "Migration debug prints");
    }
    if (pdevDebug != -1) {
	status = SetFlag(FS_SET_PDEV_DEBUG, pdevDebug,
			 "pseudo-device debug prints");
    }
    if (nameCaching != -1) {
	status = SetFlag(FS_SET_NAME_CACHING, nameCaching, "Name caching");
    }
    if (clientCaching != -1) {
	status = SetFlag(FS_SET_CLIENT_CACHING, clientCaching, "Client caching");
    }
    if (noStickySegs != -1) {
	status = SetFlag(FS_SET_NO_STICKY_SEGS, noStickySegs, 
			 "No sticky segments");
    }

    if (contextSwitches != -1) {
	status = Fs_Command(FS_TEST_CS, 4, (Address) &contextSwitches);
    }
    if (numCleanerProcs != -1) {
	status = SetInt(FS_SET_CLEANER_PROCS, numCleanerProcs,
			"Num block cleaner procs");
    }
    if (readAhead != -1){
	status = SetInt(FS_SET_READ_AHEAD, readAhead,
			"Num blocks of read ahead");
    }
    if (readAheadTracing != -1) {
	status = SetFlag(FS_SET_RA_TRACING, readAheadTracing, 
			"Read ahead tracing");
    }
    if (writeThrough != -1) {
	status = SetFlag(FS_SET_WRITE_THROUGH, writeThrough, "Write through");
    }
    if (writeBackOnClose != -1) {
	status = SetFlag(FS_SET_WRITE_BACK_ON_CLOSE, writeBackOnClose,
		"Write-back-on-close");
    }
    if (delayTmpFiles != -1) {
	status = SetFlag(FS_SET_DELAY_TMP_FILES, delayTmpFiles,
			 "Delay tmp files");
    }
    if (tmpDirNum != -1) {
	status = Fs_Command(FS_SET_TMP_DIR_NUM, sizeof(int), &tmpDirNum);
    }
    if (writeBackASAP != -1) {
	status = SetFlag(FS_SET_WRITE_BACK_ASAP, writeBackASAP,
			 "Write back ASAP");
    }
    if (wbOnLastDirtyBlock != -1) {
	status = SetFlag(FS_SET_WB_ON_LAST_DIRTY_BLOCK, wbOnLastDirtyBlock,
			 "Write back on last dirty block");
    }
    if (blockSkew != -1) {
	status = SetInt(FS_SET_BLOCK_SKEW, blockSkew,
			 "Block gap for allocation");
    }
    if (largeFileMode != -1) {
	status = SetFlag(FS_SET_LARGE_FILE_MODE, largeFileMode,
			 "Large file mode for cache");
    }
    if (maxFilePortion != -1) {
	status = SetFlag(FS_SET_MAX_FILE_PORTION, maxFilePortion,
			 "Maximum large file portion for cache");
    }
    if (deleteHistograms != -1) {
	status = SetFlag(FS_SET_DELETE_HISTOGRAMS, deleteHistograms,
			 "Keep histograms of size/age distributions on delete");
    }
    exit(status);
}

ReturnStatus
SetInt(command, value, comment)
    int command;		/* Argument to Fs_Command */
    int value;			/* Value for int */
    char *comment;		/* For Io_Print */
{
    register int newValue;
    register ReturnStatus status;

    newValue = value;
    status = Fs_Command(command, sizeof(int), (Address) &value);
    if (status != SUCCESS) {
	printf("%s failed <%x>\n", comment, status);
    } else {
	printf("%s %d, was %d\n", comment, newValue, value);
    }
    return(status);
}

ReturnStatus
SetFlag(command, value, comment)
    int command;		/* Argument to Fs_Command */
    int value;			/* Value for flag */
    char *comment;		/* For Io_Print */
{
    register int newValue;
    register ReturnStatus status;

    newValue = value;
    status = Fs_Command(command, sizeof(int), (Address) &value);
    if (status != SUCCESS) {
	printf("%s failed <%x>\n", comment, status);
    } else {
	printf("%s %s, was %s\n", comment,
		     newValue ? "on" : "off",
		     value ? "on" : "off");
    }
    return(status);
}
