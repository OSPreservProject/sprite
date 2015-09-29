/* 
 * lfsrecov.c --
 *
 *	The lfsrecov program - Perform crash recovery on an LFS file system.
 *	This program attempts to roll forward all changes made since the last
 *	checkpoint. It also deals with deleted but still open files present at
 *	shutdown or crash.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user2/mendel/lfs/src/cmds/checkLfs/RCS/checkLfs.c,v 1.1 90/06/01 10:10:18 mendel Exp Locker: mendel $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <hash.h>
#include <libc.h>

#include "lfsrecov.h"
#include "desc.h"
#include "dirlog.h"
#include "fileop.h"
#include "usage.h"

/*
 * Start and end points of the tail of the recovery log.
 */
LogPoint	logStart;	/* Start of recovery log. */
LogPoint	logEnd;		/* End of recovery log. */

Lfs	*lfsPtr;		/* File system being recovered. */

/*
 * Arguments.
 */
int	blockSize = 512;	/* File system block size. */
Boolean	verboseFlag = FALSE;	/* Trace progress of program. */
Boolean	showLog = FALSE;	/* Show contents of log being processed. */
char	*deviceName;		/* Device to use. */
Boolean	writeFlag = FALSE;
int	memAvail = 8*1024;	/* Amount of data to cache size in kilobytes. */
Boolean	statsFlag = FALSE;
char	*statDir = "/sprite/lfsstats"; 
/*
 * Testing options.
 */

Boolean recreateDirEntries = FALSE; /* Recreate any directory entries. */
int	testWait = 0;		    /* Time to wait after reading checkpoint
				     * and before recovery. */
Boolean	testCheck = FALSE;	   /* Test recovery by checkpoint against
				    * checkpoint. */

Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"Recovery a LFS file system.\n Synopsis: \"lfsrecov [switches] deviceName\"\n Command-line switches are:"},
    {OPT_TRUE, "write", (Address) &writeFlag, 
	"Write changes to disk."},
    {OPT_INT, "memAvail", (Address) &memAvail, 
	"Kilobytes of memory available to the recovery program."},
    {OPT_TRUE, "stats", (Address) &statsFlag, 
	"Write stats to disk."},
    {OPT_STRING, "statDir", (Address) &statDir, 
	"Directory to put stats from recovery."},
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"The following options are for debugging and testing:"},
    {OPT_TRUE, "showLog", (Address) &showLog, 
	"Show contents of log being processed."},
    {OPT_TRUE, "verbose", (Address) &verboseFlag, 
	"Output progress messages during execution."},
    {OPT_TRUE, "recreateDirEntries", (Address) &recreateDirEntries, 
	"For testing, recreate any directory from checkpoint."},
    {OPT_INT, "testWait", (Address) &testWait, 
	"For testing, wait after reading checkpoint."},
    {OPT_TRUE, "testCheck", (Address) &testCheck, 
	"For testing, check against checkpoint."},
};


LogPoint testLogEnd;	/* Log end for testing purposes. */
caddr_t  startSbrk;	/* Starting sbrk() value for program. Used to 
			   track program size. */
Stats	stats;

/*
 * Forward routine declartions.
 */

static Boolean WriteOutStats _ARGS_((FILE *fd, Boolean noWork));

static double timediff _ARGS_((struct timeval *ep, struct timeval *sp));

static Boolean ScanRecoveryLog _ARGS_((Lfs *lfsPtr, enum Pass pass));

extern void RollMetaDataForward _ARGS_((Lfs *lfsPtr));


void CompareCheckpoints _ARGS_((Lfs *lfsPtr, Lfs *newLfsPtr));

/*
 * System routines not defined in header files. 
 */

extern caddr_t sbrk _ARGS_((int numBytes));
extern int  fsync _ARGS_((int fd));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of lfsrecov - parse arguments and do the work.
 *
 * Results:
 *	0 if file system was successfully recovered.
 *	1 if problem found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
