/* 
 * showlfssegment.c --
 *
 *	The showlfssegment program - Show the segment usage of an
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
static char rcsid[] = "$Header: /sprite/src/admin/showlfssegment/RCS/showlfssegment.c,v 1.1 92/03/11 21:54:25 shirriff Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "../lfslib/lfslib.h"
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#endif

#include <sprite.h>
#include <stdio.h>
#include <option.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>


int	blockSize = 512;
int	offset = 64;

char	*deviceName;

Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"This program shows the usage of an LFS file system.\n Synopsis: \"showLfsUsage [switches] deviceName\"\n Command-line switches are:"},
    {OPT_INT, "blockSize", (Address) &blockSize, 
	"Block size of file system."},
    {OPT_INT, "offset", (Address) &offset, 
	"Offset into device to start file system."},
};
/*
 * Forward routine declartions. 
 */

static void PrintSegUsageSummary _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr,
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void PrintDescMapSummary _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr, 
	int startAddress, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void PrintFileLayoutSummary _ARGS_((Lfs *lfsPtr, LfsSeg *segPtr,
	int startAddr, int offset, LfsSegSummaryHdr *segSummaryHdrPtr));
static void ShowDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr));

static char *DirOpFlagsToString _ARGS_((int opFlags));
static char *FileTypeToString _ARGS_((int fileType));

