/* 
 * lfscheck.c --
 *
 *	The lfscheck program - Check an LFS file system to make sure it
 *	is consistent.
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

#ifdef __STDC__
#define _HAS_PROTOTYPES
#endif

#include <cfuncproto.h>
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#endif
#include <sprite.h>
#include <stdio.h>
#include <sys/types.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <fs.h>
#include <kernel/fs.h>
#include <kernel/dev.h>
#include <kernel/fsdm.h>
#include <kernel/fslcl.h>
#include <kernel/devDiskLabel.h>

#include <kernel/lfsDesc.h>
#include <kernel/lfsDescMap.h>
#include <kernel/lfsFileLayout.h>
#include <kernel/lfsDirOpLog.h>
#include <kernel/lfsSegLayout.h>
#include <kernel/lfsStableMem.h>
#include <kernel/lfsSuperBlock.h>
#include <kernel/lfsUsageArray.h>
#include <kernel/lfsStats.h>

#include "fscheck.h"


/*
 * The super block of the file system.
 */
LfsSuperBlock	*superBlockPtr;

/*
 * The descriptor map and segment usage array of file system.
 */
ClientData descMapDataPtr;	    /* Descriptor map of file system. */
LfsDescMapCheckPoint *descMapCheckPointPtr; /* Most current descriptor map 
					     * check point. */
FdInfo		*descInfoArray;

ClientData   usageArrayDataPtr;	/* Data of usage map. */


typedef struct Seg {
    int	  segNo;	/* Segment number. */
    int	  segSizeInBlocks;	/* Segment size in blocks. */
    int   diskFd;		/* Open file of disk. */
} Seg;

/*
 * Info kept for each block in the file system while running in full check
 * mode. We use bit fields to keep the size down.
 */

typedef struct BlockInfo {
    int	    type :8;	/* Type of block. See defines below. */
    int     found:1;	/* TRUE if found in summary region. */
    int		 :3;
    int     blockNum:20; /* Block number of block's owner. */
    int     fileNum;	/* File number of block's owner. */
} BlockInfo;
/*
 * Block info types. 
 *	UNUSED	- Not allocated.
 *	FILE	- Owned by a file. FileNum and blockNum specify the file
 *		  and block.
 *	DESC_MAP - A block of the descriptor map. BlockNum specifies the block.
 *	USAGE_ARRAY - A block of the segment usage array.
 *	SUMMARY	    - Block is a summary block.
 *	CHECKPOINT  - Block is a checkpoint block.
 *	DESC	    - Block contains file descriptors.
 *	DIRLOG	    - Directory log block
 */
#define	UNUSED	0
#define	FILE	1
#define	DESC_MAP 2
#define USAGE_ARRAY 3
#define	SUMMARY	    4
#define CHECKPOINT  5
#define	DESC	   6
#define	DIRLOG	    7
#define	UNKNOWN   8

char	*blockTypeNames[UNKNOWN+1] = { 
    "Unused", "File", "DescMap" , "UsageArray", "Summary", "Checkpoint",
    "Descriptor", "DirLog", "Unknown" 
};

BlockInfo	*blockInfoArray;  /* BlockInfo for each block in fs. */
int		*usageBitMap;	  /* Bit map of block usage during quick check
				   * of file system. */
int		*foundBitMap;	  /* Bit map of block found during summary
				   * check. */
int		*descBlockBitMap;	  /* Bit map of desc blocks. */
int		numBlocks;	  /* Number of blocks in fs. */

int	*activeBytesArray;    /* Computed active bytes for each segment. */

int	minorErrors;
int	majorErrors;

/*
 * Arguments.
 */
int	blockSize = 512;	/* File system block size. */
int	superBlockOffset = 64;	/* Offset of super block. */
Boolean dumpFlag = FALSE;	/* Dump version description of file system. */
Boolean showDirLog = FALSE;	/* Show the directory log. */
Boolean	verboseFlag = FALSE;	/* Trace progress of program. */
char	*deviceName;		/* Device to use. */
Boolean	full = FALSE;
Boolean oldcp = FALSE;
Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"Check a LFS file system and report problems.\n Synopsis: \"checkLfs [switches] deviceName\"\n Command-line switches are:"},
    {OPT_INT, "blockSize", (Address) &blockSize, 
	"Block size of file system."},
    {OPT_INT, "superBlockOffset", (Address) &superBlockOffset, 
	"Offset into device of the superBlock."},
    {OPT_TRUE, "dump", (Address) &dumpFlag, 
	"Print out a description of file system."},
    {OPT_TRUE, "showDirLog", (Address) &showDirLog, 
	"Print out the directory operation log."},
    {OPT_TRUE, "full", (Address) &full, 
	"Full a full error analysis."},
    {OPT_TRUE, "verbose", (Address) &verboseFlag, 
	"Output progress messages during execution."},
    {OPT_TRUE, "oldcp", (Address) &oldcp, 
	"Output progress messages during execution."},
};
/*
 * Forward routine declartions.
 */
extern Boolean LoadUsageArray _ARGS_((int diskFd, int checkPointSize, 
			char *checkPointPtr));
extern Boolean LoadDescMap _ARGS_((int diskFd, int checkPointSize,
			char *checkPointPtr));
extern char *GetUsageState _ARGS_((LfsSegUsageEntry *entryPtr));
static void CheckAllFiles _ARGS_((int diskFd));
static void CheckFile _ARGS_((int diskFd, int fileNum, 			
			LfsFileDescriptor *descPtr));
static void CheckIndirectBlock _ARGS_((int diskFd, int fileNum, 
		LfsFileDescriptor *descPtr, int blockNum, int blockAddress));
static void CheckBlock _ARGS_((int diskFd, int fileNum, 
		LfsFileDescriptor *descPtr, int blockNum, int blockAddress));
