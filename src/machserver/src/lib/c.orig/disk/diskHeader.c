/* 
 * diskHeader.c --
 *
 *	Routines to read in the disk header information.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/disk/RCS/diskHeader.c,v 1.10 90/03/16 17:41:12 jhh Exp Locker: mendel $ SPRITE (Berkeley)";
#endif not lint

#include "disk.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static Sun_DiskLabel 	*ConvertToSunLabel();
static Dec_DiskLabel	*ConvertToDecLabel();
static Disk_Label	*ConvertFromSunLabel();
static Disk_Label	*ConvertFromDecLabel();
static short		SeeSunCheckSum();
static int		CheckSunCheckSum();
static int		MakeSunCheckSum();

/*
 * BOOT_SECTOR		Where the boot sectors start on disk.
 */
#define	BOOT_SECTOR		1
#define NUM_BOOT_SECTORS	15

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadDecLabel --
 *
 *	Read the appropriate sector of a disk partition and see if its
 *	a dec label.  If so, return a pointer to a Dec_DiskLabel.
 *	Note:  This reads a Sprite-modified Dec label, which has
 *	additional information.  If the label is a standard dec
 *	label, the numHeads field will be set to -1 to indicate
 *	the Sprite-dependent information is invalid.
 *
 * Results:
 *	A pointer to the label data if could read it, 0 otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
