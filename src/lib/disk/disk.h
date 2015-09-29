/*
 * disk.h --
 *
 *	Definitions for utilities that examine an OFS filesystem through
 *	a raw disk interface.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/disk/RCS/disk.h,v 1.4 91/10/07 17:40:43 voelker Exp $ SPRITE (Berkeley)
 */

#ifndef _DISK
#define _DISK

#include <sys/file.h>

#include <kernel/fs.h>
#include <kernel/dev.h>
#include <kernel/fsdm.h>
#include <kernel/ofs.h>
#include <kernel/devDiskLabel.h>

/*
 * include files for LFS structures and constants
 */
#include <kernel/lfsDesc.h>
#include <kernel/lfsDescMap.h>
#include <kernel/lfsFileLayout.h>
#include <kernel/lfsSegLayout.h>
#include <kernel/lfsStableMem.h>
#include <kernel/lfsSuperBlock.h>
#include <kernel/lfsUsageArray.h>

/*
 * These should be here.  They should be in some machine dependent header
 * file.  But for now ...
 */
#define BITS_PER_BYTE           8
#define BITS_PER_INT            32

/*
 * DISK_SECTORS_PER_BLOCK	Number of disk sectors per file system block.
 * DISK_KBYTES_PER_BLOCK	Number of kbyte chunks per file system block.
 */
#define DISK_SECTORS_PER_BLOCK       (FS_BLOCK_SIZE / DEV_BYTES_PER_SECTOR)
#define DISK_KBYTES_PER_BLOCK        (FS_BLOCK_SIZE / 1024)

/*
 * Maximum number of partitions on a disk.
 */
#define DISK_MAX_PARTS 8

/*
 * Maximum length of an ascii label inside a disk label.
 */
#define DISK_MAX_ASCII_LABEL 256

/*
 * We understand two types of native disk labels: Sun labels and DEC labels.
 */

typedef int Disk_NativeLabelType;

#define DISK_NO_LABEL  ((Disk_NativeLabelType) 0)
#define DISK_SUN_LABEL ((Disk_NativeLabelType) 1)
#define DISK_DEC_LABEL ((Disk_NativeLabelType) 2)

/*
 * Information about a disk partition.
 */
typedef struct Disk_Partition {
    int		firstCylinder;		/* First cylinder in partition. */
    int		numCylinders;		/* Number of cylinders in partition.*/
} Disk_Partition;

/*
 * This is a canonical disk label.  The use of this structure allows
 * programs to read and write disk labels without worrying about
 * the format of the machine-specific disk label.  Fields labelled (RO)
 * are read-only.  They should not be modified by a user program.
 * Fields labelled (+) can be modified by user programs, but the results
 * may not be what is desired.  These fields reflect the location and size
 * of other data structures on the disk.  Simply changing them in the
 * label will not suffice.  The data structures will have to be moved also.
 * For some types of machines it is not possible to change these fields
 * because their location is hard-wired into the prom.  Refer to the prom
 * documentation and kernel source code.  The bottom line is you don't
 * want to change the (+) fields unless you really know what you're doing.
 */
typedef struct Disk_Label {
    int	numHeads;		/* Number of heads */
    int	numSectors;		/* Number of sectors per track */
    int numCylinders;		/* Number of cylinders on the disk. */
    int numAltCylinders;	/* Number of alternate cylinders. */
    char asciiLabel[DISK_MAX_ASCII_LABEL];	/* Ascii label. */
    Disk_Partition partitions[DISK_MAX_PARTS]; /* Partition map */
    Disk_NativeLabelType labelType; /* Type of native disk label */
    char *labelPtr;		/* Pointer to native disk label. */
    int	bootSector;		/* (+) Starting sector of boot program */
    int	numBootSectors; 	/* (+) Number of boot sectors. */
    int	summarySector;		/* (+) Start of summary information. */
    int	numSummarySectors; 	/* (+) Number of sectors in summary info. */
    int domainSector;		/* (+) Sector where domain header starts. */
    int numDomainSectors;	/* (+) Number of sectors in domain header. */
    int numPartitions;		/* (RO) Number of partitions on disk. */
    int asciiLabelLen;		/* (RO) Length of ascii label. */
    int labelSector;		/* (RO) Location of native disk label. */
} Disk_Label;

/*
 * Return values for Disk_HasFilesystem.
 */
#define DISK_HAS_NO_FS   0
#define DISK_HAS_OFS     1
#define DISK_HAS_LFS     2

/*
 * Forward Declarations.
 */
Disk_Label		*Disk_ReadLabel();
int			Disk_WriteLabel();
int			Disk_EraseLabel();
Disk_Label		*Disk_NewLabel();
Dec_DiskLabel		*Disk_ReadDecLabel();
Sun_DiskLabel		*Disk_ReadSunLabel();
Fsdm_DiskHeader		*Disk_ReadDiskHeader();
Ofs_SummaryInfo		*Disk_ReadSummaryInfo();
int			Disk_WriteSummaryInfo();
Ofs_DomainHeader	*Disk_ReadDomainHeader();
int			Disk_WriteDomainHeader();
void			Disk_PrintDomainHeader();
void			Disk_PrintSummaryInfo();
int			Disk_BlockWrite();
int			Disk_SectorWrite();
int			Disk_BlockRead();
int			Disk_SectorRead();
int			Disk_BadBlockRead();
void			Disk_PrintLabel();
char			*Disk_GetLabelTypeName();
void			Disk_PrintFileDescBitmap();
void			Disk_PrintDataBlockBitmap();
void			Disk_PrintDirEntry();


int                     Disk_HasFilesystem();
LfsSuperBlock*          Disk_ReadLfsSuperBlock();
ReturnStatus            Disk_WriteLfsSuperBlock();
LfsCheckPointHdr*       Disk_ReadLfsCheckPointHdr();
ReturnStatus            Disk_WriteLfsCheckPointHdr();
ReturnStatus            Disk_WriteLfsCheckPointArea();
LfsCheckPointTrailer*   Disk_LfsCheckPointTrailer();
ReturnStatus            Disk_ForEachCheckPointRegion();
void                    Disk_PrintLfsSuperBlockHdr();
void                    Disk_PrintLfsStableMemParams();
void                    Disk_PrintLfsDescMapParams();
void                    Disk_PrintLfsSegUsageParams();
void                    Disk_PrintLfsFileLayoutParams();
void                    Disk_PrintLfsSuperBlock();
void                    Disk_PrintLfsCheckPointHdr();
void                    Disk_PrintLfsCheckPointRegion();
void                    Disk_PrintLfsCheckPointTrailer();

#endif DISK


