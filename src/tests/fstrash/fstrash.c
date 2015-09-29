/* 
 * fstrash.c --
 *
 *	Format a domain to be a filesystem with lots of errors. This program 
 *	is intended to be a test for fscheck. Refer to fstrash.man for details
 *	on the filesystem created and the actions fscheck should take to 
 *	correct it.
 *	Warning - the trashed filesystem has been constructed by hand, and as
 *	a result this code is very sensitive to changes. If you want to add a
 *	new file you must do the following things:
 *	 - find an available file descriptor (see man page)
 *	 - find available data blocks (see man page)
 *	 - create a routine to fill in the descriptor (SetFooFD)
 *	 - add a call to this routine in WriteFileDesc
 *	 - mark the fd as in use in MakeFileDescBitmap
 *	 - mark the data blocks as in use in WriteBitmap
 *	 - add the file as an entry in a directory (WriteRootDirectory)
 *	 - if the new file is a directory create a new routine to fill in
 *		the directory (WriteFoo) and add a call to this routine in
 *		MakeFilesystem
 *	Since you are creating a trashed file system you may want to skip some
 *	of the above steps. Just make sure that you don't do something 
 *	unexpected by using blocks that are already in use, forgetting to
 *	add the file to a directory, etc. 
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
static char rcsid[] = "$Header: /sprite/src/tests/fstrash/RCS/fstrash.c,v 1.4 89/06/19 14:19:31 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "option.h"
#include "disk.h"
#include <stdio.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>

/*
 * Constants settable via the command line.
 */
int kbytesToFileDesc = 4;	/* The ratio of kbytes to
				 * the number of file descriptors */
Boolean printOnly = TRUE;	/* Stop after computing the domain header
				 * and just print it out. No disk writes */
Boolean copySuperBlock = FALSE;	/* Copy the super block from the first
				 * disk partition to the one being formatted */
Boolean makeFile = TRUE;	/* Make a file in the root directory,
				 * this is used when testing the filesystem */
Boolean overlapBlocks = FALSE;	/* Allow filesystem blocks to overlap track
				 * boundaries.  Some disk systems can't deal. */
Boolean rootFree = FALSE;	/* Mark the root as free */
Boolean rootFile = FALSE;	/* Make the root a file */
Boolean noRoot = FALSE;		/* Zero out the root fd and block 0 */
Boolean scsiDisk = TRUE;	/* If TRUE then simpler geometry is computed
				 * that knows that SCSI controllers don't
				 * think in terms of cylinders, heads, and
				 * sectors.  Instead, they think in terms of
				 * logical sector numbers, so we punt on finding
				 * rotationally optimal block positions. */
/*
 * The following are used to go from a command line like
 * makeFilesystem -D rsd0 -P b
 * to /dev/rsd0a 	- for the partition that has the disk label
 * and to /dev/rsd0b	- for the partition to format.
 */
char *deviceName;		/* Set to "rsd0" or "rxy1", etc. */
char *partName;			/* Set to "a", "b", "c" ... "g" */
char defaultFirstPartName[] = "a";
char *firstPartName = defaultFirstPartName;
char defaultDevDirectory[] = "/dev/";
char *devDirectory = defaultDevDirectory;

Option optionArray[] = {
    {OPT_STRING, "dev", (Address)&deviceName,
	"Required: Name of device, eg \"rsd0\" or \"rxy1\""},
    {OPT_STRING, "part", (Address)&partName,
	"Required: Partition ID: (a, b, c, d, e, f, g)"},
    {OPT_TRUE, "overlap", (Address)&overlapBlocks,
	"Overlap filesystem blocks across track boundaries (FALSE)"},
    {OPT_TRUE, "copySuper", (Address)&copySuperBlock,
	"Copy the super block to the partition (FALSE)"},
    {OPT_INT, "ratio", (Address)&kbytesToFileDesc,
	"Ratio of Kbytes to file descriptors (4)"},
    {OPT_TRUE, "test", (Address)&printOnly,
	"Print results, don't write disk (TRUE)"},
    {OPT_FALSE, "write", (Address)&printOnly,
	"Write the disk (FALSE)"},
    {OPT_STRING, "dir", (Address)&devDirectory,
	"Name of device directory (\"/dev/\")"},
    {OPT_STRING, "initialPart", (Address)&firstPartName,
	"Name of initial partition (\"a\")"},
    {OPT_TRUE, "freeRoot", (Address)&rootFree,
	"Mark root fd as being free. (FALSE)"},
    {OPT_TRUE, "fileRoot", (Address)&rootFile,
	"Mark root fd as being a file. (FALSE)"},
    {OPT_TRUE, "noRoot", (Address)&noRoot,
	"Zero out root fd and block 0 (FALSE)"},
    {OPT_FALSE, "noscsi", (Address)&scsiDisk,
	"Compute geometry for non-SCSI disk (FALSE)"},

};
int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 * Forward Declarations.
 */
void SetSummaryInfo();
void SetDomainHeader();
void SetDiskGeometry();
void SetSCSIDiskGeometry();
void SetDomainParts();
void SetRootFileDescriptor();
void SetBadBlockFileDescriptor();
void SetLostFoundFileDescriptor();
void SetEmptyFileDescriptor();
void SetTooBigFD();
void SetTooSmallFD();
void SetHoleFileFD();
void SetHoleDirFD();
void SetBadEntryFileFD();
void SetFragFileFD();
void SetCopyFragFileFD();
void SetCopyBlockFileFD();
void SetCopyIndBlockFileFD();
void SetCopyBogusIndBlockFileFD();
void SetDirFD();
ReturnStatus WriteFileDesc();
ReturnStatus WriteFileDescBitmap();
ReturnStatus WriteBitmap();
char *MakeFileDescBitmap();

