/*
 * lfslib.h --
 *
 *	Declarations of user level routines for accessing LFS file system
 *	data structures.  This routines should be used for recovery and
 *	other programs that need scan an LFS file system.
 *	
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
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.5 90/01/12 12:03:25 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _LFSLIB
#define _LFSLIB

#if defined(__STDC__) || defined(SABER)
/*
 * Enable the function prototypes for standard C compilers. They also
 * work when using SaberC.
 */
#define _HAS_PROTOTYPES
#endif

#include <cfuncproto.h>
#include <sprite.h>
#include <bit.h>
#include <fs.h>
#include <list.h>
#include <kernel/fs.h>
#include <kernel/dev.h>
#include <kernel/fsdm.h>
#include <kernel/fslcl.h>
#include <kernel/devDiskLabel.h>
#include <kernel/lfsDesc.h>
#include <kernel/lfsDescMap.h>
#include <kernel/lfsFileLayout.h>
#include <kernel/lfsDirOpLog.h>
#include <kernel/lfsSegLayout.h>
#include <kernel/lfsStableMem.h>
#include <kernel/lfsSuperBlock.h>
#include <kernel/lfsUsageArray.h>
#include <kernel/lfsStats.h>

/* constants */

/* data structures */
typedef struct LogPoint {
    int	segNo;		/* LFS segment number. */
    int blockOffset;	/* Block offset into segment. */
} LogPoint;

/*
 * LfsCheckPoint contains the info and memory needed to perform checkpoints.
 * The file system timestamp and the next checkpoint area to write
 * indicator are kept here. 
 */
typedef struct LfsCheckPoint {
    int	  timestamp;	/* Current file system timestamp. */
    int	  nextArea;	/* Next checkpoint area to write. Must be 0 or 1. */
    char  *buffer;	/* Memory buffer to place checkpoint. */
    int	  maxSize;	/* Maximum size of the buffer. */
} LfsCheckPoint;

/*
 * LfsSegLogRange describes the position in the segmented log of a segment. 
 */
typedef struct LfsSegLogRange {
    int		current;	/* Current segment being accessed */
    int		prevSeg;	/* The segment that was written before the
				 * current segment. */
    int		nextSeg;	/* Next segment to be written after the
				 * current segment. */
} LfsSegLogRange;

/*
 * Data blocks are added to LfsSeg structures in units of LfsSegElement's.
 */
typedef struct LfsSegElement {
    Address  	 address;         /* The address of the start of the buffer. */
    unsigned int lengthInBlocks;  /* Length of the buffer in units of file 
				   * system blocks. */
    ClientData   clientData;	  /* ClientData usable by the callback 
				   * functions. */
} LfsSegElement;


typedef struct LfsDescMap {
    LfsDescMapCheckPoint checkPoint;
    ClientData		smemPtr;
} LfsDescMap;

typedef struct LfsSegUsage {
    LfsSegUsageCheckPoint checkPoint;
    ClientData		smemPtr;
} LfsSegUsage;

typedef struct LfsFileLayout {
    List_Links	activeFileListHdr;
} LfsFileLayout;

/*
 * Lfs - the main data structured used by these library routines to access
 * 	 an LFS file system.
 */
typedef struct Lfs {
    int		diskFd;			/* Open file descriptor of LFS disk. */
    ClientData	diskCache;		/* Cache of disk blocks. */
    int		superBlockOffset;	/* Offset of the super block. */
    char	*deviceName;		/* Name of the device with LFS. */
    char	*programName;		/* Name of program being run. */

    LfsSuperBlock superBlock; /* The superblock of the file system */
    LfsCheckPoint checkPoint;   /* Checkpoint data. */

    LfsCheckPointHdr	*checkPointHdrPtr;  /* Current checkpoint header. */
    char	  *name;		/* Name used for error messages. */
    Lfs_Stats	stats;		/* Current stats. */
    LfsDescMap	  descMap;	/* Descriptor map data. */
    LfsSegUsage   usageArray;   /* Segment usage array data. */
    LfsFileLayout fileLayout;	/* File layout data structures. */
    LogPoint	logEnd;	     	       /* End of log. */
    struct { 
	int blocksReadDisk;	    /* Number of blocks read from disk. */
	int blocksWrittenDisk;      /* Number of blocks written to disk. */
	int writeSummaryBlock;	    /* Summary blocks written. */
	int segUsageBlockWrite;	    /* Seg usage block written. */
	int descMapBlockWrite;	    /* Desc map block written. */
	int fileWritten;	    /* Number of files written. */
	int fileBlockWritten;	    /* File blocks (512) written. */
	int descWritten;	    /* Number of desc written. */
    } pstats;			/* Program stats. */
} Lfs;