static void PrintSegmentContents _ARGS_((Lfs *lfsPtr, int segNum));
extern void panic();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of mklfs - parse arguments and do the work.
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
    Lfs	*lfsPtr;
    int	segNum;



    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 3) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(1);
	 segNum = 0; /* Lint avoidance. */
    } else {
	deviceName = argv[1];
	segNum = atoi(argv[2]);
    }
    lfsPtr = LfsLoadFileSystem(argv[0], deviceName, blockSize, offset,
			     O_RDONLY);
    if (lfsPtr == (Lfs *) NULL) {
	exit(1);
    }
    if ((segNum < 0 || segNum >= lfsPtr->superBlock.usageArray.numberSegments) &&
	(segNum != -1) ) {
	fprintf(stderr, "%s: Bad segment number %s\n", argv[0], argv[2]);
	exit(1);
    }
    if (segNum == 0) {
	for (segNum = 1; segNum < lfsPtr->superBlock.usageArray.numberSegments;
		segNum++) { 


	    printf("Segment number %d\n", segNum);
	    PrintSegmentContents(lfsPtr, segNum);
	}
    } else {


	printf("Segment number %d\n", segNum);
	PrintSegmentContents(lfsPtr, segNum);
    }

    exit(0);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintSegmentContents --
 *
 *	Print out the contents of an LFS segement.
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
PrintSegmentContents(lfsPtr, segNum)
    Lfs	*lfsPtr;	/* File system. */
    int	segNum;		/* LfsSegment number. */
{
    LfsSeg	*segPtr;
    int startAddr;
    char *summaryLimitPtr, *summaryPtr;
    LfsSegSummary *segSummaryPtr;
    LfsSegSummaryHdr *segSummaryHdrPtr;
    int blockOffset;


    blockOffset = 0;
    segPtr = LfsSegInit(lfsPtr, segNum);
    startAddr = LfsSegStartAddr(segPtr) + LfsSegSizeInBlocks(lfsPtr);
    do { 
	segSummaryPtr = (LfsSegSummary *) 
		    LfsSegFetchBlock(segPtr, blockOffset, blockSize);
	if (segSummaryPtr->size == 0) {
	    LfsSegReleaseBlock(segPtr, (char *) segSummaryPtr);
	    break;
	}
	if (segSummaryPtr->magic != LFS_SEG_SUMMARY_MAGIC) {
	    fprintf(stderr,"%s: Bad magic number 0x%x for summary region of segment %d\n", deviceName, segSummaryPtr->magic, segNum);
	    break;
	}
	printf("Addr %d Summary Time %d PrevSeg %d NextSeg %d bytes %d NextSum %d\n",
	    startAddr-blockOffset,
	    segSummaryPtr->timestamp, segSummaryPtr->prevSeg, 
	    segSummaryPtr->nextSeg,
	    segSummaryPtr->size, segSummaryPtr->nextSummaryBlock);
	blockOffset++;
	summaryLimitPtr = (char *) segSummaryPtr + segSummaryPtr->size;
	summaryPtr = (char *) (segSummaryPtr + 1);
	while (summaryPtr < summaryLimitPtr) {
	   segSummaryHdrPtr = (LfsSegSummaryHdr *) summaryPtr;
	   if (segSummaryHdrPtr->lengthInBytes == 0) {
		break;
	   }
	   switch (segSummaryHdrPtr->moduleType) { 
	   case LFS_SEG_USAGE_MOD:
	      PrintSegUsageSummary(lfsPtr, segPtr, startAddr - blockOffset, blockOffset, 
				segSummaryHdrPtr);
	       break;
	   case LFS_DESC_MAP_MOD:
	      PrintDescMapSummary(lfsPtr, segPtr, startAddr - blockOffset, blockOffset, 
				segSummaryHdrPtr);
	       break;
	   case LFS_FILE_LAYOUT_MOD:
	      PrintFileLayoutSummary(lfsPtr, segPtr,startAddr - blockOffset, blockOffset, 
				segSummaryHdrPtr);
		break;
	   default: {
		fprintf(stderr,"%s:CheckSummary: Unknown module type %d at %d, Size %d Blocks %d\n",
			deviceName, segSummaryHdrPtr->moduleType, startAddr - blockOffset, segSummaryHdrPtr->lengthInBytes, segSummaryHdrPtr->numDataBlocks);
		break;
		}
	   }
	   summaryPtr += segSummaryHdrPtr->lengthInBytes;
	   blockOffset += (segSummaryHdrPtr->numDataBlocks-1);
       }
       if (segSummaryPtr->nextSummaryBlock == -1) { 
	   blockOffset = 0;
       } else {
	   blockOffset = segSummaryPtr->nextSummaryBlock-1;
       }
       LfsSegReleaseBlock(segPtr, (char *) segSummaryPtr);
    } while( blockOffset != 0);
    LfsSegRelease(segPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintSegUsageSummary --
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
PrintSegUsageSummary(lfsPtr, segPtr, startAddress, offset, segSummaryHdrPtr) 
    Lfs	*lfsPtr;
    LfsSeg *segPtr;
    int startAddress;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    int blocks, *blockArray, i, startAddr, fsBlocks;

    fsBlocks = lfsPtr->superBlock.usageArray.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	fprintf(stderr,"%s:PrintSegUsageSummary: Wrong block count; is %d should be %s\n", deviceName, blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > lfsPtr->superBlock.usageArray.stableMem.maxNumBlocks)){
	   fprintf(stderr,"%s:PrintSegUsageSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	printf("Addr %d UsageArray Block %d %s\n", startAddr, blockArray[i],
		(LfsGetUsageArrayBlockIndex(lfsPtr,blockArray[i]) == 
			startAddr) ? "alive" : "dead");
    }

}

