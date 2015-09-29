/*
 * fsinfo.c --
 *
 *     Print out information related to the file systems, if any, found
 *  on the disk devices (/dev/rsd*) passed as arguments.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fsinfo/RCS/fsinfo.c,v 1.5 91/10/28 14:38:48 voelker Exp Locker: voelker $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys/file.h"
#include "stdio.h"
#include "errno.h"

#include "disk.h"
#include "option.h"

#define GENERIC_OFS_PREFIX  "(new domain)"

/*
 * output strings
 */
#define LFS_NAME            "lfs"
#define OFS_NAME            "ofs"
#define EMPTY_PREFIX_NAME   "(none)"

/*
 * delayed printing of header;  only print the header if
 * more information is going to follow it
 */
static Boolean headerNotPrinted;

/*
 * If true, fsinfo will print out error messages.  fsinfo will
 * complain if it finds a file system on a partition on which
 * the file system was not created;  if it tries to open an invalid
 * partition, which includes the device for the entire disk
 * (e.g., /dev/rsd00);  or, if no file system is found, a message
 * to that effect
 */
Boolean verbose = FALSE;

Option optionArray[] = {
    {OPT_DOC, "", (Address)NIL,
	 "fsinfo [-verbose] device [device]..."},
    {OPT_TRUE, "verbose", (Address)&verbose,
	 "Print error messages"}
};
static int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * PartitionIndex --
 *
 *	Calculates the partition number/index using the device name,
 *      e.g., /dev/rsd00a would have a partition index of 0.
 *      Partition designations start at 'a' and are limited by the
 *      number of partitions on the disk, as specified in the disk label.
 *
 * Results:
 *	If no valid partition index could be found from the device
 *      name, then an index of -1 is returned;  otherwise, the integer
 *      index corresponding to the partition is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
