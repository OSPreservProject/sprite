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
static char rcsid[] = "$Header: /sprite/src/lib/disk/RCS/diskHeader.c,v 1.15 92/12/09 16:37:08 jhh Exp Locker: eklee $ SPRITE (Berkeley)";
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
	    fprintf(stderr, 
		    "Disk_ReadSunLabel: Bad magic 0x%x, should be 0x%x\n",
		    sunLabelPtr->magic, SUN_DISK_MAGIC);
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

/*
 *----------------------------------------------------------------------
 *
 * Disk_HasFilesystem
 *
 *	Determines if a filesystem can be found on the disk stream.
 *
 * Results:
 *	DISK_HAS_NO_FS if no filesystem could be found;
 *      DISK_HAS_OFS if an old filesystem was found;
 *      DISK_HAS_LFS if a log structured filesystem was found;
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
int
Disk_HasFilesystem(fileID, diskLabelPtr)
    int fileID;                /* Disk stream */
    Disk_Label *diskLabelPtr;  /* Disk Label */ 
{
    Ofs_DomainHeader *headerPtr;
    LfsSuperBlock    *superPtr;
    int              status;
    unsigned int     magic;

    /*
     * Note: We check for an OFS first because when an OFS is created
     *       the old filesystem on the disk is not explicitly erased.
     *       So if the old filesystem was a LFS, is is possible that
     *       the LfsSuperBlock is still on the disk.  'fsmake' should
     *       be changed to erase the old filesystem, but since the OFS
     *       is rarely used it is not crucial.  'mklfs' does the
     *       right thing.
     *
     *       Also, Disk_ReadDomainHeader() and Disk_ReadLfsSuperBlock()
     *       were not used because we don't want any error messages
     *       printed out.
     */

    /*
     * check for an OFS by trying to read a domain header.  If one
     * is found that has a valid magic number, then assume that
     * an OFS is on the disk
     */
    headerPtr = (Ofs_DomainHeader *)malloc((unsigned) 
	(diskLabelPtr->numDomainSectors * DEV_BYTES_PER_SECTOR));
    if (headerPtr == NULL) {
	perror("Disk_HasFilesystem:  allocating Ofs_DomainHeader");
	exit(FAILURE);
    }
    /*
     * if the domainSector is negative, then there is no Ofs_DomainHeader
     * on the disk -- just check to see if it is an LFS then...
     */
    if (diskLabelPtr -> domainSector >= 0 && 
	diskLabelPtr -> summarySector >= 0 ) {
	status = Disk_SectorRead(fileID, diskLabelPtr -> domainSector,
				 diskLabelPtr -> numDomainSectors,
				 (Address)headerPtr);
	magic = headerPtr -> magic;
	free(headerPtr);
    } else {
	status = -1;
    }
    if (status < 0 || magic != OFS_DOMAIN_MAGIC) {
	/*
	 * check for a LFS by trying to read a LfsSuperBlock.  If
	 * one is found that has a valid magic number, then assume that
	 * a LFS is on the disk
	 */
	superPtr = (LfsSuperBlock *)malloc(LFS_SUPER_BLOCK_SIZE);
	if (superPtr == NULL) {
	    perror("Disk_HasFilesystem:  allocating LfsSuperBlock");
	    exit(FAILURE);
	}
	status = Disk_SectorRead(fileID, LFS_SUPER_BLOCK_OFFSET,
				 LFS_SUPER_BLOCK_SIZE / DEV_BYTES_PER_SECTOR,
				 (Address)superPtr);
	magic = superPtr ->hdr.magic;
	free(superPtr);
	if (status < 0 || magic != LFS_SUPER_BLOCK_MAGIC) {
	    return DISK_HAS_NO_FS;
	} else {
	    return DISK_HAS_LFS;
	}
    } else {
	return DISK_HAS_OFS;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadLfsSuperBlock
 *
 *	Read the LFS SuperBlock from the stream.
 *
 * Results:
 *	A pointer to the LFS SuperBlock if could read it, NULL otherwise.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
LfsSuperBlock *
Disk_ReadLfsSuperBlock(fileID, diskLabelPtr)
    int fileID;                /* Disk stream */
    Disk_Label *diskLabelPtr;  /* Disk Label */
{
    LfsSuperBlock    *superPtr;
    int              fstype, status;
    
    fstype = Disk_HasFilesystem(fileID, diskLabelPtr);
    if (fstype != DISK_HAS_LFS) {
	return ((LfsSuperBlock *)NULL);
    }
    /*
     * Note:  Reading of LfsSuperBlock is dependent on the blockSize constant
     * set at the top of /sprite/src/admin/mklfs/mklfs.c
     */
    superPtr = (LfsSuperBlock *)malloc(LFS_SUPER_BLOCK_SIZE);
    if (superPtr == NULL) {
	perror("Disk_ReadLfsSuperBlock: allocating LfsSuperBlock");
	exit(FAILURE);
    }
    status = Disk_SectorRead(fileID, LFS_SUPER_BLOCK_OFFSET,
			     LFS_SUPER_BLOCK_SIZE / DEV_BYTES_PER_SECTOR,
			     (Address)superPtr);
    if (status < 0) {
	free((Address)superPtr);
	return ((LfsSuperBlock *)0);
    } else {
	if (superPtr -> hdr.magic != LFS_SUPER_BLOCK_MAGIC) {
	    fprintf(stderr, "Disk_ReadLfsSuperBlock, bad magic <%x>\n",
		    superPtr -> hdr.magic);
	    free((Address)superPtr);
	    return ((LfsSuperBlock *)0);
	} else {
	    return superPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteLfsSuperBlock
 *
 *	Write the LFS SuperBlock to the stream.
 *
 * Results:
 *	-1 if the SuperBlock could not be written, 0 if it could.
 *
 * Side effects:
 *	Does the write.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteLfsSuperBlock(fileID, superPtr)
    int 		fileID;			/* Stream to raw disk */
    LfsSuperBlock	*superPtr;		/* Lfs SuperBlock */
{
    return Disk_SectorWrite(fileID, LFS_SUPER_BLOCK_OFFSET,
		    LFS_SUPER_BLOCK_SIZE / DEV_BYTES_PER_SECTOR, 
		    (Address)superPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadLfsCheckPointHdr
 *
 *	Read the current checkpoint region in its entirety.  The caller
 *      can choose which checkpoint region to read by specifying 
 *      either `0' or `1' in the area argument (any other value
 *      will return the current checkpoint).
 *
 * Results:
 *	A pointer to the header of the checkpoint region upon
 *      success, NULL otherwise.  Using the header, the other
 *      sections of the checkpoint can be reached.
 *
 * Side effects:
 *	Memory allocation.
 *
 *----------------------------------------------------------------------
 */
LfsCheckPointHdr *
Disk_ReadLfsCheckPointHdr(fileID, labelPtr, areaPtr)
    int        fileID;
    Disk_Label *labelPtr;
    int        *areaPtr;       /* return value specifying checkpoint area */
{
    LfsSuperBlock        *superPtr;
    LfsCheckPointHdr     *checkPointHdrPtr;
    LfsCheckPointTrailer *trailerPtr;
    int                  checkPointOffset, checkPointSize;
    int                  numSectors;
    char                 *bufferPtr;
    unsigned int         stamp0, stamp1;
    int                  status;
    
    superPtr = Disk_ReadLfsSuperBlock(fileID, labelPtr);
    if (superPtr == NULL) {
	return ((LfsCheckPointHdr *)NULL);
    }
    /*
     * Examine the two checkpoint areas to locate the checkpoint area with the
     * newest timestamp.
     */
    numSectors = ((sizeof(LfsCheckPointHdr) + DEV_BYTES_PER_SECTOR) - 1) /
	         DEV_BYTES_PER_SECTOR;
    bufferPtr = (char *)malloc(numSectors * DEV_BYTES_PER_SECTOR);
    if (bufferPtr == NULL) {
	perror("Disk_ReadLfsCheckPointHdr: allocating LfsCheckPointHdr");
	exit(FAILURE);
    }
    /*
     * Checkpoint region one.
     */
    status = Disk_SectorRead(fileID, superPtr -> hdr.checkPointOffset[0],
			     numSectors, bufferPtr);
    if (status < 0) {
	fprintf(stderr, "Disk_ReadCheckPointHdr: could not read header #0\n");
	free(bufferPtr);
	free(superPtr);
	return (LfsCheckPointHdr *)NULL;
    }
    stamp0 = ((LfsCheckPointHdr *)bufferPtr) -> timestamp;
    /*
     * Checkpoint region two.
     */
    status = Disk_SectorRead(fileID, superPtr -> hdr.checkPointOffset[1],
			     numSectors, bufferPtr);
    if (status < 0) {
	fprintf(stderr, "Disk_ReadCheckPointHdr: could not read header #1\n");
	free(bufferPtr);
	free(superPtr);
	return (LfsCheckPointHdr *)NULL;
    }
    stamp1 = ((LfsCheckPointHdr *)bufferPtr) -> timestamp;
    free(bufferPtr);
    /*
     * Get the latest checkpoint header, and return in `areaPtr' 
     * which checkpoint area the header is for. (It should be in the
     * structure, but it's not so oh well.)  If `areaPtr' 
     * specifies which area to get, then get that one.
     */
    if (areaPtr != NULL && *areaPtr == 0) {
	checkPointOffset = superPtr -> hdr.checkPointOffset[0];
    } else if (areaPtr != NULL && *areaPtr == 1) {
	checkPointOffset = superPtr -> hdr.checkPointOffset[1];
    } else if (stamp0 < stamp1) {
	if (areaPtr != NULL) {
	    *areaPtr = 1;
	}
	checkPointOffset = superPtr -> hdr.checkPointOffset[1];
    } else {
	if (areaPtr != NULL) {
	    *areaPtr = 0;
	}
	checkPointOffset = superPtr -> hdr.checkPointOffset[0];
    }

    checkPointSize = superPtr -> hdr.maxCheckPointBlocks *
	             superPtr -> hdr.blockSize;
    checkPointHdrPtr = (LfsCheckPointHdr *)malloc(checkPointSize);
    if (checkPointHdrPtr == NULL) {
	perror("allocating buffer for a full Lfs CheckPoint");
	exit(FAILURE);
    }
    status = Disk_SectorRead(fileID, checkPointOffset, 
			     superPtr -> hdr.maxCheckPointBlocks,
			     (Address)checkPointHdrPtr);
    free(superPtr);
    if (status < 0) {
	fprintf(stderr, "cannot read the entire checkpoint region");
	free(checkPointHdrPtr);
	return (LfsCheckPointHdr *)NULL;
    }
    /*
     * if we cannot find a valid trailer, then the checkpoint is not valid
     */
    trailerPtr = Disk_LfsCheckPointTrailer(checkPointHdrPtr);
    if (trailerPtr == NULL) {
	free(checkPointHdrPtr);
	return (LfsCheckPointHdr *)NULL;
    }
    return checkPointHdrPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteLfsCheckPointHdr
 *
 *	Write the checkpoint header only.
 *
 * Results:
 *	0 if the header was successfully written, -1 if it wasn't.
 *
 * Side effects:
 *	Disk write.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteLfsCheckPointHdr(fileID, headerPtr, area, labelPtr)
    int              fileID;
    LfsCheckPointHdr *headerPtr;
    int              area;   /* which checkpoint area this is: 0 or 1 */
    Disk_Label       *labelPtr;
{
    LfsSuperBlock *superPtr;
    int           numSectors;
    int           status;

    if (area < 0 || area > 1) {
	return -1;
    }
    superPtr = Disk_ReadLfsSuperBlock(fileID, labelPtr);
    if (superPtr == NULL) {
	return -1;
    }
    numSectors = ((sizeof(LfsCheckPointHdr) + DEV_BYTES_PER_SECTOR) - 1) /
	         DEV_BYTES_PER_SECTOR;
    status = Disk_SectorWrite(fileID, superPtr->hdr.checkPointOffset[area],
			     numSectors, (Address)headerPtr);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteLfsCheckPointArea
 *
 *	Write the checkpoint area, headed by an LfsCheckPointHdr,
 *      to disk.  Such an area is returned from Disk_ReadLfsCheckPointHdr().
 *      Note: this does not constitute an update.
 *
 * Results:
 *	0 if the area was successfully written, -1 if it wasn't.
 *
 * Side effects:
 *	Disk write.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_WriteLfsCheckPointArea(fileID, areaPtr, area, labelPtr)
    int              fileID;
    LfsCheckPointHdr *areaPtr;
    int              area;  /* which checkpoint area this is: 0 or 1 */
    Disk_Label       *labelPtr;
{
    LfsSuperBlock *superPtr;
    int           status;

    if (area < 0 || area > 1) {
	return -1;
    }
    superPtr = Disk_ReadLfsSuperBlock(fileID, labelPtr);
    if (superPtr == NULL) {
	return -1;
    }
    status = Disk_SectorWrite(fileID, 
			      superPtr->hdr.checkPointOffset[area],
			      superPtr->hdr.maxCheckPointBlocks,
			      (Address)areaPtr);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_LfsCheckPointTrailer
 *
 *	Find the trailer of the checkpoint region using the checkpoint header
 *
 * Results:
 *	A pointer to the trailer of the checkpoint region upon
 *      success, NULL otherwise.  
 *
 *      Note: the pointer returned is a pointer into the header.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
LfsCheckPointTrailer *
Disk_LfsCheckPointTrailer(checkPointPtr)
    LfsCheckPointHdr *checkPointPtr;
{
    char                 *bytePtr;
    LfsCheckPointTrailer *trailerPtr;

    bytePtr = (char *)checkPointPtr;
    trailerPtr = (checkPointPtr == NULL) ? (LfsCheckPointTrailer *)NULL :
	(LfsCheckPointTrailer *)(bytePtr + checkPointPtr -> size -
				 sizeof(LfsCheckPointTrailer));
    if (trailerPtr == NULL) {
	fprintf(stderr, "Lfs CheckPoint does not have a valid trailer\n");
	return (LfsCheckPointTrailer *)NULL;
    } else if (checkPointPtr -> timestamp != trailerPtr -> timestamp) {
	fprintf(stderr, "CheckPoint header timestamp <%d> does not match\n",
		checkPointPtr -> timestamp);
	fprintf(stderr, "trailer timestamp <%d>.\n", trailerPtr -> timestamp);
	return (LfsCheckPointTrailer *)NULL;
    }
    return trailerPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Disk_ForEachCheckPointRegion
 *
 *	For every checkpoint region found in the checkpoint header,
 *      execute the procedure argument on that region.  The procedure
 *      argument must take a LfsCheckPointRegion pointer as an
 *      argument, and have a return value of ReturnStatus.  If the
 *      procedure returns a non-zero return value, the region iteration
 *      stops and that value is returned.
 *
 * Results:
 *      0 if there were no problems and the procedure was invoked
 *        with every checkpoit region as an argument;
 *	-1 if the trailer could not be found from the header;
 *      a non-zero return value from the procedure argument.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Disk_ForEachCheckPointRegion(checkPointPtr, regionProc)
    LfsCheckPointHdr *checkPointPtr;
    ReturnStatus (*regionProc)();
{
    char                 *bytePtr, *maxBytePtr;
    LfsCheckPointTrailer *trailerPtr;
    LfsCheckPointRegion  *regionPtr;
    ReturnStatus status;

    status = 0;
    trailerPtr = Disk_LfsCheckPointTrailer(checkPointPtr);
    if (trailerPtr == NULL) {
	return -1;
    }
    bytePtr = (char *)checkPointPtr;
    bytePtr += sizeof(LfsCheckPointHdr);
    maxBytePtr = (char *)trailerPtr;
    while (bytePtr < maxBytePtr) {
	regionPtr = (LfsCheckPointRegion *)bytePtr;
	status = regionProc(regionPtr);
	if (status) {
	    return status;
	}
	bytePtr += regionPtr -> size;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Disk_ReadLfsSegSummary --
 *
 *	Reads a segment summary block from an LFS segment. Each segment
 *	contains a linked list of summary blocks. To read the first
 *	one pass NULL as the prevSegPtr. For subsequent summary blocks
 *	pass in a pointer to the previous block.
 *
 * Results:
 *	Pointer to the segment summary block, or NULL if the indicated
 *	block doesn't exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

LfsSegSummary	*
Disk_ReadLfsSegSummary(fileID, superPtr, segment, prevSegPtr)
    int			fileID;		/* Stream to raw disk. */
    LfsSuperBlock	*superPtr;	/* Lfs SuperBlock */
    int			segment;	/* Segment # to use. */
    LfsSegSummary	*prevSegPtr;	/* Previous segment summary block
					 * for the segment. */

{
    int			blockOffset = 1;
    LfsSegSummary	*segPtr;
    int			status;
    int			segOffset;
    int			blocksPerSeg;

    if (prevSegPtr != NULL) {
	blockOffset = prevSegPtr->nextSummaryBlock;
    }
    blocksPerSeg = superPtr->usageArray.segmentSize/DEV_BYTES_PER_SECTOR;
    segPtr = (LfsSegSummary *) malloc(DEV_BYTES_PER_SECTOR);
    segOffset = segment * blocksPerSeg + superPtr->hdr.logStartOffset;
    status = Disk_SectorRead(fileID, segOffset + blocksPerSeg - blockOffset,
		1, (Address) segPtr);
    if (status < 0) {
	free((char *) segPtr);
	return NULL;
    }
    return segPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Disk_WriteLfsSegSummary --
 *
 *	Writes a segment summary block to an LFS segment. To write the first
 *	one pass NULL as the prevSegPtr. For subsequent summary blocks
 *	pass in a pointer to the previous block.
 *
 * Results:
 *	Standard return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Disk_WriteLfsSegSummary(fileID, superPtr, segment, prevSegPtr, segPtr)
    int			fileID;		/* Stream to raw disk. */
    LfsSuperBlock	*superPtr;	/* Lfs SuperBlock */
    int			segment;	/* Segment # to use. */
    LfsSegSummary	*prevSegPtr;	/* Previous segment summary block
					 * for the segment. */
    LfsSegSummary	*segPtr;	/* Segment to write. */
{
    int			blockOffset = 1;
    int			status;
    int			segOffset;
    int			blocksPerSeg;

    if (prevSegPtr != NULL) {
	blockOffset = prevSegPtr->nextSummaryBlock;
    }
    blocksPerSeg = superPtr->usageArray.segmentSize/DEV_BYTES_PER_SECTOR;
    segOffset = segment * blocksPerSeg + superPtr->hdr.logStartOffset;
    status = Disk_SectorWrite(fileID, segOffset + blocksPerSeg - blockOffset,
		1, (Address) segPtr);
    if (status < 0) {
	return FAILURE;
    }
    return SUCCESS;
}





