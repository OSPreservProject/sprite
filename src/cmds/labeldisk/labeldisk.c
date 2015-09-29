/* 
 * labeldisk.c --
 *
 *	Read and possibly change the disk label.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/labeldisk/RCS/labeldisk.c,v 1.15 92/02/06 12:05:21 voelker Exp Locker: voelker $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include "option.h"
#include "disk.h"
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <kernel/dev.h>

#define Min(a,b) (((a) < (b)) ? (a) : (b))
/*
 * Constants settable via the command line.
 */
char *fromName;         /* e.g. set to "/dev/rsd0" */
char *toName;		/* e.g. set to "/dev/rsd0g" */
Boolean writeLabel = FALSE;
Boolean writeSun = FALSE;
Boolean writeDec = FALSE;
Boolean newLabel = FALSE;
Boolean writeQuick = FALSE;

Option optionArray[] = {
    {OPT_DOC,"",(Address)NIL,
	"labeldisk [-from fromDevice] [-w] [-q] [-sun] [-dec] [-new] toDevice"},
    {OPT_STRING, "from", (char *)&fromName,
	"Read the disk label from this partition"},
    {OPT_TRUE, "w", (Address)&writeLabel,
	"Write a new disk label"},
    {OPT_TRUE, "sun", (Address)&writeSun,
	"Write a Sun label"},
    {OPT_TRUE, "dec", (Address)&writeDec,
	"Write a Dec label"},
    {OPT_TRUE, "new", (Address)&newLabel,
	"Ignore any old label"},
    {OPT_TRUE, "q", (Address)&writeQuick,
	"Copy label quickly, without prompting to change the label"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);

static ReturnStatus	LabelDisk();
static void		EditLabel();
static void		InputNumber();
static int		IsYes();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Open the disk device and call LabelDisk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    int openFlags;
    int toStreamID;
    int fromStreamID;
    int tmp;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);
    if (argc != 2) {
	if (argc == 1 && fromName != NULL && strlen(fromName) > 0) {
	    toName = fromName;
	} else {
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(FAILURE);
	}
    } else {
	if (fromName == NULL || strlen(fromName) == 0) {
	    fromName = argv[1];
	}
	toName = argv[1];
    } 
    if (writeSun || writeDec || writeQuick) {
	writeLabel = TRUE;
    }
    if (writeLabel) {
	openFlags = O_RDWR;
    } else {
	openFlags = O_RDONLY;
    }
    if ((writeSun?1:0)+(writeDec?1:0) > 1) {
	fprintf(stderr,
		"Can't specify more than one of -sun, -dec.\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(FAILURE);
    } 
    fromStreamID = open(fromName, openFlags, 0);
    if (fromStreamID < 0) {
	perror("Can't open device");
	exit(FAILURE);
    }
    tmp = strcmp(fromName, toName);
    if (tmp) {
	toStreamID = open(toName, openFlags, 0);
	if (toStreamID < 0) {
	    perror("Can't open device");
	    exit(FAILURE);
	}
    } else {
	toStreamID = fromStreamID;
    }
    if (LabelDisk(fromStreamID, toStreamID) != SUCCESS) {
	if (errno == 0) {
	    fprintf(stderr, "labeldisk: Short read, or EOF encountered ");
	    fprintf(stderr, "on most likely an empty partition.\n");
	} else {
	    perror("labeldisk");
	}
    }
    exit(errno);
}

