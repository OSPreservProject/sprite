/*
 * lfsSeg.h --
 *
 *	Declarations of segment writing/cleaning and checkpoint interface
 *	for LFS.  Any module that wishes to write objects to the log
 *	must register itself with the lfsSeg module and support this
 *	interface.
 *	
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

#ifndef _LFSSEG
#define _LFSSEG

#include "lfsInt.h"
#include "lfsSegLayout.h"

/* 
 * The interface to writing objects to the LFS log is operated uses a 
 * callback interface.  Any module may start a segment write by calling
 * the LfsSegWriteStart(). The segment write code (lfsSeg.c) then
 * selects a clean segment on disk, initializes a LfsSeg structure for that
 * segment, and calls the Layout() subroutine for each module.  The Layout()
 * adds data blocks and/or summary bytes to the segment. Data blocks
 * added to the LfsSeg may use a scatter/gather interface
 * consisting of LfsSegElement structures will each SegElement pointing to
 * zero or more blocks. Once the segment
 * is full or all modules exhaust there need to write, the segment is pushed
 * to disk and WriteDone() callback is done for each module writing data.
 * 
 */

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

/*
 * LfsSegLogRange describes the position in the log of the segment. 
 */
typedef struct LfsSegLogRange {
    int		current;	/* Current segment being written. */
    int		prevSeg;	/* Previous segment that was written. */
    int		nextSeg;	/* Next segment to be written. */
} LfsSegLogRange;
/*
 * The LfsSeg structure is used to describe a segment while objects are being
 * added to the segment during writing and removed from the segment during
 * cleaning.  A segment is divided into two regions: the data block region
 * and the summary region.  The data block region is may contain zero to 
 * the segment size (segmentSize in LfsSuperBlockHdr)-1 blocks. The blocks are
 * pointed to by an array of LfsSegElement.  The summary region is allocated
 * in bytes and may grow from one block to an entire segment.
 */

typedef struct LfsSeg {
    Lfs	         *lfsPtr;	/* The LFS file system this segment belongs.
				 * Static segment attributes such as number
				 * blocks can be found in here. */
    char	 *segMemPtr;	/* Memory allocated for segment.  */
    LfsSegElement *segElementPtr; /* The SegElements making up the data 
				   * region of the segment. */
    char    *summaryPtr;      /* A pointer to the start of the summary
			       * region of this segment. */
    LfsSegLogRange logRange;	/* Placement of segment in segmented log. */
    int	    numElements;      /* Number of LfsSegElement describing the 
			       * segment. */
    int	    numDataBlocks;    /* Number of data blocks in segment. */
    char    *summaryLimitPtr; /* The last byte of the summary region + 1.*/
	/*
	 * Some operations of segments require scanning thru the summary 
	 * and SegElements.  The following fields keep the start require
	 * from this scans. 
	 */
    LfsSegSummaryHdr    *curSummaryHdrPtr; 
				/* Current module header being processed. */
    int	     curElement;	/* Current LfsSegElement. */
    int	     curBlockOffset;    /* The block offset into the segment of the
				 * end of curElement. */
    int	     curDataBlockLimit;   /* Last block offset available to the data
				   * portion of the segment. */
    char     *curSummaryPtr;    /* Current offset into the summary region. */
    char     *curSummaryLimitPtr; /* Current last byte of the summary reigon 
				   * plus 1. */
} LfsSeg;

/*
 * The callback interface used by objects wishing to write data to the log is
 * defined by the LfsSegIoInterface.   
 * The exact function each of these layout procedures is 
 * defined in LfsSeg.c
 */

typedef struct LfsSegIoInterface {

    ReturnStatus (*attach)(); 
	/* File system attach routine. Calling sequence:
	 * attach(lfsPtr, checkPointSize, checkPointPtr)
	 *    Lfs *lfsPtr; -- File system to attach.
	 *    int  checkPointSize; Size of checkpoint data
	 *    char *checkPointPtr; Data from last checkpoint
	 * Attach is called when the file system is attached and is responsible
	 * for build any in memory data structures needed by the file system.
	 * The routine returns SUCCESS if things are going ok.
	 */

    Boolean (*layout)();   
	/* Segment layout routine. Calling sequence:
	 * layout(segPtr)
	 *     LfsSeg *segPtr; -- Segment to fill in.
	 * The module should place all the dirty blocks it can into the log.
	 * The routine returns TRUE if the module has more data that needs to
	 * be written to the log. 
	 */

    Boolean (*clean)();  
	/* Segment cleaning routine. Calling sequence:
	 * clean(segToCleanPtr, segPtr)
	 *     LfsSeg *segToCleanPtr;  -- Segment to clean.
	 *     LfsSeg *segPtr; -- Segment to fill in.
	 * Copy the alive blocks from the segToClean into segPtr. 
	 * The routine returns TRUE if the module has more data that needs to
	 * be written to the log. 
	 */
    Boolean (*checkpoint)();
	/* Segment checkpoint routine. Calling sequence:
	 * checkpoint(segPtr, flags, checkPointPtr,  checkPointSizePtr)
	 *     LfsSeg *segPtr; -- Segment to fill in.
	 *     int	flags;    -- Defined in lfsInt.h
	 *     char *checkPointPtr; -- Checkpoint buffer.
	 *     int  *checkPointSizePtr -- Out: bytes added to checkpoint area.
	 * The routine returns TRUE if the module has more data that needs to
	 * be written to the log. 
	 */
    void  (*writeDone)(); 
	/* Segment write finished callback. Calling seq:
	 * writeDone(segPtr, flags)
	 *     LfsSeg *segPtr; -- Segment finishing write.
	 *     int	flags;    -- Defined in lfsInt.h.
	 * Called when a segment write finishes.
	 */

    int	clientDataPerSegSize; /* Size in bytes of client data for each segment
			       * for this module. */
} LfsSegIoInterface;