/*
 * Structure used to address segments on disk.
 */
typedef struct LfsSeg {
    Lfs	  *lfsPtr;		/* File system of segemnt. */
    int	  segNo;		/* Segment number. */
    LfsSegElement *segElementPtr; /* The SegElements making up the data 
				   * region of the segment. */
    char	   *memPtr;	/* Segment memory allocated for segment. */
    LfsSegLogRange logRange;	/* Placement of segment in segmented log. */
    int	    numElements;      /* Number of LfsSegElement describing the 
			       * segment. */
    int	    numBlocks;        /* Number of blocks in segment. */
    int	    startBlockOffset; /* Starting block offset of this segment. */
    int	    activeBytes;      /* Number of active bytes in segment. */
	/*
	 * Some operations of segments require scanning thru the summary 
	 * and SegElements.  The following fields keep the start require
	 * from this scans. 
	 */
    LfsSegSummary	*curSegSummaryPtr; /* Summary of current summary 
					    * block. */
    LfsSegSummaryHdr    *curSummaryHdrPtr; 
				/* Current module header being processed. */
    int	     curElement;	/* Current LfsSegElement. */
    int	     curBlockOffset;    /* The block offset into the segment of the
				 * end of curElement. */
    int	     curDataBlockLimit;   /* Last block offset available in the 
				   * segment. */
    char     *curSummaryPtr;    /* Current offset into the summary region. */
    char     *curSummaryLimitPtr; /* Current last byte of the summary block 
				   * plus 1. */

} LfsSeg;

/*
 * Structure used to keep track of open and active files.
 */
typedef struct LfsFile {
    List_Links	links;	   /* For lists of files. MUST BE FIRST. */
    Lfs		*lfsPtr;   /* File system of file. */
    LfsFileDescriptor desc; /* Copy of file's file descriptor. */
    List_Links	blockListHdr; /* Header for list of file's blocks in memory. */
    Boolean	descModified; /* The descriptor was modified. */
    Boolean	beingWritten;	/* The file is being written. */
} LfsFile;

typedef struct LfsFileBlock {
    List_Links	links;	   /* For lists of blocks. MUST BE FIRST. */
    LfsFile	*filePtr;  /* File in which this block resides. */
    int		blockNum;  /* Block number in file. */
    int		blockSize; /* Size of the block in bytes. */
    int		blockAddr; /* Block disk address of this block. FSDM_NIL_INDEX
			    * if the block hasn't been allocated to disk yet. */
    Boolean	modified;  /* The block has been modified in memory. */
    char	contents[FS_BLOCK_SIZE]; /* The contents of the block. */
} LfsFileBlock;
/*
 * Macros
 */
/*
 *	LfsGetDescMapEntry() - Return the descriptor map entry for a file number.
 *	LfsGetUsageArrayEntry() - Return an usage array entry for a segment number.
 *	LfsGetDescMapBlockIndex() - Return the block index of a desc map block.
 *	LfsGetUsageArrayBlockIndex() - Return the block index of a usage array block.
 *	
 */

#define	LfsGetDescMapEntry(lfsPtr, fileNumber) \
  ((LfsDescMapEntry *) LfsGetStableMemEntry((lfsPtr)->descMap.smemPtr, fileNumber))
#define	LfsDescMapEntryModified(lfsPtr, fileNumber) \
   (LfsMarkStableMemEntryDirty((lfsPtr)->descMap.smemPtr, fileNumber))