/*
 *----------------------------------------------------------------------
 *
 * LabelDisk --
 *	Read and perhaps write the disk label.
 *
 * Results:
 *	A status.
 *
 * Side effects:
 *	Rewrites the partition information on the 0'th sector of the disk.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
LabelDisk(fromStreamID, toStreamID)
    int fromStreamID;			/* Handle on raw input disk */
    int toStreamID;                     /* Handle on raw output disk */
{
    Disk_Label 			*diskLabelPtr;
    int 			n;
    char 			answer[80];
    Disk_NativeLabelType	inLabel, outLabel;
    static char                 buffer[1024], buffer2[1024];
    ReturnStatus		status;
    Ofs_DomainHeader            *headerPtr;

    if (writeSun) {
	outLabel = DISK_SUN_LABEL;
    } else if (writeDec) {
	outLabel = DISK_DEC_LABEL;
    } else {
	outLabel = DISK_NO_LABEL;
    }
    diskLabelPtr = Disk_ReadLabel(fromStreamID);
    if (newLabel || diskLabelPtr == NULL) {
	if (!newLabel) {
	    printf("The disk does not have a label.\n");
	}
	if (!writeLabel) {
	    return FAILURE;
	}
	if (outLabel == DISK_NO_LABEL) {
	    printf("You must specify the -sun, or -dec option.\n");
	    return FAILURE;
	}
	inLabel = DISK_NO_LABEL;
	diskLabelPtr = Disk_NewLabel(outLabel);
    }
    if (diskLabelPtr != NULL) {
	inLabel = diskLabelPtr->labelType;
    }
    if (outLabel == DISK_NO_LABEL) {
	outLabel = inLabel;
    }
    Disk_PrintLabel(diskLabelPtr);
    if (!writeLabel) {
	return SUCCESS;
    }
    if (inLabel != outLabel) {
	/*
	 * At this point, the output label should be an empty buffer.
	 * diskHeaderPointer should be a valid label or empty buffer.
	 * diskInfoPtr should be have the proper sector values and string.
	 */
	printf(
	  "Do you really want to replace a %s label with a %s label? (y/n) ",
	  Disk_GetLabelTypeName(inLabel), Disk_GetLabelTypeName(outLabel));
	n = scanf("%s", answer);
	if (n == EOF) {
	    exit(1);
	}
	if (strcasecmp(answer, "y")) {
	    exit(1);
	}
    }
    if (!writeQuick) {
	EditLabel(toStreamID, diskLabelPtr);
    } else {
	printf("\nCommit label? (y/n) ");
	if (!IsYes()) {
	    exit(0);
	}
    }
    if (inLabel != outLabel && fromStreamID == toStreamID) {
	status = Disk_EraseLabel(fromStreamID, inLabel);
	if (status != SUCCESS) {
	    printf("Unable to erase old label.\n");
	}
    }
    printf("Write this label to all valid partitions? (y/n) ");
    n = scanf("%s", answer);
    if ((n == EOF) || strcasecmp(answer, "y")) {
	printf("Writing disk label to %s.\n", toName);
	status = Disk_WriteLabel(toStreamID, diskLabelPtr, outLabel);
	return status;
    } else {
	int part, cyls, streamID, len, cantWriteToRawDisk = FALSE;
	char c, *devName;

	len = strlen(toName);
	devName = (char *)malloc(sizeof(char) * len + 1);
	    if (devName == NULL) {
	    perror("allocating string");
	    exit(FAILURE);
	}
	c = toName[len - 1];
	if (c >= 'a' && c < 'i') {
	    /*
	     * the original destination name is a partition, 
	     * e.g. "/dev/rsd00a", so make sure to write the label
	     * to the beginning of the raw disk and then
	     * write the label to all valid partitions
	     */
	    sprintf(devName, "%s",toName);
	    devName[--len] = '\0';  
	    streamID = open(devName, O_RDWR, 0);
	} else {
	    /*
	     * the original destination name is a raw disk,
	     * e.g. "/dev/rsd00", so write the label to the raw
	     * disk and then to all valid partitions
	     */
	    sprintf(devName, "%s",toName);
	    streamID = toStreamID;
	}
	printf("raw device: %s\n", devName);
	if (streamID >= 0) {
	    status = Disk_WriteLabel(streamID, diskLabelPtr, outLabel);
	    if (status != SUCCESS) {
		printf("Unable to write disk label to ");
		printf("%s...skipping\n", devName);
		cantWriteToRawDisk = TRUE;
	    }
	} else {
	    cantWriteToRawDisk = TRUE;
	}
	devName[len + 1] = '\0';
	for (part = 0 ; part < diskLabelPtr -> numPartitions; part++) {
	    cyls = diskLabelPtr->partitions[part].numCylinders;
	    if (cyls <= 0) {
		continue;
	    }
	    devName[len] = 'a' + part;
	    printf("partition: %s\n", devName);
	    if (!strcmp(devName, fromName)) {
		streamID = fromStreamID;
	    } else if (!strcmp(devName, toName)) {
		streamID = toStreamID;
	    } else {
		streamID = open(devName, O_RDWR, 0);
		if (streamID < 0) {
		    perror("Can't open device");
		    exit(FAILURE);
		}
	    }
	    status = Disk_HasFilesystem(streamID, diskLabelPtr);
	    if (status == DISK_HAS_NO_FS) {
		printf("Could not find file system on ");
		printf("%s...skipping\n", devName);
		continue;
	    }
	    status = Disk_WriteLabel(streamID, diskLabelPtr, outLabel);
	    if (status) {
		printf("Unable to write disk label to ");
		printf("%s...skipping\n", devName);
	    } else {
		/*
		 * Having written to the partition, if the partition
		 * starts at cylinder 0 then we have effectively
		 * written to the raw disk
		 */
		if (diskLabelPtr->partitions[part].firstCylinder == 0) {
		    cantWriteToRawDisk = FALSE;
		}
	    }
	}
	if (cantWriteToRawDisk == TRUE) {
	    devName[len] = '\0';
	    printf("Warning:  either couldn't open and write to");
	    printf("%s or\ncouldn't write to a valid partition ", devName);
	    printf("that starts at cylinder 0.\n");
	}
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * EditLabel --
 *
 *	Interactively edits the disk label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes contents of the disk label.
 *
 *----------------------------------------------------------------------
 */
static void
EditLabel(streamID, labelPtr)
    int			streamID;	/* Handle on raw disk */
    Disk_Label		*labelPtr;	/* The disk label */
{
    int                 numHeads, numSectors, part;
    ReturnStatus        status = SUCCESS;
    int                 first, cyls, last, lastSector, lastCyl;
    char	        buffer[DEV_BYTES_PER_SECTOR];
    char		*tmpPtr;
    int			labelLen;

editLabel:
    printf("Size of ascii label (%d): ", labelPtr->asciiLabelLen);
    InputNumber(&labelPtr->asciiLabelLen);
    printf("ascii label (%s): ", labelPtr->asciiLabel);
    tmpPtr = fgets(buffer, labelPtr->asciiLabelLen,stdin);
    if (tmpPtr == NULL) {
	exit(1);
    }
    printf("%d\n", strlen(buffer));
    if (strlen(buffer) > 1) {
	labelLen = Min(strlen(buffer), labelPtr->asciiLabelLen);
	strncpy(labelPtr->asciiLabel, buffer, labelLen);
	labelPtr->asciiLabel[labelLen - 1] = '\0';
    }
    printf("Number of heads (%d): ", labelPtr->numHeads);
    InputNumber(&labelPtr->numHeads);
    printf("Number of sectors per track (%d): ", labelPtr->numSectors);
    InputNumber(&labelPtr->numSectors);
    printf("Number of cylinders (%d): ", labelPtr->numCylinders);
    InputNumber(&labelPtr->numCylinders);
    printf("Number of alternate cylinders (%d): ", labelPtr->numAltCylinders);
    InputNumber(&labelPtr->numAltCylinders);
    printf("Starting sector of boot program (%d): ", labelPtr->bootSector);
    InputNumber(&labelPtr->bootSector);
    printf("Number of boot sectors (%d): ", labelPtr->numBootSectors);
    InputNumber(&labelPtr->numBootSectors);
    printf("Starting sector of summary info (%d): ", labelPtr->summarySector);
    InputNumber(&labelPtr->summarySector);
    printf("Starting sector of domain header (%d): ", labelPtr->domainSector);
    InputNumber(&labelPtr->domainSector);
    numHeads = labelPtr->numHeads;
    numSectors = labelPtr->numSectors;
    if (numHeads * numSectors != 
	labelPtr->numHeads * labelPtr->numSectors) {
	printf(
    "The size of a cylinder changed so I have to zero the partition map.\n");
	for (part=0 ; part < labelPtr->numPartitions; part++) {
	    labelPtr->partitions[part].firstCylinder = 0;
	    labelPtr->partitions[part].numCylinders = 0;
	}
    }
    for (part=0 ; part < labelPtr->numPartitions; part++) {
	first = labelPtr->partitions[part].firstCylinder;
	cyls = labelPtr->partitions[part].numCylinders;
	last = (cyls > 0) ? (cyls + first - 1) : first;
	printf("\n%c: First %4d Last %4d Num %4d (%7d sectors)\n",
		'a' + part, first, last, cyls,
		cyls *  labelPtr->numHeads * labelPtr->numSectors);
	printf("First (%d): ", labelPtr->partitions[part].firstCylinder);
	InputNumber(&labelPtr->partitions[part].firstCylinder);
	printf("Num (%d): ", labelPtr->partitions[part].numCylinders);
	InputNumber(&labelPtr->partitions[part].numCylinders);
    }
    printf("\nNew Label\n");
    Disk_PrintLabel(labelPtr);
    /* Determine the last cylinder */
    lastCyl = 0;
    for (part=0 ; part < labelPtr->numPartitions; part++) {
	first = labelPtr->partitions[part].firstCylinder;
	cyls = labelPtr->partitions[part].numCylinders;
	last = (cyls > 0) ? (cyls + first - 1) : first;
	if (last>lastCyl) {
	    lastCyl = last;
	}
    }
    lastSector = (lastCyl + 1) * numHeads * numSectors - 1;
    status = Disk_SectorRead(streamID, lastSector, 1, buffer);
    if (status != 0) {
	printf("I couldn't read the last sector (sector %d)!!!\n", lastSector);
	printf("Either the disk isn't as big as you think it is,\n");
	printf("or the device is not a raw device.\n");
	printf("(cylinders=%d, numHeads=%d, numSectors=%d)\n", lastCyl+1,
	    numHeads, numSectors);
	printf("Status = %x\n", status);
    }
    printf("\nCommit new label? (y/n) ");
    if (!IsYes()) {
	printf("Try again\n");
	goto editLabel;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InputNumber --
 *
 *	Get a number interactively.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is read from stdin..
 *
 *----------------------------------------------------------------------
 */
static void
InputNumber(number)
    int		*number;	/* Place to store number. */
{
    int n, val;
    char buffer[80];

    if (fgets(buffer, 80, stdin) == NULL) {
	exit(1);
    }
    n = sscanf(buffer, "%d", &val);
    if (n < 1) {
	return;
    }
    *number = val;
}

/*
 *----------------------------------------------------------------------
 *
 * IsYes --
 *
 *	Returns TRUE if user answers y.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
IsYes()
{
    int n;
    char answer[80];
    n = scanf("%s", answer);
    if (n == EOF) {
	exit(1);
    }
    if (strcmp(answer, "y")==0) {
	return TRUE;
    } else {
	return FALSE;
    }
}