/*
 *----------------------------------------------------------------------
 *
 * PrintDescMapSummary --
 *
 *	Check the segment summary regions for the desc map.
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
PrintDescMapSummary(lfsPtr, segPtr, startAddress, offset, segSummaryHdrPtr) 
    Lfs	*lfsPtr;
    LfsSeg *segPtr;
    int startAddress;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    int blocks, *blockArray, i, startAddr, fsBlocks;

    fsBlocks = lfsPtr->superBlock.descMap.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	fprintf(stderr,"%s:PrintDescMapSummary: Wrong block count; is %d should be %s\n", deviceName,blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > lfsPtr->superBlock.descMap.stableMem.maxNumBlocks)){
	   fprintf(stderr,"%s:PrintDescMapSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	printf("Addr %d DescMap Block %d %s\n", startAddr, blockArray[i],
		(LfsGetDescMapBlockIndex(lfsPtr,blockArray[i]) == 
			startAddr) ? "alive" : "dead");
    }

}

/*
 *----------------------------------------------------------------------
 *
 * PrintFileLayoutSummary --
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
PrintFileLayoutSummary(lfsPtr, segPtr, startAddr, offset, segSummaryHdrPtr) 
    Lfs	*lfsPtr;
    LfsSeg  *segPtr;
    int startAddr;
    int offset;
    LfsSegSummaryHdr *segSummaryHdrPtr;
{
    char *summaryPtr, *limitPtr;
    int descMapBlocks;
    int startAddress,  ssize;
    LfsDescMapEntry *entryPtr;
    LfsFile	*filePtr;
    ReturnStatus status;
    Boolean dead;
    int sizeJunk;

    ssize = LfsSegSize(lfsPtr);


    startAddress = startAddr;
    descMapBlocks = lfsPtr->superBlock.descMap.stableMem.blockSize/blockSize;
    summaryPtr = (char *) (segSummaryHdrPtr + 1);
    limitPtr = summaryPtr + segSummaryHdrPtr->lengthInBytes - 
			sizeof(LfsSegSummaryHdr);
    while (summaryPtr < limitPtr) {
	switch (*(unsigned short *) summaryPtr) {
	case LFS_FILE_LAYOUT_DESC: {
	    int		fileNumber;
	    int		slot;
	    LfsFileDescriptor	*descPtr;
	    descPtr = (LfsFileDescriptor *) LfsSegFetchBlock(segPtr, offset, 
						descMapBlocks*blockSize);
	    offset += descMapBlocks;
	    startAddress -= descMapBlocks;
	    for (slot = 0; slot < lfsPtr->superBlock.fileLayout.descPerBlock; 
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
		    (fileNumber >= lfsPtr->superBlock.descMap.maxDesc)) {
		   fprintf(stderr,"%s:PrintFileLayoutSummary: bad file number %d in desc block at %d\n", deviceName, fileNumber, startAddress);
		   continue;
		}
		entryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
		printf("Addr %d Descriptor %s %d len %d %s\n",
			startAddress,
			FileTypeToString(descPtr[slot].common.fileType),
			descPtr[slot].fileNumber,
			descPtr[slot].common.lastByte+1,
			((entryPtr == (LfsDescMapEntry*) NIL) ||
			 (entryPtr->blockAddress != startAddress)) ? "dead" :
			 "alive");
	     }
	    /*
	     * Skip over the summary bytes describing this block. 
	     */
	    summaryPtr += sizeof(LfsFileLayoutDesc);
	    LfsSegReleaseBlock(segPtr, (char *) descPtr);
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
		(fileSumPtr->fileNumber >= lfsPtr->superBlock.descMap.maxDesc)) {
	       fprintf(stderr,"%s:PrintFileLayoutSummary: bad file number %d at %d\n", deviceName, fileSumPtr->fileNumber, startAddress);
	       goto out;
	    }
	    status = LfsFileOpen(lfsPtr, fileSumPtr->fileNumber, &filePtr);
	    if (status != SUCCESS) {
		dead = TRUE;
	    } else {
		dead = FALSE;
	    }

	    printf("Addr %d File %d blocks %d version %d dataBlocks %d %s\n",
		startAddress - fileSumPtr->numBlocks,
		fileSumPtr->fileNumber, fileSumPtr->numBlocks,
		fileSumPtr->truncVersion,
		fileSumPtr->numDataBlocks,
		dead ? "dead" : "alive");

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
		printf("Addr %d File %d block %d %s\n",
			addr, fileSumPtr->fileNumber, blockArray[i],
			(!dead && LfsFileBlockAddr(filePtr,blockArray[i], &sizeJunk) ==  addr) ? "alive" : "dead");
	    }
	    if (!dead) {
		LfsFileClose(filePtr);
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
	    int			i, addr, blocks;
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
		     LfsSegFetchBlock(segPtr, offset, blocks*blockSize);
		addr -= blocks;
		offset += blocks;
		if (hdrPtr->magic != LFS_DIROP_LOG_MAGIC) {
		    fprintf(stderr,"Bad dir op log magic number.\n");
		}
		ShowDirLogBlock(hdrPtr, addr);
		LfsSegReleaseBlock(segPtr, (char *) hdrPtr);
	    }

	    startAddress = startAddress - logSumPtr->numBlocks;
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

