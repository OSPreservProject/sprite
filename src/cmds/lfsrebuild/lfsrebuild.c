/* 
 * lfsrebuild.c --
 *
 *	The lfsrebuild program - Rebuild an LFS file system's metadata 
 *	structures and restore the file system to a consistent state. This
 *	program should be run after metadata corruption is detected in a 
 *	LFS file system. 
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
#include <varargs.h>
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
#include "layout.h"

/*
 * The super block of the file system.
 */
LfsSuperBlock	*superBlockPtr;
int		currentTimestamp;
/*
 * The descriptor map and segment usage array of file system.
 */
ClientData descMapDataPtr;	    /* Descriptor map of file system. */
LfsDescMapCheckPoint *descMapCheckPointPtr; /* Most current descriptor map 
					     * check point. */
LfsSegUsageCheckPoint	*usageCheckPointPtr;
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
    int     found:1;	/* TRUE if found in summary region. */
    int	    type :4;	/* Type of block. See defines below. */
    int     tversion:7; /* Trunc version if file. */
    int     blockNum:20; /* Block number of block's owner. */
    int     fileNum;	/* File number of block's owner. */

} BlockInfo;

#define	SAME_VERSION(version, binfo) \
		(((version) & 0x7f) == ((binfo).tversion)&0x7f)
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

static Boolean computeUsageArray;

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
Boolean shownew = FALSE;
Boolean doWrites = FALSE;

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
    {OPT_TRUE, "shownew", (Address) &shownew, 
	"Print new metadata to standard out."},
    {OPT_TRUE, "write", (Address) &doWrites, 
	"Write new metadata to disk."},
};
/*
 * Forward routine declartions.
 */
extern Boolean AllocUsageArray _ARGS_(());
extern Boolean AllocDescMap _ARGS_(());
extern char *GetUsageState _ARGS_((LfsSegUsageEntry *entryPtr));
static void FindAllFiles _ARGS_((int diskFd));
static Boolean CheckFile _ARGS_((int diskFd, int fileNum, 			
			LfsFileDescriptor *descPtr));
static Boolean CheckIndirectBlock _ARGS_((int diskFd, int fileNum, 
		LfsFileDescriptor *descPtr, int blockNum, int blockAddress));
static Boolean CheckBlock _ARGS_((int diskFd, int fileNum, 
		LfsFileDescriptor *descPtr, int blockNum, int blockAddress));
