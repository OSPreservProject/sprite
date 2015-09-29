/*
 * mt.c --
 *	Manipulate a tape drive.
 *      This is a bastardized version of Sprite's native tape
 *      utility hacked to understand standard mt command options.
 */
#include <sprite.h>
#include <status.h>
#include <time.h>
#include <sys/file.h>
#include <fs.h>
#include <dev/tape.h>
#include <stdio.h>
#include <option.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "mtio.h"

static int CmdLookup();
static int CvtInteger();

#define DEFTAPE "/dev/rmt12";
char *tapeFile = "";

#define NUMCMDS  17
typedef struct Cmd {
    char *name;
    int id;
} Cmd;

Option optionArray[] = {
    { OPT_STRING, "f", (Address)&tapeFile, "Name of tape device" }, 
};
int numOptions = sizeof(optionArray) / sizeof(Option);

main(argc, argv)
int argc;
char **argv;
{
    int oldOffset;
    ReturnStatus status;
    int tapeStream;
    int count = 1;
    int cmdIndx;
    int openFlags;
    Dev_TapeStatus tapeStatus;
    char *defTape = DEFTAPE;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (!*tapeFile) {
	if ((tapeFile=getenv("TAPE")) == (char *)NULL) {
	    tapeFile = defTape;
	}
    }

    if ((argc < 2) || (argc > 3)) {
        fprintf(stderr,"usage: mt [ -f device ] command [ count ]\n");
	exit(-1);
    }

    if ((cmdIndx=CmdLookup(argv[1])) < 0) {
        fprintf(stderr,"mt: unknown command: %s\n", argv[1]);
	exit(-1);
    }

    if ((argc > 2) && (CvtInteger(argv[2],1,INT_MAX,&count) != SUCCESS)) {
        fprintf(stderr,"mt: bad count: %s\n", argv[2]);
	exit(-1);
    }

    if (cmdIndx == MTWEOF) {
	openFlags = O_RDWR;
    } else {
	openFlags = O_RDONLY;
    }
    tapeStream = open(tapeFile, openFlags, 0);
    if (tapeStream < 0) {
	perror("Can't open tape drive");
	exit(errno);
    }

    switch (cmdIndx) {
    case MTWEOF:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_WEOF, count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't write file mark(s)");
	    exit(status);
	}
	break;
    case MTFSF:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_FILES, count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape files forward");
	    exit(status);
	}
	break;
    case MTBSF:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_FILES, -count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape files backward");
	    exit(status);
	}
	break;
    case MTFSR:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_BLOCKS, count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape records forward");
	    exit(status);
	}
	break;
    case MTBSR:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_BLOCKS, -count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape records backward");
	    exit(status);
	}
	break;
    case MTREW:
	status = Ioc_Reposition(tapeStream, IOC_BASE_ZERO, 0, &oldOffset);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't rewind tape drive");
	    exit(status);
	}
	break;
    case MTOFFL:
	status = Ioc_Reposition(tapeStream, IOC_BASE_ZERO, 0, &oldOffset);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't rewind tape drive");
	    exit(status);
	}
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_UNLOAD, 0);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't unload tape");
	    exit(status);
	}
	break;
    case MTASF:
	status = Ioc_Reposition(tapeStream, IOC_BASE_ZERO, 0, &oldOffset);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't rewind tape drive");
	    exit(status);
	}
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_FILES, count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape files forward");
	    exit(status);
	}
	break;
    case MTRETEN:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_RETENSION, 1);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't retension tape");
	    exit(status);
	}
	break;
    case MTERASE:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_ERASE, 0);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't erase tape");
	    exit(status);
	}
	break;
    case MTEOM:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_EOD, 0);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't position at end-of-tape");
	    exit(status);
	}
	break;
    case MTNBSF:
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_FILES, -(count+1));
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape files backward");
	    exit(status);
	}
	status = Ioc_TapeCommand(tapeStream, IOC_TAPE_SKIP_FILES, 1);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't skip tape files foreward");
	    exit(status);
	}
	break;
    case MTCACHE:
	fprintf(stderr,"Sorry the cache option is not supported.\n");
	break;
    case MTNOCACHE:
	fprintf(stderr,"Sorry the nocache option is not supported.\n");
	break;
    case MTSTATUS:
	bzero((char *) &tapeStatus, sizeof(tapeStatus));
	status = Fs_IOControl(tapeStream, IOC_TAPE_STATUS, 0, NULL, 
	    sizeof(tapeStatus), &tapeStatus);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Can't get status");
	    exit(status);
	}
	printf("Type : 0x%x\n", tapeStatus.type);
	printf("Serial : %s\n", tapeStatus.serial);
	printf("Block size : %d\n", tapeStatus.blockSize);
	printf("Current block : %d\n", tapeStatus.position);
	printf("Remaining blocks : %d\n", tapeStatus.remaining);
	printf("Data errors : %d\n", tapeStatus.dataError);
	printf("Read/Write retry : %d\n", tapeStatus.readWriteRetry);
	printf("Tracking retry : %d\n", tapeStatus.trackingRetry);
	printf("Write protect : %d\n", tapeStatus.writeProtect);
	printf("Buffered mode : %d\n", tapeStatus.bufferedMode);
	printf("Speed : %d\n", tapeStatus.speed);
	printf("Density : %d\n", tapeStatus.density);
	break;
    }

    close(tapeStream);
    exit(SUCCESS);
}

static int
CmdLookup(string)
    char *string;
{
    int i;
    static Cmd cmds[NUMCMDS] = {
	"asf", MTASF,
	"bsf", MTBSF,
	"bsr", MTBSR,
	"cache", MTCACHE,
	"eof", MTWEOF,
	"eom", MTEOM,
	"erase", MTERASE,
	"fsf", MTFSF,
	"fsr", MTFSR,
	"nbsf", MTNBSF,
	"nocache", MTNOCACHE,
	"offline", MTOFFL,
	"retension", MTRETEN,
	"rewind", MTREW,
	"rewoffl", MTOFFL,
	"status", MTSTATUS,
	"weof", MTWEOF
	};

    for (i=0; i<NUMCMDS; i++) {
	if (strcmp(string,cmds[i].name) == 0) {
	    return cmds[i].id;
	}
    }

    return -1;
}

static int
CvtInteger(string, low, high, valPtr)
    char *string;             /* ascii number to convert */
    int low;                  /* lowest acceptable value */
    int high;                 /* highest acceptable value */
    int *valPtr;              /* converted value */
{
    char *endPtr;

    *valPtr = strtol(string, &endPtr, 0);

    if ((*endPtr != '\0') || (*valPtr < low) || (*valPtr > high)) {
	return FAILURE;
    }

    return SUCCESS;
	
}