extern LfsSegIoInterface *lfsSegIoInterfacePtrs[];

/*
 * Macros for uses in SegIoInterface callback procedures. 
 *
 * See documentation in the procedure header for the slow version.
 */
/*
 * Increase the size of the summary region to bytesNeeded bytes.
 * char *
 * LfsSegGrowSummary(segPtr, blocks,  bytesNeeded)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 *	int	blocks;		-- Insure this many data blocks remain.
 *	int	bytesNeeded;    -- Number of bytes needed in the summary
 *				   region
 */
#define	LfsSegGrowSummary(segPtr, blocks, bytesNeeded) \
		LfsSegSlowGrowSummary((segPtr), (blocks), (bytesNeeded))

/*
 * Return the value of the current pointer into the summary region.
 * char *
 * LfsSegGetSummaryPtr(segPtr)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 */
#define	LfsSegGetSummaryPtr(segPtr)  ((segPtr)->curSummaryPtr)

/*
 * Set the value of the current end of summary region.
 * char *
 * LfsSegSegSummaryPtr(segPtr, newEndPtr)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 *	char	*newEndPtr;	-- New value
 */
#define	LfsSegSetSummaryPtr(segPtr, newEndPtr)  \
	((segPtr)->curSummaryPtr = (newEndPtr))

/*
 * Return the number of summary bytes left in the summary segment.
 * int
 * LfsSegSummaryBytesLeft(segPtr)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 */

#define	LfsSegSummaryBytesLeft(segPtr) \
	((segPtr)->curSummaryLimitPtr - (segPtr)->curSummaryPtr)

/*
 * Return the number of file system blocks left in the segment.
 * int
 * LfsSegBlocksLeft(segPtr)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 */
#define	LfsSegBlocksLeft(segPtr) ((segPtr)->curDataBlockLimit - \
				  (segPtr)->curBlockOffset)

/*
 * Return the disk address of an LfsSegElement
 * int
 * LfsSegDiskAddress(segPtr, segElementPtr)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 *	LfsSegElement *segElementPtr; -- Segment element of interest
 */

#define	LfsSegDiskAddress(segPtr, segElementPtr) \
		LfsSegSlowDiskAddress((segPtr), (segElementPtr))
/*
 * Add a LfsSegElement to a segment.
 * LfsSegElement
 * LfsSegAddDataBuffer(segPtr, blocks, bufferPtr, clientData)
 *	LfsSeg	*segPtr; 	-- Segment of interest. 
 *	int	blocks;		-- Size of buffer to add in fsBlocks.
 *	char   *bufferPtr;	-- Buffer to add.
 *	ClientData clientData;	-- ClientData associated with this field.
 */

#define	LfsSegAddDataBuffer(segPtr, blocks, bufferPtr, clientData) \
	LfsSegSlowAddDataBuffer((segPtr), (blocks), (bufferPtr), (clientData))

#define	LfsSegGetBufferPtr(segPtr) \
			((segPtr)->segElementPtr + (segPtr)->curElement)
#define	LfsSegSetBufferPtr(segPtr, bufferPtr) \
		((segPtr)->curElement = ((bufferPtr) - (segPtr)->segElementPtr))

#define	LfsSegSetCurBlockOffset(segPtr, blockOffset) \
			((segPtr)->curBlockOffset = blockOffset)

#define	LfsSegFetchBytes(segPtr, blockOffset, size) \
		(LfsSegGetBufferPtr(segPtr)->address + \
			LfsBlocksToBytes((segPtr)->lfsPtr, blockOffset))
/* procedures */

extern void LfsSegIoRegister();
extern void LfsSegWriteStart();
extern char *LfsSegSlowGrowSummary();
extern LfsSegElement *LfsSegSlowAddDataBuffer();
extern unsigned int LfsSegSlowDiskAddress();
extern int   LfsSegSlowBlocksLeft();
extern Boolean LfsSegNullLayout();


#endif /* _LFSSEG */