main(argc,argv)
    int	argc;
    char *argv[];
{
    ReturnStatus	status;
    Boolean 		needRecovery;
    Lfs		*newLfsPtr;

    /*
     * Parse the arguments and open the device containing the file system.
     */

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(1);
    } else {
	deviceName = argv[1];
    }
    if (testCheck) {
	startSbrk = sbrk(0);
    }
    gettimeofday(&stats.startTimeOfDay, (struct timezone *) NULL);

    lfsPtr = LfsLoadFileSystem(argv[0], deviceName, blockSize, 
			LFS_SUPER_BLOCK_OFFSET, writeFlag ? O_RDWR : O_RDONLY);
    if (lfsPtr == (Lfs *) NULL) {
	exit(1);
    }
    if (testWait > 0) {
	sleep(testWait);
    }
    if (testCheck) {
	newLfsPtr = LfsLoadFileSystem(argv[0], deviceName, blockSize,
				LFS_SUPER_BLOCK_OFFSET, O_RDONLY);
	if (newLfsPtr  == (Lfs *) NULL) {
	    fprintf(stderr, "Can't load file system to check.\n");
	    testCheck = FALSE;
	} else { 
	    testLogEnd = newLfsPtr->logEnd;
	}
    } else {
	newLfsPtr  = (Lfs *) NULL;
    }
    stats.crashTime = lfsPtr->checkPointHdrPtr->detachSeconds;
    stats.timestamp = lfsPtr->checkPointHdrPtr->timestamp;
    stats.loadBlockReads = lfsPtr->pstats.blocksReadDisk;

    LfsDiskCache(lfsPtr, memAvail*1024);
    if (verboseFlag) {
	printf("Starting Pass 1 on %s\n", lfsPtr->name);
    }
    gettimeofday(&stats.startPass1TimeOfDay, (struct timezone *) NULL);
    needRecovery = ScanRecoveryLog(lfsPtr, PASS1);

    stats.pass1BlockReads = lfsPtr->pstats.blocksReadDisk - 
				stats.loadBlockReads;
    if (!needRecovery) {
	if (verboseFlag) {
	    printf("No recovery needed for %s\n", lfsPtr->name);
	}
	goto dumpStats;
    }

    RollMetaDataForward(lfsPtr);

    if (verboseFlag) {
	printf("Starting Pass 2 on %s\n", lfsPtr->name);
    }
    gettimeofday(&stats.startPass2TimeOfDay, (struct timezone *) NULL);
    ScanRecoveryLog(lfsPtr, PASS2);

    stats.pass2BlockReads = lfsPtr->pstats.blocksReadDisk - 
			    stats.loadBlockReads - stats.pass1BlockReads;
    UpdateLost_Found(lfsPtr);

    if (testCheck) {
	CompareCheckpoints(lfsPtr, newLfsPtr);
    }
    gettimeofday(&stats.startWriteTimeOfDay, (struct timezone *) NULL);
    if (writeFlag) {
	if (verboseFlag) {
	    printf("Checkpointing changes to %s\n", lfsPtr->name);
	}
	status = LfsCheckPointFileSystem(lfsPtr, 0);
	if (status != SUCCESS) {
	    exit(1);
	}
    }
 dumpStats:
    gettimeofday(&stats.endTimeOfDay, (struct timezone *) NULL);
    if (statsFlag) {
	char statsFileName[MAXPATHLEN];
	FILE	*fd;
	(void) sprintf(statsFileName, "%d/%s", statDir, lfsPtr->name);
	fd = fopen(statsFileName, "a");
	if (fd == (FILE *) NULL) {
	    perror(statsFileName);
	    fprintf(stderr,"Can't open stats file\n");
	} else {
	    if (!WriteOutStats(fd, needRecovery)) {
		fprintf(stderr,"Can't write stats file\n");
	    }
	    if (fsync(fileno(fd)) != 0) {
		perror("fsync");
		fprintf(stderr,"Can't fsync stats file\n");
	    }
	    fclose(fd);
	}
    }
    exit(0);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ScanRecoveryLog --
 *
 *	Perform recovery by scanning thru the tail of the log.
 *	During PASS1 The segments and inodes written since the last 
 *      checkpoint checkpoint are recovered.
 *
 * Results:
 *	TRUE if recovery is need. FALSE if checkpoint is up to date.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
ScanRecoveryLog(lfsPtr, pass)
    Lfs	*lfsPtr;	/*  The file system. */
    enum Pass pass;	/* The pass number. */
{
    int startAddr, nextSeg = -1;
    int	startingTimeStamp; /* Starting timestamp of checkpoint. */
    int	endingTimeStamp;   /* Maximum ending time. */
    char *summaryLimitPtr, *summaryPtr;
    LfsSegSummary *segSummaryPtr;
    LfsSegSummaryHdr *segSummaryHdrPtr;
    LogPoint	curLoc;
    LfsSeg	*segPtr;
    Boolean done;
    int		 addr;


