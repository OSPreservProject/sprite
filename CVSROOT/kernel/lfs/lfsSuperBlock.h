/*
 * lfsSuperBlock.h --
 *
 *	Declarations defining the disk resident format of the LFS 
 *	super block. The main purpose of the super block is to allow
 *	the file system to be recovered and reattached upon file server
 *	reboot. The super block is divided into two section the static
 *	super block and the checkpoint area.  The super block
 *	contains the nonchangable parameters that describe the LFS. 
 *	The checkpoint area contains the parameters that change when the
 *	file system is modified.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _LFSSUPERBLOCK
#define _LFSSUPERBLOCK

#ifdef EKERNEL
#include <lfsDescMap.h>
#include <lfsUsageArray.h>
#include <lfsFileLayout.h>
#else
#include <kernel/lfsDescMap.h>
#include <kernel/lfsUsageArray.h>
#include <kernel/lfsFileLayout.h>
#endif
/*
 * The LfsSuperBlockHdr contains static parameters describing the file system
 * layout on disk. 
 */
#define	LFS_SUPER_BLOCK_HDR_SIZE	128
typedef struct LfsSuperBlockHdr {
    unsigned int magic;		/* Better be LFS_SUPER_BLOCK_MAGIC. */
    unsigned int version;  	/* Version number describing the format used
				 * for this LFS.  */
    int blockSize;		/* The block size of this file system. Should
				 * be set to the minumum addressable unit. */
	/*
	 * File system layout. 
	 */
    int maxCheckPointBlocks;  /* Maximum size of checkpoint region in blocks. */
    int checkPointOffset[2];/* The block offset into the device of the
			     * two checkpoint areas. Two areas are
			     * used so we never update in place. The
			     * format the segment is defined below. */
    int logStartOffset;     /* The block offset starting the segmented log. */
    int	 checkpointInterval;	/* Frequency of checkpoint in seconds. */
    int  maxNumCacheBlocks;     /* Maximum number of blocks to clean at time.*/
    char reserved[LFS_SUPER_BLOCK_HDR_SIZE-9*sizeof(int)];
			    /* Reserved, must be set to zero. */

} LfsSuperBlockHdr;

#define	LFS_SUPER_BLOCK_MAGIC		0x106d15c 	/* LogDisc */
#define	LFS_SUPER_BLOCK_VERSION		1
#define	LFS_SUPER_BLOCK_SIZE		512
#define LFS_SUPER_BLOCK_OFFSET		64
/*
 * The format a LFS super block. 
 */
typedef struct LfsSuperBlock {
    LfsSuperBlockHdr  hdr;	/* Header describing the layout of the LFS. */
    int		reserved;	/* Reseved field must be zero. */
    LfsDescMapParams  descMap;	/* Descriptor map parameters. */
    LfsSegUsageParams usageArray; /* The segment usage map parameters. */
    LfsFileLayoutParams fileLayout; /* Parameters describing file layout. */
    char padding[LFS_SUPER_BLOCK_SIZE-sizeof(LfsFileLayoutParams) - 
		 sizeof(LfsSegUsageParams)-sizeof(LfsDescMapParams) -
		 sizeof(int) - sizeof(LfsSuperBlockHdr)];	
} LfsSuperBlock;


/*
 * Format of the LFS checkpoint areas.  The checkpoint area consists of
 * a LfsCheckPointHdr structure followed by zero or more LfsCheckPointRegion
 * from each module.  The last LfsCheckPointRegion is ended with a 
 * LfsCheckPointTrailer.
 */

typedef struct LfsCheckPointHdr {
    unsigned int timestamp;	/* Timestamp of this checkpoint. */
    int size;			/* Size of checkpoint in bytes. */
    unsigned int version;	/* Region write version number. */
    char domainPrefix[64];	/* Last prefix used for the domain */
    int	 domainNumber;		/* Last domain we ran under. */
    int	 attachSeconds;		/* Time the disk was attached */
    int	 detachSeconds;		/* Time the disk was off-lined. */
    int	 serverID;		/* Sprite ID of server. */
} LfsCheckPointHdr;

typedef struct LfsCheckPointRegion {
    unsigned int type;		/* Region type -- see log writing types in
				 * lfsLogFormat.h. */
    int size;			/* Size of the region in bytes. */
} LfsCheckPointRegion;

typedef struct LfsCheckPointTrailer {
    unsigned int timestamp;	/* Timestamp of this checkpoint. Must match
				 * the checkpoint on header. */
    unsigned int checkSum;	/* A checksum of the checkpoint check used to
				 * detect partial checkpoint writes. */
} LfsCheckPointTrailer;


#define	LFS_SUPER_BLOCK_SIZE_OK() \
	((sizeof(LfsSuperBlock) == LFS_SUPER_BLOCK_SIZE) &&		\
	 (sizeof(LfsSuperBlockHdr) == LFS_SUPER_BLOCK_HDR_SIZE) &&	\
	 (sizeof(LfsDescMapParams) == LFS_DESC_MAP_PARAM_SIZE) &&	\
	 (sizeof(LfsSegUsageParams) == LFS_USAGE_ARRAY_PARAM_SIZE) &&   \
	 (sizeof(LfsFileLayoutParams) == LFS_FILE_LAYOUT_PARAMS_SIZE))

#endif /* _LFSSUPERBLOCK */
