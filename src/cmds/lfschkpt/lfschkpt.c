/* 
 * lfschkpt.c --
 *
 *	The lfschkpt program - A program to display and edit the the
 *	superblock and checkpoint header info of an LFS file system.
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
static char rcsid[] = "$Header: /sprite/src/admin/lfschkpt/RCS/lfschkpt.c,v 1.1 91/05/31 13:01:22 mendel Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"

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
#include <unistd.h>
#include <bstring.h>

/*
 * NOCHANGE - No change requested in attributes.
 */

#define NOCHANGE	(-2)

int	maxNumCacheBlocks = NOCHANGE;
int	checkpointInterval = NOCHANGE;
int	domainNumber = NOCHANGE;
int	serverID = NOCHANGE;
int	minNumClean = NOCHANGE;
int	minFreeBlocks = NOCHANGE;
int	wasteBlocks = NOCHANGE;
int	numSegsToClean = NOCHANGE;

Boolean	bothFlag = FALSE;
Boolean	writeFlag = FALSE;

int	blockSize = 512;		/* Block size of file system. */

char	*deviceName;

Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"This program displays and allows for changing fields of the superblock\n and checkpoint headers of LFS file systems.\n Synopsis: \"lfschkpt [switches] deviceName\"\n Command-line switches are: (-2 means don't change)"},
    {OPT_INT, "maxNumCacheBlocks", (Address) &maxNumCacheBlocks, 
		"Max number of file cache blocks to used during cleaning."},
    {OPT_INT, "checkpointInterval", (Address) &checkpointInterval, 
		"Frequency of checkpoint in seconds."},
    {OPT_INT, "domainNumber", (Address) &domainNumber, 
		"Domain number of file system."},
    {OPT_INT, "serverID", (Address) &serverID, 
		"Sprite ID of server for this disk."},
    {OPT_INT, "minNumClean", (Address) &minNumClean, 
		"Min number of clean segments to maintain in system."},
    {OPT_INT, "minFreeBlocks", (Address) &minFreeBlocks, 
		"Min number of free blocks to maintain on disk."},
    {OPT_INT, "wasteBlocks", (Address) &wasteBlocks, 
		"Maximum number of blocks to waste in a segment."},
    {OPT_INT, "numSegsToClean", (Address) &numSegsToClean, 
		"Num segments above minNumClean before cleaning stops."},
    {OPT_TRUE, "both", (char *) &bothFlag, 
		"Show both checkpoint regions"},
    {OPT_TRUE, "write", (char *) &writeFlag, 
		"Apply the specified changes to the disk without asking."},
};
/*
 * Forward routine declartions. 
 */

extern void panic();
static void PrintSuperBlock _ARGS_((LfsSuperBlock *superBlockPtr));
static void PrintCheckPointHdr _ARGS_((LfsCheckPointHdr *headerPtr, int region));
extern int Read _ARGS_((int diskFd, int blockOffset, int bufferSize, 
			char *bufferPtr));