    endingTimeStamp = startingTimeStamp = lfsPtr->checkPointHdrPtr->timestamp;

    /*
     * Move forward thru the recovery log. 
     */
    logStart = lfsPtr->logEnd;
    if (verboseFlag && (pass == PASS1)) {
	printf("Recovery log starts at <%d,%d> at %d\n", 
			logStart.segNo, logStart.blockOffset,
			startingTimeStamp);
    }
    for(curLoc = logStart, done = FALSE; !done; ) { 
        segPtr = LfsSegInit(lfsPtr, curLoc.segNo);
        startAddr = LfsSegStartAddr(segPtr) + LfsSegSizeInBlocks(lfsPtr);
	do { 
	    /*
	     * During testing, check to see if we hit the end checkpoint.
	     */
	    if (testCheck && 
		(testLogEnd.blockOffset == curLoc.blockOffset) &&
		(testLogEnd.segNo == curLoc.segNo)) {
		done = TRUE;
		break;
	    }
	    if (pass == PASS1) { 
		stats.segSummaryFetch++;
	    }
	    /*
	     * Spin thru the summary blocks of this segment. We know we
	     * have reached end of log if:
	     * 1) The summary region is zero (ie size of zero). 
	     * 2) The summary region is not valid. 
	     * 3) The timestamp went backward.
	     * All this means that the write that was to generated the
	     * data didn't complete.
	     */
	    segSummaryPtr = (LfsSegSummary *) 
			LfsSegFetchBlock(segPtr, curLoc.blockOffset, blockSize);
	    if (segSummaryPtr->size == 0) {
		LfsSegReleaseBlock(segPtr, (char *) segSummaryPtr);
		done = TRUE;
		break;
	    }
	    if (segSummaryPtr->magic != LFS_SEG_SUMMARY_MAGIC) {
		fprintf(stderr,"%s: Bad magic number 0x%x for summary region of segment %d\n", deviceName, segSummaryPtr->magic, curLoc.segNo);
		done = TRUE;
		break;
	    }
	    if (segSummaryPtr->timestamp < startingTimeStamp) {
		done = TRUE;
		break;
	    }
	    if (verboseFlag) {
		printf("Scaning summary @ <%d,%d> time %d\n", curLoc.segNo, 
				 curLoc.blockOffset, segSummaryPtr->timestamp);
	    }
	    endingTimeStamp = segSummaryPtr->timestamp;
	    if (pass == PASS1) { 
		/*
		 * Record the segment as part of the recovery log if we haven't
		 * already.
		 */
		RecordSegInLog(curLoc.segNo, curLoc.blockOffset);
		addr = startAddr-curLoc.blockOffset;
		if (showLog) { 
		    printf("Addr %d Summary Time %d PrevSeg %d NextSeg %d bytes %d NextSum %d\n",
			addr,
			segSummaryPtr->timestamp, segSummaryPtr->prevSeg, 
			segSummaryPtr->nextSeg,
			segSummaryPtr->size, segSummaryPtr->nextSummaryBlock);
		}
	    }
	    curLoc.blockOffset++;
	    /*
	     * Scan thru this summary region.
	     */
	    summaryLimitPtr = (char *) segSummaryPtr + segSummaryPtr->size;
	    summaryPtr = (char *) (segSummaryPtr + 1);
	    while (summaryPtr < summaryLimitPtr) {
	       addr = startAddr-curLoc.blockOffset;
	       segSummaryHdrPtr = (LfsSegSummaryHdr *) summaryPtr;
	       if (segSummaryHdrPtr->lengthInBytes == 0) {
		    break;
	       }
	       switch (segSummaryHdrPtr->moduleType) { 
		   case LFS_SEG_USAGE_MOD:
		      RecovSegUsageSummary(lfsPtr, pass, segPtr, addr,
				   curLoc.blockOffset,  segSummaryHdrPtr);
		       break;
		   case LFS_DESC_MAP_MOD:
		      RecovDescMapSummary(lfsPtr, pass, segPtr, addr,
				   curLoc.blockOffset,  segSummaryHdrPtr);
		      break;
		   case LFS_FILE_LAYOUT_MOD:
		      RecovFileLayoutSummary(lfsPtr, pass, segPtr, addr,
				   curLoc.blockOffset,  segSummaryHdrPtr);
		      break;
		   default: 
			fprintf(stderr,"%s:CheckSummary: Unknown module type %d at %d, Size %d Blocks %d\n",
				deviceName, segSummaryHdrPtr->moduleType, addr,
				segSummaryHdrPtr->lengthInBytes, 
				segSummaryHdrPtr->numDataBlocks);
			break;
	       }
	       summaryPtr += segSummaryHdrPtr->lengthInBytes;
	       curLoc.blockOffset += (segSummaryHdrPtr->numDataBlocks-1);
	   }
	   /*
	    * On to the next summary block.  
	    */
	   if (segSummaryPtr->nextSummaryBlock == -1) { 
	       curLoc.blockOffset = 0;
	   } else {
	       curLoc.blockOffset = segSummaryPtr->nextSummaryBlock-1;
	   }
	   nextSeg = segSummaryPtr->nextSeg;
	    LfsSegReleaseBlock(segPtr, (char *) segSummaryPtr);
	} while( curLoc.blockOffset != 0);
	/*
	 * On to the next segment.
	 */
	LfsSegRelease(segPtr);
	if (!done) { 
	    curLoc.segNo = nextSeg;
	    curLoc.blockOffset = 0;
	}
    }
    logEnd = curLoc;
    if (verboseFlag && (pass == PASS1)) {
	printf("Recovery log end at <%d,%d> at %d\n", 
			logEnd.segNo, logEnd.blockOffset,
			endingTimeStamp);
    }
    return ((logEnd.segNo != logStart.segNo) || 
	    (logEnd.blockOffset != logStart.blockOffset));
}


