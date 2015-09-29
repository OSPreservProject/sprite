/* 
 * lfsquickcheck.c --
 *
 *	The lfsquickcheck program - To a quick check of an LFS file system 
 *	to make sure it	is consistent.
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


struct	ActiveDesc {
	int	fileNumber;
	LfsDescMapEntry *descMapPtr;
} *activeDesc;

int		numBlocks;	  /* Number of blocks in fs. */


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
static char *GetOwner _ARGS_((int blockNum));
static void PrintSuperBlock _ARGS_((LfsSuperBlock *superBlockPtr));
static void PrintCheckPointHdr _ARGS_((LfsCheckPointHdr *headerPtr, int region));
static ClientData LoadStableMem _ARGS_((int diskFd, LfsStableMemParams *smemParamsPtr, LfsStableMemCheckPoint *cpPtr));
static int GetStableMemBlockIndex _ARGS_((ClientData clientData, int blockNum));
static void ShowDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr));

static char *FmtTime _ARGS_((void));

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
    int			choosenOne;



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
	exit(EXIT_READ_FAILURE);
    }
    if (dumpFlag) {
	PrintCheckPointHdr(checkPointHdr, 0);
    }
    if (DiskRead(diskFd, superBlockPtr->hdr.checkPointOffset[1],
		sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+1))  != 
	sizeof(LfsCheckPointHdr)) {
	fprintf(stderr,"%s:Can't read checkPointHeader 1.\n", deviceName);
	exit(EXIT_READ_FAILURE);
    }
    if (dumpFlag) {
	PrintCheckPointHdr(checkPointHdr+1, 1);
    }

    choosenOne = (checkPointHdr[0].timestamp<checkPointHdr[1].timestamp) ?
				1 : 0;

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
	printf("%s:Reporting errors\n",FmtTime());
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

	printf("FreeBlocks %d (%3.1f%%) dirtyActiveBytes %d (%3.1f%%) currentSegment %d\n",
	cp->freeBlocks, 
	100.0*cp->freeBlocks/ (double) numBlocks,
	cp->dirtyActiveBytes, 
	100.0*cp->dirtyActiveBytes/(double)usagePtr->segmentSize,
	cp->currentSegment);
    }

    usageArrayDataPtr =  LoadStableMem(diskFd, smemParamsPtr, cpPtr);
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

    descMapDataPtr =  LoadStableMem(diskFd, smemParamsPtr, cpPtr);

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
    activeDesc = (struct ActiveDesc *) 
		malloc(descMapCheckPointPtr->numAllocDesc * 
			sizeof(activeDesc[0]));
    numAlloced = 0;
    for (i = 0; i < descMapParamsPtr->maxDesc; i++) {
	descMapPtr = DescMapEntry(i);
	if (descMapPtr->flags == LFS_DESC_MAP_ALLOCED) {
	    if (numAlloced < descMapCheckPointPtr->numAllocDesc) { 
		activeDesc[numAlloced].fileNumber = i;
		activeDesc[numAlloced].descMapPtr = descMapPtr;
	    }
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
	descMapCheckPointPtr->numAllocDesc = numAlloced;

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
LoadStableMem(diskFd, smemParamsPtr, cpPtr)
    int diskFd;	/* Disk of file system. */
    LfsStableMemParams  *smemParamsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *cpPtr;  /* Checkpoint pointer. */
{
    int arraySize;
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
		fprintf(stderr, "%s:Bad stable mem header for memType %d blockNum %d\n"
				, deviceName, smemParamsPtr->memType, blockNum);
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
int comparProc(aPtr, bPtr)
    struct ActiveDesc *aPtr, *bPtr;
{
    return aPtr->descMapPtr->blockAddress - bPtr->descMapPtr->blockAddress;
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
    int bufSize, i, j, d, segNum;
    LfsSegUsageEntry		*entryPtr;

    int bufLoc;

    bufSize = superBlockPtr->fileLayout.descPerBlock * sizeof(*descPtr);

    descBuf = alloca(bufSize);
    bufLoc = -1;

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
    printf("%s\n", FmtTime());
    qsort((char *) activeDesc,  descMapCheckPointPtr->numAllocDesc,
		sizeof(activeDesc[0]), comparProc);
    printf("%s\n", FmtTime());


    for (d = 0; d < descMapCheckPointPtr->numAllocDesc; d++) {
        descMapPtr = activeDesc[d].descMapPtr;
	i = activeDesc[d].fileNumber;
	if ((descMapPtr->flags != LFS_DESC_MAP_ALLOCED) || (i < 3)) {
	    continue;
	}
	if ((descMapPtr->blockAddress < 0) || 
	    (descMapPtr->blockAddress > numBlocks)) {
	    majorErrors++;
	   fprintf(stderr, "%s:CheckAllFiles: Desc %d address out of range %d\n",
			deviceName, i, descMapPtr->blockAddress);
	    continue;
	}
	if (bufLoc != descMapPtr->blockAddress) { 
	    if (DiskRead(diskFd, descMapPtr->blockAddress, bufSize, descBuf)
			    != bufSize) {
		majorErrors++;
		fprintf(stderr,"%s:CheckAllFiles: Can't read desc for file %d\n",
				    deviceName,i);
	    }
	    bufLoc = descMapPtr->blockAddress;
	}
	segNum = BlockToSegmentNum(descMapPtr->blockAddress);
	entryPtr = (LfsSegUsageEntry *) UsageArrayEntry(segNum);
	if (entryPtr->flags & LFS_SEG_USAGE_CLEAN) {
	    majorErrors++;
	   fprintf(stderr, "%s:CheckAllFiles: Desc %d address in clean segment %d (%d)\n",
			deviceName, i, segNum, descMapPtr->blockAddress);
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
	    continue;
	} 
	if (descPtr->common.fileType == FS_DIRECTORY) {
	}
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