#define Max(a,b) ((a) > (b)) ? (a) : (b)


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Create the required file names from the command line
 *	arguments.  Then open the first partition on the disk
 *	because it contains the disk label, and open the partition
 *	that is to be formatted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls MakeFilesystem
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status;	/* status of system calls */
    int firstPartFID;		/* File ID for first parition on the disk */
    int partFID;		/* File ID for partiton to format */
    char firstPartitionName[64];
    char partitionName[64];
    int partition;		/* Index of partition derived from the
				 * partition name */
    int spriteID;		/* Host ID of the machine with the disks */
    Fs_Attributes attrs;
    char 	answer[100];

    (void)Opt_Parse(argc, argv,optionArray, numOptions, 0);

    status = SUCCESS;
    if (deviceName == (char *)0) {
	fprintf(stderr,"Specify device name with -dev option\n");
	status = FAILURE;
    }
    if (partName == (char *)0) {
	fprintf(stderr,"Specify partition with -part option\n");
	status = FAILURE;
    }
    if (status != SUCCESS) {
	exit(status);
    }
    if (!printOnly) {
	printf("The \"-write\" option will cause fstrash to overwrite the current filesystem.\nDo you really want to do this?[y/n] ");
	if (scanf("%10s",answer) != 1) {
	    exit(SUCCESS);
	}
	if ((*answer != 'y') && (*answer != 'Y')) {
	    exit(SUCCESS);
	}
    }


    /*
     * Gen up the name of the first partition on the disk,
     * and the name of the parition that needs to be formatted.
     */
    (void) strcpy(firstPartitionName, devDirectory);	/* eg. /dev/ */
    (void) strcpy(partitionName, devDirectory);
    (void) strcat(firstPartitionName, deviceName);	/* eg. /dev/rxy0 */
    (void) strcat(partitionName, deviceName);
    (void) strcat(firstPartitionName, firstPartName);	/* eg. /dev/rxy0a */
    (void) strcat(partitionName, partName);		/* eg. /dev/rxy0b */


    firstPartFID = open(firstPartitionName, O_RDONLY);
    if (firstPartFID < 0 ) {
	perror("Can't open first partition");
	exit(FAILURE);
    }
    partFID = open(partitionName, O_RDWR);
    if (partFID < 0) {
	perror("Can't open partition to format");
	exit(FAILURE);
    }

    partition = partName[0] - 'a';
    if (partition < 0 || partition > 7) {
	fprintf(stderr,
	       "Can't determine partition index from the partition name\n");
	exit(FAILURE);
    }

    /*
     * Determine where the disk is located so we can set the
     * spriteID in the header correctly.
     */
    Fs_GetAttributesID(firstPartFID, &attrs);
    if (attrs.devServerID == FS_LOCALHOST_ID) {
	Sys_GetMachineInfo(NULL, NULL, &spriteID);
	printf("Making filesystem for local host, ID = 0x%x\n", spriteID);
    } else {
	spriteID = attrs.devServerID;
	printf("Making filesystem for remote host 0x%x\n", spriteID);
    }
    printf("MakeFilesystem based on 4K filesystem blocks\n");
    status = MakeFilesystem(firstPartFID, partFID, partition, spriteID);

    fflush(stderr);
    fflush(stdout);
    (void)close(firstPartFID);
    (void)close(partFID);
    exit(status);
}