#define	LfsGetUsageArrayEntry(lfsPtr, segNo) \
   ((LfsSegUsageEntry *) LfsGetStableMemEntry((lfsPtr)->usageArray.smemPtr, segNo))
#define	LfsUsageArrayEntryModified(lfsPtr, segNo) \
    (LfsMarkStableMemEntryDirty((lfsPtr)->usageArray.smemPtr, segNo))

#define LfsGetDescMapBlockIndex(lfsPtr, blockNum) \
    LfsGetStableMemBlockIndex((lfsPtr)->descMap.smemPtr, blockNum)
#define LfsGetUsageArrayBlockIndex(lfsPtr, blockNum) \
    LfsGetStableMemBlockIndex((lfsPtr)->usageArray.smemPtr, blockNum)


#define	LfsFileFetchDesc(filePtr) (&((filePtr)->desc))
#define	LfsFileBlockMem(fileBlockPtr)	((fileBlockPtr)->contents)
#define	LfsFileBlockSize(fileBlockPtr)	((fileBlockPtr)->blockSize)
#define	LfsFileBlockAddress(fileBlockPtr)	((fileBlockPtr)->blockAddr)


/* Useful macros for LFS.
 *
 * LfsSegSize(lfsPtr)	- Return the segment size in bytes.
 * LfsSegSizeInBlocks(lfsPtr) - Return the segment size in blocks.
 * LfsBlockSize(lfsPtr)       - Return the block size.
 * LfsBytesToBlocks(lfsPtr, bytes) - Convert bytes into the number of blocks
 *				     it would take to contain the bytes.
 * LfsBlocksToBytes(lfsPtr, blocks) - Convert from blocks into bytes.
 * LfsSegNumToDiskAddress(lfsPtr, segNum) - Convert a segment number into
 *					    a disk address.
 * LfsBlockToSegmentNum(lfsPtr, diskAdress)  - Compute the segment number 
 *
 * LfsGetCurrentTimestamp(lfsPtr) - Return the current file system timestamp
 */


#define	LfsBlockSize(lfsPtr) ((lfsPtr)->superBlock.hdr.blockSize)
#define	LfsSegSize(lfsPtr) ((lfsPtr)->superBlock.usageArray.segmentSize)

#define	LfsSegSizeInBlocks(lfsPtr) (LfsSegSize(lfsPtr)/LfsBlockSize(lfsPtr))


#define	LfsBytesToBlocks(lfsPtr, bytes)	\
	 (((bytes) + (LfsBlockSize(lfsPtr)-1))/LfsBlockSize(lfsPtr))

#define	LfsBlocksToBytes(lfsPtr, blocks) ((blocks)*LfsBlockSize(lfsPtr))


#define LfsValidSegmentNum(lfsPtr, segNum) (((segNum) >= 0) && \
		((segNum) < (lfsPtr)->superBlock.usageArray.numberSegments))


#define LfsSegNumToDiskAddress(lfsPtr, segNum) \
		     (((lfsPtr)->superBlock.hdr.logStartOffset + \
		(LfsSegSizeInBlocks((lfsPtr)) * (segNum))))

#define LfsDiskAddrToSegmentNum(lfsPtr, diskAddress) \
		(((diskAddress) - \
				(lfsPtr)->superBlock.hdr.logStartOffset) / \
					 LfsSegSizeInBlocks((lfsPtr)))

#define	LfsGetCurrentTimestamp(lfsPtr)	(++((lfsPtr)->checkPoint.timestamp))


/* procedures */

extern Lfs *LfsLoadFileSystem _ARGS_((char *programName, char *deviceName, 
		int blockSize, int  superBlockOffset, int flags));

extern int LfsGetStableMemBlockIndex _ARGS_((ClientData clientData, int blockNum));
extern char *LfsGetStableMemEntry _ARGS_((ClientData clientData, int entryNumber));

extern void LfsMarkStableMemEntryDirty _ARGS_((ClientData clientData, 
					int entryNumber));

