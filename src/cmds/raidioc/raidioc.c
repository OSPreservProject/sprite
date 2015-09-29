/* 
 * raidioc.c --
 *
 *	Initiates RAID IO-control operations.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.2 89/01/07 04:12:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sys/file.h>
#include <stdio.h>
#include "dev/raid.h"

#define OUT_BUF_SIZE 256
#define IN_BUF_SIZE 4096

char		inBuf[IN_BUF_SIZE];
RaidIOCParam	*IOCParam = (RaidIOCParam *) inBuf;
char		 outBuf[OUT_BUF_SIZE]; /* buffer for messages returned by IOC */
char		*cmdName;


/*
 *----------------------------------------------------------------------
 *
 * PrintUsage --
 *
 *	Prints correct usage of command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints correct usage.
 *
 *----------------------------------------------------------------------
 */

void
PrintUsage()
{
    printf("Usage:\n");
    printf("\t%s <dev> print\n", cmdName);
    printf("\t%s <dev> reconfig configfile\n", cmdName);
    printf("\t%s <dev> lock\n", cmdName);
    printf("\t%s <dev> unlock\n", cmdName);
    printf("\t%s <dev> enable\n", cmdName);
    printf("\t%s <dev> disable\n", cmdName);
    printf("\t%s <dev> savestate\n", cmdName);
    printf("\t%s <dev> enablelog\n", cmdName);
    printf("\t%s <dev> disablelog\n", cmdName);
    printf("\t%s <dev> hardinit <startStripe> <numStripe> [<uSec>]\n",cmdName);
    printf("\t%s <dev> paritycheck <startStripe> <numStripe> [<uSec>]\n",cmdName);
    printf("\t%s <dev> fail <row> <col>\n", cmdName);
    printf("\t%s <dev> replace <row> <col> <type> <unit>\n", cmdName);
    printf("\t%s <dev> reconstruct <row> <col> [<numSector> <uSec>]\n",cmdName);
    printf("\t%s <dev> restorestate <type> <unit> <offset>\n",cmdName);
}


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Initiates IOC's.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on IOC.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int		 argc;
    char       **argv;
{
    int		 fd;
    int		 configFd;
    int		 status;
    int		 IOCcmd;
    int		 n;

    cmdName = *argv;
    argc--; argv++;

    if (argc < 1) {
	PrintUsage();
	return(1);
    }
    fd = open(*argv, O_RDONLY, 0);
    if (fd < 0) {
	fprintf(stderr, "Error: could not open device '%s'\n", *argv);
	return(2);
    }
    argc--; argv++;

    if (argc < 1) {
	PrintUsage();
	return(-1);
    }
    if (strcmp("print", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_PRINT; argc--; argv++;
    } else if (strcmp("reconfig", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_RECONFIG; argc--; argv++;
	if (argc < 1) {
	    PrintUsage();
	    return(-1);
	}
	configFd = open(*argv, O_RDONLY, 0666);
	if (configFd < 0) {
	    fprintf(stderr, "Error: could not open device '%s'\n", *argv);
	    return(2);
	}
	argc--; argv++;
	n = read(configFd, IOCParam->buf, IN_BUF_SIZE-1-sizeof(RaidIOCParam));
	IOCParam->buf[n] = 0;
	close(configFd);
	puts(IOCParam->buf);
	status = Fs_IOControl(fd, IOCcmd, sizeof(RaidIOCParam)+n+1, IOCParam,
		 OUT_BUF_SIZE, outBuf);
	printf("status:%d\n", status);
	return status;
    } else if (strcmp("hardinit", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_HARDINIT; argc--; argv++;
	if (argc < 2) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->startStripe = atoi(*argv); argc--; argv++;
	IOCParam->numStripe = atoi(*argv); argc--; argv++;
    } else if (strcmp("paritycheck", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_PARITYCHECK; argc--; argv++;
	if (argc < 2) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->startStripe = atoi(*argv); argc--; argv++;
	IOCParam->numStripe = atoi(*argv); argc--; argv++;
    } else if (strcmp("fail", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_FAIL; argc--; argv++;
	if (argc < 2) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->row = atoi(*argv); argc--; argv++;
	IOCParam->col = atoi(*argv); argc--; argv++;
    } else if (strcmp("replace", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_REPLACE; argc--; argv++;
	if (argc < 4) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->row = atoi(*argv); argc--; argv++;
	IOCParam->col = atoi(*argv); argc--; argv++;
	IOCParam->type = atoi(*argv); argc--; argv++;
	IOCParam->unit = atoi(*argv); argc--; argv++;
    } else if (strcmp("reconstruct", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_RECONSTRUCT; argc--; argv++;
	if (argc < 4) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->row = atoi(*argv); argc--; argv++;
	IOCParam->col = atoi(*argv); argc--; argv++;
	IOCParam->numStripe = atoi(*argv); argc--; argv++;
	IOCParam->uSec = atoi(*argv); argc--; argv++;
    } else if (strcmp("lock", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_LOCK; argc--; argv++;
    } else if (strcmp("unlock", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_UNLOCK; argc--; argv++;
    } else if (strcmp("enable", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_ENABLE; argc--; argv++;
    } else if (strcmp("disable", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_DISABLE; argc--; argv++;
    } else if (strcmp("savestate", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_SAVE_STATE; argc--; argv++;
    } else if (strcmp("enablelog", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_ENABLE_LOG; argc--; argv++;
    } else if (strcmp("disablelog", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_DISABLE_LOG; argc--; argv++;
    } else if (strcmp("restorestate", *argv) == 0) {
	IOCcmd = IOC_DEV_RAID_RESTORE_STATE; argc--; argv++;
	if (argc < 3) {
	    PrintUsage();
	    return(-1);
	}
	IOCParam->type = atoi(*argv); argc--; argv++;
	IOCParam->unit = atoi(*argv); argc--; argv++;
	IOCParam->startStripe = atoi(*argv); argc--; argv++;
    } else {
	PrintUsage();
	return(-1);
    }

printf("ROW %d:COL %d:TYPE %d:UNIT %d:START_STRIPE %d:NUM_STRIPE %d:USEC %d\n",
        IOCParam->row, IOCParam->col, IOCParam->type, IOCParam->unit,
	IOCParam->startStripe, IOCParam->numStripe, IOCParam->uSec);
    status = Fs_IOControl(fd, IOCcmd, sizeof(RaidIOCParam), IOCParam, OUT_BUF_SIZE, outBuf);
    printf("status:%d\n", status);
    outBuf[OUT_BUF_SIZE] = '\0';
    puts(outBuf);
    exit(status);
}