static void ComputeUsageArray _ARGS_((int diskFd));
static void BuildUsageArray _ARGS_((int diskFd));
static void CheckSummaryRegions _ARGS_((int diskFd));
static void CheckSegUsageSummary _ARGS_((int diskFd, Seg *segPtr, int timestamp,
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void CheckDescMapSummary _ARGS_((int diskFd, Seg *segPtr, int timestamp,
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void CheckFileLayoutSummary _ARGS_((int diskFd, Seg *segPtr,
			int timestamp,
	int startAddr, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static char *GetOwner _ARGS_((int blockNum));
static void PrintSuperBlock _ARGS_((LfsSuperBlock *superBlockPtr));
static void PrintCheckPointHdr _ARGS_((LfsCheckPointHdr *headerPtr, int region));
static ClientData AllocStableMem _ARGS_((LfsStableMemParams *smemParamsPtr,  int type));
static void SetStableMemBlockIndex _ARGS_((ClientData clientData, int blockNum, int address));
static void ShowDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr));

static char *FmtTime _ARGS_((void));
static Seg *SegInit _ARGS_((int diskFd, int segNumber));
static int SegStartAddr _ARGS_((Seg *segPtr));
static char *SegFetchBlock _ARGS_((Seg *segPtr, int blockOffset, int size));
static void SegReleaseBlock _ARGS_((Seg *segPtr, char *memPtr));
static void SegRelease _ARGS_((Seg *segPtr));
static Boolean IsZero _ARGS_((char *dataPtr, int size));

extern int open();
extern void panic();
extern int gettimeofday _ARGS_((struct timeval *tp, struct timezone *tzp));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of lfsrebuild - parse arguments and do the work.
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
    LfsCheckPointHdr	checkPointHdr[2];
    LfsCheckPointHdr    *checkPointHdrPtr = (LfsCheckPointHdr *) NULL;
    char		*checkPointPtr, *trailerPtr = (char *) NULL;
    LfsCheckPointTrailer *trailPtr = (LfsCheckPointTrailer *) NULL;
    Boolean		gotOne;
    int			choosenOne, i, try;
    int			checkPointSize;

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(EXIT_BAD_ARG);
    } else {
	deviceName = argv[1];
    }
    diskFd = open(deviceName, doWrites ? O_RDWR : O_RDONLY, 0);
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
    if (dumpFlag) {
	PrintCheckPointHdr(checkPointHdr+1, 1);
    }

    maxCheckPointSize = superBlockPtr->hdr.maxCheckPointBlocks * 
				blockSize;
    checkPointPtr = malloc(maxCheckPointSize);
    choosenOne = (checkPointHdr[0].timestamp<checkPointHdr[1].timestamp) ?
				1 : 0;
    currentTimestamp = checkPointHdr[choosenOne].timestamp+1;
    numBlocks = superBlockPtr->usageArray.numberSegments * 
		    (superBlockPtr->usageArray.segmentSize/blockSize) +
		     superBlockPtr->hdr.logStartOffset;
    /*
     * Allocate the blockInfoArray for the file system. Zero 
     * marks all blocks as UNUSED.
     */
    blockInfoArray = (BlockInfo *) calloc(numBlocks, sizeof(BlockInfo));
    /*
     * Mark blocks upto log start as checkpoint blocks.
     */
    for (i = 0; i < superBlockPtr->hdr.logStartOffset; i++) {
	    blockInfoArray[i].type = CHECKPOINT;
	    blockInfoArray[i].found = TRUE;
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
    if (oldcp) {
	choosenOne = !choosenOne;
    }
    gotOne = FALSE;
    for (try = 0; (try <= 1) && !gotOne; try++) {
	if (verboseFlag) {
	    printf("%s:Read LFS checkpoint %d from %s\n", FmtTime(), choosenOne,
		    deviceName);
	}
	if (DiskRead(diskFd, superBlockPtr->hdr.checkPointOffset[choosenOne],
		    maxCheckPointSize, checkPointPtr) != maxCheckPointSize) {
	    fprintf(stderr,"%s:Can't read checkPoint %d\n", deviceName, choosenOne);
	    continue;
	}


	checkPointHdrPtr = (LfsCheckPointHdr *) checkPointPtr;
	trailerPtr = (checkPointPtr + checkPointHdrPtr->size - 
				    sizeof(LfsCheckPointTrailer));
	trailPtr = (LfsCheckPointTrailer *) trailerPtr;
	if (trailPtr->timestamp != checkPointHdrPtr->timestamp) {
	    fprintf(stderr,"%s:Header timestamp %d doesn't match trailer timestamp %d on checkpoint %d\n", deviceName, checkPointHdrPtr->timestamp, 
				trailPtr->timestamp, choosenOne);
	    continue;
	}
	if (dumpFlag) {
	    printf("Using checkpoint area %d with timestamp %d domain %d (%s)\n",
		choosenOne, checkPointHdrPtr->timestamp, 
		checkPointHdrPtr->domainNumber, checkPointHdrPtr->domainPrefix);
	}

	gotOne = TRUE;

    }
    if (!gotOne) {
	fprintf(stderr,"%s:Can't find good checkpoint region.\n");
    }
    AllocUsageArray();
    AllocDescMap();
    if (verboseFlag) {
	printf("%s:Checking summary regions\n", FmtTime());
    }
    CheckSummaryRegions(diskFd);

    FindAllFiles(diskFd);
    if (verboseFlag) {
	printf("%s:Checking usage array\n",FmtTime());
    }
    ComputeUsageArray(diskFd);
    BuildUsageArray(diskFd);
    if (verboseFlag) {
	printf("%s:Checking directory tree\n",FmtTime());
    }
    CheckDirTree(diskFd);
    WriteSegCheckpoint(diskFd, checkPointPtr + sizeof(LfsCheckPointHdr),
		&checkPointSize);

    checkPointHdrPtr->timestamp = ++currentTimestamp;
    checkPointHdrPtr->size = checkPointSize + sizeof(LfsCheckPointHdr) + 
			 sizeof(LfsCheckPointTrailer);
    checkPointHdrPtr->version = 1;
    checkPointHdrPtr->detachSeconds = 0;
    trailPtr = (LfsCheckPointTrailer *) 
		(checkPointPtr + checkPointHdrPtr->size - 
				sizeof(LfsCheckPointTrailer));
    trailPtr->timestamp = checkPointHdrPtr->timestamp;
    trailPtr->checkSum = 0;

    /*
     * Append the stats to the checkpoint regions.
     */
    bzero((char *) (trailPtr + 1), sizeof(Lfs_Stats));
    if ( DiskWrite(diskFd, superBlockPtr->hdr.checkPointOffset[0], maxCheckPointSize, (char *) checkPointHdrPtr) != maxCheckPointSize) {
	fprintf(stderr,"Can't write checkpoint region\n");
    }
    exit(EXIT_OK);
    return EXIT_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AllocUsageArray --
 *
 *	Allocate memory for the segment usage array.
 *
 * Results:
 *	TRUE if array can be allocated. FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
AllocUsageArray()
{
    LfsSegUsageParams	*usagePtr;
    LfsStableMemParams  *smemParamsPtr;
    Boolean ret = TRUE;

    usagePtr = &(superBlockPtr->usageArray);
    smemParamsPtr = &(superBlockPtr->usageArray.stableMem);

    usageCheckPointPtr =  (LfsSegUsageCheckPoint *) 
			calloc(1, sizeof(LfsSegUsageCheckPoint));


   usageArrayDataPtr =  AllocStableMem(smemParamsPtr, USAGE_ARRAY);
   return ret;
}


/*
 *----------------------------------------------------------------------
 *
 * AllocDescMap --
 *
 *	Allocate memory for the descriptor map array into memory.
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
AllocDescMap()
{
    LfsStableMemParams  *smemParamsPtr;
    Boolean 		ret = TRUE;

    smemParamsPtr = &(superBlockPtr->descMap.stableMem);

    descMapCheckPointPtr =  (LfsDescMapCheckPoint *) 
			    calloc(1, sizeof(LfsDescMapCheckPoint));

    descMapDataPtr =  AllocStableMem(smemParamsPtr, DESC_MAP);

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


/*
 *----------------------------------------------------------------------
 *
 * DiskWrite --
 *
 *	Write data to the disk.
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
DiskWrite(diskFd, blockOffset, bufferSize, bufferPtr)
    int	diskFd;		/* File descriptor of disk. */
    int	blockOffset;	/* Block offset to start write. */
    char *bufferPtr;	/* Buffer containing data to write */
    int	 bufferSize;	/* Size of buffer. */
{
    int	status;

    if (!doWrites) {
	return bufferSize;
    }
    /*
     * Seek to the start of the blocks to write.
     */
    status = lseek(diskFd, blockOffset*blockSize, L_SET);
    if (status < 0) {
	fprintf(stderr,"%s:", deviceName);
	perror("lseek");
	return status;
    }
    /*
     * Writes the blocks
     */
    status = write(diskFd, bufferPtr, bufferSize);
    if (status != bufferSize) {
	if (status < 0) {
	    fprintf(stderr,"%s:", deviceName);
	    perror("read device");
	    return status;
	}
	fprintf(stderr,"%s:Short read on device %d != %d\n",deviceName,
		status, bufferSize);
    } 
    return status;
}

typedef struct StableMem {
    LfsStableMemParams  *paramsPtr;  /* Parameters of stable memory. */
    LfsStableMemCheckPoint *checkpointPtr;  /* Checkpoint pointer. */
    int			*blockIndexPtr;	    /* Block index of addresses. */
    int			*dirtyBitMap;	    /* Bitmap of dirty blocks. */
    char		*dataPtr;	    /* Data of stablemem */

} StableMem;

/*
 *----------------------------------------------------------------------
 *
 * AllocStableMem --
 *
 *	Allocate a stable memory data structure
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
AllocStableMem(smemParamsPtr, type)
    LfsStableMemParams  *smemParamsPtr;  /* Parameters of stable memory. */
    int		type;
{
    int arraySize;
    int *blockIndexPtr;
    StableMem *stableMemPtr;

    arraySize = smemParamsPtr->blockSize * smemParamsPtr->maxNumBlocks;
    stableMemPtr = (StableMem *) malloc(sizeof(StableMem));
    stableMemPtr->paramsPtr = smemParamsPtr;
    stableMemPtr->checkpointPtr = 
	(LfsStableMemCheckPoint *) calloc(1, sizeof(LfsStableMemCheckPoint));
    blockIndexPtr = (int *) calloc(smemParamsPtr->maxNumBlocks, sizeof(int));
    stableMemPtr->blockIndexPtr = blockIndexPtr;
    Bit_Alloc(smemParamsPtr->maxNumBlocks, stableMemPtr->dirtyBitMap);
    stableMemPtr->dataPtr = calloc(1, arraySize);
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
    Bit_Set(blockNum, stableMemPtr->dirtyBitMap);
    return stableMemPtr->dataPtr + 
        blockNum * stableMemPtr->paramsPtr-> blockSize + offset;
}
static void
SetStableMemBlockIndex(clientData, blockNum, address)
    ClientData clientData;
    int blockNum;
    int address;
{
    StableMem *stableMemPtr = (StableMem *) clientData;

    if ((blockNum < 0) || 
	(blockNum >= stableMemPtr->paramsPtr->maxNumBlocks)) {
	fprintf(stderr,"Bad stable memory block num %d\n", blockNum);
	return;
    }
    stableMemPtr->blockIndexPtr[blockNum] = address;
    return;
}
Boolean
StableMemCheckpoint(clientData, segPtr, checkPointPtr, checkPointSizePtr)
    ClientData clientData;
    LfsSeg	*segPtr;
    char	*checkPointPtr;
    int		*checkPointSizePtr;
{
    StableMem *stableMemPtr = (StableMem *) clientData;
    LfsStableMemBlockHdr *hdrPtr;
    LfsStableMemCheckPoint	checkPoint;
    LfsSegElement	*bufferPtr;
    int blockNum, offset, blockSize, fsBlocks;
    char	*summaryPtr;

    offset = 0;
    blockSize = stableMemPtr->paramsPtr-> blockSize;
    fsBlocks = blockSize/superBlockPtr->hdr.blockSize;

    checkPoint.numBlocks = 0;
    while ( (blockNum = Bit_FindFirstSet(
			   stableMemPtr->paramsPtr->maxNumBlocks,
			   stableMemPtr->dirtyBitMap)) != -1) {
	char *dataPtr = stableMemPtr->dataPtr + blockNum * blockSize;
	if (IsZero(dataPtr, blockSize)) {
	    SetStableMemBlockIndex(clientData,blockNum, FSDM_NIL_INDEX);
	    Bit_Clear(blockNum, stableMemPtr->dirtyBitMap);
	    continue;
	}
	summaryPtr = LfsSegGrowSummary(segPtr, fsBlocks, sizeof(int));
	if (summaryPtr == (char *)NIL) {
	    return TRUE;
	}
	hdrPtr = (LfsStableMemBlockHdr *) dataPtr;
	hdrPtr->magic = LFS_STABLE_MEM_BLOCK_MAGIC;
	hdrPtr->memType = stableMemPtr->paramsPtr->memType;
	hdrPtr->blockNum = blockNum;
	hdrPtr->reserved = 0;

	bufferPtr = LfsSegAddDataBuffer(segPtr, fsBlocks, dataPtr, 
				(ClientData)NIL);
	*(int *)summaryPtr = blockNum;
	LfsSegSetSummaryPtr(segPtr,summaryPtr + sizeof(int));
	SetStableMemBlockIndex(clientData, blockNum, 
				LfsSegDiskAddress(segPtr, bufferPtr));
	segPtr->activeBytes += blockSize;
	Bit_Clear(blockNum, stableMemPtr->dirtyBitMap);
	if (blockNum >= checkPoint.numBlocks) {
	    checkPoint.numBlocks = blockNum+1;
	}
    }
    *(LfsStableMemCheckPoint *) checkPointPtr = checkPoint;
    bcopy((char *) stableMemPtr->blockIndexPtr, 
	    checkPointPtr + sizeof(LfsStableMemCheckPoint), 
	    sizeof(int) * checkPoint.numBlocks);
    *checkPointSizePtr = sizeof(int) * checkPoint.numBlocks + 
			    sizeof(LfsStableMemCheckPoint);

    return FALSE;
}
Boolean
LfsSegUsageCheckpoint(segPtr, checkPointPtr, checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    LfsSegUsageCheckPoint *cp = (LfsSegUsageCheckPoint *) checkPointPtr;
    int		size;
    Boolean	full;

    *cp = *usageCheckPointPtr;
    size = sizeof(LfsSegUsageCheckPoint);

    full = StableMemCheckpoint(usageArrayDataPtr, segPtr, checkPointPtr + size,
		checkPointSizePtr);
    if (!full) { 
	*checkPointSizePtr = (*checkPointSizePtr) + size;
    }
    return full;

}

/*
 *----------------------------------------------------------------------
 *
 * DescMapCheckpoint --
 *
 *	Routine to handle checkpointing of the descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	checkpointed.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsDescMapCheckpoint(segPtr, checkPointPtr, checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    LfsDescMapCheckPoint *cp = (LfsDescMapCheckPoint *) checkPointPtr;
    int		size, dataSize;
    Boolean	full;

    *cp = *descMapCheckPointPtr;
    size = sizeof(LfsDescMapCheckPoint);
    dataSize = 0;
    full = StableMemCheckpoint(descMapDataPtr, segPtr, checkPointPtr + size, 
			&dataSize);
    if (!full) { 
	(*checkPointSizePtr) = dataSize + size;
    }
    return full;

}


/*
 *----------------------------------------------------------------------
 *
 * FindAllFiles --
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
FindAllFiles(diskFd)
    int diskFd;
{
    LfsFileDescriptor	*descPtr;
    char *descBuf;
    LfsDescMapEntry *descMapPtr;
    int bufSize, i, j;

    bufSize = superBlockPtr->fileLayout.descPerBlock * sizeof(*descPtr);

    descBuf = alloca(bufSize);
    descMapPtr = DescMapEntry(0);
    descMapPtr->flags = LFS_DESC_MAP_ALLOCED;
    descMapCheckPointPtr->numAllocDesc++;
    descMapPtr = DescMapEntry(1);
    descMapPtr->flags = LFS_DESC_MAP_ALLOCED;
    descMapCheckPointPtr->numAllocDesc++;
    descInfoArray[0].flags = descInfoArray[1].flags = FD_UNREADABLE;

    for (i = 2; i < superBlockPtr->descMap.maxDesc; i++) {
	if ((descInfoArray[i].flags & FD_ALLOCATED) == 0) {
	    continue;
	}
	if (DiskRead(diskFd, descInfoArray[i].addr, bufSize, descBuf)
			!= bufSize) {
	    majorErrors++;
	    fprintf(stderr,"%s:FindAllFiles: Can't read desc for file %d\n",
				deviceName,i);
	}
	descPtr = (LfsFileDescriptor *)descBuf;
	for (j = 0; j < superBlockPtr->fileLayout.descPerBlock; j++) {
	    if (!(descPtr->common.flags & FSDM_FD_ALLOC)) {
		break;
	    }
	    if (descPtr->common.magic != FSDM_FD_MAGIC) {
		majorErrors++;
		fprintf(stderr,"%s:FindAllFiles: Corrupted descriptor block at %d, magic number 0x%x\n", deviceName, descMapPtr->blockAddress, descPtr->common.magic);
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
	    fprintf(stderr,"%s:FindAllFiles: Can't desc for file %d at %d\n",
			deviceName,i, descMapPtr->blockAddress);
	    descInfoArray[i].flags = FD_UNREADABLE;
	    continue;
	} 
	if (!CheckFile(diskFd, i, descPtr)) {
	    descInfoArray[i].flags = FD_UNREADABLE;
	    continue;
	}
	descMapPtr = DescMapEntry(i);
	descMapPtr->blockAddress = descInfoArray[i].addr;
	descMapPtr->flags = LFS_DESC_MAP_ALLOCED;
	descMapPtr->truncVersion = descInfoArray[i].truncVersion;
	descMapCheckPointPtr->numAllocDesc++;
	descInfoArray[i].origLinkCount = descPtr->common.numLinks;
	if (shownew) {
	    printf("Desc %d blockAddress %d\n", i , descMapPtr->blockAddress);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeUsageArray --
 *
 *	Check all the files in the and build the correct segment usage
 *	array.
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
ComputeUsageArray(diskFd)
    int diskFd;
{
    LfsFileDescriptor	*descPtr;
    char *descBuf;
    LfsDescMapEntry *descMapPtr;
    int bufSize, i, j;

    computeUsageArray = TRUE;

    bufSize = superBlockPtr->fileLayout.descPerBlock * sizeof(*descPtr);
    descBuf = alloca(bufSize);

    for (i = 2; i < superBlockPtr->descMap.maxDesc; i++) {
	descMapPtr = DescMapEntry(i);
	if (!(descMapPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	    continue;
	}
	if (DiskRead(diskFd, descMapPtr->blockAddress, bufSize, descBuf)
			!= bufSize) {
	    majorErrors++;
	    fprintf(stderr,"%s:FindAllFiles: Can't read desc for file %d\n",
				deviceName,i);
	}
	descPtr = (LfsFileDescriptor *)descBuf;
	for (j = 0; j < superBlockPtr->fileLayout.descPerBlock; j++) {
	    if (!(descPtr->common.flags & FSDM_FD_ALLOC)) {
		break;
	    }
	    if (descPtr->common.magic != FSDM_FD_MAGIC) {
		majorErrors++;
		fprintf(stderr,"%s:FindAllFiles: Corrupted descriptor block at %d, magic number 0x%x\n", deviceName, descMapPtr->blockAddress, descPtr->common.magic);
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
	    fprintf(stderr,"%s:FindAllFiles: Can't desc for file %d at %d\n",
			deviceName,i, descMapPtr->blockAddress);
	    continue;
	} 
	CheckFile(diskFd, i, descPtr);
	activeBytesArray[BlockToSegmentNum(descMapPtr->blockAddress)] +=
				    sizeof(LfsFileDescriptor);
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
static Boolean
CheckFile(diskFd, fileNum, descPtr) 
    int diskFd;
    int fileNum;
    LfsFileDescriptor *descPtr;
{
    int i;
    Boolean good;

    for (i = 0; i < FSDM_NUM_DIRECT_BLOCKS; i++) {
	if (descPtr->common.direct[i] != FSDM_NIL_INDEX) {
	    if (i * FS_BLOCK_SIZE > descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName,fileNum, i, descPtr->common.lastByte);
		return FALSE;
		 continue;
	    }
	    good = CheckBlock(diskFd, fileNum, descPtr, i, descPtr->common.direct[i]);
	    if (!good) {
		return FALSE;
	    }
	}
    }
    if (descPtr->common.indirect[0] != FSDM_NIL_INDEX) { 
	    if (FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE > 
			descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
		"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName,fileNum, -1, descPtr->common.lastByte);
		return FALSE;
	    }
	    good = CheckIndirectBlock(diskFd, fileNum, descPtr, -1,
				descPtr->common.indirect[0]);
	    if (!good) {
		return FALSE;
	    }
    }
    if (descPtr->common.indirect[1] != FSDM_NIL_INDEX) { 
	    if (FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE + 
			FS_BLOCK_SIZE * FS_BLOCK_SIZE/4 > 
			descPtr->common.lastByte + 1) {
		majorErrors++;
		fprintf(stderr, 
	"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName, fileNum, -2, descPtr->common.lastByte);
		return FALSE;
	    }
	    good = CheckIndirectBlock(diskFd, fileNum, descPtr, -2, 
				descPtr->common.indirect[1]);
	    if (!good) {
		return FALSE;
	    }
    }
    if (descPtr->common.indirect[2] != FSDM_NIL_INDEX) { 
		majorErrors++;
		fprintf(stderr, 
	"%s:CheckFile: File %d has a non-NIL block %d after lastByte %d.\n",
		    deviceName, fileNum, -3, descPtr->common.lastByte);
	return FALSE;
    }
    return TRUE;
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

static Boolean
CheckIndirectBlock(diskFd, fileNum, descPtr, blockNum, blockAddress)
    int diskFd;
    int fileNum;
    LfsFileDescriptor *descPtr;
    int blockNum;
    int	blockAddress;
{
    int  i;
    int blockPtrs[FS_BLOCK_SIZE/4];
    Boolean good;



    good = CheckBlock(diskFd, fileNum, descPtr, blockNum, blockAddress);
    if (!good) {
	return FALSE;
    }
    if (DiskRead(diskFd, blockAddress, FS_BLOCK_SIZE, (char *)blockPtrs)
		!= FS_BLOCK_SIZE) {
	majorErrors++;
	fprintf(stderr,"%s:CheckIndirectBlock: Can't read block %d of file %d.\n",
				deviceName, blockNum, fileNum);
	return FALSE;

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
		good = CheckBlock(diskFd, fileNum, descPtr,
			   start + i,blockPtrs[i]);
		if (!good) {
		  return FALSE;
		}
	    }
	}
	return TRUE;
    } 
    if (blockNum == -2) {
	for (i = 0; i < FS_BLOCK_SIZE/4; i++) {
	    if (blockPtrs[i] != FSDM_NIL_INDEX) { 
		good = CheckIndirectBlock(diskFd, fileNum, descPtr,
			    - i  - (FSDM_NUM_INDIRECT_BLOCKS+1),blockPtrs[i]);
		if (!good) {
		  return FALSE;
		}
	    }
	}
	return TRUE;
    }
    majorErrors++;
    fprintf(stderr,"%s:CheckIndirectBlock: Bad block number %d for file %d\n",
			deviceName, blockNum, fileNum);
    return FALSE;
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
static Boolean
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
	return FALSE;
    }
    for (j = 0; j < sizeInBlocks; j++) {
	if ((blockInfoArray[blockAddress + j].type != FILE) &&
	    (blockInfoArray[blockAddress + j].fileNum != fileNum) &&
	    (blockInfoArray[blockAddress + j].blockNum != blockNum) &&
	    !SAME_VERSION(descInfoArray[fileNum].truncVersion, blockInfoArray[blockAddress + j])) {
	    majorErrors++;
	    fprintf(stderr, 
	    "%s:CheckFile:file %d block %d duplicate usage of block %d ",
		    deviceName,

			fileNum, blockNum, blockAddress + j);
	    fprintf(stderr,"Previous use at %s\n", 
				GetOwner(blockAddress + j));
	    return FALSE;
	} 
    }
    if (computeUsageArray) { 
	activeBytesArray[BlockToSegmentNum(blockAddress)] 
			+= blockSize*sizeInBlocks;
    }
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * BuildUsageArray --
 *
 *	Build the segment usage array.
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
BuildUsageArray(diskFd)
int diskFd;
{
    int segNo;
    Boolean foundCurrent;
    LfsSegUsageEntry *usageArrayPtr;

    usageCheckPointPtr->freeBlocks = 0;  
    usageCheckPointPtr->numClean = 0;
    usageCheckPointPtr->numDirty = 0;
    usageCheckPointPtr->dirtyActiveBytes = 
			(int) (superBlockPtr->usageArray.segmentSize * .90);
    usageCheckPointPtr->currentSegment = 0;
    usageCheckPointPtr->currentBlockOffset = -1;
    usageCheckPointPtr->curSegActiveBytes = 0;
    usageCheckPointPtr->previousSegment = -1;
    usageCheckPointPtr->cleanSegList = -1;
    foundCurrent = FALSE;
    for (segNo = 0; segNo < superBlockPtr->usageArray.numberSegments; 
	segNo++) {
	usageArrayPtr = UsageArrayEntry(segNo);
	if (activeBytesArray[segNo] == 0) {
	    usageArrayPtr->flags = LFS_SEG_USAGE_CLEAN;
	    usageArrayPtr->activeBytes = usageCheckPointPtr->cleanSegList;
	    usageCheckPointPtr->cleanSegList = segNo;
	    usageCheckPointPtr->numClean++;
	    usageCheckPointPtr->freeBlocks += 
			superBlockPtr->usageArray.segmentSize/blockSize;
	} else if (activeBytesArray[segNo] <
				usageCheckPointPtr->dirtyActiveBytes) {
	    if (!foundCurrent) { 
		usageArrayPtr->activeBytes = 0;
		usageArrayPtr->flags = 0;
		usageCheckPointPtr->currentSegment = segNo;
		usageCheckPointPtr->curSegActiveBytes = 
				activeBytesArray[segNo];

		usageArrayPtr->activeBytes  = 0;
		foundCurrent = TRUE;
	    } else { 
		usageArrayPtr->activeBytes = activeBytesArray[segNo];
		usageArrayPtr->flags = LFS_SEG_USAGE_DIRTY;
		usageCheckPointPtr->numDirty++;
		usageCheckPointPtr->freeBlocks += 
		    (superBlockPtr->usageArray.segmentSize - 
			usageArrayPtr->activeBytes + blockSize - 1)/blockSize;
	    }
	} else {
	    usageArrayPtr->flags = 0;
	    if (!foundCurrent) { 
		usageArrayPtr->activeBytes = 0;
		usageCheckPointPtr->currentSegment = segNo;
		usageCheckPointPtr->curSegActiveBytes = 
				activeBytesArray[segNo];
		foundCurrent = TRUE;
	    } else {
		usageArrayPtr->activeBytes = activeBytesArray[segNo];
		usageCheckPointPtr->freeBlocks += 
		    (superBlockPtr->usageArray.segmentSize - 
			usageArrayPtr->activeBytes + blockSize - 1)/blockSize;
	    }
	}
	if (shownew) {
	    printf("Seg %d flag %d activeBytes %d\n", segNo,
			usageArrayPtr->flags,
			usageArrayPtr->activeBytes);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * LfsGetLogTail --
 *
 *	Get the next available clean blocks to write the log to.
 *
 * Results:
 *	SUCCESS if log space was retrieved. FS_NO_DISK_SPACE if log
 *	space is not available. FS_WOULD_BLOCK if operation of blocked.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsGetLogTail(logRangePtr, startBlockPtr)
    LfsSegLogRange *logRangePtr;  /* Segments numbers returned. */
    int		   *startBlockPtr; /* OUT: Starting offset into segment. */
{
    LfsSegUsageEntry *s;
    int		segNumber;

    if (usageCheckPointPtr->currentBlockOffset != -1) {
	/*
	 * There is still room in the existing segment. Use it.
	 */
	logRangePtr->prevSeg = usageCheckPointPtr->previousSegment;
	logRangePtr->current = usageCheckPointPtr->currentSegment;
	logRangePtr->nextSeg =  usageCheckPointPtr->cleanSegList;
	(*startBlockPtr) = usageCheckPointPtr->currentBlockOffset;
	return SUCCESS;
    }
    /*
     * Need to location a new segment.
     */
    if (usageCheckPointPtr->numClean == 0) {
	fprintf(stderr, "No clean segments to write\n");
	return FS_NO_DISK_SPACE;
    }
    s = UsageArrayEntry( usageCheckPointPtr->currentSegment);
    s->activeBytes = usageCheckPointPtr->curSegActiveBytes;
    usageCheckPointPtr->freeBlocks += 
		    (superBlockPtr->usageArray.segmentSize - 
			s->activeBytes + blockSize - 1)/blockSize;
    if (s->activeBytes <= usageCheckPointPtr->dirtyActiveBytes) {
	s->flags |= LFS_SEG_USAGE_DIRTY;
	usageCheckPointPtr->numDirty++;
    }
    s->timeOfLastWrite = 2;
    segNumber = usageCheckPointPtr->cleanSegList;
    s = UsageArrayEntry(segNumber);
    usageCheckPointPtr->cleanSegList = s->activeBytes;

    logRangePtr->prevSeg = usageCheckPointPtr->previousSegment = usageCheckPointPtr->currentSegment;
    logRangePtr->current = usageCheckPointPtr->currentSegment = segNumber;
    logRangePtr->nextSeg =  usageCheckPointPtr->cleanSegList;

    usageCheckPointPtr->numClean--;
    s->activeBytes = usageCheckPointPtr->curSegActiveBytes = 0;
    s->flags  &= ~LFS_SEG_USAGE_CLEAN;
    (*startBlockPtr) = 0;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsSetLogTail --
 *
 *	Set the next available clean blocks to write the log to.
 *
 * Results:
 *	None
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

void
LfsSetLogTail( logRangePtr, startBlock, activeBytes)
    LfsSegLogRange *logRangePtr;  /* Segments numbers. */
    int	startBlock; /* Starting offset into segment. */
    int	activeBytes;	/* Number of bytes written. */
{
    usageCheckPointPtr->currentBlockOffset = startBlock;
    if (activeBytes > 0) {
	usageCheckPointPtr->curSegActiveBytes += activeBytes;
	usageCheckPointPtr->freeBlocks -= 
		    (activeBytes + blockSize - 1)/blockSize;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * LfsSegUsageCheckpointUpdate --
 *
 *	This routine is used to update fields of the seg usage 
 *	checkpoint that change when the checkpoint itself is 
 *	written to the log.
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
LfsSegUsageCheckpointUpdate(checkPointPtr, size)
    char *checkPointPtr; /* Checkpoint region for SegUsage. */
    int	 size;		 /* Size of checkpoint region. */
{

    (*(LfsSegUsageCheckPoint *) checkPointPtr) = *usageCheckPointPtr;
    return;
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
	    if (segSummaryPtr->timestamp >= currentTimestamp) {
		currentTimestamp = segSummaryPtr->timestamp + 1;
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
		  CheckSegUsageSummary(diskFd, segPtr, 
				segSummaryPtr->timestamp, 
				startAddr - blockOffset, 
				blockOffset, segSummaryHdrPtr);
		   break;
	       case LFS_DESC_MAP_MOD:
		  CheckDescMapSummary(diskFd, segPtr, 
				segSummaryPtr->timestamp, 
				startAddr - blockOffset, blockOffset, 
				segSummaryHdrPtr);
		   break;
	       case LFS_FILE_LAYOUT_MOD:
		  CheckFileLayoutSummary(diskFd, segPtr, 
				segSummaryPtr->timestamp, 
				startAddr - blockOffset, blockOffset, 
				 segSummaryHdrPtr);
		    break;
	       default: {
		    fprintf(stderr,"%s:CheckSummary: Unknown module type %d at %d, Size %d Blocks %d\n",
			    deviceName, segSummaryHdrPtr->moduleType,
			    startAddr - blockOffset, 
			    segSummaryHdrPtr->lengthInBytes
			   , segSummaryHdrPtr->numDataBlocks);
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
CheckSegUsageSummary(diskFd, segPtr, timestamp,startAddress, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg *segPtr;
    int timestamp;
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
	    blockInfoArray[startAddr + j].type = USAGE_ARRAY;
	    blockInfoArray[startAddr + j].blockNum = blockArray[i];
	    blockInfoArray[startAddr + j].found = TRUE;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckDescMapSummary --
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
CheckDescMapSummary(diskFd, segPtr, timestamp,startAddress, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg *segPtr;
    int timestamp;
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
	    blockInfoArray[startAddr + j].type = DESC_MAP;
	    blockInfoArray[startAddr + j].blockNum = blockArray[i];
	    blockInfoArray[startAddr + j].found = TRUE;
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
CheckFileLayoutSummary(diskFd, segPtr, timestamp, startAddr, offset, segSummaryHdrPtr) 
    int diskFd;
    Seg  *segPtr;
    int timestamp;
    int startAddr;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    char *summaryPtr, *limitPtr;
    int descMapBlocks;
    int startAddress, j, ssize;

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
		blockInfoArray[addr].type = DESC;
		if (timestamp >= descInfoArray[fileNumber].timestamp) {
		    descInfoArray[fileNumber].timestamp = timestamp;
		    descInfoArray[fileNumber].flags = FD_ALLOCATED;
		    if (descPtr[slot].common.fileType == FS_DIRECTORY) {
			descInfoArray[fileNumber].flags |= IS_A_DIRECTORY;
		    }
		    descInfoArray[fileNumber].addr = startAddress;
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
	    if (fileSumPtr->truncVersion > 
			descInfoArray[fileSumPtr->fileNumber].truncVersion) {
		descInfoArray[fileSumPtr->fileNumber].truncVersion = 
				fileSumPtr->truncVersion;
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
		    blockInfoArray[addr+j].type = FILE;
		    blockInfoArray[addr+j].fileNum = fileSumPtr->fileNumber;
		    blockInfoArray[addr+j].blockNum = blockArray[i];
		    blockInfoArray[addr+j].found = TRUE;
		    blockInfoArray[startAddr + j].tversion = 
				fileSumPtr->truncVersion;
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
		    blockInfoArray[addr+j].type = DIRLOG;
		    blockInfoArray[addr+j].found = TRUE;
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

static Boolean 
IsZero(dataPtr, size)
    register char *dataPtr;
    register int	size;
{
    for (; size > 0; dataPtr++, size--) {
	if (*dataPtr) {
		return FALSE;
	}
    }
    return TRUE;

}