static void
ShowDirLogBlock(hdrPtr, addr)
    LfsDirOpLogBlockHdr *hdrPtr;
    int addr;
{
    LfsDirOpLogEntry *entryPtr, *limitPtr;

    limitPtr = (LfsDirOpLogEntry *) (((char *) hdrPtr) + hdrPtr->size);
    entryPtr = (LfsDirOpLogEntry *) (hdrPtr+1);
    while (entryPtr < limitPtr) {
	entryPtr->dirEntry.fileName[entryPtr->dirEntry.nameLength] = '\0';
	printf("Addr %d LogSeqNum %d %s %d \"%s\" links %d in %d at %d\n",
		addr,
		entryPtr->hdr.logSeqNum, 
		DirOpFlagsToString(entryPtr->hdr.opFlags), 
		entryPtr->dirEntry.fileNumber, 
		entryPtr->dirEntry.fileName,
		entryPtr->hdr.linkCount,
		entryPtr->hdr.dirFileNumber,
		entryPtr->hdr.dirOffset);
	entryPtr = (LfsDirOpLogEntry *) 
		     (((char *)entryPtr) + LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr));
    }

}

static char *
DirOpFlagsToString(opFlags)
    int opFlags;
{
    int op = opFlags & FSDM_LOG_OP_MASK;
    char buffer[128];
    static char *opcodes[] = {"UNKNOWN", "CREATE", "UNLINK", "LINK", 
			"RENAME_DELETE", "RENAME_LINK", "RENAME_LINK", 
			"UNKNOWN"};
    if ((op < 0) || (op >= sizeof(opcodes)/sizeof(opcodes[0]))) {
	op = sizeof(opcodes)/sizeof(opcodes[0])-1;
    }
    strcpy(buffer,opcodes[op]);
    if (opFlags & FSDM_LOG_STILL_OPEN) {
	strcat(buffer, "-OPEN");
	opFlags ^= FSDM_LOG_STILL_OPEN;
    }
    if (opFlags & FSDM_LOG_START_ENTRY) {
	strcat(buffer, "-START");
	opFlags ^= FSDM_LOG_START_ENTRY;
    }
    if (opFlags & FSDM_LOG_END_ENTRY) {
	strcat(buffer, "-END");
	opFlags ^= FSDM_LOG_END_ENTRY;
    }
    if (opFlags & FSDM_LOG_IS_DIRECTORY) {
	strcat(buffer, "-DIR");
	opFlags ^= FSDM_LOG_IS_DIRECTORY;
    }
    if (opFlags & ~FSDM_LOG_OP_MASK) {
	strcat(buffer, "-UNKNOWN");
    }
    return buffer;

}
static char *
FileTypeToString(fileType)
    int fileType;
{
    static char *names[] = { "File", "Dir", "Symlink", "Rmtlink", "Dev", 
				 "Rmtdev", "Pipe", "NamedPipe", "Pdev", "Pfs",
				 "XtraFile", "Unknown"};

    if ((fileType < 0) || (fileType >= sizeof(names)/sizeof(names[0]))) {
	fileType = sizeof(names)/sizeof(names[0])-1;
    }
    return names[fileType];
}


