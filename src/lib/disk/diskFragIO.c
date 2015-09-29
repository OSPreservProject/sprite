/* 
 * fragIO.c --
 *
 *	Routines to allow fragments to be read and written.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/disk/RCS/diskFragIO.c,v 1.6 91/09/14 15:17:11 mendel Exp $ SPRITE (Berkeley)";
#endif not lint

#include <disk.h>
#include <stdio.h>

#define	SECTORS_PER_FRAG       (FS_FRAGMENT_SIZE / DEV_BYTES_PER_SECTOR)


/*
 *----------------------------------------------------------------------
 *
 * Disk_FragRead --
 *	Read fragments from the disk file at a specified 1K block offset.
 *	This has to use the disk geometry information to figure out
 *	what disk sectors correspond to the block.
 *
 * Results:
 *	0 if could read the disk, -1 if could not.  If couldn't read the disk
 *	then the error is stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Disk_FragRead(openFileID, headerPtr, firstFrag, numFrags, buffer)
    int 		openFileID; /* Handle on raw disk */
    Ofs_DomainHeader 	*headerPtr; /* Domain header with geometry 
				     * information. */
    int 		firstFrag;  /* First frag to read */
    int 		numFrags;   /* The number of fragments to read */
    Address 		buffer;	    /* The buffer to read into */
{
    register Ofs_Geometry	*geoPtr;
    register int	cylinder;
    register int	rotationalSet;
    register int	blockNumber;
    int			sector;

    geoPtr = &headerPtr->geometry;

    blockNumber		= firstFrag / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    if (geoPtr->rotSetsPerCyl > 0) {
	/*
	 * Original mapping scheme using rotational sets.
	 */
	blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
	rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
	blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;
    
	sector = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		 geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		 rotationalSet + geoPtr->blockOffset[blockNumber];
	sector += (firstFrag % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;
    } else if (geoPtr->rotSetsPerCyl == OFS_SCSI_MAPPING){
	/*
	 * New mapping for scsi devices.
	 */
	sector = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		    firstFrag * SECTORS_PER_FRAG - cylinder * 
		    geoPtr->blocksPerCylinder * DISK_SECTORS_PER_BLOCK;
    } else {
	return -1;
    }
    return(Disk_SectorRead(openFileID, sector, 
		numFrags * SECTORS_PER_FRAG, buffer));
}


/*
 *----------------------------------------------------------------------
 *
 * Disk_FragWrite --
 *	Write fragments to the disk file at a specified 1K block offset.
 *	This has to use the disk geometry information to figure out
 *	what disk sectors correspond to the block.
 *
 * Results:
 *	0 if could write the disk, -1 if could not.  If couldn't write the
 *	disk then the error is stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Disk_FragWrite(openFileID, headerPtr, firstFrag, numFrags, buffer)
    int 		openFileID; /* Handle on raw disk */
    Ofs_DomainHeader 	*headerPtr; /* Domain header with geometry 
				       information. */
    int 		firstFrag;  /* First frag to write */
    int 		numFrags;   /* The number of fragments to write */
    Address 		buffer;	    /* The buffer to write out of. */
{
    register Ofs_Geometry *geoPtr;
    register int cylinder;
    register int rotationalSet;
    register int blockNumber;
    int 	 sector;

    geoPtr = &headerPtr->geometry;

    blockNumber		= firstFrag / FS_FRAGMENTS_PER_BLOCK;
    cylinder		= blockNumber / geoPtr->blocksPerCylinder;
    if (geoPtr->rotSetsPerCyl > 0) {
	/*
	 * Original mapping scheme using rotational sets.
	 */
	blockNumber		-= cylinder * geoPtr->blocksPerCylinder;
	rotationalSet	= blockNumber / geoPtr->blocksPerRotSet;
	blockNumber		-= rotationalSet * geoPtr->blocksPerRotSet;
    
	sector = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		 geoPtr->sectorsPerTrack * geoPtr->tracksPerRotSet *
		 rotationalSet + geoPtr->blockOffset[blockNumber];
	sector += (firstFrag % FS_FRAGMENTS_PER_BLOCK) * SECTORS_PER_FRAG;
    } else if (geoPtr->rotSetsPerCyl == OFS_SCSI_MAPPING){
	/*
	 * New mapping for scsi devices.
	 */
	sector = geoPtr->sectorsPerTrack * geoPtr->numHeads * cylinder +
		    firstFrag * SECTORS_PER_FRAG - cylinder * 
		    geoPtr->blocksPerCylinder * DISK_SECTORS_PER_BLOCK;
    } else {
	return -1;
    }

     return(Disk_SectorWrite(openFileID, sector,
		numFrags * SECTORS_PER_FRAG, buffer));
}
