/*
 * sunDiskLabel.h --
 *
 *      This defines the disk label that Sun writes on the 0'th sector of
 *      the 0'th cylinder of its SMD disks.  The disk label contains some
 *      geometry information and also the division of the disk into a
 *      number of partitions.  Each partition is identified to the drive
 *      by a different unit number.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SUNDISKLABEL
#define _SUNDISKLABEL

/*
 * SUN_NUM_DISK_PARTS is the number of partitions that are recorded in
 * the label information.  The size of the padding in the Sun_DiskLabel
 * type is dependend on this number...
 */
#define SUN_NUM_DISK_PARTS  8

/*
 * A disk is divided into partitions and this type specifies where a
 * partition starts and how many blocks (sectors) it contains.
 */
typedef struct Sun_DiskMap {
    int cylinder;
    int numBlocks;
} Sun_DiskMap;

/*
 * Label information on the zero'th sector.  The partition map and the
 * number of heads, sectors, and cylinders are used by the disk driver to
 * map from a block number in a disk partion to a block number on the raw
 * disk.  The gap and interleaving information are used by the block
 * allocation routines.
 */
typedef struct Sun_DiskLabel {
    char    asciiLabel[128];    	/* For compatibility with other systems
					 * that write an ascii label on a disk*/
    char    pad[512-(128+8*8+11*2)];	/* The sector size is 512 bytes, and
					 * padding puts our label info at the
					 * end of the sector. */
    unsigned short  gap1;		/* size of gap 1 */
    unsigned short  gap2;		/* size of gap 2 */
    unsigned short  interleave;		/* interleave factor */
    unsigned short  numCylinders;	/* # of data cylinders */
    unsigned short  numAltCylinders;	/* # of alternate cylinders */
    unsigned short  numHeads;		/* # of heads in this partition */
    unsigned short  numSectors;		/* # of 512 byte sectors per track */
    unsigned short  bhead;		/* ?? Head number under which this
					 * label is found, consistency chk */
    unsigned short  partitionID;	/* physical partition # */
    Sun_DiskMap map[SUN_NUM_DISK_PARTS]; /* Indicates the size and starting
					 * cylinder of the disk partitions */
    unsigned short  magic;		/* Identifies this label format */
    unsigned short  checkSum;		/* XOR checksum of sector */
} Sun_DiskLabel;
 
#define SUN_DISK_MAGIC       0xDABE

/*
 * The sun label does not describe the location of the filesystem header
 * information that comes after the zero'th label sector.  (The sprite label
 * will, but is never used.)  Instead, the following constants are used.
 * SUN_SUMMARY_SECTOR  one sector of summary info
 * SUN_DOMAIN_SECTOR  the first sector of the static domain header
 */
#define SUN_SUMMARY_SECTOR	17
#define SUN_DOMAIN_SECTOR	18

#endif _SUNDISKLABEL
