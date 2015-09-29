/* 
 * diskPrint.c --
 *
 *	Routines to print out data structures found on the disk.
 *
 * Copyright (C) 1987 Regents of the University of California
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
static char rcsid[] = "$Header: /sprite/src/lib/disk/RCS/diskPrint.c,v 1.8 92/08/31 12:20:44 voelker Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h> 
#include "disk.h"


/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintDomainHeader --
 *
 *	Print out the domain header.  Used in testing.
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
Disk_PrintDomainHeader(headerPtr)
    register Ofs_DomainHeader *headerPtr;/* Reference to domain header to print*/
{
    register Ofs_Geometry *geoPtr;
    register int	index;

    printf("Domain Header <%x>\n", headerPtr->magic);
    printf("First Cyl %d, num Cyls %d", headerPtr->firstCylinder,
		    headerPtr->numCylinders);
    printf(", raw size %d kbytes\n", headerPtr->numCylinders *
		headerPtr->geometry.sectorsPerTrack *
		headerPtr->geometry.numHeads / 2);
    printf("%-20s %10s %10s\n", "", "offset", "blocks");
    printf("%-20s %10d %10d\n", "FD Bitmap", headerPtr->fdBitmapOffset,
		    headerPtr->fdBitmapBlocks);
    printf("%-20s %10d %10d %10d\n", "File Desc", headerPtr->fileDescOffset,
		    headerPtr->numFileDesc/FSDM_FILE_DESC_PER_BLOCK,
		    headerPtr->numFileDesc);
    printf("%-20s %10d %10d\n", "Bitmap", headerPtr->bitmapOffset,
		    headerPtr->bitmapBlocks);
    printf("%-20s %10d %10d\n", "Data Blocks", headerPtr->dataOffset,
		    headerPtr->dataBlocks);
    geoPtr = &headerPtr->geometry;
    printf("Geometry\n");
    printf("sectorsPerTrack %d, numHeads %d\n", geoPtr->sectorsPerTrack,
			      geoPtr->numHeads);
    printf("blocksPerRotSet %d, tracksPerRotSet %d\n",
			   geoPtr->blocksPerRotSet, geoPtr->tracksPerRotSet);
    printf("rotSetsPerCyl %d, blocksPerCylinder %d\n",
			   geoPtr->rotSetsPerCyl, geoPtr->blocksPerCylinder);
    printf("Offset	(Sorted)\n");
    for (index = 0 ; index < geoPtr->blocksPerRotSet ; index++) {
	printf("%8d %8d\n", geoPtr->blockOffset[index],
		       geoPtr->sortedOffsets[index]);
    }

    printf(">> %d files, %d kbytes\n", headerPtr->numFileDesc,
		headerPtr->dataBlocks * 4);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintSummaryInfo --
 *
 *	Print out the summary information.
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
Disk_PrintSummaryInfo(summaryPtr)
    register Ofs_SummaryInfo *summaryPtr; /* Reference to summary info to print */
{
    printf("\"%s\" (%d) \t%d Kbytes free, %d file descriptors free\n",
	    summaryPtr->domainPrefix, 
	    summaryPtr->domainNumber,
	    summaryPtr->numFreeKbytes,
	    summaryPtr->numFreeFileDesc);
    printf("Attach seconds: %d\n");
    printf("Detach seconds: %d\n");
    if (summaryPtr->flags & OFS_DOMAIN_NOT_SAFE) {
	printf("OFS_DOMAIN_NOT_SAFE\n");
    }
    if (summaryPtr->flags & OFS_DOMAIN_ATTACHED_CLEAN) {
	printf("OFS_DOMAIN_ATTACHED_CLEAN\n");
    }
    if (summaryPtr->flags & OFS_DOMAIN_TIMES_VALID) {
	printf("OFS_DOMAIN_TIMES_VALID\n");
    }
    if (summaryPtr->flags & OFS_DOMAIN_JUST_CHECKED) {
	printf("OFS_DOMAIN_JUST_CHECKED\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintFileDescBitmap --
 *
 *	Print out the file descriptor bitmap.
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
Disk_PrintFileDescBitmap(headerPtr, bitmap)
    Ofs_DomainHeader	*headerPtr;	/* Pointer to disk header info. */
    char		*bitmap;	/* Pointer to file desc bit map. */
{
    register int index;

    printf("File Descriptor bitmap\n");
    for (index = 0;
	 index < headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE;) {
	if ((index % 32) == 0) {
	    printf("%6d ", index * BITS_PER_BYTE);
	    if (index * BITS_PER_BYTE > headerPtr->numFileDesc) {
		printf(" (The rest of the map is not used)\n");
		break;
	    }
	}
	printf("%02x", bitmap[index] & 0xff);
	index++;
	if ((index % 32) == 0) {
	    printf("\n");
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintDataBlockBitmap --
 *
 *	Print out the data block bitmap.
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
Disk_PrintDataBlockBitmap(headerPtr, bitmap)
    Ofs_DomainHeader	*headerPtr;	/* Ptr to disk header info. */
    char		*bitmap;	/* Ptr to data block bit map. */
{
    register int index;

    printf("Data block bitmap:\n");
    for (index = 0;
	 index < headerPtr->bitmapBlocks * FS_BLOCK_SIZE;) {
	if ((index % 32) == 0) {
	    printf("%6d ", index * BITS_PER_BYTE);
	    if (index * BITS_PER_BYTE >
		headerPtr->dataBlocks * DISK_KBYTES_PER_BLOCK) {
		printf(" (The rest of the bitmap is not used)\n");
		break;
	    }
	}
	printf("%02x", bitmap[index] & 0xff);
	index++;
	if ((index % 32) == 0) {
	    printf("\n");
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintDirEntry --
 *
 *	Print out one directory entry
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
Disk_PrintDirEntry(dirEntryPtr)
    Fslcl_DirEntry *dirEntryPtr;	/* Ptr to directory entry. */
{
    printf("\"%-15s\", File Number = %d, Rec Len = %d, Name Len = %d\n",
		   dirEntryPtr->fileName, dirEntryPtr->fileNumber,
		   dirEntryPtr->recordLength, dirEntryPtr->nameLength);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLabel--
 *
 *	Print the contents of a Disk_Label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed.
 *
 *----------------------------------------------------------------------
 */

void
Disk_PrintLabel(labelPtr)
    Disk_Label		*labelPtr;	/* Label to be printed.*/
{
    int	i;
    int first, last, cyls;

    printf("Ascii label: \"%s\"\n", labelPtr->asciiLabel);
    printf("Length of ascii label: %d\n", labelPtr->asciiLabelLen);
    printf("The disk has a %s label.\n", 
	Disk_GetLabelTypeName(labelPtr->labelType));
    printf("The disk label is in sector %d\n", labelPtr->labelSector);
    printf("Number of heads: %d\n", labelPtr->numHeads);
    printf("Number of sectors per track: %d\n", labelPtr->numSectors);
    printf("Number of cylinders: %d\n", labelPtr->numCylinders);
    printf("Number of alternate cylinders: %d\n", labelPtr->numAltCylinders);
    printf("Starting boot sector: %d\n", labelPtr->bootSector);
    printf("Number of boot sectors: %d\n", labelPtr->numBootSectors);
    printf("Start of summary info: %d\n", labelPtr->summarySector);
    printf("Number of summary sectors: %d\n", labelPtr->numSummarySectors);
    printf("Start of domain header: %d\n", labelPtr->domainSector);
    printf("Number of domain header sectors: %d\n", labelPtr->numDomainSectors);
    printf("Number of disk partitions: %d\n", labelPtr->numPartitions);
    printf("Partition map:\n");
    for (i = 0; i < labelPtr->numPartitions; i++) {
	first = labelPtr->partitions[i].firstCylinder;
	cyls = labelPtr->partitions[i].numCylinders;
	last = (cyls > 0) ? (cyls + first - 1) : first;
	printf("%c: First %4d Last %4d Num %4d (%7d sectors)\n",
	    i + 'a', first, last, cyls,
	    cyls *  labelPtr->numHeads * labelPtr->numSectors);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_GetLabelTypeName --
 *
 *	Returns a string which is the name of the machine-specific
 *	label type.
 *
 * Results:
 *	Character string name of label type if the type parameter is
 *	valid, NULL otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Disk_GetLabelTypeName(labelType)
    Disk_NativeLabelType labelType;	/* Type of machine specific label. */
{
    static char *names[3] = {"NO LABEL", "Sun", "Dec"};

    if (labelType < 0 || labelType > 2) {
	return NULL;
    }
    return names[labelType];
}


/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsSuperBlockHdr
 *
 *	Print out the contents of the LFS SuperBlock header
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsSuperBlockHdr(headerPtr)
    LfsSuperBlockHdr *headerPtr;
{
    if (headerPtr == NULL) {
	return;
    }
    printf("LFS SuperBlock magic:                0x%x\n", headerPtr -> magic);
    printf("LFS version:                         %d\n", headerPtr -> version);
    printf("LFS block size in bytes:             %d\n", 
	   headerPtr -> blockSize);
    printf("Maximum number of checkpoint blocks: %d\n", 
	   headerPtr -> maxCheckPointBlocks);
    printf("Checkpoint #0 offset:                %d\n", 
	   headerPtr -> checkPointOffset[0]);
    printf("Checkpoint #1 offset:                %d\n", 
	   headerPtr -> checkPointOffset[1]);
    printf("Segemented log offset:               %d\n", 
	   headerPtr -> logStartOffset);
    printf("Frequency of checkpoint in seconds:  %d\n",
	   headerPtr -> checkpointInterval);
    printf("Maximum number of blocks to clean at one time: %d\n",
	   headerPtr -> maxNumCacheBlocks);
    printf("Unique ID for file system:           %d\n",
	   headerPtr -> domainUID);
    printf("Partition that the file system was built in: %c\n",
	   headerPtr -> partition);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsStableMemParams
 *
 *	Print out the contents of a LFS StableMemParam structure
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsStableMemParams(stableMemPtr)
    LfsStableMemParams *stableMemPtr;
{
    if (stableMemPtr == NULL) {
	return;
    }
    printf("Stable memory type:          %d\n", stableMemPtr -> memType);
    printf("Block size in bytes:         %d\n", stableMemPtr -> blockSize);
    printf("Size of each entry in bytes: %d\n", stableMemPtr -> entrySize);
    printf("Maximum number of entries:   %d\n", stableMemPtr -> maxNumEntries);
    printf("Number of entries per block: %d\n", 
	   stableMemPtr -> entriesPerBlock);
    printf("Maximum nmber of blocks:     %d\n", stableMemPtr -> maxNumBlocks);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsDescMapParams
 *
 *	Print out the contents of a LFS DescMapParams structure
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsDescMapParams(descMapPtr)
    LfsDescMapParams *descMapPtr;
{
    if (descMapPtr == NULL) {
	return;
    }
    printf("Version number:                 %d\n", descMapPtr -> version);
    printf("Maximum size in descriptor map: %d\n\n", descMapPtr -> maxDesc);
    printf("Index Parameters for the descriptor map:\n");
    Disk_PrintLfsStableMemParams(&(descMapPtr -> stableMem));
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsSegUsageParams
 *
 *	Print out the contents of a LFS SegUsageParams structure
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsSegUsageParams(segUsagePtr)
    LfsSegUsageParams *segUsagePtr;
{
    if (segUsagePtr == NULL) {
	return;
    }
    printf("Number of bytes in each segment:            %d\n", 
	   segUsagePtr -> segmentSize);
    printf("Number of segments in file system:          %d\n", 
	   segUsagePtr -> numberSegments);
    printf("Minimum number of clean segments allowed:   %d\n", 
	   segUsagePtr -> minNumClean);
    printf("Minimum number of free blocks allowed:      %d\n", 
	   segUsagePtr -> minFreeBlocks);
    printf("Number of allowed waste blocks per segment: %d\n", 
	   segUsagePtr -> wasteBlocks);
    printf("Number of segments to clean at a time:      %d\n\n", 
	   segUsagePtr -> numSegsToClean);
    printf("Index Parameters for the Segment Usage Map:\n");
    Disk_PrintLfsStableMemParams(&(segUsagePtr -> stableMem));
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsFileLayoutParams
 *
 *	Print out the contents of a LFS FileLayoutParams structure
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsFileLayoutParams(fileLayoutPtr)
    LfsFileLayoutParams *fileLayoutPtr;
{
    if (fileLayoutPtr == NULL) {
	return;
    }
    printf("Number of file descriptors to pack per block: %d\n",
	   fileLayoutPtr -> descPerBlock);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsSuperBlock
 *
 *	Print out the contents of a LFS SuperBlock
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsSuperBlock(superPtr)
    LfsSuperBlock *superPtr;
{
    if (superPtr == NULL) {
	return;
    }
    printf("SuperBlock Header:\n");
    Disk_PrintLfsSuperBlockHdr(&(superPtr -> hdr));
    printf("\nDescriptor Map Parameters:\n");
    Disk_PrintLfsDescMapParams(&(superPtr -> descMap));
    printf("\nSegment Usage Map Parameters:\n");
    Disk_PrintLfsSegUsageParams(&(superPtr -> usageArray));
    printf("\nFile Layout Parameters:\n");
    Disk_PrintLfsFileLayoutParams(&(superPtr -> fileLayout));
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsCheckPointHdr
 *
 *	Print out the contents of a LFS CheckPointHdr
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsCheckPointHdr(headerPtr)
    LfsCheckPointHdr *headerPtr;
{
    if (headerPtr == NULL) {
	return;
    }
    printf("Timestamp of the checkpoint:      %d\n", headerPtr -> timestamp);
    printf("Size of the checkpoint in bytes:  %d\n", headerPtr -> size);
    printf("Region write version number:      %d\n", headerPtr -> version);
    printf("Last prefix used for this domain: %s\n", headerPtr -> 
	   domainPrefix);
    printf("Last domain run under:            %d\n", headerPtr -> 
	   domainNumber);
    printf("Time the disk was attached:       %d\n", headerPtr -> 
	   attachSeconds);
    printf("Time the disk was off-lined:      %d\n", headerPtr -> 
	   detachSeconds);
    printf("Sprite ID of server:              %d\n", headerPtr -> serverID);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsCheckPointRegion
 *
 *	Print out the contents of a LFS CheckPointRegion
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsCheckPointRegion(regionPtr)
    LfsCheckPointRegion *regionPtr;
{
    if (regionPtr == NULL) {
	return;
    }
    printf("Region type:             %s\n", 
	   (regionPtr -> type == LFS_FILE_LAYOUT_MOD) ? "LFS_FILE_LAYOUT_MOD" :
	   ((regionPtr -> type == LFS_DESC_MAP_MOD) ? "LFS_DESC_MAP_MOD" :
	   "LFS_SEG_USAGE_MOD"));
    printf("Size of region in bytes: %d\n", regionPtr -> size);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_PrintLfsCheckPointTrailer
 *
 *	Print out the contents of a LFS CheckPointTrailer
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to stdout.
 *
 *----------------------------------------------------------------------
 */
void
Disk_PrintLfsCheckPointTrailer(trailerPtr)
    LfsCheckPointTrailer *trailerPtr;
{
    if (trailerPtr == NULL) {
	return;
    }
    printf("Timestamp of the checkpoint: %d\n", trailerPtr -> timestamp);
    printf("Checksum of the checkpoint:  %d\n", trailerPtr -> checkSum);
}