Dec_DiskLabel *
Disk_ReadDecLabel(fileID)
    int fileID;	/* Handle on raw disk */
{
    Address		buffer;
    Dec_DiskLabel	*decLabelPtr;

    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR);

    if (Disk_SectorRead(fileID, DEC_LABEL_SECTOR, 1, buffer) < 0) {
	free((char *)buffer);
	return((Dec_DiskLabel *)0);
    } else {
	decLabelPtr = (Dec_DiskLabel *)buffer;
	if (decLabelPtr->magic != DEC_LABEL_MAGIC) {
	    free((char*)buffer);
	    return((Dec_DiskLabel *)0);
	} else {
	    if (decLabelPtr->spriteMagic != FSDM_DISK_MAGIC) {
		/* Original dec label, not Sprite modified dec label. */
		decLabelPtr->numHeads = -1;
	    }
	    if (decLabelPtr->version != DEC_LABEL_VERSION) {
		printf("Warning: Wrong dec label version: %x vs. %x\n",
			decLabelPtr->version, DEC_LABEL_VERSION);
	    }
	    return(decLabelPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadSunLabel --
 *
 *	Read the first sector of a disk partition and see if its
 *	a sun label.  If so, return a pointer to a Sun_DiskLabel.
 *
 * Results:
 *	A pointer to the super block data if could read it, 0 otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
Sun_DiskLabel *
Disk_ReadSunLabel(fileID)
    int fileID;	/* Handle on raw disk */
{
    Address		buffer;
    Sun_DiskLabel	*sunLabelPtr;

    buffer = (Address)malloc(DEV_BYTES_PER_SECTOR);

    if (Disk_SectorRead(fileID, 0, 1, buffer) < 0) {
	free((char *)buffer);
	return((Sun_DiskLabel *)0);
    } else {
	sunLabelPtr = (Sun_DiskLabel *)buffer;
	if (sunLabelPtr->magic != SUN_DISK_MAGIC) {
	    free((char*)buffer);
	    return((Sun_DiskLabel *)0);
	} else {
	    return(sunLabelPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadDiskHeader --
 *
 *	Read the super block and return a pointer to it.
 *
 * Results:
 *	A pointer to the super block data if could read it, 0 otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
Fsdm_DiskHeader *
Disk_ReadDiskHeader(openFileID)
    int openFileID;	/* Handle on raw disk */
{
    Address		buffer;
    Fsdm_DiskHeader	*diskHeaderPtr;

    buffer = (Address) malloc(DEV_BYTES_PER_SECTOR);

    if (Disk_SectorRead(openFileID, 0, 1, buffer) < 0) {
	return((Fsdm_DiskHeader *)0);
    } else {
	diskHeaderPtr = (Fsdm_DiskHeader *)buffer;
	if (diskHeaderPtr->magic != FSDM_DISK_MAGIC) {
	    return((Fsdm_DiskHeader *)0);
	} else {
	    return(diskHeaderPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadDomainHeader --
 *
 *	Read the domain header and return a pointer to it.
 *
 * Results:
 *	A pointer to the domain header if could read it, NULL otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
Ofs_DomainHeader *
Disk_ReadDomainHeader(fileID, diskLabelPtr)
    int fileID;			/* Stream to raw disk */
    Disk_Label *diskLabelPtr;	/* Disk label */
{
    Ofs_DomainHeader	*headerPtr;

    headerPtr = (Ofs_DomainHeader *)malloc((unsigned) 
	(diskLabelPtr->numDomainSectors * DEV_BYTES_PER_SECTOR));

    if (Disk_SectorRead(fileID, diskLabelPtr->domainSector,
				    diskLabelPtr->numDomainSectors,
				    (Address)headerPtr) < 0) {
	return((Ofs_DomainHeader *)0);
    } else {
	if (headerPtr->magic != OFS_DOMAIN_MAGIC) {
	    fprintf(stderr, "Disk_ReadDomainHeader, bad magic <%x>\n",
		    headerPtr->magic);
	    free((Address)headerPtr);
	    return((Ofs_DomainHeader *)0);
	} else {
	    return(headerPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteDomainHeader --
 *
 *	Write the domain header.
 *
 * Results:
 *	SUCCESS if write succeeded, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteDomainHeader(fileID, diskLabelPtr, headerPtr)
    int 		fileID;			/* Stream to raw disk */
    Disk_Label 		*diskLabelPtr;		/* Disk label */
    Ofs_DomainHeader	*headerPtr;		/* Domain header. */
{

    return Disk_SectorWrite(fileID, diskLabelPtr->domainSector,
		    diskLabelPtr->numDomainSectors, (Address)headerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadSummaryInfo --
 *
 *	Read the summary information and return a pointer to it.
 *
 * Results:
 *	A pointer to the summary information if it could be read,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
Ofs_SummaryInfo *
Disk_ReadSummaryInfo(fileID, diskLabelPtr)
    int fileID;			/* Stream to raw disk */
    Disk_Label *diskLabelPtr;	/* Disk label */
{
    Ofs_SummaryInfo *summaryPtr;

    summaryPtr = (Ofs_SummaryInfo *) malloc (sizeof(Ofs_SummaryInfo));

    if (Disk_SectorRead(fileID, diskLabelPtr->summarySector, 
		diskLabelPtr->numSummarySectors, (Address)summaryPtr) < 0) {
	return((Ofs_SummaryInfo *)0);
    } else {
	return(summaryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteSummaryInfo --
 *
 *	Write the summary information.
 *
 * Results:
 *	SUCCESS if write succeeded, FAILURE otherwise
 *
 * Side effects:
 *	The summary information is written to the disk.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteSummaryInfo(fileID, diskLabelPtr, summaryPtr)
    int fileID;			/* Stream to raw disk */
    Disk_Label *diskLabelPtr;	/* Disk label */
    Ofs_SummaryInfo *summaryPtr; /* Summary information */
{
    return Disk_SectorWrite(fileID, diskLabelPtr->summarySector,
		diskLabelPtr->numSummarySectors, (Address)summaryPtr);
}
/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteDecLabel --
 *
 *	Write a DEC label to the appropriate sector of a disk partition.
 *
 * Results:
 *	SUCCESS if write succeeded, FAILURE otherwise
 *
 * Side effects:
 *	The label is written to the disk.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteDecLabel(fileID, labelPtr)
    int 		fileID;		/* Handle on raw disk */
    Dec_DiskLabel 	*labelPtr; 	/* Ptr to DEC label */
{
    return Disk_SectorWrite(fileID, DEC_LABEL_SECTOR, 1, (Address) labelPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteSunLabel --
 *
 *	Write a Sun label to the appropriate sector of a disk partiton.
 *
 * Results:
 *	0 if write succeeded, -1 otherwise
 *
 * Side effects:
 *	The label is written to the disk.
 *
 *----------------------------------------------------------------------
 */
int
Disk_WriteSunLabel(fileID, labelPtr)
    int 		fileID;		/* Handle on raw disk */
    Sun_DiskLabel	*labelPtr;	/* Ptr to Sun label */
{
    return Disk_SectorWrite(fileID, 0, 1, (Address) labelPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadLabel --
 *
 *	Read a label off the disk and convert it to a Disk_Label.
 *
 * Results:
 *	Pointer to Disk_Label if the label was read and converted,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */

Disk_Label *
Disk_ReadLabel(fileID)
    int fileID;			/* Handle on raw disk */
{
    Sun_DiskLabel	*sunLabelPtr;
    Dec_DiskLabel	*decLabelPtr;

    sunLabelPtr = Disk_ReadSunLabel(fileID);
    if (sunLabelPtr != (Sun_DiskLabel *)0) {
	return ConvertFromSunLabel(fileID, sunLabelPtr);
    } 
    decLabelPtr = Disk_ReadDecLabel(fileID);
    if (decLabelPtr != (Dec_DiskLabel *)0) {
	return ConvertFromDecLabel(fileID, decLabelPtr);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteLabel --
 *
 *	Converts a Disk_Label into a machine-specific disk label and
 *	writes it on the disk.
 *
 * Results:
 *	0 if everything worked
 *	-1 if the label could not be converted
 *	-2 otherwise
 *
 * Side effects:
 *	A label is written on the disk.
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */

int
Disk_WriteLabel(fileID, labelPtr)
    int 		fileID;		/* Handle on raw disk */
    Disk_Label		*labelPtr; 	/* Ptr to label to write. */
{
    int			status;	

    switch(labelPtr->labelType) {
	case DISK_SUN_LABEL: {
	    Sun_DiskLabel	*sunLabelPtr;
	    sunLabelPtr = ConvertToSunLabel(labelPtr);
	    if (sunLabelPtr == NULL) {
		return -1;
	    }
	    status = Disk_WriteSunLabel(fileID, sunLabelPtr);
	    free((char *) sunLabelPtr);
	    if (status < 0) {
		return -2;
	    }
	    break;
	}
	case DISK_DEC_LABEL: {
	    Dec_DiskLabel	*decLabelPtr;
	    decLabelPtr = ConvertToDecLabel(labelPtr);
	    if (decLabelPtr == NULL) {
		return -1;
	    }
	    status = Disk_WriteDecLabel(fileID, decLabelPtr);
	    free((char *) decLabelPtr);
	    if (status < 0) {
		return -2;
	    }
	    break;
	}
	default :
	    fprintf(stderr, "Unknown label type %d\n", labelPtr->labelType);
	    return -1;
	    break;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_NewLabel --
 *
 *	Creates a new disk label.
 *
 * Results:
 *	NULL if there was an error, a new disk label otherwise.
 *
 * Side effects:
 *	Memory allocation
 *
 *----------------------------------------------------------------------
 */

Disk_Label *
Disk_NewLabel(labelType) 
    Disk_NativeLabelType	labelType;	/* Native type of label. */
{
    Disk_Label		*labelPtr;

    labelPtr = (Disk_Label *) malloc(sizeof(Disk_Label));
    if (labelPtr == NULL) {
	fprintf(stderr, "Disk_NewLabel: Out of memory.\n");
	return NULL;
    }
    bzero((char *) labelPtr, sizeof(Disk_Label));
    strcpy(labelPtr->asciiLabel, "New label");
    switch(labelType) {
	case DISK_SUN_LABEL: 
	    labelPtr->asciiLabelLen = 128;
	    labelPtr->numPartitions = SUN_NUM_DISK_PARTS;
	    labelPtr->bootSector = SUN_BOOT_SECTOR;
	    labelPtr->numBootSectors = SUN_SUMMARY_SECTOR - SUN_BOOT_SECTOR -1;
	    labelPtr->summarySector = SUN_SUMMARY_SECTOR;
	    labelPtr->numSummarySectors = 1;
	    labelPtr->domainSector = SUN_DOMAIN_SECTOR;
	    labelPtr->numDomainSectors = OFS_NUM_DOMAIN_SECTORS;
	    labelPtr->labelType = labelType;
	    labelPtr->labelPtr = NULL;
	    labelPtr->labelSector = SUN_LABEL_SECTOR;
	    break;
	case DISK_DEC_LABEL: 
	    labelPtr->asciiLabelLen = 128;
	    labelPtr->numPartitions = DEC_NUM_DISK_PARTS;
	    labelPtr->bootSector = DEC_BOOT_SECTOR;
	    labelPtr->numBootSectors = DEC_SUMMARY_SECTOR - DEC_BOOT_SECTOR -1;
	    labelPtr->summarySector = DEC_SUMMARY_SECTOR;
	    labelPtr->numSummarySectors = 1;
	    labelPtr->domainSector = DEC_DOMAIN_SECTOR;
	    labelPtr->numDomainSectors = OFS_NUM_DOMAIN_SECTORS;
	    labelPtr->labelType = labelType;
	    labelPtr->labelPtr = NULL;
	    labelPtr->labelSector = DEC_LABEL_SECTOR;
	    break;
	default : 
	    fprintf(stderr, "Unknown label type %d\n", labelType);
	    free((char *) labelPtr);
	    labelPtr = NULL;
	    break;
    }
    return labelPtr;
}



/*
 *----------------------------------------------------------------------
 *
 * ConvertFromSunLabel --
 *
 *	Makes a Disk_Label from a Sun_DiskLabel
 *
 * Results:
 *	Pointer to Disk_Label if the label was read and converted,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation
 *
 *----------------------------------------------------------------------
 */

static Disk_Label *
ConvertFromSunLabel(fileID, sunLabelPtr)
    int 		fileID;		/* Handle on raw disk */
    Sun_DiskLabel	*sunLabelPtr;	/* Sun label to be converted */
{
    Disk_Label		*labelPtr;
    int			i;
    char 		buffer[DEV_BYTES_PER_SECTOR * OFS_NUM_DOMAIN_SECTORS];
    Ofs_DomainHeader	*domainHeaderPtr = (Ofs_DomainHeader *) buffer;
    int			sectorsPerCyl;

    labelPtr = (Disk_Label *) malloc(sizeof(Disk_Label));
    if (labelPtr == NULL) {
	fprintf(stderr, "Malloc failed.\n");
	return NULL;
    }
    bzero((char *) labelPtr, sizeof(Disk_Label));
    if (sunLabelPtr->magic != SUN_DISK_MAGIC) {
	fprintf(stderr, "Bad magic number on disk <%x> not <%x>\n",
	    sunLabelPtr->magic, SUN_DISK_MAGIC);
    }
    if (!CheckSunCheckSum(sunLabelPtr)) {
	printf("Check sum incorrect, 0x%x not 0x%x\n",
	    SeeSunCheckSum(sunLabelPtr), sunLabelPtr->checkSum);
    }
    labelPtr->numCylinders = sunLabelPtr->numCylinders;
    labelPtr->numAltCylinders = sunLabelPtr->numAltCylinders;
    labelPtr->numHeads = sunLabelPtr->numHeads;
    labelPtr->numSectors = sunLabelPtr->numSectors;
    labelPtr->asciiLabelLen = 128;
    strncpy(labelPtr->asciiLabel, sunLabelPtr->asciiLabel, 128);
    labelPtr->bootSector = BOOT_SECTOR;
    labelPtr->numDomainSectors = OFS_NUM_DOMAIN_SECTORS;
    /*
     * Go looking for the domain header.  This allows us a variable
     * number of boot sectors, without having to add additional fields
     * to the Sun label.
     */
    for (i = BOOT_SECTOR + 1; 
	 i < FSDM_MAX_BOOT_SECTORS + 3; 
	 i+= FSDM_BOOT_SECTOR_INC) {
	if (Disk_SectorRead(fileID, i, OFS_NUM_DOMAIN_SECTORS, buffer) < 0) {
	    fprintf(stderr, "Can't read sector %d.\n", i);
	    return(NULL);
	}
	if (domainHeaderPtr->magic == OFS_DOMAIN_MAGIC) {
	    labelPtr->summarySector = i - 1;
	    labelPtr->domainSector = i;
	    labelPtr->numBootSectors = labelPtr->summarySector - 1;
	    break;
	}
    }
    /* 
     * Check if we found a domain header.
     */
    if (i >= FSDM_MAX_BOOT_SECTORS + 3) {
	labelPtr->summarySector = -1;
	labelPtr->domainSector = -1;
	labelPtr->numBootSectors = -1;
    }
    labelPtr->numSummarySectors = 1;
    labelPtr->numPartitions = SUN_NUM_DISK_PARTS;
    sectorsPerCyl = labelPtr->numHeads * labelPtr->numSectors;
    for (i = 0; i < labelPtr->numPartitions; i++) {
	labelPtr->partitions[i].firstCylinder = sunLabelPtr->map[i].cylinder;
	if (sunLabelPtr->map[i].numBlocks % sectorsPerCyl != 0) {
	    printf(
    "Warning: size of partition %d (0x%x) is not multiple of cylinder size\n",
		    i, sunLabelPtr->map[i].numBlocks);
	}
	labelPtr->partitions[i].numCylinders = 	
	    sunLabelPtr->map[i].numBlocks / sectorsPerCyl;
    }
    labelPtr->labelSector = SUN_LABEL_SECTOR;
    labelPtr->labelType = DISK_SUN_LABEL;
    labelPtr->labelPtr = (char *) sunLabelPtr;
    return labelPtr;
}
/*
 *----------------------------------------------------------------------
 *
 * ConvertFromDecLabel --
 *
 *	Makes a Disk_Label from a Dec_DiskLabel
 *
 * Results:
 *	Pointer to Disk_Label if the label was read and converted,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Disk_Label *
ConvertFromDecLabel(fileID, decLabelPtr)
    int 		fileID;		/* Handle on raw disk */
    Dec_DiskLabel	*decLabelPtr;	/* DEC label to be converted */
{
    Disk_Label		*labelPtr;
    int			bytesPerCyl;
    int			i;

    if (decLabelPtr->numHeads == -1) {
	fprintf(stderr, "Dec label does not have Sprite extensions.\n");
	return NULL;
    }
    labelPtr = (Disk_Label *) malloc(sizeof(Disk_Label));
    if (labelPtr == NULL) {
	fprintf(stderr, "Malloc failed.\n");
	return NULL;
    }
    bzero((char *) labelPtr, sizeof(Disk_Label));
    if (decLabelPtr->magic != DEC_LABEL_MAGIC) {
	printf("Bad magic number on disk <%x> not <%x>\n",
	    decLabelPtr->magic, DEC_LABEL_MAGIC);
    }
    labelPtr->numSectors = decLabelPtr->numSectors;
    labelPtr->numHeads = decLabelPtr->numHeads;
    labelPtr->numCylinders = decLabelPtr->numCylinders;
    labelPtr->numAltCylinders = decLabelPtr->numAltCylinders;
    labelPtr->asciiLabelLen = 128;
    strncpy(labelPtr->asciiLabel, decLabelPtr->asciiLabel, 128);
    labelPtr->bootSector = decLabelPtr->bootSector;
    labelPtr->numDomainSectors = decLabelPtr->numDomainSectors;
    labelPtr->numBootSectors = decLabelPtr->numBootSectors;
    labelPtr->summarySector = decLabelPtr->summarySector;
    labelPtr->domainSector = decLabelPtr->domainSector;
    labelPtr->numDomainSectors = decLabelPtr->numDomainSectors;
    labelPtr->numSummarySectors = 1;
    labelPtr->numPartitions = DEC_NUM_DISK_PARTS;
    bytesPerCyl = labelPtr->numHeads * labelPtr->numSectors *
	    DEV_BYTES_PER_SECTOR;
    for (i = 0; i < labelPtr->numPartitions; i++) {
	if (decLabelPtr->map[i].offsetBytes % bytesPerCyl != 0) {
	    printf(
    "Warning: start of partition %d (0x%x) is not multiple of cylinder size\n",
		    i, decLabelPtr->map[i].offsetBytes);
	}
	labelPtr->partitions[i].firstCylinder = 
	    decLabelPtr->map[i].offsetBytes / bytesPerCyl;
	if (decLabelPtr->map[i].numBytes % bytesPerCyl != 0) {
	    printf(
    "Warning: size of partition %d (0x%x) is not multiple of cylinder size\n",
		    i, decLabelPtr->map[i].numBytes);
	}
	labelPtr->partitions[i].numCylinders = 
	    decLabelPtr->map[i].numBytes / bytesPerCyl;
    }
    labelPtr->labelSector = DEC_LABEL_SECTOR;
    labelPtr->labelType = DISK_DEC_LABEL;
    labelPtr->labelPtr = (char *) decLabelPtr;
    return labelPtr;
}

/*
 *----------------------------------------------------------------------
 *
 *  ConvertToSunLabel --
 *
 *	Makes a Sun_DiskLabel from a Disk_Label
 *
 * Results:
 *	Pointer to Sun_DiskLabel if the label was converted and written,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation
 *
 *----------------------------------------------------------------------
 */

static Sun_DiskLabel *
ConvertToSunLabel(labelPtr)
    Disk_Label		*labelPtr;	/* Label to be converted */
{
    Sun_DiskLabel	*sunLabelPtr;
    int			i;

    if (labelPtr->numPartitions > SUN_NUM_DISK_PARTS) {
	fprintf(stderr, "Too many disk partitions for a Sun label (%d > %d)\n",
	    labelPtr->numPartitions, SUN_NUM_DISK_PARTS);
	return NULL;
    }
    sunLabelPtr = (Sun_DiskLabel *) malloc(sizeof(Sun_DiskLabel));
    if (sunLabelPtr == NULL) {
	fprintf(stderr, "Malloc failed.\n");
	return NULL;
    }
    bzero((char *) sunLabelPtr, sizeof(Sun_DiskLabel));
    sunLabelPtr->magic = SUN_DISK_MAGIC;
    sunLabelPtr->numCylinders = labelPtr->numCylinders;
    sunLabelPtr->numAltCylinders = labelPtr->numAltCylinders;
    sunLabelPtr->numHeads = labelPtr->numHeads;
    sunLabelPtr->numSectors = labelPtr->numSectors;
    strncpy(sunLabelPtr->asciiLabel, labelPtr->asciiLabel, 128);
    sunLabelPtr->partitionID = 0;
    sunLabelPtr->bhead = 0;
    sunLabelPtr->gap1 = 65535;
    sunLabelPtr->gap2 = 65535;
    sunLabelPtr->interleave = 1;
    for (i = 0; i < SUN_NUM_DISK_PARTS; i++) {
	sunLabelPtr->map[i].cylinder = labelPtr->partitions[i].firstCylinder;
	sunLabelPtr->map[i].numBlocks = labelPtr->partitions[i].numCylinders *
	    labelPtr->numHeads * labelPtr->numSectors;
    }
    MakeSunCheckSum(sunLabelPtr);
    return sunLabelPtr;
}
/*
 *----------------------------------------------------------------------
 *
 * ConvertToDecLabel --
 *
 *	Makes a Dec_DiskLabel from a Disk_Label
 *
 * Results:
 *	Pointer to Dec_DiskLabel if the label was converted and written,
 *	NULL otherwise.
 *
 * Side effects:
 *	Memory allocation
 *
 *----------------------------------------------------------------------
 */
static Dec_DiskLabel *
ConvertToDecLabel(labelPtr)
    Disk_Label	*labelPtr;	/* Label to be converted */
{
    Dec_DiskLabel	*decLabelPtr;
    int			bytesPerCyl;
    int			i;

    if (labelPtr->numPartitions > DEC_NUM_DISK_PARTS) {
	fprintf(stderr, "Too many disk partitions for a DEC label (%d > %d)\n",
	    labelPtr->numPartitions, DEC_NUM_DISK_PARTS);
	return NULL;
    }
    decLabelPtr = (Dec_DiskLabel *) malloc(sizeof(Dec_DiskLabel));
    if (decLabelPtr == NULL) {
	fprintf(stderr, "Malloc failed.\n");
	return NULL;
    }
    bzero((char *) decLabelPtr, sizeof(Dec_DiskLabel));
    decLabelPtr->magic = DEC_LABEL_MAGIC;
    decLabelPtr->spriteMagic = FSDM_DISK_MAGIC;
    decLabelPtr->version = DEC_LABEL_VERSION;
    decLabelPtr->isPartitioned = 1;
    decLabelPtr->numHeads = labelPtr->numHeads;
    decLabelPtr->numSectors = labelPtr->numSectors;
    decLabelPtr->domainSector = labelPtr->domainSector;
    decLabelPtr->numDomainSectors = labelPtr->numDomainSectors;
    decLabelPtr->numCylinders = labelPtr->numCylinders;
    decLabelPtr->numAltCylinders = labelPtr->numAltCylinders;
    decLabelPtr->bootSector = labelPtr->bootSector;
    decLabelPtr->numBootSectors = labelPtr->numBootSectors;
    decLabelPtr->summarySector = labelPtr->summarySector;
    strncpy(decLabelPtr->asciiLabel, labelPtr->asciiLabel, 128);
    bytesPerCyl = labelPtr->numHeads *labelPtr->numSectors *
	    DEV_BYTES_PER_SECTOR;
    for (i = 0; i < DEC_NUM_DISK_PARTS; i++) {
	decLabelPtr->map[i].numBytes = 
	    labelPtr->partitions[i].numCylinders * bytesPerCyl;
	decLabelPtr->map[i].offsetBytes = 
	    labelPtr->partitions[i].firstCylinder * bytesPerCyl;
    }
    return decLabelPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Sun checksum routines --
 *
 *	The following routines manipulate the checksum in a Sun label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static short
SeeSunCheckSum(sunLabelPtr)
    Sun_DiskLabel *sunLabelPtr;
{
        short *sp, sum = 0;
        short count = DEV_BYTES_PER_SECTOR/sizeof(short) - 1;

        sp = (short *)sunLabelPtr;
        while (count--)
                sum ^= *sp++;
        return (sum);
}

static int
CheckSunCheckSum(sunLabelPtr)
    Sun_DiskLabel *sunLabelPtr;
{
        short *sp, sum = 0;
        short count = DEV_BYTES_PER_SECTOR/sizeof(short);

        sp = (short *)sunLabelPtr;
        while (count--)
                sum ^= *sp++;
        return (sum ? 0 : 1);
}

static int
MakeSunCheckSum(sunLabelPtr)
    Sun_DiskLabel *sunLabelPtr;
{
        short *sp, sum = 0;
        short count = DEV_BYTES_PER_SECTOR/sizeof(short) - 1;

        sunLabelPtr->checkSum = 0;
        sp = (short *)sunLabelPtr;
        while (count--)
                sum ^= *sp++;
        sunLabelPtr->checkSum = sum;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_EraseLabel --
 *
 *	Erase a label from the disk.
 *
 * Results:
 *	0 if the label was erased properly
 *	-1 otherwise.
 *
 * Side effects:
 *	A sector is cleared on the disk.
 *
 *----------------------------------------------------------------------
 */

int
Disk_EraseLabel(fileID, labelType)
    int				fileID;		/* Handle on raw disk. */
    Disk_NativeLabelType	labelType;	/* Type of label. */
{
    char	buffer[DEV_BYTES_PER_SECTOR];
    int		sector;

    switch (labelType) {
	case DISK_SUN_LABEL:
	    sector = SUN_LABEL_SECTOR;
	    break;
	case DISK_DEC_LABEL:
	    sector = DEC_LABEL_SECTOR;
	    break;
	default:
	    printf("Unknown label type.\n");
	    return -1;
    }
    bzero(buffer, DEV_BYTES_PER_SECTOR);
    return Disk_SectorWrite(fileID, sector, 1, buffer);
}