static void CheckUsageArray _ARGS_((int diskFd));
static void CheckSummaryRegions _ARGS_((int diskFd));
static void CheckSegUsageSummary _ARGS_((int diskFd, Seg *segPtr,
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void CheckDescMapSummary _ARGS_((int diskFd, Seg *segPtr, 
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void CheckFileLayoutSummary _ARGS_((int diskFd, Seg *segPtr,
	int startAddr, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static char *GetOwner _ARGS_((int blockNum));
static void PrintSuperBlock _ARGS_((LfsSuperBlock *superBlockPtr));
static void PrintCheckPointHdr _ARGS_((LfsCheckPointHdr *headerPtr, int region));
static ClientData LoadStableMem _ARGS_((int diskFd, LfsStableMemParams *smemParamsPtr, LfsStableMemCheckPoint *cpPtr, int type));
static int GetStableMemBlockIndex _ARGS_((ClientData clientData, int blockNum));
static void ShowDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr));

static char *FmtTime _ARGS_((void));
static Seg *SegInit _ARGS_((int diskFd, int segNumber));
static int SegStartAddr _ARGS_((Seg *segPtr));
static char *SegFetchBlock _ARGS_((Seg *segPtr, int blockOffset, int size));
static void SegReleaseBlock _ARGS_((Seg *segPtr, char *memPtr));
static void SegRelease _ARGS_((Seg *segPtr));

extern int open();
extern void panic();
extern int gettimeofday _ARGS_((struct timeval *tp, struct timezone *tzp));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of checklfs - parse arguments and do the work.
 *
 * Results:
 *	None.
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
    int	   diskFd, maxCheckPointSize;
    LfsCheckPointHdr	checkPointHdr[2], *checkPointHdrPtr;
    char		*checkPointPtr, *trailerPtr;
    LfsCheckPointRegion *regionPtr;
    LfsCheckPointTrailer *trailPtr;
    int			choosenOne, i;



    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(EXIT_BAD_ARG);
    } else {
	deviceName = argv[1];
    }
    diskFd = open(deviceName, O_RDONLY, 0);
    if (diskFd < 0) {
	fprintf(stderr,"%s:", argv[0]);
	perror(deviceName);
	exit(EXIT_HARD_ERROR);
    }
    /*
     * Fill in the super block header.
     */
    superBlockPtr = (LfsSuperBlock *) malloc(LFS_SUPER_BLOCK_SIZE);
    if (DiskRead(diskFd, superBlockOffset, sizeof(LfsSuperBlock), 
		(char *)superBlockPtr) != sizeof(LfsSuperBlock)) {
	fprintf(stderr,"%s:Can't read superblock.\n", deviceName);
	exit(EXIT_READ_FAILURE);

    }
    if (superBlockPtr->hdr.magic != LFS_SUPER_BLOCK_MAGIC) {
	fprintf(stderr,"%s:Bad magic number for filesystem\n", deviceName);
	exit(EXIT_READ_FAILURE);
    }
    if (dumpFlag) {
       PrintSuperBlock(superBlockPtr);
    }

    /*
     * Examine the two checkpoint areas to locate the checkpoint area with the
     * newest timestamp.
     */
    if (DiskRead(diskFd, superBlockPtr->hdr.checkPointOffset[0],
		sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+0)) != 
	sizeof(LfsCheckPointHdr)) {
	fprintf(stderr,"%s:Can't read checkPointHeader 0.\n", deviceName);
	checkPointHdr[0].timestamp = 0;
    }
    if (dumpFlag) {
	PrintCheckPointHdr(checkPointHdr, 0);
    }
    if (DiskRead(diskFd, superBlockPtr->hdr.checkPointOffset[1],
		sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+1))  != 
	sizeof(LfsCheckPointHdr)) {
	fprintf(stderr,"%s:Can't read checkPointHeader 1.\n", deviceName);
	checkPointHdr[1].timestamp = 0;
    }
    if ((checkPointHdr[0].timestamp == 0) &&
	(checkPointHdr[1].timestamp == 0)) {
	fprintf(stderr,"%s:Can't read either checkPointHeader.\n", deviceName);
	exit(EXIT_READ_FAILURE);
    }
    if (dumpFlag) {
	PrintCheckPointHdr(checkPointHdr+1, 1);
    }

    choosenOne = (checkPointHdr[0].timestamp<checkPointHdr[1].timestamp) ?
				1 : 0;

    if (oldcp) {
	choosenOne = !choosenOne;
    }
    maxCheckPointSize = superBlockPtr->hdr.maxCheckPointBlocks * 
				blockSize;
    checkPointPtr = malloc(maxCheckPointSize);
    if (verboseFlag) {
	printf("%s:Read LFS checkpoint from %s\n", FmtTime(), deviceName);
    }
    if (DiskRead(diskFd, superBlockPtr->hdr.checkPointOffset[choosenOne],
		maxCheckPointSize, checkPointPtr) != maxCheckPointSize) {
	fprintf(stderr,"%s:Can't read checkPoint %d\n", deviceName, choosenOne);
	exit(EXIT_READ_FAILURE);
    }


    checkPointHdrPtr = (LfsCheckPointHdr *) checkPointPtr;
    trailerPtr = (checkPointPtr + checkPointHdrPtr->size - 
				sizeof(LfsCheckPointTrailer));
    trailPtr = (LfsCheckPointTrailer *) trailerPtr;
    if (trailPtr->timestamp != checkPointHdrPtr->timestamp) {
	fprintf(stderr,"%s:Header timestamp %d doesn't match trailer timestamp %d\n", deviceName, checkPointHdrPtr->timestamp, trailPtr->timestamp);
	minorErrors++;
    }
    if (dumpFlag) {
	printf("Using checkpoint area %d with timestamp %d domain %d (%s)\n",
	    choosenOne, checkPointHdrPtr->timestamp, 
	    checkPointHdrPtr->domainNumber, checkPointHdrPtr->domainPrefix);
    }

    checkPointPtr = checkPointPtr + sizeof(LfsCheckPointHdr);

    numBlocks = superBlockPtr->usageArray.numberSegments * 
			    (superBlockPtr->usageArray.segmentSize/blockSize) +
			    superBlockPtr->hdr.logStartOffset;
    if (full) {
	/*
	 * Allocate the blockInfoArray for the file system. Zero 
	 * marks all blocks as UNUSED.
	 */
	blockInfoArray = (BlockInfo *) calloc(numBlocks, sizeof(BlockInfo));
    } else {
	/*
	 * Allocate the block usage bitmap.
	 */
	Bit_Alloc(numBlocks, usageBitMap);
	Bit_Alloc(numBlocks, descBlockBitMap);
    }
    /*
     * Allocate the descriptor found array. Zero marks all descriptors
     * as unallocated.
     */
    descInfoArray = (FdInfo *) calloc(superBlockPtr->descMap.maxDesc, 
					sizeof(FdInfo));

    /*
     * Allocate the real active bytes array. Initialize each entry to zero
     * bytes used.
     */
    activeBytesArray = (int *) calloc(superBlockPtr->usageArray.numberSegments,
					sizeof(activeBytesArray[0]));
    /*
     * Mark blocks upto log start as checkpoint blocks.
     */
    for (i = 0; i < superBlockPtr->hdr.logStartOffset; i++) {
	if (full) { 
	    blockInfoArray[i].type = CHECKPOINT;
	    blockInfoArray[i].found = TRUE;
	} else {
	    Bit_Set(i, usageBitMap);
	}
    }
    /*
     * Load the LFS metadata from the last checkpoint.
     */
    while (checkPointPtr < trailerPtr) { 
	regionPtr = (LfsCheckPointRegion *) checkPointPtr;
	if (regionPtr->size == 0) {
	    break;
	}
	switch (regionPtr->type) {
	case LFS_SEG_USAGE_MOD:
	    LoadUsageArray(diskFd, regionPtr->size - sizeof(*regionPtr),
				(char *) (regionPtr+1));
	    break;
	case LFS_DESC_MAP_MOD:
	    LoadDescMap(diskFd, regionPtr->size - sizeof(*regionPtr),
				(char *) (regionPtr+1));
	    break;
	case LFS_FILE_LAYOUT_MOD:
	    if (regionPtr->size != sizeof(*regionPtr)) {
		minorErrors++;
		fprintf(stderr,"%s:Bad size %d for FILE_LAYOUT checkpoint\n",
				deviceName,regionPtr->size);
	    }
	    break;
	default: {
	    minorErrors++;
	    fprintf(stderr,"%s:Unknown region type %d of size %d\n",
			deviceName,
			regionPtr->type, regionPtr->size);
	    break;
	    }
	}
	checkPointPtr += regionPtr->size;
    }

    if (verboseFlag) {
	printf("%s:Checking files\n", FmtTime());
    }
    CheckAllFiles(diskFd);
    if (verboseFlag) {
	printf("%s:Checking usage array\n",FmtTime());
    }
    CheckUsageArray(diskFd);
    if (verboseFlag) {
	printf("%s:Checking directory tree\n",FmtTime());
    }
    CheckDirTree(diskFd);

    if (verboseFlag) {
	printf("%s:Checking summary regions\n", FmtTime());
    }
    if (!full) { 
	foundBitMap = descBlockBitMap;
	Bit_Zero(numBlocks, foundBitMap);
	for (i = 0; i < superBlockPtr->hdr.logStartOffset; i++) {
	    Bit_Set(i, foundBitMap);
	}
    }
    CheckSummaryRegions(diskFd);

    /*
     * Report any block that was pointed to by something but to described
     * by that's block summary info.
     */

    if (verboseFlag) {
	printf("%s:Reporting errors\n", FmtTime());
    }
    for (i = 0; i < numBlocks; i++) {
	if (full) { 
	    if ((blockInfoArray[i].type != UNUSED) && !blockInfoArray[i].found) {
		 majorErrors++;
		fprintf(stderr,"%s:No summary region for block at %d own by %s\n",
			    deviceName, i, GetOwner(i));
	    }
	} else {
	    if (Bit_IsSet(i, usageBitMap) && Bit_IsClear(i, foundBitMap)) {
		majorErrors++;
		fprintf(stderr,"%s:No summary region for in use block at %d\n",
			    deviceName, i);
	    }

	}

    }
    /*
     * Report any desc allocated but not described by a summary block.
     */
    for (i = 2; i < superBlockPtr->descMap.maxDesc; i++) {
	LfsDescMapEntry *descMapPtr;
	descMapPtr = DescMapEntry(i);
	if ((descMapPtr->flags == LFS_DESC_MAP_ALLOCED) &&
		!(descInfoArray[i].flags & FD_ALLOCATED)) {
	    majorErrors++;
	    fprintf(stderr,"%s:No summary region for desc %d; should be a %d\n",
			deviceName, i, descMapPtr->blockAddress);
	}
    }
    if (majorErrors + minorErrors > 0) {
	printf("%s: %d major errors %d minor errors\n", deviceName, 
		majorErrors, minorErrors);
	if (majorErrors > 0) {
	    exit(EXIT_HARD_ERROR);
	} 
    }
    exit(EXIT_OK);
    return EXIT_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * LoadUsageArray --
 *
 *	Load the segment usage array into memory.
 *
 * Results:
 *	TRUE if array can be loaded. FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
LoadUsageArray(diskFd, checkPointSize, checkPointPtr)
    int	diskFd;		/* File descriptor of disk. */
    int	checkPointSize; /* Size of the checkpoint region. */
    char *checkPointPtr; /* The checkpoint region. */
{
    LfsSegUsageParams	*usagePtr;
    LfsSegUsageCheckPoint	*cp;
    LfsSegUsageEntry		*entryPtr;
    LfsStableMemCheckPoint *cpPtr;
    LfsStableMemParams  *smemParamsPtr;
    int	i, numClean,  numDirty, numFull, freeBlocks;
    Boolean ret = TRUE;

    usagePtr = &(superBlockPtr->usageArray);
    smemParamsPtr = &(superBlockPtr->usageArray.stableMem);

    cp = (LfsSegUsageCheckPoint *) checkPointPtr;
     cpPtr = (LfsStableMemCheckPoint *)
			(checkPointPtr + sizeof(LfsSegUsageCheckPoint));

    if (dumpFlag) { 
	printf("NumClean segments %d (%3.1f%%) NumDirty %d (%3.1f%%)\n",
	cp->numClean, 
	100.0*cp->numClean/(double)usagePtr->numberSegments,
	cp->numDirty,
	100.0*cp->numDirty/(double)usagePtr->numberSegments);

	printf("FreeBlocks %d (%3.1f%%) dirtyActiveBytes %d (%3.1f%%)\ncurrentSegment %d currentBlockOffset %d curSegActiveBytes %d\npreviousSegment %d cleanSegList %d\n",
	cp->freeBlocks, 
	100.0*cp->freeBlocks/ (double) numBlocks,
	cp->dirtyActiveBytes, 
	100.0*cp->dirtyActiveBytes/(double)usagePtr->segmentSize,
	cp->currentSegment, cp->currentBlockOffset, cp->curSegActiveBytes,
	cp->previousSegment, cp->cleanSegList);
    }

    usageArrayDataPtr =  LoadStableMem(diskFd, smemParamsPtr, cpPtr, 
			USAGE_ARRAY);
    entryPtr = (LfsSegUsageEntry *) UsageArrayEntry(cp->currentSegment);
    entryPtr->activeBytes = cp->curSegActiveBytes;
    if (entryPtr->activeBytes <= cp->dirtyActiveBytes) {
	entryPtr->flags = LFS_SEG_USAGE_DIRTY;
    }
   if (dumpFlag) { 
	for (i = 0; i < usagePtr->numberSegments; i++) {
	    entryPtr = (LfsSegUsageEntry *) UsageArrayEntry(i);
	    printf("SegNum %d activeBytes %d state %s\n", 
		    i, entryPtr->activeBytes,
		    GetUsageState(entryPtr));
	}
    }
    freeBlocks = 0;
    numClean = numDirty = numFull = 0;
    for (i = 0; i < usagePtr->numberSegments; i++) {
	int bytes;
	entryPtr = (LfsSegUsageEntry *) UsageArrayEntry(i);
	bytes =  ((entryPtr->flags & LFS_SEG_USAGE_CLEAN) ? 0 :
				entryPtr->activeBytes);
	bytes = usagePtr->segmentSize - bytes;
	freeBlocks += (bytes + blockSize - 1)/blockSize;
	if (entryPtr->flags == LFS_SEG_USAGE_CLEAN) {
	    numClean++;
	} else if (entryPtr->flags == LFS_SEG_USAGE_DIRTY) {
	    numDirty++;
	    if (entryPtr->activeBytes > cp->dirtyActiveBytes) {
		minorErrors++;
	       fprintf(stderr, 
       "%s:UsageArray: segment %d is marked dirty with activeBytes of %d\n",
			deviceName,i, entryPtr->activeBytes);
	    }
	} else if (entryPtr->flags == 0) {
	    numFull++;
	    if (entryPtr->activeBytes < cp->dirtyActiveBytes) {
		minorErrors++;
	       fprintf(stderr, 
       "%s:UsageArray: segment %d is marked full with activeBytes of %d\n",
			deviceName,i, entryPtr->activeBytes);
	    }
	} else {
		numFull++;
		minorErrors++;
	       fprintf(stderr, 
	       "%s:UsageArray: segment %d has unknown flags of 0x%x\n",
			deviceName, i, entryPtr->flags);
	}
	if (entryPtr->flags != LFS_SEG_USAGE_CLEAN) {
	    if ((entryPtr->activeBytes < 0) ||
		(entryPtr->activeBytes > usagePtr->segmentSize)) {
		   majorErrors++;
		   fprintf(stderr, 
		   "%s:UsageArray: segment %d has bad activeBytes of %d\n",
			    deviceName, i, entryPtr->activeBytes);
	    }
	}
    }
    if (numClean != cp->numClean) {
	minorErrors++;
	fprintf(stderr,"%s:UsageArray: Clean count wrong; is %d should be %d\n",
		    deviceName, cp->numClean, numClean);

    }
    if (numDirty != cp->numDirty) {
	minorErrors++;
	fprintf(stderr,"%s:UsageArray: Dirty count wrong; is %d should be %d\n",
		    deviceName, cp->numDirty, numDirty);

    }
    if (freeBlocks != cp->freeBlocks) {
	minorErrors++;
	fprintf(stderr,"%s:UsageArray: FreeBlocks wrong; is %d should be %d\n",
		    deviceName, cp->freeBlocks, freeBlocks);
    }
   return ret;
}


/*
 *----------------------------------------------------------------------
 *
 * LoadDescMap --
 *
 *	Load the descriptor map array into memory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean 
LoadDescMap(diskFd, checkPointSize, checkPointPtr)
    int	diskFd;		/* File descriptor of disk. */
    int	checkPointSize; /* Size of the checkpoint region. */
    char *checkPointPtr; /* The checkpoint region. */
{
    LfsDescMapParams	*descMapParamsPtr;
    LfsDescMapEntry	*descMapPtr;
    LfsDescMapCheckPoint	*cp;
    int				i;
    LfsStableMemCheckPoint *cpPtr;
    int			numAlloced;
    LfsStableMemParams  *smemParamsPtr;
   Boolean 		ret = TRUE;

    descMapParamsPtr = &(superBlockPtr->descMap);
    smemParamsPtr = &(superBlockPtr->descMap.stableMem);


    descMapCheckPointPtr = cp = (LfsDescMapCheckPoint *) checkPointPtr;

     cpPtr = (LfsStableMemCheckPoint *)
			(checkPointPtr + sizeof(LfsDescMapCheckPoint));

    descMapDataPtr =  LoadStableMem(diskFd, smemParamsPtr, cpPtr, DESC_MAP);

    if (dumpFlag) { 
	printf("DescMap num allocated: %d\n", cp->numAllocDesc);
	for (i = 0; i < superBlockPtr->descMap.maxDesc; i++) {
	    descMapPtr = DescMapEntry(i);
	    if (descMapPtr->flags == LFS_DESC_MAP_ALLOCED) { 
		printf("File %d at %d (seg %d) version %d flags %d\n", 
			    i, descMapPtr->blockAddress, 
			    descMapPtr->blockAddress/
			    (superBlockPtr->usageArray.segmentSize/blockSize),
			    descMapPtr->truncVersion,
			    descMapPtr->flags);
		}
	}
    }
    numAlloced = 0;
    for (i = 0; i < descMapParamsPtr->maxDesc; i++) {
	descMapPtr = DescMapEntry(i);
	if (descMapPtr->flags == LFS_DESC_MAP_ALLOCED) {
	    numAlloced++;
	} else if (descMapPtr->flags != 0) {
	    minorErrors++;
	    fprintf(stderr,"%s:Unknowned desc map flags (0x%x) for file %d\n",
			deviceName, descMapPtr->flags, i);
	}
    }
    if (numAlloced != descMapCheckPointPtr->numAllocDesc) {
	majorErrors++;
	fprintf(stderr, "%s:DescMap: Bad alloc count; is %d should be %d\n",
			deviceName, numAlloced, descMapCheckPointPtr->numAllocDesc);

    }
    return ret;

}

char *
GetUsageState(entryPtr)
    LfsSegUsageEntry *entryPtr;
{
    if (entryPtr->flags & LFS_SEG_USAGE_DIRTY) 
	return "Dirty";
    if (entryPtr->flags & LFS_SEG_USAGE_CLEAN) 
	return "Clean";
    return "Full";
}

/*
 *----------------------------------------------------------------------
 *
 * DiskRead --
 *
 *	Read data from disk.
 *
 * Results:
 *	The number of bytes returned.  -1 if error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
DiskRead(diskFd, blockOffset, bufferSize, bufferPtr)
    int	diskFd;		/* File descriptor of disk. */
    int	blockOffset;	/* Block offset to start read. */
    char *bufferPtr;	/* Buffer to place data. */
    int	 bufferSize;	/* Size of buffer. */
{
    int	status;
    int	blocks;
    char *bufPtr;


    /*
     * Seek to the start of the blocks to read.
     */
    status = lseek(diskFd, blockOffset*blockSize, L_SET);
    if (status < 0) {
	fprintf(stderr,"%s:", deviceName);
	perror("lseek");
	return status;
    }
    /*
     * Read the blocks handling the case the a request that is not a 
     * multiple number of blocks by reading to a temp buffer and copying.
     */
    blocks = (bufferSize + blockSize-1)/blockSize;
    if (bufferSize != blocks * blockSize) { 
	bufPtr = malloc(blocks*blockSize);
    } else {
	bufPtr = bufferPtr;
    }
    status = read(diskFd, bufPtr, blocks*blockSize);
    if (status != blocks*blockSize) {
	if (status < 0) {
	    fprintf(stderr,"%s:", deviceName);
	    perror("read device");
	    return status;
	}
	fprintf(stderr,"%s:Short read on device %d != %d\n",deviceName,
		status, blocks*blockSize);
    } else {
	status = bufferSize;
    }
    if (bufPtr != bufferPtr) { 
	bcopy(bufPtr, bufferPtr, bufferSize);
	free(bufPtr);
    }
    return status;
}

typedef struct StableMem {
    LfsStableMemParams  *paramsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *checkpointPtr;  /* Checkpoint pointer. */
    char		*dataPtr;
} StableMem;

/*
 *----------------------------------------------------------------------
 *
 * LoadStableMem --
 *
 *	Load a stable memory data structure
 *
 * Results:
 *	A clientdata that can be used to access stable mem.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ClientData 
LoadStableMem(diskFd, smemParamsPtr, cpPtr, type)
    int diskFd;	/* Disk of file system. */
    LfsStableMemParams  *smemParamsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *cpPtr;  /* Checkpoint pointer. */
    int		type;
{
    int arraySize, i;
    char *startDataPtr, *dataPtr;
    int *blockIndexPtr;
    int blockNum;
    LfsStableMemBlockHdr *hdrPtr;
    StableMem *stableMemPtr;

    arraySize = smemParamsPtr->blockSize * smemParamsPtr->maxNumBlocks;
    dataPtr = startDataPtr = malloc(arraySize);
    blockIndexPtr = (int *)(((char *) cpPtr) + sizeof(LfsStableMemCheckPoint));
    for (blockNum = 0; blockNum < cpPtr->numBlocks; blockNum++) {
	unsigned int blockIndex = blockIndexPtr[blockNum];
	if (blockIndex == FSDM_NIL_INDEX) {
	    bzero(dataPtr, smemParamsPtr->blockSize);
	} else {
	    if (blockIndex+smemParamsPtr->blockSize/blockSize > numBlocks) {
		   majorErrors++;
		   fprintf(stderr, "%s:StableMem%d:Block %d out of range %d\n",
				deviceName, smemParamsPtr->memType, 
				blockNum, blockIndex);
		blockIndex = 0;
	    }
	    for (i = 0; i < smemParamsPtr->blockSize/blockSize; i++) {
		if (full) { 
		    if (blockInfoArray[blockIndex + i].type != UNUSED) {
		        majorErrors++;
			fprintf(stderr, "%s:StableMem%d:Block %d duplicate usage of block %d ",
    
				    deviceName, smemParamsPtr->memType, blockNum, blockIndex + i);
			fprintf(stderr,"Previous use at %s\n",
				     GetOwner(blockIndex + i));
    
		    } else {
			blockInfoArray[blockIndex + i].type = type;
			blockInfoArray[blockIndex + i].blockNum = blockNum;
			activeBytesArray[BlockToSegmentNum(blockIndex + i)] +=
					    blockSize;
		    }
		} else {
		    if (Bit_IsSet(blockIndex + i, usageBitMap)) {
		        majorErrors++;
			fprintf(stderr, "%s:StableMem%d:Block %d duplicate usage of block %d\n",
    
				    deviceName, smemParamsPtr->memType, blockNum, blockIndex + i);
		    } else {
			Bit_Set(blockIndex + i, usageBitMap);
			activeBytesArray[BlockToSegmentNum(blockIndex + i)] +=
					    blockSize;
		    }
		}
	    }
	    if (DiskRead(diskFd, blockIndex, 
				  smemParamsPtr->blockSize, dataPtr) != 
		smemParamsPtr->blockSize) { 
		majorErrors++;
		fprintf(stderr, "%s:Can't read desc map block %d\n", deviceName, blockNum);
	    }
	    hdrPtr = (LfsStableMemBlockHdr *) dataPtr;
	    if ((hdrPtr->magic != LFS_STABLE_MEM_BLOCK_MAGIC) || 
	        (hdrPtr->memType != smemParamsPtr->memType) ||
		(hdrPtr->blockNum != blockNum)) {
		majorErrors++;
		fprintf(stderr, "%s:Bad stable mem header at %d for memType %d blockNum %d\n"
				, deviceName, blockIndex, 
				smemParamsPtr->memType, blockNum);
	    }
	}
	dataPtr += smemParamsPtr->blockSize;
    }
    bzero(dataPtr, (smemParamsPtr->maxNumBlocks - cpPtr->numBlocks) * 
			smemParamsPtr->blockSize);
    stableMemPtr = (StableMem *) malloc(sizeof(StableMem));
    stableMemPtr->paramsPtr = smemParamsPtr;
    stableMemPtr->checkpointPtr = cpPtr;
    stableMemPtr->dataPtr = startDataPtr;
    return (ClientData) stableMemPtr;
}
char *
GetStableMemEntry(clientData, entryNumber)
    ClientData clientData;
    int entryNumber;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int blockNum, offset;

    if ((entryNumber < 0) || 
	(entryNumber >= stableMemPtr->paramsPtr->maxNumEntries)) {
	fprintf(stderr,"Bad stable memory entry number %d\n", entryNumber);
	entryNumber = 0;
    }
    blockNum = entryNumber / stableMemPtr->paramsPtr->entriesPerBlock;
    offset = (entryNumber % stableMemPtr->paramsPtr->entriesPerBlock) * 
		stableMemPtr->paramsPtr->entrySize + 
		   sizeof(LfsStableMemBlockHdr);

    return stableMemPtr->dataPtr + 
        blockNum * stableMemPtr->paramsPtr-> blockSize + offset;
}
static int
GetStableMemBlockIndex(clientData, blockNum)
    ClientData clientData;
    int blockNum;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    int *blockIndexPtr;

    if ((blockNum < 0) || 
	(blockNum >= stableMemPtr->paramsPtr->maxNumBlocks)) {
	fprintf(stderr,"Bad stable memory block num %d\n", blockNum);
	return FSDM_NIL_INDEX;
    }
    blockIndexPtr = (int *)((char *) (stableMemPtr->checkpointPtr) + sizeof(LfsStableMemCheckPoint));
    return blockIndexPtr[blockNum];
}