extern int Write _ARGS_((int diskFd, int blockOffset, int bufferSize, 
			char *bufferPtr));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of lfschkpt - parse arguments and do the work.
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
    int	   diskFd, cpHdrBlocks, changed, choose, cpSize;
    LfsCheckPointHdr	*checkPointHdr[2];
    LfsSuperBlock *superBlockPtr;


    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(1);
    } else {
	deviceName = argv[1];
    }
    diskFd = open(deviceName, writeFlag ? O_RDWR : O_RDONLY, 0);
    if (diskFd < 0) {
	fprintf(stderr,"%s:", argv[0]);
	perror(deviceName);
	exit(-1);
    }
    superBlockPtr = (LfsSuperBlock *) malloc(LFS_SUPER_BLOCK_SIZE);
    if (superBlockPtr == (LfsSuperBlock *)  NULL) {
	fprintf(stderr,"%s:Can't malloc memory for superblock\n", deviceName);
	exit(1);
    }
    if (Read(diskFd, LFS_SUPER_BLOCK_OFFSET, LFS_SUPER_BLOCK_SIZE, 
		(char *)superBlockPtr) != LFS_SUPER_BLOCK_SIZE) {
	fprintf(stderr,"%s:Can't read superblock.\n", deviceName);
	exit(-1);

    }
    if (superBlockPtr->hdr.magic != LFS_SUPER_BLOCK_MAGIC) {
	fprintf(stderr,"%s:Bad magic number for filesystem\n", deviceName);
	exit(-1);
    }
    PrintSuperBlock(superBlockPtr);

    cpHdrBlocks = (sizeof(LfsCheckPointHdr) + blockSize - 1)/blockSize;
    cpSize = blockSize * cpHdrBlocks;
    checkPointHdr[0] = (LfsCheckPointHdr *) malloc(cpSize);
    checkPointHdr[1] = (LfsCheckPointHdr *) malloc(cpSize);
    if ((checkPointHdr[0] == (LfsCheckPointHdr *) NULL) ||
	(checkPointHdr[1] == (LfsCheckPointHdr *) NULL)) {
	fprintf(stderr,"%s:Can't malloc memory for checkpoints\n", deviceName);
	exit(1);
    }

    /*
     * Examine the two checkpoint areas to locate the checkpoint area with the
     * newest timestamp.
     */
    if (Read(diskFd, superBlockPtr->hdr.checkPointOffset[0], cpSize,
		 (char *) (checkPointHdr[0])) != cpSize) {
	fprintf(stderr,"%s:Can't read checkPointHeader 0.\n", deviceName);
	free((char *) checkPointHdr[0]);
	checkPointHdr[0] = (LfsCheckPointHdr *) NULL;
    }
    if (Read(diskFd, superBlockPtr->hdr.checkPointOffset[1], cpSize,
		 (char *) (checkPointHdr[1])) != cpSize) {
	fprintf(stderr,"%s:Can't read checkPointHeader 1.\n", deviceName);
	free((char *) checkPointHdr[1]);
	checkPointHdr[1] = (LfsCheckPointHdr *) NULL;
    }
    if ((checkPointHdr[0] == (LfsCheckPointHdr *) NULL) &&
	(checkPointHdr[1] == (LfsCheckPointHdr *) NULL)) {
	fprintf(stderr,"%s:Can't read either checkPointHeader.\n", deviceName);
	exit(-1);
    }
    if ((checkPointHdr[0] == (LfsCheckPointHdr *) NULL) ||
	(checkPointHdr[1]->timestamp > checkPointHdr[0]->timestamp)) {
	choose = 1;
    } else {
	choose = 0;
    }
    PrintCheckPointHdr(checkPointHdr[choose], choose);
    if (bothFlag) {
	 PrintCheckPointHdr(checkPointHdr[!choose], !choose);
    }