/*
 *----------------------------------------------------------------------
 *
 * RollMetaDataForward --
 *
 *	Roll forward the changes to the metadata structure caused
 *	by the descriptors going out to the recovery log.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
RollMetaDataForward(lfsPtr)
    Lfs	*lfsPtr;
{
    ClientData clientData;
    int		fileNumber;
    int		address;
    LfsFileDescriptor	*descPtr;

    RollSegUsageForward(lfsPtr);

    /*
     * Next, scan thru all the descriptors in the recovery log. Update
     * the descriptor map and segment usage table.
     */
     clientData = (ClientData) NIL;
     while (ScanNewDesc(&clientData, &fileNumber, &address, &descPtr)) {
	 if (descPtr == (LfsFileDescriptor *) NIL) {
	     address = FSDM_NIL_INDEX;
	     stats.descDelete++;
	 } else {
	     stats.descMove++;
	 }
	 RecoveryFile(fileNumber, address, descPtr);
     }
     ScanNewDescEnd(&clientData);

}


/*
 *----------------------------------------------------------------------
 *
 * WriteOutStats --
 *
 *	Write out program stats.
 *
 * Results:
 *	TRUE if write worked, FALSE if an error occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
WriteOutStats(fd, noWork)
    FILE *fd;	/* File to write stats to. */
    Boolean noWork;
{
    time_t t = time((time_t *)NULL);

    fprintf(fd, "date: %s\n", ctime(&t));
    fprintf(fd, "name: %s\n", lfsPtr->name);
    fprintf(fd, "checkpointInterval: %d\n", 
			lfsPtr->superBlock.hdr.checkpointInterval);
    fprintf(fd, "crashTime: %d\n", stats.crashTime);
    fprintf(fd, "timestamp: %d\n", stats.timestamp);
    fprintf(fd, "noWork: %d\n", noWork);
    fprintf(fd, "totalTime: %f\n", 
		timediff(&stats.endTimeOfDay, &stats.startTimeOfDay)); 
    fprintf(fd, "loadTime: %f\n", 
		timediff(&stats.startPass1TimeOfDay, &stats.startTimeOfDay)); 
    fprintf(fd, "pass1Time: %f\n", 
	timediff(&stats.startPass2TimeOfDay, &stats.startPass1TimeOfDay)); 
    if (!noWork) {
	fprintf(fd, "pass2Time: %f\n", 
	    timediff(&stats.startWriteTimeOfDay, &stats.startPass2TimeOfDay)); 
	fprintf(fd, "writeTime: %f\n", 
	    timediff(&stats.endTimeOfDay, &stats.startWriteTimeOfDay)); 
    }
#define	PSF(FIELD, NAME)  fprintf(fd, "%s: %d\n", NAME, stats.FIELD);
    PSF(numLogSegments, "numLogSegments");
    PSF(segSummaryFetch, "segSummaryFetch");
    PSF(descMapBlocks, "descMapBlocks");
    PSF(descBlocks, "descBlocks");
    PSF(numDesc, "numDesc");
    PSF(numFileBlocks, "numFileBlocks");
    PSF(numDirLogBlocks, "numDirLogBlocks");
    PSF(numDirLogEntries, "numDirLogEntries");
    PSF(numDirLogDelete, "numDirLogDelete");
    PSF(numDirLogDeleteOpen, "numDirLogDeleteOpen");
    PSF(numDirLogUnlink, "numDirLogUnlink");
    PSF(numDirLogCreate, "numDirLogCreate");
    PSF(numDirLogWithoutEnd, "numDirLogWithoutEnd");
    PSF(dirLogBothForward, "dirLogBothForward");
    PSF(dirLogDirBlockBackward, "dirLogDirBlockBackward");
    PSF(dirLogBothBackwardCreate, "dirLogBothBackwardCreate");
    PSF(dirLogBothBackward, "dirLogBothBackward");
    PSF(descDelete, "descDelete");
    PSF(descMove, "descMove");
    PSF(filesToLostFound, "filesToLostFound");
    PSF(dirEntryAdded, "dirEntryAddeds");
    PSF(dirEntryRemoved, "dirEntryRemoved");
    PSF(loadBlockReads, "loadBlockReads");
    PSF(pass1BlockReads, "pass1BlockReads");
    PSF(pass2BlockReads, "pass2BlockReads");
#undef PSF
    fprintf(fd, "LfsblocksReadDisk: %d\n", lfsPtr->pstats.blocksReadDisk);
    fprintf(fd, "LfsblocksWrittenDisk: %d\n", lfsPtr->pstats.blocksWrittenDisk);
    fprintf(fd, "LfswriteSummaryBlock: %d\n", lfsPtr->pstats.writeSummaryBlock);
    fprintf(fd, "LfssegUsageBlockWrite: %d\n", lfsPtr->pstats.segUsageBlockWrite);
    fprintf(fd, "LfsdescMapBlockWrite: %d\n", lfsPtr->pstats.descMapBlockWrite);
    fprintf(fd, "LfsfileWritten: %d\n", lfsPtr->pstats.fileWritten);
    fprintf(fd, "LfsfileBlockWritten: %d\n", 
	    lfsPtr->pstats.fileBlockWritten);
    fprintf(fd, "LfsdescWritten: %d\n", 
	    lfsPtr->pstats.descWritten);
    return !ferror(fd);
}