/*
 *----------------------------------------------------------------------
 *
 * CheckAllFiles --
 *
 *	Check all the files in the system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckAllFiles(diskFd)
    int diskFd;
{
    LfsFileDescriptor	*descPtr;
    char *descBuf;
    LfsDescMapEntry *descMapPtr;
    int bufSize, i, j;

    bufSize = superBlockPtr->fileLayout.descPerBlock * sizeof(*descPtr);

#ifdef notdef
    descBuf = alloca(bufSize);
#else
    descBuf = malloc(bufSize);
#endif
    descMapPtr = DescMapEntry(0);
    if (descMapPtr->flags != LFS_DESC_MAP_ALLOCED) {
	majorErrors++;
	fprintf(stderr,"%s:CheckAllFiles: file 0 not allocated.\n",deviceName);
    }
    descMapPtr = DescMapEntry(1);
    if (descMapPtr->flags != LFS_DESC_MAP_ALLOCED) {
	majorErrors++;
	fprintf(stderr,"%s:CheckAllFiles: file 1 not allocated.\n",deviceName);
    }
    descInfoArray[0].flags = descInfoArray[1].flags = FD_UNREADABLE;

    for (i = superBlockPtr->descMap.maxDesc-1; i > 1; i--) {
	descMapPtr = DescMapEntry(i);
	if (descMapPtr->flags != LFS_DESC_MAP_ALLOCED) {
	    continue;
	}
	if ((descMapPtr->blockAddress < 0) || 
	    (descMapPtr->blockAddress > numBlocks)) {
	    majorErrors++;
	   fprintf(stderr, "%s:CheckAllFiles: Desc %d address out of range %d\n",
			deviceName, i, descMapPtr->blockAddress);
	    descInfoArray[i].flags = FD_UNREADABLE;
	    continue;
	}
	if (full) { 
	    if ((blockInfoArray[descMapPtr->blockAddress].type != UNUSED) &&
		(blockInfoArray[descMapPtr->blockAddress].type != DESC)) {
		majorErrors++;
		fprintf(stderr, "%s:CheckAllFiles:Desc block for %d duplicate usage of block %d ",
    
			    deviceName, i, descMapPtr->blockAddress);
		fprintf(stderr,"Previous use at %s\n",
			    GetOwner(descMapPtr->blockAddress));
    
	    } else {
		blockInfoArray[descMapPtr->blockAddress].type = DESC;
		blockInfoArray[descMapPtr->blockAddress].blockNum = 0;
		activeBytesArray[BlockToSegmentNum(descMapPtr->blockAddress)] +=
				    sizeof(LfsFileDescriptor);
	    }
	} else {
	    if (Bit_IsSet(descMapPtr->blockAddress, usageBitMap) &&
		Bit_IsClear(descMapPtr->blockAddress, descBlockBitMap)) {
		majorErrors++;
		fprintf(stderr, "%s:CheckAllFiles:Desc block for %d duplicate usage of block %d\n",
    
			    deviceName, i, descMapPtr->blockAddress);

		activeBytesArray[BlockToSegmentNum(descMapPtr->blockAddress)] +=
				    sizeof(LfsFileDescriptor);
		Bit_Set(descMapPtr->blockAddress, descBlockBitMap);
	    } else {
		Bit_Set(descMapPtr->blockAddress, usageBitMap);
		Bit_Set(descMapPtr->blockAddress, descBlockBitMap);
		activeBytesArray[BlockToSegmentNum(descMapPtr->blockAddress)] +=
				    sizeof(LfsFileDescriptor);
	    }
	}
	if (DiskRead(diskFd, descMapPtr->blockAddress, bufSize, descBuf)
			!= bufSize) {
	    majorErrors++;
	    fprintf(stderr,"%s:CheckAllFiles: Can't read desc for file %d\n",
				deviceName,i);
	}
	descPtr = (LfsFileDescriptor *)descBuf;
	for (j = 0; j < superBlockPtr->fileLayout.descPerBlock; j++) {
	    if (!(descPtr->common.flags & FSDM_FD_ALLOC)) {
		break;
	    }
	    if (descPtr->common.magic != FSDM_FD_MAGIC) {
		majorErrors++;
		fprintf(stderr,"%s:CheckAllFiles: Corrupted descriptor block at %d, magic number 0x%x\n", deviceName, descMapPtr->blockAddress, descPtr->common.magic);
	    }
	    if (descPtr->fileNumber == i) {
		break;
	    }
	    descPtr++;
	}
	if ((j >= superBlockPtr->fileLayout.descPerBlock) ||
	    !(descPtr->common.flags & FSDM_FD_ALLOC) ||
	    (descPtr->fileNumber != i)) {
	    majorErrors++;
	    fprintf(stderr,"%s:CheckAllFiles: Can't desc for file %d at %d\n",
			deviceName,i, descMapPtr->blockAddress);
	    descInfoArray[i].flags = FD_UNREADABLE;
	    continue;
	} else {
	    descInfoArray[i].flags = FD_ALLOCATED;
	}
	if (descPtr->common.fileType == FS_DIRECTORY) {
	    descInfoArray[i].flags |= IS_A_DIRECTORY;
	}
	descInfoArray[i].origLinkCount = descPtr->common.numLinks;
	CheckFile(diskFd, i, descPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckFile --
 *
 *	Check a file in the system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckFile(diskFd, fileNum, descPtr) 
    int diskFd;
    int fileNum;
    LfsFileDescriptor *descPtr;
{
    int i;

    for (i = 0; i < FSDM_NUM_DIRECT_BLOCKS; i++) {
	if (descPtr->common.direct[i] != FSDM_NIL_INDEX) {
	    if (i * FS_BLOCK_SIZE > descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName,fileNum, i, descPtr->common.lastByte);
		 continue;
	    }
	    CheckBlock(diskFd, fileNum, descPtr, i, descPtr->common.direct[i]);
	}
    }
    if (descPtr->common.indirect[0] != FSDM_NIL_INDEX) { 
	    if (FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE > 
			descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName,fileNum, -1, descPtr->common.lastByte);
	    }
	    CheckIndirectBlock(diskFd, fileNum, descPtr, -1,
				descPtr->common.indirect[0]);
    }
    if (descPtr->common.indirect[1] != FSDM_NIL_INDEX) { 
	    if (FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE + 
			FS_BLOCK_SIZE * FS_BLOCK_SIZE/4 > 
			descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
	"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName, fileNum, -2, descPtr->common.lastByte);
	    }
	    CheckIndirectBlock(diskFd, fileNum, descPtr, -2, 
				descPtr->common.indirect[1]);
    }
    if (descPtr->common.indirect[2] != FSDM_NIL_INDEX) { 
		majorErrors++;
		fprintf(stderr, 
	"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName, fileNum, -3, descPtr->common.lastByte);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CheckIndirectBlock --
 *
 *	Check an indirect block of a file..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CheckIndirectBlock(diskFd, fileNum, descPtr, blockNum, blockAddress)
    int diskFd;
    int fileNum;
    LfsFileDescriptor *descPtr;
    int blockNum;
    int	blockAddress;
{
    int  i;
    int blockPtrs[FS_BLOCK_SIZE/4];



    CheckBlock(diskFd, fileNum, descPtr, blockNum, blockAddress);
    if (DiskRead(diskFd, blockAddress, FS_BLOCK_SIZE, (char *)blockPtrs)
		!= FS_BLOCK_SIZE) {
	majorErrors++;
	fprintf(stderr,"%s:CheckIndirectBlock: Can't read block %d of file %d.\n",
				deviceName, blockNum, fileNum);
	return;

    }
    if ((blockNum == -1) || (blockNum < -3)) { 
	int start;
	if (blockNum == -1) {
	    start = FSDM_NUM_DIRECT_BLOCKS;
	} else {
	    start = FSDM_NUM_DIRECT_BLOCKS + (FS_BLOCK_SIZE/4) + 
			     (FS_BLOCK_SIZE/4) * 
			     (-blockNum - (FSDM_NUM_INDIRECT_BLOCKS+1));
	}
	for (i = 0; i < FS_BLOCK_SIZE/4; i++) {
	    if (blockPtrs[i] != FSDM_NIL_INDEX) { 
		CheckBlock(diskFd, fileNum, descPtr,
			   start + i,blockPtrs[i]);
	    }
	}
	return;
    } 
    if (blockNum == -2) {
	for (i = 0; i < FS_BLOCK_SIZE/4; i++) {
	    if (blockPtrs[i] != FSDM_NIL_INDEX) { 
		CheckIndirectBlock(diskFd, fileNum, descPtr,
			    - i  - (FSDM_NUM_INDIRECT_BLOCKS+1),blockPtrs[i]);
	    }
	}
	return;
    }
    majorErrors++;
    fprintf(stderr,"%s:CheckIndirectBlock: Bad block number %d for file %d\n",
			deviceName, blockNum, fileNum);
}


/*
 *----------------------------------------------------------------------
 *
 * CheckFile --
 *
 *	Check a block of a file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckBlock(diskFd, fileNum, descPtr, blockNum, blockAddress)
    int diskFd;
    int fileNum;
    LfsFileDescriptor *descPtr;
    int blockNum;
    int	blockAddress;
{
    int j;
    int sizeInBlocks;


    sizeInBlocks = FS_BLOCK_SIZE/blockSize;
    if (blockNum >= 0) { 
	if ((blockNum+1) * FS_BLOCK_SIZE > descPtr->common.lastByte + 1) {
	    int size;
	    size = descPtr->common.lastByte - FS_BLOCK_SIZE*blockNum +  1;
	     sizeInBlocks = (size + blockSize - 1)/blockSize;
	 }
    }

    if ((blockAddress < 0) || 
	(blockAddress+sizeInBlocks > numBlocks)) {
	majorErrors++;
	fprintf(stderr, "%s:CheckFile: file %d block %d is out of range %d\n",
		deviceName, fileNum, blockNum, blockAddress);
	return;
    }
    for (j = 0; j < sizeInBlocks; j++) {
	if (full) { 
	    if (blockInfoArray[blockAddress+j].type != UNUSED) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile:file %d block %d duplicate usage of block %d ",
			deviceName,
    
			    fileNum, blockNum, blockAddress + j);
		fprintf(stderr,"Previous use at %s\n", 
				    GetOwner(blockAddress + j));
	    } else {
		blockInfoArray[blockAddress + j].type = FILE;
		blockInfoArray[blockAddress + j].fileNum = fileNum;
		blockInfoArray[blockAddress + j].blockNum = blockNum;
	    }
	} else {
	    if (Bit_IsSet(blockAddress+j, usageBitMap)) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile:file %d block %d duplicate usage of block %d\n",
			deviceName,
			    fileNum, blockNum, blockAddress + j);
	    } else {
		Bit_Set(blockAddress+j, usageBitMap);
	    }
	}
    }
    activeBytesArray[BlockToSegmentNum(blockAddress)] 
			+= blockSize*sizeInBlocks;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckUsageArray --
 *
 *	Check the segment usage array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckUsageArray(diskFd)
int diskFd;
{
    int segNo;
    LfsSegUsageEntry *usageArrayPtr;

    for (segNo = 0; segNo < superBlockPtr->usageArray.numberSegments; 
	segNo++) {
	usageArrayPtr = UsageArrayEntry(segNo);
	if (usageArrayPtr->flags == LFS_SEG_USAGE_CLEAN) {
	    if (activeBytesArray[segNo] != 0) {
		majorErrors++;
		fprintf(stderr, "%s:Clean segment %d with activeBytes %d\n",
			deviceName,segNo, activeBytesArray[segNo]);
	    }
	    continue;
	}
	if (usageArrayPtr->activeBytes != activeBytesArray[segNo]) {
		if (usageArrayPtr->activeBytes < activeBytesArray[segNo]) { 
		    majorErrors++;
		 } else {
		    minorErrors++;
		 }
		fprintf(stderr,"%s:CheckUsageArray: Active bytes for seg %d is wrong; is %d should be %d\n", deviceName, segNo, usageArrayPtr->activeBytes, 
			activeBytesArray[segNo]);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckSummaryRegions --
 *
 *	Check the segment summary regions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckSummaryRegions(diskFd)
int diskFd;
{
    int segNo, startAddr, blockOffset;
    char *summaryLimitPtr, *summaryPtr;
    LfsSegSummary *segSummaryPtr;
    LfsSegSummaryHdr *segSummaryHdrPtr;
    Seg	*segPtr;

    for (segNo = 0; segNo < superBlockPtr->usageArray.numberSegments; 
	segNo++) {
	segPtr = SegInit(diskFd, segNo);
	startAddr = SegStartAddr(segPtr) + segPtr->segSizeInBlocks;
	blockOffset = 1; 
	do { 
	    segSummaryPtr = (LfsSegSummary *) 
			SegFetchBlock(segPtr, blockOffset-1, blockSize);
	    if (segSummaryPtr->size == 0) {
		SegReleaseBlock(segPtr, (char *) segSummaryPtr);
		break;
	    }
	    if (segSummaryPtr->magic != LFS_SEG_SUMMARY_MAGIC) {
		majorErrors++;
		fprintf(stderr,"%s: Bad magic number 0x%x for summary region of segment %d\n", deviceName, segSummaryPtr->magic, segNo);
		SegReleaseBlock(segPtr, (char *) segSummaryPtr);
		break;
	    }
	    summaryLimitPtr = (char *) segSummaryPtr + segSummaryPtr->size;
	    summaryPtr = (char *) (segSummaryPtr + 1);
	    while (summaryPtr < summaryLimitPtr) {
	       segSummaryHdrPtr = (LfsSegSummaryHdr *) summaryPtr;
	       if (segSummaryHdrPtr->lengthInBytes == 0) {
		    break;
	       }
	       switch (segSummaryHdrPtr->moduleType) { 
	       case LFS_SEG_USAGE_MOD:
		  CheckSegUsageSummary(diskFd, segPtr, startAddr - blockOffset, blockOffset, 
				    segSummaryHdrPtr);
		   break;
	       case LFS_DESC_MAP_MOD:
		  CheckDescMapSummary(diskFd, segPtr, startAddr - blockOffset, blockOffset, 
				    segSummaryHdrPtr);
		   break;
	       case LFS_FILE_LAYOUT_MOD:
		  CheckFileLayoutSummary(diskFd, segPtr,startAddr - blockOffset, blockOffset, 
				    segSummaryHdrPtr);
		    break;
	       default: {
		    fprintf(stderr,"%s:CheckSummary: Unknown module type %d at %d, Size %d Blocks %d\n",
			    deviceName, segSummaryHdrPtr->moduleType, startAddr - blockOffset, segSummaryHdrPtr->lengthInBytes, segSummaryHdrPtr->numDataBlocks);
		    break;
		    }
	       }
	       summaryPtr += segSummaryHdrPtr->lengthInBytes;
	       blockOffset += segSummaryHdrPtr->numDataBlocks;
	   }
	   SegReleaseBlock(segPtr, (char *) segSummaryPtr);
	   blockOffset = segSummaryPtr->nextSummaryBlock;
	} while( blockOffset != -1);
	SegRelease(segPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckSegUsageSummary --
 *
 *	Check the segment summary regions for the seg usage map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckSegUsageSummary(diskFd, segPtr, startAddress, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg *segPtr;
    int startAddress;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    int blocks, *blockArray, i, startAddr, fsBlocks, j;

    fsBlocks = superBlockPtr->usageArray.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	majorErrors++;
	fprintf(stderr,"%s:CheckSegUsageSummary: Wrong block count; is %d should be %s\n", deviceName, blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > superBlockPtr->usageArray.stableMem.maxNumBlocks)){
	    majorErrors++;
	   fprintf(stderr,"%s:CheckSegUsageSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	for (j = 0; j < fsBlocks; j++) { 
	    if (UsageArrayBlockIndex(blockArray[i]) != startAddr) {
		if (full) { 
		    if (blockInfoArray[startAddr + j].type != UNUSED) {
			majorErrors++;
			fprintf(stderr,"%s:CheckSegUsageSummary: Summary wrong. Not current block %d at %d in use by %s\n", deviceName, blockArray[i], startAddr + j,
			    GetOwner(startAddr + j));
		     }
		 } else {
		    if (Bit_IsSet(startAddr + j, usageBitMap)) {
			majorErrors++;
			fprintf(stderr,"%s:CheckSegUsageSummary: Summary wrong. Not current block %d at %d\n", deviceName, blockArray[i], startAddr + j);
		     }
		 }
	    } else {
		if (full) { 
		    if ((blockInfoArray[startAddr + j].type != USAGE_ARRAY) ||
			(blockInfoArray[startAddr + j].blockNum != blockArray[i])) {
			majorErrors++;
			fprintf(stderr,"%s:CheckSegUsageSummary: Summary wrong. Current block %d at %d in use by %s\n", deviceName, blockArray[i], startAddr + j,
			    GetOwner(startAddr + j));
		     }
		     blockInfoArray[startAddr + j].found = TRUE;
	       } else { 
		   if (Bit_IsClear(startAddr + j, usageBitMap)) {
			fprintf(stderr,"%s:CheckSegUsageSummary: Summary wrong. Current block %d at %d\n", deviceName, blockArray[i], startAddr + j);
		   }
		   Bit_Set(startAddr + j, foundBitMap);
	       }
	    }
	}
    }

}

/*
 *----------------------------------------------------------------------
 *
 * CheckSegUsageSummary --
 *
 *	Check the segment summary regions for the seg usage map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckDescMapSummary(diskFd, segPtr, startAddress, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg *segPtr;
    int startAddress;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    int blocks, *blockArray, i, startAddr, fsBlocks, j;

    fsBlocks = superBlockPtr->descMap.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	majorErrors++;
	fprintf(stderr,"%s:CheckDescMapSummary: Wrong block count; is %d should be %s\n", deviceName,blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > superBlockPtr->descMap.stableMem.maxNumBlocks)){
	    majorErrors++;
	   fprintf(stderr,"%s:CheckDescMapSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	for (j = 0; j < fsBlocks; j++) { 
	    if (DescMapBlockIndex(blockArray[i]) != startAddr) {
		if (full) { 
		    if (blockInfoArray[startAddr + j].type != UNUSED) {
			majorErrors++;
			fprintf(stderr,"%s:CheckDescMapSummary: Summary wrong. Not current block %d at %d in use by %s\n", deviceName,blockArray[i], startAddr + j,
			     GetOwner(startAddr + j));
		     }
		 } else {
		    if (Bit_IsSet(startAddr + j, usageBitMap)) {
			majorErrors++;
			fprintf(stderr,"%s:CheckDescMapSummary: Summary wrong. Not current block %d at %d\n", deviceName,blockArray[i], startAddr + j);
		     }
		 }
	    } else {
		if (full) { 
		    if ((blockInfoArray[startAddr + j].type != DESC_MAP) ||
			(blockInfoArray[startAddr + j].blockNum != blockArray[i])) {
			majorErrors++;
			fprintf(stderr,"%s:CheckDescMapSummary: Summary wrong. Current block %d at %d in use by %s\n", deviceName, blockArray[i], startAddr + j,
			     GetOwner(startAddr + j));
		     }
		     blockInfoArray[startAddr + j].found = TRUE;
		 } else {
		     if (Bit_IsClear(startAddr + j, usageBitMap)) {
			majorErrors++;
			fprintf(stderr,"%s:CheckDescMapSummary: Summary wrong. Current block %d at %d\n", deviceName, blockArray[i], startAddr + j);
		     }
		     Bit_Set(startAddr + j, foundBitMap);
		 }

	    }
	}
    }

}

/*
 *----------------------------------------------------------------------
 *
 * CheckFileLayoutSummary --
 *
 *	Check the segment summary regions for the file layout code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckFileLayoutSummary(diskFd, segPtr, startAddr, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg  *segPtr;
    int startAddr;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    char *summaryPtr, *limitPtr;
    int descMapBlocks;
    int startAddress, j, ssize;
    LfsDescMapEntry *descMapPtr;

    ssize = superBlockPtr->usageArray.segmentSize;


    startAddress = startAddr;
    descMapBlocks = superBlockPtr->descMap.stableMem.blockSize/blockSize;
    summaryPtr = (char *) (segSummaryHdrPtr + 1);
    limitPtr = summaryPtr + segSummaryHdrPtr->lengthInBytes - 
			sizeof(LfsSegSummaryHdr);
    while (summaryPtr < limitPtr) {
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    int		fileNumber;
	    int		slot;
	    LfsFileDescriptor	*descPtr;
	    descPtr = (LfsFileDescriptor *) SegFetchBlock(segPtr, offset, 
						descMapBlocks*blockSize);
	    offset += descMapBlocks;
	    startAddress -= descMapBlocks;
	    for (slot = 0; slot < superBlockPtr->fileLayout.descPerBlock; 
		slot++) {
		int addr;
		/*
		 * The descriptor block is terminated by an unallocated
		 * descriptor.
		 */
		if (!(descPtr[slot].common.flags & FSDM_FD_ALLOC)) {
		    break;
		}
		addr = startAddress + (slot * sizeof(LfsFileDescriptor))/
						blockSize;
		fileNumber = descPtr[slot].fileNumber;
		if ((fileNumber < 0) || 
		    (fileNumber >= superBlockPtr->descMap.maxDesc)) {
		    majorErrors++;
		   fprintf(stderr,"%s:CheckFileLayoutSummary: bad file number %d in desc block at %d\n", deviceName, fileNumber, startAddress);
		   continue;
		}
		if (full) { 
		    if ((blockInfoArray[addr].type != DESC) &&
			(blockInfoArray[addr].type != UNUSED)) {
			majorErrors++;
			fprintf(stderr,"%s:CheckFileLayoutSummary: Desc block at %d overlaps %s\n", deviceName, addr, GetOwner(addr));
		    }
		    blockInfoArray[addr].found = TRUE;
		} else {
		    Bit_Set(addr, foundBitMap);
		}
	     }
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    SegReleaseBlock(segPtr, (char *) descPtr);
	    break;
	}
	case LFS_FILE_LAYOUT_DATA: {
	    int	*blockArray;
	    int			  i;
	    LfsFileLayoutSummary *fileSumPtr;
	    int firstBlock;
	    Boolean dead;
	    /*
	     * We ran into a data block. If it is still alive bring it into
	     * the cache. 
	     */
	     fileSumPtr = (LfsFileLayoutSummary *) summaryPtr;
	    if ((fileSumPtr->fileNumber < 0) || 
		(fileSumPtr->fileNumber >= superBlockPtr->descMap.maxDesc)) {
		majorErrors++;
	       fprintf(stderr,"%s:CheckFileLayoutSummary: bad file number %d at %d\n", deviceName, fileSumPtr->fileNumber, startAddress);
	       goto out;
	    }
	     /*
	      * Liveness check.   First see if the version number is
	      * the same and the file is still allocated.
	      */
	     descMapPtr = DescMapEntry(fileSumPtr->fileNumber);
	     if ((descMapPtr->flags != LFS_DESC_MAP_ALLOCED) || 
		 (descMapPtr->truncVersion != fileSumPtr->truncVersion)) {
		dead = TRUE;
	    } else {
		dead = FALSE;
	    }

	    /*
	     * For each block ... 
	     */
	    blockArray = (int *)(summaryPtr + sizeof(LfsFileLayoutSummary));
	    firstBlock = fileSumPtr->numBlocks -
		 (fileSumPtr->numDataBlocks-1) * FS_BLOCK_SIZE/blockSize;
	    for (i = 0; i < fileSumPtr->numDataBlocks; i++) {
		int addr, blocks;

		addr = startAddress - i*FS_BLOCK_SIZE/blockSize - firstBlock;
		blocks = (i == 0) ? firstBlock : (FS_BLOCK_SIZE/blockSize);
		for (j = 0; j < blocks ; j++) { 
		    if (dead) {
			if (full) { 
			    if (blockInfoArray[addr+j].type != UNUSED) {
				majorErrors++;
				fprintf(stderr,"%s:CheckFileLayoutSummary: Dead block %d of file %d in use at %d, used by %s\n", 
				deviceName,
				blockArray[i], fileSumPtr->fileNumber, addr+j,
				GetOwner(addr + j));
			     }
			 } else {
			     if (Bit_IsSet(addr+j, usageBitMap)) {
				majorErrors++;
				fprintf(stderr,"%s:CheckFileLayoutSummary: Dead block %d of file %d in use at %d\n", 
				deviceName,
				blockArray[i], fileSumPtr->fileNumber, addr+j);
			     }
			 }
		    } else {
			if (full) { 
			    if (!((blockInfoArray[addr+j].type == UNUSED) ||
				((blockInfoArray[addr+j].type == FILE) &&
				(blockInfoArray[addr+j].fileNum == 
					    fileSumPtr->fileNumber) &&
				(blockInfoArray[addr+j].blockNum == blockArray[i])))){
				majorErrors++;
				fprintf(stderr,"%s:CheckFileLayoutSummary: Block %d of file %d in use at %d, used by %s\n", 
				deviceName,
				blockArray[i], fileSumPtr->fileNumber, addr+j,
				GetOwner(addr + j));
			     } else {
				 blockInfoArray[addr+j].found = TRUE;
			     }
			} else {
			    Bit_Set(addr+j, foundBitMap);
			}
		    }
		}
	    }
	    out:
	    startAddress = startAddress - fileSumPtr->numBlocks;
	    offset += fileSumPtr->numBlocks;
	    summaryPtr += sizeof(LfsFileLayoutSummary) + 
				fileSumPtr->numDataBlocks * sizeof(int); 
	    break;
	  }

	case LFS_FILE_LAYOUT_DIR_LOG: {
	    LfsFileLayoutLog	*logSumPtr;
	    int			numBlocks;
	    int			i,j, addr, blocks;
	    LfsDirOpLogBlockHdr *hdrPtr = (LfsDirOpLogBlockHdr *) NIL;

	    /* 
	     * Directory log info is not needed during clean so we 
	     * just skip over it.
	     */
	     logSumPtr = (LfsFileLayoutLog *) summaryPtr;
	     summaryPtr = summaryPtr + sizeof(LfsFileLayoutLog);
	     numBlocks = logSumPtr->numBlocks;
	     addr = startAddress;
	     for (i = 0; i < logSumPtr->numDataBlocks; i++) {
		if (numBlocks > FS_BLOCK_SIZE/blockSize) {
		    blocks = FS_BLOCK_SIZE/blockSize;
		} else {
		    blocks = numBlocks;
		}

	        hdrPtr = (LfsDirOpLogBlockHdr *)
		     SegFetchBlock(segPtr, offset, blocks*blockSize);
		addr -= blocks;
		offset += blocks;
		if (hdrPtr->magic != LFS_DIROP_LOG_MAGIC) {
		    fprintf(stderr,"Bad dir op log magic number.\n");
		}
		if (showDirLog) { 
		    ShowDirLogBlock(hdrPtr, addr);
		}
		for (j = 0; j < blocks; j++) {
		    if (full) { 
			if (blockInfoArray[addr+j].type != UNUSED) {
				majorErrors++;
				fprintf(stderr,"%s:CheckFileLayoutSummary: Log block in use at %d, used by %s\n", 
				deviceName,
				addr+j,
				GetOwner(addr + j));
			}
			blockInfoArray[addr+j].type = DIRLOG;
			blockInfoArray[addr+j].found = TRUE;
		    } else {
			if (Bit_IsSet(addr+j, usageBitMap)) {
				majorErrors++;
				fprintf(stderr,"%s:CheckFileLayoutSummary: Log block in use at %d\n", 
				deviceName,
				addr+j);
			}
			Bit_Set(addr+j, usageBitMap);
			Bit_Set(addr+j, foundBitMap);
		    }
		}
		numBlocks -= blocks;
	     }

	     startAddress = startAddress - logSumPtr->numBlocks;
	     if ( hdrPtr != (LfsDirOpLogBlockHdr *) NIL) { 
		 SegReleaseBlock(segPtr, (char *) hdrPtr);
	    }
	     break;
	}
	case LFS_FILE_LAYOUT_DBL_INDIRECT: 
	case LFS_FILE_LAYOUT_INDIRECT: 
	default: {
	    panic("Unknown type");
	}
      }
    }

}
static char *
GetOwner(blockNum)
    int blockNum;
{
    static char buffer[128];

    sprintf(buffer, "<%d,%d,%d>", blockInfoArray[blockNum].type,
				blockInfoArray[blockNum].fileNum,
				blockInfoArray[blockNum].blockNum);

    return buffer;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintSuperBlock --
 *
 *	Print super block contents.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
PrintSuperBlock(superBlockPtr)
    LfsSuperBlock *superBlockPtr;
{
    printf("SuperBlock.hdr.version: %d\n", superBlockPtr->hdr.version);
    printf("SuperBlock.hdr.blockSize: %d\n", 
			    superBlockPtr->hdr.blockSize);
    printf("SuperBlock.hdr.maxCheckPointBlocks: %d\n", 
			    superBlockPtr->hdr.maxCheckPointBlocks);
    printf("SuperBlock.hdr.checkPointOffset[0]: %d\n", 
		    superBlockPtr->hdr.checkPointOffset[0]);
    printf("SuperBlock.hdr.checkPointOffset[1]: %d\n", 
		    superBlockPtr->hdr.checkPointOffset[1]);
    printf("SuperBlock.hdr.logStartOffset: %d\n", 
		    superBlockPtr->hdr.logStartOffset);
    printf("SuperBlock.hdr.maxNumCacheBlocks: %d\n", 
		    superBlockPtr->hdr.maxNumCacheBlocks);
    printf("SuperBlock.descMap.version: %d\n", superBlockPtr->descMap.version);
    printf("SuperBlock.descMap.maxDesc: %d\n", superBlockPtr->descMap.maxDesc);
    printf("SuperBlock.descMap.stableMem.blockSize: %d\n", 
			superBlockPtr->descMap.stableMem.blockSize);
    printf("SuperBlock.descMap.stableMem.maxNumBlocks: %d\n", 
			superBlockPtr->descMap.stableMem.maxNumBlocks);
    printf("SuperBlock.usageArray.segmentSize: %d\n", 
				superBlockPtr->usageArray.segmentSize);
    printf("SuperBlock.usageArray.numberSegments: %d\n", 
				superBlockPtr->usageArray.numberSegments);
    printf("SuperBlock.usageArray.minNumClean: %d\n", 
				superBlockPtr->usageArray.minNumClean);
    printf("SuperBlock.usageArray.minFreeBlocks: %d\n", 
				superBlockPtr->usageArray.minFreeBlocks);
    printf("SuperBlock.usageArray.stableMem.blockSize: %d\n", 
			superBlockPtr->descMap.stableMem.blockSize);
    printf("SuperBlock.usageArray.stableMem.maxNumBlocks: %d\n", 
			superBlockPtr->descMap.stableMem.maxNumBlocks);
    printf("SuperBlock.fileLayout.descPerBlock: %d\n", 
				superBlockPtr->fileLayout.descPerBlock);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintCheckPointHdr --
 *
 *	Print check point header contents.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
PrintCheckPointHdr(headerPtr, region)
    LfsCheckPointHdr *headerPtr;
    int region;
{
    printf("CheckPointHdr[%d].timestamp: %d\n", region, headerPtr->timestamp);
    printf("CheckPointHdr[%d].size: %d\n", region, headerPtr->size);
    printf("CheckPointHdr[%d].version: %d\n", region, headerPtr->version);
    printf("CheckPointHdr[%d].domainPrefix: %s\n", region, 
					headerPtr->domainPrefix);
    printf("CheckPointHdr[%d].domainNumber: %d\n", region,
				headerPtr->domainNumber);
    printf("CheckPointHdr[%d].attachSeconds: %d\n", region, 
			headerPtr->attachSeconds);
    printf("CheckPointHdr[%d].detachSeconds: %d\n", region, 
			headerPtr->detachSeconds);
    printf("CheckPointHdr[%d].serverID: %d\n", region, 
			headerPtr->serverID);
}

static void
ShowDirLogBlock(hdrPtr, addr)
    LfsDirOpLogBlockHdr *hdrPtr;
    int addr;
{
    LfsDirOpLogEntry *entryPtr, *limitPtr;

    printf("Dirlog block at %d, size %d\n", addr, hdrPtr->size);
    limitPtr = (LfsDirOpLogEntry *) (((char *) hdrPtr) + hdrPtr->size);
    entryPtr = (LfsDirOpLogEntry *) (hdrPtr+1);
    while (entryPtr < limitPtr) {
	printf("LogSeqNum %d opFlags 0x%x dirFile %d dirOffset %d linkCount %d\n",
		entryPtr->hdr.logSeqNum, entryPtr->hdr.opFlags, 
		entryPtr->hdr.dirFileNumber,
		entryPtr->hdr.dirOffset, entryPtr->hdr.linkCount);
	entryPtr->dirEntry.fileName[entryPtr->dirEntry.nameLength] = '\0';
	printf("    File %d Name %s\n", entryPtr->dirEntry.fileNumber, 
		entryPtr->dirEntry.fileName);
	entryPtr = (LfsDirOpLogEntry *) 
		     (((char *)entryPtr) + LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr));
    }

}

static char *
FmtTime()
{
     struct timeval tim;
     time_t  timeVal;
     static char timeBuffer[128];

     if (gettimeofday(&tim, (struct timezone *) NULL)) {
	 perror("gettimeofday");
	 return "Unknown";
     }
     timeVal = tim.tv_sec;
     (void)strcpy(timeBuffer, ctime(&timeVal) + sizeof("Sun Sep 16 ")-1);

     sprintf(timeBuffer+sizeof("01:03:52")-1,".%03d", tim.tv_usec/1000);
     return timeBuffer;
}

static Seg *
SegInit(diskFd, segNumber)
    int	diskFd;
    int	segNumber;	/* Segment number to operate on. */
{
    static Seg seg;
    seg.segNo = segNumber;
    seg.segSizeInBlocks = superBlockPtr->usageArray.segmentSize/blockSize;
    seg.diskFd = diskFd;
    return &seg;
}

static int
SegStartAddr(segPtr)
    Seg	*segPtr;
{
    return  segPtr->segNo * segPtr->segSizeInBlocks + 
			superBlockPtr->hdr.logStartOffset;
}
static char *
SegFetchBlock(segPtr, blockOffset, size)
    Seg		*segPtr;
    int		blockOffset;
    int		size;
{
    char *buf;
    int	  startAddr;

    startAddr = SegStartAddr(segPtr) + segPtr->segSizeInBlocks - blockOffset -
		size/blockSize;
    buf = malloc(size);

    if (DiskRead(segPtr->diskFd, startAddr , size, buf) != size) {
	fprintf(stderr,"%s:SegFetchBlock: Can't read seg %d offset %d.\n", deviceName, segPtr->segNo, blockOffset);
    }
    return buf;
}
static void
SegReleaseBlock(segPtr, memPtr)
    Seg		*segPtr;
    char	*memPtr;
{
    free(memPtr);
}
static void
SegRelease(segPtr)
    Seg		*segPtr;
{
}
