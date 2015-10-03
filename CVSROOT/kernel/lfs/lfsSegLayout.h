/*
 * lfsSegLayout.h --
 *
 *	Declarations of the structures defining the resident image
 *	of an LFS segment. An LFS segment is divided into two 
 *	regions the summary region and data block region. The
 *	summary region is used the identify the blocks in the data 
 *	block region during clean and aid in crash recovery.
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

#ifndef _LFSSEGLAYOUT
#define _LFSSEGLAYOUT


/*
 * The summary area of each segment contains a LfsSegSummary structured 
 * followed by Lfs*Summary for each region type with blocks in the segment.
 * Each region summary type is started by a LfsSegSummaryHdr.
 */
typedef struct LfsSegSummary {
    unsigned int  magic;	/* Better be LFS_SEG_SUMMARY_MAGIC. */
    unsigned int  timestamp;    /* Timestamp of last write. */
    unsigned int  prevSeg;      /* The previous segment written. */
    unsigned int  nextSeg;      /* The next segment to write. */
    int  size;			/* The size of this segment's summary region
				 * in bytes including this structure. */
    int nextSummaryBlock;	/* The block offset of the next summary block 
				 * in this segment segment. -1 if this is the
				 * last summary block in this segment. */
} LfsSegSummary;

#define	LFS_SEG_SUMMARY_MAGIC	0x1065e6	/* logseg */

typedef struct LfsSegSummaryHdr {
    unsigned short moduleType;	   /* Module type of this summary region. */
    unsigned short lengthInBytes;  /* Length of this summary region in bytes. */
    int   numDataBlocks; 	  /* Number data blocks described by this 
				    * region. */
} LfsSegSummaryHdr;

/*
 * A list of module type and their priorities. 
 */
#define	LFS_FILE_LAYOUT_MOD 0
#define	LFS_DESC_MAP_MOD    1
#define	LFS_SEG_USAGE_MOD   2
#define	LFS_MAX_NUM_MODS    3

#endif /* _LFSSEGLAYOUT */

