/*
 * fileop.h --
 *
 *	Declarations of the interface the the file and dir mantipulation
 *	routines in files.c
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.7 91/02/09 13:24:52 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _FILEOP
#define _FILEOP

/* constants */

/* data structures */

/* procedures */

extern ReturnStatus AddEntryToDirectory _ARGS_((int dirFileNumber, 
		int dirOffset, Fslcl_DirEntry *dirEntryPtr, 
		Boolean isDirectory, Boolean mayExist));
extern ReturnStatus RemovedEntryFromDirectory _ARGS_((
		int dirFileNumber, int dirOffset, Fslcl_DirEntry *dirEntryPtr,
		Boolean isDirectory, Boolean mayBeGone));
extern ReturnStatus GrowDirectory _ARGS_((LfsFile *dirFilePtr, int dirOffset));

extern ReturnStatus CreateLostDirectory _ARGS_((int fileNumber));

enum DescOpType { OP_ABS, /* Set it to an absolute value. */
		  OP_REL, /* Set it relative to the new value. */
		};

extern ReturnStatus UpdateDescLinkCount _ARGS_((enum DescOpType opType,
			int fileNumber, int linkCount));

extern  void RecordIndirectBlockUsage _ARGS_((LfsFile  *oldFilePtr, 
	LfsFile  *newFilePtr, 
	int virtualBlockNum,  int startBlockNum, int step, 
	int lastByteBlock[2], int lastByteBlockSize[2]));

extern void RecordBlockUsageChange _ARGS_((int fileNumber, LfsFile *oldFilePtr,
					   LfsFile *newFilePtr));

extern void RecoveryFile _ARGS_((int fileNumber, int address, 
				LfsFileDescriptor *descPtr));

extern void RecordLostCreate _ARGS_((int dirFileNumber, 
			Fslcl_DirEntry *dirEntryPtr));

extern char *FileTypeToString _ARGS_((int fileType));

extern Boolean DescExists _ARGS_((int fileNumber));

extern enum LogStatus DirBlockStatus _ARGS_((int dirFileNumber, 
				int dirOffset, int startAddr, int addr));

extern enum LogStatus DescStatus _ARGS_(( int dirFileNumber,  
				int startAddr, int addr));

extern void RecovFileLayoutSummary _ARGS_((Lfs *lfsPtr, enum Pass pass, 
	LfsSeg *segPtr, int startAddr, int offset, 
	LfsSegSummaryHdr *segSummaryHdrPtr));

extern ReturnStatus MakeEmptyDirectory _ARGS_((int fileNumber, 
			int parentFileNumber));

extern void UpdateLost_Found _ARGS_((Lfs *lfsPtr));

#endif /* _FILEOP */


