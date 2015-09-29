
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
 * The LfsSeg structure is used to describe a segment while objects are being
 * added to the segment during writing and removed from the segment during
 * cleaning.  A segment is divided into two regions: the data block region
 * and the summary region.  The data block region is may contain zero to 
 * the segment size (segmentSize in LfsSuperBlockHdr)-1 blocks. The blocks are
 * pointed to by an array of LfsSegElement.  The summary region is allocated
 * in bytes and may grow from one block to an entire segment.
 */

typedef struct LfsSeg {
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
 *				   region.
 */
#define	LfsSegGrowSummary(segPtr, blocks, bytesNeeded) \
	LfsSegSlowGrowSummary((segPtr), (blocks), (bytesNeeded), FALSE)

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
	((segPtr)->memPtr + LfsSegSize((segPtr)->lfsPtr) - \
		LfsBlocksToBytes((segPtr)->lfsPtr, blockOffset))
/* procedures */

extern char *LfsSegSlowGrowSummary _ARGS_((LfsSeg *segPtr, 
			int dataBlocksNeeded, int sumBytesNeeded, 
			Boolean addNewBlock));
extern LfsDiskAddr LfsSegSlowDiskAddress _ARGS_((LfsSeg *segPtr, 
			LfsSegElement *segElementPtr));
extern LfsSegElement *LfsSegSlowAddDataBuffer _ARGS_((LfsSeg *segPtr,
			int blocks, char *bufferPtr, ClientData clientData));

extern Boolean WriteSegCheckpoint _ARGS_((int diskId, char *checkPointPtr,
					 int	*checkPointSizePtr));


extern Boolean LfsSegUsageCheckpoint _ARGS_((LfsSeg *segPtr, 
		char *checkPointPtr, int *checkPointSizePtr));
extern Boolean LfsDescMapCheckpoint _ARGS_((LfsSeg *segPtr, 
		char *checkPointPtr, int  *checkPointSizePtr));

extern void LfsSegUsageCheckpointUpdate _ARGS_((char *checkPointPtr, int size));

extern ReturnStatus LfsGetLogTail _ARGS_((
			LfsSegLogRange *logRangePtr, int *startBlockPtr ));

extern void LfsSetLogTail _ARGS_((
			LfsSegLogRange *logRangePtr, int startBlock, 
			int activeBytes));