/*
 *----------------------------------------------------------------------
 *
 * timediff --
 *
 *	Compute the difference in seconds between two timevals. 
 *
 * Results:
 *	Difference in seconds
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


static double
timediff (ep,sp)
    struct timeval *ep;		/* End value. */
    struct timeval *sp;		/* Start value. */
{
    double seconds;

    seconds = (ep->tv_sec - sp->tv_sec)
			   + (ep->tv_usec - sp->tv_usec)/1000000.0;

    return seconds;
}

/*
 *----------------------------------------------------------------------
 *
 * CompareCheckpoints --
 *
 *	Compare the recovery generated checkpoint against the system
 * 	generated one during testing
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
CompareCheckpoints(lfsPtr, newLfsPtr)
    Lfs	*lfsPtr;	/* Recovery generated one. */
    Lfs *newLfsPtr;	/* System generated one. */
{
    LfsDescMapCheckPoint *dCpPtr, *dNewCpPtr;
    LfsSegUsageCheckPoint *sCpPtr, *sNewCpPtr;
    int	segNum, fileNumber, numSegs, blockNum;
    int *adjustmentArray;
    /* 
     * Compare descriptor map data structure. 
     */
    dCpPtr = &lfsPtr->descMap.checkPoint;
    dNewCpPtr = &newLfsPtr->descMap.checkPoint;

    if (dCpPtr->numAllocDesc != dNewCpPtr->numAllocDesc) {
	fprintf(stderr,"CompareCheckpoints: descMap numAllocDesc my %d ckp %d\n",
		dCpPtr->numAllocDesc, dNewCpPtr->numAllocDesc);
    }
    for (fileNumber = 0; fileNumber < lfsPtr->superBlock.descMap.maxDesc; 
	 fileNumber++) {
	LfsDescMapEntry	*entryPtr, *newEntryPtr;
	entryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
	newEntryPtr = LfsGetDescMapEntry(newLfsPtr, fileNumber);
	if ((entryPtr->flags == newEntryPtr->flags) &&
	    !(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	   continue;
	}
#define	C(FIELD, NAME) if(entryPtr->FIELD != newEntryPtr->FIELD) { fprintf(stderr, "CompareCheckpoints: File %d descMap %s my %d ckp %d\n", fileNumber, NAME, entryPtr->FIELD, newEntryPtr->FIELD); }
	C(blockAddress, "blockAddress");
	C(flags, "flags"); C(truncVersion, "truncVersion");
	C(accessTime, "accessTime");
#undef C
    }

    /*
     * Compare seg usage map.
     */
    sCpPtr = &lfsPtr->usageArray.checkPoint;
    sNewCpPtr = &newLfsPtr->usageArray.checkPoint;

#define	C(FIELD, NAME) if(sCpPtr->FIELD != sNewCpPtr->FIELD) { fprintf(stderr, "CompareCheckpoints: segUsage %s my %d ckp %d\n", NAME, sCpPtr->FIELD, sNewCpPtr->FIELD); }
    C(freeBlocks, "freeBlocks"); C(numClean, "numClean");
    C(numDirty, "numDirty"); C(dirtyActiveBytes, "dirtyActiveBytes");
    C(currentSegment, "currentSegment"); 
    C(currentBlockOffset, "currentBlockOffset");
    C(curSegActiveBytes, "curSegActiveBytes"); 
    C(previousSegment, "previousSegment");
    C(cleanSegList, "cleanSegList"); 
#undef C

    numSegs = lfsPtr->superBlock.usageArray.numberSegments;
    adjustmentArray = (int *) alloca(numSegs * sizeof(adjustmentArray[0]));
    bzero((char *)adjustmentArray, numSegs * sizeof(adjustmentArray[0]));
    for (blockNum = 0; blockNum < lfsPtr->superBlock.usageArray.stableMem.maxNumBlocks; blockNum++) {
	int	oldBlock, newBlock;
	oldBlock = LfsGetUsageArrayBlockIndex(lfsPtr, blockNum);
	newBlock = LfsGetUsageArrayBlockIndex(newLfsPtr, blockNum);
	adjustmentArray[LfsDiskAddrToSegmentNum(lfsPtr, oldBlock)] -= 
			lfsPtr->superBlock.usageArray.stableMem.blockSize;
	adjustmentArray[LfsDiskAddrToSegmentNum(lfsPtr, newBlock)] += 
			lfsPtr->superBlock.usageArray.stableMem.blockSize;

    }
    for (segNum = 0; segNum < numSegs; segNum++) {
	LfsSegUsageEntry	*entryPtr, *newEntryPtr;
	entryPtr = LfsGetUsageArrayEntry(lfsPtr, segNum);
	newEntryPtr = LfsGetUsageArrayEntry(newLfsPtr, segNum);
	if ((entryPtr->flags == newEntryPtr->flags) &&
	    (entryPtr->flags & LFS_SEG_USAGE_CLEAN)) {
	   continue;
	}
#define	C(ADJ, FIELD, NAME) if(ADJ + entryPtr->FIELD != newEntryPtr->FIELD) { fprintf(stderr, "CompareCheckpoints: Seg %d entry %s my %d ckp %d\n", segNum, NAME, ADJ + entryPtr->FIELD, newEntryPtr->FIELD); }
	C(adjustmentArray[segNum], activeBytes, "activeBytes");
	C(0, timeOfLastWrite, "timeOfLastWrite"); C(0, flags, "flags");
#undef C
    }
    printf("Memend = %d (growth = %d)\n", (int)sbrk(0), sbrk(0) - startSbrk);

}