/*
 *----------------------------------------------------------------------
 *
 * MakeFilesystem --
 *
 *	Format a disk partition, or domain.
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
MakeFilesystem(firstPartFID, partFID, partition, spriteID)
    int firstPartFID;	/* Handle on the first partition of the disk */
    int partFID;	/* Handle on the partition of the disk to format */
    int partition;	/* Index of parition that is to be formatted */
    int spriteID;	/* Host ID of the machine with the disks, this
			 * gets written on the disk and used at boot time */
{
    ReturnStatus status;
    Disk_Label *labelPtr;
    Fsdm_DomainHeader *headerPtr;
    Fsdm_SummaryInfo *summaryPtr;

    /*
     * Read the copy of the super block at the beginning of the partition
     * to find out basic disk geometry and where to write the domain header.
     */
    labelPtr = Disk_ReadLabel(firstPartFID);
    if (labelPtr == (Disk_Label *)0) {
	fprintf(stderr,"MakeFilesystem: Unable to read disk label.\n");
	return(FAILURE);
    }

    headerPtr = (Fsdm_DomainHeader *)
	malloc((unsigned) labelPtr->numDomainSectors * DEV_BYTES_PER_SECTOR);
    SetDomainHeader(labelPtr, headerPtr, spriteID, partition);
    Disk_PrintDomainHeader(headerPtr);

    if (!printOnly) {
	if (copySuperBlock && partition != 0) {
	    status = CopySuperBlock(firstPartFID, partFID);
	    if (status != SUCCESS) {
		perror("CopySuperBlock failed"); 
		return(status);
	    }
	}
	status = Disk_SectorWrite(partFID, labelPtr->domainSector,
			    labelPtr->numDomainSectors, (Address)headerPtr);
	if (status != SUCCESS) {
	    perror("DomainHeader write failed");
	    return(status);
	}
    }
    summaryPtr = (Fsdm_SummaryInfo *) malloc(DEV_BYTES_PER_SECTOR);
    SetSummaryInfo(labelPtr, headerPtr, summaryPtr);
    Disk_PrintSummaryInfo(summaryPtr);
    if (!printOnly) {
	status = Disk_SectorWrite(partFID, labelPtr->summarySector, 1,
			    (Address)summaryPtr);
	if (status != SUCCESS) {
	    perror("Summary sector  write failed");
	    return(status);
	}
    }

    status = WriteFileDesc(headerPtr, partFID);
    if (status != SUCCESS) {
	perror("WriteFileDesc failed");
	return(status);
    }
    status = WriteBitmap(headerPtr, partFID);
    if (status != SUCCESS) {
	perror("WriteBitmap failed");
	return(status);
    }
    status = WriteRootDirectory(headerPtr, partFID);
    if (status != SUCCESS) {
	perror("WriteRootDirectory failed");
	return(status);
    }
    status = WriteLostFoundDirectory(headerPtr, partFID);
    if (status != SUCCESS) {
	perror("WriteLostFoundDirectory failed");
	return(status);
    }
    status = WriteEmptyDirectory(headerPtr, partFID, 17, 16, 64);
    if (status != SUCCESS) {
	perror("WriteEmptyDirectory failed");
	return(status);
    }
    status = WriteEmptyDirectory(headerPtr, partFID, 9, 2, 32);
    if (status != SUCCESS) {
	perror("WriteEmptyDirectory failed");
	return(status);
    }
    status = WriteBadDotDotDirectory(headerPtr, partFID, 21, 2, 76);
    if (status != SUCCESS) {
	perror("WriteEmptyDirectory failed");
	return(status);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * CopySuperBlock --
 *
 *	Copy the super block from the first sector of the disk to
 *	the first sector of the partition being formatted.
 *
 * Results:
 *	A return code from the I/O.
 *
 * Side effects:
 *	Writes on the zero'th sector of the partition.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
CopySuperBlock(firstPartFID, partFID)
    int firstPartFID;
    int partFID;
{
    ReturnStatus status;
    char *block;

    block = (char *)malloc(DEV_BYTES_PER_SECTOR);

    status = Disk_SectorRead(firstPartFID, 0, 1, block);
    if (status != SUCCESS) {
	return(status);
    }
    status = Disk_SectorWrite(partFID, 0, 1, block);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * SetDomainHeader --
 *
 *	Compute the domain header based on the partition size and
 *	other basic disk parameters.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Fill in the domain header.
 *
 *----------------------------------------------------------------------
 */
void
SetDomainHeader(labelPtr, headerPtr, spriteID, partition)
    Disk_Label *labelPtr;	/* Information from the super block */
    Fsdm_DomainHeader *headerPtr;	/* Reference to domain header to fill in */
    int spriteID;		/* Host ID of machine with the disks */
    int partition;		/* Index of partition to format */
{
    Fsdm_Geometry *geoPtr;/* The layout information for the disk */

    headerPtr->magic = FSDM_DOMAIN_MAGIC;
    headerPtr->firstCylinder = labelPtr->partitions[partition].firstCylinder;
    headerPtr->numCylinders = labelPtr->partitions[partition].numCylinders;
    /*
     * The device.serverID from the disk is used during boot to discover
     * the host"s spriteID if reverse arp couldn't find a host ID.  The
     * unit number of disk indicates what partition of the disk this
     * domain header applies to.  For example, both the "a" and "c" partitions
     * typically start at sector zero, but only one is valid.  During boot
     * time the unit number is used to decide which partition should be
     * attached.
     */
    headerPtr->device.serverID = spriteID;
    headerPtr->device.type = -1;
    headerPtr->device.unit = partition;
    headerPtr->device.data = (ClientData)-1;

    geoPtr = &headerPtr->geometry;
    if (scsiDisk) {
	SetSCSIDiskGeometry(labelPtr, geoPtr);
    } else {
	SetDiskGeometry(labelPtr, geoPtr);
    }
    SetDomainParts(labelPtr, partition, headerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SetSCSIDiskGeometry --
 *
 *	This computes the rotational set arrangment depending on the
 *	disk geometry.  No fancy block skewing is done.  The cylinders
 *	are divided into rotational sets that minimize the amount of
 *	wasted space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fill in the geometry struct.
 *
 *----------------------------------------------------------------------
 */
void
SetSCSIDiskGeometry(labelPtr, geoPtr)
    Disk_Label		*labelPtr;	/* The disk label. */
    Fsdm_Geometry 	*geoPtr;	/* Fancy geometry information */
{
    int index;		/* Array index */
    int blocksPerCyl;		/* The number of blocks in a cylinder */

    geoPtr->numHeads = labelPtr->numHeads;
    geoPtr->sectorsPerTrack = labelPtr->numSectors;

    blocksPerCyl = geoPtr->sectorsPerTrack * geoPtr->numHeads /
		DISK_SECTORS_PER_BLOCK;

    printf("Disk has %d tracks/cyl, %d sectors/track\n",
	    geoPtr->numHeads, geoPtr->sectorsPerTrack);
    printf("%d 4K Blocks fit on a cylinder with %d 512 byte sectors wasted\n",
	    blocksPerCyl, geoPtr->sectorsPerTrack * geoPtr->numHeads - 
	    blocksPerCyl * DISK_SECTORS_PER_BLOCK);
    geoPtr->rotSetsPerCyl = FSDM_SCSI_MAPPING;
    geoPtr->blocksPerRotSet = 0;
    geoPtr->blocksPerCylinder = blocksPerCyl;
    geoPtr->tracksPerRotSet = 0;
    /*
     * Don't use rotational sorting anyway.
     */
    for (index = 0 ; index < FSDM_MAX_ROT_POSITIONS ; index++) {
	geoPtr->sortedOffsets[index] = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetDiskGeometry --
 *
 *	This computes the rotational set arrangment depending on the
 *	disk geometry.  The basic rules for this are that filesystem blocks
 *	are skewed on successive tracks, and that the skewing pattern
 *	repeats in either 2 or 4 tracks.  This is specific to the fact that
 *	filesystem blocks are 4Kbytes.  This means that one disk track
 *	contains N/4 filesystem blocks and that one sector per track
 *	is wasted if there are an odd number of sectors per track.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fill in the geometry struct.
 *
 *----------------------------------------------------------------------
 */
void
SetDiskGeometry(labelPtr, geoPtr)
    register Disk_Label *labelPtr;/* Basic geometry information */
    register Fsdm_Geometry *geoPtr;	/* Fancy geometry information */
{
    register int index;		/* Array index */
    int numBlocks;		/* The number of blocks in a rotational set */
    int tracksPerSet;		/* Total number of tracks in a rotational set */
    int numTracks;		/* The number of tracks in the set so far */
    int extraSectors;		/* The number of leftover sectors in a track */
    int offset;			/* The sector offset within a track */
    int startingOffset;		/* The offset of the first block in a track */
    int offsetIncrement;	/* The skew of the starting offset on each
				 * successive track of the rotational set */
    Boolean overlap;		/* TRUE if filesystem blocks overlap tracks */

    geoPtr->numHeads = labelPtr->numHeads;
    geoPtr->sectorsPerTrack = labelPtr->numSectors;

    /*
     * Figure out some basic parameters of the rotational set.  The number
     * of tracks in the set is either 2 or 4.  If 2, then the blocks on
     * successive tracks are skewed by 1/2 a filesystem block.  If 4,
     * blocks are skewed by 1/4 block.  A 4 track rotational set is best
     * becasue there are more rotational positions.  If, however, it
     * causes 2 or 3 wasted tracks at the end, or if blocks naturally
     * overlap by 1/2 block, then only 2 tracks per rotational set are
     * used.
     */
    switch(geoPtr->numHeads % 4) {
	case 0:
	case 1: {
	    extraSectors = geoPtr->sectorsPerTrack % DISK_SECTORS_PER_BLOCK;
	    if (extraSectors < DISK_SECTORS_PER_BLOCK/4) {
		/*
		 * Not enough extra sectors to overlap blocks onto the
		 * next track.  The blocks will fit evenly on a track,
		 * but the blocks on the following tracks will be skewed.
		 */
		tracksPerSet = 4;
		overlap = FALSE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/4;
	    } else if (extraSectors < DISK_SECTORS_PER_BLOCK/2) {
		/*
		 * Enough to overlap the first 1/4 block onto the next track.
		 */
		tracksPerSet = 4;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK * 3/4;
	    } else if (extraSectors < DISK_SECTORS_PER_BLOCK * 3/4) {
		/*
		 * Enough to overlap 1/2 block.
		 */
		tracksPerSet = 2;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/2;
	    } else {
		/*
		 * Enough to overlap 3/4 block.
		 */
		tracksPerSet = 4;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/4;
	    }
	    break;
	}
	case 2:
	case 3: {
	    /*
	     * Instead of wasting 2 or 3 tracks to have a 4 track rotational
	     * set, the rotational set is only 2 tracks long.  Also see if
	     * the blocks naturally overlap by 1/2 block.
	     */
	    tracksPerSet = 2;
	    offsetIncrement = DISK_SECTORS_PER_BLOCK/2;
	    if ((geoPtr->sectorsPerTrack % DISK_SECTORS_PER_BLOCK) <
		      DISK_SECTORS_PER_BLOCK/2) {
		overlap = FALSE;
	    } else {
		overlap = TRUE;
	    }
	}
    }
    if (!overlapBlocks) {
	overlap = FALSE;
	offsetIncrement = 0;
    }
    printf("overlap %s, offsetIncrement %d\n", (overlap ? "TRUE" : "FALSE"),
		      offsetIncrement);
    /*
     * Determine rotational position of the blocks in the rotational set.
     */
    extraSectors = geoPtr->sectorsPerTrack;
    startingOffset = 0;
    offset = startingOffset;
    for (numBlocks = 0, numTracks = 0 ; ; ) {
	if (extraSectors >= DISK_SECTORS_PER_BLOCK) {
	    /*
	     * Ok to fit in another filesystem block on this track.
	     */
	    geoPtr->blockOffset[numBlocks] = offset;
	    numBlocks++;	
	    offset += DISK_SECTORS_PER_BLOCK;
	    extraSectors -= DISK_SECTORS_PER_BLOCK;
	} else {
	    /*
	     * The current block has to take up room on the next track.
	     */
	    numTracks++;
	    if (numTracks < tracksPerSet) {
		/*
		 * Ok to go to the next track.
		 */
		startingOffset += offsetIncrement;
		if (overlap) {
		    /*
		     * If the current block can overlap to the next track,
		     * use the current offset.  Because of the overlap
		     * there are fewer sectors available for blocks on
		     * the next track.
		     */
		    geoPtr->blockOffset[numBlocks] = offset;
		    numBlocks++;
		    extraSectors = geoPtr->sectorsPerTrack - startingOffset;
		}
		offset = startingOffset + numTracks * geoPtr->sectorsPerTrack;
		if (!overlap) {
		    /*
		     * If no overlap the whole next track is available.
		     */
		    extraSectors = geoPtr->sectorsPerTrack;
		}
	    } else {
		/*
		 * Done.
		 */
		for (index = numBlocks; index < FSDM_MAX_ROT_POSITIONS; index++){
		    geoPtr->blockOffset[index] = -1;
		}
		break;
	    }
	}
    }
    geoPtr->blocksPerRotSet = numBlocks;
    geoPtr->tracksPerRotSet = tracksPerSet;
    geoPtr->rotSetsPerCyl = geoPtr->numHeads / tracksPerSet;
    geoPtr->blocksPerCylinder = numBlocks * geoPtr->rotSetsPerCyl;

    /*
     * Now the rotational positions have to be sorted so that rotationally
     * optimal blocks can be found.  The array sortedOffsets is set so
     * that the I'th element has the index into blockOffset which contains
     * the I'th rotational position, eg.
     *	blockOffset	sortedOffsets
     *	    0 (+0)		0
     *	    8 (+0)		2
     *	    4 (+17)		1
     *	   12 (+17)		3
     */

    offsetIncrement = DISK_SECTORS_PER_BLOCK / tracksPerSet;
    for (index = 0 ; index < FSDM_MAX_ROT_POSITIONS ; index++) {
	geoPtr->sortedOffsets[index] = -1;
    }
    for (index = 0 ; index < numBlocks ; index++) {
	offset = geoPtr->blockOffset[index] % geoPtr->sectorsPerTrack;
	geoPtr->sortedOffsets[offset/offsetIncrement] = index;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetDomainParts --
 *
 *	Set up the way the domain is divided into 4 areas:  the bitmap
 *	for the file descriptors, the file descriptors, the bitmap for
 *	the data blocks, and the data blocks.
 *
 * Results:
 *	The geometry information is completed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetDomainParts(labelPtr, partition, headerPtr)
    register Disk_Label		*labelPtr;	/* The disk label. */
    int partition;
    register Fsdm_DomainHeader *headerPtr;
{
    register Fsdm_Geometry *geoPtr;
    int numFiles;		/* Number of files computed from the size */
    int numBlocks;		/* Number of blocks in the partition.  This
				 * is computed and then dolled out to the
				 * different things stored in the partition */
    int offset;			/* Block offset within partition */
    int bitmapBytes;		/* Number of bytes taken up by a bitmap */
    int reservedBlocks;		/* Number of blocks reserved at beginning
				 * of partition */
    int maxSector;
    int numCylinders;

    /*
     * Set aside a number of blocks at the begining of the partition for
     * things like the super block, the boot program, and the domain header.
     */
    geoPtr = &headerPtr->geometry;
    maxSector = labelPtr->labelSector;
    maxSector = Max(maxSector,labelPtr->bootSector + labelPtr->numBootSectors);
    maxSector = Max(maxSector,labelPtr->summarySector
	+ labelPtr->numSummarySectors);
    maxSector = Max(maxSector,labelPtr->domainSector
	+ labelPtr->numDomainSectors);
    numCylinders = labelPtr->partitions[partition].numCylinders;
    if (scsiDisk) {
	/*
	 * For a scsi disk just reserve whole cylinders.
	 */
	int cylinders;
	cylinders = maxSector / (geoPtr->sectorsPerTrack * geoPtr->numHeads);
	if ((maxSector % (geoPtr->sectorsPerTrack * geoPtr->numHeads)) != 0) {
	    cylinders++;
	}
	reservedBlocks = cylinders * geoPtr->blocksPerCylinder;
	numBlocks = geoPtr->blocksPerCylinder * numCylinders - reservedBlocks;
     } else {
	/*
	 * If we are using rotational sets then reserve whole sets.
	 */
	int sets;
	sets = maxSector / (geoPtr->tracksPerRotSet * geoPtr->sectorsPerTrack);
	if ((maxSector % (geoPtr->tracksPerRotSet * geoPtr->sectorsPerTrack)) 
	!= 0) {
	    sets++;
	}
	reservedBlocks = sets * geoPtr->blocksPerRotSet;
	numBlocks = geoPtr->blocksPerCylinder * numCylinders - reservedBlocks;
    }
    printf("Reserving %d blocks for domain header, etc.\n", reservedBlocks);
    /*
     * Determine the number of filesystem blocks available and compute a
     * first guess at the number of file descriptors.  If at the end of
     * the computation things don't fit nicely, then the number of files
     * is changed and the computation is repeated.
     */
    numFiles = 0;
    do {
	if (numFiles == 0) {
	    numFiles = numBlocks * DISK_KBYTES_PER_BLOCK / kbytesToFileDesc;
	}
	numFiles		  &= ~(FSDM_FILE_DESC_PER_BLOCK-1);
	offset			  = reservedBlocks;

	headerPtr->fdBitmapOffset = offset;
	bitmapBytes		  = (numFiles - 1) / BITS_PER_BYTE + 1;
	headerPtr->fdBitmapBlocks = (bitmapBytes - 1) / FS_BLOCK_SIZE + 1;
	numBlocks		  -= headerPtr->fdBitmapBlocks;
	offset			  += headerPtr->fdBitmapBlocks;

	headerPtr->fileDescOffset = offset;
	headerPtr->numFileDesc	  = numFiles;
	numBlocks		  -= numFiles / FSDM_FILE_DESC_PER_BLOCK;
	offset			  += numFiles / FSDM_FILE_DESC_PER_BLOCK;
	/*
	 * The data blocks will start on a cylinder.  Try the next
	 * cylinder boundary after the start of the bitmap.
	 */
	headerPtr->bitmapOffset	  = offset;
	headerPtr->dataOffset	  = ((offset-1) / geoPtr->blocksPerCylinder + 1)
				     * geoPtr->blocksPerCylinder;
	headerPtr->dataBlocks	  = headerPtr->numCylinders *
				      geoPtr->blocksPerCylinder -
				      headerPtr->dataOffset;
	bitmapBytes		  = (headerPtr->dataBlocks * DISK_KBYTES_PER_BLOCK -
				       1) / BITS_PER_BYTE + 1;
	headerPtr->bitmapBlocks	  = (bitmapBytes - 1) / FS_BLOCK_SIZE + 1;
	/*
	 * Check the size of the bit map against space available for it
	 * between the end of the file descriptors and the start of the
	 * data blocks.
	 */
	if (headerPtr->dataOffset - headerPtr->bitmapOffset <
	    headerPtr->bitmapBlocks) {
	    int numBlocksNeeded;
	    /*
	     * Need more blocks to hold the bitmap.  Reduce the number
	     * of file descriptors to get the blocks and re-iterate.
	     */
	    numBlocksNeeded = headerPtr->bitmapBlocks -
		(headerPtr->dataOffset - headerPtr->bitmapOffset);
	    numFiles -= numBlocksNeeded * FSDM_FILE_DESC_PER_BLOCK;
	} else if (headerPtr->dataOffset - headerPtr->bitmapOffset >
		    headerPtr->bitmapBlocks) {
	    int extraBlocks;
	    /*
	     * There are extra blocks between the end of the file descriptors
	     * and the start of the bitmap.  Increase the number of
	     * file descriptors and re-iterate.
	     */
	    extraBlocks = headerPtr->dataOffset - headerPtr->bitmapOffset -
		    headerPtr->bitmapBlocks;
	    numFiles += extraBlocks * FSDM_FILE_DESC_PER_BLOCK;
	}
    } while (headerPtr->dataOffset - headerPtr->bitmapOffset !=
		headerPtr->bitmapBlocks);
    headerPtr->dataCylinders	= headerPtr->dataBlocks /
				  geoPtr->blocksPerCylinder ;
}
/*
 *----------------------------------------------------------------------
 *
 * SetSummaryInfo --
 *
 *	This routine is out of date and writes bogus info to the disk.
 *	Fscheck should fix the data anyway.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Fill in the summary info.
 *
 *----------------------------------------------------------------------
 */
void
SetSummaryInfo(labelPtr, headerPtr, summaryPtr)
    Disk_Label *labelPtr;	/* Information from the super block */
    Fsdm_DomainHeader *headerPtr;	/* Domain header to summarize */
    Fsdm_SummaryInfo *summaryPtr;	/* Reference to summary info to fill in */
{

    bzero((Address)summaryPtr, DEV_BYTES_PER_SECTOR);

    strcpy(summaryPtr->domainPrefix, "(new domain)");
    /*
     * 9 blocks are already allocated, one for the root directory,
     * and 8 more for lost+found.
     */
    summaryPtr->numFreeKbytes = headerPtr->dataBlocks * (FS_BLOCK_SIZE / 1024)
				- 9;
    /*
     * 4 file descriptors are already used, 0 and 1 are reserved,
     * 2 is for the root, and 3 is for lost+found.
     */
    summaryPtr->numFreeFileDesc = headerPtr->numFileDesc - 5;
    if (makeFile) {
	summaryPtr->numFreeFileDesc--;
    }
    /*
     * The summary state field is unused.
     */
    summaryPtr->state = 0;
    /*
     * The domain number under which this disk partition is mounted is
     * recorded on disk so servers re-attach disks under the same "name".
     * We set it to the special value so it gets a new number assigned
     * when it is first attached.
     */
    summaryPtr->domainNumber = -1;
    /*
     * The flags field is used to record whether or not the disk has been
     * safely "sync"ed to disk upon shutdown.
     */
    summaryPtr->flags = 0;
    summaryPtr->fixCount = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteFileDesc --
 *
 *	Write out the file descriptor array to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteFileDesc(headerPtr, partFID)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;	/* File handle for partition to format */
{
    ReturnStatus status;
    char *bitmap;
    char *block;
    register Fsdm_FileDescriptor *fileDescPtr;
    register int index;

    bitmap = MakeFileDescBitmap(headerPtr);
    if (!printOnly) {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->fdBitmapOffset,
				headerPtr->fdBitmapBlocks, (Address)bitmap);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    /*
     * Make the first block of file descriptors.  This contains some
     * canned file descriptors for the root, bad block file, and the
     * lost and found directory.  For (early system) testing an empty file
     * can also be created.
     */
    block = (char *)malloc(FS_BLOCK_SIZE);
    bzero(block, FS_BLOCK_SIZE);
    for (index = 0;
         index < FSDM_FILE_DESC_PER_BLOCK;
	 index++ ) {
	fileDescPtr = (Fsdm_FileDescriptor *)((int)block +
					   index * FSDM_MAX_FILE_DESC_SIZE);
	fileDescPtr->magic = FSDM_FD_MAGIC;
	if (index < FSDM_BAD_BLOCK_FILE_NUMBER) {
	    fileDescPtr->flags = FSDM_FD_RESERVED;
	} else if (index == FSDM_BAD_BLOCK_FILE_NUMBER) {
	    SetBadBlockFileDescriptor(fileDescPtr);
	} else if (index == FSDM_ROOT_FILE_NUMBER) {
	    SetRootFileDescriptor(fileDescPtr);
	} else if (index == FSDM_LOST_FOUND_FILE_NUMBER) {
	    SetLostFoundFileDescriptor(fileDescPtr);
	} else if ((index == 4) && makeFile) {
	    SetEmptyFileDescriptor(fileDescPtr);
	} else if ((index == 5)) {
	    SetEmptyFileDescriptor(fileDescPtr);
	} else if ((index == 6)) {
	    SetTooBigFD(fileDescPtr);
	} else if ((index == 7)) {
	    SetTooSmallFD(fileDescPtr);
	} else if ((index == 8)) {
	    SetHoleFileFD(headerPtr, partFID, fileDescPtr);
	} else if ((index == 9)) {
	    SetHoleDirFD(headerPtr, partFID, fileDescPtr);
	} else if ((index == 10)) {
	    SetBadEntryFileFD(headerPtr, partFID, fileDescPtr);
	} else if ((index == 11)) {
	    SetFragFileFD(fileDescPtr);	
	} else if ((index == 12)) {
	    SetCopyFragFileFD(fileDescPtr);	
	} else if ((index == 13)) {
	    SetCopyBlockFileFD(fileDescPtr);	
	} else if ((index == 14)) {
	    SetCopyIndBlockFileFD(headerPtr, partFID, fileDescPtr);	
	} else if ((index == 15)) {
	    SetCopyBogusIndBlockFileFD(headerPtr, partFID, fileDescPtr);	
	} else if ((index == 16)) {
	    fileDescPtr->magic = ~FSDM_FD_MAGIC;
	} else if ((index == 17)) {
	    SetDirFD(fileDescPtr);
	} else if ((index == 18)) {
	    SetOutputFD(fileDescPtr);
	} else if ((index == 21)) {
	    SetBadDotDotFD(fileDescPtr);
	} else {
	    fileDescPtr->flags = FSDM_FD_FREE;
	}
    }
    if (!printOnly) {
	/*
	 * Write out the first, specially hand crafted, block of file
	 * descriptors.
	 */
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->fileDescOffset,
				    1, (Address)block);
	if (status != SUCCESS) {
	    return(status);
	}
	/*
	 * Redo the block for the remaining file descriptors
	 */
	bzero(block, FS_BLOCK_SIZE);
	for (index = 0;
	     index < FSDM_FILE_DESC_PER_BLOCK;
	     index++ ) {
	    fileDescPtr = (Fsdm_FileDescriptor *)((int)block + index *
					       FSDM_MAX_FILE_DESC_SIZE);
	    fileDescPtr->magic = FSDM_FD_MAGIC;
	    fileDescPtr->flags = FSDM_FD_FREE;
	}
	/*
	 * Write out the remaining file descriptors.
	 */
	for (index = FSDM_FILE_DESC_PER_BLOCK;
	     index < headerPtr->numFileDesc;
	     index += FSDM_FILE_DESC_PER_BLOCK) {
	    status = Disk_BlockWrite(partFID, headerPtr,
		     headerPtr->fileDescOffset + (index/FSDM_FILE_DESC_PER_BLOCK),
		     1, (Address)block);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
    } else {
	status = SUCCESS;
    }
    return(status);
}

/*
 * This macro makes setting bits in the fd and data block bitmap easier.
 */

#define SET(num)  {   bitmap[(num) >> 3] |=(1 << (7 - ((num) & 0x7))); }


/*
 *----------------------------------------------------------------------
 *
 * 
 * MakeFileDescBitmap --
 *
 *	Compute out the bitmap for file descriptor array to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
MakeFileDescBitmap(headerPtr)
    register Fsdm_DomainHeader *headerPtr;
{
    register char *bitmap;
    register int index;

    /*
     * Allocate and initialize the bitmap to all 0"s to mean all free.
     */
    bitmap = (char *)malloc((unsigned) headerPtr->fdBitmapBlocks *
				 FS_BLOCK_SIZE);
    bzero((Address)bitmap, headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE);

    SET(0);
    SET(1);
    if (!noRoot) {
	SET(2);
    }
    SET(3);
    SET(4);
    SET(6);
    SET(7);
    SET(8);
    SET(9);
    SET(10);
    SET(11);
    SET(12);
    SET(13);
    SET(14);
    SET(15);
    /*
     * 16 is allocated but not marked in the bitmap.
     */
    SET(17);
    SET(18);
    /*
     * 20 is marked in the bitmap but not used.
     */
    SET(20);

    SET(21);
    /*
     * Don't set 22 -- it is used by directory 21. 
     */
    /*
     * Set the bits in the map at the end that don't correspond to
     * any existing file descriptors.
     */
    index = headerPtr->numFileDesc / BITS_PER_BYTE;
    if (headerPtr->numFileDesc % BITS_PER_BYTE) {
	register int bitIndex;
	/*
	 * Take care the last byte that only has part of its bits set.
	 */
	for (bitIndex = headerPtr->numFileDesc % BITS_PER_BYTE;
	     bitIndex < BITS_PER_BYTE;
	     bitIndex++) {
	    bitmap[index] |= 1 << ((BITS_PER_BYTE - 1) - bitIndex);
	}
	index++;
    }
    for ( ; index < headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE; index++) {
	bitmap[index] = 0xff;
    }

    if (printOnly) {
	Disk_PrintFileDescBitmap(headerPtr, bitmap);
    }
    return(bitmap);
}

/* 
 * This macro makes setting an entire block (4 fragments) easier. The block
 * to set must be on a block boundary.
 */

#define SET4K(num)  {   bitmap[(num) >> 3] |=(0xf << (4 - ((num) & 0x7))); }


/*
 *----------------------------------------------------------------------
 *
 * WriteBitmap --
 *
 *	Write out the bitmap for the data blocks.  This knows that the
 *	first 1K fragment is allocated to the root directory and 8K is
 *	allocated to lost and found, plus lots of other blocks are allocated
 *	to the rest of the files.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the bitmap.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteBitmap(headerPtr, partFID)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    ReturnStatus status;
    char *bitmap;
    int kbytesPerCyl;
    int bitmapBytesPerCyl;
    int index;

    bitmap = (char *)malloc((unsigned) headerPtr->bitmapBlocks * FS_BLOCK_SIZE);
    bzero(bitmap, headerPtr->bitmapBlocks * FS_BLOCK_SIZE);
    /*
     * Set the bit corresponding to the kbyte used for the root directory
     * and the next 8K reserved for lost and found.
     *   ________
     *	|0______7|	Bits are numbered like this in a byte.
     *
     * See fstrash.man for info on how these blocks are used
     */
    if (!noRoot) {
	SET(0);
    }
    SET(1);
    SET4K(4);
    SET(8);
    SET(9);

    SET(11);
    SET4K(12);
    SET(16);
    SET(17);

    SET4K(32);
    SET4K(36);

    SET4K(44);
    SET4K(48);
    SET4K(52);
    SET4K(56);

    SET(61);
    SET(62);

    SET(64);

    SET4K(68);
    SET4K(72);

    SET(76);

    /*
     * The bitmap is organized by cylinder.  There are whole number of
     * bytes in the bitmap for each cylinder.  Each bit in the bitmap
     * corresponds to 1 kbyte on the disk.
     */
    kbytesPerCyl = headerPtr->geometry.blocksPerCylinder * DISK_KBYTES_PER_BLOCK;
    bitmapBytesPerCyl = (kbytesPerCyl - 1) / BITS_PER_BYTE + 1;
    if ((kbytesPerCyl % BITS_PER_BYTE) != 0) {
	/*
	 * There are bits in the last byte of the bitmap for each cylinder
	 * that don't have kbytes behind them.  Set those bits here so
	 * the blocks don't get allocated.
	 */
	register int extraBits;
	register int mask;

	extraBits = kbytesPerCyl % BITS_PER_BYTE;
	/*
	 * Set up a mask that has the right part filled with 1"s.
	 */
	mask = 0x0;
	for ( ; extraBits < BITS_PER_BYTE ; extraBits++) {
	    mask |= 1 << ((BITS_PER_BYTE - 1) - extraBits);
	}
	for (index = 0;
	     index < headerPtr->dataBlocks * DISK_KBYTES_PER_BLOCK / BITS_PER_BYTE;
	     index += bitmapBytesPerCyl) {
	    bitmap[index + bitmapBytesPerCyl - 1] |= mask;
	}
    }
    /*
     * Set the bits in the bitmap that correspond to non-existent cylinders;
     * the bitmap is allocated a whole number of blocks on the disk
     * so there are bytes at its end that don't have blocks behind them.
     */
 
    for (index = headerPtr->dataCylinders * bitmapBytesPerCyl;
	 index < headerPtr->bitmapBlocks * FS_BLOCK_SIZE;
	 index++) {
	bitmap[index] = 0xff;
    }
    if (printOnly) {
	Disk_PrintDataBlockBitmap(headerPtr, bitmap);
	status = SUCCESS;
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->bitmapOffset,
			    headerPtr->bitmapBlocks, (Address)bitmap);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteRootDirectory --
 *
 *	Write the data blocks of the root directory.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the root directory"s data block.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteRootDirectory(headerPtr, partFID)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    ReturnStatus status;
    char block[FS_BLOCK_SIZE];
    Fslcl_DirEntry *dirEntryPtr;
    int offset;
    int i;

    bzero((Address) block, FS_BLOCK_SIZE);

    if (!noRoot) {

	dirEntryPtr = (Fslcl_DirEntry *)block;
    
	offset = FillDirEntry(dirEntryPtr,".",FSDM_ROOT_FILE_NUMBER);

	dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"..",FSDM_ROOT_FILE_NUMBER);
    
	/*
	 * Add lost and found.
	 */
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"lost+found",
			       FSDM_LOST_FOUND_FILE_NUMBER);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"testFile",4);
    
	/*
	 * file 5 is an unreferenced file
	 */
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"tooBig",6);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"tooSmall",7);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"holeFile",8);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"holeDir",9);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"badIndexFile",10);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"illegalFrag",11);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"copyBadFrag",12);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"copyBlock",13);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"copyIndBlock",14);
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"copyBogusInd",15);
	/*
	 * 16 and 17 are unreferenced.
	 */
    
	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"output",18);

	dirEntryPtr = (Fslcl_DirEntry *) ((int)block + offset);
	offset += FillDirEntry(dirEntryPtr,"badDotDot",21);

	dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - offset +
				    dirEntryPtr->recordLength;

    for (dirEntryPtr = (Fslcl_DirEntry *)&block[FSLCL_DIR_BLOCK_SIZE], i = 1; 
	 i < FS_BLOCK_SIZE / FSLCL_DIR_BLOCK_SIZE;
	 i++,dirEntryPtr=(Fslcl_DirEntry *)((int)dirEntryPtr + FSLCL_DIR_BLOCK_SIZE)) {
	 dirEntryPtr->fileNumber = 0;
	 dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	 dirEntryPtr->nameLength = 0;
    }

    }
    if (printOnly) {
	printf("Root Directory\n");
	offset = 0;
	dirEntryPtr = (Fslcl_DirEntry *)block;
	while (offset + dirEntryPtr->recordLength != FSLCL_DIR_BLOCK_SIZE) {
	    Disk_PrintDirEntry(dirEntryPtr);
	    offset += dirEntryPtr->recordLength;
	    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	}
	status = SUCCESS;
    } else {
	/*
	 * This write trounces the data beyond the stuff allocated to
	 * the root directory.  Currently this is ok and is done because
	 * BlockWrite writes whole numbers of filesystem blocks.
	 */
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset, 1,
				       block);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteLostFoundDirectory --
 *
 *	Write the data blocks of the lost and found directory.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the root directory"s data block.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteLostFoundDirectory(headerPtr, partFID)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    ReturnStatus status;
    char *block;
    Fslcl_DirEntry *dirEntryPtr;
    char *fileName;
    int length;
    int offset;
    int	i;

    block = (char *)malloc(FSDM_NUM_LOST_FOUND_BLOCKS * FS_BLOCK_SIZE);
    dirEntryPtr = (Fslcl_DirEntry *)block;

    fileName = ".";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = FSDM_LOST_FOUND_FILE_NUMBER;
    dirEntryPtr->recordLength = Fslcl_DirRecLength(length);
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    offset = dirEntryPtr->recordLength;

    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
    fileName = "..";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = FSDM_ROOT_FILE_NUMBER;
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - offset;
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);

    /*
     * Fill out the rest of the directory with empty blocks.
     */

    for (dirEntryPtr = (Fslcl_DirEntry *)&block[FSLCL_DIR_BLOCK_SIZE], i = 1; 
	 i < FSDM_NUM_LOST_FOUND_BLOCKS * FS_BLOCK_SIZE / FSLCL_DIR_BLOCK_SIZE;
	 i++,dirEntryPtr=(Fslcl_DirEntry *)((int)dirEntryPtr + FSLCL_DIR_BLOCK_SIZE)) {
	 dirEntryPtr->fileNumber = 0;
	 dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	 dirEntryPtr->nameLength = 0;
    }
    if (printOnly) {
	printf("Lost+found Directory\n");
	offset = 0;
	dirEntryPtr = (Fslcl_DirEntry *)block;
	Disk_PrintDirEntry(dirEntryPtr);
	offset += dirEntryPtr->recordLength;
	dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	Disk_PrintDirEntry(dirEntryPtr);
	status = SUCCESS;
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 1,
			    FSDM_NUM_LOST_FOUND_BLOCKS, block);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteEmptyDirectory --
 *
 *	Write the data blocks of the empty directory (file 17).
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteEmptyDirectory(headerPtr, partFID, selfFdNum, parentFdNum, blockNum)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;
    int selfFdNum;
    int parentFdNum;
    int blockNum;
{
    ReturnStatus status;
    char *block;
    Fslcl_DirEntry *dirEntryPtr;
    char *fileName;
    int length;
    int offset;
    int	i;

    block = (char *)malloc(FS_BLOCK_SIZE);
    dirEntryPtr = (Fslcl_DirEntry *)block;

    fileName = ".";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = selfFdNum;
    dirEntryPtr->recordLength = Fslcl_DirRecLength(length);
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    offset = dirEntryPtr->recordLength;

    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
    fileName = "..";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = parentFdNum;
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - offset;
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);

    /*
     * Fill out the rest of the directory with empty blocks.
     */

    for (dirEntryPtr = (Fslcl_DirEntry *)&block[FSLCL_DIR_BLOCK_SIZE], i = 1; 
	 i < FS_BLOCK_SIZE / FSLCL_DIR_BLOCK_SIZE;
	 i++,dirEntryPtr=(Fslcl_DirEntry *)((int)dirEntryPtr + FSLCL_DIR_BLOCK_SIZE)) {
	 dirEntryPtr->fileNumber = 0;
	 dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	 dirEntryPtr->nameLength = 0;
    }
    if (printOnly) {
	printf("empty Directory\n");
	offset = 0;
	dirEntryPtr = (Fslcl_DirEntry *)block;
	Disk_PrintDirEntry(dirEntryPtr);
	offset += dirEntryPtr->recordLength;
	dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	Disk_PrintDirEntry(dirEntryPtr);
	status = SUCCESS;
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 
				(blockNum / FS_FRAGMENTS_PER_BLOCK),
				1, block);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteBadDotDotDirectory --
 *
 *	Write a directory whose ".." points to an unallocated file.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
