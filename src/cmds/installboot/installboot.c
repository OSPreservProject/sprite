/* 
 * installboot.c --
 *
 *	Copy a boot program to the correct place on the disk.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/installboot/RCS/installboot.c,v 1.8 92/01/08 22:54:48 jhh Exp $ SPRITE (Berkeley)";
#endif

#include <sprite.h>
#include <option.h>
#include <disk.h>

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>

/*
 * Settable via the command line.
 */
Boolean unixStyleBootFile = FALSE;
Boolean keepHeader = FALSE;

/*
 * The following are used to go from a command line like
 * bootInstall -dev rsd0
 * to /dev/rsd0a 	- for the partition that has the disk label
 * and to /dev/rsd0b	- for the partition to format.
 */
char *deviceName;		/* Set to "rsd0" or "rxy1", etc. */
char defaultFirstPartName[] = "a";
char *firstPartName = defaultFirstPartName;
char devDirectory[] = "/dev/";

Option optionArray[] = {
    {OPT_STRING, "dev", (Address)&deviceName,
	"Required: Name of device, eg \"rsd0\" or \"rxy1\""},
    {OPT_STRING, "part", (Address)&firstPartName,
	"Optional: Partition ID: (a, b, c, d, e, f, g)"},
    {OPT_TRUE, "noStrip", (Address)&keepHeader,
	"Do not strip off the a.out header (sun4c)\n",},
    {OPT_TRUE, "u", (Address)&unixStyleBootFile,
	"The boot file has no a.out header (unix style)\n",},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 * Forward Declarations.
 */
ReturnStatus InstallBoot();
ReturnStatus DecHeader(), SunHeader();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Create the required file names from the command line
 *	arguments.  Then open the first partition on the disk
 *	and copy the boot program there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls InstallBoot
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status;	/* status of system calls */
    int diskFID;		/* File ID for first parition on the disk */
    int bootFID;		/* File ID for partiton to format */
    char firstPartitionName[64];
    char *bootFile;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    if (deviceName == (char *)0) {
	printf("Specify device name with -dev option\n");
	status = FAILURE;
    } else if (argc < 2) {
	printf("Specify boot program after options\n");
	status = FAILURE;
    } else {
	bootFile = argv[1];
	status = SUCCESS;
    }
    if (status != SUCCESS) {
	exit(FAILURE);
    }
    /*
     * Gen up the name of the first partition on the disk.
     */
    (void)strcpy(firstPartitionName, devDirectory);	/* eg. /dev/ */
    (void)strcat(firstPartitionName, deviceName);	/* eg. /dev/rxy0 */
    (void)strcat(firstPartitionName, firstPartName);	/* eg. /dev/rxy0a */

    diskFID = open(firstPartitionName, O_RDWR, 0);
    if (diskFID < 0) {
	fprintf(stderr, "Can't open \"%s\": %s\n",
				  firstPartitionName, strerror(errno));
	exit(status);
    }
    bootFID = open(bootFile, O_RDONLY, 0);
    if (bootFID < 0) {
	fprintf(stderr, "Can't open boot file \"%s\": %s\n",
				  bootFile, strerror(errno));
	exit(status);
    }
    status = InstallBoot(diskFID, bootFID);

    exit(status);
}

/*
 *----------------------------------------------------------------------
 *
 * InstallBoot --
 *
 *	Write a boot program to the boot sectors of the disk.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Write all over the disk partition.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
InstallBoot(diskFID, bootFID)
    int diskFID;	/* Handle on the first partition of the disk */
    int bootFID;	/* Handle on the boot program */
{
    ReturnStatus status;
    register int numBlocks;
#if 1    
    Disk_Label *diskInfoPtr;
#else    
    Disk_Info  *diskInfoPtr;
#endif    
    Dec_DiskBoot	decBootInfo;
    int bytesRead;
    Address sector;
    int sectorIndex;
    Address loadAddr;
    Address execAddr;
    int	length;
    int	headerSize;
    int toRead;
    int decDisk = 0;	/* 1 if this is a Dec disk. */

    /*
     * Read the copy of the super block at the beginning of the partition
     * to find out basic disk geometry and where to write the boot program.
     */
#if 1
    if ((diskInfoPtr = Disk_ReadLabel(diskFID)) == NULL) {
	return FAILURE;
    }
#else    
    diskInfoPtr = Disk_ReadDiskInfo(diskFID, 0);
    if (diskInfoPtr == (Disk_Info *)0) {
	return(FAILURE);
    }
#endif    
    if (Disk_ReadDecLabel(diskFID) != (Dec_DiskLabel *)0) {
	decDisk++;
    }

    if ((headerSize = SunHeader(bootFID, &loadAddr, &execAddr, &length)) < 0) {
	if (DecHeader(bootFID, &loadAddr, &execAddr, &length) != SUCCESS) {
	    printf("Need impure text format (OMAGIC) file\n");
	    return FAILURE;
	}
    }

    if (keepHeader) {
	lseek(bootFID, 0L, 0);
	length += headerSize;
    }

    /*
     * Write the boot information block.
     */
    if (decDisk) {
	diskInfoPtr->bootSector = DEC_BOOT_SECTOR + 1;
	decBootInfo.magic = DEC_BOOT_MAGIC;
	decBootInfo.mode = 0;
	decBootInfo.loadAddr = (int) loadAddr;
	decBootInfo.execAddr = (int) execAddr;
	decBootInfo.map[0].numBlocks = diskInfoPtr->numBootSectors;
	decBootInfo.map[0].startBlock = diskInfoPtr->bootSector;
	decBootInfo.map[1].numBlocks = 0;
	status = Disk_SectorWrite(diskFID, DEC_BOOT_SECTOR, 1, &decBootInfo);
	if (status < 0) {
	    fprintf(stderr, "Sector write %d failed: ", DEC_BOOT_SECTOR);
	    perror("");
	    return(errno);
	}
    }
    /*
     * Write the remaining code to the correct place on the disk.
     */
    sector = (Address)malloc(DEV_BYTES_PER_SECTOR);
    for (sectorIndex=0 ; sectorIndex < diskInfoPtr->numBootSectors && length>0;
			 sectorIndex++) {
	bzero(sector, DEV_BYTES_PER_SECTOR);
	toRead = length < DEV_BYTES_PER_SECTOR ? length :
		DEV_BYTES_PER_SECTOR;
	bytesRead = read(bootFID, sector, toRead);
	if (bytesRead < toRead) {
	    perror("Boot file read failed");
	    return(status);
	}
	if (bytesRead > 0) {
	    length -= bytesRead;
	    status = Disk_SectorWrite(diskFID,
				 diskInfoPtr->bootSector + sectorIndex,
				 1, sector);
	    if (status < 0) {
		fprintf(stderr, "Sector write %d failed: ", sectorIndex);
		perror("");
		return(errno);
	    }
	} else {
	    sectorIndex++;
	    break;
	}
    }
    printf("Wrote %d sectors\n", sectorIndex);
    if (length > 0) {
	printf("Warning: didn't reach end of boot program!\n");
    }
    return(SUCCESS);
}
