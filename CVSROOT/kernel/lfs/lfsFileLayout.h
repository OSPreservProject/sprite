/*
 * lfsFileLayout.h --
 *
 *	Declarations of data structures describing the layout of 
 *	files and descriptors in LFS segments.
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

#ifndef _LFSFILELAYOUT
#define _LFSFILELAYOUT

/*
 * LfsFileLayoutSummary defines the format of file layout summary bytes.
 * The LfsFileLayoutSummary maybe followed by zero or more bytes of
 * summary info describing the 
 */

typedef struct LfsFileLayoutSummary {
    unsigned short	blockType;     /* Type of data block. See below. */
    unsigned short	numBlocks;     /* Number of file system blocks covered
					* by this summary. */
    int	fileNumber;  		       /* File number of block(s) owner. */
    unsigned short 	truncVersion;  /* Truncate version number used for
					* cleaning files. */
    unsigned short      numDataBlocks; /* Number of file block ranges after 
				        * this structure. */
    /*
     * LfsFileLayoutSummary is followed by numDataBlocks logical block
     * numbers.
     */
} LfsFileLayoutSummary;
/*
 * LfsFileLayoutLog describes the format of directory log in the summary
 * region. 
 * The LfsFileLayoutLog will be followed by numBytes bytes of log.
 */

typedef struct LfsFileLayoutLog {
    unsigned short	blockType;     /* Type of data block. See below. */
    unsigned short	numBytes;     /* Number of file system blocks covered
				       * by this summary. */
    /*
     * LfsFileLayoutLog is followed by numBytes bytes of log entries.
     */
} LfsFileLayoutLog;

/*
 * LfsFileLayoutDesc describes the format of summary record for 
 * descriptor blocks. */
typedef struct LfsFileLayoutDesc {
    unsigned short	blockType;     /* Type of data block. See below. */
    unsigned short	numBlocks;    /* Number of file system blocks covered
				       * by this summary. */
} LfsFileLayoutDesc;

/*
 * Valid blockType fields.
 */
#define	LFS_FILE_LAYOUT_DESC	     0x0   /* File descriptor block. */
#define	LFS_FILE_LAYOUT_DATA	     0x1   /* File data block. */
#define	LFS_FILE_LAYOUT_INDIRECT     0x2   /* File indirect block. */
#define	LFS_FILE_LAYOUT_DBL_INDIRECT 0x3   /* File double indirect block. */
#define	LFS_FILE_LAYOUT_DIR_LOG	     0x4   /* Directory log data. */


#define	LFS_FILE_LAYOUT_PARAMS_SIZE 32
typedef struct LfsFileLayoutParams {
    int	 descPerBlock;	/* Number of file descriptors to pack together per
			 * block.  */
    char pad[LFS_FILE_LAYOUT_PARAMS_SIZE - 4];

} LfsFileLayoutParams;

/*
 * Routines for lint purposes only.
 */

extern ReturnStatus LfsFileLayoutAttach();
extern Boolean	LfsFileLayoutProc(), LfsFileLayoutClean();
extern Boolean LfsFileLayoutCheckpoint();
extern void LfsFileLayoutWriteDone();

#endif /* _LFSFILELAYOUT */