PartitionIndex(deviceName, labelPtr)
    char *deviceName;
    Disk_Label *labelPtr;
{
    if (deviceName != NULL) {
	char c;
	int n;

	c = deviceName[strlen(deviceName) - 1];
	n = (int)(c - 'a');
	if (n < 0 || n > labelPtr->numPartitions - 1) {
	    return -1;
	}
	return n;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintHeader --
 *
 *      Prints the column labels to stdout.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
PrintHeader()
{
    printf("%-13s %-3s  %-15s  %8s  %8s  %9s  %7s\n",
	   "Dev Name", "FS", "Prefix", "Dom. Num", "SpriteID",
	   "Start Cyl", "End Cyl");
}	   

/*
 *----------------------------------------------------------------------
 *
 * PrintInfo --
 *
 *      Prints the file system type, the domain prefix, the domain number,
 *      the SpriteID, the start cylinder and size, and the index of
 *      the partition denoted by `deviceName'.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
PrintInfo(deviceName, fsType, domainPrefix, domainNumber, spriteID, 
	  labelPtr, index)
    char       *deviceName;             /* partition name, e.g., /dev/rsd00a */
    char       *fsType;                 /* file system type */
    char       *domainPrefix;           /* domain prefix */
    int        domainNumber, spriteID;  /* domain # and Sprite ID */
    Disk_Label *labelPtr;               /* disk label */
    int        index;                   /* partition index */
{
    int startCyl, endCyl;
    char *prefix;

    if (labelPtr == NULL) {
	return;
    }
    startCyl = labelPtr->partitions[index].firstCylinder;
    endCyl = labelPtr->partitions[index].numCylinders;
    if (endCyl > 0) {
	endCyl += startCyl - 1;
    } else {
	endCyl = startCyl;
    }
    if (domainPrefix == NULL || *domainPrefix == NULL) {
	prefix = EMPTY_PREFIX_NAME;
    } else if (!strcmp(GENERIC_OFS_PREFIX, domainPrefix)) {
	prefix = EMPTY_PREFIX_NAME;
    } else {
	prefix = domainPrefix;
    } 
    printf("%-13s %3s  %-15s  %8d  %8d  %9d  %7d\n",
	   deviceName, fsType, prefix, domainNumber, spriteID,
	   startCyl, endCyl); 
}    

/*
 *----------------------------------------------------------------------
 *
 * PrintLfsInfo --
 *
 *      Prints the domain prefix, the domain number,
 *      the SpriteID, the start cylinder and size, and the index of
 *      the LFS partition found on the stream
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
int
PrintLfsInfo(stream, deviceName, labelPtr)
    int stream;
    char *deviceName;
    Disk_Label *labelPtr;
{
    LfsCheckPointHdr *headerPtr;
    int index;
    int area = 0;

    headerPtr = Disk_ReadLfsCheckPointHdr(stream, labelPtr, &area);
    if (headerPtr == NULL) {
	return FAILURE;
    } 
    /*
     * only print header if subsequent information is going to be printed
     */
    index = PartitionIndex(deviceName, labelPtr);
    if (index == -1) {
	if (verbose == TRUE) {
	    if (headerNotPrinted == TRUE) {
		PrintHeader();
		headerNotPrinted = FALSE;
	    }
	    printf("%s: Bad partition.  ", deviceName);
	    printf("If this is the raw disk handle,\n");
	    printf("try specifying the `a' partition instead.\n");
	}
	free(headerPtr);
	return FAILURE;
    }
    if (headerNotPrinted == TRUE) {
	PrintHeader();
	headerNotPrinted = FALSE;
    }
    PrintInfo(deviceName, LFS_NAME, headerPtr->domainPrefix, 
	      headerPtr->domainNumber, headerPtr->serverID, 
	      labelPtr, index);
    free(headerPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintOfsInfo --
 *
 *      Prints the domain prefix, the domain number,
 *      the SpriteID, the start cylinder and size, and the index of
 *      the OFS partition found on the stream.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
int
PrintOfsInfo(stream, deviceName, labelPtr)
    int stream;
    char *deviceName;
    Disk_Label *labelPtr;
{
    Ofs_DomainHeader *headerPtr;
    Ofs_SummaryInfo *summaryPtr;
    int index;

    headerPtr = Disk_ReadDomainHeader(stream, labelPtr);
    if (headerPtr == NULL) {
	return FAILURE;
    }
    summaryPtr = Disk_ReadSummaryInfo(stream, labelPtr);
    if (summaryPtr == NULL) {
	free(headerPtr);
	return FAILURE;
    }
    index = PartitionIndex(deviceName, labelPtr);
    if (index == -1) {
	if (verbose == TRUE) {
	    if (headerNotPrinted == TRUE) {
		PrintHeader();
		headerNotPrinted = FALSE;
	    }
	    printf("%s: Bad partition.  ", deviceName);
	    printf("If this is the raw disk handle,\n");
	    printf("try specifying the `a' partition instead.\n");
	}
	free(headerPtr);
	free(summaryPtr);
	return FAILURE;
    }
    if (DISK_PARTITION(&(headerPtr->device)) != index) {
	if (verbose == TRUE) {
	    if (headerNotPrinted == TRUE) {
		PrintHeader();
		headerNotPrinted = FALSE;
	    }
	    printf("%s: The partition has an OFS on it", deviceName);
	    printf(", but the OFS\nwas created on partition `%c'.\n",
		   DISK_PARTITION(&(headerPtr->device)) + 'a');
	}
	free(headerPtr);
	free(summaryPtr);
	return FAILURE;
    }
    if (headerNotPrinted == TRUE) {
	PrintHeader();
	headerNotPrinted = FALSE;
    }
    PrintInfo(deviceName, OFS_NAME, summaryPtr->domainPrefix, 
	      summaryPtr->domainNumber, headerPtr->device.serverID, 
	      labelPtr, index);
    free(headerPtr);
    free(summaryPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      Open the device streams, then read and print the partition and 
 *      file system info found to stdout.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
main(argc, argv)
    int argc;
    char *argv[];
{
    int        stream;
    int        n, fstype;
    Disk_Label *labelPtr;
    int        retval, problemEncountered = 0;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);
    if (argc == 1) {
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(FAILURE);
    }
    headerNotPrinted = TRUE;
    for (n = 1; n < argc; n++) {
	stream = open(argv[n], O_RDONLY, 0);
	if (stream < 0) {
	    continue;
	}
	labelPtr = Disk_ReadLabel(stream);
	if (labelPtr == NULL) {
	    if (verbose == TRUE) {
		if (headerNotPrinted) {
		    PrintHeader();
		    headerNotPrinted = FALSE;
		}
		printf("fsinfo: cannot find label on device %s.  ", argv[n]);
		printf("Is the label\non the disk of the correct type for ");
		printf("the machine being used?\n");
	    }
	    problemEncountered = 1;
	    continue;
	}
	fstype = Disk_HasFilesystem(stream, labelPtr);
	switch (fstype) {
	    case DISK_HAS_OFS:
	        retval = PrintOfsInfo(stream, argv[n], labelPtr);
		if (retval == FAILURE) {
		    problemEncountered = 1;
		}
		break;
	    case DISK_HAS_LFS:
	        retval = PrintLfsInfo(stream, argv[n], labelPtr);
		if (retval == FAILURE) {
		    problemEncountered = 1;
		}
		break;
	    default:
	        if (verbose == TRUE) {
		    if (headerNotPrinted) {
			PrintHeader();
			headerNotPrinted = FALSE;
		    }
		    printf("%s: no file system found\n", argv[n]);
		}
		break;
	}
	free(labelPtr);
    }
    if (headerNotPrinted == TRUE) {
	printf("No file systems found.\n");
    }
    if (problemEncountered) {
	exit(1);
    } else {
	exit(0);
    }
}







