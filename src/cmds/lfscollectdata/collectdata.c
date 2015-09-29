/* 
 * lfscollectdata.c --
 *
 *	The collectdata program - Collect the data from an LFS file system. 
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


/*
 * Arguments.
 */
int	blockSize = 512;	/* File system block size. */
char	*deviceName;		/* Device to use. */
Boolean	printzero = FALSE;		/* Print zero numbers. */
char    *outFileName = "-";	/* Output file name. */

Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"Dump the statistic counters from an LFS file system.\n Synopsis: \"lfscollectdata [switches] deviceName\"\n Command-line switches are:"},
    {OPT_TRUE, "printzero", (Address) &printzero, 
	"Do output numbers that are zero."},
    {OPT_TRUE, "o", (Address) &outFileName, 
	"Path name of output file."},
};

static void PrintData _ARGS_((Lfs *lfsPtr, FILE *outFile));
static void ShowCheckpointHdr _ARGS_((Lfs *lfsPtr, FILE *outFile));
static void ShowUsageArray _ARGS_((Lfs *lfsPtr, FILE *outFile));
static void ShowStats _ARGS_((Lfs_Stats *statsPtr, FILE *outFile));

extern int gettimeofday _ARGS_((struct timeval *tp, struct timezone *tzp));



/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of lfscollectdata - parse arguments and do the work.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Exits.
 *
 *----------------------------------------------------------------------
 */

int
main(argc,argv)
    int	argc;
    char *argv[];
{
    Lfs		*lfsPtr;
    FILE	*outFile;
    int		err;

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(1);
    } else {
	deviceName = argv[1];
    }
    if ((outFileName[0] == '-') && (outFileName[1] == 0)) {
	outFile = stdout;
    } else {
	outFile = fopen(outFileName, "w");
	if (outFile == (FILE *) NULL) {
	    perror("open");
	    exit(1);
	}
    }
    lfsPtr = LfsLoadFileSystem(argv[0], deviceName, blockSize, 
			LFS_SUPER_BLOCK_OFFSET,
				O_RDONLY);
    if (lfsPtr == (Lfs *) NULL) {
	exit(1);
    }
    PrintData(lfsPtr, outFile);
    err = fflush(outFile);
    if (err) { 
	fprintf(stderr,"%s: Error writing to file.\n", argv[0]);
    }
    exit((err == 0) ? 0 : 1);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintData --
 *
 *	Print the interesting data from a file system.
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
PrintData(lfsPtr, outFile)
    Lfs	*lfsPtr;
    FILE *outFile;
{
    struct timeval tim;

    gettimeofday(&tim, (struct timezone *) NULL);
    ShowCheckpointHdr(lfsPtr, outFile); 
    ShowUsageArray(lfsPtr, outFile);
    ShowStats(&lfsPtr->stats, outFile);
    fprintf(outFile, "gettimeofday.tv_sec %u\n", tim.tv_sec);
    fprintf(outFile, "gettimeofday.tv_usec %u\n", tim.tv_usec);
}

/*
 *----------------------------------------------------------------------
 *
 * ShowCheckpointHdr --
 *
 *	Output the contents of the current checkpoint header of a filesystem.
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
ShowCheckpointHdr(lfsPtr, outFile)
    Lfs	*lfsPtr;
    FILE *outFile;
{
    LfsCheckPointHdr	*hdrPtr;

    hdrPtr = lfsPtr->checkPointHdrPtr;

#ifdef __STDC__
#define	M(f)	if (printzero || hdrPtr-> f ) fprintf(outFile, "LfsCheckPointHdr." #f " %u\n", hdrPtr-> f )
#else
#define	M(f)	if (printzero || hdrPtr-> f ) fprintf(outFile, "LfsCheckPointHdr.f %u\n", hdrPtr-> f )
#endif
    M(timestamp);
    M(version);
    fprintf(outFile, "LfsCheckPointHdr.domainPrefix %s\n", hdrPtr->domainPrefix);
    M(domainNumber);
    M(attachSeconds);
    M(detachSeconds);
    M(serverID);
#undef M
}


/*
 *----------------------------------------------------------------------
 *
 * ShowUsageArray --
 *
 *	Output the contents of the segment usage array.
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
ShowUsageArray(lfsPtr, outFile)
    Lfs	*lfsPtr;
    FILE *outFile;
{
    LfsSegUsageParams	*usagePtr;
    LfsSegUsageCheckPoint	*cp;
    LfsSegUsageEntry		*entryPtr;
    int	i;

    usagePtr = &(lfsPtr->superBlock.usageArray);
    cp = &lfsPtr->usageArray.checkPoint;

#ifdef __STDC__
#define	M(f)	if (printzero || cp-> f ) fprintf(outFile, "LfsSegUsageCheckPoint." #f " %u\n", cp-> f )
#else
#define	M(f)	if (printzero || cp-> f ) fprintf(outFile, "LfsSegUsageCheckPoint.f %u\n", cp-> f )
#endif
    M(freeBlocks);
    M(numClean);
    M(numDirty);
    M(dirtyActiveBytes);
    M(currentSegment);
    M(currentBlockOffset);
    M(curSegActiveBytes);
    M(previousSegment);
    M(cleanSegList);
#undef M
    for (i = 0; i < usagePtr->numberSegments; i++) {
	entryPtr = LfsGetUsageArrayEntry(lfsPtr, i);
	fprintf(outFile, 
	   "LfsSegUsageEntry.Seg %d activeBytes %d lastWrite %u flags %u\n", 
		i, entryPtr->activeBytes, entryPtr->timeOfLastWrite, 
		entryPtr->flags);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ShowStats --
 *
 *	Output the stats structure of the file system..
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
ShowStats(statsPtr, outFile)
    Lfs_Stats *statsPtr;
    FILE *outFile;
{
    int	i;
#include "statprint.h"
    for (i = 0; i < LFS_STATS_CDIST_BUCKETS; i++) {
	if (printzero || statsPtr->cleaningDist[i]) 
	    fprintf(outFile,"Lfs_Stats.cleaningDist %d %u\n",i, 
			statsPtr->cleaningDist[i]);
    }
}

