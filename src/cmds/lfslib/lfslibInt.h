/*
 * lfsInt.h --
 *
 *	Data structures and routines internal to the Lfs library.
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

#ifndef _LFSINT
#define _LFSINT

/* constants */

/* data structures */

/* procedures */

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
extern int LfsSegSlowDiskAddress _ARGS_((LfsSeg *segPtr, 
			LfsSegElement *segElementPtr));
extern LfsSegElement *LfsSegSlowAddDataBuffer _ARGS_((LfsSeg *segPtr,
			int blocks, char *bufferPtr, ClientData clientData));


extern ReturnStatus LfsSegCheckPoint _ARGS_((struct Lfs *lfsPtr, 
			char *checkPointPtr, 
			int *checkPointSizePtr));



/*
 * Stable memory routines.
 */
extern ClientData LfsLoadStableMem _ARGS_((Lfs *lfsPtr, 
			LfsStableMemParams *smemParamsPtr, 
			LfsStableMemCheckPoint *cpPtr));

extern Boolean LfsStableMemCheckpoint _ARGS_((struct LfsSeg *segPtr, 
		char *checkPointPtr,  int *checkPointSizePtr, 
		ClientData clientData));
extern void LfsStableMemWriteDone _ARGS_((struct LfsSeg *segPtr, int flags, 
		ClientData clientData));


extern Boolean LfsLoadDescMap _ARGS_((Lfs *lfsPtr, int checkPointSize,
			char *checkPointPtr));

extern Boolean LfsDescMapCheckpoint _ARGS_((LfsSeg *segPtr, char *checkPointPtr,
			int *checkPointSizePtr));

extern void LfsDescMapWriteDone _ARGS_((LfsSeg *segPtr, int flags));

extern Boolean LfsLoadUsageArray _ARGS_((Lfs *lfsPtr, int checkPointSize, 
			char *checkPointPtr));

extern Boolean LfsUsageCheckpoint _ARGS_((LfsSeg *segPtr, char *checkPointPtr,
			int *checkPointSizePtr));

extern void LfsSegUsageCheckpointUpdate _ARGS_((Lfs *lfsPtr,
		char *checkPointPtr, int size));

extern void LfsSegUsageWriteDone _ARGS_((LfsSeg *segPtr, int flags));


extern Boolean LfsFileLayoutCheckpoint _ARGS_((LfsSeg *segPtr, 
			char *checkPointPtr, int *checkPointSizePtr));

extern void LfsFileLayoutWriteDone _ARGS_((LfsSeg *segPtr, int flags));


#endif /* _LFSINT */