extern int LfsDiskRead _ARGS_((Lfs *lfsPtr, int blockOffset, int bufferSize, 
			    char *bufferPtr));

extern int LfsDiskWrite _ARGS_((Lfs *lfsPtr, int blockOffset, int bufferSize, 
			    char *bufferPtr));

extern void LfsDiskCache _ARGS_((Lfs *lfsPtr, int maxCacheBytes));

extern void LfsDiskCacheInvalidate _ARGS_((Lfs *lfsPtr, int blockOffset, 
					int blockSize));

extern LfsSeg *LfsSegInit _ARGS_((Lfs *lfsPtr, int segNumber));
extern int LfsSegStartAddr _ARGS_((LfsSeg *segPtr));
extern char *LfsSegFetchBlock _ARGS_((LfsSeg *segPtr, int blockOffset, int size));
extern void LfsSegReleaseBlock _ARGS_((LfsSeg *segPtr, char *memPtr));
extern void LfsSegRelease _ARGS_((LfsSeg *segPtr));

extern ReturnStatus LfsFileOpen _ARGS_((Lfs *lfsPtr, int fileNumber,
				     LfsFile **filePtrPtr));
extern ReturnStatus LfsFileOpenDesc _ARGS_((Lfs *lfsPtr, 
			LfsFileDescriptor *descPtr, LfsFile **filePtrPtr));
extern void	    LfsFileClose _ARGS_((LfsFile *filePtr));

extern ReturnStatus LfsFileBlockFetch _ARGS_((LfsFile *filePtr, int blockNum, 
				LfsFileBlock **fileBlockPtrPtr));

extern ReturnStatus LfsFileBlockAlloc _ARGS_((LfsFile *filePtr, int blockNum, 
			int blocksize, 	LfsFileBlock **fileBlockPtrPtr));

extern int LfsFileBlockAddr _ARGS_((LfsFile *filePtr, int blockNum,
					 int *blocksizePtr));

extern ReturnStatus LfsFileStoreDesc _ARGS_((LfsFile *filePtr));

extern ReturnStatus LfsFileStoreBlock _ARGS_((LfsFileBlock *fileBlockPtr,
						int newSize));

extern ReturnStatus LfsFileAddToDirectory _ARGS_((LfsFile *dirFilePtr, 
						 LfsFile *filePtr, 
						 char *name));

extern ReturnStatus LfsFileNameLookup _ARGS_((LfsFile *dirFilePtr, 
						 char *name,
						 int *fileNumberPtr, 
						 int *dirOffsetPtr));

extern void LfsSegUsageAdjustBytes _ARGS_((Lfs *lfsPtr, int diskAddr, 
					int changeInBytes));



extern ReturnStatus LfsDescMapGetVersion _ARGS_((struct Lfs *lfsPtr,
			int fileNumber, unsigned short *versionNumPtr));
extern ReturnStatus LfsDescMapGetDiskAddr _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, LfsDiskAddr *diskAddrPtr));
extern ReturnStatus LfsDescMapSetDiskAddr _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, LfsDiskAddr diskAddr));
extern ReturnStatus LfsDescMapGetAccessTime _ARGS_((struct Lfs *lfsPtr,
			int fileNumber, int *accessTimePtr));
extern ReturnStatus LfsDescMapSetAccessTime _ARGS_((struct Lfs *lfsPtr, 
			int fileNumber, int accessTime));

extern ReturnStatus LfsGetNewFileNumber _ARGS_((struct Lfs *lfsPtr, 
			int dirFileNumber, int *fileNumberPtr));


extern ReturnStatus LfsGetLogTail _ARGS_((struct Lfs *lfsPtr, Boolean cantWait,
			LfsSegLogRange *logRangePtr, int *startBlockPtr ));

extern void LfsSetLogTail _ARGS_((struct Lfs *lfsPtr, 
			LfsSegLogRange *logRangePtr, int startBlock, 
			int activeBytes));

extern ReturnStatus LfsCheckPointFileSystem _ARGS_((Lfs *lfsPtr, int flags));

#endif /* _LFSLIB */