#define	CHANGEIT(newval, name, oldval, flag)	\
	if ((newval) != NOCHANGE) { \
		printf("Changing %s from %d to %d\n",(name),(oldval),(newval));\
		(oldval) = (newval); \
		(flag) = TRUE; \
	}

    changed = FALSE;

    CHANGEIT(maxNumCacheBlocks, "maxNumCacheBlocks", 
		superBlockPtr->hdr.maxNumCacheBlocks, changed);
    CHANGEIT(checkpointInterval, "checkpointInterval", 
		superBlockPtr->hdr.checkpointInterval, changed);


    CHANGEIT(minNumClean, "minNumClean", 
		superBlockPtr->usageArray.minNumClean, changed);
    CHANGEIT(minFreeBlocks, "minFreeBlocks", 
		superBlockPtr->usageArray.minFreeBlocks, changed);
    CHANGEIT(wasteBlocks, "wasteBlocks", 
		superBlockPtr->usageArray.wasteBlocks, changed);
    CHANGEIT(numSegsToClean, "numSegsToClean", 
		superBlockPtr->usageArray.numSegsToClean, changed);

    if (changed) { 
	if(!writeFlag) {
	    printf("CHANGES NOT APPLIED TO DISK - Use -write option\n");
	    changed = FALSE;
	}
    }

    if (changed) {
        if (Write(diskFd, LFS_SUPER_BLOCK_OFFSET, LFS_SUPER_BLOCK_SIZE, 
		(char *)superBlockPtr) != LFS_SUPER_BLOCK_SIZE) {
	    fprintf(stderr,"%s:Can't rewrite superblock.\n", deviceName);
	    exit(-1);
	}
    }

    changed = FALSE;
    CHANGEIT(domainNumber, "domainNumber", 
		checkPointHdr[choose]->domainNumber, changed);
    CHANGEIT(serverID, "serverID", 
		checkPointHdr[choose]->serverID, changed);

    if (changed) { 
	if(!writeFlag) {
	    printf("CHANGES NOT APPLIED TO DISK - Use -write option\n");
	    changed = FALSE;
	}
    }
    if (changed) {
	if (Write(diskFd, superBlockPtr->hdr.checkPointOffset[choose], cpSize,
		     (char *) (checkPointHdr[choose])) != cpSize) {
	    fprintf(stderr,"%s:Can't write checkPointHeader %d.\n", deviceName,
					choose);
	}
	exit(-1);
    }

    exit(0);
    return 0;
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
    printf("SuperBlock.hdr.checkpointInterval: %d\n", 
		    superBlockPtr->hdr.checkpointInterval);
    printf("SuperBlock.hdr.maxNumCacheBlocks: %d\n", 
		    superBlockPtr->hdr.maxNumCacheBlocks);
    printf("SuperBlock.descMap.version: %d\n", superBlockPtr->descMap.version);
    printf("SuperBlock.descMap.maxDesc: %d\n", superBlockPtr->descMap.maxDesc);
    printf("SuperBlock.descMap.stableMem.blockSize: %d\n", 
			superBlockPtr->descMap.stableMem.blockSize);
    printf("SuperBlock.descMap.stableMem.entrySize: %d\n", 
			superBlockPtr->descMap.stableMem.entrySize);
    printf("SuperBlock.descMap.stableMem.maxNumEntries: %d\n", 
			superBlockPtr->descMap.stableMem.maxNumEntries);
    printf("SuperBlock.descMap.stableMem.entriesPerBlock: %d\n", 
			superBlockPtr->descMap.stableMem.entriesPerBlock);
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
    printf("SuperBlock.usageArray.wasteBlocks: %d\n", 
				superBlockPtr->usageArray.wasteBlocks);
    printf("SuperBlock.usageArray.numSegsToClean: %d\n", 
				superBlockPtr->usageArray.numSegsToClean);
    printf("SuperBlock.usageArray.stableMem.blockSize: %d\n", 
			superBlockPtr->usageArray.stableMem.blockSize);
    printf("SuperBlock.usageArray.stableMem.entrySize: %d\n", 
			superBlockPtr->usageArray.stableMem.entrySize);
    printf("SuperBlock.usageArray.stableMem.maxNumEntries: %d\n", 
			superBlockPtr->usageArray.stableMem.maxNumEntries);
    printf("SuperBlock.usageArray.stableMem.entriesPerBlock: %d\n", 
			superBlockPtr->usageArray.stableMem.entriesPerBlock);
    printf("SuperBlock.usageArray.stableMem.maxNumBlocks: %d\n", 
			superBlockPtr->usageArray.stableMem.maxNumBlocks);
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


/*
 *----------------------------------------------------------------------
 *
 * Read --
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
Read(diskFd, blockOffset, bufferSize, bufferPtr)
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
 * Write --
 *
 *	Wrote data to disk.
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
Write(diskFd, blockOffset, bufferSize, bufferPtr)
    int	diskFd;		/* File descriptor of disk. */
    int	blockOffset;	/* Block offset to start write. */
    char *bufferPtr;	/* Buffer to read data from. */
    int	 bufferSize;	/* Size of buffer. */
{
    int	status;


    /*
     * Seek to the start of the blocks to read.
     */
    status = lseek(diskFd, blockOffset*blockSize, L_SET);
    if (status < 0) {
	fprintf(stderr,"%s:", deviceName);
	perror("lseek");
	return status;
    }
    status = write(diskFd, bufferPtr, bufferSize);
    if (status != bufferSize) {
	if (status < 0) {
	    fprintf(stderr,"%s:", deviceName);
	    perror("write device");
	    return status;
	}
	fprintf(stderr,"%s:Short write on device %d != %d\n",deviceName,
		status, bufferSize);
    } 
    return status;
}