WriteBadDotDotDirectory(headerPtr, partFID, selfFdNum, parentFdNum, blockNum)
    register Fsdm_DomainHeader *headerPtr;
    int partFID;
    int selfFdNum;
    int parentFdNum;
    int blockNum;
{
    ReturnStatus status;
    char *block;
    Fslcl_DirEntry *dirEntryPtr;
    char *fileName;
    int length;
    int offset;
    int	i;

    block = (char *)malloc(FS_BLOCK_SIZE);
    dirEntryPtr = (Fslcl_DirEntry *)block;

    fileName = ".";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = selfFdNum;
    dirEntryPtr->recordLength = Fslcl_DirRecLength(length);
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    offset = dirEntryPtr->recordLength;

    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
    fileName = "..";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = selfFdNum + 1;
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - offset;
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);

    /*
     * Fill out the rest of the directory with empty blocks.
     */

    for (dirEntryPtr = (Fslcl_DirEntry *)&block[FSLCL_DIR_BLOCK_SIZE], i = 1; 
	 i < FS_BLOCK_SIZE / FSLCL_DIR_BLOCK_SIZE;
	 i++,dirEntryPtr=(Fslcl_DirEntry *)((int)dirEntryPtr + FSLCL_DIR_BLOCK_SIZE)) {
	 dirEntryPtr->fileNumber = 0;
	 dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	 dirEntryPtr->nameLength = 0;
    }
    if (printOnly) {
	printf("empty Directory\n");
	offset = 0;
	dirEntryPtr = (Fslcl_DirEntry *)block;
	Disk_PrintDirEntry(dirEntryPtr);
	offset += dirEntryPtr->recordLength;
	dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	Disk_PrintDirEntry(dirEntryPtr);
	status = SUCCESS;
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 
				(blockNum / FS_FRAGMENTS_PER_BLOCK),
				1, block);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 *  FillDirEntry--
 *
 *	Fills in the directory entry.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
int
FillDirEntry(dirEntryPtr, fileName, fileNum)
    Fslcl_DirEntry *dirEntryPtr;
    char *fileName;
    int fileNum;
{
    int length;

    length = strlen(fileName);
    dirEntryPtr->fileNumber = fileNum;
    dirEntryPtr->recordLength = Fslcl_DirRecLength(length);
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    return dirEntryPtr->recordLength;
}
